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

#include "pvmf_omx_videodec_node.h"
#include "pvlogger.h"
#include "oscl_error_codes.h"
#include "pvmf_omx_videodec_port.h"
#include "pv_mime_string_utils.h"
#include "oscl_snprintf.h"
#include "pvmf_media_cmd.h"
#include "pvmf_media_msg_format_ids.h"
#include "pvmi_kvp_util.h"
// needed for capability and config
#include "pv_video_config_parser.h"

#ifdef _DEBUG
#include <stdio.h>
#endif

#include "omx_core.h"
#include "pvmf_omx_videodec_callbacks.h"     //used for thin AO in Decoder's callbacks
#include "pv_omxcore.h"
#include "pv_omxmastercore.h"

#if OMX_DEBUG_LOG
#include <utils/Log.h>
#undef LOG_TAG
#define LOG_TAG "OMXD"
#undef PVLOGGER_LOGMSG
#define PVLOGGER_LOGMSG(IL, LOGGER, LEVEL, MESSAGE) JJLOGE MESSAGE
#define JJLOGE(id, ...) LOGE(__VA_ARGS__)
#endif

static const OMX_U32 OMX_SPEC_VERSION = 0x00000101;
#define CONFIG_VERSION_SIZE(param) \
       param.nVersion.nVersion = OMX_SPEC_VERSION; \
       param.nSize = sizeof(param);


#define PVOMXVIDEODEC_EXTRA_YUVBUFFER_POOLNUM 3
#define PVOMXVIDEODEC_MEDIADATA_POOLNUM (PVOMXVIDEODECMAXNUMDPBFRAMESPLUS1 + PVOMXVIDEODEC_EXTRA_YUVBUFFER_POOLNUM)
#define PVOMXVIDEODEC_MEDIADATA_CHUNKSIZE 128


// Node default settings
#define PVOMXVIDEODECNODE_CONFIG_POSTPROCENABLE_DEF false
#define PVOMXVIDEODECNODE_CONFIG_POSTPROCTYPE_DEF 0  // 0 (nopostproc),1(deblock),3(deblock&&dering)
#define PVOMXVIDEODECNODE_CONFIG_DROPFRAMEENABLE_DEF false
#define PVOMXVIDEODECNODE_CONFIG_MIMETYPE_DEF 0
// H263 default settings
#define PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_DEF 40000
#define PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_MIN 20000
#define PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_MAX 120000
#define PVOMXVIDEODECNODE_CONFIG_H263MAXWIDTH_DEF 352
#define PVOMXVIDEODECNODE_CONFIG_H263MAXHEIGHT_DEF 288
#define PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MIN 4
#define PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MAX 352
// M4v default settings
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_DEF 40000
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_MIN 20000
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_MAX 120000
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXWIDTH_DEF 352
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXHEIGHT_DEF 288
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MIN 4
#define PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MAX 352

// AVC default settings
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXBITSTREAMFRAMESIZE_DEF 20000
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXBITSTREAMFRAMESIZE_MIN 20000
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXBITSTREAMFRAMESIZE_MAX 120000
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXWIDTH_DEF 352
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXHEIGHT_DEF 288
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXDIMENSION_MIN 4
#define PVOMXVIDEODECNODE_CONFIG_AVCMAXDIMENSION_MAX 352

/* WMV default settings */
#define PVOMXVIDEODECNODE_CONFIG_WMVMAXWIDTH_DEF 352
#define PVOMXVIDEODECNODE_CONFIG_WMVMAXHEIGHT_DEF 288


#define PVMF_OMXVIDEODEC_NUM_METADATA_VALUES 6

// Constant character strings for metadata keys
static const char PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY[] = "codec-info/video/format";
static const char PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY[] = "codec-info/video/width";
static const char PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY[] = "codec-info/video/height";
static const char PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY[] = "codec-info/video/profile";
static const char PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY[] = "codec-info/video/level";
static const char PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY[] = "codec-info/video/avgbitrate";//(bits per sec)

