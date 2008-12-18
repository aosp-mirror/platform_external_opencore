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
#ifndef PVMF_OMX_VIDEOENC_NODE_H_INCLUDED
#define PVMF_OMX_VIDEOENC_NODE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif
#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif
#ifndef OSCL_PRIQUEUE_H_INCLUDED
#include "oscl_priqueue.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#include "pvmf_format_type.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifndef __PVM4VENCODER_H
#include "pvm4vencoder.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_NODE_UTILS_H_INCLUDED
#include "pvmf_node_utils.h"
#endif
#ifndef PVMF_SIMPLE_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_simple_media_buffer.h"
#endif
#ifndef PV_MP4_H263_ENC_EXTENSION_H_INCLUDED
#include "pvmp4h263encextension.h"
#endif
#ifndef PVMF_OMX_VIDEOENC_TUNEABLES_H_INCLUDED
#include "pvmf_omx_videoenc_tuneables.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif
#ifndef OSCL_TICKCOUNT_H_INCLUDED
#include "oscl_tickcount.h"
#endif

//Holmes add for omx {
#ifndef OMX_Core_h
#include "omx_core.h"
#endif
#ifndef PVMF_OMX_VIDEOENC_CALLBACKS_H_INCLUDED
#include "pvmf_omx_videoenc_callbacks.h"
#endif

//Default values for number of Input/Output buffers. If the component needs more than this, it will be
// negotiated. If the component does not need more than this number, the default is used
#define NUMBER_INPUT_BUFFER 3
#define NUMBER_OUTPUT_BUFFER 5

typedef struct OutputBufCtrlStruct
{
	OMX_BUFFERHEADERTYPE *pBufHdr;
}
OutputBufCtrlStruct;

typedef struct InputBufCtrlStruct
{
	OMX_BUFFERHEADERTYPE *pBufHdr;
	PVMFSharedMediaDataPtr pMediaData;
}
InputBufCtrlStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CUSTOM DEALLOCATOR FOR MEDIA DATA SHARED PTR WRAPPER:
//	1) Deallocates the underlying output buffer
//	2) Deallocates the pvci buffer wrapper and the rest of accompanying structures
//	   Deallocator is created as part of the wrapper, and travels with the buffer wrapper

class PVOMXBufferSharedPtrWrapperCombinedCleanupDA
	: public OsclDestructDealloc
{
public:
	PVOMXBufferSharedPtrWrapperCombinedCleanupDA(Oscl_DefAlloc* allocator, void *pMempoolData) :
		buf_alloc(allocator),ptr_to_data_to_dealloc(pMempoolData) {};
	virtual ~PVOMXBufferSharedPtrWrapperCombinedCleanupDA() {};

	virtual void destruct_and_dealloc(OsclAny* ptr)
	{
		// call buffer deallocator
		if(buf_alloc !=NULL)
		{
		buf_alloc->deallocate(ptr_to_data_to_dealloc);
		}

		// finally, free the shared ptr wrapper memory
		oscl_free(ptr);
	}

private:
	Oscl_DefAlloc* buf_alloc;
	void *ptr_to_data_to_dealloc;
};
//Holmes add for omx }


// Forward declarations
class PVMFVideoEncPort;
class OsclClock;

// Allocators
typedef OsclMemAllocDestructDealloc<uint8> PVMFVideoEncNodeAllocDestructDealloc;
typedef OsclMemAllocator PVMFVideoEncNodeAlloc;

/** Node command type */
typedef PVMFGenericNodeCommand<PVMFVideoEncNodeAlloc> PVMFVideoEncNodeCommand;

/** Command queue type */
typedef PVMFNodeCommandQueue<PVMFVideoEncNodeCommand,PVMFVideoEncNodeAlloc> PVMFVideoEncNodeCmdQueue;

/** Port vector type */
typedef PVMFPortVector<PVMFVideoEncPort,PVMFVideoEncNodeAlloc> PVMFVideoEncPortVector;

