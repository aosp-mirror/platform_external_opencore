/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#include "avcenc_lib.h"
#include "oscl_base_macros.h"
#include "oscl_math.h"

/* rate control variables */
#define OMEGA  0.9
#define GAMMAP 0.5
#define BETAP  0.5
#define RC_MAX_QUANT 51
#define RC_MIN_QUANT 10   //cap to 10 to prevent rate fluctuation    
#define MINVALUE 4.0
#define QP_DELTA 5   	//Define the largest variation of quantization parameters
#define QP_DELTA_I  10
#define I_SLICE_BIT_RATIO 1.0 // ratio of I-frame size over P-frame size
#define I_OVER_P_RATIO   3.8

/* local functions */
void RCUpdateRCModel(AVCEncObject *encvid);
double ComputeFrameMAD(AVCCommonObj *video, AVCRateControl *rateCtrl);
void RCModelEstimator(AVCRateControl *rateCtrl, int n_windowSize, bool *m_rgRejected);
void updateMADModel(AVCCommonObj *video, AVCRateControl *rateCtrl);
void MADModelEstimator(AVCCommonObj *video, AVCRateControl *rateCtrl, int n_windowSize,
                       double *MADPictureC1, double *MADPictureC2, bool *PictureRejected);

double QP2Qstep(int QP);
int Qstep2QP(double Qstep);


AVCEnc_Status RCDetermineFrameNum(AVCEncObject *encvid, AVCRateControl *rateCtrl, uint32 modTime, uint *frameNum)
{
    AVCCommonObj *video = encvid->common;
    AVCSliceHeader *sliceHdr = video->sliceHdr;
    uint32 modTimeRef = encvid->modTimeRef;
    int32  currFrameNum ;
    int  frameInc;


    /* check with the buffer fullness to make sure that we have enough bits to encode this frame */
    /* we can use a threshold to guarantee minimum picture quality */
    /**********************************/

    /* for now, the default is to encode every frame, To Be Changed */
    if (rateCtrl->first_frame)
    {
        encvid->modTimeRef = modTime;
        encvid->wrapModTime = 0;
        encvid->prevFrameNum = 0;
        encvid->prevProcFrameNum = 0;

        *frameNum = 0;

        /* set frame type to IDR-frame */
        video->nal_unit_type = AVC_NALTYPE_IDR;
        sliceHdr->slice_type = AVC_I_ALL_SLICE;
        video->slice_type = AVC_I_SLICE;

        return AVCENC_SUCCESS;
    }
    else
    {
        if (modTime < modTimeRef) /* modTime wrapped around */
        {
            encvid->wrapModTime += ((uint32)0xFFFFFFFF - modTimeRef) + 1;
            encvid->modTimeRef = modTimeRef = 0;
        }
        modTime += encvid->wrapModTime; /* wrapModTime is non zero after wrap-around */

        currFrameNum = (int32)(((modTime - modTimeRef) * rateCtrl->frame_rate + 200) / 1000); /* add small roundings */

        if (currFrameNum <= (int32)video->prevFrameNum || currFrameNum == (int32)encvid->prevProcFrameNum)
        {
            return AVCENC_FAIL;  /* this is a late frame do not encode it */
        }

        frameInc = currFrameNum - video->prevFrameNum;

        RCUpdateBuffer(video, rateCtrl, frameInc);	/* in case more frames dropped */

        *frameNum = currFrameNum;

        /* This part would be similar to DetermineVopType of m4venc */
        if ((*frameNum >= (uint)rateCtrl->idrPeriod && rateCtrl->idrPeriod > 0) || *frameNum > video->MaxFrameNum) /* first frame or IDR*/
        {
            /* set frame type to IDR-frame */
            if (rateCtrl->idrPeriod)
            {
                encvid->modTimeRef += (uint32)(rateCtrl->idrPeriod * 1000 / rateCtrl->frame_rate);
                *frameNum -= rateCtrl->idrPeriod;
            }
            else
            {
                encvid->modTimeRef += (uint32)(video->MaxFrameNum * 1000 / rateCtrl->frame_rate);
                *frameNum -= video->MaxFrameNum;
            }

            video->nal_unit_type = AVC_NALTYPE_IDR;
            sliceHdr->slice_type = AVC_I_ALL_SLICE;
            video->slice_type = AVC_I_SLICE;
        }
        else
        {
            video->nal_unit_type = AVC_NALTYPE_SLICE;
            sliceHdr->slice_type = AVC_P_ALL_SLICE;
            video->slice_type = AVC_P_SLICE;
        }

        encvid->prevProcFrameNum = currFrameNum;
    }

    return AVCENC_SUCCESS;
}

void RCUpdateBuffer(AVCCommonObj *video, AVCRateControl *rateCtrl, int frameInc)
{
    int tmp;

    OSCL_UNUSED_ARG(video);

    if (rateCtrl->rcEnable == TRUE)
    {
        if (frameInc > 1)
        {
            tmp = rateCtrl->bitsPerFrame * (frameInc - 1);
            rateCtrl->CurrentBufferFullness -= tmp;
            if (rateCtrl->CurrentBufferFullness < 0)
            {
                rateCtrl->CurrentBufferFullness = 0;
            }
        }
    }
}


