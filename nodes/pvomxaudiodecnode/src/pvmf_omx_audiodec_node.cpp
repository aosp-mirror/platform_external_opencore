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
#include "pvmf_omx_audiodec_node.h"
#include "pvlogger.h"
#include "oscl_error_codes.h"
#include "pvmf_omx_audiodec_port.h"
#include "pv_mime_string_utils.h"
#include "oscl_snprintf.h"
#include "pvmf_media_cmd.h"
#include "pvmf_media_msg_format_ids.h"
#include "pvmi_kvp_util.h"
#include "latmpayloadparser.h"

#include "omx_core.h"
#include "pvmf_omx_audiodec_callbacks.h"     //used for thin AO in Decoder's callbacks
#include "pv_omxcore.h"
#include "pv_omxmastercore.h"

#define PVOMXAUDIODEC_MEDIADATA_POOLNUM 2*NUMBER_OUTPUT_BUFFER
#define PVOMXAUDIODEC_MEDIADATA_CHUNKSIZE 128


// Node default settings

#define PVOMXAUDIODECNODE_CONFIG_MIMETYPE_DEF 0

#define PVMF_OMXAUDIODEC_NUM_METADATA_VALUES 6

// Constant character strings for metadata keys
static const char PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_FORMAT_KEY[] = "codec-info/audio/format";
static const char PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_CHANNELS_KEY[] = "codec-info/audio/channels";
static const char PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_SAMPLERATE_KEY[] = "codec-info/audio/sample-rate";
static const char PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_AVGBITRATE_KEY[] = "codec-info/audio/avgbitrate";
static const char PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_AACOBJECTTYPE_KEY[] = "codec-info/audio/aac-objecttype";
static const char PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_AACSTREAMTYPE_KEY[] = "codec-info/audio/aac-streamtype";


static const char PVOMXAUDIODECMETADATA_SEMICOLON[] = ";";



// OMX CALLBACKS
// 1) AO OMX component running in the same thread as the OMX node
//	In this case, the callbacks can be called directly from the component
//	The callback: OMX Component->CallbackEventHandler->EventHandlerProcessing
//	The callback can perform do RunIfNotReady

// 2) Multithreaded component
//	In this case, the callback is made using the threadsafe callback (TSCB) AO
//	Component thread : OMX Component->CallbackEventHandler->TSCB(ReceiveEvent) => event is queued
//  Node thread		 : dequeue event => TSCB(ProcessEvent)->ProcessCallbackEventHandler->EventHandlerProcessing



// callback for Event Handler - in multithreaded case, event is queued to be processed later
//	in AO case, event is processed immediately by calling EventHandlerProcessing
OMX_ERRORTYPE CallbackEventHandler_Audio(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_EVENTTYPE aEvent,
        OMX_OUT OMX_U32 aData1,
        OMX_OUT OMX_U32 aData2,
        OMX_OUT OMX_PTR aEventData)
{

    PVMFOMXAudioDecNode *Node = (PVMFOMXAudioDecNode *) aAppData;

    if (Node->IsComponentMultiThreaded())
    {
        // allocate the memory for the callback event specific data
        EventHandlerSpecificData_Audio* ED = (EventHandlerSpecificData_Audio*) Node->iThreadSafeHandlerEventHandler->iMemoryPool->allocate(sizeof(EventHandlerSpecificData_Audio));

        // pack the relevant data into the structure
        ED->hComponent = aComponent;
        ED->pAppData = aAppData;
        ED->eEvent = aEvent;
        ED->nData1 = aData1;
        ED->nData2 = aData2;
        ED->pEventData = aEventData;

        // convert the pointer into OsclAny ptr
        OsclAny* P = (OsclAny*) ED;


        // CALL the generic callback AO API:
        Node->iThreadSafeHandlerEventHandler->ReceiveEvent(P);

        return OMX_ErrorNone;
    }
    else
    {

        OMX_ERRORTYPE status;
        status = Node->EventHandlerProcessing(aComponent, aAppData, aEvent, aData1, aData2, aEventData);
        return status;
    }

}

// callback for EmptyBufferDone - in multithreaded case, event is queued to be processed later
//	in AO case, event is processed immediately by calling EmptyBufferDoneProcessing
OMX_ERRORTYPE CallbackEmptyBufferDone_Audio(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{

    PVMFOMXAudioDecNode *Node = (PVMFOMXAudioDecNode *) aAppData;
    if (Node->IsComponentMultiThreaded())
    {

        // allocate the memory for the callback event specific data
        //EmptyBufferDoneSpecificData* ED = (EmptyBufferDoneSpecificData*) oscl_malloc(sizeof (EmptyBufferDoneSpecificData));
        EmptyBufferDoneSpecificData_Audio* ED = (EmptyBufferDoneSpecificData_Audio*) Node->iThreadSafeHandlerEmptyBufferDone->iMemoryPool->allocate(sizeof(EmptyBufferDoneSpecificData_Audio));

        // pack the relevant data into the structure
        ED->hComponent = aComponent;
        ED->pAppData = aAppData;
        ED->pBuffer = aBuffer;

        // convert the pointer into OsclAny ptr
        OsclAny* P = (OsclAny*) ED;

        // CALL the generic callback AO API:
        Node->iThreadSafeHandlerEmptyBufferDone->ReceiveEvent(P);

        return OMX_ErrorNone;
    }
    else
    {
        OMX_ERRORTYPE status;
        status = Node->EmptyBufferDoneProcessing(aComponent, aAppData, aBuffer);
        return status;
    }

}

// callback for FillBufferDone - in multithreaded case, event is queued to be processed later
//	in AO case, event is processed immediately by calling FillBufferDoneProcessing
OMX_ERRORTYPE CallbackFillBufferDone_Audio(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{
    PVMFOMXAudioDecNode *Node = (PVMFOMXAudioDecNode *) aAppData;
    if (Node->IsComponentMultiThreaded())
    {

        // allocate the memory for the callback event specific data
        //FillBufferDoneSpecificData* ED = (FillBufferDoneSpecificData*) oscl_malloc(sizeof (FillBufferDoneSpecificData));
        FillBufferDoneSpecificData_Audio* ED = (FillBufferDoneSpecificData_Audio*) Node->iThreadSafeHandlerFillBufferDone->iMemoryPool->allocate(sizeof(FillBufferDoneSpecificData_Audio));

        // pack the relevant data into the structure
        ED->hComponent = aComponent;
        ED->pAppData = aAppData;
        ED->pBuffer = aBuffer;

        // convert the pointer into OsclAny ptr
        OsclAny* P = (OsclAny*) ED;

        // CALL the generic callback AO API:
        Node->iThreadSafeHandlerFillBufferDone->ReceiveEvent(P);

        return OMX_ErrorNone;
    }
    else
    {
        OMX_ERRORTYPE status;
        status = Node->FillBufferDoneProcessing(aComponent, aAppData, aBuffer);
        return status;
    }

}

// Callback processing in multithreaded case - dequeued event - call EventHandlerProcessing
OsclReturnCode PVMFOMXAudioDecNode::ProcessCallbackEventHandler_MultiThreaded(OsclAny* P)
{

    // re-cast the pointer

    EventHandlerSpecificData_Audio* ED = (EventHandlerSpecificData_Audio*) P;

    OMX_HANDLETYPE aComponent = ED->hComponent;
    OMX_PTR aAppData = ED->pAppData;
    OMX_EVENTTYPE aEvent = ED->eEvent;
    OMX_U32 aData1 = ED->nData1;
    OMX_U32 aData2 = ED->nData2;
    OMX_PTR aEventData = ED->pEventData;


    EventHandlerProcessing(aComponent, aAppData, aEvent, aData1, aData2, aEventData);


    // release the allocated memory when no longer needed

    iThreadSafeHandlerEventHandler->iMemoryPool->deallocate(ED);
    ED = NULL;

    return OsclSuccess;
}



// Callback processing in multithreaded case - dequeued event - call EmptyBufferDoneProcessing
OsclReturnCode PVMFOMXAudioDecNode::ProcessCallbackEmptyBufferDone_MultiThreaded(OsclAny* P)
{


    // re-cast the pointer
    EmptyBufferDoneSpecificData_Audio* ED = (EmptyBufferDoneSpecificData_Audio*) P;

    OMX_HANDLETYPE aComponent = ED->hComponent;
    OMX_PTR aAppData = ED->pAppData;
    OMX_BUFFERHEADERTYPE* aBuffer = ED->pBuffer;

    EmptyBufferDoneProcessing(aComponent, aAppData, aBuffer);

    // release the allocated memory when no longer needed

    iThreadSafeHandlerEmptyBufferDone->iMemoryPool->deallocate(ED);
    ED = NULL;

    return OsclSuccess;
}


// Callback processing in multithreaded case - dequeued event - call FillBufferDoneProcessing
OsclReturnCode PVMFOMXAudioDecNode::ProcessCallbackFillBufferDone_MultiThreaded(OsclAny* P)
{

    // re-cast the pointer
    FillBufferDoneSpecificData_Audio* ED = (FillBufferDoneSpecificData_Audio*) P;

    OMX_HANDLETYPE aComponent = ED->hComponent;
    OMX_PTR aAppData = ED->pAppData;
    OMX_BUFFERHEADERTYPE* aBuffer = ED->pBuffer;


    FillBufferDoneProcessing(aComponent, aAppData, aBuffer);


    // release the allocated memory when no longer needed

    iThreadSafeHandlerFillBufferDone->iMemoryPool->deallocate(ED);
    ED = NULL;

    return OsclSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Class Distructor
/////////////////////////////////////////////////////////////////////////////
PVMFOMXAudioDecNode::~PVMFOMXAudioDecNode()
{
    LogDiagnostics();

    //Clearup decoder
    DeleteOMXAudioDecoder();
    DeleteLATMParser();
    // Cleanup callback AOs and Mempools
    if (iThreadSafeHandlerEventHandler)
    {
        OSCL_DELETE(iThreadSafeHandlerEventHandler);
        iThreadSafeHandlerEventHandler = NULL;
    }
    if (iThreadSafeHandlerEmptyBufferDone)
    {
        OSCL_DELETE(iThreadSafeHandlerEmptyBufferDone);
        iThreadSafeHandlerEmptyBufferDone = NULL;
    }
    if (iThreadSafeHandlerFillBufferDone)
    {
        OSCL_DELETE(iThreadSafeHandlerFillBufferDone);
        iThreadSafeHandlerFillBufferDone = NULL;
    }

    if (iMediaDataMemPool)
    {
        iMediaDataMemPool->removeRef();
        iMediaDataMemPool = NULL;
    }

    if (iOutBufMemoryPool)
    {
        iOutBufMemoryPool->removeRef();
        iOutBufMemoryPool = NULL;
    }
    if (iInBufMemoryPool)
    {
        iInBufMemoryPool->removeRef();
        iInBufMemoryPool = NULL;
    }

    //Thread logoff
    if (IsAdded())
    {
        RemoveFromScheduler();
        iIsAdded = false;
    }

    //Cleanup allocated interfaces

    //Cleanup allocated ports
    ReleaseAllPorts();

    //Cleanup commands
    //The command queues are self-deleting, but we want to
    //notify the observer of unprocessed commands.
    while (!iCurrentCommand.empty())
    {
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFFailure);
    }
    while (!iInputCommands.empty())
    {
        CommandComplete(iInputCommands, iInputCommands.front(), PVMFFailure);
    }

    //Release Input buffer
    iDataIn.Unbind();

}

/////////////////////////////////////////////////////////////////////////////
// Add AO to the scheduler
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::ThreadLogon()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode:ThreadLogon"));

    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
            {
                AddToScheduler();
                iIsAdded = true;
            }
            iLogger = PVLogger::GetLoggerObject("PVMFOMXAudioDecNode");
            iRunLogger = PVLogger::GetLoggerObject("Run.PVMFOMXAudioDecNode");
            iDataPathLogger = PVLogger::GetLoggerObject("datapath");
            iClockLogger = PVLogger::GetLoggerObject("clock");
            iDiagnosticsLogger = PVLogger::GetLoggerObject("pvplayerdiagnostics.decnode.OMXAudioDecnode");

            SetState(EPVMFNodeIdle);
            return PVMFSuccess;

        default:
            return PVMFErrInvalidState;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Remove AO from the scheduler
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::ThreadLogoff()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode:ThreadLogoff"));

    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            if (IsAdded())
            {
                RemoveFromScheduler();
                iIsAdded = false;
            }
            iLogger = NULL;
            SetState(EPVMFNodeCreated);
            return PVMFSuccess;
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code


        default:
            return PVMFErrInvalidState;
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

    }
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::GetCapability() called"));

    aNodeCapability = iCapability;
    return PVMFSuccess;
}

