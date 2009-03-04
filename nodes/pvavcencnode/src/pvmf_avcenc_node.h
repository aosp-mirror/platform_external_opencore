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

#ifndef PVMF_AVCENC_NODE_H_INCLUDED
#define PVMF_AVCENC_NODE_H_INCLUDED

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
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_NODE_UTILS_H_INCLUDED
#include "pvmf_node_utils.h"
#endif
#ifndef PVMF_SIMPLE_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_simple_media_buffer.h"
#endif
#ifndef PVMF_MEDIA_FRAG_GROUP_H_INCLUDED
#include "pvmf_media_frag_group.h"
#endif
#ifndef PV_MP4_H263_ENC_EXTENSION_H_INCLUDED
#include "pvmp4h263encextension.h"
#endif
#ifndef PVMF_AVCENC_TUNEABLES_H_INCLUDED
#include "pvmf_avcenc_tuneables.h"
#endif
#ifndef PVAVCENCODERINTERFACE_H_INCLUDED
#include "pvavcencoderinterface.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif

#define PROFILING_ON (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_PROF)

#if PROFILING_ON
#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
#endif
#endif


// Forward declarations
class PVMFAvcEncPort;
class PVMFAvcEncNodeOutputData;

// Allocators
typedef OsclMemAllocDestructDealloc<uint8> PVMFAvcEncNodeAllocDestructDealloc;
typedef OsclMemAllocator PVMFAvcEncNodeAlloc;
typedef OsclMemPoolFixedChunkAllocator PVMFAvcEncNodeMemPool;

/** Node command type */
typedef PVMFGenericNodeCommand<PVMFAvcEncNodeAlloc> PVMFAvcEncNodeCommand;

/** Command queue type */
typedef PVMFNodeCommandQueue<PVMFAvcEncNodeCommand, PVMFAvcEncNodeAlloc> PVMFAvcEncNodeCmdQueue;

/** Port vector type */
typedef PVMFPortVector<PVMFAvcEncPort, PVMFAvcEncNodeAlloc> PVMFAvcEncPortVector;

