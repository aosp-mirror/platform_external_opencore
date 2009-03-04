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

#include "pvmf_downloadmanager_node.h"
#include "pvmf_download_data_source.h"
#include "pvmf_local_data_source.h"
#include "pvmf_protocol_engine_factory.h"
#include "pvmf_socket_factory.h"
#include "pvmf_socket_node.h"
#include "pvlogger.h"
#include "oscl_error_codes.h"
#include "oscl_str_ptr_len.h" // for OSCL_ASCII_CASE_MAGIC_BIT
#include "pvmi_datastreamuser_interface.h"
#include "pv_mime_string_utils.h"
#include "pvmi_kvp_util.h"
#include "pvmf_source_context_data.h"
#include "oscl_utf8conv.h"

///////////////////////////////////////////////////////////////////////////////
//
// Capability and config interface related constants and definitions
//   - based on pv_player_engine.h
//
///////////////////////////////////////////////////////////////////////////////

static const DownloadManagerKeyStringData DownloadManagerConfig_BaseKeys[] =
{
    {"user-agent", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_WCHARPTR},
    {"http-version", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"http-timeout", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"download-progress-info", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_CHARPTR},
    {"protocol-extension-header", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_CHARPTR},
    {"num-redirect-attempts", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"http-header-request-disabled", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL},
    {"max-tcp-recv-buffer-size-download", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"max-tcp-recv-buffer-count-download", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32}
};

static const uint DownloadManagerConfig_NumBaseKeys =
    (sizeof(DownloadManagerConfig_BaseKeys) /
     sizeof(DownloadManagerKeyStringData));

enum BaseKeys_IndexMapType
{
    BASEKEY_SESSION_CONTROLLER_USER_AGENT = 0,
    BASEKEY_SESSION_CONTROLLER_HTTP_VERSION,
    BASEKEY_SESSION_CONTROLLER_HTTP_TIMEOUT,
    BASEKEY_SESSION_CONTROLLER_DOWNLOAD_PROGRESS_INFO,
    BASEKEY_SESSION_CONTROLLER_PROTOCOL_EXTENSION_HEADER,
    BASEKEY_SESSION_CONTROLLER_NUM_REDIRECT_ATTEMPTS,
    BASEKEY_SESSION_CONTROLLER_NUM_HTTP_HEADER_REQUEST_DISABLED,
    BASEKEY_MAX_TCP_RECV_BUFFER_SIZE,
    BASEKEY_MAX_TCP_RECV_BUFFER_COUNT
};

void PVMFDownloadManagerNode::Assert(bool x)
{
    if (!x)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_CRIT, (0, "PVMFDownloadManagerNode Assertion Failed!"));
        OSCL_ASSERT(0);
    }
}

void PVMFDownloadManagerSubNodeContainerBase::Assert(bool x)
{
    if (!x)
    {
        iContainer->Assert(x);
    }
}

PVMFDownloadManagerNode::PVMFDownloadManagerNode(int32 aPriority)
        : OsclActiveObject(aPriority, "PVMFDownloadManagerNode")
{
    int32 err;
    OSCL_TRY(err, ConstructL(););
    if (err != OsclErrNone)
    {
        //if a leave happened, cleanup and re-throw the error
        iInputCommands.clear();
        iCurrentCommand.clear();
        iCancelCommand.clear();
        iCapability.iInputFormatCapability.clear();
        iCapability.iOutputFormatCapability.clear();
        OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
        OSCL_CLEANUP_BASE_CLASS(OsclActiveObject);
        OSCL_LEAVE(err);
    }

    iDNodeUuids.clear();
    iDNodeUuidCount = 0;
}

void PVMFDownloadManagerNode::ConstructL()
{
    iDebugMode = false;
    iLogger = NULL;
    iExtensionRefCount = 0;
    iSourceFormat = PVMF_FORMAT_UNKNOWN;
    iMimeType = PVMF_MIME_FORMAT_UNKNOWN;
    iSourceData = NULL;
    iPlayBackClock = NULL;


    iParserInit = false;
    iDataReady = false;
    iDownloadComplete = false;
    iRecognizerError = false;

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
    iInitFailedLicenseRequired = false;
#endif

    iProtocolEngineNodePort = NULL;
    iSocketNodePort = NULL;
    iPlayerNodeRegistry = NULL;

    //create the sub-node command queue.  Use a reserve to avoid dynamic memory failure later.
    //Max depth is the max number of sub-node commands for any one node command. Init command may take up to 15
    iSubNodeCmdVec.reserve(15);

    //Create the input command queue. Max depth is undetermined -- just reserve 10.
    iInputCommands.Construct(1000 //start cmd id
                             , 10);//reserve.

    //Create the "current command" queue. Max depth is 1 for each of these.
    iCurrentCommand.Construct(0, 1);
    iCancelCommand.Construct(0, 1);

    //create node containers.
    //@TODO this will create unused node containers.  think about
    //optimizing it.
    iFormatParserNode.Construct(PVMFDownloadManagerSubNodeContainerBase::EFormatParser, this);
    iProtocolEngineNode.Construct(PVMFDownloadManagerSubNodeContainerBase::EProtocolEngine, this);
    iSocketNode.Construct(PVMFDownloadManagerSubNodeContainerBase::ESocket, this);
    iRecognizerNode.Construct(PVMFDownloadManagerSubNodeContainerBase::ERecognizer, this);

    //Set the node capability data.
    iCapability.iCanSupportMultipleInputPorts = false;
    iCapability.iCanSupportMultipleOutputPorts = true;
    iCapability.iHasMaxNumberOfPorts = true;
    iCapability.iMaxNumberOfPorts = 6;
    iCapability.iInputFormatCapability.push_back(PVMF_MPEG4FF);
    iCapability.iInputFormatCapability.push_back(PVMF_ASFFF);
    iCapability.iInputFormatCapability.push_back(PVMF_RMFF);
    iCapability.iOutputFormatCapability.push_back(PVMF_AMR_IETF);
    iCapability.iOutputFormatCapability.push_back(PVMF_MPEG4_AUDIO);
    iCapability.iOutputFormatCapability.push_back(PVMF_M4V);
    iCapability.iOutputFormatCapability.push_back(PVMF_H263);
    iCapability.iOutputFormatCapability.push_back(PVMF_RV);
    iCapability.iOutputFormatCapability.push_back(PVMF_WMV);

    iFileBufferDatastreamFactory = NULL;
#if(PVMF_DOWNLOADMANAGER_SUPPORT_PPB)
    iMemoryBufferDatastreamFactory = NULL;
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_PPB

    iDownloadFileName = NULL;

    iProtocolEngineNode.iNode = PVMFProtocolEngineNodeFactory::CreatePVMFProtocolEngineNode(OsclActiveObject::EPriorityNominal);
    OsclError::LeaveIfNull(iProtocolEngineNode.iNode);
    iProtocolEngineNode.Connect();

    iSocketNode.iNode = PVMFSocketNodeFactory::CreatePVMFSocketNode(OsclActiveObject::EPriorityNominal);
    OsclError::LeaveIfNull(iSocketNode.iNode);
    iSocketNode.Connect();
}

PVMFDownloadManagerNode::~PVMFDownloadManagerNode()
{
    //remove the clock observer
    if (iPlayBackClock != NULL)
    {
        iPlayBackClock->RemoveClockStateObserver(*this);
    }

    Cancel();
    if (IsAdded())
        RemoveFromScheduler();

    //if any sub-node commands are outstanding, there will be
    //a crash when they callback-- so panic here instead.

    if (iFormatParserNode.CmdPending()
            || iProtocolEngineNode.CmdPending()
            || iSocketNode.CmdPending()
            || iRecognizerNode.CmdPending()
       )
    {
        OSCL_ASSERT(0);
    }

    //this is to ensure that there are no more callbacks from PE node to parser node,
    //in case parser node had some outstanding request resume notifications
    if (iProtocolEngineNode.DownloadProgress() != NULL)
    {
        (iProtocolEngineNode.DownloadProgress())->setFormatDownloadSupportInterface(NULL);
    }

    //make sure the subnodes got cleaned up
    iFormatParserNode.Cleanup();
    iProtocolEngineNode.Cleanup();
    iSocketNode.Cleanup();
    iRecognizerNode.Cleanup();

    //delete the subnodes
    if (iFormatParserNode.iNode)
    {
        iDNodeUuidCount--;

        bool release_status = false;
        int32 leavecode = 0;
        OSCL_TRY(leavecode, release_status = iPlayerNodeRegistry->ReleaseNode(iDNodeUuids[iDNodeUuidCount], iFormatParserNode.iNode));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFDownloadManagerNode::~PVMFDownloadManagerNode() Error in releasing Download Manager Node")););

        if (release_status == false)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::~PVMFDownloadManagerNode() Factory returned false while releasing the download manager node"));
        }

        iDNodeUuids.clear();
    }

    if (iProtocolEngineNode.iNode)
        PVMFProtocolEngineNodeFactory::DeletePVMFProtocolEngineNode(iProtocolEngineNode.iNode);

    if (iSocketNode.iNode)
        PVMFSocketNodeFactory::DeletePVMFSocketNode(iSocketNode.iNode);

    // delete the data stream factory (This has to come after deleting anybody who uses it, like the protocol engine node or the parser node.)
    if (iFileBufferDatastreamFactory)
    {
        OSCL_DELETE(iFileBufferDatastreamFactory);
        iFileBufferDatastreamFactory = NULL;
    }
#if(PVMF_DOWNLOADMANAGER_SUPPORT_PPB)
    if (iMemoryBufferDatastreamFactory)
    {
        OSCL_DELETE(iMemoryBufferDatastreamFactory);
        iMemoryBufferDatastreamFactory = NULL;
    }
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_PPB

    //The command queues are self-deleting, but we want to notify the observer of unprocessed commands.
    while (!iCurrentCommand.empty())
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFFailure, NULL, NULL);
    while (!iCancelCommand.empty())
        CommandComplete(iCancelCommand, iCancelCommand.front(), PVMFFailure, NULL, NULL);
    while (!iInputCommands.empty())
        CommandComplete(iInputCommands, iInputCommands.front(), PVMFFailure, NULL, NULL);

}

//Public API From node interface.
PVMFStatus PVMFDownloadManagerNode::ThreadLogon()
{
    if (iInterfaceState != EPVMFNodeCreated)
        return PVMFErrInvalidState;

    //logon this node.
    if (!IsAdded())
        AddToScheduler();

    iLogger = PVLogger::GetLoggerObject("pvdownloadmanagernode");

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ThreadLogon() called"));

    //logon the sub-nodes.
    if (iProtocolEngineNode.iNode)
        iProtocolEngineNode.iNode->ThreadLogon();

    if (iSocketNode.iNode)
        iSocketNode.iNode->ThreadLogon();

    ChangeNodeState(EPVMFNodeIdle);
    return PVMFSuccess;
}


//Public API From node interface.
PVMFStatus PVMFDownloadManagerNode::ThreadLogoff()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ThreadLogoff() called"));

    if (iInterfaceState != EPVMFNodeIdle)
        return PVMFErrInvalidState;

    //logoff this node.
    if (IsAdded())
        RemoveFromScheduler();

    iLogger = NULL;

    //logoff the sub-nodes.
    if (iFormatParserNode.iNode)
        iFormatParserNode.iNode->ThreadLogoff();
    if (iProtocolEngineNode.iNode)
        iProtocolEngineNode.iNode->ThreadLogoff();
    if (iSocketNode.iNode)
        iSocketNode.iNode->ThreadLogoff();

    ChangeNodeState(EPVMFNodeCreated);
    return PVMFSuccess;
}


//Public API From node interface.
PVMFStatus PVMFDownloadManagerNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetCapability() called"));

    aNodeCapability = iCapability;

    return PVMFSuccess;
}


