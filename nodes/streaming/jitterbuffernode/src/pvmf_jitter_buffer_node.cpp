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
#ifndef OSCL_EXCLUSIVE_PTR_H_INCLUDED
#include "oscl_exclusive_ptr.h"
#endif
#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
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
    iBroadCastSession = false;
    iStreamID = 0;
    iLogger = NULL;
    iDataPathLogger = NULL;
    iDataPathLoggerIn = NULL;
    iDataPathLoggerOut = NULL;
    iClockLogger = NULL;
    iClockLoggerSessionDuration = NULL;
    iClockLoggerRebuff = NULL;
    iDiagnosticsLogger = NULL;
    iExtensionInterface = NULL;
    iClientPlayBackClock = NULL;
    iEstimatedServerClock = NULL;
    iRTCPClock = NULL;
    iJitterBufferDurationInMilliSeconds = DEFAULT_JITTER_BUFFER_DURATION_IN_MS;
    iJitterBufferUnderFlowThresholdInMilliSeconds = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
    iPlayBackThresholdInMilliSeconds = DEFAULT_PLAY_BACK_THRESHOLD_IN_MS;
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
    iJitterBufferDurationTimer = NULL;
    iBufferingStatusTimer = NULL;
    iFireWallPacketTimer = NULL;
    iDisableFireWallPackets = false;

    iNumUnderFlow = 0;
    iUseSessionDurationTimerForEOS = true;
    iUnderFlowStartByte = 0;
    iStartBufferingTickCount = 0;
    iPrevBufferingStartPacketTs = 0;
    iThinningIntervalMS = 0;
    iBufferAlloc = NULL;

    iNumRunL = 0;

    hasAudioRTCP = false;
    hasVideoRTCP = false;
    gotRTCPReports = false;

    int32 err;

    iMaxNumBufferResizes = DEFAULT_MAX_NUM_SOCKETMEMPOOL_RESIZES;
    iBufferResizeSize = DEFAULT_MAX_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;

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

             OsclExclusivePtr<OsclClock> estServClockAutoPtr;
             OsclExclusivePtr<OsclClock> rtcpClockAutoPtr;
             OsclExclusivePtr<PvmfRtcpTimer> rtcpTimerAutoPtr;
             OsclExclusivePtr<PvmfJBInactivityTimer> inactivityTimerAutoPtr;
             OsclExclusivePtr<PvmfJBSessionDurationTimer> sessionDurationTimerAutoPtr;
             OsclExclusivePtr<PvmfJBJitterBufferDurationTimer> jitterBufferDurationTimerAutoPtr;
             typedef OsclTimer<PVMFJitterBufferNodeAllocator> osclTimerType;
             OsclExclusivePtr<osclTimerType> bufferingStatusTimerAutoPtr;
             OsclExclusivePtr<PvmfFirewallPacketTimer> firewallPacketTimerAutoPtr;

             PVMF_JITTER_BUFFER_NEW(NULL, OsclClock, (), iEstimatedServerClock);
             estServClockAutoPtr.set(iEstimatedServerClock);
             iEstimatedServerClock->SetClockTimebase(iEstimatedServerClockTimeBase);


             PVMF_JITTER_BUFFER_NEW(NULL, OsclClock, (), iRTCPClock);
             rtcpClockAutoPtr.set(iRTCPClock);
             iRTCPClock->SetClockTimebase(iRTCPClockTimeBase);

             PVMF_JITTER_BUFFER_NEW(NULL, PvmfJBInactivityTimer, (this), iRemoteInactivityTimer);
             inactivityTimerAutoPtr.set(iRemoteInactivityTimer);

             PVMF_JITTER_BUFFER_NEW(NULL, PvmfJBSessionDurationTimer, (this), iSessionDurationTimer);
             sessionDurationTimerAutoPtr.set(iSessionDurationTimer);
             iSessionDurationTimer->SetEstimatedServerClock(iEstimatedServerClock);

             PVMF_JITTER_BUFFER_NEW(NULL, PvmfJBJitterBufferDurationTimer, (this), iJitterBufferDurationTimer);
             jitterBufferDurationTimerAutoPtr.set(iJitterBufferDurationTimer);
             iJitterBufferDurationTimer->setJitterBufferDurationInMS(iJitterBufferDurationInMilliSeconds);

             /*
              * Set the node capability data.
              * This node can support an unlimited number of ports.
              */
             iCapability.iCanSupportMultipleInputPorts = true;
             iCapability.iCanSupportMultipleOutputPorts = true;
             iCapability.iHasMaxNumberOfPorts = false;
             iCapability.iMaxNumberOfPorts = 0;//no maximum
             iCapability.iInputFormatCapability.push_back(PVMF_RTP);
             iCapability.iOutputFormatCapability.push_back(PVMF_RTP);

             PVMF_JITTER_BUFFER_NEW(NULL,
                                    OsclTimer<PVMFJitterBufferNodeAllocator>,
                                    ("PVMFJitterBufferBufferingStatusTimer"),
                                    iBufferingStatusTimer);

             iBufferingStatusTimer->SetObserver(this);
             /* Milli Sec resolution */
             iBufferingStatusTimer->SetFrequency(PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_FREQUENCY);
             bufferingStatusTimerAutoPtr.set(iBufferingStatusTimer);

             PVMF_JITTER_BUFFER_NEW(NULL, PvmfFirewallPacketTimer, (this), iFireWallPacketTimer);
             firewallPacketTimerAutoPtr.set(iFireWallPacketTimer);

             // seed the random number generator
             iRandGen.Seed(RTCP_RAND_SEED);

             estServClockAutoPtr.release();
             rtcpClockAutoPtr.release();
             rtcpTimerAutoPtr.release();
             inactivityTimerAutoPtr.release();
             sessionDurationTimerAutoPtr.release();
             jitterBufferDurationTimerAutoPtr.release();
             bufferingStatusTimerAutoPtr.release();
             firewallPacketTimerAutoPtr.release();
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

    /* Cleanup allocated interfaces */
    if (iExtensionInterface)
    {
        /*
         * clear the interface container
         * the interface can't function without the node
         */
        iExtensionInterface->iContainer = NULL;
        iExtensionInterface->removeRef();
    }

    iUseSessionDurationTimerForEOS = true;

    /* Cleanup allocated ports */
    while (!iPortVector.empty())
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

    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfJBInactivityTimer, iRemoteInactivityTimer);
    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfJBSessionDurationTimer, iSessionDurationTimer);
    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfJBJitterBufferDurationTimer, iJitterBufferDurationTimer);
    PVMF_JITTER_BUFFER_DELETE(NULL, OsclClock, iEstimatedServerClock);
    PVMF_JITTER_BUFFER_DELETE(NULL, OsclClock, iRTCPClock);
    PVMF_JITTER_BUFFER_DELETE(NULL, PvmfFirewallPacketTimer, iFireWallPacketTimer);

    if (iBufferingStatusTimer)
    {
        iBufferingStatusTimer->Clear();
    }
    PVMF_JITTER_BUFFER_TEMPLATED_DELETE(NULL, OsclTimer<PVMFJitterBufferNodeAllocator>, OsclTimer, iBufferingStatusTimer);

    if (iBufferAlloc != NULL)
    {
        iBufferAlloc->CancelDeallocationNotifications();
        iBufferAlloc->CancelFreeChunkAvailableCallback();
        iBufferAlloc->DecrementKeepAliveCount();
        if (iBufferAlloc->getNumOutStandingBuffers() == 0)
        {
            OSCL_DELETE((iBufferAlloc));
        }
        iBufferAlloc = NULL;
    }

    if (NULL != iClientPlayBackClock)
    {
        iClientPlayBackClock->RemoveClockStateObserver(*this);
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
            if (iRemoteInactivityTimer)
            {
                iRemoteInactivityTimer->Cancel();
            }
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
            }

            if (iBufferingStatusTimer)
            {
                iBufferingStatusTimer->Clear();
            }
            if (iFireWallPacketTimer)
            {
                iFireWallPacketTimer->Cancel();
            }
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
 * retrive a port iterator.
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
    PVMF_JBNODE_LOGINFO((0, "JitterBufferNode:DoReset"));

    LogSessionDiagnostics();

    /* This node allows a reset from any idle or error state */
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            /* Stop inactivity timer */
            iRemoteInactivityTimer->Stop();
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

            /* Stop Firewall Packet Timer */
            iFireWallPacketTimer->Stop();
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
            PVMFStatus aStatus = PVMFSuccess;
            if (iEstimatedServerClock)
            {
                if (!(iEstimatedServerClock->Stop()))
                {
                    aStatus = PVMFFailure;
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoReset: Error - iEstimatedServerClock->Stop failed"));
                }
            }
            if (iRTCPClock)
            {
                if (!(iRTCPClock->Stop()))
                {
                    aStatus = PVMFFailure;
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferNode::DoReset: Error - iRTCPClock->Stop failed"));
                }
            }
            /* delete all ports and notify observer */
            while (!iPortVector.empty())
            {
                iPortVector.Erase(&iPortVector.front());
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

            if (iBufferAlloc != NULL)
            {
                iBufferAlloc->CancelDeallocationNotifications();
                iBufferAlloc->CancelFreeChunkAvailableCallback();
                iBufferAlloc->DecrementKeepAliveCount();
                if (iBufferAlloc->getNumOutStandingBuffers() == 0)
                {
                    OSCL_DELETE((iBufferAlloc));
                }
                iBufferAlloc = NULL;
            }

            /* logoff & go back to Created state */
            SetState(EPVMFNodeIdle);
            PVMFStatus status = ThreadLogoff();

            CommandComplete(iInputCommands, aCmd, status);
        }
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
        /* add a reference each time we hand out the interface pointer.*/
        iExtensionInterface->addRef();
        *ptr = (PVInterface*)iExtensionInterface;
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
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

    //Only the specified mime types here will get their timebases
    //adjusted when the first sender reports come in.
    PVMFStreamType portType = PVMF_STREAM_TYPE_UNKNOWN;
    if (!oscl_CIstrcmp(mimetype->get_cstr(), "RTP/H264"))
        portType = PVMF_STREAM_TYPE_VIDEO;
    else if (!oscl_CIstrcmp(mimetype->get_cstr(), "RTP/mpeg4-generic"))
        portType = PVMF_STREAM_TYPE_AUDIO;
    else
        portType = PVMF_STREAM_TYPE_UNKNOWN;

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

            RTCPPortIndex[portType] = iPortParamsQueue.size();

            switch (portType)
            {
                case PVMF_STREAM_TYPE_AUDIO:
                    hasAudioRTCP = true;
                    break;

                case PVMF_STREAM_TYPE_VIDEO:
                    hasVideoRTCP = true;
                    break;

                case PVMF_STREAM_TYPE_UNKNOWN:
                default:
                    break;
            }
        }
    }
    else
    {
        portParams.tag = PVMF_JITTER_BUFFER_PORT_TYPE_INPUT;

        InputPortIndex[portType] = iPortParamsQueue.size();
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
            break;
        case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
            //don't log this port for now...
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
    PVMFJitterBufferPort* port;
    aCmd.PVMFJitterBufferNodeCommandBase::Parse((PVMFPortInterface*&)port);
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
                iEstimatedServerClock->SetStartTime32(start, OSCLCLOCK_MSEC);
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
        iFireWallPacketTimer->Cancel();

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
    iFireWallPacketTimer->Cancel();
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
            /* Diagnostic logging */
            iDiagnosticsLogged = false;
            /* If auto paused, implies jitter buffer is not empty */
            if (oAutoPause == false)
            {
                if (oSessionDurationExpired == false)
                {
                    /* Start remote inactivity timer */
                    iRemoteInactivityTimer->Start();
                    StartEstimatedServerClock();

                }
                /* Start RTCP Clock */
                iRTCPClock->Start();
                if ((oDelayEstablished == false) ||
                        (iJitterBufferState == PVMF_JITTER_BUFFER_IN_TRANSITION))
                {
                    /* Counter for intelligent streaming is reset at Start or setPlaybackRange*/
                    iNumUnderFlow = 0;
                    /* Start Buffering Status Timer */
                    iBufferingStatusTimer->Request(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID,
                                                   0,
                                                   PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_CYCLES,
                                                   this);
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
    /* Cancel Jitterbuffer duration timer */
    iJitterBufferDurationTimer->Cancel();
    /* Cancel Buffering status timer */
    iBufferingStatusTimer->Cancel(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID);
    /* Stop inactivity timer */
    iRemoteInactivityTimer->Stop();
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

    /* Stop Firewall Packet Timer */
    iFireWallPacketTimer->Stop();
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
            /* Stop inactivity timer */
            iRemoteInactivityTimer->Stop();
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
            /* Stop Firewall Packet Timer */
            iFireWallPacketTimer->Stop();
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
            /* Cancel the inactivity timer */
            iRemoteInactivityTimer->Stop();
            /*Cancel Buffering Status Timer*/
            iBufferingStatusTimer->Cancel(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID);
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
                uint64 srRecvTime;
                iRTCPClock->GetCurrentTime64(srRecvTime, OSCLCLOCK_MSEC);

                aPortParamsPtr->iRTCPStats.lastSenderReportRecvTime = srRecvTime;

                //Save the NTP and RTP timestamps for later calculations...
                aPortParamsPtr->iRTCPStats.lastSenderReportNTP =
                    (((uint64)rtcpSR.NTP_timestamp_high) << 32) + (uint64)rtcpSR.NTP_timestamp_low;
                aPortParamsPtr->iRTCPStats.lastSenderReportRTP = rtcpSR.RTP_timestamp;

                // Check if sender reports have been received for
                // both audio and video streams.  If so, set up
                // the Jitter Buffer timing parameters.

                // If we only have one track, skip the A/V offset calculations
                // If we are still waiting for reports from one or more RTCP ports...
                if (hasAudioRTCP && hasVideoRTCP && !gotRTCPReports)
                {
                    uint32 audioInputPortIndex = InputPortIndex[PVMF_STREAM_TYPE_AUDIO];
                    uint32 videoInputPortIndex = InputPortIndex[PVMF_STREAM_TYPE_VIDEO];
                    uint32 audioRTCPPortIndex  = RTCPPortIndex[PVMF_STREAM_TYPE_AUDIO];
                    uint32 videoRTCPPortIndex  = RTCPPortIndex[PVMF_STREAM_TYPE_VIDEO];
                    // Check to see if we've received an RTCP SR for both the audio and video ports
                    if ((iPortParamsQueue[videoRTCPPortIndex].iRTCPStats.lastSenderReportRecvTime != (uint64)0) &&
                            (iPortParamsQueue[audioRTCPPortIndex].iRTCPStats.lastSenderReportRecvTime != (uint64)0))
                    {
                        //If we have received the first RTP packets, go ahead and calculate the A/V sync.
                        //Otherwise, try again later (on the next RTCP report).
                        if ((iPortParamsQueue[videoInputPortIndex].iJitterBuffer->GetRTPTimeStampOffset() != 0) &&
                                (iPortParamsQueue[audioInputPortIndex].iJitterBuffer->GetRTPTimeStampOffset() != 0))
                        {
                            uint64 initNTP;
                            uint64 deltaNTP;
                            uint32 deltaNTP32;
                            uint32 deltaRTP;
                            uint64 deltaRTP64;
                            uint32 firstTS[NUM_PVMF_STREAM_TYPES];
                            uint32 SR_RTP[NUM_PVMF_STREAM_TYPES];
                            uint64 SR_NTP[NUM_PVMF_STREAM_TYPES];
                            uint32 RTPTB[NUM_PVMF_STREAM_TYPES];
                            uint32 timescale;

                            //Get the timestamps from the first packets on the RTP input ports.
                            firstTS[PVMF_STREAM_TYPE_AUDIO] = iPortParamsQueue[audioInputPortIndex].iJitterBuffer->GetRTPTimeStampOffset();
                            firstTS[PVMF_STREAM_TYPE_VIDEO] = iPortParamsQueue[videoInputPortIndex].iJitterBuffer->GetRTPTimeStampOffset();

                            // Get the timestamps from the RTCP SRs
                            SR_RTP[PVMF_STREAM_TYPE_AUDIO] = iPortParamsQueue[audioRTCPPortIndex].iRTCPStats.lastSenderReportRTP;
                            SR_NTP[PVMF_STREAM_TYPE_AUDIO] = iPortParamsQueue[audioRTCPPortIndex].iRTCPStats.lastSenderReportNTP;
                            SR_RTP[PVMF_STREAM_TYPE_VIDEO] = iPortParamsQueue[videoRTCPPortIndex].iRTCPStats.lastSenderReportRTP;
                            SR_NTP[PVMF_STREAM_TYPE_VIDEO] = iPortParamsQueue[videoRTCPPortIndex].iRTCPStats.lastSenderReportNTP;

                            // Use the audio RTCP SR to calculate an NTP time for the
                            // first audio packet received
                            timescale = iPortParamsQueue[audioInputPortIndex].timeScale;

                            if (SR_RTP[PVMF_STREAM_TYPE_AUDIO] > firstTS[PVMF_STREAM_TYPE_AUDIO])
                            {
                                deltaRTP = SR_RTP[PVMF_STREAM_TYPE_AUDIO] - firstTS[PVMF_STREAM_TYPE_AUDIO];
                                deltaRTP64 = ((uint64) deltaRTP / (uint64)timescale) << 32;
                                deltaRTP64 += ((uint64) deltaRTP % (uint64)timescale) * (uint64)0xFFFFFFFF / (uint64)timescale;
                                initNTP = SR_NTP[PVMF_STREAM_TYPE_AUDIO] - deltaRTP64;
                            }
                            else
                            {
                                deltaRTP = firstTS[PVMF_STREAM_TYPE_AUDIO] - SR_RTP[PVMF_STREAM_TYPE_AUDIO];
                                deltaRTP64 = ((uint64) deltaRTP / (uint64)timescale) << 32;
                                deltaRTP64 += ((uint64) deltaRTP % (uint64)timescale) * (uint64)0xFFFFFFFF / (uint64)timescale;
                                initNTP = SR_NTP[PVMF_STREAM_TYPE_AUDIO] + deltaRTP64;
                            }

                            // Now use the video RTCP SR to calculate the video RTP timestamp
                            // that corresponds to this NTP time
                            timescale = iPortParamsQueue[videoInputPortIndex].timeScale;

                            if (SR_NTP[PVMF_STREAM_TYPE_VIDEO] > initNTP)
                            {
                                deltaNTP = SR_NTP[PVMF_STREAM_TYPE_VIDEO] - initNTP;
                                // Convert to RTP timescale units
                                deltaNTP32 = ((deltaNTP * (uint64)timescale) + (uint64)0x80000000) >> 32;
                                RTPTB[PVMF_STREAM_TYPE_VIDEO] = SR_RTP[PVMF_STREAM_TYPE_VIDEO] - deltaNTP32;
                            }
                            else
                            {
                                deltaNTP = initNTP - SR_NTP[PVMF_STREAM_TYPE_VIDEO];
                                // Convert to RTP timescale units
                                deltaNTP32 = ((deltaNTP * (uint64)timescale) + (uint64)0x80000000) >> 32;
                                RTPTB[PVMF_STREAM_TYPE_VIDEO] = SR_RTP[PVMF_STREAM_TYPE_VIDEO] + deltaNTP32;
                            }

                            // Since audio is the reference track, we just set the timebase to the
                            // first received packet
                            RTPTB[PVMF_STREAM_TYPE_AUDIO] = firstTS[PVMF_STREAM_TYPE_AUDIO];

#ifdef DEBUG_RTCP
                            {
                                for (uint32 p = PVMF_STREAM_TYPE_AUDIO; p <= PVMF_STREAM_TYPE_VIDEO; p++)
                                {
                                    int32 delta = (int32)(RTPTB[p] - firstTS[p]);
                                    delta *= 1000;
                                    delta /= (int32)iPortParamsQueue[InputPortIndex[p]].timeScale;

                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                                    (0,
                                                     "Stream %d: timeScale=%uHz firstTS=%u RTCP.RTP=%u RTCP.NTP=%u.%06u newTB=%u delta=%dms\n",
                                                     p,
                                                     iPortParamsQueue[InputPortIndex[p]].timeScale,
                                                     firstTS[p],
                                                     SR_RTP[p],
                                                     (uint32)(SR_NTP[p] >> 32),
                                                     (uint32)(((((uint64)((uint32)(SR_NTP[p]) & 0xffffffff)) * (uint64)1000000) + (uint64)500000) >> 32),
                                                     RTPTB[p],
                                                     delta
                                                    )
                                                   );
                                }
                            }
#endif

                            //Purge old video data that we don't need to render.
                            iPortParamsQueue[videoInputPortIndex].iJitterBuffer->PurgeElementsWithTimestampLessThan(RTPTB[PVMF_STREAM_TYPE_VIDEO]);

#ifndef AVSYNC_DISABLE_RTCP_PROCESSING
                            //Only adjust the timebase if the sender reports have valid wall clock values.
                            if ((SR_NTP[PVMF_STREAM_TYPE_AUDIO] != (uint64)0)
                                    && (SR_NTP[PVMF_STREAM_TYPE_VIDEO] != (uint64)0))
                            {
                                //Only adjust the video input port.
                                iPortParamsQueue[videoInputPortIndex].iJitterBuffer->SetRTPTimeStampOffset(RTPTB[PVMF_STREAM_TYPE_VIDEO]);
                            }
#endif

                            gotRTCPReports = true;
                        }
                    }
                }
            }

            //If the RTCP type is BYE, set the end-of-stream flag.
            if (BYE_RTCP_PACKET == array_of_packet_types[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFJitterBufferNode::ProcessIncomingRTCPReport - BYE"));

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
    if (aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumRecievedUptoThisRR == 0)
    {
        aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumRecievedUptoThisRR =
            jbStats.seqNumBase;
    }
    if (jbStats.maxSeqNumReceived -
            aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumRecievedUptoThisRR)
    {
        reportBlock->fractionLost =
            (int8)(((jbStats.totalPacketsLost - aFeedBackPortParamsPtr->iRTCPStats.packetLossUptoThisRR) * 256) /
                   (jbStats.maxSeqNumReceived - aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumRecievedUptoThisRR));
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
        uint64 currRRGenTime;
        iRTCPClock->GetCurrentTime64(currRRGenTime, OSCLCLOCK_MSEC);

        uint64 lastSenderReportRecvTime =
            aFeedBackPortParamsPtr->iRTCPStats.lastSenderReportRecvTime;

        uint64 delaySinceLastSR64 =
            (currRRGenTime - lastSenderReportRecvTime);

        uint32 delaySinceLastSR32 =
            Oscl_Int64_Utils::get_uint64_lower32(delaySinceLastSR64);

        reportBlock->delaySinceLastSR = (delaySinceLastSR32 << 16) / 1000;

        aFeedBackPortParamsPtr->iRTCPStats.lastRRGenTime = currRRGenTime;
    }

    /* Update variables for the next RR cycle */
    aFeedBackPortParamsPtr->iRTCPStats.maxSeqNumRecievedUptoThisRR =
        jbStats.maxSeqNumReceived;
    aFeedBackPortParamsPtr->iRTCPStats.packetLossUptoThisRR =
        jbStats.totalPacketsLost;

    PVMFJitterBufferPort* jbRTCPPort =
        (PVMFJitterBufferPort*)aFeedBackPortParamsPtr->iPort;
    PVMFSharedMediaDataPtr rtcpOut;
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImpl;
    PVMFRTCPMemPool* rtcpBufAlloc = aFeedBackPortParamsPtr->iRTCPTimer->getRTCPBuffAlloc();
    int32 err;

    OSCL_TRY(err,
             mediaDataImpl = rtcpBufAlloc->getMediaDataImpl(MAX_RTCP_BLOCK_SIZE);
             rtcpOut = PVMFMediaData::createMediaData(mediaDataImpl,
                       &(rtcpBufAlloc->iMediaDataMemPool));
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
     * If Rate Adaptation is enabled send NADU APP packet,
     * if frequency criteria is met
     */
    if (aInputPortParamsPtr->oRateAdaptation)
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
            PVMFTimestamp tsOfNextPacketToBeDecoded = jbStats.maxTimeStampRetrievedWithoutRTPOffset;
            if (jbStats.currentOccupancy > 0)
            {
                tsOfNextPacketToBeDecoded =
                    aInputPortParamsPtr->iJitterBuffer->peekNextElementTimeStamp();
            }

            uint32 in_wrap_count = 0;
            /*
             * Convert Time stamp to milliseconds
             */
            aInputPortParamsPtr->mediaClockConverter.set_clock(tsOfNextPacketToBeDecoded, in_wrap_count);
            converted_ts =
                aInputPortParamsPtr->mediaClockConverter.get_converted_ts(1000);


            uint64 timebase64 = 0;
            uint64 clientClock64 = 0;
            if (iClientPlayBackClock != NULL)
                iClientPlayBackClock->GetCurrentTime64(clientClock64,
                                                       OSCLCLOCK_MSEC,
                                                       timebase64);
            uint32 diff32 = converted_ts -
                            Oscl_Int64_Utils::get_uint64_lower32(clientClock64);

            //set playoutdelay to 0xffff whenever we go into rebuffering
            if (oDelayEstablished == false)
            {
                diff32 = RTCP_NADU_APP_DEFAULT_PLAYOUT_DELAY;
            }
            PVMF_JBNODE_LOG_RTCP((0, "RTCP_PKT: Mime=%s, RTP_TS=%d, C_CLOCK=%d, DIFF=%d, RE-BUF=%d", aInputPortParamsPtr->iMimeType.get_cstr(), converted_ts, Oscl_Int64_Utils::get_uint64_lower32(clientClock64), diff32, oDelayEstablished));
            App.pss0_app_data.playoutdelayinms = (uint16)diff32;
            App.pss0_app_data.nsn = (jbStats.lastRetrievedSeqNum + 1);
            if (0 == jbStats.lastRetrievedSeqNum)
            {
                App.pss0_app_data.nsn = jbStats.seqNumBase;
            }
            App.pss0_app_data.nun = RTCP_NADU_APP_DEFAULT_NUN;
            PVMFJitterBufferPort* jbPort =
                OSCL_STATIC_CAST(PVMFJitterBufferPort*, aInputPortParamsPtr->iPort);
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
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferNode:NodeErrorEvent Type %d Data %d"
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
        /* Cancel and reschedule in the inactivity timer */
        iRemoteInactivityTimer->Cancel();
        uint32 inactivityDurationInMS = iRemoteInactivityTimer->getMaxInactivityDurationInMS();
        iRemoteInactivityTimer->RunIfNotReady(inactivityDurationInMS*1000);
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
                jitterBuffer->NotifyFreeSpaceAvailable(OSCL_STATIC_CAST(PVMFJitterBufferObserver*, this),
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
                uint64 timebase64 = 0;
                uint64 estServerClock = 0;
                uint64 clientClock = 0;
                iEstimatedServerClock->GetCurrentTime64(estServerClock, OSCLCLOCK_MSEC, timebase64);
                if (iClientPlayBackClock != NULL)
                    iClientPlayBackClock->GetCurrentTime64(clientClock, OSCLCLOCK_MSEC, timebase64);
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
                                                                         "MaxSNReg=%d, MaxTSReg=%d, LastSNRet=%d, LastTSRet=%d, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
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
                                                                         "MaxSNReg=%d, MaxTSReg=%d, LastSNRet=%d, LastTSRet=%d, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
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

            if ((portContainerPtr->iPort->IncomingMsgQueueSize() > 0) ||
                    (portContainerPtr->iPort->OutgoingMsgQueueSize() > 0) ||
                    ((jbPort->iCounterpartPortParams->oJitterBufferEmpty == false) &&
                     (oDelayEstablished == true)))
            {
                if (portContainerPtr->oProcessOutgoingMessages && (oStopOutputPorts == false))
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
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferNode::CheckForPortRescheduling: Error - GetPortContainer failed", this));
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
        if (oSessionDurationExpired)
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
            /* Create Port Allocators */
            PVMFJitterBufferPort* jbPort =
                OSCL_STATIC_CAST(PVMFJitterBufferPort*, aPort);
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

            // the port allocators are required iff the node-level buffer
            // allocator was not already created
            if (this->iBufferAlloc == NULL)
            {
                if (aUserSpecifiedBuffParams)
                {
                    jbPort->createPortAllocators(portParams.iMimeType, sizeInBytes,
                                                 aMaxNumBuffResizes, aBuffResizeSize);
                }
                else
                {
                    jbPort->createPortAllocators(portParams.iMimeType, sizeInBytes);
                }
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
                    uint64 timebase64 = 0;
                    uint64 clientClock64 = 0;
                    if (iClientPlayBackClock != NULL)
                        iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);

#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                    uint64 serverClock64 = 0;
                    iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - Purging Upto SeqNum =%d", aSeqNum));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - Before Purge - EstServClock=%d",
                                                 Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - Before Purge - ClientClock=%d",
                                                 Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
#endif

                    it->iJitterBuffer->PurgeElementsWithSeqNumsLessThan(aSeqNum,
                            Oscl_Int64_Utils::get_uint64_lower32(clientClock64));
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                    timebase64 = 0;
                    serverClock64 = 0;
                    iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - After Purge - EstServClock=%d",
                                                 Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::ClearJitterBuffer - After Purge - ClientClock=%d",
                                                 Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
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
                    rtpInfoParams.nptTimeInMS = aNPTInMS;
                    rtpInfoParams.rtpTimeScale = portParams.timeScale;
                    portParams.iJitterBuffer->setRTPInfoParams(rtpInfoParams);
                    /* In case this is after a reposition purge the jitter buffer */
                    if (oPlayAfterASeek)
                    {
                        uint64 timebase64 = 0;
                        uint64 clientClock64 = 0;
                        if (iClientPlayBackClock != NULL)
                            iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                        uint64 serverClock64 = 0;
                        iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - Purging Upto SeqNum =%d", aSeqNumBase));
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - Before Purge - EstServClock=%d",
                                                     Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - Before Purge - ClientClock=%d",
                                                     Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
#endif
                        portParams.iJitterBuffer->PurgeElementsWithSeqNumsLessThan(aSeqNumBase,
                                Oscl_Int64_Utils::get_uint64_lower32(clientClock64));
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                        timebase64 = 0;
                        serverClock64 = 0;
                        iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - After Purge - EstServClock=%d",
                                                     Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
                        PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::setPortRTPParams - After Purge - ClientClock=%d",
                                                     Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
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
        iFireWallPacketTimer->Start();
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
                    /* Cancel Buffering status timer */
                    iBufferingStatusTimer->Cancel(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
                    uint64 timebase64 = 0;
                    uint64 clientClock64 = 0;
                    uint64 serverClock64 = 0;
                    if (iClientPlayBackClock != NULL)
                        iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
                    iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::RegisterDataPacket - Time Delay Established - EstServClock=%d",
                                                 Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
                    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::RegisterDataPacket - Time Delay Established - ClientClock=%d",
                                                 Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
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
PVMFJitterBufferNode::IsJitterBufferReady(PVMFJitterBufferPortParams* aPortParams)
{
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

    uint64 timebase64 = 0;
    uint64 estServerClock = 0;
    uint64 clientClock = 0;
    /*
     * Get current estimated server clock in milliseconds
     */
    iEstimatedServerClock->GetCurrentTime64(estServerClock, OSCLCLOCK_USEC, timebase64);

    /*
     * Get current client playback clock in milliseconds
     */
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime64(clientClock, OSCLCLOCK_USEC, timebase64);

    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - EstServClock=%2d", estServerClock));
    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - ClientClock=%2d", clientClock));

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
            /* Cancel Buffering status timer */
            iBufferingStatusTimer->Cancel(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID);
            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Cancelling Jitter Buffer Duration Timer"));
            iJitterBufferDurationTimer->Cancel();
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
                oDelayEstablished = false;
                iJitterDelayPercent = 0;
                /* Start timer */
                iBufferingStatusTimer->Request(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID,
                                               0,
                                               PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_CYCLES,
                                               this);

                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Starting Jitter Buffer Duration Timer"));
                iJitterBufferDurationTimer->Start();

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
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - EstServClock=%2d", estServerClock));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - ClientClock=%2d", clientClock));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Estimated Serv Clock Less Than ClientClock!!!!"));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - EstServClock=%d",
                                             Oscl_Int64_Utils::get_uint64_lower32(estServerClock) / 1000));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Check - ClientClock=%d",
                                             Oscl_Int64_Utils::get_uint64_lower32(clientClock) / 1000));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Estimated Serv Clock Less Than ClientClock!!!!"));
            }
            return oDelayEstablished;
        }


        uint64 diff64 = estServerClock - clientClock;
        uint32 diff32 = Oscl_Int64_Utils::get_uint64_lower32(diff64);
        uint32 diff32ms = diff32 / 1000;
        if (diff32ms >= iJitterBufferDurationInMilliSeconds)
        {
            if (oDelayEstablished == false && aJitterBuffer->CheckNumElements() == true)
            {
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Cancelling Jitter Buffer Duration Timer"));
                iJitterBufferDurationTimer->Cancel();
                /* Coming out of rebuffering */
                UpdateRebufferingStats(PVMFInfoDataReady);
                iJitterDelayPercent = 100;
                ReportInfoEvent(PVMFInfoBufferingStatus);
                ReportInfoEvent(PVMFInfoDataReady);
                ReportInfoEvent(PVMFInfoBufferingComplete);
                /* Cancel Buffering status timer */
                iBufferingStatusTimer->Cancel(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID);
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - EstServClock=%2d", estServerClock));
                PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - ClientClock=%2d",  clientClock));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - EstServClock=%d",
                                             Oscl_Int64_Utils::get_uint64_lower32(estServerClock) / 1000));
                PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Established - ClientClock=%d",
                                             Oscl_Int64_Utils::get_uint64_lower32(clientClock) / 1000));
                if (iUseSessionDurationTimerForEOS == true)
                {
                    ComputeCurrentSessionDurationMonitoringInterval();
                    iSessionDurationTimer->Start();
                }
                else
                {
                    iSessionDurationTimer->Stop();
                }
                oDelayEstablished = true;
            }
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
                        iBufferingStatusTimer->Request(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID,
                                                       0,
                                                       PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_CYCLES,
                                                       this);

                        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Starting Jitter Buffer Duration Timer"));
                        iJitterBufferDurationTimer->Start();

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

                            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - EstServClock=%2d", estServerClock));
                            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - ClientClock=%2d",  clientClock));
                            PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::IsJitterBufferReady: Jitter Delay not met"));
                            PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - EstServClock=%d",
                                                         Oscl_Int64_Utils::get_uint64_lower32(estServerClock) / 1000));
                            PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::IsJitterBufferReady - Time Delay Violated - ClientClock=%d",
                                                         Oscl_Int64_Utils::get_uint64_lower32(clientClock) / 1000));

                            if (iClientPlayBackClock != NULL)
                                iClientPlayBackClock->Pause();
                        }
                    }
                    /* we are past the end of the clip, no more rebuffering */
                }
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
                                /* Cancel and reschedule in the inactivity timer */
                                iRemoteInactivityTimer->Cancel();
                                uint32 inactivityDurationInMS = iRemoteInactivityTimer->getMaxInactivityDurationInMS();
                                iRemoteInactivityTimer->RunIfNotReady(inactivityDurationInMS*1000);
                            }
                        }
                        if (iBufferAlloc != NULL)
                        {
                            iBufferAlloc->CancelFreeChunkAvailableCallback();
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

    bool oJitterBufferReady = IsJitterBufferReady(portParamsPtr);
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
            PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::SendData: Output Queue Busy - Mime=%s", outPortParamsPtr->iMimeType.get_cstr()));
            return PVMFErrBusy;
        }

        PVMFSharedMediaDataPtr mediaOut;
        PVMFSharedMediaMsgPtr mediaOutMsg;

        /* Check if there are any pending media commands */
        if (jitterBuffer->CheckForPendingCommands(mediaOutMsg) == true)
        {
            PVMFStatus status = outputPort->QueueOutgoingMsg(mediaOutMsg);
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
            uint64 timebase64 = 0;
            uint64 clientClock64 = 0;
            if (iClientPlayBackClock != NULL)
                iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Media Command Sent, Mime=%s, CmdId=%d, TS=%d, SEQNUM= %d, ClientClock= %2d",
                                            portParamsPtr->iMimeType.get_cstr(), mediaOutMsg->getFormatID(), mediaOutMsg->getTimestamp(), mediaOutMsg->getSeqNum(), clientClock64));
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

            if (iBufferAlloc != NULL)
            {
                iBufferAlloc->CancelDeallocationNotifications();
            }

            PVMFTimestamp ts;
            uint32 converted_ts;
            uint32 in_wrap_count = 0;
            {
//This option disables playback until the A/V Sync is complete.
//This feature is incomplete as it lacks a time-out mechanism.
//An alternative may be to increase the jitter buffer size.
#ifdef AVSYNC_WAIT_FOR_RTCP_SENDER_REPORTS
                //Only need to wait for A/V sync if we have both audio and video RTCP ports.
                if (hasAudioRTCP && hasVideoRTCP && !gotRTCPReports)
                {
                    //Wait for RTCP Sender Reports to arrive before outputting
                    //packets.
                    return PVMFErrBusy;
                }
#endif

                mediaOut = jitterBuffer->retrievePacket();
            }
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
                uint64 timebase64 = 0;
                uint64 clientClock64 = 0;
                uint64 serverClock64 = 0;
                if (iClientPlayBackClock != NULL)
                    iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
                iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
                outPortParamsPtr->iLastMsgTimeStamp = converted_ts;

                //PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: EstServClock= %2d", serverClock64));
                PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Mime=%s, SSRC=%d, TS=%d, SEQNUM= %d, ClientClock= %2d",
                                                portParamsPtr->iMimeType.get_cstr(),
                                                mediaOutMsg->getStreamID(),
                                                mediaOutMsg->getTimestamp(),
                                                mediaOutMsg->getSeqNum(),
                                                clientClock64));

                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::SendData:"
                                                     "MaxSNReg=%d, MaxTSReg=%d, LastSNRet=%d, LastTSRet=%d, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
                                                     stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                                     stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                                     (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum),
                                                     Oscl_Int64_Utils::get_uint64_lower32(serverClock64),
                                                     Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
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
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::SendData: Jitter Buffer Empty - Mime=%s", portParamsPtr->iMimeType.get_cstr()));
            portParamsPtr->oJitterBufferEmpty = true;

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
                if (iBufferAlloc != NULL)
                {
                    iBufferAlloc->CancelFreeChunkAvailableCallback();
                }
                ReportInfoEvent(PVMFJitterBufferNodeJitterBufferLowWaterMarkReached);
                PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::SendData: Sending Auto Resume - Mime=%s", portParamsPtr->iMimeType.get_cstr()));
            }
            return PVMFSuccess;
        }
        //reevalute jitter
        oJitterBufferReady = IsJitterBufferReady(portParamsPtr);
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
            if (it->oJitterBufferEmpty == false)
            {
                oEmpty = false;
                return PVMFSuccess;
            }
        }
    }
    oEmpty = true;
    return PVMFSuccess;
}