/////////////////////////////////////////////////////////////////////////////
PVMFPortIter* PVMFOMXAudioDecNode::GetPorts(const PVMFPortFilter* aFilter)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::GetPorts() called"));

    OSCL_UNUSED_ARG(aFilter);

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::QueueCommandL(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVMFCommandId id;

    id = iInputCommands.AddL(aCmd);

    if (iInputCommands.size() == 1)
    {
        //wakeup the AO all the rest of input commands will reschedule the AO in Run
        RunIfNotReady();
    }
    return id;
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::QueryUUID(PVMFSessionId s, const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, PVMFOMXAudioDecNodeAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::QueryUUID() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_QUERYUUID, aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::QueryInterface(PVMFSessionId s, const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::QueryInterface() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_QUERYINTERFACE, aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::RequestPort(PVMFSessionId s, int32 aPortTag, const PvmfMimeString* /* aPortConfig */, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::RequestPort() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_REQUESTPORT, aPortTag, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::ReleasePort(PVMFSessionId s, PVMFPortInterface& aPort, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ReleasePort() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Init(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Init() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_INIT, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Prepare(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Prepare() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_PREPARE, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Start(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Start() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_START, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Stop(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Stop() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_STOP, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Flush(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Flush() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_FLUSH, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Pause(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Pause() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_PAUSE, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::Reset(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Reset() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::CancelAllCommands(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::CancelAllCommands() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_CANCELALL, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::CancelCommand(PVMFSessionId s, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::CancelCommand() called"));
    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommandBase::Construct(s, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_CANCELCMD, aCmdId, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::SetDecoderNodeConfiguration(PVMFOMXAudioDecNodeConfig& aNodeConfig)
{
    iNodeConfig = aNodeConfig;
    return PVMFSuccess;
}


/////////////////////
// Private Section //
/////////////////////

/////////////////////////////////////////////////////////////////////////////
// Class Constructor
/////////////////////////////////////////////////////////////////////////////
PVMFOMXAudioDecNode::PVMFOMXAudioDecNode(int32 aPriority) :
        OsclActiveObject(aPriority, "PVMFOMXAudioDecNode"),
        iInPort(NULL),
        iOutPort(NULL),
        iOutBufMemoryPool(NULL),
        iMediaDataMemPool(NULL),
        iOMXComponentOutputBufferSize(0),
        iOutputAllocSize(0),
        iProcessingState(EPVMFOMXAudioDecNodeProcessingState_Idle),
        iOMXAudioDecoder(NULL),
        iSendBOS(false),
        iStreamID(0),
        iBOSTimestamp(0),
        iSeqNum(0),
        iSeqNum_In(0),
        iIsAdded(true),
        iLogger(NULL),
        iDataPathLogger(NULL),
        iClockLogger(NULL),
        iExtensionRefCount(0),
        iEndOfDataReached(false),
        iEndOfDataTimestamp(0),
        iDiagnosticsLogger(NULL),
        iDiagnosticsLogged(false),
        iNewWidth(0),
        iNewHeight(0),
        iAvgBitrateValue(0),
        iResetInProgress(false),
        iResetMsgSent(false)
{
    iInterfaceState = EPVMFNodeCreated;

    iNodeConfig.iMimeType = PVOMXAUDIODECNODE_CONFIG_MIMETYPE_DEF;


    int32 err;
    OSCL_TRY(err,

             //Create the input command queue.  Use a reserve to avoid lots of
             //dynamic memory allocation.
             iInputCommands.Construct(PVMF_OMXAUDIODEC_NODE_COMMAND_ID_START, PVMF_OMXAUDIODEC_NODE_COMMAND_VECTOR_RESERVE);

             //Create the "current command" queue.  It will only contain one
             //command at a time, so use a reserve of 1.
             iCurrentCommand.Construct(0, 1);

             //Set the node capability data.
             //This node can support an unlimited number of ports.
             iCapability.iCanSupportMultipleInputPorts = false;
             iCapability.iCanSupportMultipleOutputPorts = false;
             iCapability.iHasMaxNumberOfPorts = true;
             iCapability.iMaxNumberOfPorts = 2;
             iCapability.iInputFormatCapability.push_back(PVMF_MPEG4_AUDIO);
             iCapability.iInputFormatCapability.push_back(PVMF_ADIF);
             iCapability.iInputFormatCapability.push_back(PVMF_LATM);
             iCapability.iInputFormatCapability.push_back(PVMF_ASF_MPEG4_AUDIO);
             iCapability.iInputFormatCapability.push_back(PVMF_AAC_SIZEHDR);

             iCapability.iInputFormatCapability.push_back(PVMF_AMR_IF2);
             iCapability.iInputFormatCapability.push_back(PVMF_AMR_IETF);
             iCapability.iInputFormatCapability.push_back(PVMF_AMR_IETF_COMBINED);
             iCapability.iInputFormatCapability.push_back(PVMF_AMRWB_IETF);
             iCapability.iInputFormatCapability.push_back(PVMF_AMRWB_IETF_PAYLOAD);

             iCapability.iInputFormatCapability.push_back(PVMF_MP3);

             iCapability.iInputFormatCapability.push_back(PVMF_WMA);

             iCapability.iOutputFormatCapability.push_back(PVMF_PCM16);

             iAvailableMetadataKeys.reserve(PVMF_OMXAUDIODEC_NUM_METADATA_VALUES);
             iAvailableMetadataKeys.clear();
            );



    iThreadSafeHandlerEventHandler = NULL;
    iThreadSafeHandlerEmptyBufferDone = NULL;
    iThreadSafeHandlerFillBufferDone = NULL;

    iInBufMemoryPool = NULL;
    iOutBufMemoryPool = NULL;

    // init to some value
    iOMXComponentOutputBufferSize = 0;
    iNumOutputBuffers = 0;
    iOMXComponentInputBufferSize = 0;
    iNumInputBuffers = 0;

    iDoNotSendOutputBuffersDownstreamFlag = false;
    iDoNotSaveInputBuffersFlag = false;

    iOutputBuffersFreed = true;// buffers have not been created yet, so they can be considered freed
    iInputBuffersFreed = true;

    // dynamic port reconfig init vars
    iSecondPortReportedChange = false;
    iDynamicReconfigInProgress = false;
    iPauseCommandWasSentToComponent = false;
    iStopCommandWasSentToComponent = false;

    // EOS flag init
    iIsEOSSentToComponent = false;
    iIsEOSReceivedFromComponent = false;

    // LATM init
    iLATMParser = NULL;
    iLATMConfigBuffer = NULL;
    iLATMConfigBufferSize = 0;

    // reset repositioning related flags
    iIsRepositioningRequestSentToComponent = false;
    iIsRepositionDoneReceivedFromComponent = false;
    iIsOutputPortFlushed = false;
    iIsInputPortFlushed = false;

    iIsRepositionIdleDoneReceivedFromComponent = false;
    iIsRepositionIdleRequestSentToComponent = false;
    iIsRepositionExecRequestSentToComponent = false;
    iIsRepositionExecDoneReceivedFromComponent = false;

    // init state of component
    iCurrentDecoderState = OMX_StateInvalid;

    iOutTimeStamp = 0;

    // counts output frames (for logging)
    iFrameCounter = 0;
    //Try Allocate FSI buffer

    // Do This first in case of Query
    OSCL_TRY(err, iFsiFragmentAlloc.size(PVOMXAUDIODEC_MEDIADATA_POOLNUM, sizeof(channelSampleInfo)));


}

/////////////////////////////////////////////////////////////////////////////
// Local Run Routine
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::Run()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Run() In"));

    // if reset is in progress, call DoReset again until Reset Msg is sent
    if ((iResetInProgress == true) &&
            (iResetMsgSent == false) &&
            (iCurrentCommand.size() > 0) &&
            (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
       )
    {
        DoReset(iCurrentCommand.front());
        return; // don't do anything else
    }
    //Check for NODE commands...
    if (!iInputCommands.empty())
    {
        if (ProcessCommand(iInputCommands.front()))
        {
            if (iInterfaceState != EPVMFNodeCreated
                    && (!iInputCommands.empty() || (iInPort && (iInPort->IncomingMsgQueueSize() > 0)) ||
                        (iDataIn.GetRep() != NULL) || (iDynamicReconfigInProgress == true)))
            {
                // reschedule if more data is available, or if port reconfig needs to be finished (even if there is no new data)
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - rescheduling after process command"));
                RunIfNotReady();
            }
            return;
        }

        if (!iInputCommands.empty())
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - rescheduling to process more commands"));
            RunIfNotReady();
        }
    }

    if (((iCurrentCommand.size() == 0) && (iInterfaceState != EPVMFNodeStarted)) ||
            ((iCurrentCommand.size() > 0) && (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_START) && (iInterfaceState != EPVMFNodeStarted)))
    {
        // rescheduling because of input data will be handled in Command Processing Part
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - Node not in Started state yet"));
        return;
    }


    // Process port activity, push out all outgoing messages
    if (iOutPort)
    {
        while (iOutPort->OutgoingMsgQueueSize())
        {
            // if port is busy it is going to wakeup from port ready event
            if (!ProcessOutgoingMsg(iOutPort))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - Outgoing Port Busy, cannot send more msgs"));
                break;
            }
        }
    }

    int loopCount = 0;
    // try to consume all data from input port at once
#if (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_REL)
    uint32 startticks = OsclTickCount::TickCount();
    uint32 starttime = OsclTickCount::TicksToMsec(startticks);
#endif

    do
    {
        // Process port activity if there is no input data that is being processed
        // Do not accept any input if EOS needs to be sent out
        if (iInPort && (iInPort->IncomingMsgQueueSize() > 0) && (iDataIn.GetRep() == NULL) && !iEndOfDataReached)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - Getting more input"));
            if (!ProcessIncomingMsg(iInPort))
            {
                //Re-schedule
                RunIfNotReady();
                return;
            }
        }

        if (iSendBOS)
        {

            // this routine may be re-entered multiple times in multiple Run's before the component goes through cycle execute->idle->execute
            if (!HandleRepositioning())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - Repositioning not done yet"));

                return;
            }

            SendBeginOfMediaStreamCommand();


        }
        // If in init or ready to decode state, process data in the input port if there is input available and input buffers are present
        // (note: at EOS, iDataIn will not be available)

        if ((iDataIn.GetRep() != NULL) ||
                ((iNumOutstandingOutputBuffers < iNumOutputBuffers) &&
                 (iProcessingState == EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode) &&
                 (iResetMsgSent == false)) ||
                ((iDynamicReconfigInProgress == true) && (iResetMsgSent == false))

           )
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::Run() - Calling HandleProcessingState"));

            // input data is available, that means there is video data to be decoded
            if (HandleProcessingState() != PVMFSuccess)
            {
                // If HandleProcessingState does not return Success, we must wait for an event
                // no point in  rescheduling
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXAudioDecNode::Run() - HandleProcessingState did not return Success"));

                return;
            }
        }

        loopCount++;
    }
    while (iInPort &&
            (((iInPort->IncomingMsgQueueSize() > 0) || (iDataIn.GetRep() != NULL)) && (iNumOutstandingInputBuffers < iNumInputBuffers))
            && (!iEndOfDataReached)
            && (iResetMsgSent == false)
          );

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_REL)
    uint32 endticks = OsclTickCount::TickCount();
    uint32 endtime = OsclTickCount::TicksToMsec(endticks);
    uint32 timeinloop = (endtime - starttime);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iRunLogger, PVLOGMSG_INFO,
                    (0, "PVMFOMXAudioDecNode::Run() - LoopCount = %d, Time spent in loop(in ms) = %d, iNumOutstandingInputBuffers = %d, iNumOutstandingOutputBuffers = %d ",
                     loopCount, timeinloop, iNumOutstandingInputBuffers, iNumOutstandingOutputBuffers));
#endif

    // EOS processing:
    // first send an empty buffer to OMX component and mark the EOS flag
    // wait for the OMX component to send async event to indicate that it has reached this EOS buffer
    // then, create and send the EOS message downstream

    if (iEndOfDataReached && !iDynamicReconfigInProgress)
    {

        // if EOS was not sent yet and we have an available input buffer, send EOS buffer to component
        if (!iIsEOSSentToComponent && (iNumOutstandingInputBuffers < iNumInputBuffers))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::Run() - Sending EOS marked buffer To Component "));

            iIsEOSSentToComponent = true;

            // if the component is not yet initialized or if it's in the middle of port reconfig,
            // don't send EOS buffer to component. It does not care. Just set the flag as if we received
            // EOS from the component to enable sending EOS downstream
            if (iProcessingState != EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode)
            {

                iIsEOSReceivedFromComponent = true;
            }
            else if (!SendEOSBufferToOMXComponent())

            {
                // for some reason, Component can't receive the EOS buffer
                // it could be that it is not initialized yet (because EOS could be the first msg). In this case,
                // send the EOS downstream anyway
                iIsEOSReceivedFromComponent = true;
            }
        }

        // we must wait for event (acknowledgment from component)
        // before sending EOS downstream. This is because OMX Component will send
        // the EOS event only after processing remaining buffers

        if (iIsEOSReceivedFromComponent)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::Run() - Received EOS from component, Sending EOS msg downstream "));

            if (iOutPort && iOutPort->IsOutgoingQueueBusy())
            {
                // note: we already tried to empty the outgoing q. If it's still busy,
                // it means that output port is busy. Just return and wait for the port to become free.
                // this will wake up the node and it will send out a msg from the q etc.
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXAudioDecNode::Run() - - EOS cannot be sent downstream, outgoing queue busy - wait"));
                return;
            }

            if (SendEndOfTrackCommand()) // this will only q the EOS
            {
                // EOS send downstream OK, so reset the flag
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXAudioDecNode::Run() - EOS was queued to be sent downstream"));

                iEndOfDataReached = false; // to resume normal processing, reset the flags
                iIsEOSSentToComponent = false;
                iIsEOSReceivedFromComponent = false;

                RunIfNotReady(); // Run again to send out the EOS msg from the outgoing q, and resume
                // normal processing
                ReportInfoEvent(PVMFInfoEndOfData);
            }
        }
        else
        {
            // keep sending output buffers, it's possible that the component needs to flush output
            //	data at the end
            while (iNumOutstandingOutputBuffers < iNumOutputBuffers)
            {
                if (!SendOutputBufferToOMXComponent())
                    break;
            }
        }

    }


    //Check for flash command complition...
    if (iInPort && iOutPort && (iCurrentCommand.size() > 0) &&
            (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_FLUSH) &&
            (iInPort->IncomingMsgQueueSize() == 0) &&
            (iOutPort->OutgoingMsgQueueSize() == 0) &&
            (iDataIn.GetRep() == NULL))
    {
        //flush command is completed
        //Debug check-- all the port queues should be empty at this point.

        OSCL_ASSERT(iInPort->IncomingMsgQueueSize() == 0 && iOutPort->OutgoingMsgQueueSize() == 0);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::Run() - Flush pending"));
        iEndOfDataReached = false;
        iIsEOSSentToComponent = false;
        iIsEOSReceivedFromComponent = false;


        //Flush is complete.  Go to initialized state.
        SetState(EPVMFNodePrepared);
        //resume port input so the ports can be re-started.
        iInPort->ResumeInput();
        iOutPort->ResumeInput();
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::Run() Out"));
}

/////////////////////////////////////////////////////////////////////////////
// This routine will dispatch recived commands
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::ProcessCommand(PVMFOMXAudioDecNodeCommand& aCmd)
{
    //normally this node will not start processing one command
    //until the prior one is finished.  However, a hi priority
    //command such as Cancel must be able to interrupt a command
    //in progress.
    if (!iCurrentCommand.empty() && !aCmd.hipri())
        return false;


    switch (aCmd.iCmd)
    {
        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_QUERYUUID:
            DoQueryUuid(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_REQUESTPORT:
            DoRequestPort(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RELEASEPORT:
            DoReleasePort(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_INIT:
            DoInit(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_PREPARE:
            DoPrepare(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_START:
            DoStart(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_STOP:
            DoStop(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_FLUSH:
            DoFlush(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_PAUSE:
            DoPause(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET:
            DoReset(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_CANCELCMD:
            DoCancelCommand(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_CANCELALL:
            DoCancelAllCommands(aCmd);
            break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_GETNODEMETADATAKEY:
        {
            PVMFStatus retval = DoGetNodeMetadataKey(aCmd);
            CommandComplete(iInputCommands, aCmd, retval);
        }
        break;

        case PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_GETNODEMETADATAVALUE:
        {
            PVMFStatus retval = DoGetNodeMetadataValue(aCmd);
            CommandComplete(iInputCommands, aCmd, retval);
        }
        break;

        default://unknown command type
            CommandComplete(iInputCommands, aCmd, PVMFFailure);
            break;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// This routine will process incomming message from the port
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    //Called by the AO to process one buffer off the port's
    //incoming data queue.  This routine will dequeue and
    //dispatch the data.

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "0x%x PVMFOMXAudioDecNode::ProcessIncomingMsg: aPort=0x%x", this, aPort));

    PVMFStatus status = PVMFFailure;
#ifdef SIMULATE_DROP_MSGS
    if ((((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed % 300 == 299))  // && (((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed < 30) )
    {

        // just dequeue
        PVMFSharedMediaMsgPtr msg;

        status = aPort->DequeueIncomingMsg(msg);
        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
        status = aPort->DequeueIncomingMsg(msg);
        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
        status = aPort->DequeueIncomingMsg(msg);
        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;

#ifdef _DEBUG
        printf("PVMFOMXAudioDecNode::ProcessIncomingMsg() SIMULATED DROP 3 MSGS\n");
#endif


    }
#endif

#ifdef SIMULATE_BOS

    if ((((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed == 6))
    {

        PVMFSharedMediaCmdPtr BOSCmdPtr = PVMFMediaCmd::createMediaCmd();

        // Set the format ID to BOS
        BOSCmdPtr->setFormatID(PVMF_MEDIA_CMD_BOS_FORMAT_ID);

        // Set the timestamp
        BOSCmdPtr->setTimestamp(201);
        BOSCmdPtr->setStreamID(0);

        // Convert to media message and send it out
        PVMFSharedMediaMsgPtr mediaMsgOut;
        convertToPVMFMediaCmdMsg(mediaMsgOut, BOSCmdPtr);

        //store the stream id and time stamp of bos message
        iStreamID = mediaMsgOut->getStreamID();
        iBOSTimestamp = mediaMsgOut->getTimestamp();
        iSendBOS = true;

#ifdef _DEBUG
        printf("PVMFOMXAudioDecNode::ProcessIncomingMsg() SIMULATED BOS\n");
#endif
        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
        return true;

    }
#endif
#ifdef SIMULATE_PREMATURE_EOS
    if (((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed == 5)
    {
        PVMFSharedMediaCmdPtr EOSCmdPtr = PVMFMediaCmd::createMediaCmd();

        // Set the format ID to EOS
        EOSCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

        // Set the timestamp
        EOSCmdPtr->setTimestamp(200);

        // Convert to media message and send it out
        PVMFSharedMediaMsgPtr mediaMsgOut;
        convertToPVMFMediaCmdMsg(mediaMsgOut, EOSCmdPtr);

        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: SIMULATED EOS"));
#ifdef _DEBUG
        printf("PVMFOMXAudioDecNode::ProcessIncomingMsg() SIMULATED EOS\n");
#endif
        // Set EOS flag
        iEndOfDataReached = true;
        // Save the timestamp for the EOS cmd
        iEndOfDataTimestamp = mediaMsgOut->getTimestamp();

        return true;
    }

#endif



    PVMFSharedMediaMsgPtr msg;

    status = aPort->DequeueIncomingMsg(msg);
    if (status != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "0x%x PVMFOMXAudioDecNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed", this));
        return false;
    }

    if (msg->getFormatID() == PVMF_MEDIA_CMD_BOS_FORMAT_ID)
    {
        //store the stream id and time stamp of bos message
        iStreamID = msg->getStreamID();
        iBOSTimestamp = msg->getTimestamp();
        iSendBOS = true;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: Received BOS stream %d, timestamp %d", iStreamID, iBOSTimestamp));
        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
        return true;
    }
    else if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
    {
        // Set EOS flag
        iEndOfDataReached = true;
        // Save the timestamp for the EOS cmd
        iEndOfDataTimestamp = msg->getTimestamp();

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: Received EOS"));

        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
        return true; // do not do conversion into media data, just set the flag and leave
    }


    ///////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    // For LATM data, need to convert to raw bitstream
    if (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_LATM)
    {
        // Keep looping and parsing LATM data until frame complete or data queue runs out
        uint8 retval; //=FRAME_INCOMPLETE;
        // if LATM parser does not exist (very first frame), create it:
        if (iLATMParser == NULL)
        {
            // Create and configure the LATM parser based on the stream MUX config
            // which should be sent as the format specific config in the first media data
            if (CreateLATMParser() != PVMFSuccess)
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::Process Incoming Msg - LATM parser cannot be created"));
                OSCL_ASSERT(false);
                ReportErrorEvent(PVMFErrResourceConfiguration);
                ChangeNodeState(EPVMFNodeError);
                return true;
            }

            // get FSI
            OsclRefCounterMemFrag DataFrag;
            msg->getFormatSpecificInfo(DataFrag);

            //get pointer to the data fragment
            uint8* initbuffer = (uint8 *) DataFrag.getMemFragPtr();
            uint32 initbufsize = (int32) DataFrag.getMemFragSize();

            iLATMConfigBufferSize = initbufsize;
            iLATMConfigBuffer = iLATMParser->ParseStreamMuxConfig(initbuffer, (int32 *) & iLATMConfigBufferSize);
            if (iLATMConfigBuffer == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg() LATM Stream MUX config parsing failed"));
                OSCL_ASSERT(false);
                ReportErrorEvent(PVMFErrResourceConfiguration);
                ChangeNodeState(EPVMFNodeError);
                return true;
            }

        }

        do
        {
            if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                // Set EOS flag
                iEndOfDataReached = true;
                // Save the timestamp for the EOS cmd
                iEndOfDataTimestamp = msg->getTimestamp();

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: Received EOS"));

                ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
                return true; // do not do conversion into media data, just set the flag and leave

            }
            // Convert the next input media msg to media data
            PVMFSharedMediaDataPtr mediaData;
            convertToPVMFMediaData(mediaData, msg);


            ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iDataPathLogger, PVLOGMSG_INFO,
                            (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: TS=%d, SEQNUM= %d", msg->getTimestamp(), msg->getSeqNum()));


            // Convert the LATM data to raw bitstream
            retval = iLATMParser->compose(mediaData);

            // if frame is complete, break out of the loop
            if (retval != FRAME_INCOMPLETE && retval != FRAME_ERROR)
                break;

            // frame is not complete, keep looping
            if (aPort->IncomingMsgQueueSize() == 0)
            {
                // no more data in the input port queue, unbind current msg, and return
                msg.Unbind();
                // enable reading more data from port
                break;
            }
            else
            {
                msg.Unbind();
                aPort->DequeueIncomingMsg(msg); // dequeue the message directly from input port

            }

            // Log parser error
            if (retval == FRAME_ERROR)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFAACDecNode::GetInputMediaData() LATM parser error"));
            }
        }
        while ((retval == FRAME_INCOMPLETE || retval == FRAME_ERROR));

        if (retval == FRAME_COMPLETE)
        {
            // Save the media data containing the parser data as the input media data
            iDataIn = iLATMParser->GetOutputBuffer();
            // set the MARKER bit on the data msg, since this is a complete frame produced by LATM parser
            iDataIn->setMarkerInfo(PVMF_MEDIA_DATA_MARKER_INFO_M_BIT);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO,
                            (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: - LATM frame assembled"));

        }
        else if ((retval == FRAME_INCOMPLETE) || (retval == FRAME_ERROR))
        {
            // Do nothing and wait for more data to come in
            PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO,
                            (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: - incomplete LATM"));
            // return immediately (i.e. don't assign anything to iDataIn, which will prevent
            // processing
            return true;
        }
        else if (retval == FRAME_OUTPUTNOTAVAILABLE)
        {
            // This should not happen since this node processes one parsed media data at a time
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: LATM parser OUTPUT NOT AVAILABLE"));

            msg.Unbind();

            OSCL_ASSERT(false);
            ReportErrorEvent(PVMFErrResourceConfiguration);
            ChangeNodeState(EPVMFNodeError);

            return true;
        }
    }
/////////////////////////////////////////////////////////
    //////////////////////////
    else
    {
        // regular (i.e. Non-LATM case)
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iDataPathLogger, PVLOGMSG_INFO,
                        (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg: TS=%d, SEQNUM= %d", msg->getTimestamp(), msg->getSeqNum()));

        convertToPVMFMediaData(iDataIn, msg);
        ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed++;
    }

    iCurrFragNum = 0; // for new message, reset the fragment counter
    iIsNewDataFragment = true;


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ProcessIncomingMsg() Received %d frames", ((PVMFOMXAudioDecPort*)aPort)->iNumFramesConsumed));

    //return true if we processed an activity...
    return true;
}

/////////////////////////////////////////////////////////////////////////////
// This routine will process outgoing message by sending it into output the port
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::ProcessOutgoingMsg(PVMFPortInterface* aPort)
{
    //Called by the AO to process one message off the outgoing
    //message queue for the given port.  This routine will
    //try to send the data to the connected port.

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "0x%x PVMFOMXAudioDecNode::ProcessOutgoingMsg: aPort=0x%x", this, aPort));

    PVMFStatus status = aPort->Send();
    if (status == PVMFErrBusy)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "0x%x PVMFOMXAudioDecNode::ProcessOutgoingMsg: Connected port goes into busy state", this));
    }

    //Report any unexpected failure in port processing...
    //(the InvalidState error happens when port input is suspended,
    //so don't report it.)
    if (status != PVMFErrBusy
            && status != PVMFSuccess
            && status != PVMFErrInvalidState)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "0x%x PVMFOMXAudioDecNode::Run: Error - ProcessPortActivity failed. port=0x%x, type=%d",
                         this, iOutPort, PVMF_PORT_ACTIVITY_OUTGOING_MSG));
        ReportErrorEvent(PVMFErrPortProcessing);
    }

    //return true if we processed an activity...
    return (status != PVMFErrBusy);
}

/////////////////////////////////////////////////////////////////////////////
// This routine will process received data usign State Machine
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::HandleProcessingState()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::HandleProcessingState() In"));

    PVMFStatus status = PVMFSuccess;

    switch (iProcessingState)
    {
        case EPVMFOMXAudioDecNodeProcessingState_InitDecoder:
        {
            // do init only if input data is available
            if (iDataIn.GetRep() != NULL)
            {
                if (!InitDecoder(iDataIn))
                {
                    // Decoder initialization failed. Fatal error
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Decoder initialization failed"));
                    ReportErrorEvent(PVMFErrResourceConfiguration);
                    ChangeNodeState(EPVMFNodeError);
                    break;
                }

                iProcessingState = EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode;
                // spin once to send output buffers
                RunIfNotReady();
                status = PVMFSuccess; // allow rescheduling
            }
            break;
        }

        case EPVMFOMXAudioDecNodeProcessingState_WaitForInitCompletion:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() WaitForInitCompletion -> wait for config buffer to return"));


            status = PVMFErrNoMemory; // prevent rescheduling
            break;
        }
        // The FOLLOWING 4 states handle Dynamic Port Reconfiguration
        case EPVMFOMXAudioDecNodeProcessingState_PortReconfig:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Sending Port Disable Command"));

            // Collect all buffers first (before starting the portDisable command)
            // FIRST send a flush command. This will return all buffers from the component. Any outstanding buffers are in MIO
            // Then wait for all buffers to come back from MIO. If we haven't sent port disable, we'll be able to process
            // other commands in the copmponent (such as pause, stop etc.)
            OMX_ERRORTYPE err = OMX_ErrorNone;
            OMX_STATETYPE sState;

            // first check the state (if executing or paused, continue)
            err = OMX_GetState(iOMXAudioDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState (): PortReconfig Can't get State of decoder - trying to send port flush request!"));

                sState = OMX_StateInvalid;
                ReportErrorEvent(PVMFErrResourceConfiguration);
                ChangeNodeState(EPVMFNodeError);
                status = PVMFFailure;
                break;
            }

            if ((sState != OMX_StateExecuting) && (sState != OMX_StatePause))
            {
                // possibly as a consequence of a previously queued cmd to go to Idle state?
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState (): PortReconfig: Component State is not executing or paused, do not proceed with port flush"));
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState (): PortReconfig Sending Flush command to component"));

                // the port will now start returning outstanding buffers
                // set the flag to prevent output from going downstream (in case of output port being reconfigured)
                // set the flag to prevent input from being saved and returned to component (in case of input port being reconfigured)
                // set the state to wait for port saying it is disabled
                if (iPortIndexForDynamicReconfig == iOutputPortIndex)
                {
                    iDoNotSendOutputBuffersDownstreamFlag = true;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Output Port"));
                }
                else if (iPortIndexForDynamicReconfig == iInputPortIndex)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Input Port"));

                    iDoNotSaveInputBuffersFlag = true;
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> UNKNOWN PORT"));
                    sState = OMX_StateInvalid;
                    ReportErrorEvent(PVMFErrResourceConfiguration);
                    ChangeNodeState(EPVMFNodeError);
                    status = PVMFFailure;
                    break;
                }

                // send command to flush appropriate port
                err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandFlush, iPortIndexForDynamicReconfig, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState (): PortReconfig : Can't send flush command !"));

                    sState = OMX_StateInvalid;
                    ReportErrorEvent(PVMFErrResourceConfiguration);
                    ChangeNodeState(EPVMFNodeError);
                    status = PVMFFailure;
                    break;
                }
            }
            // now sit back and wait for buffers to return
            // if there is a pause/stop cmd in the meanwhile, component will process it
            // and the node will end up in pause/stop state (so this internal state does not matter)
            iProcessingState = EPVMFOMXAudioDecNodeProcessingState_WaitForBufferReturn;
            // fall through to the next case to check if all buffers are already back
        }

        case EPVMFOMXAudioDecNodeProcessingState_WaitForBufferReturn:
        {
            // as buffers are coming back, Run may be called, wait until all buffers are back, then Free them all

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> WaitForBufferReturn "));
            // check if it's output port being reconfigured
            if (iPortIndexForDynamicReconfig == iOutputPortIndex)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> WaitForBufferReturn Output "));

                // if all buffers have returned, free them
                if (iNumOutstandingOutputBuffers == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> all output buffers are back, send port disable command"));

                    // port reconfiguration is required. Only one port at a time is disabled and then re-enabled after buffer resizing
                    OMX_SendCommand(iOMXAudioDecoder, OMX_CommandPortDisable, iPortIndexForDynamicReconfig, NULL);

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> all output buffers are back, free them"));
                    if (false == iOutputBuffersFreed)
                    {
                        if (!FreeBuffersFromComponent(iOutBufMemoryPool, // allocator
                                                      iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                                      iNumOutputBuffers, // number of buffers
                                                      iOutputPortIndex, // port idx
                                                      false // this is not input
                                                     ))
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot free output buffers "));

                            SetState(EPVMFNodeError);
                            ReportErrorEvent(PVMFErrNoMemory);
                            return PVMFErrNoMemory;
                        }
                    }
                    // if the callback (that port is disabled) has not arrived yet, wait for it
                    // if it has arrived, it will set the state to PortReEnable
                    if (iProcessingState != EPVMFOMXAudioDecNodeProcessingState_PortReEnable)
                        iProcessingState = EPVMFOMXAudioDecNodeProcessingState_WaitForPortDisable;

                    status = PVMFSuccess; // allow rescheduling of the node potentially
                }
                else
                    status = PVMFErrNoMemory; // must wait for buffers to come back. No point in automatic rescheduling
                // but each buffer will reschedule the node when it comes in
            }
            else if (iPortIndexForDynamicReconfig == iInputPortIndex)
            { // this is input port
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> WaitForBufferReturn Input"));


                // if all buffers have returned, free them
                if (iNumOutstandingInputBuffers == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> all input buffers are back, send port disable command"));

                    // port reconfiguration is required. Only one port at a time is disabled and then re-enabled after buffer resizing
                    OMX_SendCommand(iOMXAudioDecoder, OMX_CommandPortDisable, iPortIndexForDynamicReconfig, NULL);

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> all input buffers are back, free them"));


                    if (false == iInputBuffersFreed)
                    {
                        if (!FreeBuffersFromComponent(iInBufMemoryPool, // allocator
                                                      iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                                      iNumInputBuffers, // number of buffers
                                                      iInputPortIndex, // port idx
                                                      true // this is input
                                                     ))
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot free input buffers "));

                            SetState(EPVMFNodeError);
                            ReportErrorEvent(PVMFErrNoMemory);
                            return PVMFErrNoMemory;

                        }
                    }
                    // if the callback (that port is disabled) has not arrived yet, wait for it
                    // if it has arrived, it will set the state to PortReEnable
                    if (iProcessingState != EPVMFOMXAudioDecNodeProcessingState_PortReEnable)
                        iProcessingState = EPVMFOMXAudioDecNodeProcessingState_WaitForPortDisable;

                    status = PVMFSuccess; // allow rescheduling of the node
                }
                else
                    status = PVMFErrNoMemory; // must wait for buffers to come back. No point in automatic
                // rescheduling. Each buffer will reschedule the node
                // when it comes in
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState() WaitForBufferReturn -> UNKNOWN PORT"));
            }

            // the state will be changed to PortReEnable once we get confirmation that Port was actually disabled
            break;
        }

        case EPVMFOMXAudioDecNodeProcessingState_WaitForPortDisable:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> wait for port disable callback"));
            // do nothing. Just wait for the port to become disabled (we'll get event from component, which will
            // transition the state to PortReEnable
            status = PVMFErrNoMemory; // prevent Rescheduling the node
            break;
        }

        case EPVMFOMXAudioDecNodeProcessingState_PortReEnable:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Sending reenable port command"));
            // set the port index so that we get parameters for the proper port
            iParamPort.nPortIndex = iPortIndexForDynamicReconfig;
            // iParamPort.nVersion = OMX_VERSION;

            // get new parameters of the port
            OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);

            // send command for port re-enabling (for this to happen, we must first recreate the buffers)
            OMX_SendCommand(iOMXAudioDecoder, OMX_CommandPortEnable, iPortIndexForDynamicReconfig, NULL);


            // get also input info (for frame duration if necessary)
            OMX_ERRORTYPE Err;
            OMX_PTR CodecProfilePtr;
            OMX_INDEXTYPE CodecProfileIndx;
            OMX_AUDIO_PARAM_AACPROFILETYPE Audio_Aac_Param;

            // determine the proper index and structure (based on codec type)
            if (iInPort)
            {
                switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
                {
                        // AAC
                    case PVMF_MPEG4_AUDIO:
                    case PVMF_LATM:
                    case PVMF_ADIF:
                    case PVMF_ASF_MPEG4_AUDIO:
                    case PVMF_AAC_SIZEHDR: // for testing
                        CodecProfilePtr = (OMX_PTR) & Audio_Aac_Param;
                        CodecProfileIndx = OMX_IndexParamAudioAac;
                        Audio_Aac_Param.nPortIndex = iInputPortIndex;
                        Audio_Aac_Param.nSize = sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE);
                        Audio_Aac_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
                        Audio_Aac_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
                        Audio_Aac_Param.nVersion.s.nRevision = SPECREVISION;
                        Audio_Aac_Param.nVersion.s.nStep = SPECSTEP;

                        // get parameters:
                        Err = OMX_GetParameter(iOMXAudioDecoder, CodecProfileIndx, CodecProfilePtr);
                        if (Err != OMX_ErrorNone)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Input port parameters problem"));

                            SetState(EPVMFNodeError);
                            ReportErrorEvent(PVMFErrResource);
                            return PVMFErrResource;
                        }

                        break;

                    default:
                        break;
                }
            }




            switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
            {
                case PVMF_MPEG4_AUDIO:
                case PVMF_LATM:
                case PVMF_ADIF:
                case PVMF_ASF_MPEG4_AUDIO:
                case PVMF_AAC_SIZEHDR: // for testing
                    iSamplesPerFrame = Audio_Aac_Param.nFrameLength;
                    break;

                    // AMR
                case PVMF_AMR_IF2:
                case PVMF_AMR_IETF:
                case PVMF_AMR_IETF_COMBINED:
                    // AMR NB has fs=8khz Mono and the frame is 20ms long, i.e. there is 160 samples per frame
                    iSamplesPerFrame = PVOMXAUDIODEC_AMRNB_SAMPLES_PER_FRAME;
                    break;

                case PVMF_AMRWB_IETF:
                case PVMF_AMRWB_IETF_PAYLOAD:
                    // AMR WB has fs=16khz Mono and the frame is 20ms long, i.e. there is 320 samples per frame
                    iSamplesPerFrame = PVOMXAUDIODEC_AMRWB_SAMPLES_PER_FRAME;
                    break;

                case PVMF_MP3:
                    // frame size is either 576 or 1152 samples per frame. However, this information cannot be
                    // obtained through OMX MP3 Params. Assume that it's 1152
                    iSamplesPerFrame = PVOMXAUDIODEC_MP3_DEFAULT_SAMPLES_PER_FRAME;
                    break;

                case PVMF_WMA:
                    // output frame size is unknown in WMA. However, the PV-WMA decoder can control the number
                    // of samples it places in an output buffer, so we can create an output buffer of arbitrary size
                    // and let the decoder control how it is filled
                    iSamplesPerFrame = 0; // unknown
                    break;

                default:
                    break;
            }

            if (iPortIndexForDynamicReconfig == iOutputPortIndex)
            {

                // GET the output buffer params and sizes
                OMX_AUDIO_PARAM_PCMMODETYPE Audio_Pcm_Param;
                Audio_Pcm_Param.nPortIndex = iOutputPortIndex; // we're looking for output port params
                Audio_Pcm_Param.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
                Audio_Pcm_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
                Audio_Pcm_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
                Audio_Pcm_Param.nVersion.s.nRevision = SPECREVISION;
                Audio_Pcm_Param.nVersion.s.nStep = SPECSTEP;


                Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamAudioPcm, &Audio_Pcm_Param);
                if (Err != OMX_ErrorNone)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot get component output parameters"));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrResource);
                    return PVMFErrResource;
                }

                iPCMSamplingRate = Audio_Pcm_Param.nSamplingRate; // can be set to 0 (if unknown)

                if (iPCMSamplingRate == 0) // use default sampling rate (i.e. 48000)
                    iPCMSamplingRate = PVOMXAUDIODEC_DEFAULT_SAMPLINGRATE;

                iNumberOfAudioChannels = Audio_Pcm_Param.nChannels;		// should be 1 or 2
                if (iNumberOfAudioChannels != 1 && iNumberOfAudioChannels != 2)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Output parameters num channels = %d", iNumberOfAudioChannels));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrResource);
                    return PVMFErrResource;
                }

                if (iSamplesPerFrame != 0) // if this value is known
                {
                    // CALCULATE NumBytes per frame, Msec per frame, etc.
                    iNumBytesPerFrame = 2 * iSamplesPerFrame * iNumberOfAudioChannels;
                    iMilliSecPerFrame = (iSamplesPerFrame * 1000) / iPCMSamplingRate;
                    // Determine the size of each PCM output buffer. Size would be big enough to hold certain time amount of PCM data
                    uint32 numframes = PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME / iMilliSecPerFrame;

                    if (PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME % iMilliSecPerFrame)
                    {
                        // If there is a remainder, include one more frame
                        ++numframes;
                    }

                    // set the output buffer size accordingly:
                    iOMXComponentOutputBufferSize = numframes * iNumBytesPerFrame;
                }
                else
                    iOMXComponentOutputBufferSize = (iPCMSamplingRate * 1000) / (PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME);

                // do we need to increase the number of buffers?
                if (iNumOutputBuffers < iParamPort.nBufferCountMin)
                    iNumOutputBuffers = iParamPort.nBufferCountMin;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState() new output buffers %d, size %d", iNumOutputBuffers, iOMXComponentOutputBufferSize));

                /* Allocate output buffers */
                if (!CreateOutMemPool(iNumOutputBuffers))
                {

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot allocate output buffers "));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrNoMemory);
                    return PVMFErrNoMemory;
                }

                if (!ProvideBuffersToComponent(iOutBufMemoryPool, // allocator
                                               iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                               iNumOutputBuffers, // number of buffers
                                               iOMXComponentOutputBufferSize, // actual buffer size
                                               iOutputPortIndex, // port idx
                                               iOMXComponentSupportsExternalOutputBufferAlloc, // can component use OMX_UseBuffer
                                               false // this is not input
                                              ))
                {


                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot provide output buffers to component"));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrNoMemory);
                    return PVMFErrNoMemory;

                }

                // do not drop output any more, i.e. enable output to be sent downstream
                iDoNotSendOutputBuffersDownstreamFlag = false;


            }
            else
            {
                // this is input port

                iOMXComponentInputBufferSize = iParamPort.nBufferSize;
                // do we need to increase the number of buffers?
                if (iNumInputBuffers < iParamPort.nBufferCountMin)
                    iNumInputBuffers = iParamPort.nBufferCountMin;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState() new buffers %d, size %d", iNumInputBuffers, iOMXComponentInputBufferSize));

                /* Allocate input buffers */
                if (!CreateInputMemPool(iNumInputBuffers))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot allocate new input buffers to component"));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrNoMemory);
                    return PVMFErrNoMemory;
                }

                if (!ProvideBuffersToComponent(iInBufMemoryPool, // allocator
                                               iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                               iNumInputBuffers, // number of buffers
                                               iOMXComponentInputBufferSize, // actual buffer size
                                               iInputPortIndex, // port idx
                                               iOMXComponentSupportsExternalInputBufferAlloc, // can component use OMX_UseBuffer
                                               true // this is input
                                              ))
                {


                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> Cannot provide new input buffers to component"));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrNoMemory);
                    return PVMFErrNoMemory;

                }
                // do not drop partially consumed input
                iDoNotSaveInputBuffersFlag = false;

            }

            // if the callback that the port was re-enabled has not arrived yet, wait for it
            // if it has arrived, it will set the state to either PortReconfig or to ReadyToDecode
            if (iProcessingState != EPVMFOMXAudioDecNodeProcessingState_PortReconfig &&
                    iProcessingState != EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode)
                iProcessingState = EPVMFOMXAudioDecNodeProcessingState_WaitForPortEnable;

            status = PVMFSuccess; // allow rescheduling of the node
            break;
        }

        case EPVMFOMXAudioDecNodeProcessingState_WaitForPortEnable:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Port Reconfiguration -> wait for port enable callback"));
            // do nothing. Just wait for the port to become enabled (we'll get event from component, which will
            // transition the state to ReadyToDecode
            status = PVMFErrNoMemory; // prevent ReScheduling
            break;
        }

        // NORMAL DATA FLOW STATE:
        case EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Ready To Decode start"));
            // In normal data flow and decoding state
            // Send all available output buffers to the decoder

            while (iNumOutstandingOutputBuffers < iNumOutputBuffers)
            {
                // grab buffer header from the mempool if possible, and send to component
                if (!SendOutputBufferToOMXComponent())

                    break;

            }


            // next, see if partially consumed input buffer needs to be resent back to OMX component
            // NOTE: it is not allowed that the component returns more than 1 partially consumed input buffers
            //		 i.e. if a partially consumed input buffer is returned, it is assumed that the OMX component
            //		 will be waiting to get data

            if (iInputBufferToResendToComponent != NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXAudioDecNode::HandleProcessingState() Sending previous - partially consumed input back to the OMX component"));

                OMX_EmptyThisBuffer(iOMXAudioDecoder, iInputBufferToResendToComponent);
                iInputBufferToResendToComponent = NULL; // do this only once
            }
            else if ((iNumOutstandingInputBuffers < iNumInputBuffers) && (iDataIn.GetRep() != NULL))
            {
                // try to get an input buffer header
                // and send the input data over to the component
                SendInputBufferToOMXComponent();
            }

            status = PVMFSuccess;
            break;


        }
        case EPVMFOMXAudioDecNodeProcessingState_Stopping:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Stopping -> wait for Component to move from Executing->Idle"));


            status = PVMFErrNoMemory; // prevent rescheduling
            break;
        }

        case EPVMFOMXAudioDecNodeProcessingState_Pausing:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleProcessingState() Pausing -> wait for Component to move from Executing->Pause"));


            status = PVMFErrNoMemory; // prevent rescheduling
            break;
        }


        case EPVMFOMXAudioDecNodeProcessingState_WaitForOutgoingQueue:
            status = PVMFErrNoMemory;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::HandleProcessingState() Do nothing since waiting for output port queue to become available"));
            break;

        default:
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::HandleProcessingState() Out"));

    return status;

}
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::SendOutputBufferToOMXComponent()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendOutputBufferToOMXComponent() In"));


    OutputBufCtrlStruct_Audio *output_buf = NULL;
    int32 errcode = 0;

    // try to get output buffer header
    OSCL_TRY(errcode, output_buf = (OutputBufCtrlStruct_Audio *) iOutBufMemoryPool->allocate(iOutputAllocSize));
    if (errcode != 0)
    {
        if (errcode == OsclErrNoResources)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                            PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::SendOutputBufferToOMXComponent() No more output buffers in the mempool"));

            iOutBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny *) iOutBufMemoryPool); // To signal when next deallocate() is called on mempool

            return false;
        }
        else
        {
            // Memory allocation for the pool failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::SendOutputBufferToOMXComponent() Output mempool error"));


            SetState(EPVMFNodeError);
            ReportErrorEvent(PVMFErrNoMemory);
            return false;
        }

    }

    //for every allocated buffer, make sure you notify when buffer is released. Keep track of allocated buffers
    // use mempool as context to recognize which buffer (input or output) was returned
    iOutBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny *)iOutBufMemoryPool);
    iNumOutstandingOutputBuffers++;

    output_buf->pBufHdr->nFilledLen = 0; // make sure you tell OMX component buffer is empty
    output_buf->pBufHdr->nOffset = 0;
    output_buf->pBufHdr->pAppPrivate = output_buf; // set pAppPrivate to be pointer to output_buf
    // (this is context for future release of this buffer to the mempool)
    // this was done during buffer creation, but still repeat just in case
    output_buf->pBufHdr->nFlags = 0; //Clear flags
    OMX_FillThisBuffer(iOMXAudioDecoder, output_buf->pBufHdr);



    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendOutputBufferToOMXComponent() Out"));

    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::NegotiateComponentParameters()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() In"));

    OMX_ERRORTYPE Err;
    // first get the number of ports and port indices
    OMX_PORT_PARAM_TYPE AudioPortParameters;
    uint32 NumPorts;
    uint32 ii;

    // get starting number
    Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamAudioInit, &AudioPortParameters);
    NumPorts = AudioPortParameters.nPorts; // must be at least 2 of them (in&out)

    if (Err != OMX_ErrorNone || NumPorts < 2)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() There is insuffucient (%d) ports", NumPorts));
        return false;
    }


    // loop through video ports starting from the starting index to find index of the first input port
    for (ii = AudioPortParameters.nStartPortNumber ;ii < AudioPortParameters.nStartPortNumber + NumPorts; ii++)
    {
        // get port parameters, and determine if it is input or output
        // if there are more than 2 ports, the first one we encounter that has input direction is picked

        iParamPort.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);

        //port
        iParamPort.nPortIndex = ii;
        Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);

        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem negotiating with port %d ", ii));

            return false;
        }

        if (iParamPort.eDir == OMX_DirInput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Found Input port index %d ", ii));

            iInputPortIndex = ii;
            break;
        }
    }
    if (ii == AudioPortParameters.nStartPortNumber + NumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Cannot find any input port "));
        return false;
    }


    // loop through video ports starting from the starting index to find index of the first output port
    for (ii = AudioPortParameters.nStartPortNumber ;ii < AudioPortParameters.nStartPortNumber + NumPorts; ii++)
    {
        // get port parameters, and determine if it is input or output
        // if there are more than 2 ports, the first one we encounter that has output direction is picked

        iParamPort.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);

        //port
        iParamPort.nPortIndex = ii;
        Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);

        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem negotiating with port %d ", ii));

            return false;
        }

        if (iParamPort.eDir == OMX_DirOutput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Found Output port index %d ", ii));

            iOutputPortIndex = ii;
            break;
        }
    }
    if (ii == AudioPortParameters.nStartPortNumber + NumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Cannot find any output port "));
        return false;
    }



    // now get input parameters
    iParamPort.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);

    //Input port
    iParamPort.nPortIndex = iInputPortIndex;
    Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem negotiating with input port %d ", iInputPortIndex));
        return false;
    }

    // preset the number of input buffers
    iNumInputBuffers = NUMBER_INPUT_BUFFER;

    // do we need to increase the number of buffers?
    if (iNumInputBuffers < iParamPort.nBufferCountMin)
        iNumInputBuffers = iParamPort.nBufferCountMin;
    iOMXComponentInputBufferSize = iParamPort.nBufferSize;

    iParamPort.nBufferCountActual = iNumInputBuffers;

    // set the number of actual input buffers
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Inport buffers %d,size %d", iNumInputBuffers, iOMXComponentInputBufferSize));

    Err = OMX_SetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem setting parameters in input port %d ", iInputPortIndex));
        return false;
    }




    // Codec specific info set/get: SamplingRate, formats etc.
    if (!GetSetCodecSpecificInfo())
        return false;


    //Port 1 for output port
    iParamPort.nPortIndex = iOutputPortIndex;
    Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem negotiating with output port %d ", iOutputPortIndex));
        return false;
    }

    // set number of output buffers and the size
    iNumOutputBuffers = NUMBER_OUTPUT_BUFFER;


    if (iNumOutputBuffers < iParamPort.nBufferCountMin)
        iNumOutputBuffers = iParamPort.nBufferCountMin;

    iParamPort.nBufferCountActual = iNumOutputBuffers;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Outport buffers %d,size %d", iNumOutputBuffers, iOMXComponentOutputBufferSize));

    Err = OMX_SetParameter(iOMXAudioDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem setting parameters in output port %d ", iOutputPortIndex));
        return false;
    }

    return true;
}