//Public API From node interface.
PVMFPortIter* PVMFDownloadManagerNode::GetPorts(const PVMFPortFilter* aFilter)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetPorts() called"));

    if (iFormatParserNode.iNode)
        return iFormatParserNode.iNode->GetPorts(aFilter);
    return NULL;
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::QueryUUID(PVMFSessionId aSessionId, const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids, bool aExactUuidsOnly, const OsclAny* aContext)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::QueryUUID() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_QUERYUUID, aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::QueryInterface(PVMFSessionId aSessionId, const PVUuid& aUuid,
        PVInterface*& aInterfacePtr, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::QueryInterface() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_QUERYINTERFACE, aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::RequestPort(PVMFSessionId aSessionId, int32 aPortTag,
        const PvmfMimeString* aPortConfig, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::RequestPort() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_REQUESTPORT, aPortTag, aPortConfig, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFStatus PVMFDownloadManagerNode::ReleasePort(PVMFSessionId aSessionId, PVMFPortInterface& aPort, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ReleasePort() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Init(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Init() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_INIT, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Prepare(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Prepare() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_PREPARE, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Start(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Start() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_START, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Stop(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Stop() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_STOP, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Flush(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Flush() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_FLUSH, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Pause(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Pause() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_PAUSE, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::Reset(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::Reset() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_RESET, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::CancelAllCommands(PVMFSessionId aSessionId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::CancelAllCommands() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_CANCELALLCOMMANDS, aContext);
    return QueueCommandL(cmd);
}


//Public API From node interface.
PVMFCommandId PVMFDownloadManagerNode::CancelCommand(PVMFSessionId aSessionId, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::CancelCommand() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId, PVMF_GENERIC_NODE_CANCELCOMMAND, aCmdId, aContext);
    return QueueCommandL(cmd);
}

//public API from PVInterface
void PVMFDownloadManagerNode::addRef()
{
    ++iExtensionRefCount;
}

//public API from PVInterface
void PVMFDownloadManagerNode::removeRef()
{
    --iExtensionRefCount;
}

//public API from PVInterface
bool PVMFDownloadManagerNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::queryInterface() In"));

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
    else if (uuid == PVMF_DATA_SOURCE_NODE_REGISRTY_INIT_INTERFACE_UUID)
    {
        PVMFDataSourceNodeRegistryInitInterface* myInterface =
            OSCL_STATIC_CAST(PVMFDataSourceNodeRegistryInitInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (uuid == PvmfDataSourcePlaybackControlUuid)
    {
        PvmfDataSourcePlaybackControlInterface* myInterface = OSCL_STATIC_CAST(PvmfDataSourcePlaybackControlInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else if (uuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
    else if (uuid == PVMFCPMPluginLicenseInterfaceUuid)
    {
        PVMFCPMPluginLicenseInterface* myInterface = OSCL_STATIC_CAST(PVMFCPMPluginLicenseInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE
    else
    {
        return false;
    }

    ++iExtensionRefCount;
    return true;
}


//public API from data source initialization interface
PVMFStatus PVMFDownloadManagerNode::SetSourceInitializationData(OSCL_wString& aSourceURL, PVMFFormatType& aSourceFormat, OsclAny* aSourceData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::SetSourceInitializationData() called"));

    //this method must be called before the Init command.
    if (iInterfaceState != EPVMFNodeIdle && iInterfaceState != EPVMFNodeCreated)
        return PVMFErrInvalidState;

    //Validate
    switch (aSourceFormat)
    {
        case PVMF_DATA_SOURCE_HTTP_URL:
            if (!aSourceData)
                return PVMFErrArgument;
            break;

        case PVMF_DATA_SOURCE_PVX_FILE:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerNode:SetSourceInitializationData() Unsupported source format."));
            return PVMFErrArgument; // unsupported format

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerNode:SetSourceInitializationData() Unsupported source format."));
            return PVMFErrArgument; // unsupported format
    }

    // Pass the source info directly to the protocol engine node.

    if (!iProtocolEngineNode.DataSourceInit())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                        "PVMFDownloadManagerNode:SetSourceInitializationData() Can't find datasourceinit interface in protocol engine subnode container."));
        return PVMFFailure; //no source init interface.
    }

    PVMFStatus status = (iProtocolEngineNode.DataSourceInit())->SetSourceInitializationData(aSourceURL, aSourceFormat, aSourceData);
    if (status != PVMFSuccess)
        return status;

    if (!iProtocolEngineNode.ProtocolEngineExtension())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                        "PVMFDownloadManagerNode:SetSourceInitializationData() Can't get ProtocolEngineExtension interface from protocol subnode container."));
        return PVMFFailure; //no ProtocolNodeExtension interface.
    }

    bool socketConfigOK = (iProtocolEngineNode.ProtocolEngineExtension())->GetSocketConfig(iServerAddr);
    if (!socketConfigOK)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                        "PVMFDownloadManagerNode: SetSourceInitializationData() Call to GetSocketConfig() on protocol engine node returned failure."));
        return PVMFErrProcessing;
    }

    switch (aSourceFormat)
    {
        case PVMF_DATA_SOURCE_HTTP_URL:
        {
            PVInterface* pvinterface = (PVInterface*)aSourceData;
            PVUuid uuid(PVMF_DOWNLOAD_DATASOURCE_HTTP_UUID);
            PVMFDownloadDataSourceHTTP* data = NULL;
            if (pvinterface->queryInterface(uuid, (PVInterface*&)data))
            {
                //extract the download file name from the opaque data.
                iDownloadFileName = data->iDownloadFileName;

                //extract CPM options
                iLocalDataSource.iUseCPMPluginRegistry = data->iUseCPMPluginRegistryForPlayback;

                //extract the playback mode
                switch (data->iPlaybackControl)
                {
                    case PVMFDownloadDataSourceHTTP::ENoPlayback:
                        iPlaybackMode = EDownloadOnly;
                        break;
                    case PVMFDownloadDataSourceHTTP::EAfterDownload:
                        iPlaybackMode = EDownloadThenPlay;
                        break;
                    case PVMFDownloadDataSourceHTTP::EAsap:
                        iPlaybackMode = EPlayAsap;
                        break;

                    case PVMFDownloadDataSourceHTTP::ENoSaveToFile:
#if(PVMF_DOWNLOADMANAGER_SUPPORT_PPB)
                        iPlaybackMode = EPlaybackOnly;
                        break;
#else
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                                        "PVMFDownloadManagerNode:SetSourceInitializationData() NoSaveToFile is not supported!"));
                        return PVMFErrArgument;//unsupported mode.
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_PPB

                    default:
                        iPlaybackMode = EPlayAsap;
                        break;
                }
            }
            else
            {
                PVUuid uuid(PVMF_SOURCE_CONTEXT_DATA_DOWNLOAD_HTTP_UUID);
                PVMFSourceContextDataDownloadHTTP* data = NULL;
                if (pvinterface->queryInterface(uuid, (PVInterface*&)data))
                {
                    //extract the download file name from the opaque data.
                    iDownloadFileName = data->iDownloadFileName;

                    //extract the playback mode
                    switch (data->iPlaybackControl)
                    {
                        case PVMFSourceContextDataDownloadHTTP::ENoPlayback:
                            iPlaybackMode = EDownloadOnly;
                            break;
                        case PVMFSourceContextDataDownloadHTTP::EAfterDownload:
                            iPlaybackMode = EDownloadThenPlay;
                            break;
                        case PVMFSourceContextDataDownloadHTTP::EAsap:
                            iPlaybackMode = EPlayAsap;
                            break;

                        case PVMFSourceContextDataDownloadHTTP::ENoSaveToFile:
#if(PVMF_DOWNLOADMANAGER_SUPPORT_PPB)
                            iPlaybackMode = EPlaybackOnly;
                            break;
#else
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                                            "PVMFDownloadManagerNode:SetSourceInitializationData() NoSaveToFile is not supported!"));
                            return PVMFErrArgument;//unsupported mode.
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_PPB

                        default:
                            iPlaybackMode = EPlayAsap;
                            break;
                    }

                    //extract the cpm usage flag from the common data
                    {
                        PVUuid uuid(PVMF_SOURCE_CONTEXT_DATA_COMMON_UUID);
                        PVMFSourceContextDataCommon* data = NULL;
                        if (pvinterface->queryInterface(uuid, (PVInterface*&)data))
                        {
                            iLocalDataSource.iUseCPMPluginRegistry = data->iUseCPMPluginRegistry;
                        }
                        else
                        {//invalid source data
                            return PVMFErrArgument;
                        }
                    }
                }
                else
                {//invalid source data
                    return PVMFErrArgument;
                }
            }
        }
        break;

        default:
            Assert(false);
            break;
    }

#if(PVMF_DOWNLOADMANAGER_SUPPORT_PPB)
    if (iPlaybackMode == EPlaybackOnly)
    {
        //make sure we have enough TCP buffers for PPB
        if (iSocketNode.iNode)
            ((PVMFSocketNode*)iSocketNode.iNode)->SetMaxTCPRecvBufferCount(PVMF_DOWNLOADMANAGER_MIN_TCP_BUFFERS_FOR_PPB);

        // Use Memory Buffer Data Stream for progressive playback
        iMemoryBufferDatastreamFactory = OSCL_NEW(PVMFMemoryBufferDataStream, ());

        Assert(iMemoryBufferDatastreamFactory != NULL);

        iReadFactory  = iMemoryBufferDatastreamFactory->GetReadDataStreamFactoryPtr();
        iWriteFactory = iMemoryBufferDatastreamFactory->GetWriteDataStreamFactoryPtr();
    }
    else
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_PPB
    {
        // Now that we have the download file name, we can instantiate the file buffer data stream object
        // Create the filebuffer data stream factory
        iFileBufferDatastreamFactory = OSCL_NEW(PVMFFileBufferDataStream, (iDownloadFileName));
        Assert(iFileBufferDatastreamFactory != NULL);
        iReadFactory  = iFileBufferDatastreamFactory->GetReadDataStreamFactoryPtr();
        iWriteFactory = iFileBufferDatastreamFactory->GetWriteDataStreamFactoryPtr();
    }

    //save the source info
    iSourceFormat = aSourceFormat;
    iSourceURL = aSourceURL;
    iSourceData = aSourceData;

    return PVMFSuccess;
}

//public API from data source initialization interface
PVMFStatus PVMFDownloadManagerNode::SetClientPlayBackClock(OsclClock* aClientClock)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::SetClientPlayBackClock() called"));

    iPlayBackClock = aClientClock;
    iPlayBackClock->SetClockStateObserver(*this);

    //pass the source info directly to the download node.
    if (NULL == iProtocolEngineNode.DataSourceInit())
        return PVMFFailure;//no source init interface.

    PVMFStatus status = (iProtocolEngineNode.DataSourceInit())->SetClientPlayBackClock(aClientClock);

    return status;
}

//public API from data source initialization interface
PVMFStatus PVMFDownloadManagerNode::SetEstimatedServerClock(OsclClock*)
{
    //not needed for download.
    return PVMFErrNotSupported;
}

PVMFDownloadManagerSubNodeContainer& PVMFDownloadManagerNode::TrackSelectNode()
{
    //Decide which sub-node is supporting track selection.
    switch (iSourceFormat)
    {
        case PVMF_DATA_SOURCE_HTTP_URL:
            //for 3gpp, the parser does track selection.
            return iFormatParserNode;


        default:
            return iFormatParserNode;
    }
}

//Public API From track selection interface.
PVMFStatus PVMFDownloadManagerNode::GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo)
{
    //this is assumed to happen only after node initialization.
    if (iInterfaceState != EPVMFNodeInitialized && iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    if (TrackSelectNode().TrackSelection())
        return (TrackSelectNode().TrackSelection())->GetMediaPresentationInfo(aInfo);
    else
        return PVMFFailure; //no track selection interface!
}

//Public API From track selection interface.
PVMFStatus PVMFDownloadManagerNode::SelectTracks(PVMFMediaPresentationInfo& aInfo)
{
    //this needs to happen after initialization.
    if (iInterfaceState != EPVMFNodeInitialized && iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    if (TrackSelectNode().TrackSelection())
        return (TrackSelectNode().TrackSelection())->SelectTracks(aInfo);
    else
        return PVMFFailure;//no track selection interface!
}


uint32 PVMFDownloadManagerNode::GetNumMetadataKeys(char* query_key)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetNumMetadataKeys() called"));
    if (iFormatParserNode.Metadata())
    {
        return (iFormatParserNode.Metadata())->GetNumMetadataKeys(query_key);
    }
    return 0;
}

uint32 PVMFDownloadManagerNode::GetNumMetadataValues(PVMFMetadataList& aKeyList)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetNumMetadataValues() called"));
    if (iFormatParserNode.Metadata())
    {
        return (iFormatParserNode.Metadata())->GetNumMetadataValues(aKeyList);
    }
    return 0;
}


PVMFCommandId PVMFDownloadManagerNode::GetNodeMetadataKeys(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, uint32 starting_index, int32 max_entries, char* query_key, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetNodeMetadataKeys() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId, PVDLM_NODE_CMD_GETNODEMETADATAKEY, aKeyList, starting_index, max_entries, query_key, aContext);
    return QueueCommandL(cmd);
}


PVMFCommandId PVMFDownloadManagerNode::GetNodeMetadataValues(PVMFSessionId aSessionId, PVMFMetadataList& aKeyList, Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, uint32 starting_index, int32 max_entries, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetNodeMetadataValue() called"));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId, PVDLM_NODE_CMD_GETNODEMETADATAVALUE, aKeyList, aValueList, starting_index, max_entries, aContext);
    return QueueCommandL(cmd);
}

// From PVMFMetadataExtensionInterface
PVMFStatus PVMFDownloadManagerNode::ReleaseNodeMetadataKeys(PVMFMetadataList& keys,
        uint32 start ,
        uint32 end)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ReleaseNodeMetadataKeys() called"));
    if (iFormatParserNode.Metadata())
    {
        return iFormatParserNode.Metadata()->ReleaseNodeMetadataKeys(keys, start, end);
    }
    return PVMFFailure;
}

// From PVMFMetadataExtensionInterface
PVMFStatus PVMFDownloadManagerNode::ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
        uint32 start,
        uint32 end)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ReleaseNodeMetadataValues() called"));

    if (iFormatParserNode.Metadata())
    {
        return iFormatParserNode.Metadata()->ReleaseNodeMetadataValues(aValueList, start, end);
    }
    return PVMFFailure;
}

//public API from data source playback interface
PVMFCommandId PVMFDownloadManagerNode::SetDataSourcePosition(PVMFSessionId aSessionId, PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aActualNPT,
        PVMFTimestamp& aActualMediaDataTS,
        bool aSeekToSyncPoint,
        uint32 aStreamID,
        OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::SetDataSourcePosition: aTargetNPT=%d, aSeekToSyncPoint=%d, aContext=0x%x",
                     aTargetNPT, aSeekToSyncPoint, aContext));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId, PVDLM_NODE_CMD_SETDATASOURCEPOSITION, aTargetNPT, aActualNPT,
            aActualMediaDataTS, aSeekToSyncPoint, aStreamID, aContext);
    return QueueCommandL(cmd);
}

