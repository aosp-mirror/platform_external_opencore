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
#ifndef PVMF_AMRENC_PORT_H_INCLUDED
#include "pvmf_amrenc_port.h"
#endif
#ifndef OSCL_PRIQUEUE_H_INCLUDED
#include "oscl_priqueue.h"
#endif
#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif
#ifndef PVMF_AMRENC_NODE_TYPES_H_INCLUDED
#include "pvmf_amrenc_node_types.h"
#endif
#ifndef PVMF_AMRENC_NODE_H_INCLUDED
#include "pvmf_amrenc_node.h"
#endif
#ifndef PV_AMR_ENC_EXTENSION_H_INCLUDED
#include "pvmfamrencnode_extension.h"
#endif

////////////////////////////////////////////////////////////////////////////
PvmfAmrEncPort::PvmfAmrEncPort(int32 aTag, PvmfAmrEncNode* aNode, PvmfAmrEncCapabilityConfig* aConfig, int32 aPriority, const char* aName)
        :  PvmfPortBaseImpl(aTag, this,
                            PVMF_AMRENC_PORT_CAPACITY, PVMF_AMRENC_PORT_RESERVE, PVMF_AMRENC_PORT_THRESHOLD,
                            PVMF_AMRENC_PORT_CAPACITY, PVMF_AMRENC_PORT_RESERVE, PVMF_AMRENC_PORT_THRESHOLD, aName),
        OsclActiveObject(aPriority, "PvmfAmrEncPort"),
        iFormat(PVMF_FORMAT_UNKNOWN),
        iNode(aNode),
        iEncConfig(aConfig)
{
    AddToScheduler();
    iLogger = PVLogger::GetLoggerObject("PvmfAmrEncPort");
#if PVMF_PORT_BASE_IMPL_STATS
    oscl_memset((OsclAny*)&(PvmfPortBaseImpl::iStats), 0, sizeof(PvmfPortBaseImplStats));
#endif
}