////////////////////////////////////////////////////////////////////////////
class PVMFAvcEncNode : public OsclTimerObject, public PVMFNodeInterface,
            public PVMp4H263EncExtensionInterface,
            public OsclMemPoolFixedChunkAllocatorObserver,
            public PvmiCapabilityAndConfig
{
    public:
        PVMFAvcEncNode(int32 aPriority);
        virtual ~PVMFAvcEncNode();

        // Virtual functions of PVMFNodeInterface
        virtual PVMFStatus ThreadLogon();
        virtual PVMFStatus ThreadLogoff();
        virtual PVMFStatus GetCapability(PVMFNodeCapability& aNodeCapability);
        virtual PVMFPortIter* GetPorts(const PVMFPortFilter* aFilter = NULL);
        virtual PVMFCommandId QueryUUID(PVMFSessionId aSession,
                                        const PvmfMimeString& aMimeType,
                                        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                        bool aExactUuidsOnly = false,
                                        const OsclAny* aContext = NULL);
        virtual PVMFCommandId QueryInterface(PVMFSessionId aSession,
                                             const PVUuid& aUuid,
                                             PVInterface*& aInterfacePtr,
                                             const OsclAny* aContext = NULL);
        virtual PVMFCommandId RequestPort(PVMFSessionId aSession, int32 aPortTag,
                                          const PvmfMimeString* aPortConfig = NULL,
                                          const OsclAny* aContext = NULL);
        virtual PVMFCommandId ReleasePort(PVMFSessionId aSession, PVMFPortInterface& aPort,
                                          const OsclAny* aContext = NULL);
        virtual PVMFCommandId Init(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId Prepare(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId Start(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId Stop(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId Flush(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId Pause(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId Reset(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        virtual PVMFCommandId CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContextData = NULL);
        virtual PVMFCommandId CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId,
                                            const OsclAny* aContextData = NULL);

        // From PVMFPortActivityHandler
        virtual void HandlePortActivity(const PVMFPortActivity& aActivity);

        // Virtual functions of PVMp4H263EncExtensionInterface
        virtual void addRef();
        virtual void removeRef();
        virtual bool queryInterface(const PVUuid& uuid, PVInterface*& iface);
        virtual bool SetNumLayers(uint32 aNumLayers);
        virtual bool SetOutputBitRate(uint32 aLayer, uint32 aBitRate);
        virtual bool SetOutputFrameSize(uint32 aLayer, uint32 aWidth, uint32 aHeight);
        virtual bool SetOutputFrameRate(uint32 aLayer, OsclFloat aFrameRate);
        virtual bool SetSegmentTargetSize(uint32 aLayer, uint32 aSizeBytes);
        virtual bool SetRateControlType(uint32 aLayer, PVMFVENRateControlType aRateControl);
        virtual bool SetDataPartitioning(bool aDataPartitioning);
        virtual bool SetRVLC(bool aRVLC);
        virtual bool SetIFrameInterval(uint32 aIFrameInterval);
        virtual bool GetVolHeader(OsclRefCounterMemFrag& aVolHeader);
        virtual bool RequestIFrame();
        virtual bool SetCodec(PVMFFormatType aCodec);
        // implemetation of PvmiCapabilityAndConfig class functions here
        void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver);

        PVMFStatus getParametersSync(PvmiMIOSession aSession,
                                     PvmiKeyType aIdentifier,
                                     PvmiKvp*& aParameters,
                                     int& aNumParamElements,
                                     PvmiCapabilityContext aContext);
        PVMFStatus releaseParameters(PvmiMIOSession aSession,
                                     PvmiKvp* aParameters,
                                     int num_elements);
        void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                  PvmiKvp* aParameters, int num_parameter_elements);
        void DeleteContext(PvmiMIOSession aSession,
                           PvmiCapabilityContext& aContext);
        void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                               int num_elements, PvmiKvp * & aRet_kvp);
        PVMFCommandId setParametersAsync(PvmiMIOSession aSession,
                                         PvmiKvp* aParameters,
                                         int num_elements,
                                         PvmiKvp*& aRet_kvp,
                                         OsclAny* context = NULL);
        uint32 getCapabilityMetric(PvmiMIOSession aSession);
        PVMFStatus verifyParametersSync(PvmiMIOSession aSession,
                                        PvmiKvp* aParameters,
                                        int num_elements);

        // function used in getParametersSync of capability class
        PVMFStatus GetConfigParameter(PvmiKvp*& aParameters, int& aNumParamElements,
                                      int32 aIndex, PvmiKvpAttr reqattr);
        // function used in VerifyParametersSync n SetParametersSync of capability class
        PVMFStatus VerifyAndSetConfigParameter(PvmiKvp& aParameter, bool aSetParam);


    private:
        void ConstructEncoderParams();

        // From OsclTimerObject
        void Run();

        /////////////////////////////////////////////////////
        //     Command processing routines
        /////////////////////////////////////////////////////
        PVMFCommandId QueueCommandL(PVMFAvcEncNodeCommand& aCmd);
        bool ProcessCommand(PVMFAvcEncNodeCommand& aCmd);
        void CommandComplete(PVMFAvcEncNodeCmdQueue& aCmdQueue, PVMFAvcEncNodeCommand& aCmd,
                             PVMFStatus aStatus, OsclAny* aData = NULL);
        void DoQueryUuid(PVMFAvcEncNodeCommand& aCmd);
        void DoQueryInterface(PVMFAvcEncNodeCommand& aCmd);
        void DoRequestPort(PVMFAvcEncNodeCommand& aCmd);
        PVMFAvcEncPort* AllocatePort(PVMFAvcEncPortVector& aPortVector, int32 aTag, OSCL_String* aMimeType, const char* aName);
        void DoReleasePort(PVMFAvcEncNodeCommand& aCmd);
        void DoInit(PVMFAvcEncNodeCommand& aCmd);
        void DoPrepare(PVMFAvcEncNodeCommand& aCmd);
        void DoStart(PVMFAvcEncNodeCommand& aCmd);
        void DoStop(PVMFAvcEncNodeCommand& aCmd);
        void DeleteAvcEncoder();
        void DoFlush(PVMFAvcEncNodeCommand& aCmd);
        bool IsFlushPending();
        void FlushComplete();
        void DoPause(PVMFAvcEncNodeCommand& aCmd);
        void DoReset(PVMFAvcEncNodeCommand& aCmd);
        void DoCancelAllCommands(PVMFAvcEncNodeCommand& aCmd);
        void DoCancelCommand(PVMFAvcEncNodeCommand& aCmd);

        /////////////////////////////////////////////////////
        //      Port activity processing routines
        /////////////////////////////////////////////////////
        bool IsProcessOutgoingMsgReady();
        bool IsProcessIncomingMsgReady();
        PVMFStatus ProcessIncomingMsg(PVMFPortInterface* aPort);
        PVMFStatus AsyncEncode(PVMFSharedMediaDataPtr& aMediaData);
        PVMFStatus SendEncodedBitstream(PVMFAvcEncNodeOutputData& aOutputData);

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
        void ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData = NULL);
        void ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData = NULL);
        void SetState(TPVMFNodeInterfaceState aState);

        // OsclMemPoolFixedChunkAllocatorObserver
        void freechunkavailable(OsclAny* aContextData);
        PVMFStatus SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg);
        void LogDiagnostics();
    private:

        // Allocators
        PVMFAvcEncNodeAllocDestructDealloc iAlloc;
        // Allocator for simple media data buffer
        OsclMemPoolFixedChunkAllocator* iMediaBufferMemPool;
        PVMFSimpleMediaBufferCombinedAlloc* iMediaDataAlloc;

        // Memory pool for media data buffer impl that holds multiple media fragments
        OsclMemPoolFixedChunkAllocator* iMediaDataGroupImplMemPool;
        PVMFMediaFragGroupCombinedAlloc<OsclMemPoolFixedChunkAllocator>* iMediaDataGroupAlloc;

        OsclMemPoolFixedChunkAllocator iMediaDataMemPool;

        bool iWaitingOnFreeChunk;

        // Command queue
        PVMFAvcEncNodeCmdQueue iCmdQueue;

        // A queue is used to hold the current command so it's easy to find out
        // whether a command is in progress, and allow cancel to interrupt
        PVMFAvcEncNodeCmdQueue iCurrentCmd;

        // Ports and port activity
        PVMFAvcEncPortVector iInPort;
        PVMFAvcEncPortVector iOutPort;
        Oscl_Vector<PVMFPortActivity, PVMFAvcEncNodeAlloc> iPortActivityQueue;
        friend class PVMFAvcEncPort;

        // Encoder
        PVMFFormatType iCodec;
        PVAVCEncoderInterface* iAvcEncoder;
        TAVCEIInputFormat iInputFormat;
        TAVCEIEncodeParam iEncodeParam;
        OsclRefCounterMemFrag iParamSet; /** Vol header */
        uint32 iSeqNum; /** Sequence number */
        bool  iReadyForNextFrame;
        OsclMemoryFragment iSPSs[PVMF_AVCENC_NODE_SPS_VECTOR_RESERVE];
        OsclMemoryFragment iPPSs[PVMF_AVCENC_NODE_PPS_VECTOR_RESERVE];
        int   iNumSPSs;
        int	  iNumPPSs;

        PVLogger* iLogger;
        int32 iExtensionRefCount;

