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
#ifndef PVMF_MP3FFPARSER_NODE_H_INCLUDED
#include "pvmf_mp3ffparser_node.h"
#endif
#ifndef IMP3FF_H_INCLUDED
#include "imp3ff.h"  // Includes for the core file format mp3 parser library
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_MEDIA_CMD_H_INCLUDED
#include "pvmf_media_cmd.h"
#endif
#ifndef PVMF_MEDIA_MSG_FORMAT_IDS_H_INCLUDED
#include "pvmf_media_msg_format_ids.h"
#endif
#ifndef PVMF_DURATION_INFOMESSAGE_H_INCLUDED
#include "pvmf_duration_infomessage.h"
#endif
#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif

#include "media_clock_converter.h"
#include "pv_gau.h"
#include "pvmf_source_context_data.h"

// Playback clock timescale
#define COMMON_PLAYBACK_CLOCK_TIMESCALE 1000

// Constants used for memory pools
#define PVMP3FF_MEDIADATA_CHUNKS_IN_POOL	8
#define PVMP3FF_MEDIADATA_CHUNKSIZE			128
// the maximum frame size depends on K * bitrate/sampling_rate
#define PVMP3FF_DEFAULT_MAX_FRAMESIZE		4096
#define PVMF3FF_DEFAULT_NUM_OF_FRAMES		2
#define PVMF3FF_DURATION_SCAN_AO_DELAY      1000
/**
 * Macros for calling PVLogger
 */
#define LOGERROR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);
#define LOGINFOHI(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG,iLogger,PVLOGMSG_INFO,m);
#define LOGINFOMED(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG,iLogger,PVLOGMSG_INFO,m);
#define LOGINFOLOW(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iLogger,PVLOGMSG_INFO,m);
#define LOGINFO(m) LOGINFOMED(m)

void PVMFMP3FFParserNode::Assert(bool aCondition)
{
    if (!aCondition)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_CRIT, (0, "PVMFMP3FFParserNode Assertion Failed!"));
        OSCL_ASSERT(0);
    }
}

/**
 * Constructor
 */
PVMFMP3FFParserNode::PVMFMP3FFParserNode(int32 aPriority)
        : OsclTimerObject(aPriority, "PVMFMP3FFParserNode"),
        iStreamID(0),
        iParseStatus(false),
        iSourceURLSet(false),
        iMP3File(NULL),
        iConfigOk(0),
        iExtensionRefCount(0),
        iMaxFrameSize(PVMP3FF_DEFAULT_MAX_FRAMESIZE),
        iMP3FormatBitrate(0),
        iLogger(NULL),
        iSendDecodeFormatSpecificInfo(true)
{
    iCPMContainer.iCPMLicenseInterface = NULL;
    iUseCPMPluginRegistry = false;
    iCPMContainer.iCPMMetaDataExtensionInterface   = NULL;
    iCPMGetMetaDataKeysCmdId = 0;
    iCPMGetMetaDataValuesCmdId = 0;
    oWaitingOnLicense  = false;
    iFileHandle = NULL;
    iAutoPaused = false;
    iDownloadProgressInterface = NULL;
    iDataStreamFactory         = NULL;
    iDataStreamInterface = NULL;
    iDataStreamReadCapacityObserver = NULL;
    iMP3ParserNodeMetadataValueCount = 0;
    iDownloadComplete = false;
    iFileSizeRecvd = false;
    iFileSize = 0;
    iCheckForMP3HeaderDuringInit = false;

    iDurationCalcAO = NULL;
    int32 err;

    OSCL_TRY(err,

             // Create the "InputCommandQueue". Use a reserve to avoid lots of
             // dynamic memory allocation.
             iInputCommands.Construct(PVMF_MP3FFPARSER_NODE_COMMAND_ID_START,
                                      PVMF_MP3FFPARSER_NODE_COMMAND_VECTOR_RESERVE);

             // Create "CurrentCommandQueue" and "CancelCommandQueue".
             // Both will contain only one command at a time, so use a reserve of 1.
             iCurrentCommand.Construct(0, 1);
             iCancelCommand.Construct(0, 1);

             // Create the port vector.
             iPortVector.Construct(PVMF_MP3FFPARSER_NODE_PORT_VECTOR_RESERVE);

             // Set the node capability data.
             // This node can support an unlimited number of ports.
             iCapability.iCanSupportMultipleInputPorts = false;
             iCapability.iCanSupportMultipleOutputPorts = false;
             iCapability.iHasMaxNumberOfPorts = false;
             iCapability.iMaxNumberOfPorts = 0;//no maximum
             iCapability.iInputFormatCapability.push_back(PVMF_MP3);
             iCapability.iOutputFormatCapability.push_back(PVMF_MP3);
             // secondry construction
             Construct();
            );

    if (err != OsclErrNone)
    {
        //if a leave happened, cleanup and re-throw the error
        iInputCommands.clear();
        iCurrentCommand.clear();
        iCancelCommand.clear();
        iPortVector.clear();
        iCapability.iInputFormatCapability.clear();
        iCapability.iOutputFormatCapability.clear();
        OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
        OSCL_CLEANUP_BASE_CLASS(OsclTimerObject);
        iFileServer.Close();
        OSCL_LEAVE(err);
    }


}

/**
 * Secondary constructor
 */
void PVMFMP3FFParserNode::Construct()
{
    iFileServer.Connect();
    iCPMContainer.Construct(PVMFSubNodeContainerBaseMp3::ECPM, this);
    //create the sub-node command queue.  Use a reserve to avoid
    //dynamic memory failure later.
    //Max depth is the max number of sub-node commands for any one node command.
    //Init command may take up to 9
    iSubNodeCmdVec.reserve(9);
}

/**
 * Destructor
 */
PVMFMP3FFParserNode::~PVMFMP3FFParserNode()
{
    //ThreadLogoff
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    if (iDurationCalcAO)
    {
        OSCL_DELETE(iDurationCalcAO);
        iDurationCalcAO = NULL;
    }
    // Unbind the download progress clock
    iDownloadProgressClock.Unbind();
    // Release the download progress interface, if any
    if (iDownloadProgressInterface)
    {
        iDownloadProgressInterface->removeRef();
        iDownloadProgressInterface = NULL;
    }

    //if any CPM commands are pending, there will be a crash when they callback,
    //so panic here instead.
    if (iCPMContainer.CmdPending())
    {
        OsclError::Panic("PVMP3FF", 1);
    }

    //Cleanup allocated ports
    while (!iPortVector.empty())
    {
        iPortVector.Erase(&iPortVector.front());
    }

    //Cleanup commands
    // The command queues are self-deleting, but we should notify the observer
    // of unprocessed commands.
    while (!iCancelCommand.empty())
    {
        CommandComplete(iCancelCommand, iCancelCommand.front(), PVMFFailure, NULL, NULL);
    }
    while (!iCurrentCommand.empty())
    {
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFFailure, NULL, NULL);
    }
    while (!iInputCommands.empty())
    {
        CommandComplete(iInputCommands, iInputCommands.front(), PVMFFailure, NULL, NULL);
    }

    ReleaseTrack();
    // Clean up the file source
    CleanupFileSource();
    //CPM Cleanup should be done after CleanupFileSource
    iCPMContainer.Cleanup();
    // Disconnect fileserver
    iFileServer.Close();
}

/**
 * Public Node API implementation
 */

/**
 * Do thread-specific node creation and go to "Idle" state.
 */
PVMFStatus PVMFMP3FFParserNode::ThreadLogon()
{
    LOGINFO((0, "PVMFMP3FFParserNode::ThreadLogon() iInterfaceState =%d", iInterfaceState));
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
            {
                AddToScheduler();
            }
            iLogger = PVLogger::GetLoggerObject("PVMFMP3FFParserNode");
            SetState(EPVMFNodeIdle);
            return PVMFSuccess;

        default:
            LOGINFO((0, "PVMFMP3FFParserNode::ThreadLogon() iInterfaceState =%d is Invalid State", iInterfaceState));
            return PVMFErrInvalidState;
    }
}

/**
 * Do thread-specific node cleanup and go to "Created" state.
 */
PVMFStatus PVMFMP3FFParserNode::ThreadLogoff()
{
    LOGINFO((0, "PVMFMP3FFParserNode::ThreadLogoff() iInterfaceState =%d", iInterfaceState));
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            if (IsAdded())
            {
                RemoveFromScheduler();
            }
            iLogger = NULL;
            SetState(EPVMFNodeCreated);
            return PVMFSuccess;

        default:
            LOGINFO((0, "PVMFMP3FFParserNode::ThreadLogoff() iInterfaceState =%d is Invalid State", iInterfaceState));
            return PVMFErrInvalidState;
    }
}

/**
 * Retrieve node capabilities.
 */
PVMFStatus PVMFMP3FFParserNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    LOGINFO((0, "PVMFMP3FFParserNode::GetCapability()"));
    aNodeCapability = iCapability;
    return PVMFSuccess;
}

/**
 * Retrive a port iterator.
 */
PVMFPortIter* PVMFMP3FFParserNode::GetPorts(const PVMFPortFilter* aFilter)
{
    LOGINFO((0, "PVMFMP3FFParserNode::GetPorts()"));
    //port filter is not implemented.
    OSCL_UNUSED_ARG(aFilter);
    iPortVector.Reset();
    return &iPortVector;
}

/**
 * Queue an asynchronous node command for QueryUUID
 */
