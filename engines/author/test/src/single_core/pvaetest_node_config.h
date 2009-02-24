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
 * @file pvaetest_node_config.h
 * @brief Utility to perform build configuration specific node configuration. This file
 * is for tye single core build configurations.
 */

#ifndef PVAETEST_NODE_CONFIG_H_INCLUDED
#define PVAETEST_NODE_CONFIG_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef PVAETESTINPUT_H_INCLUDED
#include "pvaetestinput.h"
#endif
#ifndef PVMF_AUDIO_ENCNODE_EXTENSION_H_INCLUDED
#include "pvmf_audio_encnode_extension.h"
#endif
#ifndef TEST_PV_AUTHOR_ENGINE_TYPEDEFS_H
#include "test_pv_author_engine_typedefs.h"
#endif

extern const uint32 KNum20msFramesPerChunk;
extern const uint32 KAudioBitrate;
extern const uint32 KAACAudioBitrate;

class PVAETestNodeConfig
{
    public:
        static bool ConfigureAudioEncoder(PVInterface* aInterface, const PvmfMimeString& aMimeType, uint32 aAudioBitrate = 0)
        {
            if (!aInterface)
                return true;

            PVInterface* myInterface;

            if (!aInterface->queryInterface(PVAudioEncExtensionUUID, myInterface))
                return false;

            PVAudioEncExtensionInterface* config = OSCL_STATIC_CAST(PVAudioEncExtensionInterface*, aInterface);

            if (aMimeType.get_cstr() == (char*)KAMRNbEncMimeType || aMimeType.get_cstr() == (char*)KAMRWbEncMimeType)
            {
                config->SetMaxNumOutputFramesPerBuffer(KNum20msFramesPerChunk);
                uint32 audioBitrate = aAudioBitrate;

                if (audioBitrate == 0)
                {
                    audioBitrate = KAudioBitrate;
                }

                switch (audioBitrate)
                {
                    case 4750:
                        config->SetOutputBitRate(GSM_AMR_4_75);
                        break;
                    case 5150:
                        config->SetOutputBitRate(GSM_AMR_5_15);
                        break;
                    case 5900:
                        config->SetOutputBitRate(GSM_AMR_5_90);
                        break;
                    case 6700:
                        config->SetOutputBitRate(GSM_AMR_6_70);
                        break;
                    case 7400:
                        config->SetOutputBitRate(GSM_AMR_7_40);
                        break;
                    case 7950:
                        config->SetOutputBitRate(GSM_AMR_7_95);
                        break;
                    case 10200:
                        config->SetOutputBitRate(GSM_AMR_10_2);
                        break;
                    case 12200:
                        config->SetOutputBitRate(GSM_AMR_12_2);
                        break;
                    default:
                        return false;
                }
            }
            else if (aMimeType == KAACADIFEncMimeType || aMimeType == KAACADTSEncMimeType)
            {
                config->SetOutputBitRate(KAACAudioBitrate);
                //config->SetOutputNumChannel();  do not set, use the input ones
                //config->SetOutputSamplingRate();
            }

            return true;
        }
};

#endif // PVAETEST_NODE_CONFIG_H_INCLUDED



