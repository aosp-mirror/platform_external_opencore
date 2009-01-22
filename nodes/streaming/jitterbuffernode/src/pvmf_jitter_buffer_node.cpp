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
#ifndef OSCL_EXCLUSIVE_PTR_H_INCLUDED
#include "oscl_exclusive_ptr.h"
#endif
#ifndef PVMF_MEDIA_CLOCK_H_INCLUDED
#include "pvmf_media_clock.h"
#endif
#ifndef __MEDIA_CLOCK_CONVERTER_H
#include "media_clock_converter.h"
#endif
#ifndef PVMF_STREAMING_MANAGER_NODE_H_INCLUDED
#include "pvmf_jitter_buffer_node.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_MEDIA_CMD_H_INCLUDED
#include "pvmf_media_cmd.h"
#endif
#ifndef PVMF_MEDIA_MSG_FORMAT_IDS_H_INCLUDED
#include "pvmf_media_msg_format_ids.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef RTCP_DECODER_H
#include "rtcp_decoder.h"
#endif
#ifndef RTCP_ENCODER_H
#include "rtcp_encoder.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif
#ifndef PVMF_BASIC_ERRORINFOMESSAGE_H_INCLUDED
#include "pvmf_basic_errorinfomessage.h"
#endif
#ifndef PVMF_ERRORINFOMESSAGE_EXTENSION_H_INCLUDED
#include "pvmf_errorinfomessage_extension.h"
#endif
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif
#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif
#ifndef OSCL_RAND_H_INCLUDED
#include "oscl_rand.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

#define PVMF_JBNODE_USE_BUFFER_SIZE_FOR_LOW_WATER_MARK

//For perf logging and AO statistics.
#if defined(NDEBUG)
//no stats
#define PVTICK uint32
#define SET_TICK(tick)
#define GET_TICKFREQ(tick)
#define TICK_INT(tick1)
#define TICKSTR ""
#else
//else use the oscl timer.
#define PVTICK uint32
#define SET_TICK(tick) tick=OsclTickCount::TickCount()
#define GET_TICKFREQ(tick) tick=OsclTickCount::TickCountFrequency()
#define TICK_INT(tick1) tick1
#define TICKSTR "Ticks"
#endif

#if defined(NDEBUG)
#define DIFF_TICK(tick1,diff)
#else
#define DIFF_TICK(tick1,diff) PVTICK _now;SET_TICK(_now);diff=TICK_INT(_now)-TICK_INT(tick1)
#endif


/**
 * Node Constructor & Destructor
 */
OSCL_EXPORT_REF PVMFJitterBufferNode::PVMFJitterBufferNode(int32 aPriority)
        : OsclActiveObject(aPriority, "JitterBufferNode")
{
    iStreamID = 0;
    iLogger = NULL;
    iDataPathLogger = NULL;
    iDataPathLoggerIn = NULL;
    iDataPathLoggerOut = NULL;
    iClockLogger = NULL;
    iClockLoggerSessionDuration = NULL;
    iClockLoggerRebuff = NULL;
    iDiagnosticsLogger = NULL;
    iJBEventsClockLogger = NULL;
    iExtensionInterface = NULL;
    iClientPlayBackClock = NULL;
    iClientPlayBackClockNotificationsInf = NULL;
    iEstimatedServerClock = NULL;
    iNonDecreasingClock = NULL;
    iNonDecreasingClockNotificationsInf = NULL;

    iIncomingMediaInactivityDurationCallBkId = 0;
    iIncomingMediaInactivityDurationCallBkPending = false;

    iNotifyBufferingStatusCallBkId = 0;
    iNotifyBufferingStatusCallBkPending = false;

    iJitterBufferDurationCallBkId = 0;
    iJitterBufferDurationCallBkPending = false;

    iMonitorReBufferingCallBkId = 0;
    iMonitorReBufferingCallBkPending = false;

    iSendFirewallPacketCallBkId = 0;
    iSendFirewallPacketCallBkPending = false;


    iRTCPClock = NULL;
    iMaxInactivityDurationForMediaInMs = DEFAULT_MAX_INACTIVITY_DURATION_IN_MS;
    iJitterBufferDurationInMilliSeconds = DEFAULT_JITTER_BUFFER_DURATION_IN_MS;
    iJitterBufferUnderFlowThresholdInMilliSeconds = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
    iPlayBackThresholdInMilliSeconds = DEFAULT_PLAY_BACK_THRESHOLD_IN_MS;
    iEstimatedServerKeepAheadInMilliSeconds = DEFAULT_ESTIMATED_SERVER_KEEPAHEAD_FOR_OOO_SYNC_IN_MS;

    oDelayEstablished = false;
    oSessionDurationExpired = false;
    oStopOutputPorts = true;
    oAutoPause = false;
    iRTCPIntervalInMicroSeconds = DEFAULT_RTCP_INTERVAL_USEC;
    iPlayStartTimeInMS = 0;
    iPlayStopTimeInMS = 0;
    oStartPending = false;
    iPlayingAfterSeek = false;
    iJitterBufferState = PVMF_JITTER_BUFFER_READY;
    iJitterDelayPercent = 0;
    iDisableFireWallPackets = false;

    iNumUnderFlow = 0;
    iUseSessionDurationTimerForEOS = true;
    iUnderFlowStartByte = 0;
    iStartBufferingTickCount = 0;
    iPrevBufferingStartPacketTs = 0;
    iThinningIntervalMS = 0;

    iExtensionInterface = NULL;
    iNumRunL = 0;
    iOverflowFlag = false;


    iBroadCastSession = false;
    iRTCPBcastAVSyncProcessed = false;

    iBufferingStatusIntervalInMs =
        (PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_CYCLES * 1000) / PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_FREQUENCY;

    iMaxNumBufferResizes = DEFAULT_MAX_NUM_SOCKETMEMPOOL_RESIZES;
    iBufferResizeSize = DEFAULT_MAX_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;

    prevMinPercentOccupancy = 100;
    consecutiveLowBufferCount = 0;
    int32 err = OsclErrNone;
    OSCL_TRY(err,
             /*
              * Create the input command queue.  Use a reserve to avoid lots of
              * dynamic memory allocation
              */
             iInputCommands.Construct(PVMF_JITTER_BUFFER_NODE_COMMAND_ID_START,
                                      PVMF_JITTER_BUFFER_VECTOR_RESERVE);

             /*
              * Create the "current command" queue.  It will only contain one
              * command at a time, so use a reserve of 1
              */
             iCurrentCommand.Construct(0, 1);

             /* Create the port vector */
             iPortVector.Construct(PVMF_JITTER_BUFFER_NODE_PORT_VECTOR_RESERVE);

             OsclExclusivePtr<PVMFMediaClock> estServClockAutoPtr;
             OsclExclusivePtr<PVMFMediaClock> rtcpClockAutoPtr;
             OsclExclusivePtr<PvmfRtcpTimer> rtcpTimerAutoPtr;
             OsclExclusivePtr<PvmfJBSessionDurationTimer> sessionDurationTimerAutoPtr;
             typedef OsclTimer<PVMFJitterBufferNodeAllocator> osclTimerType;

             PVMF_JITTER_BUFFER_NEW(NULL, PVMFMediaClock, (), iEstimatedServerClock);
             estServClockAutoPtr.set(iEstimatedServerClock);
             iEstimatedServerClock->SetClockTimebase(iEstimatedServerClockTimeBase);

             PVMF_JITTER_BUFFER_NEW(NULL, PVMFMediaClock, (), iNonDecreasingClock);
             iNonDecreasingClock->SetClockTimebase(iNonDecreasingClockTimeBase);

             PVMF_JITTER_BUFFER_NEW(NULL, PVMFMediaClock, (), iRTCPClock);
             rtcpClockAutoPtr.set(iRTCPClock);
             iRTCPClock->SetClockTimebase(iRTCPClockTimeBase);
             PVMF_JITTER_BUFFER_NEW(NULL, PvmfJBSessionDurationTimer, (this), iSessionDurationTimer);
             sessionDurationTimerAutoPtr.set(iSessionDurationTimer);
             iSessionDurationTimer->SetEstimatedServerClock(iEstimatedServerClock);

             if (iEstimatedServerClock)
{
    PVMFStatus status = iEstimatedServerClock->ConstructMediaClockNotificationsInterface(iEstimatedClockNotificationsInf, *this);
        if (PVMFSuccess != status || !iEstimatedClockNotificationsInf)
        {
            iEstimatedClockNotificationsInf = NULL;
            OSCL_ASSERT(false);
        }
    }

    /*Construct the event clock notification intf, and start the event clock*/
    if (iNonDecreasingClock)
{
    iNonDecreasingClock->ConstructMediaClockNotificationsInterface(iNonDecreasingClockNotificationsInf, *this);
        if (NULL == iNonDecreasingClockNotificationsInf)
        {
            OSCL_ASSERT(false);
        }
        uint32 start = 0;
        iNonDecreasingClock->Stop();
        bool overflowFlag = false;
        iNonDecreasingClock->SetStartTime32(start, PVMF_MEDIA_CLOCK_MSEC, overflowFlag);
        iNonDecreasingClock->Start();
    }

    /*
     * Set the node capability data.
     * This node can support an unlimited number of ports.
     */
    iCapability.iCanSupportMultipleInputPorts = true;
    iCapability.iCanSupportMultipleOutputPorts = true;
    iCapability.iHasMaxNumberOfPorts = false;
    iCapability.iMaxNumberOfPorts = 0;//no maximum
    iCapability.iInputFormatCapability.push_back(PVMFFormatType(PVMF_MIME_RTP));
    iCapability.iOutputFormatCapability.push_back(PVMFFormatType(PVMF_MIME_RTP));

    // seed the random number generator
    iRandGen.Seed(RTCP_RAND_SEED);

    estServClockAutoPtr.release();
    rtcpClockAutoPtr.release();
    rtcpTimerAutoPtr.release();
    sessionDurationTimerAutoPtr.release();
            );

    if (err != OsclErrNone)
    {
        CleanUp();
        OSCL_LEAVE(err);
    }

}

void PVMFJitterBufferNode::CleanUp()
{
    /* if a leave happened, cleanup and re-throw the error */
    iInputCommands.clear();
    iCurrentCommand.clear();
    iPortVector.clear();
    iCapability.iInputFormatCapability.clear();
    iCapability.iOutputFormatCapability.clear();
    OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
    OSCL_CLEANUP_BASE_CLASS(OsclActiveObject);
    return;
}

OSCL_EXPORT_REF PVMFJitterBufferNode::~PVMFJitterBufferNode()
{
    LogSessionDiagnostics();

    Cancel();
    /* thread logoff */
    if (IsAdded())
        RemoveFromScheduler();

    if (iExtensionInterface)
    {
        iExtensionInterface->removeRef();
    }

    iUseSessionDurationTimerForEOS = true;

    for (uint32 i = 0; i < iPortParamsQueue.size(); i++)
    {
        if (iPortParamsQueue[i].iBufferAlloc != NULL)
        {
            iPortParamsQueue[i].iBufferAlloc->CancelFreeMemoryAvailableCallback();
            iPortParamsQueue[i].iBufferAlloc->removeRef();
            iPortParamsQueue[i].iBufferAlloc = NULL;
        }
    }

    /* Cleanup allocated ports */
    while (!iPortVector.empty())
    {
        /* delete corresponding port params */
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
            if (it->iPort == iPortVector.front())
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    /* Delete the jitter buffer */
                    PVMF_JITTER_BUFFER_DELETE(NULL,
                                              PVMFJitterBufferImpl,
                                              ((PVMFJitterBufferImpl*)(it->iJitterBuffer)));
                    DestroyFireWallPacketMemAllocators(it);
                }
                else if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
                {

                    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfRtcpTimer, it->iRTCPTimer);
                }
                iPortParamsQueue.erase(it);
                break;
            }
        }
        iPortVector.Erase(&iPortVector.front());
    }

    /*
     * Cleanup commands
     * The command queues are self-deleting, but we want to
     * notify the observer of unprocessed commands.
     */
    while (!iCurrentCommand.empty())
    {
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFFailure);
    }
    while (!iInputCommands.empty())
    {
        CommandComplete(iInputCommands, iInputCommands.front(), PVMFFailure);
    }


    CancelEventCallBack(JB_MONITOR_REBUFFERING);
    CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfJBSessionDurationTimer, iSessionDurationTimer);
    PVMF_JITTER_BUFFER_DELETE(NULL, PVMFMediaClock, iRTCPClock);
    CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
    CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);


    if ((NULL != iClientPlayBackClock) && (NULL != iClientPlayBackClockNotificationsInf))
    {
        iClientPlayBackClockNotificationsInf->RemoveClockStateObserver(*this);
        iClientPlayBackClock->DestroyMediaClockNotificationsInterface(iClientPlayBackClockNotificationsInf);
        iClientPlayBackClockNotificationsInf = NULL;
    }

    if (NULL != iNonDecreasingClock)
    {
        if (NULL != iNonDecreasingClockNotificationsInf)
        {
            iNonDecreasingClock->DestroyMediaClockNotificationsInterface(iNonDecreasingClockNotificationsInf);
            iNonDecreasingClockNotificationsInf = NULL;
        }
        PVMF_JITTER_BUFFER_DELETE(NULL, PVMFMediaClock, iNonDecreasingClock);
        iNonDecreasingClock = NULL;
    }

    if (NULL != iEstimatedServerClock)
    {
        if (NULL != iEstimatedClockNotificationsInf)
        {
            iEstimatedServerClock->DestroyMediaClockNotificationsInterface(iEstimatedClockNotificationsInf);
            iEstimatedClockNotificationsInf = NULL;
        }
        PVMF_JITTER_BUFFER_DELETE(NULL, PVMFMediaClock, iEstimatedServerClock);
        iEstimatedServerClock = NULL;
    }
}

/**
 * Public Node API implementation
 */

/**
 * Do thread-specific node creation and go to "Idle" state.
 */
OSCL_EXPORT_REF PVMFStatus PVMFJitterBufferNode::ThreadLogon()
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:ThreadLogon"));
    PVMFStatus status;
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
        {
            if (!IsAdded())
                AddToScheduler();
            iLogger = PVLogger::GetLoggerObject("JitterBufferNode");
            iDataPathLogger = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer");
            iDataPathLoggerIn = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.in");
            iDataPathLoggerOut = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.out");
            iDataPathLoggerFireWall = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.fw");
            iClockLogger = PVLogger::GetLoggerObject("clock.jitterbuffernode");
            iClockLoggerSessionDuration = PVLogger::GetLoggerObject("clock.streaming_manager.sessionduration");
            iClockLoggerRebuff = PVLogger::GetLoggerObject("clock.jitterbuffernode.rebuffer");
            iDiagnosticsLogger = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");
            iDataPathLoggerFlowCtrl = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.flowctrl");
            iDataPathLoggerRTCP = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.rtcp");
            iJBEventsClockLogger = PVLogger::GetLoggerObject("jitterbuffernode.eventsclock");
            iRTCPAVSyncLogger = PVLogger::GetLoggerObject("jitterbuffernode.rtcpavsync");
            iDiagnosticsLogged = false;
            SetState(EPVMFNodeIdle);
            status = PVMFSuccess;
        }
        break;
        default:
            status = PVMFErrInvalidState;
            break;
    }

    return status;
}

/**
 * Do thread-specific node cleanup and go to "Created" state.
 */
OSCL_EXPORT_REF PVMFStatus PVMFJitterBufferNode::ThreadLogoff()
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:ThreadLogoff"));
    PVMFStatus status;
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
        {
            /* Cancel any outstanding timers */
            CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
            if (iSessionDurationTimer)
            {
                iSessionDurationTimer->Cancel();
            }

            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
                {
                    if (it->iRTCPTimer != NULL)
                        it->iRTCPTimer->Cancel();
                }
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
            }
            CancelEventCallBack(JB_MONITOR_REBUFFERING);
            CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
            CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
            if (IsAdded())
            {
                RemoveFromScheduler();
            }
            iLogger = NULL;
            iDataPathLogger = NULL;
            iDataPathLoggerIn = NULL;
            iDataPathLoggerOut = NULL;
            iDataPathLoggerFireWall = NULL;
            iClockLogger = NULL;
            iClockLoggerSessionDuration = NULL;
            iDiagnosticsLogger = NULL;
            iDataPathLoggerFlowCtrl = NULL;
            iDataPathLoggerRTCP = NULL;
            SetState(EPVMFNodeCreated);
            status = PVMFSuccess;
        }
        break;

        default:
            status = PVMFErrInvalidState;
            break;
    }
    return status;
}

/**
 * retrieve node capabilities.
 */
OSCL_EXPORT_REF PVMFStatus PVMFJitterBufferNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:GetCapability"));
    aNodeCapability = iCapability;
    return PVMFSuccess;
}

/**
 * retrieve a port iterator.
 */
