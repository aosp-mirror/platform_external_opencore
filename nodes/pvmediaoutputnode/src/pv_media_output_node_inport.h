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
/**
 *
 * @file pvmi_io_interface_node_inport.h
 * @brief Input port for media io interface wrapper node
 *
 */

#ifndef PV_MEDIA_OUTPUT_NODE_INPORT_H_INCLUDED
#define PV_MEDIA_OUTPUT_NODE_INPORT_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_PORT_BASE_IMPL_H_INCLUDED
#include "pvmf_port_base_impl.h"
#endif
#ifndef PVMI_MEDIA_TRANSFER_H_INCLUDED
#include "pvmi_media_transfer.h"
#endif
#ifndef PVMF_NODES_SYNC_CONTROL_H_INCLUDED
#include "pvmf_nodes_sync_control.h"
#endif
#ifndef PVMF_SYNC_UTIL_DATA_QUEUE_H_INCLUDED
#include "pvmf_sync_util_data_queue.h"
#endif
#ifndef PVMI_KVP_H_INCLUDED
#include "pvmi_kvp.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif

// Forward declaration
class PVMediaOutputNode;

enum PVMFMediaOutputNodePortMediaTimeStatus
{
    PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ERROR,
    PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ON_TIME,
    PVMF_MEDIAOUTPUTNODEPORT_MEDIA_LATE,
    PVMF_MEDIAOUTPUTNODEPORT_MEDIA_EARLY
};

#define THRESHOLD_FOR_DROPPED_VIDEO_FRAMES 120

/**
 * Observer class for the inactivity timer AO
 */
class PVMediaOutputNodePortTimerObserver
{
    public:
        virtual ~PVMediaOutputNodePortTimerObserver() {}
        /**
         * A timer event, indicating that the timer has expired.
         */
        virtual void PVMediaOutputNodePortTimerEvent() = 0;
};

/**
 * Timer used while pacing data for passive media output comps
 */
class PVMediaOutputNodePortTimer : public OsclTimerObject
{
    public:
        PVMediaOutputNodePortTimer(PVMediaOutputNodePortTimerObserver* aObserver);

        virtual ~PVMediaOutputNodePortTimer();

        /** Start Timer */
        PVMFStatus Start();

        PVMFStatus setTimerDurationInMS(uint32 duration);

        /** Stop Timer events */
        PVMFStatus Stop();

        bool IsTimerStarted()
        {
            return iStarted;
        }

    private:
        void Run();
        uint32 iDurationInMS;
        PVMediaOutputNodePortTimerObserver* iObserver;
        bool iStarted;
};


