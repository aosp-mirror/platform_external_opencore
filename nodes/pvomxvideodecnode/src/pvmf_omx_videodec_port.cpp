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
#include "pvmf_omx_videodec_port.h"
#include "pvmf_omx_videodec_node.h"


PVMFOMXVideoDecPort::PVMFOMXVideoDecPort(int32 aTag, PVMFNodeInterface* aNode, const char*name)
        : PvmfPortBaseImpl(aTag, aNode, name)
{
    iOMXNode = (PVMFOMXVideoDecNode *) aNode;
    Construct();
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecPort::Construct()
{
    iLogger = PVLogger::GetLoggerObject("PVMFOMXVideoDecPort");
    oscl_memset(&iStats, 0, sizeof(PvmfPortBaseImplStats));
    iNumFramesGenerated = 0;
    iNumFramesConsumed = 0;
    iTrackConfig = NULL;
    iTrackConfigSize = 0;
    PvmiCapabilityAndConfigPortFormatImpl::Construct(
        PVMF_OMX_VIDEO_DEC_PORT_INPUT_FORMATS
        , PVMF_OMX_VIDEO_DEC_PORT_INPUT_FORMATS_VALTYPE);
}


PVMFOMXVideoDecPort::~PVMFOMXVideoDecPort()
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
bool PVMFOMXVideoDecPort::IsFormatSupported(PVMFFormatType aFmt)
{
    return ((aFmt == PVMF_YUV420) || (aFmt == PVMF_H264_RAW) || (aFmt == PVMF_H264_MP4) || (aFmt == PVMF_H264) || (aFmt == PVMF_M4V) || (aFmt == PVMF_H263)
            || (aFmt == PVMF_WMV)
           );
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecPort::FormatUpdated()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO
                    , (0, "PVMFOMXVideoDecPort::FormatUpdated %d", iFormat));
}

bool
PVMFOMXVideoDecPort::pvmiSetPortFormatSpecificInfoSync(OsclRefCounterMemFrag& aMemFrag)
{
    if ((iConnectedPort) &&
            (iTag == PVMF_OMX_VIDEO_DEC_NODE_PORT_TYPE_SINK))
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

PVMFStatus PVMFOMXVideoDecPort::Connect(PVMFPortInterface* aPort)
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
            config->getParametersSync(NULL, (PvmiKeyType)PVMF_FORMAT_SPECIFIC_INFO_KEY, kvp, numKvp, NULL);
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

            config->releaseParameters(NULL, kvp, numKvp);
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

void PVMFOMXVideoDecPort::setParametersSync(PvmiMIOSession aSession,
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




PVMFStatus PVMFOMXVideoDecPort::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{

    OSCL_UNUSED_ARG(aSession);

    // in case of WMV call the Node verify to make sure that the parameters are OK
    // (the node will also make sure that the downstream node is OK with the change
    // if necessary)
    if (iFormat == PVMF_WMV)
    {
        if (iOMXNode->VerifyParametersSync(aSession, aParameters, num_elements))
        {
            return PVMFSuccess;
        }
        else
        {
            return PVMFErrNotSupported;
        }
    }
    else
    {
        return PVMFSuccess;
    }
}

PVMFStatus PVMFOMXVideoDecPort::verifyConnectedPortParametersSync(const char* aFormatValType,
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
        if (pv_mime_strcmp(aFormatValType, PVMF_FORMAT_SPECIFIC_INFO_KEY) == 0)
        {
            OsclRefCounterMemFrag* aFormatValue = (OsclRefCounterMemFrag*)aConfig;
            if (aFormatValue->getMemFragSize() > 0)
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

                kvp.value.key_specific_value = (OsclAny*)(aFormatValue->getMemFragPtr());
                kvp.capacity = aFormatValue->getMemFragSize();
                int32 err;
                OSCL_TRY(err, status = capConfig->verifyParametersSync(NULL, &kvp, 1););
                /* ignore the error for now */
                alloc.deallocate((OsclAny*)(kvp.key));

                return status;
            }
        }
        else if (pv_mime_strcmp(aFormatValType, PVMF_BITRATE_VALUE_KEY) == 0 ||
                 pv_mime_strcmp(aFormatValType, PVMF_FRAMERATE_VALUE_KEY) == 0)
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