OSCL_EXPORT_REF PVMFPortIter* PVMFJitterBufferNode::GetPorts(const PVMFPortFilter* aFilter)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:GetPorts"));
    OSCL_UNUSED_ARG(aFilter);//port filter is not implemented.
    iPortVector.Reset();
    return &iPortVector;
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::QueryUUID(PVMFSessionId s,
                                const PvmfMimeString& aMimeType,
                                Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                bool aExactUuidsOnly,
                                const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:QueryUUID"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_QUERYUUID,
            aMimeType,
            aUuids,
            aExactUuidsOnly,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::QueryInterface(PVMFSessionId s,
                                     const PVUuid& aUuid,
                                     PVInterface*& aInterfacePtr,
                                     const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:QueryInterface"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_QUERYINTERFACE,
            aUuid,
            aInterfacePtr,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::RequestPort(PVMFSessionId s,
                                  int32 aPortTag,
                                  const PvmfMimeString* aPortConfig,
                                  const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:RequestPort"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_REQUESTPORT,
            aPortTag,
            aPortConfig,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::ReleasePort(PVMFSessionId s,
                                  PVMFPortInterface& aPort,
                                  const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:ReleasePort"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_RELEASEPORT,
            aPort,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFJitterBufferNode::Init(PVMFSessionId s,
        const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Init"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_INIT,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFJitterBufferNode::Prepare(PVMFSessionId s,
        const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Prepare"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_PREPARE,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::Start(PVMFSessionId s,
                            const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Start"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_START,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::Stop(PVMFSessionId s,
                           const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Stop"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_STOP,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::Flush(PVMFSessionId s,
                            const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Flush"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_FLUSH,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::Pause(PVMFSessionId s,
                            const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Pause"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_PAUSE,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::Reset(PVMFSessionId s,
                            const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:Reset"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_RESET,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFJitterBufferNode::CancelAllCommands(PVMFSessionId s,
                                        const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:CancelAllCommands"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_CANCELALLCOMMANDS,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFJitterBufferNode::CancelCommand(PVMFSessionId s, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:CancelCommand"));
    PVMFJitterBufferNodeCommand cmd;
    cmd.PVMFJitterBufferNodeCommandBase::Construct(s,
            PVMF_JITTER_BUFFER_NODE_CANCELCOMMAND,
            aCmdId,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * This routine is called by various command APIs to queue an
 * asynchronous command for processing by the command handler AO.
 * This function may leave if the command can't be queued due to
 * memory allocation failure.
 */
PVMFCommandId PVMFJitterBufferNode::QueueCommandL(PVMFJitterBufferNodeCommand& aCmd)
{
    PVMFCommandId id;

    id = iInputCommands.AddL(aCmd);

    if (IsAdded())
    {
        //wakeup the AO
        RunIfNotReady();
    }
    return id;
}

/**
 * Asynchronous Command processing routines.
 * These routines are all called under the AO.
 */

/**
 * Called by the command handler AO to process a command from
 * the input queue.
 * Return true if a command was processed, false if the command
 * processor is busy and can't process another command now.
 */
bool PVMFJitterBufferNode::ProcessCommand(PVMFJitterBufferNodeCommand& aCmd)
{
    /*
     * normally this node will not start processing one command
     * until the prior one is finished.  However, a hi priority
     * command such as Cancel must be able to interrupt a command
     * in progress.
     */
    if (!iCurrentCommand.empty() && !aCmd.hipri())
        return false;

    switch (aCmd.iCmd)
    {
        case PVMF_JITTER_BUFFER_NODE_QUERYUUID:
            DoQueryUuid(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_REQUESTPORT:
            DoRequestPort(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_RELEASEPORT:
            DoReleasePort(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_INIT:
            DoInit(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_PREPARE:
            DoPrepare(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_START:
            DoStart(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_STOP:
            DoStop(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_FLUSH:
            DoFlush(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_PAUSE:
            DoPause(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_RESET:
            DoReset(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_CANCELALLCOMMANDS:
            DoCancelAllCommands(aCmd);
            break;

        case PVMF_JITTER_BUFFER_NODE_CANCELCOMMAND:
            DoCancelCommand(aCmd);
            break;

        default:
        {
            /* unknown command type */
            CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
        }
        break;
    }

    return true;
}

/**
 * The various command handlers call this when a command is complete.
 */
void PVMFJitterBufferNode::CommandComplete(PVMFJitterBufferNodeCmdQ& aCmdQ,
        PVMFJitterBufferNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                         , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    PVInterface* extif = NULL;
    PVMFBasicErrorInfoMessage* errormsg = NULL;
    if (aEventUUID && aEventCode)
    {
        PVMF_JITTER_BUFFER_NEW(NULL, PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL), errormsg);
        extif = OSCL_STATIC_CAST(PVInterface*, errormsg);
    }

    /* create response */
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, extif, aEventData);
    PVMFSessionId session = aCmd.iSession;

    /* Erase the command from the queue */
    aCmdQ.Erase(&aCmd);

    /* Report completion to the session observer */
    ReportCmdCompleteEvent(session, resp);

    if (errormsg)
    {
        errormsg->removeRef();
    }

    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if ((aStatus == PVMFFailure) ||
            (aStatus == PVMFErrNoMemory) ||
            (aStatus == PVMFErrNoResources))
    {
        SetState(EPVMFNodeError);
    }
}

void PVMFJitterBufferNode::CommandComplete(PVMFJitterBufferNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                         , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    PVInterface* extif = NULL;
    PVMFBasicErrorInfoMessage* errormsg = NULL;
    if (aEventUUID && aEventCode)
    {
        PVMF_JITTER_BUFFER_NEW(NULL, PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL), errormsg);
        extif = OSCL_STATIC_CAST(PVInterface*, errormsg);
    }

    /* create response */
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, extif, aEventData);
    PVMFSessionId session = aCmd.iSession;

    /* Report completion to the session observer */
    ReportCmdCompleteEvent(session, resp);

    if (errormsg)
    {
        errormsg->removeRef();
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if ((aStatus == PVMFFailure) ||
            (aStatus == PVMFErrNoMemory) ||
            (aStatus == PVMFErrNoResources))
    {
        SetState(EPVMFNodeError);
    }
}

/**
 * The various command handlers call this when a INTERNAL command is complete.
 * Does not report completion as it is an internal command
 */
void PVMFJitterBufferNode::InternalCommandComplete(PVMFJitterBufferNodeCmdQ& aCmdQ,
        PVMFJitterBufferNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData)
{
    OSCL_UNUSED_ARG(aEventData);

    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:InternalCommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                         , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    /* Erase the command from the queue */
    aCmdQ.Erase(&aCmd);

    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if ((aStatus == PVMFFailure) ||
            (aStatus == PVMFErrNoMemory) ||
            (aStatus == PVMFErrNoResources))
    {
        SetState(EPVMFNodeError);
    }
}

void
PVMFJitterBufferNode::MoveCmdToCurrentQueue(PVMFJitterBufferNodeCommand& aCmd)
{
    int32 err;
    OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
    if (err != OsclErrNone)
    {
        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
        return;
    }
    iInputCommands.Erase(&aCmd);
    return;
}

/**
 * Called by the command handler AO to do the node Reset.
 */
void PVMFJitterBufferNode::DoReset(PVMFJitterBufferNodeCommand& aCmd)
{
    PVMF_JBNODE_LOGERROR((0, "JitterBufferNode:DoReset %d", iInterfaceState));

    LogSessionDiagnostics();

    /* This node allows a reset from any idle or error state */
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
            CancelEventCallBack(JB_MONITOR_REBUFFERING);
            /* Stop session duration timer */
            iSessionDurationTimer->Stop();
            /* Stop RTCP Timer */
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
                {
                    if (it->iRTCPTimer != NULL)
                        it->iRTCPTimer->Stop();
                }
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
            }

            CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
            /* Pause Estimated Server Clock */
            PVMFStatus aStatus = PVMFSuccess;
            if (iEstimatedServerClock)
            {
                iEstimatedServerClock->Pause();
            }
            if (iRTCPClock)
            {
                iRTCPClock->Pause();
            }
            /* Clear queued messages in ports */
            uint32 i;
            for (i = 0; i < iPortVector.size(); i++)
            {
                PVMFJitterBufferPortParams* portParams = NULL;
                bool bRet = getPortContainer(iPortVector[i], portParams);
                if (bRet)
                {
                    if (portParams->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                    {
                        portParams->iJitterBuffer->ResetJitterBuffer();
                    }

                    portParams->ResetParams();
                }
                iPortVector[i]->ClearMsgQueues();
            }

            if (aStatus == PVMFSuccess)
            {
                /* Reset State Variables */
                oDelayEstablished = false;
                oSessionDurationExpired = false;
                oStopOutputPorts = true;
                oAutoPause = false;
                oStartPending = false;
                iJitterBufferState = PVMF_JITTER_BUFFER_READY;
                iJitterDelayPercent = 0;
            }
        }
        /* Intentional fall thru */
        case EPVMFNodeCreated:
        case EPVMFNodeIdle:
        case EPVMFNodeInitialized:
        case EPVMFNodePrepared:
        case EPVMFNodeError:
        {
            /* Stop Estimated Server Clock */
            if (iEstimatedServerClock)
            {
                iEstimatedServerClock->Stop();
            }
            if (iRTCPClock)
            {
                iRTCPClock->Stop();
            }
            /* delete all ports and notify observer */
            while (!iPortVector.empty())
            {
                iPortVector.Erase(&iPortVector.front());
            }

            for (uint32 i = 0; i < iPortParamsQueue.size(); i++)
            {
                if (iPortParamsQueue[i].iBufferAlloc != NULL)
                {
                    iPortParamsQueue[i].iBufferAlloc->CancelFreeMemoryAvailableCallback();
                    iPortParamsQueue[i].iBufferAlloc->removeRef();
                    iPortParamsQueue[i].iBufferAlloc = NULL;
                }
            }

            /* delete port params */
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            while (!iPortParamsQueue.empty())
            {
                it = iPortParamsQueue.begin();
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    /* Delete the jitter buffer */
                    PVMF_JITTER_BUFFER_DELETE(NULL,
                                              PVMFJitterBufferImpl,
                                              ((PVMFJitterBufferImpl*)(it->iJitterBuffer)));
                    DestroyFireWallPacketMemAllocators(it);
                }
                else if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
                {

                    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfRtcpTimer, it->iRTCPTimer);
                }
                iPortParamsQueue.erase(it);
            }
            /* restore original port vector reserve */
            iPortVector.Reconstruct();
            iUseSessionDurationTimerForEOS = true;
            SetState(EPVMFNodeIdle);
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
        }
        iNonDecreasingClock->Stop();
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/**
 * Called by the command handler AO to do the Query UUID
 */
void PVMFJitterBufferNode::DoQueryUuid(PVMFJitterBufferNodeCommand& aCmd)
{
    /* This node supports Query UUID from any state */
    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;
    aCmd.PVMFJitterBufferNodeCommandBase::Parse(mimetype, uuidvec, exactmatch);

    /*
     * Try to match the input mimetype against any of
     * the custom interfaces for this node
     */

    /*
     * Match against custom interface1...
     * also match against base mimetypes for custom interface1,
     * unless exactmatch is set.
     */
    if (*mimetype == PVMF_JITTERBUFFER_CUSTOMINTERFACE_MIMETYPE
            || (!exactmatch && *mimetype == PVMF_JITTERBUFFER_MIMETYPE)
            || (!exactmatch && *mimetype == PVMF_JITTERBUFFER_BASEMIMETYPE))
    {
        PVUuid uuid(PVMF_JITTERBUFFERNODE_EXTENSIONINTERFACE_UUID);
        uuidvec->push_back(uuid);
    }
    CommandComplete(iInputCommands, aCmd, PVMFSuccess);
}

/**
 * Called by the command handler AO to do the Query Interface.
 */
void PVMFJitterBufferNode::DoQueryInterface(PVMFJitterBufferNodeCommand& aCmd)
{
    /* This node supports Query Interface from any state */
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.PVMFJitterBufferNodeCommandBase::Parse(uuid, ptr);

    if (*uuid == PVUuid(PVMF_JITTERBUFFERNODE_EXTENSIONINTERFACE_UUID))
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
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoQueryInterface: Error - Out of memory"));
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
            iExtensionInterface =
                OSCL_PLACEMENT_NEW(ptr, PVMFJitterBufferExtensionInterfaceImpl(this));
        }
        if (iExtensionInterface->queryInterface(*uuid, *ptr))
        {
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
        }
        else
        {
            CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
        }
    }
    else
    {
        /* not supported */
        *ptr = NULL;
        CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
    }
}

/**
 * Called by the command handler AO to do the port request
 */
void PVMFJitterBufferNode::DoRequestPort(PVMFJitterBufferNodeCommand& aCmd)
{
    /* This node supports port request from any state */
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::DoRequestPort"));

    /* retrieve port tag*/
    int32 tag;
    OSCL_String* mimetype;
    aCmd.PVMFJitterBufferNodeCommandBase::Parse(tag, mimetype);

    PVMFJitterBufferPortParams portParams;
    /*
     * Input ports have tags: 0, 3, 6, ...
     * Output ports have tags: 1, 4, 7, ...
     * Feedback ports have tags: 2, 5, 8, ...
     */
    if (tag % 3)
    {
        if (tag % 3 == 1)
        {
            portParams.tag = PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT;
        }
        else if (tag % 3 == 2)
        {
            portParams.tag = PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK;
            PVMF_JITTER_BUFFER_NEW(NULL, PvmfRtcpTimer, (this), (portParams.iRTCPTimer));
        }
    }
    else
    {
        portParams.tag = PVMF_JITTER_BUFFER_PORT_TYPE_INPUT;
    }

    //set port name for datapath logging.
    OSCL_StackString<20> portname;
    switch (portParams.tag)
    {
        case PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT:
            portname = "JitterBufOut";
            break;
        case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
            //don't log this port for now...
            //portname="JitterBufFeedback";
            break;
        case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
            //don't log this port for now...
            //portname="JitterBufIn";
            break;
        default:
            break;
    }

    /* Allocate a new port */
    OsclAny *ptr = NULL;
    int32 err;
    OSCL_TRY(err, ptr = iPortVector.Allocate(););
    if (err != OsclErrNone || !ptr)
    {
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoRequestPort: Error - iPortVector Out of memory"));
        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
        return;
    }

    OsclExclusivePtr<PVMFJitterBufferPort> portAutoPtr;
    OsclExclusivePtr<PVMFJitterBuffer> jitterBufferAutoPtr;

    PVMFJitterBufferPort* port = NULL;
    PVMFJitterBufferImpl* jbPtr = NULL;

    /*
     * create base port with default settings
     */
    port = OSCL_PLACEMENT_NEW(ptr, PVMFJitterBufferPort(tag, this, portname.get_str()));
    portAutoPtr.set(port);

    /* Add the port to the port vector. */
    OSCL_TRY(err, iPortVector.AddL(port););
    if (err != OsclErrNone)
    {
        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
        return;
    }

    portParams.iJitterBuffer = NULL;
    portParams.iPort = port;
    portParams.id = tag;
    OSCL_StackString<8> rtp(_STRLIT_CHAR("RTP"));
    if (mimetype != NULL)
    {
        if (oscl_strncmp(mimetype->get_cstr(), rtp.get_cstr(), 3) == 0)
        {
            portParams.iTransportType = rtp;
            portParams.eTransportType = PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP;
        }
        portParams.iMimeType = mimetype->get_str();
    }

    /*
     * create jitter buffer if input port
     */
    if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
    {
        PVMF_JITTER_BUFFER_NEW(NULL, PVMFJitterBufferImpl, (mimetype), jbPtr);
        jitterBufferAutoPtr.set(jbPtr);
        portParams.iJitterBuffer = jbPtr;
        jbPtr->SetEstimatedServerClock(iEstimatedServerClock);
        CreateFireWallPacketMemAllocators(&portParams);
        if (iBroadCastSession == true)
        {
            jbPtr->SetBroadCastSession();
        }
    }

    OSCL_TRY(err, iPortParamsQueue.push_back(portParams););
    if (err != OsclErrNone)
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::DoRequestPort: Error - iPortParamsQueue.push_back() failed", this));
        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
        return;
    }


    // Update the iPortParams for all existing ports since adding a new Port Parameters element might
    // have caused reallocation of the vector elements.
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        PVMFJitterBufferPortParams* portParametersPtr = it;
        PVMFJitterBufferPort* portPtr = OSCL_REINTERPRET_CAST(PVMFJitterBufferPort*, portParametersPtr->iPort);
        portPtr->iPortParams = portParametersPtr;

        // Update also the port counterpart and port counterpart parameters
        PVMFPortInterface* cpPort = getPortCounterpart(portPtr);
        if (cpPort != NULL)
        {
            portPtr->iPortCounterpart = (PVMFJitterBufferPort*)cpPort;
            PVMFJitterBufferPortParams* cpPortContainerPtr = NULL;
            if (getPortContainer(portPtr->iPortCounterpart, cpPortContainerPtr))
            {
                portPtr->iCounterpartPortParams = cpPortContainerPtr;
            }
            else
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoRequestPort: getPortContainer for port counterpart failed"));
                CommandComplete(iInputCommands, aCmd, PVMFFailure);
                return;
            }
        }

    }

    portAutoPtr.release();
    jitterBufferAutoPtr.release();

    /* Return the port pointer to the caller. */
    CommandComplete(iInputCommands, aCmd, PVMFSuccess, (OsclAny*)port);
}

/**
 * Called by the command handler AO to do the port release
 */
void PVMFJitterBufferNode::DoReleasePort(PVMFJitterBufferNodeCommand& aCmd)
{
    /*This node supports release port from any state*/

    /* Find the port in the port vector */
    PVMFPortInterface* p = NULL;
    aCmd.PVMFJitterBufferNodeCommandBase::Parse(p);

    PVMFJitterBufferPort* port = (PVMFJitterBufferPort*)p;

    PVMFJitterBufferPort** portPtr = iPortVector.FindByValue(port);
    if (portPtr)
    {
        /* delete corresponding port params */
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if (it->iPort == iPortVector.front())
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    /* Delete the jitter buffer */
                    PVMF_JITTER_BUFFER_DELETE(NULL,
                                              PVMFJitterBufferImpl,
                                              ((PVMFJitterBufferImpl*)(it->iJitterBuffer)));
                    DestroyFireWallPacketMemAllocators(it);
                }
                else if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
                {

                    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfRtcpTimer, it->iRTCPTimer);
                }

                iPortParamsQueue.erase(it);
                break;
            }
        }
        /* delete the port */
        iPortVector.Erase(portPtr);

        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else
    {
        /* port not found */
        CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
    }
}

/**
 * Called by the command handler AO to do the node Init
 */
void PVMFJitterBufferNode::DoInit(PVMFJitterBufferNodeCommand& aCmd)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::DoInit"));
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            /*
             * this node doesn't need to do anything to get ready
             * to prepare.
             */
            if (PVMFMediaClock::RUNNING != iNonDecreasingClock->GetState())
            {
                iNonDecreasingClock->Start(); //JB node may would have been paused -> reset -> Init
            }

            SetState(EPVMFNodeInitialized);
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/**
 * Called by the command handler AO to do the node Prepare
 */
void PVMFJitterBufferNode::DoPrepare(PVMFJitterBufferNodeCommand& aCmd)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::DoPrepare"));
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
        {
            uint32 i;
            for (i = 0; i < iPortVector.size(); i++)
            {
                PVMFJitterBufferPortParams* portContainerPtr1 = NULL;
                if (getPortContainer(iPortVector[i], portContainerPtr1))
                {
                    iPortVector[i]->iPortParams = portContainerPtr1;
                }
                else
                {
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoPrepare: getPortContainer - Self"));
                    CommandComplete(iInputCommands, aCmd, PVMFFailure);
                    break;
                }
                PVMFPortInterface* cpPort = getPortCounterpart(iPortVector[i]);
                if (cpPort != NULL)
                {
                    iPortVector[i]->iPortCounterpart = (PVMFJitterBufferPort*)cpPort;
                    PVMFJitterBufferPortParams* portContainerPtr2 = NULL;
                    if (getPortContainer(iPortVector[i]->iPortCounterpart, portContainerPtr2))
                    {
                        iPortVector[i]->iCounterpartPortParams = portContainerPtr2;
                    }
                    else
                    {
                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoPrepare: getPortContainer - Counterpart"));
                        CommandComplete(iInputCommands, aCmd, PVMFFailure);
                        break;
                    }
                }
            }
            /* initialize the estimated server clock */
            if (iEstimatedServerClock)
            {
                uint32 start = 0;
                iEstimatedServerClock->Stop();
                bool overflowFlag = false;
                iEstimatedServerClock->SetStartTime32(start, PVMF_MEDIA_CLOCK_MSEC, overflowFlag);
            }
            if (iDisableFireWallPackets == false)
            {
                MoveCmdToCurrentQueue(aCmd);
                /* Wait for firewall packet exchange, if any (wait of setServerInfo Call) */
            }
            else
            {
                PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::DoPrepare: FW Pkts Disabled"));
                /* Complete prepare */
                SetState(EPVMFNodePrepared);
                CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            }
        }
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

void PVMFJitterBufferNode::CompletePrepare()
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::CompletePrepare"));
    SetState(EPVMFNodePrepared);
    PVMFJitterBufferNodeCommand cmd = iCurrentCommand.front();
    if (iDisableFireWallPackets == false)
    {
        CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);

        bool oFireWallPacketExchangeComplete = true;
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                if (it->oFireWallPacketRecvd == false)
                {
                    oFireWallPacketExchangeComplete = false;
                }
            }
        }
        PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
        int32 errcode = PVMFJitterBufferNodeFirewallPacketExchangeFailed;
        if (iCurrentCommand.size() > 0)
        {
            if (oFireWallPacketExchangeComplete == false)
            {
                /* Signal streaming manager in case it wants to attempt protocol rollover */
                CommandComplete(cmd, PVMFSuccess, NULL, &eventuuid, &errcode);
            }
            else
            {
                CommandComplete(cmd, PVMFSuccess);
            }
            iCurrentCommand.Erase(&iCurrentCommand.front());
        }
    }
    else
    {
        // need this check so we don't seg fault
        // will this delete ever be necessary? this method is used synchronously
        if (iCurrentCommand.size() > 0)
        {
            CommandComplete(cmd, PVMFSuccess);
            iCurrentCommand.Erase(&iCurrentCommand.front());
        }
    }
    return;
}

void PVMFJitterBufferNode::CancelPrepare()
{
    CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
    PVMFJitterBufferNodeCommand cmd = iCurrentCommand.front();
    CommandComplete(cmd, PVMFErrCancelled);
    iCurrentCommand.Erase(&iCurrentCommand.front());
    return;
}

/**
 * Called by the command handler AO to do the node Start
 */
void PVMFJitterBufferNode::DoStart(PVMFJitterBufferNodeCommand& aCmd)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::DoStart"));
    PVMFStatus status = PVMFSuccess;
    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
        case EPVMFNodePaused:
        {
            if (PVMFMediaClock::RUNNING != iNonDecreasingClock->GetState())
            {
                iNonDecreasingClock->Start();
            }
            /* Diagnostic logging */
            iDiagnosticsLogged = false;
            /* If auto paused, implies jitter buffer is not empty */
            if (oAutoPause == false)
            {
                if (oSessionDurationExpired == false)
                {
                    RequestEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                    StartEstimatedServerClock();

                }
                /* Start RTCP Clock */
                iRTCPClock->Start();

                if (iInterfaceState == EPVMFNodePaused)
                {
                    if (iDisableFireWallPackets == false)
                    {
                        uint32 currticks = OsclTickCount::TickCount();
                        uint32 startTime = OsclTickCount::TicksToMsec(currticks);
                        uint32 diff = (startTime - iPauseTime);
                        if (diff > PVMF_JITTER_BUFFER_NODE_FIREWALL_PKT_DEFAULT_PAUSE_DURATION_IN_MS)
                        {
                            ResetFireWallPacketInfoAndResend();
                        }
                    }
                }

                if ((oDelayEstablished == false) ||
                        (iJitterBufferState == PVMF_JITTER_BUFFER_IN_TRANSITION))
                {
                    /* Counter for intelligent streaming is reset at Start or setPlaybackRange*/
                    iNumUnderFlow = 0;
                    /* Start Buffering Status Timer */
                    RequestEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
                    /*
                     * Move start to current msg queue where it would stay
                     * jitter buffer is full.
                     */
                    oStartPending = true;
                    MoveCmdToCurrentQueue(aCmd);
                    ReportInfoEvent(PVMFInfoBufferingStart);
                }
                else
                {
                    /* Just resuming from a paused state with enough data in jitter buffer */
                    oStartPending = false;
                    SetState(EPVMFNodeStarted);
                    /* Enable Output Ports */
                    StartOutputPorts();
                    CommandComplete(iInputCommands, aCmd, PVMFSuccess);
                }
            }
            else
            {
                if (iInterfaceState == EPVMFNodePaused)
                {
                    uint32 currticks = OsclTickCount::TickCount();
                    uint32 startTime = OsclTickCount::TicksToMsec(currticks);
                    uint32 diff = (startTime - iPauseTime);
                    if (diff > PVMF_JITTER_BUFFER_NODE_FIREWALL_PKT_DEFAULT_PAUSE_DURATION_IN_MS)
                    {
                        ResetFireWallPacketInfoAndResend();
                    }
                }
                oStartPending = false;
                SetState(EPVMFNodeStarted);
                /* Enable Output Ports */
                StartOutputPorts();
                CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            }
        }
        break;

        default:
            status = PVMFErrInvalidState;
            CommandComplete(iInputCommands, aCmd, status);
            break;
    }
    return;
}

void PVMFJitterBufferNode::CompleteStart()
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::CompleteStart"));

    PVMFJitterBufferNodeCommand aCmd = iCurrentCommand.front();
    if (iJitterBufferState == PVMF_JITTER_BUFFER_READY)
    {
        switch (iInterfaceState)
        {
            case EPVMFNodePrepared:
            case EPVMFNodePaused:
            case EPVMFNodeStarted:
            {
                /* transition to Started */
                oStartPending = false;
                SetState(EPVMFNodeStarted);
                /* Enable Output Ports */
                StartOutputPorts();
                /* Enable remote activity monitoring */
                Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
                for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
                {
                    it->oMonitorForRemoteActivity = true;
                }
                CommandComplete(aCmd, PVMFSuccess);
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
            break;

            default:
            {
                SetState(EPVMFNodeError);
                CommandComplete(aCmd, PVMFErrInvalidState);
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
            break;
        }
    }
    else
    {
        SetState(EPVMFNodeError);
        CommandComplete(aCmd, PVMFErrInvalidState);
        /* Erase the command from the current queue */
        iCurrentCommand.Erase(&iCurrentCommand.front());
    }
}

void PVMFJitterBufferNode::CancelStart()
{
    CancelEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
    CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
    CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
    /* Stop session duration timer */
    iSessionDurationTimer->Stop();
    /* Stop RTCP Timer */
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
        {
            if (it->iRTCPTimer != NULL)
                it->iRTCPTimer->Stop();
        }
    }

    CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
    /* Pause Estimated Server Clock */
    if (iEstimatedServerClock)
    {
        iEstimatedServerClock->Pause();
    }
    if (iRTCPClock)
    {
        iRTCPClock->Pause();
    }
    PVMFJitterBufferNodeCommand aCmd = iCurrentCommand.front();
    oStartPending = false;
    CommandComplete(aCmd, PVMFErrCancelled);
    /* Erase the command from the current queue */
    iCurrentCommand.Erase(&iCurrentCommand.front());
    return;
}

/**
 * Called by the command handler AO to do the node Stop
 */
void PVMFJitterBufferNode::DoStop(PVMFJitterBufferNodeCommand& aCmd)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::DoStop"));
    LogSessionDiagnostics();

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
            /* Stop session duration timer */
            iSessionDurationTimer->Stop();
            /* Stop RTCP Timer */
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
                {
                    if (it->iRTCPTimer != NULL)
                        it->iRTCPTimer->Stop();
                }
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
                it->iRTCPStats.oRTCPByeRecvd = false;
            }
            CancelEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
            /* Pause Estimated Server Clock */
            PVMFStatus aStatus = PVMFSuccess;
            if (iEstimatedServerClock)
            {
                iEstimatedServerClock->Pause();
            }
            if (iRTCPClock)
            {
                iRTCPClock->Pause();
            }
            /* Clear queued messages in ports */
            uint32 i;
            for (i = 0; i < iPortVector.size(); i++)
            {
                PVMFJitterBufferPortParams* portParams = NULL;
                bool bRet = getPortContainer(iPortVector[i], portParams);
                if (bRet)
                {
                    if (portParams->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                    {
                        portParams->iJitterBuffer->ResetJitterBuffer();
                    }
                    portParams->ResetParams();
                }
                iPortVector[i]->ClearMsgQueues();
            }

            if (aStatus == PVMFSuccess)
            {
                /* Reset State Variables */
                oDelayEstablished = false;
                oSessionDurationExpired = false;
                oStopOutputPorts = true;
                oAutoPause = false;
                oStartPending = false;
                iJitterBufferState = PVMF_JITTER_BUFFER_READY;
                iJitterDelayPercent = 0;

                /* transition to Prepared state */
                SetState(EPVMFNodePrepared);
            }
            CommandComplete(iInputCommands, aCmd, aStatus);
        }
        iNonDecreasingClock->Stop();
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/**
 * Called by the command handler AO to do the node Flush
 */
void PVMFJitterBufferNode::DoFlush(PVMFJitterBufferNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            /*
             * the flush is asynchronous.  move the command from
             * the input command queue to the current command, where
             * it will remain until the flush completes.
             */
            int32 err;
            OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
            if (err != OsclErrNone)
            {
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
            iInputCommands.Erase(&aCmd);

            /* Notify all ports to suspend their input */
            for (uint32 i = 0;i < iPortVector.size();i++)
                iPortVector[i]->SuspendInput();

            PVMFStatus status = PVMFSuccess;
            /* Stop Estimated Server Clock */
            if (iEstimatedServerClock)
            {
                if (!(iEstimatedServerClock->Stop()))
                {
                    status = PVMFFailure;
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoFlush: Error - iEstimatedServerClock->Stop failed"));
                }
            }
            CommandComplete(iInputCommands, aCmd, status);
        }
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/**
 * A routine to tell if a flush operation is in progress.
 */
bool PVMFJitterBufferNode::FlushPending()
{
    return (iCurrentCommand.size() > 0
            && iCurrentCommand.front().iCmd == PVMF_JITTER_BUFFER_NODE_FLUSH);
}


/**
 * Called by the command handler AO to do the node Pause
 */
void PVMFJitterBufferNode::DoPause(PVMFJitterBufferNodeCommand& aCmd)
{
    iPauseTime = 0;
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            uint32 currticks = OsclTickCount::TickCount();
            iPauseTime = OsclTickCount::TicksToMsec(currticks);
            SetState(EPVMFNodePaused);
            pauseEstimatedServerClock();
            /* Cancel session duration timer */
            iSessionDurationTimer->Cancel();
            CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
            CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
            }
            iNonDecreasingClock->Pause();
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
        }
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
}

/**
 * Called by the command handler AO to do the Cancel All
 */
void PVMFJitterBufferNode::DoCancelAllCommands(PVMFJitterBufferNodeCommand& aCmd)
{
    /* first cancel the current command if any */
    if (!iCurrentCommand.empty())
    {
        if (iCurrentCommand.front().iCmd == PVMF_JITTER_BUFFER_NODE_PREPARE)
        {
            CancelPrepare();
        }
        else if (iCurrentCommand.front().iCmd == PVMF_JITTER_BUFFER_NODE_START)
        {
            CancelStart();
        }
        else
        {
            OSCL_ASSERT(false);
        }
    }
    /* next cancel all queued commands */
    {
        /* start at element 1 since this cancel command is element 0. */
        while (iInputCommands.size() > 1)
            CommandComplete(iInputCommands, iInputCommands[1], PVMFErrCancelled);
    }

    uint32 i;
    for (i = 0; i < iPortVector.size(); i++)
    {
        PVMFJitterBufferPortParams* portParams = NULL;
        bool bRet = getPortContainer(iPortVector[i], portParams);
        if (bRet)
        {
            if (portParams->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                portParams->iJitterBuffer->ResetJitterBuffer();
            }
            portParams->ResetParams();
        }
        iPortVector[i]->ClearMsgQueues();
    }


    /* finally, report cancel complete.*/
    CommandComplete(iInputCommands, aCmd, PVMFSuccess);
}

/**
 * Called by the command handler AO to do the Cancel single command
 */
void PVMFJitterBufferNode::DoCancelCommand(PVMFJitterBufferNodeCommand& aCmd)
{
    /* extract the command ID from the parameters.*/
    PVMFCommandId id;
    aCmd.PVMFJitterBufferNodeCommandBase::Parse(id);

    /* first check "current" command if any */
    {
        PVMFJitterBufferNodeCommand* cmd = iCurrentCommand.FindById(id);
        if (cmd)
        {
            /* cancel the queued command */
            CommandComplete(iCurrentCommand, *cmd, PVMFErrCancelled);
            /* report cancel success */
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            return;
        }
    }

    /* next check input queue */
    {
        /* start at element 1 since this cancel command is element 0 */
        PVMFJitterBufferNodeCommand* cmd = iInputCommands.FindById(id, 1);
        if (cmd)
        {
            /* cancel the queued command */
            CommandComplete(iInputCommands, *cmd, PVMFErrCancelled);
            /* report cancel success */
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            return;
        }
    }
    /* if we get here the command isn't queued so the cancel fails */
    CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
}

/**
 * Called by the command handler AO on RTCP timer expiry to
 * generate RTCP reports
 */
PVMFStatus PVMFJitterBufferNode::GenerateRTCPRR(PVMFJitterBufferPortParams* pFeedbackPort)
{
    PVMFJitterBufferPortParams* pInputPort;
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    int32 errcode = PVMFJitterBufferNodeRTCPRRGenerationFailed;

    // find the input port
    if (!LocateInputPortForFeedBackPort(pFeedbackPort, pInputPort))
    {
        return PVMFFailure;
    }

    // check the port status
    if (pFeedbackPort->iPort->IsConnected())
    {
        if (pFeedbackPort->iPort->IsOutgoingQueueBusy() == false)
        {
            PVMFStatus status = ComposeAndSendFeedBackPacket(pInputPort, pFeedbackPort);
            if ((status != PVMFSuccess) && (status != PVMFErrNoMemory))
            {
                PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::GenerateRTCPRR: ComposeAndSendFeedBackPacket failed", this));
                ReportErrorEvent(PVMFErrProcessing, NULL, &eventuuid, &errcode);
                return PVMFFailure;
            }
        }
        else
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::GenerateRTCPRR: Feedback Port - Outgoing queue full"));
            //Don't do ReportErrorEvent because the case where RTCP reader reports
            //are backed up is not sufficient reason to report a critical error to
            //the application layer.
            return PVMFFailure;
        }
    }

    /* Reschedule the RTCP timer for the next interval */
    pFeedbackPort->iRTCPIntervalInMicroSeconds = CalcRtcpInterval(pFeedbackPort);
    pFeedbackPort->iRTCPTimer->RunIfNotReady(pFeedbackPort->iRTCPIntervalInMicroSeconds);
    return PVMFSuccess;
}

bool
PVMFJitterBufferNode::LocateFeedBackPort(PVMFJitterBufferPortParams*& aInputPortParamsPtr,
        PVMFJitterBufferPortParams*& aFeedBackPortParamsPtr)
{
    uint32 inputPortId = aInputPortParamsPtr->id;

    /* Feedback port id must be inputPortId + 2 */

    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
    {
        if ((it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK) &&
                ((int32)it->id == (int32)inputPortId + 2))
        {
            aFeedBackPortParamsPtr = it;
            return true;
        }
    }
    return false;
}

uint32
PVMFJitterBufferNode::CalcRtcpInterval(PVMFJitterBufferPortParams* pFeedbackPort)
{
    float interval;

#if RTCP_FIXED_INTERVAL_MODE
    OSCL_UNUSED_ARG(pFeedbackPort);
    interval = DEFAULT_RTCP_INTERVAL_SEC;
#else
    float rtcp_min_time = pFeedbackPort->iInitialRtcp ? (float)DEFAULT_RTCP_INTERVAL_SEC / 2.0 :
                          (float)DEFAULT_RTCP_INTERVAL_SEC;

    if (pFeedbackPort->RtcpBwConfigured && (pFeedbackPort->RR > 0))
    {
        float divisor = (float)pFeedbackPort->RR;
        if (pFeedbackPort->RR > pFeedbackPort->RS)
        {
            divisor = (float)pFeedbackPort->RS;
        }
        interval = pFeedbackPort->avg_rtcp_size * 8 *
                   ((float)pFeedbackPort->numSenders + 1) / divisor;

        if (interval < rtcp_min_time)
        {
            interval = rtcp_min_time;
        }
    }
    else
    {
        interval = rtcp_min_time;
    }
#endif

    // generate a random number on [0, 1000)
    uint32 n = iRandGen.Rand() % 1000;

    // map the number onto the range [0.5, 1.5)
    float window = 0.5 + ((float)n) / 1000.0;

    // generate the actual interval, in seconds
    float interval_scaled = interval * window / 1.21828;

    // return the interval in microseconds
    return (uint32)(interval_scaled * 1000000);
}

bool
PVMFJitterBufferNode::LocateInputPortForFeedBackPort(PVMFJitterBufferPortParams*& aFeedBackPortParamsPtr,
        PVMFJitterBufferPortParams*& aInputPortParamsPtr)
{
    int32 feedBackPortId = aFeedBackPortParamsPtr->id;

    /* Input port id must be feedBackPortId - 2 */

    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
    {
        if ((it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT) &&
                ((feedBackPortId - 2) == it->id))
        {
            aInputPortParamsPtr = it;
            return true;
        }
    }
    return false;
}
/*
 * Called on incoming feedback port activity
 */