PVMFCommandId PVMFMP3FFParserNode::QueryUUID(PVMFSessionId aSession,
        const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::QueryUUID()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_QUERYUUID,
            aMimeType,
            aUuids,
            aExactUuidsOnly,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for QueryInterface
 */
PVMFCommandId PVMFMP3FFParserNode::QueryInterface(PVMFSessionId aSession,
        const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::QueryInterface()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_QUERYINTERFACE,
            aUuid,
            aInterfacePtr,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for RequestPort
 */
PVMFCommandId PVMFMP3FFParserNode::RequestPort(PVMFSessionId aSession,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aPortConfig);
    LOGINFO((0, "PVMFMP3FFParserNode::RequestPort()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_REQUESTPORT,
            aPortTag,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for ReleasePort
 */
PVMFCommandId PVMFMP3FFParserNode::ReleasePort(PVMFSessionId aSession,
        PVMFPortInterface& aPort,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::ReleasePort()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_RELEASEPORT,
            aPort,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Init
 */
PVMFCommandId PVMFMP3FFParserNode::Init(PVMFSessionId aSession,
                                        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Init()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_INIT,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Prepare
 */
PVMFCommandId PVMFMP3FFParserNode::Prepare(PVMFSessionId aSession,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Prepare()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_PREPARE,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Start
 */
PVMFCommandId PVMFMP3FFParserNode::Start(PVMFSessionId aSession,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Start()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_START,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Stop
 */
PVMFCommandId PVMFMP3FFParserNode::Stop(PVMFSessionId aSession,
                                        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Stop()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_STOP,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Flush
 */
PVMFCommandId PVMFMP3FFParserNode::Flush(PVMFSessionId aSession,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Flush()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_FLUSH,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Pause
 */
PVMFCommandId PVMFMP3FFParserNode::Pause(PVMFSessionId aSession,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Pause()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_PAUSE,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for Reset
 */
PVMFCommandId PVMFMP3FFParserNode::Reset(PVMFSessionId aSession,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::Reset()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_RESET,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for CancelAllCommands
 */
PVMFCommandId PVMFMP3FFParserNode::CancelAllCommands(PVMFSessionId aSession,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::CancelAllCommands()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_CANCELALLCOMMANDS,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command for CancelCommand
 */
PVMFCommandId PVMFMP3FFParserNode::CancelCommand(PVMFSessionId aSession,
        PVMFCommandId aCmdId,
        const OsclAny* aContext)
{
    LOGINFO((0, "PVMFMP3FFParserNode::CancelCommand()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSession,
            PVMF_GENERIC_NODE_CANCELCOMMAND,
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
PVMFCommandId PVMFMP3FFParserNode::QueueCommandL(PVMFMP3FFParserNodeCommand& aCmd)
{
    if (IsAdded())
    {
        PVMFCommandId id;
        id = iInputCommands.AddL(aCmd);
        /* Wakeup the AO */
        RunIfNotReady();
        return id;
    }
    OSCL_LEAVE(OsclErrInvalidState);
    return -1;
}

/**
 * Asynchronous Command processing routines.
 * These routines are all called under the AO.
 */

/**
 * Called by the command handler AO to process a command from
 * the input queue.
 */
void PVMFMP3FFParserNode::ProcessCommand()
{
    // Can't do anything when an asynchronous cancel is in progress
    // need to wait on completion
    if (!iCancelCommand.empty())
    {
        return; //keep waiting.
    }

    // If a command is in progress, only a hi-pri command can interrupt it.
    if (!iCurrentCommand.empty() && !iInputCommands.front().hipri() && iInputCommands.front().iCmd != PVMP3FF_NODE_CMD_CANCEL_GET_LICENSE)
    {
        return; //keep waiting
    }

    // InputCommandQueue shall have at least one command queued
    OSCL_ASSERT(!iInputCommands.empty());
    // This call will process the first node command in the input queue.
    // The newest or highest pri command is in the front of the queue.
    PVMFMP3FFParserNodeCommand& aCmd = iInputCommands.front();

    PVMFStatus cmdstatus;
    OsclAny* eventdata = NULL;
    PVInterface* extmsg = NULL;
    if (aCmd.hipri() || iInputCommands.front().iCmd == PVMP3FF_NODE_CMD_CANCEL_GET_LICENSE)
    {
        //Process the Hi-Pri commands
        switch (aCmd.iCmd)
        {
            case PVMF_GENERIC_NODE_CANCELALLCOMMANDS:
                cmdstatus = DoCancelAllCommands(aCmd);
                break;
            case PVMF_GENERIC_NODE_CANCELCOMMAND:
                cmdstatus = DoCancelCommand(aCmd);
                break;
            case PVMP3FF_NODE_CMD_CANCEL_GET_LICENSE:
                cmdstatus = DoCancelGetLicense(aCmd);
                break;

            default:
                cmdstatus = PVMFErrNotSupported;
                break;
        } // end switch, processing hi-priority commands

        // If completion is pending, move the command from the InputCommandQueue
        // to the CancelCommandQueue. This is necessary since the InputCommandQueue
        // could get rearranged by new in-coming commands.
        if (cmdstatus == PVMFPending)
        {
            iCancelCommand.StoreL(aCmd);
            iInputCommands.Erase(&aCmd);
        }
    }
    else
    {
        //Process the normal pri commands.
        switch (aCmd.iCmd)
        {
            case PVMF_GENERIC_NODE_QUERYUUID:
                cmdstatus = DoQueryUuid(aCmd);
                break;
            case PVMF_GENERIC_NODE_QUERYINTERFACE:
                cmdstatus = DoQueryInterface(aCmd);
                break;
            case PVMF_GENERIC_NODE_REQUESTPORT:
            {
                PVMFPortInterface*port;
                cmdstatus = DoRequestPort(aCmd, port);
                eventdata = (OsclAny*)port;
            }
            break;
            case PVMF_GENERIC_NODE_RELEASEPORT:
                cmdstatus = DoReleasePort(aCmd);
                break;
            case PVMF_GENERIC_NODE_INIT:
                cmdstatus = DoInit(aCmd);
                break;
            case PVMF_GENERIC_NODE_PREPARE:
                cmdstatus = DoPrepare(aCmd);
                break;
            case PVMF_GENERIC_NODE_START:
                cmdstatus = DoStart(aCmd);
                break;
            case PVMF_GENERIC_NODE_STOP:
                cmdstatus = DoStop(aCmd);
                break;
            case PVMF_GENERIC_NODE_FLUSH:
                cmdstatus = DoFlush(aCmd);
                break;
            case PVMF_GENERIC_NODE_PAUSE:
                cmdstatus = DoPause(aCmd);
                break;
            case PVMF_GENERIC_NODE_RESET:
                cmdstatus = DoReset(aCmd);
                break;
            case PVMP3FF_NODE_CMD_GETNODEMETADATAKEY:
                cmdstatus = DoGetMetadataKeys(aCmd);
                break;
            case PVMP3FF_NODE_CMD_GETNODEMETADATAVALUE:
                cmdstatus = DoGetMetadataValues(aCmd);
                break;
            case PVMP3FF_NODE_CMD_SETDATASOURCEPOSITION:
                cmdstatus = DoSetDataSourcePosition(aCmd);
                break;
            case PVMP3FF_NODE_CMD_QUERYDATASOURCEPOSITION:
                cmdstatus = DoQueryDataSourcePosition(aCmd);
                break;
            case PVMP3FF_NODE_CMD_SETDATASOURCERATE:
                cmdstatus = DoSetDataSourceRate(aCmd);
                break;
            case PVMP3FF_NODE_CMD_GET_LICENSE_W:
                cmdstatus = DoGetLicense(aCmd, true);
                break;

            case PVMP3FF_NODE_CMD_GET_LICENSE:
                cmdstatus = DoGetLicense(aCmd);
                break;
            default:
                // command not supported
                OSCL_ASSERT(false);
                cmdstatus = PVMFFailure;
                break;
        } // end switch, processing low priority commands

        // If completion is pending, move the command from the InputCommandQueue
        // to the CurrentCommandQueue. This is necessary since the InputCommandQueue
        // could get rearranged by new in-coming commands.
        if (cmdstatus == PVMFPending)
        {
            iCurrentCommand.StoreL(aCmd);
            iInputCommands.Erase(&aCmd);
        }
    } // end else

    // The command has been processed, this means that the command might
    // have been failed/succeeded, but it is no more pending
    // report command complete to the observer
    if (cmdstatus != PVMFPending)
    {
        CommandComplete(iInputCommands, aCmd, cmdstatus, extmsg, eventdata);
    }
}

void PVMFMP3FFParserNode::CompleteInit(PVMFStatus aStatus)
{
    if ((iCurrentCommand.empty() == false) &&
            (iCurrentCommand.front().iCmd == PVMF_GENERIC_NODE_INIT))
    {
        if (!iSubNodeCmdVec.empty())
        {
            iSubNodeCmdVec.front().iSubNodeContainer->CommandDone(PVMFSuccess, NULL, NULL);
        }
        else
        {
            CommandComplete(iCurrentCommand, iCurrentCommand.front(), aStatus, NULL, NULL);
        }
    }
}

/**
 * Reports command complete event to the observer, switches node state according to
 * the command processed, Erases the command from the queue and reports command
 * complete event to the Node Observer
 */
void PVMFMP3FFParserNode::CommandComplete(PVMFMP3FFParserNodeCmdQ& aCmdQ,
        PVMFMP3FFParserNodeCommand& aCmd,
        PVMFStatus aStatus, PVInterface* aExtMsg,
        OsclAny* aEventData)
{
    LOGINFO((0, "PVMFMP3FFParserNode::CommandComplete() Id %d Cmd %d Status %d Context %d Data %d"
             , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    // If the command failed or was cancelled there may be un-processed
    // sub-node commands, so clear the vector now.
    if (!iSubNodeCmdVec.empty())
    {
        iSubNodeCmdVec.clear();
    }

    //Do state transitions according the command was processed successfully
    if (aStatus == PVMFSuccess)
    {
        switch (aCmd.iCmd)
        {
            case PVMF_GENERIC_NODE_INIT:
                SetState(EPVMFNodeInitialized);
                break;
            case PVMF_GENERIC_NODE_PREPARE:
                SetState(EPVMFNodePrepared);
                break;
            case PVMF_GENERIC_NODE_START:
                SetState(EPVMFNodeStarted);
                break;
            case PVMF_GENERIC_NODE_STOP:
            case PVMF_GENERIC_NODE_FLUSH:
                SetState(EPVMFNodePrepared);
                break;
            case PVMF_GENERIC_NODE_PAUSE:
                SetState(EPVMFNodePaused);
                break;
            case PVMF_GENERIC_NODE_RESET:
                SetState(EPVMFNodeIdle);
                ThreadLogoff();
                break;
            default:
                break;
        }
    }
    else
    {
        // Log that the command completion was failed
        PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMFMP3FFParserNode:CommandComplete Failed!"));
    }

    // Create response to notify the observer
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aExtMsg, aEventData);
    PVMFSessionId session = aCmd.iSession;

    // Erase the command from the queue
    aCmdQ.Erase(&aCmd);

    //Report command completion to the session observer
    ReportCmdCompleteEvent(session, resp);

    // Re-schedule Node if the AO is active and there are additional
    // commands to be processed.
    if (iInputCommands.size() > 0 && IsAdded())
    {
        RunIfNotReady();
    }
}

/**
 * CommandHandler for Reset command
 */
PVMFStatus PVMFMP3FFParserNode::DoReset(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoReset() In"));

    OSCL_UNUSED_ARG(aCmd);
    // Check if the node can accept the Reset command in the current state
    PVMFStatus status;
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
        case EPVMFNodeIdle:
        case EPVMFNodeInitialized:
        case EPVMFNodePrepared:
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        case EPVMFNodeError:
        {
            if (iDurationCalcAO->IsBusy())
            {
                iDurationCalcAO->Cancel();
            }
            // Delete all ports and notify observer
            while (!iPortVector.empty())
            {
                iPortVector.Erase(&iPortVector.front());
            }

            // Restore original port vector reserve.
            iPortVector.Reconstruct();
            // Stop and cleanup
            ReleaseTrack();
            CleanupFileSource();
            // Cleanup CPM
            if (iCPMContainer.iCPM)
            {
                Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMUsageComplete);
                Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMCloseSession);
                Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMReset);
                Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMCleanup);
                RunIfNotReady();
                //wait on CPM commands to execute
                status = PVMFPending;
            }
            else
            {
                status = PVMFSuccess;
            }
        }
        break;
        default:
            // Reset was not processed becasue of Node's invalid state
            status = PVMFErrInvalidState;
            break;
    }
    return status;
}

/**
 * CommandHandler for Query UUID
 */
PVMFStatus PVMFMP3FFParserNode::DoQueryUuid(PVMFMP3FFParserNodeCommand& aCmd)
{
    // This node supports Query UUID from any state
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoQueryUuid() In"));

    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;
    aCmd.PVMFMP3FFParserNodeCommandBase::Parse(mimetype, uuidvec, exactmatch);

    // Match the input mimetype against any of the custom interfaces
    // for this node
    if (*mimetype == PVMF_DATA_SOURCE_INIT_INTERFACE_MIMETYPE)
    {
        PVUuid uuid(PVMF_DATA_SOURCE_INIT_INTERFACE_UUID);
        uuidvec->push_back(uuid);
    }
    else if (*mimetype == PVMF_TRACK_SELECTION_INTERFACE_MIMETYPE)
    {
        PVUuid uuid(PVMF_TRACK_SELECTION_INTERFACE_UUID);
        uuidvec->push_back(uuid);
    }
    else if (*mimetype == PVMF_META_DATA_EXTENSION_INTERFACE_MIMETYPE)
    {
        PVUuid uuid(KPVMFMetadataExtensionUuid);
        uuidvec->push_back(uuid);
    }
    else if (*mimetype == PVMF_FF_PROGDOWNLOAD_SUPPORT_INTERFACE_MIMETYPE)
    {
        PVUuid uuid(PVMF_FF_PROGDOWNLOAD_SUPPORT_INTERFACE_UUID);
        uuidvec->push_back(uuid);
    }
    else if (*mimetype == PVMF_DATA_SOURCE_PLAYBACK_CONTROL_INTERFACE_MIMETYPE)
    {
        PVUuid uuid(PvmfDataSourcePlaybackControlUuid);
        uuidvec->push_back(uuid);
    }
    else if (*mimetype == PVMI_DATASTREAMUSER_INTERFACE_MIMETYPE)
    {
        PVUuid uuid(PVMIDatastreamuserInterfaceUuid);
        uuidvec->push_back(uuid);
    }
    return PVMFSuccess;
}

/**
 * CommandHandler for Query Interface
 */
PVMFStatus PVMFMP3FFParserNode::DoQueryInterface(PVMFMP3FFParserNodeCommand& aCmd)
{
    // This node supports Query Interface from any state
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoQueryInterface() In"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.PVMFMP3FFParserNodeCommandBase::Parse(uuid, ptr);
    PVMFStatus status;

    if (queryInterface(*uuid, *ptr))
    {
        status = PVMFSuccess;
    }
    else
    {
        // Interface not supported
        *ptr = NULL;
        status = PVMFFailure;
    }
    return status;
}

/**
 * CommandHandler for port request
 */
PVMFStatus PVMFMP3FFParserNode::DoRequestPort(PVMFMP3FFParserNodeCommand& aCmd,
        PVMFPortInterface*&aPort)
{
    // This node supports port request from any state
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoRequestPort In"));
    aPort = NULL;
    // Retrieve port tag.
    int32 tag;
    OSCL_String* mimetype;
    aCmd.PVMFMP3FFParserNodeCommandBase::Parse(tag, mimetype);

    //mimetype is not used on this node
    //validate the tag
    if (tag != PVMF_MP3FFPARSER_NODE_PORT_TYPE_SOURCE)
    {
        // Invalid port tag
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFMP3FFParserNode::DoRequestPort: Error - Invalid port tag"));
        return PVMFFailure;
    }

    //Allocate a new port
    OsclAny *ptr = NULL;
    int32 err = 0;
    OSCL_TRY(err, ptr = iPortVector.Allocate(););
    if (err != OsclErrNone || !ptr)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFMP3FFParserNode::DoRequestPort: Error - iPortVector Out of memory"));
        return PVMFErrNoMemory;
    }

    // Create base port with default settings...
    PVMFMP3FFParserPort*outport = new(ptr) PVMFMP3FFParserPort(tag, this,
            0, 0, 0,	// input queue isn't needed.
            DEFAULT_DATA_QUEUE_CAPACITY,
            DEFAULT_DATA_QUEUE_CAPACITY,
            DEFAULT_READY_TO_RECEIVE_THRESHOLD_PERCENT);

    //Add the port to the port vector.
    err = 0;
    OSCL_TRY(err, iPortVector.AddL(outport););
    if (err != OsclErrNone)
    {
        return PVMFErrNoMemory;
    }

    OsclMemPoolResizableAllocator* trackdatamempool = NULL;
    TrackDataMemPoolProxyAlloc* trackdatamempoolproxy = NULL;
    PVMFSimpleMediaBufferCombinedAlloc *mediadataimplalloc = NULL;
    PVMFMemPoolFixedChunkAllocator* mediadatamempool = NULL;
    MediaClockConverter* clockconv = NULL;
    err = 0;
    // Try block starts
    OSCL_TRY(err,
             // Instantiate the mem pool which will hold the actual track data
             trackdatamempool = OSCL_NEW(OsclMemPoolResizableAllocator,
                                         (2 * PVMF3FF_DEFAULT_NUM_OF_FRAMES * iMaxFrameSize, 2));
             trackdatamempoolproxy = OSCL_NEW(TrackDataMemPoolProxyAlloc, (*trackdatamempool));

             // Instantiate an allocator for the mediadata implementation,
             //have it use the mem pool defined above as its allocator
             mediadataimplalloc = OSCL_NEW(PVMFSimpleMediaBufferCombinedAlloc,
                                           (trackdatamempoolproxy));
             // Instantiate another memory pool for the media data structures.
             mediadatamempool = OSCL_NEW(PVMFMemPoolFixedChunkAllocator,
                                         ("Mp3FFPar", PVMP3FF_MEDIADATA_CHUNKS_IN_POOL,
                                          PVMP3FF_MEDIADATA_CHUNKSIZE));
             clockconv = OSCL_NEW(MediaClockConverter, (iMP3File->GetTimescale()));
            ); // Try block end

    if (err != 0 || trackdatamempool == NULL || !trackdatamempoolproxy ||
            mediadataimplalloc == NULL || mediadatamempool == NULL || clockconv == NULL)
    {
        if (clockconv)
        {
            OSCL_DELETE(clockconv);
        }
        if (mediadatamempool)
        {
            OSCL_DELETE(mediadatamempool);
        }
        if (mediadataimplalloc)
        {
            OSCL_DELETE(mediadataimplalloc);
        }
        if (trackdatamempool)
        {
            trackdatamempool->removeRef();
        }
        if (trackdatamempoolproxy)
        {
            OSCL_DELETE(trackdatamempoolproxy);
        }
        iPortVector.Erase(&outport);

        return PVMFErrNoMemory;
    }

    // Instantiate the PVMP3FFNodeTrackPortInfo object that contains the port.
    iTrack.iPort = outport;
    iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_UNINITIALIZED;
    iTrack.iClockConverter = clockconv;
    iTrack.iMediaDataMemPool = mediadatamempool;
    iTrack.iTrackDataMemoryPool = trackdatamempool;
    iTrack.iTrackDataMemoryPoolProxy = trackdatamempoolproxy;
    iTrack.iMediaDataImplAlloc = mediadataimplalloc;
    iTrack.timestamp_offset = 0;

    // Return the port pointer to the caller.
    aPort = outport;
    return PVMFSuccess;
}

/**
 * Called by the command handler AO to do the port release
 */
PVMFStatus PVMFMP3FFParserNode::DoReleasePort(PVMFMP3FFParserNodeCommand& aCmd)
{
    //This node supports release port from any state
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoReleasePort() In"));
    //Find the port in the port vector
    PVMFStatus status;
    PVMFMP3FFParserPort* port;
    aCmd.PVMFMP3FFParserNodeCommandBase::Parse((PVMFPortInterface*&)port);
    PVMFMP3FFParserPort** portPtr = iPortVector.FindByValue(port);
    if (iDurationCalcAO->IsBusy())
    {
        iDurationCalcAO->Cancel();
    }
    if (portPtr)
    {
        //delete the port.
        iPortVector.Erase(portPtr);
        ReleaseTrack();
        status = PVMFSuccess;
    }
    else
    {
        //port not found.
        status = PVMFFailure;
    }
    return status;
}

/**
 * Called by the command handler AO to do the node Init
 */
PVMFStatus PVMFMP3FFParserNode::DoInit(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoInit() In"));

    OSCL_UNUSED_ARG(aCmd);
    PVMFStatus status = PVMFSuccess;
    // Process Init according to the Node State
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            if (iMP3File == NULL)
            {
                // Instantiate the IMpeg3File object, this class represents the mp3ff library
                MP3ErrorType bSuccess = MP3_SUCCESS;
                int32 leavecode = 0;
                PVMFDataStreamFactory* dsFactory = iCPMContainer.iCPMContentAccessFactory;
                if ((dsFactory == NULL) && (iDataStreamFactory != NULL))
                {
                    dsFactory = iDataStreamFactory;
                }

                // Try block start
                OSCL_TRY(leavecode, iMP3File = OSCL_NEW(IMpeg3File,
                                                        (iSourceURL, bSuccess,
                                                         &iFileServer, dsFactory,
                                                         iFileHandle, false)));
                // Try block end

                if (leavecode || !iMP3File || (bSuccess != MP3_SUCCESS))
                {
                    // creation of IMpeg3File object failed or resulted in error
                    SetState(EPVMFNodeError);
                    return PVMFErrResource;
                }
                OSCL_TRY(leavecode, iDurationCalcAO = OSCL_NEW(PVMp3DurationCalculator,
                                                      (OsclActiveObject::EPriorityIdle, iMP3File, this)));
                if (leavecode || !iDurationCalcAO)
                {
                    // creation of iDurationCalcAO object failed or resulted in error
                    SetState(EPVMFNodeError);
                    return PVMFErrResource;
                }
            }
            //If a CPM flag was provided in the source data, then
            //we need to go through the CPM sequence to check access on the file.
            if (iUseCPMPluginRegistry)
            {
                if (oWaitingOnLicense == false)
                {
                    Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMInit);
                    Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMOpenSession);
                    Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMRegisterContent);
                }
                else
                {
                    Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMApproveUsage);
                    Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMCheckUsage);
                }
                RunIfNotReady();
                status = PVMFPending;
            }
            else
            {
                status = CheckForMP3HeaderAvailability();
                if (status == MP3_SUCCESS)
                {
                    iDurationCalcAO->ScheduleAO();
                }
            }
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }
    return status;
}

/**
 * CommandHandler for node Prepare
 */
PVMFStatus PVMFMP3FFParserNode::DoPrepare(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoPrepare() In"));

    OSCL_UNUSED_ARG(aCmd);
    // Process Prepare according to the Node State
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
            //this node doesn't do anything to get ready to start.
            break;

        default:
            return PVMFErrInvalidState;
            break;
    }
    // If this is an PDL session request callback from ProgressInterface when data with
    // ts TimeStamp is downloaded, DPI shall callback the node once data is downloaded
    if ((iDownloadProgressInterface != NULL) &&
            (iDownloadComplete == false))
    {
        // check for download complete
        // if download is not complete, request to be notified when data is ready
        uint32 bytesReady = 0;
        PvmiDataStreamStatus status = iDataStreamInterface->QueryReadCapacity(iDataStreamSessionID, bytesReady);
        if (status == PVDS_END_OF_STREAM)
        {
            if (!iFileSizeRecvd)
            {
                iFileSize = bytesReady;
                iFileSizeRecvd = true;
            }
            return PVMFSuccess;
        }
        uint32 ts = 0;
        iDownloadProgressInterface->requestResumeNotification(ts, iDownloadComplete);
        // Data is not available, autopause the track.
        iAutoPaused = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVMFMP3FFParserNode::DoPrepare() - Auto Pause Triggered, TS = %d", ts));
    }
    return PVMFSuccess;
}

/**
 * CommandHandler for node Start
 */
PVMFStatus PVMFMP3FFParserNode::DoStart(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoStart() In"));

    OSCL_UNUSED_ARG(aCmd);
    PVMFStatus status = PVMFSuccess;
    // Process Start according to the Node State
    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
            break;

        case EPVMFNodePaused:
        {
            iAutoPaused = false;
            // If track was in Autopause state, change track state to
            // retrieve more data in case
            if (PVMP3FFNodeTrackPortInfo::TRACKSTATE_DOWNLOAD_AUTOPAUSE == iTrack.iState)
            {
                iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
            }
            break;
        }
        default:
            status = PVMFErrInvalidState;
            break;
    }
    return status;
}

/**
 * CommandHandler for node Stop
 */
PVMFStatus PVMFMP3FFParserNode::DoStop(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoStop() In"));

    OSCL_UNUSED_ARG(aCmd);
    PVMFStatus status = PVMFSuccess;
    // Process Stop according to the Node State
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            // Clear queued messages in ports
            uint32 index;
            for (index = 0; index < iPortVector.size(); index++)
            {
                iPortVector[index]->ClearMsgQueues();
            }
            // Position the parser to beginning of file
            iMP3File->SeekToTimestamp(0);
            // reset the track
            ResetTrack();
            status = PVMFSuccess;
            iFileSizeRecvd = false;
            iDownloadComplete = false;
            iDurationCalcAO->Cancel();
        }
        break;
        default:
            status = PVMFErrInvalidState;
            break;
    }
    return status;
}

/**
 * CommandHandler for node Flush
 */
PVMFStatus PVMFMP3FFParserNode::DoFlush(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoFlush() In"));

    // Process Flush according to the Node State
    PVMFStatus status = PVMFSuccess;
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            // Flush is asynchronous. Move the command from InputCommandQueue
            // to the current command, where it will remain until the flush
            // completes
            int32 err;
            OSCL_TRY(err, iCurrentCommand.StoreL(aCmd););
            if (err != OsclErrNone)
            {
                // Was unable to move the command to the CurrentCommandQueue
                // because of Memory unavailability
                status = PVMFErrNoMemory;
                break;
            }
            // Command copied to CurrentCommandQueue, now erase from InputCommandQueue
            iInputCommands.Erase(&aCmd);

            // Notify all ports to suspend their input
            for (uint32 index = 0;index < iPortVector.size();index++)
            {
                iPortVector[index]->SuspendInput();
            }
            // reset the track
            ResetTrack();
            if (iDurationCalcAO->IsBusy())
            {
                iDurationCalcAO->Cancel();
            }
            // Completion of Flush shall be handled by Run()
            status = PVMFPending;
        }
        break;
        default:
            // Could not perform flush in current state
            status = PVMFErrInvalidState;
            break;
    }
    return status;
}