AVCEnc_Status InitRateControlModule(AVCHandle *avcHandle)
{
    AVCEncObject *encvid = (AVCEncObject*) avcHandle->AVCObject;
    AVCCommonObj *video = encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;
    double L1, L2, L3, bpp;
    int qp;
    int i;
    double BufferSize;

    rateCtrl->MADofMB = (double*) avcHandle->CBAVC_Malloc(encvid->avcHandle->userData,
                        video->PicSizeInMbs * sizeof(double), DEFAULT_ATTR);

    if (rateCtrl->rcEnable == TRUE)
    {
        rateCtrl->basicUnit = video->PicSizeInMbs;//video->PicWidthInMbs; //  // default
        if (rateCtrl->idrPeriod == 1 && rateCtrl->basicUnit < (int)video->PicSizeInMbs) // all I's
        {
            rateCtrl->basicUnit = video->PicSizeInMbs; // only do frame-level RC for this case
        }


        rateCtrl->BUCFMAD = (double*) avcHandle->CBAVC_Malloc(encvid->avcHandle->userData,
                            video->PicHeightInMbs * sizeof(double), DEFAULT_ATTR);

        rateCtrl->BUPFMAD = (double*) avcHandle->CBAVC_Malloc(encvid->avcHandle->userData,
                            video->PicHeightInMbs * sizeof(double), DEFAULT_ATTR);

        if (!rateCtrl->MADofMB || !rateCtrl->BUCFMAD || !rateCtrl->BUPFMAD)
        {
            CleanupRateControlModule(avcHandle);
            return AVCENC_MEMORY_FAIL;
        }

        /// at the sequence level
        if (rateCtrl->basicUnit > (int)video->PicSizeInMbs)
            rateCtrl->basicUnit = video->PicSizeInMbs;
        if (rateCtrl->basicUnit < (int)video->PicSizeInMbs)
            rateCtrl->TotalNumberofBasicUnit = video->PicSizeInMbs / rateCtrl->basicUnit;

        /*initialize the parameters of fluid flow traffic model*/

        rateCtrl->bitsPerFrame = (int32)(rateCtrl->bitRate / rateCtrl->frame_rate);

        BufferSize = rateCtrl->cpbSize;
        //BufferSize = rateCtrl->bitRate * 2.56; // for testing
        rateCtrl->CurrentBufferFullness = 0;

        /*HRD consideration*/
        rateCtrl->initDelayOffset = (int32)(BufferSize * 0.8); // for testing

        /*initialize the previous window size*/
        rateCtrl->m_windowSize = 0;
        rateCtrl->MADm_windowSize = 0;
        rateCtrl->NumberofCodedPFrame = 0;
        rateCtrl->NumberofGOP = 0;
        /*remaining # of bits in GOP */
        rateCtrl->R = 0;

        /*quadratic rate-distortion model*/
        rateCtrl->PPreHeader = 0;

        rateCtrl->m_X1 = rateCtrl->bitRate * 1.0;
        rateCtrl->m_X2 = 0.0;
        /* linear prediction model for P picture*/
        rateCtrl->PMADPictureC1 = 1.0;
        rateCtrl->PMADPictureC2 = 0.0;

        for (i = 0;i < 20;i++)
        {
            rateCtrl->m_rgQp[i] = 0;
            rateCtrl->m_rgRp[i] = 0.0;
            rateCtrl->PPictureMAD[i] = 0.0;
        }
        rateCtrl->PPictureMAD[20] = 0.0;

        /*basic unit layer rate control*/
        rateCtrl->PAveHeaderBits1 = 0;
        rateCtrl->PAveHeaderBits3 = 0;
        if (rateCtrl->TotalNumberofBasicUnit >= 9)
            rateCtrl->DDquant = 1;
        else
            rateCtrl->DDquant = 2;

        /*compute the initial QP*/
        bpp = 1.0 * rateCtrl->bitRate / (rateCtrl->frame_rate * (video->PicSizeInMbs << 8));
        if (video->PicWidthInSamplesL == 176)
        {
            L1 = 0.1;
            L2 = 0.3;
            L3 = 0.6;
        }
        else if (video->PicWidthInSamplesL == 352)
        {
            L1 = 0.2;
            L2 = 0.6;
            L3 = 1.2;
        }
        else
        {
            L1 = 0.6;
            L2 = 1.4;
            L3 = 2.4;
        }

        if (rateCtrl->initQP == 0)
        {
            if (bpp <= L1)
                qp = 35;
            else
                if (bpp <= L2)
                    qp = 25;
                else
                    if (bpp <= L3)
                        qp = 20;
                    else
                        qp = 15;
            rateCtrl->initQP = qp;
        }

        rateCtrl->PAveFrameQP = rateCtrl->initQP;
    }

    return AVCENC_SUCCESS;
}


void CleanupRateControlModule(AVCHandle *avcHandle)
{
    AVCEncObject *encvid = (AVCEncObject*) avcHandle->AVCObject;
    AVCRateControl *rateCtrl = encvid->rateCtrl;

    if (rateCtrl->MADofMB)
    {
        avcHandle->CBAVC_Free(avcHandle->userData, (int)(rateCtrl->MADofMB));
    }
    if (rateCtrl->BUCFMAD)
    {
        avcHandle->CBAVC_Free(avcHandle->userData, (int)(rateCtrl->BUCFMAD));
    }
    if (rateCtrl->BUPFMAD)
    {
        avcHandle->CBAVC_Free(avcHandle->userData, (int)(rateCtrl->BUPFMAD));
    }

    return ;
}

void RCInitGOP(AVCEncObject *encvid)
{
    AVCCommonObj *video = encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;

    int np = rateCtrl->idrPeriod - 1;
    bool Overum = FALSE;
    int OverBits;
    int OverDuantQp;
    int AllocatedBits;
    int GOPDquant;
    int PAverageQp;
    OsclFloat denom, numer;
    bool initGOP = TRUE;

    if (rateCtrl->rcEnable == TRUE)
    {

        /*** additional part from JM12.4 RC3 ****/
        if (np < 0)
        {
            np = video->MaxFrameNum - 1;   // IPPPPPP
        }
        else if (np == 0)
        {
            np = (1 << 16) - 1; // IIIIIII
            if (rateCtrl->first_frame == FALSE)
            {
                initGOP = FALSE;
            }
        }

        if (initGOP == TRUE)
        {
            numer = (OsclFloat)((np + 1) * rateCtrl->bitsPerFrame);
            denom = (OsclFloat)((np + 1) + I_SLICE_BIT_RATIO - 1);

            rateCtrl->RCPSliceBits = (int)(numer / denom + 0.5);
            rateCtrl->RCISliceBits = (np == 0) ? (int)(I_SLICE_BIT_RATIO * rateCtrl->RCPSliceBits + 0.5) : 0;

            /* check if the last GOP over uses its budget. If yes, the initial QP of the I frame in
            the coming  GOP will be increased.*/

            if (rateCtrl->R < 0)
                Overum = TRUE;
            OverBits = -rateCtrl->R;

            /*initialize the lower bound and the upper bound for the target bits of each frame, HRD consideration*/
            rateCtrl->LowerBound = (int32)(rateCtrl->R + rateCtrl->bitsPerFrame);
            rateCtrl->UpperBound1 = (int32)(rateCtrl->R + rateCtrl->initDelayOffset);

            /*compute the total number of bits for the current GOP*/
            AllocatedBits = (int)((1 + np) * rateCtrl->bitsPerFrame);
            rateCtrl->R += AllocatedBits;
            rateCtrl->Np  = np;

            OverDuantQp = (int)(8 * OverBits / AllocatedBits + 0.5);
            rateCtrl->GOPOverdue = FALSE;

            /*Compute InitialQp for each GOP*/
            rateCtrl->NumberofGOP++;
            if (rateCtrl->NumberofGOP == 1)
            {
                //rateCtrl->QPLastGOP = rateCtrl->initQP ; this causes too much quality fluctuation in I-frame
            }
            else
            {
                /*compute the average QP of P frames in the previous GOP*/
                PAverageQp = (int)(1.0 * rateCtrl->TotalQpforPPicture / rateCtrl->NumberofPPicture + 0.5);

                GOPDquant = (int)(0.5 + 1.0 * (np + 1) / 15);
                if (GOPDquant > 2)
                    GOPDquant = 2;

                PAverageQp -= GOPDquant;

                if (rateCtrl->basicUnit < (int)video->PicSizeInMbs)
                {
                    if (PAverageQp > (int)(rateCtrl->QPLastPFrame - 2))
                        PAverageQp--;
                    // this clipping causes too much quality fluctuation in I-frame
                    //PAverageQp = AVC_MAX(rateCtrl->QPLastGOP-2,  PAverageQp);
                    //PAverageQp = AVC_MIN(rateCtrl->QPLastGOP+2, PAverageQp);
                    PAverageQp = AVC_MIN(RC_MAX_QUANT, PAverageQp);
                    PAverageQp = AVC_MAX(RC_MIN_QUANT, PAverageQp);
                }

                //rateCtrl->QPLastGOP =
                rateCtrl->initQP = PAverageQp;
                rateCtrl->Pm_Qp = PAverageQp;
                rateCtrl->PAveFrameQP = PAverageQp;
            }

            rateCtrl->TotalQpforPPicture = 0;
            rateCtrl->NumberofPPicture = 0;
        }
    }

    return ;
}