PVMFStatus
PVMFJitterBufferNode::ProcessIncomingRTCPReport(PVMFSharedMediaMsgPtr& aMsg,
        PVMFJitterBufferPortParams* aPortParamsPtr)
{
    RTCP_Decoder rtcpDec;
    RTCP_SR rtcpSR;
    RTCP_BYE rtcpBye;
    RTCP_Decoder::Error_t retval;
    int32 max_array_size = MAX_RTCP_SOURCES;
    RTCPPacketType array_of_packet_types[MAX_RTCP_SOURCES];
    OsclMemoryFragment array_of_packets[MAX_RTCP_SOURCES];
    int32 filled_size = 0;

    PVMFSharedMediaDataPtr rtcpDataIn;
    convertToPVMFMediaData(rtcpDataIn, aMsg);

    uint32 numFrags = rtcpDataIn->getNumFragments();

    //Process each RTCP packet.
    //Typically, only one is received at a time.
    for (uint32 i = 0; i < numFrags; i++)
    {
        OsclRefCounterMemFrag memfrag;

        //Get the next memory fragment from the media message.
        if (rtcpDataIn->getMediaFragment(i, memfrag) == false)
        {
            return PVMFFailure;
        }

        //Get the pointer to the packet.
        OsclMemoryFragment receivedMsg = memfrag.getMemFrag();

        /* Find out what type of RTCP packet we have */
        //This populates the variables "filled_size", "array_of_packet_types", and "array_of_packets"
        //by breaking up compound RTCP packets into individual reports.
        rtcpDec.scan_compound_packet(receivedMsg,
                                     max_array_size,
                                     filled_size,
                                     array_of_packet_types,
                                     array_of_packets);

        // update packet size averages - we treat the compound packet
        // as a single packet
        aPortParamsPtr->avg_rtcp_size = (receivedMsg.len + 15.0 * aPortParamsPtr->avg_rtcp_size) / 16.0;

        //Process each individual report.
        for (int32 ii = 0; ii < filled_size; ii++)
        {
            /* Use the appropriate decoder */

            //If the RTCP type indicates a Sender Report...
            if (SR_RTCP_PACKET == array_of_packet_types[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFJitterBufferNode::ProcessIncomingRTCPReport - Sender Report"));

                //Decode the Sender Report.
                retval = rtcpDec.DecodeSR(array_of_packets[ii], rtcpSR);
                if (RTCP_Decoder::FAIL == retval)
                {
                    return PVMFFailure;
                }

                aPortParamsPtr->iRTCPStats.lastSenderReportTS =
                    (rtcpSR.NTP_timestamp_high << 16) |
                    ((rtcpSR.NTP_timestamp_low >> 16) & 0x0000ffff);

                /*
                 * Get RTCP Recv Time in milliseconds
                 */
                uint32 srRecvTime;
                bool overflowFlag = false;
                iRTCPClock->GetCurrentTime32(srRecvTime, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);

                aPortParamsPtr->iRTCPStats.lastSenderReportRecvTime = srRecvTime;

                //Save the NTP and RTP timestamps for later calculations...
                aPortParamsPtr->iRTCPStats.lastSenderReportNTP =
                    (((uint64)rtcpSR.NTP_timestamp_high) << 32) + (uint64)rtcpSR.NTP_timestamp_low;
                aPortParamsPtr->iRTCPStats.lastSenderReportRTP = rtcpSR.RTP_timestamp;

                if (iBroadCastSession && !iRTCPBcastAVSyncProcessed)
                {
                    bool ret = ProcessRTCPSRforAVSync();
                    if (ret == false)
                    {
                        // No need to return error as perhaps there's not enough information yet
                        // to attempt a/v sync
                        return PVMFSuccess;
                    }
                }

            }

            //If the RTCP type is BYE, set the end-of-stream flag.
            if (BYE_RTCP_PACKET == array_of_packet_types[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFJitterBufferNode::ProcessIncomingRTCPReport - BYE"));

                PVMF_JBNODE_LOG_RTCP_ERR((0, "RTCP_BYE_RECVD: Mime=%s", aPortParamsPtr->iMimeType.get_cstr()));

                //for live streams, treat RTCP BYE as EOS
                if (iPlayStopTimeAvailable == false)
                {
                    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "USING RTCP_BYE TO TRIGGER EOS: Mime=%s", aPortParamsPtr->iMimeType.get_cstr()));
                    PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "USING RTCP_BYE TO TRIGGER EOS: Mime=%s", aPortParamsPtr->iMimeType.get_cstr()));
                    PVMF_JBNODE_LOG_RTCP_ERR((0, "USING RTCP_BYE TO TRIGGER EOS: Mime=%s", aPortParamsPtr->iMimeType.get_cstr()));
                    oSessionDurationExpired = true;
                    CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                    /* Cancel clock update notifications */
                    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
                    for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
                    {
                        CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
                        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                        {
                            if (it->iJitterBuffer != NULL)
                            {
                                it->iJitterBuffer->CancelServerClockNotificationUpdates();
                            }
                            else
                            {
                                OSCL_ASSERT(false);
                            }
                        }
                    }
                    /* Pause Estimated server clock & RTCP Clock */
                    iEstimatedServerClock->Pause();
                    iRTCPClock->Pause();
                    if (IsAdded())
                    {
                        RunIfNotReady();
                    }
                }

                retval = rtcpDec.DecodeBYE(array_of_packets[ii], rtcpBye);
                if (RTCP_Decoder::FAIL == retval)
                {
                    return PVMFFailure;
                }
                /* The packet is a RTCP BYE, set the end of stream flag */
                else if (retval == RTCP_Decoder::RTCP_SUCCESS)
                {
                    if (aPortParamsPtr->iRTCPStats.oRTCPByeRecvd == false)
                    {
                        PVMFJitterBufferPortParams* inputPortParamsPtr = NULL;
                        if (LocateInputPortForFeedBackPort(aPortParamsPtr,
                                                           inputPortParamsPtr) == false)
                        {
                            return PVMFFailure;
                        }
                        aPortParamsPtr->iRTCPStats.oRTCPByeRecvd = true;

                        PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
                        int32 infocode = PVMFJitterBufferNodeRTCPBYERecvd;
                        ReportInfoEvent(PVMFInfoRemoteSourceNotification,
                                        (OsclAny*)(inputPortParamsPtr->iMimeType.get_cstr()),
                                        &eventuuid,
                                        &infocode);
                        PVMF_JBNODE_LOG_RTCP_ERR((0, "RTCP_BYE_PROCESSED: Mime=%s", aPortParamsPtr->iMimeType.get_cstr()));
                    }
                }
            }

            //All other RTCP types (Receiver Reports) are ignored.

            aPortParamsPtr->iRTCPStats.oSRRecvd = true;
        }
    }
    return PVMFSuccess;
}




PVMFStatus
PVMFJitterBufferNode::ComposeAndSendFeedBackPacket(PVMFJitterBufferPortParams*& aInputPortParamsPtr,
        PVMFJitterBufferPortParams*& aFeedBackPortParamsPtr)
{
    uint32 senderSSRC;
    RTCP_Encoder rtcpEncode;
    RTCP_ReportBlock *reportBlock = NULL;
    RTCP_RR *pRR = NULL;
    OsclExclusivePtr<RTCP_RR> rtcpRRAutoPtr;

    pRR = OSCL_NEW(RTCP_RR, (1));

    if (NULL == pRR)
    {
        return PVMFErrNoMemory;
    }
    rtcpRRAutoPtr.set(pRR);

    reportBlock = pRR->get_report_block(0);
    if (NULL == reportBlock)
    {
        return PVMFErrNoMemory;
    }

    /* Get Jitter Buffer Stats from RTP port */
    PVMFJitterBufferStats jbStats =
        aInputPortParamsPtr->iJitterBuffer->getJitterBufferStats();

    /* Get InterArrivalJitter from RTP port */
    uint32 interArrivalJitter =
        aInputPortParamsPtr->iJitterBuffer->getInterArrivalJitter();

    uint32 sourceSSRC32 = jbStats.ssrc;
    senderSSRC = (sourceSSRC32 >> 10) | (sourceSSRC32 << 22);

    pRR->senderSSRC = senderSSRC;
    rtcpEncode.SetSSRC(senderSSRC);
    reportBlock->sourceSSRC = sourceSSRC32;

    /* Compute packet loss fraction */
    if (aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumReceivedUptoThisRR == 0)
    {
        aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumReceivedUptoThisRR =
            jbStats.seqNumBase;
    }
    if (jbStats.maxSeqNumReceived -
            aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumReceivedUptoThisRR)
    {
        reportBlock->fractionLost =
            (int8)(((jbStats.totalPacketsLost - aFeedBackPortParamsPtr->iRTCPStats.packetLossUptoThisRR) * 256) /
                   (jbStats.maxSeqNumReceived - aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumReceivedUptoThisRR));
    }
    else
    {
        reportBlock->fractionLost = 0;
    }

    reportBlock->cumulativeNumberOfPacketsLost = jbStats.totalPacketsLost;
    reportBlock->highestSequenceNumberReceived = jbStats.maxSeqNumReceived;
    reportBlock->interarrivalJitter = interArrivalJitter;
    reportBlock->lastSR = aFeedBackPortParamsPtr->iRTCPStats.lastSenderReportTS;

    if (aFeedBackPortParamsPtr->iRTCPStats.oSRRecvd)
    {
        uint32 currRRGenTime;
        bool overflowFlag = false;

        iRTCPClock->GetCurrentTime32(currRRGenTime, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);

        uint32 lastSenderReportRecvTime = (uint32)
                                          aFeedBackPortParamsPtr->iRTCPStats.lastSenderReportRecvTime;

        uint32 delaySinceLastSR64 =
            (currRRGenTime - lastSenderReportRecvTime);

        uint32 delaySinceLastSR32 = delaySinceLastSR64;

        reportBlock->delaySinceLastSR = (delaySinceLastSR32 << 16) / 1000;

        aFeedBackPortParamsPtr->iRTCPStats.lastRRGenTime = currRRGenTime;
    }

    /* Update variables for the next RR cycle */
    aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumReceivedUptoThisRR =
        jbStats.maxSeqNumReceived;
    aFeedBackPortParamsPtr->iRTCPStats.packetLossUptoThisRR =
        jbStats.totalPacketsLost;

    PVMFJitterBufferPort* jbRTCPPort =
        (PVMFJitterBufferPort*)aFeedBackPortParamsPtr->iPort;
    PVMFSharedMediaDataPtr rtcpOut;
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImpl;
    PVMFRTCPMemPool* rtcpBufAlloc = aFeedBackPortParamsPtr->iRTCPTimer->getRTCPBuffAlloc();
    if (!rtcpBufAlloc->iMediaDataMemPool)
    {
        return PVMFErrNoMemory;
    }
    int32 err;

    OSCL_TRY(err,
             mediaDataImpl = rtcpBufAlloc->getMediaDataImpl(MAX_RTCP_BLOCK_SIZE);
             rtcpOut = PVMFMediaData::createMediaData(mediaDataImpl,
                       (rtcpBufAlloc->iMediaDataMemPool));
            );

    if (err != OsclErrNone)
    {
        return PVMFErrNoMemory;
    }

    /* Retrieve memory fragment to write to */
    OsclRefCounterMemFrag refCtrMemFragOut;
    rtcpOut->getMediaFragment(0, refCtrMemFragOut);

    OsclMemoryFragment memFrag = refCtrMemFragOut.getMemFrag();
    memFrag.len = MAX_RTCP_BLOCK_SIZE;

    RTCP_APP *appPtr = NULL;
    RTCP_APP App;

    PVMF_JBNODE_LOG_RTCP((0, "RTCP_PKT: Mime=%s, MaxSNRecvd=%d, MaxTSRecvd=%d, MaxSNRet=%d, MaxTSRet=%d", aInputPortParamsPtr->iMimeType.get_cstr(), jbStats.maxSeqNumReceived, jbStats.maxTimeStampRegistered, jbStats.lastRetrievedSeqNum, jbStats.maxTimeStampRetrieved));
    /*
     * If Rate Adaptation is enabled and we have received some RTP packets, then send NADU APP packet,
     * if frequency criteria is met
     */
    if (aInputPortParamsPtr->oRateAdaptation && (jbStats.totalNumPacketsReceived > 0))
    {
        aInputPortParamsPtr->iRateAdaptationRTCPRRCount++;
        if (aInputPortParamsPtr->iRateAdaptationRTCPRRCount ==
                aInputPortParamsPtr->iRateAdaptationFeedBackFrequency)
        {
            oscl_memcpy(App.type, PSS0_APP_RTCP_NAME, oscl_strlen(PSS0_APP_RTCP_NAME));
            App.ssrc = senderSSRC;
            App.subtype = RTCP_NADU_APP_SUBTYPE;
            App.pss0_app_data.sourcessrc = sourceSSRC32;
            PVMFTimestamp converted_ts = 0;
            //set playoutdelay to 0xffff by default, if JB is empty we will use this
            uint32 diff32 = RTCP_NADU_APP_DEFAULT_PLAYOUT_DELAY;
            uint32 clientClock32 = 0;
            uint32 timebase32 = 0;
            bool overflowFlag = false;

            if (iClientPlayBackClock != NULL)
                iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag,
                                                       PVMF_MEDIA_CLOCK_MSEC,
                                                       timebase32);
            if (jbStats.currentOccupancy > 0)
            {
                PVMFTimestamp tsOfNextPacketToBeDecoded = jbStats.maxTimeStampRetrievedWithoutRTPOffset;
                tsOfNextPacketToBeDecoded =
                    aInputPortParamsPtr->iJitterBuffer->peekNextElementTimeStamp();

                uint32 in_wrap_count = 0;
                /*
                 * Convert Time stamp to milliseconds
                 */
                aInputPortParamsPtr->mediaClockConverter.set_clock(tsOfNextPacketToBeDecoded, in_wrap_count);
                converted_ts =
                    aInputPortParamsPtr->mediaClockConverter.get_converted_ts(1000);

                //ts should be ahead of clock
                //if not we are falling behind on one track, so set playout delay to zero
                diff32 = 0;
                bool clkEarly =
                    PVTimeComparisonUtils::IsEarlier(clientClock32,
                                                     converted_ts,
                                                     diff32);
                if (clkEarly == false)
                {
                    diff32 = 0;
                }
            }
            PVMF_JBNODE_LOG_RTCP((0, "RTCP_PKT: Mime=%s, RTP_TS=%d, C_CLOCK=%d, DIFF=%d, RE-BUF=%d", aInputPortParamsPtr->iMimeType.get_cstr(), converted_ts, clientClock32, diff32, oDelayEstablished));
            App.pss0_app_data.playoutdelayinms = (uint16)diff32;
            App.pss0_app_data.nsn = (jbStats.lastRetrievedSeqNum + 1);
            if (0 == jbStats.lastRetrievedSeqNum)
            {
                App.pss0_app_data.nsn = jbStats.seqNumBase;
            }
            App.pss0_app_data.nun = RTCP_NADU_APP_DEFAULT_NUN;

            uint32 fbsInBytes = 0;
            if (jbStats.packetSizeInBytesLeftInBuffer < aInputPortParamsPtr->iRateAdaptationFreeBufferSpaceInBytes)
            {
                fbsInBytes =
                    (aInputPortParamsPtr->iRateAdaptationFreeBufferSpaceInBytes - jbStats.packetSizeInBytesLeftInBuffer);
            }
            PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::ComposeAndSendFeedBackPacket: Total=%d, Occ=%d, freebufferspace = %d",
                                 aInputPortParamsPtr->iRateAdaptationFreeBufferSpaceInBytes,
                                 jbStats.packetSizeInBytesLeftInBuffer,
                                 fbsInBytes));
            App.pss0_app_data.freebufferspace = (fbsInBytes) / 64;
            aInputPortParamsPtr->iRateAdaptationRTCPRRCount = 0;
            appPtr = &App;
            PVMF_JBNODE_LOG_RTCP((0, "NADU_PKT: Mime=%s, PDelay=%d, FBS_BYTES=%d, FBS=%d, NSN=%d",
                                  aInputPortParamsPtr->iMimeType.get_cstr(),
                                  App.pss0_app_data.playoutdelayinms,
                                  fbsInBytes,
                                  App.pss0_app_data.freebufferspace,
                                  App.pss0_app_data.nsn));
        }
    }
    if (rtcpEncode.EncodeCompoundRR(*pRR,
                                    memFrag,
                                    appPtr) != RTCP_Encoder::RTCP_SUCCESS)
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::ComposeAndSendFeedBackPacket: EncodeCompoundRR failed", this));
        return PVMFFailure;
    }
    rtcpOut->setMediaFragFilledLen(0, memFrag.len);


    // update average packet length - treat compound packets as single
    aFeedBackPortParamsPtr->avg_rtcp_size = (memFrag.len + 15.0 * aFeedBackPortParamsPtr->avg_rtcp_size) / 16.0;


    PVMFSharedMediaMsgPtr rtcpMsg;
    convertToPVMFMediaMsg(rtcpMsg, rtcpOut);

    PVMFStatus status = jbRTCPPort->QueueOutgoingMsg(rtcpMsg);
    if (status != PVMFSuccess)
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::ComposeAndSendFeedBackPacket: Error - output queue busy", this));
    }
    PVMF_JBNODE_LOG_RTCP((0, "PVMFJitterBufferNode - SENT RTCP RR - TrackMimeType = %s", aInputPortParamsPtr->iMimeType.get_cstr()));
    aFeedBackPortParamsPtr->iInitialRtcp = false;
    return status;
}

bool PVMFJitterBufferNode::ProcessRTCPSRforAVSync()
{
    // The following criteria must hold before the RTCP SRs can be processed for a/v sync
    // a) The Jitter Buffers of all tracks have received at least one packet
    // b) At least one RTCP report has been received for each track
    // c) The wall clock value of the RTCP SRs is not zero

    // temporary vectors to save the indexes of rtcp and input ports
    Oscl_Vector<uint32, PVMFJitterBufferNodeAllocator> indexesRTCPPort;
    Oscl_Vector<uint32, PVMFJitterBufferNodeAllocator> indexesInputPort;

    // Check the criteria
    for (uint32 ii = 0; ii < iPortParamsQueue.size(); ii++)
    {
        PVMFJitterBufferPortParams& portParams = iPortParamsQueue[ii];
        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            // make sure the JB has received data
            uint32 tsOffset = 0;
            if (portParams.iJitterBuffer->GetRTPTimeStampOffset(tsOffset) == false)
                return false;

            // Save the index for later processing
            int32 err = OsclErrNone;
            OSCL_TRY(err, indexesInputPort.push_back(ii));
            if (err != OsclErrNone)
                return false;
        }
        else
            if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
            {
                // make sure we have an RTCP report available
                if (portParams.iRTCPStats.lastSenderReportRecvTime == OSCL_STATIC_CAST(uint64, 0))
                    return false;

                // RTCP SR must have valid clock wall vale
                if (portParams.iRTCPStats.lastSenderReportNTP == OSCL_STATIC_CAST(uint64, 0))
                    return false;

                // Save the index for later processing
                int32 err = OsclErrNone;
                OSCL_TRY(err, indexesRTCPPort.push_back(ii));
                if (err != OsclErrNone)
                    return false;

            }
    }

    // for each feedback port there must be an input port
    OSCL_ASSERT(indexesRTCPPort.size() == indexesInputPort.size());

    // temporary vector to save the calculated init ntp for each track
    Oscl_Vector<uint64, PVMFJitterBufferNodeAllocator> initNtpTracks;

    // temporary vector to save the calculated rtp timebase of each track
    Oscl_Vector<uint32, PVMFJitterBufferNodeAllocator> RTPTBArray;

    // Initialize temporary vectors
    int32 err = OsclErrNone;
    OSCL_TRY(err, initNtpTracks.reserve(indexesRTCPPort.size()));
    if (err != OsclErrNone)
        return false;

    OSCL_TRY(err, RTPTBArray.push_back(indexesRTCPPort.size()));
    if (err != OsclErrNone)
        return false;

    for (uint32 tt = 0; tt < indexesRTCPPort.size(); tt++)
    {
        initNtpTracks.push_back(0);
        RTPTBArray.push_back(0);
    }


    // Find the track whose first rtp packet correspond to the smallest NTP
    uint32 lowestNTPIndex = 0;
    uint64 lowestNTP = 0;
    for (uint32 jj = 0; jj < indexesRTCPPort.size(); jj++)
    {
        PVMFJitterBufferPortParams& inputPortParams = iPortParamsQueue[indexesInputPort[jj]];
        PVMFJitterBufferPortParams& rtcpPortParams = iPortParamsQueue[indexesRTCPPort[jj]];

        uint32 firstRTP;
        inputPortParams.iJitterBuffer->GetRTPTimeStampOffset(firstRTP);
        uint32 timescale = inputPortParams.timeScale;
        uint32 srRTP = rtcpPortParams.iRTCPStats.lastSenderReportRTP;
        uint64 srNTP = rtcpPortParams.iRTCPStats.lastSenderReportNTP;

        uint32 deltaRTP = 0;
        if (srRTP >= firstRTP)
        {
            deltaRTP = srRTP - firstRTP;
        }
        else
        {
            deltaRTP = firstRTP - srRTP;
        }

        uint64 deltaRTPInNTPFormat = ((uint64) deltaRTP / (uint64)timescale) << 32;
        deltaRTPInNTPFormat += ((uint64) deltaRTP % (uint64)timescale) * (uint64)0xFFFFFFFF / (uint64)timescale;

        uint64 initNTP = 0;
        if (srRTP >= firstRTP)
        {
            initNTP = srNTP - deltaRTPInNTPFormat;
        }
        else
        {
            initNTP = srNTP + deltaRTPInNTPFormat;
        }


        if (jj == 0)
        {
            lowestNTPIndex = jj;
            lowestNTP = initNTP;
        }
        else
            if (initNTP < lowestNTP)
            {
                lowestNTPIndex = jj;
                lowestNTP = initNTP;
            }

        // Save the reference ntp value
        initNtpTracks[jj] = initNTP;

        PVMF_JBNODE_LOG_RTCP_AVSYNC((0,
                                     "PVMFJitterBufferNode::ProcessRTCPSRforAVSync(): srRTP=%d, firstRTP=%d, timescale=%d srNTPHigh=0x%x, srNTPLow=0x%x initNTPHigh=0x%x initNTPLow=0x%x deltaRTPHigh=0x%x deltaRTPLow=0x%x",
                                     srRTP, firstRTP, timescale, Oscl_Int64_Utils::get_uint64_upper32(srNTP), Oscl_Int64_Utils::get_uint64_lower32(srNTP),
                                     Oscl_Int64_Utils::get_uint64_upper32(initNTP), Oscl_Int64_Utils::get_uint64_lower32(initNTP),
                                     Oscl_Int64_Utils::get_uint64_upper32(deltaRTPInNTPFormat), Oscl_Int64_Utils::get_uint64_lower32(deltaRTPInNTPFormat)));

    }


    // Calculate the new timebase for all tracks
    for (uint32 kk = 0; kk < indexesRTCPPort.size(); kk++)
    {
        PVMFJitterBufferPortParams& inputPortParams = iPortParamsQueue[indexesInputPort[kk]];
        uint32 firstRTP;
        inputPortParams.iJitterBuffer->GetRTPTimeStampOffset(firstRTP);

        if (kk == lowestNTPIndex)
        {
            // Just set the RTP TB to the first rtp packet
            RTPTBArray[kk] = firstRTP;
        }
        else
        {
            uint64 initNTP = initNtpTracks[kk];
            uint32 timescale = inputPortParams.timeScale;

            OSCL_ASSERT(lowestNTP <= initNTP);

            uint64 deltaNTP = initNTP - lowestNTP;
            uint32 deltaNTPInRTPUnits = ((deltaNTP * (uint64)timescale) + (uint64)0x80000000) >> 32;
            uint32 rtpTimeBase = firstRTP - deltaNTPInRTPUnits;
            RTPTBArray[kk] = rtpTimeBase;
        }
    }

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    // Log parameters
    for (uint32 mm = 0; mm < indexesRTCPPort.size(); mm++)
    {
        PVMFJitterBufferPortParams& inputPortParams = iPortParamsQueue[indexesInputPort[mm]];
        PVMFJitterBufferPortParams& rtcpPortParams = iPortParamsQueue[indexesRTCPPort[mm]];

        uint32 firstRTP;
        inputPortParams.iJitterBuffer->GetRTPTimeStampOffset(firstRTP);
        uint32 timescale = inputPortParams.timeScale;
        uint32 srRTP = rtcpPortParams.iRTCPStats.lastSenderReportRTP;
        uint64 srNTP = rtcpPortParams.iRTCPStats.lastSenderReportNTP;
        int32 delta = ((firstRTP - RTPTBArray[mm]) * 1000) / timescale;
        uint32 srNTPHigh = Oscl_Int64_Utils::get_uint64_upper32(srNTP);
        srNTP = srNTP & uint64(0xffffffff);
        srNTP *= uint64(1000000);
        srNTP += uint64(500000);
        srNTP = srNTP / uint64(0xffffffff);
        uint32 srNTPLow = Oscl_Int64_Utils::get_uint64_lower32(srNTP);

        PVMF_JBNODE_LOG_RTCP_AVSYNC((0,
                                     "Stream %d: mime=%s timeScale=%uHz firstTS=%u RTCP.RTP=%u RTCP.NTP=%u.%06u newTB=%u delta=%dms\n",
                                     mm,
                                     inputPortParams.iMimeType.get_cstr(),
                                     timescale,
                                     firstRTP,
                                     srRTP,
                                     srNTPHigh,
                                     srNTPLow,
                                     RTPTBArray[mm],
                                     delta
                                    )
                                   );

    }
#endif

    // Adjust the RTP TB
    for (uint32 ll = 0; ll < indexesInputPort.size(); ll++)
    {
        PVMFJitterBufferPortParams& inputPortParams = iPortParamsQueue[indexesInputPort[ll]];
        inputPortParams.iJitterBuffer->SetRTPTimeStampOffset(RTPTBArray[ll]);
    }

    //Notify SM plugin that RTP TB data is available for PVR purposes
    // No need to create a public class to publish the format of the information sent in this event
    // Just define this structure internally. The only client of this event is the SM broadcast
    // plugin, so it's the only component that needs to be aware of this format
    struct RTPTBInfoEventData
    {
        const PvmfMimeString* mimeType;
        uint32 rtpTB;
    };

    for (uint32 nn = 0; nn < indexesInputPort.size(); nn++)
    {
        PVMFJitterBufferPortParams& inputPortParams = iPortParamsQueue[indexesInputPort[nn]];
        RTPTBInfoEventData infoData;
        infoData.mimeType = &(inputPortParams.iMimeType);
        infoData.rtpTB = RTPTBArray[nn];
        ReportInfoEvent(PVMFJitterBufferNodeRTCPDataProcessed, (OsclAny*)(&infoData));
    }




    iRTCPBcastAVSyncProcessed = true;
    return true;
}




/////////////////////////////////////////////////////
// Event reporting routines.
/////////////////////////////////////////////////////
void PVMFJitterBufferNode::SetState(TPVMFNodeInterfaceState s)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode:SetState %d", s));
    PVMFNodeInterface::SetState(s);
}

void PVMFJitterBufferNode::ReportErrorEvent(PVMFEventType aEventType,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode:NodeErrorEvent Type %d Data %d"
                          , aEventType, aEventData));

    if (aEventUUID && aEventCode)
    {
        PVMFBasicErrorInfoMessage* eventmsg;
        PVMF_JITTER_BUFFER_NEW(NULL, PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL), eventmsg);
        PVMFAsyncEvent asyncevent(PVMFErrorEvent,
                                  aEventType,
                                  NULL,
                                  OSCL_STATIC_CAST(PVInterface*, eventmsg),
                                  aEventData,
                                  NULL,
                                  0);
        PVMFNodeInterface::ReportErrorEvent(asyncevent);
        eventmsg->removeRef();
    }
    else
    {
        PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData);
    }
}

