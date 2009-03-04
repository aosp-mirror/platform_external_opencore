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
#ifndef PVMF_OMX_AUDIODEC_NODE_EXTENSION_INTERFACE_H_INCLUDED
#define PVMF_OMX_AUDIODEC_NODE_EXTENSION_INTERFACE_H_INCLUDED

struct PVMFOMXAudioDecNodeConfig
{
    bool iPostProcessingEnable;
    int32 iPostProcessingMode;
    bool iDropFrame;
    uint32 iMimeType;
};

//Mimetype and Uuid for the custom interface
#define PVMF_OMX_AUDIO_DEC_NODE_CUSTOM1_UUID 2,3,4,0xef,0xbe,0xbf,0xdf,0x30,0x21,0x73,0x33

//Mimetypes for the custom interface
#define PVMF_OMX_AUDIO_DEC_NODE_CUSTOM1_MIMETYPE "pvxxx/OMXAudioDecNode/Custom1"

class PVMFOMXAudioDecNodeExtensionInterface : public PVInterface
{
    public:
        //**********begin PVInterface
        virtual void addRef() = 0;
        virtual void removeRef() = 0;
        virtual bool queryInterface(const PVUuid& uuid, PVInterface*& iface) = 0;
        //**********end PVInterface

        virtual PVMFStatus SetDecoderNodeConfiguration(PVMFOMXAudioDecNodeConfig& aConfig) = 0;

};

#endif //PVMF_OMX_AUDIODEC_NODE_EXTENSION_INTERFACE_H_INCLUDED