void RCInitFrameQP(AVCEncObject *encvid)
{
    AVCCommonObj *video = encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;
    AVCPicParamSet *picParam = video->currPicParams;
    int i;
    int32 T = 0, T1 = 0;
    int32 UpperBound;
    int np = rateCtrl->idrPeriod - 1; // TotalPFrame
    int bits;

    if (rateCtrl->rcEnable == TRUE)
    {

        if (np < 0) np = video->MaxFrameNum - 1; // IPPPPP case
        else if (np == 0) np = (1 << 16) - 1;

        rateCtrl->NumberofCodedMacroBlocks = 0;

        if ((video->slice_type == AVC_P_SLICE) || (rateCtrl->idrPeriod == 1 && rateCtrl->first_frame == FALSE))
        {
            /* predefine the  target buffer level for each picture.
               frame layer rate control */
            if (rateCtrl->basicUnit == (int)video->PicSizeInMbs)
            {
                if (rateCtrl->NumberofPPicture == 1)
                {
                    rateCtrl->TargetBufferLevel = rateCtrl->CurrentBufferFullness;
                    rateCtrl->DeltaP = rateCtrl->CurrentBufferFullness / (np - 1);
                    rateCtrl->TargetBufferLevel -= (int)rateCtrl->DeltaP;
                }
                else if (rateCtrl->NumberofPPicture > 1)
                    rateCtrl->TargetBufferLevel -= (int)rateCtrl->DeltaP;
            }
            /* basic unit layer rate control */
            else
            {
                if (rateCtrl->NumberofCodedPFrame > 0)
                {
                    for (i = 0;i < rateCtrl->TotalNumberofBasicUnit;i++)
                        rateCtrl->BUPFMAD[i] = rateCtrl->BUCFMAD[i];
                }

                if (rateCtrl->NumberofGOP == 1)
                {
                    if (rateCtrl->NumberofPPicture == 1)
                    {
                        rateCtrl->TargetBufferLevel = rateCtrl->CurrentBufferFullness;
                        rateCtrl->DeltaP = rateCtrl->CurrentBufferFullness / (np - 1);
                        rateCtrl->TargetBufferLevel -= (int)rateCtrl->DeltaP;
                    }
                    else if (rateCtrl->NumberofPPicture > 1)
                        rateCtrl->TargetBufferLevel -= (int)rateCtrl->DeltaP;
                }
                else if (rateCtrl->NumberofGOP > 1)
                {
                    if (rateCtrl->NumberofPPicture == 0)
                    {
                        rateCtrl->TargetBufferLevel = rateCtrl->CurrentBufferFullness;
                        rateCtrl->DeltaP = rateCtrl->CurrentBufferFullness / np;
                        rateCtrl->TargetBufferLevel -= (int)rateCtrl->DeltaP;
                    }
                    else if (rateCtrl->NumberofPPicture > 0)
                        rateCtrl->TargetBufferLevel -= (int)rateCtrl->DeltaP;
                }
            }
        }

        /* additional code from RC3 */
        if (video->slice_type == AVC_P_SLICE || rateCtrl->first_frame == 0)
        {
            if (rateCtrl->NumberofCodedPFrame > 0)
            {
                if (rateCtrl->idrPeriod == 1) // all I frames
                {
                    T = rateCtrl->R / (rateCtrl->Np + 1);  // ratio
                    T1 = (int32)(rateCtrl->bitsPerFrame - GAMMAP * (rateCtrl->CurrentBufferFullness - rateCtrl->TargetBufferLevel) + 0.5);
                    T1 = AVC_MAX(0, T1);
                    T = (int32)(BETAP * T + (1.0 - BETAP) * T1 + 0.5);
                }
                else
                {
                    bits = (video->slice_type == AVC_P_SLICE) ? rateCtrl->RCPSliceBits : rateCtrl->RCISliceBits;

                    // this part deviates from the JM code */
                    T = (bits * rateCtrl->R) / (rateCtrl->bitsPerFrame * (rateCtrl->Np + 1));  // ratio

                    if (video->slice_type == AVC_I_SLICE)
                    {
                        T = (int32)(T / (I_OVER_P_RATIO * 4));
                    }
                    else
                    {
                        T1 = (int32)(rateCtrl->bitsPerFrame - GAMMAP * (rateCtrl->CurrentBufferFullness - rateCtrl->TargetBufferLevel) + 0.5);
                        T1 = AVC_MAX(0, T1);
                        T = (int32)(BETAP * T + (1.0 - BETAP) * T1 + 0.5);
                    }
                }
            }

            /* reserve some bits for smoothing, commented out  */
            /* HRD consideration */
            if (video->slice_type == AVC_P_SLICE || rateCtrl->idrPeriod == 1)
            {
                UpperBound = (int32)(OMEGA * rateCtrl->UpperBound1);
                T = AVC_MAX(T, rateCtrl->LowerBound);
                rateCtrl->T = AVC_MIN(T, UpperBound);
            }
            else
            {
                rateCtrl->T = T;
            }
        }

        /* frame layer rate control */
        rateCtrl->NumberofHeaderBits = 0;
        rateCtrl->NumberofTextureBits = 0;

        /* basic unit layer rate control */
        if (rateCtrl->basicUnit < (int)video->PicSizeInMbs)
        {
            rateCtrl->TotalFrameQP = 0;
            rateCtrl->NumberofBasicUnitHeaderBits = 0;
            rateCtrl->NumberofBasicUnitTextureBits = 0;
            rateCtrl->TotalMADBasicUnit = 0;
            rateCtrl->NumberofBasicUnit = rateCtrl->TotalNumberofBasicUnit;
        }
        video->QPy = rateCtrl->m_Qc = rateCtrl->PAveFrameQP = RCCalculateQP(encvid, rateCtrl);

        rateCtrl->numFrameBits = 0; // reset

    } // rcEnable
    else
    {
        video->QPy = rateCtrl->initQP;
    }

//	printf(" %d ",video->QPy);

    if (video->CurrPicNum == 0 && encvid->outOfBandParamSet == FALSE)
    {
        picParam->pic_init_qs_minus26 = 0;
        picParam->pic_init_qp_minus26 = video->QPy - 26;
    }

    // need this for motion estimation
    encvid->lambda_mode = QP2QUANT[AVC_MAX(0,video->QPy-SHIFT_QP)];
    encvid->lambda_motion = LAMBDA_FACTOR(encvid->lambda_mode);
    return ;
}



