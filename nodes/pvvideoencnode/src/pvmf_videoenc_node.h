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
#ifndef PVMF_VIDEOENC_NODE_H_INCLUDED
#define PVMF_VIDEOENC_NODE_H_INCLUDED

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
#ifndef PVMF_VIDEOENC_TUNEABLES_H_INCLUDED
#include "pvmf_videoenc_tuneables.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif
#ifndef OSCL_TICKCOUNT_H_INCLUDED
#include "oscl_tickcount.h"
#endif
// Forward declarations
class PVMFVideoEncPort;
class PVMFVideoEncNodeOutputData;
class OsclClock;
// Allocators
typedef OsclMemAllocDestructDealloc<uint8> PVMFVideoEncNodeAllocDestructDealloc;
typedef OsclMemAllocator PVMFVideoEncNodeAlloc;
typedef OsclMemPoolFixedChunkAllocator PVMFVideoEncNodeMemPool;

/** Node command type */
typedef PVMFGenericNodeCommand<PVMFVideoEncNodeAlloc> PVMFVideoEncNodeCommand;

/** Command queue type */
typedef PVMFNodeCommandQueue<PVMFVideoEncNodeCommand, PVMFVideoEncNodeAlloc> PVMFVideoEncNodeCmdQueue;

/** Port vector type */
typedef PVMFPortVector<PVMFVideoEncPort, PVMFVideoEncNodeAlloc> PVMFVideoEncPortVector;

////////////////////////////////////////////////////////////////////////////
class PVMFVideoEncNode : public OsclTimerObject, public PVMFNodeInterface, public MPVCVEIObserver,
            public PVMp4H263EncExtensionInterface, public PvmiCapabilityAndConfig
{
    public:
        PVMFVideoEncNode(int32 aPriority);
        ~PVMFVideoEncNode();

        // Virtual functions of PVMFNodeInterface
        OSCL_IMPORT_REF PVMFStatus ThreadLogon();
        OSCL_IMPORT_REF PVMFStatus ThreadLogoff();
        OSCL_IMPORT_REF PVMFStatus GetCapability(PVMFNodeCapability& aNodeCapability);
        OSCL_IMPORT_REF PVMFPortIter* GetPorts(const PVMFPortFilter* aFilter = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryUUID(PVMFSessionId aSession,
                                                const PvmfMimeString& aMimeType,
                                                Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                                bool aExactUuidsOnly = false,
                                                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryInterface(PVMFSessionId aSession,
                const PVUuid& aUuid,
                PVInterface*& aInterfacePtr,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId RequestPort(PVMFSessionId aSession, int32 aPortTag,
                const PvmfMimeString* aPortConfig = NULL,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId ReleasePort(PVMFSessionId aSession, PVMFPortInterface& aPort,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Init(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Prepare(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Start(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Stop(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Flush(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Pause(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Reset(PVMFSessionId aSession, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContextData = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId,
                const OsclAny* aContextData = NULL);

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
        PVMFCommandId QueueCommandL(PVMFVideoEncNodeCommand& aCmd);
        bool ProcessCommand(PVMFVideoEncNodeCommand& aCmd);
        void CommandComplete(PVMFVideoEncNodeCmdQueue& aCmdQueue, PVMFVideoEncNodeCommand& aCmd,
                             PVMFStatus aStatus, OsclAny* aData = NULL);
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
        PVMFStatus SendEncodedBitstream(PVMFVideoEncNodeOutputData& aOutputData);

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
        PVMFStatus SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg);
    private:
        void LogDiagnostics();
        // Allocators
        PVMFVideoEncNodeAllocDestructDealloc iAlloc;
        PVMFVideoEncNodeMemPool iMediaBufferMemPool;
        PVMFSimpleMediaBufferCombinedAlloc* iMediaDataAlloc;
        PVMFVideoEncNodeMemPool iMediaDataMemPool;

        // Command queue
        PVMFVideoEncNodeCmdQueue iCmdQueue;

        // A queue is used to hold the current command so it's easy to find out
        // whether a command is in progress, and allow cancel to interrupt
        PVMFVideoEncNodeCmdQueue iCurrentCmd;

        // Ports and port activity
        PVMFVideoEncPortVector iInPort;
        PVMFVideoEncPortVector iOutPort;
        Oscl_Vector<PVMFPortActivity, PVMFVideoEncNodeAlloc> iPortActivityQueue;
        friend class PVMFVideoEncPort;

        // Encoder
        CPVM4VEncoder* iVideoEncoder;
        TPVVideoInputFormat iInputFormat;
        TPVVideoEncodeParam iEncodeParam;
        OsclRefCounterMemFrag iVolHeader; /** Vol header */
        uint32 iSeqNum; /** Sequence number */

        PVLogger* iLogger;

        PVLogger* iDiagnosticsLogger;
        uint32 total_ticks;

        int32 iExtensionRefCount;

#if PROFILING_ON
        // Statistics
        struct PVVideoEncNodeStats
        {
            uint32 iNumFrames;
            uint32 iNumFramesSkipped;
            uint32 iDuration;
            uint32 iMinEncTime;
            uint32 iMaxEncTime;
            uint32 iAverageEncTime;
        };
        PVVideoEncNodeStats iStats;
        bool oDiagnosticsLogged;
#endif
};

////////////////////////////////////////////////////////////////////////////
class PVMFVideoEncNodeOutputData
{
    public:
        PVMFVideoEncNodeOutputData();
        PVMFVideoEncNodeOutputData(const PVMFVideoEncNodeOutputData& aData);
        PVMFStatus Allocate(PVMFSimpleMediaBufferCombinedAlloc* aBufferAlloc, PVMFVideoEncNodeMemPool* aMemPool);

        TPVVideoOutputData iEncoderOutput;
        PVMFSharedMediaDataPtr iMediaData;
};

#endif // PVMF_VIDEOENC_NODE_H_INCLUDED