/**
 * A routine to tell if a flush operation is in progress.
 */
bool PVMFMP3FFParserNode::FlushPending()
{
    // Check if current command is FLUSH
    return (iCurrentCommand.size() > 0 &&
            iCurrentCommand.front().iCmd == PVMF_GENERIC_NODE_FLUSH);
}


/**
 * CommandHandler for node Pause
 */
PVMFStatus PVMFMP3FFParserNode::DoPause(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoPause() In"));

    OSCL_UNUSED_ARG(aCmd);
    PVMFStatus status = PVMFSuccess;
    // Process Pause according to the Node State
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
            status = PVMFSuccess;
            break;
        default:
            status = PVMFErrInvalidState;
    }
    return status;
}

/**
 * CommandHandler for Cancelling All Commands
 */
PVMFStatus PVMFMP3FFParserNode::DoCancelAllCommands(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoCancelAllCommands() In"));

    OSCL_UNUSED_ARG(aCmd);
    // First cancel the current command if any
    while (!iCurrentCommand.empty())
    {
        if (iCPMContainer.CancelPendingCommand())
        {
            return PVMFPending;//wait on sub-node cancel to complete.
        }
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFErrCancelled, NULL, NULL);
    }

    // Next cancel all InputCommands, start at element 1 since
    // this cancel command is element 0.
    while (iInputCommands.size() > 1)
    {
        CommandComplete(iInputCommands, iInputCommands[1], PVMFErrCancelled, NULL, NULL);
    }
    return PVMFSuccess;
}

/**
 * CommandHandler for Cancelling a single command
 */
PVMFStatus PVMFMP3FFParserNode::DoCancelCommand(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoCancelCommand() In"));

    // Extract the command ID to be cancelled
    PVMFCommandId id;
    aCmd.PVMFMP3FFParserNodeCommandBase::Parse(id);

    //first check "current" command if any
    PVMFMP3FFParserNodeCommand* cmd = iCurrentCommand.FindById(id);
    if (cmd)
    {
        if (iCPMContainer.CancelPendingCommand())
        {
            return PVMFPending;//wait on sub-node cancel to complete.
        }
        CommandComplete(iCurrentCommand, *cmd, PVMFErrCancelled, NULL, NULL);
        return PVMFSuccess;
    }

    // Next cancel all InputCommands, start at element 1 since
    // this cancel command is element 0.
    cmd = iInputCommands.FindById(id, 1);
    if (cmd)
    {
        //cancel the queued command
        CommandComplete(iInputCommands, *cmd, PVMFErrCancelled, NULL, NULL);
        //report cancel success
        return PVMFSuccess;
    }
    //if we get here the command isn't queued so the cancel fails.
    return PVMFFailure;
}

/**
 * CommandHandler for fetching Metadata Keys
 */
PVMFStatus PVMFMP3FFParserNode::DoGetMetadataKeys(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoGetMetadataKeys() In"));

    /* Get Metadata keys from CPM for protected content only */
    if (iCPMContainer.iCPMMetaDataExtensionInterface != NULL)
    {
        GetCPMMetaDataKeys();
        return PVMFPending;
    }
    return (CompleteGetMetadataKeys(aCmd));
}

PVMFStatus
PVMFMP3FFParserNode::CompleteGetMetadataKeys(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::CompleteGetMetadataKeys() In"));

    PVMFMetadataList* keylistptr = NULL;
    uint32 starting_index;
    int32 max_entries;
    char* query_key;

    aCmd.PVMFMP3FFParserNodeCommand::Parse(keylistptr, starting_index, max_entries, query_key);
    // Check parameters
    if (keylistptr == NULL || !iMP3File)
    {
        // The list pointer is invalid
        return PVMFErrArgument;
    }
    // The underlying mp3 ff library will fill in the keys.
    iMP3File->GetMetadataKeys(*keylistptr, starting_index, max_entries, query_key);

    /* Copy the requested keys */
    uint32 num_entries = 0;
    int32 num_added = 0;
    int32 leavecode = 0;
    uint32 lcv = 0;

    for (lcv = 0; lcv < iCPMMetadataKeys.size(); lcv++)
    {
        if (query_key == NULL)
        {
            /* No query key so this key is counted */
            ++num_entries;
            if (num_entries > (uint32)starting_index)
            {
                /* Past the starting index so copy the key */
                leavecode = 0;
                OSCL_TRY(leavecode, keylistptr->push_back(iCPMMetadataKeys[lcv]));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFMP3FFParserNode::CompleteGetMetadataKeys() Memory allocation failure when copying metadata key"));
                                     return PVMFErrNoMemory);
                num_added++;
            }
        }
        else
        {
            /* Check if the key matches the query key */
            if (pv_mime_strcmp(iCPMMetadataKeys[lcv].get_cstr(), query_key) >= 0)
            {
                /* This key is counted */
                ++num_entries;
                if (num_entries > (uint32)starting_index)
                {
                    /* Past the starting index so copy the key */
                    leavecode = 0;
                    OSCL_TRY(leavecode, keylistptr->push_back(iCPMMetadataKeys[lcv]));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFMP3FFParserNode::CompleteGetMetadataKeys() Memory allocation failure when copying metadata key"));
                                         return PVMFErrNoMemory);
                    num_added++;
                }
            }
        }
        /* Check if max number of entries have been copied */
        if ((max_entries > 0) && (num_added >= max_entries))
        {
            break;
        }
    }
    return PVMFSuccess;

}

/**
 * CommandHandler for fetching Metadata Values
 */
PVMFStatus PVMFMP3FFParserNode::DoGetMetadataValues(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoGetMetadataValues() In"));

    PVMFMetadataList* keylistptr = NULL;
    Oscl_Vector<PvmiKvp, OsclMemAllocator>* valuelistptr = NULL;
    uint32 starting_index;
    int32 max_entries;

    // Extract parameters from command structure
    aCmd.PVMFMP3FFParserNodeCommand::Parse(keylistptr, valuelistptr, starting_index, max_entries);
    if (iMP3File == NULL || keylistptr == NULL || valuelistptr == NULL)
    {
        // The list pointer is invalid, or we cannot access the mp3 ff library.
        return PVMFFailure;
    }
    // The underlying mp3 ff library will fill in the values.
    PVMFStatus status = iMP3File->GetMetadataValues(*keylistptr, *valuelistptr, starting_index, max_entries);

    iMP3ParserNodeMetadataValueCount = (*valuelistptr).size();

    if (iCPMContainer.iCPMMetaDataExtensionInterface != NULL)
    {
        iCPMGetMetaDataValuesCmdId =
            (iCPMContainer.iCPMMetaDataExtensionInterface)->GetNodeMetadataValues(iCPMContainer.iSessionId,
                    (*keylistptr),
                    (*valuelistptr),
                    0);
        return PVMFPending;
    }
    return status;
}


/**
 * Event reporting routines
 */
void PVMFMP3FFParserNode::SetState(TPVMFNodeInterfaceState s)
{
    LOGINFO((0, "PVMFMP3FFParserNode::SetState() %d", s));
    PVMFNodeInterface::SetState(s);
}

void PVMFMP3FFParserNode::ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOGINFO((0, "PVMFMP3FFParserNode::ReportErrorEvent() Type %d Data %d"
             , aEventType, aEventData));
    PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData);
}

void PVMFMP3FFParserNode::ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData, PVInterface*aExtMsg)
{
    LOGINFO((0, "PVMFMP3FFParserNode::ReportInfoEvent() Type %d Data %d"
             , aEventType, aEventData));
    PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData, aExtMsg);
}
////////////////////////////////////////////////////////////////////////
/**
 * Port Processing routines
 */
////////////////////////////////////////////////////////////////////////

/**
 * Port Activity Handler
 */
void PVMFMP3FFParserNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::PortActivity: port=0x%x, type=%d",
                     aActivity.iPort, aActivity.iType));

    // A port has reported some activity or state change.
    // Find out whether processing event needs to be queued
    // and/or node event needs to be reported to the observer.
    switch (aActivity.iType)
    {
        case PVMF_PORT_ACTIVITY_CREATED:
            //Report port created info event to the node.
            PVMFNodeInterface::ReportInfoEvent(PVMFInfoPortCreated,
                                               (OsclAny*)aActivity.iPort);
            break;
        case PVMF_PORT_ACTIVITY_DELETED:
            //Report port deleted info event to the node.
            PVMFNodeInterface::ReportInfoEvent(PVMFInfoPortDeleted,
                                               (OsclAny*)aActivity.iPort);
            break;
        case PVMF_PORT_ACTIVITY_CONNECT:
        case PVMF_PORT_ACTIVITY_DISCONNECT:
        case PVMF_PORT_ACTIVITY_INCOMING_MSG:
            //nothing needed.
            break;
        case PVMF_PORT_ACTIVITY_OUTGOING_MSG:
            //An outgoing message was queued on this port.
            if (!aActivity.iPort->IsConnectedPortBusy())
                RunIfNotReady();
            break;
        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_BUSY:
            //No action is needed here, the node checks for
            //outgoing queue busy as needed during data processing.
            break;
        case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
            //Outgoing queue was previously busy, but is now ready.
            HandleOutgoingQueueReady(aActivity.iPort);
            break;
        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_BUSY:
            // The connected port is busy
            // No action is needed here, the port processing code
            // checks for connected port busy during data processing.
            break;
        case PVMF_PORT_ACTIVITY_CONNECTED_PORT_READY:
            // The connected port has transitioned from Busy to Ready.
            // It's time to start processing outgoing messages again.
            if (aActivity.iPort->OutgoingMsgQueueSize() > 0)
            {
                RunIfNotReady();
            }
            break;
        default:
            break;
    }
}

/**
 * Outgoing message handler
 */
PVMFStatus PVMFMP3FFParserNode::ProcessOutgoingMsg(PVMFPortInterface* aPort)
{
    // Called by the AO to process one message off the outgoing
    // message queue for the given port.  This routine will
    // try to send the data to the connected port.

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::ProcessOutgoingMsg: aPort=0x%x",
                     aPort));

    PVMFStatus status = aPort->Send();
    if (status == PVMFErrBusy)
    {
        // Port was busy
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFMP3FFParserNode::ProcessOutgoingMsg: \
						Connected port goes into busy state"));
    }
    return status;
}