////////////////////////////////////////////////////////////////////////////
class PVMFOMXVideoEncNode
    : public OsclTimerObject
    , public PVMFNodeInterface
    , public OsclMemPoolFixedChunkAllocatorObserver
    , public MPVCVEIObserver //remove?
    , public PVMp4H263EncExtensionInterface //remove?
    , public PvmiCapabilityAndConfig
{
public:
	PVMFOMXVideoEncNode(int32 aPriority);
	~PVMFOMXVideoEncNode();

	// Virtual functions of PVMFNodeInterface
	OSCL_IMPORT_REF PVMFStatus ThreadLogon();
	OSCL_IMPORT_REF PVMFStatus ThreadLogoff();
	OSCL_IMPORT_REF PVMFStatus GetCapability(PVMFNodeCapability& aNodeCapability);
	OSCL_IMPORT_REF PVMFPortIter* GetPorts(const PVMFPortFilter* aFilter = NULL);
    OSCL_IMPORT_REF PVMFCommandId QueryUUID(PVMFSessionId aSession,
	                                      const PvmfMimeString& aMimeType,
                                          Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                          bool aExactUuidsOnly=false,
                                          const OsclAny* aContext=NULL);
    OSCL_IMPORT_REF PVMFCommandId QueryInterface(PVMFSessionId aSession,
	                                           const PVUuid& aUuid,
                                               PVInterface*& aInterfacePtr,
                                               const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId RequestPort(PVMFSessionId aSession, int32 aPortTag,
	                                        const PvmfMimeString* aPortConfig=NULL,
	                                        const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId ReleasePort(PVMFSessionId aSession, PVMFPortInterface& aPort,
	                                        const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Init(PVMFSessionId aSession, const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Prepare(PVMFSessionId aSession, const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Start(PVMFSessionId aSession, const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Stop(PVMFSessionId aSession, const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Flush(PVMFSessionId aSession, const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Pause(PVMFSessionId aSession, const OsclAny* aContext=NULL);
	OSCL_IMPORT_REF PVMFCommandId Reset(PVMFSessionId aSession, const OsclAny* aContext=NULL);
    OSCL_IMPORT_REF PVMFCommandId CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContextData=NULL);
    OSCL_IMPORT_REF PVMFCommandId CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId,
	                                          const OsclAny* aContextData=NULL);

	// From PVMFPortActivityHandler
	void HandlePortActivity(const PVMFPortActivity& aActivity);

	// Virtual functions of PVMp4H263EncExtensionInterface
	OSCL_IMPORT_REF void addRef();
	OSCL_IMPORT_REF void removeRef();
	OSCL_IMPORT_REF bool queryInterface(const PVUuid& uuid, PVInterface*& iface);
	OSCL_IMPORT_REF bool SetNumLayers(uint32 aNumLayers);
	OSCL_IMPORT_REF bool SetOutputBitRate(uint32 aLayer, uint32 aBitRate);
	OSCL_IMPORT_REF bool SetOutputFrameSize(uint32 aLayer, uint32 aWidth, uint32 aHeight);
	OSCL_IMPORT_REF bool SetOutputFrameRate(uint32 aLayer, OsclFloat aFrameRate);
	OSCL_IMPORT_REF bool SetSegmentTargetSize(uint32 aLayer, uint32 aSizeBytes);
	OSCL_IMPORT_REF bool SetRateControlType(uint32 aLayer, PVMFVENRateControlType aRateControl);
	OSCL_IMPORT_REF bool SetDataPartitioning(bool aDataPartitioning);
	OSCL_IMPORT_REF bool SetRVLC(bool aRVLC);
	OSCL_IMPORT_REF bool SetIFrameInterval(uint32 aIFrameInterval);
	OSCL_IMPORT_REF bool GetVolHeader(OsclRefCounterMemFrag& aVolHeader);
	OSCL_IMPORT_REF bool RequestIFrame();
	OSCL_IMPORT_REF bool SetCodec(PVMFFormatType aCodec);

	// From MPVCVEIObserver
	void HandlePVCVEIEvent(uint32 aId, uint32 aEvent, uint32 aParam1);

	// implemetation of PvmiCapabilityAndConfig class functions here
	void setObserver (PvmiConfigAndCapabilityCmdObserver* aObserver);

	uint32 getCapabilityMetric (PvmiMIOSession aSession);
    PVMFStatus getParametersSync(PvmiMIOSession aSession,
		                                 PvmiKeyType aIdentifier,
		                                 PvmiKvp*& aParameters,
		                                 int& aNumParamElements,
		                                 PvmiCapabilityContext aContext);
	void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
		                           int num_elements, PvmiKvp * & aRet_kvp);
	PVMFCommandId setParametersAsync(PvmiMIOSession aSession,
                                             PvmiKvp* aParameters,
		                                     int num_elements,
		                                     PvmiKvp*& aRet_kvp,
		                                     OsclAny* context=NULL);
	void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                      PvmiKvp* aParameters, int num_parameter_elements);
	PVMFStatus verifyParametersSync(PvmiMIOSession aSession,
                                            PvmiKvp* aParameters,
                                            int num_elements);
	PVMFStatus releaseParameters(PvmiMIOSession aSession,
		                                 PvmiKvp* aParameters,
		                                 int num_elements);
    void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
	void DeleteContext(PvmiMIOSession aSession,
                               PvmiCapabilityContext& aContext);
	// function used in getParametersSync of capability class
    PVMFStatus GetConfigParameter(PvmiKvp*& aParameters, int& aNumParamElements,
                                  int32 aIndex, PvmiKvpAttr reqattr);
    // function used in VerifyParametersSync n SetParametersSync of capability class
	PVMFStatus VerifyAndSetConfigParameter(PvmiKvp& aParameter, bool aSetParam);

//Holmes add for omx {
	// Virtual functions of OsclMemPoolFixedChunkAllocatorObserver
	void freechunkavailable(OsclAny *aContext);

	//*****************************************************
	//********** CB Functions to serve OpenMAX Encoder
	//*****************************************************
	//Process callback functions. They will be executed in testapp thread context
	//	These callbacks are used only in the Multithreaded component case
	OsclReturnCode ProcessCallbackEventHandler_MultiThreaded(OsclAny* P);
	OsclReturnCode ProcessCallbackEmptyBufferDone_MultiThreaded(OsclAny* P);
	OsclReturnCode ProcessCallbackFillBufferDone_MultiThreaded(OsclAny* P);

	//Callback objects - again, these are used only in the case of Multithreaded component
	EventHandlerThreadSafeCallbackAO*	 iThreadSafeHandlerEventHandler;
	EmptyBufferDoneThreadSafeCallbackAO* iThreadSafeHandlerEmptyBufferDone;
	FillBufferDoneThreadSafeCallbackAO*  iThreadSafeHandlerFillBufferDone;

	OMX_CALLBACKTYPE       iCallbacks; // structure that contains callback ptrs.

	// OMX CALLBACKS
	// 1) AO OMX component running in the same thread as the OMX node
	//	In this case, the callbacks can be called directly from the component
	//	The callback: OMX Component->CallbackEventHandler->EventHandlerProcessing
	//	The callback can perform do RunIfNotReady
	// 2) Multithreaded component
	//	In this case, the callback is made using the threadsafe callback (TSCB) AO
	//	Component thread : OMX Component->CallbackEventHandler->TSCB(ReceiveEvent)
	//  Node thread		 : TSCB(ProcessEvent)->ProcessCallbackEventHandler_MultiThreaded->EventHandlerProcessing
	//==============================================================================

	OMX_ERRORTYPE EventHandlerProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
								  OMX_OUT OMX_PTR aAppData,
								  OMX_OUT OMX_EVENTTYPE aEvent,
								  OMX_OUT OMX_U32 aData1,
								  OMX_OUT OMX_U32 aData2,
								  OMX_OUT OMX_PTR aEventData);

	OMX_ERRORTYPE EmptyBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
									 OMX_OUT OMX_PTR aAppData,
									 OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer);

	OMX_ERRORTYPE FillBufferDoneProcessing(OMX_OUT OMX_HANDLETYPE aComponent,
									OMX_OUT OMX_PTR aAppData,
									OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer);

	bool IsComponentMultiThreaded() {return iIsOMXComponentMultiThreaded;};
//Holmes add for omx }

private:
	void ConstructEncoderParams();

	// From OsclTimerObject
	void Run();

	/////////////////////////////////////////////////////
	//     Command processing routines
	/////////////////////////////////////////////////////
	PVMFCommandId QueueCommandL(PVMFVideoEncNodeCommand& aCmd);
	bool ProcessCommand(PVMFVideoEncNodeCommand& aCmd);
	void CommandComplete(PVMFVideoEncNodeCmdQueue& aCmdQueue, PVMFVideoEncNodeCommand& aCmd,
	                     PVMFStatus aStatus, OsclAny* aData=NULL);
	void DoQueryUuid(PVMFVideoEncNodeCommand& aCmd);
	void DoQueryInterface(PVMFVideoEncNodeCommand& aCmd);
	void DoRequestPort(PVMFVideoEncNodeCommand& aCmd);
	PVMFVideoEncPort* AllocatePort(PVMFVideoEncPortVector& aPortVector, int32 aTag, OSCL_String* aMimeType, const char* aName = NULL);
	void DoReleasePort(PVMFVideoEncNodeCommand& aCmd);
	void DoInit(PVMFVideoEncNodeCommand& aCmd);
	void DoPrepare(PVMFVideoEncNodeCommand& aCmd);
	void DoStart(PVMFVideoEncNodeCommand& aCmd);
	void DoStop(PVMFVideoEncNodeCommand& aCmd);
	void DeleteVideoEncoder();
	void DoFlush(PVMFVideoEncNodeCommand& aCmd);
	bool IsFlushPending();
	void FlushComplete();
	void DoPause(PVMFVideoEncNodeCommand& aCmd);
	void DoReset(PVMFVideoEncNodeCommand& aCmd);
	void DoCancelAllCommands(PVMFVideoEncNodeCommand& aCmd);
	void DoCancelCommand(PVMFVideoEncNodeCommand& aCmd);

	/////////////////////////////////////////////////////
	//      Port activity processing routines
	/////////////////////////////////////////////////////
	bool IsProcessOutgoingMsgReady();
	bool IsProcessIncomingMsgReady();
	PVMFStatus ProcessIncomingMsg(PVMFPortInterface* aPort);
	PVMFStatus SyncEncodeAndSend(PVMFSharedMediaDataPtr& aMediaData);
	PVMFStatus SendEncodedBitstream(PVMFSharedMediaDataPtr& iMediaData);

	/////////////////////////////////////////////////////
	//      Encoder settings routine
	/////////////////////////////////////////////////////
	PVMFStatus SetInputFormat(PVMFFormatType aFormat);
	PVMFStatus SetInputFrameSize(uint32 aWidth, uint32 aHeight, uint8 aFrmOrient = 0);
	PVMFStatus SetInputFrameRate(OsclFloat aFrameRate);
	PVMFStatus SetCodecType(PVMFFormatType aCodec);
	PVMFFormatType GetCodecType();
	uint32 GetOutputBitRate(uint32 aLayer);
	OsclFloat GetOutputFrameRate(uint32 aLayer);
	PVMFStatus GetOutputFrameSize(uint32 aLayer, uint32& aWidth, uint32& aHeight);
	uint32 GetIFrameInterval();

	// Event reporting
	void ReportErrorEvent(PVMFEventType aEventType,OsclAny* aEventData=NULL);
	void ReportInfoEvent(PVMFEventType aEventType,OsclAny* aEventData=NULL);
	void SetState(TPVMFNodeInterfaceState aState);
	PVMFStatus SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg);

//Holmes add for omx {
        // Resolve race condition between pv omx encoder node and qualcomm encoder
        uint32 mInputBufferRefCount;

	bool SetDefaultCapabilityFlags();
	bool NegotiateComponentParameters();
	bool CreateOutputMemPool(uint32 num_buffers);
	bool CreateInputMemPool(uint32 num_buffers);
	bool ProvideBuffersToComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
										  uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  uint32 aNumBuffers,    // number of buffers
										  uint32 aActualBufferSize, // aactual buffer size
										  uint32 aPortIndex,      // port idx
										  bool aUseBufferOK,		// can component use OMX_UseBuffer or should it use OMX_AllocateBuffer
										  bool	aIsThisInputBuffer		// is this input or output
										  );
	bool FreeBuffersFromComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
										  uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
										  uint32 aNumBuffers,    // number of buffers
										  uint32 aPortIndex,      // port idx
										  bool	aIsThisInputBuffer		// is this input or output
										  );
	void HandleComponentStateChange(OMX_U32 decoder_state);
	OsclSharedPtr<PVMFMediaDataImpl> WrapOutputBuffer(uint8 *pData,uint32 aDataLen, OsclAny *pContext);
	bool QueueOutputBuffer(OsclSharedPtr<PVMFMediaDataImpl> &mediadataimplout,uint32 aDataLen);
	PVMFStatus HandleProcessingState();
	bool SendOutputBufferToOMXComponent();
	bool SendInputBufferToOMXComponent();
	bool SendEOSBufferToOMXComponent();
	void DropCurrentBufferUnderConstruction();
	void SendIncompleteBufferUnderConstruction();
	bool ProcessOutgoingMsg(PVMFPortInterface* aPort);
	PVMFStatus SendEndOfTrackCommand(); //temp overloading


	// Handle of OMX Component
	OMX_HANDLETYPE iOMXVideoEncoder;

	// Current State of the component
	OMX_STATETYPE iCurrentEncoderState;

	// Shared pointer for Media Msg.Input buffer
	PVMFSharedMediaDataPtr iDataIn;

	//EOS control flags
	bool iIsEOSSentToComponent;
	bool iIsEOSReceivedFromComponent;

	////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS
	bool iOMXComponentSupportsExternalOutputBufferAlloc;
	bool iOMXComponentSupportsExternalInputBufferAlloc;
	bool iOMXComponentSupportsMovableInputBuffers;
	bool iIsOMXComponentMultiThreaded;
	bool iOMXComponentSupportsPartialFrames;
	bool iOMXComponentCanHandleIncompleteFrames;

	// DYNAMIC PORT RE-CONFIGURATION
	uint32 iInputPortIndex;
	uint32 iOutputPortIndex;
	OMX_PARAM_PORTDEFINITIONTYPE iParamPort;
	uint32 iPortIndexForDynamicReconfig;
	bool iSecondPortReportedChange;
	bool iDynamicReconfigInProgress;
	uint32 iSecondPortToReconfig;

	// OUTPUT BUFFER RELATED MEMBERS
	OsclMemPoolFixedChunkAllocator *iMediaDataMemPool;
	OsclMemPoolFixedChunkAllocator *iOutBufMemoryPool;
	uint32 iOMXComponentOutputBufferSize; // Size of output buffer (negotiated with component)
	uint32 iOutputAllocSize; // size of output to allocate (OMX_ALLOCATE_BUFFER =  size of buf header )
							// (OMX_USE_BUFFER = size of buf header + iOMXCoponentOutputBufferSize)
	uint32 iNumOutputBuffers; // Number of output buffers (negotiated with component)
	uint32 iNumOutstandingOutputBuffers; // Number of output buffers in possession of the component or downstream,
										// namely, number of unavailable buffers
	bool iDoNotSendOutputBuffersDownstreamFlag; // flag to prevent sending output buffers downstream during flushing etc.
	bool iOutputBuffersFreed; // flag to prevent freeing the buffers twice
	OMX_PTR ipPrivateData; // ptr to private data to be sent with output buffer

	// INPUT BUFFER RELATED MEMBERS
	OsclMemPoolFixedChunkAllocator *iInBufMemoryPool;
	uint32 iOMXComponentInputBufferSize; // size of input buffer that the component sees (negotiated with the component)
	uint32 iInputAllocSize; // size of input buffer to allocate (OMX_ALLOCATE_BUFFER =  size of buf header )
							// (OMX_USE_BUFFER = size of buf header + iOMXCoponentInputBufferSize)
	uint32 iNumInputBuffers; // total num of input buffers (negotiated with component)
	uint32 iNumOutstandingInputBuffers; // number of input buffers in use (i.e. unavailable)

	bool iDoNotSaveInputBuffersFlag;
	bool iInputBuffersFreed; // flag to prevent freeing buffers twice

	//
	PvmfFormatIndex iOMXComponentInputYUVFormat;

	//
	OMX_BUFFERHEADERTYPE *iInputBufferToResendToComponent; // ptr to input buffer that is not empty, but that the OMX component returned
															// we need to resend this same buffer back to the component

    // State definitions for HandleProcessingState() state machine
	typedef enum
	{
		EPVMFOMXVideoEncNodeProcessingState_Idle,                   //default state after constraction/reset
		EPVMFOMXVideoEncNodeProcessingState_InitEncoder,            //initialization of encoder after handle was obtained
		EPVMFOMXVideoEncNodeProcessingState_WaitForInitCompletion,  // waiting for init completion
		EPVMFOMXVideoEncNodeProcessingState_ReadyToEncode,          //nornal operation state of the decoder
		EPVMFOMXVideoEncNodeProcessingState_WaitForOutputBuffer,    //wait state for avalible media output buffer
		EPVMFOMXVideoEncNodeProcessingState_WaitForOutputPort,      //wait state, output port is busy
		EPVMFOMXVideoEncNodeProcessingState_WaitForOutgoingQueue,   //wait state, outgoing queue
		EPVMFOMXVideoEncNodeProcessingState_PortReconfig,			// Dynamic Port Reconfiguration - step 1
		EPVMFOMXVideoEncNodeProcessingState_WaitForBufferReturn,    // Dynamic Port Reconfiguration - step 2
		EPVMFOMXVideoEncNodeProcessingState_WaitForPortDisable,		// Dynamic Port Reconfiguration - step 3
		EPVMFOMXVideoEncNodeProcessingState_PortReEnable,			// Dynamic Port Reconfiguration - step 4
		EPVMFOMXVideoEncNodeProcessingState_WaitForPortEnable,		// Dynamic Port Reconfiguration - step 5
		EPVMFOMXVideoEncNodeProcessingState_Stopping,				// when STOP command is issued, the node has to wait for component to transition into
																	// idle state. The buffers keep coming back , the node is rescheduled
																	// to run. Prevent the node from sending buffers back
		EPVMFOMXVideoEncNodeProcessingState_Pausing					// when PAUSE command is issued, the node has to wait for component to transition into
																	// paused state. The buffers may still keep coming back , the node is rescheduled
																	// to run. Prevent the node from sending buffers back to component
	}
	PVMFOMXVideoEncNode_ProcessingState;

    // State of HandleProcessingState() state machine
	PVMFOMXVideoEncNode_ProcessingState iProcessingState;

	bool iResetInProgress;
	bool iResetMsgSent;

	// Time stamp to be used on output buffer
	uint32 iOutTimeStamp;

	// input buffer fragmentation etc.
	uint32 iCopyPosition;				// for copying memfrag data into a buffer
	uint32 iFragmentSizeRemainingToCopy;
	bool iIsNewDataFragment;

	// partial frame assembly logic flags
	bool iObtainNewInputBuffer;
	bool iKeepDroppingMsgsUntilMarkerBit;
	InputBufCtrlStruct *iInputBufferUnderConstruction;

	// input data info
	uint32 iCurrFragNum;
	uint32 iCodecSeqNum;	// sequence number tracking
	uint32 iInPacketSeqNum;

	uint32 iInTimestamp;
	uint32 iInDuration;
	uint32 iInNumFrags;
	uint32 iCurrentMsgMarkerBit;

	// EOS flag
	bool iEndOfDataReached;
	// Time stame upon EOS
	PVMFTimestamp iEndOfDataTimestamp;