int RCCalculateQP(AVCEncObject *encvid, AVCRateControl *rateCtrl)
{
    AVCCommonObj *video = encvid->common;
    double dtmp;
    int m_Bits;
    int PAverageQP;
    int SumofBasicUnit;
    int i;
    double sqrt_dtmp;
    double PreviousPictureMAD;
    double MADPictureC1;
    double MADPictureC2;
    double m_X1 = rateCtrl->m_X1;
    double m_X2 = rateCtrl->m_X2;
    int m_Qc ;
    double m_Qstep;
    int m_Qp;
    double CurrentBUMAD;
    double TotalBUMAD;
    int m_Hp;
    int TotalBasicUnitBits;
    int np = rateCtrl->idrPeriod - 1;

    if (np < 0) np = video->MaxFrameNum - 1; // IPPPPP case
    else if (np == 0) np = (1 << 16) - 1; // IIIIII case

    int Qstep2QP(double Qstep);

    /* frame layer rate control */
    if (rateCtrl->basicUnit == (int)video->PicSizeInMbs || (video->slice_type != AVC_P_SLICE && rateCtrl->idrPeriod == 1)) // I_slice for basic unit comes here too
    {
        if (rateCtrl->first_frame) //(video->slice_type==AVC_I_SLICE)
        {
            m_Qc = rateCtrl->initQP;
            rateCtrl->Pm_Qp = m_Qc;
            return m_Qc;
        }
        else if ((rateCtrl->first_frame == FALSE) && (rateCtrl->NumberofPPicture == 0)) /* first P-frame */
        {
            m_Qc = rateCtrl->initQP;
            //if(active_sps->frame_mbs_only_flag)
            rateCtrl->TotalQpforPPicture += m_Qc;
            rateCtrl->Pm_Qp = m_Qc;
            return m_Qc;
        }
        else /* subsequent frames */
        {
            MADPictureC1 = rateCtrl->PMADPictureC1;
            MADPictureC2 = rateCtrl->PMADPictureC2;
            PreviousPictureMAD = rateCtrl->PPictureMAD[0];
            m_Qp = rateCtrl->Pm_Qp;
            m_Hp = rateCtrl->PPreHeader;

            if (rateCtrl->basicUnit < (int)video->PicSizeInMbs && video->slice_type != AVC_P_SLICE)
            {
                PreviousPictureMAD = rateCtrl->PreviousWholeFrameMAD;
            }

            if (video->slice_type == AVC_I_SLICE)
            {
                m_Hp = 0;
            }

            /* predict the MAD of current picture*/
            rateCtrl->CurrentFrameMAD = MADPictureC1 * PreviousPictureMAD + MADPictureC2;

            /*compute the number of bits for the texture*/
            if (rateCtrl->T < 0) /* if target bits is already below zero, just increase Qp */
            {
                m_Qc = m_Qp + QP_DELTA;
                m_Qc = AVC_MIN(m_Qc, RC_MAX_QUANT); // clipping
            }
            else /* get QP from R-Q model */
            {
                if (video->slice_type != AVC_P_SLICE && rateCtrl->idrPeriod != 1)
                {
                    if (rateCtrl->basicUnit < (int)video->PicSizeInMbs)
                    {
                        m_Bits = (rateCtrl->T - m_Hp) / rateCtrl->TotalNumberofBasicUnit;
                    }
                    else
                    {
                        m_Bits = rateCtrl->T - m_Hp;
                    }
                }
                else
                {
                    m_Bits = rateCtrl->T - m_Hp;
                    m_Bits = AVC_MAX(m_Bits, (int)(rateCtrl->bitsPerFrame / MINVALUE));
                }

                dtmp = rateCtrl->CurrentFrameMAD * m_X1 * rateCtrl->CurrentFrameMAD * m_X1 \
                       + 4 * m_X2 * rateCtrl->CurrentFrameMAD * m_Bits;

                if (dtmp > 0) sqrt_dtmp = oscl_sqrt(dtmp);
                else sqrt_dtmp = 0.0;

                if ((m_X2 == 0.0) || (dtmp < 0) || ((sqrt_dtmp - m_X1 * rateCtrl->CurrentFrameMAD) <= 0.0)) // fall back 1st order mode
                    m_Qstep = (float)(m_X1 * rateCtrl->CurrentFrameMAD / (double) m_Bits);
                else // 2nd order mode
                    m_Qstep = (float)((2 * m_X2 * rateCtrl->CurrentFrameMAD) / (sqrt_dtmp - m_X1 * rateCtrl->CurrentFrameMAD));

                m_Qc = Qstep2QP(m_Qstep);
                m_Qc = AVC_MIN(m_Qc, RC_MAX_QUANT); // clipping
                m_Qc = AVC_MAX(RC_MIN_QUANT, m_Qc);

                if (video->slice_type == AVC_P_SLICE)
                {
                    m_Qc = AVC_MIN(m_Qp + QP_DELTA,  m_Qc);  // control variation
                    m_Qc = AVC_MAX(m_Qp - QP_DELTA, m_Qc); // control variation
                }
                else
                {
                    m_Qc = AVC_MIN(m_Qp + QP_DELTA_I,  m_Qc);  // control variation
                    m_Qc = AVC_MAX(m_Qp - QP_DELTA_I, m_Qc); // control variation
                }

            }
            rateCtrl->TotalQpforPPicture += m_Qc;
            rateCtrl->Pm_Qp = m_Qc;

            return m_Qc;
        }
    }
    /**********basic unit layer rate control*********/
    else
    {
        if (rateCtrl->first_frame) //(video->slice_type==AVC_I_SLICE)
        {
            m_Qc = rateCtrl->initQP;
            rateCtrl->Pm_Qp = m_Qc;
            return m_Qc;
        }
        else //if(video->slice_type==AVC_P_SLICE)
        {
            if ((rateCtrl->NumberofGOP == 1) && (rateCtrl->NumberofPPicture == 0))
            {
                /*top field of the first P frame*/
                m_Qc = rateCtrl->initQP;
                rateCtrl->NumberofBasicUnitHeaderBits = 0;
                rateCtrl->NumberofBasicUnitTextureBits = 0;
                rateCtrl->NumberofBasicUnit--;
                /*bottom field of the first P frame*/
                if (rateCtrl->NumberofBasicUnit == 0)
                {
                    rateCtrl->TotalQpforPPicture += m_Qc;
                    rateCtrl->PAveFrameQP = m_Qc;
                    rateCtrl->PAveHeaderBits3 = rateCtrl->PAveHeaderBits2;
                }
                rateCtrl->Pm_Qp = m_Qc;
                rateCtrl->TotalFrameQP += m_Qc;
                return m_Qc;
            }
            else /* subsequent P-frames */
            {
                m_Hp = rateCtrl->PPreHeader;
                m_Qp = rateCtrl->Pm_Qp;
                MADPictureC1 = rateCtrl->PMADPictureC1;
                MADPictureC2 = rateCtrl->PMADPictureC2;

                SumofBasicUnit = rateCtrl->TotalNumberofBasicUnit;
                /*the average QP of the previous frame is used to coded the first basic unit of the current frame or field*/
                if (rateCtrl->NumberofBasicUnit == SumofBasicUnit)
                {
                    if (rateCtrl->T <= 0)
                    {
                        m_Qc = rateCtrl->PAveFrameQP + 2;
                        if (m_Qc > RC_MAX_QUANT)
                            m_Qc = RC_MAX_QUANT;
                        rateCtrl->GOPOverdue = TRUE;
                    }
                    else
                    {
                        m_Qc = rateCtrl->PAveFrameQP;
                    }
                    rateCtrl->TotalFrameQP += m_Qc;
                    rateCtrl->NumberofBasicUnit--;
                    rateCtrl->Pm_Qp = rateCtrl->PAveFrameQP;
                    return m_Qc;
                }
                else
                {
                    /*compute the number of remaining bits*/
                    TotalBasicUnitBits = rateCtrl->NumberofBasicUnitHeaderBits + rateCtrl->NumberofBasicUnitTextureBits;
                    rateCtrl->T -= TotalBasicUnitBits;
                    rateCtrl->NumberofBasicUnitHeaderBits = 0;
                    rateCtrl->NumberofBasicUnitTextureBits = 0;
                    if (rateCtrl->T < 0)
                    {
                        if (rateCtrl->GOPOverdue == TRUE)
                            m_Qc = m_Qp + 2;
                        else
                            m_Qc = m_Qp + rateCtrl->DDquant;//2
                        m_Qc = AVC_MIN(m_Qc, RC_MAX_QUANT);  // clipping
                        if (rateCtrl->basicUnit >= (int)video->PicWidthInMbs)
                            m_Qc = AVC_MIN(m_Qc, rateCtrl->PAveFrameQP + 6);
                        else
                            m_Qc = AVC_MIN(m_Qc, rateCtrl->PAveFrameQP + 3);

                        rateCtrl->TotalFrameQP += m_Qc;
                        rateCtrl->NumberofBasicUnit--;
                        if (rateCtrl->NumberofBasicUnit == 0)
                        {
                            PAverageQP = (int)(1.0 * rateCtrl->TotalFrameQP / rateCtrl->TotalNumberofBasicUnit + 0.5);
                            if (rateCtrl->NumberofPPicture == (np - 1))
                                rateCtrl->QPLastPFrame = PAverageQP;

                            rateCtrl->TotalQpforPPicture += PAverageQP;
                            rateCtrl->PAveFrameQP = PAverageQP;
                            rateCtrl->PAveHeaderBits3 = rateCtrl->PAveHeaderBits2;
                        }
                        if (rateCtrl->GOPOverdue == TRUE)
                            rateCtrl->Pm_Qp = rateCtrl->PAveFrameQP;
                        else
                            rateCtrl->Pm_Qp = m_Qc;
                        return m_Qc;
                    }
                    else
                    {
                        /*predict the MAD of current picture*/
                        rateCtrl->CurrentFrameMAD = MADPictureC1 * rateCtrl->BUPFMAD[rateCtrl->TotalNumberofBasicUnit-rateCtrl->NumberofBasicUnit] + MADPictureC2;
                        TotalBUMAD = 0;
                        for (i = rateCtrl->TotalNumberofBasicUnit - 1; i >= (rateCtrl->TotalNumberofBasicUnit - rateCtrl->NumberofBasicUnit);i--)
                        {
                            CurrentBUMAD = MADPictureC1 * rateCtrl->BUPFMAD[i] + MADPictureC2;
                            TotalBUMAD += CurrentBUMAD * CurrentBUMAD;
                        }

                        /*compute the total number of bits for the current basic unit*/
                        m_Bits = (int)(rateCtrl->T * rateCtrl->CurrentFrameMAD * rateCtrl->CurrentFrameMAD / TotalBUMAD);
                        /*compute the number of texture bits*/
                        m_Bits -= rateCtrl->PAveHeaderBits2;

                        m_Bits = AVC_MAX(m_Bits, (int)(rateCtrl->bitsPerFrame / (MINVALUE * rateCtrl->TotalNumberofBasicUnit)));

                        dtmp = rateCtrl->CurrentFrameMAD * m_X1 * rateCtrl->CurrentFrameMAD * m_X1 \
                               + 4 * m_X2 * rateCtrl->CurrentFrameMAD * m_Bits;
                        if ((m_X2 == 0.0) || (dtmp < 0) || ((oscl_sqrt(dtmp) - m_X1 * rateCtrl->CurrentFrameMAD) <= 0.0))   // fall back 1st order mode
                            m_Qstep = (float)(m_X1 * rateCtrl->CurrentFrameMAD / (double) m_Bits);
                        else // 2nd order mode
                            m_Qstep = (float)((2 * m_X2 * rateCtrl->CurrentFrameMAD) / (oscl_sqrt(dtmp) - m_X1 * rateCtrl->CurrentFrameMAD));

                        m_Qc = Qstep2QP(m_Qstep);
                        m_Qc = AVC_MIN(m_Qp + rateCtrl->DDquant,  m_Qc); // control variation
                        if (rateCtrl->basicUnit >= (int)video->PicWidthInMbs)
                            m_Qc = AVC_MIN(rateCtrl->PAveFrameQP + 6, m_Qc);
                        else
                            m_Qc = AVC_MIN(rateCtrl->PAveFrameQP + 3, m_Qc);

                        m_Qc = AVC_MIN(m_Qc, RC_MAX_QUANT);  // clipping

                        m_Qc = AVC_MAX(m_Qp - rateCtrl->DDquant, m_Qc);  // control variation
                        if (rateCtrl->basicUnit >= (int)video->PicWidthInMbs)
                            m_Qc = AVC_MAX(rateCtrl->PAveFrameQP - 6, m_Qc);
                        else
                            m_Qc = AVC_MAX(rateCtrl->PAveFrameQP - 3, m_Qc);

                        m_Qc = AVC_MAX(RC_MIN_QUANT, m_Qc);

                        rateCtrl->TotalFrameQP += m_Qc;
                        rateCtrl->Pm_Qp = m_Qc;
                        rateCtrl->NumberofBasicUnit--;
                        if ((rateCtrl->NumberofBasicUnit == 0) && (video->slice_type == AVC_P_SLICE))
                        {
                            /*frame coding or field coding*/
                            PAverageQP = (int)(1.0 * rateCtrl->TotalFrameQP / rateCtrl->TotalNumberofBasicUnit + 0.5);
                            if (rateCtrl->NumberofPPicture == (np - 1))
                                rateCtrl->QPLastPFrame = PAverageQP;

                            rateCtrl->TotalQpforPPicture += PAverageQP;
                            rateCtrl->PAveFrameQP = PAverageQP;
                            rateCtrl->PAveHeaderBits3 = rateCtrl->PAveHeaderBits2;
                        }
                        return m_Qc;
                    }
                }
            } /* subsequent P-frames */
        } /* AVC_P_SLICE */
    }
    return m_Qc;
}


