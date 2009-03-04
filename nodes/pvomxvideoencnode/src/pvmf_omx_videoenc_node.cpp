/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
 * Copyright (C) 2008 HTC Inc.
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

//#define LOG_NDEBUG 0
#define LOG_TAG "PVMFOMXVideoEncNode"
#include <utils/Log.h>

#ifndef PVMF_OMX_VIDEOENC_NODE_H_INCLUDED
#include "pvmf_omx_videoenc_node.h"
#endif
#ifndef OSCL_ERROR_CODES_H_INCLUDED
#include "oscl_error_codes.h"
#endif
#ifndef PVMF_OMX_VIDEOENC_PORT_H_INCLUDED
#include "pvmf_omx_videoenc_port.h"
#endif
#ifndef PVMF_OMX_VIDEOENC_NODE_FACTORY_H_INCLUDED
#include "pvmf_omx_videoenc_node_factory.h"
#endif
#ifndef PVMF_OMX_VIDEOENC_NODE_TYPES_H_INCLUDED
#include "pvmf_omx_videoenc_node_types.h"
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

#include "omx_core.h"
#include "pvmf_omx_videoenc_callbacks.h"     //used for thin AO in encoder's callbacks
#include "pv_omxcore.h"
#include "pv_omxmastercore.h"


static const OMX_U32 OMX_SPEC_VERSION = 0x00000101;
#define CONFIG_VERSION_SIZE(param) \
       param.nVersion.nVersion = OMX_SPEC_VERSION; \
       param.nSize = sizeof(param);

#define PVOMXVIDEOENC_MEDIADATA_CHUNKSIZE 128

#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m);
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m);
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

static const uint32 DEFAULT_VOL_HEADER_LENGTH = 28;
static const uint8 DEFAULT_VOL_HEADER[DEFAULT_VOL_HEADER_LENGTH] =
{
	0x00, 0x00, 0x01, 0xB0, 0x08, 0x00, 0x00, 0x01,
	0xB5, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x01, 0x20, 0x00, 0x84, 0x40, 0x07, 0xA8, 0x50,
	0x20, 0xF0, 0xA3, 0x1F
};

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
    LOGV("CallbackEventHandler");
    PVMFOMXVideoEncNode *Node = (PVMFOMXVideoEncNode *) aAppData;

    if ( Node->IsComponentMultiThreaded() )
    {
        // allocate the memory for the callback event specific data
        //EventHandlerSpecificData* ED = (EventHandlerSpecificData*) oscl_malloc(sizeof (EventHandlerSpecificData));
        EventHandlerSpecificData* ED = (EventHandlerSpecificData*) Node->iThreadSafeHandlerEventHandler->iMemoryPool->allocate(sizeof (EventHandlerSpecificData));

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
    PVMFOMXVideoEncNode *Node = (PVMFOMXVideoEncNode *) aAppData;
    if ( Node->IsComponentMultiThreaded() )
    {
        // allocate the memory for the callback event specific data
        //EmptyBufferDoneSpecificData* ED = (EmptyBufferDoneSpecificData*) oscl_malloc(sizeof (EmptyBufferDoneSpecificData));
        EmptyBufferDoneSpecificData* ED = (EmptyBufferDoneSpecificData*) Node->iThreadSafeHandlerEmptyBufferDone->iMemoryPool->allocate(sizeof (EmptyBufferDoneSpecificData));

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
    PVMFOMXVideoEncNode *Node = (PVMFOMXVideoEncNode *) aAppData;
    if ( Node->IsComponentMultiThreaded() )
    {
        // allocate the memory for the callback event specific data
        //FillBufferDoneSpecificData* ED = (FillBufferDoneSpecificData*) oscl_malloc(sizeof (FillBufferDoneSpecificData));
        FillBufferDoneSpecificData* ED = (FillBufferDoneSpecificData*) Node->iThreadSafeHandlerFillBufferDone->iMemoryPool->allocate(sizeof (FillBufferDoneSpecificData));

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

////////////////////////////////////////////////////////////////////////////
//              PVMFOMXVideoEncNodeFactory implementation
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFNodeInterface* PVMFOMXVideoEncNodeFactory::CreateVideoEncNode(int32 aPriority)
{
    int32 err = 0;
    PVMFOMXVideoEncNode* node = NULL;

    OSCL_TRY(err,
             node = OSCL_NEW(PVMFOMXVideoEncNode, (aPriority));
             if (!node)
             OSCL_LEAVE(OsclErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err, return NULL;);

    return (PVMFNodeInterface*)node;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNodeFactory::DeleteVideoEncNode(PVMFNodeInterface* aNode)
{
    if (!aNode)
        return false;

    OSCL_DELETE(aNode);
    aNode = NULL;
    return true;
}

////////////////////////////////////////////////////////////////////////////
//              PVMFOMXVideoEncNode implementation
////////////////////////////////////////////////////////////////////////////
PVMFOMXVideoEncNode::PVMFOMXVideoEncNode(int32 aPriority) :
        OsclTimerObject(aPriority, "PVMFOMXVideoEncNode"),
        iMediaBufferMemPool(PVVIDENC_MEDIADATA_POOLNUM, PVVIDENC_MEDIABUFFER_CHUNKSIZE),
        iMediaDataMemPoolOld(PVVIDENC_MEDIADATA_POOLNUM, PVVIDENC_MEDIADATA_CHUNKSIZE)
{
    mInputBufferRefCount = 0;
    iInterfaceState = EPVMFNodeCreated;

	// CB Functions to serve OpenMAX Encoder
    iThreadSafeHandlerEventHandler = NULL;
    iThreadSafeHandlerEmptyBufferDone = NULL;
    iThreadSafeHandlerFillBufferDone = NULL;
	memset(&iCallbacks, 0, sizeof(iCallbacks));

	// Handle of OMX Component
	iOMXVideoEncoder = NULL;

	// Current State of the component
	OMX_STATETYPE iCurrentEncoderState = OMX_StateInvalid;

	// Shared pointer for Media Msg.Input buffer
	//PVMFSharedMediaDataPtr iDataIn; //Init this value ?

	//EOS control flags
	iIsEOSSentToComponent = false;
	iIsEOSReceivedFromComponent = false;

	// OMX COMPONENT CAPABILITY RELATED MEMBERS
	iOMXComponentSupportsExternalOutputBufferAlloc = false;
	iOMXComponentSupportsExternalInputBufferAlloc = false;
	iOMXComponentSupportsMovableInputBuffers = false;
	iIsOMXComponentMultiThreaded = true;
	iOMXComponentSupportsPartialFrames = false;
	iOMXComponentCanHandleIncompleteFrames = true;

	// DYNAMIC PORT RE-CONFIGURATION
	iInputPortIndex = 0;
	iOutputPortIndex = 0;
	memset(&iParamPort, 0, sizeof(iParamPort));
	iPortIndexForDynamicReconfig = 0;
	iSecondPortReportedChange = false;
	iDynamicReconfigInProgress = false;
	iSecondPortToReconfig = 0;

	// OUTPUT BUFFER RELATED MEMBERS
	iMediaDataMemPool = NULL;
	iOutBufMemoryPool = NULL;
	iOMXComponentOutputBufferSize = 0;
	iOutputAllocSize = 0;
	iNumOutputBuffers = 0;
	iNumOutstandingOutputBuffers = 0;
	iDoNotSendOutputBuffersDownstreamFlag = false;
	iOutputBuffersFreed = false;
	ipPrivateData = NULL;

	// INPUT BUFFER RELATED MEMBERS
	iInBufMemoryPool = NULL;
	iOMXComponentInputBufferSize = 0;
	iInputAllocSize = 0;
	iNumInputBuffers = 0;
	iNumOutstandingInputBuffers = 0;
	iDoNotSaveInputBuffersFlag = false;
	iInputBuffersFreed = false;

	iOMXComponentInputYUVFormat = PVMF_YUV420;
	iInputBufferToResendToComponent = NULL;

	iProcessingState = EPVMFOMXVideoEncNodeProcessingState_Idle;

	iResetInProgress = false;
	iResetMsgSent = false;

	// Time stamp to be used on output buffer
	iOutTimeStamp = 0;

	// input buffer fragmentation etc.
	iCopyPosition = 0;
	iFragmentSizeRemainingToCopy = 0;
	iIsNewDataFragment = true;

	// partial frame assembly logic flags
	iObtainNewInputBuffer = true;
	iKeepDroppingMsgsUntilMarkerBit = false;

	iInputBufferUnderConstruction = NULL;

	// input data info
	iCurrFragNum = 0;
	iCodecSeqNum = 0;
	iInPacketSeqNum = 0;
	iInTimestamp = 0;
	iInDuration = 0;
	iInNumFrags = 0;
	iCurrentMsgMarkerBit = 1;

	iEndOfDataReached = false;
	iEndOfDataTimestamp = 0;

    iExtensionRefCount = 0;

    iSeqNum = 0;

	// Allocate memory for VOL header
	uint refCounterSize = oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
	uint size = refCounterSize + DEFAULT_VOL_HEADER_LENGTH;
	uint8 *memBuffer = NULL;

    int32 err;
    OSCL_TRY(err,
             //Create the input command queue
             iCmdQueue.Construct(PVMF_OMX_VIDEOENC_NODE_CMD_ID_START, PVMF_OMX_VIDEOENC_NODE_CMD_QUEUE_RESERVE);
             iCurrentCmd.Construct(0, 1); // There's only 1 current command

             //Create the port vector.
             iInPort.Construct(PVMF_OMX_VIDEOENC_NODE_PORT_VECTOR_RESERVE);
             iOutPort.Construct(PVMF_OMX_VIDEOENC_NODE_PORT_VECTOR_RESERVE);

             // Create media data allocator
             iMediaDataAlloc = OSCL_NEW(PVMFSimpleMediaBufferCombinedAlloc, (&iMediaBufferMemPool));
             if (!iMediaDataAlloc)
             OSCL_LEAVE(OsclErrNoMemory);

			 memBuffer = (uint8*)iAlloc.allocate(size);
			 if (!memBuffer)
			 {
				 OSCL_LEAVE(PVMFErrNoMemory);
			 }

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

	// Save default VOL header
	oscl_memset(memBuffer, 0, DEFAULT_VOL_HEADER_LENGTH);
	OsclMemoryFragment volHeader;
	OsclRefCounter* refCounter = new (memBuffer) OsclRefCounterDA(memBuffer,
		(OsclDestructDealloc*)&iAlloc);
	memBuffer += refCounterSize;
	volHeader.ptr = memBuffer;
	oscl_memcpy(volHeader.ptr, (OsclAny*)DEFAULT_VOL_HEADER, DEFAULT_VOL_HEADER_LENGTH);
	volHeader.len = DEFAULT_VOL_HEADER_LENGTH;
	iVolHeader = OsclRefCounterMemFrag(volHeader, refCounter, DEFAULT_VOL_HEADER_LENGTH);

    ConstructEncoderParams();

    iLogger = PVLogger::GetLoggerObject("PVMFOMXVideoEncNode");

}

////////////////////////////////////////////////////////////////////////////
PVMFOMXVideoEncNode::~PVMFOMXVideoEncNode()
{
	//Clearup encoder
    DeleteVideoEncoder();

	// Cleanup callback AOs and Mempools
	if(iThreadSafeHandlerEventHandler)
	{
		OSCL_DELETE(iThreadSafeHandlerEventHandler);
		iThreadSafeHandlerEventHandler = NULL;
	}
	if(iThreadSafeHandlerEmptyBufferDone)
	{
		OSCL_DELETE(iThreadSafeHandlerEmptyBufferDone);
		iThreadSafeHandlerEmptyBufferDone = NULL;
	}
	if(iThreadSafeHandlerFillBufferDone)
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
	if(iInBufMemoryPool)
	{
		iInBufMemoryPool->removeRef();
		iInBufMemoryPool = NULL;
	}

    if (iMediaDataAlloc)
    {
        OSCL_DELETE(iMediaDataAlloc);
        iMediaDataAlloc = NULL;
    }

    while (!iInPort.empty())
        iInPort.Erase(&iInPort.front());
    while (!iOutPort.empty())
        iOutPort.Erase(&iOutPort.front());

    // Clean up command queues
    while (!iCmdQueue.empty())
    {
        CommandComplete(iCmdQueue, iCmdQueue[0], PVMFFailure);
    }

    while (!iCurrentCmd.empty())
    {
        CommandComplete(iCurrentCmd, iCurrentCmd[0], PVMFFailure);
    }

    Cancel();
    SetState(EPVMFNodeIdle);
    ThreadLogoff();

	//Release Input buffer
	iDataIn.Unbind();

}

//=================================================================================================
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFOMXVideoEncNode::ThreadLogon()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
                AddToScheduler();
            SetState(EPVMFNodeIdle);
            return PVMFSuccess;

        default:
            return PVMFErrInvalidState;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFOMXVideoEncNode::ThreadLogoff()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            if (IsAdded())
                RemoveFromScheduler();
            iLogger = NULL;
            SetState(EPVMFNodeCreated);
            return PVMFSuccess;

        default:
            return PVMFErrInvalidState;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFOMXVideoEncNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::GetCapability"));

    aNodeCapability.iInputFormatCapability.push_back(PVMF_YUV420);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_M4V);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_H263);
    aNodeCapability.iCanSupportMultipleOutputPorts = false;
    aNodeCapability.iCanSupportMultipleInputPorts = false;
    aNodeCapability.iHasMaxNumberOfPorts = true;
    aNodeCapability.iMaxNumberOfPorts = 2;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFPortIter* PVMFOMXVideoEncNode::GetPorts(const PVMFPortFilter* aFilter)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::GetPorts"));
    OSCL_UNUSED_ARG(aFilter);//port filter is not implemented.

    int32 err = 0;
    PVMFVideoEncPortVector* port = NULL;
    OSCL_TRY(err,
             port = OSCL_NEW(PVMFVideoEncPortVector, ());
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
                         LOG_ERR((0, "PVMFOMXVideoEncNode::GetPorts: Error - Out of memory"));
                        );

    return port;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::QueryUUID(PVMFSessionId aSession,
        const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::QueryUUID"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYUUID,
                  aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::QueryInterface(PVMFSessionId aSession,
        const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::QueryInterface"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYINTERFACE,
                  aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::RequestPort(PVMFSessionId aSession,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::RequestPort: aPortTag=%d", aPortTag));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_REQUESTPORT,
                  aPortTag, aPortConfig, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::ReleasePort(PVMFSessionId aSession,
        PVMFPortInterface& aPort,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::ReleasePort"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Init(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Init"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_INIT, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Prepare(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Prepare"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PREPARE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Start(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Start"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_START, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Stop(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Stop"));
    PVMFVideoEncNodeCommand cmd;

    // DoFlush before DoStop
    cmd.Construct(aSession, PVMF_GENERIC_NODE_FLUSH, aContext);
    QueueCommandL(cmd);

    cmd.Construct(aSession, PVMF_GENERIC_NODE_STOP, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Flush(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Flush"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_FLUSH, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Pause(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Pause"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PAUSE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::Reset(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Reset"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RESET, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::CancelAllCommands"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELALLCOMMANDS, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFOMXVideoEncNode::CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::CancelCommand: aCmdId=%d", aCmdId));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELCOMMAND, aCmdId, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0, "0x%x PVMFOMXVideoEncNode::PortActivity: port=0x%x, type=%d",
		this, aActivity.iPort, aActivity.iType));

	switch(aActivity.iType)
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
				(0, "PVMFOMXVideoEncNode::PortActivity: IncomingMsgQueueSize=%d", aActivity.iPort->IncomingMsgQueueSize()));
			if(aActivity.iPort->IncomingMsgQueueSize() == 1)
			{
				//wake up the AO to process the port activity event.
				RunIfNotReady();
			}
			break;

		case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
			if (iProcessingState == EPVMFOMXVideoEncNodeProcessingState_WaitForOutgoingQueue)
			{
				iProcessingState = EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode;
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
				(0, "0x%x PVMFOMXVideoEncNode::PortActivity: Connected port is now ready", this));
			RunIfNotReady();
			break;

		default:
			break;
	}

}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFOMXVideoEncNode::addRef()
{
    ++iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFOMXVideoEncNode::removeRef()
{
    if (iExtensionRefCount > 0)
        --iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    if (uuid == PVMp4H263EncExtensionUUID)
    {
        PVMp4H263EncExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMp4H263EncExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else if (uuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = 	OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else
    {
        iface = NULL;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetNumLayers(uint32 aNumLayers)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetNumLayers: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if (aNumLayers > MAX_LAYER) // MAX_LAYER defined in cvei.h
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::SetNumLayers: Error Max num layers is %d", MAX_LAYER));
        return false;
    }

    iEncodeParam.iNumLayer = aNumLayers;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetOutputBitRate(uint32 aLayer, uint32 aBitRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetOutputBitRate: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::SetOutputBitRate: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iBitRate[aLayer] = aBitRate;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetOutputFrameSize(uint32 aLayer, uint32 aWidth, uint32 aHeight)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetOutputFrameSize: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::SetOutputFrameSize: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iFrameWidth[aLayer] = aWidth;
    iEncodeParam.iFrameHeight[aLayer] = aHeight;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetOutputFrameRate(uint32 aLayer, OsclFloat aFrameRate)
{
    LOGV("SetOutputFrameRate: %f", aFrameRate);
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetOutputFrameRate: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::SetOutputFrameRate: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iFrameRate[aLayer] = OSCL_STATIC_CAST(float, aFrameRate);
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetSegmentTargetSize(uint32 aLayer, uint32 aSizeBytes)
{
    OSCL_UNUSED_ARG(aLayer);

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetSegmentTargetSize: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    iEncodeParam.iPacketSize = aSizeBytes;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetRateControlType(uint32 aLayer, PVMFVENRateControlType aRateControl)
{
    OSCL_UNUSED_ARG(aLayer);

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetRateControlType: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    switch (aRateControl)
    {
        case PVMFVEN_RATE_CONTROL_CONSTANT_Q:
            iEncodeParam.iRateControlType = ECONSTANT_Q;
            break;
        case PVMFVEN_RATE_CONTROL_CBR:
            iEncodeParam.iRateControlType = ECBR_1;
            break;
        case PVMFVEN_RATE_CONTROL_VBR:
            iEncodeParam.iRateControlType = EVBR_1;
            break;
        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetDataPartitioning(bool aDataPartitioning)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetDataPartitioning: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if (iEncodeParam.iContentType == ECVEI_H263)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::SetDataPartitioning: Error data partitioning not supported for H263"));
        return false;
    }

    if (aDataPartitioning)
        iEncodeParam.iContentType = ECVEI_STREAMING;
    else
        iEncodeParam.iContentType = ECVEI_DOWNLOAD;

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetIFrameInterval(uint32 aIFrameInterval)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetIFrameInterval: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    iEncodeParam.iIFrameInterval = aIFrameInterval;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetRVLC(bool aRVLC)
{
    OSCL_UNUSED_ARG(aRVLC);

    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::SetRVLC"));
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::GetVolHeader(OsclRefCounterMemFrag& aVolHeader)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::GetVolHeader"));

    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
        case EPVMFNodePrepared:
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            break;

        default:
            LOG_ERR((0, "PVMFOMXVideoEncNode::GetVolHeader: Error - Wrong state"));
            return false;
    }

    if (iEncodeParam.iContentType == ECVEI_H263)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::GetVolHeader: Error - VOL header only for M4V encode"));
        return false;
    }

	uint8 *ptr = (uint8 *)iVolHeader.getMemFragPtr();
	//If data partioning mode
	if (iEncodeParam.iContentType == ECVEI_STREAMING)
	{
		ptr[iVolHeader.getMemFragSize() - 1] = 0x8F;
	}
	//else combined mode
	else
	{
		ptr[iVolHeader.getMemFragSize() - 1] = 0x1F;
	}

	aVolHeader = iVolHeader;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::RequestIFrame()
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::RequestIFrame"));

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            break;
        default:
            LOG_ERR((0, "PVMFOMXVideoEncNode::RequestIFrame: Error - Wrong state"));
            return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFOMXVideoEncNode::SetCodec(PVMFFormatType aCodec)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::SetCodec %d", aCodec));

    if (SetCodecType(aCodec) == PVMFSuccess)
    {
        return true;
    }
    else
    {
        return false;
    }

}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::HandlePVCVEIEvent(uint32 aId, uint32 aEvent, uint32 aParam1)
{
    OSCL_UNUSED_ARG(aId);
    OSCL_UNUSED_ARG(aEvent);
    OSCL_UNUSED_ARG(aParam1);
}

////////////////////////////////////////////////////////////////////////////
//                        Private methods
////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::ConstructEncoderParams()
{
    iInputFormat.iVideoFormat = ECVEI_YUV420;
    iInputFormat.iFrameWidth = DEFAULT_FRAME_WIDTH;
    iInputFormat.iFrameHeight = DEFAULT_FRAME_HEIGHT;
    iInputFormat.iFrameRate = (float)DEFAULT_FRAME_RATE;

    oscl_memset(&iEncodeParam, 0, sizeof(TPVVideoEncodeParam));
    iEncodeParam.iEncodeID = 0;
    iEncodeParam.iNumLayer = 1;
    iEncodeParam.iFrameWidth[0] = DEFAULT_FRAME_WIDTH;
    iEncodeParam.iFrameHeight[0] = DEFAULT_FRAME_HEIGHT;
    iEncodeParam.iBitRate[0] = DEFAULT_BITRATE;
    iEncodeParam.iFrameRate[0] = (float)DEFAULT_FRAME_RATE;
    iEncodeParam.iFrameQuality = 10;
    iEncodeParam.iIFrameInterval = 10;
    iEncodeParam.iBufferDelay = (float)0.2;
    iEncodeParam.iContentType = ECVEI_H263;
    iEncodeParam.iRateControlType = ECBR_1;
    iEncodeParam.iIquant[0] = 15;
    iEncodeParam.iPquant[0] = 12;
    iEncodeParam.iBquant[0] = 12;
    iEncodeParam.iSearchRange = 16;
    iEncodeParam.iMV8x8 = false;
    iEncodeParam.iPacketSize = 256;
    iEncodeParam.iNoCurrentSkip = false;
    iEncodeParam.iNoFrameSkip = false;
    iEncodeParam.iClipDuration = 0;
    iEncodeParam.iProfileLevel = ECVEI_CORE_LEVEL2;
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::Run()
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Run: In"));

    // if reset is in progress, call DoReset again until Reset Msg is sent
    if ((iResetInProgress == true) &&
        (iResetMsgSent == false) &&
        (iCurrentCmd.size() > 0) &&
        (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_RESET))
    {
        DoReset(iCurrentCmd.front());
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - Calling DoReset"));
        return; // don't do anything else
    }

    // Check for NODE commands...
    if (!iCmdQueue.empty())
    {
        if (ProcessCommand(iCmdQueue.front()))
        {
            if (iInterfaceState != EPVMFNodeCreated && 
                (!iCmdQueue.empty() || (iInPort.size() > 0 && (iInPort[0]->IncomingMsgQueueSize() > 0)) || (iDataIn.GetRep()!=NULL)) ) 
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - rescheduling after process command"));
                RunIfNotReady();
            }
            return;
        }

        if (!iCmdQueue.empty())
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - rescheduling to process more commands"));
            RunIfNotReady();
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - Input commands empty"));
    }

    if ( ( (iCurrentCmd.size() == 0) && (iInterfaceState != EPVMFNodeStarted) ) ||
         ( (iCurrentCmd.size()>0) && (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_START) && (iInterfaceState != EPVMFNodeStarted) ) )
    {
        // rescheduling because of input data will be handled in Command Processing Part
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - Node not in Started state yet"));
        return;
    }

    // Process port activity, push out all outgoing messages
    if (iOutPort.size() > 0)
    {
        while(iOutPort[0]->OutgoingMsgQueueSize())
        {
            // if port is busy it is going to wakeup from port ready event
            if (!ProcessOutgoingMsg(iOutPort[0]))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - Outgoing Port Busy, cannot send more msgs"));
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
        if (iInPort.size() && (iInPort[0]->IncomingMsgQueueSize() > 0) && (iDataIn.GetRep() == NULL) && !iEndOfDataReached)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - Getting more input"));
            if (ProcessIncomingMsg(iInPort[0]) != PVMFSuccess)
            {
                // Re-schedule to come back.
                RunIfNotReady();
                return;
            }
        }

        // If in init or ready to decode state, process data in the input port if there is input available and input buffers are present
        // (note: at EOS, iDataIn will not be available)
        if( (iDataIn.GetRep() != NULL) ||
            ((iNumOutstandingOutputBuffers < iNumOutputBuffers) &&
            (iProcessingState == EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode) &&
            (iResetMsgSent == false)) ||
            ( (iDynamicReconfigInProgress == true) && (iResetMsgSent==false)) )
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0,"PVMFOMXVideoEncNode::Run() - Calling HandleProcessingState"));

            // input data is available, that means there is video data to be decoded
            if (HandleProcessingState() != PVMFSuccess)
            {
                // If HandleProcessingState does not return Success, we must wait for an event
                // no point in rescheduling
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0,"PVMFOMXVideoEncNode::Run() - HandleProcessingState did not return Success"));
                return;
            }
        }
        loopCount++;
        } while( iInPort.size() &&
                 (( (iInPort[0]->IncomingMsgQueueSize() > 0) || (iDataIn.GetRep() != NULL) ) && (iNumOutstandingInputBuffers < iNumInputBuffers) ) && 
                 (!iEndOfDataReached) &&
                 (iResetMsgSent == false) );
#if (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_REL)
    uint32 endticks = OsclTickCount::TickCount();
    uint32 endtime = OsclTickCount::TicksToMsec(endticks);
    uint32 timeinloop = (endtime - starttime);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_INFO,
                    (0,"PVMFOMXVideoEncNode::Run() - LoopCount = %d, Time spent in loop(in ms) = %d, iNumOutstandingInputBuffers = %d, iNumOutstandingOutputBuffers = %d ",
                    loopCount, timeinloop, iNumOutstandingInputBuffers, iNumOutstandingOutputBuffers));
#endif
    // EOS processing:
    // first send an empty buffer to OMX component and mark the EOS flag
    // wait for the OMX component to send async event to indicate that it has reached this EOS buffer
    // then, create and send the EOS message downstream

    if (iEndOfDataReached && !iDynamicReconfigInProgress)
    {

        // if EOS was not sent yet and we have an available ninput buffer, send EOS buffer to component
        if (!iIsEOSSentToComponent && (iNumOutstandingInputBuffers < iNumInputBuffers) )
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0,"PVMFOMXVideoEncNode::Run() - Sending EOS marked buffer To Component "));

            iIsEOSSentToComponent = true;


            // if the component is not yet initialized or if it's in the middle of port reconfig,
            // don't send EOS buffer to component. It does not care. Just set the flag as if we received
            // EOS from the component to enable sending EOS downstream
            if (iProcessingState != EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode)
            {
                iIsEOSReceivedFromComponent = true;
            }
            else if( !SendEOSBufferToOMXComponent() )
            {
                // for some reason, Component can't receive the EOS buffer
                // it could be that it is not initialized yet (because EOS could be the first msg). In this case,
                // send the EOS downstream anyway
                iIsEOSReceivedFromComponent = true;
            }
        }

        // We must wait for event (acknowledgment from component)
        // before sending EOS downstream. This is because OMX Component will send
        // the EOS event only after processing remaining buffers

        if (iIsEOSReceivedFromComponent)
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0,"PVMFOMXVideoEncNode::Run() - Received EOS from component, Sending EOS msg downstream "));

            if ((iOutPort.size() > 0) && iOutPort[0]->IsOutgoingQueueBusy())
            {
                // note: we already tried to empty the outgoing q. If it's still busy,
                // it means that output port is busy. Just return and wait for the port to become free.
                // this will wake up the node and it will send out a msg from the q etc.
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0,"PVMFOMXVideoEncNode::Run() - - EOS cannot be sent downstream, outgoing queue busy - wait"));
                return;
            }

            if (SendEndOfTrackCommand()) // this will only q the EOS
            {
                // EOS send downstream OK, so reset the flag
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                               (0,"PVMFOMXVideoEncNode::Run() - EOS was queued to be sent downstream"));

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
            // data at the end
            while (iNumOutstandingOutputBuffers < iNumOutputBuffers)
            {
                if (!SendOutputBufferToOMXComponent())
                    break;
            }
        }

    }


    // Check for flush command completion...
    if ((iInPort.size() > 0) && (iOutPort.size() > 0) && (iCurrentCmd.size()>0) &&
        (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH) &&
        (iInPort[0]->IncomingMsgQueueSize() == 0) &&
        (iOutPort[0]->OutgoingMsgQueueSize() == 0) &&
        (iDataIn.GetRep() == NULL) )
    {
        // flush command is completed
        // Debug check-- all the port queues should be empty at this point.

        OSCL_ASSERT(iInPort[0]->IncomingMsgQueueSize() == 0 && iOutPort[0]->OutgoingMsgQueueSize() == 0);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::Run() - Flush pending"));

        // clear the node input data and flags
        iDataIn.Unbind();
        iEndOfDataReached = false;
        iIsEOSSentToComponent = false;
        iIsEOSReceivedFromComponent = false;

        // Flush is complete.  Go to initialized state.
        SetState(EPVMFNodePrepared);
        // resume port input so the ports can be re-started.
        iInPort[0]->ResumeInput();
        iOutPort[0]->ResumeInput();
        CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);
        RunIfNotReady();
    }

    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::Run: Out"));
}