static const char PVOMXVIDEODECMETADATA_SEMICOLON[] = ";";



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
OMX_ERRORTYPE CallbackEventHandler(OMX_OUT OMX_HANDLETYPE aComponent,
                                   OMX_OUT OMX_PTR aAppData,
                                   OMX_OUT OMX_EVENTTYPE aEvent,
                                   OMX_OUT OMX_U32 aData1,
                                   OMX_OUT OMX_U32 aData2,
                                   OMX_OUT OMX_PTR aEventData)
{

    PVMFOMXVideoDecNode *Node = (PVMFOMXVideoDecNode *) aAppData;

    if (Node->IsComponentMultiThreaded())
    {
        // allocate the memory for the callback event specific data
        //EventHandlerSpecificData* ED = (EventHandlerSpecificData*) oscl_malloc(sizeof (EventHandlerSpecificData));
        EventHandlerSpecificData* ED = (EventHandlerSpecificData*) Node->iThreadSafeHandlerEventHandler->iMemoryPool->allocate(sizeof(EventHandlerSpecificData));

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
OMX_ERRORTYPE CallbackEmptyBufferDone(OMX_OUT OMX_HANDLETYPE aComponent,
                                      OMX_OUT OMX_PTR aAppData,
                                      OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{

    PVMFOMXVideoDecNode *Node = (PVMFOMXVideoDecNode *) aAppData;
    if (Node->IsComponentMultiThreaded())
    {

        // allocate the memory for the callback event specific data
        //EmptyBufferDoneSpecificData* ED = (EmptyBufferDoneSpecificData*) oscl_malloc(sizeof (EmptyBufferDoneSpecificData));
        EmptyBufferDoneSpecificData* ED = (EmptyBufferDoneSpecificData*) Node->iThreadSafeHandlerEmptyBufferDone->iMemoryPool->allocate(sizeof(EmptyBufferDoneSpecificData));

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
OMX_ERRORTYPE CallbackFillBufferDone(OMX_OUT OMX_HANDLETYPE aComponent,
                                     OMX_OUT OMX_PTR aAppData,
                                     OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{
    PVMFOMXVideoDecNode *Node = (PVMFOMXVideoDecNode *) aAppData;
    if (Node->IsComponentMultiThreaded())
    {

        // allocate the memory for the callback event specific data
        //FillBufferDoneSpecificData* ED = (FillBufferDoneSpecificData*) oscl_malloc(sizeof (FillBufferDoneSpecificData));
        FillBufferDoneSpecificData* ED = (FillBufferDoneSpecificData*) Node->iThreadSafeHandlerFillBufferDone->iMemoryPool->allocate(sizeof(FillBufferDoneSpecificData));

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
OsclReturnCode PVMFOMXVideoDecNode::ProcessCallbackEventHandler_MultiThreaded(OsclAny* P)
{

    // re-cast the pointer

    EventHandlerSpecificData* ED = (EventHandlerSpecificData*) P;

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
OsclReturnCode PVMFOMXVideoDecNode::ProcessCallbackEmptyBufferDone_MultiThreaded(OsclAny* P)
{


    // re-cast the pointer
    EmptyBufferDoneSpecificData* ED = (EmptyBufferDoneSpecificData*) P;

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
OsclReturnCode PVMFOMXVideoDecNode::ProcessCallbackFillBufferDone_MultiThreaded(OsclAny* P)
{

    // re-cast the pointer
    FillBufferDoneSpecificData* ED = (FillBufferDoneSpecificData*) P;

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
PVMFOMXVideoDecNode::~PVMFOMXVideoDecNode()
{
    LogDiagnostics();

    //Clearup decoder
    DeleteOMXVideoDecoder();

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
PVMFStatus PVMFOMXVideoDecNode::ThreadLogon()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXVideoDecNode:ThreadLogon"));

    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
            {
                AddToScheduler();
                iIsAdded = true;
            }
            iLogger = PVLogger::GetLoggerObject("PVMFOMXVideoDecNode");
            iRunlLogger = PVLogger::GetLoggerObject("Run.PVMFOMXVideoDecNode");
            iDataPathLogger = PVLogger::GetLoggerObject("datapath");
            iClockLogger = PVLogger::GetLoggerObject("clock");
            iDiagnosticsLogger = PVLogger::GetLoggerObject("pvplayerdiagnostics.decnode.OMXVideoDecnode");

            SetState(EPVMFNodeIdle);
            return PVMFSuccess;
        default:
            return PVMFErrInvalidState;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Remove AO from the scheduler
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::ThreadLogoff()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFOMXVideoDecNode:ThreadLogoff"));

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

        default:
            return PVMFErrInvalidState;
    }
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::GetCapability() called"));

    aNodeCapability = iCapability;
    return PVMFSuccess;
}

/////////////////////////////////////////////////////////////////////////////
PVMFPortIter* PVMFOMXVideoDecNode::GetPorts(const PVMFPortFilter* aFilter)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::GetPorts() called"));

    OSCL_UNUSED_ARG(aFilter);

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::QueueCommandL(PVMFOMXVideoDecNodeCommand& aCmd)
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
PVMFCommandId PVMFOMXVideoDecNode::QueryUUID(PVMFSessionId s, const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, PVMFOMXVideoDecNodeAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::QueryUUID() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_QUERYUUID, aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::QueryInterface(PVMFSessionId s, const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::QueryInterface() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_QUERYINTERFACE, aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::RequestPort(PVMFSessionId s, int32 aPortTag, const PvmfMimeString* /* aPortConfig */, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::RequestPort() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_REQUESTPORT, aPortTag, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::ReleasePort(PVMFSessionId s, PVMFPortInterface& aPort, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ReleasePort() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Init(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Init() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_INIT, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Prepare(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Prepare() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_PREPARE, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Start(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Start() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_START, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Stop(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Stop() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_STOP, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Flush(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Flush() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_FLUSH, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Pause(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Pause() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_PAUSE, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::Reset(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Reset() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::CancelAllCommands(PVMFSessionId s, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::CancelAllCommands() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_CANCELALL, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::CancelCommand(PVMFSessionId s, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::CancelCommand() called"));
    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommandBase::Construct(s, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_CANCELCMD, aCmdId, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::SetDecoderNodeConfiguration(PVMFOMXVideoDecNodeConfig& aNodeConfig)
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
PVMFOMXVideoDecNode::PVMFOMXVideoDecNode(int32 aPriority) :
        OsclActiveObject(aPriority, "PVMFOMXVideoDecNode"),
        iInPort(NULL),
        iOutPort(NULL),
        iOutBufMemoryPool(NULL),
        iMediaDataMemPool(NULL),
        iOMXComponentOutputBufferSize(0),
        iOutputAllocSize(0),
        iProcessingState(EPVMFOMXVideoDecNodeProcessingState_Idle),
        iOMXVideoDecoder(NULL),
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
        iH263MaxBitstreamFrameSize(PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_DEF),
        iH263MaxWidth(PVOMXVIDEODECNODE_CONFIG_H263MAXWIDTH_DEF),
        iH263MaxHeight(PVOMXVIDEODECNODE_CONFIG_H263MAXHEIGHT_DEF),
        iM4VMaxBitstreamFrameSize(PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_DEF),
        iM4VMaxWidth(PVOMXVIDEODECNODE_CONFIG_M4VMAXWIDTH_DEF),
        iM4VMaxHeight(PVOMXVIDEODECNODE_CONFIG_M4VMAXHEIGHT_DEF),
        iNewWidth(0),
        iNewHeight(0),
        iAvgBitrateValue(0),
        iResetInProgress(false),
        iResetMsgSent(false)
{
    iInterfaceState = EPVMFNodeCreated;

    iNodeConfig.iPostProcessingEnable = PVOMXVIDEODECNODE_CONFIG_POSTPROCENABLE_DEF;
    iNodeConfig.iPostProcessingMode = PVOMXVIDEODECNODE_CONFIG_POSTPROCTYPE_DEF;
    iNodeConfig.iDropFrame = PVOMXVIDEODECNODE_CONFIG_DROPFRAMEENABLE_DEF;
    iNodeConfig.iMimeType = PVOMXVIDEODECNODE_CONFIG_MIMETYPE_DEF;


    int32 err;
    OSCL_TRY(err,

             //Create the input command queue.  Use a reserve to avoid lots of
             //dynamic memory allocation.
             iInputCommands.Construct(PVMF_OMXVIDEODEC_NODE_COMMAND_ID_START, PVMF_OMXVIDEODEC_NODE_COMMAND_VECTOR_RESERVE);

             //Create the "current command" queue.  It will only contain one
             //command at a time, so use a reserve of 1.
             iCurrentCommand.Construct(0, 1);

             //Set the node capability data.
             //This node can support an unlimited number of ports.
             iCapability.iCanSupportMultipleInputPorts = false;
             iCapability.iCanSupportMultipleOutputPorts = false;
             iCapability.iHasMaxNumberOfPorts = true;
             iCapability.iMaxNumberOfPorts = 2;
             iCapability.iInputFormatCapability.push_back(PVMF_H264_MP4);
             iCapability.iInputFormatCapability.push_back(PVMF_H264_RAW);
             iCapability.iInputFormatCapability.push_back(PVMF_H264);
             iCapability.iInputFormatCapability.push_back(PVMF_M4V);
             iCapability.iInputFormatCapability.push_back(PVMF_H263);
             iCapability.iInputFormatCapability.push_back(PVMF_WMV);
             iCapability.iOutputFormatCapability.push_back(PVMF_YUV420);

             iAvailableMetadataKeys.reserve(PVMF_OMXVIDEODEC_NUM_METADATA_VALUES);
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

    // init state of component
    iCurrentDecoderState = OMX_StateInvalid;

    iOutTimeStamp = 0;

    // counts output frames (for logging)
    iOutFrameReceived = 0;
    iOutFrameSent = 0;
    iInFrameSent = 0;
    iInFrameReceived = 0;
    // reset repositioning related flags
    iIsRepositioningRequestSentToComponent = false;
    iIsRepositionDoneReceivedFromComponent = false;
    iIsOutputPortFlushed = false;
    iIsInputPortFlushed = false;

    iInputBufferUnderConstruction = NULL; // for partial frame assembly
    iFirstPieceOfPartialFrame = true;
    iObtainNewInputBuffer = true;
    iFirstDataMsgAfterBOS = true;
    iKeepDroppingMsgsUntilMarkerBit = false;

    // need to init this allocator since verifyParameterSync (using the buffers) may be called through
    // port interface before anything else happens. This seems a good place to do it
    OSCL_TRY(err, iYuvFsiFragmentAlloc.size(PVOMXVIDEODEC_MEDIADATA_POOLNUM, sizeof(PVMFYuvFormatSpecificInfo0)));

    OSCL_TRY(err, iPrivateDataFsiFragmentAlloc.size(PVOMXVIDEODEC_MEDIADATA_POOLNUM, sizeof(OsclAny *)));



}

/////////////////////////////////////////////////////////////////////////////
// Local Run Routine
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::Run()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Run() In"));

    // if reset is in progress, call DoReset again until Reset Msg is sent
    if ((iResetInProgress == true) &&
            (iResetMsgSent == false) &&
            (iCurrentCommand.size() > 0) &&
            (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
       )
    {
        DoReset(iCurrentCommand.front());
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Calling DoReset"));
        return; // don't do anything else
    }
    //Check for NODE commands...
    if (!iInputCommands.empty())
    {
        if (ProcessCommand(iInputCommands.front()))
        {
            if (iInterfaceState != EPVMFNodeCreated
                    && (!iInputCommands.empty() || (iInPort && (iInPort->IncomingMsgQueueSize() > 0)) ||
                        (iDataIn.GetRep() != NULL)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - rescheduling after process command"));
                RunIfNotReady();
            }
            return;
        }

        if (!iInputCommands.empty())
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - rescheduling to process more commands"));
            RunIfNotReady();
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Input commands empty"));
    }

    if (((iCurrentCommand.size() == 0) && (iInterfaceState != EPVMFNodeStarted)) ||
            ((iCurrentCommand.size() > 0) && (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_START) && (iInterfaceState != EPVMFNodeStarted)))
    {
        // rescheduling because of input data will be handled in Command Processing Part
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Node not in Started state yet"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Outgoing Port Busy, cannot send more msgs"));
                break;
            }
        }
    }
    int loopCount = 0;
#if (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_REL)
    uint32 startticks = OsclTickCount::TickCount();
    uint32 starttime = OsclTickCount::TicksToMsec(startticks);
#endif
    do // Try to consume all the data from the Input port
    {
        // Process port activity if there is no input data that is being processed
        // Do not accept any input if EOS needs to be sent out
        if (iInPort && (iInPort->IncomingMsgQueueSize() > 0) && (iDataIn.GetRep() == NULL) && !iEndOfDataReached)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Getting more input"));
            if (!ProcessIncomingMsg(iInPort))
            {
                //Re-schedule to come back.
                RunIfNotReady();
                return;
            }
        }

        if (iSendBOS)
        {

            // this routine may be re-entered multiple times in multiple Run's before the component goes through flushing all ports
            if (!HandleRepositioning())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Repositioning not done yet"));

                return;
            }

            SendBeginOfMediaStreamCommand();
        }

        // If in init or ready to decode state, process data in the input port if there is input available and input buffers are present
        // (note: at EOS, iDataIn will not be available)
        if ((iDataIn.GetRep() != NULL) ||
                ((iNumOutstandingOutputBuffers < iNumOutputBuffers) &&
                 (iProcessingState == EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode) &&
                 (iResetMsgSent == false)) ||
                ((iDynamicReconfigInProgress == true) && (iResetMsgSent == false))
           )
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoDecNode::Run() - Calling HandleProcessingState"));

            // input data is available, that means there is video data to be decoded
            if (HandleProcessingState() != PVMFSuccess)
            {
                // If HandleProcessingState does not return Success, we must wait for an event
                // no point in  rescheduling
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXVideoDecNode::Run() - HandleProcessingState did not return Success"));
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
#if (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_REL)
    uint32 endticks = OsclTickCount::TickCount();
    uint32 endtime = OsclTickCount::TicksToMsec(endticks);
    uint32 timeinloop = (endtime - starttime);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iRunlLogger, PVLOGMSG_INFO,
                    (0, "PVMFOMXVideoDecNode::Run() - LoopCount = %d, Time spent in loop(in ms) = %d, iNumOutstandingInputBuffers = %d, iNumOutstandingOutputBuffers = %d ",
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
                            (0, "PVMFOMXVideoDecNode::Run() - Sending EOS marked buffer To Component "));

            iIsEOSSentToComponent = true;


            // if the component is not yet initialized or if it's in the middle of port reconfig,
            // don't send EOS buffer to component. It does not care. Just set the flag as if we received
            // EOS from the component to enable sending EOS downstream
            if (iProcessingState != EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode)
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
                            (0, "PVMFOMXVideoDecNode::Run() - Received EOS from component, Sending EOS msg downstream "));

            if (iOutPort && iOutPort->IsOutgoingQueueBusy())
            {
                // note: we already tried to empty the outgoing q. If it's still busy,
                // it means that output port is busy. Just return and wait for the port to become free.
                // this will wake up the node and it will send out a msg from the q etc.
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXVideoDecNode::Run() - - EOS cannot be sent downstream, outgoing queue busy - wait"));
                return;
            }

            if (SendEndOfTrackCommand()) // this will only q the EOS
            {
                // EOS send downstream OK, so reset the flag
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXVideoDecNode::Run() - EOS was queued to be sent downstream"));

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
            (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_FLUSH) &&
            (iInPort->IncomingMsgQueueSize() == 0) &&
            (iOutPort->OutgoingMsgQueueSize() == 0) &&
            (iDataIn.GetRep() == NULL))
    {
        //flush command is complited
        //Debug check-- all the port queues should be empty at this point.

        OSCL_ASSERT(iInPort->IncomingMsgQueueSize() == 0 && iOutPort->OutgoingMsgQueueSize() == 0);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::Run() - Flush pending"));
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

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::Run() Out"));
}

/////////////////////////////////////////////////////////////////////////////
// This routine will dispatch recived commands
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::ProcessCommand(PVMFOMXVideoDecNodeCommand& aCmd)
{
    //normally this node will not start processing one command
    //until the prior one is finished.  However, a hi priority
    //command such as Cancel must be able to interrupt a command
    //in progress.
    if (!iCurrentCommand.empty() && !aCmd.hipri())
        return false;

    switch (aCmd.iCmd)
    {
        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_QUERYUUID:
            DoQueryUuid(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_REQUESTPORT:
            DoRequestPort(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RELEASEPORT:
            DoReleasePort(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_INIT:
            DoInit(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_PREPARE:
            DoPrepare(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_START:
            DoStart(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_STOP:
            DoStop(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_FLUSH:
            DoFlush(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_PAUSE:
            DoPause(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET:
            DoReset(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_CANCELCMD:
            DoCancelCommand(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_CANCELALL:
            DoCancelAllCommands(aCmd);
            break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_GETNODEMETADATAKEY:
        {
            PVMFStatus retval = DoGetNodeMetadataKey(aCmd);
            CommandComplete(iInputCommands, aCmd, retval);
        }
        break;

        case PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_GETNODEMETADATAVALUE:
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
bool PVMFOMXVideoDecNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    //Called by the AO to process one buffer off the port's
    //incoming data queue.  This routine will dequeue and
    //dispatch the data.

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "0x%x PVMFOMXVideoDecNode::ProcessIncomingMsg: aPort=0x%x", this, aPort));

    PVMFStatus status = PVMFFailure;


#ifdef SIMULATE_BOS

    if (((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed == 6))
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
        printf("PVMFOMXVideoDecNode::ProcessIncomingMsg() SIMULATED BOS\n");
#endif
        ((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed++;
        return true;

    }
#endif
#ifdef SIMULATE_PREMATURE_EOS
    if (((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed == 5)
    {
        PVMFSharedMediaCmdPtr EOSCmdPtr = PVMFMediaCmd::createMediaCmd();

        // Set the format ID to EOS
        EOSCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

        // Set the timestamp
        EOSCmdPtr->setTimestamp(200);

        // Convert to media message and send it out
        PVMFSharedMediaMsgPtr mediaMsgOut;
        convertToPVMFMediaCmdMsg(mediaMsgOut, EOSCmdPtr);

        ((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed++;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::ProcessIncomingMsg: SIMULATED EOS"));
#ifdef _DEBUG
        printf("PVMFOMXVideoDecNode::ProcessIncomingMsg() SIMULATED EOS\n");
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
                    (0, "0x%x PVMFOMXVideoDecNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed", this));
        return false;
    }

    if (msg->getFormatID() == PVMF_MEDIA_CMD_BOS_FORMAT_ID)
    {
        //store the stream id and time stamp of bos message
        iStreamID = msg->getStreamID();
        iBOSTimestamp = msg->getTimestamp();
        iSendBOS = true;

        // if new BOS arrives, and
        //if we're in the middle of a partial frame assembly
        // abandon it and start fresh
        if (iObtainNewInputBuffer == false)
        {
            if (iInputBufferUnderConstruction != NULL)
            {
                if (iInBufMemoryPool != NULL)
                {
                    iInBufMemoryPool->deallocate((OsclAny *)iInputBufferUnderConstruction);
                }
                iInputBufferUnderConstruction = NULL;
            }
            iObtainNewInputBuffer = true;

        }

        // needed to init the sequence numbers and timestamp for partial frame assembly
        iFirstDataMsgAfterBOS = true;
        iKeepDroppingMsgsUntilMarkerBit = false;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::ProcessIncomingMsg: Received BOS stream %d, timestamp %d", iStreamID, iBOSTimestamp));
        ((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed++;
        return true;
    }
    else if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
    {
        // Set EOS flag
        iEndOfDataReached = true;
        // Save the timestamp for the EOS cmd
        iEndOfDataTimestamp = msg->getTimestamp();

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::ProcessIncomingMsg: Received EOS"));

        ((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed++;
        return true; // do not do conversion into media data, just set the flag and leave
    }

    convertToPVMFMediaData(iDataIn, msg);


    iCurrFragNum = 0; // for new message, reset the fragment counter
    iIsNewDataFragment = true;

    ((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed++;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ProcessIncomingMsg() Received %d frames", ((PVMFOMXVideoDecPort*)aPort)->iNumFramesConsumed));

    //return true if we processed an activity...
    return true;
}

/////////////////////////////////////////////////////////////////////////////
// This routine will process outgoing message by sending it into output the port
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::ProcessOutgoingMsg(PVMFPortInterface* aPort)
{
    //Called by the AO to process one message off the outgoing
    //message queue for the given port.  This routine will
    //try to send the data to the connected port.

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "0x%x PVMFOMXVideoDecNode::ProcessOutgoingMsg: aPort=0x%x", this, aPort));

    PVMFStatus status = aPort->Send();
    if (status == PVMFErrBusy)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "0x%x PVMFOMXVideoDecNode::ProcessOutgoingMsg: Connected port goes into busy state", this));
    }

    //Report any unexpected failure in port processing...
    //(the InvalidState error happens when port input is suspended,
    //so don't report it.)
    if (status != PVMFErrBusy
            && status != PVMFSuccess
            && status != PVMFErrInvalidState)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "0x%x PVMFOMXVideoDecNode::Run: Error - ProcessPortActivity failed. port=0x%x, type=%d",
                         this, iOutPort, PVMF_PORT_ACTIVITY_OUTGOING_MSG));
        ReportErrorEvent(PVMFErrPortProcessing);
    }

    //return true if we processed an activity...
    return (status != PVMFErrBusy);
}

/////////////////////////////////////////////////////////////////////////////
// This routine will process received data usign State Machine
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::HandleProcessingState()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::HandleProcessingState() In"));

    PVMFStatus status = PVMFSuccess;

    switch (iProcessingState)
    {
        case EPVMFOMXVideoDecNodeProcessingState_InitDecoder:
        {
            // do init only if input data is available
            if (iDataIn.GetRep() != NULL)
            {
                if (!InitDecoder(iDataIn))
                {
                    // Decoder initialization failed. Fatal error
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Decoder initialization failed"));
                    ReportErrorEvent(PVMFErrResourceConfiguration);
                    ChangeNodeState(EPVMFNodeError);
                    break;
                }

                iProcessingState = EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode;
                // spin once to send output buffers
                RunIfNotReady();
                status = PVMFSuccess; // allow rescheduling
            }
            break;
        }

        case EPVMFOMXVideoDecNodeProcessingState_WaitForInitCompletion:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() WaitForInitCompletion -> wait for config buffer to return"));


            status = PVMFErrNoMemory; // prevent rescheduling
            break;
        }
        // The FOLLOWING 4 states handle Dynamic Port Reconfiguration
        case EPVMFOMXVideoDecNodeProcessingState_PortReconfig:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Sending Port Disable Command"));

            // port reconfiguration is required. Only one port at a time is disabled and then re-enabled after buffer resizing
            OMX_SendCommand(iOMXVideoDecoder, OMX_CommandPortDisable, iPortIndexForDynamicReconfig, NULL);
            // the port will now start returning outstanding buffers
            // set the flag to prevent output from going downstream (in case of output port being reconfigd)
            // set the flag to prevent input from being saved and returned to component (in case of input port being reconfigd)
            // set the state to wait for port saying it is disabled
            if (iPortIndexForDynamicReconfig == iOutputPortIndex)
            {
                iDoNotSendOutputBuffersDownstreamFlag = true;
            }
            else if (iPortIndexForDynamicReconfig == iInputPortIndex)
            {
                iDoNotSaveInputBuffersFlag = true;

            }
            iProcessingState = EPVMFOMXVideoDecNodeProcessingState_WaitForBufferReturn;


            // fall through to the next case to check if all buffers are already back
        }

        case EPVMFOMXVideoDecNodeProcessingState_WaitForBufferReturn:
        {
            // as buffers are coming back, Run may be called, wait until all buffers are back, then Free them all

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> WaitForBufferReturn "));
            // check if it's output port being reconfigured
            if (iPortIndexForDynamicReconfig == iOutputPortIndex)
            {
                // if all buffers have returned, free them
                if (iNumOutstandingOutputBuffers == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> all output buffers are back, free them"));
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
                                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Cannot free output buffers "));

                            SetState(EPVMFNodeError);
                            ReportErrorEvent(PVMFErrNoMemory);
                            return PVMFErrNoMemory;
                        }
                    }
                    // if the callback (that port is disabled) has not arrived yet, wait for it
                    // if it has arrived, it will set the state to PortReEnable
                    if (iProcessingState != EPVMFOMXVideoDecNodeProcessingState_PortReEnable)
                        iProcessingState = EPVMFOMXVideoDecNodeProcessingState_WaitForPortDisable;

                    status = PVMFSuccess; // allow rescheduling of the node potentially
                }
                else
                    status = PVMFErrNoMemory; // must wait for buffers to come back. No point in automatic rescheduling
                // but each buffer will reschedule the node when it comes in
            }
            else
            { // this is input port

                // if all buffers have returned, free them
                if (iNumOutstandingInputBuffers == 0)
                {

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> all input buffers are back, free them"));
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
                                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Cannot free input buffers "));

                            SetState(EPVMFNodeError);
                            ReportErrorEvent(PVMFErrNoMemory);
                            return PVMFErrNoMemory;

                        }
                    }
                    // if the callback (that port is disabled) has not arrived yet, wait for it
                    // if it has arrived, it will set the state to PortReEnable
                    if (iProcessingState != EPVMFOMXVideoDecNodeProcessingState_PortReEnable)
                        iProcessingState = EPVMFOMXVideoDecNodeProcessingState_WaitForPortDisable;

                    status = PVMFSuccess; // allow rescheduling of the node
                }
                else
                    status = PVMFErrNoMemory; // must wait for buffers to come back. No point in automatic
                // rescheduling. Each buffer will reschedule the node
                // when it comes in
            }


            // the state will be changed to PortReEnable once we get confirmation that Port was actually disabled
            break;
        }

        case EPVMFOMXVideoDecNodeProcessingState_WaitForPortDisable:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> wait for port disable callback"));
            // do nothing. Just wait for the port to become disabled (we'll get event from component, which will
            // transition the state to PortReEnable
            status = PVMFErrNoMemory; // prevent Rescheduling the node
            break;
        }

        case EPVMFOMXVideoDecNodeProcessingState_PortReEnable:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Sending reenable port command"));
            // set the port index so that we get parameters for the proper port
            iParamPort.nPortIndex = iPortIndexForDynamicReconfig;

            // get new parameters of the port
            OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);

            // send command for port re-enabling (for this to happen, we must first recreate the buffers)
            OMX_SendCommand(iOMXVideoDecoder, OMX_CommandPortEnable, iPortIndexForDynamicReconfig, NULL);

            // is this output port?
            if (iPortIndexForDynamicReconfig == iOutputPortIndex)
            {
                iOMXComponentOutputBufferSize = ((iParamPort.format.video.nFrameWidth + 15) & (~15)) * ((iParamPort.format.video.nFrameHeight + 15) & (~15)) * 3 / 2;

                // check the new buffer size
                if (iInPort)
                {
                    switch (((PVMFOMXVideoDecPort*)iInPort)->iFormat)
                    {
                        case PVMF_H264:
                        case PVMF_H264_MP4:
                        case PVMF_H264_RAW:
                        case PVMF_M4V:
                        case PVMF_H263:
                            iOMXComponentOutputBufferSize = ((iParamPort.format.video.nFrameWidth + 15) & (~15)) * ((iParamPort.format.video.nFrameHeight + 15) & (~15)) * 3 / 2;
                            break;
                        case PVMF_WMV: // This is a requirement for the WMV decoder that we have currently
                            iOMXComponentOutputBufferSize = ((iParamPort.format.video.nFrameWidth + 3) & (~3)) * (iParamPort.format.video.nFrameHeight) * 3 / 2;
                            break;
                        default:
                            OSCL_ASSERT(false);
                            break;
                    }
                }
                // set the new width / height
                iYUVWidth =  iParamPort.format.video.nFrameWidth;
                iYUVHeight = iParamPort.format.video.nFrameHeight;

                if (iOMXComponentOutputBufferSize < iParamPort.nBufferSize)
                    iOMXComponentOutputBufferSize = iParamPort.nBufferSize;

                // do we need to increase the number of buffers?
                if (iNumOutputBuffers < iParamPort.nBufferCountMin)
                    iNumOutputBuffers = iParamPort.nBufferCountMin;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::HandleProcessingState() new output buffers %d, size %d", iNumOutputBuffers, iOMXComponentOutputBufferSize));

                /* Allocate output buffers */
                if (!CreateOutMemPool(iNumOutputBuffers))
                {

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Cannot allocate output buffers "));

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
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Cannot provide output buffers to component"));

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
                                (0, "PVMFOMXVideoDecNode::HandleProcessingState() new buffers %d, size %d", iNumInputBuffers, iOMXComponentInputBufferSize));

                /* Allocate input buffers */
                if (!CreateInputMemPool(iNumInputBuffers))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Cannot allocate new input buffers to component"));

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
                                    (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> Cannot provide new input buffers to component"));

                    SetState(EPVMFNodeError);
                    ReportErrorEvent(PVMFErrNoMemory);
                    return PVMFErrNoMemory;

                }
                // do not drop partially consumed input
                iDoNotSaveInputBuffersFlag = false;


            }

            // if the callback that the port was re-enabled has not arrived yet, wait for it
            // if it has arrived, it will set the state to either PortReconfig or to ReadyToDecode
            if (iProcessingState != EPVMFOMXVideoDecNodeProcessingState_PortReconfig &&
                    iProcessingState != EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode)
                iProcessingState = EPVMFOMXVideoDecNodeProcessingState_WaitForPortEnable;

            status = PVMFSuccess; // allow rescheduling of the node
            break;
        }

        case EPVMFOMXVideoDecNodeProcessingState_WaitForPortEnable:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Port Reconfiguration -> wait for port enable callback"));
            // do nothing. Just wait for the port to become enabled (we'll get event from component, which will
            // transition the state to ReadyToDecode
            status = PVMFErrNoMemory; // prevent ReScheduling
            break;
        }

        // NORMAL DATA FLOW STATE:
        case EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Ready To Decode start"));
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
                                (0, "PVMFOMXVideoDecNode::HandleProcessingState() Sending previous - partially consumed input back to the OMX component"));

                OMX_EmptyThisBuffer(iOMXVideoDecoder, iInputBufferToResendToComponent);
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
        case EPVMFOMXVideoDecNodeProcessingState_Stopping:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Stopping -> wait for Component to move from Executing->Idle"));

            status = PVMFErrNoMemory; // prevent rescheduling
            break;
        }

        case EPVMFOMXVideoDecNodeProcessingState_Pausing:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleProcessingState() Pausing -> wait for Component to move from Executing->Pause"));


            status = PVMFErrNoMemory; // prevent rescheduling
            break;
        }

        case EPVMFOMXVideoDecNodeProcessingState_WaitForOutgoingQueue:
            status = PVMFErrNoMemory;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::HandleProcessingState() Do nothing since waiting for output port queue to become available"));
            break;

        default:
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::HandleProcessingState() Out"));

    return status;

}
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::SendOutputBufferToOMXComponent()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendOutputBufferToOMXComponent() In"));


    OutputBufCtrlStruct *output_buf = NULL;
    int32 errcode = 0;

    // try to get output buffer header
    OSCL_TRY(errcode, output_buf = (OutputBufCtrlStruct *) iOutBufMemoryPool->allocate(iOutputAllocSize));
    if (errcode != 0)
    {
        if (errcode == OsclErrNoResources)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                            PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::SendOutputBufferToOMXComponent() No more output buffers in the mempool"));

            iOutBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny *) iOutBufMemoryPool); // To signal when next deallocate() is called on mempool

            return false;
        }
        else
        {
            // Memory allocation for the pool failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::SendOutputBufferToOMXComponent() Output mempool error"));


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
    output_buf->pBufHdr->nFlags = 0; // Clear flags
    OMX_FillThisBuffer(iOMXVideoDecoder, output_buf->pBufHdr);



    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendOutputBufferToOMXComponent() Out"));

    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::NegotiateComponentParameters()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() In"));

    OMX_ERRORTYPE Err;
    // first get the number of ports and port indices
    OMX_PORT_PARAM_TYPE VideoPortParameters;
    uint32 NumPorts;
    uint32 ii;

    // get starting number
    CONFIG_VERSION_SIZE(VideoPortParameters);
    Err = OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamVideoInit, &VideoPortParameters);
    NumPorts = VideoPortParameters.nPorts; // must be at least 2 of them (in&out)

    if (Err != OMX_ErrorNone || NumPorts < 2)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() There is insuffucient (%d) ports", NumPorts));
        return false;
    }


    // loop through video ports starting from the starting index to find index of the first input port
    for (ii = VideoPortParameters.nStartPortNumber ;ii < VideoPortParameters.nStartPortNumber + NumPorts; ii++)
    {
        // get port parameters, and determine if it is input or output
        // if there are more than 2 ports, the first one we encounter that has input direction is picked


        //port
        CONFIG_VERSION_SIZE(iParamPort);

        iParamPort.nPortIndex = ii; // iInputPortIndex; //OMF_MC_H264D_PORT_INDEX_OF_STREAM;
        Err = OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);

        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem negotiating with port %d ", ii));

            return false;
        }

        if (iParamPort.eDir == OMX_DirInput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Found Input port index %d ", ii));

            iInputPortIndex = ii;
            break;
        }
    }
    if (ii == VideoPortParameters.nStartPortNumber + NumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Cannot find any input port "));
        return false;
    }


    // loop through video ports starting from the starting index to find index of the first output port
    for (ii = VideoPortParameters.nStartPortNumber ;ii < VideoPortParameters.nStartPortNumber + NumPorts; ii++)
    {
        // get port parameters, and determine if it is input or output
        // if there are more than 2 ports, the first one we encounter that has output direction is picked

        CONFIG_VERSION_SIZE(iParamPort);

        //port
        iParamPort.nPortIndex = ii;
        Err = OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);

        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem negotiating with port %d ", ii));

            return false;
        }

        if (iParamPort.eDir == OMX_DirOutput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Found Output port index %d ", ii));

            iOutputPortIndex = ii;
            break;
        }
    }
    if (ii == VideoPortParameters.nStartPortNumber + NumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Cannot find any output port "));
        return false;
    }

    CONFIG_VERSION_SIZE(iParamPort);
    //Input port
    iParamPort.nPortIndex = iInputPortIndex;
    Err = OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem negotiating with input port %d ", iInputPortIndex));
        return false;
    }

    // preset the number of input buffers
    //iNumInputBuffers = NUMBER_INPUT_BUFFER;
    iNumInputBuffers = iParamPort.nBufferCountActual;

    // do we need to increase the number of buffers?
    if (iNumInputBuffers < iParamPort.nBufferCountMin)
        iNumInputBuffers = iParamPort.nBufferCountMin;
    iOMXComponentInputBufferSize = iParamPort.nBufferSize;

    iParamPort.nBufferCountActual = iNumInputBuffers;

    // set the number of actual input buffers
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Inport buffers %d,size %d", iNumInputBuffers, iOMXComponentInputBufferSize));

    pvVideoConfigParserInputs aInputs;
    pvVideoConfigParserOutputs aOutputs;

    aInputs.inPtr = (uint8*)((PVMFOMXVideoDecPort*)iInPort)->iTrackConfig;
    aInputs.inBytes = (int32)((PVMFOMXVideoDecPort*)iInPort)->iTrackConfigSize;
    aInputs.iMimeType = ((PVMFOMXVideoDecPort*)iInPort)->iFormat;

    int16 status;
    status = pv_video_config_parser(&aInputs, &aOutputs);
    if (status != 0)
    {
        return false;
    }

// before setting the buffer size, get track config from the port, and feed it to the config parser and then  port settings of
// the OMX component
// set the width/height on INPUT port parameters (this may change during port reconfig)
    if ((aOutputs.width != 0) && (aOutputs.height != 0))
    {
        iParamPort.format.video.nFrameWidth = aOutputs.width;
        iParamPort.format.video.nFrameHeight = aOutputs.height;
    }

    Err = OMX_SetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem setting parameters in input port %d ", iInputPortIndex));
        return false;
    }

    //Port 1 for output port
    CONFIG_VERSION_SIZE(iParamPort);

    iParamPort.nPortIndex = iOutputPortIndex;
    Err = OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem negotiating with output port %d ", iOutputPortIndex));
        return false;
    }

    iNumOutputBuffers = iParamPort.nBufferCountActual;
    if (iNumOutputBuffers > NUMBER_OUTPUT_BUFFER)
        iNumOutputBuffers = NUMBER_OUTPUT_BUFFER;

    // check if params are OK. In case of H263, width/height cannot be obtained until
    // 1st frame is decoded, so read them from the output port.
    // otherwise, used Width/Height from the config parser utility
    if ((aOutputs.width != 0) && (aOutputs.height != 0) && iInPort && ((PVMFOMXVideoDecPort*)iInPort)->iFormat != PVMF_H263)
    {
        iYUVWidth  = aOutputs.width;
        iYUVHeight = aOutputs.height;
    }
    else
    {
        iYUVWidth =  iParamPort.format.video.nFrameWidth;
        iYUVHeight = iParamPort.format.video.nFrameHeight;
    }

    iOMXComponentOutputBufferSize = iParamPort.nBufferSize;
    if (iNumOutputBuffers < iParamPort.nBufferCountMin)
        iNumOutputBuffers = iParamPort.nBufferCountMin;

    iParamPort.nBufferCountActual = iNumOutputBuffers;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Outport buffers %d,size %d", iNumOutputBuffers, iOMXComponentOutputBufferSize));

    Err = OMX_SetParameter(iOMXVideoDecoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem setting parameters in output port %d ", iOutputPortIndex));
        return false;
    }


// Get video color format
    OMX_VIDEO_PARAM_PORTFORMATTYPE VideoPortFormat;
    // init to unknown
    iOMXVideoColorFormat = OMX_COLOR_FormatUnused;
    CONFIG_VERSION_SIZE(VideoPortFormat);
    VideoPortFormat.nPortIndex = iOutputPortIndex;

    VideoPortFormat.nIndex = 0; // read the preferred format - first

// doing this in a while loop while incrementing nIndex will get all supported formats
// until component says OMX_ErrorNoMore
// For now, we just use the preferred one (with nIndex=0) assuming it is supported at MIO
    Err = OMX_GetParameter(iOMXVideoDecoder, OMX_IndexParamVideoPortFormat, &VideoPortFormat);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem getting video port format"));
        return false;
    }
    // check if color format is valid
    if (VideoPortFormat.eCompressionFormat == OMX_VIDEO_CodingUnused)
    {
        // color format is valid, so read it
        iOMXVideoColorFormat = VideoPortFormat.eColorFormat;


        // Now set the format to confirm parameters
        CONFIG_VERSION_SIZE(VideoPortFormat);
        Err = OMX_SetParameter(iOMXVideoDecoder, OMX_IndexParamVideoPortFormat, &VideoPortFormat);
        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::NegotiateComponentParameters() Problem setting video port format"));
            return false;
        }
    }


    // now that we have the color format, interpret it
    if (iOMXVideoColorFormat == OMX_COLOR_Format8bitRGB332)
    {
        iYUVFormat = PVMF_RGB8;
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_Format12bitRGB444)
    {
        iYUVFormat = PVMF_RGB12;
    }
    else if (iOMXVideoColorFormat >= OMX_COLOR_Format16bitARGB4444 && iOMXVideoColorFormat <= OMX_COLOR_Format16bitBGR565)
    {
        iYUVFormat = PVMF_RGB16;
    }
    else if (iOMXVideoColorFormat >= OMX_COLOR_Format24bitRGB888 && iOMXVideoColorFormat <= OMX_COLOR_Format24bitARGB1887)
    {
        iYUVFormat = PVMF_RGB24;
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV420Planar)
    {
        iYUVFormat = PVMF_YUV420_PLANAR; // Y, U, V are separate - entire planes
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV420PackedPlanar)
    {
        iYUVFormat = PVMF_YUV420_PACKEDPLANAR; // each slice contains Y,U,V separate
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
    {
        iYUVFormat = PVMF_YUV420_SEMIPLANAR; // Y and UV interleaved - entire planes
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV422PackedSemiPlanar)
    {
        iYUVFormat = PVMF_YUV422_PACKEDSEMIPLANAR; // Y and UV interleaved - sliced
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV422Planar)
    {
        iYUVFormat = PVMF_YUV422_PLANAR; // Y, U, V are separate - entire planes
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV422PackedPlanar)
    {
        iYUVFormat = PVMF_YUV422_PACKEDPLANAR; // each slice contains Y,U,V separate
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV422SemiPlanar)
    {
        iYUVFormat = PVMF_YUV422_SEMIPLANAR; // Y and UV interleaved - entire planes
    }
    else if (iOMXVideoColorFormat == OMX_COLOR_FormatYUV422PackedSemiPlanar)
    {
        iYUVFormat = PVMF_YUV422_PACKEDSEMIPLANAR; // Y and UV interleaved - sliced
    }
    else if (iOMXVideoColorFormat == 0x7FA30C00)
    {
        iYUVFormat = PVMF_YUV420_SEMIPLANAR_YVU; // semiplanar with Y and VU interleaved
    }
    else
    {
        iYUVFormat = PVMF_FORMAT_UNKNOWN;
        return false;
    }
    return true;
}

bool PVMFOMXVideoDecNode::SetDefaultCapabilityFlags()
{

    iIsOMXComponentMultiThreaded = true;

    iOMXComponentSupportsExternalOutputBufferAlloc = false;
    iOMXComponentSupportsExternalInputBufferAlloc = false;
    iOMXComponentSupportsMovableInputBuffers = false;

    iOMXComponentSupportsPartialFrames = false;
    iOMXComponentNeedsNALStartCode = true;
    iOMXComponentCanHandleIncompleteFrames = true;

    return true;
}



bool PVMFOMXVideoDecNode::SendEOSBufferToOMXComponent()
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendEOSBufferToOMXComponent() In"));


    // first of all, check if the component is running. EOS could be sent prior to component/decoder
    // even being initialized

    // returning false will ensure that the EOS will be sent downstream anyway without waiting for the
    // Component to respond
    if (iCurrentDecoderState != OMX_StateExecuting)
        return false;

    // get an input buffer. Without a buffer, no point in proceeding
    InputBufCtrlStruct *input_buf = NULL;
    int32 errcode = 0;

    // we already checked that the number of buffers is OK, so we don't expect problems
    // try to get input buffer header
    OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct *) iInBufMemoryPool->allocate(iInputAllocSize));
    if (errcode != 0)
    {
        if (errcode == OsclErrNoResources)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                            PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::SendEOSBufferToOMXComponent() No more buffers in the mempool - unexpected"));

            iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool); // To signal when next deallocate() is called on mempool

            return false;
        }
        else
        {
            // Memory allocation for the pool failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::SendEOSBufferToOMXComponent() Input mempool error"));


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
    OMX_EmptyThisBuffer(iOMXVideoDecoder, input_buf->pBufHdr);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendEOSBufferToOMXComponent() Out"));

    return true;

}

// this method is called under certain conditions only if the node is doing partial frame assembly
void PVMFOMXVideoDecNode::DropCurrentBufferUnderConstruction()
{
    if (iObtainNewInputBuffer == false)
    {
        if (iInputBufferUnderConstruction != NULL)
        {
            if (iInBufMemoryPool != NULL)
            {
                iInBufMemoryPool->deallocate((OsclAny *)iInputBufferUnderConstruction);
            }

            iInputBufferUnderConstruction = NULL;
        }
        iObtainNewInputBuffer = true;

    }
}

// this method is called under certain conditions only if the node is doing partial frame assembly
void PVMFOMXVideoDecNode::SendIncompleteBufferUnderConstruction()
{

    // this should never be the case, but check anyway
    if (iInputBufferUnderConstruction != NULL)
    {
        // mark as end of frame (the actual end piece is missing)
        iInputBufferUnderConstruction->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::SendIncompleteBufferUnderConstruction()  - Sending Incomplete Buffer 0x%x to OMX Component MARKER field set to %x, TS=%d", iInputBufferUnderConstruction->pBufHdr->pBuffer, iInputBufferUnderConstruction->pBufHdr->nFlags, iInTimestamp));

        OMX_EmptyThisBuffer(iOMXVideoDecoder, iInputBufferUnderConstruction->pBufHdr);

        iInputBufferUnderConstruction = NULL;
        iObtainNewInputBuffer = true;

    }
}

bool PVMFOMXVideoDecNode::SendInputBufferToOMXComponent()
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() In"));


    // first need to take care of  missing packets if node is assembling partial frames.
    // The action depends whether the component (I) can handle incomplete frames/NALs or (II) cannot handle incomplete frames/NALs
    if (!iOMXComponentSupportsPartialFrames)
    {

        // there are 4 cases after receiving a media msg and realizing there were missing packet(s):

        // a) TS remains the same - i.e. missing 1 or more pieces in the middle of the same frame
        //		 I) basically ignore  - keep assembling the same frame  (middle will be missing)
        //		II) drop current buffer, drop msgs until next msg with marker bit arrives


        // b) TS is different than previous frame. Previous frame was sent OK (had marker bit).
        //				New frame assembly has not started yet. one or more pieces are missing from
        //				the beginning of the frame
        //	  	 I) basically ignore - get a new buffer and start assembling new frame (beginning will be missing)
        //		II) no buffer to drop, but keep dropping msgs until next msg with marker bit arrives

        // c) TS is different than previous frame. Frame assembly has started (we were in the middle of a frame)
        //		but only 1 piece is missing => We know that the missing frame must have had the marker bit

        //		 I) send out current buffer (last piece will be missing), get a new buffer and start assembling new frame (which is OK)
        //		II) just drop current buffer. Get a new buffer and start assembling new frame (no need to wait for marker bit)

        // d) TS is different than previous frame. Frame assembly has started ( we were in the middle of a frame)
        //		multiple pieces are missing => The last piece of the frame with the marker bit is missing for sure, but
        //		there could be also other frames missing or the beginning of the next frame is missing etc.

        //		 I) send out current bufer (last piece will be missing). Get a new buffer and start assembling new frame (beginning COULD BE missing as well)
        //		II) drop current buffer. Keep dropping msgs until next msg with marker bit arrives


        // extract info from the media message

        uint32 current_msg_seq_num = iDataIn->getSeqNum();
        uint32 current_msg_ts = iDataIn->getTimestamp();
        uint32 current_msg_marker;
        if (iSetMarkerBitForEveryFrag == true) // PV AVC case
        {

            current_msg_marker = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT;
        }
        else
        {
            current_msg_marker = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
        }

        // check if this is the very first data msg
        if (iFirstDataMsgAfterBOS)
        {
            iFirstDataMsgAfterBOS = false;
            //init the sequence number & ts to make sure dropping logic does not kick in
            iInPacketSeqNum = current_msg_seq_num - 1;
            iInTimestamp = current_msg_ts - 10;
        }


        // first check if we need to keep dropping msgs
        if (iKeepDroppingMsgsUntilMarkerBit)
        {
            // drop this message
            iDataIn.Unbind();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() Dropping input msg with seqnum %d until marker bit", current_msg_seq_num));

            //if msg has marker bit, stop dropping msgs
            if (current_msg_marker != 0)
            {
                iKeepDroppingMsgsUntilMarkerBit = false;
                // also remember the sequence number & timestamp so that we have reference
                iInPacketSeqNum = current_msg_seq_num;
                iInTimestamp = current_msg_ts;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() Input msg with seqnum %d has marker bit set. Stop dropping msgs", current_msg_seq_num));

            }
            return true;
        }

        // compare current and saved sequence number - difference should be exactly 1
        //	if it is more, there is something missing
        if ((current_msg_seq_num - iInPacketSeqNum) > 1)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - MISSING PACKET DETECTED. Input msg with seqnum %d, TS=%d. Previous seqnum: %d, Previous TS: %d", current_msg_seq_num, iInPacketSeqNum, current_msg_ts, iInTimestamp));

            // find out which case it is by comparing TS
            if (current_msg_ts == iInTimestamp)
            {

                // this is CASE a)
                // same ts, i.e. pieces are missing from the middle of the current frame
                if (!iOMXComponentCanHandleIncompleteFrames)
                {
                    // drop current buffer, drop msgs until you hit msg with marker bit
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Drop current buffer under construction. Keep dropping msgs until marker bit"));

                    DropCurrentBufferUnderConstruction();
                    iKeepDroppingMsgsUntilMarkerBit = true;
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Continue processing"));

                }
            }
            else // new ts and old ts are different
            {
                //  are we at the beginning of the new frame assembly?
                if (iObtainNewInputBuffer)
                {
                    // CASE b)
                    // i.e. we sent out previous frame, but have not started assembling a new frame. Pieces are missing from the beginning
                    if (!iOMXComponentCanHandleIncompleteFrames)
                    {
                        // there is no current buffer to drop, but drop msgs until you hit msg with marker bit
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - No current buffer under construction. Keep dropping msgs until marker bit"));

                        iKeepDroppingMsgsUntilMarkerBit = true;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Continue processing"));
                    }
                }
                else	// no, we are in the middle of a frame assembly, but new ts is different
                {
                    // is only 1 msg missing?
                    if ((current_msg_seq_num - iInPacketSeqNum) == 2)
                    {
                        // CASE c)
                        // only the last piece of the previous frame is missing
                        if (iOMXComponentCanHandleIncompleteFrames)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Send incomplete buffer under construction. Start assembling new frame"));

                            SendIncompleteBufferUnderConstruction();
                        }
                        else
                        {
                            // drop current frame only, but no need to wait until next marker bit.
                            // start assembling new frame
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Drop current buffer under construction. It's OK to start assembling new frame. Only 1 packet is missing"));

                            DropCurrentBufferUnderConstruction();
                        }
                    }
                    else
                    {
                        // CASE d)
                        // (multiple) final piece(s) of the previous frame are missing and possibly pieces at the
                        // beginning of a new frame are also missing
                        if (iOMXComponentCanHandleIncompleteFrames)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Send incomplete buffer under construction. Start assembling new frame (potentially damaged)"));

                            SendIncompleteBufferUnderConstruction();
                        }
                        else
                        {
                            // drop current frame. start assembling new frame, but first keep dropping
                            // until you hit msg with marker bit.
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Drop current buffer under construction. Keep dropping msgs until marker bit"));

                            DropCurrentBufferUnderConstruction();
                            iKeepDroppingMsgsUntilMarkerBit = true;
                        }
                    }
                }// end of if(obtainNewInputBuffer)/else
            }// end of if(curr_msg_ts == iInTimestamp)
        }//end of if(deltaseqnum>1)/else

        // check if we need to keep dropping msgs
        if (iKeepDroppingMsgsUntilMarkerBit)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() Dropping input msg with seqnum %d until marker bit", current_msg_seq_num));

            // drop this message
            iDataIn.Unbind();

            //if msg has marker bit, stop dropping msgs
            if (current_msg_marker != 0)
            {
                iKeepDroppingMsgsUntilMarkerBit = false;
                // also remember the sequence number & timestamp so that we have reference
                iInPacketSeqNum = current_msg_seq_num;
                iInTimestamp = current_msg_ts;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() Input msg with seqnum %d has marker bit set. Stop dropping msgs", current_msg_seq_num));

            }
            return true;
        }

    }// end of if/else (iOMXSUpportsPartialFrames)

    InputBufCtrlStruct *input_buf = NULL;
    int32 errcode = 0;

// NOTE: a) if NAL start codes must be inserted i.e. iOMXComponentNeedsNALStartCodes is TRUE, then iOMXComponentSupportsMovableInputBuffers must be set to FALSE.
//		 b) if iOMXComponentSupportsPartialFrames is FALSE, then iOMXComponentSupportsMovableInputBuffers must be FALSE as well
//		 c) if iOMXCOmponentSupportsPartialFrames is FALSE, and the input frame/NAL size is larger than the buffer size, the frame/NAL is discarded

    do
    {
        // do loop to loop over all fragments
        // first of all , get an input buffer. Without a buffer, no point in proceeding
        if (iObtainNewInputBuffer == true) // if partial frames are being reconstructed, we may be copying data into
            //existing buffer, so we don't need the new buffer
        {
            // try to get input buffer header
            OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct *) iInBufMemoryPool->allocate(iInputAllocSize));
            if (errcode != 0)
            {
                if (errcode == OsclErrNoResources)
                {

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                                    PVLOGMSG_DEBUG, (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() No more buffers in the mempool"));

                    iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool); // To signal when next deallocate() is called on mempool

                    return false;
                }
                else
                {
                    // Memory allocation for the pool failed
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() Input mempool error"));


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
            input_buf->pBufHdr->nFilledLen = 0; // init this for now
            // save this in a class member
            iInputBufferUnderConstruction = input_buf;
            // set flags
            if (iOMXComponentSupportsPartialFrames == true)
            {
                // if partial frames can be sent, then send them
                // but we'll always need the new buffer for the new fragment
                iObtainNewInputBuffer = true;
            }
            else
            {
                // if we need to assemble partial frames, then obtain a new buffer
                // only after assembling the partial frame
                iObtainNewInputBuffer = false;
            }

            iFirstPieceOfPartialFrame = true;
        }
        else
        {
            input_buf = iInputBufferUnderConstruction;
        }

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

            if (iSetMarkerBitForEveryFrag == true) // PV AVC case
            {
                iCurrentMsgMarkerBit = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT;
            }
            else
            {
                iCurrentMsgMarkerBit = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
            }

            // logging info:
            if (iDataIn->getNumFragments() > 1)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - New msg has MULTI-FRAGMENTS"));
            }

            if (!(iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT) && !(iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_END_OF_NAL_BIT))
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - New msg has NO MARKER BIT"));
            }
        }


        // get a memfrag from the message
        OsclRefCounterMemFrag frag;
        iDataIn->getMediaFragment(iCurrFragNum, frag);


        if (iOMXComponentSupportsMovableInputBuffers)
        {
            // no copying required
            // Note: This cannot be used for NAL start code insertion and
            //		 for the case when partial frames are not supported by the component

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
                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Buffer 0x%x of size %d, %d frag out of tot. %d, TS=%d", input_buf->pBufHdr->pBuffer, frag.getMemFragSize(), iCurrFragNum + 1, iDataIn->getNumFragments(), iInTimestamp));

            iCurrFragNum++; // increment fragment number and move on to the next
            iIsNewDataFragment = true; // update the flag

        }
        else
        {

            // in this case, no need to use input msg refcounter, each buffer fragment is copied over and treated separately
            (input_buf->pMediaData).Unbind();


            if (iOMXComponentNeedsNALStartCode == true && iFirstPieceOfPartialFrame == true)
            {
                oscl_memcpy(input_buf->pBufHdr->pBuffer + input_buf->pBufHdr->nFilledLen,
                            (void *) NAL_START_CODE,
                            NAL_START_CODE_SIZE);
                input_buf->pBufHdr->nFilledLen += NAL_START_CODE_SIZE;
                iFirstPieceOfPartialFrame = false;

            }

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
            uint32 bytes_remaining_in_buffer = (input_buf->pBufHdr->nAllocLen - input_buf->pBufHdr->nFilledLen);

            if (iFragmentSizeRemainingToCopy <= bytes_remaining_in_buffer)
            {

                oscl_memcpy(input_buf->pBufHdr->pBuffer + input_buf->pBufHdr->nFilledLen,
                            (void *)((uint8 *)frag.getMemFragPtr() + iCopyPosition),
                            iFragmentSizeRemainingToCopy);

                input_buf->pBufHdr->nFilledLen += iFragmentSizeRemainingToCopy;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Copied %d bytes of fragment %d out of %d into buffer 0x%x of size %d, TS=%d ", iFragmentSizeRemainingToCopy, iCurrFragNum + 1, iDataIn->getNumFragments(), input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFilledLen, iInTimestamp));

                iCopyPosition += iFragmentSizeRemainingToCopy;
                iFragmentSizeRemainingToCopy = 0;



                iIsNewDataFragment = true; // done with this fragment. Get a new one
                iCurrFragNum++;

            }
            else
            {
                // copy as much as you can of the current fragment into the current buffer
                if (bytes_remaining_in_buffer > 0)
                {
                    oscl_memcpy(input_buf->pBufHdr->pBuffer + input_buf->pBufHdr->nFilledLen,
                                (void *)((uint8 *)frag.getMemFragPtr() + iCopyPosition),
                                bytes_remaining_in_buffer);

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Copied %d bytes of fragment %d out of %d into buffer 0x%x of size %d, TS=%d", input_buf->pBufHdr->nAllocLen, iCurrFragNum + 1, iDataIn->getNumFragments(), input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFilledLen, iInTimestamp));
                }
                input_buf->pBufHdr->nFilledLen = input_buf->pBufHdr->nAllocLen;
                iCopyPosition += bytes_remaining_in_buffer; // move current position within fragment forward
                iFragmentSizeRemainingToCopy -= bytes_remaining_in_buffer;
                iIsNewDataFragment = false; // set the flag to indicate we're still working on the "old" fragment

                if (!iOMXComponentSupportsPartialFrames)
                {
                    // if partial frames are not supported, and data cannot fit into the buffer, i.e. the buffer is full at this point
                    // simply go through remaining fragments if they exist and "drop" them
                    // i.e. send what data is alrady copied in the buffer and ingore the rest
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - Reconstructing partial frame - more data cannot fit in buffer 0x%x, TS=%d.Skipping data.", input_buf->pBufHdr->pBuffer, iInTimestamp));

                    iIsNewDataFragment = true; // done with this fragment, get a new one
                    iCurrFragNum++;
                }


            }

        }


        // set buffer fields (this is the same regardless of whether the input is movable or not)
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
        // a) AVC - file playback - each fragment is a complete NAL (1 or more frags i.e. NALs per msg)
        //    AVC - streaming	- 1 msg contains 1 full NAL or a portion of a NAL
        // NAL may be broken up over multiple msgs. Frags are not allowed in streaming
        // b) M4V - file playback - each msg is 1 frame
        //    M4V - streaming   - 1 frame may be broken up into multiple messages and fragments

        // c) WMV - file playback - 1 frame is 1 msg
        //    WMV - streaming     - 1 frame may be broken up into multiple messages and fragments

        if (iSetMarkerBitForEveryFrag == true)
        {
            // this is basically the AVC case of PV
            //
            if (iIsNewDataFragment)
            {
                if ((iDataIn->getNumFragments() > 1))
                {
                    // if more than 1 fragment in the message and we have not broken it up
                    //(i.e. this is the last piece of a broken up piece), put marker bit on it unconditionally
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - END OF FRAGMENT - Multifragmented msg AVC case, Buffer 0x%x MARKER bit set to 1, TS=%d", input_buf->pBufHdr->pBuffer, iInTimestamp));
                    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                    // if NAL is complete, make sure you send it and obtain new buffer
                    iObtainNewInputBuffer = true;
                }
                else if ((iDataIn->getNumFragments() == 1))
                {
                    // this is (the last piece of broken up by us) single-fragmented message. This can be a piece of a NAL (streaming) or a full NAL (file )
                    // apply marker bit if the message carries one
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - END OF FRAGMENT - Buffer 0x%x MARKER bit set to %d, TS=%d", input_buf->pBufHdr->pBuffer, iCurrentMsgMarkerBit, iInTimestamp));

                    if (iCurrentMsgMarkerBit)
                    {
                        input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                        // once NAL is complete, make sure you send it and obtain new buffer

                        iObtainNewInputBuffer = true;
                    }

                }
            }
            else
            {
                // we are separating fragments that are too big, i.e. bigger than
                // what 1 buffer can hold, this fragment Can NEVER have marker bit
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - NOT END OF FRAGMENT - Buffer 0x%x MARKER bit set to 0, TS=%d", input_buf->pBufHdr->pBuffer, iInTimestamp));

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
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - END OF MESSAGE - Buffer 0x%x MARKER bit set to 1, TS=%d", input_buf->pBufHdr->pBuffer, iInTimestamp));

                    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
                    // once frame is complete, make sure you send it and obtain new buffer

                    iObtainNewInputBuffer = true;
                }
                else
                {

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - END OF MESSAGE - Buffer 0x%x MARKER bit set to 0, TS=%d", input_buf->pBufHdr->pBuffer, iInTimestamp));
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() - NOT END OF MESSAGE - Buffer 0x%x MARKER bit set to 0, TS=%d", input_buf->pBufHdr->pBuffer, iInTimestamp));
            }


        }// end of else(setmarkerbitforeveryfrag)


        // set the key frame flag if necessary (mark every fragment that belongs to it)
        if (iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_RANDOM_ACCESS_POINT_BIT)
            input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

        if (iObtainNewInputBuffer == true)
        {
            // if partial frames are supported, this flag will always be set
            // if partial frames are not supported, this flag will be set only
            // if the partial frame/NAL has been assembled, so we can send it


            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent()  - Sending Buffer 0x%x to OMX Component MARKER field set to %x, TS=%d", input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFlags, iInTimestamp));

            OMX_EmptyThisBuffer(iOMXVideoDecoder, input_buf->pBufHdr);
            iInputBufferUnderConstruction = NULL; // this buffer is gone to OMX component now
        }

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
                    (0, "PVMFOMXVideoDecNode::SendInputBufferToOMXComponent() Out"));

    return true;

}
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::InitDecoder(PVMFSharedMediaDataPtr& DataIn)
{
    uint16 length = 0, size = 0;
    uint8 *tmp_ptr;

    OsclRefCounterMemFrag DataFrag;
    OsclRefCounterMemFrag refCtrMemFragOut;



    // NOTE: the component may not start decoding without providing the Output buffer to it,
    //		here, we're sending input/config buffers.
    //		Then, we'll go to ReadyToDecode state and send output as well

    switch (((PVMFOMXVideoDecPort*)iInPort)->iFormat)
    {
        case PVMF_H264:
        case PVMF_H264_MP4:
        {
            DataIn->getFormatSpecificInfo(DataFrag);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::InitDecoder() Config NALs (Size=%d)", DataFrag.getMemFragSize()));
            //get pointer to the data fragment
            uint8* initbuffer = (uint8 *) DataFrag.getMemFragPtr();
            uint32 initbufsize = (int32) DataFrag.getMemFragSize();

            if (initbufsize > 0)
            {

                // there may be more than 1 NAL in config info in format specific data memfragment (SPS, PPS)
                tmp_ptr = initbuffer;
                do
                {
                    length = (uint16)(tmp_ptr[1] << 8) | tmp_ptr[0];
                    size += (length + 2);
                    if (size > initbufsize)
                        break;
                    tmp_ptr += 2;


                    if (!SendConfigBufferToOMXComponent(tmp_ptr, length))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXVideoDecNode::InitDecoder() Error in processing config buffer"));
                        return false;

                    }

                    tmp_ptr += length;

                }
                while (size < initbufsize);
            }

        }
        break;


        case PVMF_M4V:
        case PVMF_H263:
        {
            DataIn->getFormatSpecificInfo(DataFrag);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::InitDecoder() VOL header (Size=%d)", DataFrag.getMemFragSize()));

            //get pointer to the data fragment
            uint8* initbuffer = (uint8 *) DataFrag.getMemFragPtr();
            uint32 initbufsize = (int32) DataFrag.getMemFragSize();

            // for H263, the initbufsize is 0, and initbuf= NULL. Config is done after 1st frame of data
            if (initbufsize > 0)
            {

                if (!SendConfigBufferToOMXComponent(initbuffer, initbufsize))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::InitDecoder() Error in processing config buffer"));
                    return false;
                }
            }


        }
        break;
        case PVMF_WMV:
        {

            // in case of WMV, get config parameters from the port
            uint8* initbuffer = ((PVMFOMXVideoDecPort*)iInPort)->getTrackConfig();
            int32 initbufsize = (int32)((PVMFOMXVideoDecPort*)iInPort)->getTrackConfigSize();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::InitDecoder() for WMV Decoder. Initialization data Size %d.", initbufsize));

            if (initbufsize > 0)
            {

                if (!SendConfigBufferToOMXComponent(initbuffer, initbufsize))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::InitDecoder() Error in processing config buffer"));
                    return false;
                }
            }
        }
        break;
        case PVMF_H264_RAW:
        default:
            // Unknown codec type
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::InitDecoder() Unknown codec type"));
            return false;
    }


    //Varibles initialization
    sendYuvFsi = true;



    iLastYUVWidth = 0;
    iLastYUVHeight = 0;



    return true;
}



