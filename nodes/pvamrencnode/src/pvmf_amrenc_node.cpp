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
#ifndef PVMF_AMRENC_NODE_H_INCLUDED
#include "pvmf_amrenc_node.h"
#endif
#ifndef OSCL_ERROR_CODES_H_INCLUDED
#include "oscl_error_codes.h"
#endif
#ifndef PVMF_AMRENC_PORT_H_INCLUDED
#include "pvmf_amrenc_port.h"
#endif
#ifndef PVMF_AMRENC_NODE_FACTORY_H_INCLUDED
#include "pvmf_amrenc_node_factory.h"
#endif
#ifndef PVMF_AMRENC_NODE_TYPES_H_INCLUDED
#include "pvmf_amrenc_node_types.h"
#endif
#ifndef PVMF_AMRENC_DATA_PROCESSOR_H_INCLUDED
#include "pvmf_amrenc_data_processor.h"
#endif
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif
#ifndef PVMF_MEDIA_MSG_FORMAT_IDS_H_INCLUDED
#include "pvmf_media_msg_format_ids.h"
#endif
#ifndef PVMF_MEDIA_CMD_H_INCLUDED
#include "pvmf_media_cmd.h"
#endif

// DLL entry point
OSCL_DLL_ENTRY_POINT_DEFAULT()

