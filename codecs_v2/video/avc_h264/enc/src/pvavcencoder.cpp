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
#include "pvavcencoder.h"
#include "oscl_mem.h"

/* global static functions */

void CbAvcEncDebugLog(uint32 *userData, AVCLogType type, char *string1, int val1, int val2)
{
    OSCL_UNUSED_ARG(userData);
    OSCL_UNUSED_ARG(type);
    OSCL_UNUSED_ARG(string1);
    OSCL_UNUSED_ARG(val1);
    OSCL_UNUSED_ARG(val2);

    return ;
}

int CbAvcEncMalloc(void *userData, int32 size, int attribute)
{
    OSCL_UNUSED_ARG(userData);
    OSCL_UNUSED_ARG(attribute);

    uint8 *mem;

    mem = (uint8*) oscl_malloc(size);

    return (int)mem;
}

void CbAvcEncFree(void *userData, int mem)
{
    OSCL_UNUSED_ARG(userData);

    oscl_free((void*)mem);

    return ;
}

int CbAvcEncDPBAlloc(void *userData, uint frame_size_in_mbs, uint num_buffers)
{
    PVAVCEncoder *pAvcEnc = (PVAVCEncoder*) userData;

    return pAvcEnc->AVC_DPBAlloc(frame_size_in_mbs, num_buffers);
}

void CbAvcEncFrameUnbind(void *userData, int indx)
{
    PVAVCEncoder *pAvcEnc = (PVAVCEncoder*) userData;

    pAvcEnc->AVC_FrameUnbind(indx);

    return ;
}

int CbAvcEncFrameBind(void *userData, int indx, uint8 **yuv)
{
    PVAVCEncoder *pAvcEnc = (PVAVCEncoder*) userData;

    return pAvcEnc->AVC_FrameBind(indx, yuv);
}



