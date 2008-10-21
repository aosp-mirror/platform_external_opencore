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
#ifndef PVMF_OMX_AUDIODEC_NODE_H_INCLUDED
#define PVMF_OMX_AUDIODEC_NODE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif

#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#include "pvmf_format_type.h"
#endif

#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif

#ifndef OSCL_PRIQUEUE_H_INCLUDED
#include "oscl_priqueue.h"
#endif

#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif

#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif
#ifndef PVMF_MEMPOOL_H_INCLUDED
#include "pvmf_mempool.h"
#endif

#ifndef PVMF_SIMPLE_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_simple_media_buffer.h"
#endif

#ifndef PVMF_POOL_BUFFER_ALLOCATOR_H_INCLUDED
#include "pvmf_pool_buffer_allocator.h"
#endif


#ifndef PVMF_POOL_BUFFER_ALLOCATOR_H_INCLUDED
#include "pvmf_pool_buffer_allocator.h"
#endif

#ifndef PVMF_NODE_UTILS_H_INCLUDED
#include "pvmf_node_utils.h"
#endif

#ifndef PVMF_OMX_AUDIODEC_PORT_H_INCLUDED
#include "pvmf_omx_audiodec_port.h"
#endif

#ifndef PVMF_OMX_AUDIODEC_NODE_EXTENSION_INTERFACE_H_INCLUDED
#include "pvmf_omx_audiodec_node_extension_interface.h"
#endif

#ifndef PVMF_META_DATA_EXTENSION_H_INCLUDED
#include "pvmf_meta_data_extension.h"
#endif

#ifndef PV_MIME_STRING_UTILS_H_INCLUDED
#include "pv_mime_string_utils.h"
#endif

#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#ifndef PVMF_OMX_AUDIODEC_CALLBACKS_H_INCLUDED
#include "pvmf_omx_audiodec_callbacks.h"
#endif


#ifndef OSCLCONFIG_IO_H_INCLUDED
#include "osclconfig_io.h"
#endif
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#if (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_REL)
#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
#endif
#endif

#define PVMFOMXAUDIODECNODE_NUM_CMD_IN_POOL 8
#define PVOMXAUDIODEC_DEFAULT_SAMPLINGRATE 48000
#define PVOMXAUDIODEC_DEFAULT_OUTPUTPCM_TIME 200
#define PVOMXAUDIODEC_AMRNB_SAMPLES_PER_FRAME 160
#define PVOMXAUDIODEC_AMRWB_SAMPLES_PER_FRAME 320
#define PVOMXAUDIODEC_MP3_DEFAULT_SAMPLES_PER_FRAME 1152

struct channelSampleInfo
{
    uint32 desiredChannels;
    uint32 samplingRate;
};


typedef struct OutputBufCtrlStruct_Audio
{
    OMX_BUFFERHEADERTYPE *pBufHdr;
}OutputBufCtrlStruct_Audio;

typedef struct InputBufCtrlStruct_Audio
{
    OMX_BUFFERHEADERTYPE *pBufHdr;
    PVMFSharedMediaDataPtr pMediaData;
} InputBufCtrlStruct_Audio;


// fwd class declaration
class PVLogger;
class PV_LATM_Parser;
//memory allocator type for this node.
typedef OsclMemAllocator PVMFOMXAudioDecNodeAllocator;


// CALLBACK PROTOTYPES
OMX_ERRORTYPE CallbackEventHandler_Audio(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_EVENTTYPE aEvent,
        OMX_OUT OMX_U32 aData1,
        OMX_OUT OMX_U32 aData2,
        OMX_OUT OMX_PTR aEventData);

OMX_ERRORTYPE CallbackEmptyBufferDone_Audio(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer);

OMX_ERRORTYPE CallbackFillBufferDone_Audio(OMX_OUT OMX_HANDLETYPE aComponent,
        OMX_OUT OMX_PTR aAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* aBuffer);