////////////////////////////////////////////////////////////////////////
/**
 * Active object implementation
 */
////////////////////////////////////////////////////////////////////////

/**
 * AO's entry point
 */
void PVMFMP3FFParserNode::Run()
{
    if (iCheckForMP3HeaderDuringInit)
    {
        // Read capacity notification was delivered
        iCheckForMP3HeaderDuringInit = false;

        PVMFStatus cmdStatus = CheckForMP3HeaderAvailability();
        if (cmdStatus == PVMFSuccess)
        {
            LOGINFO((0, "PVMFMP3FFParserNode::Run() CheckForMP3HeaderAvailability() succeeded"));
            // complete init command
            CompleteInit(cmdStatus);
        }
        else
        {
            LOGINFO((0, "PVMFMP3FFParserNode::Run() CheckForMP3HeaderAvailability() failed %d", cmdStatus));
        }
        return;
    }

    // Check InputCommandQueue, If command present
    // process commands
    if (!iInputCommands.empty())
    {
        ProcessCommand();
    }
    //Issue commands to the sub-nodes.
    if (!iCPMContainer.CmdPending() && !iSubNodeCmdVec.empty())
    {
        PVMFStatus status = iSubNodeCmdVec.front().iSubNodeContainer->IssueCommand(iSubNodeCmdVec.front().iCmd);
        if (status != PVMFPending)
        {
            iSubNodeCmdVec.front().iSubNodeContainer->CommandDone(status, NULL, NULL);
        }
    }

    if ((iPortVector.empty()))
        return;

    PVMFPortInterface*port = iPortVector.front();
    // Send outgoing messages
    if ((iInterfaceState == EPVMFNodeStarted || FlushPending()) &&
            port &&
            port->OutgoingMsgQueueSize() > 0 &&
            !port->IsConnectedPortBusy())
    {
        ProcessOutgoingMsg(port);
        // Re-schedule if there is additional data to send.
        if (port->OutgoingMsgQueueSize() > 0 && !port->IsConnectedPortBusy())
        {
            RunIfNotReady();
        }
    }

    // Create new data and send to the output queue
    if (iInterfaceState == EPVMFNodeStarted && !FlushPending())
    {
        if (HandleTrackState())  // Handle track state returns true if there is more data to be sent
        {
            // Re-schedule if there is more data to send out
            RunIfNotReady();
        }
    }

    // If we get here we did not process any ports or commands.
    // Check for completion of a flush command
    if (FlushPending() &&
            (!port || port->OutgoingMsgQueueSize() == 0))
    {
        //resume port input so the ports can be re-started.
        port->ResumeInput();
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess, NULL, NULL);
    }
}

void PVMFMP3FFParserNode::PassDatastreamFactory(PVMFDataStreamFactory& aFactory,
        int32 aFactoryTag,
        const PvmfMimeString* aFactoryConfig)
{
    OSCL_UNUSED_ARG(aFactoryTag);
    OSCL_UNUSED_ARG(aFactoryConfig);

    if (iDataStreamFactory == NULL)
    {
        iDataStreamFactory = &aFactory;
        PVUuid uuid = PVMIDataStreamSyncInterfaceUuid;
        PVInterface* iFace =
            iDataStreamFactory->CreatePVMFCPMPluginAccessInterface(uuid);
        if (iFace != NULL)
        {
            iDataStreamInterface = OSCL_STATIC_CAST(PVMIDataStreamSyncInterface*, iFace);
            iDataStreamInterface->OpenSession(iDataStreamSessionID, PVDS_READ_ONLY);
        }
    }
    else
    {
        OSCL_ASSERT(false);
    }
}

void
PVMFMP3FFParserNode::PassDatastreamReadCapacityObserver(PVMFDataStreamReadCapacityObserver* aObserver)
{
    iDataStreamReadCapacityObserver = aObserver;
}

int32 PVMFMP3FFParserNode::convertSizeToTime(uint32 aFileSize, uint32& aNPTInMS)
{
    OSCL_UNUSED_ARG(aFileSize);
    OSCL_UNUSED_ARG(aNPTInMS);
    return -1;
}

void PVMFMP3FFParserNode::setFileSize(const uint32 aFileSize)
{
    iFileSize = aFileSize;
    iFileSizeRecvd = true;
}

void PVMFMP3FFParserNode::setDownloadProgressInterface(PVMFDownloadProgressInterface* download_progress)
{
    if (iDownloadProgressInterface)
    {
        iDownloadProgressInterface->removeRef();
    }
    iDownloadProgressInterface = download_progress;
    // get the download clock
    iDownloadProgressClock = iDownloadProgressInterface->getDownloadProgressClock();
}

void PVMFMP3FFParserNode::playResumeNotification(bool aDownloadComplete)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::playResumeNotification() In"));

    // DownloadProgressInterface signalled for resuming the playback
    iDownloadComplete = aDownloadComplete;
    if (aDownloadComplete)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::playResumeNotification Unbinding download clock"));
        iDownloadProgressClock.Unbind();
    }

    // Resume playback
    if (iAutoPaused)
    {
        iAutoPaused = false;
        switch (iTrack.iState)
        {
            case PVMP3FFNodeTrackPortInfo::TRACKSTATE_DOWNLOAD_AUTOPAUSE:
                iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
                break;
            default:
                break;
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::playResumeNotification() Sending PVMFInfoDataReady event"));
        // Re-schedule AO
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////
/**
 * Private section
 */
////////////////////////////////////////////////////////////////////////

/**
 * Handle Node's track state
 */
bool PVMFMP3FFParserNode::HandleTrackState()
{
    // Flag to be active again or not
    bool ret_status = false;

    switch (iTrack.iState)
    {
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_UNINITIALIZED:
            // Node doesnt need to do any format specific initialization
            // just skip this step and set the state to GetData
            iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
            // Continue on to retrieve and send the first frame
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA:
            // Check if node needs to send BOS
            if (iTrack.iSendBOS)
            {
                // Send BOS downstream on all available tracks
                if (!SendBeginOfMediaStreamCommand(iTrack))
                {
                    return true;
                }
            }
            // First, grab some data from the core mp3 ff parser library
            if (!RetrieveTrackData(iTrack))
            {
                // If it returns false, break out of the switch and return false,
                // this means node doesn't need to be run again in the current state.
                if (iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK)
                {
                    RunIfNotReady();
                }
                break;
            }
            iSendDecodeFormatSpecificInfo = true;
            iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_SENDDATA;
            // Continue to send data
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_SENDDATA:
            if (SendTrackData(iTrack))
            {
                iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
                ret_status = true;
            }
            break;
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK:
            // Check if node needs to send BOS
            if (iTrack.iSendBOS)
            {
                // Send BOS downstream on all available tracks
                if (!SendBeginOfMediaStreamCommand(iTrack))
                {
                    return true;
                }
            }
            if (SendEndOfTrackCommand(iTrack))
            {
                // EOS command sent successfully
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVMFMP3FFParserNode::HandleTrackState() EOS media command sent successfully"));
                iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_ENDOFTRACK;
                ReportInfoEvent(PVMFInfoEndOfData);
            }
            else
            {
                // EOS command sending failed -- wait on outgoing queue ready notice
                // before trying again.
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVMFMP3FFParserNode::HandleTrackState() EOS media command sending failed"));
                return true;
            }
            break;
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRACKDATAPOOLEMPTY:
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_MEDIADATAPOOLEMPTY:
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_INITIALIZED:
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_ENDOFTRACK:
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_DESTFULL:
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_SOURCEEMPTY:
        case PVMP3FFNodeTrackPortInfo::TRACKSTATE_ERROR:
        default:
            break;
    }
    return ret_status;
}

/**
 * Retrieve Data from Mp3FF
 */
bool PVMFMP3FFParserNode::RetrieveTrackData(PVMP3FFNodeTrackPortInfo& aTrackPortInfo)
{
    // Parsing is successful, we must have estimated the duration by now.
    // Pass the duration to download progress interface
    if (iDownloadProgressInterface && iFileSizeRecvd && iFileSize > 0)
    {
        iMP3File->SetFileSize(iFileSize);
        uint32 durationInMsec = iMP3File->GetDuration();
        if (durationInMsec > 0)
        {
            iDownloadProgressInterface->setClipDuration(durationInMsec);
            int32 leavecode = 0;
            PVMFDurationInfoMessage* eventMsg = NULL;
            OSCL_TRY(leavecode, eventMsg = OSCL_NEW(PVMFDurationInfoMessage, (durationInMsec)));
            ReportInfoEvent(PVMFInfoDurationAvailable, NULL, OSCL_STATIC_CAST(PVInterface*, eventMsg));
            if (eventMsg)
            {
                eventMsg->removeRef();
            }
            iFileSize = 0;
        }
    }

    if (iAutoPaused == true)
    {
        aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_DOWNLOAD_AUTOPAUSE;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFMP3FFParserNode::RetrieveTrackData() Node in Auto pause"));
        return false;
    }

    //Maximum number of mp3 frames to be read at a time
    uint32 numsamples = PVMF3FF_DEFAULT_NUM_OF_FRAMES;
    // Create new media data buffer from pool
    int errcode = 0;
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImplOut;
    // Try block start
    OSCL_TRY(errcode,
             mediaDataImplOut = aTrackPortInfo.iMediaDataImplAlloc->allocate(numsamples * iMaxFrameSize)
            );
    // Try block end
    if (errcode != 0)
    {
        // There was an error while allocating MediaDataImpl
        if (errcode == OsclErrNoResources)
        {
            aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRACKDATAPOOLEMPTY;
            aTrackPortInfo.iTrackDataMemoryPool->notifyfreeblockavailable(*this, numsamples*iMaxFrameSize);	// Enable flag to receive event when next deallocate() is called on pool
        }
        else if (errcode == OsclErrNoMemory)
        {
            // Memory allocation for the pool failed
            aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_ERROR;
            ReportErrorEvent(PVMFErrNoMemory, NULL);
        }
        else if (errcode == OsclErrArgument)
        {
            // Invalid parameters passed to mempool
            aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_ERROR;
            ReportErrorEvent(PVMFErrArgument, NULL);
        }
        else
        {
            // General error
            aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_ERROR;
            ReportErrorEvent(PVMFFailure, NULL);
        }
        // All above conditions were Error conditions
        return false;
    } //end if

    // Reset error code for further processing
    errcode = 0;
    PVMFSharedMediaDataPtr mediadataout;
    // Try block start
    OSCL_TRY(errcode,
             mediadataout = PVMFMediaData::createMediaData(mediaDataImplOut,
                            aTrackPortInfo.iMediaDataMemPool)
            );
    // Try block end
    // Catch block start
    OSCL_FIRST_CATCH_ANY(errcode,
                         aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_MEDIADATAPOOLEMPTY;
                         aTrackPortInfo.iMediaDataMemPool->notifyfreechunkavailable(*this);
                         return false);
    // Catch block end

    // Retrieve memory fragment to write to
    OsclRefCounterMemFrag refCtrMemFragOut;
    OsclMemoryFragment memFragOut;
    mediadataout->getMediaFragment(0, refCtrMemFragOut);
    memFragOut.ptr = refCtrMemFragOut.getMemFrag().ptr;

    // Set up the GAU structure
    GAU gau;
    gau.numMediaSamples = numsamples;
    gau.buf.num_fragments = 1;
    gau.buf.buf_states[0] = NULL;
    gau.buf.fragments[0].ptr = refCtrMemFragOut.getMemFrag().ptr;
    gau.buf.fragments[0].len = refCtrMemFragOut.getCapacity();

    // Mp3FF ErrorCode
    MP3ErrorType error = MP3_SUCCESS;
    // Grab data from the mp3 ff parser library
    int32 retval = iMP3File->GetNextBundledAccessUnits(&numsamples, &gau, error);

    // Determine actual size of the retrieved data by summing each sample length in GAU
    uint32 actualdatasize = 0;
    for (uint32 index = 0; index < numsamples; ++index)
    {
        actualdatasize += gau.info[index].len;
    }

    if (retval > 0)
    {
        // Set buffer size
        mediadataout->setMediaFragFilledLen(0, actualdatasize);
        // Return the unused space from mempool back
        if (refCtrMemFragOut.getCapacity() > actualdatasize)
        {
            // Need to go to the resizable memory pool and free some memory
            aTrackPortInfo.iTrackDataMemoryPoolProxy->trim(refCtrMemFragOut.getCapacity() - actualdatasize);
            mediaDataImplOut->setCapacity(actualdatasize);
        }

        // Set format specific info for the first frame
        if (iSendDecodeFormatSpecificInfo)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMFMP3FFParserNode::RetrieveTrackData() Send channel sample info"));
            iSendDecodeFormatSpecificInfo = false;
            mediadataout->setFormatSpecificInfo(iDecodeFormatSpecificInfo);
        }
        // Save the media data in the trackport info
        aTrackPortInfo.iMediaData = mediadataout;

        // Retrieve timestamp and convert to milliseconds
        aTrackPortInfo.iClockConverter->update_clock(gau.info[0].ts);
        uint32 timestamp = aTrackPortInfo.iClockConverter->get_converted_ts(COMMON_PLAYBACK_CLOCK_TIMESCALE);
        timestamp += aTrackPortInfo.timestamp_offset;

        // Set the media data timestamp
        aTrackPortInfo.iMediaData->setTimestamp(timestamp);

        // Set msg sequence number and stream id
        aTrackPortInfo.iMediaData->setSeqNum(aTrackPortInfo.iSeqNum++);
        aTrackPortInfo.iMediaData->setStreamID(iStreamID);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFMP3FFParserNode::RetrieveTrackData Seq=%d, TS=%d", aTrackPortInfo.iSeqNum, timestamp));
        if (error == MP3_INSUFFICIENT_DATA && !iDownloadProgressInterface)
        {
            //parser reported underflow during local playback session
            if (!SendTrackData(iTrack))
            {
                // SendTrackData un-successful
                iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_SENDDATA;
                return false;
            }
        }
    }
    // EOS may occur even if some data was read
    // Also handles the condition if somehow error was not set even
    // if there is no data to read.
    if ((retval <= 0) || (MP3_SUCCESS != error))
    {
        // ffparser was not able to read data
        if (error == MP3_INSUFFICIENT_DATA) // mp3ff return insufficient data
        {
            // Check if DPI is present
            if (iDownloadProgressInterface != NULL)
            {
                if (iDownloadComplete)
                {
                    aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK;
                    return false;
                }
                // ffparser ran out of data, need to request a call back from DPI
                // when data beyond current timestamp is downloaded
                uint32 timeStamp = iMP3File->GetTimestampForCurrentSample();
                iDownloadProgressInterface->requestResumeNotification(timeStamp, iDownloadComplete);
                // Change track state to Autopause and set iAutopause
                aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_DOWNLOAD_AUTOPAUSE;
                iAutoPaused = true;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFMP3FFParserNode::RetrieveTrackData() \
								Auto pause Triggered"));
                return false;
            }
            else
            {
                // if we recieve Insufficient data for local playback from parser library that means
                // its end of track, so change track state to send end of track.
                aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK;
                return false;
            }
        }
        else if (error == MP3_END_OF_FILE) // mp3ff return EOF
        {
            aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK;
            return false;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE,
                            (0, "PVMFMP3FFParserNode::RetrieveTrackData() \
							Unknown error reported by mp3ff %d", error));
            ReportErrorEvent(PVMFErrPortProcessing, (OsclAny*)(aTrackPortInfo.iPort));
            aTrackPortInfo.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK;
            return false;
        }
    }
    return true;
}