/* ///////////////////////////////////////////////////////////////////////// */
PVAVCEncoder::PVAVCEncoder()
{
//iEncoderControl
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF PVAVCEncoder::~PVAVCEncoder()
{
    CleanupEncoder();
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF PVAVCEncoder* PVAVCEncoder::New()
{
    PVAVCEncoder* self = new PVAVCEncoder;
    if (self && self->Construct())
        return self;
    if (self)
        delete self;
    return NULL;
}

/* ///////////////////////////////////////////////////////////////////////// */
bool PVAVCEncoder::Construct()
{
    oscl_memset((void *)&iAvcHandle, 0, sizeof(AVCHandle));

    iAvcHandle.CBAVC_DPBAlloc = &CbAvcEncDPBAlloc;
    iAvcHandle.CBAVC_FrameBind = &CbAvcEncFrameBind;
    iAvcHandle.CBAVC_FrameUnbind = &CbAvcEncFrameUnbind;
    iAvcHandle.CBAVC_Free = &CbAvcEncFree;
    iAvcHandle.CBAVC_Malloc = &CbAvcEncMalloc;
    iAvcHandle.CBAVC_DebugLog = &CbAvcEncDebugLog;
    iAvcHandle.userData = this;

    iYUVIn = NULL;
    iState = ECreated;
    iFramePtr = NULL;
    iDPB = NULL;
    iFrameUsed = NULL;

    return true;
}

/* ///////////////////////////////////////////////////////////////////////// */
/** overload function */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::Initialize(TAVCEIInputFormat *aVidInFormat, TAVCEIEncodeParam *aEncParam,
        void* extSPS, void* extPPS)
{
    AVCEncParams aEncOption; /* encoding options */

    if (EAVCEI_SUCCESS != Init(aVidInFormat, aEncParam, aEncOption))
    {
        return EAVCEI_FAIL;
    }

    if (AVCENC_SUCCESS != PVAVCEncInitialize(&iAvcHandle, &aEncOption, extSPS, extPPS))
    {
        return EAVCEI_FAIL;
    }

    iIDR = true;
    iState = EInitialized; // change state to initialized

    return EAVCEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::Initialize(TAVCEIInputFormat *aVidInFormat, TAVCEIEncodeParam *aEncParam)
{
    AVCEncParams aEncOption; /* encoding options */

    if (EAVCEI_SUCCESS != Init(aVidInFormat, aEncParam, aEncOption))
    {
        return EAVCEI_FAIL;
    }


    if (AVCENC_SUCCESS != PVAVCEncInitialize(&iAvcHandle, &aEncOption, NULL, NULL))
    {
        return EAVCEI_FAIL;
    }

    iIDR = true;
    iDispOrd = 0;
    iState = EInitialized; // change state to initialized

    return EAVCEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
TAVCEI_RETVAL PVAVCEncoder::Init(TAVCEIInputFormat* aVidInFormat, TAVCEIEncodeParam* aEncParam, AVCEncParams& aEncOption)
{
    if (iState == EInitialized || iState == EEncoding)  /* clean up before re-initialized */
    {

        PVAVCCleanUpEncoder(&iAvcHandle);
        if (iYUVIn)
        {
            oscl_free(iYUVIn);
            iYUVIn = NULL;
        }
        if (iVideoFormat == EAVCEI_VDOFMT_RGB24 || iVideoFormat == EAVCEI_VDOFMT_RGB12)
        {
#if defined(RGB24_INPUT)||defined(RGB12_INPUT)
            freeRGB2YUVTables();
#else
            return EAVCEI_FAIL;
#endif
        }
    }

    iState = ECreated; // change state back to created

    iId = aEncParam->iEncodeID;

    iSrcWidth = aVidInFormat->iFrameWidth;
    iSrcHeight = aVidInFormat->iFrameHeight;
    iSrcFrameRate = aVidInFormat->iFrameRate;
    iVideoFormat =  aVidInFormat->iVideoFormat;
    iFrameOrientation = aVidInFormat->iFrameOrientation;

    // allocate iYUVIn
    if ((iSrcWidth&0xF) || (iSrcHeight&0xF) || (iVideoFormat != EAVCEI_VDOFMT_YUV420)) /* Not multiple of 16 */
    {
        iYUVIn = (uint8*) oscl_malloc(((((iSrcWidth + 15) >> 4) * ((iSrcHeight + 15) >> 4)) * 3) << 7);
        if (iYUVIn == NULL)
        {
            return EAVCEI_FAIL;
        }
    }

    // check the buffer delay according to the clip duration
    if (aEncParam->iClipDuration > 0 && aEncParam->iRateControlType == EAVCEI_RC_VBR_1)
    {
        if (aEncParam->iBufferDelay > (float)(aEncParam->iClipDuration / 10000.0))   //enforce 10% variation of the clip duration as the bound of buffer delay
        {
            aEncParam->iBufferDelay = (float)(aEncParam->iClipDuration / 10000.0);
        }
    }

    /* Initialize the color conversion table if needed*/
    if (iVideoFormat == EAVCEI_VDOFMT_RGB24 || iVideoFormat == EAVCEI_VDOFMT_RGB12)
    {
#if defined(RGB24_INPUT)||defined(RGB12_INPUT)
        initRGB2YUVTables();
#else
        return EAVCEI_FAIL;
#endif
    }

    if (aEncParam->iNumLayer > 1)
    {
        return EAVCEI_FAIL;
    }

    aEncOption.width = iEncWidth = aEncParam->iFrameWidth[0];
    aEncOption.height = iEncHeight = aEncParam->iFrameHeight[0];
    iEncFrameRate = aEncParam->iFrameRate[0];
    aEncOption.frame_rate = (uint32)(1000 * iEncFrameRate);

    if (aEncParam->iRateControlType == EAVCEI_RC_CONSTANT_Q)
    {
        aEncOption.rate_control = AVC_OFF;
        aEncOption.bitrate = 48000; // default
    }
    else if (aEncParam->iRateControlType == EAVCEI_RC_CBR_1)
        aEncOption.rate_control = AVC_ON;
    else if (aEncParam->iRateControlType == EAVCEI_RC_VBR_1)
        aEncOption.rate_control = AVC_ON;
    else
        return EAVCEI_FAIL;

    // Check the bitrate, framerate, image size and buffer delay for 3GGP compliance
#ifdef FOR_3GPP_COMPLIANCE
    Check3GPPCompliance(aEncParam, iEncWidth, iEncHeight);
#endif

    // future :: map aEncParam->iEncMode to EncMode inside AVCEncoder

    iPacketSize = aEncParam->iPacketSize;
    aEncOption.profile = mapProfile(aEncParam->iProfile);
    aEncOption.level = mapLevel(aEncParam->iLevel);

    //aEncOption.src_interval = (int)(1000/aVidInFormat->iFrameRate + 0.5);

    aEncOption.bitrate = aEncParam->iBitRate[0];
    aEncOption.initQP = aEncParam->iIquant[0];

    aEncOption.init_CBP_removal_delay = (uint32)(aEncParam->iBufferDelay * 1000); // make it millisecond
    aEncOption.CPB_size = ((uint32)(aEncParam->iBufferDelay * aEncOption.bitrate));

    switch (aEncParam->iIFrameInterval)
    {
        case -1:
            aEncOption.idr_period = 0;
            break;
        case 0:
            aEncOption.idr_period = 1;
            break;
        default:
            aEncOption.idr_period = (int)(aEncParam->iIFrameInterval *  aVidInFormat->iFrameRate);
            break;
    }

    aEncOption.intramb_refresh = aEncParam->iNumIntraMBRefresh;
    aEncOption.auto_scd = (aEncParam->iSceneDetection == true) ? AVC_ON : AVC_OFF;
    aEncOption.out_of_band_param_set = (aEncParam->iOutOfBandParamSet == true) ? AVC_ON : AVC_OFF;

    /* default values */
    aEncOption.poc_type = 0;
    aEncOption.log2_max_poc_lsb_minus_4 = 12;
    aEncOption.num_ref_frame = 1;
    aEncOption.num_slice_group = 1;
    aEncOption.fmo_type = 0; /// FMO is disabled for now.
    aEncOption.db_filter = AVC_ON;
    aEncOption.disable_db_idc = 0;
    aEncOption.alpha_offset = 0;
    aEncOption.beta_offset = 0;
    aEncOption.constrained_intra_pred = AVC_OFF;

    aEncOption.data_par = AVC_OFF;
    aEncOption.fullsearch = AVC_OFF;
    aEncOption.search_range = 16;
    aEncOption.sub_pel = AVC_ON;
    aEncOption.submb_pred = AVC_OFF;
    aEncOption.rdopt_mode = AVC_OFF;
    aEncOption.bidir_pred = AVC_OFF;


    return EAVCEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::GetParameterSet(uint8 *paramSet, int32 *size, int *aNALType)
{
    uint aSize;
    AVCEnc_Status avcStatus ;

    if (iState != EInitialized) /* has to be initialized first */
        return EAVCEI_FAIL;

    aSize = *size;

    if (paramSet == NULL || size == NULL)
    {
        return EAVCEI_INPUT_ERROR;
    }

    avcStatus =	PVAVCEncodeNAL(&iAvcHandle, paramSet, &aSize, aNALType);

    if (avcStatus == AVCENC_WRONG_STATE)
    {
        *size = 0;
        return EAVCEI_FAIL;
    }

    switch (*aNALType)
    {
        case AVC_NALTYPE_SPS:
        case AVC_NALTYPE_PPS:
            *size = aSize;
            return EAVCEI_SUCCESS;
        default:
            *size = 0;
            return EAVCEI_FAIL;
    }
}

#ifdef PVAUTHOR_PROFILING
#include "pvauthorprofile.h"
#endif


/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::Encode(TAVCEIInputData *aVidIn
#ifdef PVAUTHOR_PROFILING
        , void *aParam1
#endif
                                                  )
{
    AVCEnc_Status status;

    if (aVidIn == NULL)
    {
        return EAVCEI_INPUT_ERROR;
    }
    // we need to check the timestamp here. If it's before the proper time,
    // we need to return EAVCEI_FRAME_DROP here.
    // also check whether encoder is ready to take a new frame.
    if (iState == EEncoding)
    {
        return EAVCEI_NOT_READY;
    }
    else if (iState == ECreated)
    {
        return EAVCEI_FAIL;
    }

#ifdef PVAUTHOR_PROFILING
    if (aParam1)((CPVAuthorProfile*)aParam1)->Start();
#endif

    if (iVideoFormat == EAVCEI_VDOFMT_YUV420)
#ifdef YUV_INPUT
    {
        if (iYUVIn) /* iSrcWidth is not multiple of 4 or iSrcHeight is odd number */
        {
            CopyToYUVIn(aVidIn->iSource, iSrcWidth, iSrcHeight,
            ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
            iVideoIn = iYUVIn;
        }
        else /* otherwise, we can just use aVidIn->iSource */
        {
            iVideoIn = aVidIn->iSource;  //   Sept 14, 2005 */
        }
    }
#else
        return EAVCEI_INPUT_ERROR;
#endif
    if (iVideoFormat == EAVCEI_VDOFMT_RGB12)
#ifdef RGB12_INPUT
    {
        RGB2YUV420_12bit((uint32 *)aVidIn->iSource, iSrcWidth, iSrcHeight,
        ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
        iVideoIn = iYUVIn;
    }
#else
        return EAVCEI_INPUT_ERROR;
#endif
    if (iVideoFormat == EAVCEI_VDOFMT_RGB24)
#ifdef RGB24_INPUT
    {
        RGB2YUV420_24bit(aVidIn->iSource, iSrcWidth, iSrcHeight,
        ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
        iVideoIn = iYUVIn;
    }
#else
        return EAVCEI_INPUT_ERROR;
#endif


#ifdef PVAUTHOR_PROFILING
    if (aParam1)((CPVAuthorProfile*)aParam1)->Stop(CPVAuthorProfile::EColorInput);
#endif
#ifdef PVAUTHOR_PROFILING
    if (aParam1)((CPVAuthorProfile*)aParam1)->Start();
#endif
    /* assign with backward-P or B-Vop this timestamp must be re-ordered */
    iTimeStamp = aVidIn->iTimeStamp;

    iVidIn.height = ((iSrcHeight + 15) >> 4) << 4;
    iVidIn.pitch = ((iSrcWidth + 15) >> 4) << 4;
    iVidIn.coding_timestamp = iTimeStamp;
    iVidIn.YCbCr[0] = (uint8*)iVideoIn;
    iVidIn.YCbCr[1] = (uint8*)(iVideoIn + iVidIn.height * iVidIn.pitch);
    iVidIn.YCbCr[2] = iVidIn.YCbCr[1] + ((iVidIn.height * iVidIn.pitch) >> 2);
    iVidIn.disp_order = iDispOrd;

    status = PVAVCEncSetInput(&iAvcHandle, &iVidIn);
#ifdef PVAUTHOR_PROFILING
    if (aParam1)((CPVAuthorProfile*)aParam1)->Stop(CPVAuthorProfile::EVideoEncode);
#endif

    switch (status)
    {
        case AVCENC_SKIPPED_PICTURE:
            return EAVCEI_FRAME_DROP;
        case AVCENC_FAIL: // not in the right state
            return EAVCEI_NOT_READY;
        case AVCENC_SUCCESS:
            iState = EEncoding;
            iDispOrd++;
            return EAVCEI_SUCCESS;
        case AVCENC_NEW_IDR:
            iState = EEncoding;
            iDispOrd++;
            iIDR = true;
            return EAVCEI_SUCCESS;
        default:
            return EAVCEI_FAIL;
    }
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::GetOutput(TAVCEIOutputData *aVidOut
#ifdef PVAUTHOR_PROFILING
        , void *aParam1
#endif
                                                     )
{
    AVCEnc_Status status;
    TAVCEI_RETVAL ret;
    uint Size;
    int nalType;
    AVCFrameIO recon;

    if (iState != EEncoding)
    {
        return EAVCEI_NOT_READY;
    }

    if (aVidOut == NULL)
    {
        return EAVCEI_INPUT_ERROR;
    }


    Size = aVidOut->iBitstreamSize;

#ifdef PVAUTHOR_PROFILING
    if (aParam1)((CPVAuthorProfile*)aParam1)->Start();
#endif

    status = PVAVCEncodeNAL(&iAvcHandle, (uint8*)aVidOut->iBitstream, &Size, &nalType);

    if (status == AVCENC_SUCCESS)
    {
        aVidOut->iLastNAL = false;
        aVidOut->iKeyFrame = iIDR;
        ret = EAVCEI_MORE_DATA;
    }
    else if (status == AVCENC_PICTURE_READY)
    {
        aVidOut->iLastNAL = true;
        aVidOut->iKeyFrame = iIDR;
        if (iIDR == true)
        {
            iIDR = false;
        }
        ret = EAVCEI_SUCCESS;
        iState = EInitialized;

        status = PVAVCEncGetRecon(&iAvcHandle, &recon);
        if (status == AVCENC_SUCCESS)
        {
            aVidOut->iFrame = recon.YCbCr[0];
            PVAVCEncReleaseRecon(&iAvcHandle, &recon);
        }
    }
    else
    {
        return EAVCEI_FAIL;
    }

    aVidOut->iLastFragment = true; /* for now */
    aVidOut->iFragment = false;  /* for now */
    aVidOut->iBitstreamSize = Size;
    aVidOut->iTimeStamp = iTimeStamp;

#ifdef PVAUTHOR_PROFILING
    if (aParam1)((CPVAuthorProfile*)aParam1)->Stop(CPVAuthorProfile::EVideoEncode);
#endif
    return ret;

}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::FlushInput()
{
    // do nothing for now.
    return EAVCEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
TAVCEI_RETVAL PVAVCEncoder::CleanupEncoder()
{
    if (iState == EInitialized || iState == EEncoding)
    {
        PVAVCCleanUpEncoder(&iAvcHandle);
        iState = ECreated;

        if (iYUVIn)
        {
            oscl_free(iYUVIn);
            iYUVIn = NULL;
        }

#if defined(RGB12_INPUT)||defined(RGB24_INPUT)
        if (iVideoFormat == EAVCEI_VDOFMT_RGB24 || iVideoFormat == EAVCEI_VDOFMT_RGB12)
            freeRGB2YUVTables();
#endif

    }
    if (iFrameUsed)
    {
        oscl_free(iFrameUsed);
        iFrameUsed = NULL;
    }
    if (iDPB)
    {
        oscl_free(iDPB);
        iDPB = NULL;
    }
    if (iFramePtr)
    {
        oscl_free(iFramePtr);
        iFramePtr = NULL;
    }
    return EAVCEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::UpdateBitRate(int32 *aBitRate)
{
    if (PVAVCEncUpdateBitRate(&iAvcHandle, aBitRate[0]) == AVCENC_SUCCESS)
        return EAVCEI_SUCCESS;
    else
        return EAVCEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::UpdateFrameRate(OsclFloat *aFrameRate)
{
    if (PVAVCEncUpdateFrameRate(&iAvcHandle, (uint32)(1000*aFrameRate[0]), 1000) == AVCENC_SUCCESS)
        return EAVCEI_SUCCESS;
    else
        return EAVCEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::UpdateIDRFrameInterval(int32 aIFrameInterval)
{
    if (PVAVCEncUpdateIDRInterval(&iAvcHandle, aIFrameInterval) == AVCENC_SUCCESS)
        return EAVCEI_SUCCESS;
    else
        return EAVCEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TAVCEI_RETVAL PVAVCEncoder::IDRRequest()
{
    if (PVAVCEncIDRRequest(&iAvcHandle) == AVCENC_SUCCESS)
        return EAVCEI_SUCCESS;
    else
        return EAVCEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF int32 PVAVCEncoder::GetEncodeWidth(int32 aLayer)
{
    OSCL_UNUSED_ARG(aLayer);
    return iEncWidth;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF int32 PVAVCEncoder::GetEncodeHeight(int32 aLayer)
{
    OSCL_UNUSED_ARG(aLayer);
    return iEncHeight;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF OsclFloat PVAVCEncoder::GetEncodeFrameRate(int32 aLayer)
{
    OSCL_UNUSED_ARG(aLayer);
    return iEncFrameRate;
}


#ifdef YUV_INPUT
/* ///////////////////////////////////////////////////////////////////////// */
/* Copy from YUV input to YUV frame inside M4VEnc lib						*/
/* When input is not YUV, the color conv will write it directly to iVideoInOut. */
/* ///////////////////////////////////////////////////////////////////////// */

void PVAVCEncoder::CopyToYUVIn(uint8 *YUV, int width, int height, int width_16, int height_16)
{
    uint8 *y, *u, *v, *yChan, *uChan, *vChan;
    int y_ind, ilimit, jlimit, i, j, ioffset;
    int size = width * height;
    int size16 = width_16 * height_16;

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16) /* if padding */
    {
        int offset = (height < height_16) ? height : height_16;

        offset = (offset * width_16);

        if (width < width_16)
        {
            offset -= (width_16 - width);
        }

        yChan = (uint8*)(iYUVIn + offset);
        oscl_memset(yChan, 16, size16 - offset); /* pad with zeros */

        uChan = (uint8*)(iYUVIn + size16 + (offset >> 2));
        oscl_memset(uChan, 128, (size16 - offset) >> 2);

        vChan = (uint8*)(iYUVIn + size16 + (size16 >> 2) + (offset >> 2));
        oscl_memset(vChan, 128, (size16 - offset) >> 2);
    }

    /* then do padding on the top */
    yChan = (uint8*)iYUVIn; /* Normal order */
    uChan = (uint8*)(iYUVIn + size16);
    vChan = (uint8*)(uChan + (size16 >> 2));

    u = (uint8*)(&(YUV[size]));
    v = (uint8*)(&(YUV[size*5/4]));

    /* To center the output */
    if (height_16 > height)   /* output taller than input */
    {
        if (width_16 >= width)  /* output wider than or equal input */
        {
            i = ((height_16 - height) >> 1) * width_16 + (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = ((height_16 - height) >> 2) * (width_16 >> 1) + (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
        }
        else  /* output narrower than input */
        {
            i = ((height_16 - height) >> 1) * width_16;
            j = ((height_16 - height) >> 2) * (width_16 >> 1);
            YUV += ((width - width_16) >> 1);
            u += ((width - width_16) >> 2);
            v += ((width - width_16) >> 2);
        }
        oscl_memset((uint8 *)yChan, 16, i);
        yChan += i;
        oscl_memset((uint8 *)uChan, 128, j);
        uChan += j;
        oscl_memset((uint8 *)vChan, 128, j);
        vChan += j;
    }
    else   /* output shorter or equal input */
    {
        if (width_16 >= width)   /* output wider or equal input */
        {
            i = (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
            YUV += (((height - height_16) >> 1) * width);
            u += (((height - height_16) >> 1) * width) >> 2;
            v += (((height - height_16) >> 1) * width) >> 2;
        }
        else  /* output narrower than input */
        {
            i = 0;
            j = 0;
            YUV += (((height - height_16) >> 1) * width + ((width - width_16) >> 1));
            u += (((height - height_16) >> 1) * width + ((width - width_16) >> 1)) >> 2;
            v += (((height - height_16) >> 1) * width + ((width - width_16) >> 1)) >> 2;
        }
        oscl_memset((uint8 *)yChan, 16, i);
        yChan += i;
        oscl_memset((uint8 *)uChan, 128, j);
        uChan += j;
        oscl_memset((uint8 *)vChan, 128, j);
        vChan += j;
    }

    /* Copy with cropping or zero-padding */
    if (height < height_16)
        jlimit = height;
    else
        jlimit = height_16;

    if (width < width_16)
    {
        ilimit = width;
        ioffset = width_16 - width;
    }
    else
    {
        ilimit = width_16;
        ioffset = 0;
    }

    /* Copy Y */
    /* Set up pointer for fast looping */
    y = (uint8*)YUV;

    if (width == width_16 && height == height_16) /* no need to pad */
    {
        oscl_memcpy(yChan, y, size);
    }
    else
    {
        for (y_ind = 0; y_ind < (jlimit - 1) ;y_ind++)
        {
            oscl_memcpy(yChan, y, ilimit);
            oscl_memset(yChan + ilimit, 16, ioffset); /* pad with zero */
            yChan += width_16;
            y += width;
        }
        oscl_memcpy(yChan, y, ilimit); /* last line no padding */
    }
    /* Copy U and V */
    /* Set up pointers for fast looping */
    if (width == width_16 && height == height_16) /* no need to pad */
    {
        oscl_memcpy(uChan, u, size >> 2);
        oscl_memcpy(vChan, v, size >> 2);
    }
    else
    {
        for (y_ind = 0; y_ind < (jlimit >> 1) - 1;y_ind++)
        {
            oscl_memcpy(uChan, u, ilimit >> 1);
            oscl_memcpy(vChan, v, ilimit >> 1);
            oscl_memset(uChan + (ilimit >> 1), 128, ioffset >> 1);
            oscl_memset(vChan + (ilimit >> 1), 128, ioffset >> 1);
            uChan += (width_16 >> 1);
            u += (width >> 1);
            vChan += (width_16 >> 1);
            v += (width >> 1);
        }
        oscl_memcpy(uChan, u, ilimit >> 1); /* last line no padding */
        oscl_memcpy(vChan, v, ilimit >> 1);
    }

    return ;
}
#endif

#if defined(RGB12_INPUT) || defined(RGB24_INPUT)
/* ///////////////////////////////////////////////////////////////////////// */
/* The following four functions are for RGB->YUV, convert				   */
/* RGB input(12bit/24bit) to YUV frame inside M4VEnc lib				   */
/* ///////////////////////////////////////////////////////////////////////// */
bool PVAVCEncoder::initRGB2YUVTables()
{
    int i;
    uint16 *pTable;

    iY_Table  = NULL;
    iCb_Table = iCr_Table = ipCb_Table = ipCr_Table = NULL;

    /* memory allocation */
    if ((iY_Table = (uint8*)oscl_malloc(384)) == NULL)
        return false;

    if ((iCb_Table = (uint16*)oscl_malloc(768 * 2)) == NULL)
        return false;

    if ((iCr_Table = (uint16*)oscl_malloc(768 * 2)) == NULL)
        return false;

#define pv_max(a, b)	((a) >= (b) ? (a) : (b))
#define pv_min(a, b)	((a) <= (b) ? (a) : (b))

    /* Table generation */
    for (i = 0; i < 384; i++)
        iY_Table[i] = (uint8) pv_max(pv_min(255, (int)(0.7152 * i + 16 + 0.5)), 0);

    pTable = iCb_Table + 384;
    for (i = -384; i < 384; i++)
        pTable[i] = (uint16) pv_max(pv_min(255, (int)(0.386 * i + 128 + 0.5)), 0);
    ipCb_Table = iCb_Table + 384;

    pTable = iCr_Table + 384;
    for (i = -384; i < 384; i++)
        pTable[i] = (uint16) pv_max(pv_min(255, (int)(0.454 * i + 128 + 0.5)), 0);
    ipCr_Table = iCr_Table + 384;

    return true;

}

void PVAVCEncoder::freeRGB2YUVTables()
{
    if (iY_Table) oscl_free(iY_Table);
    if (iCb_Table) oscl_free(iCb_Table);
    if (iCr_Table) oscl_free(iCr_Table);

    iY_Table  = NULL;
    iCb_Table = iCr_Table = ipCb_Table = ipCr_Table = NULL;

}
#endif
#ifdef RGB12_INPUT
#ifdef VERSION_0
/* Assume B is in the first 4 bits, G is in the second 4 bits, and R is in the third 4 bits, of a whole 16bit unsigned integer */
void PVAVCEncoder::RGB2YUV420_12bit(uint16 *inputRGB, int width, int height, int width_16, int height_16)
{
    int i, j;
    uint8 *tempY, *tempU, *tempV;
    uint16 *inputRGB_prevRow = NULL;


    tempY = iYUVIn; /* Normal order */
    tempU = iYUVIn + height_16 * width_16;
    tempV = iYUVIn + height_16 * width_16 + (height_16 * width_16 >> 2);

    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++)
        {

            *tempY++ = iY_Table[(6616*(inputRGB[i] << 4 & 0x00f0) + ((inputRGB[i] & 0x00f0) << 16) + 19481 * (inputRGB[i] >> 4 & 0x00f0)) >> 16];

            /* downsampling U, V */
            if (j % 2 == 1 && i % 2 == 1)
            {

                *tempU++ = (uint8)(ipCb_Table[(((inputRGB[i] << 4 & 0x00f0) << 16) - ((inputRGB[i] & 0x00f0) << 16) +
                                               19525 * ((inputRGB[i] << 4 & 0x00f0) - (inputRGB[i] >> 4 & 0x00f0))) >> 16] + /* bottom right(current) */

                                   ipCb_Table[(((inputRGB[i-1] << 4 & 0x00f0) << 16) - ((inputRGB[i-1] & 0x00f0) << 16) +
                                               19525 * ((inputRGB[i-1] << 4 & 0x00f0) - (inputRGB[i-1] >> 4 & 0x00f0))) >> 16] + /* bottom left */

                                   ipCb_Table[(((inputRGB_prevRow[i] << 4 & 0x00f0) << 16) - ((inputRGB_prevRow[i] & 0x00f0) << 16) +
                                               19525 * ((inputRGB_prevRow[i] << 4 & 0x00f0) - (inputRGB_prevRow[i] >> 4 & 0x00f0))) >> 16] + /* top right */

                                   ipCb_Table[(((inputRGB_prevRow[i-1] << 4 & 0x00f0) << 16) - ((inputRGB_prevRow[i-1] & 0x00f0) << 16) +
                                               19525 * ((inputRGB_prevRow[i-1] << 4 & 0x00f0) - (inputRGB_prevRow[i-1] >> 4 & 0x00f0))) >> 16] + /* top left */

                                   2 >> 2);


                *tempV++ = (uint8)(ipCr_Table[(((inputRGB[i] >> 4 & 0x00f0) << 16) - ((inputRGB[i] & 0x00f0) << 16) +
                                               6640 * ((inputRGB[i] >> 4 & 0x00f0) - (inputRGB[i] << 4 & 0x00f0))) >> 16] + /* bottom right(current) */

                                   ipCr_Table[(((inputRGB[i-1] >> 4 & 0x00f0) << 16) - ((inputRGB[i-1] & 0x00f0) << 16) +
                                               6640 * ((inputRGB[i-1] >> 4 & 0x00f0) - (inputRGB[i-1] << 4 & 0x00f0))) >> 16] + /* bottom left */

                                   ipCr_Table[(((inputRGB_prevRow[i] >> 4 & 0x00f0) << 16) - ((inputRGB_prevRow[i] & 0x00f0) << 16) +
                                               6640 * ((inputRGB_prevRow[i] >> 4 & 0x00f0) - (inputRGB_prevRow[i] << 4 & 0x00f0))) >> 16] + /* top right */

                                   ipCr_Table[(((inputRGB_prevRow[i-1] >> 4 & 0x00f0) << 16) - ((inputRGB_prevRow[i-1] & 0x00f0) << 16) +
                                               6640 * ((inputRGB_prevRow[i-1] >> 4 & 0x00f0) - (inputRGB_prevRow[i-1] << 4 & 0x00f0))) >> 16] + /* top left */

                                   2 >> 2);
            }
        }

        /* do padding if input RGB size(width) is different from the output YUV size(width_16) */
        if (width < width_16)
        {
            oscl_memset(tempY, *(tempY - 1), width_16 - width);
            tempY += (width_16 - width);

            if (j % 2 == 1)
            {
                oscl_memset(tempU, *(tempU - 1), (width_16 - width) >> 1);
                tempU += (width_16 - width) >> 1;
                oscl_memset(tempV, *(tempV - 1), (width_16 - width) >> 1);
                tempV += (width_16 - width) >> 1;
            }
        }

        inputRGB_prevRow = inputRGB;
        inputRGB += width;	/* move to the next row */
    }

    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16)
    {
        tempY = iYUVIn + height * width_16;
        tempU = tempY - width_16;/* tempU is for temporary use, not meaning U stuff */
        for (i = height; i < height_16; i++)
        {
            oscl_memcpy(tempY, tempU, width_16);
            tempY += width_16;
        }

        tempU = iYUVIn + height_16 * width_16 + (height * width_16 >> 2);
        tempV = tempU - (width_16 >> 1); /* tempV is for temporary use, not meaning V stuff */
        for (i = height >> 1; i<height_16 >> 1; i++)
        {
            oscl_memcpy(tempU, tempV, (width_16 >> 1));
            tempU += (width_16 >> 1);
        }

        tempV = iYUVIn + height_16 * width_16 + (height_16 * width_16 >> 2) + (height * width_16 >> 2);
        tempY = tempV - (width_16 >> 1); /* tempY is for temporary use, not meaning Y stuff */
        for (i = height >> 1; i<height_16 >> 1; i++)
        {
            oscl_memcpy(tempV, tempY, (width_16 >> 1));
            tempV += (width_16 >> 1);
        }

    }

}
#endif

/* Assume width is divisible by 8 */
void PVAVCEncoder::RGB2YUV420_12bit(
    uint32 *inputRGB,
    int     width,
    int     height,
    int     width_16,
    int     height_16)
{
    int i;
    int j;
    int ilimit;
    int jlimit;
    uint32 *tempY;
    uint32 *tempU;
    uint32 *tempV;
    uint32 pixels;
    uint32 pixels_nextRow;
    uint32 yuv_value;
    uint32 yuv_value1;
    uint32 yuv_value2;
    int R_ds; /* "_ds" is the downsample version */
    int G_ds; /* "_ds" is the downsample version */
    int B_ds; /* "_ds" is the downsample version */
    int adjust = (width >> 1);
    int size16 = height_16 * width_16;
    int tmp;
    uint32 temp;

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16)
    { /* if padding */
        int offset = (height < height_16) ? height : height_16;

        offset = (offset * width_16);

        if (width < width_16)
        {
            offset -= (width_16 - width);
        }

        tempY = (uint32 *)iYUVIn + (offset >> 2);
        oscl_memset((uint8 *)tempY, 16, size16 - offset); /* pad with zeros */

        tempU = (uint32 *)iYUVIn + (size16 >> 2) + (offset >> 4);
        oscl_memset((uint8 *)tempU, 128, (size16 - offset) >> 2);

        tempV = (uint32 *)iYUVIn + (size16 >> 2) + (size16 >> 4) + (offset >> 4);
        oscl_memset((uint8 *)tempV, 128, (size16 - offset) >> 2);
    }

    /* then do padding on the top */
    tempY = (uint32 *)iYUVIn; /* Normal order */
    tempU = tempY + ((size16) >> 2);
    tempV = tempU + ((size16) >> 4);

    /* To center the output */
    if (height_16 > height)
    {
        if (width_16 >= width)
        {
            i = ((height_16 - height) >> 1) * width_16 + (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = ((height_16 - height) >> 2) * (width_16 >> 1) + (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
        }
        else
        {
            i = ((height_16 - height) >> 1) * width_16;
            j = ((height_16 - height) >> 2) * (width_16 >> 1);
            inputRGB += (width - width_16) >> 2;
        }
        oscl_memset((uint8 *)tempY, 16, i);
        tempY += (i >> 2);
        oscl_memset((uint8 *)tempU, 128, j);
        tempU += (j >> 2);
        oscl_memset((uint8 *)tempV, 128, j);
        tempV += (j >> 2);
    }
    else
    {
        if (width_16 >= width)
        {
            i = (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
            inputRGB += (((height - height_16) >> 1) * width) >> 1;

            oscl_memset((uint8 *)tempY, 16, i);
            tempY += (i >> 2);
            oscl_memset((uint8 *)tempU, 128, j);
            tempU += (j >> 2);
            oscl_memset((uint8 *)tempV, 128, j);
            tempV += (j >> 2);

        }
        else
        {
            i = 0;
            j = 0;
            inputRGB += (((height - height_16) >> 1) * width + ((width - width_16) >> 1)) >> 1 ;
        }
    }

    /* ColorConv RGB12-to-YUV420 with cropping or zero-padding */
    if (height < height_16)
    {
        jlimit = height;
    }
    else
    {
        jlimit = height_16;
    }

    if (width < width_16)
    {
        ilimit = width >> 1;
    }
    else
    {
        ilimit = width_16 >> 1;
    }

    width = width_16 - width;


    for (j = 0; j < jlimit; j++)
    {

        for (i = 0; i < ilimit; i += 4)
        {
            pixels =  inputRGB[i];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            yuv_value = (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]    |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8));

            pixels =  inputRGB[i+1];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            *tempY++ = (yuv_value                                                         |
                        (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]     |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8)) << 16);

            pixels =  inputRGB[i+2];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            yuv_value = (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]    |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8));

            pixels =  inputRGB[i+3];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            *tempY++ = (yuv_value                                                         |
                        (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]     |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8)) << 16);


            /* downsampling U, V */

            pixels_nextRow = inputRGB[i+3+adjust/*(width>>1)*/];
            G_ds    =  pixels & 0x00F000F0;
            G_ds   += (pixels_nextRow & 0x00F000F0);
            G_ds   += (G_ds >> 16);

            G_ds   -= 2; /* equivalent to adding constant 2<<16 = 131072 */

            pixels &= 0x0F0F0F0F;
            pixels += (pixels_nextRow & 0x0F0F0F0F);

            pixels += (pixels >> 16);

            B_ds = (pixels & 0x0003F) << 4;

            R_ds = (pixels & 0x03F00) >> 4;

            tmp  = B_ds - R_ds;

            yuv_value1 = ipCb_Table[(((B_ds-G_ds)<<16) + 19525*tmp)>>18] << 24;
            yuv_value2 = ipCr_Table[(((R_ds-G_ds)<<16) -  6640*tmp)>>18] << 24;

            pixels =  inputRGB[i+2];
            pixels_nextRow = inputRGB[i+2+adjust/*(width>>1)*/];

            G_ds    =  pixels & 0x00F000F0;
            G_ds   += (pixels_nextRow & 0x00F000F0);
            G_ds   += (G_ds >> 16);

            G_ds   -= 2; /* equivalent to adding constant 2<<16 = 131072 */

            pixels &= 0x0F0F0F0F;
            pixels += (pixels_nextRow & 0x0F0F0F0F);

            pixels += (pixels >> 16);

            B_ds = (pixels & 0x0003F) << 4;

            R_ds = (pixels & 0x03F00) >> 4;
            tmp  = B_ds - R_ds;

            yuv_value1 |= ipCb_Table[(((B_ds-G_ds)<<16) + 19525*tmp)>>18] << 16;
            yuv_value2 |= ipCr_Table[(((R_ds-G_ds)<<16) -  6640*tmp)>>18] << 16;

            pixels =  inputRGB[i+1];
            pixels_nextRow = inputRGB[i+1+adjust /*(width>>1)*/];

            G_ds    =  pixels & 0x00F000F0;
            G_ds   += (pixels_nextRow & 0x00F000F0);
            G_ds   += (G_ds >> 16);

            G_ds   -= 2; /* equivalent to adding constant 2<<16 = 131072 */

            pixels &= 0x0F0F0F0F;
            pixels += (pixels_nextRow & 0x0F0F0F0F);

            pixels += (pixels >> 16);

            B_ds = (pixels & 0x0003F) << 4;

            R_ds = (pixels & 0x03F00) >> 4;
            tmp  = B_ds - R_ds;


            yuv_value1 |= ipCb_Table[(((B_ds-G_ds)<<16) + 19525*tmp)>>18] << 8;
            yuv_value2 |= ipCr_Table[(((R_ds-G_ds)<<16) -  6640*tmp)>>18] << 8;

            pixels =  inputRGB[i];
            pixels_nextRow = inputRGB[i+adjust/*(width>>1)*/];

            G_ds    =  pixels & 0x00F000F0;
            G_ds   += (pixels_nextRow & 0x00F000F0);
            G_ds   += (G_ds >> 16);

            G_ds   -= 2; /* equivalent to adding constant 2<<16 = 131072 */

            pixels &= 0x0F0F0F0F;
            pixels += (pixels_nextRow & 0x0F0F0F0F);

            pixels += (pixels >> 16);

            B_ds = (pixels & 0x0003F) << 4;

            R_ds = (pixels & 0x03F00) >> 4;
            tmp  = B_ds - R_ds;

            *tempU++ = yuv_value1 | (ipCb_Table[(((B_ds-G_ds)<<16) + 19525*tmp)>>18]);
            *tempV++ = yuv_value2 | (ipCr_Table[(((R_ds-G_ds)<<16) -  6640*tmp)>>18]);
        }

        /* do padding if input RGB size(width) is different from the output YUV size(width_16) */

        if ((width > 0) && j < jlimit - 1)
        {
            oscl_memset((uint8 *)tempY, 16/*(*(tempY-1))>>24*/, width);
            tempY += width >> 2;
            oscl_memset((uint8 *)tempU, 128/*(*(tempU-1))>>24*/, width >> 1);
            tempU += width >> 3;
            oscl_memset((uint8 *)tempV, 128/*(*(tempV-1))>>24*/, width >> 1);
            tempV += width >> 3;
        }

        if (j++ == (jlimit - 1))
        {
            break;          /* dealing with a odd height  */
        }

        inputRGB += adjust; /* (160/2 = 80 ) */ /*(width>>1)*/; /* move to the next row */

        for (i = 0; i < ilimit; i += 4)
        {
            pixels =  inputRGB[i];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            yuv_value = (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]    |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8));

            pixels =  inputRGB[i+1];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            *tempY++ = (yuv_value                                                         |
                        (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]     |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8)) << 16);

            pixels =  inputRGB[i+2];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            yuv_value = (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]    |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8));

            pixels =  inputRGB[i+3];
            temp = (827 * (pixels & 0x000F000F) + 2435 * ((pixels & 0x0F000F00) >> 8));
            *tempY++ = (yuv_value                                                         |
                        (iY_Table[((temp&0x0FFFF)>> 9) + (pixels & 0x000000F0)]     |
                         (iY_Table[(temp         >>25) + ((pixels & 0x00F00000)>>16)] << 8)) << 16);

        }

        /* do padding if input RGB size(width) is different from the output YUV size(width_16) */
        if ((width > 0) && j < jlimit - 1)
        {
            oscl_memset((uint8 *)tempY, 16/*(*(tempY-1))>>24*/, width);
            tempY += width >> 2;
        }

        inputRGB += adjust; /* (160/2 = 80 ) */ /*(width>>1)*/; /* move to the next row */

    } /* for(j=0; j<jlimit; j++)*/
}
#endif
#ifdef RGB24_INPUT
/* Assume B is in the first byte, G is in the second byte, and R is in the third byte, of a whole 3-octct group */
void PVAVCEncoder::RGB2YUV420_24bit(uint8 *inputRGB, int width, int height, int width_16, int height_16)
{
    int i, j, ilimit, jlimit;
    uint8 *tempY, *tempU, *tempV;
    uint8 *inputRGB_prevRow = NULL;
    int32 size16 = height_16 * width_16;
    int32 adjust = (width + (width << 1));

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16) /* if padding */
    {
        int offset = (height < height_16) ? height : height_16;

        offset = (offset * width_16);

        if (width < width_16)
        {
            offset -= (width_16 - width);
        }
        tempY = iYUVIn + offset;
        oscl_memset((uint8 *)tempY, 16, size16 - offset); /* pad with zeros */

        tempU = iYUVIn + size16 + (offset >> 2);
        oscl_memset((uint8 *)tempU, 128, (size16 - offset) >> 2);

        tempV = iYUVIn + size16 + (size16 >> 2) + (offset >> 2);
        oscl_memset((uint8 *)tempV, 128, (size16 - offset) >> 2);
    }

    /* then do padding on the top */
    tempY = iYUVIn; /* Normal order */
    tempU = iYUVIn + size16;
    tempV = tempU + (size16 >> 2);

    /* To center the output */
    if (height_16 > height)
    {
        if (width_16 >= width)
        {
            i = ((height_16 - height) >> 1) * width_16 + (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = ((height_16 - height) >> 2) * (width_16 >> 1) + (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
        }
        else
        {
            i = ((height_16 - height) >> 1) * width_16;
            j = ((height_16 - height) >> 2) * (width_16 >> 1);
            inputRGB += ((width - width_16) >> 1) * 3;
        }
        oscl_memset((uint8 *)tempY, 16, i);
        tempY += i;
        oscl_memset((uint8 *)tempU, 128, j);
        tempU += j;
        oscl_memset((uint8 *)tempV, 128, j);
        tempV += j;
    }
    else
    {
        if (width_16 >= width)
        {
            i = (((width_16 - width) >> 3) << 2);
            /* make sure that (width_16-width)>>1 is divisible by 4 */
            j = (((width_16 - width) >> 4) << 2);
            /* make sure that (width_16-width)>>2 is divisible by 4 */
            inputRGB += (((height - height_16) >> 1) * width) * 3;
        }
        else
        {
            i = 0;
            j = 0;
            inputRGB += (((height - height_16) >> 1) * width + ((width - width_16) >> 1)) * 3;
        }
        oscl_memset((uint8 *)tempY, 16, i);
        tempY += i;
        oscl_memset((uint8 *)tempU, 128, j);
        tempU += j;
        oscl_memset((uint8 *)tempV, 128, j);
        tempV += j;
    }

    /* ColorConv RGB24-to-YUV420 with cropping or zero-padding */
    if (height < height_16)
        jlimit = height;
    else
        jlimit = height_16;

    if (width < width_16)
        ilimit = width;
    else
        ilimit = width_16;

    if (iFrameOrientation > 0)		//Bottom_UP RGB
    {
        inputRGB += (jlimit - 1) * width * 3 ; // move to last row
        adjust = -adjust;
    }

    for (j = 0; j < jlimit; j++)
    {
        for (i = 0; i < ilimit*3; i += 3)
        {

            *tempY++ = iY_Table[(6616*inputRGB[i] + (inputRGB[i+1] << 16) + 19481 * inputRGB[i+2]) >> 16];

            /* downsampling U, V */
            if (j % 2 == 1 && i % 2 == 1)
            {

                *tempU++ = (unsigned char)((ipCb_Table[((inputRGB[i] << 16) - (inputRGB[i+1] << 16) + 19525 * (inputRGB[i] - inputRGB[i+2])) >> 16] + /* bottom right(current) */
                                            ipCb_Table[((inputRGB[i-3] << 16) - (inputRGB[i-2] << 16) + 19525 * (inputRGB[i-3] - inputRGB[i-1])) >> 16] + /* bottom left */
                                            ipCb_Table[((inputRGB_prevRow[i] << 16) - (inputRGB_prevRow[i+1] << 16) + 19525 * (inputRGB_prevRow[i] - inputRGB_prevRow[i+2])) >> 16] + /* top right */
                                            ipCb_Table[((inputRGB_prevRow[i-3] << 16) - (inputRGB_prevRow[i-2] << 16) + 19525 * (inputRGB_prevRow[i-3] - inputRGB_prevRow[i-1])) >> 16]  + /* top left */
                                            2) >> 2);


                *tempV++ = (unsigned char)((ipCr_Table[((inputRGB[i+2] << 16) - (inputRGB[i+1] << 16) + 6640 * (inputRGB[i+2] - inputRGB[i])) >> 16] + /* bottom right(current) */
                                            ipCr_Table[((inputRGB[i-1] << 16) - (inputRGB[i-2] << 16) + 6640 * (inputRGB[i-1] - inputRGB[i-3])) >> 16] + /* bottom left */
                                            ipCr_Table[((inputRGB_prevRow[i+2] << 16) - (inputRGB_prevRow[i+1] << 16) + 6640 * (inputRGB_prevRow[i+2] - inputRGB_prevRow[i])) >> 16] + /* top right */
                                            ipCr_Table[((inputRGB_prevRow[i-1] << 16) - (inputRGB_prevRow[i-2] << 16) + 6640 * (inputRGB_prevRow[i-1] - inputRGB_prevRow[i-3])) >> 16]  + /* top left */
                                            2) >> 2);

            }
        }

        /* do padding if input RGB size(width) is different from the output YUV size(width_16) */
        if (width < width_16 && j < jlimit - 1)
        {
            oscl_memset(tempY, 16/* *(tempY-1)*/, width_16 - width);
            tempY += (width_16 - width);

            if (j % 2 == 1)
            {
                oscl_memset(tempU, 128/* *(tempU-1)*/, (width_16 - width) >> 1);
                tempU += (width_16 - width) >> 1;
                oscl_memset(tempV, 128/* *(tempV-1)*/, (width_16 - width) >> 1);
                tempV += (width_16 - width) >> 1;
            }
        }

        inputRGB_prevRow = inputRGB;
        inputRGB += adjust ; /* move to the next row */
    }

}
#endif

#ifdef FOR_3GPP_COMPLIANCE
void PVAVCEncoder::Check3GPPCompliance(TAVCEIEncodeParam *aEncParam, int *aEncWidth, int *aEncHeight)
{

//MPEG-4 Simple profile and level 0
#define MAX_BITRATE 64000
#define MAX_FRAMERATE 15
#define MAX_WIDTH 176
#define MAX_HEIGHT 144
#define MAX_BUFFERSIZE 163840

    // check bitrate, framerate, video size and vbv buffer
    if (aEncParam->iBitRate[0] > MAX_BITRATE) aEncParam->iBitRate[0] = MAX_BITRATE;
    if (aEncParam->iFrameRate[0] > MAX_FRAMERATE) aEncParam->iFrameRate[0] = MAX_FRAMERATE;
    if (aEncWidth[0] > MAX_WIDTH) aEncWidth[0] = MAX_WIDTH;
    if (aEncHeight[0] > MAX_HEIGHT) aEncHeight[0] = MAX_HEIGHT;
    if (aEncParam->iBitRate[0]*aEncParam->iBufferDelay > MAX_BUFFERSIZE)
        aEncParam->iBufferDelay = (float)MAX_BUFFERSIZE / aEncParam->iBitRate[0];
}
#endif

AVCProfile PVAVCEncoder::mapProfile(TAVCEIProfile in)
{
    AVCProfile out;

    switch (in)
    {
        case EAVCEI_PROFILE_DEFAULT:
        case EAVCEI_PROFILE_BASELINE:
            out = AVC_BASELINE;
            break;
        case EAVCEI_PROFILE_MAIN:
            out = AVC_MAIN;
            break;
        case EAVCEI_PROFILE_EXTENDED:
            out = AVC_EXTENDED;
            break;
        case EAVCEI_PROFILE_HIGH:
            out = AVC_HIGH;
            break;
        case EAVCEI_PROFILE_HIGH10:
            out = AVC_HIGH10;
            break;
        case EAVCEI_PROFILE_HIGH422:
            out = AVC_HIGH422;
            break;
        case EAVCEI_PROFILE_HIGH444:
            out = AVC_HIGH444;
            break;
        default:
            out = AVC_BASELINE;
            break;
    }

    return out;
}

AVCLevel PVAVCEncoder::mapLevel(TAVCEILevel in)
{
    AVCLevel out;

    switch (in)
    {
        case EAVCEI_LEVEL_AUTODETECT:
        case EAVCEI_LEVEL_1:
            out = AVC_LEVEL1;
            break;
        case EAVCEI_LEVEL_1B:
            out = AVC_LEVEL1_B;
            break;
        case EAVCEI_LEVEL_11:
            out = AVC_LEVEL1_1;
            break;
        case EAVCEI_LEVEL_12:
            out = AVC_LEVEL1_2;
            break;
        case EAVCEI_LEVEL_13:
            out = AVC_LEVEL1_3;
            break;
        case EAVCEI_LEVEL_2:
            out = AVC_LEVEL2;
            break;
        case EAVCEI_LEVEL_21:
            out = AVC_LEVEL2_1;
            break;
        case EAVCEI_LEVEL_22:
            out = AVC_LEVEL2_2;
            break;
        case EAVCEI_LEVEL_3:
            out = AVC_LEVEL3;
            break;
        case EAVCEI_LEVEL_31:
            out = AVC_LEVEL3_1;
            break;
        case EAVCEI_LEVEL_32:
            out = AVC_LEVEL3_2;
            break;
        case EAVCEI_LEVEL_4:
            out = AVC_LEVEL4;
            break;
        case EAVCEI_LEVEL_41:
            out = AVC_LEVEL4_1;
            break;
        case EAVCEI_LEVEL_42:
            out = AVC_LEVEL4_2;
            break;
        case EAVCEI_LEVEL_5:
            out = AVC_LEVEL5;
            break;
        case EAVCEI_LEVEL_51:
            out = AVC_LEVEL5_1;
            break;
        default:
            out = AVC_LEVEL5_1;
            break;
    }

    return out;
}


/* ///////////////////////////////////////////////////////////////////////// */

int PVAVCEncoder::AVC_DPBAlloc(uint frame_size_in_mbs, uint num_buffers)
{
    int ii;
    uint frame_size = (frame_size_in_mbs << 8) + (frame_size_in_mbs << 7);

    if (iDPB) oscl_free(iDPB); // free previous one first

    iDPB = (uint8*) oscl_malloc(sizeof(uint8) * frame_size * num_buffers);
    if (iDPB == NULL)
    {
        return 0;
    }

    iNumFrames = num_buffers;

    if (iFrameUsed) oscl_free(iFrameUsed); // free previous one

    iFrameUsed = (bool*) oscl_malloc(sizeof(bool) * num_buffers);
    if (iFrameUsed == NULL)
    {
        return 0;
    }

    if (iFramePtr) oscl_free(iFramePtr); // free previous one
    iFramePtr = (uint8**) oscl_malloc(sizeof(uint8*) * num_buffers);
    if (iFramePtr == NULL)
    {
        return 0;
    }

    iFramePtr[0] = iDPB;
    iFrameUsed[0] = false;

    for (ii = 1; ii < (int)num_buffers; ii++)
    {
        iFrameUsed[ii] = false;
        iFramePtr[ii] = iFramePtr[ii-1] + frame_size;
    }

    return 1;
}

/* ///////////////////////////////////////////////////////////////////////// */
void PVAVCEncoder::AVC_FrameUnbind(int indx)
{
    if (indx < iNumFrames)
    {
        iFrameUsed[indx] = false;
    }

    return ;
}

/* ///////////////////////////////////////////////////////////////////////// */
int PVAVCEncoder::AVC_FrameBind(int indx, uint8** yuv)
{
    if ((iFrameUsed[indx] == true) || (indx >= iNumFrames))
    {
        return 0; // already in used
    }

    iFrameUsed[indx] = true;
    *yuv = iFramePtr[indx];

    return 1;
}