PVMFCommandId PVMFDownloadManagerNode::QueryDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aSeekPointBeforeTargetNPT,
        PVMFTimestamp& aSeekPointAfterTargetNPT,
        OsclAny* aContextData,
        bool aSeekToSyncPoint)
{
    // Implemented to complete interface file definition
    // Not tested on logical plane
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::QueryDataSourcePosition: aTargetNPT=%d, aSeekToSyncPoint=%d, aContext=0x%x", aTargetNPT,
                     aContextData, aSeekToSyncPoint));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId, PVDLM_NODE_CMD_QUERYDATASOURCEPOSITION, aTargetNPT, aSeekPointBeforeTargetNPT,
            aSeekToSyncPoint, aContextData);
    return QueueCommandL(cmd);
}

PVMFCommandId PVMFDownloadManagerNode::QueryDataSourcePosition(PVMFSessionId aSessionId, PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aActualNPT,
        bool aSeekToSyncPoint,
        OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::QueryDataSourcePosition: aTargetNPT=%d, aSeekToSyncPoint=%d, aContext=0x%x",
                     aTargetNPT, aSeekToSyncPoint, aContext));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId, PVDLM_NODE_CMD_QUERYDATASOURCEPOSITION, aTargetNPT, aActualNPT,
            aSeekToSyncPoint, aContext);
    return QueueCommandL(cmd);
}


PVMFCommandId PVMFDownloadManagerNode::SetDataSourceRate(PVMFSessionId aSessionId, int32 aRate, OsclTimebase* aTimebase, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::SetDataSourceRate: aRate=%d", aRate));

    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId, PVDLM_NODE_CMD_SETDATASOURCERATE, aRate, aTimebase, aContext);
    return QueueCommandL(cmd);
}

PVMFStatus PVMFDownloadManagerNode::SetPlayerNodeRegistry(PVPlayerNodeRegistryInterface* aRegistry)
{
    iPlayerNodeRegistry = aRegistry;
    return PVMFSuccess;
}

void PVMFDownloadManagerNode::Run()
{
    //Process async node commands.
    if (!iInputCommands.empty())
        ProcessCommand();

    //Issue commands to the sub-nodes.
    if (!iProtocolEngineNode.CmdPending()
            && !iFormatParserNode.CmdPending()
            && !iSocketNode.CmdPending()
            && !iRecognizerNode.CmdPending()
            && !iSubNodeCmdVec.empty())
    {
        PVMFStatus status = iSubNodeCmdVec.front().iNC->IssueCommand(iSubNodeCmdVec.front().iCmd);
        if (status != PVMFPending)
            iSubNodeCmdVec.front().iNC->CommandDone(status, NULL, NULL);
    }
}

PVMFCommandId PVMFDownloadManagerNode::QueueCommandL(PVMFDownloadManagerNodeCommand& aCmd)
{
    //add a command to the async node command queue and return command ID

    PVMFCommandId id = iInputCommands.AddL(aCmd);

    // Wakeup the AO
    RunIfNotReady();

    return id;
}

void PVMFDownloadManagerNode::ProcessCommand()
{
    //This call will process the first node command in the input queue.


    //Can't do anything when an asynchronous cancel is in progress -- just need to wait on completion.
    if (!iCancelCommand.empty())
        return; //keep waiting.

    //If a command is in progress, only a hi-pri command can interrupt it.
    if (!iCurrentCommand.empty()
            && !iInputCommands.front().hipri()
       )
    {
        return; //keep waiting
    }

    //The newest or highest pri command is in the front of the queue.
    Assert(!iInputCommands.empty());
    PVMFDownloadManagerNodeCommand& aCmd = iInputCommands.front();

    PVMFStatus cmdstatus;
    if (aCmd.hipri())
    {
        //Process the Hi-Pri commands.
        switch (aCmd.iCmd)
        {
            case PVMF_GENERIC_NODE_CANCELALLCOMMANDS:
                cmdstatus = DoCancelAllCommands(aCmd);
                break;

            case PVMF_GENERIC_NODE_CANCELCOMMAND:
                cmdstatus = DoCancelCommand(aCmd);
                break;

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
            case PVDLM_NODE_CMD_CANCEL_GET_LICENSE:
                cmdstatus = DoCancelGetLicense(aCmd);
                break;
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

            default:
                cmdstatus = PVMFErrNotSupported;
                break;
        }

        //If completion is pending, move the command from
        //the input queue to the cancel queue.
        //This is necessary since the input queue could get
        //rearranged by new commands coming in.
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
                cmdstatus = DoRequestPort(aCmd);
                break;

            case PVMF_GENERIC_NODE_RELEASEPORT:
                cmdstatus = DoReleasePort(aCmd);
                break;

            case PVMF_GENERIC_NODE_INIT:
                cmdstatus = DoInitNode(aCmd);
                break;

            case PVMF_GENERIC_NODE_PREPARE:
                cmdstatus = DoPrepareNode(aCmd);
                break;

            case PVMF_GENERIC_NODE_START:
                cmdstatus = DoStartNode(aCmd);
                break;

            case PVMF_GENERIC_NODE_STOP:
                cmdstatus = DoStopNode(aCmd);
                break;

            case PVMF_GENERIC_NODE_FLUSH:
                cmdstatus = DoFlushNode(aCmd);
                break;

            case PVMF_GENERIC_NODE_PAUSE:
                cmdstatus = DoPauseNode(aCmd);
                break;

            case PVMF_GENERIC_NODE_RESET:
                cmdstatus = DoResetNode(aCmd);
                break;

            case PVDLM_NODE_CMD_GETNODEMETADATAKEY:
                cmdstatus = DoGetNodeMetadataKey(aCmd);
                break;

            case PVDLM_NODE_CMD_GETNODEMETADATAVALUE:
                cmdstatus = DoGetNodeMetadataValue(aCmd);
                break;

            case PVDLM_NODE_CMD_SETDATASOURCEPOSITION:
                cmdstatus = DoSetDataSourcePosition(aCmd);
                break;

            case PVDLM_NODE_CMD_QUERYDATASOURCEPOSITION:
                cmdstatus = DoQueryDataSourcePosition(aCmd);
                break;

            case PVDLM_NODE_CMD_SETDATASOURCERATE:
                // Rate change not supported for download
                cmdstatus = PVMFErrNotSupported;
                break;

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
            case PVDLM_NODE_CMD_GET_LICENSE_W:
                cmdstatus = DoGetLicense(aCmd, true);
                break;

            case PVDLM_NODE_CMD_GET_LICENSE:
                cmdstatus = DoGetLicense(aCmd);
                break;
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

            default:
                Assert(false);
                cmdstatus = PVMFFailure;
                break;
        }

        //If completion is pending, move the command from the input queue to the current command.
        //This is necessary since the input queue could get rearranged by new commands coming in.
        if (cmdstatus == PVMFPending)
        {
            iCurrentCommand.StoreL(aCmd);
            iInputCommands.Erase(&aCmd);
        }
    }

    if (cmdstatus != PVMFPending)
        CommandComplete(iInputCommands, aCmd, cmdstatus, NULL, NULL);
}

void PVMFDownloadManagerNode::CommandComplete(PVMFDownloadManagerNodeCmdQueue& aCmdQ, PVMFDownloadManagerNodeCommand& aCmd, PVMFStatus aStatus,
        PVInterface*aExtMsg, OsclAny* aEventData)
{
    //Complete a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::CommandComplete() In Id %d Cmd %d Status %d Context %d Data %d",
                    aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    if (aStatus != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVMFDownloadManagerNode::CommandComplete() Failure!"));
    }

    //if the command failed or was cancelled there may be un-processed sub-node commands, so clear the vector now.
    if (!iSubNodeCmdVec.empty())
        iSubNodeCmdVec.clear();


    //Do the post-command state changes and anything else.
    if (aStatus == PVMFSuccess)
    {
        switch (aCmd.iCmd)
        {
            case PVMF_GENERIC_NODE_INIT:
                ChangeNodeState(EPVMFNodeInitialized);
                break;
            case PVMF_GENERIC_NODE_PREPARE:
                ChangeNodeState(EPVMFNodePrepared);
                break;
            case PVMF_GENERIC_NODE_START:
                ChangeNodeState(EPVMFNodeStarted);
                break;
            case PVMF_GENERIC_NODE_PAUSE:
                ChangeNodeState(EPVMFNodePaused);
                break;
            case PVMF_GENERIC_NODE_STOP:
                ChangeNodeState(EPVMFNodePrepared);
                break;
            case PVMF_GENERIC_NODE_FLUSH:
                ChangeNodeState(EPVMFNodePrepared);
                break;
            case PVMF_GENERIC_NODE_RESET:
                //drive this node back to Created state.
                ChangeNodeState(EPVMFNodeIdle);
                ThreadLogoff();
                break;
        }
    }

    //create response
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aExtMsg, aEventData);
    PVMFSessionId session = aCmd.iSession;

    //Erase the command from the queue.
    aCmdQ.Erase(&aCmd);

    //Report completion to the session observer.
    ReportCmdCompleteEvent(session, resp);

    //re-schedule if there are more commands and node isn't logged off
    if (!iInputCommands.empty()
            && IsAdded())
        RunIfNotReady();
}


void PVMFDownloadManagerNode::ReportErrorEvent(PVMFEventType aEventType, PVInterface*aExtMsg, OsclAny* aEventData)
{
    //Report a node error event

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ReportErrorEvent() In Type %d Data %d ExtMsg %d",
                    aEventType, aEventData, aExtMsg));

    PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData, aExtMsg);
}


void PVMFDownloadManagerNode::ReportInfoEvent(PVMFAsyncEvent &aEvent)
{
    //Report a node info event

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ReportInfoEvent() In Type %d Data %d ExtMsg %d",
                    aEvent.GetEventType(), aEvent.GetEventData(), aEvent.GetEventExtensionInterface()));

    PVMFNodeInterface::ReportInfoEvent(aEvent);

    //For download-then-play mode, generate data ready event when buffering
    //is complete.  We will have suppressed the real initial data ready
    //event from PE node in this case.
    if (aEvent.GetEventType() == PVMFInfoBufferingComplete
            && iPlaybackMode == PVMFDownloadManagerNode::EDownloadThenPlay
            && !iDataReady)
    {
        GenerateDataReadyEvent();
    }
}

void PVMFDownloadManagerNode::GenerateDataReadyEvent()
{
    PVMFAsyncEvent info(PVMFInfoEvent, PVMFInfoDataReady, NULL, NULL);
    ReportInfoEvent(info);
    iDataReady = true;
}

bool PVMFDownloadManagerNode::FilterPlaybackEventsFromSubNodes(const PVMFAsyncEvent& aEvent)
{
    switch (aEvent.GetEventType())
    {
        case PVMFInfoUnderflow:
            //filter any underflow that happens before data ready
            if (!iDataReady)
                return true;
            else
                iDataReady = false;
            break;
        case PVMFInfoDataReady:
            //filter any data ready that happens before download complete
            //in dl-then-play mode
            if (iPlaybackMode == EDownloadThenPlay
                    && !iDownloadComplete)
            {
                return true;
            }
            //filter any data ready in dl-only mode, though I don't
            //think it's possible.
            if (iPlaybackMode == EDownloadOnly)
                return true;

            iDataReady = true;

            break;
        case PVMFInfoRemoteSourceNotification:
            //we get this event for "not pseudostreamable" for both fast-track
            //and 3gpp.  Only pass it up for 3gpp.
            if (iSourceFormat != PVMF_DATA_SOURCE_HTTP_URL)
                return true;
            break;
        default:
            break;
    }
    return false;
}

void PVMFDownloadManagerNode::ChangeNodeState(TPVMFNodeInterfaceState aNewState)
{
    //Update the node state

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::ChangeNodeState() Old %d New %d", iInterfaceState, aNewState));

    PVMFNodeInterface::SetState(aNewState);
}


PVMFStatus PVMFDownloadManagerNode::DoQueryUuid(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoQueryUuid() In"));

    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;
    aCmd.PVMFDownloadManagerNodeCommandBase::Parse(mimetype, uuidvec, exactmatch);

    // TODO Add MIME string matching
    // For now just return all available extension interface UUID
    uuidvec->push_back(PVMF_TRACK_SELECTION_INTERFACE_UUID);
    uuidvec->push_back(PVMF_DATA_SOURCE_INIT_INTERFACE_UUID);
    uuidvec->push_back(KPVMFMetadataExtensionUuid);
    uuidvec->push_back(PvmfDataSourcePlaybackControlUuid);
    uuidvec->push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID);

    return PVMFSuccess;
}


PVMFStatus PVMFDownloadManagerNode::DoQueryInterface(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoQueryInterface() In"));

    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.PVMFDownloadManagerNodeCommandBase::Parse(uuid, ptr);

    if (queryInterface(*uuid, *ptr))
    {
        //Schedule further queries on sub-nodes...
        return ScheduleSubNodeCommands(aCmd);
    }
    else
    {
        //interface not supported
        *ptr = NULL;
        return PVMFFailure;
    }
}