/////////////////////////////////////////////////////
//     Command processing routines
/////////////////////////////////////////////////////
PVMFCommandId PVMFOMXVideoEncNode::QueueCommandL(PVMFVideoEncNodeCommand& aCmd)
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
bool PVMFOMXVideoEncNode::ProcessCommand(PVMFVideoEncNodeCommand& aCmd)
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
            LOG_ERR((0, "PVMFOMXVideoEncNode::ProcessCommand: Error - Unknown command type %d", aCmd.iCmd));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            break;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::CommandComplete(PVMFVideoEncNodeCmdQueue& aCmdQueue, PVMFVideoEncNodeCommand& aCmd,
        PVMFStatus aStatus, OsclAny* aData)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::CommandComplete: Id=%d, Type=%d, Status=%d, Context=0x%x, Data0x%x"
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
void PVMFOMXVideoEncNode::DoQueryUuid(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::DoQueryUuid"));
    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator>* uuidvec;
    bool exactMatch;
    aCmd.Parse(mimetype, uuidvec, exactMatch);

    int32 err = 0;
    PVMFStatus status = PVMFSuccess;
    OSCL_TRY(err, uuidvec->push_back(PVMp4H263EncExtensionUUID););
    OSCL_FIRST_CATCH_ANY(err, status = PVMFErrNoMemory;);
    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoQueryInterface(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::DoQueryInterface"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.Parse(uuid, ptr);

    PVMFStatus status = PVMFSuccess;
    if (!queryInterface(*uuid, *ptr))
        status = PVMFFailure;

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoRequestPort(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::DoRequestPort"));
    int32 tag;
    OSCL_String* mimetype;
    PVMFVideoEncPort* port = NULL;
    aCmd.Parse(tag, mimetype);

    switch (tag)
    {
        case PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_INPUT:
        {
            if (iInPort.size() >= PVMF_OMX_VIDEOENC_NODE_MAX_INPUT_PORT)
            {
                LOG_ERR((0, "PVMFOMXVideoEncNode::DoRequestPort: Error - Max number of input port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVVideoEncIn";

            port = AllocatePort(iInPort, tag, mimetype, portname.get_cstr());
            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }

        break;

        case PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_OUTPUT:
        {
            if (iOutPort.size() >= PVMF_OMX_VIDEOENC_NODE_MAX_OUTPUT_PORT)
            {
                LOG_ERR((0, "PVMFOMXVideoEncNode::DoRequestPort: Error - Max number of output port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVVideoEncOut";

            port = AllocatePort(iOutPort, tag, mimetype, portname.get_cstr());
            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        break;

        default:
            LOG_ERR((0, "PVMFOMXVideoEncNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            return;
    }

    //Return the port pointer to the caller.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess, (OsclAny*)port);
}

////////////////////////////////////////////////////////////////////////////
PVMFVideoEncPort* PVMFOMXVideoEncNode::AllocatePort(PVMFVideoEncPortVector& aPortVector, int32 aTag,
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
                         LOG_ERR((0, "PVMFOMXVideoEncNode::AllocatePort: Error - iPortVector Out of memory"));
                         return NULL;
                        );
    PVMFVideoEncPort* port = OSCL_PLACEMENT_NEW(ptr, PVMFVideoEncPort(aTag, this, Priority(), aName));

    // if format was provided in mimestring, set it now.
    if (aMimeType)
    {
        PVMFFormatType format = GetFormatIndex(aMimeType->get_str());
        if ((port->SetFormat(format) != PVMFSuccess) ||
                ((aTag == PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_OUTPUT) && (SetCodecType(format) != PVMFSuccess)))
        {
            aPortVector.DestructAndDealloc(port);
            LOG_ERR((0, "PVMFOMXVideoEncNode::AllocatePort: Error - port->SetFormat or SetCodecType failed"));
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
void PVMFOMXVideoEncNode::DoReleasePort(PVMFVideoEncNodeCommand& aCmd)
{
    //Find the port in the port vector
    PVMFVideoEncPort* port = NULL;
    PVMFVideoEncPort** portPtr = NULL;
    aCmd.Parse((PVMFPortInterface*&)port);

    if (!port)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::DoReleasePort: Error - Invalid port pointer"));
        CommandComplete(iCmdQueue, aCmd, PVMFFailure);
        return;
    }

    PVMFStatus status = PVMFSuccess;
    switch (port->GetPortTag())
    {
        case PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_INPUT:
            portPtr = iInPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PVMFOMXVideoEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iInPort.Erase(portPtr);
            }
            break;

        case PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_OUTPUT:
            portPtr = iOutPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PVMFOMXVideoEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iOutPort.Erase(portPtr);
            }
            break;

        default:
            LOG_ERR((0, "PVMFOMXVideoEncNode::DoReleasePort: Error - Invalid port tag"));
            status = PVMFFailure;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoInit(PVMFVideoEncNodeCommand& aCmd)
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
void PVMFOMXVideoEncNode::DoPrepare(PVMFVideoEncNodeCommand& aCmd)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_STRING Role = NULL;

    LOG_STACK_TRACE((0, "PVMFVIdeoEncNode::DoPrepare"));
    mInputBufferRefCount = 0;
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
		{
            // Check format of output data
            uint32 Format = ((PVMFVideoEncPort*)iOutPort[0])->iFormat;
            switch (Format)
            {
                case PVMF_H263:
                    Role = "video_encoder.h263";
                    break;
                case PVMF_M4V:
                    Role = "video_encoder.mpeg4";
                    break;
                case PVMF_H264:
                case PVMF_H264_MP4:
                case PVMF_H264_RAW:
                    //Role = "video_encoder.avc";
                    //break;
                case PVMF_WMV:
                    //Role = "video_encoder.wmv";
                    //break;
                default:
                    // Illegal codec specified.
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFOMXVideoEncNode::DoPrepare() Input port format other then codec type"));
                    CommandComplete(iCmdQueue, aCmd, PVMFErrArgument);
                    return;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoEncNode::Initializing OMX component and encoder for role %s", Role));

            /* Set callback structure */
            iCallbacks.EventHandler    = CallbackEventHandler;    //event_handler;
            iCallbacks.EmptyBufferDone = CallbackEmptyBufferDone; //empty_buffer_done;
            iCallbacks.FillBufferDone  = CallbackFillBufferDone;  //fill_buffer_done;

            // determine components which can fit the role
            // then, create the component. If multiple components fit the role,
            // the first one registered will be selected. If that one fails to
            // be created, the second one in the list is selected etc.
            OMX_U32 num_comps = 0;
            OMX_STRING *CompOfRole;
            // call once to find out the number of components that can fit the role
            //PV_Master
            PV_MasterOMX_GetComponentsOfRole(Role, &num_comps, NULL);

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFOMXVideoEncNode::DoPrepare(): There are %d components of role %s ", num_comps, Role));

            int i;

            if (num_comps > 0)
            {
                CompOfRole = (OMX_STRING *)oscl_malloc(num_comps * sizeof(OMX_STRING));

                for (i = 0; i < num_comps; i++)
                    CompOfRole[i] = (OMX_STRING) oscl_malloc(PV_OMX_MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8));

                // call 2nd time to get the component names
                //PV_Master
                PV_MasterOMX_GetComponentsOfRole(Role, &num_comps, (OMX_U8 **)CompOfRole);

                for (i = 0; i < num_comps; i++)
                {
                    // try to create component
                    err = PV_MasterOMX_GetHandle(&iOMXVideoEncoder, (OMX_STRING)CompOfRole[i], (OMX_PTR)this, (OMX_CALLBACKTYPE *) & iCallbacks);

                    // if successful, no need to continue
                    if ( (err == OMX_ErrorNone) && (iOMXVideoEncoder != NULL))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMFOMXVideoEncNode::DoPrepare(): Got Component %s handle ", CompOfRole[i]));

                        break;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMFOMXVideoEncNode::DoPrepare(): Cannot get component %s handle, try another component if available", CompOfRole[i]));
                    }
                }
                // whether successful or not, need to free CompOfRoles
                for (i = 0; i < num_comps; i++)
                {
                    oscl_free(CompOfRole[i]);
                    CompOfRole[i] = NULL;
                }

                oscl_free(CompOfRole);
                // check if there was a problem
                if ( (err != OMX_ErrorNone) || (iOMXVideoEncoder == NULL) )
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoEncNode::Can't get handle for encoder!"));
                    iOMXVideoEncoder = NULL;
                    CommandComplete(iCmdQueue, aCmd, PVMFErrResource);
                    return;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoEncNode::No component can handle role %s !", Role));
                iOMXVideoEncoder = NULL;
                CommandComplete(iCmdQueue, aCmd, PVMFErrResource);
                return;
            }

            if (!iOMXVideoEncoder)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoResources);
                return;
            }

            // GET CAPABILITY FLAGS FROM PV COMPONENT, IF this fails, use defaults
            PV_OMXComponentCapabilityFlagsType Cap_flags = {OMX_FALSE, OMX_FALSE, OMX_FALSE, OMX_FALSE};
            err = OMX_GetParameter(iOMXVideoEncoder, (OMX_INDEXTYPE)PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX, &Cap_flags);
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
                iOMXComponentCanHandleIncompleteFrames = true;
            }

            // make sure that copying is used where necessary
            if (!iOMXComponentSupportsPartialFrames)
            {
                iOMXComponentSupportsMovableInputBuffers = false;
            }

            // find out about parameters
            if (!NegotiateComponentParameters())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoEncNode::DoPrepare() Cannot get component parameters"));

                CommandComplete(iCmdQueue, aCmd, PVMFErrNoResources);
                return;
            }

			// create active objects to handle callbacks in case of multithreaded implementation
			// NOTE: CREATE THE THREADSAFE CALLBACK AOs REGARDLESS OF WHETHER MULTITHREADED COMPONENT OR NOT
			//		If it is not multithreaded, we won't use them
			//		The Flag iIsComponentMultiThreaded decides which mechanism is used for callbacks.
			//		This flag is set by looking at component capabilities (or to true by default)
			if(iThreadSafeHandlerEventHandler)
			{
				OSCL_DELETE(iThreadSafeHandlerEventHandler);
				iThreadSafeHandlerEventHandler = NULL;
			}
			// substitute default parameters: observer(this node),queuedepth(3),nameAO for logging
			// Get the priority of video dec node, and set the threadsafe callback AO priority to 1 higher
			iThreadSafeHandlerEventHandler = OSCL_NEW(EventHandlerThreadSafeCallbackAO,(this,10,"EventHandlerAO",Priority()+2));

			if(iThreadSafeHandlerEmptyBufferDone)
			{
				OSCL_DELETE(iThreadSafeHandlerEmptyBufferDone);
				iThreadSafeHandlerEmptyBufferDone = NULL;
			}
			// use queue depth of iNumInputBuffers to prevent deadlock
			iThreadSafeHandlerEmptyBufferDone = OSCL_NEW(EmptyBufferDoneThreadSafeCallbackAO,(this,iNumInputBuffers,"EmptyBufferDoneAO",Priority()+1));

			if(iThreadSafeHandlerFillBufferDone)
			{
				OSCL_DELETE(iThreadSafeHandlerFillBufferDone);
				iThreadSafeHandlerFillBufferDone = NULL;
			}
			// use queue depth of iNumOutputBuffers to prevent deadlock
			iThreadSafeHandlerFillBufferDone = OSCL_NEW(FillBufferDoneThreadSafeCallbackAO,(this,iNumOutputBuffers,"FillBufferDoneAO",Priority()+1));

			if( (iThreadSafeHandlerEventHandler == NULL) ||
				(iThreadSafeHandlerEmptyBufferDone == NULL) ||
				(iThreadSafeHandlerFillBufferDone == NULL)
			)
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
					(0,"PVMFOMXVideoEncNode::Can't get threadsafe callbacks for encoder!"));
						iOMXVideoEncoder = NULL;
			}

			// Init Encoder
			iCurrentEncoderState = OMX_StateLoaded;

            /* Change state to OMX_StateIdle from OMX_StateLoaded. */
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                (0,"PVMFOMXVideoEncNode::DoPrepare(): Changing Component state Loaded -> Idle "));

            err = OMX_SendCommand(iOMXVideoEncoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
            if (err != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                    (0,"PVMFOMXVideoEncNode::DoPrepare() Can't send StateSet command!"));
                CommandComplete(iCmdQueue,aCmd,PVMFErrNoResources);
                return;
            }

			/* Allocate input buffers */
			if(!CreateInputMemPool(iNumInputBuffers))
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
					(0,"PVMFOMXVideoEncNode::DoPrepare() Can't allocate mempool for input buffers!"));

				CommandComplete(iCmdQueue,aCmd,PVMFErrNoResources);
				return;
			}

			if(!ProvideBuffersToComponent(iInBufMemoryPool, // allocator
										  iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  iNumInputBuffers, // number of buffers
										  iOMXComponentInputBufferSize, // actual buffer size
										  iInputPortIndex, // port idx
										  iOMXComponentSupportsExternalInputBufferAlloc, // can component use OMX_UseBuffer
										  true // this is input
										  ))
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
					(0,"PVMFOMXVideoEncNode::DoPrepare() Component can't use input buffers!"));

				CommandComplete(iCmdQueue,aCmd,PVMFErrNoResources);
				return;
			}

			/* Allocate output buffers */
			if(!CreateOutputMemPool(iNumOutputBuffers))
			{

                                LOGE("DoPrepare(): failed in allocating mempool for output buffers!");
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
					(0,"PVMFOMXVideoEncNode::DoPrepare() Can't allocate mempool for output buffers!"));

				CommandComplete(iCmdQueue,aCmd,PVMFErrNoResources);
				return;
			}

			if(!ProvideBuffersToComponent(iOutBufMemoryPool, // allocator
										  iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  iNumOutputBuffers, // number of buffers
										  iOMXComponentOutputBufferSize, // actual buffer size
										  iOutputPortIndex, // port idx
										  iOMXComponentSupportsExternalOutputBufferAlloc, // can component use OMX_UseBuffer
										  false // this is not input
										  ))
			{

                                LOGE("DoPrepare(): OMX component failed in using output buffers!");
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
					(0,"PVMFOMXVideoEncNode::DoPrepare() Component can't use output buffers!"));

				CommandComplete(iCmdQueue,aCmd,PVMFErrNoResources);
				return;
			}

			//this command is asynchronous.  move the command from
			//the input command queue to the current command, where
			//it will remain until it completes. We have to wait for
			// OMX component state transition to complete