void RCInitChromaQP(AVCEncObject *encvid)
{
    AVCCommonObj *video = encvid->common;
    AVCMacroblock *currMB = video->currMB;
    int q_bits;

    /* we have to do the same thing for AVC_CLIP3(0,51,video->QSy) */

    video->QPy_div_6 = (currMB->QPy * 43) >> 8;
    video->QPy_mod_6 = currMB->QPy - 6 * video->QPy_div_6;
    currMB->QPc = video->QPc = mapQPi2QPc[AVC_CLIP3(0,51,currMB->QPy + video->currPicParams->chroma_qp_index_offset)];
    video->QPc_div_6 = (video->QPc * 43) >> 8;
    video->QPc_mod_6 = video->QPc - 6 * video->QPc_div_6;

    /* pre-calculate this to save computation */
    q_bits = 4 + video->QPy_div_6;
    if (video->slice_type == AVC_I_SLICE)
    {
        encvid->qp_const = 682 << q_bits;		// intra
    }
    else
    {
        encvid->qp_const = 342 << q_bits;		// inter
    }

    q_bits = 4 + video->QPc_div_6;
    if (video->slice_type == AVC_I_SLICE)
    {
        encvid->qp_const_c = 682 << q_bits;    // intra
    }
    else
    {
        encvid->qp_const_c = 342 << q_bits;    // inter
    }

    encvid->lambda_mode = QP2QUANT[AVC_MAX(0,currMB->QPy-SHIFT_QP)];
    encvid->lambda_motion = LAMBDA_FACTOR(encvid->lambda_mode);

    return ;
}


