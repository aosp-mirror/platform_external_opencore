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

#include "mpeg4_dec.h"
#include "oscl_mem.h"
#include "omx_mpeg4_component.h"


#define MAX_LAYERS 1
#define PVH263DEFAULTHEIGHT 288
#define PVH263DEFAULTWIDTH 352

// from m4v_config_parser.h
OSCL_IMPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int length, int *width, int *height, int *, int *);

Mpeg4Decoder_OMX::Mpeg4Decoder_OMX()
{
    pFrame0 = NULL;
    pFrame1 = NULL;

    VO_START_CODE1[0] = 0x00;
    VO_START_CODE1[1] = 0x00;
    VO_START_CODE1[2] = 0x01;
    VO_START_CODE1[3] = 0x00;

    VOSH_START_CODE1[0] = 0x00;
    VOSH_START_CODE1[1] = 0x00;
    VOSH_START_CODE1[2] = 0x01;
    VOSH_START_CODE1[3] = 0xB0;

    VOP_START_CODE1[0] = 0x00;
    VOP_START_CODE1[1] = 0x00;
    VOP_START_CODE1[2] = 0x01;
    VOP_START_CODE1[3] = 0xB6;

    H263_START_CODE1[0] = 0x00;
    H263_START_CODE1[1] = 0x00;
    H263_START_CODE1[2] = 0x80;

}


/* Initialization routine */
OMX_ERRORTYPE Mpeg4Decoder_OMX::Mp4DecInit()
{
    Mpeg4InitFlag = 0;
    return OMX_ErrorNone;
}


/*Decode routine */
OMX_BOOL Mpeg4Decoder_OMX::Mp4DecodeVideo(OMX_U8* aOutBuffer, OMX_U32* aOutputLength,
        OMX_U8** aInputBuf, OMX_U32* aInBufSize,
        OMX_PARAM_PORTDEFINITIONTYPE* aPortParam,
        OMX_S32* aIsFirstBuffer, OMX_BOOL aMarkerFlag, OMX_BOOL *aResizeFlag)
{
    OMX_BOOL Status = OMX_TRUE;
    static OMX_S32 display_Width, display_Height;
    OMX_S32 OldWidth, OldHeight, OldFrameSize;

    OldWidth = aPortParam->format.video.nFrameWidth;
    OldHeight = aPortParam->format.video.nFrameHeight;
    *aResizeFlag = OMX_FALSE;

#ifdef _DEBUG
    static OMX_U32 FrameCount = 0;
#endif
    OMX_U32 UseExtTimestamp = 0;
    OMX_S32 TimeStamp;
    OMX_S32 MaxSize = BIT_BUFF_SIZE, FrameSize, InputSize, InitSize;
    OMX_U8* pTempFrame, *pSrc[3];

    if (Mpeg4InitFlag == 0)
    {
        if (!aMarkerFlag)
        {
            InitSize = m4v_getVideoHeader(0, *aInputBuf, *aInBufSize);
        }
        else
        {
            InitSize = *aInBufSize;
        }

        if (PV_TRUE != InitializeVideoDecode(&display_Width, &display_Height,
                                             aInputBuf, (OMX_S32*)aInBufSize, MPEG4_MODE))
            return OMX_FALSE;

        Mpeg4InitFlag = 1;
        aPortParam->format.video.nFrameWidth = display_Width;
        aPortParam->format.video.nFrameHeight = display_Height;
        if ((display_Width != OldWidth) || (display_Height != OldHeight))
            *aResizeFlag = OMX_TRUE;

        *aIsFirstBuffer = 1;
        *aInBufSize -= InitSize;
        return OMX_TRUE;
    }

    MaxSize = *aInBufSize;

    if ((* (OMX_S32*)aInBufSize) <= 0)
    {
        return OMX_FALSE;
    }

    TimeStamp = -1;
    InputSize = *aInBufSize;

    // in case of H263, read the 1st frame to find out the sizes (use the m4v_config)
    if ((0 == *aIsFirstBuffer) && (H263_MODE == CodecMode))
    {
        OMX_S32 aligned_width, aligned_height;
        if (iGetM4VConfigInfo(*aInputBuf, *aInBufSize, (int *) &aligned_width, (int *) &aligned_height, (int*) &display_Width, (int *) &display_Height))
            return OMX_FALSE;

        aPortParam->format.video.nFrameWidth = display_Width; // use non 16byte aligned values (display_width) for H263
        aPortParam->format.video.nFrameHeight = display_Height; // like in the case of M4V (PVGetVideoDimensions also returns display_width/height)
        if ((display_Width != OldWidth) || (display_Height != OldHeight))
            *aResizeFlag = OMX_TRUE;

        *aIsFirstBuffer = 1;
        return OMX_TRUE;
    }

    Status = (OMX_BOOL) PVDecodeVideoFrame(&VideoCtrl, aInputBuf,
                                           (uint32*) & TimeStamp,
                                           (int32*)aInBufSize,
                                           (uint32*) & UseExtTimestamp,
                                           (OMX_U8*) pFrame0);

    if (Status == PV_TRUE)
    {

#ifdef _DEBUG
        //printf("Frame number %d\n", ++FrameCount);
#endif
        // advance input buffer ptr
        *aInputBuf += (InputSize - *aInBufSize);

        pTempFrame = (OMX_U8*) pFrame0;
        pFrame0 = (OMX_U8*) pFrame1;
        pFrame1 = (OMX_U8*) pTempFrame;

        PVGetVideoDimensions(&VideoCtrl, (int32*) &display_Width, (int32*) &display_Height);
        if ((display_Width != OldWidth) || (display_Height != OldHeight))
        {

            aPortParam->format.video.nFrameWidth = display_Width;
            aPortParam->format.video.nFrameHeight = display_Height;
            *aResizeFlag = OMX_TRUE;
        }
        FrameSize = (((display_Width + 15) >> 4) << 4) * (((display_Height + 15) >> 4) << 4);
        OldFrameSize = (((OldWidth + 15) >> 4) << 4) * (((OldHeight + 15) >> 4) << 4);

        // THIS SHOULD NEVER HAPPEN, but just in case
        // check so to not write a larger output into a smaller buffer
        if (FrameSize <= OldFrameSize)
        {
            *aOutputLength = (FrameSize * 3) >> 1;

            pSrc[0] = VideoCtrl.outputFrame;
            pSrc[1] = pSrc[0] + FrameSize;
            pSrc[2] = pSrc[0] + FrameSize + FrameSize / 4;

            *aOutputLength = (FrameSize * 3) >> 1;

            oscl_memcpy(aOutBuffer, pSrc[0], FrameSize);
            oscl_memcpy(aOutBuffer + FrameSize, pSrc[1], FrameSize >> 2);
            oscl_memcpy(aOutBuffer + FrameSize + FrameSize / 4, pSrc[2], FrameSize >> 2);
        }
        else
        {
            *aOutputLength = 0;
        }


    }
    else
    {
        *aInBufSize = InputSize;
        *aOutputLength = 0;
    }

    return Status;
}