#if 1
			int32 err;
			OSCL_TRY(err,iCurrentCmd.StoreL(aCmd););
			if (err!=OsclErrNone)
			{
				CommandComplete(iCmdQueue,aCmd,PVMFErrNoMemory);
				return;
			}
			iCmdQueue.Erase(&aCmd);
#else
            SetState(EPVMFNodePrepared);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
#endif
            break;
		}
        case EPVMFNodePrepared:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoStart(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::DoStart"));

	PVMFStatus status = PVMFSuccess;

	OMX_ERRORTYPE  err;
	OMX_STATETYPE sState;

	switch(iInterfaceState)
	{
		case EPVMFNodePrepared:
		case EPVMFNodePaused:
		{
			//Get state of OpenMAX encoder
			err = OMX_GetState(iOMXVideoEncoder, &sState);
			if (err != OMX_ErrorNone)
			{
				//Error condition report
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,

					(0,"PVMFOMXVideoEncNode::DoStart(): Can't get State of encoder!"));

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
					(0,"PVMFOMXVideoEncNode::DoStart() Changing Component state Idle->Executing"));

				err = OMX_SendCommand(iOMXVideoEncoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
				if (err != OMX_ErrorNone)
				{
					//Error condition report
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::DoStart(): Can't send StateSet command to encoder!"));

					status = PVMFErrInvalidState;
				}

			}
			else
			{
				//Error condition report
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::DoStart(): Encoder is not in the Idle or Pause state!"));

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
		CommandComplete(iCmdQueue, aCmd, status);
	}
	else
	{
		//this command is asynchronous.  move the command from
		//the input command queue to the current command, where
		//it will remain until it completes.
		int32 err;
		OSCL_TRY(err,iCurrentCmd.StoreL(aCmd););
		if (err!=OsclErrNone)
		{
			CommandComplete(iCmdQueue,aCmd,PVMFErrNoMemory);
		}
		iCmdQueue.Erase(&aCmd);
	}
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoStop(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::DoStop"));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            // Clear queued messages in ports
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                iInPort[i]->ClearMsgQueues();
            for (i = 0; i < iOutPort.size(); i++)
                iOutPort[i]->ClearMsgQueues();

            // Video encoder is created on Start, so in parallel it's deleted in Stop
            DeleteVideoEncoder();
            //transition to Prepared state
            SetState(EPVMFNodePrepared);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);

        }
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
void PVMFOMXVideoEncNode::DeleteVideoEncoder()
{
    LOGV("DeleteVideoEncoder");
    if (iOMXVideoEncoder != NULL) {
        OMX_ERRORTYPE err;
        OMX_STATETYPE state;
        err = OMX_GetState(iOMXVideoEncoder, &state);
        if (err != OMX_ErrorNone) {
            LOGE("Failed to get encoder state with failure code(%d)", err);
        } else if (state != OMX_StateLoaded && state != OMX_StateInvalid) {
            LOGE("OMX_FreeHandle is called in a state(%d) other than OMX_StateLoaded or OMX_StateInvalid.", state);
        }

        // No matter what, call OMX_FreeHandle anyway
        err = PV_MasterOMX_FreeHandle(iOMXVideoEncoder);
        if (err != OMX_ErrorNone) {
            LOGE("Failed to free encoder handle with failure code(%d)", err);
        } else {
            LOGI("video encoder handle has been successfully released");
            iOMXVideoEncoder = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoFlush(PVMFVideoEncNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0,"PVMFOMXVideoEncNode::DoFlush(): In"));

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            int32 err;
            uint32 i;
            bool msgPending;
            msgPending = false;

            // FIXME: should we do this repeatedly?
            for (i = 0; i < iInPort.size(); i++)
            {
                if (iInPort[i]->IncomingMsgQueueSize() > 0)
                    msgPending = true;
                iInPort[i]->SuspendInput();
                if (iInterfaceState != EPVMFNodeStarted)
                {
                    // Port is in idle if node state is not started. Call ProcessIncomingMsgReady
                    // to wake up port AO
                    ((PVMFVideoEncPort*)iInPort[i])->ProcessIncomingMsgReady();
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
                    ((PVMFVideoEncPort*)iOutPort[i])->ProcessOutgoingMsgReady();
                }
            }

            // Don't repeatedly poll state from encoder and don't repeatedly send the same
            // state transition request to encoder. Only request and send command when necessary.
            if (iProcessingState == EPVMFOMXVideoEncNodeProcessingState_Stopping && mInputBufferRefCount != 0) {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0,"PVMFOMXVideoEncNode::DoFlush(): wait for buffer done for all buffers!"));
                return;
            } else if (iProcessingState == EPVMFOMXVideoEncNodeProcessingState_Stopping && mInputBufferRefCount == 0) {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0,"PVMFOMXVideoEncNode::DoFlush(): buffer done for all buffers"));

                // Flush completes only if the following conditions are all met:
                // 1. all buffers have been released by the encoder
                // 2. we are in the middle of doing flush
                // 3. there are no message pending for the ports
                // 4. the state has been transferred to OMX_StateIdle
                if (!msgPending) {
                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                     (0,"PVMFOMXVideoEncNode::DoFlush(): no message pending"));
                    if (iCurrentEncoderState == OMX_StateIdle) {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0,"PVMFOMXVideoEncNode::DoFlush(): flush completion ready!"));
                        // the flush is asynchronous.  move the command from
                        // the input command queue to the current command, where
                        // it will remain until the flush completes.
                        OSCL_TRY(err, iCurrentCmd.StoreL(aCmd););
                        OSCL_FIRST_CATCH_ANY(err,
                                             CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                                             return;
                                            );
                        iCmdQueue.Erase(&aCmd);
                        FlushComplete();
                    } else {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                        (0,"PVMFOMXVideoEncNode::DoFlush(): wait for StateExecuting->StateIdle event!"));
                    }
                } else {
                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                     (0,"PVMFOMXVideoEncNode::DoFlush(): wait for pending messages to be completely processed!"));
                }
                return;
            } else if (iProcessingState != EPVMFOMXVideoEncNodeProcessingState_Stopping) {
                OMX_ERRORTYPE omx_err;
                OMX_STATETYPE sState;

                // Get state of OpenMAX encoder
                omx_err = OMX_GetState(iOMXVideoEncoder, &sState);
                if (omx_err != OMX_ErrorNone)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0,"PVMFOMXVideoEncNode::DoFlush(): can't get encoder state(%d)", omx_err));
                    sState = OMX_StateInvalid;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0,"PVMFOMXVideoEncNode::DoFlush(): start handling flush when encoder is in state(%d)!", sState));
                if ((sState == OMX_StateExecuting) || (sState == OMX_StatePause))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0,"PVMFOMXVideoEncNode::DoFlush() Changing Component State Executing->Idle or Pause->Idle"));

                    omx_err = OMX_SendCommand(iOMXVideoEncoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
                    if (omx_err != OMX_ErrorNone)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0,"PVMFOMXVideoEncNode::DoFlush(): Can't send StateSet command to encoder!"));

                        CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
                        break;
                    } 

                    // prevent the node from sending more buffers etc.
                    // if port reconfiguration is in process, let the state remain one of the port config states
                    // if there is a start command, we can do it seemlessly (by continuing the port reconfig)
                    if (iProcessingState != EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0,"PVMFOMXVideoEncNode::DoFlush(): Called in an invalid processing state(%d)!", iProcessingState));
                    }
                    iProcessingState = EPVMFOMXVideoEncNodeProcessingState_Stopping;
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0,"PVMFOMXVideoEncNode::DoFlush(): Encoder is not in the Executing or Pause state!"));

                    CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
                    break;
                }
            }
            break;

        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::IsFlushPending()
{
    return (iCurrentCmd.size() > 0
            && iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::FlushComplete()
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::FlushComplete"));
    uint32 i = 0;
    
    // flush completes only when all queues of all ports are clear.
    // otherwise, just return from this method and wait for FlushComplete
    // from the remaining ports.
    for (i = 0; i < iInPort.size(); i++)
    {
        if ( iInPort[i]->IncomingMsgQueueSize() > 0 ||
             iInPort[i]->OutgoingMsgQueueSize() > 0 )
        {
            return;
        }
    }

    for (i = 0; i < iOutPort.size(); i++)
    {
        if ( iOutPort[i]->IncomingMsgQueueSize() > 0 ||
             iOutPort[i]->OutgoingMsgQueueSize() > 0 )
        {
            return;
        }
    }

    // resume port input so the ports can be re-started.
    for (i = 0; i < iInPort.size(); i++)
        iInPort[i]->ResumeInput();
    for (i = 0; i < iOutPort.size(); i++)
        iOutPort[i]->ResumeInput();

    // clear the node input data and flags
    iDataIn.Unbind();
    iEndOfDataReached = false;
    iIsEOSSentToComponent = false;
    iIsEOSReceivedFromComponent = false;

    if (iCurrentCmd.empty())
    {
       LOG_ERR((0, "PVMFOMXVideoEncNode::FlushComplete: Error - iCurrentCmd is empty"));
       return;
    }

    // flush completes, go to prepared state.
    SetState(EPVMFNodePrepared);
    CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);

    if (!iCmdQueue.empty())
    {
        // If command queue is not empty, schedule to process the next command
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoPause(PVMFVideoEncNodeCommand& aCmd)
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
void PVMFOMXVideoEncNode::DoReset(PVMFVideoEncNodeCommand& aCmd)
{

	OMX_ERRORTYPE  err;
	OMX_STATETYPE sState;

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::DoReset() In"));

	switch(iInterfaceState)
	{
		case EPVMFNodeIdle:
		case EPVMFNodeInitialized:
		case EPVMFNodePrepared:
		case EPVMFNodeStarted:
		case EPVMFNodePaused:
		case EPVMFNodeError:
		{
			//Check if encoder is initilized
			if (iOMXVideoEncoder != NULL)
			{

				//if we're in the middle of a partial frame assembly
				// abandon it and start fresh
				if(iObtainNewInputBuffer == false)
				{
					if(iInputBufferUnderConstruction != NULL)
					{
						if(iInBufMemoryPool!=NULL )
						{
							iInBufMemoryPool->deallocate((OsclAny *)iInputBufferUnderConstruction);
						}
						iInputBufferUnderConstruction = NULL;
					}
					iObtainNewInputBuffer = true;

				}

				iKeepDroppingMsgsUntilMarkerBit = false;

				// HTC's fix for the race condition between pv omx encoder node and qualcomm encoder
				// Remove polling the state of OpenMAX encoder to reduce the extra delay
				err = OMX_GetState(iOMXVideoEncoder, &sState);
				if (err != OMX_ErrorNone)
				{
					//Error condition report
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::DoReset(): Can't get State of encoder!"));
					if(iResetInProgress)
					{
						// cmd is in current q
						iResetInProgress = false;
						if((iCurrentCmd.size()>0) &&
						   (iCurrentCmd.front().iCmd==PVMF_GENERIC_NODE_RESET)
						   )
						{
							CommandComplete(iCurrentCmd,iCurrentCmd.front(),PVMFErrResource);
						}
					}
					else
					{
						CommandComplete(iCmdQueue,aCmd,PVMFErrResource);
					}
					return;
				}

				if (sState == OMX_StateLoaded)
				{
					// this is a value obtained by synchronous call to component. Either the component was
					// already in this state without node issuing any commands,
					// or perhaps we started the Reset, but the callback notification has not yet arrived.
					if(iResetInProgress)
					{
						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						 (0,"PVMFOMXVideoEncNode::DoReset() OMX comp is in loaded state. Wait for official callback to change variables etc."));
						return;
					}
					else
					{
						CommandComplete(iCmdQueue,aCmd,PVMFErrResource);
						return;
					}
				}

				if (sState == OMX_StateIdle)
				{
					//this command is asynchronous.  move the command from
					//the input command queue to the current command, where
					//it will remain until it is completed.
					if(!iResetInProgress)
					{
						int32 err;
						OSCL_TRY(err,iCurrentCmd.StoreL(aCmd););
						if (err != OsclErrNone)
						{
							CommandComplete(iCmdQueue,aCmd,PVMFErrNoMemory);
							return;
						}
						iCmdQueue.Erase(&aCmd);

						iResetInProgress = true;
					}

					// if buffers aren't all back (due to timing issues with different callback AOs
					//		state change can be reported before all buffers are returned)
					if(iNumOutstandingInputBuffers > 0 || iNumOutstandingOutputBuffers > 0)
					{
						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::DoReset() Waiting for %d input and-or %d output buffers",iNumOutstandingInputBuffers,iNumOutstandingOutputBuffers));

						return;
					}

					if(!iResetMsgSent)
					{
						// We can come here only if all buffers are already back
						// Don't repeat any of this twice.
						/* Change state to OMX_StateLoaded form OMX_StateIdle. */
						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
							(0,"PVMFOMXVideoEncNode::DoReset() Changing Component State Idle->Loaded"));

						err = OMX_SendCommand(iOMXVideoEncoder, OMX_CommandStateSet, OMX_StateLoaded, NULL);
						if (err != OMX_ErrorNone)
						{
							//Error condition report
							PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
								(0,"PVMFOMXVideoEncNode::DoReset(): Can't send StateSet command to encoder!"));
						}

						iResetMsgSent = true;

						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
							(0,"PVMFOMXVideoEncNode::DoReset() freeing output buffers"));

						if(iOutputBuffersFreed == false)
						{
							if( !FreeBuffersFromComponent(iOutBufMemoryPool, // allocator
										  iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  iNumOutputBuffers, // number of buffers
										  iOutputPortIndex, // port idx
										  false // this is not input
										  ))
							{
								PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
									(0,"PVMFOMXVideoEncNode::DoReset() Cannot free output buffers "));

								if(iResetInProgress)
								{
									iResetInProgress = false;
									if((iCurrentCmd.size() > 0) &&
										(iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_RESET)
									)
									{
										CommandComplete(iCurrentCmd,iCurrentCmd.front() ,PVMFErrResource);
									}
								}
							}
						}
						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
							(0,"PVMFOMXVideoEncNode::DoReset() freeing input buffers "));

						if(iInputBuffersFreed == false)
						{
							if( !FreeBuffersFromComponent(iInBufMemoryPool, // allocator
												  iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
												  iNumInputBuffers, // number of buffers
												  iInputPortIndex, // port idx
												  true // this is input
												  ))
							{
								PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
									(0,"PVMFOMXVideoEncNode::DoReset() Cannot free input buffers "));

								if(iResetInProgress)
								{
									iResetInProgress = false;
									if((iCurrentCmd.size() > 0) &&
									   (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_RESET)
									   )
									{
										CommandComplete(iCurrentCmd,iCurrentCmd.front(),PVMFErrResource);
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
						(0,"PVMFOMXVideoEncNode::DoReset(): encoder is not in the Idle state!"));
					if(iResetInProgress)
					{
						iResetInProgress = false;
						if( (iCurrentCmd.size()>0) &&
							(iCurrentCmd.front().iCmd==PVMF_GENERIC_NODE_RESET)
							)
						{
							CommandComplete(iCurrentCmd,iCurrentCmd.front(),PVMFErrInvalidState);
						}
					}
					else
					{
                                                // HTC's fix for the race condition between pv omx encoder node and
                                                // qualcomm encoder.
                                                if (sState == OMX_StateExecuting) {
                                                    LOGV("@@@@@@@@@@@ reset wait for encoder @@@@@@@@@@");
                                                    return;
                                                }
						CommandComplete(iCmdQueue,aCmd,PVMFErrInvalidState);
					}
					break;
				}//end of if (sState == OMX_StateIdle)
			}//end of if (iOMXVideoEncoder != NULL)

		    //This example node allows a reset from any idle state.
		    if (IsAdded())
		    {
		        while (!iInPort.empty())
		            iInPort.Erase(&iInPort.front());
		        while (!iOutPort.empty())
		            iOutPort.Erase(&iOutPort.front());

		        //restore original port vector reserve.
		        iInPort.Reconstruct();
		        iOutPort.Reconstruct();
		    }
		    else
		    {
		        OSCL_LEAVE(OsclErrInvalidState);

		    }

			iDataIn.Unbind();

			iEndOfDataReached = false;
			iIsEOSSentToComponent = false;
			iIsEOSReceivedFromComponent = false;

			iProcessingState = EPVMFOMXVideoEncNodeProcessingState_Idle;
			//logoff & go back to Created state.
			SetState(EPVMFNodeIdle);

			if(iResetInProgress)
			{
				iResetInProgress = false;
				if((iCurrentCmd.size()>0) &&
				   (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_RESET)
				   )
				{
					CommandComplete(iCurrentCmd,iCurrentCmd.front(),PVMFSuccess);
				}
			}
			else
			{
				CommandComplete(iCmdQueue,aCmd,PVMFSuccess);
			}
		}
		break;

		default:
			CommandComplete(iCmdQueue,aCmd,PVMFErrInvalidState);
			break;
	}

}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::DoCancelAllCommands(PVMFVideoEncNodeCommand& aCmd)
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
void PVMFOMXVideoEncNode::DoCancelCommand(PVMFVideoEncNodeCommand& aCmd)
{
    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.Parse(id);

    //first check "current" command if any
    {
        PVMFVideoEncNodeCommand* cmd = iCurrentCmd.FindById(id);
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
        PVMFVideoEncNodeCommand* cmd = iCmdQueue.FindById(id, 1);
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
//               Port activity processing routines
////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::IsProcessOutgoingMsgReady()
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
bool PVMFOMXVideoEncNode::IsProcessIncomingMsgReady()
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
PVMFStatus PVMFOMXVideoEncNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::ProcessIncomingMsg: aPort=0x%x", aPort));
    PVMFStatus status = PVMFFailure;

    switch (aPort->GetPortTag())
    {
        case PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_INPUT:
        {
            if (!IsProcessIncomingMsgReady())
            {
                LOG_ERR((0, "PVMFOMXVideoEncNode::ProcessIncomingMsg: Error - Not ready."));
                return PVMFErrBusy;
            }

            PVMFSharedMediaMsgPtr msg;
            status = aPort->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PVMFOMXVideoEncNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed"));
                return status;
            }

            if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                status = SendEndOfTrackCommand(msg);
                return status;
            }
			convertToPVMFMediaData(iDataIn, msg);
			status = PVMFSuccess;

			iCurrFragNum = 0; // for new message, reset the fragment counter
			iIsNewDataFragment = true;
        }
        break;

        case PVMF_OMX_VIDEOENC_NODE_PORT_TYPE_OUTPUT:
            // Nothing to be done
            status = PVMFSuccess;
            break;

        default:
            LOG_ERR((0, "PVMFOMXVideoEncNode::ProcessIncomingMsg: Error - Invalid port tag"));
            ReportErrorEvent(PVMF_OMX_VIDEOENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)aPort);
            status = PVMFFailure;
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::SendEncodedBitstream(PVMFSharedMediaDataPtr& iMediaData)
{
    PVMFStatus status = PVMFSuccess;

    // Send bitstream data to downstream node
    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaMsg(mediaMsgOut, iMediaData);

    for (uint32 i = 0; i < iOutPort.size(); i++)
    {
        status = iOutPort[i]->QueueOutgoingMsg(mediaMsgOut);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFOMXVideoEncNode::SendEncodedBitstream: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
//                 Encoder settings routines
////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::SetCodecType(PVMFFormatType aCodec)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetCodecType: Error iInterfaceState=%d", iInterfaceState));
            return PVMFErrInvalidState;
        default:
            break;
    }

    switch (aCodec)
    {
        case PVMF_H263:
            iEncodeParam.iContentType = ECVEI_H263;
            break;
        case PVMF_M4V:
            iEncodeParam.iContentType = ECVEI_STREAMING;
            break;
        default:
            return PVMFErrNotSupported;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::SetInputFormat(PVMFFormatType aFormat)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::SetInputFormat: aFormat=%d", aFormat));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetInputFormat: Error - iInterfaceState=%d", iInterfaceState));
            return PVMFErrInvalidState;
        default:
            break;
    }

    switch (aFormat)
    {
        case PVMF_YUV420:
            iInputFormat.iVideoFormat = ECVEI_YUV420;
            break;
        case PVMF_YUV422:
            iInputFormat.iVideoFormat = ECVEI_UYVY;
            break;
        case PVMF_RGB24:
            iInputFormat.iVideoFormat = ECVEI_RGB24;
            break;
        case PVMF_RGB12:
            iInputFormat.iVideoFormat = ECVEI_RGB12;
            break;
        default:
            return PVMFFailure;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::SetInputFrameSize(uint32 aWidth, uint32 aHeight, uint8 aFrmOrient)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetInputFrameSize: Error iInterfaceState=%d", iInterfaceState));
            return false;
        default:
            break;
    }

    iInputFormat.iFrameWidth = aWidth;
    iInputFormat.iFrameHeight = aHeight;
    iInputFormat.iFrameOrientation = aFrmOrient;
    return true;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::SetInputFrameRate(OsclFloat aFrameRate)
{
    LOGV("SetInputFrameRate(%f)", aFrameRate);
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFOMXVideoEncNode::SetInputFrameRate: Error iInterfaceState=%d", iInterfaceState));
            return false;
        default:
            break;
    }

    if (aFrameRate < MIN_FRAME_RATE_IN_FPS) {
        LOGE("intended input frame rate is too low");
        return false;
    }
    iInputFormat.iFrameRate = OSCL_STATIC_CAST(float, aFrameRate);
    iEncodeParam.iNoFrameSkip = iEncodeParam.iNoCurrentSkip = false;
    return true;
}