//Holmes add for omx }

	// Allocators
	PVMFVideoEncNodeAllocDestructDealloc iAlloc;
	OsclMemPoolFixedChunkAllocator iMediaBufferMemPool;
	PVMFSimpleMediaBufferCombinedAlloc* iMediaDataAlloc;
	OsclMemPoolFixedChunkAllocator iMediaDataMemPoolOld;


	// Command queue
	PVMFVideoEncNodeCmdQueue iCmdQueue;

	// A queue is used to hold the current command so it's easy to find out
	// whether a command is in progress, and allow cancel to interrupt
	PVMFVideoEncNodeCmdQueue iCurrentCmd;

	// Ports and port activity
	PVMFVideoEncPortVector iInPort;
	PVMFVideoEncPortVector iOutPort;
	friend class PVMFVideoEncPort;

	PVLogger* iLogger;

	int32 iExtensionRefCount;

	// Encoder
	//CPVM4VEncoder* iVideoEncoder;
	TPVVideoInputFormat iInputFormat;
	TPVVideoEncodeParam iEncodeParam;
	uint32 iSeqNum; /** Sequence number */
	OsclRefCounterMemFrag iVolHeader; /** Vol header */

};

#endif // PVMF_OMX_VIDEOENC_NODE_H_INCLUDED

