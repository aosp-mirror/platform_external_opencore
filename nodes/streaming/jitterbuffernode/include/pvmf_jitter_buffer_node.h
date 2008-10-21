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
#ifndef PVMF_JITTER_BUFFER_NODE_H_INCLUDED
#define PVMF_JITTER_BUFFER_NODE_H_INCLUDED

#ifndef OSCL_EXCLUSIVE_PTR_H_INCLUDED
#include "oscl_exclusive_ptr.h"
#endif
#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
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

class PVMFJitterBufferNode : public PVMFNodeInterface,
            public OsclActiveObject,
            public PVMFNodeErrorEventObserver,
            public PVMFNodeInfoEventObserver,
            public PVMFNodeCmdStatusObserver,
            public PvmfRtcpTimerObserver,
            public PvmfJBInactivityTimerObserver,
            public PvmfJBSessionDurationTimerObserver,
            public PvmfJBJitterBufferDurationTimerObserver,
            public OsclTimerObserver,
            public PVMFJitterBufferObserver,
            public PvmfFirewallPacketTimerObserver,
            public OsclMemPoolFixedChunkAllocatorObserver,
            public OsclClockStateObserver,
            public PVMFSMSharedBufferAllocWithReSizeAllocDeallocObserver
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
        //from OsclClockStateObserver
        virtual void ClockStateUpdated();
    private:
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
        uint32 iStreamID;

        PVMFJitterBufferExtensionInterfaceImpl *iExtensionInterface;

        /*
         * Jitter buffer related params
         */
        OsclClock *iClientPlayBackClock;
        OsclClock *iEstimatedServerClock;
        OsclTimebase_Tickcount iEstimatedServerClockTimeBase;

        uint32    iJitterBufferDurationInMilliSeconds;
        uint32    iJitterBufferUnderFlowThresholdInMilliSeconds;
        uint32    iPlayBackThresholdInMilliSeconds;
        bool      oDelayEstablished;

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
        void setClientPlayBackClock(OsclClock* clientClock)
        {
            iClientPlayBackClock = clientClock;
            iClientPlayBackClock->SetClockStateObserver(*this);
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
            if (iJitterBufferDurationTimer)
            {
                iJitterBufferDurationTimer->Start();
            }
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

        bool iBroadCastSession;
        void SetBroadCastSession()
        {
            iBroadCastSession = true;
        };


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
            if ((iInterfaceState == EPVMFNodePrepared) ||
                    (duration > iJitterBufferDurationInMilliSeconds))  // only update to higher value than the default
            {
                iJitterBufferDurationInMilliSeconds = duration;
            }
            if (iJitterBufferDurationTimer != NULL)
            {
                iJitterBufferDurationTimer->setJitterBufferDurationInMS(iJitterBufferDurationInMilliSeconds);
            }
        }
        void getJitterBufferDurationInMilliSeconds(uint32& duration)
        {
            duration = iJitterBufferDurationInMilliSeconds;
        }

        PVMFStatus HasSessionDurationExpired(bool& aExpired)
        {
            aExpired = oSessionDurationExpired;
            return PVMFSuccess;
        }

        void SetSharedBufferResizeParams(uint32 maxNumResizes, uint32 resizeSize);
        void GetSharedBufferResizeParams(uint32& maxNumResizes, uint32& resizeSize);

        PVMFPortInterface* getPortCounterpart(PVMFPortInterface* aPort);
        bool CheckForSpaceInJitterBuffer(PVMFPortInterface* aPort);
        PVMFStatus CheckForHighWaterMark(PVMFPortInterface* aPort, bool& aHighWaterMarkReached);
        PVMFStatus CheckForLowWaterMark(PVMFPortInterface* aPort, bool& aLowWaterMarkReached);
        PVMFStatus RequestMemCallBackForAutoResume(PVMFPortInterface* aPort);
        void freechunkavailable(OsclAny* aContextData);
        void chunkdeallocated(OsclAny* aContextData);
        bool RegisterDataPacket(PVMFPortInterface* aPort,
                                PVMFJitterBuffer* aJitterBuffer,
                                PVMFSharedMediaDataPtr& aDataPacket);
        PVMFJitterBuffer* findJitterBuffer(PVMFPortInterface* aPort);
        PVMFPortInterface* findPortForJitterBuffer(PVMFJitterBuffer*);
        bool IsJitterBufferReady(PVMFJitterBufferPortParams*);
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
        /* Timer for buffering status */
        OsclTimer<PVMFJitterBufferNodeAllocator>* iBufferingStatusTimer;
        void TimeoutOccurred(int32 timerID, int32 timeoutInfo);
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

        OsclClock *iRTCPClock;
        OsclTimebase_Tickcount iRTCPClockTimeBase;

        /*
         * InActivity Timer Releated
         */
        PvmfJBInactivityTimer* iRemoteInactivityTimer;
        uint32 iRemoteMaxInactivityDurationInMS;
        void PVMFJBInactivityTimerEvent();

        /*
         * Session Duration timer related
         */
        bool iUseSessionDurationTimerForEOS;
        PvmfJBSessionDurationTimer* iSessionDurationTimer;
        uint32 iSessionDurationInMS;
        void PVMFJBSessionDurationTimerEvent();
        void ComputeCurrentSessionDurationMonitoringInterval();

        /*
         * Jitter Buffer Duration timer related
         */
        PvmfJBJitterBufferDurationTimer* iJitterBufferDurationTimer;
        void PVMFJBJitterBufferDurationTimerEvent();

        /* firewall packet related */
        void DisableFireWallPackets()
        {
            iDisableFireWallPackets = true;
        };

        PVMFStatus CreateFireWallPacketMemAllocators(PVMFJitterBufferPortParams*);
        PVMFStatus DestroyFireWallPacketMemAllocators(PVMFJitterBufferPortParams*);
        PvmfFirewallPacketTimer* iFireWallPacketTimer;

        PVMFJitterBufferFireWallPacketInfo iFireWallPacketInfo;
        bool iDisableFireWallPackets;

        void PvmfFirewallPacketTimerEvent();
        PVMFStatus SendFireWallPackets();
        PVMFStatus ResetFireWallPacketInfoAndResend();
        void CheckForFireWallRecv(bool &aComplete);
        void CheckForFireWallPacketAttempts(bool &aComplete);
        PVMFStatus DecodeFireWallPackets(PVMFSharedMediaDataPtr aPacket,
                                         PVMFJitterBufferPortParams* aPortParamsPtr);

        /* HTTP Streaming Related */
        PVMFSMSharedBufferAllocWithReSize* iBufferAlloc;
        OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> CreateResizablePortAllocator(uint32 aSize,
                OSCL_String& aName);

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
        uint iMaxNumBufferResizes;
        uint iBufferResizeSize;

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

        //These arrays map a steamID (VID or AUD) to an RTCP or RTP port index.
        uint32 RTCPPortIndex [NUM_PVMF_STREAM_TYPES];
        uint32 InputPortIndex[NUM_PVMF_STREAM_TYPES];
};

#endif