////////////////////////////////////////////////////////////////////////////
PVMFFormatType PVMFOMXVideoEncNode::GetCodecType()
{
    switch (iEncodeParam.iContentType)
    {
        case ECVEI_H263:
            return PVMF_H263;
        case ECVEI_STREAMING:
        case ECVEI_DOWNLOAD:
            return PVMF_M4V;
        default:
            return PVMF_FORMAT_UNKNOWN;
    }
}

////////////////////////////////////////////////////////////////////////////
uint32 PVMFOMXVideoEncNode::GetOutputBitRate(uint32 aLayer)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::GetOutputBitRate: Error - Invalid layer number"));
        return 0;
    }

    return iEncodeParam.iBitRate[aLayer];
}

////////////////////////////////////////////////////////////////////////////
OsclFloat PVMFOMXVideoEncNode::GetOutputFrameRate(uint32 aLayer)
{
    LOGV("GetOutputFrameRate");
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::GetOutputFrameRate: Error Invalid layer number"));
        return 0;
    }
   
    return (OsclFloat)iEncodeParam.iFrameRate[aLayer];
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::GetOutputFrameSize(uint32 aLayer, uint32& aWidth, uint32& aHeight)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFOMXVideoEncNode::GetOutputFrameSize: Error Invalid layer number"));
        return PVMFFailure;
    }

    aWidth = iEncodeParam.iFrameWidth[aLayer];
    aHeight = iEncodeParam.iFrameHeight[aLayer];
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
uint32 PVMFOMXVideoEncNode::GetIFrameInterval()
{
    return iEncodeParam.iIFrameInterval;
}

////////////////////////////////////////////////////////////////////////////
//                 Event reporting routines.
////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::SetState(TPVMFNodeInterfaceState aState)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::SetState %d", aState));
    PVMFNodeInterface::SetState(aState);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_ERR((0, "PVMFOMXVideoEncNode::ReportErrorEvent: aEventType=%d aEventData=0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData);
}

////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_STACK_TRACE((0, "PVMFOMXVideoEncNode::ReportInfoEvent: aEventType=%d, aEventData0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData);
}