void PVMFJitterBufferNode::ReportInfoEvent(PVMFEventType aEventType,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode:NodeInfoEvent Type %d Data %d"
                         , aEventType, aEventData));

    if (aEventType == PVMFInfoBufferingStatus)
    {
        uint8 localbuffer[8];
        oscl_memset(localbuffer, 0, 8);
        localbuffer[0] = 1;
        oscl_memcpy(&localbuffer[4], &iJitterDelayPercent, sizeof(uint32));
        PVMFAsyncEvent asyncevent(PVMFInfoEvent,
                                  aEventType,
                                  NULL,
                                  NULL,
                                  aEventData,
                                  localbuffer,
                                  8);
        PVMFNodeInterface::ReportInfoEvent(asyncevent);
    }
    else if (aEventType == PVMFJitterBufferNodeStreamThinningRecommended)
    {
        uint8 localbuffer[12];
        oscl_memset(localbuffer, 0, 12);
        localbuffer[0] = 1;
        oscl_memcpy(&localbuffer[4], &iUnderFlowEndPacketTS, sizeof(uint32));
        oscl_memcpy(&localbuffer[8], &iEstimatedBitRate, sizeof(uint32));
        PVMFAsyncEvent asyncevent(PVMFInfoEvent,
                                  aEventType,
                                  NULL,
                                  NULL,
                                  aEventData,
                                  localbuffer,
                                  12);
        PVMFNodeInterface::ReportInfoEvent(asyncevent);
    }
    else if (aEventUUID && aEventCode)
    {
        PVMFBasicErrorInfoMessage* eventmsg;
        PVMF_JITTER_BUFFER_NEW(NULL, PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL), eventmsg);
        PVMFErrorInfoMessageInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFErrorInfoMessageInterface*, eventmsg);
        PVMFAsyncEvent asyncevent(PVMFInfoEvent,
                                  aEventType,
                                  NULL,
                                  OSCL_STATIC_CAST(PVInterface*, interimPtr),
                                  aEventData,
                                  NULL,
                                  0);
        PVMFNodeInterface::ReportInfoEvent(asyncevent);
        eventmsg->removeRef();
    }
    else
    {
        PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData);
    }
}

/////////////////////////////////////////////////////
// Port Processing routines
/////////////////////////////////////////////////////
void PVMFJitterBufferNode::QueuePortActivity(PVMFJitterBufferPortParams* aPortParams,
        const PVMFPortActivity &aActivity)
{
    OSCL_UNUSED_ARG(aPortParams);
    OSCL_UNUSED_ARG(aActivity);

    if (IsAdded())
    {
        /*
         * wake up the AO to process the port activity event.
         */
        RunIfNotReady();
    }
}

void PVMFJitterBufferNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::PortActivity: port=0x%x, type=%d",
                         aActivity.iPort, aActivity.iType));

    PVMFJitterBufferPortParams* portParamsPtr = NULL;
    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aActivity.iPort);
    portParamsPtr = jbPort->iPortParams;

    if (aActivity.iType != PVMF_PORT_ACTIVITY_DELETED)
    {
        if (portParamsPtr == NULL)
        {
            ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aActivity.iPort));
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::HandlePortActivity - getPortContainer Failed", this));
            return;
        }
    }

    /*
     * A port is reporting some activity or state change.  This code
     * figures out whether we need to queue a processing event
     * for the AO, and/or report a node event to the observer.
     */

    switch (aActivity.iType)
    {
        case PVMF_PORT_ACTIVITY_CREATED:
            /*
             * Report port created info event to the node.
             */
            ReportInfoEvent(PVMFInfoPortCreated, (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_DELETED:
            /*
             * Report port deleted info event to the node.
             */
            ReportInfoEvent(PVMFInfoPortDeleted, (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_CONNECT:
            //nothing needed.
            break;

        case PVMF_PORT_ACTIVITY_DISCONNECT:
        {
            /*
             * flush the associated jitter buffer if a port is disconnected.
             */
            if (portParamsPtr->iJitterBuffer != NULL)
            {
                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJitterBufferNode::HandlePortActivity - CancelDeallocationNotifications"));
                for (uint32 i = 0; i < iPortParamsQueue.size(); i++)
                {
                    if (iPortParamsQueue[i].iBufferAlloc != NULL)
                    {
                        iPortParamsQueue[i].iBufferAlloc->CancelFreeMemoryAvailableCallback();
                        iPortParamsQueue[i].iBufferAlloc->removeRef();
                        iPortParamsQueue[i].iBufferAlloc = NULL;
                    }
                }
                portParamsPtr->iJitterBuffer->FlushJitterBuffer();
            }
        }
        break;

        case PVMF_PORT_ACTIVITY_OUTGOING_MSG:
        {
            if (portParamsPtr->oProcessOutgoingMessages)
            {
                /*
                 * An outgoing message was queued on this port.
                 * All ports have outgoing messages
                 * in this node
                 */
                QueuePortActivity(portParamsPtr, aActivity);
            }
        }
        break;

        case PVMF_PORT_ACTIVITY_INCOMING_MSG:
        {
            /*
             * An outgoing message was queued on this port.
             * Only input and feedback ports have incoming messages
             * in this node
             */
            int32 portTag = portParamsPtr->tag;
            switch (portTag)
            {
                case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
                case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
                    if (portParamsPtr->oProcessIncomingMessages)
                    {
                        QueuePortActivity(portParamsPtr, aActivity);
                    }
                    break;

                default:
                    OSCL_ASSERT(false);
                    break;
            }
        }
        break;

        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_BUSY:
        {
            int32 portTag = portParamsPtr->tag;
            switch (portTag)
            {
                case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
                    /*
                     * We typically use incoming port's outgoing q
                     * only in case of 3GPP streaming, wherein we
                     * send firewall packets. If it is busy, it does
                     * not stop us from registering incoming data pkts.
                     * so do nothing.
                     */
                    break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT:
                {
                    /*
                     * This implies that this output port cannot accept any more
                     * msgs on its outgoing queue. This would usually imply that
                     * the corresponding input port must stop processing messages,
                     * however in case of jitter buffer the input and output ports
                     * are separated by a huge jitter buffer. Therefore continue
                     * to process the input.
                     */
                }
                break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
                    portParamsPtr->oProcessIncomingMessages = false;
                    break;

                default:
                    OSCL_ASSERT(false);
                    break;
            }
        }
        break;

        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
        {
            int32 portTag = portParamsPtr->tag;
            /*
             * Outgoing queue was previously busy, but is now ready.
             * We may need to schedule new processing events depending
             * on the port type.
             */
            switch (portTag)
            {
                case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
                    /*
                     * We never did anything in PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_BUSY
                     * so do nothing
                     */
                    break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT:
                {
                    /*
                     * This implies that this output port can accept more
                     * msgs on its outgoing queue. This implies that the corresponding
                     * input port can start processing messages again.
                     */
                    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aActivity.iPort);
                    PVMFJitterBufferPortParams* inPortParams = jbPort->iCounterpartPortParams;
                    if (inPortParams != NULL)
                    {
                        inPortParams->oProcessIncomingMessages = true;
                    }
                    else
                    {
                        OSCL_ASSERT(false);
                    }
                }
                break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
                    portParamsPtr->oProcessIncomingMessages = true;
                    break;

                default:
                    OSCL_ASSERT(false);
                    break;
            }
            if (IsAdded())
            {
                RunIfNotReady();
            }
        }
        break;

        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_BUSY:
        {
            /*
             * The connected port has become busy (its incoming queue is
             * busy).
             */
            int32 portTag = portParamsPtr->tag;
            switch (portTag)
            {
                case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
                    break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT:
                {
                    /*
                     * This implies that this output port cannot send any more
                     * msgs from its outgoing queue. It should stop processing
                     * messages till the connect port is ready.
                     */
                    portParamsPtr->oProcessOutgoingMessages = false;
                }
                break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
                    portParamsPtr->oProcessOutgoingMessages = false;
                    break;

                default:
                    OSCL_ASSERT(false);
                    break;
            }
        }
        break;

        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_READY:
        {
            /*
             * The connected port has transitioned from Busy to Ready.
             * It's time to start processing messages outgoing again.
             */
            int32 portTag = portParamsPtr->tag;
            switch (portTag)
            {
                case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
                    break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT:
                    /*
                     * This implies that this output port can now send
                     * msgs from its outgoing queue. It can start processing
                     * messages now.
                     */
                    portParamsPtr->oProcessOutgoingMessages = true;
                    break;

                case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
                    portParamsPtr->oProcessOutgoingMessages = true;
                    break;

                default:
                    OSCL_ASSERT(false);
                    break;
            }
            if (IsAdded())
            {
                RunIfNotReady();
            }
        }
        break;

        default:
            break;
    }
}

/*
 * called by the AO to process a port activity message
 */
bool PVMFJitterBufferNode::ProcessPortActivity(PVMFJitterBufferPortParams* aPortParams)
{
    PVMFStatus status = PVMFSuccess;
    switch (aPortParams->tag)
    {
        case PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT:
        {
            if ((aPortParams->oProcessOutgoingMessages) &&
                    (aPortParams->iPort->OutgoingMsgQueueSize() > 0))
            {
                status = ProcessOutgoingMsg(aPortParams);
            }
            /*
             * Send data out of jitter buffer as long as there's:
             *  - more data to send
             *  - outgoing queue isn't in a Busy state.
             *  - ports are not paused
             */
            PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPortParams->iPort);
            if (aPortParams->oProcessOutgoingMessages)
            {
                if (oStopOutputPorts == false)
                {
                    SendData(OSCL_STATIC_CAST(PVMFPortInterface*, jbPort->iPortCounterpart));
                }
            }
        }
        break;

        case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
        {
            PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::ProcessPortActivity: input port- aPortParams->oProcessIncomingMessages %d aPortParams->iPort->IncomingMsgQueueSize()  %d" ,
                                 aPortParams->oProcessIncomingMessages, aPortParams->iPort->IncomingMsgQueueSize()));
            if ((aPortParams->oProcessIncomingMessages) &&
                    (aPortParams->iPort->IncomingMsgQueueSize() > 0))
            {
                status = ProcessIncomingMsg(aPortParams);
            }
            if ((aPortParams->oProcessOutgoingMessages) &&
                    (aPortParams->iPort->OutgoingMsgQueueSize() > 0))
            {
                status = ProcessOutgoingMsg(aPortParams);
            }
        }
        break;

        case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
        {
            PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::ProcessPortActivity: - aPortParams->oProcessIncomingMessages %d aPortParams->iPort->IncomingMsgQueueSize()  %d" ,
                                 aPortParams->oProcessIncomingMessages, aPortParams->iPort->IncomingMsgQueueSize()));

            if ((aPortParams->oProcessIncomingMessages) &&
                    (aPortParams->iPort->IncomingMsgQueueSize() > 0))
            {
                status = ProcessIncomingMsg(aPortParams);
            }
            if ((aPortParams->oProcessOutgoingMessages) &&
                    (aPortParams->iPort->OutgoingMsgQueueSize() > 0))
            {
                status = ProcessOutgoingMsg(aPortParams);
            }
        }
        break;

        default:
            break;
    }

    /*
     * Report any unexpected failure in port processing...
     * (the InvalidState error happens when port input is suspended,
     * so don't report it.)
     */
    if (status != PVMFErrBusy
            && status != PVMFSuccess
            && status != PVMFErrInvalidState)
    {
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessPortActivity: Error - ProcessPortActivity failed. port=0x%x",
                              aPortParams->iPort));
        ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPortParams->iPort));
    }

    /*
     * return true if we processed an activity...
     */
    return (status != PVMFErrBusy);
}

/////////////////////////////////////////////////////
PVMFStatus PVMFJitterBufferNode::ProcessIncomingMsg(PVMFJitterBufferPortParams* aPortParams)
{
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    PVMFPortInterface* aPort = aPortParams->iPort;
    /*
     * Called by the AO to process one buffer off the port's
     * incoming data queue.  This routine will dequeue and
     * dispatch the data.
     */
    PVMF_JBNODE_LOGINFO((0, "0x%x PVMFJitterBufferNode::ProcessIncomingMsg: aPort=0x%x", this, aPort));

    aPortParams->iNumMediaMsgsRecvd++;

    if (aPortParams->oMonitorForRemoteActivity == true)
    {
        CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
        RequestEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
    }

    switch (aPortParams->tag)
    {
        case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
        {
            /* Parse packet header - mainly to retrieve time stamp */
            PVMFJitterBuffer* jitterBuffer = aPortParams->iJitterBuffer;
            if (jitterBuffer == NULL)
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsg: findJitterBuffer failed"));
                int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
                ReportErrorEvent(PVMFErrArgument, (OsclAny*)(aPort), &eventuuid, &errcode);
                return PVMFErrArgument;
            }
            /*
             * Check for space in the jitter buffer before dequeing the message
             */
            if (!CheckForSpaceInJitterBuffer(aPort))
            {
                aPortParams->oProcessIncomingMessages = false;
                jitterBuffer->NotifyFreeSpaceAvailable(this,
                                                       OSCL_STATIC_CAST(OsclAny*, aPort));
                int32 infocode = PVMFJitterBufferNodeJitterBufferFull;
                ReportInfoEvent(PVMFInfoOverflow, (OsclAny*)(aPort), &eventuuid, &infocode);
                PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsg: Jitter Buffer full"));
                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::ProcessIncomingMsg: Jitter Buffer full"));
                return PVMFErrBusy;
            }

            /*
             * Incoming message recvd on the input port.
             * Dequeue the message
             */
            PVMFSharedMediaMsgPtr msg;
            PVMFStatus status = aPort->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsg: Error - INPUT PORT - DequeueIncomingMsg failed"));
                return status;
            }

            if (aPortParams->eTransportType == PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP)
            {
                ProcessIncomingMsgRTP(aPortParams, msg);
            }
            else
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsg: Invalid Transport Type"));
                OSCL_ASSERT(0);
            }
        }
        break;

        case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
        {
            /*
             * Incoming RTCP reports - recvd on the input port.
             * Dequeue the message - Need to fully implement
             * RTCP
             */
            PVMFSharedMediaMsgPtr msg;
            PVMFStatus status = aPort->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
                PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::ProcessIncomingMsg: Error - FB PORT - DequeueIncomingMsg failed", this));
                return status;
            }
            if (aPortParams->eTransportType == PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP)
            {
                ProcessIncomingMsgRTP(aPortParams, msg);
            }
            else
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsg: Invalid Transport Type"));
                OSCL_ASSERT(0);
            }
        }
        break;

        default:
        {
            ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
            PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsg - Invalid Port Tag"));
            return PVMFFailure;
        }
    }
    return (PVMFSuccess);
}

/////////////////////////////////////////////////////
PVMFStatus PVMFJitterBufferNode::ProcessIncomingMsgRTP(PVMFJitterBufferPortParams* aPortParams,
        PVMFSharedMediaMsgPtr& aMsg)
{
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    PVMFPortInterface* aPort = aPortParams->iPort;

    switch (aPortParams->tag)
    {
        case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
        {
            /* Parse packet header - mainly to retrieve time stamp */
            PVMFJitterBuffer* jitterBuffer = aPortParams->iJitterBuffer;
            /*
             * Check for msg format id. If msg format id is a media data then register
             * the packet in jitter buffer. If it is a command that we do not understand
             * log the command in a queue and send it out at an apporiate time.
             */
            PVUid32 msgFormatID = aMsg->getFormatID();
            if (msgFormatID > PVMF_MEDIA_CMD_FORMAT_IDS_START)
            {
                if (msgFormatID == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
                {
                    /*
                     * Check for upstream EOS. Set the boolean if it is. EOS from jitter buffer
                     * is sent when jitter buffer is all empty. We do not really expect EOS from
                     * upstream nodes in case of RTP processing. So ignore it.
                     */
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - Unexpected EOS"));
                    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - Unexpected EOS"));
                    aPortParams->oUpStreamEOSRecvd = true;
                    return PVMFSuccess;
                }
                else /* unknown command */
                {
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - Unknown Cmd Recvd"));
                    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - Unknown Cmd Recvd"));
                    jitterBuffer->addMediaCommand(aMsg);
                }
            }
            else /* Media data */
            {
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                PVMFJitterBufferStats stats = jitterBuffer->getJitterBufferStats();
                uint32 timebase32 = 0;
                uint32 estServerClock = 0;
                uint32 clientClock = 0;
                bool overflowFlag = false;
                iEstimatedServerClock->GetCurrentTime32(estServerClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                if (iClientPlayBackClock != NULL)
                    iClientPlayBackClock->GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
#endif
                PVMFSharedMediaDataPtr inputDataPacket;
                convertToPVMFMediaData(inputDataPacket, aMsg);

                if (aPortParams->bTransportHeaderPreParsed)
                {
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - Invalid TransportType"));
                    OSCL_ASSERT(0);
                }
                else
                {
                    /*
                     * We assume the following. Each input msg contains an integral number of
                     * RTP packets. These packets are stored in memory fragments and each memfrag
                     * can contain JUST ONE complete RTP packet.
                     * Jitterbuffer holds an array of media msgs instead of just refcounted memfrags.
                     * Reason being memfrags cannot hold additional info like ts, seqnum etc,
                     * whereas mediamsgs can.
                     * If "oInPlaceProcessing" boolean if set to true means that media msg wrappers
                     * that carry the RTP packets from downstream node can be reused. "ParsePacketHeader"
                     * does pointer arithmetic on input memory frags and reappends them to "inputDataPacket"
                     * We register "inputDataPacket" in jitter buffer's circular array.
                     * If "oInPlaceProcessing" boolean if set to false, then inside "ParsePacketHeader"
                     * we allocate a new media msg wrapper and transfer the memory fragments from "inputDataPacket"
                     * to "dataPacket".We register "dataPacket" in jitter buffer's circular array.
                     * If there are multiple RTP packets in "inputDataPacket" then we have to allocate new
                     * media msg wrappers for each one of these. So "oInPlaceProcessing" cannot be true if
                     * downstream node packs multiple RTP packets in a single media msg.
                     */
                    if (inputDataPacket->getNumFragments() > 1)
                    {
                        if (aPortParams->oInPlaceProcessing == true)
                        {
                            PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - In Place Processing - No Support for Multiple MemFrags "));
                            PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - In Place Processing - No Support for Multiple MemFrags "));
                            int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
                            ReportErrorEvent(PVMFErrProcessing, (OsclAny*)(aPort), &eventuuid, &errcode);
                            return PVMFFailure;
                        }
                    }
                    OsclSharedPtr<PVMFMediaDataImpl> mediaDataIn;
                    if (!inputDataPacket->getMediaDataImpl(mediaDataIn))
                    {
                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: corrupt input media msg"));
                        PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: corrupt input media msg"));
                        int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
                        ReportErrorEvent(PVMFErrProcessing, (OsclAny*)(aPort), &eventuuid, &errcode);
                        return PVMFFailure;
                    }
                    for (uint32 i = 0; i < inputDataPacket->getNumFragments(); i++)
                    {
                        uint32 fragsize = 0;
                        mediaDataIn->getMediaFragmentSize(i, fragsize);
                        if (fragsize > RTP_FIXED_HEADER_SIZE)
                        {
                            if (inputDataPacket->getNumFragments() > 1)
                            {
                                if (aPortParams->oInPlaceProcessing == true)
                                {
                                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - In Place Processing - No Support for Multiple MemFrags "));
                                    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - In Place Processing - No Support for Multiple MemFrags "));
                                    int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
                                    ReportErrorEvent(PVMFErrProcessing, (OsclAny*)(aPort), &eventuuid, &errcode);
                                    return PVMFFailure;
                                }
                            }

                            PVMFSharedMediaDataPtr dataPacket;
                            if (jitterBuffer->ParsePacketHeader(inputDataPacket, dataPacket, i))
                            {
                                //dataPacket is NULL if inPlaceProcess is true. For 3gpp live streaming
                                if (CheckStateForRegisteringRTPPackets() == false)
                                {
                                    if (dataPacket.GetRep())
                                    {
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                                        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: - Ignoring - Wrong State"
                                                                               "Size=%d, SSRC=%d", inputDataPacket->getFilledSize(), dataPacket->getStreamID()));
                                        PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsg: - Ignoring - Wrong State"
                                                                       "Size=%d, SSRC=%d", inputDataPacket->getFilledSize(), dataPacket->getStreamID()));
                                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsg: - Ignoring - Wrong State"
                                                              "Size=%d, SSRC=%d", inputDataPacket->getFilledSize(), dataPacket->getStreamID()));
#endif
                                    }
                                    return PVMFSuccess;
                                }
                                if (aPortParams->oInPlaceProcessing)
                                {
                                    /*
                                     * Register the same in jitter buffer
                                     */
                                    if (!RegisterDataPacket(aPort, jitterBuffer, inputDataPacket))
                                    {
                                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: RegisterDataPacket failed"));
                                        int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
                                        ReportErrorEvent(PVMFErrProcessing, (OsclAny*)(aPort), &eventuuid, &errcode);
                                        return PVMFFailure;
                                    }
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                                    PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::ProcessIncomingMsgRTP:"
                                                                         "MaxSNReg=%d, MaxTSReg=%u, LastSNRet=%d, LastTSRet=%u, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
                                                                         stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                                                         stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                                                         (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum),
                                                                         Oscl_Int64_Utils::get_uint64_lower32(estServerClock),
                                                                         Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
#endif
                                }
                                else
                                {
                                    /*
                                     * Register the same in jitter buffer
                                     */
                                    if (!RegisterDataPacket(aPort, jitterBuffer, dataPacket))
                                    {
                                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: RegisterDataPacket failed"));
                                        int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
                                        ReportErrorEvent(PVMFErrProcessing, (OsclAny*)(aPort), &eventuuid, &errcode);
                                        return PVMFFailure;
                                    }
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                                    PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::ProcessIncomingMsgRTP:"
                                                                         "MaxSNReg=%d, MaxTSReg=%u, LastSNRet=%d, LastTSRet=%u, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
                                                                         stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                                                         stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                                                         (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum),
                                                                         Oscl_Int64_Utils::get_uint64_lower32(estServerClock),
                                                                         Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
#endif
                                }
                            }
                            else
                            {
                                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: ParsePacketHeader failed"));
                                int32 infocode = PVMFJitterBufferNodeInputDataPacketHeaderParserError;
                                ReportInfoEvent(PVMFErrCorrupt, (OsclAny*)(aPort), &eventuuid, &infocode);
                            }
                            SendData(aPort);
                        }
                        else
                        {
                            if (aPortParams->eTransportType == PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP)
                            {
                                /* Could be a firewall packet */
                                if (DecodeFireWallPackets(inputDataPacket, aPortParams) != PVMFSuccess)
                                {
                                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: DecodeFireWallPackets failed"));
                                    int32 infocode = PVMFJitterBufferNodeCorruptRTPPacket;
                                    ReportInfoEvent(PVMFErrCorrupt, (OsclAny*)(aPort), &eventuuid, &infocode);
                                }
                            }
                            else
                            {
                                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: Too Small - Ignoring"
                                                                       "Size=%d", inputDataPacket->getFilledSize()));
                                PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: Too Small - Ignoring"
                                                               "Size=%d", inputDataPacket->getFilledSize()));
                                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP: Too Small - Ignoring"
                                                      "Size=%d", inputDataPacket->getFilledSize()));
                            }
                        }
                    }
                }
            }
        }
        break;

        case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
        {
            PVMFStatus status = ProcessIncomingRTCPReport(aMsg, aPortParams);
            if (status != PVMFSuccess)
            {
                PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::ProcessIncomingMsgRTP: ProcessIncomingRTCPReport failed", this));
                int32 errcode = PVMFJitterBufferNodeRTCPSRProcFailed;
                ReportErrorEvent(PVMFErrProcessing, (OsclAny*)(aPort), &eventuuid, &errcode);
                return status;
            }
        }
        break;

        default:
        {
            ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
            PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ProcessIncomingMsgRTP - Invalid Port Tag"));
            return PVMFFailure;
        }
    }
    return (PVMFSuccess);
}



/////////////////////////////////////////////////////
PVMFStatus PVMFJitterBufferNode::ProcessOutgoingMsg(PVMFJitterBufferPortParams* aPortParams)
{
    PVMFPortInterface* aPort = aPortParams->iPort;
    /*
     * Called by the AO to process one message off the outgoing
     * message queue for the given port.  This routine will
     * try to send the data to the connected port.
     */
    PVMF_JBNODE_LOGINFO((0, "0x%x PVMFJitterBufferNode::ProcessOutgoingMsg: aPort=0x%x", this, aPort));

    /*
     * If connected port is busy, the outgoing message cannot be process. It will be
     * queued to be processed again after receiving PORT_ACTIVITY_CONNECTED_PORT_READY
     * from this port.
     */
    if (aPort->IsConnectedPortBusy())
    {
        PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "0x%x PVMFJitterBufferNode::ProcessOutgoingMsg: Connected port is busy", this));
        aPortParams->oProcessOutgoingMessages = false;
        return PVMFErrBusy;
    }

    PVMFStatus status = PVMFSuccess;

    status = aPort->Send();
    if (status == PVMFErrBusy)
    {
        PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::ProcessOutgoingMsg: Connected port goes into busy state"));
        aPortParams->oProcessOutgoingMessages = false;
    }
    else
    {
        aPortParams->iNumMediaMsgsSent++;
    }
    return status;
}

bool PVMFJitterBufferNode::CheckForPortRescheduling()
{
    //This method is only called from JB Node AO's Run.
    //Purpose of this method is to determine whether the node
    //needs scheduling based on any outstanding port activities
    //Here is the scheduling criteria for different port types:
    //a) PVMF_JITTER_BUFFER_PORT_TYPE_INPUT - If there are incoming
    //msgs waiting in incoming msg queue then node needs scheduling,
    //as long oProcessIncomingMessages is true. This boolean stays true
    //as long we can register packets in JB. If JB is full this boolean
    //is made false (when CheckForSpaceInJitterBuffer() returns false)
    //and is once again made true in JitterBufferFreeSpaceAvailable() callback.
    //We also use the input port briefly as a bidirectional port in case of
    //RTSP streaming to do firewall packet exchange. So if there are outgoing
    //msgs and oProcessOutgoingMessages is true then node needs scheduling.
    //b) PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT - As long as:
    //	- there are msgs in outgoing queue
    //	- oProcessOutgoingMessages is true
    //	- and as long as there is data in JB and we are not in buffering
    //then node needs scheduling.
    //c) PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK - As long as:
    //	- there are msgs in incoming queue and oProcessIncomingMessages is true
    //	- there are msgs in outgoing queue and oProcessOutgoingMessages is true
    uint32 i;
    for (i = 0; i < iPortVector.size(); i++)
    {
        PVMFJitterBufferPortParams* portContainerPtr = iPortVector[i]->iPortParams;
        if (portContainerPtr == NULL)
        {
            PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::CheckForPortRescheduling: Error - GetPortContainer failed"));
            return false;
        }
        if (portContainerPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portContainerPtr->iPort->IncomingMsgQueueSize() > 0)
            {
                if (portContainerPtr->oProcessIncomingMessages)
                {
                    /*
                     * Found a port that has outstanding activity and
                     * is not busy.
                     */
                    return true;
                }
            }
            if (portContainerPtr->iPort->OutgoingMsgQueueSize() > 0)
            {
                if (portContainerPtr->oProcessOutgoingMessages)
                {
                    /*
                     * Found a port that has outstanding activity and
                     * is not busy.
                     */
                    return true;
                }
            }
        }
        else if (portContainerPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT)
        {
            PVMFJitterBufferPort* jbPort =
                OSCL_STATIC_CAST(PVMFJitterBufferPort*, portContainerPtr->iPort);
            if ((portContainerPtr->iPort->OutgoingMsgQueueSize() > 0) ||
                    ((jbPort->iCounterpartPortParams->oJitterBufferEmpty == false) &&
                     (oDelayEstablished == true) && !IsCallbackPending(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, jbPort->iCounterpartPortParams)))
            {
                if ((portContainerPtr->oProcessOutgoingMessages) && (oStopOutputPorts == false))
                {
                    /*
                     * Found a port that has outstanding activity and
                     * is not busy.
                     */
                    return true;
                }
            }
        }
        else if (portContainerPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
        {
            if (portContainerPtr->iPort->IncomingMsgQueueSize() > 0)
            {
                if (portContainerPtr->oProcessIncomingMessages)
                {
                    /*
                     * Found a port that has outstanding activity and
                     * is not busy.
                     */
                    return true;
                }
            }
            if (portContainerPtr->iPort->OutgoingMsgQueueSize() > 0)
            {
                if (portContainerPtr->oProcessOutgoingMessages)
                {
                    /*
                     * Found a port that has outstanding activity and
                     * is not busy.
                     */
                    return true;
                }
            }
        }
    }
    /*
     * No port processing needed - either all port activity queues are empty
     * or the ports are backed up due to flow control.
     */
    return false;
}