PVMFStatus PVMFDownloadManagerNode::DoRequestPort(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoRequestPort() In"));

    if (iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoReleasePort(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoReleasePort() In"));

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoInitNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoInitNode() In"));

    if (iInterfaceState != EPVMFNodeIdle)
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoPrepareNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoPrepareNode() In"));

    if (iInterfaceState != EPVMFNodeInitialized)
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoStartNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoStartNode() In"));

    if (iInterfaceState != EPVMFNodePrepared
            && iInterfaceState != EPVMFNodePaused)
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoStopNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoStopNode() In"));

    if (iInterfaceState != EPVMFNodeStarted
            && iInterfaceState != EPVMFNodePaused
            && iInterfaceState != EPVMFNodeError)//allow a stop in error state.
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoFlushNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoFlushNode() In"));

    if (iInterfaceState != EPVMFNodeStarted
            && iInterfaceState != EPVMFNodePaused)
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoPauseNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoPauseNode() In"));

    if (iInterfaceState != EPVMFNodeStarted)
        return PVMFErrInvalidState;

    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoResetNode(PVMFDownloadManagerNodeCommand& aCmd)
{
    //remove the clock observer
    if (iPlayBackClock != NULL)
    {
        iPlayBackClock->RemoveClockStateObserver(*this);
    }

    //Start executing a node command
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoResetNode() In"));

    if (iInterfaceState == EPVMFNodeStarted
            || iInterfaceState == EPVMFNodePaused)
        return PVMFErrInvalidState;

    //Reset the sub-nodes first.
    return ScheduleSubNodeCommands(aCmd);
}

PVMFStatus PVMFDownloadManagerNode::DoCancelAllCommands(PVMFDownloadManagerNodeCommand& aCmd)
{
    OSCL_UNUSED_ARG(aCmd);
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoCancelAllCommands() In"));

    //first cancel the current command if any
    while (!iCurrentCommand.empty())
    {
        if (iFormatParserNode.CancelPendingCommand()
                || iProtocolEngineNode.CancelPendingCommand()
                || iSocketNode.CancelPendingCommand()
                || iRecognizerNode.CancelPendingCommand()
           )
        {
            return PVMFPending;//wait on sub-node cancel to complete.
        }
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFErrCancelled, NULL, NULL);
    }

    //next cancel all queued commands
    //start at element 1 since this cancel command is element 0.
    while (iInputCommands.size() > 1)
    {
        CommandComplete(iInputCommands, iInputCommands[1], PVMFErrCancelled, NULL, NULL);
    }

    return PVMFSuccess;
}

PVMFStatus PVMFDownloadManagerNode::DoCancelCommand(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoCancelCommand() In"));

    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.PVMFDownloadManagerNodeCommandBase::Parse(id);

    //first check "current" command if any
    PVMFDownloadManagerNodeCommand* cmd = iCurrentCommand.FindById(id);
    if (cmd)
    {
        if (iFormatParserNode.CancelPendingCommand()
                || iProtocolEngineNode.CancelPendingCommand()
                || iRecognizerNode.CancelPendingCommand()
           )
        {
            return PVMFPending;//wait on sub-node cancel to complete.
        }
        CommandComplete(iCurrentCommand, *cmd, PVMFErrCancelled, NULL, NULL);
        return PVMFSuccess;
    }

    //next check input queue.
    //start at element 1 since this cancel command is element 0.
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


PVMFStatus PVMFDownloadManagerNode::DoGetNodeMetadataKey(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoGetNodeMetadataKey() In"));

    return ScheduleSubNodeCommands(aCmd);
}



PVMFStatus PVMFDownloadManagerNode::DoGetNodeMetadataValue(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoGetNodeMetadataValue() In"));

    return ScheduleSubNodeCommands(aCmd);
}



PVMFStatus PVMFDownloadManagerNode::DoSetDataSourcePosition(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoSetDataSourcePosition() In"));

    return ScheduleSubNodeCommands(aCmd);
}


PVMFStatus PVMFDownloadManagerNode::DoQueryDataSourcePosition(PVMFDownloadManagerNodeCommand& aCmd)
{
    //Start executing a node command

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoQueryDataSourcePosition() In"));

    return ScheduleSubNodeCommands(aCmd);
}

void PVMFDownloadManagerNode::ContinueInitAfterTrackSelectDecision()
{
    //this is called during the Init sequence, once we have enough information
    //to make a definite track select decision.

    //See whether we need to stop to allow track selection on the download server.
    //If it's download-only, we don't offer this option, since the download must
    //be started in the Init.
    if (iPlaybackMode != EDownloadOnly
            && TrackSelectNode().iType == PVMFDownloadManagerSubNodeContainerBase::EProtocolEngine)
    {
        //stop the Init sequence here so we can do track selection on the download
        //server.
        ;
    }
    else
    {
        //else download-only, or there's no track selection available from the
        //PE node.  Continue the Init or Prepare sequence.
        ContinueFromDownloadTrackSelectionPoint();
    }
}

void PVMFDownloadManagerNode::ContinueFromDownloadTrackSelectionPoint()
{
    //Continue the Init or Prepare sequence, stopping at parser init.

    //start the download.
    Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EStart);

    //initiate file recognize & parse, unless this is download-only mode.
    if (iPlaybackMode != EDownloadOnly)
    {
        //do recognizer sequence if needed.
        {
            Push(iRecognizerNode, PVMFDownloadManagerSubNodeContainerBase::ERecognizerStart);
            Push(iRecognizerNode, PVMFDownloadManagerSubNodeContainerBase::ERecognizerClose);
        }

        //create parser
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EParserCreate);

        // Send commands to the parser node to query these extension interfaces.
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDataSourceInit);
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryTrackSelection);
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryMetadata);
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDatastreamUser);
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDataSourcePlayback);
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryFFProgDownload);
        Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::ESetFFProgDownloadSupport);
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::ECPMQueryLicenseInterface);
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

        {
            //for 3gpp, go ahead and init parser.  Init will block until
            //receiving movie atom.
            iParserInit = true;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EInit);
        }
    }
}


PVMFNodeInterface* PVMFDownloadManagerNode::CreateParser()
{
    if (!(iMimeType == PVMF_MIME_FORMAT_UNKNOWN))
    {
        PVMFNodeInterface *iSourceNode = NULL;
        PVMFFormatType outputFormatType = PVMF_FORMAT_UNKNOWN;
        iFmt = GetFormatIndex(iMimeType.get_str());
        PVMFStatus status =
            iPlayerNodeRegistry->QueryRegistry(iFmt, outputFormatType, iDNodeUuids);
        if ((status == PVMFSuccess) && (iDNodeUuids.size() > 0))
        {
            int32 leavecode = 0;
            OSCL_TRY(leavecode, iSourceNode = iPlayerNodeRegistry->CreateNode(iDNodeUuids[iDNodeUuidCount]));
            OSCL_FIRST_CATCH_ANY(leavecode, return NULL);
            iDNodeUuidCount++;
            return iSourceNode;
        }
    }
    return NULL;
}

PVMFStatus PVMFDownloadManagerNode::ScheduleSubNodeCommands(PVMFDownloadManagerNodeCommand& aCmd)
{
    //given the node command ID, create the sub-node command vector, initiate the processing and return the node command status.

    Assert(iSubNodeCmdVec.empty());

    //Create the vector of all the commands in the sequence.
    switch (aCmd.iCmd)
    {

        case PVMF_GENERIC_NODE_QUERYINTERFACE:
        {
            //When we get here we've already called queryInterface on this node
            //for the interface.  This code schedules any additional sub-node commands
            //that are needed to support the interface.

            //extract uuid from Node command...
            PVUuid*aUuid;
            PVInterface**aInterface;
            aCmd.PVMFDownloadManagerNodeCommandBase::Parse(aUuid, aInterface);
            Assert(aUuid != NULL);

            if (*aUuid == PVMF_DATA_SOURCE_INIT_INTERFACE_UUID)
            {
                //To support data source init interface we need a bunch of sub-node interfaces.
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EQueryProtocolEngine);
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDatastreamUser);
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDataSourceInit);
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDownloadProgress);
            }
            else
            {
                //nothing else needed for any other interface.
                return PVMFSuccess;
            }
        }
        break;

        case PVMF_GENERIC_NODE_INIT:
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
            //check for second "Init" command after a license acquire.
            if (iInitFailedLicenseRequired)
            {
                iParserInit = true;
                Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EInit);
            }
            else
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE
            {
                //reset any prior download/playback event
                iDownloadComplete = false;
                iParserInit = false;
                iDataReady = false;
                iRecognizerError = false;
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
                iInitFailedLicenseRequired = false;
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

                //reset any prior track select decisions.
                iFormatParserNode.iTrackSelection = NULL;

                //reset any prior recognizer decisions.
                iMimeType = PVMF_MIME_FORMAT_UNKNOWN;

                // Send the INIT command to the protocol engine node, followed by the Socket Node.
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EInit);
                Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::EInit);
                // Issue the port request to the Protocol Engine Node and the socket node
                // NOTE: The request for the socket node's port must come first, followed by the protocol node,
                // because the code to connect the two ports in CommandDone() will do so when the protocol node's port is returned.
                Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::ERequestPort);
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::ERequestPort);
                // The two ports will be connected in CommandDone, when the 2nd port request completes.
                // After the ports are connected, the datastream factory is passed to the protocol engine node.
                Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::EPrepare);
                Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::EStart);
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EPrepare);

                if (TrackSelectNode().iType == PVMFDownloadManagerSubNodeContainerBase::EFormatParser)
                {
                    //parser is doing track selection, there's no question
                    ContinueInitAfterTrackSelectDecision();
                }
            }
            break;

        case PVMF_GENERIC_NODE_PREPARE:
            //if protocol engine node did track selection, then we need to continue
            //to the file parse stage here.  Otherwise it was already done in the Init.
            if (TrackSelectNode().iType == PVMFDownloadManagerSubNodeContainerBase::EProtocolEngine)
            {
                ContinueFromDownloadTrackSelectionPoint();
            }
            //if we initiated file parse sequence already, then go ahead and prepare
            //the parser node.
            if (iParserInit)
            {
                Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EPrepare);
            }
            break;

        case PVMF_GENERIC_NODE_REQUESTPORT:
            //if file isn't parsed (as in download-only), then fail command
            if (!iFormatParserNode.iNode)
                return PVMFErrNotSupported;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::ERequestPort);
            break;

        case PVMF_GENERIC_NODE_RELEASEPORT:
            //if file isn't parsed (as in download-only), then fail command
            if (!iFormatParserNode.iNode)
                return PVMFErrNotSupported;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EReleasePort);
            break;

        case PVMF_GENERIC_NODE_START:
            //if file isn't parsed (as in download-only), just ignore command
            if (!iFormatParserNode.iNode)
                return PVMFSuccess;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EStart);
            break;

        case PVMF_GENERIC_NODE_STOP:
            //just stop parser here.
            //if I stop socket and protocol engine, then re-start hangs.
            //tbd, would be nice to be able to stop & re-start the download.
            //if file isn't parsed (as in download-only), just ignore command
            if (!iFormatParserNode.iNode)
                return PVMFSuccess;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EStop);
            break;

        case PVMF_GENERIC_NODE_FLUSH:
            //if file isn't parsed (as in download-only), just ignore command
            if (!iFormatParserNode.iNode)
                return PVMFSuccess;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EFlush);
            break;

        case PVMF_GENERIC_NODE_PAUSE:
            //note: pause/resume download is not supported.
            //if file isn't parsed (as in download-only), just ignore command
            if (!iFormatParserNode.iNode)
                return PVMFSuccess;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EPause);
            break;

        case PVMF_GENERIC_NODE_RESET:
            if (iSocketNode.iNode->GetState() == EPVMFNodeStarted
                    || iSocketNode.iNode->GetState() == EPVMFNodePaused)
            {
                Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::EStop);
            }
            if (iProtocolEngineNode.iNode->GetState() == EPVMFNodeStarted
                    || iProtocolEngineNode.iNode->GetState() == EPVMFNodePaused)
            {
                Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EStop);
            }
            if (iFormatParserNode.iNode)
                Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EReset);
            Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::EReset);
            Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::EReset);
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::ECleanup);
            Push(iProtocolEngineNode, PVMFDownloadManagerSubNodeContainerBase::ECleanup);
            Push(iSocketNode, PVMFDownloadManagerSubNodeContainerBase::ECleanup);
            break;

        case PVDLM_NODE_CMD_SETDATASOURCEPOSITION:
            //if file isn't parsed (as in download-only), then fail command
            if (!iFormatParserNode.iNode)
                return PVMFErrNotSupported;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::ESetDataSourcePosition);
            break;

        case PVDLM_NODE_CMD_QUERYDATASOURCEPOSITION:
            //if file isn't parsed (as in download-only), then fail command
            if (!iFormatParserNode.iNode)
                return PVMFErrNotSupported;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EQueryDataSourcePosition);
            break;

        case PVDLM_NODE_CMD_GETNODEMETADATAKEY:
            //if file isn't parsed (as in download-only), then fail command
            if (!iFormatParserNode.iNode)
                return PVMFErrNotSupported;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EGetMetadataKey);
            break;

        case PVDLM_NODE_CMD_GETNODEMETADATAVALUE:
            //if file isn't parsed (as in download-only), then fail command
            if (!iFormatParserNode.iNode)
                return PVMFErrNotSupported;
            Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::EGetMetadataValue);
            break;

        default:
            Assert(false);
            break;
    }

    if (iSubNodeCmdVec.empty())
    {
        //in a few cases there's nothing needed and no new commands
        //were issued-- so succeed here.
        return PVMFSuccess;
    }
    else
    {
        //Wakeup the node to start issuing the sub-node commands.
        RunIfNotReady();

        //the node command is pending.
        return PVMFPending;
    }
}