bool PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent(uint8 *initbuffer, uint32 initbufsize)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() In"));


    // first of all , get an input buffer. Without a buffer, no point in proceeding
    InputBufCtrlStruct *input_buf = NULL;
    int32 errcode = 0;

    // try to get input buffer header
    OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct *) iInBufMemoryPool->allocate(iInputAllocSize));
    if (errcode != 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() Input buffer mempool problem -unexpected at init"));

        return false;
    }

    // Got a buffer OK
    // keep track of buffers. When buffer is deallocated/released, the counter will be decremented
    iInBufMemoryPool->notifyfreechunkavailable(*this, (OsclAny*) iInBufMemoryPool);
    iNumOutstandingInputBuffers++;

    input_buf->pBufHdr->nFilledLen = 0; //init this to 0

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
                        (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() - New msg has NO MARKER BIT"));
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
                        (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() - Config Buffer 0x%x of size %d", initbuffer, initbufsize));

    }
    else
    {

        // in this case, no need to use input msg refcounter, each buffer fragment is copied over and treated separately
        (input_buf->pMediaData).Unbind();

        // we assume the buffer is large enough to fit the config data

        iCopyPosition = 0;
        iFragmentSizeRemainingToCopy  = initbufsize;

        if (iOMXComponentNeedsNALStartCode == true)
        {

            oscl_memcpy(input_buf->pBufHdr->pBuffer,
                        (void *) NAL_START_CODE,
                        NAL_START_CODE_SIZE);
            input_buf->pBufHdr->nFilledLen += NAL_START_CODE_SIZE;

        }

        // can the remaining fragment fit into the buffer?
        uint32 bytes_remaining_in_buffer = (input_buf->pBufHdr->nAllocLen - input_buf->pBufHdr->nFilledLen);

        if (iFragmentSizeRemainingToCopy <= bytes_remaining_in_buffer)
        {

            oscl_memcpy(input_buf->pBufHdr->pBuffer + input_buf->pBufHdr->nFilledLen,
                        (void *)(initbuffer + iCopyPosition),
                        iFragmentSizeRemainingToCopy);

            input_buf->pBufHdr->nFilledLen += iFragmentSizeRemainingToCopy;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() - Copied %d bytes into buffer 0x%x of size %d", iFragmentSizeRemainingToCopy, input_buf->pBufHdr->pBuffer, input_buf->pBufHdr->nFilledLen));

            iCopyPosition += iFragmentSizeRemainingToCopy;
            iFragmentSizeRemainingToCopy = 0;


        }
        else
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() Config buffer too large problem -unexpected at init"));

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
                    (0, "PVMFOMXVideoDecNode::SendConfigBufferToOMXComponent() - END OF FRAGMENT - Buffer 0x%x MARKER bit set to 1", input_buf->pBufHdr->pBuffer));

    input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

    OMX_EmptyThisBuffer(iOMXVideoDecoder, input_buf->pBufHdr);

    return true;

}



