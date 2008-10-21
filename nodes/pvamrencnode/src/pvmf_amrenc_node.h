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
#define PVMF_AMRENC_NODE_H_INCLUDED

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
#ifndef PVMFAMRENCNODE_EXTENSION_H_INCLUDED
#include "pvmfamrencnode_extension.h"
#endif
#ifndef PVMF_AMRENC_DATA_PROCESSOR_INTERFACE_H_INCLUDED
#include "pvmf_amrenc_data_processor_interface.h"
#endif
#ifndef PVMF_AMRENC_TUNEABLES_H_INCLUDED
#include "pvmf_amrenc_tuneables.h"
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
class PvmfAmrEncPort;

// Allocators
typedef OsclMemAllocDestructDealloc<uint8> PvmfAmrEncNodeAllocDestructDealloc;
typedef OsclMemAllocator PvmfAmrEncNodeAlloc;
typedef OsclMemPoolFixedChunkAllocator PvmfAmrEncNodeMemPool;

/** Node command type */
typedef PVMFGenericNodeCommand<PvmfAmrEncNodeAlloc> PvmfAmrEncNodeCommand;

/** Command queue type */
typedef PVMFNodeCommandQueue<PvmfAmrEncNodeCommand, PvmfAmrEncNodeAlloc> PvmfAmrEncNodeCmdQueue;

/** Port vector type */
typedef PVMFPortVector<PvmfAmrEncPort, PvmfAmrEncNodeAlloc> PvmfAmrEncPortVector;

////////////////////////////////////////////////////////////////////////////
class PvmfAmrEncNode : public OsclTimerObject,
            public PVMFNodeInterface,
            public PvmfAmrEncDataProcessorObserver,
            public PvmiCapabilityAndConfig
{
    public:
        PvmfAmrEncNode(int32 aPriority, PvmfAmrEncDataProcessorInterface* aDataProcessor);
        ~PvmfAmrEncNode();

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

        // Pure virtuals from PvmfAmrEncDataProcessorObserver
        void OutputMemoryAvailable() {};
        void ProcessComplete(PvmfAmrEncDataProcessResult aResult, PVMFSharedMediaDataPtr& aMediaDataOut);

        // Port activity processing routines
        bool IsProcessOutgoingMsgReady();
        bool IsProcessIncomingMsgReady();
        PVMFStatus ProcessIncomingMsg(PVMFPortInterface* aPort);

        PvmfAmrEncDataProcessorInterface* GetDataProcessor()
        {
            return iDataProcessor;
        }

        /////////////////////////////////////////////////////
        //     // cap config interface
        /////////////////////////////////////////////////////

        // implemetation of PvmiCapabilityAndConfig class functions here

        void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver);

        PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                                     PvmiKvp*& aParameters, int& num_parameter_elements,
                                     PvmiCapabilityContext aContext);
        PVMFStatus releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);
        void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                  PvmiKvp* aParameters, int num_parameter_elements);
        void DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                               int num_elements, PvmiKvp * & aRet_kvp);
        PVMFCommandId setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                         int num_elements, PvmiKvp*& aRet_kvp, OsclAny* context = NULL);
        uint32 getCapabilityMetric(PvmiMIOSession aSession);
        PVMFStatus verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);

        // function used in VerifyParametersSync n SetParametersSync of capability class
        PVMFStatus VerifyAndSetConfigParameter(PvmiKvp& aParameter, bool aSetParam);

        // function used in getParametersSync of capability class
        PVMFStatus GetConfigParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr);


    private:
        // From OsclTimerObject
        void Run();

        /////////////////////////////////////////////////////
        //     Command processing routines
        /////////////////////////////////////////////////////
        PVMFCommandId QueueCommandL(PvmfAmrEncNodeCommand& aCmd);
        bool ProcessCommand(PvmfAmrEncNodeCommand& aCmd);
        void CommandComplete(PvmfAmrEncNodeCmdQueue& aCmdQueue, PvmfAmrEncNodeCommand& aCmd,
                             PVMFStatus aStatus, OsclAny* aData = NULL);
        void DoQueryUuid(PvmfAmrEncNodeCommand& aCmd);
        void DoQueryInterface(PvmfAmrEncNodeCommand& aCmd);
        void DoRequestPort(PvmfAmrEncNodeCommand& aCmd);
        PvmfAmrEncPort* AllocatePort(PvmfAmrEncPortVector& aPortVector, int32 aTag, OSCL_String* aMimeType, const char* aName = NULL);
        void DoReleasePort(PvmfAmrEncNodeCommand& aCmd);
        void DoInit(PvmfAmrEncNodeCommand& aCmd);
        void DoPrepare(PvmfAmrEncNodeCommand& aCmd);
        void DoStart(PvmfAmrEncNodeCommand& aCmd);
        void DoStop(PvmfAmrEncNodeCommand& aCmd);
        void DeleteVideoEncoder();
        void DoFlush(PvmfAmrEncNodeCommand& aCmd);
        bool IsFlushPending();
        void FlushComplete();
        void DoPause(PvmfAmrEncNodeCommand& aCmd);
        void DoReset(PvmfAmrEncNodeCommand& aCmd);
        void DoCancelAllCommands(PvmfAmrEncNodeCommand& aCmd);
        void DoCancelCommand(PvmfAmrEncNodeCommand& aCmd);

        // Event reporting
        void ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData = NULL);
        void ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData = NULL);
        void SetState(TPVMFNodeInterfaceState aState);
        PVMFStatus SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg);

    private:

        // Allocators
        PvmfAmrEncNodeAllocDestructDealloc iAlloc;

        // Command queue
        PvmfAmrEncNodeCmdQueue iCmdQueue;

        // A queue is used to hold the current command so it's easy to find out
        // whether a command is in progress, and allow cancel to interrupt
        PvmfAmrEncNodeCmdQueue iCurrentCmd;

        // Ports and port activity
        PvmfAmrEncPortVector iInPort;
        PvmfAmrEncPortVector iOutPort;
        friend class PvmfAmrEncPort;

        // Encoder
        PvmfAmrEncDataProcessorInterface* iDataProcessor;

        // Logger
        PVLogger* iLogger;
};

#endif // PVMF_AMRENC_NODE_H_INCLUDED