/**
 * Send track data to output port
 */
bool PVMFMP3FFParserNode::SendTrackData(PVMP3FFNodeTrackPortInfo& aTrackPortInfo)
{
    // Send frame to downstream node via port
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                    (0, "PVMFMP3FFParserNode::SendTrackData: SeqNum %d", \
                     aTrackPortInfo.iMediaData->getSeqNum()));

    PVMFSharedMediaMsgPtr mediaMsgOut;
    // Convert media data to media message
    convertToPVMFMediaMsg(mediaMsgOut, aTrackPortInfo.iMediaData);
    // Queue media msg to port's outgoing queue
    PVMFStatus status = aTrackPortInfo.iPort->QueueOutgoingMsg(mediaMsgOut);
    if (status != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFMP3FFParserNode::SendTrackData: Outgoing queue busy"));
        return false;
    }

    //keep count of the number of source frames generated on this port
    aTrackPortInfo.iPort->iNumFramesGenerated++;
    // Don't need reference to iMediaData so unbind it
    aTrackPortInfo.iMediaData.Unbind();
    return true;
}

/**
 * Send BOS command to output port
 */
bool PVMFMP3FFParserNode::SendBeginOfMediaStreamCommand(PVMP3FFNodeTrackPortInfo& aTrackPortInfoPtr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::SendBeginOfMediaStreamCommand"));

    // Create media command
    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();
    // Set command id to BOS
    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_BOS_FORMAT_ID);
    // Set timestamp of the media command
    sharedMediaCmdPtr->setTimestamp(aTrackPortInfoPtr.timestamp_offset);
    // SeqNum of BOS doesnt play any role, set to 0
    sharedMediaCmdPtr->setSeqNum(0);
    // Convert media command to media message
    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);
    // Set stream id
    mediaMsgOut->setStreamID(iStreamID);
    // Queue media msg to output pout
    if (aTrackPortInfoPtr.iPort->QueueOutgoingMsg(mediaMsgOut) != PVMFSuccess)
    {
        // Output queue is busy, so wait for the output queue being ready
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFMP3FFParserNode::SendBeginOfMediaStreamCommand: \
						Outgoing queue busy. "));
        return false;
    }
    // BOS was sent successfully, reset the BOS flag
    aTrackPortInfoPtr.iSendBOS = false;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                    (0, "PVMFMP3FFParserNode::SendBeginOfMediaStreamCommand: \
					BOS Sent StreamID %d", iStreamID));
    return true;
}

/**
 * Send EOS command to output port
 */
bool PVMFMP3FFParserNode::SendEndOfTrackCommand(PVMP3FFNodeTrackPortInfo& aTrackPortInfo)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::SendEndOfTrackCommand() In"));
    // Create media command
    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();
    // Set command id to EOS
    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);
    // Retrieve timestamp and convert to milliseconds
    uint32 timestamp = aTrackPortInfo.iClockConverter->get_converted_ts(COMMON_PLAYBACK_CLOCK_TIMESCALE);
    timestamp += aTrackPortInfo.timestamp_offset;
    // Set the timestamp
    sharedMediaCmdPtr->setTimestamp(timestamp);
    // Set the sequence number
    sharedMediaCmdPtr->setSeqNum(aTrackPortInfo.iSeqNum++);
    //set stream id
    sharedMediaCmdPtr->setStreamID(iStreamID);
    // Convert media command to media message
    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);

    // Queue media msg to output pout
    if (aTrackPortInfo.iPort->QueueOutgoingMsg(mediaMsgOut) != PVMFSuccess)
    {
        // Output queue is busy, so wait for the output queue being ready
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "PVMFMP3FFParserNode::SendEndOfTrackCommand: Outgoing queue busy. "));
        return false;
    }
    // EOS was sent successfully
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::SendEndOfTrackCommand() Out"));
    return true;
}


/**
 * Outgoing port queue handler
 */
bool PVMFMP3FFParserNode::HandleOutgoingQueueReady(PVMFPortInterface* aPortInterface)
{
    if (iTrack.iPort == aPortInterface &&
            iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_SENDDATA)
    {
        // Found the element and right state
        // re-send the data
        if (!SendTrackData(iTrack))
        {
            // SendTrackData un-successful
            return false;
        }

        // Success in re-sending the data, change state to getdata
        iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
        return true;
    }
    // Either the track was not in correct state or the port was not correct
    return false;
}

/**
 * Parse the File
 */
PVMFStatus PVMFMP3FFParserNode::ParseFile()
{
    if (!iSourceURLSet)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFMP3FFParserNode::ParseFile() SourceURL not set"));
        // Can't init the node if the node, File name is not specified yet.
        return PVMFFailure;
    }

    MP3ErrorType mp3Err = iMP3File->ParseMp3File();

    if (mp3Err == MP3_INSUFFICIENT_DATA)
    {
        return PVMFPending;
    }
    else if (mp3Err == MP3_END_OF_FILE ||
             mp3Err != MP3_SUCCESS)
    {
        SetState(EPVMFNodeError);
        ReportErrorEvent(PVMFErrResource);
        return mp3Err;
    }

    // Find out what the largest frame in the file is. This information is used
    // when allocating the buffers in DoRequestPort().
    iMaxFrameSize = iMP3File->GetMaxBufferSizeDB();
    if (iMaxFrameSize <= 0)
    {
        iMaxFrameSize = PVMP3FF_DEFAULT_MAX_FRAMESIZE;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO,
                        (0, "PVMFMP3FFParserNode::ParseFile() Mp3FF \
						MaxFrameSize %d", iMaxFrameSize));
    }

    // get config Details from mp3ff
    MP3ContentFormatType mp3format;
    iConfigOk = iMP3File->GetConfigDetails(mp3format);
    if (!iConfigOk)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO,
                        (0, "PVMFMP3FFParserNode::ParseFile() Mp3FF \
						Config Not returned", iMaxFrameSize));

    }
    else
    {
        iMP3FormatBitrate = mp3format.Bitrate;
    }
    return PVMFSuccess;
}

/**
 * Reset the trackinfo
 */
void PVMFMP3FFParserNode::ResetTrack()
{
    iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_UNINITIALIZED;
    iTrack.iMediaData.Unbind();
    iTrack.iSeqNum = 0;
    iTrack.timestamp_offset = 0;
    iTrack.iSendBOS = false;
    iAutoPaused = false;
}

/**
 * Release the trackinfo and do necessary cleanup
 */
void PVMFMP3FFParserNode::ReleaseTrack()
{
    iTrack.iMediaData.Unbind();

    iTrack.iPort = NULL;

    if (iTrack.iTrackDataMemoryPool != NULL)
    {
        iTrack.iTrackDataMemoryPool->removeRef();
        iTrack.iTrackDataMemoryPool = NULL;
    }

    if (iTrack.iTrackDataMemoryPoolProxy != NULL)
    {
        OSCL_DELETE(iTrack.iTrackDataMemoryPoolProxy);
        iTrack.iTrackDataMemoryPoolProxy = NULL;
    }

    if (iTrack.iMediaDataImplAlloc != NULL)
    {
        OSCL_DELETE(iTrack.iMediaDataImplAlloc);
        iTrack.iMediaDataImplAlloc = NULL;
    }

    if (iTrack.iMediaDataMemPool != NULL)
    {
        OSCL_DELETE(iTrack.iMediaDataMemPool);
        iTrack.iMediaDataMemPool = NULL;
    }

    if (iTrack.iClockConverter != NULL)
    {
        OSCL_DELETE(iTrack.iClockConverter);
        iTrack.iClockConverter = NULL;
    }

    return;
}

/**
 * Cleanup all file sources
 */
void PVMFMP3FFParserNode::CleanupFileSource()
{
    if (iDurationCalcAO && iDurationCalcAO->IsBusy())
    {
        iDurationCalcAO->Cancel();
    }
    if (iMP3File)
    {
        OSCL_DELETE(iMP3File);
        iMP3File = NULL;
    }

    if (iDataStreamInterface != NULL)
    {
        PVInterface* iFace = OSCL_STATIC_CAST(PVInterface*, iDataStreamInterface);
        PVUuid uuid = PVMIDataStreamSyncInterfaceUuid;
        iDataStreamFactory->DestroyPVMFCPMPluginAccessInterface(uuid, iFace);
        iDataStreamInterface = NULL;
    }
    if (iDataStreamFactory != NULL)
    {
        iDataStreamFactory->removeRef();
        iDataStreamFactory = NULL;
    }
    iMP3ParserNodeMetadataValueCount = 0;

    iUseCPMPluginRegistry = NULL;
    iCPMSourceData.iFileHandle = NULL;

    if (iFileHandle)
    {
        OSCL_DELETE(iFileHandle);
        iFileHandle = NULL;
    }
    iSourceURLSet = false;
    oWaitingOnLicense = false;
    iDownloadComplete = false;
}

/**
 * From OsclMemPoolFixedChunkAllocatorObserver
 * Call back is received when free mem-chunk is available
 */