/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::CreateOutMemPool(uint32 num_buffers)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::CreateOutMemPool() start"));
    // In the case OMX component wants to allocate its own buffers,
    // mempool only contains OutputBufCtrlStructures (i.e. ptrs to buffer headers)
    // In case OMX component uses pre-allocated buffers (here),
    // mempool allocates OutputBufCtrlStructure (i.e. ptrs to buffer hdrs), followed by actual buffers

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::CreateOutMemPool() Allocating output buffer header pointers"));

    iOutputAllocSize = oscl_mem_aligned_size((uint32)sizeof(OutputBufCtrlStruct));

    if (iOMXComponentSupportsExternalOutputBufferAlloc)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::CreateOutMemPool() Allocating output buffers of size %d as well", iOMXComponentOutputBufferSize));

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
                        PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::CreateOutMemPool() Memory pool structure for output buffers failed to allocate"));
        return false;
    }



    // allocate a dummy buffer to actually create the mempool
    OsclAny *dummy_alloc = NULL; // this dummy buffer will be released at end of scope
    leavecode = 0;
    OSCL_TRY(leavecode, dummy_alloc = iOutBufMemoryPool->allocate(iOutputAllocSize));
    if (leavecode || dummy_alloc == NULL)
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
                        PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::CreateOutMemPool() Memory pool for output buffers failed to allocate"));
        return false;
    }
    iOutBufMemoryPool->deallocate(dummy_alloc);
    // init the counter
    iNumOutstandingOutputBuffers = 0;

    // allocate mempool for media data message wrapper
    leavecode = 0;
    OSCL_TRY(leavecode, iMediaDataMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (num_buffers, PVOMXVIDEODEC_MEDIADATA_CHUNKSIZE)));
    if (leavecode || iMediaDataMemPool == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::CreateOutMemPool() Media Data Buffer pool for output buffers failed to allocate"));
        return false;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::CreateOutMemPool() done"));
    return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Creates memory pool for input buffer management ///////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::CreateInputMemPool(uint32 num_buffers)
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
                    (0, "PVMFOMXVideoDecNode::CreateInputMemPool() start "));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::CreateInputMemPool() allocating buffer header pointers and shared media data ptrs "));



    iInputAllocSize = oscl_mem_aligned_size((uint32) sizeof(InputBufCtrlStruct)); //aligned_size_buffer_header_ptr+aligned_size_media_data_ptr;

    // Need to allocate buffers in the node either if component supports external buffers buffers
    // but they are not movable

    if ((iOMXComponentSupportsExternalInputBufferAlloc && !iOMXComponentSupportsMovableInputBuffers))
    {
        //pre-negotiated input buffer size
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::CreateOutMemPool() Allocating input buffers of size %d as well", iOMXComponentInputBufferSize));

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
                        PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::CreateInputMemPool() Memory pool structure for input buffers failed to allocate"));
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
                        PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::CreateInputMemPool() Memory pool for input buffers failed to allocate"));
        return false;
    }

    // init the counter
    iNumOutstandingInputBuffers = 0;


    iInputBufferToResendToComponent = NULL; // nothing to resend yet
    iInBufMemoryPool->deallocate(dummy_alloc);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::CreateInputMemPool() done"));
    return true;
}
////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::ProvideBuffersToComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
        uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
        uint32 aNumBuffers,    // number of buffers
        uint32 aActualBufferSize, // aactual buffer size
        uint32 aPortIndex,      // port idx
        bool aUseBufferOK,		// can component use OMX_UseBuffer or should it use OMX_AllocateBuffer
        bool	aIsThisInputBuffer		// is this input or output
                                                   )
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ProvideBuffersToComponent() enter"));

    uint32 ii = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OsclAny **ctrl_struct_ptr = NULL;	// temporary array to keep the addresses of buffer ctrl structures and buffers

    ctrl_struct_ptr = (OsclAny **) oscl_malloc(aNumBuffers * sizeof(OsclAny *));
    if (ctrl_struct_ptr == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::ProvideBuffersToComponent ctrl_struct_ptr == NULL"));
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
                                (0, "PVMFOMXVideoDecNode::ProvideBuffersToComponent ->allocate() failed for no mempool chunk available"));
            }
            else
            {
                // General error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::ProvideBuffersToComponent ->allocate() failed due to some general error"));

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

                InputBufCtrlStruct *temp = (InputBufCtrlStruct *) ctrl_struct_ptr[ii];
                oscl_memset(&(temp->pMediaData), 0, sizeof(PVMFSharedMediaDataPtr));
                temp->pMediaData = PVMFSharedMediaDataPtr(NULL, NULL);

                // advance ptr to skip the structure
                pB += oscl_mem_aligned_size(sizeof(InputBufCtrlStruct));

                err = OMX_UseBuffer(iOMXVideoDecoder,	// hComponent
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
                OutputBufCtrlStruct *temp = (OutputBufCtrlStruct *) ctrl_struct_ptr[ii];
                // advance buffer ptr to skip the structure
                pB += oscl_mem_aligned_size(sizeof(OutputBufCtrlStruct));


                err = OMX_UseBuffer(iOMXVideoDecoder,	// hComponent
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

                InputBufCtrlStruct *temp = (InputBufCtrlStruct *) ctrl_struct_ptr[ii];
                // make sure ptrs are initialized to NULL
                oscl_memset(&(temp->pMediaData), 0, sizeof(PVMFSharedMediaDataPtr));
                temp->pMediaData = PVMFSharedMediaDataPtr(NULL, NULL);

                err = OMX_AllocateBuffer(iOMXVideoDecoder,
                                         &(temp->pBufHdr),
                                         aPortIndex,
                                         ctrl_struct_ptr[ii],
                                         (OMX_U32)aActualBufferSize);
            }
            else
            {
                OutputBufCtrlStruct *temp = (OutputBufCtrlStruct *) ctrl_struct_ptr[ii];
                err = OMX_AllocateBuffer(iOMXVideoDecoder,
                                         &(temp->pBufHdr),
                                         aPortIndex,
                                         ctrl_struct_ptr[ii],
                                         (OMX_U32)aActualBufferSize);
            }

        }

        if (err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::ProvideBuffersToComponent() Problem using/allocating a buffer"));


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

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ProvideBuffersToComponent() done"));
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::FreeBuffersFromComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
        uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
        uint32 aNumBuffers,    // number of buffers
        uint32 aPortIndex,      // port idx
        bool	aIsThisInputBuffer		// is this input or output
                                                  )
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::FreeBuffersToComponent() enter"));

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
                                (0, "PVMFOMXVideoDecNode::FreeBuffersFromComponent ->allocate() failed for no mempool chunk available"));
            }
            else
            {
                // General error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::FreeBuffersFromComponent ->allocate() failed due to some general error"));

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
            InputBufCtrlStruct *temp = (InputBufCtrlStruct *) ctrl_struct_ptr[ii];
            err = OMX_FreeBuffer(iOMXVideoDecoder,
                                 aPortIndex,
                                 temp->pBufHdr);

        }
        else
        {
            iNumOutstandingOutputBuffers++;
            OutputBufCtrlStruct *temp = (OutputBufCtrlStruct *) ctrl_struct_ptr[ii];
            err = OMX_FreeBuffer(iOMXVideoDecoder,
                                 aPortIndex,
                                 temp->pBufHdr);

        }

        if (err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::FreeBuffersFromComponent() Problem freeing a buffer"));

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
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::FreeBuffersFromComponent() done"));
    return true;
}



/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR EVENT HANDLER
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXVideoDecNode::EventHandlerProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
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

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_CommandFlush - completed on port %d", aData2));

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
                    RunIfNotReady();


                }
                break;

                case OMX_CommandPortDisable:
                {
                    // if port disable command is done, we can re-allocate the buffers and re-enable the port

                    iProcessingState = EPVMFOMXVideoDecNodeProcessingState_PortReEnable;
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
                                        (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_CommandPortEnable - completed on port %d, dynamic reconfiguration needed on port %d", aData2, iSecondPortToReconfig));

                        iProcessingState = EPVMFOMXVideoDecNodeProcessingState_PortReconfig;
                        iPortIndexForDynamicReconfig = iSecondPortToReconfig;
                        iSecondPortReportedChange = false;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_CommandPortEnable - completed on port %d, resuming normal data flow", aData2));
                        iProcessingState = EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode;
                        iDynamicReconfigInProgress = false;
                        // in case pause or stop command was sent to component
                        // change processing state (because the node might otherwise
                        // start sending buffers to component before pause/stop is processed)
                        if (iPauseCommandWasSentToComponent)
                        {
                            iProcessingState = EPVMFOMXVideoDecNodeProcessingState_Pausing;
                        }
                        if (iStopCommandWasSentToComponent)
                        {
                            iProcessingState = EPVMFOMXVideoDecNodeProcessingState_Stopping;
                        }

                    }
                    RunIfNotReady();
                    break;
                }

                case OMX_CommandMarkBuffer:
                    // nothing to do here yet;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_CommandMarkBuffer - completed - no action taken"));

                    break;

                default:
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: Unsupported event"));
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
                                (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventError - Bitstream corrupt error"));
                // Errors from corrupt bitstream are reported as info events
                ReportInfoEvent(PVMFInfoProcessingFailure, NULL);

            }
            else if (aData1 == OMX_ErrorInsufficientResources)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventError - Insufficient Resources"));
                ReportErrorEvent(PVMFErrNoResources);
                SetState(EPVMFNodeError);
            }
            else
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventError"));
                // for now, any error from the component will be reported as error
                ReportErrorEvent(PVMFErrorEvent, NULL, NULL);
                SetState(EPVMFNodeError);
            }
            break;

        }

        case OMX_EventBufferFlag:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventBufferFlag (EOS) flag returned from OMX component"));

            RunIfNotReady();
            break;
        }//end of case OMX_EventBufferFlag

        case OMX_EventMark:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventMark returned from OMX component - no action taken"));

            RunIfNotReady();
            break;
        }//end of case OMX_EventMark

        case OMX_EventPortSettingsChanged:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned from OMX component"));

            // first check if dynamic reconfiguration is already in progress,
            // if so, wait until this is completed, and then initiate the 2nd reconfiguration
            if (iDynamicReconfigInProgress)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned for port %d, dynamic reconfig already in progress", aData1));

                iSecondPortToReconfig = aData1;
                iSecondPortReportedChange = true;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned for port %d", aData1));

                iProcessingState = EPVMFOMXVideoDecNodeProcessingState_PortReconfig;
                iPortIndexForDynamicReconfig = aData1;
                iDynamicReconfigInProgress = true;
            }

            RunIfNotReady();
            break;
        }//end of case OMX_PortSettingsChanged

        case OMX_EventResourcesAcquired:        //not supported
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::EventHandlerProcessing: OMX_EventResourcesAcquired returned from OMX component - no action taken"));

            RunIfNotReady();

            break;
        }

        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::EventHandlerProcessing:  Unknown Event returned from OMX component - no action taken"));

            break;
        }

    }//end of switch (eEvent)



    return OMX_ErrorNone;
}




/////////////////////////////////////////////////////////////////////////////
// This function handles the event of OMX component state change
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::HandleComponentStateChange(OMX_U32 decoder_state)
{
    switch (decoder_state)
    {
        case OMX_StateIdle:
        {
            iCurrentDecoderState = OMX_StateIdle;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleComponentStateChange: OMX_StateIdle reached"));

            //  this state can be reached either going from OMX_Loaded->OMX_Idle (preparing)
            //	or going from OMX_Executing->OMX_Idle (stopping)


            if ((iCurrentCommand.size() > 0) &&
                    (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_PREPARE))
            {
                iProcessingState = EPVMFOMXVideoDecNodeProcessingState_InitDecoder;
                SetState(EPVMFNodePrepared);
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
                RunIfNotReady();
            }
            else if ((iCurrentCommand.size() > 0) &&
                     (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_STOP))
            {
                // if we are stopped, we won't start until the node gets DoStart command.
                //	in this case, we are ready to start sending buffers
                if (iProcessingState == EPVMFOMXVideoDecNodeProcessingState_Stopping)
                    iProcessingState = EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode;
                // if the processing state was not stopping, leave the state as it was (continue port reconfiguration)
                SetState(EPVMFNodePrepared);
                iStopCommandWasSentToComponent = false;
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);

                RunIfNotReady();
            }
            else if ((iCurrentCommand.size() > 0) &&
                     (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET))
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
                            (0, "PVMFOMXVideoDecNode::HandleComponentStateChange: OMX_StateExecuting reached"));

            // this state can be reached going from OMX_Idle -> OMX_Executing (preparing)
            //	or going from OMX_Pause -> OMX_Executing (coming from pause)
            //	either way, this is a response to "DoStart" command

            if ((iCurrentCommand.size() > 0) &&
                    (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_START))
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
                            (0, "PVMFOMXVideoDecNode::HandleComponentStateChange: OMX_StatePause reached"));


            //	This state can be reached going from OMX_Executing-> OMX_Pause
            if ((iCurrentCommand.size() > 0) &&
                    (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_PAUSE))
            {
                // if we are paused, we won't start until the node gets DoStart command.
                //	in this case, we are ready to start sending buffers
                if (iProcessingState == EPVMFOMXVideoDecNodeProcessingState_Pausing)
                    iProcessingState = EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode;
                // if the processing state was not pausing, leave the state as it was (continue port reconfiguration)


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
                            (0, "PVMFOMXVideoDecNode::HandleComponentStateChange: OMX_StateLoaded reached"));
            //Check if command's responce is pending
            if ((iCurrentCommand.size() > 0) &&
                    (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET))
            {

                // move this here
                if (iInPort)
                {
                    OSCL_DELETE(((PVMFOMXVideoDecPort*)iInPort));
                    iInPort = NULL;
                }

                if (iOutPort)
                {
                    OSCL_DELETE(((PVMFOMXVideoDecPort*)iOutPort));
                    iOutPort = NULL;
                }

                iDataIn.Unbind();

                // Reset the metadata key list
                iAvailableMetadataKeys.clear();


                iProcessingState = EPVMFOMXVideoDecNodeProcessingState_Idle;
                //logoff & go back to Created state.
                SetState(EPVMFNodeIdle);
                PVMFStatus status = ThreadLogoff();
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), status);
                iResetInProgress = false;
                iResetMsgSent = false;
                //DeleteOMXVideoDecoder();
            }

            break;
        }//end of case OMX_StateLoaded

        case OMX_StateInvalid:
        default:
        {
            iCurrentDecoderState = OMX_StateInvalid;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::HandleComponentStateChange: OMX_StateInvalid reached"));

            break;
        }//end of case OMX_StateInvalid

    }//end of switch(decoder_state)

}






/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR EMPTY BUFFER DONE - input buffer was consumed
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXVideoDecNode::EmptyBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::EmptyBufferDoneProcessing: In"));

    OSCL_ASSERT((void*) aComponent == (void*) iOMXVideoDecoder); // component should match the component
    OSCL_ASSERT(aAppData == (OMX_PTR)(this));		// AppData should represent this node ptr

    // first, get the buffer "context", i.e. pointer to application private data that contains the
    // address of the mempool buffer (so that it can be released)
    InputBufCtrlStruct *pContext = (InputBufCtrlStruct *)(aBuffer->pAppPrivate);



    // if a buffer is not empty, log a msg, but release anyway
    if ((aBuffer->nFilledLen > 0) && (iDoNotSaveInputBuffersFlag == false))
        // if dynamic port reconfig is in progress for input port, don't keep the buffer
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::EmptyBufferDoneProcessing: Input buffer returned non-empty with %d bytes still in it", aBuffer->nFilledLen));


    }

    iInputBufferToResendToComponent = NULL;


    // input buffer is to be released,
    // refcount needs to be decremented (possibly - the input msg associated with the buffer will be unbound)
    // NOTE: in case of "moveable" input buffers (passed into component without copying), unbinding decrements a refcount which eventually results
    //			in input message being released back to upstream mempool once all its fragments are returned
    //		in case of input buffers passed into component by copying, unbinding has no effect
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::EmptyBufferDoneProcessing: Release input buffer (with %d refcount remaining of input message)", (pContext->pMediaData).get_count() - 1));


    (pContext->pMediaData).Unbind();


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::EmptyBufferDoneProcessing: Release input buffer %x back to mempool", pContext));

    iInBufMemoryPool->deallocate((OsclAny *) pContext);


    // the OMX spec says that no error is to be returned
    return OMX_ErrorNone;

}



/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR FILL BUFFER DONE - output buffer is ready
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXVideoDecNode::FillBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: In"));

    OSCL_ASSERT((void*) aComponent == (void*) iOMXVideoDecoder); // component should match the component
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
                        (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Output buffer has EOS set"));

    }

    // if a buffer is empty, or if it should not be sent downstream (say, due to state change)
    // release the buffer back to the pool
    if ((aBuffer->nFilledLen == 0) || (iDoNotSendOutputBuffersDownstreamFlag == true))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Release output buffer %x back to mempool - buffer empty or not to be sent downstream", pContext));

        iOutBufMemoryPool->deallocate(pContext);

    }
    else
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Output frame %d received", iFrameCounter++));

        // get pointer to actual buffer data
        uint8 *pBufdata = ((uint8*) aBuffer->pBuffer);
        // move the data pointer based on offset info
        pBufdata += aBuffer->nOffset;

        iOutTimeStamp = aBuffer->nTimeStamp;
        //ipPrivateData =  aBuffer->pPlatformPrivate; // record the pointer
        oscl_memcpy(&ipPrivateData, &(aBuffer->pPlatformPrivate), sizeof(ipPrivateData));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Wrapping buffer %x of size %d", pBufdata, aBuffer->nFilledLen));
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
                            (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Problem wrapping buffer %x of size %d - releasing the buffer", pBufdata, aBuffer->nFilledLen));

            iOutBufMemoryPool->deallocate(pContext);
        }
        else
        {

            // if there's a problem queuing output buffer, MediaDataOut will expire at end of scope and
            // release buffer back to the pool, (this should not be the case)
            if (QueueOutputBuffer(MediaDataOut, aBuffer->nFilledLen))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Buffer %x of size %d queued - reschedule the node to send out", pBufdata, aBuffer->nFilledLen));

                // if queing went OK,
                // re-schedule the node so that outgoing queue can be emptied (unless the outgoing port is busy)
                if ((iOutPort) && !(iOutPort->IsConnectedPortBusy()))
                    RunIfNotReady();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::FillBufferDoneProcessing: Problem queing buffer %x of size %d - releasing the buffer", pBufdata, aBuffer->nFilledLen));
            }


        }

    }
    // the OMX spec says that no error is to be returned
    return OMX_ErrorNone;

}
////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Put output buffer in outgoing queue //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::QueueOutputBuffer(OsclSharedPtr<PVMFMediaDataImpl> &mediadataimplout, uint32 aDataLen)
{

    bool status = true;
    PVMFSharedMediaDataPtr mediaDataOut;
    int32 leavecode = 0;

    // NOTE: ASSUMPTION IS THAT OUTGOING QUEUE IS BIG ENOUGH TO QUEUE ALL THE OUTPUT BUFFERS
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::QueueOutputFrame: In"));

    // First check if we can put outgoing msg. into the queue
    if (iOutPort->IsOutgoingQueueBusy())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFOMXVideoDecNode::QueueOutputFrame() OutgoingQueue is busy"));
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

        // Set Streamid
        mediaDataOut->setStreamID(iStreamID);

        // Set sequence number
        mediaDataOut->setSeqNum(iSeqNum++);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iDataPathLogger, PVLOGMSG_INFO, (0, ":PVMFOMXVideoDecNode::QueueOutputFrame(): - SeqNum=%d, TS=%d", iSeqNum, iOutTimeStamp));
        if (iLastYUVWidth != iYUVWidth || iYUVHeight != iLastYUVHeight)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoDecNode::QueueOutputFrame - Sending YUV FSI"));

            // set a flag to send Fsi configuration
            sendYuvFsi = true;
            //store new values for reference
            iLastYUVWidth = iYUVWidth ;
            iLastYUVHeight = iYUVHeight;
        }

        int fsiErrorCode = 0;

        // Check if Fsi configuration need to be sent
        if (sendYuvFsi)
        {
            OsclRefCounterMemFrag yuvFsiMemfrag;

            OSCL_TRY(fsiErrorCode, yuvFsiMemfrag = iYuvFsiFragmentAlloc.get(););

            OSCL_FIRST_CATCH_ANY(fsiErrorCode, PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                 (0, "PVMFOMXVideoDecNode::RemoveOutputFrame() Failed to allocate memory for  FSI")));

            if (fsiErrorCode == 0)
            {
                PVMFYuvFormatSpecificInfo0* fsiInfo = (PVMFYuvFormatSpecificInfo0*)yuvFsiMemfrag.getMemFragPtr();

                fsiInfo->uid = PVMFYuvFormatSpecificInfo0_UID;
                fsiInfo->video_format = iYUVFormat;
                fsiInfo->display_width = iYUVWidth;
                fsiInfo->display_height = iYUVHeight;
                switch (((PVMFOMXVideoDecPort*)iInPort)->iFormat)
                {
                    case PVMF_H264:
                    case PVMF_H264_MP4:
                    case PVMF_H264_RAW:
                    case PVMF_M4V:
                    case PVMF_H263:
                        fsiInfo->width = (iYUVWidth + 15) & (~15);
                        fsiInfo->height = (iYUVHeight + 15) & (~15);
                        break;
                    case PVMF_WMV:
                        fsiInfo->width = (iYUVWidth + 3) & -4;
                        fsiInfo->height = iYUVHeight;
                        break;
                    default:
                        fsiInfo->width = iYUVWidth;
                        fsiInfo->height = iYUVHeight;
                        break;
                }

                //mediaDataOut->setFormatSpecificInfo(yuvFsiMemfrag);
                ((PVMFOMXVideoDecPort*)iOutPort)->pvmiSetPortFormatSpecificInfoSync(yuvFsiMemfrag);
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXVideoDecNode::QueueOutputFrame - Problem allocating Output FSI"));
                SetState(EPVMFNodeError);
                ReportErrorEvent(PVMFErrNoMemory);
                return false; // this is going to make everything go out of scope
            }

            // Reset the flag
            sendYuvFsi = false;
        }