PVMFStatus PVMFOMXVideoEncNode::SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg)
{
    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();

    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

    // Set the timestamp
    sharedMediaCmdPtr->setTimestamp(aMsg->getTimestamp());

    // Set the sequence number
    sharedMediaCmdPtr->setSeqNum(aMsg->getSeqNum());

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);

    for (uint32 ii = 0; ii < iOutPort.size(); ii++)
    {
        PVMFStatus status = iOutPort[ii]->QueueOutgoingMsg(mediaMsgOut);

        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFOMXVideoEncNode::SendEndOfTrackCommand: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::SetDefaultCapabilityFlags()
{

    iIsOMXComponentMultiThreaded = true;

    iOMXComponentSupportsExternalOutputBufferAlloc = true;
    iOMXComponentSupportsExternalInputBufferAlloc = false;
    iOMXComponentSupportsMovableInputBuffers = false;

    iOMXComponentSupportsPartialFrames = false;
    iOMXComponentCanHandleIncompleteFrames = false;

    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::NegotiateComponentParameters()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() In"));

    OMX_ERRORTYPE Err;
    // first get the number of ports and port indices
    OMX_PORT_PARAM_TYPE VideoPortParameters;
    uint32 NumPorts;
    uint32 ii;

    // get starting number
    CONFIG_VERSION_SIZE(VideoPortParameters);
    Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamVideoInit, &VideoPortParameters);
    NumPorts = VideoPortParameters.nPorts; // must be at least 2 of them (in&out)

    if ( Err != OMX_ErrorNone || NumPorts < 2)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() There is insuffucient (%d) ports", NumPorts));
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
        Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);

        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with port %d ", ii));

            return false;
        }

        if (iParamPort.eDir == OMX_DirInput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Found Input port index %d ", ii));

            iInputPortIndex = ii;
            break;
        }
    }
    if (ii == VideoPortParameters.nStartPortNumber + NumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Cannot find any input port "));
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
        Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);

        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with port %d ", ii));

            return false;
        }

        if (iParamPort.eDir == OMX_DirOutput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Found Output port index %d ", ii));

            iOutputPortIndex = ii;
            break;
        }
    }
    if (ii == VideoPortParameters.nStartPortNumber + NumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Cannot find any output port "));
        return false;
    }

    CONFIG_VERSION_SIZE(iParamPort);
    //Input port
    iParamPort.nPortIndex = iInputPortIndex;
    Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with input port %d ", iInputPortIndex));
        return false;
    }

    uint32 width, height;
    GetOutputFrameSize(0, width, height);

    iParamPort.format.video.nFrameWidth = width;
    iParamPort.format.video.nFrameHeight = height;

    Err = OMX_SetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem setting parameters in input port %d ", iInputPortIndex));
        return false;
    }

    //Port 1 for output port
    CONFIG_VERSION_SIZE(iParamPort);

    iParamPort.nPortIndex = iOutputPortIndex;
    Err = OMX_GetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output port %d ", iOutputPortIndex));
        return false;
    }

    iParamPort.format.video.nFrameWidth = width;
    iParamPort.format.video.nFrameHeight = height;

    Err = OMX_SetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem setting parameters in output port %d ", iOutputPortIndex));
        return false;
    }

    // FIXME: This should consider non-mpeg4 component {
    //*********************************************************************************************
    //*********************************************************************************************
    OMX_VIDEO_PARAM_PROFILELEVELTYPE profileLevel; // OMX_IndexParamVideoProfileLevelCurrent
    CONFIG_VERSION_SIZE(profileLevel);
    Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamVideoProfileLevelCurrent, &profileLevel);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output profileLevel"));
        return false;
    }
    if (width <= 176)
    {
        profileLevel.eLevel = OMX_VIDEO_MPEG4Level0;
    }
    else
    {
        profileLevel.eLevel = OMX_VIDEO_MPEG4Level1;
    }
    Err = OMX_SetParameter(iOMXVideoEncoder, OMX_IndexParamVideoProfileLevelCurrent, &profileLevel);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output profileLevel"));
        return false;
    }
    //*********************************************************************************************
    //*********************************************************************************************
    // FIXME: This should consider non-mpeg4 component {

    OMX_VIDEO_PARAM_BITRATETYPE bitrate; // OMX_IndexParamVideoBitrate
    CONFIG_VERSION_SIZE(bitrate);
    Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamVideoBitrate, &bitrate);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output bitrate"));
        return false;
    }
    bitrate.eControlRate = OMX_Video_ControlRateVariable;
    bitrate.nTargetBitrate = GetOutputBitRate(0);
    Err = OMX_SetParameter(iOMXVideoEncoder, OMX_IndexParamVideoBitrate, &bitrate);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output bitrate"));
        return false;
    }

    OMX_CONFIG_FRAMERATETYPE framerate; // OMX_IndexConfigVideoFramerate
    CONFIG_VERSION_SIZE(framerate);
    Err = OMX_GetConfig(iOMXVideoEncoder, OMX_IndexConfigVideoFramerate, &framerate);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output framerate"));
        return false;
    }
    int nFrameRate = (int)GetOutputFrameRate(0);
    framerate.xEncodeFramerate = nFrameRate << 16;
    Err = OMX_SetConfig(iOMXVideoEncoder, OMX_IndexConfigVideoFramerate, &framerate);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output framerate"));
        return false;
    }

    CONFIG_VERSION_SIZE(iParamPort);
    //Input port
    iParamPort.nPortIndex = iInputPortIndex;
    Err = OMX_GetParameter(iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with input port %d ", iInputPortIndex));
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
                    (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Inport buffers %d,size %d", iNumInputBuffers, iOMXComponentInputBufferSize));

    Err = OMX_SetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem setting parameters in input port %d ", iInputPortIndex));
        return false;
    }

    //Port 1 for output port
    CONFIG_VERSION_SIZE(iParamPort);

    iParamPort.nPortIndex = iOutputPortIndex;
    Err = OMX_GetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem negotiating with output port %d ", iOutputPortIndex));
        return false;
    }

    iNumOutputBuffers = iParamPort.nBufferCountActual;
    if (iNumOutputBuffers > NUMBER_OUTPUT_BUFFER)
        iNumOutputBuffers = NUMBER_OUTPUT_BUFFER;

    iOMXComponentOutputBufferSize = iParamPort.nBufferSize;

    // FIXME:
    // Allocate larger output buffer to reduce the chance that qualcomm encoder
    // overflows the supplied buffers. This should never happen, if qualcomm
    // encoder checks the buffer size before writing output data to the buffer. 
    if (iInputFormat.iFrameRate > MIN_FRAME_RATE_IN_FPS) {
        // Qualcomm encoder rate control assumes fixed frame rate; thus, we have to
        // consider the worst case for buffer size allocation. The required buffer size
        // according to Qualcomm is inversely proportional to the input frame rate. 
        iOMXComponentOutputBufferSize *= (iInputFormat.iFrameRate / MIN_FRAME_RATE_IN_FPS);
    }
    iParamPort.nBufferSize = iOMXComponentOutputBufferSize;  // Keep encoder informed of the updated size
    
    if (iNumOutputBuffers < iParamPort.nBufferCountMin)
        iNumOutputBuffers = iParamPort.nBufferCountMin;

    iParamPort.nBufferCountActual = iNumOutputBuffers;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Outport buffers %d,size %d", iNumOutputBuffers, iOMXComponentOutputBufferSize));

    Err = OMX_SetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);
    if (Err != OMX_ErrorNone)
    {
        LOGE("NegotiateComponentParameters(): failed in setting parameters in output port %d ", iOutputPortIndex);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem setting parameters in output port %d ", iOutputPortIndex));
        return false;
    }

    // Get video color format
    OMX_VIDEO_PARAM_PORTFORMATTYPE VideoPortFormat;
    OMX_COLOR_FORMATTYPE VideoColorFormat;
    // init to unknown
    VideoColorFormat = OMX_COLOR_FormatUnused;
    CONFIG_VERSION_SIZE(VideoPortFormat);
    VideoPortFormat.nPortIndex = iInputPortIndex;
    VideoPortFormat.nIndex = 0; // read the preferred format - first

    // doing this in a while loop while incrementing nIndex will get all supported formats
    // until component says OMX_ErrorNoMore
    // For now, we just use the preferred one (with nIndex=0) assuming it is supported at MIO
    Err = OMX_GetParameter (iOMXVideoEncoder, OMX_IndexParamVideoPortFormat, &VideoPortFormat);
    if (Err != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem getting video port format"));
        return false;
    }
    // check if color format is valid
    if (VideoPortFormat.eCompressionFormat == OMX_VIDEO_CodingUnused)
    {
        // color format is valid, so read it
        VideoColorFormat = VideoPortFormat.eColorFormat;


        // Now set the format to confirm parameters
        CONFIG_VERSION_SIZE(VideoPortFormat);
        Err = OMX_SetParameter (iOMXVideoEncoder, OMX_IndexParamVideoPortFormat, &VideoPortFormat);
        if (Err != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::NegotiateComponentParameters() Problem setting video port format"));
            return false;
        }
    }

    // now that we have the color format, interpret it
    if (VideoColorFormat == OMX_COLOR_Format8bitRGB332)
    {
        iOMXComponentInputYUVFormat = PVMF_RGB8;
    }
    else if (VideoColorFormat == OMX_COLOR_Format12bitRGB444)
    {
        iOMXComponentInputYUVFormat = PVMF_RGB12;
    }
    else if (VideoColorFormat >= OMX_COLOR_Format16bitARGB4444 && VideoColorFormat <= OMX_COLOR_Format16bitBGR565)
    {
        iOMXComponentInputYUVFormat = PVMF_RGB16;
    }
    else if (VideoColorFormat >= OMX_COLOR_Format24bitRGB888 && VideoColorFormat <= OMX_COLOR_Format24bitARGB1887)
    {
        iOMXComponentInputYUVFormat = PVMF_RGB24;
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV420Planar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV420_PLANAR; // Y, U, V are separate - entire planes
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV420PackedPlanar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV420_PACKEDPLANAR; // each slice contains Y,U,V separate
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV420_SEMIPLANAR; // Y and UV interleaved - entire planes
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV422PackedSemiPlanar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV422_PACKEDSEMIPLANAR; // Y and UV interleaved - sliced
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV422Planar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV422_PLANAR; // Y, U, V are separate - entire planes
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV422PackedPlanar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV422_PACKEDPLANAR; // each slice contains Y,U,V separate
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV422SemiPlanar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV422_SEMIPLANAR; // Y and UV interleaved - entire planes
    }
    else if (VideoColorFormat == OMX_COLOR_FormatYUV422PackedSemiPlanar)
    {
        iOMXComponentInputYUVFormat = PVMF_YUV422_PACKEDSEMIPLANAR; // Y and UV interleaved - sliced
    }
    else if (VideoColorFormat == 0x7FA30C00) // TODO: Implementation specific value, need a better way to handle this
    {
        iOMXComponentInputYUVFormat = PVMF_YUV420_SEMIPLANAR_YVU; // semiplanar with Y and VU interleaved
    }
    else
    {
        iOMXComponentInputYUVFormat = PVMF_FORMAT_UNKNOWN;
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::CreateOutputMemPool(uint32 num_buffers)
{
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::CreateOutMemPool() start"));
	// In the case OMX component wants to allocate its own buffers,
	// mempool only contains OutputBufCtrlStructures (i.e. ptrs to buffer headers)
	// In case OMX component uses pre-allocated buffers (here),
	// mempool allocates OutputBufCtrlStructure (i.e. ptrs to buffer hdrs), followed by actual buffers

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::CreateOutMemPool() Allocating output buffer header pointers"));

	iOutputAllocSize = oscl_mem_aligned_size((uint32)sizeof(OutputBufCtrlStruct));

	if(iOMXComponentSupportsExternalOutputBufferAlloc)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::CreateOutMemPool() Allocating output buffers of size %d as well",iOMXComponentOutputBufferSize));
		// Actual output buffer size is based on pre-negotiated output buffer size
		iOutputAllocSize += (iOMXComponentOutputBufferSize + 4096);
	}
        LOGD("@@@@@@@@@@@@ iOutputAllocSize = %d and iOMXComponentOutputBufferSize = %d @@@@@@@@@@@@@", iOutputAllocSize, iOMXComponentOutputBufferSize);

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
		PVLOGMSG_ERR, (0,"PVMFOMXVideoEncNode::CreateOutMemPool() Memory pool structure for output buffers failed to allocate"));
		return false;
	}

	// allocate a dummy buffer to actually create the mempool
	OsclAny *dummy_alloc = NULL; // this dummy buffer will be released at end of scope
	leavecode = 0;
	OSCL_TRY(leavecode, dummy_alloc = iOutBufMemoryPool->allocate(iOutputAllocSize));
	if (leavecode || dummy_alloc == NULL)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
		PVLOGMSG_ERR, (0,"PVMFOMXVideoEncNode::CreateOutMemPool() Memory pool for output buffers failed to allocate"));
		return false;
	}
	iOutBufMemoryPool->deallocate(dummy_alloc);
	// init the counter
	iNumOutstandingOutputBuffers = 0;

	// allocate mempool for media data message wrapper
	leavecode = 0;
	OSCL_TRY(leavecode, iMediaDataMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (num_buffers, PVOMXVIDEOENC_MEDIADATA_CHUNKSIZE)) );
	if (leavecode || iMediaDataMemPool == NULL)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0,"PVMFOMXVideoEncNode::CreateOutMemPool() Media Data Buffer pool for output buffers failed to allocate"));
		return false;
	}

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::CreateOutMemPool() done"));
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Creates memory pool for input buffer management ///////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::CreateInputMemPool(uint32 num_buffers)
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
		(0,"PVMFOMXVideoEncNode::CreateInputMemPool() start "));

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::CreateInputMemPool() allocating buffer header pointers and shared media data ptrs "));

	iInputAllocSize = oscl_mem_aligned_size((uint32) sizeof(InputBufCtrlStruct)); //aligned_size_buffer_header_ptr+aligned_size_media_data_ptr;

	// Need to allocate buffers in the node either if component supports external buffers buffers
	// but they are not movable

	if(iOMXComponentSupportsExternalInputBufferAlloc)
	{
		//pre-negotiated input buffer size
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::CreateOutMemPool() Allocating input buffers of size %d as well",iOMXComponentInputBufferSize));

		iInputAllocSize += iOMXComponentInputBufferSize;
	}

	if (iInBufMemoryPool)
	{
		iInBufMemoryPool->removeRef();
		iInBufMemoryPool = NULL;
	}

	int32 leavecode = 0;
	OSCL_TRY(leavecode, iInBufMemoryPool = OSCL_NEW( OsclMemPoolFixedChunkAllocator, (num_buffers)););
	if (leavecode || iInBufMemoryPool == NULL)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
		PVLOGMSG_ERR, (0,"PVMFOMXVideoEncNode::CreateInputMemPool() Memory pool structure for input buffers failed to allocate"));
		return false;
	}
	// try to allocate a dummy buffer to actually create the mempool and allocate the needed memory
	// allocate a dummy buffer to actually create the mempool, this dummy buffer will be released at end of scope of this method
	OsclAny *dummy_alloc = NULL;
	leavecode = 0;
	OSCL_TRY(leavecode, dummy_alloc = iInBufMemoryPool->allocate(iInputAllocSize));
	if (leavecode || dummy_alloc==NULL)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
		PVLOGMSG_ERR, (0,"PVMFOMXVideoEncNode::CreateInputMemPool() Memory pool for input buffers failed to allocate"));
		return false;
	}
	iInBufMemoryPool->deallocate(dummy_alloc);

	// init the counter
	iNumOutstandingInputBuffers = 0;

	iInputBufferToResendToComponent = NULL; // nothing to resend yet

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::CreateInputMemPool() done"));
	return true;
}

////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::ProvideBuffersToComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
										  uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  uint32 aNumBuffers,    // number of buffers
										  uint32 aActualBufferSize, // aactual buffer size
										  uint32 aPortIndex,      // port idx
										  bool aUseBufferOK,		// can component use OMX_UseBuffer or should it use OMX_AllocateBuffer
										  bool	aIsThisInputBuffer		// is this input or output
										  )
{
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::ProvideBuffersToComponent() enter"));

	uint32 ii=0;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OsclAny **ctrl_struct_ptr = NULL;	// temporary array to keep the addresses of buffer ctrl structures and buffers

	ctrl_struct_ptr = (OsclAny **) oscl_malloc(aNumBuffers * sizeof(OsclAny *));
	if(ctrl_struct_ptr == NULL)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::ProvideBuffersToComponent ctrl_struct_ptr == NULL"));
		return false;
	}

	// Now, go through all buffers and tell component to
	// either use a buffer, or to allocate its own buffer
	for (ii = 0; ii < aNumBuffers; ii++)
	{
		int32 errcode=0;
		// get the address where the buf hdr ptr will be stored
		OSCL_TRY(errcode, ctrl_struct_ptr[ii] = (OsclAny *) aMemPool->allocate(aAllocSize));
		if ( (errcode != OsclErrNone) || (ctrl_struct_ptr[ii] == NULL) )
		{
			if (errcode == OsclErrNoResources)
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::ProvideBuffersToComponent ->allocate() failed for no mempool chunk available"));
			}
			else
			{
				// General error
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
				(0,"PVMFOMXVideoEncNode::ProvideBuffersToComponent ->allocate() failed due to some general error"));

				ReportErrorEvent(PVMFFailure);
				SetState(EPVMFNodeError);
			}

			return false;
		}

		memset(ctrl_struct_ptr[ii], 0, aAllocSize);

		if(aUseBufferOK)
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
			if(aIsThisInputBuffer)
			{
				InputBufCtrlStruct *temp = (InputBufCtrlStruct *) ctrl_struct_ptr[ii];
				temp->pMediaData = PVMFSharedMediaDataPtr(NULL,NULL);

				// advance ptr to skip the structure
				pB += oscl_mem_aligned_size(sizeof(InputBufCtrlStruct));

				err = OMX_UseBuffer(iOMXVideoEncoder,	// hComponent
								&(temp->pBufHdr),		// address where ptr to buffer header will be stored
								aPortIndex,				// port index (for port for which buffer is provided)
								ctrl_struct_ptr[ii],	// App. private data = pointer to beginning of allocated data
														//				to have a context when component returns with a callback (i.e. to know
														//				what to free etc.
								(OMX_U32)aActualBufferSize,	// buffer size
								pB);						// buffer data ptr

			}
			else
			{
				OutputBufCtrlStruct *temp = (OutputBufCtrlStruct *) ctrl_struct_ptr[ii];

				// advance buffer ptr to skip the structure
				pB += oscl_mem_aligned_size(sizeof(OutputBufCtrlStruct));

					err = OMX_UseBuffer(iOMXVideoEncoder,	// hComponent
								&(temp->pBufHdr),		// address where ptr to buffer header will be stored
								aPortIndex,				// port index (for port for which buffer is provided)
								ctrl_struct_ptr[ii],	// App. private data = pointer to beginning of allocated data
														//				to have a context when component returns with a callback (i.e. to know
														//				what to free etc.
								(OMX_U32)aActualBufferSize,	// buffer size
								pB);						// buffer data ptr
			}
		}
		else{
			// the component must allocate its own buffers.
			if(aIsThisInputBuffer)
			{

				InputBufCtrlStruct *temp = (InputBufCtrlStruct *) ctrl_struct_ptr[ii];
				// make sure ptrs are initialized to NULL
				temp->pMediaData = PVMFSharedMediaDataPtr(NULL,NULL);

				err = OMX_AllocateBuffer(iOMXVideoEncoder,
									&(temp->pBufHdr),
									aPortIndex,
									ctrl_struct_ptr[ii],
									(OMX_U32)aActualBufferSize);
			}
			else
			{
				OutputBufCtrlStruct *temp = (OutputBufCtrlStruct *) ctrl_struct_ptr[ii];
				err = OMX_AllocateBuffer(iOMXVideoEncoder,
									&(temp->pBufHdr),
									aPortIndex,
									ctrl_struct_ptr[ii],
									(OMX_U32)aActualBufferSize);
			}

		}

		if (err != OMX_ErrorNone)
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
			(0,"PVMFOMXVideoEncNode::ProvideBuffersToComponent() Problem using/allocating a buffer"));

			return false;
		}

	}

	for(ii = 0; ii < aNumBuffers; ii++)
	{
		// after initializing the buffer hdr ptrs, return them
		// to the mempool
		aMemPool->deallocate((OsclAny*) ctrl_struct_ptr[ii]);
	}

	oscl_free(ctrl_struct_ptr);
	// set the flags
	if(aIsThisInputBuffer)
	{
		iInputBuffersFreed = false;
	}
	else
	{
		iOutputBuffersFreed = false;
	}

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::ProvideBuffersToComponent() done"));
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::FreeBuffersFromComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
										  uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  uint32 aNumBuffers,    // number of buffers
										  uint32 aPortIndex,      // port idx
										  bool	aIsThisInputBuffer		// is this input or output
										  )
{
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::FreeBuffersToComponent() enter"));

	uint32 ii=0;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	OsclAny **ctrl_struct_ptr = NULL;	// temporary array to keep the addresses of buffer ctrl structures and buffers

	ctrl_struct_ptr = (OsclAny **) oscl_malloc(aNumBuffers * sizeof(OsclAny *));
	if(ctrl_struct_ptr == NULL)
	{
		return false;
	}

	// Now, go through all buffers and tell component to free them
	for (ii = 0; ii < aNumBuffers; ii++)
	{
		int32 errcode=0;
		// get the address where the buf hdr ptr will be stored

		OSCL_TRY(errcode, ctrl_struct_ptr[ii] = (OsclAny *) aMemPool->allocate(aAllocSize));
		if ( (errcode != OsclErrNone) || (ctrl_struct_ptr[ii] == NULL) )
		{
			if (errcode == OsclErrNoResources)
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::FreeBuffersFromComponent ->allocate() failed for no mempool chunk available"));
			}
			else
			{
				// General error
				PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
				(0,"PVMFOMXVideoEncNode::FreeBuffersFromComponent ->allocate() failed due to some general error"));

				ReportErrorEvent(PVMFFailure);
				SetState(EPVMFNodeError);
			}

			return false;
		}
		// to maintain correct count
		aMemPool->notifyfreechunkavailable( (*this), (OsclAny*) aMemPool);

		if(aIsThisInputBuffer)
		{
			iNumOutstandingInputBuffers++;
			// get the buf hdr pointer
			InputBufCtrlStruct *temp = (InputBufCtrlStruct *) ctrl_struct_ptr[ii];
			err = OMX_FreeBuffer(iOMXVideoEncoder,
								aPortIndex,
								temp->pBufHdr);
		}
		else
		{
			iNumOutstandingOutputBuffers++;
			OutputBufCtrlStruct *temp = (OutputBufCtrlStruct *) ctrl_struct_ptr[ii];
			err = OMX_FreeBuffer(iOMXVideoEncoder,
								 aPortIndex,
								 temp->pBufHdr);
		}

		if (err != OMX_ErrorNone)
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
			(0,"PVMFOMXVideoEncNode::FreeBuffersFromComponent() Problem freeing a buffer"));

			return false;
		}

	}

	for(ii = 0; ii < aNumBuffers; ii++)
	{
		// after freeing the buffer hdr ptrs, return them
		// to the mempool (which will itself then be deleted promptly)
		aMemPool->deallocate((OsclAny*) ctrl_struct_ptr[ii]);
	}

	oscl_free(ctrl_struct_ptr);

	// mark buffers as freed (so as not to do it twice)
	if(aIsThisInputBuffer)
	{
		iInputBuffersFreed = true;
	}
	else
	{
		iOutputBuffersFreed = true;
	}
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::FreeBuffersFromComponent() done"));
	return true;
}

// Callback processing in multithreaded case - dequeued event - call EventHandlerProcessing
OsclReturnCode PVMFOMXVideoEncNode::ProcessCallbackEventHandler_MultiThreaded(OsclAny* P)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "PVMFOMXVideoEncNode::ProcessCallbackEventHandler_MultiThreaded: In"));
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
OsclReturnCode PVMFOMXVideoEncNode::ProcessCallbackEmptyBufferDone_MultiThreaded(OsclAny* P)
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
OsclReturnCode PVMFOMXVideoEncNode::ProcessCallbackFillBufferDone_MultiThreaded(OsclAny* P)
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