PVMFStatus PVMFJitterBufferNode::CheckForEOS()
{
    /*
     * Checks to see if all jitter buffers are empty. If they are
     * then it would stop scheduling the jitter buffer node, so generate
     * EOS.
     */
    Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            if (it->oJitterBufferEmpty == false)
            {
                return PVMFErrBusy;
            }
        }
    }
    /*
     * Implies that all jitter buffers are empty
     */
    PVMFStatus status = PVMFSuccess;
    for (it = iPortParamsQueue.begin();
            it != iPortParamsQueue.end();
            it++)
    {
        if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
        {
            status = GenerateAndSendEOSCommand(it->iPort);
        }
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
        iSessionDurationTimer->Stop();
        if (iUseSessionDurationTimerForEOS == true)
        {
            oSessionDurationExpired = false;
            iSessionDurationTimer->setSessionDurationInMS(((iPlayStopTimeInMS - iPlayStartTimeInMS) + PVMF_EOS_TIMER_GAURD_BAND_IN_MS));
            ComputeCurrentSessionDurationMonitoringInterval();
            iSessionDurationTimer->Start();
        }
        // Only at Prepare state, this API is called by streaming manager node
        // We set thinning interval in here
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

/*
 * Called by PvmfJBInactivityTimer on inactivity timer expiry
 */
void PVMFJitterBufferNode::PVMFJBInactivityTimerEvent()
{
    PVUuid eventuuid = PVMFJitterBufferNodeEventTypeUUID;
    int32 errcode = PVMFJitterBufferNodeRemoteInactivityTimerExpired;
    /*
     * If start is pending, this means that inactivity timer expired
     * during the buffering state. Send start failure, else send the
     * info event
     */
    if (iCurrentCommand.size() > 0)
    {
        PVMFJitterBufferNodeCommand cmd = iCurrentCommand.front();
        CommandComplete(cmd, PVMFFailure, NULL, &eventuuid, &errcode);
        /* Erase the command from the current queue */
        iCurrentCommand.Erase(&iCurrentCommand.front());
    }
    else
    {
        ReportErrorEvent(PVMFErrTimeout, NULL, &eventuuid, &errcode);
    }
    return;
}

/*
 * Called by PvmfJBSessionDurationTimer on session duration timer expiry
 */
void PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent()
{
    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Session Duration Timer Expired"));
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Session Duration Timer Expired"));
    /* Check if the estimated server clock is past the expected value */
    uint64 expectedEstServClockVal =
        iSessionDurationTimer->GetExpectedEstimatedServClockValAtSessionEnd();
    uint64 timebase64 = 0;
    uint64 estServClock = 0;
    iEstimatedServerClock->GetCurrentTime64(estServClock, OSCLCLOCK_MSEC, timebase64);
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - CurrEstServClock = %2d", estServClock));
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - ExpectedEstServClock = %2d", expectedEstServClockVal));
    if (estServClock >= expectedEstServClockVal)
    {
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::PVMFJBSessionDurationTimerEvent - Session Duration Has Elapsed"));
        oSessionDurationExpired = true;
        /* Cancel clock update notifications */
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin(); it != iPortParamsQueue.end(); it++)
        {
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
    if (iJitterBufferState != PVMF_JITTER_BUFFER_IN_TRANSITION)
    {
        oSessionDurationExpired = true;
        iSessionDurationTimer->Cancel();
        PVMF_JBNODE_LOGDATATRAFFIC((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Out Of Band EOS Recvd"));
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::NotifyOutOfBandEOS - Out Of Band EOS Recvd"));
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

/*
 * Buffering status event timer
 */
void PVMFJitterBufferNode::TimeoutOccurred(int32 timerID,
        int32 timeoutInfo)
{
    OSCL_UNUSED_ARG(timeoutInfo);

    if (timerID == PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID)
    {
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
                iBufferingStatusTimer->Request(PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID,
                                               0,
                                               PVMF_JITTER_BUFFER_BUFFERING_STATUS_EVENT_CYCLES,
                                               this);
            }
        }
    }
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
        oStopOutputPorts = true;
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
            uint64 timebase64 = 0;
            uint64 clientClock64 = 0;
            if (iClientPlayBackClock != NULL)
                iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
            timebase64 = 0;
            uint64 estServClock64 = 0;
            iEstimatedServerClock->GetCurrentTime64(estServClock64, OSCLCLOCK_MSEC, timebase64);
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: MimeType=%s, StreamID=%d",
                                            portParams->iMimeType.get_cstr(),
                                            msg->getStreamID()));
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: ClientClock=%d",
                                            Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: EstServClock=%d",
                                            Oscl_Int64_Utils::get_uint64_lower32(estServClock64)));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: MimeType=%s, StreamID=%d",
                                                   portParams->iMimeType.get_cstr(),
                                                   msg->getStreamID()));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: ClientClock=%d",
                                                   Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PVMFJitterBufferNode::GenerateAndSendEOSCommand: EstServClock=%d",
                                                   Oscl_Int64_Utils::get_uint64_lower32(estServClock64)));
            portParams->oEOSReached = true;
            return (status);
        }
    }
    return PVMFSuccess;
}