//Default values for number of Input/Output buffers. If the component needs more than this, it will be
// negotiated. If the component does not need more than this number, the default is used
#define NUMBER_INPUT_BUFFER 5
#define NUMBER_OUTPUT_BUFFER 9


////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////
/////////////////////////
/////////////////////////
// CUSTOM DEALLOCATOR FOR MEDIA DATA SHARED PTR WRAPPER:
//						1) Deallocates the underlying output buffer
//						2) Deallocates the pvci buffer wrapper and the rest of accompanying structures
//					  Deallocator is created as part of the wrapper, and travels with the buffer wrapper

class PVOMXBufferSharedPtrWrapperCombinedCleanupDA : public OsclDestructDealloc
{
    public:
        PVOMXBufferSharedPtrWrapperCombinedCleanupDA(Oscl_DefAlloc* allocator, void *pMempoolData) :
                buf_alloc(allocator), ptr_to_data_to_dealloc(pMempoolData) {};
        virtual ~PVOMXBufferSharedPtrWrapperCombinedCleanupDA() {};

        virtual void destruct_and_dealloc(OsclAny* ptr)
        {
            // call buffer deallocator
            if (buf_alloc != NULL)
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


#define PVMFOMXAudioDecNodeCommandBase PVMFGenericNodeCommand<PVMFOMXAudioDecNodeAllocator>  // to remove typedef warning on symbian

class PVMFOMXAudioDecNodeCommand: public PVMFOMXAudioDecNodeCommandBase
{
    public:
        //constructor for Custom2 command
        void Construct(PVMFSessionId s, int32 cmd, int32 arg1, int32 arg2, int32& arg3, const OsclAny*aContext)
        {
            PVMFOMXAudioDecNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)arg1;
            iParam2 = (OsclAny*)arg2;
            iParam3 = (OsclAny*) & arg3;
        }
        void Parse(int32&arg1, int32&arg2, int32*&arg3)
        {
            arg1 = (int32)iParam1;
            arg2 = (int32)iParam2;
            arg3 = (int32*)iParam3;
        }

        void Construct(PVMFSessionId s, int32 cmd, PVMFMetadataList* aKeyList, uint32 aStartIndex, int32 aMaxEntries, char* aQueryKey, const OsclAny* aContext)
        {
            PVMFOMXAudioDecNodeCommandBase::Construct(s, cmd, aContext);
            iStartIndex = aStartIndex;
            iMaxEntries = aMaxEntries;

            if (aQueryKey == NULL)
            {
                query_key[0] = 0;
            }
            else
            {
                if (aQueryKey != NULL)
                    oscl_strncpy(query_key, aQueryKey, oscl_strlen(aQueryKey) + 1);
            }

            iParam1 = (OsclAny*)aKeyList;
            iParam2 = NULL;
            iParam3 = NULL;
            iParam4 = NULL;
            iParam5 = NULL;
        }

        void Parse(PVMFMetadataList*& MetaDataListPtr, uint32 &aStartingIndex, int32 &aMaxEntries, char*&aQueryKey)
        {
            MetaDataListPtr = (PVMFMetadataList*)iParam1;
            aStartingIndex = iStartIndex;
            aMaxEntries = iMaxEntries;
            if (query_key[0] == 0)
            {
                aQueryKey = NULL;
            }
            else
            {
                aQueryKey = query_key;
            }
        }

        // Constructor and parser for GetNodeMetadataValue
        void Construct(PVMFSessionId s, int32 cmd, PVMFMetadataList* aKeyList, Oscl_Vector<PvmiKvp, OsclMemAllocator>* aValueList, uint32 aStartIndex, int32 aMaxEntries, const OsclAny* aContext)
        {
            PVMFOMXAudioDecNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aKeyList;
            iParam2 = (OsclAny*)aValueList;

            iStartIndex = aStartIndex;
            iMaxEntries = aMaxEntries;

            iParam3 = NULL;
            iParam4 = NULL;
            iParam5 = NULL;
        }
        void Parse(PVMFMetadataList* &aKeyList, Oscl_Vector<PvmiKvp, OsclMemAllocator>* &aValueList, uint32 &aStartingIndex, int32 &aMaxEntries)
        {
            aKeyList = (PVMFMetadataList*)iParam1;
            aValueList = (Oscl_Vector<PvmiKvp, OsclMemAllocator>*)iParam2;
            aStartingIndex = iStartIndex;
            aMaxEntries = iMaxEntries;
        }

        virtual bool hipri()
        {	//this routine identifies commands that need to
            //go at the front of the queue.  derived command
            //classes can override it if needed.
            return (iCmd == PVOMXAUDIODEC_NODE_CMD_CANCELALL
                    || iCmd == PVOMXAUDIODEC_NODE_CMD_CANCELCMD);
        }

        enum PVOMXAudioDecNodeCmdType
        {
            PVOMXAUDIODEC_NODE_CMD_QUERYUUID,
            PVOMXAUDIODEC_NODE_CMD_QUERYINTERFACE,
            PVOMXAUDIODEC_NODE_CMD_INIT,
            PVOMXAUDIODEC_NODE_CMD_PREPARE,
            PVOMXAUDIODEC_NODE_CMD_REQUESTPORT,
            PVOMXAUDIODEC_NODE_CMD_START,
            PVOMXAUDIODEC_NODE_CMD_PAUSE,
            PVOMXAUDIODEC_NODE_CMD_STOP,
            PVOMXAUDIODEC_NODE_CMD_FLUSH,
            PVOMXAUDIODEC_NODE_CMD_RELEASEPORT,
            PVOMXAUDIODEC_NODE_CMD_RESET,
            PVOMXAUDIODEC_NODE_CMD_CANCELCMD,
            PVOMXAUDIODEC_NODE_CMD_CANCELALL,
            PVOMXAUDIODEC_NODE_CMD_INVALID,
            PVOMXAUDIODEC_NODE_CMD_GETNODEMETADATAKEY,
            PVOMXAUDIODEC_NODE_CMD_GETNODEMETADATAVALUE
        };

    private:
        uint32 iStartIndex;
        uint32 iMaxEntries;
        char query_key[256];

};

//Default vector reserve size
#define PVMF_OMXAUDIODEC_NODE_COMMAND_VECTOR_RESERVE 10

//Starting value for command IDs
#define PVMF_OMXAUDIODEC_NODE_COMMAND_ID_START 6000

///////////////////////////////////////////////////////////////////////////////////////////////////////
//CAPABILITY AND CONFIG

// Structure to hold the key string info for
// videodecnode's capability-and-config
struct PVOMXAudioDecNodeKeyStringData
{
    char iString[64];
    PvmiKvpType iType;
    PvmiKvpValueType iValueType;
};

// The number of characters to allocate for the key string
#define PVOMXAUDIODECNODECONFIG_KEYSTRING_SIZE 128



/// #########################################################
/// #########################################################
// Key string info at the base level ("x-pvmf/audio/decoder")
#define PVOMXAUDIODECNODECONFIG_BASE_NUMKEYS 6
const PVOMXAudioDecNodeKeyStringData PVOMXAudioDecNodeConfigBaseKeys[PVOMXAUDIODECNODECONFIG_BASE_NUMKEYS] =
{
    {"silenceinsertion_enable", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL},
    {"aac_he_v1_enable", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL},
    {"aac_he_v2_enable", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL},
    {"format-type", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32}
};

// Key string info at render ("x-pvmf/video/render")
#define PVOMXAUDIODECNODECONFIG_RENDER_NUMKEYS 2
const PVOMXAudioDecNodeKeyStringData PVOMXAudioDecNodeConfigRenderKeys[PVOMXAUDIODECNODECONFIG_RENDER_NUMKEYS] =
{
    {"sampling_rate", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"channels", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32}
};


/////////////////////////////////////////////////////////////////////////////////////////
/////////###############################################################
//// ###################################################################





//Mimetypes for the custom interface
#define PVMF_OMX_AUDIO_DEC_NODE_MIMETYPE "pvxxx/OMXAudioDecNode"
#define PVMF_BASEMIMETYPE "pvxxx"

//Command queue type
typedef PVMFNodeCommandQueue<PVMFOMXAudioDecNodeCommand, PVMFOMXAudioDecNodeAllocator> PVMFOMXAudioDecNodeCmdQ;


class PVMFOMXAudioDecNode
            : public OsclActiveObject
            , public PVMFNodeInterface
            , public OsclMemPoolFixedChunkAllocatorObserver
            , public PVMFOMXAudioDecNodeExtensionInterface
            , public PVMFMetadataExtensionInterface
            , public PvmiCapabilityAndConfig

{
    public:
        PVMFOMXAudioDecNode(int32 aPriority);
        ~PVMFOMXAudioDecNode();

        // From PVMFNodeInterface
        PVMFStatus ThreadLogon();
        PVMFStatus ThreadLogoff();
        PVMFStatus GetCapability(PVMFNodeCapability& aNodeCapability);
        PVMFPortIter* GetPorts(const PVMFPortFilter* aFilter = NULL);
        PVMFCommandId QueryUUID(PVMFSessionId, const PvmfMimeString& aMimeType,
                                Oscl_Vector<PVUuid, PVMFOMXAudioDecNodeAllocator>& aUuids,
                                bool aExactUuidsOnly = false,
                                const OsclAny* aContext = NULL);
        PVMFCommandId QueryInterface(PVMFSessionId, const PVUuid& aUuid,
                                     PVInterface*& aInterfacePtr,
                                     const OsclAny* aContext = NULL);
        PVMFCommandId RequestPort(PVMFSessionId
                                  , int32 aPortTag, const PvmfMimeString* aPortConfig = NULL, const OsclAny* aContext = NULL);
        PVMFCommandId ReleasePort(PVMFSessionId, PVMFPortInterface& aPort, const OsclAny* aContext = NULL);
        PVMFCommandId Init(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId Prepare(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId Start(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId Stop(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId Flush(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId Pause(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId Reset(PVMFSessionId, const OsclAny* aContext = NULL);
        PVMFCommandId CancelAllCommands(PVMFSessionId, const OsclAny* aContextData = NULL);
        PVMFCommandId CancelCommand(PVMFSessionId, PVMFCommandId aCmdId, const OsclAny* aContextData = NULL);

        // From PVMFPortActivityHandler
        void HandlePortActivity(const PVMFPortActivity& aActivity);

        // From PVInterface
        virtual void addRef();
        virtual void removeRef();
        virtual bool queryInterface(const PVUuid& uuid, PVInterface*& iface);
        virtual PVMFStatus SetDecoderNodeConfiguration(PVMFOMXAudioDecNodeConfig& aConfig);

        //**********begin PVMFMetadataExtensionInterface
        uint32 GetNumMetadataKeys(char* query_key = NULL);
        uint32 GetNumMetadataValues(PVMFMetadataList& aKeyList);
        PVMFCommandId GetNodeMetadataKeys(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, uint32 starting_index, int32 max_entries,
                                          char* query_key = NULL, const OsclAny* aContextData = NULL);
        PVMFCommandId GetNodeMetadataValues(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList,
                                            Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 starting_index, int32 max_entries, const OsclAny* aContextData = NULL);
        PVMFStatus ReleaseNodeMetadataKeys(PVMFMetadataList& aKeyList, uint32 starting_index, uint32 end_index);
        PVMFStatus ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 starting_index, uint32 end_index);
        //**********End PVMFMetadataExtensionInterface

        //********** CB Functions to serve OpenMAX Decoder

        //Process callback functions. They will be executed in testapp thread context
        //	These callbacks are used only in the Multithreaded component case
        OsclReturnCode ProcessCallbackEventHandler_MultiThreaded(OsclAny* P);
        OsclReturnCode ProcessCallbackEmptyBufferDone_MultiThreaded(OsclAny* P);
        OsclReturnCode ProcessCallbackFillBufferDone_MultiThreaded(OsclAny* P);

        //Callback objects - again, these are used only in the case of Multithreaded component
        EventHandlerThreadSafeCallbackAO_Audio*	 iThreadSafeHandlerEventHandler;
        EmptyBufferDoneThreadSafeCallbackAO_Audio* iThreadSafeHandlerEmptyBufferDone;
        FillBufferDoneThreadSafeCallbackAO_Audio*  iThreadSafeHandlerFillBufferDone;

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



        bool IsComponentMultiThreaded()
        {
            return iIsOMXComponentMultiThreaded;
        };


        // From PvmiCapabilityAndConfig
        void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver);
        PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext);
        PVMFStatus releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements);
        void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext, PvmiKvp* aParameters, int aNumParamElements);
        void DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP);
        PVMFCommandId setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp*& aRetKVP, OsclAny* aContext = NULL);
        uint32 getCapabilityMetric(PvmiMIOSession aSession);
        PVMFStatus verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements);

        // for WMA params
        bool VerifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);


    private:
        void CommandComplete(PVMFOMXAudioDecNodeCmdQ& aCmdQ, PVMFOMXAudioDecNodeCommand& aCmd, PVMFStatus aStatus, OsclAny* aEventData = NULL);

        void DoQueryUuid(PVMFOMXAudioDecNodeCommand&);
        void DoQueryInterface(PVMFOMXAudioDecNodeCommand&);
        void DoRequestPort(PVMFOMXAudioDecNodeCommand&);
        void DoReleasePort(PVMFOMXAudioDecNodeCommand&);
        void DoInit(PVMFOMXAudioDecNodeCommand&);
        void DoPrepare(PVMFOMXAudioDecNodeCommand&);
        void DoStart(PVMFOMXAudioDecNodeCommand&);
        void DoStop(PVMFOMXAudioDecNodeCommand&);
        void DoPause(PVMFOMXAudioDecNodeCommand&);
        void DoReset(PVMFOMXAudioDecNodeCommand&);
        void DoFlush(PVMFOMXAudioDecNodeCommand&);
        PVMFStatus DoGetNodeMetadataKey(PVMFOMXAudioDecNodeCommand&);
        PVMFStatus DoGetNodeMetadataValue(PVMFOMXAudioDecNodeCommand&);
        void DoCancelAllCommands(PVMFOMXAudioDecNodeCommand&);
        void DoCancelCommand(PVMFOMXAudioDecNodeCommand&);

        void Run();
        bool ProcessCommand(PVMFOMXAudioDecNodeCommand& aCmd);
        bool ProcessIncomingMsg(PVMFPortInterface* aPort);
        bool ProcessOutgoingMsg(PVMFPortInterface* aPort);
        PVMFStatus HandleProcessingState();

        bool InitDecoder(PVMFSharedMediaDataPtr&);

        bool NegotiateComponentParameters();
        bool GetSetCodecSpecificInfo();
        bool SetDefaultCapabilityFlags();
        bool CreateOutMemPool(uint32 num);
        bool CreateInputMemPool(uint32 num);
        bool ProvideBuffersToComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
                                       uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                       uint32 aNumBuffers,    // number of buffers
                                       uint32 aActualBufferSize, // aactual buffer size
                                       uint32 aPortIndex,      // port idx
                                       bool aUseBufferOK,	// can component use OMX_UseBuffer?
                                       bool	aIsThisInputBuffer // is this input or output
                                      );

        bool FreeBuffersFromComponent(OsclMemPoolFixedChunkAllocator *aMemPool, // allocator
                                      uint32 aAllocSize,	 // size to allocate from pool (hdr only or hdr+ buffer)
                                      uint32 aNumBuffers,    // number of buffers
                                      uint32 aPortIndex,      // port idx
                                      bool	aIsThisInputBuffer		// is this input or output
                                     );

        OsclSharedPtr<class PVMFMediaDataImpl> WrapOutputBuffer(uint8 *pData, uint32 aDataLen, OsclAny *pContext);
        bool QueueOutputBuffer(OsclSharedPtr<PVMFMediaDataImpl> &mediadataimplout, uint32 aDataLen);

        bool SendOutputBufferToOMXComponent();
        bool SendInputBufferToOMXComponent();

        bool SendConfigBufferToOMXComponent(uint8 *initbuffer, uint32 initbufsize);
        bool SendEOSBufferToOMXComponent();

        bool HandleRepositioning(void);
        bool SendBeginOfMediaStreamCommand(void);
        bool SendEndOfTrackCommand(void);

        // latm parser for AAC - LATM
        PVMFStatus CreateLATMParser(void);
        PVMFStatus DeleteLATMParser(void);

        bool ReleaseAllPorts();
        bool DeleteOMXAudioDecoder();

        void ChangeNodeState(TPVMFNodeInterfaceState aNewState);

        void HandleComponentStateChange(OMX_U32 decoder_state);



        // Capability And Config Helper Methods
        PVMFStatus DoCapConfigGetParametersSync(PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext);
        PVMFStatus DoCapConfigReleaseParameters(PvmiKvp* aParameters, int aNumElements);
        void DoCapConfigSetParameters(PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP);
        PVMFStatus DoCapConfigVerifyParameters(PvmiKvp* aParameters, int aNumElements);

        PVMFStatus DoGetVideoDecNodeParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr);
        PVMFStatus DoVerifyAndSetVideoDecNodeParameter(PvmiKvp& aParameter, bool aSetParam);


        // From OsclMemPoolFixedChunkAllocatorObserver
        void freechunkavailable(OsclAny*);

        PVMFCommandId QueueCommandL(PVMFOMXAudioDecNodeCommand& aCmd);


        friend class PVMFOMXAudioDecPort;

        // Ports pointers
        PVMFPortInterface* iInPort;
        PVMFPortInterface* iOutPort;

        // Commands
        PVMFOMXAudioDecNodeCmdQ iInputCommands;
        PVMFOMXAudioDecNodeCmdQ iCurrentCommand;

        // Shared pointer for Media Msg.Input buffer
        PVMFSharedMediaDataPtr iDataIn;


        // OUTPUT BUFFER RELATED MEMBERS


        // Output buffer memory pool
        OsclMemPoolFixedChunkAllocator *iOutBufMemoryPool;
        //PVOMXBufferAllocatorImplementation *iOutBufMemoryPool;
        // Memory pool for simple media data
        OsclMemPoolFixedChunkAllocator *iMediaDataMemPool;

        // Fragment pool for format specific info
        PVMFBufferPoolAllocator iFsiFragmentAlloc;

        // Size of output buffer (negotiated with component)
        uint32 iOMXComponentOutputBufferSize;

        // size of output to allocate (OMX_ALLOCATE_BUFFER =  size of buf header )
        // (OMX_USE_BUFFER = size of buf header + iOMXCoponentOutputBufferSize)
        uint32 iOutputAllocSize;

        // Number of output buffers (negotiated with component)
        uint32 iNumOutputBuffers;

        // Number of output buffers in possession of the component or downstream,
        // namely, number of unavailable buffers
        uint32 iNumOutstandingOutputBuffers;

        // flag to prevent sending output buffers downstream during flushing etc.
        bool iDoNotSendOutputBuffersDownstreamFlag;

        // flag to prevent freeing the buffers twice
        bool iOutputBuffersFreed;


        // INPUT BUFFER RELATED MEMBERS
        OsclMemPoolFixedChunkAllocator *iInBufMemoryPool;
        uint32 iOMXComponentInputBufferSize; // size of input buffer that the component sees (negotiated with the component)
        uint32 iInputAllocSize; 	// size of input buffer to allocate (OMX_ALLOCATE_BUFFER =  size of buf header )
        // (OMX_USE_BUFFER = size of buf header + iOMXCoponentInputBufferSize)
        uint32 iNumInputBuffers; // total num of input buffers (negotiated with component)

        uint32 iNumOutstandingInputBuffers; // number of input buffers in use (i.e. unavailable)

        bool iDoNotSaveInputBuffersFlag;

        // flag to prevent freeing buffers twice
        bool iInputBuffersFreed;

        // input buffer fragmentation etc.
        uint32 iCopyPosition;				// for copying memfrag data into a buffer
        uint32 iFragmentSizeRemainingToCopy;
        bool	iIsNewDataFragment;

        // input data info
        uint32 iCurrFragNum;
        uint32 iCodecSeqNum;	// sequence number tracking
        uint32 iInPacketSeqNum;

        uint32 iInTimestamp;
        uint32 iInDuration;
        uint32 iInNumFrags;
        uint32 iCurrentMsgMarkerBit;

        // DYNAMIC PORT RE-CONFIGURATION
        uint32 iInputPortIndex;
        uint32 iOutputPortIndex;
        OMX_PARAM_PORTDEFINITIONTYPE iParamPort;
        uint32 iPortIndexForDynamicReconfig;
        bool iSecondPortReportedChange;
        bool iDynamicReconfigInProgress;
        uint32 iSecondPortToReconfig;
        bool iPauseCommandWasSentToComponent;
        bool iStopCommandWasSentToComponent;



        OMX_BUFFERHEADERTYPE *iInputBufferToResendToComponent; // ptr to input buffer that is not empty, but that the OMX component returned
        // we need to resend this same buffer back to the component


        ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS
        bool iOMXComponentSupportsExternalOutputBufferAlloc;
        bool iOMXComponentSupportsExternalInputBufferAlloc;
        bool iOMXComponentSupportsMovableInputBuffers;
        bool iSetMarkerBitForEveryFrag; // is every fragment complete frame (AVC file playback = each fragment is a NAL)
        bool iIsOMXComponentMultiThreaded;

        // State definitions for HandleProcessingState() state machine
        typedef enum
        {
            EPVMFOMXAudioDecNodeProcessingState_Idle,                  //default state after constraction/reset
            EPVMFOMXAudioDecNodeProcessingState_InitDecoder,           //initialization of H264 decoder after handle was obtained
            EPVMFOMXAudioDecNodeProcessingState_WaitForInitCompletion, // waiting for init completion
            EPVMFOMXAudioDecNodeProcessingState_ReadyToDecode,         //nornal operation state of the decoder
            EPVMFOMXAudioDecNodeProcessingState_WaitForOutputBuffer,   //wait state for avalible media output buffer
            EPVMFOMXAudioDecNodeProcessingState_WaitForOutputPort,     //wait state, output port is busy
            EPVMFOMXAudioDecNodeProcessingState_WaitForOutgoingQueue,   //wait state, outgoing queue
            EPVMFOMXAudioDecNodeProcessingState_PortReconfig,			// Dynamic Port Reconfiguration - step 1
            EPVMFOMXAudioDecNodeProcessingState_WaitForBufferReturn,		//	step 2
            EPVMFOMXAudioDecNodeProcessingState_WaitForPortDisable,		// Dynamic Port Reconfiguration - step 3
            EPVMFOMXAudioDecNodeProcessingState_PortReEnable,			// Dynamic Port Reconfiguration - step 4
            EPVMFOMXAudioDecNodeProcessingState_WaitForPortEnable,		// step 5
            EPVMFOMXAudioDecNodeProcessingState_Stopping,				// when STOP command is issued, the node has to wait for component to transition into
            // idle state. The buffers keep coming back , the node is rescheduled
            // to run. Prevent the node from sending buffers back
            EPVMFOMXAudioDecNodeProcessingState_Pausing					// when PAUSE command is issued, the node has to wait for component to transition into
            // paused state. This
            // prevents the node from sending buffers back
        } PVMFOMXAudioDecNode_ProcessingState;

        // State of HandleProcessingState() state machine
        PVMFOMXAudioDecNode_ProcessingState iProcessingState;

        // Handle of OMX Component
        OMX_HANDLETYPE iOMXAudioDecoder;

        // Current State of the component
        OMX_STATETYPE iCurrentDecoderState;

        // BOS
        bool iSendBOS;
        uint32 iStreamID;
        uint32 iBOSTimestamp;

        // repositioning related flags
        bool iIsRepositioningRequestSentToComponent;
        bool iIsRepositionDoneReceivedFromComponent;
        bool iIsOutputPortFlushed;
        bool iIsInputPortFlushed;

        bool iIsRepositionIdleDoneReceivedFromComponent;
        bool iIsRepositionIdleRequestSentToComponent;
        bool iIsRepositionExecRequestSentToComponent;
        bool iIsRepositionExecDoneReceivedFromComponent;

        //EOS control flags
        bool iIsEOSSentToComponent;
        bool iIsEOSReceivedFromComponent;


        // Send Fsi configuration flag
        bool	sendFsi;

        // Audio parameters
        // the output buffer size is calculated from the parameters below
        uint32 iPCMSamplingRate;		// typically 8,16,22.05,32,44.1, 48 khz
        uint32 iNumberOfAudioChannels;	// can be 1 or 2
        uint32 iSamplesPerFrame;		// number of samples per 1 frame of data (if known) per channel
        uint32 iNumBytesPerFrame;		// depends on number of samples/channel and number of channels
        uint32 iMilliSecPerFrame;		//


        // LATM parser for AAC
        PV_LATM_Parser *iLATMParser;
        uint8 *iLATMConfigBuffer;
        uint32 iLATMConfigBufferSize;

        // Pointer to input data fragment
        uint8* iBitstreamBuffer;
        // Size of input data fragment
        int32 iBitstreamSize;

        // Output frame sequence counter
        uint32 iSeqNum;

        // Input frame sequence counter
        uint32 iSeqNum_In;

        // Added to Scheduler Flag
        bool iIsAdded;

        // Log related
        PVLogger* iLogger;
        PVLogger* iDataPathLogger;
        PVLogger* iClockLogger;
        PVLogger *iRunLogger;

        // Counter of fragment read from current Media Msg.Input buffer
        uint fragnum;
        // Number of fragments in the Media Msg.Input buffer
        uint numfrags;

        // Time stamp to be used on output buffer
        uint32 iOutTimeStamp;

        // Node configuration update
        PVMFOMXAudioDecNodeConfig iNodeConfig;

        // Capability exchange
        PVMFNodeCapability iCapability;

        // Reference counter for extension
        uint32 iExtensionRefCount;

        // Vector for KVP
        Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> iAvailableMetadataKeys;

        // EOS flag
        bool iEndOfDataReached;
        // Time stame upon EOS
        PVMFTimestamp iEndOfDataTimestamp;

        /* Diagnostic log related */
        PVLogger* iDiagnosticsLogger;
        bool iDiagnosticsLogged;
        void LogDiagnostics();

        uint32 iFrameCounter;

        uint32 iH263MaxBitstreamFrameSize;
        uint32 iH263MaxWidth;
        uint32 iH263MaxHeight;
        uint32 iM4VMaxBitstreamFrameSize;
        uint32 iM4VMaxWidth;
        uint32 iM4VMaxHeight;

        uint32 iNewWidth , iNewHeight;

        uint32 iAvgBitrateValue;
        bool iResetInProgress;
        bool iResetMsgSent;
};


#endif // PVMF_OMXAUDIODEC_NODE_H_INCLUDED