/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR EVENT HANDLER
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXVideoEncNode::EventHandlerProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_EVENTTYPE aEvent,
        OMX_OUT OMX_U32 aData1,
        OMX_OUT OMX_U32 aData2,
        OMX_OUT OMX_PTR aEventData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: In"));
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
                case OMX_CommandPortDisable:
                {
                    // if port disable command is done, we can re-allocate the buffers and re-enable the port

                    iProcessingState = EPVMFOMXVideoEncNodeProcessingState_PortReEnable;
                    iPortIndexForDynamicReconfig = aData2;

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
                                        (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_CommandPortEnable - completed on port %d, dynamic reconfiguration needed on port %d", aData2, iSecondPortToReconfig));

                        iProcessingState = EPVMFOMXVideoEncNodeProcessingState_PortReconfig;
                        iPortIndexForDynamicReconfig = iSecondPortToReconfig;
                        iSecondPortReportedChange = false;
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_CommandPortEnable - completed on port %d, resuming normal data flow", aData2));
                        iProcessingState = EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode;
                        iDynamicReconfigInProgress = false;
                    }
                    RunIfNotReady();
                    break;
                }
                case OMX_CommandMarkBuffer:
                    // nothing to do here yet;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_CommandMarkBuffer - completed - no action taken"));

                    break;

                default:
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: Unsupported event"));
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
                                (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventError - Bitstream corrupt error"));
                // Errors from corrupt bitstream are reported as info events
                ReportInfoEvent(PVMFInfoProcessingFailure, NULL);

            }
            else
            {

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventError"));
                // for now, any error from the component will be reported as error
                ReportErrorEvent(PVMF_OMX_VIDEOENC_NODE_ERROR_ENCODE_ERROR, NULL);
                SetState(EPVMFNodeError);
            }
            break;

        }

        case OMX_EventBufferFlag:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventBufferFlag (EOS) flag returned from OMX component"));

            RunIfNotReady();
            break;
        }//end of case OMX_EventBufferFlag

        case OMX_EventMark:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventMark returned from OMX component - no action taken"));

            RunIfNotReady();
            break;
        }//end of case OMX_EventMark

        case OMX_EventPortSettingsChanged:
        {

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned from OMX component"));

            // first check if dynamic reconfiguration is already in progress,
            // if so, wait until this is completed, and then initiate the 2nd reconfiguration
            if (iDynamicReconfigInProgress)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned for port %d, dynamic reconfig already in progress", aData1));

                iSecondPortToReconfig = aData1;
                iSecondPortReportedChange = true;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventPortSettingsChanged returned for port %d", aData1));

                iProcessingState = EPVMFOMXVideoEncNodeProcessingState_PortReconfig;
                iPortIndexForDynamicReconfig = aData1;
                iDynamicReconfigInProgress = true;
            }

            RunIfNotReady();
            break;
        }//end of case OMX_PortSettingsChanged

        case OMX_EventResourcesAcquired:        //not supported
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::EventHandlerProcessing: OMX_EventResourcesAcquired returned from OMX component - no action taken"));

            RunIfNotReady();

            break;
        }

        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::EventHandlerProcessing:  Unknown Event returned from OMX component - no action taken"));

            break;
        }

    }//end of switch (eEvent)

    return OMX_ErrorNone;
}



/////////////////////////////////////////////////////////////////////////////
// This function handles the event of OMX component state change
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::HandleComponentStateChange(OMX_U32 encoder_state)
{
    switch (encoder_state)
    {
        case OMX_StateIdle:
        {
            iCurrentEncoderState = OMX_StateIdle;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::HandleComponentStateChange: OMX_StateIdle reached"));

            //  this state can be reached either going from OMX_Loaded->OMX_Idle (preparing)
            //	or going from OMX_Executing->OMX_Idle (stopping)

            if ((iCurrentCmd.size() > 0) &&
                    (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_PREPARE))
            {
                iProcessingState = EPVMFOMXVideoEncNodeProcessingState_InitEncoder;
                SetState(EPVMFNodePrepared);
                CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);
                RunIfNotReady();
            }
            else if ((iCurrentCmd.size() > 0) &&
                     (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_STOP))
            {
                // if we are stopped, we won't start until the node gets DoStart command.
                // in this case, we are ready to start sending buffers
                if (iProcessingState == EPVMFOMXVideoEncNodeProcessingState_Stopping)
                    iProcessingState = EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode;
                // if the processing state was not stopping, leave the state as it was (continue port reconfiguration)
                SetState(EPVMFNodePrepared);
                CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);

                RunIfNotReady();
            }
            else if ((iCurrentCmd.size() > 0) &&
                     (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_RESET))
            {
                // State change to Idle was initiated due to Reset. First need to reach idle, and then loaded
                // Once Idle is reached, we need to initiate idle->loaded transition
                RunIfNotReady();
            }
            else if ((iCurrentCmd.size() > 0) &&
                     (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH))
            {

                SetState(EPVMFNodePrepared);
                CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);

                // State change to Idle was initiated due to Reset. First need to reach idle, and then loaded
                // Once Idle is reached, we need to initiate idle->loaded transition
                RunIfNotReady();
            }
			break;
        }//end of case OMX_StateIdle

        case OMX_StateExecuting:
        {
            iCurrentEncoderState = OMX_StateExecuting;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::HandleComponentStateChange: OMX_StateExecuting reached"));

            // this state can be reached going from OMX_Idle -> OMX_Executing (preparing)
            //	or going from OMX_Pause -> OMX_Executing (coming from pause)
            //	either way, this is a response to "DoStart" command

            if ((iCurrentCmd.size() > 0) &&
                    (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_START))
            {
                SetState(EPVMFNodeStarted);
                CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);

                RunIfNotReady();
            }

            break;
        }//end of case OMX_StateExecuting

        case OMX_StatePause:
        {
            iCurrentEncoderState = OMX_StatePause;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::HandleComponentStateChange: OMX_StatePause reached"));


            //	This state can be reached going from OMX_Executing-> OMX_Pause
            if ((iCurrentCmd.size() > 0) &&
                    (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_PAUSE))
            {
                // if we are paused, we won't start until the node gets DoStart command.
                //	in this case, we are ready to start sending buffers
                if (iProcessingState == EPVMFOMXVideoEncNodeProcessingState_Pausing)
                    iProcessingState = EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode;
                // if the processing state was not pausing, leave the state as it was (continue port reconfiguration)


                SetState(EPVMFNodePaused);
                CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);
                RunIfNotReady();
            }

            break;
        }//end of case OMX_StatePause

        case OMX_StateLoaded:
        {
            iCurrentEncoderState = OMX_StateLoaded;

            //  this state can be reached only going from OMX_Idle ->OMX_Loaded (stopped to reset)
            //

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::HandleComponentStateChange: OMX_StateLoaded reached"));
            //Check if command's responce is pending
            if ((iCurrentCmd.size() > 0) &&
                ((iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_RESET) ||
                 (iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH)))
            {
                // move this here
                while (!iInPort.empty())
                    iInPort.Erase(&iInPort.front());
                while (!iOutPort.empty())
                    iOutPort.Erase(&iOutPort.front());

                iDataIn.Unbind();

                iProcessingState = EPVMFOMXVideoEncNodeProcessingState_Idle;
                //logoff & go back to Created state.
                SetState(EPVMFNodeIdle);
                PVMFStatus status = ThreadLogoff();
                CommandComplete(iCurrentCmd, iCurrentCmd.front(), status);
                iResetInProgress = false;
                iResetMsgSent = false;
                DeleteVideoEncoder();
            }

            break;
        }//end of case OMX_StateLoaded

        case OMX_StateInvalid:
        default:
        {
            iCurrentEncoderState = OMX_StateInvalid;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFOMXVideoEncNode::HandleComponentStateChange: OMX_StateInvalid reached"));

            break;
        }//end of case OMX_StateInvalid

    }

}

/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR EMPTY BUFFER DONE - input buffer was consumed
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXVideoEncNode::EmptyBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoEncNode::EmptyBufferDoneProcessing: In"));
    
    LOGV("==> %s: mInputBufferRefCount = %d", __FUNCTION__, mInputBufferRefCount);
    if (mInputBufferRefCount > 0) {
        --mInputBufferRefCount;
    }
    OSCL_ASSERT((void*) aComponent == (void*) iOMXVideoEncoder); // component should match the component
    OSCL_ASSERT(aAppData == (OMX_PTR) (this));		// AppData should represent this node ptr

    // first, get the buffer "context", i.e. pointer to application private data that contains the
    // address of the mempool buffer (so that it can be released)
    InputBufCtrlStruct *pContext = (InputBufCtrlStruct *) (aBuffer->pAppPrivate);

    // if a buffer is not empty, log a msg, but release anyway
    if ( (aBuffer->nFilledLen > 0) && (iDoNotSaveInputBuffersFlag == false) )
        // if dynamic port reconfig is in progress for input port, don't keep the buffer
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::EmptyBufferDoneProcessing: Input buffer returned non-empty with %d bytes still in it", aBuffer->nFilledLen));


    }

    iInputBufferToResendToComponent = NULL;


    // input buffer is to be released,
    // refcount needs to be decremented (possibly - the input msg associated with the buffer will be unbound)
    // NOTE: in case of "moveable" input buffers (passed into component without copying), unbinding decrements a refcount which eventually results
    //			in input message being released back to upstream mempool once all its fragments are returned
    //		in case of input buffers passed into component by copying, unbinding has no effect
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoEncNode::EmptyBufferDoneProcessing: Release input buffer (with %d refcount remaining of input message)", (pContext->pMediaData).get_count() - 1 ));


    (pContext->pMediaData).Unbind();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoEncNode::EmptyBufferDoneProcessing: Release input buffer %x back to mempool", pContext));

    iInBufMemoryPool->deallocate((OsclAny *) pContext);


    // the OMX spec says that no error is to be returned
    return OMX_ErrorNone;

}


/////////////////////////////////////////////////////////////////////////////
////////////////////// CALLBACK PROCESSING FOR FILL BUFFER DONE - output buffer is ready
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE PVMFOMXVideoEncNode::FillBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: In"));

    OSCL_ASSERT((void*) aComponent == (void*) iOMXVideoEncoder); // component should match the component
    OSCL_ASSERT(aAppData == (OMX_PTR) (this));		// AppData should represent this node ptr

    // first, get the buffer "context", i.e. pointer to application private data that contains the
    // address of the mempool buffer (so that it can be released)
    OsclAny *pContext = (OsclAny*) aBuffer->pAppPrivate;


    // check for EOS flag
    if ( (aBuffer->nFlags & OMX_BUFFERFLAG_EOS) )
    {
        // EOS received
        iIsEOSReceivedFromComponent = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Output buffer has EOS set"));

    }

    // if a buffer is empty, or if it should not be sent downstream (say, due to state change)
    // release the buffer back to the pool
    if ( (aBuffer->nFilledLen == 0) || (iDoNotSendOutputBuffersDownstreamFlag == true) )
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Release output buffer %x back to mempool - buffer empty or not to be sent downstream", pContext));

        iOutBufMemoryPool->deallocate(pContext);
    }
    else
    {

#if 0
        // iFrameCounter was not defined. The information about the total number of output frame
        // might be useful, but removed it for now. -jdong
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Output frame %d received", iFrameCounter++));
#endif

        // get pointer to actual buffer data
        uint8 *pBufdata = ((uint8*) aBuffer->pBuffer);
        // move the data pointer based on offset info
        pBufdata += aBuffer->nOffset;

		if ((iSeqNum == 0) && (((PVMFVideoEncPort*)iOutPort[0])->iFormat == PVMF_M4V))
		{
#if 0
			const char frame_header_start[4] = {0x00, 0x00, 0x01, 0xB6};
			int i;

			iVOLSize = 0;
			for (i=0; i<aBuffer->nFilledLen-4; i++)
			{
				if (memcmp(&pBufdata[i], frame_header_start, 4) != 0)
				{
					iVOLHeader[i] = pBufdata[i];
					iVOLSize++;
				}
				else
				{
					break;
				}
			}
#else
			memcpy(iVolHeader.getMemFragPtr(), pBufdata, DEFAULT_VOL_HEADER_LENGTH);
#endif
		}


		aBuffer->nTimeStamp /= 1000;
        iOutTimeStamp = aBuffer->nTimeStamp;
        //ipPrivateData =  aBuffer->pPlatformPrivate; // record the pointer
        oscl_memcpy(&ipPrivateData, &(aBuffer->pPlatformPrivate), sizeof(ipPrivateData));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Wrapping buffer %x of size %d", pBufdata, aBuffer->nFilledLen));
        // wrap the buffer into the MediaDataImpl wrapper, and queue it for sending downstream
        // wrapping will create a refcounter. When refcounter goes to 0 i.e. when media data
        // is released in downstream components, the custom deallocator will automatically release the buffer back to the
        //	mempool. To do that, the deallocator needs to have info about Context
        // NOTE: we had to wait until now to wrap the buffer data because we only know
        //			now where the actual data is located (based on buffer offset)
        OsclSharedPtr<PVMFMediaDataImpl> MediaDataOut = WrapOutputBuffer(pBufdata, (uint32) (aBuffer->nFilledLen), pContext);
        if (aBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME) {
            MediaDataOut->setMarkerInfo(PVMF_MEDIA_DATA_MARKER_INFO_RANDOM_ACCESS_POINT_BIT);
        }

        // if you can't get the MediaDataOut, release the buffer back to the pool
        if (MediaDataOut.GetRep() == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Problem wrapping buffer %x of size %d - releasing the buffer", pBufdata, aBuffer->nFilledLen));

            iOutBufMemoryPool->deallocate(pContext);
        }
        else
        {

            // if there's a problem queuing output buffer, MediaDataOut will expire at end of scope and
            // release buffer back to the pool, (this should not be the case)
            if (QueueOutputBuffer(MediaDataOut, aBuffer->nFilledLen))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Buffer %x of size %d queued - reschedule the node to send out", pBufdata, aBuffer->nFilledLen));

                // if queing went OK,
                // re-schedule the node so that outgoing queue can be emptied (unless the outgoing port is busy)
                if ( (iOutPort.size() > 0) && !(iOutPort[0]->IsConnectedPortBusy()) )
                    RunIfNotReady();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFOMXVideoEncNode::FillBufferDoneProcessing: Problem queing buffer %x of size %d - releasing the buffer", pBufdata, aBuffer->nFilledLen));
            }


        }

    }
    // the OMX spec says that no error is to be returned
    return OMX_ErrorNone;

}

//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Attach a MediaDataImpl wrapper (refcount, deallocator etc.)
/////////////////////////////// to the output buffer /////////////////////////////////////////
OsclSharedPtr<PVMFMediaDataImpl> PVMFOMXVideoEncNode::WrapOutputBuffer(uint8 *pData,uint32 aDataLen, OsclAny *pContext)
{
	// wrap output buffer into a mediadataimpl
	 uint32 aligned_class_size = oscl_mem_aligned_size(sizeof(PVMFSimpleMediaBuffer));
	 uint32 aligned_cleanup_size = oscl_mem_aligned_size(sizeof(PVOMXBufferSharedPtrWrapperCombinedCleanupDA));
	 uint32 aligned_refcnt_size = oscl_mem_aligned_size(sizeof (OsclRefCounterDA));
	 uint8 *my_ptr = (uint8*) oscl_malloc(aligned_refcnt_size + aligned_cleanup_size + aligned_class_size);

	if(my_ptr == NULL)
	{
		OsclSharedPtr<PVMFMediaDataImpl> null_buff(NULL,NULL);
		return null_buff;
	}
	 // create a deallocator and pass the buffer_allocator to it as well as pointer to data that needs to be returned to the mempool
	 PVOMXBufferSharedPtrWrapperCombinedCleanupDA *cleanup_ptr =
		 OSCL_PLACEMENT_NEW(my_ptr + aligned_refcnt_size, PVOMXBufferSharedPtrWrapperCombinedCleanupDA(iOutBufMemoryPool,pContext) );

	//ModifiedPvciBufferCombinedCleanup* cleanup_ptr = OSCL_PLACEMENT_NEW(my_ptr + aligned_refcnt_size,ModifiedPvciBufferCombinedCleanup(aOutput.GetRefCounter()) );

    // create the ref counter after the cleanup object (refcount is set to 1 at creation)
    OsclRefCounterDA *my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterDA(my_ptr,cleanup_ptr) );

    my_ptr += aligned_refcnt_size + aligned_cleanup_size;

	PVMFMediaDataImpl* media_data_ptr = OSCL_PLACEMENT_NEW(my_ptr, PVMFSimpleMediaBuffer((void *) pData, // ptr to data
							                                             aDataLen, // capacity
													                     my_refcnt ) ); // ref counter

	OsclSharedPtr<PVMFMediaDataImpl> MediaDataImplOut(media_data_ptr, my_refcnt);

	MediaDataImplOut->setMediaFragFilledLen(0,aDataLen);

	return MediaDataImplOut;

}

bool PVMFOMXVideoEncNode::QueueOutputBuffer(OsclSharedPtr<PVMFMediaDataImpl> &mediadataimplout,uint32 aDataLen)
{
	bool status = true;
	PVMFSharedMediaDataPtr mediaDataOut;
	int32 leavecode = 0;

	// NOTE: ASSUMPTION IS THAT OUTGOING QUEUE IS BIG ENOUGH TO QUEUE ALL THE OUTPUT BUFFERS
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::QueueOutputFrame: In"));

// FIXME: Is this correct?
// Remove checking IsOutgoingQueueBusy
/*
	// First check if we can put outgoing msg. into the queue
	if (iOutPort[0]->IsOutgoingQueueBusy())
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
			(0,"PVMFOMXVideoEncNode::QueueOutputFrame() OutgoingQueue is busy"));
		return false;
	}
*/
	OSCL_TRY(leavecode,
		mediaDataOut = PVMFMediaData::createMediaData(mediadataimplout,iMediaDataMemPool););
	if (leavecode==0)
	{
		// Update the filled length of the fragment
		mediaDataOut->setMediaFragFilledLen(0, aDataLen);
		// Set timestamp
		mediaDataOut->setTimestamp(iOutTimeStamp);
		// Set Streamid
		//mediaDataOut->setStreamID(iStreamID);
		// Set sequence number
		mediaDataOut->setSeqNum(iSeqNum++);

		// Send vol header for m4v bitstream
		if(mediaDataOut->getSeqNum() == 0 && iEncodeParam.iContentType == ECVEI_STREAMING)
			mediaDataOut->setFormatSpecificInfo(iVolHeader);

		SendEncodedBitstream(mediaDataOut);
	}

	return status;
}

