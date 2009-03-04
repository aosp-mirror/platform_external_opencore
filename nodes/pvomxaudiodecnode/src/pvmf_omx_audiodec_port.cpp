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
#include "pvmf_omx_audiodec_port.h"
#include "pvmf_omx_audiodec_node.h"


PVMFOMXAudioDecPort::PVMFOMXAudioDecPort(int32 aTag, PVMFNodeInterface* aNode, char*name)
        : PvmfPortBaseImpl(aTag, aNode, name)
{

    iOMXNode = (PVMFOMXAudioDecNode *) aNode;
    Construct();
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecPort::Construct()
{
    iLogger = PVLogger::GetLoggerObject("PVMFOMXAudioDecPort");
    oscl_memset(&iStats, 0, sizeof(PvmfPortBaseImplStats));
    iNumFramesGenerated = 0;
    iNumFramesConsumed = 0;
    iTrackConfig = NULL;
    iTrackConfigSize = 0;
    PvmiCapabilityAndConfigPortFormatImpl::Construct(
        PVMF_OMX_AUDIO_DEC_PORT_INPUT_FORMATS
        , PVMF_OMX_AUDIO_DEC_PORT_INPUT_FORMATS_VALTYPE);
}


PVMFOMXAudioDecPort::~PVMFOMXAudioDecPort()
{
    if (iTrackConfig != NULL)
    {
        OSCL_FREE(iTrackConfig);
        iTrackConfigSize = 0;
    }
    Disconnect();
    ClearMsgQueues();
}

////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecPort::IsFormatSupported(PVMFFormatType aFmt)
{

    return ((aFmt == PVMF_PCM16) ||
            (aFmt == PVMF_LATM) ||
            (aFmt == PVMF_MPEG4_AUDIO) ||
            (aFmt == PVMF_ADIF) ||
            (aFmt == PVMF_ASF_MPEG4_AUDIO) ||
            (aFmt == PVMF_AAC_SIZEHDR) ||
            (aFmt == PVMF_AMR_IF2) ||
            (aFmt == PVMF_AMR_IETF) ||
            (aFmt == PVMF_AMR_IETF_COMBINED) ||
            (aFmt == PVMF_AMRWB_IETF) ||
            (aFmt == PVMF_AMRWB_IETF_PAYLOAD) ||
            (aFmt == PVMF_MP3) ||
            (aFmt == PVMF_WMA)
           );

}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecPort::FormatUpdated()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO
                    , (0, "PVMFOMXAudioDecPort::FormatUpdated %d", iFormat));
}


bool
PVMFOMXAudioDecPort::pvmiSetPortFormatSpecificInfoSync(OsclRefCounterMemFrag& aMemFrag)
{
    if ((iConnectedPort) &&
            (iTag == PVMF_OMX_AUDIO_DEC_NODE_PORT_TYPE_SINK))
    {
        PvmiCapabilityAndConfig *config = NULL;
        iConnectedPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID,
                                       (OsclAny*&)config);

        /*
         * Create PvmiKvp for capability settings
         */
        if ((config) && (aMemFrag.getMemFragSize() > 0))
        {
            OsclMemAllocator alloc;
            PvmiKvp kvp;
            kvp.key = NULL;
            kvp.length = oscl_strlen(PVMF_FORMAT_SPECIFIC_INFO_KEY) + 1; // +1 for \0
            kvp.key = (PvmiKeyType)alloc.ALLOCATE(kvp.length);
            if (kvp.key == NULL)
            {
                return false;
            }
            oscl_strncpy(kvp.key, PVMF_FORMAT_SPECIFIC_INFO_KEY, kvp.length);

            kvp.value.key_specific_value = (OsclAny*)(aMemFrag.getMemFragPtr());
            kvp.capacity = aMemFrag.getMemFragSize();
            PvmiKvp* retKvp = NULL; // for return value
            int32 err;
            OSCL_TRY(err, config->setParametersSync(NULL, &kvp, 1, retKvp););
            /* ignore the error for now */
            alloc.deallocate((OsclAny*)(kvp.key));
        }
        return true;
    }
    return false;
}



//////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecPort::Connect(PVMFPortInterface* aPort)
{
    if (!aPort)
    {
        return PVMFErrArgument;
    }

    if (iConnectedPort)
    {
        return PVMFFailure;
    }

    PvmiCapabilityAndConfig *config;
    aPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)config);

    if (config != NULL)
    {
        PvmiKvp kv;
        PvmiKvp* kvp = &kv;
        int numKvp = 0;
        PVMFStatus status =
            config->getParametersSync(NULL, PVMF_FORMAT_SPECIFIC_INFO_KEY, kvp, numKvp, NULL);
        if (status == PVMFSuccess)
        {
            if (iTrackConfig != NULL)
            {
                OSCL_FREE(iTrackConfig);
                iTrackConfigSize = 0;
            }
            if (kvp)
            {
                iTrackConfigSize = kvp->capacity;
                iTrackConfig = (uint8*)(OSCL_MALLOC(sizeof(uint8) * iTrackConfigSize));
                oscl_memcpy(iTrackConfig, kvp->value.key_specific_value, iTrackConfigSize);
            }
            //config->releaseParameters(NULL, kvp, numKvp);
            OsclMemAllocator alloc;
            alloc.deallocate((OsclAny*)kvp->key);
        }
    }

    // Automatically connect the peer.

    if (aPort->PeerConnect(this) != PVMFSuccess)
    {
        return PVMFFailure;
    }

    iConnectedPort = aPort;
    PortActivity(PVMF_PORT_ACTIVITY_CONNECT);
    return PVMFSuccess;
}