// in case of special YVU format, attach fsi to every outgoing message containing ptr to private data
        if (iYUVFormat == PVMF_YUV420_SEMIPLANAR_YVU)
        {
            OsclRefCounterMemFrag privatedataFsiMemFrag;

            OSCL_TRY(fsiErrorCode, privatedataFsiMemFrag = iPrivateDataFsiFragmentAlloc.get(););

            OSCL_FIRST_CATCH_ANY(fsiErrorCode, PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                 (0, "PVMFOMXVideoDecNode::RemoveOutputFrame() Failed to allocate memory for  FSI for private data")));


            if (fsiErrorCode == 0)
            {
                uint8 *fsiptr = (uint8*) privatedataFsiMemFrag.getMemFragPtr();
                privatedataFsiMemFrag.getMemFrag().len = sizeof(ipPrivateData);
                oscl_memcpy(fsiptr, &ipPrivateData, sizeof(ipPrivateData)); // store ptr data into fsi
                mediaDataOut->setFormatSpecificInfo(privatedataFsiMemFrag);
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMFOMXVideoDecNode::QueueOutputFrame - Problem allocating Output FSI for private data"));
                SetState(EPVMFNodeError);
                ReportErrorEvent(PVMFErrNoMemory);
                return false; // this is going to make everything go out of scope
            }
        }

        if (fsiErrorCode == 0)
        {
            // Send frame to downstream node
            PVMFSharedMediaMsgPtr mediaMsgOut;
            convertToPVMFMediaMsg(mediaMsgOut, mediaDataOut);

            if (iOutPort && (iOutPort->QueueOutgoingMsg(mediaMsgOut) == PVMFSuccess))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::QueueOutputFrame(): Queued frame OK "));

            }
            else
            {
                // we should not get here because we always check for whether queue is busy or not MC
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::QueueOutputFrame(): Send frame failed"));
                return false;
            }

        }


    }//end of if (leavecode==0)
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFOMXVideoDecNode::QueueOutputFrame() call PVMFMediaData::createMediaData is failed"));
        return false;
    }

    return status;

}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Attach a MediaDataImpl wrapper (refcount, deallocator etc.)
/////////////////////////////// to the output buffer /////////////////////////////////////////
OsclSharedPtr<PVMFMediaDataImpl> PVMFOMXVideoDecNode::WrapOutputBuffer(uint8 *pData, uint32 aDataLen, OsclAny *pContext)
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
bool PVMFOMXVideoDecNode::SendBeginOfMediaStreamCommand()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendBeginOfMediaStreamCommand() In"));

    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();
    // Set the formatID, timestamp, sequenceNumber and streamID for the media message
    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_BOS_FORMAT_ID);
    sharedMediaCmdPtr->setTimestamp(iBOSTimestamp);
    //reset the sequence number
    uint32 seqNum = 0;
    sharedMediaCmdPtr->setSeqNum(seqNum);
    sharedMediaCmdPtr->setStreamID(iStreamID);

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);
    if (iOutPort->QueueOutgoingMsg(mediaMsgOut) != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::SendBeginOfMediaStreamCommand() Outgoing queue busy"));
        return false;
    }

    iSendBOS = false;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendBeginOfMediaStreamCommand() BOS Sent StreamID %d", iStreamID));
    return true;
}
////////////////////////////////////
bool PVMFOMXVideoDecNode::SendEndOfTrackCommand(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendEndOfTrackCommand() In"));

    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();

    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

    // Set the timestamp
    sharedMediaCmdPtr->setTimestamp(iEndOfDataTimestamp);

    // Set Streamid
    sharedMediaCmdPtr->setStreamID(iStreamID);

    // Set the sequence number
    sharedMediaCmdPtr->setSeqNum(iSeqNum++);

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);
    if (iOutPort->QueueOutgoingMsg(mediaMsgOut) != PVMFSuccess)
    {
        // this should not happen because we check for queue busy before calling this function
        return false;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::SendEndOfTrackCommand() Out"));
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//The various command handlers call this routine when a command is complete.
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::CommandComplete(PVMFOMXVideoDecNodeCmdQ& aCmdQ, PVMFOMXVideoDecNodeCommand& aCmd, PVMFStatus aStatus, OsclAny* aEventData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
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
void PVMFOMXVideoDecNode::DoInit(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoInit() In"));
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
void PVMFOMXVideoDecNode::DoPrepare(PVMFOMXVideoDecNodeCommand& aCmd)
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

            uint32 Format = ((PVMFOMXVideoDecPort*)iInPort)->iFormat;
            switch (Format)
            {
                case PVMF_H264:
                case PVMF_H264_MP4:
                case PVMF_H264_RAW:
                    Role = "video_decoder.avc";
                    break;
                case PVMF_M4V:
                    Role = "video_decoder.mpeg4";
                    break;
                case PVMF_H263:
                    Role = "video_decoder.h263";
                    break;
                case PVMF_WMV:
                    Role = "video_decoder.wmv";
                    break;

                default:
                    // Illegal codec specified.
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoPrepare() Input port format other then codec type"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
                    return;
            }


            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoDecNode::Initializing OMX component and decoder for role %s", Role));

            /* Set callback structure */
            iCallbacks.EventHandler    = CallbackEventHandler; //event_handler;
            iCallbacks.EmptyBufferDone = CallbackEmptyBufferDone; //empty_buffer_done;
            iCallbacks.FillBufferDone  = CallbackFillBufferDone; //fill_buffer_done;


            // determine components which can fit the role
            // then, create the component. If multiple components fit the role,
            // the first one registered will be selected. If that one fails to
            // be created, the second one in the list is selected etc.
            OMX_U32 num_comps = 0;
            OMX_STRING *CompOfRole;
            // call once to find out the number of components that can fit the role
            //PV_Master
            PV_MasterOMX_GetComponentsOfRole(Role, &num_comps, NULL);
            uint32 ii;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoDecNode::DoPrepare(): There are %d components of role %s ", num_comps, Role));

            if (num_comps > 0)
            {
                CompOfRole = (OMX_STRING *)oscl_malloc(num_comps * sizeof(OMX_STRING));

                for (ii = 0; ii < num_comps; ii++)
                    CompOfRole[ii] = (OMX_STRING) oscl_malloc(PV_OMX_MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8));

                // call 2nd time to get the component names
                //PV_Master
                PV_MasterOMX_GetComponentsOfRole(Role, &num_comps, (OMX_U8 **)CompOfRole);

                for (ii = 0; ii < num_comps; ii++)
                {
                    // try to create component
                    err = PV_MasterOMX_GetHandle(&iOMXVideoDecoder, (OMX_STRING) CompOfRole[ii], (OMX_PTR) this, (OMX_CALLBACKTYPE *) & iCallbacks);

                    // if successful, no need to continue
                    if ((err == OMX_ErrorNone) && (iOMXVideoDecoder != NULL))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMFOMXVideoDecNode::DoPrepare(): Got Component %s handle ", CompOfRole[ii]));

                        break;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMFOMXVideoDecNode::DoPrepare(): Cannot get component %s handle, try another component if available", CompOfRole[ii]));
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
                if ((err != OMX_ErrorNone) || (iOMXVideoDecoder == NULL))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::Can't get handle for decoder!"));
                    iOMXVideoDecoder = NULL;
                    CommandComplete(iInputCommands, aCmd, PVMFErrResource);
                    return;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::No component can handle role %s !", Role));
                iOMXVideoDecoder = NULL;
                CommandComplete(iInputCommands, aCmd, PVMFErrResource);
                return;
            }



            if (!iOMXVideoDecoder)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }

            // GET CAPABILITY FLAGS FROM PV COMPONENT, IF this fails, use defaults
            PV_OMXComponentCapabilityFlagsType Cap_flags;
            err = OMX_GetParameter(iOMXVideoDecoder, (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX, &Cap_flags);
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
                //temporarily define 3 flags here
                iOMXComponentSupportsPartialFrames = true;
                iOMXComponentNeedsNALStartCode = false;
                iOMXComponentCanHandleIncompleteFrames = true;
            }
            // do some sanity checking

            if ((Format != PVMF_H264) && (Format != PVMF_H264_MP4) && (Format != PVMF_H264_RAW))
            {
                iOMXComponentNeedsNALStartCode = false;
            }

            // make sure that copying is used where necessary
            if (!iOMXComponentSupportsPartialFrames || iOMXComponentNeedsNALStartCode)
            {
                iOMXComponentSupportsMovableInputBuffers = false;
            }

            // find out about parameters

            if (!NegotiateComponentParameters())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoPrepare() Cannot get component parameters"));

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
            iThreadSafeHandlerEventHandler = OSCL_NEW(EventHandlerThreadSafeCallbackAO, (this, 10, "EventHandlerAO", Priority() + 2));

            if (iThreadSafeHandlerEmptyBufferDone)
            {
                OSCL_DELETE(iThreadSafeHandlerEmptyBufferDone);
                iThreadSafeHandlerEmptyBufferDone = NULL;
            }
            // use queue depth of iNumInputBuffers to prevent deadlock
            iThreadSafeHandlerEmptyBufferDone = OSCL_NEW(EmptyBufferDoneThreadSafeCallbackAO, (this, iNumInputBuffers, "EmptyBufferDoneAO", Priority() + 1));

            if (iThreadSafeHandlerFillBufferDone)
            {
                OSCL_DELETE(iThreadSafeHandlerFillBufferDone);
                iThreadSafeHandlerFillBufferDone = NULL;
            }
            // use queue depth of iNumOutputBuffers to prevent deadlock
            iThreadSafeHandlerFillBufferDone = OSCL_NEW(FillBufferDoneThreadSafeCallbackAO, (this, iNumOutputBuffers, "FillBufferDoneAO", Priority() + 1));

            if ((iThreadSafeHandlerEventHandler == NULL) ||
                    (iThreadSafeHandlerEmptyBufferDone == NULL) ||
                    (iThreadSafeHandlerFillBufferDone == NULL)
               )
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::Can't get threadsafe callbacks for decoder!"));
                iOMXVideoDecoder = NULL;
            }

            // ONLY FOR AVC FILE PLAYBACK WILL 1 FRAGMENT CONTAIN ONE FULL NAL
            if ((Format == PVMF_H264) || (Format == PVMF_H264_MP4))
            {
                // every memory fragment in case of AVC is a full NAL
                iSetMarkerBitForEveryFrag = true;
            }
            else
            {
                iSetMarkerBitForEveryFrag = false;
            }


            // Init Decoder
            iCurrentDecoderState = OMX_StateLoaded;
            /* Change state to OMX_StateIdle from OMX_StateLoaded. */
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoDecNode::DoPrepare(): Changing Component state Loaded -> Idle "));

            err = OMX_SendCommand(iOMXVideoDecoder, OMX_CommandStateSet, OMX_StateIdle, NULL);

            if (err != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoPrepare() Can't send StateSet command!"));
                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }
            /* Allocate input buffers */
            if (!CreateInputMemPool(iNumInputBuffers))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoPrepare() Can't allocate mempool for input buffers!"));

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
                                (0, "PVMFOMXVideoDecNode::DoPrepare() Component can't use input buffers!"));

                CommandComplete(iInputCommands, aCmd, PVMFErrNoResources);
                return;
            }


            /* Allocate output buffers */
            if (!CreateOutMemPool(iNumOutputBuffers))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoPrepare() Can't allocate mempool for output buffers!"));

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
                                (0, "PVMFOMXVideoDecNode::DoPrepare() Component can't use output buffers!"));

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
void PVMFOMXVideoDecNode::DoStart(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoStart() In"));

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
            err = OMX_GetState(iOMXVideoDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,

                                (0, "PVMFOMXVideoDecNode::DoStart(): Can't get State of decoder!"));

                sState = OMX_StateInvalid;
            }

            if ((sState == OMX_StateIdle) || (sState == OMX_StatePause))
            {
                /* Change state to OMX_StateExecuting form OMX_StateIdle. */
                // init the flag
                iDoNotSendOutputBuffersDownstreamFlag = false; // or if output was not being sent downstream due to state changes
                // re-anable sending output

                iDoNotSaveInputBuffersFlag = false;


                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::DoStart() Changing Component state Idle->Executing"));

                err = OMX_SendCommand(iOMXVideoDecoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::DoStart(): Can't send StateSet command to decoder!"));

                    status = PVMFErrInvalidState;
                }

            }
            else
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoStart(): Decoder is not in the Idle or Pause state!"));

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
void PVMFOMXVideoDecNode::DoStop(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoStop() In"));

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

            //if we're in the middle of a partial frame assembly
            // abandon it and start fresh
            if (iObtainNewInputBuffer == false)
            {
                if (iInputBufferUnderConstruction != NULL)
                {
                    if (iInBufMemoryPool != NULL)
                    {
                        iInBufMemoryPool->deallocate((OsclAny *)iInputBufferUnderConstruction);
                    }
                    iInputBufferUnderConstruction = NULL;
                }
                iObtainNewInputBuffer = true;

            }

            iFirstDataMsgAfterBOS = true;

            //Get state of OpenMAX decoder
            err = OMX_GetState(iOMXVideoDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoStop(): Can't get State of decoder!"));

                sState = OMX_StateInvalid;
            }

            if ((sState == OMX_StateExecuting) || (sState == OMX_StatePause))
            {
                /* Change state to OMX_StateIdle from OMX_StateExecuting or OMX_StatePause. */

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::DoStop() Changing Component State Executing->Idle or Pause->Idle"));

                err = OMX_SendCommand(iOMXVideoDecoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::DoStop(): Can't send StateSet command to decoder!"));

                    CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                    break;
                }

                // prevent the node from sending more buffers etc.
                // if port reconfiguration is in process, let the state remain one of the port config states
                //	if there is a start command, we can do it seemlessly (by continuing the port reconfig)
                if (iProcessingState == EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode)
                    iProcessingState = EPVMFOMXVideoDecNodeProcessingState_Stopping;

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
                                (0, "PVMFOMXVideoDecNode::DoStop(): Decoder is not in the Executing or Pause state!"));

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
void PVMFOMXVideoDecNode::DoFlush(PVMFOMXVideoDecNodeCommand& aCmd)
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
            // Stop data source

            break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::DoPause(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoPause() In"));

    OMX_ERRORTYPE  err;
    OMX_STATETYPE sState;

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:


            //Get state of OpenMAX decoder
            err = OMX_GetState(iOMXVideoDecoder, &sState);
            if (err != OMX_ErrorNone)
            {
                //Error condition report
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoPause(): Can't get State of decoder!"));

                sState = OMX_StateInvalid;
            }

            if (sState == OMX_StateExecuting)
            {
                /* Change state to OMX_StatePause from OMX_StateExecuting. */
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoDecNode::DoPause() Changing Component State Executing->Idle"));

                err = OMX_SendCommand(iOMXVideoDecoder, OMX_CommandStateSet, OMX_StatePause, NULL);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::DoPause(): Can't send StateSet command to decoder!"));

                    CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
                    break;
                }
                // prevent the node from sending more buffers etc.
                // if port reconfiguration is in process, let the state remain one of the port config states
                //	if there is a start command, we can do it seemlessly (by continuing the port reconfig)
                if (iProcessingState == EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode)
                    iProcessingState = EPVMFOMXVideoDecNodeProcessingState_Pausing;

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
                                (0, "PVMFOMXVideoDecNode::DoPause(): Decoder is not in the Executing state!"));
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
void PVMFOMXVideoDecNode::DoReset(PVMFOMXVideoDecNodeCommand& aCmd)
{

    OMX_ERRORTYPE  err;
    OMX_STATETYPE sState;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DoReset() In"));

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
            if (iOMXVideoDecoder != NULL)
            {

                //if we're in the middle of a partial frame assembly
                // abandon it and start fresh
                if (iObtainNewInputBuffer == false)
                {
                    if (iInputBufferUnderConstruction != NULL)
                    {
                        if (iInBufMemoryPool != NULL)
                        {
                            iInBufMemoryPool->deallocate((OsclAny *)iInputBufferUnderConstruction);
                        }
                        iInputBufferUnderConstruction = NULL;
                    }
                    iObtainNewInputBuffer = true;

                }

                iFirstDataMsgAfterBOS = true;
                iKeepDroppingMsgsUntilMarkerBit = false;

                //Get state of OpenMAX decoder
                err = OMX_GetState(iOMXVideoDecoder, &sState);
                if (err != OMX_ErrorNone)
                {
                    //Error condition report
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoDecNode::DoReset(): Can't get State of decoder!"));
                    if (iResetInProgress)
                    {
                        // cmd is in current q
                        iResetInProgress = false;
                        if ((iCurrentCommand.size() > 0) &&
                                (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
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
                                        (0, "PVMFOMXVideoDecNode::DoReset() OMX comp is in loaded state. Wait for official callback to change variables etc."));
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
                                        (0, "PVMFOMXVideoDecNode::DoReset() Waiting for %d input and-or %d output buffers", iNumOutstandingInputBuffers, iNumOutstandingOutputBuffers));

                        return;
                    }

                    if (!iResetMsgSent)
                    {
                        // We can come here only if all buffers are already back
                        // Don't repeat any of this twice.
                        /* Change state to OMX_StateLoaded form OMX_StateIdle. */
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXVideoDecNode::DoReset() Changing Component State Idle->Loaded"));

                        err = OMX_SendCommand(iOMXVideoDecoder, OMX_CommandStateSet, OMX_StateLoaded, NULL);
                        if (err != OMX_ErrorNone)
                        {
                            //Error condition report
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                            (0, "PVMFOMXVideoDecNode::DoReset(): Can't send StateSet command to decoder!"));
                        }

                        iResetMsgSent = true;


                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXVideoDecNode::DoReset() freeing output buffers"));

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
                                                (0, "PVMFOMXVideoDecNode::DoReset() Cannot free output buffers "));

                                if (iResetInProgress)
                                {
                                    iResetInProgress = false;
                                    if ((iCurrentCommand.size() > 0) &&
                                            (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
                                       )
                                    {
                                        CommandComplete(iCurrentCommand, iCurrentCommand.front() , PVMFErrResource);
                                    }
                                }

                            }

                        }
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0, "PVMFOMXVideoDecNode::DoReset() freeing input buffers "));
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
                                                (0, "PVMFOMXVideoDecNode::DoReset() Cannot free input buffers "));

                                if (iResetInProgress)
                                {
                                    iResetInProgress = false;
                                    if ((iCurrentCommand.size() > 0) &&
                                            (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
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
                                    (0, "PVMFOMXVideoDecNode::DoReset(): Decoder is not in the Idle state!"));
                    if (iResetInProgress)
                    {
                        iResetInProgress = false;
                        if ((iCurrentCommand.size() > 0) &&
                                (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
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
            }//end of if (iOMXVideoDecoder != NULL)

            //delete all ports and notify observer.
            if (iInPort)
            {
                OSCL_DELETE(((PVMFOMXVideoDecPort*)iInPort));
                iInPort = NULL;
            }

            if (iOutPort)
            {
                OSCL_DELETE(((PVMFOMXVideoDecPort*)iOutPort));
                iOutPort = NULL;
            }

            iDataIn.Unbind();


            // Reset the metadata key list
            iAvailableMetadataKeys.clear();

            iEndOfDataReached = false;
            iIsEOSSentToComponent = false;
            iIsEOSReceivedFromComponent = false;


            iProcessingState = EPVMFOMXVideoDecNodeProcessingState_Idle;
            //logoff & go back to Created state.
            SetState(EPVMFNodeIdle);


            if (iResetInProgress)
            {
                iResetInProgress = false;
                if ((iCurrentCommand.size() > 0) &&
                        (iCurrentCommand.front().iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
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
void PVMFOMXVideoDecNode::DoRequestPort(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DoRequestPort() In"));
    //This node supports port request from any state

    //retrieve port tag.
    int32 tag;
    OSCL_String* portconfig;

    aCmd.PVMFOMXVideoDecNodeCommandBase::Parse(tag, portconfig);

    PVMFPortInterface* port = NULL;
    int32 leavecode = 0;
    //validate the tag...
    switch (tag)
    {
        case PVMF_OMX_VIDEO_DEC_NODE_PORT_TYPE_SOURCE:
            if (iInPort)
            {
                CommandComplete(iInputCommands, aCmd, PVMFFailure);
                break;
            }
            OSCL_TRY(leavecode, iInPort = OSCL_NEW(PVMFOMXVideoDecPort, ((int32)tag, this, "OMXVideoDecIn(Video)")););
            if (leavecode || iInPort == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoRequestPort: Error - Input port instantiation failed"));
                CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
                return;
            }
            port = iInPort;
            break;

        case PVMF_OMX_VIDEO_DEC_NODE_PORT_TYPE_SINK:
            if (iOutPort)
            {
                CommandComplete(iInputCommands, aCmd, PVMFFailure);
                break;
            }
            OSCL_TRY(leavecode, iOutPort = OSCL_NEW(PVMFOMXVideoDecPort, ((int32)tag, this, "OMXVideoDecOut(Video)")));
            if (leavecode || iOutPort == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoDecNode::DoRequestPort: Error - Output port instantiation failed"));
                CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
                return;
            }
            port = iOutPort;
            break;

        default:
            //bad port tag
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
            return;
    }

    //Return the port pointer to the caller.
    CommandComplete(iInputCommands, aCmd, PVMFSuccess, (OsclAny*)port);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::DoReleasePort(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVMFOMXVideoDecPort* port;
    aCmd.PVMFOMXVideoDecNodeCommandBase::Parse((PVMFPortInterface*&)port);

    if (port != NULL && (port == iInPort || port == iOutPort))
    {
        if (port == iInPort)
        {
            OSCL_DELETE(((PVMFOMXVideoDecPort*)iInPort));
            iInPort = NULL;
        }
        else
        {
            OSCL_DELETE(((PVMFOMXVideoDecPort*)iOutPort));
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
PVMFStatus PVMFOMXVideoDecNode::DoGetNodeMetadataKey(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DoGetNodeMetadataKey() In"));

    PVMFMetadataList* keylistptr = NULL;
    uint32 starting_index;
    int32 max_entries;
    char* query_key;

    aCmd.PVMFOMXVideoDecNodeCommand::Parse(keylistptr, starting_index, max_entries, query_key);

    // Check parameters
    if (keylistptr == NULL)
    {
        // The list pointer is invalid
        return PVMFErrArgument;
    }

    // Update the available metadata keys
    iAvailableMetadataKeys.clear();
    int32 leavecode = 0;
    OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY));

    if (iYUVWidth > 0 && iYUVHeight > 0)
    {
        leavecode = 0;
        OSCL_TRY(leavecode,
                 iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY);
                 iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY));
    }
    // add the profile, level and avgbitrate
    PVMF_MPEGVideoProfileType aProfile;
    PVMF_MPEGVideoLevelType aLevel;
    if (GetProfileAndLevel(aProfile, aLevel) == PVMFSuccess)
    {
        // For H263 this metadata will be available only after first frame decoding
        leavecode = 0;
        OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY));
        leavecode = 0;
        OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY));
    }
    leavecode = 0;
    OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY));


    if ((starting_index < 0) || (starting_index > (iAvailableMetadataKeys.size() - 1)) || max_entries == 0)
    {
        // Invalid starting index and/or max entries
        return PVMFErrArgument;
    }

    // Copy the requested keys
    uint32 num_entries = 0;
    int32 num_added = 0;
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
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                                     (0, "PVMFOMXVIDEODECNode::DoGetNodeMetadataKey() Memory allocation failure when copying metadata key"));
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
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetNodeMetadataKey() Memory allocation failure when copying metadata key"));
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
PVMFStatus PVMFOMXVideoDecNode::DoGetNodeMetadataValue(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetNodeMetadataValue() In"));

    PVMFMetadataList* keylistptr = NULL;
    Oscl_Vector<PvmiKvp, OsclMemAllocator>* valuelistptr = NULL;
    uint32 starting_index;
    int32 max_entries;

    aCmd.PVMFOMXVideoDecNodeCommand::Parse(keylistptr, valuelistptr, starting_index, max_entries);

    // Check the parameters
    if (keylistptr == NULL || valuelistptr == NULL)
    {
        return PVMFErrArgument;
    }

    uint32 numkeys = keylistptr->size();

    if (starting_index < 0 || starting_index > (numkeys - 1) || numkeys <= 0 || max_entries == 0)
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

        if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY) == 0) &&
                iYUVWidth > 0)
        {
            // Video width
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY) + 1; // for "codec-info/video/width;"
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
                    oscl_strncpy(KeyVal.key, PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY, oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY) + 1);
                    oscl_strncat(KeyVal.key, PVOMXVIDEODECMETADATA_SEMICOLON, oscl_strlen(PVOMXVIDEODECMETADATA_SEMICOLON));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                    KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                    // Copy the value
                    KeyVal.value.uint32_value = iYUVWidth;
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
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY) == 0) &&
                 iYUVHeight > 0)
        {
            // Video height
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY) + 1; // for "codec-info/video/height;"
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
                    oscl_strncpy(KeyVal.key, PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY, oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY) + 1);
                    oscl_strncat(KeyVal.key, PVOMXVIDEODECMETADATA_SEMICOLON, oscl_strlen(PVOMXVIDEODECMETADATA_SEMICOLON));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                    KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                    // Copy the value
                    KeyVal.value.uint32_value = iYUVHeight;
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
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY) == 0))
        {
            // Video profile
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY) + 1; // for "codec-info/video/profile;"
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR) + 1; // for "uint32" and NULL terminator

                // Allocate memory for the string
                leavecode = 0;
                OSCL_TRY(leavecode,
                         KeyVal.key = OSCL_ARRAY_NEW(char, KeyLen);
                        );

                if (leavecode == 0)
                {
                    PVMF_MPEGVideoProfileType aProfile;
                    PVMF_MPEGVideoLevelType aLevel;
                    if (GetProfileAndLevel(aProfile, aLevel) == PVMFSuccess)
                    {
                        // Copy the key string
                        oscl_strncpy(KeyVal.key, PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY, oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY) + 1);
                        oscl_strncat(KeyVal.key, PVOMXVIDEODECMETADATA_SEMICOLON, oscl_strlen(PVOMXVIDEODECMETADATA_SEMICOLON));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                        KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                        // Copy the value
                        KeyVal.value.uint32_value = (uint32)aProfile; // This is to be decided, who will interpret these value
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
                else
                {
                    // Memory allocation failed
                    KeyVal.key = NULL;
                    break;
                }
            }
        }
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY) == 0))
        {
            // Video level
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY) + 1; // for "codec-info/video/level;"
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR) + 1; // for "uint32" and NULL terminator

                // Allocate memory for the string
                leavecode = 0;
                OSCL_TRY(leavecode,
                         KeyVal.key = OSCL_ARRAY_NEW(char, KeyLen);
                        );

                if (leavecode == 0)
                {
                    PVMF_MPEGVideoProfileType aProfile;
                    PVMF_MPEGVideoLevelType aLevel;
                    if (GetProfileAndLevel(aProfile, aLevel) == PVMFSuccess)
                    {
                        // Copy the key string
                        oscl_strncpy(KeyVal.key, PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY, oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY) + 1);
                        oscl_strncat(KeyVal.key, PVOMXVIDEODECMETADATA_SEMICOLON, oscl_strlen(PVOMXVIDEODECMETADATA_SEMICOLON));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                        oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                        KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                        // Copy the value
                        KeyVal.value.uint32_value = (uint32)aLevel; // This is to be decided, who will interpret these value
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
                else
                {
                    // Memory allocation failed
                    KeyVal.key = NULL;
                    break;
                }
            }
        }
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY) == 0) &&
                 (iAvgBitrateValue > 0))
        {
            // Video average bitrate
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY) + 1; // for "codec-info/video/avgbitrate;"
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
                    oscl_strncpy(KeyVal.key, PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY, oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY) + 1);
                    oscl_strncat(KeyVal.key, PVOMXVIDEODECMETADATA_SEMICOLON, oscl_strlen(PVOMXVIDEODECMETADATA_SEMICOLON));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
                    KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                    // Copy the value
                    KeyVal.value.uint32_value = iAvgBitrateValue;
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
        else if ((oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY) == 0) &&
                 (((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H263 || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_M4V ||
                  ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H264 || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H264_MP4 ||
                  ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H264_RAW
                  || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_WMV))
        {
            // Format
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                KeyLen = oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY) + 1; // for "codec-info/video/format;"
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
                KeyLen += oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR) + 1; // for "char*" and NULL terminator

                uint32 valuelen = 0;
                switch (((PVMFOMXVideoDecPort*)iInPort)->iFormat)
                {
                    case PVMF_H264:
                        valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_H264_VIDEO)) + 1; // Value string plus one for NULL terminator
                        break;

                    case PVMF_H264_MP4:
                        valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_H264_VIDEO_MP4)) + 1; // Value string plus one for NULL terminator
                        break;

                    case PVMF_H264_RAW:
                        valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_H264_VIDEO_RAW)) + 1; // Value string plus one for NULL terminator
                        break;

                    case PVMF_M4V:
                        valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_M4V)) + 1; // Value string plus one for NULL terminator
                        break;

                    case PVMF_H263:
                        valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_H2631998)) + 1; // Value string plus one for NULL terminator
                        break;
                    case PVMF_WMV:
                        valuelen = oscl_strlen(_STRLIT_CHAR(PVMF_MIME_WMV)) + 1; // Value string plus one for NULL terminator
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
                    oscl_strncpy(KeyVal.key, PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY, oscl_strlen(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY) + 1);
                    oscl_strncat(KeyVal.key, PVOMXVIDEODECMETADATA_SEMICOLON, oscl_strlen(PVOMXVIDEODECMETADATA_SEMICOLON));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
                    oscl_strncat(KeyVal.key, PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR));
                    KeyVal.key[KeyLen-1] = NULL_TERM_CHAR;
                    // Copy the value
                    switch (((PVMFOMXVideoDecPort*)iInPort)->iFormat)
                    {
                        case PVMF_H264:
                            oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_H264_VIDEO), valuelen);
                            break;

                        case PVMF_H264_MP4:
                            oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_H264_VIDEO_MP4), valuelen);
                            break;

                        case PVMF_H264_RAW:
                            oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_H264_VIDEO_RAW), valuelen);
                            break;
                        case PVMF_M4V:
                            oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_M4V), valuelen);
                            break;
                        case PVMF_H263:
                            oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_H2631998), valuelen);
                            break;
                        case PVMF_WMV:
                            oscl_strncpy(KeyVal.value.pChar_value, _STRLIT_CHAR(PVMF_MIME_WMV), valuelen);
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
bool PVMFOMXVideoDecNode::ReleaseAllPorts()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ReleaseAllPorts() In"));

    if (iInPort)
    {
        iInPort->ClearMsgQueues();
        iInPort->Disconnect();
        OSCL_DELETE(((PVMFOMXVideoDecPort*)iInPort));
        iInPort = NULL;
    }

    if (iOutPort)
    {
        iOutPort->ClearMsgQueues();
        iOutPort->Disconnect();
        OSCL_DELETE(((PVMFOMXVideoDecPort*)iOutPort));
        iOutPort = NULL;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// Clean Up Decoder
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::DeleteOMXVideoDecoder()
{
    OMX_ERRORTYPE  err;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DeleteOMXVideoDecoder() In"));

    if (iOMXVideoDecoder != NULL)
    {
        /* Free Component handle. */
        err = PV_MasterOMX_FreeHandle(iOMXVideoDecoder);
        if (err != OMX_ErrorNone)
        {
            //Error condition report
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::DeleteOMXVideoDecoder(): Can't free decoder's handle!"));
        }
        iOMXVideoDecoder = NULL;

    }//end of if (iOMXVideoDecoder != NULL)


    return true;
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::ChangeNodeState(TPVMFNodeInterfaceState aNewState)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ChangeNodeState() Changing state from %d to %d", iInterfaceState, aNewState));
    iInterfaceState = aNewState;
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::freechunkavailable(OsclAny *aContext)
{

    // check context to see whether input or output buffer was returned to the mempool
    if (aContext == (OsclAny *) iInBufMemoryPool)
    {

        iNumOutstandingInputBuffers--;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::freechunkavailable() Memory chunk in INPUT mempool was deallocated, %d out of %d now available", iNumInputBuffers - iNumOutstandingInputBuffers, iNumInputBuffers));

        // notification only works once.
        // If there are multiple buffers coming back in a row, make sure to set the notification
        // flag in the mempool again, so that next buffer also causes notification
        iInBufMemoryPool->notifyfreechunkavailable(*this, aContext);

    }
    else if (aContext == (OsclAny *) iOutBufMemoryPool)
    {

        iNumOutstandingOutputBuffers--;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::freechunkavailable() Memory chunk in OUTPUT mempool was deallocated, %d out of %d now available", iNumOutputBuffers - iNumOutstandingOutputBuffers, iNumOutputBuffers));

        // notification only works once.
        // If there are multiple buffers coming back in a row, make sure to set the notification
        // flag in the mempool again, so that next buffer also causes notification
        iOutBufMemoryPool->notifyfreechunkavailable(*this, aContext);

    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::freechunkavailable() UNKNOWN mempool "));

    }

    // reschedule
    if (IsAdded())
        RunIfNotReady();


}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "0x%x PVMFOMXVideoDecNode::PortActivity: port=0x%x, type=%d",
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
                            (0, "PVMFOMXVideoDecNode::PortActivity: IncomingMsgQueueSize=%d", aActivity.iPort->IncomingMsgQueueSize()));
            if (aActivity.iPort->IncomingMsgQueueSize() == 1)
            {
                //wake up the AO to process the port activity event.
                RunIfNotReady();
            }
            break;

        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
            if (iProcessingState == EPVMFOMXVideoDecNodeProcessingState_WaitForOutgoingQueue)
            {
                iProcessingState = EPVMFOMXVideoDecNodeProcessingState_ReadyToDecode;
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
                            (0, "0x%x PVMFOMXVideoDecNode::PortActivity: Connected port is now ready", this));
            RunIfNotReady();
            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::DoCancelAllCommands(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DoCancelAllCommands"));

    //first cancel the current command if any
    {
        while (!iCurrentCommand.empty())
        {
            CommandComplete(iCurrentCommand, iCurrentCommand[0], PVMFErrCancelled);
        }
    }

    // next cancel all queued commands before this one
    for(int i=0; i< iInputCommands.size();)
    {
        PVMFOMXVideoDecNodeCommand* cmd = &iInputCommands[i];
        if ((aCmd.iId <= cmd->iId) && ((cmd->iId - aCmd.iId) < 0x80000000)) {
            ++i;
        } else {
            CommandComplete(iInputCommands, *cmd, PVMFErrCancelled);
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
void PVMFOMXVideoDecNode::DoCancelCommand(PVMFOMXVideoDecNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DoCancelCommand"));

    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.PVMFOMXVideoDecNodeCommandBase::Parse(id);

    //first check "current" command if any
    {
        PVMFOMXVideoDecNodeCommand* cmd = iCurrentCommand.FindById(id);
        if (cmd)
        {

            // if reset is being canceled:
            if (cmd->iCmd == PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_RESET)
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
        PVMFOMXVideoDecNodeCommand* cmd = iInputCommands.FindById(id, 1);
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
void PVMFOMXVideoDecNode::DoQueryUuid(PVMFOMXVideoDecNodeCommand& aCmd)
{
    //This node supports Query UUID from any state

    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;
    aCmd.PVMFOMXVideoDecNodeCommandBase::Parse(mimetype, uuidvec, exactmatch);

    //Try to match the input mimetype against any of
    //the custom interfaces for this node

    //Match against custom interface1...
    if (*mimetype == PVMF_OMX_VIDEO_DEC_NODE_CUSTOM1_MIMETYPE
            //also match against base mimetypes for custom interface1,
            //unless exactmatch is set.
            || (!exactmatch && *mimetype == PVMF_OMX_VIDEO_DEC_NODE_MIMETYPE)
            || (!exactmatch && *mimetype == PVMF_BASEMIMETYPE))
    {

        PVUuid uuid(PVMF_OMX_VIDEO_DEC_NODE_CUSTOM1_UUID);
        uuidvec->push_back(uuid);
    }
    CommandComplete(iInputCommands, aCmd, PVMFSuccess);
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::DoQueryInterface(PVMFOMXVideoDecNodeCommand&  aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::DoQueryInterface"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.PVMFOMXVideoDecNodeCommandBase::Parse(uuid, ptr);

    if (*uuid == PVUuid(PVMF_OMX_VIDEO_DEC_NODE_CUSTOM1_UUID))
    {
        addRef();
        *ptr = (PVMFOMXVideoDecNodeExtensionInterface*)this;
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
void PVMFOMXVideoDecNode::addRef()
{
    ++iExtensionRefCount;
}

/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::removeRef()
{
    --iExtensionRefCount;
}


//////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::HandleRepositioning()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::HandleRepositioning() IN"));


    // 1) Send Flush command to component for both input and output ports
    // 2) "Wait" until component flushes both ports
    // 3) Resume
    OMX_ERRORTYPE  err = OMX_ErrorNone;
    OMX_STATETYPE sState = OMX_StateInvalid;


    if (!iIsRepositioningRequestSentToComponent)
    {

        // first check the state (if executing or paused, continue)
        err = OMX_GetState(iOMXVideoDecoder, &sState);
        if (err != OMX_ErrorNone)
        {
            //Error condition report
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::HandleRepositioning(): Can't get State of decoder - trying to send reposition request!"));

            sState = OMX_StateInvalid;
            ReportErrorEvent(PVMFErrResourceConfiguration);
            ChangeNodeState(EPVMFNodeError);
            return false;
        }

        if ((sState != OMX_StateExecuting) && (sState != OMX_StatePause))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoDecNode::HandleRepositioning() Component State is not executing or paused, do not proceed with repositioning"));

            return true;
        }

        iIsRepositioningRequestSentToComponent = true; // prevent sending requests multiple times
        iIsInputPortFlushed = false;	// flag that will be set to true once component flushes the port
        iIsOutputPortFlushed = false;
        iDoNotSendOutputBuffersDownstreamFlag = true;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::HandleRepositioning() Sending Flush command to component"));

        // send command to flush all ports (arg is -1)

        err = OMX_SendCommand(iOMXVideoDecoder, OMX_CommandFlush, -1, NULL);
        if (err != OMX_ErrorNone)
        {
            //Error condition report
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoDecNode::HandleRepositioning(): Can't send flush command  - trying to send reposition request!"));

            sState = OMX_StateInvalid;
            ReportErrorEvent(PVMFErrResourceConfiguration);
            ChangeNodeState(EPVMFNodeError);
            return false;
        }
    }

    if (iIsRepositionDoneReceivedFromComponent)
    {

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoDecNode::HandleRepositioning() Component has flushed both ports, and is done repositioning"));

        iIsRepositioningRequestSentToComponent = false; // enable sending requests again
        iIsRepositionDoneReceivedFromComponent = false;
        iIsInputPortFlushed = false;
        iIsOutputPortFlushed = false;

        iDoNotSendOutputBuffersDownstreamFlag = false;
        return true;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoDecNode::HandleRepositioning() Component is not yet done repositioning "));

    return false;

}





/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoDecNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    PVUuid my_uuid(PVMF_OMX_VIDEO_DEC_NODE_CUSTOM1_UUID);
    if (uuid == my_uuid)
    {
        PVMFOMXVideoDecNodeExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMFOMXVideoDecNodeExtensionInterface*, this);
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

/////////////////////////////////////////////////////////////////////////////
uint32 PVMFOMXVideoDecNode::GetNumMetadataKeys(char* aQueryKeyString)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::GetNumMetadataKeys() called"));

    // Update the available metadata keys
    iAvailableMetadataKeys.clear();
    int32 errcode = 0;
    OSCL_TRY(errcode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY));

    if (iYUVWidth > 0 && iYUVHeight > 0)
    {
        errcode = 0;
        OSCL_TRY(errcode,
                 iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY);
                 iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY));
    }
    // add the profile, level and avgbitrate
    PVMF_MPEGVideoProfileType aProfile;
    PVMF_MPEGVideoLevelType aLevel;
    if (GetProfileAndLevel(aProfile, aLevel) == PVMFSuccess)
    {
        // For H263 this metadata will be available only after first frame decoding
        errcode = 0;
        OSCL_TRY(errcode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY));
        errcode = 0;
        OSCL_TRY(errcode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY));
    }
    errcode = 0;
    OSCL_TRY(errcode, iAvailableMetadataKeys.push_back(PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY));



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
void PVMFOMXVideoDecNode::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    OSCL_UNUSED_ARG(aObserver);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::setObserver()"));
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::setObserver() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


PVMFStatus PVMFOMXVideoDecNode::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::getParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigGetParametersSync(aIdentifier, aParameters, aNumParamElements, aContext);
}


PVMFStatus PVMFOMXVideoDecNode::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::releaseParameters()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigReleaseParameters(aParameters, aNumElements);
}


void PVMFOMXVideoDecNode::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::createContext()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::createContext() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVMFOMXVideoDecNode::setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext, PvmiKvp* aParameters, int aNumParamElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::setContextParameters()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumParamElements);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::setContextParameters() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVMFOMXVideoDecNode::DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DeleteContext()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DeleteContext() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVMFOMXVideoDecNode::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::setParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    // Complete the request synchronously
    DoCapConfigSetParameters(aParameters, aNumElements, aRetKVP);
}