bool PVMFOMXAudioDecNode::GetSetCodecSpecificInfo()
{

    // for AAC, need to let the decoder know about the type of AAC format. Need to get the frame length
    // need to get the parameters
    OMX_PTR CodecProfilePtr;
    OMX_INDEXTYPE CodecProfileIndx;
    OMX_AUDIO_PARAM_AACPROFILETYPE Audio_Aac_Param;
    OMX_AUDIO_PARAM_AMRTYPE Audio_Amr_Param;
    OMX_AUDIO_PARAM_MP3TYPE Audio_Mp3_Param;
    OMX_AUDIO_PARAM_WMATYPE Audio_Wma_Param;
    OMX_ERRORTYPE Err = OMX_ErrorNone;

    // determine the proper index and structure (based on codec type)
    switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
    {

            // AAC
        case PVMF_MPEG4_AUDIO:
        case PVMF_LATM:
        case PVMF_ADIF:
        case PVMF_ASF_MPEG4_AUDIO:
        case PVMF_AAC_SIZEHDR: // for testing
            CodecProfilePtr = (OMX_PTR) & Audio_Aac_Param;
            CodecProfileIndx = OMX_IndexParamAudioAac;
            Audio_Aac_Param.nPortIndex = iInputPortIndex;
            Audio_Aac_Param.nSize = sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE);
            Audio_Aac_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
            Audio_Aac_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
            Audio_Aac_Param.nVersion.s.nRevision = SPECREVISION;
            Audio_Aac_Param.nVersion.s.nStep = SPECSTEP;
            break;

            // AMR
        case PVMF_AMR_IF2:
        case PVMF_AMR_IETF:
        case PVMF_AMR_IETF_COMBINED:
        case PVMF_AMRWB_IETF:
        case PVMF_AMRWB_IETF_PAYLOAD:
            CodecProfilePtr = (OMX_PTR) & Audio_Amr_Param;
            CodecProfileIndx = OMX_IndexParamAudioAmr;
            Audio_Amr_Param.nPortIndex = iInputPortIndex;
            Audio_Amr_Param.nSize = sizeof(OMX_AUDIO_PARAM_AMRTYPE);
            Audio_Amr_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
            Audio_Amr_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
            Audio_Amr_Param.nVersion.s.nRevision = SPECREVISION;
            Audio_Amr_Param.nVersion.s.nStep = SPECSTEP;
            break;
            // MP3
        case PVMF_MP3:
            CodecProfilePtr = (OMX_PTR) & Audio_Mp3_Param;
            CodecProfileIndx = OMX_IndexParamAudioMp3;
            Audio_Mp3_Param.nPortIndex = iInputPortIndex;
            Audio_Mp3_Param.nSize = sizeof(OMX_AUDIO_PARAM_MP3TYPE);
            Audio_Mp3_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
            Audio_Mp3_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
            Audio_Mp3_Param.nVersion.s.nRevision = SPECREVISION;
            Audio_Mp3_Param.nVersion.s.nStep = SPECSTEP;
            break;
            //WMA
        case PVMF_WMA:
            CodecProfilePtr = (OMX_PTR) & Audio_Wma_Param;
            CodecProfileIndx = OMX_IndexParamAudioWma;
            Audio_Wma_Param.nPortIndex = iInputPortIndex;
            Audio_Wma_Param.nSize = sizeof(OMX_AUDIO_PARAM_WMATYPE);
            Audio_Wma_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
            Audio_Wma_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
            Audio_Wma_Param.nVersion.s.nRevision = SPECREVISION;
            Audio_Wma_Param.nVersion.s.nStep = SPECSTEP;
            break;

        default:
            break;
    }

    // first get parameters:
    Err = OMX_GetParameter(iOMXAudioDecoder, CodecProfileIndx, CodecProfilePtr);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem getting codec profile parameter on input port %d ", iInputPortIndex));
        return false;
    }
    // Set the stream format
    switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
    {

            // AAC FORMATS:
        case PVMF_MPEG4_AUDIO:
            Audio_Aac_Param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4ADTS;
            break;
        case PVMF_LATM:
            Audio_Aac_Param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4LATM;
            break;
        case PVMF_ADIF:
            Audio_Aac_Param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatADIF;

            break;
        case PVMF_ASF_MPEG4_AUDIO:
            Audio_Aac_Param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4ADTS;

            break;
        case PVMF_AAC_SIZEHDR: // for testing
            Audio_Aac_Param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4ADTS;
            break;


            // AMR FORMATS
        case PVMF_AMR_IF2:
            Audio_Amr_Param.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatIF2;
            Audio_Amr_Param.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0; // we don't know the bitrate yet, but for init
            // purposes, we'll set this to any NarrowBand bitrate
            // to indicate NB vs WB

            break;

            // File format
            // NB
        case PVMF_AMR_IETF:
            Audio_Amr_Param.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
            Audio_Amr_Param.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0; // we don't know the bitrate yet, but for init
            // purposes, we'll set this to any NarrowBand bitrate
            // to indicate NB vs WB
            break;

            // WB
        case PVMF_AMRWB_IETF:
            Audio_Amr_Param.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
            Audio_Amr_Param.eAMRBandMode = OMX_AUDIO_AMRBandModeWB0; // we don't know the bitrate yet, but for init
            // purposes, we'll set this to any WideBand bitrate
            // to indicate NB vs WB

            break;

            // streaming with Table of Contents

        case PVMF_AMR_IETF_COMBINED:
            Audio_Amr_Param.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatRTPPayload;
            Audio_Amr_Param.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0; // we don't know the bitrate yet, but for init
            // purposes, we'll set this to any WideBand bitrate
            // to indicate NB vs WB

            break;
        case PVMF_AMRWB_IETF_PAYLOAD:
            Audio_Amr_Param.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatRTPPayload;
            Audio_Amr_Param.eAMRBandMode = OMX_AUDIO_AMRBandModeWB0; // we don't know the bitrate yet, but for init
            // purposes, we'll set this to any WideBand bitrate
            // to indicate NB vs WB

            break;

        case PVMF_MP3:
            // nothing to do here
            break;

        case PVMF_WMA:
            Audio_Wma_Param.eFormat = OMX_AUDIO_WMAFormatUnused; // set this initially
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Unknown format in input port negotiation "));
            return false;

    }



    // set parameters to inform teh component of the stream type
    Err = OMX_SetParameter(iOMXAudioDecoder, CodecProfileIndx, CodecProfilePtr);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem setting codec profile parameter on input port %d ", iInputPortIndex));
        return false;
    }


    // read the output frame size

    switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
    {
            // AAC
        case PVMF_MPEG4_AUDIO:
        case PVMF_LATM:
        case PVMF_ADIF:
        case PVMF_ASF_MPEG4_AUDIO:
        case PVMF_AAC_SIZEHDR: // for testing
            // AAC frame size is 1024 samples or 2048 samples for AAC-HE
            iSamplesPerFrame = Audio_Aac_Param.nFrameLength;
            break;

            // AMR
        case PVMF_AMR_IF2:
        case PVMF_AMR_IETF:
        case PVMF_AMR_IETF_COMBINED:
            // AMR NB has fs=8khz Mono and the frame is 20ms long, i.e. there is 160 samples per frame
            iSamplesPerFrame = PVOMXAUDIODEC_AMRNB_SAMPLES_PER_FRAME;
            break;

        case PVMF_AMRWB_IETF:
        case PVMF_AMRWB_IETF_PAYLOAD:
            // AMR WB has fs=16khz Mono and the frame is 20ms long, i.e. there is 320 samples per frame
            iSamplesPerFrame = PVOMXAUDIODEC_AMRWB_SAMPLES_PER_FRAME;
            break;

        case PVMF_MP3:
            // frame size is either 576 or 1152 samples per frame. However, this information cannot be
            // obtained through OMX MP3 Params. Assume that it's 1152
            iSamplesPerFrame = PVOMXAUDIODEC_MP3_DEFAULT_SAMPLES_PER_FRAME;
            break;
        case PVMF_WMA:
            // output frame size is unknown in WMA. However, the PV-WMA decoder can control the number
            // of samples it places in an output buffer, so we can create an output buffer of arbitrary size
            // and let the decoder control how it is filled
            iSamplesPerFrame = 0; // unknown
            break;

        default:
            break;
    }


    // iSamplesPerFrame depends on the codec.
    // for AAC: iSamplesPerFrame = 1024
    // for AAC+: iSamplesPerFrame = 2048
    // for AMRNB: iSamplesPerFrame = 160
    // for AMRWB: iSamplesPerFrame = 320
    // for MP3:	  iSamplesPerFrame = unknown, but either 1152 or 576 (we pick 1152 as default)
    // for WMA:	   unknown (iSamplesPerFrame is set to 0)

    // GET the output buffer params and sizes
    OMX_AUDIO_PARAM_PCMMODETYPE Audio_Pcm_Param;
    Audio_Pcm_Param.nPortIndex = iOutputPortIndex; // we're looking for output port params
    Audio_Pcm_Param.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    Audio_Pcm_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    Audio_Pcm_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    Audio_Pcm_Param.nVersion.s.nRevision = SPECREVISION;
    Audio_Pcm_Param.nVersion.s.nStep = SPECSTEP;


    Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamAudioPcm, &Audio_Pcm_Param);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::NegotiateComponentParameters() Problem negotiating PCM parameters with output port %d ", iOutputPortIndex));
        return false;
    }


    // these are some initial default values that may change
    iPCMSamplingRate = Audio_Pcm_Param.nSamplingRate; // can be set to 0 (if unknown)

    if (iPCMSamplingRate == 0) // use default sampling rate (i.e. 48000)
        iPCMSamplingRate = PVOMXAUDIODEC_DEFAULT_SAMPLINGRATE;

    iNumberOfAudioChannels = Audio_Pcm_Param.nChannels;		// should be 1 or 2
    if (iNumberOfAudioChannels != 1 && iNumberOfAudioChannels != 2)
        return false;


    if (iSamplesPerFrame != 0) // if this value is known
    {
        // CALCULATE NumBytes per frame, Msec per frame, etc.

        iNumBytesPerFrame = 2 * iSamplesPerFrame * iNumberOfAudioChannels;
        iMilliSecPerFrame = (iSamplesPerFrame * 1000) / iPCMSamplingRate;
        // Determine the size of each PCM output buffer. Size would be big enough to hold certain time amount of PCM data
        uint32 numframes = PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME / iMilliSecPerFrame;

        if (PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME % iMilliSecPerFrame)
        {
            // If there is a remainder, include one more frame
            ++numframes;
        }
        // set the output buffer size accordingly:
        iOMXComponentOutputBufferSize = numframes * iNumBytesPerFrame;
    }
    else
        iOMXComponentOutputBufferSize = (iPCMSamplingRate * 1000) / (PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME);

    return true;

}