bool PVMFJitterBufferNode::CheckForPortActivityQueues()
{
    uint32 i;
    for (i = 0; i < iPortVector.size(); i++)
    {
        PVMFJitterBufferPortParams* portContainerPtr = NULL;

        if (!getPortContainer(iPortVector[i], portContainerPtr))
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::CheckForPortActivityQueues: Error - GetPortContainer failed", this));
            return false;
        }

        if ((portContainerPtr->iPort->IncomingMsgQueueSize() > 0) ||
                (portContainerPtr->iPort->OutgoingMsgQueueSize() > 0))
        {
            /*
             * Found a port that still has an outstanding activity.
             */
            return true;
        }
    }
    /*
     * All port activity queues are empty
     */
    return false;
}

/**
 * Active object implementation
 */

/**
  * This AO handles both API commands and port activity.
  * The AO will either process one command or service one connected
  * port per call.  It will re-schedule itself and run continuously
  * until it runs out of things to do.
  */
void PVMFJitterBufferNode::Run()
{
    iNumRunL++;
    /*
     * Process commands.
     */
    if (!iInputCommands.empty())
    {
        if (ProcessCommand(iInputCommands.front()))
        {
            /*
             * note: need to check the state before re-scheduling
             * since the node could have been reset in the ProcessCommand
             * call.
             */
            if (iInterfaceState != EPVMFNodeCreated)
            {
                if (IsAdded())
                {
                    RunIfNotReady();
                }
            }
            return;
        }
    }

    /*
     * Process port activity
     */
    if (((iInterfaceState == EPVMFNodeInitialized) ||
            (iInterfaceState == EPVMFNodePrepared) ||
            (iInterfaceState == EPVMFNodeStarted)  ||
            (iInterfaceState == EPVMFNodePaused)) ||
            FlushPending())
    {
        uint32 i;
        for (i = 0; i < iPortVector.size(); i++)
        {
            PVMFJitterBufferPortParams* portContainerPtr =
                iPortVector[i]->iPortParams;

            if (portContainerPtr == NULL)
            {
                if (!getPortContainer(iPortVector[i], portContainerPtr))
                {
                    PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::Run: Error - getPortContainer failed", this));
                    return;
                }
                iPortVector[i]->iPortParams = portContainerPtr;
            }

            ProcessPortActivity(portContainerPtr);
        }

        if (CheckForPortRescheduling())
        {
            if (IsAdded())
            {
                /*
                 * Re-schedule since there is atleast one port that needs processing
                 */
                RunIfNotReady();
            }
            return;
        }
    }

    if (iInterfaceState == EPVMFNodeStarted)
    {
        /*
         * Queue Data to Output ports from jitter buffer only after
         * a certain time, i.e, after the session duration expiry.
         * This means that the server would have stopped sending us
         * data, and we need to make sure we send out the remaining
         * jitter buffer contents onward.
         */
        if (oSessionDurationExpired || RTCPByeRcvd())
        {
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    SendData(it->iPort);
                }
            }

            if (CheckForPortRescheduling())
            {
                if (IsAdded())
                {
                    /*
                     * Re-schedule since there is atleast one port that needs processing
                     */
                    RunIfNotReady();
                }
                return;
            }
            else
            {
                bool oEmpty;
                CheckJitterBufferEmpty(oEmpty);
                if (oEmpty)
                {
                    /* Check and generate EOS, if jitter buffers are all empty */
                    CheckForEOS();
                }
            }
            return;
        }
    }
    /*
     * If we get here we did not process any ports or commands.
     * Check for completion of a flush command...
     */
    if (FlushPending() && (!CheckForPortActivityQueues()))
    {
        uint32 i;
        /*
         * Debug check-- all the port queues should be empty at
         * this point.
         */
        for (i = 0;i < iPortVector.size();i++)
        {
            if (iPortVector[i]->IncomingMsgQueueSize() > 0 ||
                    iPortVector[i]->OutgoingMsgQueueSize() > 0)
            {
                OSCL_ASSERT(false);
            }
        }
        /*
         * Flush is complete.  Go to prepared state.
         */
        SetState(EPVMFNodePrepared);
        /*
         * resume port input so the ports can be re-started.
         */
        for (i = 0;i < iPortVector.size();i++)
        {
            iPortVector[i]->ResumeInput();
        }
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    return;
}

bool PVMFJitterBufferNode::RTCPByeRcvd()
{
    bool retval = false;
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->iRTCPStats.oRTCPByeRecvd)
        {
            retval = true;
            break;
        }
    }
    return retval;
}

void PVMFJitterBufferNode::DoCancel()
{
    /*
     * the base class cancel operation is sufficient.
     */
    OsclActiveObject::DoCancel();
}

bool
PVMFJitterBufferNode::setPortParams(PVMFPortInterface* aPort,
                                    uint32 aTimeScale,
                                    uint32 aBitRate,
                                    OsclRefCounterMemFrag& aConfig,
                                    bool aRateAdaptation,
                                    uint32 aRateAdaptationFeedBackFrequency)

{
    return setPortParams(aPort, aTimeScale, aBitRate, aConfig, aRateAdaptation,
                         aRateAdaptationFeedBackFrequency, false);
}

bool
PVMFJitterBufferNode::setPortParams(PVMFPortInterface* aPort,
                                    uint32 aTimeScale,
                                    uint32 aBitRate,
                                    OsclRefCounterMemFrag& aConfig,
                                    bool aRateAdaptation,
                                    uint32 aRateAdaptationFeedBackFrequency,
                                    uint aMaxNumBuffResizes, uint aBuffResizeSize)
{
    return setPortParams(aPort, aTimeScale, aBitRate, aConfig, aRateAdaptation,
                         aRateAdaptationFeedBackFrequency, true,
                         aMaxNumBuffResizes, aBuffResizeSize);
}
bool
PVMFJitterBufferNode::setPortParams(PVMFPortInterface* aPort,
                                    uint32 aTimeScale,
                                    uint32 aBitRate,
                                    OsclRefCounterMemFrag& aConfig,
                                    bool aRateAdaptation,
                                    uint32 aRateAdaptationFeedBackFrequency,
                                    bool aUserSpecifiedBuffParams,
                                    uint aMaxNumBuffResizes, uint aBuffResizeSize)
{
    OSCL_UNUSED_ARG(aUserSpecifiedBuffParams);
    uint32 i;
    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

        if (portParams.iPort == aPort)
        {
            portParams.timeScale = aTimeScale;
            portParams.mediaClockConverter.set_timescale(aTimeScale);
            portParams.bitrate = aBitRate;
            portParams.iTrackConfig = aConfig;
            portParams.oRateAdaptation = aRateAdaptation;
            portParams.iRateAdaptationFeedBackFrequency = aRateAdaptationFeedBackFrequency;

            /* Compute buffer size based on bitrate and jitter duration*/
            uint32 sizeInBytes = 0;
            if (((int32)iJitterBufferDurationInMilliSeconds > 0) &&
                    ((int32)aBitRate > 0))
            {
                uint32 byteRate = aBitRate / 8;
                uint32 overhead = (byteRate * PVMF_JITTER_BUFFER_NODE_MEM_POOL_OVERHEAD) / 100;
                uint32 durationInSec = iJitterBufferDurationInMilliSeconds / 1000;
                sizeInBytes = ((byteRate + overhead) * durationInSec);
                if (sizeInBytes < MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES)
                {
                    sizeInBytes = MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES;
                }
                sizeInBytes += (2 * MAX_SOCKET_BUFFER_SIZE);
            }

            if ((portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT) || (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK))
            {
                portParams.SetJitterBufferMemPoolInfo(sizeInBytes, aBuffResizeSize, aMaxNumBuffResizes, 3000);
            }

            portParams.iRateAdaptationFreeBufferSpaceInBytes = sizeInBytes;
            iPortParamsQueue[i] = portParams;
            return true;
        }
    }
    return false;
}

bool
PVMFJitterBufferNode::setPortSSRC(PVMFPortInterface* aPort, uint32 aSSRC)
{
    PVMFJitterBufferPortParams* portParamsPtr = NULL;

    if (!getPortContainer(aPort, portParamsPtr))
    {
        return false;
    }

    if (portParamsPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
    {
        if (portParamsPtr->iJitterBuffer != NULL)
        {
            portParamsPtr->iJitterBuffer->setSSRC(aSSRC);
            portParamsPtr->SSRC = aSSRC;
        }
        return true;
    }
    return false;
}

bool
PVMFJitterBufferNode::ClearJitterBuffer(PVMFPortInterface* aPort,
                                        uint32 aSeqNum)
{
    /* Typically called only for HTTP streaming sessions */
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
    {
        it->iPort->ClearMsgQueues();
        it->oUpStreamEOSRecvd = false;
        it->oEOSReached = false;
    }
    for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
    {
        if (it->iPort == aPort)
        {
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                if (it->iJitterBuffer != NULL)
                {
                    uint32 timebase32 = 0;
                    uint32 clientClock32 = 0;
                    bool overflowFlag = false;
                    if (iClientPlayBackClock != NULL)
                        iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                    uint32 serverClock32 = 0;
                    iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - Purging Upto SeqNum =%d", aSeqNum));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - Before Purge - EstServClock=%d",
                                                 serverClock32));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - Before Purge - ClientClock=%d",
                                                 clientClock32));
#endif

                    it->iJitterBuffer->PurgeElementsWithSeqNumsLessThan(aSeqNum,
                            clientClock32);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                    timebase32 = 0;
                    serverClock32 = 0;
                    iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - After Purge - EstServClock=%d",
                                                 serverClock32));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - After Purge - ClientClock=%d",
                                                 clientClock32));
#endif
                    it->iJitterBuffer->SetEOS(false);
                    /*
                     * Since we flushed the jitter buffer, set it to ready state,
                     * reset the delay flag
                     */
                    oDelayEstablished = false;
                    oSessionDurationExpired = false;
                    iSessionDurationTimer->Cancel();
                    iSessionDurationTimer->Stop();
                    iJitterBufferState = PVMF_JITTER_BUFFER_READY;
                }
                return true;
            }
        }
    }
    return false;
}

bool
PVMFJitterBufferNode::setPortRTPParams(PVMFPortInterface* aPort,
                                       bool   aSeqNumBasePresent,
                                       uint32 aSeqNumBase,
                                       bool   aRTPTimeBasePresent,
                                       uint32 aRTPTimeBase,
                                       bool   aNPTTimeBasePresent,
                                       uint32 aNPTInMS,
                                       bool oPlayAfterASeek)
{
    uint32 i;
    //The above method is called only during 3GPP repositioning, however, since the aPort param in the signature
    // takes care only of the input port, the output port msg queues aren't cleared.
    // As a result ClearMsgQueues need to be called explicity on all the ports.
    //The oPlayAfterASeek check is necessary since the clearing of msg queues has to be carried out only during repositioning,
    // not otherwise
    if (oPlayAfterASeek)
    {
        for (i = 0; i < iPortParamsQueue.size(); i++)
        {
            PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];
            portParams.iPort->ClearMsgQueues();
        }
    }
    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

        if (portParams.iPort == aPort)
        {
            if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                if (portParams.iJitterBuffer != NULL)
                {
                    PVMFRTPInfoParams rtpInfoParams;
                    rtpInfoParams.seqNumBaseSet = aSeqNumBasePresent;
                    rtpInfoParams.seqNum = aSeqNumBase;
                    rtpInfoParams.rtpTimeBaseSet = aRTPTimeBasePresent;
                    rtpInfoParams.rtpTime = aRTPTimeBase;
                    rtpInfoParams.nptTimeBaseSet = aNPTTimeBasePresent;
                    rtpInfoParams.nptTimeInMS = aNPTInMS;
                    rtpInfoParams.rtpTimeScale = portParams.timeScale;
                    portParams.iJitterBuffer->setRTPInfoParams(rtpInfoParams, oPlayAfterASeek);
                    /* In case this is after a reposition purge the jitter buffer */
                    if (oPlayAfterASeek)
                    {
                        uint32 timebase32 = 0;
                        uint32 clientClock32 = 0;
                        bool overflowFlag = false;
                        if (iClientPlayBackClock != NULL)
                            iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                        uint32 serverClock32 = 0;
                        iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - Purging Upto SeqNum =%d", aSeqNumBase));
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - Before Purge - EstServClock=%d",
                                                     serverClock32));
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - Before Purge - ClientClock=%d",
                                                     clientClock32));
#endif
                        portParams.iJitterBuffer->PurgeElementsWithSeqNumsLessThan(aSeqNumBase,
                                clientClock32);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                        timebase32 = 0;
                        serverClock32 = 0;
                        iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - After Purge - EstServClock=%d",
                                                     serverClock32));
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - After Purge - ClientClock=%d",
                                                     clientClock32));
#endif
                        /*
                         * Since we flushed the jitter buffer, set it to ready state,
                         * reset the delay flag
                         */
                        oDelayEstablished = false;
                        iJitterBufferState = PVMF_JITTER_BUFFER_READY;
                        iPlayingAfterSeek = true;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool PVMFJitterBufferNode::setPortRTCPParams(PVMFPortInterface* aPort,
        int aNumSenders,
        uint32 aRR,
        uint32 aRS)
{
    uint32 i;
    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        if (iPortParamsQueue[i].iPort == aPort)
        {
            OSCL_ASSERT(iPortParamsQueue[i].tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK);

            iPortParamsQueue[i].RR = aRR;
            iPortParamsQueue[i].RS = aRS;
            iPortParamsQueue[i].numSenders = aNumSenders;
            iPortParamsQueue[i].RtcpBwConfigured = true;
            return true;
        }
    }
    return false;
}
/* computes the max next ts of all tracks */
PVMFTimestamp PVMFJitterBufferNode::getActualMediaDataTSAfterSeek()
{
    PVMFTimestamp mediaTS = 0;
    uint32 in_wrap_count = 0;
    uint32 i;

    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portParams.iJitterBuffer != NULL)
            {
                PVMFTimestamp ts =
                    portParams.iJitterBuffer->peekNextElementTimeStamp();
                /*
                 * Convert Time stamp to milliseconds
                 */
                portParams.mediaClockConverter.set_clock(ts, in_wrap_count);
                PVMFTimestamp converted_ts =
                    portParams.mediaClockConverter.get_converted_ts(1000);
                if (converted_ts > mediaTS)
                {
                    mediaTS = converted_ts;
                }
            }
        }
    }
    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portParams.eTransportType != PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_ASF)
            {
                if (portParams.iJitterBuffer != NULL)
                {
                    portParams.iJitterBuffer->SetAdjustedTSInMS(mediaTS);
                }
            }
        }
    }
    return mediaTS;
}

PVMFStatus
PVMFJitterBufferNode::setServerInfo(PVMFJitterBufferFireWallPacketInfo& aServerInfo)
{
    if (iDisableFireWallPackets == false)
    {
        iFireWallPacketInfo = aServerInfo;
        if (iFireWallPacketInfo.iFormat == PVMF_JB_FW_PKT_FORMAT_RTP)
        {
            PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::setServerInfo: Fw Pkt Exchange Start - PVMF_JB_FW_PKT_FORMAT_RTP"));
        }
        else
        {
            PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::setServerInfo: Fw Pkt Exchange Start - PVMF_JB_FW_PKT_FORMAT_PV"));
        }
        RequestEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET);
        PVMFStatus status = SendFireWallPackets();
        if (status != PVMFSuccess)
        {
            CommandComplete(iCurrentCommand,
                            iCurrentCommand.front(),
                            status);
        }
    }
    else
    {
        PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::setServerInfo: FW Pkts Disabled"));
        if (iCurrentCommand.size() > 0)
        {
            if (iCurrentCommand.front().iCmd == PVMF_JITTER_BUFFER_NODE_PREPARE)
            {
                /* No firewall packet exchange - Complete Prepare */
                CompletePrepare();
            }
        }
    }
    return PVMFSuccess;
}

bool
PVMFJitterBufferNode::getPortContainer(PVMFPortInterface* aPort,
                                       PVMFJitterBufferPortParams*& aPortParamsPtr)
{
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->iPort == aPort)
        {
            aPortParamsPtr = it;
            return true;
        }
    }
    return false;
}

PVMFJitterBuffer*
PVMFJitterBufferNode::findJitterBuffer(PVMFPortInterface* aPort)
{
    uint32 ii;
    for (ii = 0; ii < iPortParamsQueue.size(); ii++)
    {

        if (iPortParamsQueue[ii].iPort == aPort)
        {
            return (iPortParamsQueue[ii].iJitterBuffer);

        }
    }
    return NULL;
}

PVMFPortInterface*
PVMFJitterBufferNode::findPortForJitterBuffer(PVMFJitterBuffer* aJitterBuffer)
{
    uint32 ii;
    for (ii = 0; ii < iPortParamsQueue.size(); ii++)
    {
        if (iPortParamsQueue[ii].iJitterBuffer == aJitterBuffer)
        {
            return (iPortParamsQueue[ii].iPort);
        }
    }
    return NULL;
}

PVMFPortInterface*
PVMFJitterBufferNode::getPortCounterpart(PVMFPortInterface* aPort)
{
    uint32 ii;
    /*
     * Get port params
     */
    for (ii = 0; ii < iPortParamsQueue.size(); ii++)
    {
        if (iPortParamsQueue[ii].iPort == aPort)
        {
            break;
        }
    }
    if (ii >= iPortParamsQueue.size())
    {
        return NULL;
    }

    PVMFJitterBufferNodePortTag tag = iPortParamsQueue[ii].tag;
    int32 id = iPortParamsQueue[ii].id;
    uint32 jj;

    /* Even numbered ports are input ports */
    if (tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
    {
        for (jj = 0; jj < iPortParamsQueue.size(); jj++)
        {
            if ((id + 1) == iPortParamsQueue[jj].id)
            {
                return (iPortParamsQueue[jj].iPort);


            }
        }
    }
    /* odd numbered ports are output ports */
    else if (tag == PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT)
    {
        for (jj = 0; jj < iPortParamsQueue.size(); jj++)
        {
            if ((id - 1) == iPortParamsQueue[jj].id)
            {
                return (iPortParamsQueue[jj].iPort);


            }
        }
    }
    return NULL;
}

bool
PVMFJitterBufferNode::CheckForSpaceInJitterBuffer(PVMFPortInterface* aPort)
{
    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    PVMFJitterBuffer* jitterBuffer = jbPort->iPortParams->iJitterBuffer;
    if (jitterBuffer == NULL)
    {
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::CheckForSpaceInJitterBuffer: findJitterBuffer failed"));
        PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
        int32 errcode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
        ReportErrorEvent(PVMFErrArgument, (OsclAny*)(aPort), &eventuuid, &errcode);
        return false;
    }
    return (jitterBuffer->CheckForMemoryAvailability());
}

void
PVMFJitterBufferNode::JitterBufferFreeSpaceAvailable(OsclAny* aContext)
{
    PVMFPortInterface* port = OSCL_STATIC_CAST(PVMFPortInterface*, aContext);
    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, port);
    PVMFJitterBufferPortParams* portParams = jbPort->iPortParams;
    if (portParams)
        portParams->oProcessIncomingMessages = true;
    if (IsAdded())
    {
        RunIfNotReady();
    }
}

void
PVMFJitterBufferNode::EstimatedServerClockUpdated(OsclAny* aContext)
{
    /*
     * We start monitoring estimated server clock updates once
     * the session duration expires AND the estimated server clock
     * does not match up the session end time
     */
    OSCL_UNUSED_ARG(aContext);
    iSessionDurationTimer->EstimatedServerClockUpdated();
}

bool PVMFJitterBufferNode::RegisterDataPacket(PVMFPortInterface* aPort,
        PVMFJitterBuffer* aJitterBuffer,
        PVMFSharedMediaDataPtr& aDataPacket)
{
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    PVMFJitterBufferAddPktStatus status = aJitterBuffer->addPacket(aDataPacket);
    if (status == PVMF_JITTER_BUFFER_ADD_PKT_ERROR)
    {
        /* Jitter buffer full */
        PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::RegisterDataPacket: jitterBuffer->addPacket failed"));
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::RegisterDataPacket: jitterBuffer->addPacket failed"));
        int32 infocode = PVMFJitterBufferNodeJitterBufferFull;
        ReportInfoEvent(PVMFInfoOverflow, (OsclAny*)(aPort), &eventuuid, &infocode);
    }
    else if (status == PVMF_JITTER_BUFFER_ADD_PKT_UNEXPECTED_DATA)
    {
        PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::RegisterDataPacket: Unexpected Data Error"));
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::RegisterDataPacket: Unexpected Data Error"));
        int32 infocode = PVMFJitterBufferNodeUnableToRegisterIncomingPacket;
        ReportInfoEvent(PVMFInfoUnexpectedData, (OsclAny*)(aPort), &eventuuid, &infocode);
    }
    else if (status == PVMF_JITTER_BUFFER_ADD_PKT_EOS_REACHED)
    {
        int32 infocode = PVMFJitterBufferNodeTrackEOSReached;
        ReportInfoEvent(PVMFInfoEndOfData, (OsclAny*)(aPort), &eventuuid, &infocode);
        PVMF_JBNODE_LOGINFO((0, "0x%x PVMFJitterBufferNode::RegisterDataPacket - EOS Reached", this));
        PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferNode::RegisterDataPacket - EOS Reached"));
    }
    else
    {
        /* if we are not rebuffering check for flow control */
        if (aPort != NULL)
        {
            PVMFStatus status;
            bool highWaterMarkReached = false;
            status = CheckForHighWaterMark(aPort, highWaterMarkReached);
            if ((status == PVMFSuccess) && (highWaterMarkReached == true))
            {
                PVMF_JBNODE_LOGINFO((0, "0x%x JB::RegisterDataPacket - HWM oDelayEstablished %d oAutoPause %d iPlayStopTimeAvailable %d", this, oDelayEstablished, oAutoPause, iPlayStopTimeAvailable));
                /*
                 * Check for jitter buffer occupancy. If the jitter buffer is in
                 * the danger of overflowing start pushing data out. If the nodes
                 * downstream cannot process the data then it would just back up
                 * in the port queues and flow control would kick in. But on the
                 * other hand if they can process it then why not send it.
                 */
                if (oDelayEstablished == false)
                {
                    /* Coming out of rebuffering */
                    UpdateRebufferingStats(PVMFInfoDataReady);
                    ReportInfoEvent(PVMFInfoDataReady);
                    ReportInfoEvent(PVMFInfoBufferingComplete);
                    CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                    uint32 timebase32 = 0;
                    uint32 clientClock32 = 0;
                    uint32 serverClock32 = 0;
                    bool overflowFlag = false;
                    if (iClientPlayBackClock != NULL)
                        iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                    iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::RegisterDataPacket - Time Delay Established - EstServClock=%d",
                                                 serverClock32));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::RegisterDataPacket - Time Delay Established - ClientClock=%d",
                                                 clientClock32));
#endif
                }
                iJitterDelayPercent = 100;
                oDelayEstablished = true;
                /* Send just once */
                if ((oAutoPause == false) && (iPlayStopTimeAvailable == true))
                {
                    oAutoPause = true;
                    ReportInfoEvent(PVMFJitterBufferNodeJitterBufferHighWaterMarkReached);
                    PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::RegisterDataPacket: Sending Auto Pause"));
                    RequestMemCallBackForAutoResume(aPort);
                }
            }
        }
    }
    return true;
}

