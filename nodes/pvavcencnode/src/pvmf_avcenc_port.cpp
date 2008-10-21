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
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

#ifndef PVMF_AVCENC_PORT_H_INCLUDED
#include "pvmf_avcenc_port.h"
#endif
#ifndef OSCL_PRIQUEUE_H_INCLUDED
#include "oscl_priqueue.h"
#endif
#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif
#ifndef PVMF_AVCENC_NODE_TYPES_H_INCLUDED
#include "pvmf_avcenc_node_types.h"
#endif
#ifndef PVMF_AVCENC_NODE_H_INCLUDED
#include "pvmf_avcenc_node.h"
#endif
#ifndef PVMI_KVP_H_INCLUDED
#include "pvmi_kvp.h"
#endif

#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m);
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m);
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

////////////////////////////////////////////////////////////////////////////
PVMFAvcEncPort::PVMFAvcEncPort(int32 aTag, PVMFAvcEncNode* aNode, int32 aPriority, const char* aName)
        :  PvmfPortBaseImpl(aTag, this,
                            PVMF_AVCENC_PORT_CAPACITY, PVMF_AVCENC_PORT_RESERVE, PVMF_AVCENC_PORT_THRESHOLD,
                            PVMF_AVCENC_PORT_CAPACITY, PVMF_AVCENC_PORT_RESERVE, PVMF_AVCENC_PORT_THRESHOLD, aName),
        OsclActiveObject(aPriority, "PVMFAvcEncPort"),
        iFormat(PVMF_FORMAT_UNKNOWN),
        iNode(aNode)
{
    AddToScheduler();
    iLogger = PVLogger::GetLoggerObject("PVMFAvcEncPort");
#if PVMF_PORT_BASE_IMPL_STATS
    oscl_memset((OsclAny*)&(PvmfPortBaseImpl::iStats), 0, sizeof(PvmfPortBaseImplStats));
#endif
}