bool PVMFOMXAudioDecNode::SetDefaultCapabilityFlags()
{

    iIsOMXComponentMultiThreaded = true;

    iOMXComponentSupportsExternalOutputBufferAlloc = true;
    iOMXComponentSupportsExternalInputBufferAlloc = true;
    iOMXComponentSupportsMovableInputBuffers = true; //false;


    return true;
}



bool PVMFOMXAudioDecNode::SendEOSBufferToOMXComponent()
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendEOSBufferToOMXComponent() In"));


    // first of all, check if the component is running. EOS could be sent prior to component/decoder
    // even being initialized

    // returning false will ensure that the EOS will be sent downstream anyway without waiting for the
    // Component to respond
    if (iCurrentDecoderState != OMX_StateExecuting)
        return false;

    // get an input buffer. Without a buffer, no point in proceeding
    InputBufCtrlStruct_Audio *input_buf = NULL;
    int32 errcode = 0;

    // we already checked that the number of buffers is OK, so we don't expect problems
    // try to get input buffer header
    OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct_Audio *) iInBufMemoryPool->allocate(iInputAllocSize));
    if (errcode != 0)
    {
        if (errcode == OsclErrNoResources)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                            PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::SendEOSBufferToOMXComponent() No more buffers in the mempool - unexpected"));

            iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool); // To signal when next deallocate() is called on mempool

            return false;
        }
        else
        {
            // Memory allocation for the pool failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::SendEOSBufferToOMXComponent() Input mempool error"));


            SetState(EPVMFNodeError);
            ReportErrorEvent(PVMFErrNoMemory);
            return false;
        }

    }

    // keep track of buffers. When buffer is deallocated/released, the counter will be decremented
    iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool);
    iNumOutstandingInputBuffers++;

    // in this case, no need to use input msg refcounter. Make sure its unbound
    (input_buf->pMediaData).Unbind();

    // THIS IS AN EMPTY BUFFER. FLAGS ARE THE ONLY IMPORTANT THING
    input_buf->pBufHdr->nFilledLen = 0;
    input_buf->pBufHdr->nOffset = 0;
    input_buf->pBufHdr->nTimeStamp = iEndOfDataTimestamp;

    // set ptr to input_buf structure for Context (for when the buffer is returned)
    input_buf->pBufHdr->pAppPrivate = (OMX_PTR) input_buf;

    // do not use Mark here (but init to NULL to prevent problems)
    input_buf->pBufHdr->hMarkTargetComponent = NULL;
    input_buf->pBufHdr->pMarkData = NULL;


    // init buffer flags
    input_buf->pBufHdr->nFlags = 0;

    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
    // most importantly, set the EOS flag:
    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;

    // send buffer to component
    OMX_EmptyThisBuffer(iOMXAudioDecoder, input_buf->pBufHdr);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendEOSBufferToOMXComponent() Out"));

    return true;

}


bool PVMFOMXAudioDecNode::SendInputBufferToOMXComponent()
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() In"));


    // first of all , get an input buffer. Without a buffer, no point in proceeding
    InputBufCtrlStruct_Audio *input_buf = NULL;
    int32 errcode = 0;

    do
    {
        // do loop to loop over all fragments

        // try to get input buffer header
        OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct_Audio *) iInBufMemoryPool->allocate(iInputAllocSize));
        if (errcode != 0)
        {
            if (errcode == OsclErrNoResources)
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                                PVLOGMSG_DEBUG, (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() No more buffers in the mempool"));

                iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool); // To signal when next deallocate() is called on mempool

                return false;
            }
            else
            {
                // Memory allocation for the pool failed
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() Input mempool error"));


                SetState(EPVMFNodeError);
                ReportErrorEvent(PVMFErrNoMemory);
                return false;
            }

        }

        // keep track of buffers. When buffer is deallocated/released, the counter will be decremented
        iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool);
        iNumOutstandingInputBuffers++;

        // Now we have the buffer header (i.e. a buffer) to send to component:
        // Depending on OMX component capabilities, either pass the input msg fragment(s) directly
        //	into OMX component without copying (and update the input msg refcount)
        //	or memcopy the content of input msg memfrag(s) into OMX component allocated buffers

        // When copying content, a special case is when the input fragment is larger than the buffer and has to
        //	be fragmented here and broken over 2 or more buffers. Potential problem with available buffers etc.

        // if this is the first fragment in a new message, extract some info:
        if (iCurrFragNum == 0)
        {

            // NOTE: SeqNum differ in Codec and in Node because of the fact that
            // one msg can contain multiple fragments that are sent to the codec as
            // separate buffers. Node tracks msgs and codec tracks even separate fragments

            iCodecSeqNum += (iDataIn->getSeqNum() - iInPacketSeqNum); // increment the codec seq. # by the same
            // amount that the input seq. number increased

            iInPacketSeqNum = iDataIn->getSeqNum(); // remember input sequence number
            iInTimestamp = iDataIn->getTimestamp();
            iInDuration = iDataIn->getDuration();
            iInNumFrags = iDataIn->getNumFragments();

            if (iSetMarkerBitForEveryFrag == true)
            {
                iCurrentMsgMarkerBit = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT;
            }
            else
            {
                iCurrentMsgMarkerBit = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
            }

            //Force marker bit for AMR streaming formats (marker bit may not be set even though full frames are present)
            if (iInPort && (
                        (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IETF_COMBINED) ||
                        (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMRWB_IETF_PAYLOAD) ||
                        (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_MP3)
                    )
               )
            {
                iCurrentMsgMarkerBit = PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
            }

            // logging info:
            if (iDataIn->getNumFragments() > 1)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - New msg has MULTI-FRAGMENTS"));
            }

            if (!(iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT) && !(iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT))
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - New msg has NO MARKER BIT"));
            }
        }


        // get a memfrag from the message
        OsclRefCounterMemFrag frag;
        iDataIn->getMediaFragment(iCurrFragNum, frag);


        if (iOMXComponentSupportsMovableInputBuffers)
        {
            // no copying required

            // increment the RefCounter of the message associated with the mem fragment/buffer
            // when sending this buffer to OMX component. (When getting the buffer back, the refcounter
            // will be decremented. Thus, when the last fragment is returned, the input mssage is finally released

            iDataIn.GetRefCounter()->addRef();

            // associate the buffer ctrl structure with the message ref counter and ptr
            input_buf->pMediaData = PVMFSharedMediaDataPtr(iDataIn.GetRep(), iDataIn.GetRefCounter());


            // set pointer to the data, length, offset
            input_buf->pBufHdr->pBuffer = (uint8 *)frag.getMemFragPtr();
            input_buf->pBufHdr->nFilledLen = frag.getMemFragSize();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - Buffer 0x%x of size %d, %d frag out of tot. %d", input_buf->pBufHdr->pBuffer, frag.getMemFragSize(), iCurrFragNum + 1, iDataIn->getNumFragments()));

            iCurrFragNum++; // increment fragment number and move on to the next
            iIsNewDataFragment = true; // update the flag

        }
        else
        {

            // in this case, no need to use input msg refcounter, each buffer fragment is copied over and treated separately
            (input_buf->pMediaData).Unbind();

            // is this a new data fragment or are we still working on separating the old one?
            if (iIsNewDataFragment == true)
            {
                //  if fragment size is larger than the buffer size,
                //	need to break up the fragment even further into smaller chunks

                // init variables needed for fragment separation
                iCopyPosition = 0;
                iFragmentSizeRemainingToCopy  = frag.getMemFragSize();

            }

            // can the remaining fragment fit into the buffer?
            if (iFragmentSizeRemainingToCopy <= (input_buf->pBufHdr->nAllocLen))
            {

                oscl_memcpy(input_buf->pBufHdr->pBuffer,
                            (void *)((uint8 *)frag.getMemFragPtr() + iCopyPosition),
                            iFragmentSizeRemainingToCopy);

                input_buf->pBufHdr->nFilledLen = iFragmentSizeRemainingToCopy;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - Copied %d bytes of fragment %d out of %d into buffer 0x%x of size %d", iFragmentSizeRemainingToCopy, iCurrFragNum + 1, iDataIn->getNumFragments(), input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFilledLen));

                iCopyPosition += iFragmentSizeRemainingToCopy;
                iFragmentSizeRemainingToCopy = 0;



                iIsNewDataFragment = true; // done with this fragment. Get a new one
                iCurrFragNum++;

            }
            else
            {
                // copy as much as you can of the current fragment into the current buffer
                oscl_memcpy(input_buf->pBufHdr->pBuffer,
                            (void *)((uint8 *)frag.getMemFragPtr() + iCopyPosition),
                            input_buf->pBufHdr->nAllocLen);

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - Copied %d bytes of fragment %d out of %d into buffer 0x%x of size %d", input_buf->pBufHdr->nAllocLen, iCurrFragNum + 1, iDataIn->getNumFragments(), input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFilledLen));

                input_buf->pBufHdr->nFilledLen = input_buf->pBufHdr->nAllocLen;
                iCopyPosition += input_buf->pBufHdr->nAllocLen; // move current position within fragment forward
                iFragmentSizeRemainingToCopy -= input_buf->pBufHdr->nAllocLen;
                iIsNewDataFragment = false; // set the flag to indicate we're still working on the "old" fragment
            }

        }


        // set buffer fields (this is the same regardless of whether the input is movable or not
        input_buf->pBufHdr->nOffset = 0;
        input_buf->pBufHdr->nTimeStamp = iInTimestamp;

        // set ptr to input_buf structure for Context (for when the buffer is returned)
        input_buf->pBufHdr->pAppPrivate = (OMX_PTR) input_buf;

        // do not use Mark here (but init to NULL to prevent problems)
        input_buf->pBufHdr->hMarkTargetComponent = NULL;
        input_buf->pBufHdr->pMarkData = NULL;


        // init buffer flags
        input_buf->pBufHdr->nFlags = 0;

        // set marker bit on or off
        // a) AAC - file playback - each fragment is a complete frame (1 msg may contain multiple fragments/frames)
        //    AAC - streaming	- 1 msg may contain a partial frame, but LATM parser will assemble a full frame
        //						(when LATM parser is done, we attach a marker bit to the data it produces)

        // b) AMR - file playback - each msg is N whole frames (marker bit is always set)
        //    AMR - streaming   - each msg is N whole frames (marker bit is missing from incoming msgs -set it here)

        // c) MP3 - file playback - 1 msg is N whole frames


        if (iSetMarkerBitForEveryFrag == true)
        {


            if (iIsNewDataFragment)
            {
                if ((iDataIn->getNumFragments() > 1))
                {
                    // if more than 1 fragment in the message and we have not broken it up
                    //(i.e. this is the last piece of a broken up piece), put marker bit on it unconditionally
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - END OF FRAGMENT - Multifragmented msg AVC case, Buffer 0x%x MARKER bit set to 1", input_buf->pBufHdr->pBuffer));
                    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                }
                else if ((iDataIn->getNumFragments() == 1))
                {
                    // this is (the last piece of broken up by us) single-fragmented message. This can be a piece of a NAL (streaming) or a full NAL (file )
                    // apply marker bit if the message carries one
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - END OF FRAGMENT - Buffer 0x%x MARKER bit set to %d", input_buf->pBufHdr->pBuffer, iCurrentMsgMarkerBit));

                    if (iCurrentMsgMarkerBit)
                        input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

                }
            }
            else
            {
                // we are separating fragments that are too big, i.e. bigger than
                // what 1 buffer can hold, this fragment Can NEVER have marker bit
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - NOT END OF FRAGMENT - Buffer 0x%x MARKER bit set to 0", input_buf->pBufHdr->pBuffer));

            }
        }
        else
        {
            // "normal" case, i.e. only fragments at ends of msgs may have marker bit set
            //					fragments in the middle of a message never have marker bit set
            // there is also a (slight) possibility we broke up the fragment into more fragments
            //	because they can't fit into input buffer. In this case, make sure you apply
            //	the marker bit (if necessary) only to the very last piece of the very last fragment

            // for all other cases, clear the marker bit flag for the buffer
            if ((iCurrFragNum == iDataIn->getNumFragments()) && iIsNewDataFragment)
            {
                // if all the fragments have been exhausted, and this is the last piece
                // of the (possibly broken up) last fragment

                // use the marker bit from the end of message
                if (iCurrentMsgMarkerBit)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - END OF MESSAGE - Buffer 0x%x MARKER bit set to 1", input_buf->pBufHdr->pBuffer));

                    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

                }
                else
                {

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - END OF MESSAGE - Buffer 0x%x MARKER bit set to 0", input_buf->pBufHdr->pBuffer));
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - NOT END OF MESSAGE - Buffer 0x%x MARKER bit set to 0", input_buf->pBufHdr->pBuffer));
            }


        }// end of else(setmarkerbitforeveryfrag)


        // set the key frame flag if necessary (mark every fragment that belongs to it)
        if (iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_RANDOM_ACCESS_POINT_BIT)
            input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;


        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() - SENDING Buffer 0x%x MARKER bit set to %d,TS=%d", input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFlags, input_buf->pBufHdr->nTimeStamp));


        OMX_EmptyThisBuffer(iOMXAudioDecoder, input_buf->pBufHdr);


        // if we sent all fragments to OMX component, decouple the input message from iDataIn
        // Input message is "decoupled", so that we can get a new message for processing into iDataIn
        //	However, the actual message is released completely to upstream mempool once all of its fragments
        //	are returned by the OMX component

        if (iCurrFragNum == iDataIn->getNumFragments())
        {
            iDataIn.Unbind();

        }


    }
    while (iCurrFragNum < iInNumFrags); //iDataIn->getNumFragments());



    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendInputBufferToOMXComponent() Out"));

    return true;

}
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::InitDecoder(PVMFSharedMediaDataPtr& DataIn)
{
    uint16 length = 0, size = 0;


    OsclRefCounterMemFrag DataFrag;
    OsclRefCounterMemFrag refCtrMemFragOut;
    uint8* initbuffer = NULL;
    uint32 initbufsize = 0;


    // NOTE: the component may not start decoding without providing the Output buffer to it,
    //		here, we're sending input/config buffers.
    //		Then, we'll go to ReadyToDecode state and send output as well

    switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
    {
        case PVMF_LATM:
        {

            // must have the LATM config buffer and size already present
            if (iLATMConfigBuffer != NULL)
            {
                initbuffer = iLATMConfigBuffer;
                initbufsize = iLATMConfigBufferSize;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::InitDecoder() Error - LATM config buffer not present"));
                return false;
            }


        }
        break;

        case PVMF_MPEG4_AUDIO:
        case PVMF_ADIF:
        case PVMF_ASF_MPEG4_AUDIO:
        case PVMF_AAC_SIZEHDR: // for testing
        {
            // get format specific info and send it as config data:
            DataIn->getFormatSpecificInfo(DataFrag);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::InitDecoder() VOL header (Size=%d)", DataFrag.getMemFragSize()));

            //get pointer to the data fragment
            initbuffer = (uint8 *) DataFrag.getMemFragPtr();
            initbufsize = (int32) DataFrag.getMemFragSize();

        }			// in some cases, initbufsize may be 0, and initbuf= NULL. Config is done after 1st frame of data
        break;

        case PVMF_AMR_IF2:
        case PVMF_AMR_IETF:
        case PVMF_AMR_IETF_COMBINED:
        case PVMF_AMRWB_IETF:
        case PVMF_AMRWB_IETF_PAYLOAD:
        case PVMF_MP3:
        {
            initbuffer = NULL; // no special config header. Need to decode 1 frame
            initbufsize = 0;
        }
        break;

        case PVMF_WMA:

            // in case of WMA, get config parameters from the port
            initbuffer = ((PVMFOMXAudioDecPort*)iInPort)->getTrackConfig();
            initbufsize = (int32)((PVMFOMXAudioDecPort*)iInPort)->getTrackConfigSize();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::InitDecoder() for WMA Decoder. Initialization data Size %d.", initbufsize));


            break;


        default:

            break;
    }


    if (initbufsize > 0)
    {


        if (!SendConfigBufferToOMXComponent(initbuffer, initbufsize))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::InitDecoder() Error in processing config buffer"));
            return false;
        }
    }


    //Varibles initialization
    sendFsi = true;


    return true;
}



bool PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent(uint8 *initbuffer, uint32 initbufsize)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() In"));


    // first of all , get an input buffer. Without a buffer, no point in proceeding
    InputBufCtrlStruct_Audio *input_buf = NULL;
    int32 errcode = 0;

    // try to get input buffer header
    OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct_Audio *) iInBufMemoryPool->allocate(iInputAllocSize));
    if (errcode != 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() Input buffer mempool problem -unexpected at init"));

        return false;
    }

    // Got a buffer OK
    // keep track of buffers. When buffer is deallocated/released, the counter will be decremented
    iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool);
    iNumOutstandingInputBuffers++;

    // Now we have the buffer header (i.e. a buffer) to send to component:
    // Depending on OMX component capabilities, either pass the input msg fragment(s) directly
    //	into OMX component without copying (and update the input msg refcount)
    //	or memcopy the content of input msg memfrag(s) into OMX component allocated buffers

    // When copying content, a special case is when the input fragment is larger than the buffer and has to
    //	be fragmented here and broken over 2 or more buffers. Potential problem with available buffers etc.

    iCodecSeqNum += (iDataIn->getSeqNum() - iInPacketSeqNum); // increment the codec seq. # by the same
    // amount that the input seq. number increased

    iInPacketSeqNum = iDataIn->getSeqNum(); // remember input sequence number
    iInTimestamp = iDataIn->getTimestamp();
    iInDuration = iDataIn->getDuration();


    if (!(iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT))
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() - New msg has NO MARKER BIT"));
    }


    if (iOMXComponentSupportsMovableInputBuffers)
    {
        // no copying required

        // increment the RefCounter of the message associated with the mem fragment/buffer
        // when sending this buffer to OMX component. (When getting the buffer back, the refcounter
        // will be decremented. Thus, when the last fragment is returned, the input mssage is finally released

        iDataIn.GetRefCounter()->addRef();

        // associate the buffer ctrl structure with the message ref counter and ptr
        input_buf->pMediaData = PVMFSharedMediaDataPtr(iDataIn.GetRep(), iDataIn.GetRefCounter());


        // set pointer to the data, length, offset
        input_buf->pBufHdr->pBuffer = initbuffer;
        input_buf->pBufHdr->nFilledLen = initbufsize;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() - Config Buffer 0x%x of size %d", initbuffer, initbufsize));

    }
    else
    {

        // in this case, no need to use input msg refcounter, each buffer fragment is copied over and treated separately
        (input_buf->pMediaData).Unbind();

        // we assume the buffer is large enough to fit the config data

        iCopyPosition = 0;
        iFragmentSizeRemainingToCopy  = initbufsize;



        // can the remaining fragment fit into the buffer?
        if (iFragmentSizeRemainingToCopy <= (input_buf->pBufHdr->nAllocLen))
        {

            oscl_memcpy(input_buf->pBufHdr->pBuffer,
                        (void *)(initbuffer + iCopyPosition),
                        iFragmentSizeRemainingToCopy);

            input_buf->pBufHdr->nFilledLen = iFragmentSizeRemainingToCopy;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() - Copied %d bytes into buffer 0x%x of size %d", iFragmentSizeRemainingToCopy, input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFilledLen));

            iCopyPosition += iFragmentSizeRemainingToCopy;
            iFragmentSizeRemainingToCopy = 0;


        }
        else
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() Config buffer too large problem -unexpected at init"));

            return false;
        }

    }


    // set buffer fields (this is the same regardless of whether the input is movable or not
    input_buf->pBufHdr->nOffset = 0;
    input_buf->pBufHdr->nTimeStamp = iInTimestamp;

    // set ptr to input_buf structure for Context (for when the buffer is returned)
    input_buf->pBufHdr->pAppPrivate = (OMX_PTR) input_buf;

    // do not use Mark here (but init to NULL to prevent problems)
    input_buf->pBufHdr->hMarkTargetComponent = NULL;
    input_buf->pBufHdr->pMarkData = NULL;


    // init buffer flags
    input_buf->pBufHdr->nFlags = 0;

    // set marker bit on

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendConfigBufferToOMXComponent() - END OF FRAGMENT - Buffer 0x%x MARKER bit set to 1", input_buf->pBufHdr->pBuffer));

    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

    OMX_EmptyThisBuffer(iOMXAudioDecoder, input_buf->pBufHdr);

    return true;

}