/////////////////////////////////////////////////////////////////////////////
// This routine will process received data usign State Machine
/////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFOMXVideoEncNode::HandleProcessingState()
{
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::HandleProcessingState() In"));

	PVMFStatus status = PVMFSuccess;

	switch(iProcessingState)
	{
		case EPVMFOMXVideoEncNodeProcessingState_InitEncoder:
		{
			// do init only if input data is available
			if(iDataIn.GetRep() != NULL)
			{
				//if (!InitEncoder(iDataIn))
				//{
				//	// Encoder initialization failed. Fatal error
				//	PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
				//		(0,"PVMFOMXVideoEncNode::HandleProcessingState() encoder initialization failed"));
				//	ReportErrorEvent(PVMFErrResourceConfiguration);
				//	ChangeNodeState(EPVMFNodeError);
				//	break;
				//}

				iProcessingState = EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode;
				// spin once to send output buffers
				RunIfNotReady();
				status = PVMFSuccess; // allow rescheduling
			}
			break;
		}

		case EPVMFOMXVideoEncNodeProcessingState_WaitForInitCompletion:
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::HandleProcessingState() WaitForInitCompletion -> wait for config buffer to return"));

			status = PVMFErrNoMemory; // prevent rescheduling
			break;
		}
		// The FOLLOWING 4 states handle Dynamic Port Reconfiguration
		case EPVMFOMXVideoEncNodeProcessingState_PortReconfig:
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Sending Port Disable Command"));

			// port reconfiguration is required. Only one port at a time is disabled and then re-enabled after buffer resizing
			OMX_SendCommand(iOMXVideoEncoder, OMX_CommandPortDisable, iPortIndexForDynamicReconfig, NULL);
			// the port will now start returning outstanding buffers
			// set the flag to prevent output from going downstream (in case of output port being reconfigd)
			// set the flag to prevent input from being saved and returned to component (in case of input port being reconfigd)
			// set the state to wait for port saying it is disabled
			if(iPortIndexForDynamicReconfig == iOutputPortIndex)
			{
				iDoNotSendOutputBuffersDownstreamFlag = true;
			}
			else if(iPortIndexForDynamicReconfig == iInputPortIndex)
			{
				iDoNotSaveInputBuffersFlag = true;

			}
			iProcessingState = EPVMFOMXVideoEncNodeProcessingState_WaitForBufferReturn;

			// fall through to the next case to check if all buffers are already back
		}

		case EPVMFOMXVideoEncNodeProcessingState_WaitForBufferReturn:
		{
			// as buffers are coming back, Run may be called, wait until all buffers are back, then Free them all
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> WaitForBufferReturn "));
			// check if it's output port being reconfigured
			if(iPortIndexForDynamicReconfig == iOutputPortIndex)
			{
				// if all buffers have returned, free them
				if(iNumOutstandingOutputBuffers == 0)
				{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> all output buffers are back, free them"));
					if(false == iOutputBuffersFreed)
					{
						if( !FreeBuffersFromComponent(iOutBufMemoryPool, // allocator
										  iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  iNumOutputBuffers, // number of buffers
										  iOutputPortIndex, // port idx
										  false // this is not input
										  ))
						{
							PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
							(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Cannot free output buffers "));

							SetState(EPVMFNodeError);
							ReportErrorEvent(PVMFErrNoMemory);
							return PVMFErrNoMemory;
						}
					}
					// if the callback (that port is disabled) has not arrived yet, wait for it
					// if it has arrived, it will set the state to PortReEnable
					if(iProcessingState != EPVMFOMXVideoEncNodeProcessingState_PortReEnable)
						iProcessingState = EPVMFOMXVideoEncNodeProcessingState_WaitForPortDisable;

					status = PVMFSuccess; // allow rescheduling of the node potentially
				}
				else
					status = PVMFErrNoMemory; // must wait for buffers to come back. No point in automatic rescheduling
											// but each buffer will reschedule the node when it comes in
			}
			else
			{ // this is input port

				// if all buffers have returned, free them
				if(iNumOutstandingInputBuffers == 0)
				{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> all input buffers are back, free them"));
					if(false == iInputBuffersFreed){
						if( !FreeBuffersFromComponent(iInBufMemoryPool, // allocator
											  iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
											  iNumInputBuffers, // number of buffers
											  iInputPortIndex, // port idx
											  true // this is input
											  ))
						{
							PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
							(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Cannot free input buffers "));

							SetState(EPVMFNodeError);
							ReportErrorEvent(PVMFErrNoMemory);
							return PVMFErrNoMemory;

						}
					}
					// if the callback (that port is disabled) has not arrived yet, wait for it
					// if it has arrived, it will set the state to PortReEnable
					if(iProcessingState != EPVMFOMXVideoEncNodeProcessingState_PortReEnable)
						iProcessingState = EPVMFOMXVideoEncNodeProcessingState_WaitForPortDisable;

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

		case EPVMFOMXVideoEncNodeProcessingState_WaitForPortDisable:
		{

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> wait for port disable callback"));
			// do nothing. Just wait for the port to become disabled (we'll get event from component, which will
			// transition the state to PortReEnable
			status = PVMFErrNoMemory; // prevent Rescheduling the node
			break;
		}

		case EPVMFOMXVideoEncNodeProcessingState_PortReEnable:
		{

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Sending reenable port command"));
			// set the port index so that we get parameters for the proper port
			iParamPort.nPortIndex = iPortIndexForDynamicReconfig;
			//iParamPort.nVersion = OMX_VERSION;

			// get new parameters of the port
			OMX_GetParameter (iOMXVideoEncoder, OMX_IndexParamPortDefinition, &iParamPort);

			// send command for port re-enabling (for this to happen, we must first recreate the buffers)
			OMX_SendCommand(iOMXVideoEncoder, OMX_CommandPortEnable, iPortIndexForDynamicReconfig, NULL);

			// is this output port?
			if(iPortIndexForDynamicReconfig == iOutputPortIndex)
			{
				iOMXComponentOutputBufferSize = ((iParamPort.format.video.nFrameWidth+15)&(~15))*((iParamPort.format.video.nFrameHeight + 15) &(~15)) * 3/2;

				// check the new buffer size
				if(iInPort.size())
				{
					switch (((PVMFVideoEncPort*)iInPort[0])->iFormat)
					{
						case PVMF_H264:
						case PVMF_H264_MP4:
						case PVMF_H264_RAW:
						case PVMF_M4V:
						case PVMF_H263:
							iOMXComponentOutputBufferSize = ((iParamPort.format.video.nFrameWidth + 15)&(~15)) * ((iParamPort.format.video.nFrameHeight + 15)&(~15)) * 3/2;
							break;
						case PVMF_WMV: // This is a requirement for the WMV encoder that we have currently
							iOMXComponentOutputBufferSize = ((iParamPort.format.video.nFrameWidth + 3)&(~3)) * (iParamPort.format.video.nFrameHeight) * 3/2;
							break;
						default:
							OSCL_ASSERT(false);
							break;
					}
				}
				// FIXME: Is this needed?
				// set the new width / height
				//iYUVWidth =  iParamPort.format.video.nFrameWidth;
				//iYUVHeight = iParamPort.format.video.nFrameHeight;

				if(iOMXComponentOutputBufferSize < iParamPort.nBufferSize)
					iOMXComponentOutputBufferSize = iParamPort.nBufferSize;

				// do we need to increase the number of buffers?
				if(iNumOutputBuffers < iParamPort.nBufferCountMin)
					iNumOutputBuffers = iParamPort.nBufferCountMin;

				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
					(0,"PVMFOMXVideoEncNode::HandleProcessingState() new output buffers %d, size %d",iNumOutputBuffers,iOMXComponentOutputBufferSize));

					/* Allocate output buffers */
				if(!CreateOutputMemPool(iNumOutputBuffers))
				{
                                        LOGE("HandleProcessingState(): port reconfiguration -> Cannot allocate output buffers");
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Cannot allocate output buffers "));

					SetState(EPVMFNodeError);
					ReportErrorEvent(PVMFErrNoMemory);
					return PVMFErrNoMemory;
				}

				if(!ProvideBuffersToComponent(iOutBufMemoryPool, // allocator
										  iOutputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  iNumOutputBuffers, // number of buffers
										  iOMXComponentOutputBufferSize, // actual buffer size
										  iOutputPortIndex, // port idx
										  iOMXComponentSupportsExternalOutputBufferAlloc, // can component use OMX_UseBuffer
										  false // this is not input
										  ))
				{

                                        LOGE("HandleProcessingState(): port reconfiguration -> Cannot provide output buffers to component");
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Cannot provide output buffers to component"));

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
				if(iNumInputBuffers < iParamPort.nBufferCountMin)
					iNumInputBuffers = iParamPort.nBufferCountMin;

				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
					(0,"PVMFOMXVideoEncNode::HandleProcessingState() new buffers %d, size %d",iNumInputBuffers,iOMXComponentInputBufferSize));

				/* Allocate input buffers */
				if(!CreateInputMemPool(iNumInputBuffers))
				{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Cannot allocate new input buffers to component"));

					SetState(EPVMFNodeError);
					ReportErrorEvent(PVMFErrNoMemory);
					return PVMFErrNoMemory;
				}

				if(!ProvideBuffersToComponent(iInBufMemoryPool, // allocator
											  iInputAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
											  iNumInputBuffers, // number of buffers
											  iOMXComponentInputBufferSize, // actual buffer size
											  iInputPortIndex, // port idx
											  iOMXComponentSupportsExternalInputBufferAlloc, // can component use OMX_UseBuffer
											  true // this is input
											  ))
				{


					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> Cannot provide new input buffers to component"));

					SetState(EPVMFNodeError);
					ReportErrorEvent(PVMFErrNoMemory);
					return PVMFErrNoMemory;

				}
				// do not drop partially consumed input
				iDoNotSaveInputBuffersFlag = false;


			}

			// if the callback that the port was re-enabled has not arrived yet, wait for it
			// if it has arrived, it will set the state to either PortReconfig or to ReadyToDecode
			if(iProcessingState != EPVMFOMXVideoEncNodeProcessingState_PortReconfig &&
				iProcessingState !=EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode)
					iProcessingState = EPVMFOMXVideoEncNodeProcessingState_WaitForPortEnable;

			status = PVMFSuccess; // allow rescheduling of the node
			break;
		}

		case EPVMFOMXVideoEncNodeProcessingState_WaitForPortEnable:
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::HandleProcessingState() Port Reconfiguration -> wait for port enable callback"));
			// do nothing. Just wait for the port to become enabled (we'll get event from component, which will
			// transition the state to ReadyToDecode
			status = PVMFErrNoMemory; // prevent ReScheduling
			break;
		}

		// NORMAL DATA FLOW STATE:
		case EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode:
		{

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				            (0,"PVMFOMXVideoEncNode::HandleProcessingState() Ready To Encode start"));
			// In normal data flow and decoding state
			// Send all available output buffers to the encoder

			while( iNumOutstandingOutputBuffers < iNumOutputBuffers)
			{
				// grab buffer header from the mempool if possible, and send to component
				if(!SendOutputBufferToOMXComponent() )

					break;

			}

			// next, see if partially consumed input buffer needs to be resent back to OMX component
			// NOTE: it is not allowed that the component returns more than 1 partially consumed input buffers
			//		 i.e. if a partially consumed input buffer is returned, it is assumed that the OMX component
			//		 will be waiting to get data

			if(iInputBufferToResendToComponent != NULL)
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
					(0,"PVMFOMXVideoEncNode::HandleProcessingState() Sending previous - partially consumed input back to the OMX component"));

				OMX_EmptyThisBuffer(iOMXVideoEncoder, iInputBufferToResendToComponent);
				iInputBufferToResendToComponent = NULL; // do this only once
			}
			else if( (iNumOutstandingInputBuffers < iNumInputBuffers) && (iDataIn.GetRep()!=NULL) )
			{
				// try to get an input buffer header
				// and send the input data over to the component
				SendInputBufferToOMXComponent();
			}

			status = PVMFSuccess;
			break;


		}
		case EPVMFOMXVideoEncNodeProcessingState_Stopping:
		{

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::HandleProcessingState() Stopping -> wait for Component to move from Executing->Idle"));

			status = PVMFErrNoMemory; // prevent rescheduling
			break;
		}

		case EPVMFOMXVideoEncNodeProcessingState_Pausing:
		{

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::HandleProcessingState() Pausing -> wait for Component to move from Executing->Pause"));


			status = PVMFErrNoMemory; // prevent rescheduling
			break;
		}

		case EPVMFOMXVideoEncNodeProcessingState_WaitForOutgoingQueue:
			status = PVMFErrNoMemory;
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::HandleProcessingState() Do nothing since waiting for output port queue to become available"));
			break;

		default:
			break;
	}

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"PVMFOMXVideoEncNode::HandleProcessingState() Out"));

	return status;

}
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::SendOutputBufferToOMXComponent()
{
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::SendOutputBufferToOMXComponent() In"));


	OutputBufCtrlStruct *output_buf=NULL;
	int32 errcode=0;

	// try to get output buffer header
	OSCL_TRY(errcode, output_buf = (OutputBufCtrlStruct *) iOutBufMemoryPool->allocate(iOutputAllocSize));
	if (errcode != 0)
	{
		if (errcode == OsclErrNoResources)
		{

			PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
				PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::SendOutputBufferToOMXComponent() No more output buffers in the mempool"));

			iOutBufMemoryPool->notifyfreechunkavailable(*this,(OsclAny *) iOutBufMemoryPool); // To signal when next deallocate() is called on mempool

			return false;
		}
		else
		{
			// Memory allocation for the pool failed
			PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
				(0,"PVMFOMXVideoEncNode::SendOutputBufferToOMXComponent() Output mempool error"));


			SetState(EPVMFNodeError);
			ReportErrorEvent(PVMFErrNoMemory);
			return false;
		}

	}

	//for every allocated buffer, make sure you notify when buffer is released. Keep track of allocated buffers
	// use mempool as context to recognize which buffer (input or output) was returned
	iOutBufMemoryPool->notifyfreechunkavailable(*this,(OsclAny *)iOutBufMemoryPool);
	iNumOutstandingOutputBuffers++;

	output_buf->pBufHdr->nFilledLen = 0; // make sure you tell OMX component buffer is empty
	output_buf->pBufHdr->nOffset = 0;
	output_buf->pBufHdr->pAppPrivate = output_buf; // set pAppPrivate to be pointer to output_buf
												   // (this is context for future release of this buffer to the mempool)
													// this was done during buffer creation, but still repeat just in case
    output_buf->pBufHdr->nFlags = 0; // Clear flags

	OMX_FillThisBuffer(iOMXVideoEncoder,output_buf->pBufHdr);

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::SendOutputBufferToOMXComponent() Out"));

	return true;
}

