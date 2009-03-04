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
/*
////////////////////////////////////////////////////////////////////////////////
//																				//
//	File: pvm4vencoder.cpp                                                      //
//																				//
//  Modified: 01/15/03									//
//			  Note:: Black in RGB is (16,128,128) in YUV						//
//																				//
//  Modified: 04/09/03				 					    //
//			  Note:: Optimized function RGB2YUV420_12bit()						//
//////////////////////////////////////////////////////////////////////////////////
*/

#include "pvm4vencoder.h"
#include "oscl_mem.h"

#include "oscl_dll.h"
OSCL_DLL_ENTRY_POINT_DEFAULT()


/* ///////////////////////////////////////////////////////////////////////// */
CPVM4VEncoder::CPVM4VEncoder()
{
//iEncoderControl
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF CPVM4VEncoder::~CPVM4VEncoder()
{
    Cancel(); /* CTimer function */
    Terminate();
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF CPVM4VEncoder* CPVM4VEncoder::New(int32 aThreadId)
{
    CPVM4VEncoder* self = new CPVM4VEncoder;
    if (self && self->Construct(aThreadId))
        return self;
    if (self)
        OSCL_DELETE(self);
    return NULL;
}

/* ///////////////////////////////////////////////////////////////////////// */
bool CPVM4VEncoder::Construct(int32 aThreadId)
{
    oscl_memset((void *)&iEncoderControl, 0, sizeof(VideoEncControls));
    iInitialized = false;
    iObserver = NULL;
    iNumOutputData = 0;
    iYUVIn = NULL;
    for (int i = 0; i < KCVEIMaxOutputBuffer; i++)
    {
        iOutputData[i] = NULL;
    }
    iState = EIdle;

    if (aThreadId >= 0)
        AddToScheduler();

    return true;
}

/* ///////////////////////////////////////////////////////////////////////// */
void CPVM4VEncoder::DoCancel()
{
    /* called when Cancel() is called.*/
    // They use Stop for PVEngine.cpp in PVPlayer.
    return ;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::SetObserver(MPVCVEIObserver *aObserver)
{
    iObserver = aObserver;
    return ECVEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::AddBuffer(TPVVideoOutputData *aVidOut)
{
    if (iNumOutputData >= KCVEIMaxOutputBuffer)
    {
        return ECVEI_FAIL;
    }

    iOutputData[iNumOutputData++] = aVidOut;

    return ECVEI_SUCCESS;
}


/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::Encode(TPVVideoInputData *aVidIn)
{
    ULong modTime;
    VideoEncFrameIO vid_in;

    if (iState != EIdle || iObserver == NULL)
    {
        return ECVEI_FAIL;
    }

    if (aVidIn->iTimeStamp >= iNextModTime)
    {
        if (iVideoFormat == ECVEI_YUV420)
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
                iVideoIn = aVidIn->iSource;  //  Sept 14, 2005 */
            }
        }
#else
            return ECVEI_FAIL;
#endif
        if (iVideoFormat == ECVEI_RGB12)
#ifdef RGB12_INPUT
        {
            RGB2YUV420_12bit((uint32 *)aVidIn->iSource, iSrcWidth, iSrcHeight,
            ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
            iVideoIn = iYUVIn;
        }
#else
            return ECVEI_FAIL;
#endif
        if (iVideoFormat == ECVEI_RGB24)
#ifdef RGB24_INPUT
        {
            RGB2YUV420_24bit(aVidIn->iSource, iSrcWidth, iSrcHeight,
            ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
            iVideoIn = iYUVIn;
        }
#else
            return ECVEI_FAIL;
#endif

        /* assign with backward-P or B-Vop this timestamp must be re-ordered */
        iTimeStamp = aVidIn->iTimeStamp;

        modTime = iTimeStamp;

#ifdef NO_SLICE_ENCODE
        return ECVEI_FAIL;
#else
        vid_in.height = ((iSrcHeight + 15) >> 4) << 4;
        vid_in.pitch = ((iSrcWidth + 15) >> 4) << 4;
        vid_in.timestamp = modTime;
        vid_in.yChan = (UChar*)iVideoIn;
        vid_in.uChan = (UChar*)(iVideoIn + vid_in.height * vid_in.pitch);
        vid_in.vChan = vid_in.uChan + ((vid_in.height * vid_in.pitch) >> 2);

        /*status = */
        (int) PVEncodeFrameSet(&iEncoderControl, &vid_in, &modTime, &iNumLayer);
#endif

        iState = EEncode;
        RunIfNotReady();

        return ECVEI_SUCCESS;
    }
    else /* if(aVidIn->iTimeStamp >= iNextModTime) */
    {
        iTimeStamp = aVidIn->iTimeStamp;
        iNumLayer = -1;
        iState = EEncode;
        RunIfNotReady();
        return ECVEI_SUCCESS;
    }
}

/* ///////////////////////////////////////////////////////////////////////// */
void CPVM4VEncoder::Run()
{
#ifndef NO_SLICE_ENCODE

    //Bool status;
    Int Size, endOfFrame = 0;
    ULong modTime;
    int32 oindx;
    VideoEncFrameIO vid_out;

    switch (iState)
    {
        case EEncode:
            /* find available bitstream */
            if (iNumOutputData <= 0)
            {
                iObserver->HandlePVCVEIEvent(iId, ECVEI_NO_BUFFERS);
                RunIfNotReady(50);
                break;
            }
            oindx	=	--iNumOutputData;						/* last-in first-out */
            Size	=	iOutputData[oindx]->iBitStreamSize;
            iOutputData[oindx]->iExternalTimeStamp	= iTimeStamp;
            iOutputData[oindx]->iVideoTimeStamp		= iTimeStamp;
            iOutputData[oindx]->iFrame				= iVideoOut;

            if (iNumLayer == -1)
            {
                iOutputData[oindx]->iBitStreamSize = 0;
                iOutputData[oindx]->iLayerNumber = iNumLayer;
                iState = EIdle;
                iObserver->HandlePVCVEIEvent(iId, ECVEI_FRAME_DONE, (uint32)iOutputData[oindx]);
                break;
            }

            /*status = */
            (int) PVEncodeSlice(&iEncoderControl, (UChar*)iOutputData[oindx]->iBitStream, &Size,
                                &endOfFrame, &vid_out, &modTime);

            iOutputData[oindx]->iBitStreamSize = Size;
            iOutputData[oindx]->iLayerNumber = iNumLayer;
            if (endOfFrame != 0) /* done with this frame */
            {
                iNextModTime = modTime;
                iOutputData[oindx]->iFrame = iVideoOut = (uint8*)vid_out.yChan;
                iOutputData[oindx]->iVideoTimeStamp = vid_out.timestamp;

                if (endOfFrame == -1) /* pre-skip */
                {
                    iOutputData[oindx]->iLayerNumber = -1;
                }
                else
                {
                    PVGetHintTrack(&iEncoderControl, &(iOutputData[oindx]->iHintTrack));
                }
                iState = EIdle;
                iObserver->HandlePVCVEIEvent(iId, ECVEI_FRAME_DONE, (uint32)iOutputData[oindx]);
            }
            else
            {
                RunIfNotReady();
                iObserver->HandlePVCVEIEvent(iId, ECVEI_BUFFER_READY, (uint32)iOutputData[oindx]);
            }

            break;
        default:
            break;
    }
#endif /* NO_SLICE_ENCODE */

    return ;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::Initialize(TPVVideoInputFormat *aVidInFormat, TPVVideoEncodeParam *aEncParam)
{

    int /*status,*/i;
    MP4EncodingMode	ENC_Mode ;
    ParamEncMode RvlcMode = PV_OFF; /* default no RVLC */
    Int	quantType[2] = {0, 0};		/* default H.263 quant*/
    VideoEncOptions aEncOption; /* encoding options */

    iState = EIdle ; // stop encoding
    iId = aEncParam->iEncodeID;

    if (aEncParam->iContentType == 	ECVEI_STREAMING)
    {
        ENC_Mode = DATA_PARTITIONING_MODE;
    }
    else if (aEncParam->iContentType == ECVEI_DOWNLOAD)
    {
        if (aEncParam->iPacketSize > 0)
        {
            ENC_Mode = COMBINE_MODE_WITH_ERR_RES;
        }
        else
        {
            ENC_Mode = COMBINE_MODE_NO_ERR_RES;
        }
    }
    else if (aEncParam->iContentType == ECVEI_H263)
    {
        if (aEncParam->iPacketSize > 0)
        {
            ENC_Mode = H263_MODE_WITH_ERR_RES;
        }
        else
        {
            ENC_Mode = H263_MODE;
        }
    }
    else
    {
        return ECVEI_FAIL;
    }

    iSrcWidth = aVidInFormat->iFrameWidth;
    iSrcHeight = aVidInFormat->iFrameHeight;
    iSrcFrameRate = (int) aVidInFormat->iFrameRate;
    iVideoFormat = (TPVVideoFormat) aVidInFormat->iVideoFormat;
    iFrameOrientation = aVidInFormat->iFrameOrientation;

    if (iInitialized == true)  /* clean up before re-initialized */
    {
        /*status = */(int) PVCleanUpVideoEncoder(&iEncoderControl);
        if (iYUVIn)
        {
            oscl_free(iYUVIn);
            iYUVIn = NULL;
        }
        if (iVideoFormat == ECVEI_RGB24 || iVideoFormat == ECVEI_RGB12)
        {
#if defined(RGB24_INPUT)||defined(RGB12_INPUT)
            freeRGB2YUVTables();
#else
            return ECVEI_FAIL;
#endif
        }
    }

    // allocate iYUVIn
    if (((iSrcWidth&0xF) || (iSrcHeight&0xF)) || iVideoFormat != ECVEI_YUV420) /* Not multiple of 16 */
    {
        iYUVIn = (uint8*) oscl_malloc(((((iSrcWidth + 15) >> 4) * ((iSrcHeight + 15) >> 4)) * 3) << 7);
        if (iYUVIn == NULL)
        {
            return ECVEI_FAIL;
        }
    }

    // check the buffer delay according to the clip duration
    if (aEncParam->iClipDuration > 0 && aEncParam->iRateControlType == EVBR_1)
    {
        if (aEncParam->iBufferDelay > aEncParam->iClipDuration / 10000.0)   //enforce 10% variation of the clip duration as the bound of buffer delay
        {
            aEncParam->iBufferDelay = aEncParam->iClipDuration / (float)10000.0;
        }
    }

    /* Initialize the color conversion table if needed*/
    if (iVideoFormat == ECVEI_RGB24 || iVideoFormat == ECVEI_RGB12)
    {
#if defined(RGB24_INPUT)||defined(RGB12_INPUT)
        initRGB2YUVTables();
#else
        return ECVEI_FAIL;
#endif
    }

    PVGetDefaultEncOption(&aEncOption, 0);

    for (i = 0; i < aEncParam->iNumLayer; i++)
    {
        aEncOption.encWidth[i] = iEncWidth[i] = aEncParam->iFrameWidth[i];
        aEncOption.encHeight[i] = iEncHeight[i] = aEncParam->iFrameHeight[i];
        aEncOption.encFrameRate[i] = iEncFrameRate[i] = aEncParam->iFrameRate[i];
    }

    if (aEncParam->iRateControlType == ECONSTANT_Q)
        aEncOption.rcType = CONSTANT_Q;
    else if (aEncParam->iRateControlType == ECBR_1)
        aEncOption.rcType = CBR_1;
    else if (aEncParam->iRateControlType == EVBR_1)
        aEncOption.rcType = VBR_1;
    else
        return ECVEI_FAIL;

    // Check the bitrate, framerate, image size and buffer delay for 3GGP compliance
#ifdef FOR_3GPP_COMPLIANCE
    Check3GPPCompliance(aEncParam, iEncWidth, iEncHeight);
#endif

    Int profile_level = (Int)ECVEI_CORE_LEVEL2;
    if (aEncParam->iNumLayer > 1) profile_level = (Int)ECVEI_CORE_SCALABLE_LEVEL3;


    aEncOption.encMode = ENC_Mode;
    aEncOption.packetSize = aEncParam->iPacketSize;
    aEncOption.profile_level = (ProfileLevelType)profile_level;
    aEncOption.rvlcEnable = RvlcMode;
    aEncOption.numLayers = aEncParam->iNumLayer;
    aEncOption.timeIncRes = 1000;
    aEncOption.tickPerSrc = (int)(1000 / aVidInFormat->iFrameRate + 0.5);

    for (i = 0; i < aEncParam->iNumLayer; i++)
    {
        aEncOption.bitRate[i] = aEncParam->iBitRate[i];
        aEncOption.iQuant[i] = aEncParam->iIquant[i];
        aEncOption.pQuant[i] = aEncParam->iPquant[i];
        aEncOption.quantType[i] = quantType[i]; /* default to H.263 */
    }

    aEncOption.vbvDelay = (float)aEncParam->iBufferDelay;
    aEncOption.intraPeriod = aEncParam->iIFrameInterval;
    aEncOption.numIntraMB = aEncParam->iNumIntraMBRefresh;
    aEncOption.sceneDetect = (aEncParam->iSceneDetection == true) ? PV_ON : PV_OFF;
    aEncOption.noFrameSkipped = (aEncParam->iNoFrameSkip == true) ? PV_ON : PV_OFF;
    aEncOption.searchRange = aEncParam->iSearchRange;
    aEncOption.mv8x8Enable = (aEncParam->iMV8x8 == true) ? PV_ON : PV_OFF;

    if (PV_FALSE == PVInitVideoEncoder(&iEncoderControl, &aEncOption))
    {
        goto FAIL;
    }

    iNextModTime = 0;
    iInitialized = true;
    return ECVEI_SUCCESS;

FAIL:
    iInitialized = false;
    return ECVEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF int32 CPVM4VEncoder::GetBufferSize()
{
    Int bufSize = 0;

    //PVGetVBVSize(&iEncoderControl,&bufSize);
    PVGetMaxVideoFrameSize(&iEncoderControl, &bufSize);

    return (int32) bufSize;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF int32 CPVM4VEncoder::GetEncodeWidth(int32 aLayer)
{
    return (int32)iEncWidth[aLayer];
}

OSCL_EXPORT_REF int32 CPVM4VEncoder::GetEncodeHeight(int32 aLayer)
{
    return (int32)iEncHeight[aLayer];
}

OSCL_EXPORT_REF float CPVM4VEncoder::GetEncodeFrameRate(int32 aLayer)
{
    return iEncFrameRate[aLayer];
}
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::GetVolHeader(uint8 *volHeader, int32 *size, int32 layer)
{
    Int aSize, aLayer = layer;

    if (iInitialized == false) /* has to be initialized first */
        return ECVEI_FAIL;

    aSize = *size;
    if (PVGetVolHeader(&iEncoderControl, (UChar*)volHeader, &aSize, aLayer) == PV_FALSE)
        return ECVEI_FAIL;

    *size = aSize;
    return ECVEI_SUCCESS;
}

#ifdef PVAUTHOR_PROFILING
#include "pvauthorprofile.h"
#endif

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::EncodeFrame(TPVVideoInputData  *aVidIn, TPVVideoOutputData *aVidOut
#ifdef PVAUTHOR_PROFILING
        , void *aParam1
#endif
                                                       )
{

    Bool status;
    Int Size;
    Int nLayer = 0;
    ULong modTime;
    VideoEncFrameIO vid_in, vid_out;

    if (aVidIn->iTimeStamp >= iNextModTime) /* time to encode */
    {
        iState = EIdle; /* stop current encoding */

        Size = aVidOut->iBitStreamSize;

#ifdef PVAUTHOR_PROFILING
        if (aParam1)((CPVAuthorProfile*)aParam1)->Start();
#endif

        if (iVideoFormat == ECVEI_YUV420)
#ifdef YUV_INPUT
        {
            if (iYUVIn) /* iSrcWidth or iSrcHeight is not multiple of 16 */
            {
                CopyToYUVIn(aVidIn->iSource, iSrcWidth, iSrcHeight,
                ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
                iVideoIn = iYUVIn;
            }
            else /* otherwise, we can just use aVidIn->iSource */
            {
                iVideoIn = aVidIn->iSource;  //  Sept 14, 2005 */
            }
        }
#else
            return ECVEI_FAIL;
#endif
        else if (iVideoFormat == ECVEI_RGB12)
#ifdef RGB12_INPUT
        {
            RGB2YUV420_12bit((uint32 *)aVidIn->iSource, iSrcWidth, iSrcHeight,
            ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
            iVideoIn = iYUVIn;
        }
#else
            return ECVEI_FAIL;
#endif

        else if (iVideoFormat == ECVEI_RGB24)
#ifdef RGB24_INPUT
        {
            RGB2YUV420_24bit(aVidIn->iSource, iSrcWidth, iSrcHeight,
            ((iSrcWidth + 15) >> 4) << 4, ((iSrcHeight + 15) >> 4) << 4);
            iVideoIn = iYUVIn;
        }
#else
            return ECVEI_FAIL;
#endif

#ifdef PVAUTHOR_PROFILING
        if (aParam1)((CPVAuthorProfile*)aParam1)->Stop(CPVAuthorProfile::EColorInput);
#endif

#ifdef PVAUTHOR_PROFILING
        if (aParam1)((CPVAuthorProfile*)aParam1)->Start();
#endif

        /* with backward-P or B-Vop this timestamp must be re-ordered */
        aVidOut->iExternalTimeStamp = aVidIn->iTimeStamp;
        aVidOut->iVideoTimeStamp = aVidOut->iExternalTimeStamp;

        vid_in.height = ((iSrcHeight + 15) >> 4) << 4;
        vid_in.pitch = ((iSrcWidth + 15) >> 4) << 4;
        vid_in.timestamp = aVidIn->iTimeStamp;
        vid_in.yChan = (UChar*)iVideoIn;
        vid_in.uChan = (UChar*)(iVideoIn + vid_in.height * vid_in.pitch);
        vid_in.vChan = vid_in.uChan + ((vid_in.height * vid_in.pitch) >> 2);

        status = PVEncodeVideoFrame(&iEncoderControl,
                                    &vid_in, &vid_out, &modTime, (UChar*)aVidOut->iBitStream, &Size, &nLayer);

        if (status == PV_TRUE)
        {
            iNextModTime = modTime;

            aVidOut->iLayerNumber = nLayer;
            aVidOut->iBitStreamSize = Size;
            aVidOut->iFrame = iVideoOut = (uint8*)vid_out.yChan;
            aVidOut->iVideoTimeStamp = vid_out.timestamp;

            PVGetHintTrack(&iEncoderControl, &aVidOut->iHintTrack);

#ifdef PVAUTHOR_PROFILING
            if (aParam1)((CPVAuthorProfile*)aParam1)->Stop(CPVAuthorProfile::EVideoEncode);
#endif
            return ECVEI_SUCCESS;
        }
        else
            return ECVEI_FAIL;
    }
    else /* if(aVidIn->iTimeStamp >= iNextModTime) */
    {
        aVidOut->iLayerNumber = -1;
        aVidOut->iBitStreamSize = 0;
#ifdef PVAUTHOR_PROFILING
        if (aParam1)((CPVAuthorProfile*)aParam1)->AddVal
            (CPVAuthorProfile::EVidSkip, iNextModTime - aVidIn->iTimeStamp);
#endif
        return ECVEI_SUCCESS;
    }
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::FlushOutput(TPVVideoOutputData *aVidOut)
{
    OSCL_UNUSED_ARG(aVidOut);
    return ECVEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
TCVEI_RETVAL CPVM4VEncoder::Terminate()
{
    iState = EIdle; /* stop current encoding */
    if (iInitialized == true)
    {
        PVCleanUpVideoEncoder(&iEncoderControl);
        iInitialized = false;

        if (iYUVIn)
        {
            oscl_free(iYUVIn);
            iYUVIn = NULL;
        }

#if defined(RGB12_INPUT)||defined(RGB24_INPUT)
        if (iVideoFormat == ECVEI_RGB24 || iVideoFormat == ECVEI_RGB12)
            freeRGB2YUVTables();
#endif
    }
    return ECVEI_SUCCESS;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::UpdateBitRate(int32 aNumLayer, int32 *aBitRate)
{
#ifndef LIMITED_API
    Int i, bitRate[2] = {0, 0};

    for (i = 0; i < aNumLayer; i++)
    {
        bitRate[i] = aBitRate[i];
    }

    if (PVUpdateBitRate(&iEncoderControl, &bitRate[0]) == PV_TRUE)
        return ECVEI_SUCCESS;
    else
#endif
        return ECVEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::UpdateFrameRate(int32 aNumLayer, float *aFrameRate)
{
    OSCL_UNUSED_ARG(aNumLayer);
#ifndef LIMITED_API
    if (PVUpdateEncFrameRate(&iEncoderControl, aFrameRate) == PV_TRUE)
        return ECVEI_SUCCESS;
    else
#else
    OSCL_UNUSED_ARG(aFrameRate);
#endif
        return ECVEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::UpdateIFrameInterval(int32 aIFrameInterval)
{
#ifndef LIMITED_API
    if (PVUpdateIFrameInterval(&iEncoderControl, aIFrameInterval) == PV_TRUE)
        return ECVEI_SUCCESS;
    else
#endif
        return ECVEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::IFrameRequest()
{
#ifndef LIMITED_API
    if (PVIFrameRequest(&iEncoderControl) == PV_TRUE)
        return ECVEI_SUCCESS;
    else
#endif
        return ECVEI_FAIL;
}

/* ///////////////////////////////////////////////////////////////////////// */
OSCL_EXPORT_REF TCVEI_RETVAL CPVM4VEncoder::SetIntraMBRefresh(int32 aNumMBRefresh)
{
#ifndef LIMITED_API
    if (PVUpdateNumIntraMBRefresh(&iEncoderControl, aNumMBRefresh) == PV_TRUE)
        return ECVEI_SUCCESS;
    else
#endif
        return ECVEI_FAIL;
}

#ifdef YUV_INPUT
/* ///////////////////////////////////////////////////////////////////////// */
/* Copy from YUV input to YUV frame inside M4VEnc lib						*/
/* When input is not YUV, the color conv will write it directly to iVideoInOut. */
/* ///////////////////////////////////////////////////////////////////////// */

void CPVM4VEncoder::CopyToYUVIn(uint8 *YUV, Int width, Int height, Int width_16, Int height_16)
{
    UChar *y, *u, *v, *yChan, *uChan, *vChan;
    Int y_ind, ilimit, jlimit, i, j, ioffset;
    Int size = width * height;
    Int size16 = width_16 * height_16;

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16) /* if padding */
    {
        Int offset = (height < height_16) ? height : height_16;

        offset = (offset * width_16);

        if (width < width_16)
        {
            offset -= (width_16 - width);
        }

        yChan = (UChar*)(iYUVIn + offset);
        oscl_memset(yChan, 16, size16 - offset); /* pad with zeros */

        uChan = (UChar*)(iYUVIn + size16 + (offset >> 2));
        oscl_memset(uChan, 128, (size16 - offset) >> 2);

        vChan = (UChar*)(iYUVIn + size16 + (size16 >> 2) + (offset >> 2));
        oscl_memset(vChan, 128, (size16 - offset) >> 2);
    }

    /* then do padding on the top */
    yChan = (UChar*)iYUVIn; /* Normal order */
    uChan = (UChar*)(iYUVIn + size16);
    vChan = (UChar*)(uChan + (size16 >> 2));

    u = (UChar*)(&(YUV[size]));
    v = (UChar*)(&(YUV[size*5/4]));

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
    y = (UChar*)YUV;

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
TCVEI_RETVAL CPVM4VEncoder::initRGB2YUVTables()
{
    Int i;
    uint16 *pTable;

    iY_Table  = NULL;
    iCb_Table = iCr_Table = ipCb_Table = ipCr_Table = NULL;

    /* memory allocation */
    if ((iY_Table = (uint8*)oscl_malloc(384)) == NULL)
        return ECVEI_FAIL;

    if ((iCb_Table = (uint16*)oscl_malloc(768 * 2)) == NULL)
        return ECVEI_FAIL;

    if ((iCr_Table = (uint16*)oscl_malloc(768 * 2)) == NULL)
        return ECVEI_FAIL;

#define pv_max(a, b)	((a) >= (b) ? (a) : (b))
#define pv_min(a, b)	((a) <= (b) ? (a) : (b))

    /* Table generation */
    for (i = 0; i < 384; i++)
        iY_Table[i] = (uint8) pv_max(pv_min(255, (Int)(0.7152 * i + 16 + 0.5)), 0);

    pTable = iCb_Table + 384;
    for (i = -384; i < 384; i++)
        pTable[i] = (uint16) pv_max(pv_min(255, (Int)(0.386 * i + 128 + 0.5)), 0);
    ipCb_Table = iCb_Table + 384;

    pTable = iCr_Table + 384;
    for (i = -384; i < 384; i++)
        pTable[i] = (uint16) pv_max(pv_min(255, (Int)(0.454 * i + 128 + 0.5)), 0);
    ipCr_Table = iCr_Table + 384;

    return ECVEI_SUCCESS;

}

void CPVM4VEncoder::freeRGB2YUVTables()
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
void CPVM4VEncoder::RGB2YUV420_12bit(uint16 *inputRGB, Int width, Int height, Int width_16, Int height_16)
{
    Int i, j;
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
void CPVM4VEncoder::RGB2YUV420_12bit(
    uint32 *inputRGB,
    Int     width,
    Int     height,
    Int     width_16,
    Int     height_16)
{
    Int i;
    Int j;
    Int ilimit;
    Int jlimit;
    uint32 *tempY;
    uint32 *tempU;
    uint32 *tempV;
    uint32 pixels;
    uint32 pixels_nextRow;
    uint32 yuv_value;
    uint32 yuv_value1;
    uint32 yuv_value2;
    Int R_ds; /* "_ds" is the downsample version */
    Int G_ds; /* "_ds" is the downsample version */
    Int B_ds; /* "_ds" is the downsample version */
    Int adjust = (width >> 1);
    Int size16 = height_16 * width_16;
    Int tmp;
    uint32 temp;

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16)
    { /* if padding */
        Int offset = (height < height_16) ? height : height_16;

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
void CPVM4VEncoder::RGB2YUV420_24bit(uint8 *inputRGB, Int width, Int height, Int width_16, Int height_16)
{
    Int i, j, ilimit, jlimit;
    uint8 *tempY, *tempU, *tempV;
    uint8 *inputRGB_prevRow = NULL;
    int32 size16 = height_16 * width_16;
    int32 adjust = (width + (width << 1));

    /* do padding at the bottom first */
    /* do padding if input RGB size(height) is different from the output YUV size(height_16) */
    if (height < height_16 || width < width_16) /* if padding */
    {
        Int offset = (height < height_16) ? height : height_16;

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

    if (iFrameOrientation > 0)
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
void CPVM4VEncoder::Check3GPPCompliance(TPVVideoEncodeParam *aEncParam, Int *aEncWidth, Int *aEncHeight)
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