////////////////////////////////////////////////////////////////////////////
//              PvmfAmrEncNodeFactory implementation
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFNodeInterface* PvmfAmrEncNodeFactory::Create(int32 aPriority)
{
    int32 err = 0;
    PvmfAmrEncNode* node = NULL;
    PvmfAmrEncDataProcessorInterface* dataProcessor = NULL;

    OSCL_TRY(err,
             dataProcessor = OSCL_NEW(PvmfAmrEncDataProcessor, ());
             if (!dataProcessor)
             OSCL_LEAVE(OsclErrNoMemory);

             node = OSCL_NEW(PvmfAmrEncNode, (aPriority, dataProcessor));
             if (!node)
                 OSCL_LEAVE(OsclErrNoMemory);
                );
    OSCL_FIRST_CATCH_ANY(err, return NULL;);

    return (PVMFNodeInterface*)node;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PvmfAmrEncNodeFactory::Delete(PVMFNodeInterface* aNode)
{
    if (!aNode)
        return false;

    PvmfAmrEncNode* node = (PvmfAmrEncNode*)aNode;
    PvmfAmrEncDataProcessor* dataProcessor = (PvmfAmrEncDataProcessor*)node->GetDataProcessor();
    OSCL_DELETE(dataProcessor);
    OSCL_DELETE(node);
    aNode = NULL;
    return true;
}

////////////////////////////////////////////////////////////////////////////
//              PvmfAmrEncNode implementation
////////////////////////////////////////////////////////////////////////////
PvmfAmrEncNode::PvmfAmrEncNode(int32 aPriority, PvmfAmrEncDataProcessorInterface* aDataProcessor) :
        OsclTimerObject(aPriority, "PvmfAmrEncNode"),
        iDataProcessor(aDataProcessor)
{
    iInterfaceState = EPVMFNodeCreated;

    int32 err;
    OSCL_TRY(err,
             //Create the input command queue
             iCmdQueue.Construct(PVMF_AMRENC_NODE_CMD_ID_START, PVMF_AMRENC_NODE_CMD_QUEUE_RESERVE);
             iCurrentCmd.Construct(0, 1); // There's only 1 current command

             //Create the port vector.
             iInPort.Construct(PVMF_AMRENC_NODE_PORT_VECTOR_RESERVE);
             iOutPort.Construct(PVMF_AMRENC_NODE_PORT_VECTOR_RESERVE);
            );

    OSCL_FIRST_CATCH_ANY(err,
                         // If a leave happened, cleanup and re-throw the error
                         iCmdQueue.clear();
                         iInPort.clear();
                         iOutPort.clear();
                         OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
                         OSCL_CLEANUP_BASE_CLASS(OsclTimerObject);
                         OSCL_LEAVE(err);
                        );

    iDataProcessor->SetObserver(this);
}

////////////////////////////////////////////////////////////////////////////
PvmfAmrEncNode::~PvmfAmrEncNode()
{
    while (!iInPort.empty())
        iInPort.Erase(&iInPort.front());
    while (!iOutPort.empty())
        iOutPort.Erase(&iOutPort.front());

    Cancel();
    SetState(EPVMFNodeIdle);
    ThreadLogoff();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncNode::ThreadLogon()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
                AddToScheduler();
            iLogger = PVLogger::GetLoggerObject("PvmfAmrEncNode");
            SetState(EPVMFNodeIdle);
            return PVMFSuccess;
            // break;	This statement was removed to avoid compiler warning for Unreachable Code

        default:
            return PVMFErrInvalidState;
            // break;	This statement was removed to avoid compiler warning for Unreachable Code
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncNode::ThreadLogoff()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            if (IsAdded())
                RemoveFromScheduler();
            iLogger = NULL;
            SetState(EPVMFNodeCreated);
            return PVMFSuccess;
            // break;	This statement was removed to avoid compiler warning for Unreachable Code

        default:
            return PVMFErrInvalidState;
            // break;	This statement was removed to avoid compiler warning for Unreachable Code
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::GetCapability"));

    aNodeCapability.iInputFormatCapability.push_back(PVMF_PCM16);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_AMR_IETF);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_AMR_IF2);
    aNodeCapability.iCanSupportMultipleOutputPorts = false;
    aNodeCapability.iCanSupportMultipleInputPorts = false;
    aNodeCapability.iHasMaxNumberOfPorts = true;
    aNodeCapability.iMaxNumberOfPorts = 2;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFPortIter* PvmfAmrEncNode::GetPorts(const PVMFPortFilter* aFilter)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::GetPorts"));
    OSCL_UNUSED_ARG(aFilter);//port filter is not implemented.

    int32 err = 0;
    PvmfAmrEncPortVector* port = NULL;
    OSCL_TRY(err,
             port = OSCL_NEW(PvmfAmrEncPortVector, ());
             if (!port)
             return NULL;

             uint32 i;
             for (i = 0; i < iInPort.size(); i++)
                 port->AddL(iInPort[i]);
                 for (i = 0; i < iOutPort.size(); i++)
                     port->AddL(iOutPort[i]);
                     port->Reset();
                    );

    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncNode::GetPorts: Error - Out of memory"));
                        );

    return port;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::QueryUUID(PVMFSessionId aSession,
        const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::QueryUUID"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYUUID,
                  aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::QueryInterface(PVMFSessionId aSession,
        const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::QueryInterface"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYINTERFACE,
                  aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::RequestPort(PVMFSessionId aSession,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::RequestPort: aPortTag=%d", aPortTag));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_REQUESTPORT,
                  aPortTag, aPortConfig, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::ReleasePort(PVMFSessionId aSession,
        PVMFPortInterface& aPort,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::ReleasePort"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Init(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Init"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_INIT, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Prepare(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Prepare"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PREPARE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Start(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Start"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_START, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Stop(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Stop"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_STOP, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Flush(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Flush"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_FLUSH, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Pause(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Pause"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PAUSE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::Reset(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Reset"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RESET, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::CancelAllCommands"));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELALLCOMMANDS, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmfAmrEncNode::CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::CancelCommand: aCmdId=%d", aCmdId));
    PvmfAmrEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELCOMMAND, aCmdId, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    // Only outgoing queue ready activity are forwarded on to the node
    // by the output port.  The rest of the events for output port and all events
    // for input port are handled within the port AO.
    if (aActivity.iPort->GetPortTag() == PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aActivity.iType)
        {
            case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
                // Notifies the node that the output queue is ready, and the node would
                // resume encoding incoming data
                uint32 i;
                for (i = 0; i < iInPort.size(); i++)
                    ((PvmfAmrEncPort*)iInPort[i])->ProcessIncomingMsgReady();
                break;

            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::ProcessComplete(PvmfAmrEncDataProcessResult aResult, PVMFSharedMediaDataPtr& aMediaDataOut)
{
    switch (aResult)
    {
        case ProcessComplete_NotEnoughDataInQueue:
        case ProcessComplete_EnoughDataToContinueProcessing:
        {
            // Send bitstream data to downstream node
            PVMFSharedMediaMsgPtr mediaMsgOut;
            convertToPVMFMediaMsg(mediaMsgOut, aMediaDataOut);

            uint32 i = 0;
            for (i = 0; i < iOutPort.size(); i++)
            {
                PVMFStatus status = iOutPort[i]->QueueOutgoingMsg(mediaMsgOut);
                switch (status)
                {
                    case PVMFSuccess:
                        break;
                    case PVMFErrBusy:
                        LOG_DEBUG((0, "PvmfAmrEncNode::ProcessComplete: Outgoing queue busy. This should not happen."));
                        break;
                    default:
                        ReportErrorEvent(PVMF_AMRENC_NODE_ERROR_ENCODE_ERROR);
                        break;
                }
            }

            if (aResult == ProcessComplete_EnoughDataToContinueProcessing)
            {
                LOG_DEBUG((0, "PvmfAmrEncNode::ProcessComplete: More data to encode."));
                RunIfNotReady();
            }
        }
        break;

        case ProcessInComplete:
            break;

        case ProcessNoOutputMemory:
            // set the observer to be notified when output memory becomes available,
            // and then wait (return PVMFSuccess)
            iDataProcessor->NotifyMemoryAvailable(*this);
            break;

        case ProcessError:
        case ProcessNotInitialized:
            LOG_ERR((0, "PvmfAmrEncNode::ProcessIncomingMsg: Error - iDataProcessor->Process failed. result=%d", aResult));
            ReportErrorEvent(PVMF_AMRENC_NODE_ERROR_ENCODE_ERROR);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
//               Port activity processing routines
////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncNode::IsProcessOutgoingMsgReady()
{
    if (iInterfaceState == EPVMFNodeStarted || IsFlushPending())
    {
        for (uint32 i = 0; i < iOutPort.size(); i++)
        {
            if (iOutPort[i]->IsConnectedPortBusy())
                return false;
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncNode::IsProcessIncomingMsgReady()
{
    if (iInterfaceState == EPVMFNodeStarted || IsFlushPending())
    {
        for (uint32 i = 0; i < iOutPort.size(); i++)
        {
            if (iOutPort[i]->IsOutgoingQueueBusy())
                return false;
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::ProcessIncomingMsg: aPort=0x%x", aPort));
    PVMFStatus status = PVMFFailure;

    switch (aPort->GetPortTag())
    {
        case PVMF_AMRENC_NODE_PORT_TYPE_INPUT:
        {
            if (!IsProcessIncomingMsgReady())
            {
                LOG_ERR((0, "PvmfAmrEncNode::ProcessIncomingMsg: Error - Not ready."));
                return PVMFErrBusy;
            }

            PVMFSharedMediaMsgPtr msg;
            status = aPort->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PvmfAmrEncNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed. status=%d", status));
                return status;
            }

            if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                status = SendEndOfTrackCommand(msg);
                return status;
            }
            PVMFSharedMediaDataPtr mediaDataIn;
            convertToPVMFMediaData(mediaDataIn, msg);
            status = iDataProcessor->QueueIncomingData(mediaDataIn);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PvmfAmrEncNode::ProcessIncomingMsg: Error - iDataProcessor->QueueIncomingData() failed. status=%d", status));
                return status;
            }
        }
        break;

        case PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT:
            // Nothing to be done
            status = PVMFSuccess;
            break;

        default:
            LOG_ERR((0, "PvmfAmrEncNode::ProcessIncomingMsg: Error - Invalid port tag"));
            ReportErrorEvent(PVMF_AMRENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)aPort);
            status = PVMFFailure;
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
//                        Private methods
////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::Run()
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Run"));

    if (!iCmdQueue.empty())
    {
        if (ProcessCommand(iCmdQueue.front()))
        {
            // Need to check the state before re-scheduling since the node could have
            // been reset in the ProcessCommand call
            if ((iInterfaceState != EPVMFNodeCreated) &&
                    !iCmdQueue.empty())
            {
                RunIfNotReady();
            }
        }
        return;
    }

    // If it gets to here, there's more queued data to encode in the data processor
    if (iDataProcessor)
        iDataProcessor->Encode();

    LOG_STACK_TRACE((0, "PvmfAmrEncNode::Run: Out"));
}

////////////////////////////////////////////////////////////////////////////
//                   Command processing routines
////////////////////////////////////////////////////////////////////////////
PVMFCommandId PvmfAmrEncNode::QueueCommandL(PvmfAmrEncNodeCommand& aCmd)
{
    if (iInterfaceState == EPVMFNodeCreated)
    {
        OSCL_LEAVE(OsclErrNotReady);
        // return 0;	This statement was removed to avoid compiler warning for Unreachable Code
    }

    PVMFCommandId id = iCmdQueue.AddL(aCmd);
    RunIfNotReady();
    return id;
}

////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncNode::ProcessCommand(PvmfAmrEncNodeCommand& aCmd)
{
    // If a command is active, only high priority commands can interrupt
    // the processing
    if (!iCurrentCmd.empty() && !aCmd.hipri())
        return false;

    switch (aCmd.iCmd)
    {
        case PVMF_GENERIC_NODE_QUERYUUID:
            DoQueryUuid(aCmd);
            break;

        case PVMF_GENERIC_NODE_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;

        case PVMF_GENERIC_NODE_REQUESTPORT:
            DoRequestPort(aCmd);
            break;

        case PVMF_GENERIC_NODE_RELEASEPORT:
            DoReleasePort(aCmd);
            break;

        case PVMF_GENERIC_NODE_INIT:
            DoInit(aCmd);
            break;

        case PVMF_GENERIC_NODE_PREPARE:
            DoPrepare(aCmd);
            break;

        case PVMF_GENERIC_NODE_START:
            DoStart(aCmd);
            break;

        case PVMF_GENERIC_NODE_STOP:
            DoStop(aCmd);
            break;

        case PVMF_GENERIC_NODE_FLUSH:
            DoFlush(aCmd);
            break;

        case PVMF_GENERIC_NODE_PAUSE:
            DoPause(aCmd);
            break;

        case PVMF_GENERIC_NODE_RESET:
            DoReset(aCmd);
            break;

        case PVMF_GENERIC_NODE_CANCELALLCOMMANDS:
            DoCancelAllCommands(aCmd);
            break;

        case PVMF_GENERIC_NODE_CANCELCOMMAND:
            DoCancelCommand(aCmd);
            break;

        default://unknown command type
            LOG_ERR((0, "PvmfAmrEncNode::ProcessCommand: Error - Unknown command type %d", aCmd.iCmd));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            break;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::CommandComplete(PvmfAmrEncNodeCmdQueue& aCmdQueue, PvmfAmrEncNodeCommand& aCmd,
                                     PVMFStatus aStatus, OsclAny* aData)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::CommandComplete: Id=%d, Type=%d, Status=%d, Context=0x%x, Data0x%x"
                     , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aData));

    //create response
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aData);
    PVMFSessionId session = aCmd.iSession;

    //Erase the command from the queue.
    aCmdQueue.Erase(&aCmd);

    //Report completion to the session observer.
    ReportCmdCompleteEvent(session, resp);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoQueryUuid(PvmfAmrEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::DoQueryUuid"));
    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator>* uuidvec;
    bool exactMatch;
    aCmd.Parse(mimetype, uuidvec, exactMatch);

    PVMFStatus status;
    if (iDataProcessor)
    {
        if (!mimetype)
        {
            OSCL_StackString<1> tmpMimeType;
            status = iDataProcessor->QueryUUID(tmpMimeType, *uuidvec, exactMatch);
        }
        else
        {
            status = iDataProcessor->QueryUUID(*mimetype, *uuidvec, exactMatch);
        }
    }
    else
    {
        status = PVMFFailure;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoQueryInterface(PvmfAmrEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::DoQueryInterface"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.Parse(uuid, ptr);

    PVMFStatus status;
    if (iDataProcessor)
    {
        status = iDataProcessor->QueryInterface(*uuid, *ptr);
        if ((status == PVMFFailure) && (PVMI_CAPABILITY_AND_CONFIG_PVUUID == *uuid))
        {
            PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
            *ptr = OSCL_STATIC_CAST(PVInterface*, myInterface);
            status = true;
        }
    }
    else
        status = PVMFFailure;

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoRequestPort(PvmfAmrEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::DoRequestPort"));
    int32 tag;
    OSCL_String* mimetype;
    PvmfAmrEncPort* port = NULL;
    aCmd.Parse(tag, mimetype);

    switch (tag)
    {
        case PVMF_AMRENC_NODE_PORT_TYPE_INPUT:
        {
            if (iInPort.size() >= PVMF_AMRENC_NODE_MAX_INPUT_PORT)
            {
                LOG_ERR((0, "PvmfAmrEncNode::DoRequestPort: Error - Max number of input port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVAmrEncIn";

            port = AllocatePort(iInPort, tag, mimetype, portname.get_cstr());
            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        break;

        case PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT:
        {
            if (iOutPort.size() >= PVMF_AMRENC_NODE_MAX_OUTPUT_PORT)
            {
                LOG_ERR((0, "PvmfAmrEncNode::DoRequestPort: Error - Max number of output port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }


            OSCL_StackString<20> portname;
            portname = "PVAmrEncOut";

            port = AllocatePort(iOutPort, tag, mimetype, portname.get_cstr());
            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        break;

        default:
            LOG_ERR((0, "PvmfAmrEncNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            return;
    }

    //Return the port pointer to the caller.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess, (OsclAny*)port);
}

////////////////////////////////////////////////////////////////////////////
PvmfAmrEncPort* PvmfAmrEncNode::AllocatePort(PvmfAmrEncPortVector& aPortVector, int32 aTag,
        OSCL_String* aMimeType, const char* aName)
{
    int32 err = 0;
    OsclAny* ptr = NULL;

    // Allocate a new port
    OSCL_TRY(err,
             ptr = aPortVector.Allocate();
             if (!ptr)
             OSCL_LEAVE(PVMFErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncNode::AllocatePort: Error - iPortVector Out of memory"));
                         return NULL;
                        );
    PvmfAmrEncPort* port = OSCL_PLACEMENT_NEW(ptr, PvmfAmrEncPort(aTag, this, iDataProcessor, Priority(), aName));

    // if format was provided in mimestring, set it now.
    if (aMimeType)
    {
        PVMFFormatType format = GetFormatIndex(aMimeType->get_str());
        if (port->SetFormat(format) != PVMFSuccess ||
                !iDataProcessor || (aTag == PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT &&
                                    iDataProcessor->SetOutputFormat(format) != PVMFSuccess))
        {
            aPortVector.DestructAndDealloc(port);
            LOG_ERR((0, "PvmfAmrEncNode::AllocatePort: Error - port->SetFormat or iDataProcessor->SetOutputFormat failed"));
            return NULL;
        }
    }

    OSCL_TRY(err, aPortVector.AddL(port););
    OSCL_FIRST_CATCH_ANY(err,
                         aPortVector.DestructAndDealloc(port);
                         return NULL;
                        );

    return port;
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoReleasePort(PvmfAmrEncNodeCommand& aCmd)
{
    //Find the port in the port vector
    PvmfAmrEncPort* port = NULL;
    PvmfAmrEncPort** portPtr = NULL;
    aCmd.Parse((PVMFPortInterface*&)port);

    if (!port)
    {
        LOG_ERR((0, "PvmfAmrEncNode::DoReleasePort: Error - Invalid port pointer"));
        CommandComplete(iCmdQueue, aCmd, PVMFFailure);
        return;
    }

    PVMFStatus status = PVMFSuccess;
    switch (port->GetPortTag())
    {
        case PVMF_AMRENC_NODE_PORT_TYPE_INPUT:
            portPtr = iInPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PvmfAmrEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iInPort.Erase(portPtr);
            }
            break;

        case PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT:
            portPtr = iOutPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PvmfAmrEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iOutPort.Erase(portPtr);
            }
            break;

        default:
            LOG_ERR((0, "PvmfAmrEncNode::DoReleasePort: Error - Invalid port tag"));
            status = PVMFFailure;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoInit(PvmfAmrEncNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            SetState(EPVMFNodeInitialized);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodeInitialized:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoPrepare(PvmfAmrEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::DoPrepare"));
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
            SetState(EPVMFNodePrepared);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodePrepared:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoStart(PvmfAmrEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::DoStart"));
    PVMFStatus status;
    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
            if (!iDataProcessor)
            {
                LOG_ERR((0, "PvmfAmrEncNode::DoPrepare: Error - iDataProcessor not available"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            status = iDataProcessor->Initialize();
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PvmfAmrEncNode::DoPrepare: Error - iDataProcessor->Initialize() failed"));
                CommandComplete(iCmdQueue, aCmd, status);
                return;
            }

            SetState(EPVMFNodeStarted);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;

        case EPVMFNodePaused:
            SetState(EPVMFNodeStarted);

            // Notify input port that the node is ready to process incoming msg again
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                ((PvmfAmrEncPort*)iInPort[i])->ProcessIncomingMsgReady();

            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodeStarted:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoStop(PvmfAmrEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::DoStop"));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            // Clear queued messages in ports
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                iInPort[i]->ClearMsgQueues();
            for (i = 0; i < iOutPort.size(); i++)
                iOutPort[i]->ClearMsgQueues();

            PVMFStatus status;
            if (!iDataProcessor)
            {
                status = PVMFFailure;
            }
            else
            {
                status = iDataProcessor->Reset();
                if (status == PVMFSuccess)
                {
                    //transition to Prepared state
                    SetState(EPVMFNodePrepared);
                }
            }

            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodePrepared:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoFlush(PvmfAmrEncNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            int32 err;
            uint32 i;
            bool msgPending;
            msgPending = false;

            for (i = 0; i < iInPort.size(); i++)
            {
                if (iInPort[i]->IncomingMsgQueueSize() > 0)
                    msgPending = true;
                iInPort[i]->SuspendInput();
                if (iInterfaceState != EPVMFNodeStarted)
                {
                    // Port is in idle if node state is not started. Call ProcessIncomingMsgReady
                    // to wake up port AO
                    ((PvmfAmrEncPort*)iInPort[i])->ProcessIncomingMsgReady();
                }
            }

            for (i = 0; i < iOutPort.size(); i++)
            {
                if (iOutPort[i]->OutgoingMsgQueueSize() > 0)
                    msgPending = true;
                iOutPort[i]->SuspendInput();
                if (iInterfaceState != EPVMFNodeStarted)
                {
                    // Port is in idle if node state is not started. Call ProcessOutgoingMsgReady
                    // to wake up port AO
                    ((PvmfAmrEncPort*)iOutPort[i])->ProcessOutgoingMsgReady();
                }
            }

            //the flush is asynchronous.  move the command from
            //the input command queue to the current command, where
            //it will remain until the flush completes.
            OSCL_TRY(err, iCurrentCmd.StoreL(aCmd););
            OSCL_FIRST_CATCH_ANY(err,
                                 CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                                 return;
                                );
            iCmdQueue.Erase(&aCmd);
            if (!msgPending)
            {
                FlushComplete();
                return;
            }
            break;

        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncNode::IsFlushPending()
{
    return (iCurrentCmd.size() > 0
            && iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::FlushComplete()
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::FlushComplete"));
    uint32 i = 0;

    // Flush is complete only when all queues of all ports are clear.
    // Other wise, just return from this method and wait for FlushComplete
    // from the remaining ports.
    for (i = 0; i < iInPort.size(); i++)
    {
        if (iInPort[i]->IncomingMsgQueueSize() > 0 ||
                iInPort[i]->OutgoingMsgQueueSize() > 0)
        {
            return;
        }
    }

    for (i = 0; i < iOutPort.size(); i++)
    {
        if (iOutPort[i]->IncomingMsgQueueSize() > 0 ||
                iOutPort[i]->OutgoingMsgQueueSize() > 0)
        {
            return;
        }
    }

    PVMFStatus status;
    if (!iDataProcessor)
    {
        status = PVMFFailure;
    }
    else
    {
        status = iDataProcessor->Reset();
        if (status == PVMFSuccess)
        {
            //transition to Prepared state
            SetState(EPVMFNodePrepared);
        }
    }

    //resume port input so the ports can be re-started.
    for (i = 0; i < iInPort.size(); i++)
        iInPort[i]->ResumeInput();
    for (i = 0; i < iOutPort.size(); i++)
        iOutPort[i]->ResumeInput();

    CommandComplete(iCurrentCmd, iCurrentCmd.front(), status);

    if (!iCmdQueue.empty())
    {
        // If command queue is not empty, schedule to process the next command
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoPause(PvmfAmrEncNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
            SetState(EPVMFNodePaused);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodePaused:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoReset(PvmfAmrEncNodeCommand& aCmd)
{
    //This example node allows a reset from any idle state.
    if (IsAdded())
    {
        uint32 i;
        for (i = 0; i < iInPort.size(); i++)
            iInPort[i]->ClearMsgQueues();
        for (i = 0; i < iOutPort.size(); i++)
            iOutPort[i]->ClearMsgQueues();

        while (!iInPort.empty())
            iInPort.Erase(&iInPort.front());
        while (!iOutPort.empty())
            iOutPort.Erase(&iOutPort.front());

        //restore original port vector reserve.
        iInPort.Reconstruct();
        iOutPort.Reconstruct();

        PVMFStatus status;
        if (iDataProcessor)
        {
            iDataProcessor->Reset();
            iDataProcessor->ThreadLogoff();
        }

        //logoff & go back to Created state.
        SetState(EPVMFNodeIdle);
        status = ThreadLogoff();

        CommandComplete(iCmdQueue, aCmd, status);
    }
    else
    {
        OSCL_LEAVE(OsclErrInvalidState);
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoCancelAllCommands(PvmfAmrEncNodeCommand& aCmd)
{
    //first cancel the current command if any
    {
        while (!iCurrentCmd.empty())
            CommandComplete(iCurrentCmd, iCurrentCmd[0], PVMFErrCancelled);
    }

    //next cancel all queued commands
    {
        //start at element 1 since this cancel command is element 0.
        while (iCmdQueue.size() > 1)
            CommandComplete(iCmdQueue, iCmdQueue[1], PVMFErrCancelled);
    }

    //finally, report cancel complete.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::DoCancelCommand(PvmfAmrEncNodeCommand& aCmd)
{
    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.Parse(id);

    //first check "current" command if any
    {
        PvmfAmrEncNodeCommand* cmd = iCurrentCmd.FindById(id);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iCurrentCmd, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            return;
        }
    }

    //next check input queue.
    {
        //start at element 1 since this cancel command is element 0.
        PvmfAmrEncNodeCommand* cmd = iCmdQueue.FindById(id, 1);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iCmdQueue, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            return;
        }
    }
    //if we get here the command isn't queued so the cancel fails.
    CommandComplete(iCmdQueue, aCmd, PVMFFailure);
}

////////////////////////////////////////////////////////////////////////////
//                 Event reporting routines.
////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::SetState(TPVMFNodeInterfaceState aState)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::SetState %d", aState));
    PVMFNodeInterface::SetState(aState);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_ERR((0, "PvmfAmrEncNode::ReportErrorEvent: aEventType=%d aEventData=0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData);
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncNode::ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_STACK_TRACE((0, "PvmfAmrEncNode::ReportInfoEvent: aEventType=%d, aEventData0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData);
}


PVMFStatus PvmfAmrEncNode::SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg)
{
    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();

    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

    // Set the timestamp
    sharedMediaCmdPtr->setTimestamp(aMsg->getTimestamp());

    // Set the sequence number
    sharedMediaCmdPtr->setSeqNum(aMsg->getSeqNum());

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);

    PVMFStatus status;
    for (uint32 ii = 0; ii < iOutPort.size(); ii++)
    {
        status = iOutPort[ii]->QueueOutgoingMsg(mediaMsgOut);

        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmfAmrEncNode::SendEndOfTrackCommand: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }

    }

    return status;
}