void RCInitMBQP(AVCEncObject *encvid)
{
    AVCCommonObj *video =  encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;
    AVCMacroblock *currMB = video->currMB;

    currMB->QPy = video->QPy; /* set to previous value or picture level */

    if (rateCtrl->rcEnable == TRUE)
    {
        if (((video->slice_type == AVC_P_SLICE) || (rateCtrl->idrPeriod == 1 && rateCtrl->first_frame == FALSE)) && (rateCtrl->NumberofCodedMacroBlocks > 0)
                && (rateCtrl->NumberofCodedMacroBlocks % rateCtrl->basicUnit == 0))
        {

            RCUpdateRCModel(encvid);
            currMB->QPy = rateCtrl->m_Qc = RCCalculateQP(encvid, rateCtrl);
        }
    }

    RCInitChromaQP(encvid);

}

void RCCalculateMAD(AVCEncObject *encvid, AVCMacroblock *currMB, uint8 *orgL, int orgPitch)
{
    AVCCommonObj *video = encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;
    uint32 dmin_lx;

    if (rateCtrl->rcEnable == TRUE)
    {
        if (currMB->mb_intra)
        {
            if (currMB->mbMode == AVC_I16)
            {
                dmin_lx = (0xFFFF << 16) | orgPitch;
                rateCtrl->MADofMB[video->mbNum] = AVCSAD_Macroblock_C(orgL,
                                                  encvid->pred_i16[currMB->i16Mode], dmin_lx, NULL);
            }
            else /* i4 */
            {
                rateCtrl->MADofMB[video->mbNum] = encvid->i4_sad / 256.;
            }
        }
        /* for INTER, we have already saved it with the MV search */

        if (rateCtrl->basicUnit < (int)video->PicSizeInMbs)
        {
            rateCtrl->TotalMADBasicUnit += rateCtrl->MADofMB[video->mbNum];
        }
    }

    return ;
}

void RCRestoreQP(AVCMacroblock *currMB, AVCCommonObj *video, AVCEncObject *encvid)
{
    currMB->QPy = video->QPy; /* use previous QP */
    RCInitChromaQP(encvid);

    return ;
}

AVCEnc_Status RCUpdateFrame(AVCEncObject *encvid)
{
    AVCCommonObj *video = encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;
    AVCEnc_Status status = AVCENC_SUCCESS;

    int nbits = rateCtrl->numFrameBits;
    int32 tmp;
    /* update the complexity weight of I, P, B frame */
    int Avem_Qc;
    int X = 0;

    if (rateCtrl->rcEnable == TRUE)
    {
        /* frame layer rate control */
        if (rateCtrl->basicUnit == (int)video->PicSizeInMbs)
            X = (int)(nbits * rateCtrl->m_Qc + 0.5);
        /* basic unit layer rate control */
        else
        {
            if (video->slice_type == AVC_P_SLICE)
            {
                Avem_Qc = rateCtrl->TotalFrameQP / rateCtrl->TotalNumberofBasicUnit;
                X = (int)(nbits * Avem_Qc + 0.5);
            }
        }

        if (video->slice_type == AVC_P_SLICE || (rateCtrl->idrPeriod == 1))
        {
            rateCtrl->Np--;
            rateCtrl->Wp = X;
            rateCtrl->NumberofCodedPFrame++;
            rateCtrl->NumberofPPicture++;

        }

        tmp = (int32)(rateCtrl->bitsPerFrame - nbits);
        rateCtrl->CurrentBufferFullness -= tmp;
#if 1 // this part may not be necessary
        if (rateCtrl->CurrentBufferFullness > rateCtrl->cpbSize*0.8 && video->slice_type != AVC_I_SLICE) /* skip current frame */
        {
            rateCtrl->curr_skip = 1;
            rateCtrl->CurrentBufferFullness += tmp;
            status = AVCENC_SKIPPED_PICTURE;
        }
        else
        {
            rateCtrl->curr_skip = 0;
            /*update the lower bound and the upper bound for the target bits of each frame, HRD consideration*/
            rateCtrl->R -= nbits; /* remaining # of bits in GOP */
            rateCtrl->LowerBound  += tmp;
            rateCtrl->UpperBound1 += tmp;
        }
#endif

        if (video->slice_type == AVC_P_SLICE || (rateCtrl->idrPeriod == 1/* && !rateCtrl->first_frame*/))
        {
            RCUpdateRCModel(encvid);

            rateCtrl->PreviousWholeFrameMAD = ComputeFrameMAD(video, rateCtrl);
        }
    }

    rateCtrl->first_frame = 0;  // reset here after we encode the first frame.

    return status;
}