////////////////////////////////////////////////////////////////////////////
PvmfAmrEncPort::~PvmfAmrEncPort()
{
    Disconnect();
    ClearMsgQueues();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncPort::QueryInterface(const PVUuid& aUuid, OsclAny*& aPtr)
{
    if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
        aPtr = (PvmiCapabilityAndConfig*)this;
    else
        aPtr = NULL;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncPort::Connect(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::Connect: aPort=0x%x", aPort));

    if (!aPort)
    {
        LOG_ERR((0, "PvmfAmrEncPort::Connect: Error - Connecting to invalid port"));
        return PVMFErrArgument;
    }

    if (iConnectedPort)
    {
        LOG_ERR((0, "PvmfAmrEncPort::Connect: Error - Already connected"));
        return PVMFFailure;
    }

    PvmiCapabilityAndConfig* config = NULL;
    aPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)config);
    if (!config)
    {
        LOG_ERR((0, "PvmfAmrEncPort::Connect: Error - Peer port does not support capability interface"));
        return PVMFFailure;
    }

    PVMFStatus status = PVMFSuccess;
    switch (iTag)
    {
        case PVMF_AMRENC_NODE_PORT_TYPE_INPUT:
            status = NegotiateInputSettings(config);
            break;
        case PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT:
            status = NegotiateOutputSettings(config);
            break;
        default:
            LOG_ERR((0, "PvmfAmrEncPort::Connect: Error - Invalid port tag"));
            status = PVMFFailure;
    }

    if (status != PVMFSuccess)
    {
        LOG_ERR((0, "PvmfAmrEncPort::Connect: Error - Settings negotiation failed. status=%d", status));
        return status;
    }

    //Automatically connect the peer.
    if (aPort->PeerConnect(this) != PVMFSuccess)
    {
        LOG_ERR((0, "PvmfAmrEncPort::Connect: Error - Peer Connect failed"));
        return PVMFFailure;
    }

    iConnectedPort = aPort;

#if PVMF_PORT_BASE_IMPL_STATS
    // Reset statistics
    oscl_memset((OsclAny*)&(PvmfPortBaseImpl::iStats), 0, sizeof(PvmfPortBaseImplStats));
#endif

    PortActivity(PVMF_PORT_ACTIVITY_CONNECT);
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
//                  PvmiCapabilityAndConfig
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncPort::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    // Not supported
    OSCL_UNUSED_ARG(aObserver);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncPort::getParametersSync(PvmiMIOSession session,
        PvmiKeyType identifier,
        PvmiKvp*& parameters,
        int& num_parameter_elements,
        PvmiCapabilityContext context)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::getParametersSync"));
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);

    parameters = NULL;
    num_parameter_elements = 0;
    PVMFStatus status = PVMFFailure;

    switch (iTag)
    {
        case PVMF_AMRENC_NODE_PORT_TYPE_INPUT:
            return GetInputParametersSync(identifier, parameters, num_parameter_elements);
        case PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT:
            return GetOutputParametersSync(identifier, parameters, num_parameter_elements);
        default:
            LOG_ERR((0, "PvmfAmrEncPort::getParametersSync: Error - Invalid port tag"));
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncPort::releaseParameters(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(num_elements);

    if (parameters)
    {
        iAlloc.deallocate((OsclAny*)parameters);
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncPort::createContext(PvmiMIOSession session, PvmiCapabilityContext& context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncPort::setContextParameters(PvmiMIOSession session,
        PvmiCapabilityContext& context,
        PvmiKvp* parameters, int num_parameter_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_parameter_elements);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncPort::DeleteContext(PvmiMIOSession session, PvmiCapabilityContext& context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncPort::setParametersSync(PvmiMIOSession session, PvmiKvp* parameters,
        int num_elements, PvmiKvp*& ret_kvp)
{
    OSCL_UNUSED_ARG(session);
    PVMFStatus status = PVMFSuccess;
    ret_kvp = NULL;

    for (int32 i = 0; i < num_elements; i++)
    {
        status = VerifyAndSetParameter(&(parameters[i]), true);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmfAmrEncPort::setParametersSync: Error - VerifiyAndSetParameter failed on parameter #%d", i));
            ret_kvp = &(parameters[i]);
            OSCL_LEAVE(OsclErrArgument);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncPort::setParametersAsync(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements,
        PvmiKvp*& ret_kvp,
        OsclAny* context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_elements);
    OSCL_UNUSED_ARG(ret_kvp);
    OSCL_UNUSED_ARG(context);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 PvmfAmrEncPort::getCapabilityMetric(PvmiMIOSession session)
{
    OSCL_UNUSED_ARG(session);
    return 0;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncPort::verifyParametersSync(PvmiMIOSession session,
        PvmiKvp* parameters, int num_elements)
{
    OSCL_UNUSED_ARG(session);

    PVMFStatus status = PVMFSuccess;
    for (int32 i = 0; (i < num_elements) && (status == PVMFSuccess); i++)
        status = VerifyAndSetParameter(&(parameters[i]));

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::SetFormat(PVMFFormatType aFormat)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::SetFormat: aFormat=%d", aFormat));
    if (!IsFormatSupported(aFormat))
        return PVMFFailure;

    iFormat = aFormat;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncPort::ProcessIncomingMsgReady()
{
    if (IncomingMsgQueueSize() > 0)
        RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncPort::ProcessOutgoingMsgReady()
{
    if (OutgoingMsgQueueSize() > 0)
        RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
//           Pure virtuals from PVMFPortActivityHandler
////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncPort::HandlePortActivity(const PVMFPortActivity& aActivity)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::HandlePortActivity: type=%d", aActivity.iType));

    if (aActivity.iPort != this)
    {
        LOG_ERR((0, "PvmfAmrEncPort::HandlePortActivity: Error - Activity is not on this port"));
        return;
    }

    switch (aActivity.iType)
    {
        case PVMF_PORT_ACTIVITY_CREATED:
            //Report port created info event to the node.
            iNode->ReportInfoEvent(PVMFInfoPortCreated,
                                   (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_DELETED:
            //Report port deleted info event to the node.
            iNode->ReportInfoEvent(PVMFInfoPortDeleted,
                                   (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_OUTGOING_MSG:
            // Wakeup the AO on the first message only. After that it re-schedules itself as needed.
            if (OutgoingMsgQueueSize() == 1 &&
                    !IsConnectedPortBusy())
            {
                RunIfNotReady();
            }
            break;

        case PVMF_PORT_ACTIVITY_INCOMING_MSG:
            //Wakeup the AO on the first message only. After that it re-schedules itself as needed.
            if (IncomingMsgQueueSize() == 1 &&
                    iNode->IsProcessIncomingMsgReady())
            {
                RunIfNotReady();
            }
            break;

        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_BUSY:
            // This is handled in the input port side when IsProcessIncomingMsgReady call failed
            break;

        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
            // Notifies the node that the output queue is ready, and the node would
            // resume encoding incoming data
            iNode->HandlePortActivity(aActivity);
            break;

        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_READY:
            if (OutgoingMsgQueueSize() > 0)
                RunIfNotReady();
            break;

        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_BUSY:
            // This is handled when iNode->ProcessOutgoingMsg failed with busy
            break;

        case PVMF_PORT_ACTIVITY_CONNECT:
        case PVMF_PORT_ACTIVITY_DISCONNECT:
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
//           Pure virtuals from OsclActiveObject
////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncPort::Run()
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::Run"));
    PVMFStatus status = PVMFSuccess;

    // Process incoming messages
    if (iTag == PVMF_AMRENC_NODE_PORT_TYPE_INPUT)
    {
        if (IncomingMsgQueueSize() > 0)
        {
            //dispatch the incoming data.
            status = iNode->ProcessIncomingMsg(this);
            switch (status)
            {
                case PVMFSuccess:
                    // Reschedule if there is more data and the node did not become busy
                    // after processing the current msg
                    if (IncomingMsgQueueSize() > 0 && iNode->IsProcessIncomingMsgReady())
                    {
                        RunIfNotReady();
                    }
                    break;

                case PVMFErrBusy:
                    // Node busy. Don't schedule next data
                    break;

                default:
                    LOG_ERR((0, "PvmfAmrEncPort::Run: Error - ProcessIncomingMsg failed. status=%d", status));
                    break;
            }
        }

        if (iNode->IsFlushPending())
        {
            if (IncomingMsgQueueSize() == 0 && OutgoingMsgQueueSize() == 0)
                iNode->FlushComplete();
        }
    }

    //Process outgoing messages
    if (iTag == PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT)
    {
        if (OutgoingMsgQueueSize() > 0)
        {
            if (iNode->IsProcessOutgoingMsgReady())
            {
                //Send data to connected port
                status = Send();
                switch (status)
                {
                    case PVMFSuccess:
                        // Reschedule if there's more data to process and connected port did not become busy
                        // after receiving the last msg
                        if (OutgoingMsgQueueSize() > 0 && iNode->IsProcessOutgoingMsgReady())
                        {
                            RunIfNotReady();
                        }
                        break;

                    case PVMFErrBusy:
                        // Connected port busy. Don't schedule next data
                        break;

                    default:
                        LOG_ERR((0, "PvmfAmrEncPort::Run: Error - Send() failed. status=%d", status));
                        iNode->ReportErrorEvent(PVMF_AMRENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)this);
                        break;
                }

                if (iNode->IsFlushPending())
                {
                    if (IncomingMsgQueueSize() == 0 && OutgoingMsgQueueSize() == 0)
                        iNode->FlushComplete();
                }
            }
        }

        if (iNode->IsFlushPending())
        {
            if (IncomingMsgQueueSize() == 0 && OutgoingMsgQueueSize() == 0)
                iNode->FlushComplete();
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//           Capabilities exchange handling routines
////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncPort::IsFormatSupported(PVMFFormatType aFormat)
{
    if (iTag == PVMF_AMRENC_NODE_PORT_TYPE_INPUT)
    {
        switch (aFormat)
        {
            case PVMF_PCM16:
                return true;
            default:
                break;
        }
    }
    else if (iTag == PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aFormat)
        {
            case PVMF_AMR_IETF:
            case PVMF_AMR_IF2:
                return true;
            default:
                break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::GetInputParametersSync(PvmiKeyType identifier, PvmiKvp*& parameters,
        int& num_parameter_elements)
{
    if (iTag != PVMF_AMRENC_NODE_PORT_TYPE_INPUT)
        return PVMFFailure;

    PVMFStatus status = PVMFSuccess;

    if (pv_mime_strcmp(identifier, INPUT_FORMATS_CAP_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, INPUT_FORMATS_VALTYPE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmfAmrEncPort::GetInputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        parameters[0].value.uint32_value = PVMF_PCM16;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::GetOutputParametersSync(PvmiKeyType identifier, PvmiKvp*& parameters,
        int& num_parameter_elements)
{
    if (iTag != PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT)
        return PVMFFailure;

    PVMFStatus status = PVMFSuccess;

    if (pv_mime_strcmp(identifier, OUTPUT_FORMATS_CAP_QUERY) == 0)
    {
        num_parameter_elements = 2;
        status = AllocateKvp(parameters, OUTPUT_FORMATS_VALTYPE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmfAmrEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
        }
        else
        {
            parameters[0].value.uint32_value = PVMF_AMR_IETF;
            parameters[1].value.uint32_value = PVMF_AMR_IF2;
        }
    }
    else if (pv_mime_strcmp(identifier, OUTPUT_FORMATS_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, OUTPUT_FORMATS_VALTYPE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmfAmrEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
        }
        else
        {
            parameters[0].value.uint32_value = iEncConfig->GetOutputFormat();
        }
    }
    else if (pv_mime_strcmp(identifier, OUTPUT_BITRATE_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, OUTPUT_BITRATE_CUR_VALUE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmfAmrEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        else
        {
            parameters[0].value.uint32_value = iEncConfig->GetOutputBitRate();
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::AllocateKvp(PvmiKvp*& aKvp, PvmiKeyType aKey, int32 aNumParams)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::AllocateKvp"));
    uint8* buf = NULL;
    uint32 keyLen = oscl_strlen(aKey) + 1;
    int32 err = 0;

    OSCL_TRY(err,
             buf = (uint8*)iAlloc.allocate(aNumParams * (sizeof(PvmiKvp) + keyLen));
             if (!buf)
             OSCL_LEAVE(OsclErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncPort::AllocateKvp: Error - kvp allocation failed"));
                         return PVMFErrNoMemory;
                        );

    int32 i = 0;
    PvmiKvp* curKvp = aKvp = new(buf) PvmiKvp;
    buf += sizeof(PvmiKvp);
    for (i = 1; i < aNumParams; i++)
    {
        curKvp += i;
        curKvp = new(buf) PvmiKvp;
        buf += sizeof(PvmiKvp);
    }

    for (i = 0; i < aNumParams; i++)
    {
        aKvp[i].key = (char*)buf;
        oscl_strncpy(aKvp[i].key, aKey, keyLen);
        buf += keyLen;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::VerifyAndSetParameter(PvmiKvp* aKvp, bool aSetParam)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::VerifyAndSetParameter: aKvp=0x%x, aSetParam=%d", aKvp, aSetParam));

    if (!aKvp)
    {
        LOG_ERR((0, "PvmfAmrEncPort::VerifyAndSetParameter: Error - Invalid key-value pair"));
        return PVMFFailure;
    }

    if (pv_mime_strcmp(aKvp->key, INPUT_FORMATS_VALTYPE) == 0 &&
            iTag == PVMF_AMRENC_NODE_PORT_TYPE_INPUT)
    {
        switch (aKvp->value.uint32_value)
        {
            case PVMF_PCM16:
                if (aSetParam)
                {
                    iFormat = aKvp->value.uint32_value;
                    iEncConfig->SetInputBitsPerSample(16);
                }
                return PVMFSuccess;

            default:
                LOG_ERR((0, "PvmfAmrEncPort::VerifyAndSetParameter: Error - Input format %d not supported",
                         aKvp->value.uint32_value));
                return PVMFFailure;
        }
    }
    else if (pv_mime_strcmp(aKvp->key, OUTPUT_FORMATS_VALTYPE) == 0 &&
             iTag == PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aKvp->value.uint32_value)
        {
            case PVMF_AMR_IETF:
            case PVMF_AMR_IF2:
                if (aSetParam)
                {
                    iFormat = aKvp->value.uint32_value;
                    iEncConfig->SetOutputFormat(iFormat);
                }
                return PVMFSuccess;

            default:
                LOG_ERR((0, "PvmfAmrEncPort::VerifyAndSetParameter: Error - Output format %d not supported",
                         aKvp->value.uint32_value));
                return PVMFFailure;
        }
    }

    LOG_ERR((0, "PvmfAmrEncPort::VerifyAndSetParameter: Error - Unsupported parameter"));
    return PVMFFailure;
}


////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::NegotiateInputSettings(PvmiCapabilityAndConfig* aConfig)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::NegotiateInputSettings: aConfig=0x%x", aConfig));
    if (!aConfig)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateInputSettings: Error - Invalid config object"));
        return PVMFFailure;
    }

    PvmiKvp* kvp = NULL;
    int numParams = 0;
    int32 err = 0;

    // Get supported output formats from peer
    PVMFStatus status = aConfig->getParametersSync(NULL, OUTPUT_FORMATS_CAP_QUERY, kvp, numParams, NULL);
    if (status != PVMFSuccess || numParams == 0)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateInputSettings: Error - config->getParametersSync(output_formats) failed"));
        return status;
    }

    // Using a priority queue, sort the kvp's returned from aConfig->getParametersSync
    // according to the preference of this port. Formats that are not supported are
    // not pushed to the priority queue and hence dropped from consideration.
    PvmiKvp* selectedKvp = NULL;
    for (int32 i = 0; i < numParams && !selectedKvp; i++)
    {
        switch (kvp[i].value.uint32_value)
        {
            case PVMF_PCM16:
                selectedKvp = &kvp[i];
                break;
            default:
                break;
        }
    }

    if (!selectedKvp)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateInputSettings: Error - No matching supported input format"));
        return PVMFFailure;
    }

    // Set format of this port, peer port and container node
    PvmiKvp* retKvp = NULL;
    iFormat = selectedKvp->value.uint32_value;
    iEncConfig->SetInputBitsPerSample(16);
    OSCL_TRY(err, aConfig->setParametersSync(NULL, selectedKvp, 1, retKvp););
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncPort::NegotiateInputSettings: Error - aConfig->setParametersSync failed. err=%d", err));
                         return PVMFFailure;
                        );

    // Release parameters back to peer and reset for the next query
    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;

    status = aConfig->getParametersSync(NULL, AUDIO_OUTPUT_SAMPLING_RATE_CUR_QUERY, kvp, numParams, NULL);
    uint32 samplingRate = 0;
    if (status != PVMFSuccess || !kvp || numParams != 1)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateInputSettings: config->getParametersSync(sampling rate) not supported. Use default."));
        samplingRate = PVMF_AMRENC_DEFAULT_SAMPLING_RATE;
    }
    else
    {
        samplingRate = kvp[0].value.uint32_value;
        aConfig->releaseParameters(NULL, kvp, numParams);
    }

    // Forward settings to encoder
    iEncConfig->SetInputSamplingRate(samplingRate);
    kvp = NULL;
    numParams = 0;

    status = aConfig->getParametersSync(NULL, AUDIO_OUTPUT_NUM_CHANNELS_CUR_QUERY, kvp, numParams, NULL);
    uint32 numChannels = 0;
    if (status != PVMFSuccess || !kvp || numParams != 1)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateInputSettings: config->getParametersSync(num channels) not supported. Use default"));
        numChannels = PVMF_AMRENC_DEFAULT_NUM_CHANNELS;
    }
    else
    {
        numChannels = kvp[0].value.uint32_value;
        aConfig->releaseParameters(NULL, kvp, numParams);
    }

    // Forward settings to encoder
    iEncConfig->SetInputNumChannels(numChannels);
    kvp = NULL;
    numParams = 0;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncPort::NegotiateOutputSettings(PvmiCapabilityAndConfig* aConfig)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncPort::NegotiateOutputSettings: aConfig=0x%x", aConfig));
    if (!aConfig)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateOutputSettings: Error - Invalid config object"));
        return PVMFFailure;
    }

    PvmiKvp* kvp = NULL;
    int numParams = 0;
    int32 i = 0;
    int32 err = 0;

    // Get supported input formats from peer
    PVMFStatus status = aConfig->getParametersSync(NULL, INPUT_FORMATS_CAP_QUERY, kvp, numParams, NULL);
    if (status != PVMFSuccess || numParams == 0)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateOutputSettings: Error - config->getParametersSync(input_formats) failed"));
        return status;
    }

    PvmiKvp* selectedKvp = NULL;
    PvmiKvp* retKvp = NULL;
    for (i = 0; i < numParams && !selectedKvp; i++)
    {
        if (kvp[i].value.uint32_value == iFormat)
            selectedKvp = &(kvp[i]);
    }

    if (!selectedKvp)
    {
        LOG_ERR((0, "PvmfAmrEncPort::NegotiateOutputSettings: Error - Output format not supported by peer"));
        return PVMFFailure;
    }

    OSCL_TRY(err, aConfig->setParametersSync(NULL, selectedKvp, 1, retKvp););
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncPort::NegotiateOutputSettings: Error - aConfig->setParametersSync failed. err=%d", err));
                         return PVMFFailure;
                        );

    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;

    return PVMFSuccess;
}




