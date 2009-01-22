/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#ifndef AAC_ENC_H_INCLUDED
#define AAC_ENC_H_INCLUDED

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OMX_Component_h
#include "omx_component.h"
#endif

#ifndef _AAC_ENC_LIB_H_
#include "aacenc_lib.h"
#endif


class OmxAacEncoder
{
    public:
        OmxAacEncoder();

        OMX_BOOL AacEncInit(OMX_AUDIO_PARAM_PCMMODETYPE aPcmMode,
                            OMX_AUDIO_PARAM_AACPROFILETYPE aAacParam,
                            OMX_U32* aInputFrameLength);

        void AacEncDeinit();

        OMX_BOOL AacEncodeFrame(OMX_U8* aOutputBuffer,
                                OMX_U32* aOutputLength,
                                OMX_U8* aInBuffer,
                                OMX_U32 aInBufSize);

    private:

        OMX_S32 AacEncConfigDefaultSettings(ENC_CONFIGURATION* pConfig);
        //Encoder settings
        ENC_CONFIGURATION iAacEncConfig;
        HANDLE_AACENCODER iAacEncoder;

        OMX_U32 iAscSize;
        OMX_U8  iAscBuf[12];
        OMX_BOOL iAscFlag;

        OMX_U8* iAacInputBuffer;
        OMX_U8* iAacOutputBuffer;

};



#endif	//#ifndef AAC_ENC_H_INCLUDED