void PVMFDownloadManagerNode::Push(PVMFDownloadManagerSubNodeContainerBase& n, PVMFDownloadManagerSubNodeContainerBase::CmdType c)
{
    //push a sub-node command onto the cmd vector
    CmdElem elem;
    elem.iCmd = c;
    elem.iNC = &n;
    iSubNodeCmdVec.push_back(elem);
}

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
PVMFCommandId
PVMFDownloadManagerNode::GetLicense(PVMFSessionId aSessionId,
                                    OSCL_wString& aContentName,
                                    OsclAny* aData,
                                    uint32 aDataSize,
                                    int32 aTimeoutMsec,
                                    OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetLicense - Wide called"));
    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId,
            PVDLM_NODE_CMD_GET_LICENSE_W,
            aContentName,
            aData,
            aDataSize,
            aTimeoutMsec,
            aContextData);
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFDownloadManagerNode::GetLicense(PVMFSessionId aSessionId,
                                    OSCL_String&  aContentName,
                                    OsclAny* aData,
                                    uint32 aDataSize,
                                    int32 aTimeoutMsec,
                                    OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetLicense - Non-Wide called"));
    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommand::Construct(aSessionId,
            PVDLM_NODE_CMD_GET_LICENSE,
            aContentName,
            aData,
            aDataSize,
            aTimeoutMsec,
            aContextData);
    return QueueCommandL(cmd);
}


PVMFCommandId
PVMFDownloadManagerNode::CancelGetLicense(PVMFSessionId aSessionId
        , PVMFCommandId aCmdId
        , OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::GetLicense - Non-Wide called"));
    PVMFDownloadManagerNodeCommand cmd;
    cmd.PVMFDownloadManagerNodeCommandBase::Construct(aSessionId,
            PVDLM_NODE_CMD_CANCEL_GET_LICENSE,
            aCmdId,
            aContextData);
    return QueueCommandL(cmd);
}

PVMFStatus PVMFDownloadManagerNode::DoGetLicense(PVMFDownloadManagerNodeCommand& aCmd,
        bool aWideCharVersion)
{
    if (iFormatParserNode.LicenseInterface() == NULL)
    {
        return PVMFErrNotSupported;
    }

    if (aWideCharVersion == true)
    {
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::ECPMGetLicenseW);
    }
    else
    {
        Push(iFormatParserNode, PVMFDownloadManagerSubNodeContainerBase::ECPMGetLicense);
    }
    RunIfNotReady();
    return PVMFPending;
}

void PVMFDownloadManagerNode::CompleteGetLicense()
{
    CommandComplete(iCurrentCommand,
                    iCurrentCommand.front(),
                    PVMFSuccess, NULL, NULL);
}

PVMFStatus PVMFDownloadManagerNode::DoCancelGetLicense(PVMFDownloadManagerNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerNode::DoCancelGetLicense called"));
    if (iFormatParserNode.LicenseInterface() == NULL)
    {
        return PVMFErrNotSupported;
    }
    else
    {
        iFormatParserNode.iCancelCmdState = PVMFDownloadManagerSubNodeContainerBase::EBusy;
        iFormatParserNode.iCPMCancelGetLicenseCmdId =
            iFormatParserNode.LicenseInterface()->CancelGetLicense(iFormatParserNode.iSessionId, iFormatParserNode.iCPMGetLicenseCmdId);
        RunIfNotReady();
    }
    return PVMFPending;
}
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

//
// PVMFDownloadManagerSubNodeContainer Implementation.
//

PVMFDownloadManagerSubNodeContainerBase::PVMFDownloadManagerSubNodeContainerBase()
{
    iCmdState = EIdle;
    iCancelCmdState = EIdle;
}

void PVMFDownloadManagerSubNodeContainerBase::Construct(NodeType t, PVMFDownloadManagerNode* c)
{
    iContainer = c;
    iType = t;
}

void PVMFDownloadManagerSubNodeContainer::Cleanup()
{
    //release all the queried interfaces.
    if (iDataSourceInit)
    {
        iDataSourceInit->removeRef();
        iDataSourceInit = NULL;
    }
    if (iProtocolEngineExtensionInt)
    {
        iProtocolEngineExtensionInt->removeRef();
        iProtocolEngineExtensionInt = NULL;
    }
    if (iDatastreamUser)
    {
        iDatastreamUser->removeRef();
        iDatastreamUser = NULL;
    }
    if (iTrackSelection)
    {
        iTrackSelection->removeRef();
        iTrackSelection = NULL;
    }
    if (iMetadata)
    {
        iMetadata->removeRef();
        iMetadata = NULL;
    }
    if (iDataSourcePlayback)
    {
        iDataSourcePlayback->removeRef();
        iDataSourcePlayback = NULL;
    }
    if (iFormatProgDownloadSupport)
    {
        iFormatProgDownloadSupport->removeRef();
        iFormatProgDownloadSupport = NULL;
    }
    if (iDownloadProgress)
    {
        iDownloadProgress->removeRef();
        iDownloadProgress = NULL;
    }
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
    if (iLicenseInterface)
    {
        iLicenseInterface->removeRef();
        iLicenseInterface = NULL;
    }
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE
    //the node instance is cleaned up elsewhere.
}

void PVMFDownloadManagerRecognizerContainer::Cleanup()
{
    // Nothing to do here 'til recognizer is integrated
}

void PVMFDownloadManagerSubNodeContainer::Connect()
{
    //Issue connect command to the sub-node.

    //This container class is the observer.
    PVMFNodeSessionInfo info(this //cmd
                             , this, NULL //info
                             , this, NULL); //err

    if (iNode)
        iSessionId = iNode->Connect(info);
}

#define LOGTRACE(x) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE, (0,x))

PVMFStatus PVMFDownloadManagerSubNodeContainer::IssueCommand(int32 aCmd)
{
    //Issue a command to the sub-node.
    //Return the sub-node completion status-- either pending, success, or failure.

    if (iType == EFormatParser)
    {
        LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand to PARSER () In");
    }
    else if (iType == EProtocolEngine)
    {
        LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand to PROTOCOLENGINE () In");
    }
    else if (iType == ESocket)
    {
        LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand to SOCKET () In");
    }

    Assert(!CmdPending());

    //find the current node command since we may need its parameters.

    Assert(!iContainer->iCurrentCommand.empty());
    PVMFDownloadManagerNodeCommand* nodeCmd = &iContainer->iCurrentCommand.front();

    //save the sub-node command code
    iCmd = aCmd;

    switch (aCmd)
    {
        case ECleanup:
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Cleanup");
            Cleanup();
            return PVMFSuccess;

        case EParserCreate:
            iNode = iContainer->CreateParser();
            if (iNode)
            {
                Connect();
                iNode->ThreadLogon();
                return PVMFSuccess;
            }
            return PVMFErrCorrupt;

        case EQueryDataSourceInit:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface(data source init)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PVMF_DATA_SOURCE_INIT_INTERFACE_UUID, iDataSourceInit);
            return PVMFPending;

        case EQueryProtocolEngine:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface(ProtocolEngine)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, KPVMFProtocolEngineNodeExtensionUuid, iProtocolEngineExtensionInt);
            return PVMFPending;

        case EQueryDatastreamUser:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface(DatastreamUser)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PVMIDatastreamuserInterfaceUuid, iDatastreamUser);
            return PVMFPending;

        case EQueryTrackSelection:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface(track selection)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PVMF_TRACK_SELECTION_INTERFACE_UUID, iTrackSelection);
            return PVMFPending;

        case EQueryMetadata:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface (metadata)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, KPVMFMetadataExtensionUuid, iMetadata);
            return PVMFPending;

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
        case ECPMQueryLicenseInterface:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface (metadata)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PVMFCPMPluginLicenseInterfaceUuid, iLicenseInterface);
            return PVMFPending;
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

        case EQueryDataSourcePlayback:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface (datasourcePB)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PvmfDataSourcePlaybackControlUuid, iDataSourcePlayback);
            return PVMFPending;

        case EInit:
            Assert(iNode != NULL);
            if (iType == EFormatParser)
            {
                // For this command, which gets pushed to the format parser node, we set the source init and also
                // set the datstream factory
                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand issuing SetSourceInitializationData to format parser node.");

                if (!DataSourceInit())
                    return PVMFFailure; //no source init interface?
                if (!DatastreamUser())
                    return PVMFFailure; //no datastreamuser interface?

                //Pass data to the parser node.
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
                if (iContainer->iInitFailedLicenseRequired)
                {
                    ;//do nothing-- data was already set on the first init call.
                }
                else
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE
                {
                    //Pass source data
                    //for 3gpp, pass the recognized file format to the parser node.
                    {
                        (DataSourceInit())->SetSourceInitializationData(iContainer->iDownloadFileName
                                , iContainer->iFmt
                                , (OsclAny*)iContainer->iSourceData);
                    }

                    //Pass datastream data.
                    (DatastreamUser())->PassDatastreamFactory(*(iContainer->iReadFactory), (int32)0);
                    PVMFFileBufferDataStreamWriteDataStreamFactoryImpl* wdsfactory =
                        OSCL_STATIC_CAST(PVMFFileBufferDataStreamWriteDataStreamFactoryImpl*, iContainer->iWriteFactory);
                    PVMFDataStreamReadCapacityObserver* obs =
                        OSCL_STATIC_CAST(PVMFDataStreamReadCapacityObserver*, wdsfactory);
                    int32 leavecode = 0;
                    OSCL_TRY(leavecode, (DatastreamUser())->PassDatastreamReadCapacityObserver(obs));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         LOGTRACE("PVMFDownloadManagerNode::IssueCommand() - PassDatastreamReadCapacityObserver Not supported"););
                }

                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Init on Format Parser Node");
                iCmdState = EBusy;
                iCmdId = iNode->Init(iSessionId);
                return PVMFPending;
            }
            else
            {
                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Init");
                iCmdState = EBusy;
                iCmdId = iNode->Init(iSessionId);
                return PVMFPending;
            }

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
        case ECPMGetLicenseW:
        {
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling ECPMGetLicenseW");
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
                LicenseInterface()->GetLicense(iSessionId,
                                               *contentName,
                                               data,
                                               dataSize,
                                               timeoutMsec);
            iCPMGetLicenseCmdId = iCmdId;

            return PVMFPending;
        }
        case ECPMGetLicense:
        {
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling ECPMGetLicense");
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
                LicenseInterface()->GetLicense(iSessionId,
                                               *contentName,
                                               data,
                                               dataSize,
                                               timeoutMsec);
            iCPMGetLicenseCmdId = iCmdId;

            return PVMFPending;
        }
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

        case ERequestPort:
            Assert(iNode != NULL);
            // The parameters to RequestPort vary depending on which node we're getting a port from, so we switch on it.
            switch (iType)
            {
                case EProtocolEngine:
                    LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling RequestPort to Protocol Engine Node");
                    iCmdState = EBusy;
                    // For protocol engine port request, we don't need port tag or config info because it's the only port we ask it for.
                    iCmdId = iNode->RequestPort(iSessionId, (int32)0);
                    return PVMFPending;

                case ESocket:
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_DEBUG, (0,
                                    "PVMFDownloadManagerSubNodeContainer::IssueCommand Calling RequestPort to socket node with port config %s", iContainer->iServerAddr.get_cstr()));
                    iCmdState = EBusy;
                    //append a mimestring to the port for socket node logging
                    iContainer->iServerAddr += ";mime=download";
                    iCmdId = iNode->RequestPort(iSessionId, PVMF_SOCKET_NODE_PORT_TYPE_PASSTHRU, &iContainer->iServerAddr);
                    return PVMFPending;

                case EFormatParser:
                    //extract params from current Node command.
                    Assert(nodeCmd->iCmd == PVMF_GENERIC_NODE_REQUESTPORT);
                    {
                        int32 aPortTag;
                        OSCL_String*aMimetype;
                        nodeCmd->PVMFDownloadManagerNodeCommandBase::Parse(aPortTag, aMimetype);

                        LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling RequestPort to Format Parser Node");
                        iCmdState = EBusy;
                        iCmdId = iNode->RequestPort(iSessionId, aPortTag, aMimetype);
                    }
                    return PVMFPending;

                default:
                    Assert(false);
                    return PVMFFailure;
            }

        case EReleasePort:
            Assert(iNode != NULL);
            {
                //extract params from current Node command.
                Assert(nodeCmd->iCmd == PVMF_GENERIC_NODE_RELEASEPORT);
                PVMFPortInterface *port;
                nodeCmd->PVMFDownloadManagerNodeCommandBase::Parse(port);
                Assert(port != NULL);

                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling ReleasePort");
                iCmdState = EBusy;
                iCmdId = iNode->ReleasePort(iSessionId, *port);
                return PVMFPending;
            }

        case EPrepare:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Prepare");
            iCmdState = EBusy;
            iCmdId = iNode->Prepare(iSessionId);
            return PVMFPending;

        case EStop:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Stop");
            iCmdState = EBusy;
            iCmdId = iNode->Stop(iSessionId);
            return PVMFPending;

        case EStart:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Start");
            iCmdState = EBusy;
            iCmdId = iNode->Start(iSessionId);
            return PVMFPending;

        case EPause:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Pause");
            iCmdState = EBusy;
            iCmdId = iNode->Pause(iSessionId);
            return PVMFPending;

        case EFlush:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Flush");
            iCmdState = EBusy;
            iCmdId = iNode->Flush(iSessionId);
            return PVMFPending;

        case EReset:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling Reset");
            iCmdState = EBusy;
            iCmdId = iNode->Reset(iSessionId);
            return PVMFPending;

        case EGetMetadataKey:
            Assert(iNode != NULL);
            {
                if (!Metadata())
                    return PVMFErrNotSupported;//no interface!

                //extract params from current Node command.
                Assert(nodeCmd->iCmd == PVDLM_NODE_CMD_GETNODEMETADATAKEY);

                PVMFMetadataList* aKeyList;
                uint32 starting_index;
                int32 max_entries;
                char* query_key;

                nodeCmd->Parse(aKeyList, starting_index, max_entries, query_key);
                Assert(aKeyList != NULL);
                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling GetMetadataKey");
                iCmdState = EBusy;
                iCmdId = (Metadata())->GetNodeMetadataKeys(iSessionId, *aKeyList, starting_index, max_entries, query_key, NULL);

                return PVMFPending;
            }


        case EGetMetadataValue:
            Assert(iNode != NULL);
            {
                if (!Metadata())
                    return PVMFErrNotSupported;//no interface!

                //extract params from current Node command.
                Assert(nodeCmd->iCmd == PVDLM_NODE_CMD_GETNODEMETADATAVALUE);
                PVMFMetadataList* aKeyList;
                Oscl_Vector<PvmiKvp, OsclMemAllocator>* aValueList;
                uint32 starting_index;
                int32 max_entries;
                nodeCmd->Parse(aKeyList, aValueList, starting_index, max_entries);
                Assert(aKeyList != NULL);
                Assert(aValueList != NULL);

                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling GetMetadataValue");
                iCmdState = EBusy;
                iCmdId = (Metadata())->GetNodeMetadataValues(iSessionId, *aKeyList, *aValueList, starting_index, max_entries, NULL);
                return PVMFPending;
            }

        case EQueryFFProgDownload:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface (format prog dl)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PVMF_FF_PROGDOWNLOAD_SUPPORT_INTERFACE_UUID, iFormatProgDownloadSupport);
            return PVMFPending;

        case EQueryDownloadProgress:
            Assert(iNode != NULL);
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryInterface (dl prog)");
            iCmdState = EBusy;
            iCmdId = iNode->QueryInterface(iSessionId, PVMF_DOWNLOAD_PROGRESS_INTERFACE_UUID, iDownloadProgress);
            return PVMFPending;

        case ESetFFProgDownloadSupport:
            Assert(iNode != NULL);

            if (!DownloadProgress() || !iContainer->iFormatParserNode.FormatProgDownloadSupport())
                return PVMFErrNotSupported;//no interface!

            //pass parser node format prog download interface to the protocol node.
            LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling setFormatDownloadSupportInterface");
            (DownloadProgress())->setFormatDownloadSupportInterface(iContainer->iFormatParserNode.FormatProgDownloadSupport());
            return PVMFSuccess;

        case ESetDataSourcePosition:
            Assert(iNode != NULL);
            {
                if (!DataSourcePlayback())
                    return PVMFErrNotSupported;//no interface!

                //extract params from current Node command.
                Assert(nodeCmd->iCmd == PVDLM_NODE_CMD_SETDATASOURCEPOSITION);
                PVMFTimestamp aTargetNPT;
                PVMFTimestamp* aActualNPT;
                PVMFTimestamp* aActualMediaDataTS;
                uint32 streamID = 0;
                bool aJump;
                nodeCmd->Parse(aTargetNPT, aActualNPT, aActualMediaDataTS, aJump, streamID);
                Assert(aActualNPT != NULL);
                Assert(aActualMediaDataTS != NULL);

                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling SetDataSourcePosition");
                iCmdState = EBusy;
                iCmdId = (DataSourcePlayback())->SetDataSourcePosition(iSessionId, aTargetNPT, *aActualNPT, *aActualMediaDataTS, aJump, streamID);
                return PVMFPending;
            }

        case EQueryDataSourcePosition:
            Assert(iNode != NULL);
            {
                if (!DataSourcePlayback())
                    return PVMFErrNotSupported;//no interface!

                //extract params from current Node command.
                Assert(nodeCmd->iCmd == PVDLM_NODE_CMD_QUERYDATASOURCEPOSITION);
                PVMFTimestamp aTargetNPT;
                PVMFTimestamp* aActualNPT;
                bool aJump;
                nodeCmd->Parse(aTargetNPT, aActualNPT, aJump);
                Assert(aActualNPT != NULL);

                LOGTRACE("PVMFDownloadManagerSubNodeContainer::IssueCommand Calling QueryDataSourcePosition");
                iCmdState = EBusy;
                iCmdId = (DataSourcePlayback())->QueryDataSourcePosition(iSessionId, aTargetNPT, *aActualNPT, aJump);
                return PVMFPending;
            }

        default:
            Assert(false);
            return PVMFFailure;
    }
}