void PVMFMP3FFParserNode::freechunkavailable(OsclAny*)
{
    if (iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_MEDIADATAPOOLEMPTY)
    {
        iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
}

/**
 * From OsclMemPoolResizableAllocatorObserver
 * Call back is received when free mem-block is available
 */
void PVMFMP3FFParserNode::freeblockavailable(OsclAny*)
{
    if (iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRACKDATAPOOLEMPTY)
    {
        iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
}


////////////////////////////////////////////////////////////////////////
/**
 * Extension interface implementation
 */
////////////////////////////////////////////////////////////////////////

void PVMFMP3FFParserNode::addRef()
{
    ++iExtensionRefCount;
}

void PVMFMP3FFParserNode::removeRef()
{
    --iExtensionRefCount;
}

bool PVMFMP3FFParserNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::queryInterface() In"));

    if (uuid == PVMF_TRACK_SELECTION_INTERFACE_UUID)
    {
        PVMFTrackSelectionExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMFTrackSelectionExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (uuid == PVMF_DATA_SOURCE_INIT_INTERFACE_UUID)
    {
        PVMFDataSourceInitializationExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMFDataSourceInitializationExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (uuid == KPVMFMetadataExtensionUuid)
    {
        PVMFMetadataExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMFMetadataExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (PvmfDataSourcePlaybackControlUuid == uuid)
    {
        PvmfDataSourcePlaybackControlInterface* myInterface = OSCL_STATIC_CAST(PvmfDataSourcePlaybackControlInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (uuid == PVMFCPMPluginLicenseInterfaceUuid)
    {
        PVMFCPMPluginLicenseInterface* myInterface = OSCL_STATIC_CAST(PVMFCPMPluginLicenseInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (PVMF_FF_PROGDOWNLOAD_SUPPORT_INTERFACE_UUID == uuid)
    {
        PVMFFormatProgDownloadSupportInterface* myInterface = OSCL_STATIC_CAST(PVMFFormatProgDownloadSupportInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (PVMIDatastreamuserInterfaceUuid == uuid)
    {
        PVMIDatastreamuserInterface* myInterface = OSCL_STATIC_CAST(PVMIDatastreamuserInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else
    {
        return false;
    }

    ++iExtensionRefCount;
    return true;
}


/**
 * From PVMFDataSourceInitializationExtensionInterface
 */
PVMFStatus PVMFMP3FFParserNode::SetSourceInitializationData(OSCL_wString& aSourceURL,
        PVMFFormatType& aSourceFormat,
        OsclAny* aSourceData)
{
    // Initialize the Source data
    if (iSourceURLSet)
    {
        CleanupFileSource();
    }

    if (aSourceFormat != PVMF_MP3FF)
    {
        // Node doesnt support any other format than MP3
        return PVMFFailure;
    }

    iSourceFormat = aSourceFormat;
    iSourceURL = aSourceURL;
    iSourceURLSet = true;
    if (aSourceData)
    {
        PVInterface* pvInterface = OSCL_STATIC_CAST(PVInterface*, aSourceData);
        PVInterface* localDataSrc = NULL;
        PVUuid localDataSrcUuid(PVMF_LOCAL_DATASOURCE_UUID);
        // Check if it is a local file
        if (pvInterface->queryInterface(localDataSrcUuid, localDataSrc))
        {
            PVMFLocalDataSource* opaqueData = OSCL_STATIC_CAST(PVMFLocalDataSource*, localDataSrc);
            iUseCPMPluginRegistry = opaqueData->iUseCPMPluginRegistry;
            if (opaqueData->iFileHandle)
            {
                iFileHandle = OSCL_NEW(OsclFileHandle, (*(opaqueData->iFileHandle)));
                iCPMSourceData.iFileHandle = iFileHandle;
            }
            if (opaqueData->iContentAccessFactory != NULL)
            {
                if (iUseCPMPluginRegistry == false)
                {
                    // External download - Not supported
                    return PVMFErrNotSupported;
                }
                else
                {
                    //Cannot have both plugin usage and a datastream factory
                    return PVMFErrArgument;
                }
            }
        }
        else
        {
            PVInterface* sourceDataContext = NULL;
            PVInterface* commonDataContext = NULL;
            PVUuid sourceContextUuid(PVMF_SOURCE_CONTEXT_DATA_UUID);
            PVUuid commonContextUuid(PVMF_SOURCE_CONTEXT_DATA_COMMON_UUID);
            if (pvInterface->queryInterface(sourceContextUuid, sourceDataContext))
            {
                if (sourceDataContext->queryInterface(commonContextUuid, commonDataContext))
                {
                    PVMFSourceContextDataCommon* cContext = OSCL_STATIC_CAST(
                                                                PVMFSourceContextDataCommon*,
                                                                commonDataContext);
                    iUseCPMPluginRegistry = cContext->iUseCPMPluginRegistry;
                    if (cContext->iFileHandle)
                    {
                        iFileHandle = OSCL_NEW(OsclFileHandle, (*(cContext->iFileHandle)));
                    }
                    if (cContext->iContentAccessFactory != NULL)
                    {
                        if (iUseCPMPluginRegistry == false)
                        {
                            // External download - Not supported
                            return PVMFErrNotSupported;
                        }
                        else
                        {
                            //Cannot have both plugin usage and a datastream factory
                            return PVMFErrArgument;
                        }
                    }
                    PVMFSourceContextData* sContext = OSCL_STATIC_CAST(
                                                          PVMFSourceContextData*,
                                                          sourceDataContext);
                    iSourceContextData = *sContext;
                    iSourceContextDataValid = true;
                }
            }
        }
    }
    return PVMFSuccess;
}

PVMFStatus PVMFMP3FFParserNode::SetClientPlayBackClock(OsclClock* aClientClock)
{
    OSCL_UNUSED_ARG(aClientClock);
    return PVMFSuccess;
}

PVMFStatus PVMFMP3FFParserNode::SetEstimatedServerClock(OsclClock* aClientClock)
{
    OSCL_UNUSED_ARG(aClientClock);
    return PVMFSuccess;
}

/**
 * From PVMFTrackSelectionExtensionInterface
 */
PVMFStatus PVMFMP3FFParserNode::GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::GetMediaPresentationInfo() In"));
    if (!iMP3File)
    {
        return PVMFFailure;
    }
    aInfo.setDurationValue(iMP3File->GetDuration());

    int32 iNumTracks = iMP3File->GetNumTracks();
    if (iNumTracks <= 0)
    {
        // Number of tracks is null
        return PVMFFailure;
    }

    int32 id;
    for (id = 0; id < iNumTracks; id++)
    {
        PVMFTrackInfo tmpTrackInfo;
        // set the port tag for this track
        tmpTrackInfo.setPortTag(PVMF_MP3FFPARSER_NODE_PORT_TYPE_SOURCE);
        // track id
        tmpTrackInfo.setTrackID(0);
        // bitrate
        uint32 aBitRate = 0;
        if (iConfigOk)
        {
            aBitRate = iMP3FormatBitrate;
        }
        tmpTrackInfo.setTrackBitRate(aBitRate);
        // config info
        MP3ContentFormatType mp3Config;
        if (!iMP3File->GetConfigDetails(mp3Config))
        {
            // mp3 config not available
            return PVMFFailure;
        }

        if (!CreateFormatSpecificInfo(mp3Config.NumberOfChannels, mp3Config.SamplingRate))
        {
            return PVMFFailure;
        }

        tmpTrackInfo.setTrackConfigInfo(iDecodeFormatSpecificInfo);
        // timescale
        uint64 timescale = (uint64)iMP3File->GetTimescale();
        tmpTrackInfo.setTrackDurationTimeScale(timescale);
        // in movie timescale
        uint32 trackDuration = iMP3File->GetDuration();
        tmpTrackInfo.setTrackDurationValue(trackDuration);
        // mime type
        OSCL_FastString mime_type = _STRLIT_CHAR(PVMF_MIME_MP3);
        tmpTrackInfo.setTrackMimeType(mime_type);
        // add the track
        aInfo.addTrackInfo(tmpTrackInfo);
    }
    return PVMFSuccess;
}

bool PVMFMP3FFParserNode::CreateFormatSpecificInfo(uint32 numChannels, uint32 samplingRate)
{
    // Allocate memory for decode specific info and ref counter
    OsclMemoryFragment frag;
    frag.ptr = NULL;
    frag.len = sizeof(channelSampleInfo);
    uint refCounterSize = oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
    uint8* memBuffer = (uint8*)iDecodeFormatSpecificInfoAlloc.ALLOCATE(refCounterSize + frag.len);
    if (!memBuffer)
    {
        // failure while allocating memory buffer
        return false;
    }

    oscl_memset(memBuffer, 0, refCounterSize + frag.len);
    // Create ref counter
    OsclRefCounter* refCounter = new(memBuffer) OsclRefCounterDA(memBuffer,
            (OsclDestructDealloc*)&iDecodeFormatSpecificInfoAlloc);
    memBuffer += refCounterSize;
    // Create channel sample info
    frag.ptr = (OsclAny*)(new(memBuffer) channelSampleInfo);
    ((channelSampleInfo*)frag.ptr)->desiredChannels = numChannels;
    ((channelSampleInfo*)frag.ptr)->samplingRate = samplingRate;

    // Store info in a ref counter memfrag
    iDecodeFormatSpecificInfo = OsclRefCounterMemFrag(frag, refCounter,
                                sizeof(struct channelSampleInfo));
    return true;
}

PVMFStatus PVMFMP3FFParserNode::SelectTracks(PVMFMediaPresentationInfo& aInfo)
{
    OSCL_UNUSED_ARG(aInfo);
    return PVMFSuccess;
}

// From PVMFMetadataExtensionInterface
uint32 PVMFMP3FFParserNode::GetNumMetadataKeys(char* aQueryString)
{
    uint32 num_entries = 0;
    if (iMP3File)
    {
        num_entries = iMP3File->GetNumMetadataKeys(aQueryString);
    }

    if (iCPMContainer.iCPMMetaDataExtensionInterface != NULL)
    {
        num_entries +=
            (iCPMContainer.iCPMMetaDataExtensionInterface)->GetNumMetadataKeys(aQueryString);
    }
    return num_entries;
}

uint32 PVMFMP3FFParserNode::GetNumMetadataValues(PVMFMetadataList& aKeyList)
{
    uint32 numvalentries = 0;
    if (iMP3File)
    {
        numvalentries = iMP3File->GetNumMetadataValues(aKeyList);
    }
    if (iCPMContainer.iCPMMetaDataExtensionInterface != NULL)
    {
        numvalentries +=
            iCPMContainer.iCPMMetaDataExtensionInterface->GetNumMetadataValues(aKeyList);
    }
    return numvalentries;
}

/**
 * From PVMFMetadataExtensionInterface
 * Queue an asynchronous node command for GetNodeMetadataKeys
 */
PVMFCommandId PVMFMP3FFParserNode::GetNodeMetadataKeys(PVMFSessionId aSessionId,
        PVMFMetadataList& aKeyList,
        uint32 starting_index,
        int32 max_entries,
        char* query_key,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::GetNodeMetadataKeys() In"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_GETNODEMETADATAKEY,
            aKeyList, starting_index,
            max_entries, query_key,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * From PVMFMetadataExtensionInterface
 * Queue an asynchronous node command for GetNodeMetadataValues
 */
PVMFCommandId PVMFMP3FFParserNode::GetNodeMetadataValues(PVMFSessionId aSessionId,
        PVMFMetadataList& aKeyList,
        Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
        uint32 starting_index,
        int32 max_entries,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::GetNodeMetadataValues() In"));

    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_GETNODEMETADATAVALUE,
            aKeyList, aValueList,
            starting_index, max_entries,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * From PVMFMetadataExtensionInterface
 * Queue an asynchronous node command for ReleaseNodeMetadataKeys
 */
PVMFStatus PVMFMP3FFParserNode::ReleaseNodeMetadataKeys(PVMFMetadataList& aMetaDataKeys,
        uint32 , uint32)
{
    OSCL_UNUSED_ARG(aMetaDataKeys);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::ReleaseNodeMetadataKeys() In"));
    return PVMFSuccess;
}

/**
 * From PVMFMetadataExtensionInterface
 * Queue an asynchronous node command for ReleaseNodeMetadataValues
 */
PVMFStatus PVMFMP3FFParserNode::ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
        uint32 start, uint32 end)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::ReleaseNodeMetadataValues() In"));
    if (iMP3File == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFMP3FFParserNode::ReleaseNodeMetadataValues() \
					   MP3 file not parsed yet"));
        return PVMFFailure;
    }

    end = OSCL_MIN(aValueList.size(), iMP3ParserNodeMetadataValueCount);

    if (start > end || aValueList.size() == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFMP3FFParserNode::ReleaseNodeMetadataValues() \
						Invalid start/end index"));
        return PVMFErrArgument;
    }

    // Go through the specified values and free it
    for (uint32 i = start; i < end; i++)
    {
        iMP3File->ReleaseMetadataValue(aValueList[i]);
    }
    return PVMFSuccess;
}

/**
 * From PvmfDataSourcePlaybackControlInterface
 * Queue an asynchronous node command for SetDataSourcePosition
 */
PVMFCommandId PVMFMP3FFParserNode::SetDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aActualNPT,
        PVMFTimestamp& aActualMediaDataTS,
        bool aSeekToSyncPoint,
        uint32 aStreamID,
        OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::SetDataSourcePosition()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_SETDATASOURCEPOSITION,
            aTargetNPT, &aActualNPT,
            &aActualMediaDataTS, aSeekToSyncPoint,
            aStreamID, aContext);
    return QueueCommandL(cmd);
}

/**
 * From PvmfDataSourcePlaybackControlInterface
 * Queue an asynchronous node command for QueryDataSourcePosition
 */
PVMFCommandId PVMFMP3FFParserNode::QueryDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aActualNPT,
        bool aSeekToSyncPoint,
        OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::QueryDataSourcePosition()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_QUERYDATASOURCEPOSITION,
            aTargetNPT, &aActualNPT,
            aSeekToSyncPoint, aContext);
    return QueueCommandL(cmd);
}

/**
 * From PvmfDataSourcePlaybackControlInterface
 * Queue an asynchronous node command for QueryDataSourcePosition
 */
PVMFCommandId PVMFMP3FFParserNode::QueryDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aSeekPointBeforeTargetNPT,
        PVMFTimestamp& aSeekPointAfterTargetNPT,
        OsclAny* aContextData,
        bool aSeekToSyncPoint)
{
    OSCL_UNUSED_ARG(aSeekPointAfterTargetNPT);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::QueryDataSourcePosition()"));
    PVMFMP3FFParserNodeCommand cmd;
    // Construct call is not changed, the aSeekPointBeforeTargetNPT will
    // contain the replaced actualNPT
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_QUERYDATASOURCEPOSITION,
            aTargetNPT, &aSeekPointBeforeTargetNPT,
            aSeekToSyncPoint, aContextData);
    return QueueCommandL(cmd);
}

/**
 * From PvmfDataSourcePlaybackControlInterface
 * Queue an asynchronous node command for SetDataSourceRate
 */
PVMFCommandId PVMFMP3FFParserNode::SetDataSourceRate(PVMFSessionId aSessionId,
        int32 aRate,
        OsclTimebase* aTimebase,
        OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::SetDataSourcePosition()"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_SETDATASOURCERATE,
            aRate, aTimebase,
            aContext);
    return QueueCommandL(cmd);
}

/**
 * Command Handler for SetDataSourceRate
 */
PVMFStatus PVMFMP3FFParserNode::DoSetDataSourceRate(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFMP3FFParserNode::DoSetDataSourceRate() In"));
    // Retrieve the new rate
    int32 rate;
    OsclTimebase* timebase = NULL;
    aCmd.PVMFMP3FFParserNodeCommand::Parse(rate, timebase);
    // This source node does not throttle the rate, so no restrictions are imposed by the source
    return PVMFSuccess;

}

/**
 * Command Handler for SetDataSourcePosition
 */
PVMFStatus PVMFMP3FFParserNode::DoSetDataSourcePosition(PVMFMP3FFParserNodeCommand& aCmd)
{
    uint32 targetNPT = 0;
    uint32* actualNPT = NULL;
    uint32* actualMediaDataTS = NULL;
    bool seektosyncpoint = false;
    uint32 streamID = 0;

    // if progressive streaming, reset download complete flag
    if ((NULL != iDataStreamInterface) && (0 != iDataStreamInterface->QueryBufferingCapacity()))
    {
        iDownloadComplete = false;
    }

    aCmd.PVMFMP3FFParserNodeCommand::Parse(targetNPT, actualNPT,
                                           actualMediaDataTS, seektosyncpoint,
                                           streamID);

    bool retVal = false;
    // duplicate bos has been received, dont perform reposition at source node
    if (iStreamID == streamID) retVal = true;

    iStreamID = streamID;
    iTrack.iSendBOS = true;

    if (retVal)
    {
        RunIfNotReady();
        return PVMFSuccess;
    }

    if (iDownloadProgressClock.GetRep())
    {
        // Get the amount downloaded so far
        bool tmpbool = false;
        uint32 dltime = 0;
        iDownloadProgressClock->GetCurrentTime32(dltime, tmpbool, OSCLCLOCK_MSEC);
        // Check if the requested time is past the downloaded clip
        if (targetNPT >= dltime)
        {
            // For now, fail in this case. In future, we want to reposition to valid location.
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFMP3FFParserNode::DoSetDataSourcePosition() \
							Positioning past the amount downloaded so return as \
							argument error"));
            return PVMFErrArgument;
        }
    }
    // get the clock offset
    iTrack.iClockConverter->update_clock(iMP3File->GetTimestampForCurrentSample());
    iTrack.timestamp_offset += iTrack.iClockConverter->get_converted_ts(COMMON_PLAYBACK_CLOCK_TIMESCALE);
    // Set the timestamp
    *actualMediaDataTS = iTrack.timestamp_offset;
    // See if targetNPT is greater or equal to clip duration
    uint32 duration = iMP3File->GetDuration();
    if (duration > 0 && targetNPT >= duration)
    {
        // report End of Stream on the track and reset the track to zero.
        iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK;
        *actualNPT = iMP3File->SeekToTimestamp(0);
        iTrack.iClockConverter->set_clock_other_timescale(*actualMediaDataTS, COMMON_PLAYBACK_CLOCK_TIMESCALE);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFMP3FFParserNode::DoSetDataSourcePosition: targetNPT=%d, actualNPT=%d, actualMediaTS=%d",
                         targetNPT, *actualNPT, *actualMediaDataTS));
        return PVMFSuccess;
    }
    // Seek to the next NPT
    // MP3 FF seeks to the beginning if the requested time is past the end of clip
    *actualNPT = iMP3File->SeekToTimestamp(targetNPT);

    if (*actualNPT == duration)
    {
        // this means there was no data to render after the seek so just send End of Track
        iTrack.iClockConverter->set_clock_other_timescale(*actualMediaDataTS, COMMON_PLAYBACK_CLOCK_TIMESCALE);
        iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_SEND_ENDOFTRACK;
        return PVMFSuccess;
    }

    iTrack.iClockConverter->set_clock_other_timescale(*actualNPT, COMMON_PLAYBACK_CLOCK_TIMESCALE);
    iTrack.timestamp_offset -= *actualNPT;

    // Reposition has occured, so reset the track state
    if (iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_ENDOFTRACK ||
            iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_DOWNLOAD_AUTOPAUSE)
    {
        if (iAutoPaused)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFMP3FFParserNode::DoSetDataSourcePosition Track Autopaused"));
            iAutoPaused = false;
            if (iDownloadProgressInterface != NULL)
            {
                iDownloadProgressInterface->cancelResumeNotification();
            }
        }
        iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "PVMFMP3FFParserNode::DoSetDataSourcePosition: targetNPT=%d, actualNPT=%d, actualMediaTS=%d",
                     targetNPT, *actualNPT, *actualMediaDataTS));
    return PVMFSuccess;
}

/**
 * Command Handler for QueryDataSourcePosition
 */
PVMFStatus PVMFMP3FFParserNode::DoQueryDataSourcePosition(PVMFMP3FFParserNodeCommand& aCmd)
{
    uint32 targetNPT = 0;
    uint32* actualNPT = NULL;
    bool seektosyncpoint = false;

    aCmd.PVMFMP3FFParserNodeCommand::Parse(targetNPT, actualNPT, seektosyncpoint);
    if (actualNPT == NULL)
    {
        return PVMFErrArgument;
    }
    // First check if MP3 file is being PDed to make sure the requested
    // position is before amount downloaded
    if (iDownloadProgressClock.GetRep())
    {
        // Get the amount downloaded so far
        bool tmpbool = false;
        uint32 dltime = 0;
        iDownloadProgressClock->GetCurrentTime32(dltime, tmpbool, OSCLCLOCK_MSEC);
        // Check if the requested time is past clip dl
        if (targetNPT >= dltime)
        {
            return PVMFErrArgument;
        }
    }
    // Determine the actual NPT without actually repositioning
    // MP3 FF goes to the beginning if the requested time is past the end of clip
    *actualNPT = targetNPT;
    iMP3File->SeekPointFromTimestamp(*actualNPT);
    return PVMFSuccess;
}

/**
 * Queues SubNode Commands
 */
void PVMFMP3FFParserNode::Push(PVMFSubNodeContainerBaseMp3& c, PVMFSubNodeContainerBaseMp3::CmdType cmd)
{
    SubNodeCmd snc;
    snc.iSubNodeContainer = &c;
    snc.iCmd = cmd;
    iSubNodeCmdVec.push_back(snc);
}


PVMFCommandId
PVMFMP3FFParserNode::GetLicense(PVMFSessionId aSessionId,
                                OSCL_wString& aContentName,
                                OsclAny* aData,
                                uint32 aDataSize,
                                int32 aTimeoutMsec,
                                OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::GetLicense - Wide called"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_GET_LICENSE_W,
            aContentName,
            aData,
            aDataSize,
            aTimeoutMsec,
            aContextData);
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFMP3FFParserNode::GetLicense(PVMFSessionId aSessionId,
                                OSCL_String&  aContentName,
                                OsclAny* aData,
                                uint32 aDataSize,
                                int32 aTimeoutMsec,
                                OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::GetLicense - Non-Wide called"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommand::Construct(aSessionId,
            PVMP3FF_NODE_CMD_GET_LICENSE,
            aContentName,
            aData,
            aDataSize,
            aTimeoutMsec,
            aContextData);
    return QueueCommandL(cmd);
}