void PVMFJitterBufferNode::PVMFJBJitterBufferDurationTimerEvent()
{
    PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::PVMFJBJitterBufferDurationTimerEvent - Recvd"));
    if (oDelayEstablished == false)
    {
        iJitterBufferDurationTimer->Start();

        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::PVMFJBJitterBufferDurationTimerEvent - Trying To Force Out of Buffering"));
        /* Force out of buffering */
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferNode::PVMFJBJitterBufferDurationTimerEvent - Jitter Buffer Duration Expired"));
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

void PVMFJitterBufferNode::PvmfFirewallPacketTimerEvent()
{
    PVMF_JBNODE_LOG_FW((0, "PVMFJitterBufferNode::PvmfFirewallPacketTimerEvent"));
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
                        int32 err;

                        if (iFireWallPacketInfo.iFormat == PVMF_JB_FW_PKT_FORMAT_PV)
                        {
                            OSCL_TRY(err,
                                     mediaDataImpl = it->iMediaDataImplAlloc->allocate(PVMF_JITTER_BUFFER_NODE_MAX_FIREWALL_PKT_SIZE);
                                     fireWallPkt = PVMFMediaData::createMediaData(mediaDataImpl,
                                                   it->iMediaMsgAlloc);
                                    );

                            if (err != OsclErrNone)
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
                            OSCL_TRY(err,
                                     mediaDataImpl = it->iMediaDataImplAlloc->allocate(PVMF_JITTER_BUFFER_NODE_MAX_RTP_FIREWALL_PKT_SIZE);
                                     fireWallPkt = PVMFMediaData::createMediaData(mediaDataImpl,
                                                   it->iMediaMsgAlloc);
                                    );

                            if (err != OsclErrNone)
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
                    int32 err;
                    if (iFireWallPacketInfo.iFormat == PVMF_JB_FW_PKT_FORMAT_PV)
                    {
                        OSCL_TRY(err,
                                 mediaDataImpl = it->iMediaDataImplAlloc->allocate(PVMF_JITTER_BUFFER_NODE_MAX_FIREWALL_PKT_SIZE);
                                 fireWallPkt = PVMFMediaData::createMediaData(mediaDataImpl,
                                               it->iMediaMsgAlloc);
                                );

                        if (err != OsclErrNone)
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
                        OSCL_TRY(err,
                                 mediaDataImpl = it->iMediaDataImplAlloc->allocate(PVMF_JITTER_BUFFER_NODE_MAX_RTP_FIREWALL_PKT_SIZE);
                                 fireWallPkt = PVMFMediaData::createMediaData(mediaDataImpl,
                                               it->iMediaMsgAlloc);
                                );

                        if (err != OsclErrNone)
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
    iFireWallPacketTimer->RunIfNotReady(iFireWallPacketInfo.iServerRoundTripDelayInMS*1000);
    return PVMFSuccess;
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

OsclSharedPtr<PVMFSharedSocketDataBufferAlloc>
PVMFJitterBufferNode::CreateResizablePortAllocator(uint32 aSize, OSCL_String& aName)
{
    uint8* my_ptr;
    OsclRefCounter* my_refcnt;
    OsclMemAllocator my_alloc;
    PVMFSharedSocketDataBufferAlloc *alloc_ptr = NULL;

    uint aligned_socket_alloc_size =
        oscl_mem_aligned_size(sizeof(PVMFSMSharedBufferAllocWithReSize));

    uint aligned_refcnt_size =
        oscl_mem_aligned_size(sizeof(OsclRefCounterSA<PVMFSharedSocketDataBufferAllocCleanupSA>));

    my_ptr = (uint8*) my_alloc.ALLOCATE(aligned_refcnt_size +
                                        aligned_socket_alloc_size);

    my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterSA<PVMFSharedSocketDataBufferAllocCleanupSA>(my_ptr));
    my_ptr += aligned_refcnt_size;

    iBufferAlloc = new PVMFSMSharedBufferAllocWithReSize(aSize, aName.get_cstr(),
            iMaxNumBufferResizes,
            iBufferResizeSize);

    alloc_ptr = OSCL_PLACEMENT_NEW(my_ptr, PVMFSharedSocketDataBufferAlloc(iBufferAlloc));
    OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> shared_alloc(alloc_ptr, my_refcnt);
    return (shared_alloc);
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

void PVMFJitterBufferNode::freechunkavailable(OsclAny* aContextData)
{
    OSCL_UNUSED_ARG(aContextData);
    if (oAutoPause == true)
    {
        PVMFJitterBufferStats stats;
        oAutoPause = false;
        Oscl_Vector<PVMFJitterBufferPortParams, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iPortParamsQueue.begin();
                it != iPortParamsQueue.end();
                it++)
        {
            if (it->oMonitorForRemoteActivity == false)
            {
                it->oMonitorForRemoteActivity = true;
                /* Cancel and reschedule in the inactivity timer */
                iRemoteInactivityTimer->Cancel();
                uint32 inactivityDurationInMS = iRemoteInactivityTimer->getMaxInactivityDurationInMS();
                iRemoteInactivityTimer->RunIfNotReady(inactivityDurationInMS*1000);
            }
            if (it->tag == PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                stats = it->iJitterBuffer->getJitterBufferStats();
            }
        }
        if (iBufferAlloc != NULL)
        {
            iBufferAlloc->CancelFreeChunkAvailableCallback();
        }
        ReportInfoEvent(PVMFJitterBufferNodeJitterBufferLowWaterMarkReached);
        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJitterBufferNode::freechunkavailable: Sending Auto Resume"));

        // just for logging
        uint32 availableSpace = iBufferAlloc->getTrueBufferSpace();
        uint32 totalBufferSize = iBufferAlloc->getTotalBufferSize();
        uint32 curroccupancy = (totalBufferSize - availableSpace);
        uint32 lowWaterMark = (uint32)(totalBufferSize * DEFAULT_JITTER_BUFFER_LOW_WATER_MARK);

        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL_E((0, "PVMFJBNode::freechunkavailable: Auto Resume"
                                               "Total=%d, Curr=%d, Avail=%d, LowM=%d",
                                               totalBufferSize, curroccupancy, availableSpace, lowWaterMark));
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
        uint64 timebase64 = 0;
        uint64 estServerClock = 0;
        uint64 clientClock = 0;
        iEstimatedServerClock->GetCurrentTime64(estServerClock, OSCLCLOCK_MSEC, timebase64);
        if (iClientPlayBackClock != NULL)
            iClientPlayBackClock->GetCurrentTime64(clientClock, OSCLCLOCK_MSEC, timebase64);
        PVMF_JBNODE_LOGDATATRAFFIC_FLOWCTRL((0, "PVMFJBNode::freechunkavailable - Auto Resume:"
                                             "MaxSNReg=%d, MaxTSReg=%d, LastSNRet=%d, LastTSRet=%d, NumMsgsInJB=%d, ServClk=%d, PlyClk=%d",
                                             stats.maxSeqNumRegistered, stats.maxTimeStampRegistered,
                                             stats.lastRetrievedSeqNum, stats.maxTimeStampRetrieved,
                                             (stats.maxSeqNumRegistered - stats.lastRetrievedSeqNum),
                                             Oscl_Int64_Utils::get_uint64_lower32(estServerClock),
                                             Oscl_Int64_Utils::get_uint64_lower32(clientClock)));
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
                    /* Cancel and reschedule in the inactivity timer */
                    iRemoteInactivityTimer->Cancel();
                    uint32 inactivityDurationInMS = iRemoteInactivityTimer->getMaxInactivityDurationInMS();
                    iRemoteInactivityTimer->RunIfNotReady(inactivityDurationInMS*1000);
                }
            }
            if (iBufferAlloc != NULL)
            {
                iBufferAlloc->CancelFreeChunkAvailableCallback();
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

                    uint64 currentTime64 = 0;
                    uint64 currentTimeBase64 = 0;
                    iEstimatedServerClock->GetCurrentTime64(currentTime64,
                                                            OSCLCLOCK_MSEC,
                                                            currentTimeBase64);

                    uint32 totalNumBytesRecvd = jbStats.totalNumBytesRecvd;
                    uint64 bytesRecvd64 = 0;
                    Oscl_Int64_Utils::set_uint64(bytesRecvd64, 0, totalNumBytesRecvd);
                    uint64 byteRate = 0;
                    if (Oscl_Int64_Utils::get_uint64_lower32(currentTime64) != 0)
                    {
                        byteRate = (bytesRecvd64 / currentTime64);
                    }
                    uint32 bitrate32 = Oscl_Int64_Utils::get_uint64_lower32(byteRate);
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
            }
        }
    }
    return true;
}

void PVMFJitterBufferNode::ClockStateUpdated()
{
    if (!oDelayEstablished)
    {
        // Don't let anyone start the clock while
        // we're rebuffering
        if (iClientPlayBackClock != NULL)
        {
            if (iClientPlayBackClock->GetState() == OsclClock::RUNNING)
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
#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    uint64 timebase64 = 0;
    uint64 clientClock64 = 0;
    uint64 serverClock64 = 0;
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
    iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - Before - EstServClock=%d",
                                 Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - Before - ClientClock=%d",
                                 Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
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
                                             OSCLCLOCK_MSEC);
    }

    // Reset the following flags for the new repositioning
    oSessionDurationExpired = false;
    oDelayEstablished = false;


#if (PVLOGGER_INST_LEVEL > PVLOGMSG_INST_LLDBG)
    timebase64 = 0;
    clientClock64 = 0;
    serverClock64 = 0;
    if (iClientPlayBackClock != NULL)
        iClientPlayBackClock->GetCurrentTime64(clientClock64, OSCLCLOCK_MSEC, timebase64);
    iEstimatedServerClock->GetCurrentTime64(serverClock64, OSCLCLOCK_MSEC, timebase64);
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - After - EstServClock=%d",
                                 Oscl_Int64_Utils::get_uint64_lower32(serverClock64)));
    PVMF_JBNODE_LOGCLOCK_REBUFF((0, "PVMFJitterBufferNode::PrepareForRepositioning - After - ClientClock=%d",
                                 Oscl_Int64_Utils::get_uint64_lower32(clientClock64)));
#endif
    return true;
}