/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::CreateOutMemPool(uint32 num_buffers)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::CreateOutMemPool() start"));
    // In the case OMX component wants to allocate its own buffers,
    // mempool only contains OutputBufCtrlStructures (i.e. ptrs to buffer headers)
    // In case OMX component uses pre-allocated buffers (here),
    // mempool allocates OutputBufCtrlStructure (i.e. ptrs to buffer hdrs), followed by actual buffers

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::CreateOutMemPool() Allocating output buffer header pointers"));

    iOutputAllocSize = oscl_mem_aligned_size((uint32)sizeof(OutputBufCtrlStruct_Audio));

    if (iOMXComponentSupportsExternalOutputBufferAlloc)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::CreateOutMemPool() Allocating output buffers of size %d as well", iOMXComponentOutputBufferSize));

        //pre-negotiated output buffer size
        iOutputAllocSize += iOMXComponentOutputBufferSize;
    }

    // for media data wrapper
    if (iMediaDataMemPool)
    {
        iMediaDataMemPool->removeRef();
        iMediaDataMemPool = NULL;
    }

    if (iOutBufMemoryPool)
    {
        iOutBufMemoryPool->removeRef();
        iOutBufMemoryPool = NULL;
    }

    int32 leavecode = 0;
    OSCL_TRY(leavecode, iOutBufMemoryPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (num_buffers)););
    if (leavecode || iOutBufMemoryPool == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                        PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::CreateOutMemPool() Memory pool structure for output buffers failed to allocate"));
        return false;
    }



    // allocate a dummy buffer to actually create the mempool
    OsclAny *dummy_alloc = NULL; // this dummy buffer will be released at end of scope
    leavecode = 0;
    OSCL_TRY(leavecode, dummy_alloc = iOutBufMemoryPool->allocate(iOutputAllocSize));
    if (leavecode || dummy_alloc == NULL)
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                        PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::CreateOutMemPool() Memory pool for output buffers failed to allocate"));
        return false;
    }
    iOutBufMemoryPool->deallocate(dummy_alloc);
    // init the counter
    iNumOutstandingOutputBuffers = 0;

    // allocate mempool for media data message wrapper
    leavecode = 0;
    OSCL_TRY(leavecode, iMediaDataMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (num_buffers, PVOMXAUDIODEC_MEDIADATA_CHUNKSIZE)));
    if (leavecode || iMediaDataMemPool == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::CreateOutMemPool() Media Data Buffer pool for output buffers failed to allocate"));
        return false;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::CreateOutMemPool() done"));
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Creates memory pool for input buffer management ///////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::CreateInputMemPool(uint32 num_buffers)
{
    // 3 cases in order of preference and simplicity

    // Case 1 (buffers allocated upstream - no memcpy needed):
    //	PV OMX Component - We use buffers allocated outside the OMX node (i.e. allocated upstream)
    // Mempool contains InputBufCtrlStructures (ptrs to buffer headers and PMVFMediaData ptrs - to keep track of when to unbind input msgs)

    // NOTE:	in this case, when providing input buffers to OMX component,
    //			OMX_UseBuffer calls will provide some initial pointers and sizes of buffers, but these
    //			are dummy values. Actual buffer pointers and filled sizes will be obtained from the input msg fragments.
    //			The PV OMX component will use the buffers even if the ptrs differ from the ones during initialization
    //			3rd party OMX components can also use this case if they are capable of ignoring the actual buffer pointers in
    //			buffer header field (i.e. if after OMX_UseBuffer(...) call, they allow the ptr to actual buffer data to change at a later time

    // CASE 2 (buffers allocated in the node - memcpy needed)
    //			If 3rd party OMX component can use buffers allocated outside the OMX component, but it cannot
    //			change buffer ptr allocations dynamically (i.e. after initialization with OMX_UseBuffer call is complete)

    //		Mempool contains InputBufCtrlStructures (ptrs to buffer headers, PVMFMediaData ptrs to keep track of when to unbind input msgs) +
    //				actual buffers.
    //			NOTE: Data must be copied from input message into the local buffer before the buffer is given to the OMX component

    // CASE 3 (buffers allocated in the component - memcpy needed)
    //			If 3rd party OMX component must allocate its own buffers
    //			Mempool only contains InputBufCtrlStruct (ptrs to buffer headers + PMVFMediaData ptrs to keep track of when to unbind input msgs)
    //			NOTE: Data must be copied from input message into the local buffer before the buffer is given to the OMX component (like in case 2)

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::CreateInputMemPool() start "));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::CreateInputMemPool() allocating buffer header pointers and shared media data ptrs "));



    iInputAllocSize = oscl_mem_aligned_size((uint32) sizeof(InputBufCtrlStruct_Audio)); //aligned_size_buffer_header_ptr+aligned_size_media_data_ptr;

    // Need to allocate buffers in the node either if component supports external buffers buffers
    // but they are not movable

    if ((iOMXComponentSupportsExternalInputBufferAlloc && !iOMXComponentSupportsMovableInputBuffers))
    {
        //pre-negotiated input buffer size
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::CreateOutMemPool() Allocating input buffers of size %d as well", iOMXComponentInputBufferSize));

        iInputAllocSize += iOMXComponentInputBufferSize;
    }

    if (iInBufMemoryPool)
    {
        iInBufMemoryPool->removeRef();
        iInBufMemoryPool = NULL;
    }

    int32 leavecode = 0;
    OSCL_TRY(leavecode, iInBufMemoryPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (num_buffers)););
    if (leavecode || iInBufMemoryPool == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                        PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::CreateInputMemPool() Memory pool structure for input buffers failed to allocate"));
        return false;
    }
    // try to allocate a dummy buffer to actually create the mempool and allocate the needed memory
    // allocate a dummy buffer to actually create the mempool, this dummy buffer will be released at end of scope of this method
    OsclAny *dummy_alloc = NULL;
    leavecode = 0;
    OSCL_TRY(leavecode, dummy_alloc = iInBufMemoryPool->allocate(iInputAllocSize));
    if (leavecode || dummy_alloc == NULL)
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                        PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::CreateInputMemPool() Memory pool for input buffers failed to allocate"));
        return false;
    }

    // init the counter
    iNumOutstandingInputBuffers = 0;


    iInputBufferToResendToComponent = NULL; // nothing to resend yet
    iInBufMemoryPool->deallocate(dummy_alloc);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::CreateInputMemPool() done"));
    return true;
}
////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::ProvideBuffersToComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
        uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
        uint32 aNumBuffers,    // number of buffers
        uint32 aActualBufferSize, // aactual buffer size
        uint32 aPortIndex,      // port idx
        bool aUseBufferOK,		// can component use OMX_UseBuffer or should it use OMX_AllocateBuffer
        bool	aIsThisInputBuffer		// is this input or output
                                                   )
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ProvideBuffersToComponent() enter"));

    uint32 ii = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OsclAny **ctrl_struct_ptr = NULL;	// temporary array to keep the addresses of buffer ctrl structures and buffers

    ctrl_struct_ptr = (OsclAny **) oscl_malloc(aNumBuffers * sizeof(OsclAny *));
    if (ctrl_struct_ptr == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::ProvideBuffersToComponent ctrl_struct_ptr == NULL"));
        return false;
    }


    // Now, go through all buffers and tell component to
    // either use a buffer, or to allocate its own buffer
    for (ii = 0; ii < aNumBuffers; ii++)
    {

        int32 errcode = 0;
        // get the address where the buf hdr ptr will be stored
        OSCL_TRY(errcode, ctrl_struct_ptr[ii] = (OsclAny *) aMemPool->allocate(aAllocSize));
        if ((errcode != OsclErrNone) || (ctrl_struct_ptr[ii] == NULL))
        {
            if (errcode == OsclErrNoResources)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::ProvideBuffersToComponent ->allocate() failed for no mempool chunk available"));
            }
            else
            {
                // General error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::ProvideBuffersToComponent ->allocate() failed due to some general error"));

                ReportErrorEvent(PVMFFailure);
                ChangeNodeState(EPVMFNodeError);
            }

            return false;
        }

        if (aUseBufferOK)
        {
            // Buffers are already allocated outside OMX component.
            // In case of output buffers, the buffer itself is located
            // just after the buffer header pointer.

            // In case of input buffers, the buffer header pointer is followed by a MediaDataSharedPtr
            //	which is used to ensure proper unbinding of the input messages. The buffer itself is either:
            //		a) allocated upstream (and the ptr to the buffer
            //			is a dummy pointer to which the component does not pay attention - PV OMX component)
            //		b) located just after the buffer header pointer and MediaDataSharedPtr

            uint8 *pB = ((uint8*) ctrl_struct_ptr[ii]);


            // in case of input buffers, initialize also MediaDataSharedPtr structure
            if (aIsThisInputBuffer)
            {

                InputBufCtrlStruct_Audio *temp = (InputBufCtrlStruct_Audio *) ctrl_struct_ptr[ii];
                oscl_memset(&(temp->pMediaData), 0, sizeof(PVMFSharedMediaDataPtr));
                temp->pMediaData = PVMFSharedMediaDataPtr(NULL, NULL);

                // advance ptr to skip the structure
                pB += oscl_mem_aligned_size(sizeof(InputBufCtrlStruct_Audio));

                err = OMX_UseBuffer(iOMXAudioDecoder,	// hComponent
                                    &(temp->pBufHdr),		// address where ptr to buffer header will be stored
                                    aPortIndex,				// port index (for port for which buffer is provided)
                                    ctrl_struct_ptr[ii],	// App. private data = pointer to beginning of allocated data
                                    //				to have a context when component returns with a callback (i.e. to know
                                    //				what to free etc.
                                    (OMX_U32)aActualBufferSize,		// buffer size
                                    pB);						// buffer data ptr

            }
            else
            {
                OutputBufCtrlStruct_Audio *temp = (OutputBufCtrlStruct_Audio *) ctrl_struct_ptr[ii];
                // advance buffer ptr to skip the structure
                pB += oscl_mem_aligned_size(sizeof(OutputBufCtrlStruct_Audio));


                err = OMX_UseBuffer(iOMXAudioDecoder,	// hComponent
                                    &(temp->pBufHdr),		// address where ptr to buffer header will be stored
                                    aPortIndex,				// port index (for port for which buffer is provided)
                                    ctrl_struct_ptr[ii],	// App. private data = pointer to beginning of allocated data
                                    //				to have a context when component returns with a callback (i.e. to know
                                    //				what to free etc.
                                    (OMX_U32)aActualBufferSize,		// buffer size
                                    pB);						// buffer data ptr


            }


        }
        else
        {
            // the component must allocate its own buffers.
            if (aIsThisInputBuffer)
            {

                InputBufCtrlStruct_Audio *temp = (InputBufCtrlStruct_Audio *) ctrl_struct_ptr[ii];
                err = OMX_AllocateBuffer(iOMXAudioDecoder,
                                         &(temp->pBufHdr),
                                         aPortIndex,
                                         ctrl_struct_ptr[ii],
                                         (OMX_U32)aActualBufferSize);
            }
            else
            {
                OutputBufCtrlStruct_Audio *temp = (OutputBufCtrlStruct_Audio *) ctrl_struct_ptr[ii];
                err = OMX_AllocateBuffer(iOMXAudioDecoder,
                                         &(temp->pBufHdr),
                                         aPortIndex,
                                         ctrl_struct_ptr[ii],
                                         (OMX_U32)aActualBufferSize);
            }

        }

        if (err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::ProvideBuffersToComponent() Problem using/allocating a buffer"));


            return false;
        }

    }

    for (ii = 0; ii < aNumBuffers; ii++)
    {
        // after initializing the buffer hdr ptrs, return them
        // to the mempool
        aMemPool->deallocate((OsclAny*) ctrl_struct_ptr[ii]);
    }

    oscl_free(ctrl_struct_ptr);
    // set the flags
    if (aIsThisInputBuffer)
    {
        iInputBuffersFreed = false;
    }
    else
    {
        iOutputBuffersFreed = false;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ProvideBuffersToComponent() done"));
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::FreeBuffersFromComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
        uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
        uint32 aNumBuffers,    // number of buffers
        uint32 aPortIndex,      // port idx
        bool	aIsThisInputBuffer		// is this input or output
                                                  )
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::FreeBuffersToComponent() enter"));

    uint32 ii = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OsclAny **ctrl_struct_ptr = NULL;	// temporary array to keep the addresses of buffer ctrl structures and buffers

    ctrl_struct_ptr = (OsclAny **) oscl_malloc(aNumBuffers * sizeof(OsclAny *));
    if (ctrl_struct_ptr == NULL)
    {
        return false;
    }


    // Now, go through all buffers and tell component to free them
    for (ii = 0; ii < aNumBuffers; ii++)
    {

        int32 errcode = 0;
        // get the address where the buf hdr ptr will be stored

        OSCL_TRY(errcode, ctrl_struct_ptr[ii] = (OsclAny *) aMemPool->allocate(aAllocSize));
        if ((errcode != OsclErrNone) || (ctrl_struct_ptr[ii] == NULL))
        {
            if (errcode == OsclErrNoResources)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::FreeBuffersFromComponent ->allocate() failed for no mempool chunk available"));
            }
            else
            {
                // General error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::FreeBuffersFromComponent ->allocate() failed due to some general error"));

                ReportErrorEvent(PVMFFailure);
                ChangeNodeState(EPVMFNodeError);
            }

            return false;
        }
        // to maintain correct count
        aMemPool->notifyfreechunkavailable((*this), (OsclAny*) aMemPool);

        if (aIsThisInputBuffer)
        {

            iNumOutstandingInputBuffers++;
            // get the buf hdr pointer
            InputBufCtrlStruct_Audio *temp = (InputBufCtrlStruct_Audio *) ctrl_struct_ptr[ii];
            err = OMX_FreeBuffer(iOMXAudioDecoder,
                                 aPortIndex,
                                 temp->pBufHdr);

        }
        else
        {
            iNumOutstandingOutputBuffers++;
            OutputBufCtrlStruct_Audio *temp = (OutputBufCtrlStruct_Audio *) ctrl_struct_ptr[ii];
            err = OMX_FreeBuffer(iOMXAudioDecoder,
                                 aPortIndex,
                                 temp->pBufHdr);

        }

        if (err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::FreeBuffersFromComponent() Problem freeing a buffer"));

            return false;
        }

    }

    for (ii = 0; ii < aNumBuffers; ii++)
    {
        // after freeing the buffer hdr ptrs, return them
        // to the mempool (which will itself then be deleted promptly)
        aMemPool->deallocate((OsclAny*) ctrl_struct_ptr[ii]);
    }

    oscl_free(ctrl_struct_ptr);

    // mark buffers as freed (so as not to do it twice)
    if (aIsThisInputBuffer)
    {
        iInputBuffersFreed = true;
    }
    else
    {
        iOutputBuffersFreed = true;
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::FreeBuffersFromComponent() done"));
    return true;
}



/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR EVENT HANDLER
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXAudioDecNode::EventHandlerProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_EVENTTYPE aEvent,
        OMX_OUT OMX_U32 aData1,
        OMX_OUT OMX_U32 aData2,
        OMX_OUT OMX_PTR aEventData)
{

    OSCL_UNUSED_ARG(aComponent);
    OSCL_UNUSED_ARG(aAppData);
    OSCL_UNUSED_ARG(aEventData);


    switch (aEvent)
    {
        case OMX_EventCmdComplete:
        {

            switch (aData1)
            {
                case OMX_CommandStateSet:
                {
                    HandleComponentStateChange(aData2);
                    break;
                }
                case OMX_CommandFlush:
                {
                    // flush can be sent as part of repositioning or as part of port reconfig
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_CommandFlush - completed on port %d", aData2));

                    if (iIsRepositioningRequestSentToComponent)
                    {
                        if (aData2 == iOutputPortIndex)
                        {
                            iIsOutputPortFlushed = true;
                        }
                        else if (aData2 == iInputPortIndex)
                        {
                            iIsInputPortFlushed = true;
                        }

                        if (iIsOutputPortFlushed && iIsInputPortFlushed)
                        {
                            iIsRepositionDoneReceivedFromComponent = true;
                        }
                    }
                    RunIfNotReady();

                }

                break;

                case OMX_CommandPortDisable:
                {
                    // if port disable command is done, we can re-allocate the buffers and re-enable the port

                    iProcessingState = EPVMFOMXAudioDecNodeProcessingState_PortReEnable;
                    iPortIndexForDynamicReconfig =  aData2;

                    RunIfNotReady();
                    break;
                }
                case OMX_CommandPortEnable:
                    // port enable command is done. Check if the other port also reported change.
                    // If not, we can start data flow. Otherwise, must start dynamic reconfig procedure for
                    // the other port as well.
                {
                    if (iSecondPortReportedChange)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_CommandPortEnable - completed on port %d, dynamic reconfiguration needed on port %d", aData2, iSecondPortToReconfig));

                        iProcessingState = EPVMFOMXAudioDecNodeProcessingState_PortReconfig;
                        iPortIndexForDynamicReconfig = iSecondPortToReconfig;
                        iSecondPortReportedChange = false;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_CommandPortEnable - completed on port %d, resuming normal data flow", aData2));
                        iProcessingState = EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode;
                        iDynamicReconfigInProgress = false;
                        // in case pause or stop command was sent to component
                        // change processing state (because the node might otherwise
                        // start sending buffers to component before pause/stop is processed)
                        if (iPauseCommandWasSentToComponent)
                        {
                            iProcessingState = EPVMFOMXAudioDecNodeProcessingState_Pausing;
                        }
                        if (iStopCommandWasSentToComponent)
                        {
                            iProcessingState = EPVMFOMXAudioDecNodeProcessingState_Stopping;
                        }

                    }
                    RunIfNotReady();
                    break;
                }

                case OMX_CommandMarkBuffer:
                    // nothing to do here yet;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_CommandMarkBuffer - completed - no action taken"));

                    break;

                default:
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: Unsupported event"));
                    break;
                }
            }//end of switch (aData1)

            break;
        }//end of case OMX_EventCmdComplete

        case OMX_EventError:
        {

            if (aData1 == OMX_ErrorStreamCorrupt)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventError - Bitstream corrupt error"));
                // Errors from corrupt bitstream are reported as info events
                ReportInfoEvent(PVMFInfoProcessingFailure, NULL);

            }
            else
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventError"));
                // for now, any error from the component will be reported as error
                ReportErrorEvent(PVMFErrorEvent, NULL, NULL);
                SetState(EPVMFNodeError);
            }
            break;



        }

        case OMX_EventBufferFlag:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventBufferFlag (EOS) flag returned from OMX component"));

            RunIfNotReady();
            break;
        }//end of case OMX_EventBufferFlag

        case OMX_EventMark:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventMark returned from OMX component - no action taken"));

            RunIfNotReady();
            break;
        }//end of case OMX_EventMark

        case OMX_EventPortSettingsChanged:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned from OMX component"));

            // first check if dynamic reconfiguration is already in progress,
            // if so, wait until this is completed, and then initiate the 2nd reconfiguration
            if (iDynamicReconfigInProgress)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned for port %d, dynamic reconfig already in progress", aData1));

                iSecondPortToReconfig = aData1;
                iSecondPortReportedChange = true;

                // check the audio sampling rate and fs right away in case of output port
                // is this output port?
                if (iSecondPortToReconfig == iOutputPortIndex)
                {

                    OMX_ERRORTYPE Err;
                    // GET the output buffer params and sizes
                    OMX_AUDIO_PARAM_PCMMODETYPE Audio_Pcm_Param;
                    Audio_Pcm_Param.nPortIndex = iOutputPortIndex; // we're looking for output port params
                    Audio_Pcm_Param.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
                    Audio_Pcm_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
                    Audio_Pcm_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
                    Audio_Pcm_Param.nVersion.s.nRevision = SPECREVISION;
                    Audio_Pcm_Param.nVersion.s.nStep = SPECSTEP;


                    Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamAudioPcm, &Audio_Pcm_Param);
                    if (Err != OMX_ErrorNone)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXAudioDecNode::EventHandlerProcessing() PortSettingsChanged -> Cannot get component output parameters"));

                        SetState(EPVMFNodeError);
                        ReportErrorEvent(PVMFErrResource);
                    }

                    iPCMSamplingRate = Audio_Pcm_Param.nSamplingRate; // can be set to 0 (if unknown)

                    if (iPCMSamplingRate == 0) // use default sampling rate (i.e. 48000)
                        iPCMSamplingRate = PVOMXAUDIODEC_DEFAULT_SAMPLINGRATE;

                    iNumberOfAudioChannels = Audio_Pcm_Param.nChannels;		// should be 1 or 2
                    if (iNumberOfAudioChannels != 1 && iNumberOfAudioChannels != 2)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXAudioDecNode::EventHandlerProcessing() PortSettingsChanged -> Output parameters num channels = %d", iNumberOfAudioChannels));

                        SetState(EPVMFNodeError);
                        ReportErrorEvent(PVMFErrResource);
                    }
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned for port %d", aData1));

                iProcessingState = EPVMFOMXAudioDecNodeProcessingState_PortReconfig;
                iPortIndexForDynamicReconfig = aData1;
                // start "discarding" data right away, don't wait
                // check the audio sampling rate and fs right away in case of output port
                // is this output port?
                if (iPortIndexForDynamicReconfig == iOutputPortIndex)
                {

                    OMX_ERRORTYPE Err;
                    // GET the output buffer params and sizes
                    OMX_AUDIO_PARAM_PCMMODETYPE Audio_Pcm_Param;
                    Audio_Pcm_Param.nPortIndex = iOutputPortIndex; // we're looking for output port params
                    Audio_Pcm_Param.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
                    Audio_Pcm_Param.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
                    Audio_Pcm_Param.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
                    Audio_Pcm_Param.nVersion.s.nRevision = SPECREVISION;
                    Audio_Pcm_Param.nVersion.s.nStep = SPECSTEP;


                    Err = OMX_GetParameter(iOMXAudioDecoder, OMX_IndexParamAudioPcm, &Audio_Pcm_Param);
                    if (Err != OMX_ErrorNone)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXAudioDecNode::EventHandlerProcessing() PortSettingsChanged -> Cannot get component output parameters"));

                        SetState(EPVMFNodeError);
                        ReportErrorEvent(PVMFErrResource);

                    }

                    iPCMSamplingRate = Audio_Pcm_Param.nSamplingRate; // can be set to 0 (if unknown)

                    if (iPCMSamplingRate == 0) // use default sampling rate (i.e. 48000)
                        iPCMSamplingRate = PVOMXAUDIODEC_DEFAULT_SAMPLINGRATE;

                    iNumberOfAudioChannels = Audio_Pcm_Param.nChannels;		// should be 1 or 2
                    if (iNumberOfAudioChannels != 1 && iNumberOfAudioChannels != 2)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXAudioDecNode::EventHandlerProcessing() PortSettingsChanged -> Output parameters num channels = %d", iNumberOfAudioChannels));

                        SetState(EPVMFNodeError);
                        ReportErrorEvent(PVMFErrResource);

                    }

                }
                iDynamicReconfigInProgress = true;
            }

            RunIfNotReady();
            break;
        }//end of case OMX_PortSettingsChanged

        case OMX_EventResourcesAcquired:        //not supported
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::EventHandlerProcessing: OMX_EventResourcesAcquired returned from OMX component - no action taken"));

            RunIfNotReady();

            break;
        }

        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::EventHandlerProcessing:  Unknown Event returned from OMX component - no action taken"));

            break;
        }

    }//end of switch (eEvent)



    return OMX_ErrorNone;
}