PVMFCommandId PVMFOMXVideoDecNode::setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp*& aRetKVP, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::setParametersAsync()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumElements);
    OSCL_UNUSED_ARG(aRetKVP);
    OSCL_UNUSED_ARG(aContext);

    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::setParametersAsync() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
    return 0;
}


uint32 PVMFOMXVideoDecNode::getCapabilityMetric(PvmiMIOSession aSession)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::getCapabilityMetric()"));
    OSCL_UNUSED_ARG(aSession);
    // Not supported so return 0
    return 0;
}


PVMFStatus PVMFOMXVideoDecNode::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::verifyParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigVerifyParameters(aParameters, aNumElements);
}


/////////////////////////////////////////////////////////////////////////////
uint32 PVMFOMXVideoDecNode::GetNumMetadataValues(PVMFMetadataList& aKeyList)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::GetNumMetadataValues() called"));

    uint32 numkeys = aKeyList.size();

    if (numkeys <= 0)
    {
        // Don't do anything
        return 0;
    }

    // Count the number of value entries for the provided key list
    uint32 numvalentries = 0;
    PVMF_MPEGVideoProfileType aProfile;
    PVMF_MPEGVideoLevelType aLevel;
    for (uint32 lcv = 0; lcv < numkeys; lcv++)
    {
        if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_WIDTH_KEY) == 0) &&
                iYUVWidth > 0)
        {
            // Video width
            ++numvalentries;
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_HEIGHT_KEY) == 0) &&
                 iYUVHeight > 0)
        {
            // Video height
            ++numvalentries;
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_PROFILE_KEY) == 0) &&
                 (GetProfileAndLevel(aProfile, aLevel) == PVMFSuccess))

        {
            // Video profile
            ++numvalentries;
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_LEVEL_KEY) == 0) &&
                 (GetProfileAndLevel(aProfile, aLevel) == PVMFSuccess))
        {
            // Video level
            ++numvalentries;
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_AVGBITRATE_KEY) == 0) &&
                 (iAvgBitrateValue > 0))

        {
            // Video average bitrate
            if (iAvgBitrateValue > 0)
                ++numvalentries;
        }
        else if ((oscl_strcmp(aKeyList[lcv].get_cstr(), PVOMXVIDEODECMETADATA_CODECINFO_VIDEO_FORMAT_KEY) == 0) &&
                 (
                     ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_WMV ||
                     ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_M4V || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H263 || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H264 || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H264_MP4 || ((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_H264_RAW))
        {
            // Format
            ++numvalentries;
        }
    }

    return numvalentries;
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::GetNodeMetadataKeys(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, uint32 starting_index, int32 max_entries, char* query_key, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNodeCommand::GetNodeMetadataKeys() called"));

    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommand::Construct(aSessionId, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_GETNODEMETADATAKEY, &aKeyList, starting_index, max_entries, query_key, aContext);
    return QueueCommandL(cmd);
}

/////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoDecNode::GetNodeMetadataValues(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 starting_index, int32 max_entries, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNodeCommand::GetNodeMetadataValue() called"));

    PVMFOMXVideoDecNodeCommand cmd;
    cmd.PVMFOMXVideoDecNodeCommand::Construct(aSessionId, PVMFOMXVideoDecNodeCommand::PVOMXVIDEODEC_NODE_CMD_GETNODEMETADATAVALUE, &aKeyList, &aValueList, starting_index, max_entries, aContext);
    return QueueCommandL(cmd);
}

// From PVMFMetadataExtensionInterface
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::ReleaseNodeMetadataKeys(PVMFMetadataList& , uint32 , uint32)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ReleaseNodeMetadataKeys() called"));
    //nothing needed-- there's no dynamic allocation in this node's key list
    return PVMFSuccess;
}