bool
PVMFJitterBufferNode::IsJitterBufferReady(PVMFJitterBufferPortParams* aPortParams, uint32& aClockDiff)
{
    aClockDiff = iJitterBufferDurationInMilliSeconds;
    PVMFJitterBuffer* aJitterBuffer = aPortParams->iJitterBuffer;
    if (iJitterBufferState == PVMF_JITTER_BUFFER_IN_TRANSITION)
    {
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Jitter Buffer In Transition - Preparing for Seek"));
        oDelayEstablished = false;
        iJitterDelayPercent = 0;
        return oDelayEstablished;
    }

    // check if this is the first time this function is called after seeking
    if (iPlayingAfterSeek)
    {
        iPlayingAfterSeek = false;
        if (aPortParams->eTransportType != PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_ASF)
        {
            // This call will normalize the timestamps in the jitterbuffers
            PVMFTimestamp ts = getActualMediaDataTSAfterSeek();
            ts = 0;
            // Adjust the RTP timestamps in the jitterbuffers
            for (uint i = 0; i < iPortParamsQueue.size(); i++)
            {
                PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];
                if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    if (portParams.iJitterBuffer != NULL)
                    {
                        portParams.iJitterBuffer->AdjustRTPTimeStamp();
                    }
                }
            }
        }
    }

    uint32 timebase32 = 0;
    uint32 estServerClock = 0;
    uint32 clientClock = 0;
    bool overflowFlag = false;
    uint32 minPercentOccupancy = 100;

    /*
     * Get current estimated server clock in milliseconds
     */
    iEstimatedServerClock->GetCurrentTime32(estServerClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);

    /*
     * Get current client playback clock in milliseconds
     */
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);

    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - EstServClock=%d", estServerClock));
    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - ClientClock=%d", clientClock));

    if (oSessionDurationExpired)
    {
        /*
         * No check needed - We are past the clip time, just play out the last
         * bit in the jitter buffer
         */
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Session Duration Expired"));
        if (oDelayEstablished == false)
        {
            /*
             * Coming out of rebuffering in case we had gone into
             * rebuffering just before
             */
            UpdateRebufferingStats(PVMFInfoDataReady);
            iJitterDelayPercent = 100;
            ReportInfoEvent(PVMFInfoBufferingStatus);
            ReportInfoEvent(PVMFInfoDataReady);
            ReportInfoEvent(PVMFInfoBufferingComplete);
            CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Cancelling Jitter Buffer Duration Timer"));
            CancelEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
        }
        oDelayEstablished = true;
    }
    else
    {
        if (estServerClock < clientClock)
        {
            /* Could happen during repositioning */
            if (oDelayEstablished == true)
            {
                aClockDiff = 0;
                oDelayEstablished = false;
                iJitterDelayPercent = 0;
                /* Start timer */
                RequestEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);

                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Starting Jitter Buffer Duration Timer"));
                RequestEventCallBack(JB_BUFFERING_DURATION_COMPLETE);

                if (oStartPending == false)
                {
                    /* Cancel session duration timer while rebuffering */
                    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::IsJitterBufferReady - Cancelling Session Duration Timer"));
                    iSessionDurationTimer->Cancel();

                    UpdateRebufferingStats(PVMFInfoUnderflow);
                    ReportInfoEvent(PVMFInfoUnderflow);
                    ReportInfoEvent(PVMFInfoBufferingStart);
                    ReportInfoEvent(PVMFInfoBufferingStatus);
                }
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - EstServClock=%d",
                                      Oscl_Int64_Utils::get_uint64_lower32(estServerClock)));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - ClientClock=%d",
                                      Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Estimated Serv Clock Less Than ClientClock!!!!"));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - EstServClock=%d",
                                             Oscl_Int64_Utils::get_uint64_lower32(estServerClock)));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - ClientClock=%d",
                                             Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Estimated Serv Clock Less Than ClientClock!!!!"));
            }
            return oDelayEstablished;
        }


        uint32 diff32ms = estServerClock - clientClock;
        aClockDiff = diff32ms;
            if (diff32ms >= iJitterBufferDurationInMilliSeconds)
            {
                for (uint32 i = 0; i < iPortParamsQueue.size(); i++)
                {
                    PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

                    if (portParams.iBufferAlloc != NULL)
                    {
                        uint32 largestContiguousFreeBlockSize = portParams.iBufferAlloc->getLargestContiguousFreeBlockSize();
                        uint32 jbSize = portParams.iBufferAlloc->getBufferSize();
                        if ((largestContiguousFreeBlockSize*100 / jbSize) < minPercentOccupancy)
                        {
                            minPercentOccupancy = (uint32)(largestContiguousFreeBlockSize * 100 / jbSize);
                        }
                    }
                }

                if ((prevMinPercentOccupancy < MIN_PERCENT_OCCUPANCY_THRESHOLD) && (minPercentOccupancy < MIN_PERCENT_OCCUPANCY_THRESHOLD))
                {
                    consecutiveLowBufferCount++;
                }
                else
                {
                    consecutiveLowBufferCount = 0;
                }
                prevMinPercentOccupancy = minPercentOccupancy;
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - minPercentOccupancy=%d, consecutiveLowBufferCount=%d",
                                      minPercentOccupancy,
                                      consecutiveLowBufferCount));


                if ((diff32ms > JITTER_BUFFER_DURATION_MULTIPLIER_THRESHOLD*iJitterBufferDurationInMilliSeconds) && !iOverflowFlag && (consecutiveLowBufferCount > CONSECUTIVE_LOW_BUFFER_COUNT_THRESHOLD))
                {
                    iOverflowFlag = true;
                    ReportInfoEvent(PVMFInfoSourceOverflow);
                    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady reporting PVMFInfoSourceOverflow"));
                }
                if (oDelayEstablished == false && aJitterBuffer->CheckNumElements() == true)
                {
                    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Cancelling Jitter Buffer Duration Timer"));
                    oDelayEstablished = true;
                    CancelEventCallBack(JB_BUFFERING_DURATION_COMPLETE);
                    /* Coming out of rebuffering */
                    UpdateRebufferingStats(PVMFInfoDataReady);
                    iJitterDelayPercent = 100;
                    ReportInfoEvent(PVMFInfoBufferingStatus);
                    ReportInfoEvent(PVMFInfoDataReady);
                    ReportInfoEvent(PVMFInfoBufferingComplete);
                    CancelEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
                    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - EstServClock=%d", estServerClock));
                    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - ClientClock=%d",  clientClock));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - EstServClock=%d",
                                                 estServerClock));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - ClientClock=%d",
                                                 clientClock));
                    if (iUseSessionDurationTimerForEOS == true)
                    {
                        ComputeCurrentSessionDurationMonitoringInterval();
                        iSessionDurationTimer->Start();
                    }
                    else
                    {
                        iSessionDurationTimer->Stop();
                    }
                }
                else if (oDelayEstablished == false && aJitterBuffer->CheckNumElements() == false)
                {
                    iJitterDelayPercent = 0;
                }
                else
                    iJitterDelayPercent = 100;
            }
            else
            {
                /*
                 * Update the buffering percent - to be used while sending buffering
                 * status events, in case we go into rebuffering or if we are in buffering
                 * state.
                 */
                iJitterDelayPercent = ((diff32ms * 100) / iJitterBufferDurationInMilliSeconds);
                if (oDelayEstablished == true)
                {
                    if (diff32ms <= iJitterBufferUnderFlowThresholdInMilliSeconds)
                    {
                        /* Implies that we are going into rebuffering */
                        if (oSessionDurationExpired == false)
                        {
                            oDelayEstablished = false;
                            /* Start timer */
                            RequestEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);

                            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Starting Jitter Buffer Duration Timer"));
                            RequestEventCallBack(JB_BUFFERING_DURATION_COMPLETE);

                            /* Supress underflow if start pending */
                            if (oStartPending == false)
                            {
                                /* Cancel session duration timer while rebuffering */
                                PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::IsJitterBufferReady - Going into Rebuffering - Cancelling Session Duration Timer"));
                                iSessionDurationTimer->Cancel();

                                UpdateRebufferingStats(PVMFInfoUnderflow);
                                ReportInfoEvent(PVMFInfoUnderflow);
                                ReportInfoEvent(PVMFInfoBufferingStart);
                                ReportInfoEvent(PVMFInfoBufferingStatus);

                                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - EstServClock=%d", estServerClock));
                                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - ClientClock=%d",  clientClock));
                                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady: Jitter Delay not met"));
                                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - EstServClock=%d",
                                                             estServerClock));
                                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - ClientClock=%d",
                                                             clientClock));

                                if (iClientPlayBackClock != NULL)
                                    iClientPlayBackClock->Pause();
                            }
                        }
                        /* we are past the end of the clip, no more rebuffering */
                    }
                }
                if (oDelayEstablished == false && aJitterBuffer->CheckNumElements() == false)
                {
                    iJitterDelayPercent = 0;
                }
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady: Delay Percent = %d", iJitterDelayPercent));
            }
        /* if we are not rebuffering check for flow control */
        PVMFPortInterface* aPort = findPortForJitterBuffer(aJitterBuffer);
        if (aPort != NULL)
        {
            /* Check for flow control if we are not rebuffering */
            PVMFStatus status;
            bool lowWaterMarkReached = false;
            status = CheckForLowWaterMark(aPort, lowWaterMarkReached);
            if ((status == PVMFSuccess) && (lowWaterMarkReached == true))
            {
                {
                    /*
                     * Check for jitter buffer occupancy. If the jitter buffer is in
                     * the danger of underflowing start send an event to resume.
                     */
                    /* Send just once */
                    if (oAutoPause == true)
                    {
                        oAutoPause = false;
                        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
                        for (it = iPortParamsQueue.begin();
                                it != iPortParamsQueue.end();
                                it++)
                        {
                            if (it->oMonitorForRemoteActivity == false)
                            {
                                it->oMonitorForRemoteActivity = true;
                                CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                                RequestEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                            }
                        }
                        ReportInfoEvent(PVMFJitterBufferNodeJitterBufferLowWaterMarkReached);
                        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::IsJitterBufferReady: Sending Auto Resume"));
                    }
                }
            }
        }
    }
    /* send start complete, if start pending */
    if (oDelayEstablished)
    {
        if (oStartPending)
        {
            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Sending Start Complete"));
            CompleteStart();
        }
    }
    return (oDelayEstablished);
}

PVMFStatus
PVMFJitterBufferNode::SendData(PVMFPortInterface* aPort)
{

    PVMFJitterBufferPort* jbPort =
        OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    PVMFPortInterface* outputPort = jbPort->iPortCounterpart;
    PVMFJitterBufferPortParams* portParamsPtr = jbPort->iPortParams;
    PVMFJitterBufferPortParams* outPortParamsPtr = jbPort->iCounterpartPortParams;
    PVMFJitterBuffer* jitterBuffer = portParamsPtr->iJitterBuffer;

    if (jitterBuffer == NULL)
    {
        return PVMFFailure;
    }

    uint32 clockDiff = 0;
    bool oJitterBufferReady = IsJitterBufferReady(portParamsPtr, clockDiff);
    if (oJitterBufferReady == false)
    {
        /* Jitter Buffer not ready - dont send data */
        return PVMFErrNotReady;
    }
    if (oStopOutputPorts == true)
    {
        /*
         * output ports paused
         */
        return PVMFSuccess;
    }

    while (oJitterBufferReady == true)
    {
        if (outputPort->IsOutgoingQueueBusy())
        {
            /* Cant send */
            outPortParamsPtr->oProcessOutgoingMessages = false;
            PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::SendData: Output Queue Busy %x outport - Mime=%s", outputPort, outPortParamsPtr->iMimeType.get_cstr()));
            return PVMFErrBusy;
        }

        PVMFSharedMediaDataPtr mediaOut;
        PVMFSharedMediaMsgPtr mediaOutMsg;
        /* Check if there are any pending media commands */
        if (jitterBuffer->CheckForPendingCommands(mediaOutMsg) == true)
        {
            PVMFStatus status = outputPort->QueueOutgoingMsg(mediaOutMsg);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
            uint32 timebase32 = 0;
            uint32 clientClock32 = 0;
            bool overflowFlag = false;

            if (iClientPlayBackClock != NULL)
                iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Media Command Sent, Mime=%s, CmdId=%d, TS=%d, SEQNUM= %d, ClientClock= %2d",
                                            portParamsPtr->iMimeType.get_cstr(), mediaOutMsg->getFormatID(), mediaOutMsg->getTimestamp(), mediaOutMsg->getSeqNum(), clientClock32));
#endif
            if (status != PVMFSuccess)
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::SendData: Media Command Send Error"));
                return status;
            }
        }

        PVMFJitterBufferStats stats = jitterBuffer->getJitterBufferStats();

        if (stats.currentOccupancy > 0)
        {
            portParamsPtr->oJitterBufferEmpty = false;
            PVMFTimestamp ts;
            uint32 converted_ts;
            uint32 in_wrap_count = 0;
            /*
            * Check and see if the current read position is pointing
            * to a hole in the jitter buffer due to packet loss
            */
            if (jitterBuffer->CheckCurrentReadPosition() == false)
            {
                /*
                 * Peek Next timestamp
                 */
                ts = jitterBuffer->peekNextElementTimeStamp();

                /*
                 * Convert Time stamp to milliseconds
                 */
                portParamsPtr->mediaClockConverter.set_clock(ts, in_wrap_count);
                converted_ts =
                    portParamsPtr->mediaClockConverter.get_converted_ts(1000);
                /*
                 * Get current client playback clock in milliseconds
                 */
                uint32 clientClock;
                bool overflowFlag = false;
                if (iClientPlayBackClock != NULL)
                    iClientPlayBackClock->GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);


                uint32 estimatedServClock = 0;
                overflowFlag = false;
                iEstimatedServerClock->GetCurrentTime32(estimatedServClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);
                uint32 delta = 0;
                if (!oSessionDurationExpired && (PVTimeComparisonUtils::IsEarlier(estimatedServClock, converted_ts + iEstimatedServerKeepAheadInMilliSeconds, delta) && (delta > 0)))
                {
                    //hold the available data packet, and wait for hole in the JB due to OOO packet to be filled
                    if (!IsCallbackPending(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, portParamsPtr))
                    {
                        PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData Detected Hole in JB PeekTs[%d] clientClock[%d] , ClientClockState [%d], estimatedServClock[%d], EstimatedServClockState[%d], oSessionDurationExpired[%d] MimeStr[%s]", converted_ts, clientClock, iClientPlayBackClock->GetState(), estimatedServClock, iEstimatedServerClock->GetState(), oSessionDurationExpired, portParamsPtr->iMimeType.get_cstr()));
                        RequestEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE , delta, portParamsPtr);
                    }
                    return PVMFErrNotReady;
                }
            }

            //Cancel pending sequencing OOO packet callback (if any)
            if (IsCallbackPending(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, portParamsPtr))
            {
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, portParamsPtr);
            }

            mediaOut = jitterBuffer->retrievePacket();

            PVMFStatus status = PVMFSuccess;
            if (mediaOut.GetRep() != NULL)
            {
                /* set time stamp in milliseconds always */
                ts = mediaOut->getTimestamp();
                portParamsPtr->mediaClockConverter.set_clock(ts, in_wrap_count);
                converted_ts =
                    portParamsPtr->mediaClockConverter.get_converted_ts(1000);
                mediaOut->setTimestamp(converted_ts);
                mediaOut->setFormatSpecificInfo(portParamsPtr->iTrackConfig);
                convertToPVMFMediaMsg(mediaOutMsg, mediaOut);
                status = outputPort->QueueOutgoingMsg(mediaOutMsg);

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                uint32 timebase32 = 0;
                uint32 clientClock32 = 0;
                uint32 serverClock32 = 0;
                bool overflowFlag = false;
                if (iClientPlayBackClock != NULL)
                    iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
                outPortParamsPtr->iLastMsgTimeStamp = converted_ts;

                //PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: EstServClock= %2d", serverClock64));
                PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Mime=%s, SSRC=%d, TS=%d, SEQNUM= %d, ClientClock= %2d",
                                                portParamsPtr->iMimeType.get_cstr(),
                                                mediaOutMsg->getStreamID(),
                                                mediaOutMsg->getTimestamp(),
                                                mediaOutMsg->getSeqNum(),
                                                clientClock32));

                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::SendData:"
                                                     "MaxSNReg=%d, MaxTSReg=%u, LastSNRet=%d, LastTSRet=%u, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
                                                     stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                                     stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                                     (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum),
                                                     serverClock32,
                                                     clientClock32));
#endif

                if (status != PVMFSuccess)
                {
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::SendData: Error"));
                    return (status);
                }
            }
        }
        else
        {
            /*
             * Jitter buffer empty - We do not go into rebuffering unless
             * the delay criteria has not been violated. There is no data
             * to send, but that is alright.
             */
            //PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Jitter Buffer Empty - Mime=%s", portParamsPtr->iMimeType.get_cstr()));
            portParamsPtr->oJitterBufferEmpty = true;

            if ((iMonitorReBufferingCallBkPending == false) && (oSessionDurationExpired == false))
            {
                RequestEventCallBack(JB_MONITOR_REBUFFERING, (clockDiff - iJitterBufferUnderFlowThresholdInMilliSeconds));
            }

            if (oAutoPause == false)
            {
            }

            /*
             * jitter buffer empty, send an event to resume in case we had auto paused
             */
            bool lowWaterMarkReached = false;
            PVMFStatus status = CheckForLowWaterMark(aPort, lowWaterMarkReached);
            if ((status == PVMFSuccess) && (lowWaterMarkReached == true))
            {
                ReportInfoEvent(PVMFJitterBufferNodeJitterBufferLowWaterMarkReached);
                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::SendData: Sending Auto Resume - Mime=%s", portParamsPtr->iMimeType.get_cstr()));
            }
            return PVMFSuccess;
        }
        //reevalute jitter
        oJitterBufferReady = IsJitterBufferReady(portParamsPtr, clockDiff);
        if (oJitterBufferReady == false)
        {
            /* Jitter Buffer not ready - dont send data */
            return PVMFErrNotReady;
        }
    }
    return PVMFSuccess;
}

PVMFStatus
PVMFJitterBufferNode::CheckJitterBufferEmpty(bool& oEmpty)
{
    /*
     * Checks to see if all jitter buffers are empty. If they are
     * then it would stop scheduling the jitter buffer node.
     */
    oEmpty = false;
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (it->oJitterBufferEmpty == true)
            {
                oEmpty = true;
            }
        }
    }

    return PVMFSuccess;
}

PVMFStatus PVMFJitterBufferNode::CheckForEOS()
{
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    PVMFStatus status = PVMFSuccess;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if ((PVMF_JITTER_BUFFER_PORT_TYPE_INPUT == it->tag) && (it->oJitterBufferEmpty) && (!it->oEOSReached) && oSessionDurationExpired)
        {
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::CheckForEOS: Mime is %s" , it->iMimeType.get_cstr()));
            status = GenerateAndSendEOSCommand(it->iPort);
        }
        else if ((PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK == it->tag) && (it->iRTCPStats.oRTCPByeRecvd) && (!oSessionDurationExpired))
        {
            //look for corresponding input port for the feedback port on which RTCP bye is received.
            PVMFJitterBufferPortParams* inputPortParam = NULL;
            LocateInputPortForFeedBackPort(it, inputPortParam);
            if (inputPortParam->oJitterBufferEmpty && (!inputPortParam->oEOSReached))
            {
                PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::CheckForEOS: Mime is %s" , it->iMimeType.get_cstr()));
                GenerateAndSendEOSCommand(inputPortParam->iPort);
            }
        }
    }

    //Check if EOS is send on all the input ports...
    bool bEOSSendOnAllInputPorts = true;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if ((PVMF_JITTER_BUFFER_PORT_TYPE_INPUT == it->tag) && (!it->oEOSReached))
            bEOSSendOnAllInputPorts = false;
    }

    if (bEOSSendOnAllInputPorts)
    {
        oStopOutputPorts = true;
        oSessionDurationExpired  = true;
    }

    return status;
}


bool
PVMFJitterBufferNode::setPlayRange(int32 aStartTimeInMS,
                                   int32 aStopTimeInMS,
                                   bool oPlayAfterASeek,
                                   bool aStopTimeAvailable)
{
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            it->iJitterBuffer->setPlayRange(aStartTimeInMS, aStopTimeInMS);
            if (oPlayAfterASeek)
            {
                it->iJitterBuffer->SetEOS(false);
            }
        }
        else if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
        {
            /* Start RTCP Timer */
            ActivateTimer(it);
            if (oPlayAfterASeek)
            {
                it->iRTCPStats.oRTCPByeRecvd = false;
            }
        }
        if (oPlayAfterASeek)
        {
            it->oUpStreamEOSRecvd = false;
            it->oEOSReached = false;
        }
    }

    iPlayStartTimeInMS = aStartTimeInMS;
    iPlayStopTimeInMS = aStopTimeInMS;
    iPlayStopTimeAvailable = aStopTimeAvailable;
    if (iPlayStopTimeAvailable == true)
    {
        /* Start Session Duration Timer only if stop duration is set */
        if ((iUseSessionDurationTimerForEOS == true) && (!oSessionDurationExpired || (oPlayAfterASeek)))
        {
            iSessionDurationTimer->Stop();
            oSessionDurationExpired = false;
            iSessionDurationTimer->setSessionDurationInMS(((iPlayStopTimeInMS - iPlayStartTimeInMS) + PVMF_EOS_TIMER_GAURD_BAND_IN_MS));
            ComputeCurrentSessionDurationMonitoringInterval();
            iSessionDurationTimer->Start();
        }
        // Only at Prepare state, this API is called by streaming manager node
        // We set thinning interval in here
#if 1
        //thinning timer value depends on clip duration
        uint32 duration = iPlayStopTimeInMS - iPlayStartTimeInMS;
        if (duration < PVMF_JITTER_BUFFER_NODE_THINNING_MIN_DURATION_MS)
        {
            iThinningIntervalMS = duration;
        }
        else
        {
            iThinningIntervalMS = (duration / 100) * PVMF_JITTER_BUFFER_NODE_THINNING_PERCENT;
        }
    }
    else
    {
        //in case of live streaming, hard-coded value is set
        iThinningIntervalMS = PVMF_JITTER_BUFFER_NODE_THINNING_LIVE_INTERVAL_MS;
    }
#else
    }
    //thinning timer value is hard-corded for any cases
    iThinningIntervalMS = 15 * 1000;
#endif

    return true;
}

void
PVMFJitterBufferNode::ActivateTimer(PVMFJitterBufferPortParams* pPort)
{
    if (pPort->RtcpBwConfigured && (pPort->RR == 0))
    {
        // RTCP has been disabled
        pPort->iRTCPTimer->setRTCPInterval(0);
        return;
    }

    pPort->iRTCPIntervalInMicroSeconds = CalcRtcpInterval(pPort);
    pPort->iRTCPTimer->setRTCPInterval(iRTCPIntervalInMicroSeconds);
    pPort->iRTCPTimer->Start();
}
void PVMFJitterBufferNode::setRTCPIntervalInMicroSecs(uint32 aRTCPInterval)
{
    OSCL_UNUSED_ARG(aRTCPInterval);
}

/*
 * Called by PvmfRtcpTimer class, on RTCP timer expiry
 */
void PVMFJitterBufferNode::RtcpTimerEvent(PvmfRtcpTimer* pTimer)
{
    /*
     * Generate and send RRs here. This is not a MIPS
     * intensive operation. Hence can be performed in
     * the observer call back itself. This also gaurantees
     * that the RRs are sent during initial buffering phase
     * as well.
     */

    // find the associated port with the expired timer
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->iRTCPTimer == pTimer) break;
    }

    OSCL_ASSERT(it != iPortParamsQueue.end());

    if (it->iRTCPStats.oRTCPByeRecvd == false)
    {
        // timer reconsideration
        uint32 timer = CalcRtcpInterval(it);
        if (timer > it->iRTCPIntervalInMicroSeconds)
        {
            it->iRTCPTimer->RunIfNotReady(timer - it->iRTCPIntervalInMicroSeconds);
            it->iRTCPIntervalInMicroSeconds = timer;
            return;
        }
        else
        {
            GenerateRTCPRR(it);
        }
    }
}


/*
 * Called by PvmfJBSessionDurationTimer on session duration timer expiry
 */
void PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent()
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent ---------------"));
    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Session Duration Timer Expired"));
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Session Duration Timer Expired"));
    /* Check if the estimated server clock is past the expected value */
    uint32 expectedEstServClockVal =
        iSessionDurationTimer->GetExpectedEstimatedServClockValAtSessionEnd();
    uint32 timebase32 = 0;
    uint32 estServClock = 0;
    bool overflowFlag = false;

    iEstimatedServerClock->GetCurrentTime32(estServClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - CurrEstServClock = %2d", estServClock));
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - ExpectedEstServClock = %2d", expectedEstServClockVal));
    if (estServClock >= expectedEstServClockVal)
    {
        PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent estServClock[%d] expectedEstServClockVal[%d]", estServClock, expectedEstServClockVal));
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Session Duration Has Elapsed"));
        oSessionDurationExpired = true;
        CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
        /* Cancel clock update notifications */
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
        {
            CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);

            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                if (it->iJitterBuffer != NULL)
                {
                    it->iJitterBuffer->CancelServerClockNotificationUpdates();
                }
                else
                {
                    OSCL_ASSERT(false);
                }
            }
        }
        /* Pause Estimated server clock & RTCP Clock */
        iEstimatedServerClock->Pause();
        iRTCPClock->Pause();
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    else
    {
        /*
         * Since we monitor the session duration in intervals, it is possible that this call back
         * happens when one such interval expires
         */
        uint64 elapsedTime = iSessionDurationTimer->GetMonitoringIntervalElapsed();
        uint32 elapsedTime32 = Oscl_Int64_Utils::get_uint64_lower32(elapsedTime);
        iSessionDurationTimer->UpdateElapsedSessionDuration(elapsedTime32);
        uint32 totalSessionDuration = iSessionDurationTimer->getSessionDurationInMS();
        uint32 elapsedSessionDurationInMS = iSessionDurationTimer->GetElapsedSessionDurationInMS();
        if (elapsedSessionDurationInMS < totalSessionDuration)
        {
            uint32 interval = (totalSessionDuration - elapsedSessionDurationInMS);
            if (interval > PVMF_JITTER_BUFFER_NODE_SESSION_DURATION_MONITORING_INTERVAL_MAX_IN_MS)
            {
                interval = PVMF_JITTER_BUFFER_NODE_SESSION_DURATION_MONITORING_INTERVAL_MAX_IN_MS;
            }
            iSessionDurationTimer->setCurrentMonitoringIntervalInMS(interval);
            iSessionDurationTimer->ResetEstimatedServClockValAtLastCancel();
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - TotalDuration=%d, ElapsedDuration=%d, CurrMonitoringInterval=%d", totalSessionDuration, elapsedSessionDurationInMS, interval));
        }
        else
        {
            /*
             * 1) Register for est serv clock update notifications on all jitter buffers
             * 2) Reschedule the session duration timer
             */
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Past Session End Time - Starting to monitor Estimated Server Clock"));
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    if (it->iJitterBuffer != NULL)
                    {
                        it->iJitterBuffer->NotifyServerClockUpdates(this, NULL);
                    }
                    else
                    {
                        OSCL_ASSERT(false);
                    }
                }
            }
            uint64 diff = (expectedEstServClockVal - estServClock);
            uint32 diff32 = Oscl_Int64_Utils::get_uint64_lower32(diff);
            /*
             * This is intentional. We do not expect the session duration and monitoring
             * intervals to exceed the max timer limit of 32 mins
             */
            iSessionDurationTimer->setSessionDurationInMS(diff32);
            iSessionDurationTimer->setCurrentMonitoringIntervalInMS(diff32);
            iSessionDurationTimer->ResetEstimatedServClockValAtLastCancel();
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - EstServClock=%d, ExpectedEstServClock=%d", Oscl_Int64_Utils::get_uint64_lower32(expectedEstServClockVal), Oscl_Int64_Utils::get_uint64_lower32(estServClock)));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - TotalDuration=%d, Interval=%d", diff32, diff32));
        }
        iSessionDurationTimer->Start();
    }
    return;
}

PVMFStatus PVMFJitterBufferNode::NotifyOutOfBandEOS()
{
    // Ignore Out Of Band EOS for any Non Live stream
    if (iPlayStopTimeAvailable == false)
    {
        if (iJitterBufferState != PVMF_JITTER_BUFFER_IN_TRANSITION)
        {
            oSessionDurationExpired = true;
            CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
            {
                CancelEventCallBack(JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE, it);
            }
            iSessionDurationTimer->Cancel();
            PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Out Of Band EOS Recvd"));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Out Of Band EOS Recvd"));
        }
        else
        {
            PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Ignoring Out Of Band EOS in Transition State"));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Ignoring Out Of Band EOS in Transition State"));
        }
    }
    else
    {
        PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Ignoring Out Of Band EOS in Transition State"));
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Ignoring Out Of Band EOS in Transition State"));
    }
    return PVMFSuccess;
}

PVMFStatus PVMFJitterBufferNode::SendBOSMessage(uint32 aStreamID)
{
    iStreamID = aStreamID;
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            QueueBOSCommand(it->iPort);
        }
    }
    PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::SendBOSMessage - BOS Recvd"));
    return PVMFSuccess;
}

bool PVMFJitterBufferNode::QueueBOSCommand(PVMFPortInterface* aPort)
{
    PVMFPortInterface* outputPort = getPortCounterpart(aPort);

    PVMFJitterBufferPortParams* portParamsPtr;
    if (!getPortContainer(outputPort, portParamsPtr))
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::QueueBOSCommand: Error - getPortContainer failed", this));
        return PVMFFailure;
    }

    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();
    // Set the formatID, timestamp, sequenceNumber and streamID for the media message
    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_BOS_FORMAT_ID);
    uint32 seqNum = 0;
    sharedMediaCmdPtr->setSeqNum(seqNum);
    sharedMediaCmdPtr->setStreamID(iStreamID);

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);

    PVMFJitterBuffer* jitterBuffer = findJitterBuffer(aPort);
    jitterBuffer->addMediaCommand(mediaMsgOut);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFJitterBufferNode::QueueBOSCommand() MIME=%s StreamID=%d", portParamsPtr->iMimeType.get_cstr(), iStreamID));
    return true;
}