/////////////////////////////////////////////////////////////////////////////
// This function handles the event of OMX component state change
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::HandleComponentStateChange(OMX_U32 decoder_state)
{
    switch (decoder_state)
    {
        case OMX_StateIdle:
        {
            iCurrentDecoderState = OMX_StateIdle;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleComponentStateChange: OMX_StateIdle reached"));

            //  this state can be reached either going from OMX_Loaded->OMX_Idle (preparing)
            //	or going from OMX_Executing->OMX_Idle (stopping)

            // Also, in case of Audio, repositioning is done by performing Executing->Idle ->Executing
            if (iIsRepositionIdleRequestSentToComponent)
            {
                iIsRepositionIdleDoneReceivedFromComponent = true;
                RunIfNotReady();
            }
            else if ((iCurrentCommand.size() > 0) &&
                     (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_PREPARE))
            {
                iProcessingState = EPVMFOMXAudioDecNodeProcessingState_InitDecoder;
                SetState(EPVMFNodePrepared);
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
                RunIfNotReady();
            }
            else if ((iCurrentCommand.size() > 0) &&
                     (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_STOP))
            {
                // if we are stopped, we won't start until the node gets DoStart command.
                //	in this case, we are ready to start sending buffers
                if (iProcessingState == EPVMFOMXAudioDecNodeProcessingState_Stopping)
                    iProcessingState = EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode;
                // if the processing state was not stopping, leave the state as it was (continue port reconfiguration)
                SetState(EPVMFNodePrepared);
                iStopCommandWasSentToComponent = false;
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);

                RunIfNotReady();
            }
            else if ((iCurrentCommand.size() > 0) &&
                     (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET))
            {
                // State change to Idle was initiated due to Reset. First need to reach idle, and then loaded
                // Once Idle is reached, we need to initiate idle->loaded transition
                RunIfNotReady();
            }
            break;
        }//end of case OMX_StateIdle

        case OMX_StateExecuting:
        {
            iCurrentDecoderState = OMX_StateExecuting;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleComponentStateChange: OMX_StateExecuting reached"));

            // this state can be reached going from OMX_Idle -> OMX_Executing (preparing)
            //	or going from OMX_Pause -> OMX_Executing (coming from pause)
            //	This is a response to "DoStart" command

            // Also, during repositioning, idle->executing transition can be requested
            if (iIsRepositionExecRequestSentToComponent)
            {
                iIsRepositionExecDoneReceivedFromComponent = true;
                RunIfNotReady();
            }
            else if ((iCurrentCommand.size() > 0) &&
                     (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_START))
            {
                SetState(EPVMFNodeStarted);
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);

                RunIfNotReady();
            }

            break;
        }//end of case OMX_StateExecuting

        case OMX_StatePause:
        {
            iCurrentDecoderState = OMX_StatePause;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleComponentStateChange: OMX_StatePause reached"));


            //	This state can be reached going from OMX_Executing-> OMX_Pause
            if ((iCurrentCommand.size() > 0) &&
                    (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_PAUSE))
            {

                // if we are paused, we won't start until the node gets DoStart command.
                //	in this case, we are ready to start sending buffers

                if (iProcessingState == EPVMFOMXAudioDecNodeProcessingState_Pausing)
                    iProcessingState = EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode;



                SetState(EPVMFNodePaused);
                iPauseCommandWasSentToComponent = false;
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
                RunIfNotReady();
            }

            break;
        }//end of case OMX_StatePause

        case OMX_StateLoaded:
        {
            iCurrentDecoderState = OMX_StateLoaded;

            //  this state can be reached only going from OMX_Idle ->OMX_Loaded (stopped to reset)
            //

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleComponentStateChange: OMX_StateLoaded reached"));
            //Check if command's responce is pending
            if ((iCurrentCommand.size() > 0) &&
                    (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET))
            {

                // move this here
                if (iInPort)
                {
                    OSCL_DELETE(((PVMFOMXAudioDecPort*)iInPort));
                    iInPort = NULL;
                }

                if (iOutPort)
                {
                    OSCL_DELETE(((PVMFOMXAudioDecPort*)iOutPort));
                    iOutPort = NULL;
                }

                iDataIn.Unbind();

                // Reset the metadata key list
                iAvailableMetadataKeys.clear();


                iProcessingState = EPVMFOMXAudioDecNodeProcessingState_Idle;
                //logoff & go back to Created state.
                SetState(EPVMFNodeIdle);
                PVMFStatus status = ThreadLogoff();
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), status);
                iResetInProgress = false;
                iResetMsgSent = false;
                //DeleteOMXAudioDecoder();
            }

            break;
        }//end of case OMX_StateLoaded

        case OMX_StateInvalid:
        default:
        {
            iCurrentDecoderState = OMX_StateInvalid;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::HandleComponentStateChange: OMX_StateInvalid reached"));

            break;
        }//end of case OMX_StateInvalid

    }//end of switch(decoder_state)

}






/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR EMPTY BUFFER DONE - input buffer was consumed
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXAudioDecNode::EmptyBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::EmptyBufferDoneProcessing: In"));

    OSCL_ASSERT((void*) aComponent == (void*) iOMXAudioDecoder); // component should match the component
    OSCL_ASSERT(aAppData == (OMX_PTR)(this));		// AppData should represent this node ptr

    // first, get the buffer "context", i.e. pointer to application private data that contains the
    // address of the mempool buffer (so that it can be released)
    InputBufCtrlStruct_Audio *pContext = (InputBufCtrlStruct_Audio *)(aBuffer->pAppPrivate);



    // if a buffer is not empty, we should send the same buffer back to the component
    // do not release it in this case
    if ((aBuffer->nFilledLen > 0) && (iDoNotSaveInputBuffersFlag == false))
        // if dynamic port reconfig is in progress for input port, don't keep the buffer
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::EmptyBufferDoneProcessing: Input buffer returned with %d bytes still in it, TS=%d", aBuffer->nFilledLen, aBuffer->nTimeStamp));

        iInputBufferToResendToComponent = NULL;

    }

    {

        iInputBufferToResendToComponent = NULL;

        // input buffer is to be released,
        // refcount needs to be decremented (possibly - the input msg associated with the buffer will be unbound)
        // NOTE: in case of "moveable" input buffers (passed into component without copying), unbinding decrements a refcount which eventually results
        //			in input message being released back to upstream mempool once all its fragments are returned
        //		in case of input buffers passed into component by copying, unbinding has no effect
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::EmptyBufferDoneProcessing: Release input buffer with TS=%d (with %d refcount remaining of input message)", aBuffer->nTimeStamp, (pContext->pMediaData).get_count() - 1));


        (pContext->pMediaData).Unbind();


        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::EmptyBufferDoneProcessing: Release input buffer %x back to mempool", pContext));

        iInBufMemoryPool->deallocate((OsclAny *) pContext);
    }

    // the OMX spec says that no error is to be returned
    return OMX_ErrorNone;

}