#if PROFILING_ON
        uint32 iMinEncDuration;
        uint32 iMaxEncDuration;
        uint32 iAverageEncDuration;
        uint32 iTotalFramesEncoded;
        uint32 iTotalEncTime;
        uint32 iNumFramesSkipped;
        PVLogger* iDiagnosticsLogger;
        bool oDiagnosticsLogged;

        // Statistics
        struct PVAvcEncNodeStats
        {
            uint32 iNumFrames;
            uint32 iNumFramesSkipped;
            uint32 iDuration;
        };
        PVAvcEncNodeStats iStats;
#endif
};

////////////////////////////////////////////////////////////////////////////
class PVMFAvcEncNodeOutputData
{
    public:
        PVMFAvcEncNodeOutputData();
        PVMFAvcEncNodeOutputData(const PVMFAvcEncNodeOutputData& aData);
        PVMFStatus Allocate(PVMFSimpleMediaBufferCombinedAlloc* aBufferAlloc, PVMFAvcEncNodeMemPool* aMemPool,
                            PVMFMediaFragGroupCombinedAlloc<OsclMemPoolFixedChunkAllocator>* aGroupAlloc);

        TAVCEIOutputData iEncoderOutput;
        PVMFSharedMediaDataPtr iMediaData;
        OsclSharedPtr<PVMFMediaDataImpl> iFragGroupmediaDataImpl;
        OsclRefCounterMemFrag iRefCtrMemFragOut;

};

#endif // PVMF_AVCENC_NODE_H_INCLUDED

