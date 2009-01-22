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
/**
	@file omx_aacenc_component.h
	OpenMax encoder component file.
*/

#ifndef OMX_AACENC_COMPONENT_H_INCLUDED
#define OMX_AACENC_COMPONENT_H_INCLUDED

#ifndef PV_OMXCOMPONENT_H_INCLUDED
#include "pv_omxcomponent.h"
#endif

#ifndef AAC_DEC_H_INCLUDED
#include "aac_enc.h"
#endif

#ifndef AACENC_TIMESTAMP_H_INCLUDED
#include "aacenc_timestamp.h"
#endif


#define INPUT_BUFFER_SIZE_AAC_ENC (INPUTBUFFER_SIZE * 2)

#define OUTPUT_BUFFER_SIZE_AAC_ENC OUTPUTBUFFER_SIZE

#define NUMBER_INPUT_BUFFER_AAC_ENC  5
#define NUMBER_OUTPUT_BUFFER_AAC_ENC  2


class OmxComponentAacEncoderAO : public OmxComponentAudio
{
    public:

        OmxComponentAacEncoderAO();
        ~OmxComponentAacEncoderAO();


        OMX_ERRORTYPE ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy);
        OMX_ERRORTYPE DestroyComponent();

        OMX_ERRORTYPE ComponentInit();
        OMX_ERRORTYPE ComponentDeInit();

        static void ComponentGetRolesOfComponent(OMX_STRING* aRoleString);

        void ProcessData();

        void ProcessInBufferFlag();
        void SyncWithInputTimestamp();

    private:

        OMX_U32			iInputFrameLength;

        AacEncTimeStampCalc iCurrentFrameTS;
        OmxAacEncoder*  ipAacEnc;
};

#endif // OMX_AACENC_COMPONENT_H_INCLUDED