PVMFStatus PVMFDownloadManagerRecognizerContainer::IssueCommand(int32 aCmd)
{
    //Issue a command to the Recognizer.
    //Return the completion status-- either pending, success, or failure.

    LOGTRACE("PVMFDownloadManagerRecognizerContainer::IssueCommand In");

    Assert(!CmdPending());

    //save the sub-node command code
    iCmd = aCmd;

    switch (aCmd)
    {
        case ERecognizerStart:
        {
            PVMFStatus status = PVMFRecognizerRegistry::OpenSession(iRecognizerSessionId, (*this));
            if (status == PVMFSuccess)
            {
                //Issue the asynchronous command to the recognizer.
                iCmdState = EBusy;
                iCmdId = PVMFRecognizerRegistry::Recognize(iRecognizerSessionId,
                         *(iContainer->iReadFactory),
                         NULL,
                         iRecognizerResultVec);
                LOGTRACE("PVMFDownloadManagerRecognizerContainer::IssueCommand Recognize Pending");
                return PVMFPending;
                //wait on the RecognizerCommandCompleted callback.
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                                "PVMFDownloadManagerRecognizerContainer::IssueCommand Open Session Failed, status %d", status));
            }
            return status;
        }
        // break;	This statement was removed to avoid compiler warning for Unreachable Code

        case ERecognizerClose:
            //close the recognizer session.
        {
            PVMFStatus status = PVMFRecognizerRegistry::CloseSession(iRecognizerSessionId);
            if (status != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                                "PVMFDownloadManagerRecognizerContainer::IssueCommand CloseSession status %d", status));
            }
            return status;
        }

        default:
            LOGTRACE("PVMFDownloadManagerRecognizerContainer::IssueCommand Error, Unknown Recognizer Command!");
            Assert(false);//unknown command type for recognizer.
            return PVMFFailure;
    }
}

//this is the callback from the Recognizer::Recognize command.
void PVMFDownloadManagerRecognizerContainer::RecognizerCommandCompleted(const PVMFCmdResp& aResponse)
{
    if (aResponse.GetCmdId() == iCmdId
            && iCmdState == EBusy)
    {
        //save the result.
        if (aResponse.GetCmdStatus() == PVMFSuccess
                && iRecognizerResultVec.size() > 0)
        {
            iContainer->iMimeType = iRecognizerResultVec[0].iRecognizedFormat;
        }

        CommandDone(aResponse.GetCmdStatus(), aResponse.GetEventExtensionInterface(), aResponse.GetEventData());

        //catch completion of cancel for recognizer commands
        //since there's no cancel to the recognizer module, the cancel
        //is done whenever the current recognizer command is done.
        if (iCancelCmdState != EIdle)
        {
            CancelCommandDone(PVMFSuccess, NULL, NULL);
        }
    }
    else
    {
        Assert(false);//unexpected response.
    }
}

//from PVMFNodeErrorEventObserver
void PVMFDownloadManagerSubNodeContainer::HandleNodeErrorEvent(const PVMFAsyncEvent& aEvent)
{
    //A sub-node is reporting an event.

    //print events
    switch (iType)
    {
        case EFormatParser:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerSubNodeContainer::HandleNodeErrorEvent Parser Node Error Event %d", aEvent.GetEventType()));
            break;
        case EProtocolEngine:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerSubNodeContainer::HandleNodeErrorEvent ProtocolEngine Node Error Event %d", aEvent.GetEventType()));
            break;
        case ESocket:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerSubNodeContainer::HandleNodeErrorEvent Socket Node Error Event %d", aEvent.GetEventType()));
            if (iContainer->iDownloadComplete)
                return; // Suppress socket node error, if the download is already complete.
            break;
        default:
            Assert(false);
            break;
    }

    //duplicate any PVMF Error events from either node.
    if (IsPVMFErrCode(aEvent.GetEventType()))
        iContainer->ReportErrorEvent(aEvent.GetEventType(), aEvent.GetEventExtensionInterface(), aEvent.GetEventData());
}

#include "pvmf_protocol_engine_node_events.h"

//from PVMFNodeInfoEventObserver
void PVMFDownloadManagerSubNodeContainer::HandleNodeInformationalEvent(const PVMFAsyncEvent& aEvent)
{
    //A sub-node is reporting an event.

    //detect sub-node error states.
    if (aEvent.GetEventType() == PVMFInfoStateChanged
            && iNode->GetState() == EPVMFNodeError)
    {
        iContainer->SetState(EPVMFNodeError);
    }

    //detect important status events.
    if (iType == EProtocolEngine)
    {
        switch (aEvent.GetEventType())
        {
            case PVMFInfoBufferingComplete:
                iContainer->iDownloadComplete = true;
                iContainer->NotifyDownloadComplete();
                break;
            case PVMFPROTOCOLENGINE_INFO_MovieAtomCompleted:
                if (iContainer->iDebugMode)
                {
                    iContainer->ReportInfoEvent((PVMFAsyncEvent&)aEvent);
                }
                break;
            default:
                break;
        }
    }

    //filter out events that we don't want to pass up to observer
    bool filter = false;
    if (iType == ESocket)
    {
        switch (aEvent.GetEventType())
        {
            case PVMFInfoRemoteSourceNotification:	//To let the socket node events propagate to pvengine
                filter = false;
                break;
            default:
                filter = true;
        }
    }
    else
    {
        switch (aEvent.GetEventType())
        {
            case PVMFInfoStateChanged:
                filter = true;//always ignore
                break;
            case PVMFInfoPortDeleted:
            case PVMFInfoPortCreated:
            case PVMFInfoPortConnected:
            case PVMFInfoPortDisconnected:
                if (iType != EFormatParser)
                    filter = true;//ignore port events unless from format parser
                break;
            case PVMFInfoUnderflow:
            case PVMFInfoDataReady:
            case PVMFInfoRemoteSourceNotification:
                //apply some filtering to these
                if (iContainer->FilterPlaybackEventsFromSubNodes(aEvent))
                    filter = true;
                break;
            default:
                break;
        }
    }

    //duplicate all remaining PVMFInfo events.
    if (!filter
            && IsPVMFInfoCode(aEvent.GetEventType()))
    {
        iContainer->ReportInfoEvent((PVMFAsyncEvent&)aEvent);
    }

    //just print and ignore other events
    switch (iType)
    {
        case EFormatParser:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerSubNodeContainer::HandleNodeInfoEvent Parser Node Info Event %d", aEvent.GetEventType()));
            break;
        case EProtocolEngine:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerSubNodeContainer::HandleNodeInfoEvent ProtocolEngine Node Info Event %d", aEvent.GetEventType()));
            break;
        case ESocket:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                            "PVMFDownloadManagerSubNodeContainer::HandleNodeInfoEvent Socket Node Info Event %d", aEvent.GetEventType()));
            break;

        default:
            Assert(false);
            break;
    }
}

bool PVMFDownloadManagerSubNodeContainer::CancelPendingCommand()
{
    //initiate sub-node command cancel, return True if cancel initiated.

    if (iCmdState != EBusy)
        return false;//nothing to cancel

    iCancelCmdState = EBusy;

    if (iNode)
    {
        LOGTRACE("PVMFDownloadManagerSubNodeContainer::CancelPendingCommand Calling Cancel");
        iCancelCmdId = iNode->CancelCommand(iSessionId, iCmdId, NULL);
    }

    return true;//cancel initiated
}

bool PVMFDownloadManagerRecognizerContainer::CancelPendingCommand()
{
    //initiate sub-node command cancel, return True if cancel initiated.

    if (iCmdState != EBusy)
        return false;//nothing to cancel

    iCancelCmdState = EBusy;

    LOGTRACE("PVMFDownloadManagerSubNodeContainer::CancelPendingCommand Calling Cancel");
    iCancelCmdId = PVMFRecognizerRegistry::CancelCommand(iRecognizerSessionId, iCmdId, NULL);

    return true;//cancel initiated
}


