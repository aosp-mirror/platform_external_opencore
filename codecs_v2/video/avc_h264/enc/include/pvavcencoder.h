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
#ifndef PVAVCENCODER_H_INCLUDED
#define PVAVCENCODER_H_INCLUDED

#ifndef PVAVCENCODERINTERFACE_H_INCLUDED
#include "pvavcencoderinterface.h"
#endif

#ifndef AVCENC_API_H_INCLUDED
#include "avcenc_api.h"
#endif

/** AVC encoder class interface. See PVAVCEncoderInterface APIs for
virtual functions definitions. */
class PVAVCEncoder : public PVAVCEncoderInterface
{

    public:
        static PVAVCEncoder* New(void);
        virtual ~PVAVCEncoder();

        virtual TAVCEI_RETVAL Initialize(TAVCEIInputFormat* aVidInFormat, TAVCEIEncodeParam* aEncParam);
        virtual TAVCEI_RETVAL Initialize(TAVCEIInputFormat *aVidInFormat, TAVCEIEncodeParam *aEncParam,
                                         void* extSPS, void* extPPS);
        virtual TAVCEI_RETVAL Encode(TAVCEIInputData* aVidIn);
        virtual TAVCEI_RETVAL GetParameterSet(uint8* paramSet, int32* size, int* nalType);
        virtual TAVCEI_RETVAL GetOutput(TAVCEIOutputData* aVidOut);
        virtual TAVCEI_RETVAL FlushInput();
        virtual TAVCEI_RETVAL CleanupEncoder();

        virtual TAVCEI_RETVAL UpdateBitRate(int32* aBitRate);
        virtual TAVCEI_RETVAL UpdateFrameRate(OsclFloat* aFrameRate);
        virtual TAVCEI_RETVAL UpdateIDRFrameInterval(int32 aIFrameInterval);
        virtual TAVCEI_RETVAL IDRRequest();

        virtual int32 GetEncodeWidth(int32 aLayer);
        virtual int32 GetEncodeHeight(int32 aLayer);
        virtual OsclFloat GetEncodeFrameRate(int32 aLayer);

        /* for avc encoder lib callback functions */
        int		AVC_DPBAlloc(uint frame_size_in_mbs, uint num_buffers);
        int		AVC_FrameBind(int indx, uint8** yuv);
        void	AVC_FrameUnbind(int indx);

    private:

        PVAVCEncoder();
        bool Construct(void);
        TAVCEI_RETVAL Init(TAVCEIInputFormat *aVidInFormat, TAVCEIEncodeParam *aEncParam, AVCEncParams& aEncOption);

#ifdef	YUV_INPUT
        void CopyToYUVIn(uint8* YUV, int width, int height, int width_16, int height_16);
#endif
        /* RGB->YUV conversion */
#if defined(RGB12_INPUT)||defined(RGB24_INPUT)
        bool initRGB2YUVTables();
        void freeRGB2YUVTables();
#endif
        //void RGB2YUV420_12bit(uint16 *inputRGB, int width, int height,int width_16,int height_16);
#ifdef RGB12_INPUT
        void RGB2YUV420_12bit(uint32* inputRGB, int width, int height, int width_16, int height_16);
#endif
#ifdef RGB24_INPUT
        void RGB2YUV420_24bit(uint8* inputRGB, int width, int height, int width_16, int height_16);
#endif

#ifdef FOR_3GPP_COMPLIANCE
        void Check3GPPCompliance(TAVCEIEncodeParam* aEncParam, int* aEncWidth, int* aEncHeight);
#endif

        AVCProfile	mapProfile(TAVCEIProfile in);
        AVCLevel	mapLevel(TAVCEILevel out);

        /* internal enum */
        enum TAVCEncState
        {
            ECreated,
            EInitialized,
            EEncoding
        };

        TAVCEncState	iState;
        uint32		iId;

        /* Pure virtuals from OsclActiveObject implemented in this derived class */
        int		iSrcWidth;
        int		iSrcHeight;
        int		iFrameOrientation;
        OsclFloat		iSrcFrameRate;
        int		iEncWidth;
        int		iEncHeight;
        OsclFloat	iEncFrameRate;
        TAVCEIVideoFormat 	iVideoFormat;

        /* variables needed in operation */
        AVCHandle iAvcHandle;
        AVCFrameIO iVidIn;
        uint8*	iYUVIn;
        uint8*	iVideoIn;
        uint8*	iVideoOut;
        uint32	iTimeStamp;
        uint32  iPacketSize;
        bool	iIDR;
        int		iDispOrd;

        uint8*	iDPB;
        bool*	iFrameUsed;
        uint8** iFramePtr;
        int		iNumFrames;

        /* Tables in color coversion */
        uint8 *	iY_Table;
        uint16*	iCb_Table;
        uint16*	iCr_Table;
        uint16*	ipCb_Table;
        uint16*	ipCr_Table;


        int		iNumLayer;
};

#endif