/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR FILL BUFFER DONE - output buffer is ready
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXAudioDecNode::FillBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: In"));

    OSCL_ASSERT((void*) aComponent == (void*) iOMXAudioDecoder); // component should match the component
    OSCL_ASSERT(aAppData == (OMX_PTR)(this));		// AppData should represent this node ptr

    // first, get the buffer "context", i.e. pointer to application private data that contains the
    // address of the mempool buffer (so that it can be released)
    OsclAny *pContext = (OsclAny*) aBuffer->pAppPrivate;


    // check for EOS flag
    if ((aBuffer->nFlags & OMX_BUFFERFLAG_EOS))
    {
        // EOS received
        iIsEOSReceivedFromComponent = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: Output buffer has EOS set"));

    }

    // if a buffer is empty, or if it should not be sent downstream (say, due to state change)
    // release the buffer back to the pool
    if ((aBuffer->nFilledLen == 0) || (iDoNotSendOutputBuffersDownstreamFlag == true))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: Release output buffer %x back to mempool - buffer empty or not to be sent downstream", pContext));

        iOutBufMemoryPool->deallocate(pContext);

    }
    else
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "Output frame %d received", iFrameCounter++));

        // get pointer to actual buffer data
        uint8 *pBufdata = ((uint8*) aBuffer->pBuffer);
        // move the data pointer based on offset info
        pBufdata += aBuffer->nOffset;

        iOutTimeStamp = aBuffer->nTimeStamp;
        //iOutTimeStamp = iInTimestamp;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: Wrapping buffer %x of size %d", pBufdata, aBuffer->nFilledLen));
        // wrap the buffer into the MediaDataImpl wrapper, and queue it for sending downstream
        // wrapping will create a refcounter. When refcounter goes to 0 i.e. when media data
        // is released in downstream components, the custom deallocator will automatically release the buffer back to the
        //	mempool. To do that, the deallocator needs to have info about Context
        // NOTE: we had to wait until now to wrap the buffer data because we only know
        //			now where the actual data is located (based on buffer offset)
        OsclSharedPtr<PVMFMediaDataImpl> MediaDataOut = WrapOutputBuffer(pBufdata, (uint32)(aBuffer->nFilledLen), pContext);

        // if you can't get the MediaDataOut, release the buffer back to the pool
        if (MediaDataOut.GetRep() == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: Problem wrapping buffer %x of size %d - releasing the buffer", pBufdata, aBuffer->nFilledLen));

            iOutBufMemoryPool->deallocate(pContext);
        }
        else
        {

            // if there's a problem queuing output buffer, MediaDataOut will expire at end of scope and
            // release buffer back to the pool, (this should not be the case)
            if (QueueOutputBuffer(MediaDataOut, aBuffer->nFilledLen))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: Buffer %x of size %d queued - reschedule the node to send out", pBufdata, aBuffer->nFilledLen));

                // if queing went OK,
                // re-schedule the node so that outgoing queue can be emptied (unless the outgoing port is busy)
                if ((iOutPort) && !(iOutPort->IsConnectedPortBusy()))
                    RunIfNotReady();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::FillBufferDoneProcessing: Problem queing buffer %x of size %d - releasing the buffer", pBufdata, aBuffer->nFilledLen));
            }


        }

    }
    // the OMX spec says that no error is to be returned
    return OMX_ErrorNone;

}
////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Put output buffer in outgoing queue //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::QueueOutputBuffer(OsclSharedPtr<PVMFMediaDataImpl> &mediadataimplout, uint32 aDataLen)
{

    bool status = true;
    PVMFSharedMediaDataPtr mediaDataOut;
    int32 leavecode = 0;

    // NOTE: ASSUMPTION IS THAT OUTGOING QUEUE IS BIG ENOUGH TO QUEUE ALL THE OUTPUT BUFFERS
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::QueueOutputFrame: In"));

    // First check if we can put outgoing msg. into the queue
    if (iOutPort->IsOutgoingQueueBusy())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFOMXAudioDecNode::QueueOutputFrame() OutgoingQueue is busy"));
        return false;
    }

    OSCL_TRY(leavecode,
             mediaDataOut = PVMFMediaData::createMediaData(mediadataimplout, iMediaDataMemPool););
    if (leavecode == 0)
    {

        // Update the filled length of the fragment
        mediaDataOut->setMediaFragFilledLen(0, aDataLen);

        // Set timestamp
        mediaDataOut->setTimestamp(iOutTimeStamp);

        // Set sequence number
        mediaDataOut->setSeqNum(iSeqNum++);
        // set stream id
        mediaDataOut->setStreamID(iStreamID);


        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iDataPathLogger, PVLOGMSG_INFO, (0, ":PVMFOMXAudioDecNode::QueueOutputFrame(): - SeqNum=%d, TS=%d", iSeqNum, iOutTimeStamp));
        int fsiErrorCode = 0;

        // Check if Fsi configuration need to be sent
        if (sendFsi)
        {

            OsclRefCounterMemFrag FsiMemfrag;

            OSCL_TRY(fsiErrorCode, FsiMemfrag = iFsiFragmentAlloc.get(););

            OSCL_FIRST_CATCH_ANY(fsiErrorCode, PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                 (0, "PVMFOMXAudioDecNode::RemoveOutputFrame() Failed to allocate memory for  FSI")));

            if (fsiErrorCode == 0)
            {

                channelSampleInfo* pcminfo = (channelSampleInfo*) FsiMemfrag.getMemFragPtr();
                OSCL_ASSERT(pcminfo != NULL);

                pcminfo->samplingRate    = iPCMSamplingRate;
                pcminfo->desiredChannels = iNumberOfAudioChannels;

                mediaDataOut->setFormatSpecificInfo(FsiMemfrag);
                ((PVMFOMXAudioDecPort*)iOutPort)->pvmiSetPortFormatSpecificInfoSync(FsiMemfrag);
            }
            else

            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXAudioDecNode::QueueOutputFrame - Problem allocating Output FSI"));
                return false; // this is going to make everything go out of scope
            }

            // Reset the flag
            sendFsi = false;
        }

        if (fsiErrorCode == 0)
        {
            // Send frame to downstream node
            PVMFSharedMediaMsgPtr mediaMsgOut;
            convertToPVMFMediaMsg(mediaMsgOut, mediaDataOut);

            if (iOutPort && (iOutPort->QueueOutgoingMsg(mediaMsgOut) == PVMFSuccess))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::QueueOutputFrame(): Queued frame OK "));

            }
            else
            {
                // we should not get here because we always check for whether queue is busy or not MC
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::QueueOutputFrame(): Send frame failed"));
                return false;
            }

        }


    }//end of if (leavecode==0)
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFOMXAudioDecNode::QueueOutputFrame() call PVMFMediaData::createMediaData is failed"));
        return false;
    }

    return status;

}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Attach a MediaDataImpl wrapper (refcount, deallocator etc.)
/////////////////////////////// to the output buffer /////////////////////////////////////////
OsclSharedPtr<PVMFMediaDataImpl> PVMFOMXAudioDecNode::WrapOutputBuffer(uint8 *pData, uint32 aDataLen, OsclAny *pContext)
{
    // wrap output buffer into a mediadataimpl
    uint32 aligned_class_size = oscl_mem_aligned_size(sizeof(PVMFSimpleMediaBuffer));
    uint32 aligned_cleanup_size = oscl_mem_aligned_size(sizeof(PVOMXBufferSharedPtrWrapperCombinedCleanupDA));
    uint32 aligned_refcnt_size = oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
    uint8 *my_ptr = (uint8*) oscl_malloc(aligned_refcnt_size + aligned_cleanup_size + aligned_class_size);

    if (my_ptr == NULL)
    {
        OsclSharedPtr<PVMFMediaDataImpl> null_buff(NULL, NULL);
        return null_buff;
    }
    // create a deallocator and pass the buffer_allocator to it as well as pointer to data that needs to be returned to the mempool
    PVOMXBufferSharedPtrWrapperCombinedCleanupDA *cleanup_ptr =
        OSCL_PLACEMENT_NEW(my_ptr + aligned_refcnt_size, PVOMXBufferSharedPtrWrapperCombinedCleanupDA(iOutBufMemoryPool, pContext));

    // create the ref counter after the cleanup object (refcount is set to 1 at creation)
    OsclRefCounterDA *my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterDA(my_ptr, cleanup_ptr));

    my_ptr += aligned_refcnt_size + aligned_cleanup_size;

    PVMFMediaDataImpl* media_data_ptr = OSCL_PLACEMENT_NEW(my_ptr, PVMFSimpleMediaBuffer((void *) pData, // ptr to data
                                        aDataLen, // capacity
                                        my_refcnt));   // ref counter

    OsclSharedPtr<PVMFMediaDataImpl> MediaDataImplOut(media_data_ptr, my_refcnt);

    MediaDataImplOut->setMediaFragFilledLen(0, aDataLen);

    return MediaDataImplOut;

}
//////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::SendBeginOfMediaStreamCommand()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendBeginOfMediaStreamCommand() In"));

    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();
    // Set the formatID, timestamp, sequenceNumber and streamID for the media message
    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_BOS_FORMAT_ID);
    sharedMediaCmdPtr->setTimestamp(iBOSTimestamp);
    //reset the sequence number
    sharedMediaCmdPtr->setSeqNum(iSeqNum);
    sharedMediaCmdPtr->setStreamID(iStreamID);

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);
    if (iOutPort->QueueOutgoingMsg(mediaMsgOut) != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::SendBeginOfMediaStreamCommand() Outgoing queue busy"));
        return false;
    }

    iSendBOS = false;

    // reset repositioning related flags
    iIsRepositionIdleDoneReceivedFromComponent = false;
    iIsRepositionIdleRequestSentToComponent = false;
    iIsRepositionExecRequestSentToComponent = false;
    iIsRepositionExecDoneReceivedFromComponent = false;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendBeginOfMediaStreamCommand() BOS Sent StreamID %d", iStreamID));
    return true;
}
////////////////////////////////////
bool PVMFOMXAudioDecNode::SendEndOfTrackCommand(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendEndOfTrackCommand() In"));

    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();

    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

    // Set the timestamp
    sharedMediaCmdPtr->setTimestamp(iEndOfDataTimestamp);

    // Set the sequence number
    sharedMediaCmdPtr->setSeqNum(iSeqNum++);
    // set stream ID
    sharedMediaCmdPtr->setStreamID(iStreamID);

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);
    if (iOutPort->QueueOutgoingMsg(mediaMsgOut) != PVMFSuccess)
    {
        // this should not happen because we check for queue busy before calling this function
        return false;
    }



    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::SendEndOfTrackCommand() Out"));
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//The various command handlers call this routine when a command is complete.
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::CommandComplete(PVMFOMXAudioDecNodeCmdQ& aCmdQ, PVMFOMXAudioDecNodeCommand& aCmd, PVMFStatus aStatus, OsclAny* aEventData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                    , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    //create response
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aEventData);
    PVMFSessionId session = aCmd.iSession;

    //Erase the command from the queue.
    aCmdQ.Erase(&aCmd);

    //Report completion to the session observer.
    ReportCmdCompleteEvent(session, resp);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoInit(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoInit() In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
        {
            SetState(EPVMFNodeInitialized);
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            break;
        }

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoPrepare(PVMFOMXAudioDecNodeCommand& aCmd)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_STRING Role = NULL;

    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
        {
            if (NULL == iInPort)
            {
                CommandComplete(iInputCommands, aCmd, PVMFFailure);
            }

            // Check format of input data

            uint32 Format = ((PVMFOMXAudioDecPort*)iInPort)->iFormat;
            switch (Format)
            {
                    // AAC
                case PVMF_MPEG4_AUDIO:
                case PVMF_LATM:
                case PVMF_ADIF:
                case PVMF_ASF_MPEG4_AUDIO:
                case PVMF_AAC_SIZEHDR:
                    Role = "audio_decoder.aac";
                    break;

                    // AMR
                case PVMF_AMR_IF2:
                case PVMF_AMR_IETF:
                case PVMF_AMR_IETF_COMBINED:
                case PVMF_AMRWB_IETF:
                case PVMF_AMRWB_IETF_PAYLOAD:

                    Role = "audio_decoder.amr";
                    break;

                case PVMF_MP3:

                    Role = "audio_decoder.mp3";
                    break;

                case PVMF_WMA:
                    Role = "audio_decoder.wma";
                    break;

                default:
                    // Illegal codec specified.
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAVCNode::DoPrepare() Input port format other then codec type"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
                    return;
            }


            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::Initializing OMX component and decoder for role %s", Role));

            /* Set callback structure */
            iCallbacks.EventHandler    = CallbackEventHandler_Audio; //event_handler;
            iCallbacks.EmptyBufferDone = CallbackEmptyBufferDone_Audio; //empty_buffer_done;
            iCallbacks.FillBufferDone  = CallbackFillBufferDone_Audio; //fill_buffer_done;


            // determine components which can fit the role
            // then, create the component. If multiple components fit the role,
            // the first one registered will be selected. If that one fails to
            // be created, the second one in the list is selected etc.
            OMX_U32 num_comps = 0;
            OMX_STRING *CompOfRole;
            // call once to find out the number of components that can fit the role
            PV_MasterOMX_GetComponentsOfRole(Role, &num_comps, NULL);
            uint32 ii;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::DoPrepare(): There are %d components of role %s ", num_comps, Role));

            if (num_comps > 0)
            {
                CompOfRole = (OMX_STRING *)oscl_malloc(num_comps * sizeof(OMX_STRING));

                for (ii = 0; ii < num_comps; ii++)
                    CompOfRole[ii] = (OMX_STRING) oscl_malloc(PV_OMX_MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8));

                // call 2nd time to get the component names
                PV_MasterOMX_GetComponentsOfRole(Role, &num_comps, (OMX_U8 **)CompOfRole);

                for (ii = 0; ii < num_comps; ii++)
                {
                    // try to create component
                    err = PV_MasterOMX_GetHandle(&iOMXAudioDecoder, (OMX_STRING) CompOfRole[ii], (OMX_PTR) this, (OMX_CALLBACKTYPE *) & iCallbacks);
                    // if successful, no need to continue
                    if ((err == OMX_ErrorNone) && (iOMXAudioDecoder != NULL))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMFOMXAudioDecNode::DoPrepare(): Got Component %s handle ", CompOfRole[ii]));

                        break;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMFOMXAudioDecNode::DoPrepare(): Cannot get component %s handle, try another component if available", CompOfRole[ii]));
                    }

                }
                // whether successful or not, need to free CompOfRoles
                for (ii = 0; ii < num_comps; ii++)
                {
                    oscl_free(CompOfRole[ii]);
                    CompOfRole[ii] = NULL;
                }

                oscl_free(CompOfRole);
                // check if there was a problem
                if ((err != OMX_ErrorNone) || (iOMXAudioDecoder == NULL))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::Can't get handle for decoder!"));
                    iOMXAudioDecoder = NULL;
                    CommandComplete(iInputCommands, aCmd, PVMFErrResource);
                    return;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::No component can handle role %s !", Role));
                iOMXAudioDecoder = NULL;
                CommandComplete(iInputCommands, aCmd, PVMFErrResource);
                return;
            }



            if (!iOMXAudioDecoder)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }

            // GET CAPABILITY FLAGS FROM PV COMPONENT, IF this fails, use defaults
            PV_OMXComponentCapabilityFlagsType Cap_flags;
            err = OMX_GetParameter(iOMXAudioDecoder, (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX, &Cap_flags);
            if (err != OMX_ErrorNone)
            {
                SetDefaultCapabilityFlags();
            }
            else
            {
                iIsOMXComponentMultiThreaded =					 Cap_flags.iIsOMXComponentMultiThreaded;
                iOMXComponentSupportsExternalInputBufferAlloc =	 Cap_flags.iOMXComponentSupportsExternalInputBufferAlloc;
                iOMXComponentSupportsExternalOutputBufferAlloc = Cap_flags.iOMXComponentSupportsExternalOutputBufferAlloc;
                iOMXComponentSupportsMovableInputBuffers =		 Cap_flags.iOMXComponentSupportsMovableInputBuffers;
            }


            // find out about parameters

            if (!NegotiateComponentParameters())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPrepare() Cannot get component parameters"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }

            // create active objects to handle callbacks in case of multithreaded implementation

            // NOTE: CREATE THE THREADSAFE CALLBACK AOs REGARDLESS OF WHETHER MULTITHREADED COMPONENT OR NOT
            //		If it is not multithreaded, we won't use them
            //		The Flag iIsComponentMultiThreaded decides which mechanism is used for callbacks.
            //		This flag is set by looking at component capabilities (or to true by default)

            if (iThreadSafeHandlerEventHandler)
            {
                OSCL_DELETE(iThreadSafeHandlerEventHandler);
                iThreadSafeHandlerEventHandler = NULL;
            }
            // substitute default parameters: observer(this node),queuedepth(3),nameAO for logging
            // Get the priority of video dec node, and set the threadsafe callback AO priority to 1 higher
            iThreadSafeHandlerEventHandler = OSCL_NEW(EventHandlerThreadSafeCallbackAO_Audio, (this, 10, "EventHandlerAO_Audio", Priority() + 2));

            if (iThreadSafeHandlerEmptyBufferDone)
            {
                OSCL_DELETE(iThreadSafeHandlerEmptyBufferDone);
                iThreadSafeHandlerEmptyBufferDone = NULL;
            }
            // use queue depth of iNumInputBuffers to prevent deadlock
            iThreadSafeHandlerEmptyBufferDone = OSCL_NEW(EmptyBufferDoneThreadSafeCallbackAO_Audio, (this, iNumInputBuffers, "EmptyBufferDoneAO_Audio", Priority() + 1));

            if (iThreadSafeHandlerFillBufferDone)
            {
                OSCL_DELETE(iThreadSafeHandlerFillBufferDone);
                iThreadSafeHandlerFillBufferDone = NULL;
            }
            // use queue depth of iNumOutputBuffers to prevent deadlock
            iThreadSafeHandlerFillBufferDone = OSCL_NEW(FillBufferDoneThreadSafeCallbackAO_Audio, (this, iNumOutputBuffers, "FillBufferDoneAO_Audio", Priority() + 1));

            if ((iThreadSafeHandlerEventHandler == NULL) ||
                    (iThreadSafeHandlerEmptyBufferDone == NULL) ||
                    (iThreadSafeHandlerFillBufferDone == NULL)
               )
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::Can't get threadsafe callbacks for decoder!"));
                iOMXAudioDecoder = NULL;
            }


            iSetMarkerBitForEveryFrag = false;


            // Init Decoder
            iCurrentDecoderState = OMX_StateLoaded;

            /* Change state to OMX_StateIdle from OMX_StateLoaded. */
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::DoPrepare(): Changing Component state Loaded -> Idle "));

            err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (err != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPrepare() Can't send StateSet command!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }


            /* Allocate input buffers */
            if (!CreateInputMemPool(iNumInputBuffers))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPrepare() Can't allocate mempool for input buffers!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }

            if (!ProvideBuffersToComponent(iInBufMemoryPool, // allocator
                                           iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                           iNumInputBuffers, // number of buffers
                                           iOMXComponentInputBufferSize, // actual buffer size
                                           iInputPortIndex, // port idx
                                           iOMXComponentSupportsExternalInputBufferAlloc, // can component use OMX_UseBuffer
                                           true // this is input
                                          ))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPrepare() Component can't use input buffers!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }


            /* Allocate output buffers */
            if (!CreateOutMemPool(iNumOutputBuffers))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPrepare() Can't allocate mempool for output buffers!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }

            if (!ProvideBuffersToComponent(iOutBufMemoryPool, // allocator
                                           iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                           iNumOutputBuffers, // number of buffers
                                           iOMXComponentOutputBufferSize, // actual buffer size
                                           iOutputPortIndex, // port idx
                                           iOMXComponentSupportsExternalOutputBufferAlloc, // can component use OMX_UseBuffer
                                           false // this is not input
                                          ))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPrepare() Component can't use output buffers!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }


            //this command is asynchronous.  move the command from
            //the input command queue to the current command, where
            //it will remain until it completes. We have to wait for
            // OMX component state transition to complete

            int32 err;
            OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
            if (err != OsclErrNone)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
            iInputCommands.Erase(&aCmd);

        }
        break;

        case EPVMFNodePrepared:
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }

}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoStart(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoStart() In"));

    iDiagnosticsLogged = false;

    PVMFStatus status = PVMFSuccess;

    OMX_ERRORTYPE  err;
    OMX_STATETYPE sState;

    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
        case EPVMFNodePaused:
        {
            //Get state of OpenMAX decoder
            err = OMX_GetState(iOMXAudioDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,

                                (0, "PVMFOMXAudioDecNode::DoStart(): Can't get State of decoder!"));

                sState = OMX_StateInvalid;
            }

            if ((sState == OMX_StateIdle) || (sState == OMX_StatePause))
            {
                /* Change state to OMX_StateExecuting form OMX_StateIdle. */
                // init the flag
                if (!iDynamicReconfigInProgress)
                {
                    iDoNotSendOutputBuffersDownstreamFlag = false; // or if output was not being sent downstream due to state changes
                    // re-enable sending output
                    iDoNotSaveInputBuffersFlag = false;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::DoStart() Changing Component state Idle->Executing"));

                err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::DoStart(): Can't send StateSet command to decoder!"));

                    status = PVMFErrInvalidState;
                }

            }
            else
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoStart(): Decoder is not in the Idle or Pause state!"));

                status = PVMFErrInvalidState;
            }


        }
        break;

        default:
            status = PVMFErrInvalidState;
            break;
    }

    if (status == PVMFErrInvalidState)
    {
        CommandComplete(iInputCommands, aCmd, status);
    }
    else
    {
        //this command is asynchronous.  move the command from
        //the input command queue to the current command, where
        //it will remain until it completes.
        int32 err;
        OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
        if (err != OsclErrNone)
        {
            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
        }
        iInputCommands.Erase(&aCmd);
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoStop(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoStop() In"));

    LogDiagnostics();

    OMX_ERRORTYPE  err;
    OMX_STATETYPE sState;

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        case EPVMFNodePrepared:
            // Stop data source
            // This will also prevent execution of HandleProcessingState

            iDataIn.Unbind();
            // Clear queued messages in ports
            if (iInPort)
            {
                iInPort->ClearMsgQueues();
            }

            if (iOutPort)
            {
                iOutPort->ClearMsgQueues();
            }

            // Clear the data flags

            iEndOfDataReached = false;
            iIsEOSSentToComponent = false;
            iIsEOSReceivedFromComponent = false;


            iDoNotSendOutputBuffersDownstreamFlag = true; // stop sending output buffers downstream
            iDoNotSaveInputBuffersFlag = true;

            //Get state of OpenMAX decoder
            err = OMX_GetState(iOMXAudioDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoStop(): Can't get State of decoder!"));

                sState = OMX_StateInvalid;
            }

            if ((sState == OMX_StateExecuting) || (sState == OMX_StatePause))
            {
                /* Change state to OMX_StateIdle from OMX_StateExecuting or OMX_StatePause. */

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::DoStop() Changing Component State Executing->Idle or Pause->Idle"));

                err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::DoStop(): Can't send StateSet command to decoder!"));

                    CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                    break;
                }

                // prevent the node from sending more buffers etc.
                // if port reconfiguration is in process, let the state remain one of the port config states
                //	if there is a start command, we can do it seemlessly (by continuing the port reconfig)
                if (iProcessingState == EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode)
                    iProcessingState = EPVMFOMXAudioDecNodeProcessingState_Stopping;

                // indicate that stop cmd was sent
                if (iDynamicReconfigInProgress)
                {
                    iStopCommandWasSentToComponent = true;
                }



            }
            else
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoStop(): Decoder is not in the Executing or Pause state!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                break;
            }

            //this command is asynchronous.  move the command from
            //the input command queue to the current command, where
            //it will remain until it completes.
            int32 err;
            OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
            if (err != OsclErrNone)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
            iInputCommands.Erase(&aCmd);

            break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoFlush(PVMFOMXAudioDecNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            //the flush is asynchronous.  move the command from
            //the input command queue to the current command, where
            //it will remain until the flush completes.
            int32 err;
            OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
            if (err != OsclErrNone)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
            iInputCommands.Erase(&aCmd);

            //Notify all ports to suspend their input
            if (iInPort)
            {
                iInPort->SuspendInput();
            }
            if (iOutPort)
            {
                iOutPort->SuspendInput();
            }
            break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoPause(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoPause() In"));

    OMX_ERRORTYPE  err;
    OMX_STATETYPE sState;

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:


            //Get state of OpenMAX decoder
            err = OMX_GetState(iOMXAudioDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPause(): Can't get State of decoder!"));

                sState = OMX_StateInvalid;
            }

            if (sState == OMX_StateExecuting)
            {
                /* Change state to OMX_StatePause from OMX_StateExecuting. */
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXAudioDecNode::DoPause() Changing Component State Executing->Pause"));

                err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandStateSet, OMX_StatePause, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::DoPause(): Can't send StateSet command to decoder!"));

                    CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                    break;
                }

                // prevent the node from sending more buffers etc.
                // if port reconfiguration is in process, let the state remain one of the port config states
                //	if there is a start command, we can do it seemlessly (by continuing the port reconfig)
                if (iProcessingState == EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode)
                    iProcessingState = EPVMFOMXAudioDecNodeProcessingState_Pausing;

                // indicate that pause cmd was sent
                if (iDynamicReconfigInProgress)
                {
                    iPauseCommandWasSentToComponent = true;
                }


            }
            else
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoPause(): Decoder is not in the Executing state!"));
                CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                break;
            }

            //this command is asynchronous.  move the command from
            //the input command queue to the current command, where
            //it will remain until it completes.
            int32 err;
            OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
            if (err != OsclErrNone)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
            iInputCommands.Erase(&aCmd);

            break;
        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoReset(PVMFOMXAudioDecNodeCommand& aCmd)
{

    OMX_ERRORTYPE  err;
    OMX_STATETYPE sState;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DoReset() In"));

    LogDiagnostics();

    switch (iInterfaceState)
    {

        case EPVMFNodeIdle:
        case EPVMFNodeInitialized:
        case EPVMFNodePrepared:
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        case EPVMFNodeError:

        {
            //Check if decoder is initilized
            if (iOMXAudioDecoder != NULL)
            {
                //Get state of OpenMAX decoder
                err = OMX_GetState(iOMXAudioDecoder, &sState);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::DoReset(): Can't get State of decoder!"));
                    if (iResetInProgress)
                    {
                        // cmd is in current q
                        iResetInProgress = false;
                        if ((iCurrentCommand.size() > 0) &&
                                (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
                           )
                        {
                            CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFErrResource);
                        }

                    }
                    else
                    {
                        CommandComplete(iInputCommands, aCmd, PVMFErrResource);
                    }
                    return;
                }

                if (sState == OMX_StateLoaded)
                {
                    // this is a value obtained by synchronous call to component. Either the component was
                    // already in this state without node issuing any commands,
                    // or perhaps we started the Reset, but the callback notification has not yet arrived.
                    if (iResetInProgress)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXAudioDecNode::DoReset() OMX comp is in loaded state. Wait for official callback to change variables etc."));
                        return;
                    }
                    else
                    {
                        CommandComplete(iInputCommands, aCmd, PVMFErrResource);
                        return;
                    }
                }

                if (sState == OMX_StateIdle)
                {


                    //this command is asynchronous.  move the command from
                    //the input command queue to the current command, where
                    //it will remain until it is completed.
                    if (!iResetInProgress)
                    {
                        int32 err;
                        OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
                        if (err != OsclErrNone)
                        {
                            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                            return;
                        }
                        iInputCommands.Erase(&aCmd);

                        iResetInProgress = true;
                    }

                    // if buffers aren't all back (due to timing issues with different callback AOs
                    //		state change can be reported before all buffers are returned)
                    if (iNumOutstandingInputBuffers > 0 || iNumOutstandingOutputBuffers > 0)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXAudioDecNode::DoReset() Waiting for %d input and-or %d output buffers", iNumOutstandingInputBuffers, iNumOutstandingOutputBuffers));

                        return;
                    }

                    if (!iResetMsgSent)
                    {
                        // We can come here only if all buffers are already back
                        // Don't repeat any of this twice.
                        /* Change state to OMX_StateLoaded form OMX_StateIdle. */
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXAudioDecNode::DoReset() Changing Component State Idle->Loaded"));

                        err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandStateSet, OMX_StateLoaded, NULL);
                        if (err != OMX_ErrorNone)
                        {
                            //Error condition report
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                            (0, "PVMFOMXAudioDecNode::DoReset(): Can't send StateSet command to decoder!"));
                        }

                        iResetMsgSent = true;


                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXAudioDecNode::DoReset() freeing output buffers"));

                        if (false == iOutputBuffersFreed)
                        {
                            if (!FreeBuffersFromComponent(iOutBufMemoryPool, // allocator
                                                          iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                                          iNumOutputBuffers, // number of buffers
                                                          iOutputPortIndex, // port idx
                                                          false // this is not input
                                                         ))
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                                (0, "PVMFOMXAudioDecNode::DoReset() Cannot free output buffers "));

                                if (iResetInProgress)
                                {
                                    iResetInProgress = false;
                                    if ((iCurrentCommand.size() > 0) &&
                                            (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
                                       )
                                    {
                                        CommandComplete(iCurrentCommand, iCurrentCommand.front() , PVMFErrResource);
                                    }
                                }

                            }

                        }
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXAudioDecNode::DoReset() freeing input buffers "));
                        if (false == iInputBuffersFreed)
                        {
                            if (!FreeBuffersFromComponent(iInBufMemoryPool, // allocator
                                                          iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                                          iNumInputBuffers, // number of buffers
                                                          iInputPortIndex, // port idx
                                                          true // this is input
                                                         ))
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                                (0, "PVMFOMXAudioDecNode::DoReset() Cannot free input buffers "));

                                if (iResetInProgress)
                                {
                                    iResetInProgress = false;
                                    if ((iCurrentCommand.size() > 0) &&
                                            (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
                                       )
                                    {
                                        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFErrResource);
                                    }
                                }


                            }
                        }



                        iEndOfDataReached = false;
                        iIsEOSSentToComponent = false;
                        iIsEOSReceivedFromComponent = false;



                        // also, perform Port deletion when the component replies with the command
                        // complete, not right here
                    } // end of if(iResetMsgSent)


                    return;

                }
                else
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXAudioDecNode::DoReset(): Decoder is not in the Idle state!"));
                    if (iResetInProgress)
                    {
                        iResetInProgress = false;
                        if ((iCurrentCommand.size() > 0) &&
                                (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
                           )
                        {
                            CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFErrInvalidState);
                        }
                    }
                    else
                    {
                        CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                    }
                    break;
                }//end of if (sState == OMX_StateIdle)
            }//end of if (iOMXAudioDecoder != NULL)

            //delete all ports and notify observer.
            if (iInPort)
            {
                OSCL_DELETE(((PVMFOMXAudioDecPort*)iInPort));
                iInPort = NULL;
            }

            if (iOutPort)
            {
                OSCL_DELETE(((PVMFOMXAudioDecPort*)iOutPort));
                iOutPort = NULL;
            }

            iDataIn.Unbind();


            // Reset the metadata key list
            iAvailableMetadataKeys.clear();

            iEndOfDataReached = false;
            iIsEOSSentToComponent = false;
            iIsEOSReceivedFromComponent = false;


            iProcessingState = EPVMFOMXAudioDecNodeProcessingState_Idle;
            //logoff & go back to Created state.
            SetState(EPVMFNodeIdle);


            if (iResetInProgress)
            {
                iResetInProgress = false;
                if ((iCurrentCommand.size() > 0) &&
                        (iCurrentCommand.front().iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
                   )
                {
                    CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
                }
            }
            else
            {
                CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            }

        }
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoRequestPort(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DoRequestPort() In"));
    //This node supports port request from any state

    //retrieve port tag.
    int32 tag;
    OSCL_String* portconfig;

    aCmd.PVMFOMXAudioDecNodeCommandBase::Parse(tag, portconfig);

    PVMFPortInterface* port = NULL;
    int32 leavecode = 0;
    //validate the tag...
    switch (tag)
    {
        case PVMF_OMX_AUDIO_DEC_NODE_PORT_TYPE_SOURCE:
            if (iInPort)
            {
                CommandComplete(iInputCommands, aCmd, PVMFFailure);
                break;
            }
            OSCL_TRY(leavecode, iInPort = OSCL_NEW(PVMFOMXAudioDecPort, ((int32)tag, this, "OMXAudioDecIn(Video)")););
            if (leavecode || iInPort == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoRequestPort: Error - Input port instantiation failed"));
                CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
                return;
            }
            port = iInPort;
            break;

        case PVMF_OMX_AUDIO_DEC_NODE_PORT_TYPE_SINK:
            if (iOutPort)
            {
                CommandComplete(iInputCommands, aCmd, PVMFFailure);
                break;
            }
            OSCL_TRY(leavecode, iOutPort = OSCL_NEW(PVMFOMXAudioDecPort, ((int32)tag, this, "OMXAudioDecOut(Video)")));
            if (leavecode || iOutPort == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXAudioDecNode::DoRequestPort: Error - Output port instantiation failed"));
                CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
                return;
            }
            port = iOutPort;
            break;

        default:
            //bad port tag
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
            return;
    }

    //Return the port pointer to the caller.
    CommandComplete(iInputCommands, aCmd, PVMFSuccess, (OsclAny*)port);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoReleasePort(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVMFOMXAudioDecPort* port;
    aCmd.PVMFOMXAudioDecNodeCommandBase::Parse((PVMFPortInterface*&)port);

    if (port != NULL && (port == iInPort || port == iOutPort))
    {
        if (port == iInPort)
        {
            OSCL_DELETE(((PVMFOMXAudioDecPort*)iInPort));
            iInPort = NULL;
        }
        else
        {
            OSCL_DELETE(((PVMFOMXAudioDecPort*)iOutPort));
            iOutPort = NULL;
        }
        //delete the port.
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else
    {
        //port not found.
        CommandComplete(iInputCommands, aCmd, PVMFFailure);
    }
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::DoGetNodeMetadataKey(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DoGetNodeMetadataKey() In"));

    PVMFMetadataList* keylistptr = NULL;
    uint32 starting_index;
    int32 max_entries;
    char* query_key;

    aCmd.PVMFOMXAudioDecNodeCommand::Parse(keylistptr, starting_index, max_entries, query_key);

    // Check parameters
    if (keylistptr == NULL)
    {
        // The list pointer is invalid
        return PVMFErrArgument;
    }

    if ((starting_index > (iAvailableMetadataKeys.size() - 1)) || max_entries == 0)
    {
        // Invalid starting index and/or max entries
        return PVMFErrArgument;
    }

    // Copy the requested keys
    uint32 num_entries = 0;
    int32 num_added = 0;
    int32 leavecode = 0;
    for (uint32 lcv = 0; lcv < iAvailableMetadataKeys.size(); lcv++)
    {
        if (query_key == NULL)
        {
            // No query key so this key is counted
            ++num_entries;
            if (num_entries > starting_index)
            {
                // Past the starting index so copy the key
                leavecode = 0;
                OSCL_TRY(leavecode, keylistptr->push_back(iAvailableMetadataKeys[lcv]));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::DoGetNodeMetadataKey() Memory allocation failure when copying metadata key"));
                                     return PVMFErrNoMemory);
                num_added++;
            }
        }
        else
        {
            // Check if the key matche the query key
            if (pv_mime_strcmp(iAvailableMetadataKeys[lcv].get_cstr(), query_key) >= 0)
            {
                // This key is counted
                ++num_entries;
                if (num_entries > starting_index)
                {
                    // Past the starting index so copy the key
                    leavecode = 0;
                    OSCL_TRY(leavecode, keylistptr->push_back(iAvailableMetadataKeys[lcv]));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::DoGetNodeMetadataKey() Memory allocation failure when copying metadata key"));
                                         return PVMFErrNoMemory);
                    num_added++;
                }
            }
        }

        // Check if max number of entries have been copied
        if (max_entries > 0 && num_added >= max_entries)
        {
            break;
        }
    }

    return PVMFSuccess;
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::DoGetNodeMetadataValue(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVMFMetadataList* keylistptr = NULL;
    Oscl_Vector<PvmiKvp, OsclMemAllocator>* valuelistptr = NULL;
    uint32 starting_index;
    int32 max_entries;

    aCmd.PVMFOMXAudioDecNodeCommand::Parse(keylistptr, valuelistptr, starting_index, max_entries);

    // Check the parameters
    if (keylistptr == NULL || valuelistptr == NULL)
    {
        return PVMFErrArgument;
    }

    uint32 numkeys = keylistptr->size();

    if (starting_index > (numkeys - 1) || numkeys <= 0 || max_entries == 0)
    {
        // Don't do anything
        return PVMFErrArgument;
    }

    uint32 numvalentries = 0;
    int32 numentriesadded = 0;
    for (uint32 lcv = 0; lcv < numkeys; lcv++)
    {
        int32 leavecode = 0;
        PvmiKvp KeyVal;
        KeyVal.key = NULL;
        uint32 KeyLen = 0;
        if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_CHANNELS_KEY) == 0))
        {
            // PCM output channels
            if (iNumberOfAudioChannels > 0)
            {
                // Increment the counter for the number of values found so far
                ++numvalentries;

                // Create a value entry if past the starting index
                if (numvalentries > starting_index)
                {
                    KeyLen = oscl_strlen(PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_CHANNELS_KEY) + 1; // for "codec-info/audio/channels;"
                    KeyLen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
                    KeyLen += oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR) + 1; // for "uint32" and NULL terminator

                    // Allocate memory for the string
                    leavecode = 0;
                    OSCL_TRY(leavecode,
                             KeyVal.key = OSCL_ARRAY_NEW(char, KeyLen);
                            );

                    if (leavecode == 0)
                    {
                        // Copy the key string
                        oscl_strncpy(KeyVal.key, PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_CHANNELS_KEY, oscl_strlen(PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_CHANNELS_KEY) + 1);
                        oscl_strncat(KeyVal.key, PVOMXAUDIODECMETADATA_SEMICOLON, oscl_strlen(PVOMXAUDIODECMETADATA_SEMICOLON));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                        KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                        // Copy the value
                        KeyVal.value.uint32_value = iNumberOfAudioChannels;
                        // Set the length and capacity
                        KeyVal.length = 1;
                        KeyVal.capacity = 1;
                    }
                    else
                    {
                        // Memory allocation failed
                        KeyVal.key = NULL;
                        break;
                    }
                }
            }
        }
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_SAMPLERATE_KEY) == 0) &&
                 (iPCMSamplingRate > 0))
        {
            // PCM output sampling rate
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_SAMPLERATE_KEY) + 1; // for "codec-info/audio/sample-rate;"
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR) + 1; // for "uint32" and NULL terminator

                // Allocate memory for the string
                leavecode = 0;
                OSCL_TRY(leavecode,
                         KeyVal.key = OSCL_ARRAY_NEW(char, KeyLen);
                        );

                if (leavecode == 0)
                {
                    // Copy the key string
                    oscl_strncpy(KeyVal.key, PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_SAMPLERATE_KEY, oscl_strlen(PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_SAMPLERATE_KEY) + 1);
                    oscl_strncat(KeyVal.key, PVOMXAUDIODECMETADATA_SEMICOLON, oscl_strlen(PVOMXAUDIODECMETADATA_SEMICOLON));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                    KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                    // Copy the value
                    KeyVal.value.uint32_value = iPCMSamplingRate;
                    // Set the length and capacity
                    KeyVal.length = 1;
                    KeyVal.capacity = 1;
                }
                else
                {
                    // Memory allocation failed
                    KeyVal.key = NULL;
                    break;
                }
            }
        }
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_FORMAT_KEY) == 0) &&
                 iInPort != NULL)
        {
            // Format
            if ((((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_LATM) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_MPEG4_AUDIO) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_ADIF) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AAC_SIZEHDR) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IF2) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IETF) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IETF_COMBINED) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMRWB_IETF) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMRWB_IETF_PAYLOAD) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_MP3) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_WMA)

               )
            {
                // Increment the counter for the number of values found so far
                ++numvalentries;

                // Create a value entry if past the starting index
                if (numvalentries > starting_index)
                {
                    KeyLen = oscl_strlen(PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_FORMAT_KEY) + 1; // for "codec-info/audio/format;"
                    KeyLen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
                    KeyLen += oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR) + 1; // for "char*" and NULL terminator

                    uint32 valuelen = 0;
                    switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
                    {
                        case PVMF_LATM:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_LATM)) + 1; // Value string plus one for NULL terminator
                            break;

                        case PVMF_MPEG4_AUDIO:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_MPEG4_AUDIO)) + 1; // Value string plus one for NULL terminator
                            break;
                        case PVMF_ADIF:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_ADIF)) + 1;
                            break;

                        case PVMF_AMR_IF2:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_AMR_IF2)) + 1;
                            break;

                        case PVMF_AMR_IETF:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_AMR_IETF)) + 1;
                            break;

                        case PVMF_AMR_IETF_COMBINED:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_AMR)) + 1;
                            break;

                        case PVMF_AMRWB_IETF:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_AMRWB_IETF)) + 1;
                            break;

                        case PVMF_AMRWB_IETF_PAYLOAD:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_AMRWB)) + 1;
                            break;

                        case PVMF_MP3:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_MP3)) + 1;
                            break;

                        case PVMF_WMA:
                            valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_WMA)) + 1;
                            break;

                        default:
                            // Should not enter here
                            OSCL_ASSERT(false);
                            valuelen = 1;
                            break;
                    }

                    // Allocate memory for the strings
                    leavecode = 0;
                    OSCL_TRY(leavecode,
                             KeyVal.key = OSCL_ARRAY_NEW(char, KeyLen);
                             KeyVal.value.pChar_value = OSCL_ARRAY_NEW(char, valuelen);
                            );

                    if (leavecode == 0)
                    {
                        // Copy the key string
                        oscl_strncpy(KeyVal.key, PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_FORMAT_KEY, oscl_strlen(PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_FORMAT_KEY) + 1);
                        oscl_strncat(KeyVal.key, PVOMXAUDIODECMETADATA_SEMICOLON, oscl_strlen(PVOMXAUDIODECMETADATA_SEMICOLON));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR));
                        KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                        // Copy the value
                        switch (((PVMFOMXAudioDecPort*)iInPort)->iFormat)
                        {
                            case PVMF_LATM:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_LATM), valuelen);
                                break;

                            case PVMF_MPEG4_AUDIO:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_MPEG4_AUDIO), valuelen);
                                break;

                            case PVMF_ADIF:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_ADIF), valuelen);
                                break;

                            case PVMF_AMR_IF2:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_AMR_IF2), valuelen);
                                break;

                            case PVMF_AMR_IETF:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_AMR_IETF), valuelen);
                                break;

                            case PVMF_AMR_IETF_COMBINED:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_AMR), valuelen);
                                break;

                            case PVMF_AMRWB_IETF:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_AMRWB_IETF), valuelen);
                                break;

                            case PVMF_AMRWB_IETF_PAYLOAD:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_AMRWB), valuelen);
                                break;

                            case PVMF_MP3:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_MP3), valuelen);
                                break;

                            case PVMF_WMA:
                                oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_WMA), valuelen);
                                break;

                            default:
                                // Should not enter here
                                OSCL_ASSERT(false);
                                break;
                        }
                        KeyVal.value.pChar_value[valuelen-1] = NULL_TERM_CHAR;
                        // Set the length and capacity
                        KeyVal.length = valuelen;
                        KeyVal.capacity = valuelen;
                    }
                    else
                    {
                        // Memory allocation failed so clean up
                        if (KeyVal.key)
                        {
                            OSCL_ARRAY_DELETE(KeyVal.key);
                            KeyVal.key = NULL;
                        }
                        if (KeyVal.value.pChar_value)
                        {
                            OSCL_ARRAY_DELETE(KeyVal.value.pChar_value);
                        }
                        break;
                    }
                }
            }
        }

        if (KeyVal.key != NULL)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, (*valuelistptr).push_back(KeyVal));
            if (leavecode != 0)
            {
                switch (GetValTypeFromKeyString(KeyVal.key))
                {
                    case PVMI_KVPVALTYPE_CHARPTR:
                        if (KeyVal.value.pChar_value != NULL)
                        {
                            OSCL_ARRAY_DELETE(KeyVal.value.pChar_value);
                            KeyVal.value.pChar_value = NULL;
                        }
                        break;

                    default:
                        // Add more case statements if other value types are returned
                        break;
                }

                OSCL_ARRAY_DELETE(KeyVal.key);
                KeyVal.key = NULL;
            }
            else
            {
                // Increment the counter for number of value entries added to the list
                ++numentriesadded;
            }

            // Check if the max number of value entries were added
            if (max_entries > 0 && numentriesadded >= max_entries)
            {
                break;
            }

        }

    }

    return PVMFSuccess;

}