void PVMFDownloadManagerSubNodeContainerBase::CommandDone(PVMFStatus aStatus, PVInterface*aExtMsg, OsclAny*aEventData)
{
    //a sub-node command is done-- process the result.

    Assert(aStatus != PVMFPending);

    //pop the sub-node command vector.
    Assert(!iContainer->iSubNodeCmdVec.empty());
    iContainer->iSubNodeCmdVec.erase(&iContainer->iSubNodeCmdVec.front());

    iCmdState = EIdle;

    PVMFStatus status = aStatus;

#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
    // Set "Init Failed License Required" flag with the results of parser Init.
    if (iType == EFormatParser && iCmd == EInit)
    {
        iContainer->iInitFailedLicenseRequired = (status == PVMFErrLicenseRequired);
    }
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE

    // Watch for the request port command completion from the protocol node, because we need to save the port pointer
    if (iType == EProtocolEngine && iCmd == ERequestPort && status == PVMFSuccess)
    {
        iContainer->iProtocolEngineNodePort = (PVMFPortInterface*)aEventData;
        // If both ports are non-null, connect them.
        if (iContainer->iSocketNodePort && iContainer->iProtocolEngineNodePort)
        {
            iContainer->iSocketNodePort->Connect(iContainer->iProtocolEngineNodePort);

            // The ports are connected, so now we pass the datastream factory to the protocol node via the extension interface, if it's available.
            if (iContainer->iProtocolEngineNode.iDatastreamUser)
            {
                ((PVMIDatastreamuserInterface*)iContainer->iProtocolEngineNode.iDatastreamUser)->PassDatastreamFactory(*(iContainer->iWriteFactory), (int32)0);
            }
        }
    }

    // Watch for the request port command completion from the socket node, because we need to save the port pointer
    if (iType == ESocket && iCmd == ERequestPort && status == PVMFSuccess)
    {
        iContainer->iSocketNodePort = (PVMFPortInterface*)aEventData;
    }



    // Watch for recognizer start failure
    if (iType == ERecognizer && iCmd == ERecognizerStart && aStatus != PVMFSuccess)
    {
        iContainer->iRecognizerError = true;
        //save the error code to report after recognizer close.
        iContainer->iRecognizerStartStatus = status;
        //purge everything from the subnode command vector except the recognizer
        //close command
        iContainer->iSubNodeCmdVec.clear();
        iContainer->Push(iContainer->iRecognizerNode, PVMFDownloadManagerSubNodeContainerBase::ERecognizerClose);
        //set status to "success" so that we'll continue with processing
        status = PVMFSuccess;
    }
    // Watch for recognizer close completion after a start failure
    else if (iContainer->iRecognizerError)
    {
        OSCL_ASSERT(iCmd == ERecognizerClose);
        iContainer->iRecognizerError = false;
        //restore the original error code from the recognizer start.
        status = iContainer->iRecognizerStartStatus;
    }

    //Check whether the node command is being cancelled.
    if (iCancelCmdState != EIdle)
    {
        if (!iContainer->iSubNodeCmdVec.empty())
        {
            //even if this command succeeded, we want to report
            //the node command status as cancelled since some sub-node
            //commands were not yet issued.
            status = PVMFErrCancelled;
            //go into an error state since it's not clear
            //how to recover from a partially completed command.
            iContainer->SetState(EPVMFNodeError);
        }
    }

    //figure out the next step in the sequence...
    //A node command is done when either all sub-node commands are
    //done or when one fails.
    if (status == PVMFSuccess
            && !iContainer->iSubNodeCmdVec.empty())
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

void PVMFDownloadManagerSubNodeContainerBase::CancelCommandDone(PVMFStatus aStatus, PVInterface*aExtMsg, OsclAny*aEventData)
{
    OSCL_UNUSED_ARG(aExtMsg);
    OSCL_UNUSED_ARG(aEventData);
    //a sub-node cancel command is done-- process the result.

    Assert(aStatus != PVMFPending);

    iCancelCmdState = EIdle;
    //print and ignore any failed sub-node cancel commands.
    if (aStatus != PVMFSuccess)
    {
        switch (iType)
        {
            case EFormatParser:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                                "PVMFDownloadManagerSubNodeContainer::CancelCommandDone Parser Node Cancel failed"));
                break;
            case EProtocolEngine:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                                "PVMFDownloadManagerSubNodeContainer::CancelCommandDone ProtocolEngine Node Cancel failed"));
                break;
            case ESocket:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_ERR, (0,
                                "PVMFDownloadManagerSubNodeContainer::CancelCommandDone Socket Node Cancel failed"));
                break;
            default:
                Assert(false);
                break;
        }
    }

    //Node cancel command is now done.
    Assert(!iContainer->iCancelCommand.empty());
    iContainer->CommandComplete(iContainer->iCancelCommand, iContainer->iCancelCommand.front(), aStatus, NULL, NULL);
}

//from PVMFNodeCmdStatusObserver
void PVMFDownloadManagerSubNodeContainer::NodeCommandCompleted(const PVMFCmdResp& aResponse)
{
    //A command to a sub-node is complete

    if (iType == EFormatParser)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerSubNodeContainer::NodeCommandCompleted FORMAT PARSER"));
    }
    else if (iType == EProtocolEngine)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerSubNodeContainer::NodeCommandCompleted PROTOCOL ENGINE"));
    }
    else if (iType == ESocket)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerSubNodeContainer::NodeCommandCompleted SOCKET"));
    }

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iContainer->iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFDownloadManagerSubNodeContainer::NodeCommandCompleted Failure! %d", aResponse.GetCmdStatus()));
    }

    if (aResponse.GetCmdId() == iCmdId
            && iCmdState == EBusy)
    {
        //Process normal command response.
        CommandDone(aResponse.GetCmdStatus(), aResponse.GetEventExtensionInterface(), aResponse.GetEventData());
    }
    else if (aResponse.GetCmdId() == iCancelCmdId
             && iCancelCmdState == EBusy)
    {
        //Process node cancel command response
        CancelCommandDone(aResponse.GetCmdStatus(), aResponse.GetEventExtensionInterface(), aResponse.GetEventData());
    }
#if(PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE)
    //Process Get License cancel command response.
    else if (aResponse.GetCmdId() == iCPMCancelGetLicenseCmdId
             && iCancelCmdState == EBusy)
    {
        CancelCommandDone(aResponse.GetCmdStatus(), aResponse.GetEventExtensionInterface(), aResponse.GetEventData());
    }
#endif//PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE
    else
    {
        Assert(false);//unexpected response.
    }
}


//From capability and config interface
PVMFStatus PVMFDownloadManagerNode::getParametersSync(PvmiMIOSession aSession,
        PvmiKeyType aIdentifier,
        PvmiKvp*& aParameters,
        int& aNumParamElements,
        PvmiCapabilityContext aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // Initialize the output parameters
    aNumParamElements = 0;
    aParameters = NULL;

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aIdentifier);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aIdentifier, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
    {
        // First component should be "x-pvmf" and there must
        // be at least two components to go past x-pvmf
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFDownloadManagerNode::getParametersSync() Invalid key string"));
        return PVMFErrArgument;
    }

    // Retrieve the second component from the key string
    pv_mime_string_extract_type(1, aIdentifier, compstr);

    // Check if it is key string for Download manager
    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) < 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFDownloadManagerNode::getParametersSync() Unsupported key"));
        return PVMFFailure;
    }


    if (compcount == 2)
    {
        // Since key is "x-pvmf/net" return all
        // nodes available at this level. Ignore attribute
        // since capability is only allowed

        // Allocate memory for the KVP list
        aParameters = (PvmiKvp*)oscl_malloc(DownloadManagerConfig_NumBaseKeys * sizeof(PvmiKvp));
        if (aParameters == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFDownloadManagerNode::getParametersSync() Memory allocation for KVP failed"));
            return PVMFErrNoMemory;
        }
        oscl_memset(aParameters, 0, DownloadManagerConfig_NumBaseKeys*sizeof(PvmiKvp));
        // Allocate memory for the key strings in each KVP
        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(DownloadManagerConfig_NumBaseKeys * DLMCONFIG_KEYSTRING_SIZE * sizeof(char));
        if (memblock == NULL)
        {
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFDownloadManagerNode::getParametersSync() Memory allocation for key string failed"));
            return PVMFErrNoMemory;
        }
        oscl_strset(memblock, 0, DownloadManagerConfig_NumBaseKeys*DLMCONFIG_KEYSTRING_SIZE*sizeof(char));
        // Assign the key string buffer to each KVP
        uint32 j;
        for (j = 0; j < DownloadManagerConfig_NumBaseKeys; ++j)
        {
            aParameters[j].key = memblock + (j * DLMCONFIG_KEYSTRING_SIZE);
        }
        // Copy the requested info
        for (j = 0; j < DownloadManagerConfig_NumBaseKeys; ++j)
        {
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/net/"), 17);
            oscl_strncat(aParameters[j].key, DownloadManagerConfig_BaseKeys[j].iString, oscl_strlen(DownloadManagerConfig_BaseKeys[j].iString));
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type="), 6);
            switch (DownloadManagerConfig_BaseKeys[j].iType)
            {
                case PVMI_KVPTYPE_AGGREGATE:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_AGGREGATE_STRING), oscl_strlen(PVMI_KVPTYPE_AGGREGATE_STRING));
                    break;

                case PVMI_KVPTYPE_POINTER:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_POINTER_STRING), oscl_strlen(PVMI_KVPTYPE_POINTER_STRING));
                    break;

                case PVMI_KVPTYPE_VALUE:
                default:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_VALUE_STRING), oscl_strlen(PVMI_KVPTYPE_VALUE_STRING));
                    break;
            }
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";valtype="), 9);
            switch (DownloadManagerConfig_BaseKeys[j].iValueType)
            {
                case PVMI_KVPVALTYPE_RANGE_INT32:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_INT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_INT32_STRING));
                    break;

                case PVMI_KVPVALTYPE_KSV:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_KSV_STRING), oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
                    break;

                case PVMI_KVPVALTYPE_CHARPTR:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_CHARPTR_STRING), oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING));
                    break;

                case PVMI_KVPVALTYPE_WCHARPTR:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_WCHARPTR_STRING), oscl_strlen(PVMI_KVPVALTYPE_WCHARPTR_STRING));
                    break;

                case PVMI_KVPVALTYPE_BOOL:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
                    break;

                case PVMI_KVPVALTYPE_UINT32:
                default:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
                    break;
            }
            aParameters[j].key[DLMCONFIG_KEYSTRING_SIZE-1] = 0;
        }

        aNumParamElements = DownloadManagerConfig_NumBaseKeys;
    }
    else if (compcount == 3)
    {
        pv_mime_string_extract_type(2, aIdentifier, compstr);

        // Determine what is requested
        PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
        if (reqattr == PVMI_KVPATTR_UNKNOWN)
        {
            reqattr = PVMI_KVPATTR_CUR;
        }
        uint i;
        for (i = 0; i < DownloadManagerConfig_NumBaseKeys; i++)
        {
            if (pv_mime_strcmp(compstr, (char*)(DownloadManagerConfig_BaseKeys[i].iString)) >= 0)
            {
                break;
            }
        }

        if (i == DownloadManagerConfig_NumBaseKeys)
        {
            // no match found
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFDownloadManagerNode::getParametersSync() Unsupported key"));
            return PVMFErrNoMemory;
        }
    }
    else
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFDownloadManagerNode::getParametersSync() Unsupported key"));
        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}


PVMFStatus PVMFDownloadManagerNode::releaseParameters(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::releaseParameters() In"));

    if (aParameters == NULL || num_elements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFDownloadManagerNode::releaseParameters() KVP list is NULL or number of elements is 0"));
        return PVMFErrArgument;
    }

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aParameters[0].key);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aParameters[0].key, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
    {
        // First component should be "x-pvmf" and there must
        // be at least two components to go past x-pvmf
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFDownloadManagerNode::releaseParameters() Unsupported key"));
        return PVMFErrArgument;
    }

    // Retrieve the second component from the key string
    pv_mime_string_extract_type(1, aParameters[0].key, compstr);

    // Assume all the parameters come from the same component so the base components are the same
    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) >= 0)
    {
        // Go through each KVP and release memory for value if allocated from heap
        for (int32 i = 0; i < num_elements; ++i)
        {
            // Next check if it is a value type that allocated memory
            PvmiKvpType kvptype = GetTypeFromKeyString(aParameters[i].key);
            if (kvptype == PVMI_KVPTYPE_VALUE || kvptype == PVMI_KVPTYPE_UNKNOWN)
            {
                PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameters[i].key);
                if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFDownloadManagerNode::releaseParameters() Valtype not specified in key string"));
                    return PVMFErrArgument;
                }

                if (keyvaltype == PVMI_KVPVALTYPE_CHARPTR && aParameters[i].value.pChar_value != NULL)
                {
                    oscl_free(aParameters[i].value.pChar_value);
                    aParameters[i].value.pChar_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_WCHARPTR && aParameters[i].value.pWChar_value != NULL)
                {
                    oscl_free(aParameters[i].value.pWChar_value);
                    aParameters[i].value.pWChar_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_CHARPTR && aParameters[i].value.pChar_value != NULL)
                {
                    oscl_free(aParameters[i].value.pChar_value);
                    aParameters[i].value.pChar_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_KSV && aParameters[i].value.key_specific_value != NULL)
                {
                    oscl_free(aParameters[i].value.key_specific_value);
                    aParameters[i].value.key_specific_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_RANGE_INT32 && aParameters[i].value.key_specific_value != NULL)
                {
                    range_int32* ri32 = (range_int32*)aParameters[i].value.key_specific_value;
                    aParameters[i].value.key_specific_value = NULL;
                    oscl_free(ri32);
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_RANGE_UINT32 && aParameters[i].value.key_specific_value != NULL)
                {
                    range_uint32* rui32 = (range_uint32*)aParameters[i].value.key_specific_value;
                    aParameters[i].value.key_specific_value = NULL;
                    oscl_free(rui32);
                }
            }
        }

        oscl_free(aParameters[0].key);

        // Free memory for the parameter list
        oscl_free(aParameters);
        aParameters = NULL;
    }
    else
    {
        // Unknown key string
        return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::releaseParameters() Out"));
    return PVMFSuccess;

}