class PVMediaOutputNodePort : public OsclTimerObject
            , public PvmfPortBaseImpl
            , public PvmfNodesSyncControlInterface
            , public PvmiMediaTransfer
            , public PVMFPortActivityHandler
            , public PvmiCapabilityAndConfig
            , public PVMediaOutputNodePortTimerObserver
            , public OsclClockObserver
            , public OsclClockStateObserver
{
    public:
        PVMediaOutputNodePort(PVMediaOutputNode* aNode);
        ~PVMediaOutputNodePort();

        void NodeStarted();

        PVMFStatus Configure(OSCL_String&);
        void PutData(PVMFSharedMediaMsgPtr& aMsg);

        //these override the PvmfPortBaseImpl routines
        OSCL_IMPORT_REF PVMFStatus Connect(PVMFPortInterface* aPort);
        OSCL_IMPORT_REF PVMFStatus Disconnect();
        OSCL_IMPORT_REF PVMFStatus PeerConnect(PVMFPortInterface* aPort);
        OSCL_IMPORT_REF PVMFStatus PeerDisconnect();
        OSCL_IMPORT_REF PVMFStatus ClearMsgQueues();

        //from PVMFPortActivityHandler
        void HandlePortActivity(const PVMFPortActivity& aActivity);

        // Pure virtual from PVInterface
        void addRef();
        void removeRef();
        bool queryInterface(const PVUuid& uuid, PVInterface*& iface);

        // Pure virtuals from PvmfNodesSyncControlInterface
        PVMFStatus SetClock(OsclClock* aClock);
        PVMFStatus ChangeClockRate(int32 aRate);
        PVMFStatus SetMargins(int32 aEarlyMargin, int32 aLateMargin);
        void ClockStarted();
        void ClockStopped();
        PVMFCommandId SkipMediaData(int32,
                                    PVMFTimestamp aStartingTimestamp,
                                    PVMFTimestamp aResumeTimestamp,
                                    uint32 aStreamID,
                                    bool aRenderSkippedData = false,
                                    bool aPlayBackPositionContinuous = false,
                                    OsclAny* aContext = NULL)
        {
            OSCL_UNUSED_ARG(aStartingTimestamp);
            OSCL_UNUSED_ARG(aResumeTimestamp);
            OSCL_UNUSED_ARG(aStreamID);
            OSCL_UNUSED_ARG(aRenderSkippedData);
            OSCL_UNUSED_ARG(aPlayBackPositionContinuous);
            OSCL_UNUSED_ARG(aContext);
            OSCL_LEAVE(OsclErrNotSupported);
            return -1;
        }

        //active vs passive mio
        void EnableMediaSync();

        // Pure virtuals from PvmfSyncUtilDataQueueObserver
        void ScheduleProcessData(PvmfSyncUtilDataQueue* aDataQueue, uint32 aTimeMilliseconds);
        // sends and info event to engine when the skipping of data is complete
        void SkipMediaDataComplete();
        // after seeing bos for every skipmediadata command issues command complete to engine
        void SkipMediaCommandComplete();

        // Pure virtuals from PvmiMediaTransfer
        void setPeer(PvmiMediaTransfer *aPeer);
        void useMemoryAllocators(OsclMemAllocator* write_alloc = NULL);
        PVMFCommandId writeAsync(uint8 format_type, int32 format_index, uint8* data, uint32 data_len,
                                 const PvmiMediaXferHeader& data_header_info, OsclAny* aContext = NULL);
        void writeComplete(PVMFStatus aStatus, PVMFCommandId write_cmd_id, OsclAny* aContext);
        PVMFCommandId readAsync(uint8* data, uint32 max_data_len, OsclAny* aContext = NULL,
                                int32* formats = NULL, uint16 num_formats = 0);
        void readComplete(PVMFStatus aStatus, PVMFCommandId  read_cmd_id, int32 format_index,
                          const PvmiMediaXferHeader& data_header_info, OsclAny* aContext);
        void statusUpdate(uint32 status_flags);
        void cancelCommand(PVMFCommandId  command_id);
        void cancelAllCommands();

        // Implement pure virtuals from PvmiCapabilityAndConfig interface
        OSCL_IMPORT_REF virtual PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                PvmiKvp*& aParameters, int& num_parameter_elements,	PvmiCapabilityContext aContext);
        OSCL_IMPORT_REF virtual PVMFStatus releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);
        OSCL_IMPORT_REF virtual void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                int num_elements, PvmiKvp * & aRet_kvp);
        OSCL_IMPORT_REF virtual PVMFStatus verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);

        // Unsupported PvmiCapabilityAndConfig methods
        void virtual setObserver(PvmiConfigAndCapabilityCmdObserver*) {};
        void virtual createContext(PvmiMIOSession , PvmiCapabilityContext&) {};
        void virtual setContextParameters(PvmiMIOSession , PvmiCapabilityContext& , PvmiKvp* , int) {};
        void virtual DeleteContext(PvmiMIOSession , PvmiCapabilityContext&) {};
        PVMFCommandId virtual setParametersAsync(PvmiMIOSession , PvmiKvp* , int , PvmiKvp*& , OsclAny* context = NULL)
        {
            OSCL_UNUSED_ARG(context);
            return -1;
        }
        uint32 virtual getCapabilityMetric(PvmiMIOSession)
        {
            return 0;
        }

        // To support config interface
        void QueryInterface(const PVUuid &aUuid, OsclAny*&aPtr)
        {
            if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
            {
                aPtr = (PvmiCapabilityAndConfig*)this;
            }
            else
            {
                aPtr = NULL;
            }
        }

        //From OsclClockObserver
        void ClockTimebaseUpdated();
        void ClockCountUpdated();
        void ClockAdjusted();
        //From OsclClockStateObserver
        void ClockStateUpdated();

        //To allow the node to set the port format.
        PVMFFormatType iPortFormat;
        bool IsFormatSupported(PVMFFormatType);
        void FormatUpdated();

        //from PVMediaOutputNodePortTimerObserver
        void PVMediaOutputNodePortTimerEvent();

        void SetSkipTimeStamp(uint32 aSkipTS, uint32 aStreamID);
        void CancelSkip();

        bool iWaitForConfig;
        bool isUnCompressedMIO;

        //BOS related
        Oscl_Vector<uint32, OsclMemAllocator> iBOSStreamIDVec;
        void ClearPreviousBOSStreamIDs(uint32 aID);

    private:
        void Run();
        bool peekHead(PVMFSharedMediaMsgPtr& dataPtr, bool& bBos);
        PVMFStatus ConfigMIO(PvmiKvp* aParameters, PvmiKvp* &aRetParameters);
        PVMFStatus SetMIOParameterInt32(PvmiKeyType aKey, int32 aValue);
        PVMFStatus SetMIOParameterUint32(PvmiKeyType aKey, uint32 aValue);
        PVMFStatus SetMIOParameterPchar(PvmiKeyType aKey, char* aValue);

        // Container node
        PVMediaOutputNode* iNode;

        uint32 iExtensionRefCount;
        OSCL_HeapString<OsclMemAllocator> iSinkFormatString;
        PVMFFormatType iSinkFormat;

        //data transfer related
        PvmiMediaTransfer* iMediaTransfer;
        PVMFCommandId iMioInfoErrorCmdId;
        PvmfMediaTypeIndex iMediaTypeIndex;
        enum WriteState {EWriteBusy, EWriteWait, EWriteOK};
        WriteState iWriteState;
        //media data cleanup queue
        class CleanupQueueElement
        {
            public:
                CleanupQueueElement(PVMFSharedMediaDataPtr d, PVMFCommandId id): iData(d), iCmdId(id) {}
                CleanupQueueElement(PVMFCommandId id): iCmdId(id) {}
                PVMFSharedMediaDataPtr iData;
                PVMFCommandId iCmdId;
        };
        Oscl_Vector<CleanupQueueElement, OsclMemAllocator> iCleanupQueue;
        uint32 iWriteAsyncContext;
        uint32 iWriteAsyncEOSContext;
        uint32 iWriteAsyncReConfigContext;
        OsclClock* iClock;
        int32 iClockRate;
        uint32 iEarlyMargin;
        uint32 iLateMargin;
        PVMediaOutputNodePortTimer* iDelayTimer;
        bool oActiveMediaOutputComp;
        bool oProcessIncomingMessage;
        uint32 iConsecutiveFramesDropped;
        bool iLateFrameEventSent;
        PVMFSharedMediaMsgPtr iCurrentMediaMsg;
        uint32 iFragIndex;
        //for sending any data
        void SendData();
        //for sending media data to the Mout.
        void SendMediaData();
        //for sending the end-of-data notice to the Mout.
        void SendEndOfData();
        //for sending reconfig notice to the Mout
        void SendReConfigNotification();
        void ClearCleanupQueue();
        void CleanupMediaTransfer();

        //skip related
        PVMFMediaOutputNodePortMediaTimeStatus CheckMediaTimeStamp(uint32& aDelta);
        PVMFMediaOutputNodePortMediaTimeStatus CheckMediaFrameStep();
        uint32 iRecentStreamID;
        uint32 iEOSStreamId;
        uint32 iSkipTimestamp;
        bool iSendStartOfDataEvent;
        bool DataToSkip(PVMFSharedMediaMsgPtr& aMsg);

        //frame step related
        bool iFrameStepMode;
        int64 iClockFrameCount;
        int64 iSyncFrameCount;

        //for datapath logging
        void LogMediaDataInfo(const char* msg, PVMFSharedMediaDataPtr mediaData, int32 p1, int32 p2);
        void LogMediaDataInfo(const char* msg, PVMFSharedMediaDataPtr mediaData);
        void LogDatapath(const char* msg);

        OsclErrorTrapImp* iOsclErrorTrapImp;
        PVLogger* iLogger;
        PVLogger* iDatapathLogger;
        PVLogger* iDatapathLoggerIn;
        PVLogger* iDatapathLoggerOut;
        PVLogger* iReposLogger;

};

#endif // PVMI_IO_INTERFACE_NODE_INPORT_H_INCLUDED