bool PVMFOMXVideoEncNode::SendInputBufferToOMXComponent()
{
	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() In"));


	// first need to take care of  missing packets if node is assembling partial frames.
	// The action depends whether the component (I) can handle incomplete frames/NALs or (II) cannot handle incomplete frames/NALs
	if(!iOMXComponentSupportsPartialFrames)
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
		uint32 current_msg_marker = iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;

		// first check if we need to keep dropping msgs
		if(iKeepDroppingMsgsUntilMarkerBit)
		{
			// drop this message
			iDataIn.Unbind();

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() Dropping input msg with seqnum %d until marker bit",current_msg_seq_num));

			//if msg has marker bit, stop dropping msgs
			if(current_msg_marker != 0)
			{
				iKeepDroppingMsgsUntilMarkerBit = false;
				// also remember the sequence number & timestamp so that we have reference
				iInPacketSeqNum = current_msg_seq_num;
				iInTimestamp = current_msg_ts;
				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() Input msg with seqnum %d has marker bit set. Stop dropping msgs",current_msg_seq_num));

			}
			return true;
		}

		// is there something missing?
		// compare current and saved sequence number - difference should be exactly 1
		//	if it is more, there is something missing
		if( (current_msg_seq_num - iInPacketSeqNum) > 1 )
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - MISSING PACKET DETECTED. Input msg with seqnum %d, TS=%d. Previous seqnum: %d, Previous TS: %d",current_msg_seq_num,iInPacketSeqNum,current_msg_ts,iInTimestamp));

			// find out which case it is by comparing TS
			if( current_msg_ts == iInTimestamp)
			{

				// this is CASE a)
				// same ts, i.e. pieces are missing from the middle of the current frame
				if(!iOMXComponentCanHandleIncompleteFrames)
				{
					// drop current buffer, drop msgs until you hit msg with marker bit
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Drop current buffer under construction. Keep dropping msgs until marker bit"));

					DropCurrentBufferUnderConstruction();
					iKeepDroppingMsgsUntilMarkerBit = true;
				}
				else
				{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Continue processing" ));

				}
			}
			else // new ts and old ts are different
			{
				//  are we at the beginning of the new frame assembly?
				if(iObtainNewInputBuffer)
				{
					// CASE b)
					// i.e. we sent out previous frame, but have not started assembling a new frame. Pieces are missing from the beginning
					if(!iOMXComponentCanHandleIncompleteFrames)
					{
						// there is no current buffer to drop, but drop msgs until you hit msg with marker bit
						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
							(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - No current buffer under construction. Keep dropping msgs until marker bit"));

						iKeepDroppingMsgsUntilMarkerBit = true;
					}
					else
					{
						PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Continue processing" ));
					}
				}
				else	// no, we are in the middle of a frame assembly, but new ts is different
				{
					// is only 1 msg missing?
					if( (current_msg_seq_num - iInPacketSeqNum) == 2)
					{
						// CASE c)
						// only the last piece of the previous frame is missing
						if(iOMXComponentCanHandleIncompleteFrames)
						{
							PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
								(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Send incomplete buffer under construction. Start assembling new frame" ));

							SendIncompleteBufferUnderConstruction();
						}
						else
						{
							// drop current frame only, but no need to wait until next marker bit.
							// start assembling new frame
							PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
								(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Drop current buffer under construction. It's OK to start assembling new frame. Only 1 packet is missing"));

							DropCurrentBufferUnderConstruction();
						}
					}
					else
					{
						// CASE d)
						// (multiple) final piece(s) of the previous frame are missing and possibly pieces at the
						// beginning of a new frame are also missing
						if(iOMXComponentCanHandleIncompleteFrames)
						{
							PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
								(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Send incomplete buffer under construction. Start assembling new frame (potentially damaged)" ));

							SendIncompleteBufferUnderConstruction();
						}
						else
						{
							// drop current frame. start assembling new frame, but first keep dropping
							// until you hit msg with marker bit.
							PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
								(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Drop current buffer under construction. Keep dropping msgs until marker bit"));

							DropCurrentBufferUnderConstruction();
							iKeepDroppingMsgsUntilMarkerBit = true;
						}
					}
				}// end of if(obtainNewInputBuffer)/else
			}// end of if(curr_msg_ts == iInTimestamp)
		}//end of if(deltaseqnum>1)/else

		// check if we need to keep dropping msgs
		if(iKeepDroppingMsgsUntilMarkerBit)
		{
			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() Dropping input msg with seqnum %d until marker bit",current_msg_seq_num));

			// drop this message
			iDataIn.Unbind();

			//if msg has marker bit, stop dropping msgs
			if(current_msg_marker != 0)
			{
				iKeepDroppingMsgsUntilMarkerBit = false;
				// also remember the sequence number & timestamp so that we have reference
				iInPacketSeqNum = current_msg_seq_num;
				iInTimestamp = current_msg_ts;
				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
					(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() Input msg with seqnum %d has marker bit set. Stop dropping msgs",current_msg_seq_num));

			}
			return true;
		}

	}// end of if/else (iOMXSUpportsPartialFrames)

	InputBufCtrlStruct *input_buf = NULL;
	int32 errcode=0;

	// NOTE: a) if NAL start codes must be inserted i.e. iOMXComponentNeedsNALStartCodes is TRUE, then iOMXComponentSupportsMovableInputBuffers must be set to FALSE.
	//		 b) if iOMXComponentSupportsPartialFrames is FALSE, then iOMXComponentSupportsMovableInputBuffers must be FALSE as well
	//		 c) if iOMXCOmponentSupportsPartialFrames is FALSE, and the input frame/NAL size is larger than the buffer size, the frame/NAL is discarded

	do{
		// do loop to loop over all fragments
		// first of all , get an input buffer. Without a buffer, no point in proceeding
		if(iObtainNewInputBuffer == true) // if partial frames are being reconstructed, we may be copying data into
											//existing buffer, so we don't need the new buffer
		{
			// try to get input buffer header
			OSCL_TRY(errcode, input_buf = (InputBufCtrlStruct *) iInBufMemoryPool->allocate(iInputAllocSize));
			if (errcode!=0)
			{
				if (errcode==OsclErrNoResources)
				{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger,
						PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() No more buffers in the mempool"));

					iInBufMemoryPool->notifyfreechunkavailable(*this,(OsclAny*) iInBufMemoryPool); // To signal when next deallocate() is called on mempool

					return false;
				}
				else
				{
					// Memory allocation for the pool failed
					PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() Input mempool error"));


					SetState(EPVMFNodeError);
					ReportErrorEvent(PVMFErrNoMemory);
					return false;
				}

			}

			// keep track of buffers. When buffer is deallocated/released, the counter will be decremented
			iInBufMemoryPool->notifyfreechunkavailable(*this,(OsclAny*) iInBufMemoryPool);
			iNumOutstandingInputBuffers++;

			// Now we have the buffer header (i.e. a buffer) to send to component:
			// Depending on OMX component capabilities, either pass the input msg fragment(s) directly
			//	into OMX component without copying (and update the input msg refcount)
			//	or memcopy the content of input msg memfrag(s) into OMX component allocated buffers
			input_buf->pBufHdr->nFilledLen = 0; // init this for now
			// save this in a class member
			iInputBufferUnderConstruction = input_buf;
			// set flags
			if(iOMXComponentSupportsPartialFrames == true)
			{
				// if partial frames can be sent, then send them
				// but we'll always need the new buffer for the new fragment
				iObtainNewInputBuffer = true;
			}
			else{
				// if we need to assemble partial frames, then obtain a new buffer
				// only after assembling the partial frame
				iObtainNewInputBuffer = false;
			}
		}
		else
		{
			input_buf = iInputBufferUnderConstruction;
		}

		// When copying content, a special case is when the input fragment is larger than the buffer and has to
		//	be fragmented here and broken over 2 or more buffers. Potential problem with available buffers etc.

		// if this is the first fragment in a new message, extract some info:
		if(iCurrFragNum == 0){

			// NOTE: SeqNum differ in Codec and in Node because of the fact that
			// one msg can contain multiple fragments that are sent to the codec as
			// separate buffers. Node tracks msgs and codec tracks even separate fragments

			iCodecSeqNum += (iDataIn->getSeqNum() - iInPacketSeqNum); // increment the codec seq. # by the same
																  // amount that the input seq. number increased

			iInPacketSeqNum = iDataIn->getSeqNum(); // remember input sequence number
			iInTimestamp = iDataIn->getTimestamp();
			iInDuration = iDataIn->getDuration();
			iInNumFrags = iDataIn->getNumFragments();

			// logging info:
			if (iDataIn->getNumFragments() > 1)
			{
				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - New msg has MULTI-FRAGMENTS"));
			}

			if(!( iDataIn->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT))
			{

				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - New msg has NO MARKER BIT"));
			}
		}


		// get a memfrag from the message
		OsclRefCounterMemFrag frag;
		iDataIn->getMediaFragment(iCurrFragNum,frag);


		if(iOMXComponentSupportsMovableInputBuffers)
		{
			// no copying required
			// Note: This cannot be used for NAL start code insertion and
			//		 for the case when partial frames are not supported by the component

			// increment the RefCounter of the message associated with the mem fragment/buffer
			// when sending this buffer to OMX component. (When getting the buffer back, the refcounter
			// will be decremented. Thus, when the last fragment is returned, the input mssage is finally released

			iDataIn.GetRefCounter()->addRef();

			// associate the buffer ctrl structure with the message ref counter and ptr
			input_buf->pMediaData = PVMFSharedMediaDataPtr(iDataIn.GetRep(),iDataIn.GetRefCounter());


			// set pointer to the data, length, offset
			input_buf->pBufHdr->pBuffer = (uint8 *)frag.getMemFragPtr();
			input_buf->pBufHdr->nFilledLen = frag.getMemFragSize();

			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Buffer 0x%x of size %d, %d frag out of tot. %d, TS=%d",input_buf->pBufHdr->pBuffer,frag.getMemFragSize(),iCurrFragNum+1,iDataIn->getNumFragments(),iInTimestamp ));

			iCurrFragNum++; // increment fragment number and move on to the next
			iIsNewDataFragment = true; // update the flag

		}
		else
		{
			// in this case, no need to use input msg refcounter, each buffer fragment is copied over and treated separately
			(input_buf->pMediaData).Unbind();

			// is this a new data fragment or are we still working on separating the old one?
			if(iIsNewDataFragment == true)
			{
				//  if fragment size is larger than the buffer size,
				//	need to break up the fragment even further into smaller chunks

				// init variables needed for fragment separation
				iCopyPosition = 0;
				iFragmentSizeRemainingToCopy  = frag.getMemFragSize();

			}

			// can the remaining fragment fit into the buffer?
			uint32 bytes_remaining_in_buffer = (input_buf->pBufHdr->nAllocLen - input_buf->pBufHdr->nFilledLen);

			if( iFragmentSizeRemainingToCopy <= bytes_remaining_in_buffer )
			{
				oscl_memcpy( input_buf->pBufHdr->pBuffer + input_buf->pBufHdr->nFilledLen,
							 (void *) ((uint8 *)frag.getMemFragPtr() + iCopyPosition),
							 iFragmentSizeRemainingToCopy);

				input_buf->pBufHdr->nFilledLen += iFragmentSizeRemainingToCopy;

				PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
					(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Copied %d bytes of fragment %d out of %d into buffer 0x%x of size %d, TS=%d ",iFragmentSizeRemainingToCopy,iCurrFragNum+1,iDataIn->getNumFragments(), input_buf->pBufHdr->pBuffer,input_buf->pBufHdr->nFilledLen,iInTimestamp));

				iCopyPosition += iFragmentSizeRemainingToCopy;
				iFragmentSizeRemainingToCopy = 0;



				iIsNewDataFragment = true; // done with this fragment. Get a new one
				iCurrFragNum++;

			}
			else
			{
				// copy as much as you can of the current fragment into the current buffer
				if(bytes_remaining_in_buffer>0)
				{
					oscl_memcpy( input_buf->pBufHdr->pBuffer + input_buf->pBufHdr->nFilledLen,
								 (void *)((uint8 *)frag.getMemFragPtr() + iCopyPosition),
								 bytes_remaining_in_buffer);

					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Copied %d bytes of fragment %d out of %d into buffer 0x%x of size %d, TS=%d",input_buf->pBufHdr->nAllocLen,iCurrFragNum+1,iDataIn->getNumFragments(), input_buf->pBufHdr->pBuffer,input_buf->pBufHdr->nFilledLen,iInTimestamp));
				}
				input_buf->pBufHdr->nFilledLen = input_buf->pBufHdr->nAllocLen;
				iCopyPosition += bytes_remaining_in_buffer; // move current position within fragment forward
				iFragmentSizeRemainingToCopy -= bytes_remaining_in_buffer;
				iIsNewDataFragment = false; // set the flag to indicate we're still working on the "old" fragment

				if(!iOMXComponentSupportsPartialFrames)
				{
					// if partial frames are not supported, and data cannot fit into the buffer, i.e. the buffer is full at this point
					// simply go through remaining fragments if they exist and "drop" them
					// i.e. send what data is alrady copied in the buffer and ingore the rest
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - Reconstructing partial frame - more data cannot fit in buffer 0x%x, TS=%d.Skipping data.",input_buf->pBufHdr->pBuffer,iInTimestamp));

					iIsNewDataFragment = true; // done with this fragment, get a new one
					iCurrFragNum++;
				}
			}

		}


		// set buffer fields (this is the same regardless of whether the input is movable or not)
		input_buf->pBufHdr->nOffset = 0;
		input_buf->pBufHdr->nTimeStamp = iInTimestamp;
		input_buf->pBufHdr->nTimeStamp *= 1000;

		// set ptr to input_buf structure for Context (for when the buffer is returned)
		input_buf->pBufHdr->pAppPrivate = (OMX_PTR) input_buf;

		// do not use Mark here (but init to NULL to prevent problems)
		input_buf->pBufHdr->hMarkTargetComponent = NULL;
		input_buf->pBufHdr->pMarkData = NULL;


		// init buffer flags
		input_buf->pBufHdr->nFlags = 0;

		{
			// "normal" case, i.e. only fragments at ends of msgs may have marker bit set
			//					fragments in the middle of a message never have marker bit set
			// there is also a (slight) possibility we broke up the fragment into more fragments
			//	because they can't fit into input buffer. In this case, make sure you apply
			//	the marker bit (if necessary) only to the very last piece of the very last fragment

			// for all other cases, clear the marker bit flag for the buffer
			if( (iCurrFragNum == iDataIn->getNumFragments()) && iIsNewDataFragment)
			{
				// if all the fragments have been exhausted, and this is the last piece
				// of the (possibly broken up) last fragment

				// use the marker bit from the end of message
				if( iCurrentMsgMarkerBit )
				{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - END OF MESSAGE - Buffer 0x%x MARKER bit set to 1, TS=%d",input_buf->pBufHdr->pBuffer,iInTimestamp));

					input_buf->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
					// once frame is complete, make sure you send it and obtain new buffer

					iObtainNewInputBuffer = true;
				}
				else{

					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - END OF MESSAGE - Buffer 0x%x MARKER bit set to 0, TS=%d",input_buf->pBufHdr->pBuffer,iInTimestamp));
				}
			}
			else
			{
					PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
						(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() - NOT END OF MESSAGE - Buffer 0x%x MARKER bit set to 0, TS=%d",input_buf->pBufHdr->pBuffer,iInTimestamp));
			}


		}// end of else(setmarkerbitforeveryfrag)


		if(iObtainNewInputBuffer == true)
		{
			// if partial frames are supported, this flag will always be set
			// if partial frames are not supported, this flag will be set only
			// if the partial frame/NAL has been assembled, so we can send it


			PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
				(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent()  - Sending Buffer 0x%x to OMX Component MARKER field set to %x, TS=%d",input_buf->pBufHdr->pBuffer,input_buf->pBufHdr->nFlags,iInTimestamp));


			OMX_EmptyThisBuffer(iOMXVideoEncoder,input_buf->pBufHdr);
			iInputBufferUnderConstruction = NULL; // this buffer is gone to OMX component now
                        
                        // HTC fix for race condition between pv omx encoder node and qualcomm encoder
                        ++mInputBufferRefCount;
                        LOGV("==> %s: mInputBufferRefCount = %d", __FUNCTION__, mInputBufferRefCount);
		}

		// if we sent all fragments to OMX component, decouple the input message from iDataIn
		// Input message is "decoupled", so that we can get a new message for processing into iDataIn
		//	However, the actual message is released completely to upstream mempool once all of its fragments
		//	are returned by the OMX component

		if(iCurrFragNum == iDataIn->getNumFragments())
		{
			iDataIn.Unbind();

		}
	}while(iCurrFragNum < iInNumFrags); //iDataIn->getNumFragments());



	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::SendInputBufferToOMXComponent() Out"));

	return true;

}

bool PVMFOMXVideoEncNode::SendEOSBufferToOMXComponent()
{

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::SendEOSBufferToOMXComponent() In"));


	// first of all, check if the component is running. EOS could be sent prior to component/encoder
	// even being initialized

	// returning false will ensure that the EOS will be sent downstream anyway without waiting for the
	// Component to respond
	if(iCurrentEncoderState != OMX_StateExecuting)
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
				PVLOGMSG_DEBUG, (0,"PVMFOMXVideoEncNode::SendEOSBufferToOMXComponent() No more buffers in the mempool - unexpected"));

			iInBufMemoryPool->notifyfreechunkavailable(*this,(OsclAny*) iInBufMemoryPool); // To signal when next deallocate() is called on mempool

			return false;
		}
		else
		{
			// Memory allocation for the pool failed
			PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
				(0,"PVMFOMXVideoEncNode::SendEOSBufferToOMXComponent() Input mempool error"));


			SetState(EPVMFNodeError);
			ReportErrorEvent(PVMFErrNoMemory);
			return false;
		}

	}

	// keep track of buffers. When buffer is deallocated/released, the counter will be decremented
	iInBufMemoryPool->notifyfreechunkavailable(*this,(OsclAny*) iInBufMemoryPool);
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
	OMX_EmptyThisBuffer(iOMXVideoEncoder, input_buf->pBufHdr);

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0,"PVMFOMXVideoEncNode::SendEOSBufferToOMXComponent() Out"));

	return true;

}

// this method is called under certain conditions only if the node is doing partial frame assembly
void PVMFOMXVideoEncNode::DropCurrentBufferUnderConstruction()
{
	if(iObtainNewInputBuffer == false)
	{
		if(iInputBufferUnderConstruction != NULL)
		{
			if(iInBufMemoryPool != NULL )
			{
				iInBufMemoryPool->deallocate((OsclAny *)iInputBufferUnderConstruction);
			}

			iInputBufferUnderConstruction = NULL;
		}
		iObtainNewInputBuffer = true;

	}
}
// this method is called under certain conditions only if the node is doing partial frame assembly
void PVMFOMXVideoEncNode::SendIncompleteBufferUnderConstruction()
{
	// this should never be the case, but check anyway
	if(iInputBufferUnderConstruction !=NULL)
	{
		// mark as end of frame (the actual end piece is missing)
		iInputBufferUnderConstruction->pBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::SendIncompleteBufferUnderConstruction()  - Sending Incomplete Buffer 0x%x to OMX Component MARKER field set to %x, TS=%d",iInputBufferUnderConstruction->pBufHdr->pBuffer,iInputBufferUnderConstruction->pBufHdr->nFlags,iInTimestamp));

		OMX_EmptyThisBuffer(iOMXVideoEncoder,iInputBufferUnderConstruction->pBufHdr);

		iInputBufferUnderConstruction = NULL;
		iObtainNewInputBuffer = true;
	}
}
/////////////////////////////////////////////////////////////////////////////
void PVMFOMXVideoEncNode::freechunkavailable(OsclAny *aContext)
{
	// check context to see whether input or output buffer was returned to the mempool
	if(aContext == (OsclAny *) iInBufMemoryPool)
	{

		iNumOutstandingInputBuffers--;

		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::freechunkavailable() Memory chunk in INPUT mempool was deallocated, %d out of %d now available",iNumInputBuffers-iNumOutstandingInputBuffers,iNumInputBuffers));

		// notification only works once.
		// If there are multiple buffers coming back in a row, make sure to set the notification
		// flag in the mempool again, so that next buffer also causes notification
		iInBufMemoryPool->notifyfreechunkavailable(*this,aContext);

	}
	else if(aContext == (OsclAny *) iOutBufMemoryPool)
	{

		iNumOutstandingOutputBuffers--;
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::freechunkavailable() Memory chunk in OUTPUT mempool was deallocated, %d out of %d now available",iNumOutputBuffers-iNumOutstandingOutputBuffers,iNumOutputBuffers));

		// notification only works once.
		// If there are multiple buffers coming back in a row, make sure to set the notification
		// flag in the mempool again, so that next buffer also causes notification
		iOutBufMemoryPool->notifyfreechunkavailable(*this,aContext);

	}
	else
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
			(0,"PVMFOMXVideoEncNode::freechunkavailable() UNKNOWN mempool "));

	}

	// reschedule
	if(IsAdded())
		RunIfNotReady();


}

/////////////////////////////////////////////////////////////////////////////
// This routine will process outgoing message by sending it into output the port
/////////////////////////////////////////////////////////////////////////////
bool PVMFOMXVideoEncNode::ProcessOutgoingMsg(PVMFPortInterface* aPort)
{
	//Called by the AO to process one message off the outgoing
	//message queue for the given port.  This routine will
	//try to send the data to the connected port.

	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
		(0, "0x%x PVMFOMXVideoEncNode::ProcessOutgoingMsg: aPort=0x%x", this, aPort));

	PVMFStatus status = aPort->Send();
	if(status == PVMFErrBusy)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
			(0, "0x%x PVMFOMXVideoEncNode::ProcessOutgoingMsg: Connected port goes into busy state", this));
	}

	//Report any unexpected failure in port processing...
	//(the InvalidState error happens when port input is suspended,
	//so don't report it.)
	if (status!=PVMFErrBusy
		&& status!=PVMFSuccess
		&& status!=PVMFErrInvalidState)
	{
		PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
			(0, "0x%x PVMFOMXVideoEncNode::Run: Error - ProcessPortActivity failed. port=0x%x, type=%d",
			this, iOutPort, PVMF_PORT_ACTIVITY_OUTGOING_MSG));
		ReportErrorEvent(PVMFErrPortProcessing);
	}

	//return true if we processed an activity...
	return (status!=PVMFErrBusy);
}

PVMFStatus PVMFOMXVideoEncNode::SendEndOfTrackCommand()
{
    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();

    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

    // Set the timestamp
    //sharedMediaCmdPtr->setTimestamp(aMsg->getTimestamp());
    sharedMediaCmdPtr->setTimestamp(iEndOfDataTimestamp);

    // Set the sequence number
    //sharedMediaCmdPtr->setSeqNum(aMsg->getSeqNum());
    sharedMediaCmdPtr->setSeqNum(iSeqNum++);

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);

    for (uint32 ii = 0; ii < iOutPort.size(); ii++)
    {
        PVMFStatus status = iOutPort[ii]->QueueOutgoingMsg(mediaMsgOut);

        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFOMXVideoEncNode::SendEndOfTrackCommand: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }
    }

    return PVMFSuccess;
}