void PVMFDownloadManagerNode::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFDownloadManagerNode::setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
        PvmiKvp* aParameters, int num_parameter_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_parameter_elements);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFDownloadManagerNode::DeleteContext(PvmiMIOSession aSession,
        PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFDownloadManagerNode::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
        int num_elements, PvmiKvp * & aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::setParametersSync() In"));

    aRet_kvp = NULL;

    // Go through each parameter
    for (int paramind = 0; paramind < num_elements; ++paramind)
    {
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(aParameters[paramind].key);

        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
        {
            // First component should be "x-pvmf" and there must
            // be at least two components to go past x-pvmf
            aRet_kvp = &aParameters[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFDownloadManagerNode::setParametersSync() Unsupported key"));
            return;
        }

        // Retrieve the second component from the key string
        pv_mime_string_extract_type(1, aParameters[paramind].key, compstr);

        // First check if it is key string for the Download manager
        if (pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) >= 0)
        {
            if (compcount == 3)
            {
                pv_mime_string_extract_type(2, aParameters[paramind].key, compstr);
                uint i;
                for (i = 0; i < DownloadManagerConfig_NumBaseKeys; i++)
                {
                    if (pv_mime_strcmp(compstr, (char*)(DownloadManagerConfig_BaseKeys[i].iString)) >= 0)
                    {
                        break;
                    }
                }

                if (DownloadManagerConfig_NumBaseKeys == i)
                {
                    // invalid third component
                    aRet_kvp = &aParameters[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFDownloadManagerNode::setParametersSync() Unsupported key"));
                    return;
                }

                // Verify and set the passed-in setting
                switch (i)
                {
                    case BASEKEY_SESSION_CONTROLLER_USER_AGENT:
                    {
                        OSCL_wHeapString<OsclMemAllocator> userAgent;
                        userAgent = aParameters[paramind].value.pWChar_value;
                        (iProtocolEngineNode.ProtocolEngineExtension())->SetUserAgent(userAgent, false);

                    }
                    break;
                    case BASEKEY_SESSION_CONTROLLER_HTTP_VERSION:
                    {
                        uint32 httpVersion;
                        httpVersion = aParameters[paramind].value.uint32_value;
                        (iProtocolEngineNode.ProtocolEngineExtension())->SetHttpVersion(httpVersion);

                    }
                    break;
                    case BASEKEY_SESSION_CONTROLLER_HTTP_TIMEOUT:
                    {
                        uint32 httpTimeout;
                        httpTimeout = aParameters[paramind].value.uint32_value;
                        (iProtocolEngineNode.ProtocolEngineExtension())->SetNetworkTimeout(httpTimeout);
                    }
                    break;
                    case BASEKEY_SESSION_CONTROLLER_DOWNLOAD_PROGRESS_INFO:
                    {
                        OSCL_HeapString<OsclMemAllocator> downloadProgressInfo;
                        downloadProgressInfo = aParameters[paramind].value.pChar_value;
                        DownloadProgressMode aMode = DownloadProgressMode_TimeBased;
                        if (IsByteBasedDownloadProgress(downloadProgressInfo)) aMode = DownloadProgressMode_ByteBased;
                        (iProtocolEngineNode.ProtocolEngineExtension())->SetDownloadProgressMode(aMode);
                    }
                    break;
                    case BASEKEY_SESSION_CONTROLLER_PROTOCOL_EXTENSION_HEADER:
                    {
                        if (IsDownloadExtensionHeaderValid(aParameters[paramind]))
                        {
                            OSCL_HeapString<OsclMemAllocator> extensionHeaderKey;
                            OSCL_HeapString<OsclMemAllocator> extensionHeaderValue;
                            HttpMethod httpMethod = HTTP_GET;
                            bool aPurgeOnRedirect = false;
                            if (GetHttpExtensionHeaderParams(aParameters[paramind],
                                                             extensionHeaderKey,
                                                             extensionHeaderValue,
                                                             httpMethod,
                                                             aPurgeOnRedirect))
                            {
                                (iProtocolEngineNode.ProtocolEngineExtension())->SetHttpExtensionHeaderField(extensionHeaderKey,
                                        extensionHeaderValue,
                                        httpMethod,
                                        aPurgeOnRedirect);
                            }
                        }

                    }
                    break;

                    case BASEKEY_SESSION_CONTROLLER_NUM_REDIRECT_ATTEMPTS:
                    {
                        uint32 numRedirects = aParameters[paramind].value.uint32_value;
                        (iProtocolEngineNode.ProtocolEngineExtension())->SetNumRedirectTrials(numRedirects);
                    }
                    break;

                    case BASEKEY_SESSION_CONTROLLER_NUM_HTTP_HEADER_REQUEST_DISABLED:
                    {
                        bool httpHeaderRequestDisabled = aParameters[paramind].value.bool_value;
                        (iProtocolEngineNode.ProtocolEngineExtension())->DisableHttpHeadRequest(httpHeaderRequestDisabled);
                    }
                    break;

                    case BASEKEY_MAX_TCP_RECV_BUFFER_SIZE:
                    {
                        uint32 size = aParameters[paramind].value.uint32_value;
                        PVMFSocketNode* socketNode =
                            (PVMFSocketNode*)(iSocketNode.iNode);
                        if (socketNode != NULL)
                        {
                            socketNode->SetMaxTCPRecvBufferSize(size);
                        }
                    }
                    break;

                    case BASEKEY_MAX_TCP_RECV_BUFFER_COUNT:
                    {
                        uint32 size = aParameters[paramind].value.uint32_value;
                        PVMFSocketNode* socketNode =
                            (PVMFSocketNode*)(iSocketNode.iNode);
                        if (socketNode != NULL)
                        {
                            socketNode->SetMaxTCPRecvBufferCount(size);
                        }
                    }
                    break;

                    default:
                    {
                        aRet_kvp = &aParameters[paramind];
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMFDownloadManagerNode::setParametersSync() Setting "
                                         "parameter %d failed", paramind));
                    }
                    break;
                }
            }
            else
            {
                // Do not support more than 3 components right now
                aRet_kvp = &aParameters[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFDownloadManagerNode::setParametersSync() Unsupported key"));
                return;
            }
        }
        else
        {
            // Unknown key string
            aRet_kvp = &aParameters[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFDownloadManagerNode::setParametersSync() Unsupported key"));
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFDownloadManagerNode::setParametersSync() Out"));
}

bool PVMFDownloadManagerNode::IsByteBasedDownloadProgress(OSCL_String &aDownloadProgressInfo)
{
    if (aDownloadProgressInfo.get_size() < 4) return false; // 4 => byte
    char *ptr = (char*)aDownloadProgressInfo.get_cstr();
    uint32 len = aDownloadProgressInfo.get_size();

    while (!(((ptr[0]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'b') &&
             ((ptr[1]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'y') &&
             ((ptr[2]  | OSCL_ASCII_CASE_MAGIC_BIT) == 't') &&
             ((ptr[3]  | OSCL_ASCII_CASE_MAGIC_BIT) == 'e')) &&
            len >= 4)
    {
        ptr++;
        len--;
    }
    if (len < 4) return false;

    return true; // find case-insentive string "byte"
}

bool PVMFDownloadManagerNode::GetHttpExtensionHeaderParams(PvmiKvp &aParameter,
        OSCL_String &extensionHeaderKey,
        OSCL_String &extensionHeaderValue,
        HttpMethod  &httpMethod,
        bool &aPurgeOnRedirect)
{
    // check if the extension header is meant for download
    if (!IsHttpExtensionHeaderValid(aParameter)) return false;

    // get aPurgeOnRedirect
    aPurgeOnRedirect = false;
    OSCL_StackString<32> purgeOnRedirect(_STRLIT_CHAR("purge-on-redirect"));
    if (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), purgeOnRedirect.get_cstr()) != NULL)
    {
        aPurgeOnRedirect = true;
    }

    // get key, value and http method of protocol extension header
    // the string value needs to be structured as follows: "key=app-feature-tag;value=xyz"
    char* extensionHeader = aParameter.value.pChar_value;
    if (!extensionHeader) return false;

    // (1) extract the key
    OSCL_StackString<8> keyTag(_STRLIT_CHAR("key="));

    OSCL_StackString<8> valueTag(_STRLIT_CHAR("value="));
    char *keyStart = oscl_strstr(OSCL_CONST_CAST(char*, extensionHeader), keyTag.get_cstr());
    if (!keyStart) return false;

    keyStart += keyTag.get_size();
    char *keyEnd = oscl_strstr(OSCL_CONST_CAST(char*, extensionHeader), valueTag.get_cstr());
    if (!keyEnd) return false;
    uint32 keyLen = getItemLen(keyStart, keyEnd);
    if (keyLen == 0) return false;
    extensionHeaderKey = OSCL_HeapString<OsclMemAllocator> (keyStart, keyLen);

    // (2) extract the value
    char* valueStart = keyEnd;
    valueStart += valueTag.get_size();

    OSCL_StackString<8> methodTag(_STRLIT_CHAR("method="));
    char* valueEnd = oscl_strstr(valueStart, methodTag.get_cstr());
    if (!valueEnd) valueEnd = extensionHeader + aParameter.capacity;
    uint32 valueLen = getItemLen(valueStart, valueEnd);
    extensionHeaderValue = OSCL_HeapString<OsclMemAllocator> (valueStart, valueLen);

    // (3) check for optional method
    char *methodStart = oscl_strstr(OSCL_CONST_CAST(char*, extensionHeader), methodTag.get_cstr());
    if (!methodStart)
    {
        httpMethod = HTTP_GET;
        return true;
    }
    methodStart += methodTag.get_size();

    OSCL_StackString<8> methodHttpGet(_STRLIT_CHAR("GET"));
    OSCL_StackString<8> methodHttpHead(_STRLIT_CHAR("HEAD"));
    OSCL_StackString<8> methodHttpPost(_STRLIT_CHAR("POST"));

    char* methodGet = NULL;
    char* methodHead = NULL;
    char* methodPost = NULL;
    methodGet = oscl_strstr(methodStart, methodHttpGet.get_cstr());
    methodHead = oscl_strstr(methodStart, methodHttpHead.get_cstr());
    methodPost = oscl_strstr(methodStart, methodHttpPost.get_cstr());

    httpMethod = HTTP_GET;
    if (methodPost != NULL) httpMethod = HTTP_POST;
    if (methodGet  != NULL) httpMethod = HTTP_GET;
    if (methodHead != NULL) httpMethod = HTTP_HEAD;
    if ((methodGet != NULL) && (methodHead != NULL)) httpMethod = HTTP_ALLMETHOD;

    return true;
}

bool PVMFDownloadManagerNode::IsHttpExtensionHeaderValid(PvmiKvp &aParameter)
{
    OSCL_StackString<32> downloadMode(_STRLIT_CHAR("mode=download"));
    OSCL_StackString<32> streamingMode(_STRLIT_CHAR("mode=streaming"));

    bool isDownloadMode  = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), downloadMode.get_cstr())  != NULL);
    bool isStreamingMode = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), streamingMode.get_cstr()) != NULL);

    // streaming mode only would fail, download mode specified or not specified will be viewed as true
    if (isStreamingMode && !isDownloadMode) return false;

    return true;
}

// remove the ending ';', ',' or ' ' and calulate value length
uint32 PVMFDownloadManagerNode::getItemLen(char *ptrItemStart, char *ptrItemEnd)
{
    char *ptr = ptrItemEnd - 1;
    uint32 itemLen = ptr - ptrItemStart;
    for (uint32 i = 0; i < itemLen; i++)
    {
        if (*ptr == ';' || *ptr == ',' || *ptr == ' ') --ptr;
        else break;
    }
    itemLen = ptr - ptrItemStart + 1;
    return itemLen;
}


PVMFCommandId PVMFDownloadManagerNode::setParametersAsync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements,
        PvmiKvp*& aRet_kvp,
        OsclAny* context)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_elements);
    OSCL_UNUSED_ARG(aRet_kvp);
    OSCL_UNUSED_ARG(context);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
    return 0;
}

uint32 PVMFDownloadManagerNode::getCapabilityMetric(PvmiMIOSession aSession)
{
    OSCL_UNUSED_ARG(aSession);
    return 0;
}

PVMFStatus PVMFDownloadManagerNode::verifyParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_elements);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
    return 0;
}

bool PVMFDownloadManagerNode::IsDownloadExtensionHeaderValid(PvmiKvp &aParameter)
{
    OSCL_StackString<32> downloadMode(_STRLIT_CHAR("mode=download"));
    OSCL_StackString<32> streamingMode(_STRLIT_CHAR("mode=streaming"));
    OSCL_StackString<32> dlaMode(_STRLIT_CHAR("mode=dla"));

    bool isDownloadMode  = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), downloadMode.get_cstr())  != NULL);
    bool isStreamingMode = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), streamingMode.get_cstr()) != NULL);
    bool isDlaMode = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), dlaMode.get_cstr()) != NULL);


    // streaming mode only would fail, download mode specified or not specified will be viewed as true
    if (isStreamingMode && !isDownloadMode) return false;

    // dla mode only would fail, download mode specified or not specified will be viewed as true
    if (isDlaMode && !isDownloadMode) return false;


    if (isDownloadMode) return true;

    return false;
}

void PVMFDownloadManagerNode::ClockStateUpdated()
{
    if (!iDataReady)
    {
        // Don't let anyone start the clock while the source node is in underflow
        if (iPlayBackClock != NULL)
        {
            if (iPlayBackClock->GetState() == OsclClock::RUNNING)
            {
                iPlayBackClock->Pause();
            }
        }
    }
}