void RCUpdateRCModel(AVCEncObject *encvid)
{
    AVCCommonObj *video = encvid->common;
    AVCRateControl *rateCtrl = encvid->rateCtrl;
    int		n_windowSize;
    int		i;
    double	error[20], std = 0.0, threshold;
    int		m_Nc;
    bool	MADModelFlag = FALSE;
    bool	m_rgRejected[21];
    int		CodedBasicUnit;

    /*frame layer rate control*/
    if (rateCtrl->basicUnit == (int)video->PicSizeInMbs)
    {
        rateCtrl->CurrentFrameMAD = ComputeFrameMAD(video, rateCtrl);
        m_Nc = rateCtrl->NumberofCodedPFrame;
    }
    /*basic unit layer rate control*/
    else
    {
        /*compute the MAD of the current basic unit*/
        rateCtrl->CurrentFrameMAD = rateCtrl->TotalMADBasicUnit / rateCtrl->basicUnit;


        rateCtrl->TotalMADBasicUnit = 0;

        /* compute the average number of header bits*/

        CodedBasicUnit = rateCtrl->TotalNumberofBasicUnit - rateCtrl->NumberofBasicUnit;
        if (CodedBasicUnit > 0)
        {
            rateCtrl->PAveHeaderBits1 = (int)(1.0 * (rateCtrl->PAveHeaderBits1 * (CodedBasicUnit - 1) + \
                                              + rateCtrl->NumberofBasicUnitHeaderBits) / CodedBasicUnit + 0.5);
            if (rateCtrl->PAveHeaderBits3 == 0)
                rateCtrl->PAveHeaderBits2 = rateCtrl->PAveHeaderBits1;
            else
                rateCtrl->PAveHeaderBits2 = (int)(1.0 * (rateCtrl->PAveHeaderBits1 * CodedBasicUnit + \
                                                  + rateCtrl->PAveHeaderBits3 * rateCtrl->NumberofBasicUnit) / rateCtrl->TotalNumberofBasicUnit + 0.5);
        }
        /*update the record of MADs for reference*/
        rateCtrl->BUCFMAD[rateCtrl->TotalNumberofBasicUnit-1-rateCtrl->NumberofBasicUnit] = rateCtrl->CurrentFrameMAD;

        if (rateCtrl->NumberofBasicUnit != 0)
            m_Nc = rateCtrl->NumberofCodedPFrame * rateCtrl->TotalNumberofBasicUnit + CodedBasicUnit;
        else
            m_Nc = (rateCtrl->NumberofCodedPFrame - 1) * rateCtrl->TotalNumberofBasicUnit + CodedBasicUnit;

    }

    if (m_Nc > 1)
        MADModelFlag = TRUE;

    rateCtrl->PPreHeader = rateCtrl->NumberofHeaderBits;
    for (i = 19; i > 0; i--)
    {// update the history
        rateCtrl->m_rgQp[i] = rateCtrl->m_rgQp[i - 1];
        rateCtrl->m_rgRp[i] = rateCtrl->m_rgRp[i - 1];
    }
    rateCtrl->m_rgQp[0] = QP2Qstep(rateCtrl->m_Qc); //*1.0/rateCtrl->CurrentFrameMAD;
    /*frame layer rate control*/
    if (rateCtrl->basicUnit == (int)video->PicSizeInMbs)
        rateCtrl->m_rgRp[0] = rateCtrl->NumberofTextureBits * 1.0 / rateCtrl->CurrentFrameMAD;
    /*basic unit layer rate control*/
    else
        rateCtrl->m_rgRp[0] = rateCtrl->NumberofBasicUnitTextureBits * 1.0 / rateCtrl->CurrentFrameMAD;


    /*compute the size of window*/
    n_windowSize = (rateCtrl->CurrentFrameMAD > rateCtrl->PreviousFrameMAD) ? (int)(rateCtrl->PreviousFrameMAD / rateCtrl->CurrentFrameMAD * 20)\
                   : (int)(rateCtrl->CurrentFrameMAD / rateCtrl->PreviousFrameMAD * 20);
    n_windowSize = AVC_MAX(n_windowSize, 1);
    n_windowSize = AVC_MIN(n_windowSize, m_Nc);
    n_windowSize = AVC_MIN(n_windowSize, rateCtrl->m_windowSize + 1);
    n_windowSize = AVC_MIN(n_windowSize, 20);

    /*update the previous window size*/
    rateCtrl->m_windowSize = n_windowSize;

    for (i = 0; i < 20; i++)
    {
        m_rgRejected[i] = FALSE;
    }

    // initial RD model estimator
    RCModelEstimator(rateCtrl, n_windowSize, m_rgRejected);

    n_windowSize = rateCtrl->m_windowSize;
    // remove outlier

    for (i = 0; i < (int) n_windowSize; i++)
    {
        error[i] = rateCtrl->m_X1 / rateCtrl->m_rgQp[i] + rateCtrl->m_X2 / (rateCtrl->m_rgQp[i] * rateCtrl->m_rgQp[i]) - rateCtrl->m_rgRp[i];
        std += error[i] * error[i];
    }
    threshold = (n_windowSize == 2) ? 0 : oscl_sqrt(std / n_windowSize);
    for (i = 0; i < (int) n_windowSize; i++)
    {
        if (AVC_ABS(error[i]) > threshold)
            m_rgRejected[i] = TRUE;
    }
    // always include the last data point
    m_rgRejected[0] = FALSE;

    // second RD model estimator
    RCModelEstimator(rateCtrl, n_windowSize, m_rgRejected);

    if (MADModelFlag)
        updateMADModel(video, rateCtrl);
    else if (video->slice_type == AVC_P_SLICE || rateCtrl->idrPeriod == 1)
        rateCtrl->PPictureMAD[0] = rateCtrl->CurrentFrameMAD;


    return ;
}

double ComputeFrameMAD(AVCCommonObj *video, AVCRateControl *rateCtrl)
{
    double TotalMAD;
    int i;
    TotalMAD = 0.0;
    for (i = 0;i < (int)video->PicSizeInMbs;i++)
        TotalMAD += rateCtrl->MADofMB[i];
    TotalMAD /= video->PicSizeInMbs;
    return TotalMAD;
}

void RCModelEstimator(AVCRateControl *rateCtrl, int n_windowSize, bool *m_rgRejected)
{
    int n_realSize = n_windowSize;
    int i;
    double oneSampleQ = 0;
    double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;
    double MatrixValue;
    bool estimateX2 = FALSE;
    double m_X1, m_X2;

    for (i = 0; i < n_windowSize; i++)
    {// find the number of samples which are not rejected
        if (m_rgRejected[i])
            n_realSize--;
    }

    // default RD model estimation results

    m_X1 = m_X2 = 0.0;

    for (i = 0; i < n_windowSize; i++)
    {
        if (!m_rgRejected[i])
            oneSampleQ = rateCtrl->m_rgQp[i];
    }
    for (i = 0; i < n_windowSize; i++)
    {// if all non-rejected Q are the same, take 1st order model
        if ((rateCtrl->m_rgQp[i] != oneSampleQ) && !m_rgRejected[i])
            estimateX2 = TRUE;
        if (!m_rgRejected[i])
            m_X1 += (rateCtrl->m_rgQp[i] * rateCtrl->m_rgRp[i]) / n_realSize;
    }

    // take 2nd order model to estimate X1 and X2
    if ((n_realSize >= 1) && estimateX2)
    {
        for (i = 0; i < n_windowSize; i++)
        {
            if (!m_rgRejected[i])
            {
                a00 = a00 + 1.0;
                a01 += 1.0 / rateCtrl->m_rgQp[i];
                a10 = a01;
                a11 += 1.0 / (rateCtrl->m_rgQp[i] * rateCtrl->m_rgQp[i]);
                b0 += rateCtrl->m_rgQp[i] * rateCtrl->m_rgRp[i];
                b1 += rateCtrl->m_rgRp[i];
            }
        }
        // solve the equation of AX = B
        MatrixValue = a00 * a11 - a01 * a10;
        if (AVC_ABS(MatrixValue) > 0.000001)
        {
            m_X1 = (b0 * a11 - b1 * a01) / MatrixValue;
            m_X2 = (b1 * a00 - b0 * a10) / MatrixValue;
        }
        else
        {
            m_X1 = b0 / a00;
            m_X2 = 0.0;
        }
    }

    rateCtrl->m_X1 = m_X1;
    rateCtrl->m_X2 = m_X2;
}

