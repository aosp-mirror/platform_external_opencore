/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#ifndef PVMF_JITTER_BUFFER_NODE_H_INCLUDED
#define PVMF_JITTER_BUFFER_NODE_H_INCLUDED

#ifndef OSCL_EXCLUSIVE_PTR_H_INCLUDED
#include "oscl_exclusive_ptr.h"
#endif
#ifndef PVMF_MEDIA_CLOCK_H_INCLUDED
#include "pvmf_media_clock.h"
#endif
#ifndef OSCL_TIMER_H_INCLUDED
#include "oscl_timer.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifdef PVMF_NODE_UTILS_H_INCLUDED
#include "pvmf_node_utils.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef PVMF_JITTER_BUFFER_PORT_H_INCLUDED
#include "pvmf_jitter_buffer_port.h"
#endif
#ifndef PVMF_JITTER_BUFFER_EXT_INTERFACE_H_INCLUDED
#include "pvmf_jitter_buffer_ext_interface.h"
#endif
#ifndef PVMF_JITTER_BUFFER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif
#ifndef PVMF_RTCP_TIMER_H_INCLUDED
#include "pvmf_rtcp_timer.h"
#endif
#ifndef PVMF_SM_NODE_EVENTS_H_INCLUDED
#include "pvmf_sm_node_events.h"
#endif
#ifndef PVMF_JB_INACTIVITY_TIMER_H_INCLUDED
#include "pvmf_jb_inactivity_timer.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

///////////////////////////////////////////////
//IDs for all of the asynchronous node commands.
///////////////////////////////////////////////
enum TPVMFJitterBufferNodeCommand
{
    PVMF_JITTER_BUFFER_NODE_QUERYUUID,
    PVMF_JITTER_BUFFER_NODE_QUERYINTERFACE,
    PVMF_JITTER_BUFFER_NODE_REQUESTPORT,
    PVMF_JITTER_BUFFER_NODE_RELEASEPORT,
    PVMF_JITTER_BUFFER_NODE_INIT,
    PVMF_JITTER_BUFFER_NODE_PREPARE,
    PVMF_JITTER_BUFFER_NODE_START,
    PVMF_JITTER_BUFFER_NODE_STOP,
    PVMF_JITTER_BUFFER_NODE_FLUSH,
    PVMF_JITTER_BUFFER_NODE_PAUSE,
    PVMF_JITTER_BUFFER_NODE_RESET,
    PVMF_JITTER_BUFFER_NODE_CANCELALLCOMMANDS,
    PVMF_JITTER_BUFFER_NODE_CANCELCOMMAND,
    /* add jitter buffer node specific commands here */
    PVMF_JITTER_BUFFER_NODE_COMMAND_LAST
};

enum PVMFStreamType
{
    PVMF_STREAM_TYPE_UNKNOWN = 0,
    PVMF_STREAM_TYPE_AUDIO = 1,
    PVMF_STREAM_TYPE_VIDEO = 2,

    //NUM_PVMF_STREAM_TYPES must always be at the end.
    NUM_PVMF_STREAM_TYPES
};

class PVLogger;

class PVMFMediaClockNotificationsIntfContext
{
    public:
        PVMFMediaClockNotificationsIntfContext(): iClockNotificationIntf(NULL), iContext(NULL) {}
        void SetContext(PVMFMediaClockNotificationsInterface* aClockNotificationsInterface, OsclAny* aContext)
        {
            iClockNotificationIntf = aClockNotificationsInterface;
            iContext = aContext;
        }
        const OsclAny* GetContextData() const
        {
            return iContext;
        }

        const PVMFMediaClockNotificationsInterface* GetMediaClockNotificationsInterface() const
        {
            return  iClockNotificationIntf;
        }

    private:
        PVMFMediaClockNotificationsInterface* iClockNotificationIntf;
        OsclAny* iContext;
};