PVMFStatus
PVMFJitterBufferNode::GenerateAndSendEOSCommand(PVMFPortInterface* aPort)
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand"));
    OSCL_StackString<8> rtp(_STRLIT_CHAR("RTP"));
    OSCL_StackString<8> rdt(_STRLIT_CHAR("RDT"));

    PVMFJitterBufferPortParams* portParams;
    if (getPortContainer(aPort, portParams) == false)
    {
        ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: Error - Invalid Port"));
        return PVMFFailure;
    }
    PVMFPortInterface* outputPort = getPortCounterpart(aPort);
    if (outputPort == NULL)
    {
        ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: Error - Unable to find Output Port"));
        return PVMFFailure;
    }
    PVMFJitterBufferPortParams* outPortParams;
    if (getPortContainer(outputPort, outPortParams) == false)
    {
        ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: Error - Invalid Port"));
        return PVMFFailure;
    }

    /* Set EOS on jitter buffer so that we dont register any more packets */
    if (portParams->iJitterBuffer->GetEOS() == false)
    {
        portParams->iJitterBuffer->SetEOS(true);
        PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: EOS Set On Jitter Buffer - MimeType=%s", portParams->iMimeType.get_cstr()));
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: EOS Set On Jitter Buffer - MimeType=%s", portParams->iMimeType.get_cstr()));
    }

    if (outputPort->IsOutgoingQueueBusy() == true)
    {
        /* come back later */
        PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: Waiting - Output Queue Busy"));
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: Waiting - Output Queue Busy"));
        return PVMFErrBusy;
    }

    if ((oscl_strncmp(portParams->iTransportType.get_cstr(), rtp.get_cstr(), 3) == 0) ||
            (oscl_strncmp(portParams->iTransportType.get_cstr(), rdt.get_cstr(), 3) == 0))
    {
        if (portParams->oEOSReached == false)
        {
            PVMFSharedMediaCmdPtr sharedMediaCmdPtr =
                PVMFMediaCmd::createMediaCmd();

            sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

            sharedMediaCmdPtr->setStreamID(iStreamID);

            PVMFSharedMediaMsgPtr msg;
            convertToPVMFMediaCmdMsg(msg, sharedMediaCmdPtr);

            msg->setTimestamp(outPortParams->iLastMsgTimeStamp);

            PVMFStatus status = outputPort->QueueOutgoingMsg(msg);
            if (status != PVMFSuccess)
            {
                ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aPort));
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: Error - output queue busy"));
                return status;
            }
            uint32 timebase32 = 0;
            uint32 clientClock32 = 0;
            bool overflowFlag = false;
            if (iClientPlayBackClock != NULL)
                iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
            timebase32 = 0;
            uint32 estServClock32 = 0;
            iEstimatedServerClock->GetCurrentTime32(estServClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: MimeType=%s, StreamID=%d",
                                            portParams->iMimeType.get_cstr(),
                                            msg->getStreamID()));
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: ClientClock=%d",
                                            clientClock32));
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: EstServClock=%d",
                                            estServClock32));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: MimeType=%s, StreamID=%d",
                                                   portParams->iMimeType.get_cstr(),
                                                   msg->getStreamID()));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: ClientClock=%d",
                                                   clientClock32));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: EstServClock=%d",
                                                   estServClock32));
            portParams->oEOSReached = true;
            return (status);
        }
        else
        {
            PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand - EOS already sent..."));
        }
    }
    return PVMFSuccess;
}

PVMFStatus
PVMFJitterBufferNode::DestroyFireWallPacketMemAllocators(PVMFJitterBufferPortParams* aPortParams)
{
    if (aPortParams->iMediaMsgAlloc != NULL)
    {
        PVMF_JITTER_BUFFER_DELETE(NULL,
                                  OsclMemPoolFixedChunkAllocator,
                                  aPortParams->iMediaMsgAlloc);
        aPortParams->iMediaMsgAlloc = NULL;
    }

    if (aPortParams->iMediaDataImplAlloc != NULL)
    {
        PVMF_JITTER_BUFFER_DELETE(NULL,
                                  PVMFSimpleMediaBufferCombinedAlloc,
                                  aPortParams->iMediaDataImplAlloc);
        aPortParams->iMediaDataImplAlloc = NULL;
    }

    if (aPortParams->iMediaDataAlloc != NULL)
    {
        PVMF_JITTER_BUFFER_DELETE(NULL,
                                  OsclMemPoolFixedChunkAllocator,
                                  aPortParams->iMediaDataAlloc);
        aPortParams->iMediaDataAlloc = NULL;
    }
    return PVMFSuccess;
}


PVMFStatus
PVMFJitterBufferNode::CreateFireWallPacketMemAllocators(PVMFJitterBufferPortParams* aPortParams)
{
    OSCL_StackString<8> rtp(_STRLIT_CHAR("RTP"));
    if (oscl_strncmp(aPortParams->iMimeType.get_cstr(), rtp.get_cstr(), 3) == 0)
    {
        /* Destroy old ones, if any */
        DestroyFireWallPacketMemAllocators(aPortParams);

        int32 leavecode;
        leavecode = 0;
        OsclExclusivePtr<OsclMemPoolFixedChunkAllocator> mediaDataAllocAutoPtr;
        OSCL_TRY(leavecode,
                 PVMF_JITTER_BUFFER_NEW(NULL,
                                        OsclMemPoolFixedChunkAllocator,
                                        (PVMF_JITTER_BUFFER_NODE_FIREWALL_PKT_MEMPOOL_SIZE),
                                        aPortParams->iMediaDataAlloc));
        if (leavecode != 0)
        {
            return PVMFErrNoMemory;
        }
        mediaDataAllocAutoPtr.set(aPortParams->iMediaDataAlloc);

        leavecode = 0;
        OsclExclusivePtr<PVMFSimpleMediaBufferCombinedAlloc> mediaDataImplAllocAutoPtr;
        OSCL_TRY(leavecode,
                 PVMF_JITTER_BUFFER_NEW(NULL,
                                        PVMFSimpleMediaBufferCombinedAlloc,
                                        (aPortParams->iMediaDataAlloc),
                                        aPortParams->iMediaDataImplAlloc));
        if (leavecode != 0)
        {
            return PVMFErrNoMemory;
        }
        mediaDataImplAllocAutoPtr.set(aPortParams->iMediaDataImplAlloc);

        leavecode = 0;
        OSCL_TRY(leavecode,
                 PVMF_JITTER_BUFFER_NEW(NULL,
                                        OsclMemPoolFixedChunkAllocator,
                                        (PVMF_JITTER_BUFFER_NODE_FIREWALL_PKT_MEMPOOL_SIZE, PVMF_JITTER_BUFFER_NODE_MEDIA_MSG_SIZE),
                                        aPortParams->iMediaMsgAlloc));
        if (leavecode != 0)
        {
            return PVMFErrNoMemory;
        }
        mediaDataImplAllocAutoPtr.release();
        mediaDataAllocAutoPtr.release();
    }
    return PVMFSuccess;
}

void PVMFJitterBufferNode::CheckForFireWallRecv(bool &aComplete)
{
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (it->oFireWallPacketRecvd == false)
            {
                aComplete = false;
                return;
            }
        }
    }
    aComplete = true;
    return;
}

void PVMFJitterBufferNode::CheckForFireWallPacketAttempts(bool &aComplete)
{
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::CheckForFireWallPacketAttempts - Mime=%s, Count=%d, Max=%d",
                                it->iMimeType.get_cstr(), it->iFireWallPacketCount, iFireWallPacketInfo.iNumAttempts));

            if (it->iFireWallPacketCount < iFireWallPacketInfo.iNumAttempts)
            {
                aComplete = false;
                return;
            }
        }
    }
    aComplete = true;
    return;
}

PVMFStatus
PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend()
{
    if (iDisableFireWallPackets == true)
    {
        return PVMFErrArgument;
    }
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    int32 errcode = PVMFJitterBufferNodeFirewallPacketGenerationFailed;
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            it->oFireWallPacketRecvd = false;
            it->iFireWallPacketCount = 0;
            for (uint32 i = 0; i < iFireWallPacketInfo.iNumAttempts; i++)
            {
                if (it->oFireWallPacketRecvd == false)
                {
                    if (it->iPort->IsOutgoingQueueBusy() == false)
                    {
                        PVMFSharedMediaDataPtr fireWallPkt;
                        OsclSharedPtr<PVMFMediaDataImpl> mediaDataImpl;
                        bool retval;

                        if (iFireWallPacketInfo.iFormat == PVMF_JB_FW_PKT_FORMAT_PV)
                        {
                            retval = Allocate(it, fireWallPkt, mediaDataImpl, PVMF_JITTER_BUFFER_NODE_MAX_FIREWALL_PKT_SIZE);

                            if (retval == false)
                            {
                                return PVMFErrNoMemory;
                            }

                            fireWallPkt->setMediaFragFilledLen(0, PVMF_JITTER_BUFFER_NODE_MAX_FIREWALL_PKT_SIZE);

                            OsclRefCounterMemFrag refCntMemFrag;
                            mediaDataImpl->getMediaFragment(0, refCntMemFrag);

                            OsclMemoryFragment memFrag = refCntMemFrag.getMemFrag();
                            OsclBinOStreamBigEndian outstream;

                            outstream.Attach(1, &memFrag);

                            outstream << it->iFireWallPacketCount;
                            it->iFireWallPacketCount++;

                            outstream << it->SSRC;
                        }
                        else
                        {
                            retval = Allocate(it, fireWallPkt, mediaDataImpl, PVMF_JITTER_BUFFER_NODE_MAX_RTP_FIREWALL_PKT_SIZE);

                            if (retval == false)
                            {
                                return PVMFErrNoMemory;
                            }

                            fireWallPkt->setMediaFragFilledLen(0, PVMF_JITTER_BUFFER_NODE_MAX_RTP_FIREWALL_PKT_SIZE);

                            OsclRefCounterMemFrag refCntMemFrag;
                            mediaDataImpl->getMediaFragment(0, refCntMemFrag);

                            OsclMemoryFragment memFrag = refCntMemFrag.getMemFrag();
                            oscl_memset(memFrag.ptr, 0, memFrag.len);

                            it->iFireWallPacketCount++;
                        }
                        PVMFSharedMediaMsgPtr fireWallMsg;
                        convertToPVMFMediaMsg(fireWallMsg, fireWallPkt);

                        PVMFStatus status = it->iPort->QueueOutgoingMsg(fireWallMsg);
                        if (status != PVMFSuccess)
                        {
                            PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend: Error - output queue busy"));
                            PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend: Error - output queue busy"));
                        }
                        PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend: Sent FireWall Packet - TrackMimeType = %s", it->iMimeType.get_cstr()));
                        PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend: Sent FireWall Packet - TrackMimeType = %s", it->iMimeType.get_cstr()));
                    }
                    else
                    {
                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend: Input Port - Outgoing queue full"));
                        PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::ResetFireWallPacketInfoAndResend: Input Port - Outgoing queue full"));
                        ReportErrorEvent(PVMFErrPortProcessing, NULL, &eventuuid, &errcode);
                        return PVMFFailure;
                    }
                }
            }
        }
    }
    return PVMFSuccess;
}

PVMFStatus PVMFJitterBufferNode::SendFireWallPackets()
{
    if (iDisableFireWallPackets == true)
    {
        return PVMFErrArgument;
    }

    bool oComplete = false;
    CheckForFireWallPacketAttempts(oComplete);
    if (oComplete == true)
    {
        return PVMFSuccess;
    }

    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    int32 errcode = PVMFJitterBufferNodeFirewallPacketGenerationFailed;
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;

    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (it->oFireWallPacketRecvd == false)
            {
                if (it->iPort->IsOutgoingQueueBusy() == false)
                {
                    PVMFSharedMediaDataPtr fireWallPkt;
                    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImpl;
                    bool retval;

                    if (iFireWallPacketInfo.iFormat == PVMF_JB_FW_PKT_FORMAT_PV)
                    {
                        retval = Allocate(it, fireWallPkt, mediaDataImpl, PVMF_JITTER_BUFFER_NODE_MAX_FIREWALL_PKT_SIZE);

                        if (retval == false)
                        {
                            return PVMFErrNoMemory;
                        }

                        fireWallPkt->setMediaFragFilledLen(0, PVMF_JITTER_BUFFER_NODE_MAX_FIREWALL_PKT_SIZE);

                        OsclRefCounterMemFrag refCntMemFrag;
                        mediaDataImpl->getMediaFragment(0, refCntMemFrag);

                        OsclMemoryFragment memFrag = refCntMemFrag.getMemFrag();
                        OsclBinOStreamBigEndian outstream;

                        outstream.Attach(1, &memFrag);

                        outstream << it->iFireWallPacketCount;
                        it->iFireWallPacketCount++;

                        outstream << it->SSRC;
                    }
                    else
                    {
                        retval = Allocate(it, fireWallPkt, mediaDataImpl, PVMF_JITTER_BUFFER_NODE_MAX_RTP_FIREWALL_PKT_SIZE);

                        if (retval == false)
                        {
                            return PVMFErrNoMemory;
                        }

                        fireWallPkt->setMediaFragFilledLen(0, PVMF_JITTER_BUFFER_NODE_MAX_RTP_FIREWALL_PKT_SIZE);

                        OsclRefCounterMemFrag refCntMemFrag;
                        mediaDataImpl->getMediaFragment(0, refCntMemFrag);

                        OsclMemoryFragment memFrag = refCntMemFrag.getMemFrag();
                        oscl_memset(memFrag.ptr, 0, memFrag.len);

                        OsclBinOStreamBigEndian outstream;
                        outstream.Attach(1, &memFrag);

                        //Skip to start of SSRC
                        outstream.seekFromCurrentPosition(8);

                        //fill in the SSRC
                        outstream << it->SSRC;

                        it->iFireWallPacketCount++;
                    }
                    PVMFSharedMediaMsgPtr fireWallMsg;
                    convertToPVMFMediaMsg(fireWallMsg, fireWallPkt);

                    PVMFStatus status = it->iPort->QueueOutgoingMsg(fireWallMsg);
                    if (status != PVMFSuccess)
                    {
                        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::SendFireWallPackets: Error - output queue busy"));
                        PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::SendFireWallPackets: Error - output queue busy"));
                    }
                    PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::SendFireWallPackets: Sent FireWall Packet - TrackMimeType = %s", it->iMimeType.get_cstr()));
                    PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::SendFireWallPackets: Sent FireWall Packet - TrackMimeType = %s", it->iMimeType.get_cstr()));
                }
                else
                {
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::SendFireWallPackets: Input Port - Outgoing queue full"));
                    PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::SendFireWallPackets: Input Port - Outgoing queue full"));
                    ReportErrorEvent(PVMFErrPortProcessing, NULL, &eventuuid, &errcode);
                    return PVMFFailure;
                }
            }
        }
    }
    PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::SendFireWallPackets: Scheduling FW Timer - Val = %d ms",
                        iFireWallPacketInfo.iServerRoundTripDelayInMS));
    /* Reschedule the firewall timer for the next interval */
    RequestEventCallBack(JB_NOTIFY_SEND_FIREWALL_PACKET, iFireWallPacketInfo.iServerRoundTripDelayInMS);
    return PVMFSuccess;
}

bool PVMFJitterBufferNode::Allocate(PVMFJitterBufferPortParams* it, PVMFSharedMediaDataPtr& fireWallPkt, OsclSharedPtr<PVMFMediaDataImpl>& mediaDataImpl, const int size)
{
    int32 err;
    OSCL_TRY(err,
             mediaDataImpl = it->iMediaDataImplAlloc->allocate(size);
             fireWallPkt = PVMFMediaData::createMediaData(mediaDataImpl,
                           it->iMediaMsgAlloc);
            );


    if (err != OsclErrNone)
    {
        return false;
    }
    return true;
}


PVMFStatus PVMFJitterBufferNode::DecodeFireWallPackets(PVMFSharedMediaDataPtr aPacket,
        PVMFJitterBufferPortParams* aPortParamsPtr)
{
    PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::DecodeFireWallPackets: Recvd Packet - TrackMimeType = %s", aPortParamsPtr->iMimeType.get_cstr()));
    if (aPortParamsPtr->oFireWallPacketRecvd == false)
    {
        OsclRefCounterMemFrag refCntMemFrag;
        aPacket->getMediaFragment(0, refCntMemFrag);

        OsclMemoryFragment memFrag = refCntMemFrag.getMemFrag();
        OsclBinIStreamBigEndian instream;

        instream.Attach(1, &memFrag);

        uint32 count;
        uint32 ssrc;

        instream >> count;
        instream >> ssrc;

        if (ssrc == aPortParamsPtr->SSRC)
        {
            PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::DecodeFireWallPackets: Decoded FireWall Packet - TrackMimeType = %s", aPortParamsPtr->iMimeType.get_cstr()));
            PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::DecodeFireWallPackets: Decoded FireWall Packet - TrackMimeType = %s", aPortParamsPtr->iMimeType.get_cstr()));
            aPortParamsPtr->oFireWallPacketRecvd = true;
            bool oComplete = false;
            CheckForFireWallRecv(oComplete);
            if (oComplete == true)
            {
                if (iInterfaceState == EPVMFNodeInitialized)
                {
                    /* Complete Prepare */
                    OSCL_ASSERT(!iCurrentCommand.empty());
                    OSCL_ASSERT(iCurrentCommand.front().iCmd == PVMF_JITTER_BUFFER_NODE_PREPARE);
                    CompletePrepare();
                }
            }
            return PVMFSuccess;
        }
        /* Bad Packet */
        return PVMFErrCorrupt;
    }
    /* Mutiple packet response - ignore */
    return PVMFSuccess;
}

void PVMFJitterBufferNode::SetSharedBufferResizeParams(uint32 maxNumResizes,
        uint32 resizeSize)
{
    // make sure we're in a state that makes sense
    OSCL_ASSERT((iInterfaceState == EPVMFNodeCreated) ||
                (iInterfaceState == EPVMFNodeIdle) ||
                (iInterfaceState == EPVMFNodeInitialized));

    iMaxNumBufferResizes = maxNumResizes;
    iBufferResizeSize = resizeSize;
}

void PVMFJitterBufferNode::GetSharedBufferResizeParams(uint32& maxNumResizes,
        uint32& resizeSize)
{
    maxNumResizes = iMaxNumBufferResizes;
    resizeSize = iBufferResizeSize;
}
PVMFStatus
PVMFJitterBufferNode::CheckForHighWaterMark(PVMFPortInterface* aPort, bool& aHighWaterMarkReached)
{
    aHighWaterMarkReached = false;
    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    PVMFJitterBufferPortParams* portParamsPtr = jbPort->iPortParams;
    if (portParamsPtr == NULL)
    {
        if (!getPortContainer(aPort, portParamsPtr))
        {
            return PVMFFailure;
        }
        jbPort->iPortParams = portParamsPtr;
    }

    if (portParamsPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
    {
        /* If EOS Recvd, dont check */
        if (portParamsPtr->oUpStreamEOSRecvd == false)
        {
            if (portParamsPtr->eTransportType == PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP)
            {
                aHighWaterMarkReached = portParamsPtr->iJitterBuffer->CheckForHighWaterMark();
            }
        }
        return PVMFSuccess;
    }
    return PVMFErrArgument;
}

PVMFStatus
PVMFJitterBufferNode::CheckForLowWaterMark(PVMFPortInterface* aPort, bool& aLowWaterMarkReached)
{
    aLowWaterMarkReached = false;

    /* If you had not auto paused earlier, this check is not needed */
    if (oAutoPause == false)
    {
        return PVMFSuccess;
    }
    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    PVMFJitterBufferPortParams* portParamsPtr = jbPort->iPortParams;
    if (portParamsPtr == NULL)
    {
        if (!getPortContainer(aPort, portParamsPtr))
        {
            return PVMFFailure;
        }
        jbPort->iPortParams = portParamsPtr;
    }

    if (portParamsPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
    {
        /* If EOS Recvd, dont check */
        if (portParamsPtr->oUpStreamEOSRecvd == false)
        {
            if (portParamsPtr->eTransportType == PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP)
            {
                aLowWaterMarkReached = portParamsPtr->iJitterBuffer->CheckForLowWaterMark();
            }
        }
        return PVMFSuccess;
    }
    return PVMFErrArgument;
}

PVMFStatus PVMFJitterBufferNode::RequestMemCallBackForAutoResume(PVMFPortInterface* aPort)
{
    /* If you had not auto paused earlier, this check is not needed */
    if (oAutoPause == false)
    {
        return PVMFSuccess;
    }

    PVMFJitterBufferPort* jbPort = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    PVMFJitterBufferPortParams* portParamsPtr = jbPort->iPortParams;
    if (portParamsPtr == NULL)
    {
        if (!getPortContainer(aPort, portParamsPtr))
        {
            return PVMFFailure;
        }
        jbPort->iPortParams = portParamsPtr;
    }
    if (portParamsPtr->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
    {
        /* If EOS Recvd, dont check */
        if (portParamsPtr->oUpStreamEOSRecvd == false)
        {
            if (portParamsPtr->eTransportType == PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP)
            {
                //not supported as of now
                return PVMFErrNotSupported;
            }
        }
        return PVMFSuccess;
    }
    return PVMFErrArgument;
}

void PVMFJitterBufferNode::freeblockavailable(OsclAny* aContextData)
{
    OSCL_UNUSED_ARG(aContextData);
    //should never get here
    OSCL_ASSERT(false);
}

void PVMFJitterBufferNode::freememoryavailable(OsclAny* aContextData)
{
    OSCL_UNUSED_ARG(aContextData);
    if (oAutoPause == true)
    {
        PVMFJitterBufferStats stats;
        oscl_memset(&stats, '0', sizeof(stats));
        oAutoPause = false;
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if (it->oMonitorForRemoteActivity == false)
            {
                it->oMonitorForRemoteActivity = true;
                CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                RequestEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
            }
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                stats = it->iJitterBuffer->getJitterBufferStats();
            }
        }
        ReportInfoEvent(PVMFJitterBufferNodeJitterBufferLowWaterMarkReached);
        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::freememoryavailable: Sending Auto Resume"));

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
        uint32 timebase32 = 0;
        uint32 estServerClock = 0;
        uint32 clientClock = 0;
        bool overflowFlag = false;
        iEstimatedServerClock->GetCurrentTime32(estServerClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
        if (iClientPlayBackClock != NULL)
            iClientPlayBackClock->GetCurrentTime32(clientClock, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);

        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::freememoryavailable - Auto Resume:"
                                             "MaxSNReg=%d, MaxTSReg=%u, LastSNRet=%d, LastTSRet=%u, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
                                             stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                             stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                             (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum),
                                             estServerClock,
                                             clientClock));
#endif
    }
}

void PVMFJitterBufferNode::chunkdeallocated(OsclAny* aContextData)
{
    OSCL_UNUSED_ARG(aContextData);

    PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJitterBufferNode::chunkdeallocated"));

    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            SendData(it->iPort);
        }
    }
    if (IsAdded())
    {
        RunIfNotReady();
    }
}

void PVMFJitterBufferNode::UpdateRebufferingStats(PVMFEventType aEventType)
{
    if (aEventType == PVMFInfoUnderflow)
    {
        if (oAutoPause == true)
        {
            oAutoPause = false;
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (it->oMonitorForRemoteActivity == false)
                {
                    it->oMonitorForRemoteActivity = true;
                    CancelEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                    RequestEventCallBack(JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED);
                }
            }
            ReportInfoEvent(PVMFJitterBufferNodeJitterBufferLowWaterMarkReached);
            PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::UpdateRebufferingStats: Sending Auto Resume"));
        }
        iNumUnderFlow++;
    }
}


void PVMFJitterBufferNode::LogSessionDiagnostics()
{
    if (iDiagnosticsLogged == false)
    {
        iDiagnosticsLogger = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");

        LogPortDiagnostics();

        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
        {
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                PVMFJitterBuffer* jitterBuffer = findJitterBuffer(it->iPort);
                if (jitterBuffer != NULL)
                {
                    PVMFJitterBufferStats jbStats = jitterBuffer->getJitterBufferStats();
                    uint32 in_wrap_count = 0;
                    uint32 max_ts_reg = jbStats.maxTimeStampRegistered;
                    it->mediaClockConverter.set_clock(max_ts_reg, in_wrap_count);

                    in_wrap_count = 0;
                    uint32 max_ts_ret = jbStats.maxTimeStampRetrieved;
                    it->mediaClockConverter.set_clock(max_ts_ret, in_wrap_count);

                    uint32 currentTime32 = 0;
                    uint32 currentTimeBase32 = 0;
                    bool overflowFlag = false;
                    iEstimatedServerClock->GetCurrentTime32(currentTime32,
                                                            overflowFlag,
                                                            PVMF_MEDIA_CLOCK_MSEC,
                                                            currentTimeBase32);
                    uint32 bitrate32 = 0;
                    uint32 totalNumBytesRecvd = jbStats.totalNumBytesRecvd;
                    if (currentTime32 != 0)
                    {
                        bitrate32 = (totalNumBytesRecvd / currentTime32);
                    }

                    bitrate32 *= 8;

                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "JitterBuffer - TrackMime Type = %s", it->iMimeType.get_cstr()));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "Total Num Packets Recvd = %d", jbStats.totalNumPacketsReceived));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "Total Num Packets Registered Into JitterBuffer = %d", jbStats.totalNumPacketsRegistered));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "Total Num Packets Retrieved From JitterBuffer = %d", jbStats.totalNumPacketsRetrieved));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "MaxSeqNum Recvd = %d", jbStats.maxSeqNumReceived));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "MaxSeqNum Registered = %d", jbStats.maxSeqNumRegistered));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "MaxSeqNum Retrieved = %d", jbStats.lastRetrievedSeqNum));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "MaxTimeStamp Registered In MS = %d", it->mediaClockConverter.get_converted_ts(1000)));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "MaxTimeStamp Retrieved In MS = %d", it->mediaClockConverter.get_converted_ts(1000)));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "Total Number of Packets Lost = %d", jbStats.totalPacketsLost));
                    PVMF_JBNODE_LOGDIAGNOSTICS((0, "Estimated Bitrate = %d", bitrate32));
                }
            }
        }
        iDiagnosticsLogged = true;
    }
}

void PVMFJitterBufferNode::LogPortDiagnostics()
{
    iDiagnosticsLogger = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");

    PVMF_JBNODE_LOGDIAGNOSTICS((0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
    PVMF_JBNODE_LOGDIAGNOSTICS((0, "PVMFJitterBufferNode - iNumRunL = %d", iNumRunL));

    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
    {
        PvmfPortBaseImpl* ptr =
            OSCL_STATIC_CAST(PvmfPortBaseImpl*, it->iPort);
        PvmfPortBaseImplStats stats;
        ptr->GetStats(stats);

        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "PVMF_JITTER_BUFFER_PORT_TYPE_INPUT"));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingMsgRecv = %d", stats.iIncomingMsgRecv));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingMsgConsumed = %d", stats.iIncomingMsgConsumed));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingQueueBusy = %d", stats.iIncomingQueueBusy));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgQueued = %d", stats.iOutgoingMsgQueued));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgSent = %d", stats.iOutgoingMsgSent));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingQueueBusy = %d", stats.iOutgoingQueueBusy));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgDiscarded = %d", stats.iOutgoingMsgDiscarded));
        }
        else if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
        {
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK"));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingMsgRecv = %d", stats.iIncomingMsgRecv));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingMsgConsumed = %d", stats.iIncomingMsgConsumed));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingQueueBusy = %d", stats.iIncomingQueueBusy));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgQueued = %d", stats.iOutgoingMsgQueued));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgSent = %d", stats.iOutgoingMsgSent));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingQueueBusy = %d", stats.iOutgoingQueueBusy));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgDiscarded = %d", stats.iOutgoingMsgDiscarded));
        }
        else if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT)
        {
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT"));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingMsgRecv = %d", stats.iIncomingMsgRecv));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingMsgConsumed = %d", stats.iIncomingMsgConsumed));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iIncomingQueueBusy = %d", stats.iIncomingQueueBusy));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgQueued = %d", stats.iOutgoingMsgQueued));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgSent = %d", stats.iOutgoingMsgSent));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingQueueBusy = %d", stats.iOutgoingQueueBusy));
            PVMF_JBNODE_LOGDIAGNOSTICS((0, "iOutgoingMsgDiscarded = %d", stats.iOutgoingMsgDiscarded));
        }
    }
}