// From PVMFMetadataExtensionInterface
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoDecNode::ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 start, uint32 end)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::ReleaseNodeMetadataValues() called"));

    if (aValueList.size() == 0 || start < 0 || start > end)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::ReleaseNodeMetadataValues() Invalid start/end index"));
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
PVMFStatus PVMFOMXVideoDecNode::DoCapConfigGetParametersSync(PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() In"));
    OSCL_UNUSED_ARG(aContext);

    // Initialize the output parameters
    aNumParamElements = 0;
    aParameters = NULL;

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aIdentifier);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aIdentifier, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/decoder")) < 0) || compcount < 3)
    {
        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/render")) < 0) || compcount != 3)
        {
            // First 3 component should be "x-pvmf/video/decoder" and there must
            // be at least three components
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigReleaseParameters() Unsupported key"));
            return PVMFErrArgument;
        }
    }

    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/render")) >= 0)
    {
        aParameters = (PvmiKvp*)oscl_malloc(PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS * sizeof(PvmiKvp));
        if (aParameters == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for KVP failed"));
            return PVMFErrNoMemory;
        }
        oscl_memset(aParameters, 0, PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS*sizeof(PvmiKvp));
        // Allocate memory for the key strings in each KVP
        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
        if (memblock == NULL)
        {
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for key string failed"));
            return PVMFErrNoMemory;
        }
        oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS*PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
        // Assign the key string buffer to each KVP
        int32 j;
        for (j = 0; j < PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS; ++j)
        {
            aParameters[j].key = memblock + (j * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE);
        }
        // Copy the requested info
        for (j = 0; j < PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS; ++j)
        {
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/video/render/"), 20);
            oscl_strncat(aParameters[j].key, PVOMXVideoDecNodeConfigRenderKeys[j].iString, oscl_strlen(PVOMXVideoDecNodeConfigRenderKeys[j].iString));
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";valtype=uint32_value"), 21);
            aParameters[j].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;

            // Copy the requested info
            switch (j)
            {
                case 0:	// "width"
                    // Return current value
                    aParameters[j].value.uint32_value = iNewWidth;
                    break;
                case 1: // "height"
                    aParameters[j].value.uint32_value = iNewHeight;
                    break;
                default:
                    break;
            }
        }

        aNumParamElements = PVOMXVIDEODECNODECONFIG_RENDER_NUMKEYS;
    }
    else if (compcount == 3)
    {
        // Since key is "x-pvmf/video/decoder" return all
        // nodes available at this level. Ignore attribute
        // since capability is only allowed

        // Allocate memory for the KVP list
        aParameters = (PvmiKvp*)oscl_malloc(PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS * sizeof(PvmiKvp));
        if (aParameters == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for KVP failed"));
            return PVMFErrNoMemory;
        }
        oscl_memset(aParameters, 0, PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS*sizeof(PvmiKvp));
        // Allocate memory for the key strings in each KVP
        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
        if (memblock == NULL)
        {
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for key string failed"));
            return PVMFErrNoMemory;
        }
        oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS*PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
        // Assign the key string buffer to each KVP
        int32 j;
        for (j = 0; j < PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS; ++j)
        {
            aParameters[j].key = memblock + (j * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE);
        }
        // Copy the requested info
        for (j = 0; j < PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS; ++j)
        {
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/video/decoder/"), 21);
            oscl_strncat(aParameters[j].key, PVOMXVideoDecNodeConfigBaseKeys[j].iString, oscl_strlen(PVOMXVideoDecNodeConfigBaseKeys[j].iString));
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type="), 6);
            switch (PVOMXVideoDecNodeConfigBaseKeys[j].iType)
            {
                case PVMI_KVPTYPE_AGGREGATE:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_AGGREGATE_STRING), oscl_strlen(PVMI_KVPTYPE_AGGREGATE_STRING));
                    break;

                case PVMI_KVPTYPE_POINTER:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_POINTER_STRING), oscl_strlen(PVMI_KVPTYPE_POINTER_STRING));
                    break;

                case PVMI_KVPTYPE_VALUE:
                default:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_VALUE_STRING), oscl_strlen(PVMI_KVPTYPE_VALUE_STRING));
                    // Now append the valtype param
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";valtype="), 9);
                    switch (PVOMXVideoDecNodeConfigBaseKeys[j].iValueType)
                    {
                        case PVMI_KVPVALTYPE_BITARRAY32:
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BITARRAY32_STRING), oscl_strlen(PVMI_KVPVALTYPE_BITARRAY32_STRING));
                            break;

                        case PVMI_KVPVALTYPE_UINT32:
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
                            break;

                        case PVMI_KVPVALTYPE_BOOL:
                        default:
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
                            break;
                    }
                    break;
            }

            aParameters[j].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;
        }

        aNumParamElements = PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS;
    }
    else
    {
        // Retrieve the fourth component from the key string
        pv_mime_string_extract_type(3, aIdentifier, compstr);

        for (int32 vdeccomp4ind = 0; vdeccomp4ind < PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS; ++vdeccomp4ind)
        {
            // Go through each video dec component string at 4th level
            if (pv_mime_strcmp(compstr, (char*)(PVOMXVideoDecNodeConfigBaseKeys[vdeccomp4ind].iString)) >= 0)
            {
                if (vdeccomp4ind == 3)
                {
                    // "x-pvmf/video/decoder/h263"
                    if (compcount == 4)
                    {
                        // Return list of H263 settings. Ignore the
                        // attribute since capability is only allowed

                        // Allocate memory for the KVP list
                        aParameters = (PvmiKvp*)oscl_malloc(PVOMXVIDEODECNODECONFIG_H263_NUMKEYS * sizeof(PvmiKvp));
                        if (aParameters == NULL)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for KVP failed"));
                            return PVMFErrNoMemory;
                        }
                        oscl_memset(aParameters, 0, PVOMXVIDEODECNODECONFIG_H263_NUMKEYS*sizeof(PvmiKvp));
                        // Allocate memory for the key strings in each KVP
                        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_H263_NUMKEYS * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
                        if (memblock == NULL)
                        {
                            oscl_free(aParameters);
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for key string failed"));
                            return PVMFErrNoMemory;
                        }
                        oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_H263_NUMKEYS*PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
                        // Assign the key string buffer to each KVP
                        int32 j;
                        for (j = 0; j < PVOMXVIDEODECNODECONFIG_H263_NUMKEYS; ++j)
                        {
                            aParameters[j].key = memblock + (j * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE);
                        }
                        // Copy the requested info
                        for (j = 0; j < PVOMXVIDEODECNODECONFIG_H263_NUMKEYS; ++j)
                        {
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/video/decoder/h263/"), 26);
                            oscl_strncat(aParameters[j].key, PVOMXVideoDecNodeConfigH263Keys[j].iString, oscl_strlen(PVOMXVideoDecNodeConfigH263Keys[j].iString));
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type=value;valtype="), 20);
                            switch (PVOMXVideoDecNodeConfigH263Keys[j].iValueType)
                            {
                                case PVMI_KVPVALTYPE_RANGE_UINT32:
                                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
                                    break;

                                case PVMI_KVPVALTYPE_UINT32:
                                default:
                                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
                                    break;
                            }

                            aParameters[j].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;
                        }

                        aNumParamElements = PVOMXVIDEODECNODECONFIG_H263_NUMKEYS;
                    }
                    else if (compcount > 4)
                    {
                        // Retrieve the fifth component from the key string
                        pv_mime_string_extract_type(4, aIdentifier, compstr);

                        for (int32 vdeccomp5ind = 0; vdeccomp5ind < PVOMXVIDEODECNODECONFIG_H263_NUMKEYS; ++vdeccomp5ind)
                        {
                            if (pv_mime_strcmp(compstr, (char*)(PVOMXVideoDecNodeConfigH263Keys[vdeccomp5ind].iString)) >= 0)
                            {
                                // Determine what is requested
                                PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
                                if (reqattr == PVMI_KVPATTR_UNKNOWN)
                                {
                                    // Default is current setting
                                    reqattr = PVMI_KVPATTR_CUR;
                                }

                                // Return the requested info
                                PVMFStatus retval = DoGetH263DecoderParameter(aParameters, aNumParamElements, vdeccomp5ind, reqattr);
                                if (retval != PVMFSuccess)
                                {
                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Retrieving H.263 parameter failed"));
                                    return retval;
                                }

                                // Break out of the for(vdeccomp5ind) loop
                                break;
                            }
                        }
                    }
                    else
                    {
                        // Right now video dec node doesn't support more than 5 components
                        // for the key sub-string so error out
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Unsupported key"));
                        return PVMFErrArgument;
                    }
                }
                else if (vdeccomp4ind == 4)
                {
                    // "x-pvmf/video/decoder/m4v"
                    if (compcount == 4)
                    {
                        // Return list of M4v settings. Ignore the
                        // attribute since capability is only allowed

                        // Allocate memory for the KVP list
                        aParameters = (PvmiKvp*)oscl_malloc(PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS * sizeof(PvmiKvp));
                        if (aParameters == NULL)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for KVP failed"));
                            return PVMFErrNoMemory;
                        }
                        oscl_memset(aParameters, 0, PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS*sizeof(PvmiKvp));
                        // Allocate memory for the key strings in each KVP
                        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
                        if (memblock == NULL)
                        {
                            oscl_free(aParameters);
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Memory allocation for key string failed"));
                            return PVMFErrNoMemory;
                        }
                        oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS*PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
                        // Assign the key string buffer to each KVP
                        int32 j;
                        for (j = 0; j < PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS; ++j)
                        {
                            aParameters[j].key = memblock + (j * PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE);
                        }
                        // Copy the requested info
                        for (j = 0; j < PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS; ++j)
                        {
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/video/decoder/m4v/"), 25);
                            oscl_strncat(aParameters[j].key, PVOMXVideoDecNodeConfigM4VKeys[j].iString, oscl_strlen(PVOMXVideoDecNodeConfigM4VKeys[j].iString));
                            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type=value;valtype="), 20);
                            switch (PVOMXVideoDecNodeConfigM4VKeys[j].iValueType)
                            {
                                case PVMI_KVPVALTYPE_RANGE_UINT32:
                                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
                                    break;

                                case PVMI_KVPVALTYPE_UINT32:
                                default:
                                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
                                    break;
                            }

                            aParameters[j].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;
                        }

                        aNumParamElements = PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS;
                    }
                    else if (compcount > 4)
                    {
                        // Retrieve the fifth component from the key string
                        pv_mime_string_extract_type(4, aIdentifier, compstr);

                        for (int32 vdeccomp5ind = 0; vdeccomp5ind < PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS; ++vdeccomp5ind)
                        {
                            if (pv_mime_strcmp(compstr, (char*)(PVOMXVideoDecNodeConfigM4VKeys[vdeccomp5ind].iString)) >= 0)
                            {
                                // Determine what is requested
                                PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
                                if (reqattr == PVMI_KVPATTR_UNKNOWN)
                                {
                                    // Default is current setting
                                    reqattr = PVMI_KVPATTR_CUR;
                                }

                                // Return the requested info
                                PVMFStatus retval = DoGetM4VDecoderParameter(aParameters, aNumParamElements, vdeccomp5ind, reqattr);
                                if (retval != PVMFSuccess)
                                {
                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Retrieving M4v parameter failed"));
                                    return retval;
                                }

                                // Break out of the for(vdeccomp5ind) loop
                                break;
                            }
                        }
                    }
                    else
                    {
                        // Right now video dec node doesn't support more than 5 components
                        // for the key sub-string so error out
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Unsupported key"));
                        return PVMFErrArgument;
                    }
                }
                else if ((vdeccomp4ind == 0) || // "postproc_enable",
                         (vdeccomp4ind == 1) ||	// "postproc_type"
                         (vdeccomp4ind == 2) ||	// "dropframe_enable"
                         (vdeccomp4ind == 5)	// "format_type"
                        )
                {
                    if (compcount == 4)
                    {
                        // Determine what is requested
                        PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
                        if (reqattr == PVMI_KVPATTR_UNKNOWN)
                        {
                            reqattr = PVMI_KVPATTR_CUR;
                        }

                        // Return the requested info
                        PVMFStatus retval = DoGetVideoDecNodeParameter(aParameters, aNumParamElements, vdeccomp4ind, reqattr);
                        if (retval != PVMFSuccess)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Retrieving video dec node parameter failed"));
                            return retval;
                        }
                    }
                    else
                    {
                        // Right now videodec node doesn't support more than 4 components
                        // for this sub-key string so error out
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Unsupported key"));
                        return PVMFErrArgument;
                    }
                }

                // Breakout of the for(vdeccomp4ind) loop
                break;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Out"));
    if (aNumParamElements == 0)
    {
        // If no one could get the parameter, return error
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigGetParametersSync() Unsupported key"));
        return PVMFFailure;
    }
    else
    {
        return PVMFSuccess;
    }
}


PVMFStatus PVMFOMXVideoDecNode::DoCapConfigReleaseParameters(PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigReleaseParameters() In"));

    if (aParameters == NULL || aNumElements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigReleaseParameters() KVP list is NULL or number of elements is 0"));
        return PVMFErrArgument;
    }

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aParameters[0].key);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aParameters[0].key, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/decoder")) < 0) || compcount < 3)
    {
        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/render")) < 0) || compcount != 4)
        {
            // First 3 component should be "x-pvmf/video/decoder" and there must
            // be at least three components
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigReleaseParameters() Unsupported key"));
            return PVMFErrArgument;
        }
    }

    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/decoder")) >= 0)
    {
        // Retrieve the third component from the key string
        pv_mime_string_extract_type(2, aParameters[0].key, compstr);

        // Go through each KVP and release memory for value if allocated from heap
        for (int32 i = 0; i < aNumElements; ++i)
        {
            // Next check if it is a value type that allocated memory
            PvmiKvpType kvptype = GetTypeFromKeyString(aParameters[i].key);
            if (kvptype == PVMI_KVPTYPE_VALUE || kvptype == PVMI_KVPTYPE_UNKNOWN)
            {
                PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameters[i].key);
                if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigReleaseParameters() Valtype not specified in key string"));
                    return PVMFErrArgument;
                }

                if (keyvaltype == PVMI_KVPVALTYPE_CHARPTR && aParameters[i].value.pChar_value != NULL)
                {
                    oscl_free(aParameters[i].value.pChar_value);
                    aParameters[i].value.pChar_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_KSV && aParameters[i].value.key_specific_value != NULL)
                {
                    oscl_free(aParameters[i].value.key_specific_value);
                    aParameters[i].value.key_specific_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_RANGE_UINT32 && aParameters[i].value.key_specific_value != NULL)
                {
                    range_uint32* rui32 = (range_uint32*)aParameters[i].value.key_specific_value;
                    aParameters[i].value.key_specific_value = NULL;
                    oscl_free(rui32);
                }

            }
        }
    }
    // Video dec node allocated its key strings in one chunk so just free the first key string ptr
    oscl_free(aParameters[0].key);

    // Free memory for the parameter list
    oscl_free(aParameters);
    aParameters = NULL;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigReleaseParameters() Out"));
    return PVMFSuccess;
}