class PVMFJitterBufferNode : public PVInterface,
            public PVMFNodeInterface,
            public OsclActiveObject,
            public PVMFNodeErrorEventObserver,
            public PVMFNodeInfoEventObserver,
            public PVMFNodeCmdStatusObserver,
            public PvmfRtcpTimerObserver,
            public PvmfJBSessionDurationTimerObserver,
            public PVMFJitterBufferObserver,
            public PVMFMediaClockStateObserver,
            public OsclMemPoolResizableAllocatorMemoryObserver,
            public PVMFMediaClockNotificationsObs
{
    public:
        OSCL_IMPORT_REF PVMFJitterBufferNode(int32 aPriority);
        OSCL_IMPORT_REF virtual ~PVMFJitterBufferNode();

        OSCL_IMPORT_REF PVMFStatus ThreadLogon();
        OSCL_IMPORT_REF PVMFStatus ThreadLogoff();
        OSCL_IMPORT_REF PVMFStatus GetCapability(PVMFNodeCapability& aNodeCapability);
        OSCL_IMPORT_REF PVMFPortIter* GetPorts(const PVMFPortFilter* aFilter = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryUUID(PVMFSessionId,
                                                const PvmfMimeString& aMimeType,
                                                Oscl_Vector< PVUuid, PVMFJitterBufferNodeAllocator >& aUuids,
                                                bool aExactUuidsOnly = false,
                                                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryInterface(PVMFSessionId,
                const PVUuid& aUuid,
                PVInterface*& aInterfacePtr,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId RequestPort(PVMFSessionId,
                int32 aPortTag,
                const PvmfMimeString* aPortConfig = NULL,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId ReleasePort(PVMFSessionId,
                PVMFPortInterface& aPort,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Init(PVMFSessionId,
                                           const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Prepare(PVMFSessionId aSession,
                                              const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Start(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Stop(PVMFSessionId,
                                           const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Flush(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Pause(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Reset(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelAllCommands(PVMFSessionId,
                const OsclAny* aContextData = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelCommand(PVMFSessionId,
                PVMFCommandId aCmdId,
                const OsclAny* aContextData = NULL);

        /**
         * Handle an error event that has been generated.
         *
         * @param "aEvent" "The event to be handled."
         */
        virtual void HandleNodeErrorEvent(const PVMFAsyncEvent& aEvent)
        {
            OSCL_UNUSED_ARG(aEvent);
        }
        /**
         * Handle an informational event that has been generated.
         *
         * @param "aEvent" "The event to be handled."
         */
        virtual void HandleNodeInformationalEvent(const PVMFAsyncEvent& aEvent)
        {
            OSCL_UNUSED_ARG(aEvent);
        }
        /**
         * Handle an event that has been generated.
         *
         * @param "aResponse"	"The response to a previously issued command."
         */
        virtual void NodeCommandCompleted(const PVMFCmdResp& aResponse)
        {
            OSCL_UNUSED_ARG(aResponse);
        }

        //from PVMFPortActivityHandler
        void HandlePortActivity(const PVMFPortActivity& aActivity);
        //from PVMFMediaClockStateObserver
        virtual void ClockStateUpdated();
        // Need to set jb state to ready when handing 404, 415 response
        virtual void UpdateJitterBufferState();
        void NotificationsInterfaceDestroyed();

        virtual void addRef()
        {
        }
        virtual void removeRef()
        {
        }
        virtual bool queryInterface(const PVUuid& uuid, PVInterface*& iface)
        {
            iface = NULL;
            if (uuid == PVUuid(PVMF_JITTERBUFFERNODE_EXTENSIONINTERFACE_UUID))
            {
                if (!iExtensionInterface)
                {
                    PVMFJitterBufferNodeAllocator alloc;
                    int32 err;
                    OsclAny*ptr = NULL;
                    OSCL_TRY(err,
                             ptr = alloc.ALLOCATE(sizeof(PVMFJitterBufferExtensionInterfaceImpl));
                            );
                    if (err != OsclErrNone || !ptr)
                    {
                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::queryInterface: Error - Out of memory"));
                        OSCL_LEAVE(OsclErrNoMemory);
                    }
                    iExtensionInterface =
                        OSCL_PLACEMENT_NEW(ptr, PVMFJitterBufferExtensionInterfaceImpl(this));
                }
                return (iExtensionInterface->queryInterface(uuid, iface));
            }
            else
            {
                return false;
            }
        }
        virtual bool PrepareForPlaylistSwitch();

    private:
        bool RTCPByeRcvd();
        //from OsclActiveObject
        void Run();
        void DoCancel();

        /**
         * Process a port activity. This method is called by Run to process a port activity.
         *
         */
        bool ProcessPortActivity(PVMFJitterBufferPortParams*);

        /**
         * Retrieve and process an incoming message from a port.
         */
        PVMFStatus ProcessIncomingMsg(PVMFJitterBufferPortParams*);
        PVMFStatus ProcessIncomingMsgRTP(PVMFJitterBufferPortParams*,
                                         PVMFSharedMediaMsgPtr&);

        /**
         * Process an outgoing message of a the specified port by sending the message to
         * the receiving side.
         */
        PVMFStatus ProcessOutgoingMsg(PVMFJitterBufferPortParams*);

        void QueuePortActivity(PVMFJitterBufferPortParams*,
                               const PVMFPortActivity&);

        bool CheckForPortRescheduling();
        bool CheckForPortActivityQueues();

        /**
         * Queue holding port activity. Only incoming and outgoing msg activity are
         * put on the queue.  For each port, there should only be at most one activity
         * of each type on the queue.
         */
        Oscl_Vector<PVMFPortActivity, PVMFJitterBufferNodeAllocator> iPortActivityQueue;

        PVMFPortVector<PVMFJitterBufferPort, PVMFJitterBufferNodeAllocator> iPortVector;

        /*
         * Queue holding port params - one per every port
         */
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator> iPortParamsQueue;

        bool getPortContainer(PVMFPortInterface* aPort,
                              PVMFJitterBufferPortParams*& aPortParams);

        PVMFJitterBufferNodeCmdQ iInputCommands;
        PVMFJitterBufferNodeCmdQ iCurrentCommand;
        PVMFNodeCapability iCapability;
        PVLogger *iLogger;
        PVLogger *iDataPathLogger;
        PVLogger *iDataPathLoggerIn;
        PVLogger *iDataPathLoggerOut;
        PVLogger *iDataPathLoggerFlowCtrl;
        PVLogger *iDataPathLoggerRTCP;
        PVLogger *iDataPathLoggerFireWall;
        PVLogger *iClockLogger;
        PVLogger *iClockLoggerSessionDuration;
        PVLogger *iClockLoggerRebuff;
        PVLogger *iJBEventsClockLogger;
        PVLogger *iRTCPAVSyncLogger;
        uint32 iStreamID;

        PVMFJitterBufferExtensionInterfaceImpl *iExtensionInterface;

        /*
         * Jitter buffer related params
         */
        PVMFMediaClock *iClientPlayBackClock;
        PVMFMediaClock *iEstimatedServerClock;
        PVMFTimebase_Tickcount iEstimatedServerClockTimeBase;

        uint32    iJitterBufferUnderFlowThresholdInMilliSeconds;
        uint32    iPlayBackThresholdInMilliSeconds;
        uint32    iEstimatedServerKeepAheadInMilliSeconds;
        bool      oDelayEstablished;
        bool      iOverflowFlag;

        PVMFJBCommandContext iInternalCmdPool[PVMF_JITTER_BUFFER_INTERNAL_CMDQ_SIZE];

        /** Command processing */
        PVMFCommandId QueueCommandL(PVMFJitterBufferNodeCommand&);
        void MoveCmdToCurrentQueue(PVMFJitterBufferNodeCommand& aCmd);
        bool ProcessCommand(PVMFJitterBufferNodeCommand&);
        void CommandComplete(PVMFJitterBufferNodeCmdQ&,
                             PVMFJitterBufferNodeCommand&,
                             PVMFStatus,
                             OsclAny* aData = NULL,
                             PVUuid* aEventUUID = NULL,
                             int32* aEventCode = NULL);
        void CommandComplete(PVMFJitterBufferNodeCommand&,
                             PVMFStatus,
                             OsclAny* aData = NULL,
                             PVUuid* aEventUUID = NULL,
                             int32* aEventCode = NULL);
        void InternalCommandComplete(PVMFJitterBufferNodeCmdQ&,
                                     PVMFJitterBufferNodeCommand&,
                                     PVMFStatus aStatus,
                                     OsclAny* aEventData = NULL);
        PVMFJBCommandContext* RequestNewInternalCmd();

        void DoQueryUuid(PVMFJitterBufferNodeCommand&);
        void DoQueryInterface(PVMFJitterBufferNodeCommand&);
        void DoRequestPort(PVMFJitterBufferNodeCommand&);
        void DoReleasePort(PVMFJitterBufferNodeCommand&);
        void DoInit(PVMFJitterBufferNodeCommand&);
        void DoPrepare(PVMFJitterBufferNodeCommand& aCmd);
        void CompletePrepare();
        void CancelPrepare();
        void DoStart(PVMFJitterBufferNodeCommand&);
        void CompleteStart();
        void CancelStart();
        bool oStartPending;
        bool iPlayingAfterSeek;

        void DoStop(PVMFJitterBufferNodeCommand&);
        void DoFlush(PVMFJitterBufferNodeCommand&);
        bool FlushPending();
        uint32 iPauseTime;
        void DoPause(PVMFJitterBufferNodeCommand&);
        void DoReset(PVMFJitterBufferNodeCommand&);
        void DoCancelAllCommands(PVMFJitterBufferNodeCommand&);
        void DoCancelCommand(PVMFJitterBufferNodeCommand&);

        void ReportErrorEvent(PVMFEventType aEventType,
                              OsclAny* aEventData = NULL,
                              PVUuid* aEventUUID = NULL,
                              int32* aEventCode = NULL);
        void ReportInfoEvent(PVMFEventType aEventType,
                             OsclAny* aEventData = NULL,
                             PVUuid* aEventUUID = NULL,
                             int32* aEventCode = NULL);
        void SetState(TPVMFNodeInterfaceState);

        void CleanUp();
        void setClientPlayBackClock(PVMFMediaClock* clientClock)
        {
            //remove ourself as observer of old clock, if any.
            if (iClientPlayBackClockNotificationsInf && iClientPlayBackClock)
            {
                iClientPlayBackClockNotificationsInf->RemoveClockStateObserver(*this);
                iClientPlayBackClock->DestroyMediaClockNotificationsInterface(iClientPlayBackClockNotificationsInf);
                iClientPlayBackClockNotificationsInf = NULL;
            }

            iClientPlayBackClock = clientClock;
            if (iClientPlayBackClock)
            {
                PVMFStatus status = iClientPlayBackClock->ConstructMediaClockNotificationsInterface(iClientPlayBackClockNotificationsInf, *this);
                if (PVMFSuccess != status || !iClientPlayBackClockNotificationsInf)
                {
                    iClientPlayBackClockNotificationsInf = NULL;
                    OSCL_ASSERT(false);
                }
            }

            //set ourself as observer of new clock.
            if (iClientPlayBackClockNotificationsInf)
                iClientPlayBackClockNotificationsInf->SetClockStateObserver(*this);
        }

        void setMaxInactivityDurationForMediaInMs(uint32 duration)
        {
            iMaxInactivityDurationForMediaInMs = duration;
        }

        void getMaxInactivityDurationForMediaInMs(uint32& duration)
        {
            duration = iMaxInactivityDurationForMediaInMs;
        }

        friend class PVMFJitterBufferPort;
        friend class PVMFJitterBufferExtensionInterfaceImpl;

        /*
         * Jitter buffer related methods
         */
        bool CheckStateForRegisteringRTPPackets();
        PVMFJitterBufferDataState iJitterBufferState;
        void StopOutputPorts()
        {
            oStopOutputPorts = true;
        }
        void StartOutputPorts()
        {
            oStopOutputPorts = false;
        }
        bool PrepareForRepositioning(bool oUseExpectedClientClockVal = false,
                                     uint32 aExpectedClientClockVal = 0);
        bool ClearJitterBuffer(PVMFPortInterface* aPort, uint32 aSeqNum);
        void FlushJitterBuffer();
        bool pauseEstimatedServerClock()
        {
            if (iEstimatedServerClock)
            {
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::pauseEstimatedServerClock"));
                iEstimatedServerClock->Pause();
            }
            StopOutputPorts();
            return true;
        }
        bool StartEstimatedServerClock()
        {
            if (iEstimatedServerClock)
            {
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::StartEstimatedServerClock"));
                iEstimatedServerClock->Start();
            }
            RequestEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
            StartOutputPorts();
            return true;
        }
        bool NotifyAutoPauseComplete()
        {
            if (iEstimatedServerClock)
            {
                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJitterBufferNode::NotifyAutoPauseComplete"));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::NotifyAutoPauseComplete"));
                iEstimatedServerClock->Pause();
            }
            return true;
        }
        bool NotifyAutoResumeComplete()
        {
            if (iEstimatedServerClock)
            {
                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJitterBufferNode::NotifyAutoResumeComplete"));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::NotifyAutoResumeComplete"));
                iEstimatedServerClock->Start();
                /* NotifyAutoResumeComplete() is only called in case of HTTP streaming,
                   therefore EstimatedServerClock has to be paused just after it is started */
                iEstimatedServerClock->Pause();
            }
            return true;
        }
        bool oAutoPause;

        bool setPortParams(PVMFPortInterface* aPort,
                           uint32 aTimeScale,
                           uint32 aBitRate,
                           OsclRefCounterMemFrag& aConfig,
                           bool aRateAdaptation = false,
                           uint32 aRateAdaptationFeedBackFrequency = 0);
        bool setPortParams(PVMFPortInterface* aPort,
                           uint32 aTimeScale,
                           uint32 aBitRate,
                           OsclRefCounterMemFrag& aConfig,
                           bool aRateAdaptation,
                           uint32 aRateAdaptationFeedBackFrequency,
                           uint aMaxNumBuffResizes, uint aBuffResizeSize);
        bool setPortParams(PVMFPortInterface* aPort,
                           uint32 aTimeScale,
                           uint32 aBitRate,
                           OsclRefCounterMemFrag& aConfig,
                           bool aRateAdaptation,
                           uint32 aRateAdaptationFeedBackFrequency,
                           bool aUserSpecifiedBuffParams,
                           uint aMaxNumBuffResizes = 0, uint aBuffResizeSize = 0);
        bool setPortSSRC(PVMFPortInterface* aPort, uint32 aSSRC);
        bool setPortRTPParams(PVMFPortInterface* aPort,
                              bool   aSeqNumBasePresent,
                              uint32 aSeqNumBase,
                              bool   aRTPTimeBasePresent,
                              uint32 aRTPTimeBase,
                              bool   aNPTTimeBasePresent,
                              uint32 aNPTInMS,
                              bool oPlayAfterASeek = false);
        bool setPortRTCPParams(PVMFPortInterface* aPort,
                               int aNumSenders,
                               uint32 aRR,
                               uint32 aRS);
        void ActivateTimer(PVMFJitterBufferPortParams* pPort);

        PVMFStatus setServerInfo(PVMFJitterBufferFireWallPacketInfo& aServerInfo);
        PVMFStatus SetTransportType(PVMFPortInterface* aPort,
                                    OSCL_String& aTransportType);
        bool PurgeElementsWithNPTLessThan(NptTimeFormat &aNPTTime);

        void setPlayBackThresholdInMilliSeconds(uint32 threshold)
        {
            iPlayBackThresholdInMilliSeconds = threshold;
        }
        void setJitterBufferRebufferingThresholdInMilliSeconds(uint32 aThreshold)
        {
            if (aThreshold < iJitterBufferDurationInMilliSeconds)
            {
                iJitterBufferUnderFlowThresholdInMilliSeconds = aThreshold;
            }
        }
        void getJitterBufferRebufferingThresholdInMilliSeconds(uint32& aThreshold)
        {
            aThreshold = iJitterBufferUnderFlowThresholdInMilliSeconds;
        }
        void setJitterBufferDurationInMilliSeconds(uint32 duration)
        {
            iJitterBufferDurationInMilliSeconds = duration;
        }
        void getJitterBufferDurationInMilliSeconds(uint32& duration)
        {
            duration = iJitterBufferDurationInMilliSeconds;
        }

        PVMFStatus HasSessionDurationExpired(bool& aExpired)
        {
            aExpired = false;
            if (oSessionDurationExpired)
            {
                aExpired = true;
                return PVMFSuccess;
            }

            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK == it->tag)
                {
                    if (it->iRTCPStats.oRTCPByeRecvd)
                    {
                        aExpired = true;
                    }
                    else
                    {
                        aExpired = false;
                        return PVMFSuccess;
                    }
                }
            }
            oSessionDurationExpired = aExpired;
            return PVMFSuccess;
        }


        void SetSharedBufferResizeParams(uint32 maxNumResizes, uint32 resizeSize);
        void GetSharedBufferResizeParams(uint32& maxNumResizes, uint32& resizeSize);

        void SetJitterBufferMemPoolInfo(const PvmfPortBaseImpl* aPort, uint32 aSize, uint32 aResizeSize, uint32 aMaxNumResizes, uint32 aExpectedNumberOfBlocksPerBuffer);
        void GetJitterBufferMemPoolInfo(const PvmfPortBaseImpl* aPort, uint32& aSize, uint32& aResizeSize, uint32& aMaxNumResizes, uint32& aExpectedNumberOfBlocksPerBuffer) const;

        PVMFPortInterface* getPortCounterpart(PVMFPortInterface* aPort);
        bool CheckForSpaceInJitterBuffer(PVMFPortInterface* aPort);
        PVMFStatus CheckForHighWaterMark(PVMFPortInterface* aPort, bool& aHighWaterMarkReached);
        PVMFStatus CheckForLowWaterMark(PVMFPortInterface* aPort, bool& aLowWaterMarkReached);
        PVMFStatus RequestMemCallBackForAutoResume(PVMFPortInterface* aPort);
        void freeblockavailable(OsclAny* aContextData);
        void freememoryavailable(OsclAny* aContextData);
        void chunkdeallocated(OsclAny* aContextData);
        bool RegisterDataPacket(PVMFPortInterface* aPort,
                                PVMFJitterBuffer* aJitterBuffer,
                                PVMFSharedMediaDataPtr& aDataPacket);
        PVMFJitterBuffer* findJitterBuffer(PVMFPortInterface* aPort);
        PVMFPortInterface* findPortForJitterBuffer(PVMFJitterBuffer*);
        bool IsJitterBufferReady(PVMFJitterBufferPortParams*, uint32& aClockDiff);
        PVMFStatus SendData(PVMFPortInterface*);
        PVMFStatus CheckJitterBufferEmpty(bool& oEmpty);
        void JitterBufferFreeSpaceAvailable(OsclAny* aContext);
        void EstimatedServerClockUpdated(OsclAny* aContext);
        bool setPlayRange(int32 aStartTimeInMS,
                          int32 aStopTimeInMS,
                          bool oPlayAfterASeek,
                          bool aStopTimeAvailable = true);
        int32 iPlayStartTimeInMS;
        int32 iPlayStopTimeInMS;
        bool  iPlayStopTimeAvailable;
        uint32 iJitterDelayPercent;
        bool  oSessionDurationExpired;
        bool  oStopOutputPorts;
        PVMFTimestamp getActualMediaDataTSAfterSeek();
        PVMFTimestamp getMaxMediaDataTS();
        PVMFStatus CheckForEOS();
        PVMFStatus GenerateAndSendEOSCommand(PVMFPortInterface* aPort);
        PVMFStatus NotifyOutOfBandEOS();
        PVMFStatus SendBOSMessage(uint32 aStramID);
        bool QueueBOSCommand(PVMFPortInterface* aPort);

        PVMFStatus SetInPlaceProcessingMode(PVMFPortInterface* aPort,
                                            bool aInPlaceProcessing)
        {
            PVMFJitterBufferPortParams* portParamsPtr = NULL;
            if (!getPortContainer(aPort, portParamsPtr))
            {
                return PVMFFailure;
            }
            portParamsPtr->oInPlaceProcessing = aInPlaceProcessing;
            if (portParamsPtr->iJitterBuffer != NULL)
            {
                portParamsPtr->iJitterBuffer->SetInPlaceProcessingMode(aInPlaceProcessing);
            }
            return PVMFSuccess;
        }

        /*
         * RTCP timer related methods
         */
        uint32 iRTCPIntervalInMicroSeconds;
        void setRTCPIntervalInMicroSecs(uint32 aRTCPInterval);
        OsclRand iRandGen;
        void RtcpTimerEvent(PvmfRtcpTimer* pTimer);
        uint32 CalcRtcpInterval(PVMFJitterBufferPortParams* pFeedbackPort);
        PVMFStatus ProcessIncomingRTCPReport(PVMFSharedMediaMsgPtr&,
                                             PVMFJitterBufferPortParams*);

        PVMFStatus GenerateRTCPRR(PVMFJitterBufferPortParams* pPort);

        bool LocateFeedBackPort(PVMFJitterBufferPortParams*&,
                                PVMFJitterBufferPortParams*&);

        bool LocateInputPortForFeedBackPort(PVMFJitterBufferPortParams*&,
                                            PVMFJitterBufferPortParams*&);

        PVMFStatus ComposeAndSendFeedBackPacket(PVMFJitterBufferPortParams*&,
                                                PVMFJitterBufferPortParams*&);

        PVMFStatus ComposeAndSendRateAdaptationFeedBackPacket(PVMFJitterBufferPortParams*&,
                PVMFJitterBufferPortParams*&);

        PVMFMediaClock *iRTCPClock;
        PVMFTimebase_Tickcount iRTCPClockTimeBase;

        /*
         * Session Duration timer related
         */
        bool iUseSessionDurationTimerForEOS;
        PvmfJBSessionDurationTimer* iSessionDurationTimer;
        uint32 iSessionDurationInMS;
        void PVMFJBSessionDurationTimerEvent();
        void ComputeCurrentSessionDurationMonitoringInterval();

        /* firewall packet related */
        void DisableFireWallPackets()
        {
            iDisableFireWallPackets = true;
        };

        PVMFStatus CreateFireWallPacketMemAllocators(PVMFJitterBufferPortParams*);
        PVMFStatus DestroyFireWallPacketMemAllocators(PVMFJitterBufferPortParams*);

        PVMFJitterBufferFireWallPacketInfo iFireWallPacketInfo;
        bool iDisableFireWallPackets;

        PVMFStatus SendFireWallPackets();

        bool Allocate(PVMFJitterBufferPortParams* it, PVMFSharedMediaDataPtr& fireWallPkt, OsclSharedPtr<PVMFMediaDataImpl>& mediaDataImpl, const int size);

        PVMFStatus ResetFireWallPacketInfoAndResend();
        void CheckForFireWallRecv(bool &aComplete);
        void CheckForFireWallPacketAttempts(bool &aComplete);
        PVMFStatus DecodeFireWallPackets(PVMFSharedMediaDataPtr aPacket,
                                         PVMFJitterBufferPortParams* aPortParamsPtr);

        /* Bitstream thinning releated */
        void UpdateRebufferingStats(PVMFEventType aEventType);
        uint32 iNumUnderFlow;
        uint32 iUnderFlowStartByte;
        uint32 iUnderFlowEndByte;
        PVMFTimestamp iUnderFlowEndPacketTS;
        uint32 iDataDurationAccumulatedWhileBuffering;
        uint32 iEstimatedBitRate;
        uint32 iStartBufferingTickCount;
        uint32 iThinningIntervalMS;
        uint32 iPrevBufferingStartPacketTs;

        /* Diagnostic log related */
        uint32 iNumRunL;

        PVLogger* iDiagnosticsLogger;
        bool iDiagnosticsLogged;
        void LogSessionDiagnostics();
        void LogPortDiagnostics();

        /* resizable reallocator configuration */
        uint32 iJitterBufferSz;
        uint iMaxNumBufferResizes;
        uint iBufferResizeSize;

        void SetJitterBufferSize(uint32 aBufferSz);
        void GetJitterBufferSize(uint32& aBufferSz) const;
        void SetJitterBufferChunkAllocator(OsclMemPoolResizableAllocator* aDataBufferAllocator, const PVMFPortInterface* aPort);

        // Broadcast related
        inline void SetBroadCastSession()
        {
            iBroadCastSession = true;
        };

        bool ProcessRTCPSRforAVSync();
        bool iRTCPBcastAVSyncProcessed;
        bool iBroadCastSession;

        /*
        "hasAudioRTCP" and "hasVideoRTCP" are fundamentally different than
        PVMFJitterBufferPortParams::iRTCPStats.oSRRecvd because the former
        indicates if the port exists at all, and the latter indicate if
        sender reports were received.  This distinction is important when the
        jitter buffer is required to wait for the first RTCP sender reports
        before returning data.
        */
        bool hasAudioRTCP;
        bool hasVideoRTCP;
        bool gotRTCPReports;
        uint32 prevMinPercentOccupancy;
        uint32 consecutiveLowBufferCount;

        //These arrays map a steamID (VID or AUD) to an RTCP or RTP port index.
        uint32 RTCPPortIndex [NUM_PVMF_STREAM_TYPES];
        uint32 InputPortIndex[NUM_PVMF_STREAM_TYPES];
        ////////////
        PVMFMediaClock * iNonDecreasingClock;
        PVMFMediaClockNotificationsInterface *iNonDecreasingClockNotificationsInf;

        PVMFMediaClockNotificationsInterface *iClientPlayBackClockNotificationsInf;
        PVMFMediaClockNotificationsInterface *iEstimatedClockNotificationsInf;


        //Request callback contexts
        PVMFMediaClockNotificationsIntfContext	iNonDecClkNotificationsInfContext;
        PVMFMediaClockNotificationsIntfContext	iClientPlayBkClkNotificationsInfContext;
        PVMFMediaClockNotificationsIntfContext	iEstimatedClockNotificationsInfContext;

        PVMFTimebase_Tickcount iNonDecreasingClockTimeBase;

        uint32 iMaxInactivityDurationForMediaInMs;
        uint32 iIncomingMediaInactivityDurationCallBkId;
        bool   iIncomingMediaInactivityDurationCallBkPending;

        uint32 iBufferingStatusIntervalInMs;
        uint32 iNotifyBufferingStatusCallBkId;
        bool   iNotifyBufferingStatusCallBkPending;

        uint32 iJitterBufferDurationInMilliSeconds;
        uint32 iJitterBufferDurationCallBkId;
        bool   iJitterBufferDurationCallBkPending;

        uint32 iMonitorReBufferingCallBkId;
        bool   iMonitorReBufferingCallBkPending;

        uint32 iSendFirewallPacketCallBkId;
        bool   iSendFirewallPacketCallBkPending;

        enum JB_NOTIFY_CALLBACK
        {
            JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED,
            JB_NOTIFY_REPORT_BUFFERING_STATUS,
            JB_BUFFERING_DURATION_COMPLETE,
            JB_MONITOR_REBUFFERING,
            JB_NOTIFY_SEND_FIREWALL_PACKET,
            JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE  //OOO->Out Of Order
        };

        bool RequestEventCallBack(JB_NOTIFY_CALLBACK aEventType, uint32 aDelay = 0, OsclAny* aContext = NULL);
        bool RequestCallBack(PVMFMediaClockNotificationsInterface *& aEventNotificationInterface, uint32 aDelay, uint32& aCallBkId, bool& aCallBackStatusPending, OsclAny* aContext = NULL);
        void CancelEventCallBack(JB_NOTIFY_CALLBACK aEventType, OsclAny* aContext = NULL);
        void CancelCallBack(PVMFMediaClockNotificationsInterface *& aEventNotificationInterface, uint32& aCallBkId, bool& aCallBackStatusPending);
        bool IsCallbackPending(JB_NOTIFY_CALLBACK aEventType, OsclAny* aContext = NULL);
        void ProcessCallBack(uint32, PVTimeComparisonUtils::MediaTimeStatus, uint32, const OsclAny*, PVMFStatus);

        void HandleEvent_IncomingMediaInactivityDurationExpired();
        void HandleEvent_NotifyReportBufferingStatus();
        void HandleEvent_JitterBufferBufferingDurationComplete();
        void HandleEvent_MonitorReBuffering();
        void HandleEvent_NotifySendFirewallPacket();
        void HandleEvent_NotifyWaitForOOOPacketComplete(OsclAny* aContext);
};
#endif