void PVMFJitterBufferNode::ComputeCurrentSessionDurationMonitoringInterval()
{
    /* Restart the session duration timer after accounting for any elapsed time */
    uint64 elapsedTime = iSessionDurationTimer->GetMonitoringIntervalElapsed();
    uint32 elapsedTime32 = Oscl_Int64_Utils::get_uint64_lower32(elapsedTime);
    iSessionDurationTimer->UpdateElapsedSessionDuration(elapsedTime32);
    uint32 totalSessionDuration = iSessionDurationTimer->getSessionDurationInMS();
    uint32 elapsedSessionDuration = iSessionDurationTimer->GetElapsedSessionDurationInMS();
    uint32 interval = (totalSessionDuration - elapsedSessionDuration);
    if (interval > PVMF_JITTER_BUFFER_NODE_SESSION_DURATION_MONITORING_INTERVAL_MAX_IN_MS)
    {
        interval = PVMF_JITTER_BUFFER_NODE_SESSION_DURATION_MONITORING_INTERVAL_MAX_IN_MS;
    }
    iSessionDurationTimer->setCurrentMonitoringIntervalInMS(interval);
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJBN::ComputeCurrentSessionDurationMonitoringInterval - TotalDuration=%d, ElapsedDuration=%d, CurrMonitoringInterval=%d", totalSessionDuration, elapsedSessionDuration, interval));
}

PVMFStatus PVMFJitterBufferNode::SetTransportType(PVMFPortInterface* aPort,
        OSCL_String& aTransportType)
{
    PVMFJitterBufferPortParams* portParams;
    if (!getPortContainer(aPort, portParams))
    {
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::SetTransportType: Error - GetPortContainer failed"));
        return PVMFErrArgument;
    }

    OSCL_StackString<8> rtp(_STRLIT_CHAR("RTP"));
    if (oscl_strncmp(aTransportType.get_cstr(), rtp.get_cstr(), 3) == 0)
    {
        portParams->iTransportType = rtp;
        portParams->eTransportType = PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP;
    }
    else
    {
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::SetTransportType: Error - Unrecognized Transport Type"));
        return PVMFErrArgument;
    }
    portParams->iMimeType = aTransportType.get_str();
    return PVMFSuccess;
}

bool PVMFJitterBufferNode::PurgeElementsWithNPTLessThan(NptTimeFormat &aNPTTime)
{
    if (aNPTTime.npt_format != NptTimeFormat::NPT_SEC)
    {
        return false;
    }

    uint32 i;
    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];
        portParams.iPort->ClearMsgQueues();
    }

    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];
        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portParams.iJitterBuffer != NULL)
            {
                //portParams.iJitterBuffer->FlushJitterBuffer();
                PVMFTimestamp baseTS = 1000 * aNPTTime.npt_sec.sec + aNPTTime.npt_sec.milli_sec;

                portParams.mediaClockConverter.set_clock_other_timescale(baseTS, 1000);
                baseTS = portParams.mediaClockConverter.get_current_timestamp();


                portParams.iJitterBuffer->PurgeElementsWithTimestampLessThan(baseTS);
                /*
                 * Since we flushed the jitter buffer, set it to ready state,
                 * reset the delay flag
                 */
                oDelayEstablished = false;
                iJitterBufferState = PVMF_JITTER_BUFFER_READY;
                if (iOverflowFlag)
                {
                    iOverflowFlag = false;
                }
            }
        }
    }
    return true;
}

void PVMFJitterBufferNode::NotificationsInterfaceDestroyed()
{
}

void PVMFJitterBufferNode::FlushJitterBuffer()
{

    for (uint32 i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];
        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portParams.iJitterBuffer != NULL)
            {
                portParams.iJitterBuffer->FlushJitterBuffer();
            }
        }
    }

}

void PVMFJitterBufferNode::ClockStateUpdated()
{
    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ClockStateUpdated - iClientPlayBackClock[%d]", iClientPlayBackClock->GetState()));
    if (!oDelayEstablished)
    {
        // Don't let anyone start the clock while
        // we're rebuffering
        if (iClientPlayBackClock != NULL)
        {
            if (iClientPlayBackClock->GetState() == PVMFMediaClock::RUNNING)
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::ClockStateUpdated - Clock was started during rebuffering.  Pausing..."));
                iClientPlayBackClock->Pause();
            }
        }
    }
}
bool PVMFJitterBufferNode::CheckStateForRegisteringRTPPackets()
{
    /*
     * We register UDP packets only in started and paused state
     * oStartPending check is needed to handle the intial buffering case.
     * Node remains in prepared state till buffering is complete
     * Therefore this additional check ensures that we register packets
     * after a start has been recvd. If oStartPending is false, then
     * node state should either be EPVMFNodeStarted or EPVMFNodePaused
     * for us to register packets.
     */
    if ((iInterfaceState == EPVMFNodeStarted) ||
            (iInterfaceState == EPVMFNodePaused) ||
            (oStartPending == true))
    {
        return true;
    }
    return false;
}

bool PVMFJitterBufferNode::PrepareForRepositioning(bool oUseExpectedClientClockVal,
        uint32 aExpectedClientClockVal)
{
    bool overflowFlag = false;
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    uint32 timebase32 = 0;
    uint32 clientClock32 = 0;
    uint32 serverClock32 = 0;
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - Before - EstServClock=%d",
                                 serverClock32));
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - Before - ClientClock=%d",
                                 clientClock32));
#endif
    oAutoPause = false;
    iJitterBufferState = PVMF_JITTER_BUFFER_IN_TRANSITION;
    PVMFTimestamp ts = 0;
    if (oUseExpectedClientClockVal)
    {
        ts = aExpectedClientClockVal;
    }
    else
    {
        //reset player clock
        ts = getActualMediaDataTSAfterSeek();
    }
    if (iClientPlayBackClock != NULL)
    {
        iClientPlayBackClock->Stop();
        iClientPlayBackClock->SetStartTime32(ts,
                                             PVMF_MEDIA_CLOCK_MSEC, overflowFlag);
    }

    // Reset the following flags for the new repositioning
    oSessionDurationExpired = false;
    oDelayEstablished = false;


    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            it->iJitterBuffer->SetEOS(false);
        }
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
        {
            it->iRTCPStats.oRTCPByeRecvd = false;
        }
    }

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    timebase32 = 0;
    clientClock32 = 0;
    serverClock32 = 0;
    overflowFlag = false;
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC, timebase32);
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - After - EstServClock=%d",
                                 serverClock32));
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - After - ClientClock=%d",
                                 clientClock32));
#endif
    return true;
}

// Need to set jb state to ready when handing 404, 415 response
void PVMFJitterBufferNode::UpdateJitterBufferState()
{
    iJitterBufferState = PVMF_JITTER_BUFFER_READY;
    oDelayEstablished = true;
}

bool PVMFJitterBufferNode::RequestEventCallBack(JB_NOTIFY_CALLBACK aEventType, uint32 aDelay, OsclAny* aContext)
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "[0x%x]PVMFJitterBufferNode::RequestEventCallBack In aEventType[%d], aDelay[%d] aContext[0x%x]", this, aEventType, aDelay, aContext));

    if (PVMFMediaClock::RUNNING != iNonDecreasingClock->GetState())
    {
        PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::RequestEventCallBack - Skipping registering callback - iInterfaceState[%d]", iInterfaceState));
        return false;
    }

    uint32* callBackId = 0;
    bool*  callBackPending = NULL;
    uint32 intervalToRequestCallBack = 0;
    PVMFMediaClockNotificationsInterface *eventNotificationsInf = NULL;

    PVMFMediaClockNotificationsIntfContext	*eventClockNotificationIntfContext = NULL;
    switch (aEventType)
    {
        case JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED:
        {
            callBackId = &iIncomingMediaInactivityDurationCallBkId;
            callBackPending = &iIncomingMediaInactivityDurationCallBkPending;
            intervalToRequestCallBack = iMaxInactivityDurationForMediaInMs;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
            iNonDecClkNotificationsInfContext.SetContext(eventNotificationsInf, aContext);
            eventClockNotificationIntfContext = &iNonDecClkNotificationsInfContext;
        }
        break;
        case JB_NOTIFY_REPORT_BUFFERING_STATUS:
        {
            callBackId = &iNotifyBufferingStatusCallBkId;
            callBackPending = &iNotifyBufferingStatusCallBkPending;
            intervalToRequestCallBack = iBufferingStatusIntervalInMs;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
            iNonDecClkNotificationsInfContext.SetContext(eventNotificationsInf, aContext);
            eventClockNotificationIntfContext = &iNonDecClkNotificationsInfContext;
        }
        break;
        case JB_BUFFERING_DURATION_COMPLETE:
        {
            callBackId = &iJitterBufferDurationCallBkId;
            callBackPending = &iJitterBufferDurationCallBkPending;
            intervalToRequestCallBack = iJitterBufferDurationInMilliSeconds;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
            iNonDecClkNotificationsInfContext.SetContext(eventNotificationsInf, aContext);
            eventClockNotificationIntfContext = &iNonDecClkNotificationsInfContext;
        }
        break;
        case JB_MONITOR_REBUFFERING:
        {
            //Playback clock is started and stopped outside the scope of this module.
            //Therefore, check state of the clock state before requesting the callback
            if (PVMFMediaClock::RUNNING != iClientPlayBackClock->GetState())
            {
                return false;
            }
            callBackId = &iMonitorReBufferingCallBkId;
            callBackPending = &iMonitorReBufferingCallBkPending;
            eventNotificationsInf = iClientPlayBackClockNotificationsInf;
            iClientPlayBkClkNotificationsInfContext.SetContext(eventNotificationsInf, aContext);
            eventClockNotificationIntfContext = &iClientPlayBkClkNotificationsInfContext;
        }
        break;
        case JB_NOTIFY_SEND_FIREWALL_PACKET:
        {
            callBackId = &iSendFirewallPacketCallBkId;
            callBackPending = &iSendFirewallPacketCallBkPending;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
            iNonDecClkNotificationsInfContext.SetContext(eventNotificationsInf, aContext);
            eventClockNotificationIntfContext = &iNonDecClkNotificationsInfContext;
        }
        break;
        case JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE:
        {
            PVMFJitterBufferPortParams * portParams = OSCL_REINTERPRET_CAST(PVMFJitterBufferPortParams *, aContext);
            callBackId = &(portParams->iWaitForOOOPacketCallBkId);
            callBackPending = &(portParams->iWaitForOOOPacketCallBkPending);
            eventNotificationsInf = iEstimatedClockNotificationsInf;
            iEstimatedClockNotificationsInfContext.SetContext(eventNotificationsInf, aContext);
            eventClockNotificationIntfContext = &iEstimatedClockNotificationsInfContext;
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::RequestEventCallBack Set Callback for sending OOO data aDelay [%d] aContext[0x%x]", aDelay, aContext));
        }
        break;
        default:
            OSCL_ASSERT(false);
    }

    if (*callBackPending)
    {
        CancelCallBack(eventNotificationsInf, *callBackId, *callBackPending);
        *callBackPending = false;
    }

    if (aDelay)
    {
        intervalToRequestCallBack = aDelay;
    }

    return RequestCallBack(eventNotificationsInf, intervalToRequestCallBack, *callBackId, *callBackPending, eventClockNotificationIntfContext);
}

bool PVMFJitterBufferNode::RequestCallBack(PVMFMediaClockNotificationsInterface *& aEventNotificationInterface, uint32 aDelay, uint32& aCallBkId, bool& aCallBackStatusPending, OsclAny* aContext)
{
    const int32 toleranceWndForCallback = 0;
    bool retval = false;
    CancelCallBack(aEventNotificationInterface, aCallBkId, aCallBackStatusPending);
    if (aDelay > 0 && aEventNotificationInterface)
    {

        PVMFStatus status = aEventNotificationInterface->SetCallbackDeltaTime(aDelay, //delta time in clock when callBack should be called
                            toleranceWndForCallback,
                            this, //observer object to be called on timeout
                            false, //no threadLock
                            aContext, //no context
                            aCallBkId); //ID used to identify the timer for cancellation
        if (PVMFSuccess != status)
        {
            aCallBackStatusPending = false;
            OSCL_ASSERT(false);
        }
        else
        {
            aCallBackStatusPending = true;
            retval = true;
        }
    }
    if (aContext)
    {
        PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::RequestCallBack callbackId[%d] aContext[0x%x]", aCallBkId, aContext));
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::RequestCallBack Out aDelay[%d], aCallBkId[%d] aCallBackStatusPending[%d]", aDelay, aCallBkId, aCallBackStatusPending));
    return retval;
}

void PVMFJitterBufferNode::CancelEventCallBack(JB_NOTIFY_CALLBACK aEventType, OsclAny* aContext)
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "[0x%x]PVMFJitterBufferNode::CancelEventCallBack In - Event Type[%d] ", this, aEventType));
    uint32 callBackId = 0;
    bool*  callBackPending = NULL;
    PVMFMediaClockNotificationsInterface *eventNotificationsInf = NULL;
    switch (aEventType)
    {
        case JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED:
        {
            callBackId = iIncomingMediaInactivityDurationCallBkId;
            callBackPending = &iIncomingMediaInactivityDurationCallBkPending;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
        }
        break;
        case JB_NOTIFY_REPORT_BUFFERING_STATUS:
        {
            callBackId = iNotifyBufferingStatusCallBkId;
            callBackPending = &iNotifyBufferingStatusCallBkPending;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
        }
        break;
        case JB_BUFFERING_DURATION_COMPLETE:
        {
            callBackId = iJitterBufferDurationCallBkId;
            callBackPending = &iJitterBufferDurationCallBkPending;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
        }
        break;
        case JB_MONITOR_REBUFFERING:
        {
            callBackId = iMonitorReBufferingCallBkId;
            callBackPending = &iMonitorReBufferingCallBkPending;
            eventNotificationsInf = iClientPlayBackClockNotificationsInf;
        }
        break;
        case JB_NOTIFY_SEND_FIREWALL_PACKET:
        {
            callBackId = iSendFirewallPacketCallBkId;
            callBackPending = &iSendFirewallPacketCallBkPending;
            eventNotificationsInf = iNonDecreasingClockNotificationsInf;
        }
        break;
        case JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE:
        {
            if (aContext)
            {
                PVMFJitterBufferPortParams * portParams = OSCL_REINTERPRET_CAST(PVMFJitterBufferPortParams *, aContext);
                callBackId = portParams->iWaitForOOOPacketCallBkId;
                callBackPending = &(portParams->iWaitForOOOPacketCallBkPending);
                PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::CancelEventCallBack cancelling Callback for sending OOO data callBackId[%d] , aContext[0x%x]", callBackId, aContext));
            }
            eventNotificationsInf = iEstimatedClockNotificationsInf;
        }
        break;
        default:
            OSCL_ASSERT(false);
    }
    CancelCallBack(eventNotificationsInf, callBackId, *callBackPending);
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::CancelEventCallBack Out - Event Type[%d] CallBackId [%d]", aEventType, callBackId));
}

void PVMFJitterBufferNode::CancelCallBack(PVMFMediaClockNotificationsInterface *& aEventNotificationInterface, uint32& aCallBkId, bool& aCallBackStatusPending)
{
    if (aCallBackStatusPending && aEventNotificationInterface)
    {
        aEventNotificationInterface->CancelCallback(aCallBkId, false);
        aCallBackStatusPending = false;
    }
}

bool PVMFJitterBufferNode::IsCallbackPending(JB_NOTIFY_CALLBACK aEventType, OsclAny* aContext)
{
    bool*  callBackPending = NULL;
    PVMFJitterBufferPortParams* portParams = OSCL_REINTERPRET_CAST(PVMFJitterBufferPortParams*, aContext);
    switch (aEventType)
    {
        case JB_INCOMING_MEDIA_INACTIVITY_DURATION_EXPIRED:
        {
            callBackPending = &iIncomingMediaInactivityDurationCallBkPending;
        }
        break;
        case JB_NOTIFY_REPORT_BUFFERING_STATUS:
        {
            callBackPending = &iNotifyBufferingStatusCallBkPending;
        }
        break;
        case JB_BUFFERING_DURATION_COMPLETE:
        {
            callBackPending = &iJitterBufferDurationCallBkPending;
        }
        break;
        case JB_MONITOR_REBUFFERING:
        {
            callBackPending = &iMonitorReBufferingCallBkPending;
        }
        break;
        case JB_NOTIFY_SEND_FIREWALL_PACKET:
        {
            callBackPending = &iSendFirewallPacketCallBkPending;
        }
        break;
        case JB_NOTIFY_WAIT_FOR_OOO_PACKET_COMPLETE:
        {
            callBackPending = &(portParams->iWaitForOOOPacketCallBkPending);
        }
        break;
        default:
            OSCL_ASSERT(false);
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::IsCallbackPending - Event Type[%d] CallBackPending [%d] aContext[0x%x]", aEventType, *callBackPending, aContext));
    return *callBackPending;
}

void PVMFJitterBufferNode::ProcessCallBack(uint32 aCallBackID,
        PVTimeComparisonUtils::MediaTimeStatus aTimerAccuracy,
        uint32 aDelta,
        const OsclAny* aContextData,
        PVMFStatus aStatus)
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::ProcessCallBack In CallBackId [%d] aDelta[%d]", aCallBackID, aDelta));
    OSCL_UNUSED_ARG(aDelta);
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(aTimerAccuracy);

    PVMFMediaClockNotificationsIntfContext *clockNotificationIntfContext = OSCL_REINTERPRET_CAST(PVMFMediaClockNotificationsIntfContext*, aContextData);
    if (aCallBackID == iIncomingMediaInactivityDurationCallBkId && (clockNotificationIntfContext->GetMediaClockNotificationsInterface() == iNonDecreasingClockNotificationsInf))
    {
        iIncomingMediaInactivityDurationCallBkPending = false;
        HandleEvent_IncomingMediaInactivityDurationExpired();
    }
    if (aCallBackID == iNotifyBufferingStatusCallBkId && (clockNotificationIntfContext->GetMediaClockNotificationsInterface() == iNonDecreasingClockNotificationsInf))
    {
        iNotifyBufferingStatusCallBkPending = false;
        HandleEvent_NotifyReportBufferingStatus();
    }
    if (aCallBackID == iJitterBufferDurationCallBkId && (clockNotificationIntfContext->GetMediaClockNotificationsInterface() == iNonDecreasingClockNotificationsInf))
    {
        iJitterBufferDurationCallBkPending = false;
        HandleEvent_JitterBufferBufferingDurationComplete();
    }
    if (aCallBackID == iMonitorReBufferingCallBkId && (clockNotificationIntfContext->GetMediaClockNotificationsInterface() == iClientPlayBackClockNotificationsInf))
    {
        iMonitorReBufferingCallBkPending = false;
        HandleEvent_MonitorReBuffering();
    }
    if (aCallBackID == iSendFirewallPacketCallBkId && (clockNotificationIntfContext->GetMediaClockNotificationsInterface() == iNonDecreasingClockNotificationsInf))
    {
        iSendFirewallPacketCallBkPending = false;
        HandleEvent_NotifySendFirewallPacket();
    }
    if (clockNotificationIntfContext->GetContextData())
    {
        PVMFJitterBufferPortParams * portParams = NULL;
        portParams  = OSCL_REINTERPRET_CAST(PVMFJitterBufferPortParams*, clockNotificationIntfContext->GetContextData());
        if (aCallBackID == portParams->iWaitForOOOPacketCallBkId && (clockNotificationIntfContext->GetMediaClockNotificationsInterface() == iEstimatedClockNotificationsInf))
        {
            portParams->iWaitForOOOPacketCallBkPending = false;
            HandleEvent_NotifyWaitForOOOPacketComplete(portParams);
        }
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::ProcessCallBack Out"));
}

void PVMFJitterBufferNode::HandleEvent_IncomingMediaInactivityDurationExpired()
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_IncomingMediaInactivityDurationExpired In"));
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    int32 errcode = PVMFJitterBufferNodeRemoteInactivityTimerExpired;

    if (iCurrentCommand.size() > 0)
    {
        PVMFJitterBufferNodeCommand cmd = iCurrentCommand.front();
        CommandComplete(cmd, PVMFFailure, NULL, &eventuuid, &errcode);
        iCurrentCommand.Erase(&iCurrentCommand.front());
    }
    else
    {
        ReportInfoEvent(PVMFErrTimeout, NULL, &eventuuid, &errcode);
        oSessionDurationExpired = true;

        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if ((it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT))
            {
                it->oUpStreamEOSRecvd = true;
            }
        }

        iSessionDurationTimer->Stop();
        if (IsAdded())
            RunIfNotReady();
    }

    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_IncomingMediaInactivityDurationExpired Out"));
}

void PVMFJitterBufferNode::HandleEvent_NotifyReportBufferingStatus()
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_NotifyReportBufferingStatus In"));
    if (oDelayEstablished == false)
    {
        /*
         * Check to see if the session duration has expired
         */
        if (oSessionDurationExpired)
        {
            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::TimeoutOccurred - Session Duration Expired"));
            /* Force out of rebuffering */
            Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
            for (it = iPortParamsQueue.begin();
                    it != iPortParamsQueue.end();
                    it++)
            {
                if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
                {
                    SendData(it->iPort);
                }
            }
            if (IsAdded())
            {
                RunIfNotReady();
            }
        }
        else
        {
            ReportInfoEvent(PVMFInfoBufferingStatus);
            RequestEventCallBack(JB_NOTIFY_REPORT_BUFFERING_STATUS);
        }
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_NotifyReportBufferingStatus Out"));
}

void PVMFJitterBufferNode::HandleEvent_JitterBufferBufferingDurationComplete()
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_JitterBufferBufferingDurationComplete In"));
    if (oDelayEstablished == false)
    {
        RequestEventCallBack(JB_BUFFERING_DURATION_COMPLETE);

        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::HandleEvent_JitterBufferBufferingDurationComplete - Trying To Force Out of Buffering"));
        /* Force out of buffering */
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::HandleEvent_JitterBufferBufferingDurationComplete - Jitter Buffer Duration Expired"));
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                SendData(it->iPort);
            }
        }
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_JitterBufferBufferingDurationComplete Out"));
}

void PVMFJitterBufferNode::HandleEvent_MonitorReBuffering()
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_MonitorReBuffering In"));
    if (oDelayEstablished == true)
    {
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::PVMFJBJitterBufferDurationTimerEvent - Trying To Force ReBuffering"));
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                SendData(it->iPort);
            }
        }
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_MonitorReBuffering Out"));
}

void PVMFJitterBufferNode::HandleEvent_NotifySendFirewallPacket()
{
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_NotifySendFirewallPacket In"));
    bool oComplete = false;
    CheckForFireWallPacketAttempts(oComplete);
    if (oComplete == false)
    {
        SendFireWallPackets();
    }
    else
    {
        PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::PvmfFirewallPacketTimerEvent - FW Pkt Exchange Complete"));
        if (iInterfaceState == EPVMFNodeInitialized)
        {
            /* We are past max num attempts */
            OSCL_ASSERT(!iCurrentCommand.empty());
            OSCL_ASSERT(iCurrentCommand.front().iCmd == PVMF_JITTER_BUFFER_NODE_PREPARE);
            CompletePrepare();
        }
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_NotifySendFirewallPacket Out"));
}

void PVMFJitterBufferNode::HandleEvent_NotifyWaitForOOOPacketComplete(OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_NotifyWaitForOOOPacketComplete In iInterfaceState[%d]", iInterfaceState));
    //Wake up the AO to send data to the connected node
    PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::HandleEvent_NotifyWaitForOOOPacketComplete Sending OOO data %x", aContext));
    if (IsAdded())
    {
        RunIfNotReady();
    }
    PVMF_JBNODE_LOG_EVENTS_CLOCK((0, "PVMFJitterBufferNode::HandleEvent_NotifyWaitForOOOPacketComplete Out"));
}

void PVMFJitterBufferNode::SetJitterBufferSize(uint32 aBufferSz)
{
    iJitterBufferSz = aBufferSz;
}

void PVMFJitterBufferNode::GetJitterBufferSize(uint32& aBufferSz) const
{
    aBufferSz = iJitterBufferSz;
}

void PVMFJitterBufferNode::SetJitterBufferChunkAllocator(OsclMemPoolResizableAllocator* aDataBufferAllocator, const PVMFPortInterface* aPort)
{
    for (uint32 i = 0; i < iPortParamsQueue.size(); i++)
    {
        if (iPortParamsQueue[i].iPort == aPort)
        {
            iPortParamsQueue[i].iBufferAlloc = aDataBufferAllocator;
            aDataBufferAllocator->addRef();
        }
    }
}

void PVMFJitterBufferNode::SetJitterBufferMemPoolInfo(const PvmfPortBaseImpl* aPort, uint32 aSize, uint32 aResizeSize, uint32 aMaxNumResizes, uint32 aExpectedNumberOfBlocksPerBuffer)
{
    PVMFJitterBufferPort* port = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    port->iPortParams->SetJitterBufferMemPoolInfo(aSize, aResizeSize, aMaxNumResizes, aExpectedNumberOfBlocksPerBuffer);

}

void PVMFJitterBufferNode::GetJitterBufferMemPoolInfo(const PvmfPortBaseImpl* aPort, uint32& aSize, uint32& aResizeSize, uint32& aMaxNumResizes, uint32& aExpectedNumberOfBlocksPerBuffer) const
{
    PVMFJitterBufferPort* port = OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
    port->iPortParams->GetJitterBufferMemPoolInfo(aSize, aResizeSize, aMaxNumResizes, aExpectedNumberOfBlocksPerBuffer);
}

/* computes the max next ts of all tracks */
PVMFTimestamp PVMFJitterBufferNode::getMaxMediaDataTS()
{
    PVMFTimestamp mediaTS = 0;
    uint32 in_wrap_count = 0;
    uint32 i;

    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portParams.iJitterBuffer != NULL)
            {
                PVMFTimestamp ts =
                    portParams.iJitterBuffer->peekMaxElementTimeStamp();
                /*
                 * Convert Time stamp to milliseconds
                 */
                portParams.mediaClockConverter.set_clock(ts, in_wrap_count);
                PVMFTimestamp converted_ts =
                    portParams.mediaClockConverter.get_converted_ts(1000);
                if (converted_ts > mediaTS)
                {
                    mediaTS = converted_ts;
                }
            }
        }
    }
    for (i = 0; i < iPortParamsQueue.size(); i++)
    {
        PVMFJitterBufferPortParams portParams = iPortParamsQueue[i];

        if (portParams.tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (portParams.eTransportType != PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_ASF)
            {
                if (portParams.iJitterBuffer != NULL)
                {
                    portParams.iJitterBuffer->SetAdjustedTSInMS(mediaTS);
                }
            }
        }
    }
    return mediaTS;
}

bool PVMFJitterBufferNode::PrepareForPlaylistSwitch()
{
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    uint32 clientClock32 = 0;
    uint32 serverClock32 = 0;
    bool overflowFlag = false;
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime32(clientClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);
    iEstimatedServerClock->GetCurrentTime32(serverClock32, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForPlaylistSwitch - Before - EstServClock=%d",
                                 serverClock32));
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForPlaylistSwitch - Before - ClientClock=%d",
                                 clientClock32));
#endif
    oAutoPause = false;
    iJitterBufferState = PVMF_JITTER_BUFFER_IN_TRANSITION;
    iClientPlayBackClock->Pause();

    return true;
}