////////////////////////////////////////////////////////////////////////////
PVMFAvcEncPort::~PVMFAvcEncPort()
{
    Disconnect();
    ClearMsgQueues();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFAvcEncPort::QueryInterface(const PVUuid& aUuid, OsclAny*& aPtr)
{
    if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
        aPtr = (PvmiCapabilityAndConfig*)this;
    else
        aPtr = NULL;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncPort::Connect(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::Connect: aPort=0x%x", aPort));

    if (!aPort)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - Connecting to invalid port"));
        return PVMFErrArgument;
    }

    if (iConnectedPort)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - Already connected"));
        return PVMFFailure;
    }

    PvmiCapabilityAndConfig* config = NULL;
    aPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)config);
    if (!config)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - Peer port does not support capability interface"));
        return PVMFFailure;
    }

    PVMFStatus status = PVMFSuccess;
    switch (iTag)
    {
        case PVMF_AVCENC_NODE_PORT_TYPE_INPUT:
            status = NegotiateInputSettings(config);
            break;
        case PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT:
            status = NegotiateOutputSettings(config);
            iOutConnectedPort = aPort;
            break;
        default:
            LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - Invalid port tag"));
            status = PVMFFailure;
    }

    if (status != PVMFSuccess)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - Settings negotiation failed. status=%d", status));
        return status;
    }

    //Automatically connect the peer.
    if (aPort->PeerConnect(this) != PVMFSuccess)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - Peer Connect failed"));
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
OSCL_EXPORT_REF void PVMFAvcEncPort::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    // Not supported
    OSCL_UNUSED_ARG(aObserver);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncPort::getParametersSync(PvmiMIOSession session,
        PvmiKeyType identifier,
        PvmiKvp*& parameters,
        int& num_parameter_elements,
        PvmiCapabilityContext context)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::getParametersSync"));
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);

    parameters = NULL;
    num_parameter_elements = 0;
    PVMFStatus status = PVMFFailure;

    switch (iTag)
    {
        case PVMF_AVCENC_NODE_PORT_TYPE_INPUT:
            return GetInputParametersSync(identifier, parameters, num_parameter_elements);
        case PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT:
            return GetOutputParametersSync(identifier, parameters, num_parameter_elements);
        default:
            LOG_ERR((0, "PVMFAvcEncPort::getParametersSync: Error - Invalid port tag"));
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncPort::releaseParameters(PvmiMIOSession session,
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
OSCL_EXPORT_REF void PVMFAvcEncPort::createContext(PvmiMIOSession session, PvmiCapabilityContext& context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFAvcEncPort::setContextParameters(PvmiMIOSession session,
        PvmiCapabilityContext& context,
        PvmiKvp* parameters, int num_parameter_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_parameter_elements);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFAvcEncPort::DeleteContext(PvmiMIOSession session, PvmiCapabilityContext& context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFAvcEncPort::setParametersSync(PvmiMIOSession session, PvmiKvp* parameters,
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
            LOG_ERR((0, "PVMFAvcEncPort::setParametersSync: Error - VerifiyAndSetParameter failed on parameter #%d", i));
            ret_kvp = &(parameters[i]);
            OSCL_LEAVE(OsclErrArgument);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncPort::setParametersAsync(PvmiMIOSession session,
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
OSCL_EXPORT_REF uint32 PVMFAvcEncPort::getCapabilityMetric(PvmiMIOSession session)
{
    OSCL_UNUSED_ARG(session);
    return 0;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncPort::verifyParametersSync(PvmiMIOSession session,
        PvmiKvp* parameters, int num_elements)
{
    OSCL_UNUSED_ARG(session);

    PVMFStatus status = PVMFSuccess;
    for (int32 i = 0; (i < num_elements) && (status == PVMFSuccess); i++)
        status = VerifyAndSetParameter(&(parameters[i]));

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncPort::SetFormat(PVMFFormatType aFormat)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::SetFormat: aFormat=%d", aFormat));
    if (!IsFormatSupported(aFormat))
        return PVMFFailure;

    iFormat = aFormat;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncPort::ProcessIncomingMsgReady()
{
    if (IncomingMsgQueueSize() > 0)
        RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncPort::ProcessOutgoingMsgReady()
{
    if (OutgoingMsgQueueSize() > 0)
        RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
//           Pure virtuals from PVMFPortActivityHandler
////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncPort::HandlePortActivity(const PVMFPortActivity& aActivity)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::HandlePortActivity: type=%d", aActivity.iType));

    if (aActivity.iPort != this)
    {
        LOG_ERR((0, "PVMFAvcEncPort::HandlePortActivity: Error - Activity is not on this port"));
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
void PVMFAvcEncPort::Run()
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::Run"));
    PVMFStatus status = PVMFSuccess;

    // Process incoming messages
    if (iTag == PVMF_AVCENC_NODE_PORT_TYPE_INPUT)
    {
        if (IncomingMsgQueueSize() > 0)
        {
            //dispatch the incoming data.
            if (iNode->IsProcessIncomingMsgReady())
            {
                status = iNode->ProcessIncomingMsg(this);
                if (status != PVMFSuccess)
                {
                    LOG_ERR((0, "PVMFAvcEncPort::Run: Error - ProcessIncomingMsg failed. status=%d", status));
                }
            }

        }


        if (iNode->IsFlushPending())
        {
            if (IncomingMsgQueueSize() == 0 && OutgoingMsgQueueSize() == 0)
            {
                iNode->FlushComplete();
            }
            else
            {
                RunIfNotReady();
            }
        }
    }

    //Process outgoing messages
    if (iTag == PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT)
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
                        LOG_ERR((0, "PVMFAvcEncPort::Run: Error - Send() failed. status=%d", status));
                        iNode->ReportErrorEvent(PVMF_AVCENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)this);
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
bool PVMFAvcEncPort::IsFormatSupported(PVMFFormatType aFormat)
{
    if (iTag == PVMF_AVCENC_NODE_PORT_TYPE_INPUT)
    {
        switch (aFormat)
        {
            case PVMF_YUV420:
            case PVMF_YUV422:
            case PVMF_RGB12:
            case PVMF_RGB24:
                return true;
            default:
                break;
        }
    }
    else if (iTag == PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aFormat)
        {
            case PVMF_H264_RAW:
            case PVMF_H264_MP4:
            case PVMF_H264:
                return true;
            default:
                break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncPort::GetInputParametersSync(PvmiKeyType identifier, PvmiKvp*& parameters,
        int& num_parameter_elements)
{
    if (iTag != PVMF_AVCENC_NODE_PORT_TYPE_INPUT)
        return PVMFFailure;

    PVMFStatus status = PVMFSuccess;

    if (pv_mime_strcmp(identifier, INPUT_FORMATS_CAP_QUERY) == 0)
    {
        num_parameter_elements = 1;//4;
        status = AllocateKvp(parameters, INPUT_FORMATS_VALTYPE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetInputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        parameters[0].value.uint32_value = PVMF_YUV420;
        parameters[1].value.uint32_value = PVMF_YUV422;
        parameters[2].value.uint32_value = PVMF_RGB12;
        parameters[3].value.uint32_value = PVMF_RGB24;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncPort::GetOutputParametersSync(PvmiKeyType identifier, PvmiKvp*& parameters,
        int& num_parameter_elements)
{
    if (iTag != PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT)
        return PVMFFailure;

    PVMFStatus status = PVMFSuccess;

    if (pv_mime_strcmp(identifier, OUTPUT_FORMATS_CAP_QUERY) == 0)
    {
        num_parameter_elements = 2;//3;
        status = AllocateKvp(parameters, OUTPUT_FORMATS_VALTYPE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
        }
        else
        {
            parameters[0].value.uint32_value = PVMF_H264_RAW;
            parameters[1].value.uint32_value = PVMF_H264_MP4;
            //parameters[2].value.uint32_value = PVMF_H264;
        }
    }
    else if (pv_mime_strcmp(identifier, OUTPUT_FORMATS_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, OUTPUT_FORMATS_VALTYPE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
        }
        else
        {
            parameters[0].value.uint32_value = iNode->GetCodecType();
        }
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_WIDTH_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_WIDTH_CUR_VALUE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        uint32 width, height;
        status = iNode->GetOutputFrameSize(0, width, height);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error -iNode->GetOutputFrameSize failed. status=%d", status));
        }
        else
        {
            parameters[0].value.uint32_value = width;
        }
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_HEIGHT_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_HEIGHT_CUR_VALUE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        else
        {
            uint32 width, height;
            status = iNode->GetOutputFrameSize(0, width, height);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - iNode->GetOutputFrameSize failed. status=%d", status));
            }
            else
            {
                parameters[0].value.uint32_value = height;
            }
        }
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_FRAME_RATE_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_FRAME_RATE_CUR_VALUE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        else
        {
            parameters[0].value.float_value = iNode->GetOutputFrameRate(0);
        }
    }
    else if (pv_mime_strcmp(identifier, OUTPUT_BITRATE_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, OUTPUT_BITRATE_CUR_VALUE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        else
        {
            parameters[0].value.uint32_value = iNode->GetOutputBitRate(0);
        }
    }
    else if (pv_mime_strcmp(identifier, VIDEO_OUTPUT_IFRAME_INTERVAL_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_IFRAME_INTERVAL_CUR_VALUE, num_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        else
        {
            parameters[0].value.uint32_value = iNode->GetIFrameInterval();
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncPort::AllocateKvp(PvmiKvp*& aKvp, PvmiKeyType aKey, int32 aNumParams)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::AllocateKvp"));
    uint8* buf = NULL;
    uint32 keyLen = oscl_strlen(aKey) + 1;
    int32 err = 0;

    OSCL_TRY(err,
             buf = (uint8*)iAlloc.allocate(aNumParams * (sizeof(PvmiKvp) + keyLen));
             if (!buf)
             OSCL_LEAVE(OsclErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PVMFAvcEncPort::AllocateKvp: Error - kvp allocation failed"));
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
PVMFStatus PVMFAvcEncPort::VerifyAndSetParameter(PvmiKvp* aKvp, bool aSetParam)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::VerifyAndSetParameter: aKvp=0x%x, aSetParam=%d", aKvp, aSetParam));

    if (!aKvp)
    {
        LOG_ERR((0, "PVMFAvcEncPort::VerifyAndSetParameter: Error - Invalid key-value pair"));
        return PVMFFailure;
    }

    if (pv_mime_strcmp(aKvp->key, INPUT_FORMATS_VALTYPE) == 0 &&
            iTag == PVMF_AVCENC_NODE_PORT_TYPE_INPUT)
    {
        switch (aKvp->value.uint32_value)
        {
            case PVMF_YUV420:
            case PVMF_YUV422:
            case PVMF_RGB12:
            case PVMF_RGB24:
                if (aSetParam)
                {
                    iFormat = aKvp->value.uint32_value;
                    iNode->SetInputFormat(iFormat);
                }
                return PVMFSuccess;

            default:
                LOG_ERR((0, "PVMFAvcEncPort::VerifyAndSetParameter: Error - Input format %d not supported",
                         aKvp->value.uint32_value));
                return PVMFFailure;
        }
    }
    else if (pv_mime_strcmp(aKvp->key, OUTPUT_FORMATS_VALTYPE) == 0 &&
             iTag == PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aKvp->value.uint32_value)
        {
            case PVMF_H264_RAW:
            case PVMF_H264_MP4:
                //case PVMF_H264:
                if (aSetParam)
                {
                    iFormat = aKvp->value.uint32_value;
                    iNode->SetCodecType(iFormat);
                }
                return PVMFSuccess;

            default:
                LOG_ERR((0, "PVMFAvcEncPort::VerifyAndSetParameter: Error - Output format %d not supported",
                         aKvp->value.uint32_value));
                return PVMFFailure;
        }
    }

    LOG_ERR((0, "PVMFAvcEncPort::VerifyAndSetParameter: Error - Unsupported parameter"));
    return PVMFFailure;
}


////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncPort::NegotiateInputSettings(PvmiCapabilityAndConfig* aConfig)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::NegotiateInputSettings: aConfig=0x%x", aConfig));
    if (!aConfig)
    {
        LOG_ERR((0, "PVMFAvcEncPort::NegotiateInputSettings: Error - Invalid config object"));
        return PVMFFailure;
    }

    PvmiKvp* kvp = NULL;
    int numParams = 0;
    int32 err = 0;
    uint32 videoFormat;

    // Get supported output formats from peer
    PVMFStatus status = aConfig->getParametersSync(NULL, OUTPUT_FORMATS_CAP_QUERY, kvp, numParams, NULL);
    if (status != PVMFSuccess || numParams == 0)
    {
        LOG_ERR((0, "PVMFAvcEncPort::NegotiateInputSettings: Error - config->getParametersSync(output_formats) failed"));
        return status;
    }

    OsclPriorityQueue < PvmiKvp*, OsclMemAllocator,
    Oscl_Vector<PvmiKvp*, OsclMemAllocator>,
    PVMFAvcEncInputFormatCompareLess > sortedKvp;

    // Using a priority queue, sort the kvp's returned from aConfig->getParametersSync
    // according to the preference of this port. Formats that are not supported are
    // not pushed to the priority queue and hence dropped from consideration.
    for (int32 i = 0; i < numParams; i++)
    {
        switch (kvp[i].value.uint32_value)
        {
            case PVMF_YUV420:
            case PVMF_YUV422:
            case PVMF_RGB12:
            case PVMF_RGB24:
            {
                videoFormat = kvp[i].value.uint32_value;
                OSCL_TRY(err, sortedKvp.push(&(kvp[i])););
                OSCL_FIRST_CATCH_ANY(err,
                                     LOG_ERR((0, "PVMFAvcEncPort::NegotiateInputSettings: Error - sortedKvp.push failed"));
                                     return PVMFErrNoMemory;
                                    );
            }
            break;
            default:
                break;
        }
    }

    if (sortedKvp.size() == 0)
    {
        LOG_ERR((0, "PVMFAvcEncPort::NegotiateInputSettings: Error - No matching supported input format"));
        return PVMFFailure;
    }

    PvmiKvp* selectedKvp = sortedKvp.top();
    PvmiKvp* retKvp = NULL;

    // Set format of this port, peer port and container node
    iFormat = selectedKvp->value.uint32_value;
    iNode->SetInputFormat(iFormat);
    OSCL_TRY(err, aConfig->setParametersSync(NULL, selectedKvp, 1, retKvp););
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PVMFAvcEncPort::NegotiateInputSettings: Error - aConfig->setParametersSync failed. err=%d", err));
                         return PVMFFailure;
                        );

    // Release parameters back to peer and reset for the next query
    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;

    // Get size (in pixels) of video data from peer
    uint32 width = 0;
    uint32 height = 0;
    uint8 orientation = 0;

    status = aConfig->getParametersSync(NULL, VIDEO_OUTPUT_WIDTH_CUR_QUERY, kvp, numParams, NULL);
    if (status != PVMFSuccess || numParams != 1)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - config->getParametersSync(current width) failed"));
        return status;
    }
    width = kvp[0].value.uint32_value;
    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;

    status = aConfig->getParametersSync(NULL, VIDEO_OUTPUT_HEIGHT_CUR_QUERY, kvp, numParams, NULL);
    if (status != PVMFSuccess || numParams != 1)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - config->getParametersSync(current height) failed"));
        return status;
    }
    height = kvp[0].value.uint32_value;
    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;

    if ((PVMF_RGB12 == videoFormat) || (PVMF_RGB24 == videoFormat))
    {
        status = aConfig->getParametersSync(NULL, VIDEO_FRAME_ORIENTATION_CUR_QUERY, kvp, numParams, NULL);
        if (status != PVMFSuccess || numParams != 1)
        {
            LOG_ERR((0, "PVMFVideoEncPort::Connect: Error - config->getParametersSync(current height) failed"));
            return status;
        }

        orientation = kvp[0].value.uint8_value;
        aConfig->releaseParameters(NULL, kvp, numParams);
        kvp = NULL;
        numParams = 0;

    }

    // Set input frame size of container node
    iNode->SetInputFrameSize(width, height, orientation);

    // Get video frame rate from peer
    status = aConfig->getParametersSync(NULL, VIDEO_OUTPUT_FRAME_RATE_CUR_QUERY, kvp, numParams, NULL);
    if (status != PVMFSuccess || numParams != 1)
    {
        LOG_ERR((0, "PVMFAvcEncPort::Connect: Error - config->getParametersSync(current frame rate) failed"));
        return status;
    }

    // Set input frame rate of container node
    iNode->SetInputFrameRate(kvp[0].value.float_value);
    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;
    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncPort::NegotiateOutputSettings(PvmiCapabilityAndConfig* aConfig)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncPort::NegotiateOutputSettings: aConfig=0x%x", aConfig));
    if (!aConfig)
    {
        LOG_ERR((0, "PVMFAvcEncPort::NegotiateOutputSettings: Error - Invalid config object"));
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
        LOG_ERR((0, "PVMFAvcEncPort::NegotiateOutputSettings: Error - config->getParametersSync(input_formats) failed"));
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
        LOG_ERR((0, "PVMFAvcEncPort::NegotiateOutputSettings: Error - Output format not supported by peer"));
        return PVMFFailure;
    }

    OSCL_TRY(err, aConfig->setParametersSync(NULL, selectedKvp, 1, retKvp););
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PVMFAvcEncPort::NegotiateOutputSettings: Error - aConfig->setParametersSync failed. err=%d", err));
                         return PVMFFailure;
                        );

    aConfig->releaseParameters(NULL, kvp, numParams);
    kvp = NULL;
    numParams = 0;

    return PVMFSuccess;
}


void PVMFAvcEncPort::SendSPS_PPS(OsclMemoryFragment *aSPSs, int aNumSPSs, OsclMemoryFragment *aPPSs, int aNumPPSs)
{
    int ii;
    PvmiCapabilityAndConfig* config = NULL;
    iOutConnectedPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)config);

    if (config)
    {
        for (ii = 0; ii < aNumSPSs; ii++)
        {
            // send SPS
            PvmiKvp* sps, *ret;
            AllocateKvp(sps, VIDEO_AVC_OUTPUT_SPS_CUR_VALUE, 1);

            sps->value.key_specific_value = aSPSs[ii].ptr;
            sps->capacity = aSPSs[ii].len;
            config->setParametersSync(NULL, sps, 1, ret);
            if (ret)
            {
                LOG_ERR((0, "PVMFAvcEncPort::SendSPS_PPS: Error returned Kvp is not null."));
            }
            // Release parameters back to peer and reset for the next query
            config->releaseParameters(NULL, sps, 1);
        }
        for (ii = 0; ii < aNumPPSs; ii++)
        {
            // send PPS
            PvmiKvp* pps, *ret;
            AllocateKvp(pps, VIDEO_AVC_OUTPUT_PPS_CUR_VALUE, 1);

            pps->value.key_specific_value = aPPSs[ii].ptr;
            pps->capacity = aPPSs[ii].len;
            config->setParametersSync(NULL, pps, 1, ret);
            if (ret)
            {
                LOG_ERR((0, "PVMFAvcEncPort::SendSPS_PPS: Error returned Kvp is not null."));
            }
            // Release parameters back to peer and reset for the next query
            config->releaseParameters(NULL, pps, 1);
        }
    }

    return ;
}