void updateMADModel(AVCCommonObj *video, AVCRateControl *rateCtrl)
{

    int n_windowSize;
    int i;
    double error[20], std = 0.0, threshold;
    int m_Nc;
    double MADPictureC1;
    double MADPictureC2;
    bool PictureRejected[21];
    int	CodedBasicUnit = rateCtrl->TotalNumberofBasicUnit - rateCtrl->NumberofBasicUnit;

    if (rateCtrl->NumberofCodedPFrame > 0)
    {
        //assert (img->type!=P_SLICE);

        /*frame layer rate control*/
        if (rateCtrl->basicUnit == (int)video->PicSizeInMbs)
            m_Nc = rateCtrl->NumberofCodedPFrame;
        /*basic unit layer rate control*/
        else
            m_Nc = rateCtrl->NumberofCodedPFrame * rateCtrl->TotalNumberofBasicUnit + CodedBasicUnit;

        for (i = 19; i > 0; i--)
        {// update the history
            rateCtrl->PPictureMAD[i] = rateCtrl->PPictureMAD[i - 1];
            rateCtrl->ReferenceMAD[i] = rateCtrl->ReferenceMAD[i-1];
        }
        rateCtrl->PPictureMAD[0] = rateCtrl->CurrentFrameMAD;

        if (rateCtrl->basicUnit == (int)video->PicSizeInMbs)
            rateCtrl->ReferenceMAD[0] = rateCtrl->PPictureMAD[1];
        else
        {
            rateCtrl->ReferenceMAD[0] = rateCtrl->BUPFMAD[rateCtrl->TotalNumberofBasicUnit-1-rateCtrl->NumberofBasicUnit];
        }
        MADPictureC1 = rateCtrl->PMADPictureC1;
        MADPictureC2 = rateCtrl->PMADPictureC2;


        /*compute the size of window*/

        n_windowSize = (rateCtrl->CurrentFrameMAD > rateCtrl->PreviousFrameMAD) ? (int)(rateCtrl->PreviousFrameMAD / rateCtrl->CurrentFrameMAD * 20)\
                       : (int)(rateCtrl->CurrentFrameMAD / rateCtrl->PreviousFrameMAD * 20);
        n_windowSize = AVC_MIN(n_windowSize, (m_Nc - 1));
        n_windowSize = AVC_MAX(n_windowSize, 1);
        n_windowSize = AVC_MIN(n_windowSize, rateCtrl->MADm_windowSize + 1);
        n_windowSize = AVC_MIN(20, n_windowSize);
        /*update the previous window size*/
        rateCtrl->MADm_windowSize = n_windowSize;

        for (i = 0; i < 20; i++)
        {
            PictureRejected[i] = FALSE;
        }
        //update the MAD for the previous frame
        if (video->slice_type == AVC_P_SLICE)
            rateCtrl->PreviousFrameMAD = rateCtrl->CurrentFrameMAD;

        // initial MAD model estimator
        MADModelEstimator(video, rateCtrl, n_windowSize, &MADPictureC1, &MADPictureC2,
                          PictureRejected);

        // remove outlier

        for (i = 0; i < (int) n_windowSize; i++)
        {
            error[i] = MADPictureC1 * rateCtrl->ReferenceMAD[i] + MADPictureC2 - rateCtrl->PPictureMAD[i];
            std += error[i] * error[i];
        }
        threshold = (n_windowSize == 2) ? 0 : oscl_sqrt(std / n_windowSize);
        for (i = 0; i < (int) n_windowSize; i++)
        {
            if (AVC_ABS(error[i]) > threshold)
                PictureRejected[i] = TRUE;
        }
        // always include the last data point
        PictureRejected[0] = FALSE;

        // second MAD model estimator
        MADModelEstimator(video, rateCtrl, n_windowSize, &MADPictureC1, &MADPictureC2,
                          PictureRejected);
    }
}

void MADModelEstimator(AVCCommonObj *video, AVCRateControl *rateCtrl, int n_windowSize,
                       double *MADPictureC1, double *MADPictureC2, bool *PictureRejected)
{
    int n_realSize = n_windowSize;
    int i;
    double oneSampleQ = 0;
    double a00 = 0.0, a01 = 0.0, a10 = 0.0, a11 = 0.0, b0 = 0.0, b1 = 0.0;
    double MatrixValue;
    bool estimateX2 = FALSE;

    for (i = 0; i < n_windowSize; i++)
    {// find the number of samples which are not rejected
        if (PictureRejected[i])
            n_realSize--;
    }

    // default MAD model estimation results

    *MADPictureC1 = *MADPictureC2 = 0.0;

    for (i = 0; i < n_windowSize; i++)
    {
        if (!PictureRejected[i])
            oneSampleQ = rateCtrl->PPictureMAD[i];
    }

    for (i = 0; i < n_windowSize; i++)
    {// if all non-rejected MAD are the same, take 1st order model
        if ((rateCtrl->PPictureMAD[i] != oneSampleQ) && !PictureRejected[i])
            estimateX2 = TRUE;
        if (!PictureRejected[i])
            *MADPictureC1 += rateCtrl->PPictureMAD[i] / (rateCtrl->ReferenceMAD[i] * n_realSize);
    }

    // take 2nd order model to estimate X1 and X2
    if ((n_realSize >= 1) && estimateX2)
    {
        for (i = 0; i < n_windowSize; i++)
        {
            if (!PictureRejected[i])
            {
                a00 = a00 + 1.0;
                a01 += rateCtrl->ReferenceMAD[i];
                a10 = a01;
                a11 += rateCtrl->ReferenceMAD[i] * rateCtrl->ReferenceMAD[i];
                b0 += rateCtrl->PPictureMAD[i];
                b1 += rateCtrl->PPictureMAD[i] * rateCtrl->ReferenceMAD[i];
            }
        }
        // solve the equation of AX = B
        MatrixValue = a00 * a11 - a01 * a10;
        if (AVC_ABS(MatrixValue) > 0.000001)
        {
            *MADPictureC2 = (b0 * a11 - b1 * a01) / MatrixValue;
            *MADPictureC1 = (b1 * a00 - b0 * a10) / MatrixValue;
        }
        else
        {
            *MADPictureC1 = b0 / a01;
            *MADPictureC2 = 0.0;
        }

    }
    if (video->slice_type == AVC_P_SLICE)
    {
        rateCtrl->PMADPictureC1 = *MADPictureC1;
        rateCtrl->PMADPictureC2 = *MADPictureC2;
    }
}

/* convert from QP to Qstep */
double QP2Qstep(int QP)
{
    int i;
    double Qstep;
    static const double QP2QSTEP[6] = { 0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125 };

    Qstep = QP2QSTEP[QP % 6];
    for (i = 0; i < (QP / 6); i++)
        Qstep *= 2;

    return Qstep;
}

/* convert from step size to QP */
int Qstep2QP(double Qstep)
{
    int q_per = 0, q_rem = 0;

    //  assert( Qstep >= QP2Qstep(0) && Qstep <= QP2Qstep(51) );
    if (Qstep < QP2Qstep(0))
        return 0;
    else if (Qstep > QP2Qstep(51))
        return 51;

    while (Qstep > QP2Qstep(5))
    {
        Qstep /= 2;
        q_per += 1;
    }

    if (Qstep <= (0.625 + 0.6875) / 2)
    {
        Qstep = 0.625;
        q_rem = 0;
    }
    else if (Qstep <= (0.6875 + 0.8125) / 2)
    {
        Qstep = 0.6875;
        q_rem = 1;
    }
    else if (Qstep <= (0.8125 + 0.875) / 2)
    {
        Qstep = 0.8125;
        q_rem = 2;
    }
    else if (Qstep <= (0.875 + 1.0) / 2)
    {
        Qstep = 0.875;
        q_rem = 3;
    }
    else if (Qstep <= (1.0 + 1.125) / 2)
    {
        Qstep = 1.0;
        q_rem = 4;
    }
    else
    {
        Qstep = 1.125;
        q_rem = 5;
    }

    return (q_per * 6 + q_rem);
}