PVMFStatus PVMFMP3FFParserNode::DoGetLicense(PVMFMP3FFParserNodeCommand& aCmd,
        bool aWideCharVersion)
{
    OSCL_UNUSED_ARG(aCmd);
    if (iCPMContainer.iCPMLicenseInterface == NULL)
    {
        return PVMFErrNotSupported;
    }

    if (aWideCharVersion == true)
    {
        Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMGetLicenseW);
    }
    else
    {
        Push(iCPMContainer, PVMFSubNodeContainerBaseMp3::ECPMGetLicense);
    }
    RunIfNotReady();
    return PVMFPending;
}

void PVMFMP3FFParserNode::CompleteGetLicense()
{
    CommandComplete(iCurrentCommand,
                    iCurrentCommand.front(),
                    PVMFSuccess, NULL, NULL);
}

PVMFCommandId
PVMFMP3FFParserNode::CancelGetLicense(PVMFSessionId aSessionId, PVMFCommandId aCmdId, OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::CancelGetLicense - called"));
    PVMFMP3FFParserNodeCommand cmd;
    cmd.PVMFMP3FFParserNodeCommandBase::Construct(aSessionId, PVMP3FF_NODE_CMD_CANCEL_GET_LICENSE, aCmdId, aContextData);
    return QueueCommandL(cmd);
}

PVMFStatus PVMFMP3FFParserNode::DoCancelGetLicense(PVMFMP3FFParserNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFMP3FFParserNode::DoCancelGetLicense() Called"));
    PVMFStatus status = PVMFErrArgument;

    if (iCPMContainer.iCPMLicenseInterface == NULL)
    {
        status = PVMFErrNotSupported;
    }
    else
    {
        /* extract the command ID from the parameters.*/
        PVMFCommandId id;
        aCmd.PVMFMP3FFParserNodeCommandBase::Parse(id);

        /* first check "current" command if any */
        PVMFMP3FFParserNodeCommand* cmd = iCurrentCommand.FindById(id);
        if (cmd)
        {
            if (cmd->iCmd == PVMP3FF_NODE_CMD_GET_LICENSE_W || cmd->iCmd == PVMP3FF_NODE_CMD_GET_LICENSE)
            {
                if (iCPMContainer.CancelPendingCommand())
                {
                    return PVMFPending;//wait on sub-node cancel to complete.
                }
                CommandComplete(iCurrentCommand, *cmd, PVMFErrCancelled, NULL, NULL);
                return PVMFSuccess;
            }
        }

        /*
         * next check input queue.
         * start at element 1 since this cancel command is element 0.
         */
        cmd = iInputCommands.FindById(id, 1);
        if (cmd)
        {
            if (cmd->iCmd == PVMP3FF_NODE_CMD_GET_LICENSE_W || cmd->iCmd == PVMP3FF_NODE_CMD_GET_LICENSE)
            {
                /* cancel the queued command */
                CommandComplete(iInputCommands, *cmd, PVMFErrCancelled, NULL, NULL);
                /* report cancel success */
                return PVMFSuccess;
            }
        }
    }
    /* if we get here the command isn't queued so the cancel fails */
    return status;
}

PVMFCommandId PVMFCPMContainerMp3::GetCPMLicenseInterface()
{
    return (iCPM->QueryInterface(iSessionId,
                                 PVMFCPMPluginLicenseInterfaceUuid,
                                 OSCL_STATIC_CAST(PVInterface*&, iCPMLicenseInterface)));
}


PVMFStatus PVMFCPMContainerMp3::CheckApprovedUsage()
{
    //compare the approved and requested usage bitmaps
    if ((iApprovedUsage.value.uint32_value & iRequestedUsage.value.uint32_value)
            != iRequestedUsage.value.uint32_value)
    {
        return PVMFErrAccessDenied;//media access denied by CPM.
    }
    return PVMFSuccess;
}

PVMFStatus PVMFCPMContainerMp3::CreateUsageKeys()
{
    iCPMContentType = iCPM->GetCPMContentType(iSessionId);
    if ((iCPMContentType != PVMF_CPM_FORMAT_OMA1) &&
            (iCPMContentType != PVMF_CPM_FORMAT_AUTHORIZE_BEFORE_ACCESS))
    {
        return PVMFFailure;//invalid content type.
    }

    //cleanup any old usage keys
    if (iRequestedUsage.key)
    {
        OSCL_ARRAY_DELETE(iRequestedUsage.key);
        iRequestedUsage.key = NULL;
    }

    if (iApprovedUsage.key)
    {
        OSCL_ARRAY_DELETE(iApprovedUsage.key);
        iApprovedUsage.key = NULL;
    }

    if (iAuthorizationDataKvp.key)
    {
        OSCL_ARRAY_DELETE(iAuthorizationDataKvp.key);
        iAuthorizationDataKvp.key = NULL;
    }

    int32 UseKeyLen = oscl_strlen(_STRLIT_CHAR(PVMF_CPM_REQUEST_USE_KEY_STRING));
    int32 AuthKeyLen = oscl_strlen(_STRLIT_CHAR(PVMF_CPM_AUTHORIZATION_DATA_KEY_STRING));
    int32 leavecode = 0;

    OSCL_TRY(leavecode,
             iRequestedUsage.key = OSCL_ARRAY_NEW(char, UseKeyLen + 1);
             iApprovedUsage.key = OSCL_ARRAY_NEW(char, UseKeyLen + 1);
             iAuthorizationDataKvp.key = OSCL_ARRAY_NEW(char, AuthKeyLen + 1);
            );

    if (leavecode || !iRequestedUsage.key || !iApprovedUsage.key || !iAuthorizationDataKvp.key)
    {
        // Leave occured, do neccessary cleanup
        if (iRequestedUsage.key)
        {
            OSCL_ARRAY_DELETE(iRequestedUsage.key);
            iRequestedUsage.key = NULL;
        }
        if (iApprovedUsage.key)
        {
            OSCL_ARRAY_DELETE(iApprovedUsage.key);
            iApprovedUsage.key = NULL;
        }
        if (iAuthorizationDataKvp.key)
        {
            OSCL_ARRAY_DELETE(iAuthorizationDataKvp.key);
            iAuthorizationDataKvp.key = NULL;
        }

        return PVMFErrNoMemory;
    }

    oscl_strncpy(iRequestedUsage.key, PVMF_CPM_REQUEST_USE_KEY_STRING, UseKeyLen);
    iRequestedUsage.key[UseKeyLen] = 0;
    iRequestedUsage.length = 0;
    iRequestedUsage.capacity = 0;
    iRequestedUsage.value.uint32_value =
        (BITMASK_PVMF_CPM_DRM_INTENT_PLAY |
         BITMASK_PVMF_CPM_DRM_INTENT_PAUSE |
         BITMASK_PVMF_CPM_DRM_INTENT_SEEK_FORWARD |
         BITMASK_PVMF_CPM_DRM_INTENT_SEEK_BACK);

    oscl_strncpy(iApprovedUsage.key, PVMF_CPM_REQUEST_USE_KEY_STRING, UseKeyLen);
    iApprovedUsage.key[UseKeyLen] = 0;
    iApprovedUsage.length = 0;
    iApprovedUsage.capacity = 0;
    iApprovedUsage.value.uint32_value = 0;

    oscl_strncpy(iAuthorizationDataKvp.key, _STRLIT_CHAR(PVMF_CPM_AUTHORIZATION_DATA_KEY_STRING),
                 AuthKeyLen);
    iAuthorizationDataKvp.key[AuthKeyLen] = 0;

    if ((iCPMContentType == PVMF_CPM_FORMAT_OMA1) ||
            (iCPMContentType == PVMF_CPM_FORMAT_AUTHORIZE_BEFORE_ACCESS))
    {
        iAuthorizationDataKvp.length = 0;
        iAuthorizationDataKvp.capacity = 0;
        iAuthorizationDataKvp.value.pUint8_value = NULL;
    }
    else
    {
        if (iRequestedUsage.key)
        {
            OSCL_ARRAY_DELETE(iRequestedUsage.key);
            iRequestedUsage.key = NULL;
        }
        if (iApprovedUsage.key)
        {
            OSCL_ARRAY_DELETE(iApprovedUsage.key);
            iApprovedUsage.key = NULL;
        }
        if (iAuthorizationDataKvp.key)
        {
            OSCL_ARRAY_DELETE(iAuthorizationDataKvp.key);
            iAuthorizationDataKvp.key = NULL;
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFCPMContainerMp3::CreateUsageKeys Usage key creation failed"));

        return PVMFFailure;
    }
    return PVMFSuccess;
}

void PVMFCPMContainerMp3::Cleanup()
{
    //cleanup usage keys
    if (iRequestedUsage.key)
    {
        OSCL_ARRAY_DELETE(iRequestedUsage.key);
        iRequestedUsage.key = NULL;
    }

    if (iApprovedUsage.key)
    {
        OSCL_ARRAY_DELETE(iApprovedUsage.key);
        iApprovedUsage.key = NULL;
    }

    if (iAuthorizationDataKvp.key)
    {
        OSCL_ARRAY_DELETE(iAuthorizationDataKvp.key);
        iAuthorizationDataKvp.key = NULL;
    }

    //cleanup cpm access
    if (iCPMContentAccessFactory)
    {
        iCPMContentAccessFactory->removeRef();
        iCPMContentAccessFactory = NULL;
    }


    //cleanup CPM object.
    if (iCPM)
    {
        iCPM->ThreadLogoff();
        PVMFCPMFactory::DestroyContentPolicyManager(iCPM);
        iCPM = NULL;
    }
}

PVMFStatus PVMFCPMContainerMp3::IssueCommand(int32 aCmd)
{
    // Issue a command to the sub-node.
    // Return the sub-node completion status: either pending, success, or failure.
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFCPMContainerMp3::IssueCommand In"));

    Assert(iCmdState == EIdle && iCancelCmdState == EIdle);
    // Find the current node command since we may need its parameters.
    Assert(!iContainer->iCurrentCommand.empty());
    PVMFMP3FFParserNodeCommand* nodeCmd = &iContainer->iCurrentCommand.front();

    //save the sub-node command code
    iCmd = aCmd;

    switch (aCmd)
    {
        case ECPMCleanup:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling Cleanup"));
            Cleanup();
            return PVMFSuccess;

        case ECPMInit:
            //make sure any prior instance is cleaned up
            Cleanup();
            //Create a CPM instance.
            Assert(iCPM == NULL);
            Assert(iContainer->iUseCPMPluginRegistry == true);
            iCPM = PVMFCPMFactory::CreateContentPolicyManager(*this);
            if (!iCPM)
            {
                return PVMFErrNoMemory;
            }
            iCPM->ThreadLogon();
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling Init"));

            iCmdState = EBusy;
            iCmdId = iCPM->Init();
            return PVMFPending;

        case ECPMOpenSession:
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling OpenSession"));
            iCmdState = EBusy;
            iCmdId = iCPM->OpenSession(iSessionId);
            return PVMFPending;

        case ECPMRegisterContent:
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling RegisterContent"));
            iCmdState = EBusy;
            iCmdId = iCPM->RegisterContent(iSessionId,
                                           iContainer->iSourceURL,
                                           iContainer->iSourceFormat,
                                           (OsclAny*) & iContainer->iCPMSourceData);
            return PVMFPending;

        case ECPMGetLicenseInterface:
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling GetCPMLicenseInterface"));
            iCmdState = EBusy;
            iCmdId = GetCPMLicenseInterface();
            return PVMFPending;

        case ECPMGetLicenseW:
        {
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling ECPMGetLicenseW"));
            iCmdState = EBusy;
            OSCL_wString* contentName = NULL;
            OsclAny* data = NULL;
            uint32 dataSize = 0;
            int32 timeoutMsec = 0;
            nodeCmd->Parse(contentName,
                           data,
                           dataSize,
                           timeoutMsec);
            iCmdId =
                iCPMLicenseInterface->GetLicense(iSessionId,
                                                 *contentName,
                                                 data,
                                                 dataSize,
                                                 timeoutMsec);
            return PVMFPending;
        }
        case ECPMGetLicense:
        {
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling ECPMGetLicense"));

            iCmdState = EBusy;
            OSCL_String* contentName = NULL;
            OsclAny* data = NULL;
            uint32 dataSize = 0;
            int32 timeoutMsec = 0;
            nodeCmd->Parse(contentName,
                           data,
                           dataSize,
                           timeoutMsec);
            iCmdId =
                iCPMLicenseInterface->GetLicense(iSessionId,
                                                 *contentName,
                                                 data,
                                                 dataSize,
                                                 timeoutMsec);

            return PVMFPending;
        }
        case ECPMApproveUsage:
        {
            Assert(iCPM != NULL);
            GetCPMMetaDataExtensionInterface();
            iCPMContentType = iCPM->GetCPMContentType(iSessionId);
            if ((iCPMContentType == PVMF_CPM_FORMAT_OMA1) ||
                    (iCPMContentType == PVMF_CPM_FORMAT_AUTHORIZE_BEFORE_ACCESS))
            {
                iCmdState = EBusy;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFCPMContainerMp3::IssueCommand Calling ApproveUsage"));

                //Create the usage keys
                {
                    PVMFStatus status = CreateUsageKeys();
                    if (status != PVMFSuccess)
                    {
                        return status;
                    }
                }
                iCPM->GetContentAccessFactory(iSessionId, iCPMContentAccessFactory);

                if (iContainer->iDataStreamReadCapacityObserver != NULL)
                {
                    iCPMContentAccessFactory->SetStreamReadCapacityObserver(iContainer->iDataStreamReadCapacityObserver);
                }

                iCmdId = iCPM->ApproveUsage(iSessionId,
                                            iRequestedUsage,
                                            iApprovedUsage,
                                            iAuthorizationDataKvp,
                                            iUsageID);
                iContainer->oWaitingOnLicense = true;
                return PVMFPending;
            }
            else
            {
                /* Unsupported format - use it as unprotected content */
                PVMFStatus status = iContainer->CheckForMP3HeaderAvailability();
                if (status == PVMFSuccess)
                {
                    iContainer->CompleteInit(status);
                }
            }
            return PVMFSuccess;
        }

        case ECPMCheckUsage:
            iContainer->oWaitingOnLicense = false;
            PVMFStatus status;
            //Check for usage approval, and if approved, parse the file.
            if ((iCPMContentType == PVMF_CPM_FORMAT_OMA1) ||
                    (iCPMContentType == PVMF_CPM_FORMAT_AUTHORIZE_BEFORE_ACCESS))
            {
                status = CheckApprovedUsage();
                if (status != PVMFSuccess)
                {
                    return status;
                }
                if (!iCPMContentAccessFactory)
                {
                    return PVMFFailure;//unexpected, since ApproveUsage succeeded.
                }
            }
            if (status == PVMFSuccess)
            {
                if (PVMFSuccess == iContainer->CheckForMP3HeaderAvailability())
                {
                    iContainer->CompleteInit(status);
                }
            }
            return status;


        case ECPMUsageComplete:
            if ((iCPMContentType == PVMF_CPM_FORMAT_OMA1) ||
                    (iCPMContentType == PVMF_CPM_FORMAT_AUTHORIZE_BEFORE_ACCESS))
            {
                Assert(iCPM != NULL);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMFCPMContainerMp3::IssueCommand Calling UsageComplete"));

                iCmdState = EBusy;
                iCmdId = iCPM->UsageComplete(iSessionId, iUsageID);
                return PVMFPending;
            }
            return PVMFSuccess;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling UsageComplete"));
        case ECPMCloseSession:
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling CloseSession"));
            iCmdState = EBusy;
            iCmdId = iCPM->CloseSession(iSessionId);
            return PVMFPending;
        case ECPMReset:
            Assert(iCPM != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMFCPMContainerMp3::IssueCommand Calling Reset"));
            iCmdState = EBusy;
            iCmdId = iCPM->Reset();
            return PVMFPending;
        default:
            Assert(false);
            return PVMFFailure;
    }
}

bool PVMFCPMContainerMp3::CancelPendingCommand()
{
    // Initiate sub-node command cancel, return True if cancel initiated.
    if (iCmdState != EBusy)
    {
        return false;//nothing to cancel
    }
    iCancelCmdState = EBusy;
    //no cancel available, just wait on current command to complete.
    //no cancel available except for GetLicense-- just wait on current command to complete.
    if (iCmd == ECPMGetLicense || iCmd == ECPMGetLicenseW)
    {
        if (iCPM && iCPMLicenseInterface)
        {
            iCancelCmdId = iCPMLicenseInterface->CancelGetLicense(iSessionId, iCmdId);
        }
    }
    return true;//cancel initiated
}

/**
 * From PVMFCPMStatusObserver: callback from the CPM object.
 */
OSCL_EXPORT_REF void PVMFCPMContainerMp3::CPMCommandCompleted(const PVMFCmdResp& aResponse)
{
    //A command to the CPM node is complete
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFCPMContainerMp3::CPMCommandCompleted "));

    PVMFCommandId aCmdId = aResponse.GetCmdId();

    if (aCmdId == iCmdId && iCmdState == EBusy)
    {
        //this is decision point, if CPM does not care about the content
        //skip rest of the CPM steps
        if (iCmd == ECPMRegisterContent)
        {
            PVMFStatus status = aResponse.GetCmdStatus();
            if (status == PVMFErrNotSupported)
            {
                //if CPM comes back as PVMFErrNotSupported then by pass rest of the CPM
                //sequence. Fake success here so that node doesnt treat this as an error
                status = PVMFSuccess;
                if (iContainer->CheckForMP3HeaderAvailability() != PVMFSuccess)
                {
                    return;
                }
            }
            else if (status == PVMFSuccess)
            {
                //proceed with rest of the CPM steps
                iContainer->Push(iContainer->iCPMContainer,
                                 PVMFSubNodeContainerBaseMp3::ECPMGetLicenseInterface);
                iContainer->Push(iContainer->iCPMContainer,
                                 PVMFSubNodeContainerBaseMp3::ECPMApproveUsage);
                iContainer->Push(iContainer->iCPMContainer,
                                 PVMFSubNodeContainerBaseMp3::ECPMCheckUsage);
            }
            CommandDone(status,
                        aResponse.GetEventExtensionInterface(),
                        aResponse.GetEventData());
        }
        else
        {
            CommandDone(aResponse.GetCmdStatus(),
                        aResponse.GetEventExtensionInterface(),
                        aResponse.GetEventData());
        }
        //catch completion of cancel for CPM commands
        //since there's no cancel to the CPM module, the cancel
        //is done whenever the current CPM command is done.
        if (iCancelCmdState != EIdle)
        {
            if (iCmd != ECPMGetLicense && iCmd != ECPMGetLicenseW)
                CancelCommandDone(PVMFSuccess, NULL, NULL);
        }
    }
    else if (aResponse.GetCmdId() == iCancelCmdId
             && iCancelCmdState == EBusy)
    {
        //Process node cancel command response
        CancelCommandDone(aResponse.GetCmdStatus(), aResponse.GetEventExtensionInterface(), aResponse.GetEventData());
    }
    else if (aResponse.GetCmdId() == iContainer->iCPMGetMetaDataKeysCmdId)
    {
        // End of GetNodeMetaDataKeys
        PVMFStatus status =
            iContainer->CompleteGetMetadataKeys(iContainer->iCurrentCommand.front());
        iContainer->CommandComplete(iContainer->iCurrentCommand,
                                    iContainer->iCurrentCommand.front(),
                                    status,
                                    NULL,
                                    NULL);
    }
    else if (aResponse.GetCmdId() == iContainer->iCPMGetMetaDataValuesCmdId)
    {
        // End of GetNodeMetaDataValues
        iContainer->CommandComplete(iContainer->iCurrentCommand,
                                    iContainer->iCurrentCommand.front(),
                                    aResponse.GetCmdStatus(),
                                    NULL,
                                    NULL);
    }
    else
    {
        Assert(false);//unexpected response
    }
}

void PVMFSubNodeContainerBaseMp3::Assert(bool aCondition)
{
    if (!aCondition)
    {
        iContainer->Assert(aCondition);
    }
}

void PVMFSubNodeContainerBaseMp3::CommandDone(PVMFStatus aStatus, PVInterface*aExtMsg,
        OsclAny*aEventData)
{
    // Sub-node command is completed, process the result.
    Assert(aStatus != PVMFPending);
    // Pop the sub-node command vector.
    Assert(!iContainer->iSubNodeCmdVec.empty());
    iContainer->iSubNodeCmdVec.erase(&iContainer->iSubNodeCmdVec.front());

    iCmdState = EIdle;
    PVMFStatus status = aStatus;
    //Check whether the node command is being cancelled.
    if (iCancelCmdState != EIdle)
    {
        if (!iContainer->iSubNodeCmdVec.empty())
        {
            //even if this command succeeded, we want to report
            //the node command status as cancelled since some sub-node
            //commands were not yet issued.
            status = PVMFErrCancelled;
            //go into an error state since the command is partially completed
            iContainer->SetState(EPVMFNodeError);
        }
    }
    //figure out the next step in the sequence
    //A node command is done when either all sub-node commands are
    //done or when one fails.
    if (status == PVMFSuccess && !iContainer->iSubNodeCmdVec.empty())
    {
        //The node needs to issue the next sub-node command.
        iContainer->RunIfNotReady();
    }
    else
    {
        //node command is done.
        Assert(!iContainer->iCurrentCommand.empty());
        iContainer->CommandComplete(iContainer->iCurrentCommand, iContainer->iCurrentCommand.front(), status, aExtMsg, aEventData);
    }
}

void PVMFSubNodeContainerBaseMp3::CancelCommandDone(PVMFStatus aStatus, PVInterface*aExtMsg, OsclAny*aEventData)
{
    // Sub-node cancel command is done: process the result
    OSCL_UNUSED_ARG(aExtMsg);
    OSCL_UNUSED_ARG(aEventData);

    Assert(aStatus != PVMFPending);
    iCancelCmdState = EIdle;
    //print and ignore any failed sub-node cancel commands.
    if (aStatus != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                        "PVMFCPMContainerMp3::CancelCommandDone CPM Node Cancel failed"));
    }

    //Node cancel command is now done.
    Assert(!iContainer->iCancelCommand.empty());
    iContainer->CommandComplete(iContainer->iCancelCommand, iContainer->iCancelCommand.front(), aStatus, NULL, NULL);
}

