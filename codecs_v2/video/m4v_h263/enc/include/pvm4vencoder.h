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
/*	File: pvm4vencoder.h                                                        */
/*	Purpose:																	*/
/*	Date:																		*/
/*	Revision History:															*/
/**	This file contains MP4 encoder related classes, structures and enumerations. */

#ifndef __PVM4VENCODER_H
#define __PVM4VENCODER_H

#include "cvei.h"
#include "mp4enc_api.h"

#define KCVEIMaxOutputBuffer	10

/** Encoding mode specific to MPEG4. */
enum TMP4EncodingMode
{
    /** H263 mode. */
    EH263_MODE,

    /** Data partitioning mode, packet size must be specified. */
    EDATA_PARTITIONG_MODE,

    /** Combined mode without resync markers. */
    ECOMBINING_MODE_NO_ERR_RES,

    /** COmbined mode with resync markers, packet size must be specified. */
    ECOMBINING_MODE_WITH_ERR_RES
};

/** Generic ON/OFF. */
enum TParamEncMode
{
    EPV_OFF,
    EPV_ON
};

/** MPEG4 encoder class interface. See CommonVideoEncoder APIs for
virtual functions definitions. */
class CPVM4VEncoder : public CommonVideoEncoder
{

    public:
        OSCL_IMPORT_REF static CPVM4VEncoder* New(int32 aThreadId);
        OSCL_IMPORT_REF ~CPVM4VEncoder();

        OSCL_IMPORT_REF virtual TCVEI_RETVAL SetObserver(MPVCVEIObserver *aObserver);
        OSCL_IMPORT_REF virtual TCVEI_RETVAL AddBuffer(TPVVideoOutputData *aVidOut);
        OSCL_IMPORT_REF virtual TCVEI_RETVAL Encode(TPVVideoInputData *aVidIn);

        OSCL_IMPORT_REF virtual TCVEI_RETVAL Initialize(TPVVideoInputFormat *aVidInFormat, TPVVideoEncodeParam *aEncParam);
        OSCL_IMPORT_REF virtual int32 GetBufferSize();
        OSCL_IMPORT_REF virtual TCVEI_RETVAL GetVolHeader(uint8 *volHeader, int32 *size, int32 layer);

        OSCL_IMPORT_REF virtual TCVEI_RETVAL EncodeFrame(TPVVideoInputData  *aVidIn, TPVVideoOutputData *aVidOut
#ifdef PVAUTHOR_PROFILING
                , void *aParam1 = 0
#endif
                                                        );

        OSCL_IMPORT_REF virtual TCVEI_RETVAL FlushOutput(TPVVideoOutputData *aVidOut);
        OSCL_IMPORT_REF virtual TCVEI_RETVAL Terminate();
        OSCL_IMPORT_REF virtual TCVEI_RETVAL UpdateBitRate(int32 aNumLayer, int32 *aBitRate);
        OSCL_IMPORT_REF virtual TCVEI_RETVAL UpdateFrameRate(int32 aNumLayer, float *aFrameRate);
        OSCL_IMPORT_REF virtual TCVEI_RETVAL UpdateIFrameInterval(int32 aIFrameInterval);
        OSCL_IMPORT_REF virtual TCVEI_RETVAL IFrameRequest();

        /** Set the forced number of intra macroblock per frame for error resiliency. */
        OSCL_IMPORT_REF TCVEI_RETVAL SetIntraMBRefresh(int32 aNumMBRefresh);

        OSCL_IMPORT_REF virtual int32 GetEncodeWidth(int32 aLayer);
        OSCL_IMPORT_REF virtual int32 GetEncodeHeight(int32 aLayer);
        OSCL_IMPORT_REF virtual float GetEncodeFrameRate(int32 aLayer);
    private:

        CPVM4VEncoder();
        bool Construct(int32 aThreadId);
#ifdef	YUV_INPUT
        void CopyToYUVIn(uint8 *YUV, int width, int height, int width_16, int height_16);
#endif
        /* RGB->YUV conversion */
#if defined(RGB12_INPUT)||defined(RGB24_INPUT)
        TCVEI_RETVAL initRGB2YUVTables();
        void freeRGB2YUVTables();
#endif
        //void RGB2YUV420_12bit(uint16 *inputRGB, int width, int height,int width_16,int height_16);
#ifdef RGB12_INPUT
        void RGB2YUV420_12bit(uint32 *inputRGB, int width, int height, int width_16, int height_16);
#endif
#ifdef RGB24_INPUT
        void RGB2YUV420_24bit(uint8 *inputRGB, int width, int height, int width_16, int height_16);
#endif

#ifdef FOR_3GPP_COMPLIANCE
        void Check3GPPCompliance(TPVVideoEncodeParam *aEncParam, int *aEncWidth, int *aEncHeight);
#endif

        /* Pure virtuals from OsclActiveObject implemented in this derived class */
        virtual void Run(void);
        virtual void DoCancel(void);

        MPVCVEIObserver *iObserver;

        int		iSrcWidth;
        int		iSrcHeight;
        int		iSrcFrameRate;
        int		iFrameOrientation;
        int		iEncWidth[4];
        int		iEncHeight[4];
        float	iEncFrameRate[4];
        TPVVideoFormat 	iVideoFormat;

        /* variables needed in operation */
        VideoEncControls iEncoderControl;
        bool	iInitialized;
        uint8   *iYUVIn;
        uint8	*iVideoIn;
        uint8	*iVideoOut;
        TPVVideoOutputData *iOutputData[KCVEIMaxOutputBuffer];
        int32		iNumOutputData;
        uint32		iTimeStamp;
        uint32		iNextModTime;

        /* Tables in color coversion */
        uint8  *iY_Table;
        uint16 *iCb_Table, *iCr_Table, *ipCb_Table, *ipCr_Table;


        int		iNumLayer;
};

#endif