void PVMFOMXVideoDecNode::DoCapConfigSetParameters(PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() In"));

    if (aParameters == NULL || aNumElements < 1)
    {
        if (aParameters)
        {
            aRetKVP = aParameters;
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Passed in parameter invalid"));
        return;
    }

    // Go through each parameter
    for (int32 paramind = 0; paramind < aNumElements; ++paramind)
    {
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(aParameters[paramind].key);
        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/decoder")) < 0) || compcount < 4)
        {
            // First 3 components should be "x-pvmf/video/decoder" and there must
            // be at least four components
            aRetKVP = &aParameters[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Unsupported key"));
            return;
        }

        if (compcount == 4)
        {
            // Verify and set the passed-in video dec node setting
            PVMFStatus retval = DoVerifyAndSetVideoDecNodeParameter(aParameters[paramind], true);
            if (retval != PVMFSuccess)
            {
                aRetKVP = &aParameters[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Setting parameter %d failed", paramind));
                return;
            }
        }
        else if (compcount == 5)
        {
            // Determine the 4th level component
            pv_mime_string_extract_type(3, aParameters[paramind].key, compstr);
            if (pv_mime_strcmp(compstr, _STRLIT_CHAR("h263")) >= 0)
            {
                // Verify and set the passed-in H.263 decoder setting
                PVMFStatus retval = DoVerifyAndSetH263DecoderParameter(aParameters[paramind], true);
                if (retval != PVMFSuccess)
                {
                    aRetKVP = &aParameters[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Setting parameter %d failed", paramind));
                    return;
                }
            }
            else if (pv_mime_strcmp(compstr, _STRLIT_CHAR("m4v")) >= 0)
            {
                // Verify and set the passed-in M4v decoder setting
                PVMFStatus retval = DoVerifyAndSetM4VDecoderParameter(aParameters[paramind], true);
                if (retval != PVMFSuccess)
                {
                    aRetKVP = &aParameters[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Setting parameter %d failed", paramind));
                    return;
                }
            }
            else
            {
                // Unknown key sub-string
                aRetKVP = &aParameters[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Unsupported key"));
                return;
            }
        }
        else
        {
            // Do not support more than 5 components right now
            aRetKVP = &aParameters[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Unsupported key"));
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigSetParameters() Out"));
}

/* This function finds a nal from the SC's, moves the bitstream pointer to the beginning of the NAL unit, returns the
	size of the NAL, and at the same time, updates the remaining size in the bitstream buffer that is passed in */
int32 PVMFOMXVideoDecNode::GetNAL_OMXNode(uint8** bitstream, int* size)
{
    int i = 0;
    int j;
    uint8* nal_unit = *bitstream;
    int count = 0;

    /* find SC at the beginning of the NAL */
    while (nal_unit[i++] == 0 && i < *size)
    {
    }

    if (nal_unit[i-1] == 1)
    {
        *bitstream = nal_unit + i;
    }
    else
    {
        j = *size;
        *size = 0;
        return j;  // no SC at the beginning, not supposed to happen
    }

    j = i;

    /* found the SC at the beginning of the NAL, now find the SC at the beginning of the next NAL */
    while (i < *size)
    {
        if (count == 2 && nal_unit[i] == 0x01)
        {
            i -= 2;
            break;
        }

        if (nal_unit[i])
            count = 0;
        else
            count++;
        i++;
    }

    *size -= i;
    return (i -j);
}

/* TODO: Functions of the decoder are directly being called here.
   Should this be a place where we add an interface that is to be implemented
   by the 3rd party codecs to negotiate the capability and config ???
*/
PVMFStatus PVMFOMXVideoDecNode::DoCapConfigVerifyParameters(PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() In"));

    if (aParameters == NULL || aNumElements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Go through each parameter
    for (int32 paramind = 0; paramind < aNumElements; ++paramind)
    {
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(aParameters[paramind].key);
        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/video/decoder")) < 0) || compcount < 4)
        {
            if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/media/format_specific_info")) < 0) || compcount < 3)
            {
                // First 3 components should be "x-pvmf/media/format_specific_info" and there must
                // be at least three components
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Unsupported key"));
                return PVMFErrArgument;
            }
            else
            {
                pvVideoConfigParserInputs aInputs;
                pvVideoConfigParserOutputs aOutputs;

                aInputs.inPtr = (uint8*)(aParameters->value.key_specific_value);
                aInputs.inBytes = (int32)aParameters->capacity;
                aInputs.iMimeType = iNodeConfig.iMimeType;

                int16 status;
                status = pv_video_config_parser(&aInputs, &aOutputs);
                if (status != 0)
                {
                    return PVMFErrNotSupported;
                }
                iNewWidth = aOutputs.width;
                iNewHeight = aOutputs.height;

                return PVMFSuccess;
            }
        }
        else
        {
            if (compcount == 4)
            {
                // Verify and set the passed-in video dec node setting
                PVMFStatus retval = DoVerifyAndSetVideoDecNodeParameter(aParameters[paramind], false);
                if (retval != PVMFSuccess)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Setting parameter %d failed", paramind));
                    return retval;
                }
            }
            else if (compcount == 5)
            {
                // Determine the 4th level component
                pv_mime_string_extract_type(3, aParameters[paramind].key, compstr);
                if (pv_mime_strcmp(compstr, _STRLIT_CHAR("h263")) >= 0)
                {
                    // Verify and set the passed-in H.263 decoder setting
                    PVMFStatus retval = DoVerifyAndSetH263DecoderParameter(aParameters[paramind], false);
                    if (retval != PVMFSuccess)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Setting parameter %d failed", paramind));
                        return retval;
                    }
                }
                else if (pv_mime_strcmp(compstr, _STRLIT_CHAR("m4v")) >= 0)
                {
                    // Verify and set the passed-in M4v decoder setting
                    PVMFStatus retval = DoVerifyAndSetM4VDecoderParameter(aParameters[paramind], false);
                    if (retval != PVMFSuccess)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Setting parameter %d failed", paramind));
                        return retval;
                    }
                }
                else
                {
                    // Unknown key sub-string
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Unsupported key"));
                    return PVMFErrArgument;
                }
            }
            else
            {
                // Do not support more than 5 components right now
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Unsupported key"));
                return PVMFErrArgument;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoCapConfigVerifyParameters() Out"));
    return PVMFSuccess;
}



PVMFStatus PVMFOMXVideoDecNode::DoGetVideoDecNodeParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetVideoDecNodeParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetVideoDecNodeParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));
    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetVideoDecNodeParameter() Memory allocation for key string failed"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/video/decoder/"), 21);
    oscl_strncat(aParameters[0].key, PVOMXVideoDecNodeConfigBaseKeys[aIndex].iString, oscl_strlen(PVOMXVideoDecNodeConfigBaseKeys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype="), 20);
    switch (PVOMXVideoDecNodeConfigBaseKeys[aIndex].iValueType)
    {
        case PVMI_KVPVALTYPE_BITARRAY32:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BITARRAY32_STRING), oscl_strlen(PVMI_KVPVALTYPE_BITARRAY32_STRING));
            break;

        case PVMI_KVPVALTYPE_KSV:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_KSV_STRING), oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
            break;

        case PVMI_KVPVALTYPE_BOOL:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
            break;

        case PVMI_KVPVALTYPE_UINT32:
        default:
            if (reqattr == PVMI_KVPATTR_CAP)
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            else
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
            }
            break;
    }
    aParameters[0].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case 0:	// "postproc_enable"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.bool_value = iNodeConfig.iPostProcessingEnable;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.bool_value = PVOMXVIDEODECNODE_CONFIG_POSTPROCENABLE_DEF;
            }

            break;

        case 1:	// "postproc_type"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iNodeConfig.iPostProcessingMode;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVOMXVIDEODECNODE_CONFIG_POSTPROCTYPE_DEF;
            }

            break;

        case 2:	// "dropframe_enable"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.bool_value = iNodeConfig.iDropFrame;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.bool_value = PVOMXVIDEODECNODE_CONFIG_DROPFRAMEENABLE_DEF;
            }

            break;

        case 5: //"format-type"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iNodeConfig.iMimeType;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVOMXVIDEODECNODE_CONFIG_MIMETYPE_DEF;
            }

            break;

        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetVideoDecNodeParameter() Invalid index to video dec node parameter"));
            return PVMFErrArgument;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetVideoDecNodeParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVMFOMXVideoDecNode::DoGetH263DecoderParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));
    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() Memory allocation for key string failed"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/video/decoder/h263/"), 26);
    oscl_strncat(aParameters[0].key, PVOMXVideoDecNodeConfigH263Keys[aIndex].iString, oscl_strlen(PVOMXVideoDecNodeConfigH263Keys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype="), 20);
    switch (PVOMXVideoDecNodeConfigH263Keys[aIndex].iValueType)
    {
        case PVMI_KVPVALTYPE_RANGE_UINT32:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            break;

        case PVMI_KVPVALTYPE_UINT32:
        default:
            if (reqattr == PVMI_KVPATTR_CAP)
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            else
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
            }
            break;
    }
    aParameters[0].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case 0:	// "maxbitstreamframesize"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iH263MaxBitstreamFrameSize;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_MIN;
                rui32->max = PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 1:	// "maxdimension"
        {
            range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
            if (rui32 == NULL)
            {
                oscl_free(aParameters[0].key);
                oscl_free(aParameters);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() Memory allocation for range uint32 failed"));
                return PVMFErrNoMemory;
            }

            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                rui32->min = iH263MaxWidth;
                rui32->max = iH263MaxHeight;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                rui32->min = PVOMXVIDEODECNODE_CONFIG_H263MAXWIDTH_DEF;
                rui32->max = PVOMXVIDEODECNODE_CONFIG_H263MAXHEIGHT_DEF;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            else
            {
                // Return capability
                rui32->min = PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MIN;
                rui32->max = PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
        }
        break;

        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() Invalid index to H.263 decoder parameter"));
            return PVMFErrArgument;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetH263DecoderParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVMFOMXVideoDecNode::DoGetM4VDecoderParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));
    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() Memory allocation for key string failed"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE*sizeof(char));
    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/video/decoder/m4v/"), 25);
    oscl_strncat(aParameters[0].key, PVOMXVideoDecNodeConfigM4VKeys[aIndex].iString, oscl_strlen(PVOMXVideoDecNodeConfigM4VKeys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype="), 20);
    switch (PVOMXVideoDecNodeConfigM4VKeys[aIndex].iValueType)
    {
        case PVMI_KVPVALTYPE_RANGE_UINT32:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            break;

        case PVMI_KVPVALTYPE_UINT32:
        default:
            if (reqattr == PVMI_KVPATTR_CAP)
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            else
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
            }
            break;
    }
    aParameters[0].key[PVOMXVIDEODECNODECONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case 0:	// "maxbitstreamframesize"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iM4VMaxBitstreamFrameSize;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_MIN;
                rui32->max = PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 1:	// "maxdimension"
        {
            range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
            if (rui32 == NULL)
            {
                oscl_free(aParameters[0].key);
                oscl_free(aParameters);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() Memory allocation for range uint32 failed"));
                return PVMFErrNoMemory;
            }

            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                rui32->min = iM4VMaxWidth;
                rui32->max = iM4VMaxHeight;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                rui32->min = PVOMXVIDEODECNODE_CONFIG_M4VMAXWIDTH_DEF;
                rui32->max = PVOMXVIDEODECNODE_CONFIG_M4VMAXHEIGHT_DEF;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            else
            {
                // Return capability
                rui32->min = PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MIN;
                rui32->max = PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
        }
        break;

        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() Invalid index to H.263 decoder parameter"));
            return PVMFErrArgument;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoGetM4VDecoderParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Valtype in key string unknown"));
        return PVMFErrArgument;
    }
    // Retrieve the fourth component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(3, aParameter.key, compstr);

    int32 vdeccomp4ind = 0;
    for (vdeccomp4ind = 0; vdeccomp4ind < PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS; ++vdeccomp4ind)
    {
        // Go through each component string at 4th level
        if (pv_mime_strcmp(compstr, (char*)(PVOMXVideoDecNodeConfigBaseKeys[vdeccomp4ind].iString)) >= 0)
        {
            // Break out of the for loop
            break;
        }
    }

    if (vdeccomp4ind == PVOMXVIDEODECNODECONFIG_BASE_NUMKEYS || vdeccomp4ind == 3 || vdeccomp4ind == 4)
    {
        // Match couldn't be found or non-leaf node specified
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Unsupported key or non-leaf node"));
        return PVMFErrArgument;
    }

    // Verify the valtype
    if (keyvaltype != PVOMXVideoDecNodeConfigBaseKeys[vdeccomp4ind].iValueType)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Valtype does not match for key"));
        return PVMFErrArgument;
    }

    switch (vdeccomp4ind)
    {
        case 0: // "postproc_enable"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                iNodeConfig.iPostProcessingEnable = aParameter.value.bool_value;
            }
            break;

        case 1: // "postproc_type"
            // Nothing to validate since it is bitarray32
            // Change the config if to set
            if (aSetParam)
            {
                iNodeConfig.iPostProcessingMode = aParameter.value.uint32_value;
                if (iNodeConfig.iPostProcessingEnable && iOMXVideoDecoder)
                {

                }
            }
            break;

        case 2: // "dropframe_enable"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                if (iInterfaceState == EPVMFNodeStarted || iInterfaceState == EPVMFNodePaused)
                {
                    // This setting cannot be changed when decoder has been initialized
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Setting cannot be changed while started or paused"));
                    return PVMFErrInvalidState;
                }

                iNodeConfig.iDropFrame = aParameter.value.bool_value;
            }
            break;

        case 5: // "format-type"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                if (iInterfaceState == EPVMFNodeStarted || iInterfaceState == EPVMFNodePaused)
                {
                    // This setting cannot be changed when decoder has been initialized
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Setting cannot be changed while started or paused"));
                    return PVMFErrInvalidState;
                }

                iNodeConfig.iMimeType = aParameter.value.uint32_value;
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Invalid index for video dec node parameter"));
            return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Valtype in key string unknown"));
        return PVMFErrArgument;
    }
    // Retrieve the fifth component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(4, aParameter.key, compstr);

    int32 vdeccomp5ind = 0;
    for (vdeccomp5ind = 0; vdeccomp5ind < PVOMXVIDEODECNODECONFIG_H263_NUMKEYS; ++vdeccomp5ind)
    {
        // Go through each component string at 5th level
        if (pv_mime_strcmp(compstr, (char*)(PVOMXVideoDecNodeConfigH263Keys[vdeccomp5ind].iString)) >= 0)
        {
            // Break out of the for loop
            break;
        }
    }

    if (vdeccomp5ind == PVOMXVIDEODECNODECONFIG_H263_NUMKEYS)
    {
        // Match couldn't be found or non-leaf node specified
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Unsupported key or non-leaf node"));
        return PVMFErrArgument;
    }

    // Verify the valtype
    if (keyvaltype != PVOMXVideoDecNodeConfigH263Keys[vdeccomp5ind].iValueType)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Valtype does not match for key"));
        return PVMFErrArgument;
    }

    switch (vdeccomp5ind)
    {
        case 0: // "maxbitstreamframesize"
            // Check if within range
            if (aParameter.value.uint32_value < PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_MIN ||
                    aParameter.value.uint32_value > PVOMXVIDEODECNODE_CONFIG_H263MAXBITSTREAMFRAMESIZE_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Invalid value for maxbitstreamframesize"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                if (iInterfaceState == EPVMFNodeStarted || iInterfaceState == EPVMFNodePaused)
                {
                    // This setting cannot be changed when decoder has been initialized
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Setting cannot be changed while started or paused"));
                    return PVMFErrInvalidState;
                }

                iH263MaxBitstreamFrameSize = aParameter.value.uint32_value;
            }
            break;

        case 1: // "maxdimension"
        {
            range_uint32* rui32 = (range_uint32*)aParameter.value.key_specific_value;
            if (rui32 == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() ksv for maxdimension is NULL"));
                return PVMFErrArgument;
            }

            // Check if within range
            if (rui32->min < PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MIN ||
                    rui32->min > PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MAX ||
                    rui32->max < PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MIN ||
                    rui32->max > PVOMXVIDEODECNODE_CONFIG_H263MAXDIMENSION_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Invalid range for maxdimension"));
                return PVMFErrArgument;
            }

            // Change the config if to set
            if (aSetParam)
            {
                if (iInterfaceState == EPVMFNodeStarted || iInterfaceState == EPVMFNodePaused)
                {
                    // This setting cannot be changed when decoder has been initialized
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Setting cannot be changed while started or paused"));
                    return PVMFErrInvalidState;
                }

                iH263MaxWidth = rui32->min;
                iH263MaxHeight = rui32->max;
            }
        }
        break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Invalid index for H.263 decoder parameter"));
            return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetH263DecoderParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Valtype in key string unknown"));
        return PVMFErrArgument;
    }
    // Retrieve the fifth component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(4, aParameter.key, compstr);

    int32 vdeccomp5ind = 0;
    for (vdeccomp5ind = 0; vdeccomp5ind < PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS; ++vdeccomp5ind)
    {
        // Go through each component string at 5th level
        if (pv_mime_strcmp(compstr, (char*)(PVOMXVideoDecNodeConfigM4VKeys[vdeccomp5ind].iString)) >= 0)
        {
            // Break out of the for loop
            break;
        }
    }

    if (vdeccomp5ind == PVOMXVIDEODECNODECONFIG_M4V_NUMKEYS)
    {
        // Match couldn't be found or non-leaf node specified
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Unsupported key or non-leaf node"));
        return PVMFErrArgument;
    }

    // Verify the valtype
    if (keyvaltype != PVOMXVideoDecNodeConfigM4VKeys[vdeccomp5ind].iValueType)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Valtype does not match for key"));
        return PVMFErrArgument;
    }

    switch (vdeccomp5ind)
    {
        case 0: // "maxbitstreamframesize"
            // Check if within range
            if (aParameter.value.uint32_value < PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_MIN ||
                    aParameter.value.uint32_value > PVOMXVIDEODECNODE_CONFIG_M4VMAXBITSTREAMFRAMESIZE_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Invalid value for maxbitstreamframesize"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                if (iInterfaceState == EPVMFNodeStarted || iInterfaceState == EPVMFNodePaused)
                {
                    // This setting cannot be changed when decoder has been initialized
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Setting cannot be changed while started or paused"));
                    return PVMFErrInvalidState;
                }

                iM4VMaxBitstreamFrameSize = aParameter.value.uint32_value;
            }
            break;

        case 1: // "maxdimension"
        {
            range_uint32* rui32 = (range_uint32*)aParameter.value.key_specific_value;
            if (rui32 == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() ksv for maxdimension is NULL"));
                return PVMFErrArgument;
            }

            // Check if within range
            if (rui32->min < PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MIN ||
                    rui32->min > PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MAX ||
                    rui32->max < PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MIN ||
                    rui32->max > PVOMXVIDEODECNODE_CONFIG_M4VMAXDIMENSION_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Invalid range for maxdimension"));
                return PVMFErrArgument;
            }

            // Change the config if to set
            if (aSetParam)
            {
                if (iInterfaceState == EPVMFNodeStarted || iInterfaceState == EPVMFNodePaused)
                {
                    // This setting cannot be changed when decoder has been initialized
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetVideoDecNodeParameter() Setting cannot be changed while started or paused"));
                    return PVMFErrInvalidState;
                }

                iM4VMaxWidth = rui32->min;
                iM4VMaxHeight = rui32->max;
            }
        }
        break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Invalid index for M4v decoder parameter"));
            return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::DoVerifyAndSetM4VDecoderParameter() Out"));
    return PVMFSuccess;
}



PVMFStatus PVMFOMXVideoDecNode::GetProfileAndLevel(PVMF_MPEGVideoProfileType& aProfile, PVMF_MPEGVideoLevelType& aLevel)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFOMXVideoDecNode::GetProfileAndLevel() In"));

    if (NULL == iOMXVideoDecoder)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::GetProfileAndLevel() iVideoDecoder is Null"));
        aProfile = PV_MPEG_VIDEO_RESERVED_PROFILE;
        aLevel	= PV_MPEG_VIDEO_LEVEL_UNKNOWN;
        return PVMFFailure;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::GetProfileAndLevel() iVideoDecoder is Null"));
    aProfile = PV_MPEG_VIDEO_RESERVED_PROFILE;
    aLevel	= PV_MPEG_VIDEO_LEVEL_UNKNOWN;
    return PVMFFailure;


}


/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoDecNode::LogDiagnostics()
{
    if (iDiagnosticsLogged == false)
    {
        iDiagnosticsLogged = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFOMXVideoDecNode - Number of YUV Frames Sent = %d", iSeqNum));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFOMXVideoDecNode - TS of last decoded video frame = %d", iOutTimeStamp));
    }
}



// DEFINITIONS for parsing the config information & sequence header for WMV

#define GetUnalignedDword( pb, dw ) \
            (dw) = ((uint32) *(pb + 3) << 24) + \
                   ((uint32) *(pb + 2) << 16) + \
                   ((uint16) *(pb + 1) << 8) + *pb;

#define GetUnalignedDwordEx( pb, dw )   GetUnalignedDword( pb, dw ); (pb) += sizeof(uint32);
#define LoadDWORD( dw, p )  GetUnalignedDwordEx( p, dw )
#ifndef MAKEFOURCC_WMC
#define MAKEFOURCC_WMC(ch0, ch1, ch2, ch3) \
        ((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |   \
        ((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))

#define mmioFOURCC_WMC(ch0, ch1, ch2, ch3)  MAKEFOURCC_WMC(ch0, ch1, ch2, ch3)
#endif

#define FOURCC_WMV3     mmioFOURCC_WMC('W','M','V','3')
#define FOURCC_WMV2     mmioFOURCC_WMC('W','M','V','2')
#define FOURCC_WMVA		mmioFOURCC_WMC('W','M','V','A')

//For WMV3
enum { NOT_WMV3 = -1, WMV3_SIMPLE_PROFILE, WMV3_MAIN_PROFILE, WMV3_PC_PROFILE, WMV3_SCREEN };

//For WMVA
#define ASFBINDING_SIZE                   1   // size of ASFBINDING is 1 byte
#define SC_SEQ          0x0F
#define SC_ENTRY        0x0E

bool PVMFOMXVideoDecNode::VerifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    // call this in case of WMV format
    //return true;

    if (((PVMFOMXVideoDecPort*)iInPort)->iFormat == PVMF_WMV)
    {

        //verify bitrate
        if (pv_mime_strcmp(aParameters->key, PVMF_BITRATE_VALUE_KEY) == 0)
        {
            if (((PVMFOMXVideoDecPort*)iOutPort)->verifyConnectedPortParametersSync(PVMF_BITRATE_VALUE_KEY, &(aParameters->value.uint32_value)) != PVMFSuccess)
            {
                return false;
            }
            return true;
        }
        else if (pv_mime_strcmp(aParameters->key, PVMF_FRAMERATE_VALUE_KEY) == 0)
        {
            if (((PVMFOMXVideoDecPort*)iOutPort)->verifyConnectedPortParametersSync(PVMF_FRAMERATE_VALUE_KEY, &(aParameters->value.uint32_value)) != PVMFSuccess)
            {
                return false;
            }
            return true;
        }
        else if (pv_mime_strcmp(aParameters->key, PVMF_FORMAT_SPECIFIC_INFO_KEY) < 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::VerifyParametersSync() - Unsupported Key"));
            return true;
        }

        // pConfig points to format specific info and sequence header.
        uint8 *pConfig = (uint8*)(aParameters->value.key_specific_value);
        uint8 *pData;
        uint32 dwdat;
        uint32 NewCompression;
        uint32 NewSeqHeader;
        uint32 NewProfile, NewFrameRate, NewBitRate;

        // We are interested in the following (and will extract it)
        //	1. Version (WMV9 or WMV8 etc.) (from format specific info)
        //  2. picture dimensions // from format specific info
        //  3. interlaced YUV411 /sprite content is not supported (from sequence header)
        //  4. framerate / bitrate information (from sequence header)

        pData = pConfig + 15; // position ptr to Width & Height

        LoadDWORD(dwdat, pData);
        iNewWidth = dwdat;
        LoadDWORD(dwdat, pData);
        iNewHeight = dwdat;

        if (((iNewWidth != (uint32)iYUVWidth) || (iNewHeight != (uint32)iYUVHeight)) && iOutPort != NULL)
        {
            // see if downstream node can handle the re-sizing
            int32 errcode;
            OsclRefCounterMemFrag yuvFsiMemfrag;
            OSCL_TRY(errcode, yuvFsiMemfrag = iYuvFsiFragmentAlloc.get());


            OSCL_FIRST_CATCH_ANY(errcode, PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoDecNode::VerifyParametersSync() Failed to allocate memory for verifyParametersSync FSI")));
            if (errcode == 0)
            {
                PVMFYuvFormatSpecificInfo0 * fsiInfo = (PVMFYuvFormatSpecificInfo0*)yuvFsiMemfrag.getMemFragPtr();
                fsiInfo->video_format = PVMF_YUV420_PLANAR;
                fsiInfo->uid = PVMFYuvFormatSpecificInfo0_UID;
                fsiInfo->display_width = iNewWidth;
                fsiInfo->display_height = iNewHeight;
                fsiInfo->width = (iNewWidth + 3) & -4;
                fsiInfo->height = iNewHeight;

            }
            else
                return false;

            if (((PVMFOMXVideoDecPort*)iOutPort)->verifyConnectedPortParametersSync(PVMF_FORMAT_SPECIFIC_INFO_KEY, &yuvFsiMemfrag) != PVMFSuccess)
                return false;

        }

        pData += 4; //position ptr to Compression type

        LoadDWORD(dwdat, pData);
        NewCompression = dwdat;

        if (NewCompression != FOURCC_WMV2 &&
                NewCompression != FOURCC_WMV3 &&
                NewCompression != FOURCC_WMVA)
            return false;


        // Check sequence header
        switch (NewCompression)
        {
            case FOURCC_WMV3:
            {
                pData = pConfig + 11 + 40; //sizeof(BITMAPINFOHEADER); // position to sequence header

                LoadDWORD(dwdat, pData);
                NewSeqHeader = dwdat; // this is little endian read sequence header

                uint32 YUV411flag, Spriteflag;

                // For FOURCC_WMV3
                uint32 YUV411;
                uint32 SpriteMode;
                uint32 LoopFilter;
                uint32 Xintra8Switch;
                uint32 MultiresEnabled;
                uint32 X16bitXform;
                uint32 UVHpelBilinear;
                uint32 ExtendedMvMode;
                uint32 DQuantCodingOn;
                uint32 XformSwitch;
                uint32 DCTTable_MB_ENABLED;
                uint32 SequenceOverlap;
                uint32 StartCode;
                uint32 PreProcRange;
                uint32 NumBFrames;
                uint32 ExplicitSeqQuantizer;
                uint32 Use3QPDZQuantizer = 0;
                uint32 ExplicitFrameQuantizer = 0;


                bool bValidProfile = true;

                NewProfile = (NewSeqHeader & 0xC0) >> 6; // 0 - simple , 1- main, 3 - complex, 2-forbidden

                if (NewProfile == WMV3_PC_PROFILE)
                    return false;

                YUV411flag = (NewSeqHeader & 0x20) >> 5;
                Spriteflag = (NewSeqHeader & 0x10) >> 4;
                if ((YUV411flag != 0) || (Spriteflag != 0))
                    return false;

                YUV411				= (uint32)YUV411flag;
                SpriteMode			= (uint32)Spriteflag;
                LoopFilter			= (NewSeqHeader & 0x800) >> 11;
                Xintra8Switch		= (NewSeqHeader & 0x400) >> 10;
                MultiresEnabled		= (NewSeqHeader & 0x200) >> 9;
                X16bitXform			= (NewSeqHeader & 0x100) >> 8;
                UVHpelBilinear		= (NewSeqHeader & 0x800000) >> 23;
                ExtendedMvMode		= (NewSeqHeader & 0x400000) >> 22;
                DQuantCodingOn		= (NewSeqHeader & 0x300000) >> 20;
                XformSwitch			= (NewSeqHeader & 0x80000) >> 19;
                DCTTable_MB_ENABLED	= (NewSeqHeader & 0x40000) >> 18;
                SequenceOverlap		= (NewSeqHeader & 0x20000) >> 17;
                StartCode			= (NewSeqHeader & 0x10000) >> 16;
                PreProcRange			= (NewSeqHeader & 0x80000000) >> 31;
                NumBFrames			= (NewSeqHeader & 0x70000000) >> 28;
                ExplicitSeqQuantizer	= (NewSeqHeader & 0x8000000) >> 27;
                if (ExplicitSeqQuantizer)
                    Use3QPDZQuantizer = (NewSeqHeader & 0x4000000) >> 26;
                else
                    ExplicitFrameQuantizer = (NewSeqHeader & 0x4000000) >> 26;

                NewFrameRate = (NewSeqHeader & 0x0E) >> 1 ; // from 2 to 30 fps (in steps of 4)
                NewFrameRate = 4 * NewFrameRate + 2; // (in fps)

                NewBitRate = (((NewSeqHeader & 0xF000) >> 24) | ((NewSeqHeader & 0x01) << 8));  // from 32 to 2016 kbps in steps of 64kbps
                NewBitRate = 64 * NewBitRate + 32; // (in kbps)

                // Verify Profile
                if (!SpriteMode)
                {
                    if (NewProfile == WMV3_SIMPLE_PROFILE)
                    {
                        bValidProfile = (Xintra8Switch == 0) &&
                                        (X16bitXform == 1) &&
                                        (UVHpelBilinear == 1) &&
                                        (StartCode == 0) &&
                                        (LoopFilter == 0) &&
                                        (YUV411 == 0) &&
                                        (MultiresEnabled == 0) &&
                                        (DQuantCodingOn == 0) &&
                                        (NumBFrames == 0) &&
                                        (PreProcRange == 0);

                    }
                    else if (NewProfile == WMV3_MAIN_PROFILE)
                    {
                        bValidProfile = (Xintra8Switch == 0) &&
                                        (X16bitXform == 1);
                    }
                    else if (NewProfile == WMV3_PC_PROFILE)
                    {
                        // no feature restrictions for complex profile.
                    }

                    if (!bValidProfile)
                    {
                        return false;
                    }
                }
                else
                {
                    if (!Xintra8Switch   &&
                            !DCTTable_MB_ENABLED  &&
                            !YUV411 &&
                            !LoopFilter &&
                            !ExtendedMvMode &&
                            !MultiresEnabled &&
                            !UVHpelBilinear &&
                            !DQuantCodingOn &&
                            !XformSwitch &&
                            !StartCode &&
                            !PreProcRange &&
                            !ExplicitSeqQuantizer &&
                            !Use3QPDZQuantizer &&
                            !ExplicitFrameQuantizer)
                        return true;
                    else
                        return false;
                }
            }
            break;
            case FOURCC_WMVA:
            {
                pData = pConfig + 11 + 40 + ASFBINDING_SIZE; //sizeof(BITMAPINFOHEADER); // position to sequence header

                LoadDWORD(dwdat, pData);
                NewSeqHeader = dwdat; // this is little endian read sequence header

                int32 iPrefix;
                //ignore start code prefix
                iPrefix = NewSeqHeader & 0xFF;
                if (iPrefix != 0) return false;
                iPrefix = (NewSeqHeader & 0xFF00) >> 8;
                if (iPrefix != 0) return false;
                iPrefix = (NewSeqHeader & 0xFF0000) >> 16;
                if (iPrefix != 1) return false;
                iPrefix = (NewSeqHeader & 0xFF000000) >> 24;
                if (iPrefix != SC_SEQ) return false;

                LoadDWORD(dwdat, pData);
                NewSeqHeader = dwdat;

                NewProfile = (NewSeqHeader & 0xC0) >> 6;
                if (NewProfile != 3)
                    return false;
                pData += 3;
                LoadDWORD(dwdat, pData);
                NewSeqHeader = dwdat;
                //ignore start code prefix
                iPrefix = NewSeqHeader & 0xFF;
                if (iPrefix != 0) return false;
                iPrefix = (NewSeqHeader & 0xFF00) >> 8;
                if (iPrefix != 0) return false;
                iPrefix = (NewSeqHeader & 0xFF0000) >> 16;
                if (iPrefix != 1) return false;
                iPrefix = (NewSeqHeader & 0xFF000000) >> 24;
                if (iPrefix != SC_ENTRY) return false;
            }
            break;

            case FOURCC_WMV2:
                break;

            default:
                return false;
        }

    } // end of if(format == PVMF_WMV)
    return true;
}

#if OMX_DEBUG_LOG
#undef PVLOGGER_LOGMSG
#define PVLOGGER_LOGMSG(IL, LOGGER, LEVEL, MESSAGE) OSCL_UNUSED_ARG(LOGGER);
#endif