/**
 * From PvmiDataStreamObserver: callback when the DataStreamCommand is completed
 */
void PVMFMP3FFParserNode::DataStreamCommandCompleted(const PVMFCmdResp& aResponse)
{
    if ((iCurrentCommand.empty() == false) &&
            (iCurrentCommand.front().iCmd == PVMF_GENERIC_NODE_INIT))
    {
        PVMFStatus cmdStatus = PVMFFailure;
        if (aResponse.GetCmdId() == iRequestReadCapacityNotificationID)
        {
            cmdStatus = aResponse.GetCmdStatus();
            if (cmdStatus == PVMFSuccess)
            {
                // set flag
                iCheckForMP3HeaderDuringInit = true;

                // Re-schedule the node
                RunIfNotReady();
            }
            else
            {
                LOGINFO((0, "PVMFMP3FFParserNode::DataStreamCommandCompleted() RequestReadCapacityNotification failed %d", cmdStatus));
                // command init command with some kind of failure
                CompleteInit(cmdStatus);
            }
        }
        return;
    }
    // Handle Autopause
    if (iAutoPaused)
    {
        if (aResponse.GetCmdStatus() == PVMFSuccess)
        {
            if (iTrack.iState == PVMP3FFNodeTrackPortInfo::TRACKSTATE_DOWNLOAD_AUTOPAUSE)
            {
                iTrack.iState = PVMP3FFNodeTrackPortInfo::TRACKSTATE_TRANSMITTING_GETDATA;
            }

            iAutoPaused = false;
            // Re-schedule the node
            RunIfNotReady();
            return;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFMP3FFParserNode::DataStreamReadCapacityNotificationCallBack() Reporting failure"));
            ReportErrorEvent(PVMFErrResource, NULL);
        }
        return;
    }
    else
    {
        /* unrecognized callback */
        OSCL_ASSERT(false);
    }
}

/**
 * From PvmiDataStreamObserver: callback for info event from DataStream
 */
void PVMFMP3FFParserNode::DataStreamInformationalEvent(const PVMFAsyncEvent& aEvent)
{
    //if Datadownload is complete then send PVMFInfoBufferingComplete event from DS to parser node
    if (aEvent.GetEventType() == PVMFInfoBufferingComplete)
    {
        iDownloadComplete = true;
    }
}

/**
 * From PvmiDataStreamObserver: callback for error event from DataStream
 */
void PVMFMP3FFParserNode::DataStreamErrorEvent(const PVMFAsyncEvent& aEvent)
{
    OSCL_UNUSED_ARG(aEvent);
    //Should never be called
    OSCL_ASSERT(false);
}

PVMFStatus PVMFMP3FFParserNode::CheckForMP3HeaderAvailability()
{
    if (iDataStreamInterface != NULL)
    {
        uint32 minBytesRequired = 0;
        if (iMP3File)
        {
            minBytesRequired = iMP3File->GetMinBytesRequired();
        }

        /*
         * First check if we have minimum number of bytes to recognize
         * the file and determine the header size.
         */
        uint32 contentLength =  iDataStreamInterface->GetContentLength();
        minBytesRequired = (minBytesRequired > contentLength) ? contentLength : minBytesRequired;

        uint32 currCapacity = 0;
        PvmiDataStreamStatus status = iDataStreamInterface->QueryReadCapacity(iDataStreamSessionID,
                                      currCapacity);

        if ((PVDS_SUCCESS == status) && (currCapacity <  minBytesRequired))
        {
            iRequestReadCapacityNotificationID =
                iDataStreamInterface->RequestReadCapacityNotification(iDataStreamSessionID,
                        *this,
                        minBytesRequired);
            return PVMFPending;
        }

        MP3ErrorType retCode = MP3_ERROR_UNKNOWN;
        if (iMP3File)
        {
            retCode = iMP3File->GetMetadataSize(iMP3MetaDataSize);
            if (retCode == MP3_SUCCESS)
            {
                /* Fetch the id3 tag size, if any and make it persistent in cache*/
                iDataStreamInterface->MakePersistent(0, iMP3MetaDataSize);
                if (currCapacity < (uint32)iMP3MetaDataSize)
                {
                    iRequestReadCapacityNotificationID =
                        iDataStreamInterface->RequestReadCapacityNotification(iDataStreamSessionID,
                                *this,
                                iMP3MetaDataSize + minBytesRequired);
                    return PVMFPending;
                }
            }
            else
            {
                iDataStreamInterface->MakePersistent(0, 0);
            }
        }
    }
    if (PVMFSuccess != ParseFile())
    {
        return PVMFFailure;
    }
    return PVMFSuccess;
}

void PVMFMP3FFParserNode::GetCPMMetaDataKeys()
{
    if (iCPMContainer.iCPMMetaDataExtensionInterface != NULL)
    {
        iCPMMetadataKeys.clear();
        iCPMGetMetaDataKeysCmdId =
            iCPMContainer.iCPMMetaDataExtensionInterface->GetNodeMetadataKeys(iCPMContainer.iSessionId,
                    iCPMMetadataKeys,
                    0,
                    PVMF_MP3_PARSER_NODE_MAX_CPM_METADATA_KEYS);
    }
}

bool PVMFCPMContainerMp3::GetCPMMetaDataExtensionInterface()
{
    bool retVal =
        iCPM->queryInterface(KPVMFMetadataExtensionUuid,
                             OSCL_STATIC_CAST(PVInterface*&, iCPMMetaDataExtensionInterface));
    return retVal;
}
PVMp3DurationCalculator::PVMp3DurationCalculator(int32 aPriority, IMpeg3File* aMP3File, PVMFNodeInterface* aNode, bool aScanEnabled):
        OsclTimerObject(aPriority, "PVMp3DurationCalculator"), iNode(aNode)
{
    iErrorCode = MP3_SUCCESS;
    iMP3File = aMP3File;
    iScanComplete = false;
    iScanEnabled = aScanEnabled;
    if (!IsAdded())
    {
        AddToScheduler();
    }
}

PVMp3DurationCalculator::~PVMp3DurationCalculator()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }
}

void PVMp3DurationCalculator::ScheduleAO()
{
    totalticks = 0;
    if (iScanEnabled && iMP3File)
    {
        RunIfNotReady();
    }
}

void PVMp3DurationCalculator::Run()
{
    // dont do the duration calculation scan in case of PS/PD
    if (((PVMFMP3FFParserNode*)iNode)->iDownloadProgressInterface)
    {
        return;
    }

    if (iErrorCode == MP3_DURATION_PRESENT)
    {
        // A valid duration is already present no need to scan the file, just send the duration event and return
        iScanComplete = true;
        int32 durationInMsec = iMP3File->GetDuration();
        int32 leavecode = 0;
        PVMFDurationInfoMessage* eventmsg = NULL;
        OSCL_TRY(leavecode, eventmsg = OSCL_NEW(PVMFDurationInfoMessage, (durationInMsec)));
        ((PVMFMP3FFParserNode*)iNode)->ReportInfoEvent(PVMFInfoDurationAvailable, NULL, OSCL_STATIC_CAST(PVInterface*, eventmsg));
        if (eventmsg)
        {
            eventmsg->removeRef();
        }
        return;
    }
    if (iErrorCode != MP3_SUCCESS)
    {
        iScanComplete = true;
        int32 durationInMsec = iMP3File->GetDuration();
        int32 leavecode = 0;
        PVMFDurationInfoMessage* eventmsg = NULL;
        OSCL_TRY(leavecode, eventmsg = OSCL_NEW(PVMFDurationInfoMessage, (durationInMsec)));
        ((PVMFMP3FFParserNode*)iNode)->ReportInfoEvent(PVMFInfoDurationAvailable, NULL, OSCL_STATIC_CAST(PVInterface*, eventmsg));
        if (eventmsg)
        {
            eventmsg->removeRef();
        }
        return;
    }
    else if (!iScanComplete)
    {
        RunIfNotReady(PVMF3FF_DURATION_SCAN_AO_DELAY);
    }
    else
    {
        return;
    }

    if (!(((PVMFMP3FFParserNode*)iNode)->iTrack.iSendBOS))
    {
        // Start the scan only when we have send first audio sample
        iErrorCode = iMP3File->ScanMP3File(PVMF3FF_DEFAULT_NUM_OF_FRAMES * 5);
    }
}