/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::ReleaseAllPorts()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ReleaseAllPorts() In"));

    if (iInPort)
    {
        iInPort->ClearMsgQueues();
        iInPort->Disconnect();
        OSCL_DELETE(((PVMFOMXAudioDecPort*)iInPort));
        iInPort = NULL;
    }

    if (iOutPort)
    {
        iOutPort->ClearMsgQueues();
        iOutPort->Disconnect();
        OSCL_DELETE(((PVMFOMXAudioDecPort*)iOutPort));
        iOutPort = NULL;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// Clean Up Decoder
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::DeleteOMXAudioDecoder()
{
    OMX_ERRORTYPE  err;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DeleteOMXAudioDecoder() In"));

    if (iOMXAudioDecoder != NULL)
    {
        /* Free Component handle. */
        err = PV_MasterOMX_FreeHandle(iOMXAudioDecoder);
        if (err != OMX_ErrorNone)
        {
            //Error condition report
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::DeleteOMXAudioDecoder(): Can't free decoder's handle!"));
        }
        iOMXAudioDecoder = NULL;

    }//end of if (iOMXAudioDecoder != NULL)


    return true;
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::ChangeNodeState(TPVMFNodeInterfaceState aNewState)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ChangeNodeState() Changing state from %d to %d", iInterfaceState, aNewState));
    iInterfaceState = aNewState;
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::freechunkavailable(OsclAny *aContext)
{

    // check context to see whether input or output buffer was returned to the mempool
    if (aContext == (OsclAny *) iInBufMemoryPool)
    {

        iNumOutstandingInputBuffers--;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::freechunkavailable() Memory chunk in INPUT mempool was deallocated, %d out of %d now available", iNumInputBuffers - iNumOutstandingInputBuffers, iNumInputBuffers));

        // notification only works once.
        // If there are multiple buffers coming back in a row, make sure to set the notification
        // flag in the mempool again, so that next buffer also causes notification
        iInBufMemoryPool->notifyfreechunkavailable(*this, aContext);

    }
    else if (aContext == (OsclAny *) iOutBufMemoryPool)
    {

        iNumOutstandingOutputBuffers--;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::freechunkavailable() Memory chunk in OUTPUT mempool was deallocated, %d out of %d now available", iNumOutputBuffers - iNumOutstandingOutputBuffers, iNumOutputBuffers));

        // notification only works once.
        // If there are multiple buffers coming back in a row, make sure to set the notification
        // flag in the mempool again, so that next buffer also causes notification
        iOutBufMemoryPool->notifyfreechunkavailable(*this, aContext);

    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::freechunkavailable() UNKNOWN mempool "));

    }

    // reschedule
    if (IsAdded())
        RunIfNotReady();


}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "0x%x PVMFOMXAudioDecNode::PortActivity: port=0x%x, type=%d",
                     this, aActivity.iPort, aActivity.iType));

    switch (aActivity.iType)
    {
        case PVMF_PORT_ACTIVITY_OUTGOING_MSG:
            //An outgoing message was queued on this port.
            //We only need to queue a port activity event on the
            //first message.  Additional events will be queued during
            //the port processing as needed.
            if (aActivity.iPort->OutgoingMsgQueueSize() == 1)
            {
                //wake up the AO to process the port activity event.
                RunIfNotReady();
            }
            break;

        case PVMF_PORT_ACTIVITY_INCOMING_MSG:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXAudioDecNode::PortActivity: IncomingMsgQueueSize=%d", aActivity.iPort->IncomingMsgQueueSize()));
            if (aActivity.iPort->IncomingMsgQueueSize() == 1)
            {
                //wake up the AO to process the port activity event.
                RunIfNotReady();
            }
            break;

        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
            if (iProcessingState == EPVMFOMXAudioDecNodeProcessingState_WaitForOutgoingQueue)
            {
                iProcessingState = EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode;
                RunIfNotReady();
            }
            break;

        case PVMF_PORT_ACTIVITY_CONNECT:
            //nothing needed.
            break;

        case PVMF_PORT_ACTIVITY_DISCONNECT:
            //clear the node input data when either port is disconnected.

            iDataIn.Unbind();
            break;

        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_BUSY:
            // The connected port has become busy (its incoming queue is
            // busy).
            // No action is needed here-- the port processing code
            // checks for connected port busy during data processing.
            break;

        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_READY:
            // The connected port has transitioned from Busy to Ready to Receive.
            // It's time to start processing outgoing messages again.

            //iProcessingState should transition from WaitForOutputPort to ReadyToDecode
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "0x%x PVMFOMXAudioDecNode::PortActivity: Connected port is now ready", this));
            RunIfNotReady();
            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoCancelAllCommands(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DoCancelAllCommands"));

    //first cancel the current command if any
    {
        while (!iCurrentCommand.empty())
        {
            CommandComplete(iCurrentCommand, iCurrentCommand[0], PVMFErrCancelled);
        }

    }

    //next cancel all queued commands
    {
        //start at element 1 since this cancel command is element 0.
        while (iInputCommands.size() > 1)
        {
            CommandComplete(iInputCommands, iInputCommands[1], PVMFErrCancelled);
        }
    }

    if (iResetInProgress && !iResetMsgSent)
    {
        // if reset is started but reset msg has not been sent, we can cancel reset
        // as if nothing happened. Otherwise, the callback will set the flag back to false
        iResetInProgress = false;
    }
    //finally, report cancel complete.
    CommandComplete(iInputCommands, aCmd, PVMFSuccess);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoCancelCommand(PVMFOMXAudioDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DoCancelCommand"));

    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.PVMFOMXAudioDecNodeCommandBase::Parse(id);

    //first check "current" command if any
    {
        PVMFOMXAudioDecNodeCommand* cmd = iCurrentCommand.FindById(id);
        if (cmd)
        {

            // if reset is being canceled:
            if (cmd->iCmd == PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_RESET)
            {
                if (iResetInProgress && !iResetMsgSent)
                {
                    // if reset is started but reset msg has not been sent, we can cancel reset
                    // as if nothing happened. Otherwise, the callback will set the flag back to false
                    iResetInProgress = false;
                }
            }
            //cancel the queued command
            CommandComplete(iCurrentCommand, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            return;
        }
    }

    //next check input queue.
    {
        //start at element 1 since this cancel command is element 0.
        PVMFOMXAudioDecNodeCommand* cmd = iInputCommands.FindById(id, 1);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iInputCommands, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            return;
        }
    }
    //if we get here the command isn't queued so the cancel fails.
    CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoQueryUuid(PVMFOMXAudioDecNodeCommand& aCmd)
{
    //This node supports Query UUID from any state

    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;
    aCmd.PVMFOMXAudioDecNodeCommandBase::Parse(mimetype, uuidvec, exactmatch);

    //Try to match the input mimetype against any of
    //the custom interfaces for this node

    //Match against custom interface1...
    if (*mimetype == PVMF_OMX_AUDIO_DEC_NODE_CUSTOM1_MIMETYPE
            //also match against base mimetypes for custom interface1,
            //unless exactmatch is set.
            || (!exactmatch && *mimetype == PVMF_OMX_AUDIO_DEC_NODE_MIMETYPE)
            || (!exactmatch && *mimetype == PVMF_BASEMIMETYPE))
    {

        PVUuid uuid(PVMF_OMX_AUDIO_DEC_NODE_CUSTOM1_UUID);
        uuidvec->push_back(uuid);
    }
    CommandComplete(iInputCommands, aCmd, PVMFSuccess);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::DoQueryInterface(PVMFOMXAudioDecNodeCommand&  aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::DoQueryInterface"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.PVMFOMXAudioDecNodeCommandBase::Parse(uuid, ptr);

    if (*uuid == PVUuid(PVMF_OMX_AUDIO_DEC_NODE_CUSTOM1_UUID))
    {
        addRef();
        *ptr = (PVMFOMXAudioDecNodeExtensionInterface*)this;
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else if (*uuid == PVUuid(KPVMFMetadataExtensionUuid))
    {
        addRef();
        *ptr = (PVMFMetadataExtensionInterface*)this;
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else if (*uuid == PVUuid(PVMI_CAPABILITY_AND_CONFIG_PVUUID))
    {
        addRef();
        *ptr = (PvmiCapabilityAndConfig*)this;
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else
    {
        //not supported
        *ptr = NULL;
        CommandComplete(iInputCommands, aCmd, PVMFFailure);
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::addRef()
{
    ++iExtensionRefCount;
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::removeRef()
{
    --iExtensionRefCount;
}

/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    PVUuid my_uuid(PVMF_OMX_AUDIO_DEC_NODE_CUSTOM1_UUID);
    if (uuid == my_uuid)
    {
        PVMFOMXAudioDecNodeExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMFOMXAudioDecNodeExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
        return true;
    }
    else if (uuid == KPVMFMetadataExtensionUuid)
    {
        PVMFMetadataExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMFMetadataExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXAudioDecNode::HandleRepositioning()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::HandleRepositioning() IN"));


    // 1) Send Flush command to component for both input and output ports
    // 2) "Wait" until component flushes both ports
    // 3) Resume
    OMX_ERRORTYPE  err = OMX_ErrorNone;
    OMX_STATETYPE sState = OMX_StateInvalid;


    if (!iIsRepositioningRequestSentToComponent)
    {

        // first check the state (if executing or paused, continue)
        err = OMX_GetState(iOMXAudioDecoder, &sState);
        if (err != OMX_ErrorNone)
        {
            //Error condition report
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::HandleRepositioning(): Can't get State of decoder - trying to send reposition request!"));

            sState = OMX_StateInvalid;
            ReportErrorEvent(PVMFErrResourceConfiguration);
            ChangeNodeState(EPVMFNodeError);
            return false;
        }

        if ((sState != OMX_StateExecuting) && (sState != OMX_StatePause))
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXAudioDecNode::HandleRepositioning() Component State is not executing or paused, do not proceed with repositioning"));

            return true;

        }


        iIsRepositioningRequestSentToComponent = true; // prevent sending requests multiple times
        iIsInputPortFlushed = false;	// flag that will be set to true once component flushes the port
        iIsOutputPortFlushed = false;
        iDoNotSendOutputBuffersDownstreamFlag = true;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::HandleRepositioning() Sending Flush command to component"));

        // send command to flush all ports (arg is -1)
        err = OMX_SendCommand(iOMXAudioDecoder, OMX_CommandFlush, -1, NULL);
        if (err != OMX_ErrorNone)
        {
            //Error condition report
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXAudioDecNode::HandleRepositioning(): Can't send flush command  - trying to send reposition request!"));

            sState = OMX_StateInvalid;
            ReportErrorEvent(PVMFErrResourceConfiguration);
            ChangeNodeState(EPVMFNodeError);
            return false;
        }

    }

    if (iIsRepositionDoneReceivedFromComponent)
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXAudioDecNode::HandleRepositioning() Component has flushed both ports, and is done repositioning"));

        iIsRepositioningRequestSentToComponent = false; // enable sending requests again
        iIsRepositionDoneReceivedFromComponent = false;
        iIsInputPortFlushed = false;
        iIsOutputPortFlushed = false;

        iDoNotSendOutputBuffersDownstreamFlag = false;
        return true;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXAudioDecNode::HandleRepositioning() Component is not yet done repositioning "));

    return false;
}



PVMFStatus PVMFOMXAudioDecNode::CreateLATMParser()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode::CreateLATMParser() In"));

    // First clean up if necessary
    DeleteLATMParser();

    // Instantiate the LATM parser
    iLATMParser = OSCL_NEW(PV_LATM_Parser, ());
    if (!iLATMParser)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::CreateLATMParser() LATM parser instantiation failed"));
        return PVMFErrNoMemory;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode::CreateLATMParser() Out"));
    return PVMFSuccess;
}

PVMFStatus PVMFOMXAudioDecNode::DeleteLATMParser()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode::DeleteLATMParser() In"));

    // Delete LATM parser if there is one
    if (iLATMParser)
    {
        OSCL_DELETE(iLATMParser);
        iLATMParser = NULL;
    }

    if (iLATMConfigBuffer)
    {
        oscl_free(iLATMConfigBuffer);
        iLATMConfigBuffer = NULL;
        iLATMConfigBufferSize = 0;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode::DeleteLATMParser() Out"));
    return PVMFSuccess;
}






/////////////////////////////////////////////////////////////////////////////
uint32 PVMFOMXAudioDecNode::GetNumMetadataKeys(char* aQueryKeyString)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::GetNumMetadataKeys() called"));

    uint32 num_entries = 0;

    if (aQueryKeyString == NULL)
    {
        num_entries = iAvailableMetadataKeys.size();
    }
    else
    {
        for (uint32 i = 0; i < iAvailableMetadataKeys.size(); i++)
        {
            if (pv_mime_strcmp(iAvailableMetadataKeys[i].get_cstr(), aQueryKeyString) >= 0)
            {
                num_entries++;
            }
        }
    }
    return num_entries; // Number of elements

}

///////////////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    OSCL_UNUSED_ARG(aObserver);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::setObserver()"));
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::setObserver() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


PVMFStatus PVMFOMXAudioDecNode::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::getParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigGetParametersSync(aIdentifier, aParameters, aNumParamElements, aContext);
}


PVMFStatus PVMFOMXAudioDecNode::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::releaseParameters()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigReleaseParameters(aParameters, aNumElements);
}


void PVMFOMXAudioDecNode::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::createContext()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::createContext() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVMFOMXAudioDecNode::setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext, PvmiKvp* aParameters, int aNumParamElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::setContextParameters()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumParamElements);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::setContextParameters() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVMFOMXAudioDecNode::DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DeleteContext()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::DeleteContext() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVMFOMXAudioDecNode::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::setParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    // Complete the request synchronously
    DoCapConfigSetParameters(aParameters, aNumElements, aRetKVP);
}


PVMFCommandId PVMFOMXAudioDecNode::setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp*& aRetKVP, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::setParametersAsync()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumElements);
    OSCL_UNUSED_ARG(aRetKVP);
    OSCL_UNUSED_ARG(aContext);

    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::setParametersAsync() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
    return 0;
}


uint32 PVMFOMXAudioDecNode::getCapabilityMetric(PvmiMIOSession aSession)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::getCapabilityMetric()"));
    OSCL_UNUSED_ARG(aSession);
    // Not supported so return 0
    return 0;
}


PVMFStatus PVMFOMXAudioDecNode::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::verifyParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigVerifyParameters(aParameters, aNumElements);
}


/////////////////////////////////////////////////////////////////////////////
uint32 PVMFOMXAudioDecNode::GetNumMetadataValues(PVMFMetadataList& aKeyList)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::GetNumMetadataValues() called"));

    uint32 numkeys = aKeyList.size();

    if (numkeys <= 0)
    {
        // Don't do anything
        return 0;
    }

    // Count the number of value entries for the provided key list
    uint32 numvalentries = 0;
    for (uint32 lcv = 0; lcv < numkeys; lcv++)
    {
        if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_CHANNELS_KEY) == 0))
        {
            // Channels
            if (iNumberOfAudioChannels > 0)
            {
                ++numvalentries;
            }
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_SAMPLERATE_KEY) == 0) && (iPCMSamplingRate > 0))
        {
            // Sample rate
            ++numvalentries;
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXAUDIODECMETADATA_CODECINFO_AUDIO_FORMAT_KEY) == 0) &&
                 iInPort != NULL)
        {
            // Format
            if ((((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_LATM) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_MPEG4_AUDIO) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_ADIF) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IF2) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IETF) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMR_IETF_COMBINED) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMRWB_IETF) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_AMRWB_IETF_PAYLOAD) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_MP3) ||
                    (((PVMFOMXAudioDecPort*)iInPort)->iFormat == PVMF_WMA)

               )

            {
                ++numvalentries;
            }
        }
    }

    return numvalentries;
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::GetNodeMetadataKeys(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, uint32 starting_index, int32 max_entries, char* query_key, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNodeCommand::GetNodeMetadataKeys() called"));

    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommand::Construct(aSessionId, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_GETNODEMETADATAKEY, &aKeyList, starting_index, max_entries, query_key, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXAudioDecNode::GetNodeMetadataValues(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 starting_index, int32 max_entries, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNodeCommand::GetNodeMetadataValue() called"));

    PVMFOMXAudioDecNodeCommand cmd;
    cmd.PVMFOMXAudioDecNodeCommand::Construct(aSessionId, PVMFOMXAudioDecNodeCommand::PVOMXAUDIODEC_NODE_CMD_GETNODEMETADATAVALUE, &aKeyList, &aValueList, starting_index, max_entries, aContext);
    return QueueCommandL(cmd);
}

// From PVMFMetadataExtensionInterface
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::ReleaseNodeMetadataKeys(PVMFMetadataList& , uint32 , uint32)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ReleaseNodeMetadataKeys() called"));
    //nothing needed-- there's no dynamic allocation in this node's key list
    return PVMFSuccess;
}

// From PVMFMetadataExtensionInterface
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXAudioDecNode::ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 start, uint32 end)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::ReleaseNodeMetadataValues() called"));

    if (aValueList.size() == 0 || start < 0 || start > end)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::ReleaseNodeMetadataValues() Invalid start/end index"));
        return PVMFErrArgument;
    }

    if (end >= aValueList.size())
    {
        end = aValueList.size() - 1;
    }

    for (uint32 i = start; i <= end; i++)
    {
        if (aValueList[i].key != NULL)
        {
            switch (GetValTypeFromKeyString(aValueList[i].key))
            {
                case PVMI_KVPVALTYPE_CHARPTR:
                    if (aValueList[i].value.pChar_value != NULL)
                    {
                        OSCL_ARRAY_DELETE(aValueList[i].value.pChar_value);
                        aValueList[i].value.pChar_value = NULL;
                    }
                    break;

                case PVMI_KVPVALTYPE_UINT32:
                case PVMI_KVPVALTYPE_UINT8:
                    // No memory to free for these valtypes
                    break;

                default:
                    // Should not get a value that wasn't created from here
                    break;
            }

            OSCL_ARRAY_DELETE(aValueList[i].key);
            aValueList[i].key = NULL;
        }
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// CAPABILITY CONFIG PRIVATE
PVMFStatus PVMFOMXAudioDecNode::DoCapConfigGetParametersSync(PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoCapConfigGetParametersSync() In"));
    OSCL_UNUSED_ARG(aContext);

    return PVMFFailure;
}


PVMFStatus PVMFOMXAudioDecNode::DoCapConfigReleaseParameters(PvmiKvp* aParameters, int aNumElements)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoCapConfigReleaseParameters() Out"));
    return PVMFSuccess;
}


void PVMFOMXAudioDecNode::DoCapConfigSetParameters(PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP)
{


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoCapConfigSetParameters() Out"));
}


PVMFStatus PVMFOMXAudioDecNode::DoCapConfigVerifyParameters(PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoCapConfigVerifyParameters() In"));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoCapConfigVerifyParameters() Out"));
    return PVMFSuccess;
}



PVMFStatus PVMFOMXAudioDecNode::DoGetVideoDecNodeParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoGetVideoDecNodeParameter() In"));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoGetVideoDecNodeParameter() Out"));
    return PVMFSuccess;
}



PVMFStatus PVMFOMXAudioDecNode::DoVerifyAndSetVideoDecNodeParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoVerifyAndSetVideoDecNodeParameter() In"));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXAudioDecNode::DoVerifyAndSetVideoDecNodeParameter() Out"));
    return PVMFSuccess;
}


/////////////////////////////////////////////////////////////////////////////
void PVMFOMXAudioDecNode::LogDiagnostics()
{
    if (iDiagnosticsLogged == false)
    {
        iDiagnosticsLogged = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode - Number of YUV Frames Sent = %d", iSeqNum));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFOMXAudioDecNode - TS of last decoded video frame = %d", iOutTimeStamp));
    }
}

// needed for WMA parameter verification
bool PVMFOMXAudioDecNode::VerifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(num_elements);
    if (pv_mime_strcmp(aParameters->key, PVMF_BITRATE_VALUE_KEY) == 0)
    {
        if (((PVMFOMXAudioDecPort*)iOutPort)->verifyConnectedPortParametersSync(PVMF_BITRATE_VALUE_KEY, &(aParameters->value.uint32_value)) != PVMFSuccess)
        {
            return false;
        }
        return true;
    }
    else if (pv_mime_strcmp(aParameters->key, PVMF_FORMAT_SPECIFIC_INFO_KEY) < 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXAudioDecNode::VerifyParametersSync() - Unsupported Key"));
        OSCL_ASSERT(false);
    }
    return true;
}