OMX_S32 Mpeg4Decoder_OMX::InitializeVideoDecode(
    OMX_S32* aWidth, OMX_S32* aHeight, OMX_U8** aBuffer, OMX_S32* aSize, OMX_S32 mode)
{
    OMX_U32 VideoDecOutputSize;
    OMX_S32 OK = PV_TRUE;
    CodecMode = MPEG4_MODE;

    if (mode == MODE_H263)
    {
        CodecMode = H263_MODE;
    }

    OK = PVInitVideoDecoder(&VideoCtrl, aBuffer, (int32*) aSize, 1,
                            PVH263DEFAULTWIDTH, PVH263DEFAULTHEIGHT, CodecMode);

    if (OK)
    {
        PVGetVideoDimensions(&VideoCtrl, (int32*) aWidth, (int32*) aHeight);
        CodecMode = PVGetDecBitstreamMode(&VideoCtrl);

        if (CodecMode == H263_MODE && (*aWidth == 0 || *aHeight == 0))
        {
            *aWidth = PVH263DEFAULTWIDTH;
            *aHeight = PVH263DEFAULTHEIGHT;
        }

        PVSetPostProcType(&VideoCtrl, 0);
        VideoDecOutputSize = (((*aWidth + 15) & - 16) * ((*aHeight + 15) & - 16) * 3) / 2;
        pFrame0 = (OMX_U8*) oscl_malloc(VideoDecOutputSize);
        pFrame1 = (OMX_U8*) oscl_malloc(VideoDecOutputSize);
        PVSetReferenceYUV(&VideoCtrl, pFrame1);
        return PV_TRUE;
    }
    else
    {
        return PV_FALSE;
    }


}

OMX_ERRORTYPE Mpeg4Decoder_OMX::Mp4DecDeinit()
{
    OMX_BOOL Status;

    if (pFrame0)
    {
        oscl_free(pFrame0);
        pFrame0 = NULL;
    }
    if (pFrame1)
    {
        oscl_free(pFrame1);
        pFrame1 = NULL;
    }

    Status = (OMX_BOOL) PVCleanUpVideoDecoder(&VideoCtrl);
    if (Status != OMX_TRUE)
    {
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}