void PVMFOMXAudioDecPort::setParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements,
        PvmiKvp * & aRet_kvp)
{

    // if port connect needs format specific info
    if (aParameters && pv_mime_strcmp(aParameters->key, PVMF_FORMAT_SPECIFIC_INFO_KEY) == 0)
    {
        if (iTrackConfig != NULL)
        {
            OSCL_FREE(iTrackConfig);
            iTrackConfigSize = 0;
        }
        iTrackConfigSize = aParameters->capacity;
        iTrackConfig = (uint8*)(OSCL_MALLOC(sizeof(uint8) * iTrackConfigSize));
        oscl_memcpy(iTrackConfig, aParameters->value.key_specific_value, iTrackConfigSize);
        return;
    }
    // call the base class function
    PvmiCapabilityAndConfigPortFormatImpl::setParametersSync(aSession, aParameters, num_elements, aRet_kvp);

}

// MACROS DEFINITIONS FOR verifyParametersSync
#define GetUnalignedWord( pb, w ) \
            (w) = ((uint16) *(pb + 1) << 8) + *pb;

#define GetUnalignedDword( pb, dw ) \
            (dw) = ((uint32) *(pb + 3) << 24) + \
                   ((uint32) *(pb + 2) << 16) + \
                   ((uint16) *(pb + 1) << 8) + *pb;

#define GetUnalignedWordEx( pb, w )     GetUnalignedWord( pb, w ); (pb) += sizeof(uint16);
#define GetUnalignedDwordEx( pb, dw )   GetUnalignedDword( pb, dw ); (pb) += sizeof(uint32);

#define LoadWORD( w, p )    GetUnalignedWordEx( p, w )
#define LoadDWORD( dw, p )  GetUnalignedDwordEx( p, dw )

#define WAVE_FORMAT_MSAUDIO1  0x0160
#define WAVE_FORMAT_WMAUDIO2  0x0161
#define WAVE_FORMAT_WMAUDIO3  0x0162
#define WAVE_FORMAT_WMAUDIO_LOSSLESS  0x0163

#define WAVE_FORMAT_MSSPEECH  10

#define ENCOPT3_UNSUPPORTED_OPTS 0xd000

PVMFStatus PVMFOMXAudioDecPort::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{

    // in case of WMA call the Node verify to make sure that the parameters are OK
    // (the node will also make sure that the downstream node is OK with the change
    // if necessary)
    if (iFormat == PVMF_WMA)
    {
        if (iOMXNode->VerifyParametersSync(aSession, aParameters, num_elements))
        {
            return PVMFSuccess;
        }
    }
    else
    {
        return PVMFSuccess;
    }

    return PVMFErrNotSupported;
}

PVMFStatus PVMFOMXAudioDecPort::verifyConnectedPortParametersSync(const char* aFormatValType,
        OsclAny* aConfig)
{

    PVMFStatus status = PVMFErrNotSupported;
    PvmiCapabilityAndConfig *capConfig;
    if (iConnectedPort)
    {
        iConnectedPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID,
                                       (OsclAny*&)capConfig);
    }
    else
        return PVMFFailure;

    if (capConfig != NULL)
    {
        if (pv_mime_strcmp(aFormatValType, PVMF_BITRATE_VALUE_KEY) == 0)
        {
            if (aConfig != NULL)
            {
                OsclMemAllocator alloc;
                PvmiKvp kvp;
                kvp.key = NULL;
                kvp.length = oscl_strlen(aFormatValType) + 1; // +1 for \0
                kvp.key = (PvmiKeyType)alloc.ALLOCATE(kvp.length);
                if (kvp.key == NULL)
                {
                    return PVMFErrNoMemory;
                }
                oscl_strncpy(kvp.key, aFormatValType, kvp.length);
                uint32* bitrate = (uint32*)aConfig;
                kvp.value.uint32_value = *bitrate;

                int32 err;
                OSCL_TRY(err, status = capConfig->verifyParametersSync(NULL, &kvp, 1););
                /* ignore the error for now */
                alloc.deallocate((OsclAny*)(kvp.key));

                return status;
            }
        }
        return PVMFErrArgument;
    }
    return PVMFFailure;
}


