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
#ifndef PVMF_SM_FSP_PVR_BASE_IMPL_H
#include "pvmf_sm_fsp_pvr_base_impl.h"
#endif

#ifndef PVMF_JITTER_BUFFER_NODE_H_INCLUDED
#include "pvmf_jitter_buffer_node.h"
#endif

#ifndef PVMF_MEDIALAYER_NODE_H_INCLUDED
#include "pvmf_medialayer_node.h"
#endif

#ifndef OSCL_SNPRINTF_H_INCLUDED
#include "oscl_snprintf.h"
#endif

#ifndef AMR_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "amr_payload_parser_factory.h"
#endif

#ifndef H263_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "h263_payload_parser_factory.h"
#endif

#ifndef H264_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "h264_payload_parser_factory.h"
#endif

#ifndef ASF_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "asf_payload_parser_factory.h"
#endif

#ifndef M4V_AUDIO_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "m4v_audio_payload_parser_factory.h"
#endif

#ifndef M4V_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "m4v_payload_parser_factory.h"
#endif

#ifndef PAYLOAD_PARSER_REGISTRY_H_INCLUDED
#include "payload_parser_registry.h"
#endif

#ifndef RFC3640_PAYLOAD_PARSER_FACTORY_H_INCLUDED
#include "rfc3640_payload_parser_factory.h"
#endif

#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif

#ifndef PVMI_KVP_UTIL_H_INCLUDED
#include "pvmi_kvp_util.h"
#endif

#ifndef PVMI_DRM_KVP_H_INCLUDED
#include "pvmi_drm_kvp.h"
#endif

#ifndef PVRTSP_CLIENT_ENGINE_NODE_H
#include "pvrtsp_client_engine_node.h"
#endif

#ifndef SDP_PARSER_H
#include "sdp_parser.h"
#endif

#ifndef PVMF_PVR_NODE_H_INCLUDED
#include "pvmf_pvr_node.h"
#endif

#ifndef PVMF_SOCKET_NODE_H_INCLUDED
#include "pvmf_socket_node.h"
#endif

#ifndef PVMF_RTSP_ENGINE_NODE_FACTORY_H_INCLUDED
#include "pvrtsp_client_engine_factory.h"
#endif



PVMFSMFSPPVRBase::PVMFSMFSPPVRBase(int32 aPriority) : PVMFSMFSPBaseNode(aPriority)
{
}


void PVMFSMFSPPVRBase::BypassError()
{
    int32 localMode = 0;
    if (iPVMFDataSourcePositionParamsPtr != NULL)
    {
        localMode = iPVMFDataSourcePositionParamsPtr->iMode;
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::BypassError() - In"));
    if (CheckChildrenNodesStart())
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFSMFSPBaseNodeCommand aCmd = iCurrentCommand.front();
            if ((aCmd.iCmd == PVMF_SMFSP_NODE_START) ||
                    (aCmd.iCmd == PVMF_SMFSP_NODE_SET_DATASOURCE_POSITION))
            {
                if (iRepositioning)
                {
                    PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::BypassError() - InRepos"));
                    iRepositioning = false;
                    iPlayListRepositioning = false;
                    iPVMFDataSourcePositionParamsPtr = NULL;
                    PVMFSMFSPChildNodeContainer* jitterBufferNodeContainer =
                        getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
                    OSCL_ASSERT(jitterBufferNodeContainer);
                    if (!jitterBufferNodeContainer)
                        return;

                    PVMFJitterBufferExtensionInterface* jbExtIntf =
                        (PVMFJitterBufferExtensionInterface*)
                        (jitterBufferNodeContainer->iExtensions[0]);

                    OSCL_ASSERT(jbExtIntf);
                    if (!jbExtIntf)
                        return;

                    /* Set jitter buffer state to ready */
                    jbExtIntf->UpdateJitterBufferState();
                }
                if ((localMode == 0) || (localMode == -1))
                {
                    SetState(EPVMFNodeStarted);
                    if (IsAdded())
                    {
                        /* wakeup the AO */
                        RunIfNotReady();
                    }
                    PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::BypassError() - InLocalMode"));
                    RunIfNotReady();
                }
                CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess, NULL, NULL, NULL);

            }
        }
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::BypassError() - Out"));
    return;
}

bool PVMFSMFSPPVRBase::CheckChildrenNodesInit()
{
    for (uint32 i = 0; i < iFSPChildNodeContainerVec.size(); i++)
    {
        if (iFSPChildNodeContainerVec[i].iNodeCmdState != PVMFSMFSP_NODE_CMD_IDLE)
        {
            return false;
        }
    }
    return true;
}

bool PVMFSMFSPPVRBase::CheckChildrenNodesPause()
{
    for (uint32 i = 0; i < iFSPChildNodeContainerVec.size(); i++)
    {
        if (iFSPChildNodeContainerVec[i].iNodeCmdState != PVMFSMFSP_NODE_CMD_IDLE
                && iFSPChildNodeContainerVec[i].iNodeCmdState != PVMFSMFSP_NODE_CMD_CANCEL_PENDING)
        {
            return false;
        }
    }
    return true;
}

bool PVMFSMFSPPVRBase::CheckChildrenNodesPrepare()
{
    for (uint32 i = 0; i < iFSPChildNodeContainerVec.size(); i++)
    {
        if (iFSPChildNodeContainerVec[i].iNodeCmdState != PVMFSMFSP_NODE_CMD_IDLE)
        {
            return false;
        }
    }
    return true;
}

bool PVMFSMFSPPVRBase::CheckChildrenNodesStart()
{
    for (uint32 i = 0; i < iFSPChildNodeContainerVec.size(); i++)
    {
        if (iFSPChildNodeContainerVec[i].iNodeCmdState == PVMFSMFSP_NODE_CMD_PENDING)
        {
            return false;
        }
    }
    return true;
}

bool PVMFSMFSPPVRBase::CheckChildrenNodesStop()
{

    for (uint32 i = 0; i < iFSPChildNodeContainerVec.size(); i++)
    {
        if (iFSPChildNodeContainerVec[i].iNodeCmdState != PVMFSMFSP_NODE_CMD_IDLE)
        {
            return false;
        }
    }

    return true;
}

void PVMFSMFSPPVRBase::CleanUp()
{
    ReleaseChildNodesExtentionInterface();
    DestroyChildNodes();
    DestroyPayloadParserRegistry();
    ResetNodeParams();
    iLogger = NULL;
}

void PVMFSMFSPPVRBase::ResetNodeParams(bool aReleaseMemory)
{
    iPVREnabled = false;
    if (aReleaseMemory)
    {
        if (iPVRExtInterface)
            iPVRExtInterface->removeRef();
        //PVR interface is pointing to interface available in nodeconatiner for PVR. So, dont relase it, just make it null, it will be released in func ReleaseChildNodesExtentionInterface
        iPVRQueryInterface	= NULL;
        if (iPVRControl)
        {
            ;	//todo release the pvr control???
        }
    }
    iPVRExtInterface = NULL;
    iPVRQueryInterface = NULL;
    iPVRControl = NULL;
    iTrackInfoVec.clear();
    PVMFSMFSPBaseNode::ResetNodeParams(aReleaseMemory);
}



void PVMFSMFSPPVRBase::CompletePrepare()
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::CompletePrepare() - In"));
    if ((CheckChildrenNodesPrepare()) && (iGraphConstructComplete))
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFSMFSPBaseNodeCommand& aCmd = iCurrentCommand.front();
            if (aCmd.iCmd == PVMF_SMFSP_NODE_PREPARE)
            {
                SetState(EPVMFNodePrepared);
                PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::Prepare() - CmdComplete - PVMFSuccess"));
                CommandComplete(iCurrentCommand, aCmd, PVMFSuccess);
            }
        }
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::CompletePrepare() - Out"));
    return;
}

void PVMFSMFSPPVRBase::CompleteStop()
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::CompleteStop() - In"));
    if (CheckChildrenNodesStop())
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFSMFSPBaseNodeCommand& aCmd = iCurrentCommand.front();
            if (aCmd.iCmd == PVMF_SMFSP_NODE_STOP)
            {
                /* transition to Prepared state */
                ResetStopCompleteParams();
                SetState(EPVMFNodePrepared);
                PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::Stop() - CmdComplete - PVMFSuccess"));
                CommandComplete(iCurrentCommand, aCmd, PVMFSuccess);
            }
        }
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::CompleteStop() - Out"));
    return;
}

PVMFStatus PVMFSMFSPPVRBase::ComputeSkipTimeStamp(PVMFTimestamp aTargetNPT,
        PVMFTimestamp aActualNPT,
        PVMFTimestamp aActualMediaDataTS,
        PVMFTimestamp& aSkipTimeStamp,
        PVMFTimestamp& aStartNPT)
{
    //for RTSP streaming we always start playback from aActualNPT
    //by defintion aActualMediaDataTS is the timestamp that corresponds
    //to aActualNPT
    OSCL_UNUSED_ARG(aTargetNPT);
    OSCL_UNUSED_ARG(aSkipTimeStamp);
    OSCL_UNUSED_ARG(aStartNPT);
    aSkipTimeStamp = aActualMediaDataTS;
    aStartNPT = aActualNPT;
    return PVMFSuccess;
}

PVMFStatus PVMFSMFSPPVRBase::ConnectPortPairs(PVMFPortInterface* aPort1,
        PVMFPortInterface* aPort2)
{
    PVMFStatus status;

    status = aPort1->Connect(aPort2);

    if (status != PVMFSuccess)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "StreamingManagerNode:ConnectPortPairs - Connect Failed"));
        return status;
    }

    return status;
}

void PVMFSMFSPPVRBase::Construct()
{
    PVMFSMFSPBaseNode::Construct();
    int32 err;
    OSCL_TRY(err,
             iLogger = PVLogger::GetLoggerObject("PVMFSMPVRFilePlaybackNode");
             iAvailableMetadataKeys.reserve(PVMFSTREAMINGMGRNODE_NUM_METADATAKEYS);
             iAvailableMetadataKeys.clear();
             CreateChildNodes();
             QueryChildNodesExtentionInterface();
             // create the payload parser registry
             PopulatePayloadParserRegistry();
             // pass the payload parser registry on to the media layer node
             PVMFSMFSPChildNodeContainer* iMediaLayerNodeContainer =
                 getChildNodeContainer(PVMF_SM_FSP_MEDIA_LAYER_NODE);
             OSCL_ASSERT(iMediaLayerNodeContainer);
             PVMFMediaLayerNodeExtensionInterface* mlExtIntf = NULL;
             if (iMediaLayerNodeContainer)
             mlExtIntf = (PVMFMediaLayerNodeExtensionInterface*)(iMediaLayerNodeContainer->iExtensions[0]);
             if (mlExtIntf)
                 mlExtIntf->setPayloadParserRegistry(PayloadParserRegistry::GetPayloadParserRegistry());
                );
    if (err != OsclErrNone)
    {
        PVMFSMFSPBaseNode::CleanUp();
        OSCL_LEAVE(err);
    }

}

void PVMFSMFSPPVRBase::PopulatePayloadParserRegistry()
{
    PayloadParserRegistry* registry =
        PayloadParserRegistry::GetPayloadParserRegistry();
    OSCL_ASSERT(registry == NULL);
    PayloadParserRegistry::Init();
    registry = PayloadParserRegistry::GetPayloadParserRegistry();

    StrPtrLen aac_latm("audio/MP4A-LATM");
    StrPtrLen amr("audio/AMR");
    StrPtrLen amrwb("audio/AMR-WB");
    StrPtrLen h263_old("video/H263-1998");
    StrPtrLen h263("video/H263-2000");
    StrPtrLen m4v("video/MP4V-ES");
    StrPtrLen h264("video/H264");
    StrPtrLen mp4a(PVMF_MIME_MPEG4_AUDIO);
    StrPtrLen rfc3640("audio/mpeg4-generic");

    IPayloadParserFactory* m4vP = OSCL_NEW(M4VPayloadParserFactory, ());
    IPayloadParserFactory* aacP = OSCL_NEW(M4VAudioPayloadParserFactory, ());
    IPayloadParserFactory* amrP = OSCL_NEW(AmrPayloadParserFactory, ());
    IPayloadParserFactory* h263P = OSCL_NEW(H263PayloadParserFactory, ());
    IPayloadParserFactory* h264P = OSCL_NEW(H264PayloadParserFactory, ());
    IPayloadParserFactory* amrwbP = OSCL_NEW(AmrPayloadParserFactory, ());
    IPayloadParserFactory* rfc3640P = OSCL_NEW(RFC3640PayloadParserFactory, ());

    registry->addPayloadParserFactoryToRegistry(m4v, m4vP);
    registry->addPayloadParserFactoryToRegistry(h264, h264P);
    registry->addPayloadParserFactoryToRegistry(aac_latm, aacP);
    registry->addPayloadParserFactoryToRegistry(mp4a, aacP);
    registry->addPayloadParserFactoryToRegistry(amr, amrP);
    registry->addPayloadParserFactoryToRegistry(amrwb, amrwbP);
    registry->addPayloadParserFactoryToRegistry(h263_old, h263P);
    registry->addPayloadParserFactoryToRegistry(h263, h263P);
    registry->addPayloadParserFactoryToRegistry(rfc3640,  rfc3640P);
}


void PVMFSMFSPPVRBase::createContext(PvmiMIOSession aSession,
                                     PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFSMFSPPVRBase::DeleteContext(PvmiMIOSession aSession,
                                     PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFSMFSPPVRBase::DestroyPayloadParserRegistry()
{
    StrPtrLen aac_latm("audio/MP4A-LATM");
    StrPtrLen amr("audio/AMR");
    StrPtrLen amrwb("audio/AMR-WB");
    StrPtrLen h263("video/H263-2000");
    StrPtrLen m4v("video/MP4V-ES");
    StrPtrLen h264("video/H264");
    StrPtrLen rfc3640("audio/mpeg4-generic");

    PayloadParserRegistry* registry =
        PayloadParserRegistry::GetPayloadParserRegistry();
    if (registry == NULL) return;

    OsclMemoryFragment memFrag;

    memFrag.ptr = (OsclAny*)(m4v.c_str());
    memFrag.len = (uint32)m4v.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    memFrag.ptr = (OsclAny*)(h264.c_str());
    memFrag.len = (uint32)h264.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    memFrag.ptr = (OsclAny*)(aac_latm.c_str());
    memFrag.len = (uint32)aac_latm.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    memFrag.ptr = (OsclAny*)(amr.c_str());
    memFrag.len = (uint32)amr.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    memFrag.ptr = (OsclAny*)(amrwb.c_str());
    memFrag.len = (uint32)amrwb.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    memFrag.ptr = (OsclAny*)(h263.c_str());
    memFrag.len = (uint32)h263.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    memFrag.ptr = (OsclAny*)(rfc3640.c_str());
    memFrag.len = (uint32)rfc3640.size();
    OSCL_DELETE(registry->lookupPayloadParserFactory(memFrag));

    PayloadParserRegistry::Cleanup();
}

PVMFCommandId PVMFSMFSPPVRBase::DoGetMetadataKeys(PVMFSMFSPBaseNodeCommand& aCmd)
{
    return DoGetMetadataKeysBase(aCmd);
}

PVMFCommandId PVMFSMFSPPVRBase::DoGetMetadataValues(PVMFSMFSPBaseNodeCommand& aCmd)
{
    iNoOfValuesIteratedForValueVect = 0;
    iNoOfValuesPushedInValueVect = 0;
    PVMFStatus retval = GetPVRPluginSpecificValues(aCmd);
    if (PVMFSuccess != retval)
        return retval;
    return DoGetMetadataValuesBase(aCmd);
}

void PVMFSMFSPPVRBase::DoPrepare(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoPrepare - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
        {
            if (iGraphConstructComplete)
            {
                /*
                 * Connect the graph here. This is needed since we would send firewall packets
                 * as part of Prepare.
                 */
                if (GraphConnect() == false)
                {
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:DoPrepare - GraphConnect Failed"));
                    SetState(EPVMFNodeError);
                    PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
                    int32 errcode = PVMFStreamingManagerNodeGraphConnectFailed;
                    CommandComplete(iInputCommands, aCmd, PVMFFailure, NULL, &eventuuid, &errcode);
                    return;
                }

                /*
                 * Prepare for streaming manager cannot be completed unless Prepare
                 * for all the children nodes are complete
                 */
                PVMFSMFSPChildNodeContainerVector::iterator it;
                for (it = iFSPChildNodeContainerVec.begin(); it != iFSPChildNodeContainerVec.end(); it++)
                {
                    PVMFSMFSPCommandContext* internalCmd = RequestNewInternalCmd();
                    if (internalCmd != NULL)
                    {
                        internalCmd->cmd =
                            it->commandStartOffset +
                            PVMF_SM_FSP_NODE_INTERNAL_PREPARE_CMD_OFFSET;
                        internalCmd->parentCmd = aCmd.iCmd;

                        OsclAny *cmdContextData =
                            OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                        PVMFNodeInterface* iNode = it->iNode;

                        iNode->Prepare(it->iSessionId, cmdContextData);
                        it->iNodeCmdState = PVMFSMFSP_NODE_CMD_PENDING;
                    }
                    else
                    {
                        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:DoPrepare:RequestNewInternalCmd - Failed"));
                        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                        return;
                    }
                }

                MoveCmdToCurrentQueue(aCmd);
            }
            else
            {
                /* Graph construction not complete, so cant prep */
                PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:DoPrepare Failed - Incomplete Graph"));
                CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            }
        }
        break;

        default:
            PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:DoPrepare Failed - Invalid State"));
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoPrepare - Out"));
}

void PVMFSMFSPPVRBase::DoQueryDataSourcePosition(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoQueryDataSourcePosition - In"));

    PVMFTimestamp repositionrequestedstartnptinms = 0;
    PVMFTimestamp* actualrepositionstartnptinmsptr = NULL;
    bool seektosyncpoint = false;

    aCmd.PVMFSMFSPBaseNodeCommand::Parse(repositionrequestedstartnptinms,
                                         actualrepositionstartnptinmsptr,
                                         seektosyncpoint);

    if (actualrepositionstartnptinmsptr == NULL)
    {
        CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
        return;
    }
    *actualrepositionstartnptinmsptr = 0;

    // This query is not supported for streaming sessions
    CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoQueryDataSourcePosition - Out"));
    return;
}

void PVMFSMFSPPVRBase::DoReleasePort(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoReleasePort - In"));
    /*
     * Since the streaming manager does not have ports of its own,
     * a release port command typically translates to disconnecting
     * the underlying media layer port.
     */
    PVMFPortInterface* port;
    aCmd.PVMFSMFSPBaseNodeCommandBase::Parse((PVMFPortInterface*&)port);

    /*
     * Find TrackInfo that corresponds to the Media Layer Output port
     * on which the current relase is being called.
     */
    PVMFPVRBaseTrackInfoVector::iterator it;
    PVMFPVRBaseTrackInfo* trackInfo = NULL;

    for (it = iTrackInfoVec.begin();
            it != iTrackInfoVec.end();
            it++)
    {
        if (it->iMediaLayerOutputPort == port)
        {
            trackInfo = it;
            break;
        }
    }

    PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
    if (trackInfo == NULL)
    {
        PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::ReleasePort() - CmdFailed - PVMFErrArgument"));
        /* invalid port */
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMPVRFilePlaybackNode::DoReleasePort Failed - Invalid Port"));
        int32 errcode = PVMFStreamingManagerNodeErrorInvalidPort;
        CommandComplete(iInputCommands, aCmd, PVMFErrArgument, NULL, &eventuuid, &errcode);
        return;
    }
    PVMFStatus status = it->iMediaLayerOutputPort->Disconnect();

    if (status != PVMFSuccess)
    {
        PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::DoReleasePort Success"));
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::DoReleasePort Failed"));
        CommandComplete(iInputCommands, aCmd, PVMFErrPortProcessing);
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoReleasePort - Out"));
}

PVMFPVRBaseTrackInfo* PVMFSMFSPPVRBase::FindTrackInfo(uint32 atrackID)
{
    PVMFPVRBaseTrackInfoVector::iterator it;

    for (it = iTrackInfoVec.begin();
            it != iTrackInfoVec.end();
            it++)
    {
        if (it->trackID == atrackID)
        {
            return (it);
        }
    }
    return NULL;
}

void PVMFSMFSPPVRBase::DoRequestPort(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoRequestPort - In"));
    /*
     * This node supports port request only after the graph
     * has been fully constructed
     */
    if (iGraphConstructComplete)
    {
        /*
         * retrieve port tag
         */
        OSCL_String* mimetype;
        int32 tag;
        aCmd.PVMFSMFSPBaseNodeCommandBase::Parse(tag, mimetype);
        /*
         * Do not Allocate a new port. RTSP unicast node treats the output
         * port from the media layer as its own output port. Find the media
         * layer output port corresponding to the input mimetype and hand the
         * same out
         */
        PVMFPVRBaseTrackInfo* trackInfo = FindTrackInfo(tag);

        PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
        int32 errcode = PVMFStreamingManagerNodeErrorInvalidRequestPortTag;

        if (trackInfo == NULL)
        {
            PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::DoRequestPort: FindTrackInfo failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrArgument, NULL, &eventuuid, &errcode);
            return;
        }
        if (trackInfo->iMediaLayerOutputPort == NULL)
        {
            PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::DoRequestPort: iMediaLayerOutputPort NULL"));
            CommandComplete(iInputCommands, aCmd, PVMFFailure, NULL, &eventuuid, &errcode);
            return;
        }
        PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::DoRequestPort() - CmdComplete - PVMFSuccess"));
        /*
         * Return the port pointer to the caller.
         */
        CommandComplete(iInputCommands,
                        aCmd,
                        PVMFSuccess,
                        (OsclAny*)(trackInfo->iMediaLayerOutputPort));

        PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::DoRequestPort Success"));
    }
    else
    {
        PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::RequestPort() - CmdFailed - PVMFErrInvalidState"));
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::DoRequestPort Failed - InvalidState"));
        CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoRequestPort - Out"));
}

void PVMFSMFSPPVRBase::DoStop(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoStop - In"));
    iStreamID = 0;
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            /*
             * Stop for streaming manager cannot be completed unless
             * Stop for all the children nodes are complete
             */
            PVMFSMFSPChildNodeContainerVector::iterator it;
            for (it = iFSPChildNodeContainerVec.begin(); it != iFSPChildNodeContainerVec.end(); it++)
            {
                PVMFSMFSPCommandContext* internalCmd = RequestNewInternalCmd();
                if (internalCmd != NULL)
                {
                    internalCmd->cmd =
                        it->commandStartOffset +
                        PVMF_SM_FSP_NODE_INTERNAL_STOP_CMD_OFFSET;
                    internalCmd->parentCmd = aCmd.iCmd;

                    OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                    PVMFNodeInterface* iNode = it->iNode;

                    iNode->Stop(it->iSessionId, cmdContextData);
                    it->iNodeCmdState = PVMFSMFSP_NODE_CMD_PENDING;
                }
                else
                {
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:DoStop:RequestNewInternalCmd - Failed"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                    return;
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        break;

        default:
            PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::DoStop Failure - Invalid State"));
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoStop - Out"));
}

void PVMFSMFSPPVRBase::GetAcutalMediaTSAfterSeek()
{
    PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
    if (iJitterBufferNodeContainer == NULL)
    {
        OSCL_LEAVE(OsclErrBadHandle);
        return;
    }
    PVMFJitterBufferExtensionInterface* jbExtIntf =
        (PVMFJitterBufferExtensionInterface*)
        (iJitterBufferNodeContainer->iExtensions[0]);

    PVMFSMFSPChildNodeContainer* iMediaLayerNodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL)
    {
        OSCL_LEAVE(OsclErrBadHandle);
        return;
    }
    PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
        (PVMFMediaLayerNodeExtensionInterface*)
        (iMediaLayerNodeContainer->iExtensions[0]);
    if (mlExtIntf == NULL)
    {
        OSCL_LEAVE(OsclErrBadHandle);
        return;
    }

    if ((iSessionSourceInfo->_sessionType == PVMF_MIME_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_MIME_DATA_SOURCE_SDP_FILE))
    {

        iActualMediaDataTS = jbExtIntf->getActualMediaDataTSAfterSeek();
        if (iActualMediaDataTSPtr != NULL)
        {
            *iActualMediaDataTSPtr = iActualMediaDataTS;
            PVMF_SM_FSP_PVR_BASE_LOGCOMMANDREPOS((0, "PVMFSMFSPPVRBase::GetAcutalMediaTSAfterSeek - TargetNPT = %d, ActualNPT=%d, ActualMediaDataTS=%d",
                                                  iRepositionRequestedStartNPTInMS, *iActualRepositionStartNPTInMSPtr, *iActualMediaDataTSPtr));
        }
        if (iPVMFDataSourcePositionParamsPtr != NULL)
        {
            iPVMFDataSourcePositionParamsPtr->iActualMediaDataTS = iActualMediaDataTS;
            PVMF_SM_FSP_PVR_BASE_LOGCOMMANDREPOS((0, "PVMFSMFSPPVRBase::GetAcutalMediaTSAfterSeek - ActualMediaDataTS=%d",
                                                  iPVMFDataSourcePositionParamsPtr->iActualMediaDataTS));
        }
    }
}

uint32 PVMFSMFSPPVRBase::getCapabilityMetric(PvmiMIOSession aSession)
{
    OSCL_UNUSED_ARG(aSession);
    return 0;
}

PVMFStatus PVMFSMFSPPVRBase::GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo)
{
    SDPInfo* sdpInfo = iSdpInfo.GetRep();

    /* Get SDP Session Info */
    sessionDescription* sessionInfo = sdpInfo->getSessionInfo();

    RtspRangeType *sessionRange = OSCL_CONST_CAST(RtspRangeType*, (sessionInfo->getRange()));

    int32 sessionStartTime = 0, sessionStopTime = 0;

    sessionRange->convertToMilliSec(sessionStartTime, sessionStopTime);

    int32 duration_msec = (sessionStopTime - sessionStartTime);

    uint64 duration64;
    Oscl_Int64_Utils::set_uint64(duration64, 0, (uint32)duration_msec);

    if (sessionRange->end_is_set == true)
    {
        aInfo.setDurationValue(duration64);
        aInfo.setDurationTimeScale(1000);
    }
    else
    {
        aInfo.SetDurationAvailable(false);
    }

    aInfo.setSeekableFlag((!(sessionInfo->getRandomAccessDenied())));

    int32 numTracks = sdpInfo->getNumMediaObjects();

    SDPAltGroupType sdpAltGroupType = sessionInfo->getSDPAltGroupType();

    PVMF_TRACK_INFO_TRACK_ALTERNATE_TYPE altType = PVMF_TRACK_ALTERNATE_TYPE_UNDEFINED;

    if (sdpAltGroupType == SDP_ALT_GROUP_LANGUAGE)
    {
        altType = PVMF_TRACK_ALTERNATE_TYPE_LANGUAGE;
    }
    else if (sdpAltGroupType == SDP_ALT_GROUP_BANDWIDTH)
    {
        altType = PVMF_TRACK_ALTERNATE_TYPE_BANDWIDTH;
    }

    for (int32 i = 0; i < numTracks; i++)
    {
        /*
         * Get the vector of mediaInfo as there can
         * alternates for each track
         */
        Oscl_Vector<mediaInfo*, SDPParserAlloc> mediaInfoVec =
            sdpInfo->getMediaInfo(i);

        uint32 minfoVecLen = mediaInfoVec.size();

        for (uint32 j = 0; j < minfoVecLen; j++)
        {
            mediaInfo* mInfo = mediaInfoVec[j];

            if (mInfo == NULL)
            {
                return PVMFFailure;
            }

            RtspRangeType *mediaRange = mInfo->getRtspRange();

            int32 mediaStartTime = 0, mediaStopTime = 0;

            mediaRange->convertToMilliSec(mediaStartTime, mediaStopTime);
            int32 mediaDuration_ms = mediaStopTime - mediaStartTime;
            uint64 mediaDuration64;
            Oscl_Int64_Utils::set_uint64(mediaDuration64, 0, (uint32)mediaDuration_ms);

            PVMFTrackInfo trackInfo;

            Oscl_Vector<PayloadSpecificInfoTypeBase*, SDPParserAlloc> payloadVector;
            payloadVector = mInfo->getPayloadSpecificInfoVector();

            if (payloadVector.size() == 0)
            {
                return PVMFFailure;
            }
            /*
             * There can be multiple payloads per media segment.
             * We only support one for now, so
             * use just the first payload
             */
            PayloadSpecificInfoTypeBase* payloadInfo = payloadVector[0];

            // set config for later
            int32 configSize = payloadInfo->configSize;
            OsclAny* config = payloadInfo->configHeader.GetRep();

            OSCL_StackString<256> mimeString;
            const char* mimeType = mInfo->getMIMEType();
            mimeString += mimeType;

            trackInfo.setTrackMimeType(mimeString);

            uint32 trackID = mInfo->getMediaInfoID();

            trackInfo.setTrackID(trackID);
            trackInfo.setPortTag(trackID);

            trackInfo.setTrackBitRate(mInfo->getBitrate());

            if (mediaRange->end_is_set == true)
            {
                trackInfo.setTrackDurationValue(mediaDuration64);
            }
            else
            {
                trackInfo.SetDurationAvailable(false);
            }

            if ((configSize > 0) && (config != NULL))
            {
                OsclMemAllocDestructDealloc<uint8> my_alloc;
                OsclRefCounter* my_refcnt;
                uint aligned_refcnt_size =
                    oscl_mem_aligned_size(sizeof(OsclRefCounterSA< OsclMemAllocDestructDealloc<uint8> >));

                uint8* my_ptr = GetMemoryChunk(my_alloc, aligned_refcnt_size + configSize);
                if (!my_ptr)
                    return false;

                my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterSA< OsclMemAllocDestructDealloc<uint8> >(my_ptr));
                my_ptr += aligned_refcnt_size;

                OsclMemoryFragment memfrag;
                memfrag.len = (uint32)configSize;
                memfrag.ptr = my_ptr;

                oscl_memcpy((void*)(memfrag.ptr), (const void*)config, memfrag.len);

                OsclRefCounterMemFrag tmpRefcntMemFrag(memfrag, my_refcnt, memfrag.len);
                trackInfo.setTrackConfigInfo(tmpRefcntMemFrag);
            }

            int32 dependsOnTrackID = mInfo->getDependsOnTrackID();

            if (dependsOnTrackID != -1)
            {
                trackInfo.setDependsOn();
                mediaInfo* baseMediaInfo = sdpInfo->getMediaInfoBasedOnDependsOnID(dependsOnTrackID);
                if (baseMediaInfo == NULL)
                {
                    return PVMFFailure;
                }
                trackInfo.addDependsOnTrackID(baseMediaInfo->getMediaInfoID());
            }

            if (altType != PVMF_TRACK_ALTERNATE_TYPE_UNDEFINED)
            {
                /* Expose alternate track ids */
                trackInfo.setTrackAlternates(altType);
                for (uint32 k = 0; k < minfoVecLen; k++)
                {
                    mediaInfo* mInfo = mediaInfoVec[k];
                    if (mInfo == NULL)
                    {
                        return PVMFFailure;
                    }
                    uint32 altID = mInfo->getMediaInfoID();
                    if (altID != trackID)
                    {
                        trackInfo.addAlternateTrackID((int32)altID);
                    }
                }
            }
            aInfo.addTrackInfo(trackInfo);
        }
    }
    iCompleteMediaPresetationInfo = aInfo;
    return PVMFSuccess;
}

uint32 PVMFSMFSPPVRBase::GetNumMetadataKeys(char* aQueryKeyString)
{
    //Metadata is avaialable in three forms
    //1. Metadata common to streaming of all type of payloads and FF specific metadata
    //2. Streaming specific metadata
    //3. CPM metadata
    //First two types are avaiable in iAvailableMetaDatakeys vector
    //Third type can be had from metadataextension interface
    //base class considers count of all of these
    return PVMFSMFSPBaseNode::GetNumMetadataKeysBase(aQueryKeyString);
}

uint32 PVMFSMFSPPVRBase::GetNumMetadataValues(PVMFMetadataList& aKeyList)
{
    //Metadata is avaialable in three forms
    //1. Metadata common to streaming of all type of payloads and FF specific metadata
    //2. Streaming specific metadata
    //3. CPM metadata
    //First two types are avaiable in iAvailableMetaDatakeys vector
    //Third type can be had from metadataextension interface
    //Base class considers count of all of these
    return PVMFSMFSPBaseNode::GetNumMetadataValuesBase(aKeyList);
}

PVMFStatus PVMFSMFSPPVRBase::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
        PvmiKvp*& aParameters, int& aNumParamElements,
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
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::getParametersSync() Invalid key string"));
        return PVMFErrArgument;
    }

    // Retrieve the second component from the key string
    pv_mime_string_extract_type(1, aIdentifier, compstr);

    // Check if it is key string for streaming manager
    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) < 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::getParametersSync() Unsupported key"));
        return PVMFFailure;
    }


    if (compcount == 2)
    {
        // Since key is "x-pvmf/net" return all
        // nodes available at this level. Ignore attribute
        // since capability is only allowed

        // Allocate memory for the KVP list
        aParameters = (PvmiKvp*)oscl_malloc(StreamingManagerConfig_NumBaseKeys * sizeof(PvmiKvp));
        if (aParameters == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::getParametersSync() Memory allocation for KVP failed"));
            return PVMFErrNoMemory;
        }
        oscl_memset(aParameters, 0, StreamingManagerConfig_NumBaseKeys*sizeof(PvmiKvp));
        // Allocate memory for the key strings in each KVP
        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(StreamingManagerConfig_NumBaseKeys * SMCONFIG_KEYSTRING_SIZE * sizeof(char));
        if (memblock == NULL)
        {
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::getParametersSync() Memory allocation for key string failed"));
            return PVMFErrNoMemory;
        }
        oscl_strset(memblock, 0, StreamingManagerConfig_NumBaseKeys*SMCONFIG_KEYSTRING_SIZE*sizeof(char));
        // Assign the key string buffer to each KVP
        uint32 j;
        for (j = 0; j < StreamingManagerConfig_NumBaseKeys; ++j)
        {
            aParameters[j].key = memblock + (j * SMCONFIG_KEYSTRING_SIZE);
        }
        // Copy the requested info
        for (j = 0; j < StreamingManagerConfig_NumBaseKeys; ++j)
        {
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/net/"), 17);
            oscl_strncat(aParameters[j].key, StreamingManagerConfig_BaseKeys[j].iString, oscl_strlen(StreamingManagerConfig_BaseKeys[j].iString));
            oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type="), 6);
            switch (StreamingManagerConfig_BaseKeys[j].iType)
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
            switch (StreamingManagerConfig_BaseKeys[j].iValueType)
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

                case PVMI_KVPVALTYPE_BOOL:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
                    break;

                case PVMI_KVPVALTYPE_UINT32:
                default:
                    oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
                    break;
            }
            aParameters[j].key[SMCONFIG_KEYSTRING_SIZE-1] = 0;
        }

        aNumParamElements = StreamingManagerConfig_NumBaseKeys;
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
        for (i = 0; i < StreamingManagerConfig_NumBaseKeys; i++)
        {
            if (pv_mime_strcmp(compstr, (char*)(StreamingManagerConfig_BaseKeys[i].iString)) >= 0)
            {
                break;
            }
        }

        if (i == StreamingManagerConfig_NumBaseKeys)
        {
            // no match found
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFSMFSPPVRBase:getParametersSync() Unsupported key"));
            return PVMFErrArgument;
        }

        PVMFStatus retval = GetConfigParameter(aParameters, aNumParamElements, i, reqattr);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFSMFSPPVRBase::getParametersSync() "
                             "Retrieving streaming manager parameter failed"));
            return retval;
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::getParametersSync() Unsupported key"));
        return PVMFErrArgument;
    }

    return PVMFSuccess;
}


void PVMFSMFSPPVRBase::HandleMediaLayerCommandCompleted(const PVMFCmdResp& aResponse, bool& aPerformErrHandling)
{
    aPerformErrHandling = false;
    PVMFSMFSPChildNodeContainer* iMediaLayerNodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL)
    {
        OSCL_LEAVE(OsclErrBadHandle);
        return;
    }

    PVMFSMFSPCommandContext *cmdContextData =
        OSCL_REINTERPRET_CAST(PVMFSMFSPCommandContext*, aResponse.GetContext());
    cmdContextData->oFree = true;

    PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::HandleMediaLayerCommandCompleted In - cmd [%d] iMediaLayerNodeContainer->iNodeCmdState [%d] iInterfaceState[%d]", cmdContextData->cmd, iMediaLayerNodeContainer->iNodeCmdState, iInterfaceState));

    OSCL_ASSERT(cmdContextData->cmd != PVMF_SM_FSP_MEDIA_LAYER_QUERY_UUID);
    OSCL_ASSERT(cmdContextData->cmd != PVMF_SM_FSP_MEDIA_LAYER_QUERY_INTERFACE);
    if (iMediaLayerNodeContainer->iNodeCmdState == PVMFSMFSP_NODE_CMD_PENDING)
    {
        if (cmdContextData->cmd == PVMF_SM_FSP_MEDIA_LAYER_REQUEST_PORT)
        {
            //This is last of the request ports
            OSCL_ASSERT(iMediaLayerNodeContainer->iNumRequestPortsPending > 0);
            if (--iMediaLayerNodeContainer->iNumRequestPortsPending == 0)
            {
                iMediaLayerNodeContainer->iNodeCmdState = PVMFSMFSP_NODE_CMD_IDLE;
            }
        }
        else
        {
            iMediaLayerNodeContainer->iNodeCmdState = PVMFSMFSP_NODE_CMD_IDLE;
        }
    }
    else if (iMediaLayerNodeContainer->iNodeCmdState == PVMFSMFSP_NODE_CMD_CANCEL_PENDING)
    {
        if ((cmdContextData->parentCmd == PVMF_SMFSP_NODE_CANCELALLCOMMANDS) || (cmdContextData->parentCmd == PVMF_SMFSP_NODE_CANCELCOMMAND) || (cmdContextData->parentCmd == PVMF_SMFSP_NODE_CANCEL_DUE_TO_ERROR))
        {
            iMediaLayerNodeContainer->iNodeCmdState = PVMFSMFSP_NODE_CMD_IDLE;
        }
        else
        {
            PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::HandleMediaLayerCommandCompleted cmd completion for cmd other than cancel during cancellation"));
            //if cancel is pending and if the parent cmd is not cancel then this is
            //is most likely the cmd that is being cancelled.
            //we ignore cmd completes from child nodes if cancel is pending
            //we simply wait on cancel complete and cancel the pending cmd
            return;
        }
    }
    else if (iMediaLayerNodeContainer->iNodeCmdState == PVMFSMFSP_NODE_CMD_IDLE)
    {
        PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::HandleMediaLayerCommandCompleted container in IDLE state already"));
        /*
         * This is to handle a usecase where a node reports cmd complete for cancelall first
         * and then reports cmd complete on the cmd that was meant to be cancelled.
         * There are two possible scenarios that could arise based on this:
         * i) SM node has reported cmd complete on both canceall and the cmd meant to be cancelled
         * to engine by the time cmd complete on the cmd that was meant to be cancelled arrives
         * from the child node. In this case iNodeCmdState would be PVMFSMFSP_NODE_CMD_NO_PENDING.
         * ii) SM node is still waiting on some other child nodes to complete cancelall.
         * In this case iNodeCmdState would be PVMFSMFSP_NODE_CMD_IDLE.
         * In either case iNodeCmdState cannot be PVMFSMFSP_NODE_CMD_PENDING or PVMFSMFSP_NODE_CMD_IDLE
         * (recall that we call ResetNodeContainerCmdState  prior to issuing cancelall)
         * Or this is the case of node reporting cmd complete multiple times for a cmd, which
         * also can be ignored
         */
        return;
    }

    if (EPVMFNodeError == iInterfaceState)//If interface is in err state, let the err handler do processing
    {
        aPerformErrHandling = true;
        return;
    }

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        if (aResponse.GetCmdStatus() != PVMFErrCancelled)
        {
            aPerformErrHandling = true;
        }

        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::HandleMediaLayerCommandCompleted - Command failed - context=0x%x, status=0x%x", aResponse.GetContext(), aResponse.GetCmdStatus()));
        if (IsBusy())
        {
            Cancel();
            RunIfNotReady();
        }
        return;
    }

    switch (cmdContextData->cmd)
    {
        case PVMF_SM_FSP_MEDIA_LAYER_INIT:
            CompleteInit();
            break;

        case PVMF_SM_FSP_MEDIA_LAYER_PREPARE:
            CompletePrepare();
            break;

        case PVMF_SM_FSP_MEDIA_LAYER_START:
        {
            CompleteStart();
        }
        break;

        case PVMF_SM_FSP_MEDIA_LAYER_STOP:
        {
            CompleteStop();
        }
        break;

        case PVMF_SM_FSP_MEDIA_LAYER_FLUSH:
            CompleteFlush();
            break;

        case PVMF_SM_FSP_MEDIA_LAYER_PAUSE:
            CompletePause();
            break;

        case PVMF_SM_FSP_MEDIA_LAYER_RESET:
            CompleteReset();
            break;

        case PVMF_SM_FSP_MEDIA_LAYER_REQUEST_PORT:
        {
            PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
                (PVMFMediaLayerNodeExtensionInterface*)
                (iMediaLayerNodeContainer->iExtensions[0]);

            if (mlExtIntf == NULL)
            {
                OSCL_LEAVE(OsclErrBadHandle);
                return;
            }

            /*
             * Save the port in TrackInfo
             */
            PVMFPVRBaseTrackInfo* trackInfo =
                FindTrackInfo(cmdContextData->portContext.trackID);

            PVMFPortInterface* port =
                (PVMFPortInterface*)aResponse.GetEventData();
            OSCL_ASSERT(trackInfo && port);
            if (!trackInfo || !port)
                return;

            if (cmdContextData->portContext.portTag ==
                    PVMF_MEDIALAYER_PORT_TYPE_INPUT)
            {
                if (trackInfo)
                    trackInfo->iMediaLayerInputPort = port;
                iMediaLayerNodeContainer->iInputPorts.push_back(port);
            }
            else if (cmdContextData->portContext.portTag ==
                     PVMF_MEDIALAYER_PORT_TYPE_OUTPUT)
            {
                if (trackInfo)
                    trackInfo->iMediaLayerOutputPort = port;
                iMediaLayerNodeContainer->iOutputPorts.push_back(port);
                uint32 preroll32 = 0;
                bool live = false;
                mlExtIntf->setOutPortStreamParams(port,
                                                  cmdContextData->portContext.trackID,
                                                  preroll32,
                                                  live);
            }
            mediaInfo* mInfo = NULL;

            SDPInfo* sdpInfo = iSdpInfo.GetRep();
            if (sdpInfo == NULL)
            {
                OSCL_LEAVE(OsclErrBadHandle);
                return;
            }
            if (trackInfo)
            {
                mInfo = sdpInfo->getMediaInfoBasedOnID(trackInfo->trackID);
                mlExtIntf->setPortMediaParams(port, trackInfo->iTrackConfig, mInfo);
            }
            CompleteGraphConstruct();
        }
        break;

        case PVMF_SM_FSP_MEDIA_LAYER_CANCEL_ALL_COMMANDS:
        {
            CompleteChildNodesCmdCancellation();
        }
        break;

        default:
            break;
    }
    return;
}

bool PVMFSMFSPPVRBase::IsFSPInternalCmd(PVMFCommandId aId)
{
    OSCL_UNUSED_ARG(aId);
    return false;
}

void PVMFSMFSPPVRBase::NodeCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::NodeCommandCompleted"));
    bool performErrHandling = false;
    HandleChildNodeCommandCompletion(aResponse, performErrHandling);
    PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::NodeCommandCompleted - performErrHandling[%d]", performErrHandling));

    if (performErrHandling == true)
    {
        HandleError(aResponse);
    }

    return;
}

PVMFStatus PVMFSMFSPPVRBase::PopulateAvailableMetadataKeys()
{
    int32 leavecode = OsclErrNone;
    OSCL_TRY(leavecode,
             PVMFSMFSPBaseNode::PopulateAvailableMetadataKeys();

             //Add feature specific streaming metadata keys
             // Create the parameter string for the index range
             if (iMetaDataInfo->iNumTracks > 0)
{
    char indexparam[18];
        oscl_snprintf(indexparam, 18, ";index=0...%d", (iMetaDataInfo->iNumTracks - 1));
        indexparam[17] = NULL_TERM_CHAR;

        iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_BITRATE_KEY);
        iAvailableMetadataKeys[0] += indexparam;
    }
    iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_PAUSE_DENIED_KEY);
            );

    if (leavecode != OsclErrNone)
        return leavecode;
    else
        return PVMFSuccess;
}

void PVMFSMFSPPVRBase::PopulateDRMInfo()
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::PopulateDRMInfo() In"));
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::PopulateDRMInfo() - CPM not supported yet"));
}

bool PVMFSMFSPPVRBase::PopulateTrackInfoVec()
{
    if (iSelectedMediaPresetationInfo.getNumTracks() == 0)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:PopulateTrackInfoVec - Selected Track List Empty"));
        return false;
    }

    /*
     * Get selected tracks
     */

    int32 numTracks = iSdpInfo->getNumMediaObjects();

    if (numTracks > 0)
    {
        for (int32 i = 0; i < numTracks; i++)
        {
            /*
             * Get the vector of mediaInfo as there can
             * alternates for each track
             */
            Oscl_Vector<mediaInfo*, SDPParserAlloc> mediaInfoVec =
                iSdpInfo->getMediaInfo(i);

            uint32 minfoVecLen = mediaInfoVec.size();
            for (uint32 j = 0; j < minfoVecLen; j++)
            {
                mediaInfo* mInfo = mediaInfoVec[j];

                if (mInfo == NULL)
                {
                    return false;
                }

                if (mInfo->getSelect())
                {
                    PVMFPVRBaseTrackInfo trackInfo;

                    trackInfo.trackID = mInfo->getMediaInfoID();
                    trackInfo.iTransportType += _STRLIT_CHAR("RTP");

                    Oscl_Vector<PayloadSpecificInfoTypeBase*, SDPParserAlloc> payloadVector;
                    payloadVector = mInfo->getPayloadSpecificInfoVector();

                    if (payloadVector.size() == 0)
                    {
                        return false;
                    }
                    /*
                     * There can be multiple payloads per media segment.
                     * We only support one for now, so
                     * use just the first payload
                     */
                    PayloadSpecificInfoTypeBase* payloadInfo = payloadVector[0];
                    trackInfo.trackTimeScale = payloadInfo->sampleRate;

                    // set config for later
                    int32 configSize = payloadInfo->configSize;
                    OsclAny* config = payloadInfo->configHeader.GetRep();

                    const char* mimeType = mInfo->getMIMEType();
                    trackInfo.iMimeType += mimeType;
                    trackInfo.portTag = mInfo->getMediaInfoID();
                    trackInfo.bitRate = mInfo->getBitrate();
                    if (mInfo->getReportFrequency() > 0)
                    {
                        trackInfo.iRateAdaptation = true;
                        trackInfo.iRateAdaptationFeedBackFrequency =
                            mInfo->getReportFrequency();
                    }

                    if ((mInfo->getRTCPReceiverBitRate() >= 0) &&
                            (mInfo->getRTCPSenderBitRate() >= 0))
                    {
                        trackInfo.iRR = mInfo->getRTCPReceiverBitRate();
                        trackInfo.iRS = mInfo->getRTCPSenderBitRate();
                        trackInfo.iRTCPBwSpecified = true;
                    }

                    if ((configSize > 0) && (config != NULL))
                    {
                        OsclMemAllocDestructDealloc<uint8> my_alloc;
                        OsclRefCounter* my_refcnt;
                        uint aligned_refcnt_size =
                            oscl_mem_aligned_size(sizeof(OsclRefCounterSA< OsclMemAllocDestructDealloc<uint8> >));

                        uint8* my_ptr = GetMemoryChunk(my_alloc, aligned_refcnt_size + configSize);
                        if (!my_ptr)
                            return false;

                        my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterSA< OsclMemAllocDestructDealloc<uint8> >(my_ptr));
                        my_ptr += aligned_refcnt_size;

                        OsclMemoryFragment memfrag;
                        memfrag.len = (uint32)configSize;
                        memfrag.ptr = my_ptr;

                        oscl_memcpy((void*)(memfrag.ptr), (const void*)config, memfrag.len);

                        OsclRefCounterMemFrag tmpRefcntMemFrag(memfrag, my_refcnt, memfrag.len);
                        trackInfo.iTrackConfig = tmpRefcntMemFrag;
                    }
                    iTrackInfoVec.push_back(trackInfo);
                }
            }
        }
    }
    else
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:PopulateTrackInfoVec - Selected Track List Empty"));
        return false;
    }

    return true;
}

bool PVMFSMFSPPVRBase::ProcessCommand(PVMFSMFSPBaseNodeCommand& aCmd)
{
    if (EPVMFNodeError == iInterfaceState)
    {
        if (iCurrErrHandlingCommand.size() > 0)
        {
            return false;
        }
        switch (aCmd.iCmd)
        {
            case PVMF_SMFSP_NODE_CANCEL_DUE_TO_ERROR:
                DoCancelAllPendingCommands(aCmd);
                break;
            case PVMF_SMFSP_NODE_RESET_DUE_TO_ERROR:
                DoResetDueToErr(aCmd);
                break;
        }

        return true;
    }


    /*
     * normally this node will not start processing one command
     * until the prior one is finished.  However, a hi priority
     * command such as Cancel must be able to interrupt a command
     * in progress.
     */
    if ((iCurrentCommand.size() > 0 && !aCmd.hipri()
            && aCmd.iCmd != PVMF_SMFSP_NODE_CANCEL_GET_LICENSE)
            || iCancelCommand.size() > 0)
        return false;

    OSCL_ASSERT(aCmd.iCmd != PVMF_SMFSP_NODE_QUERYUUID);
    OSCL_ASSERT(aCmd.iCmd != PVMF_SMFSP_NODE_SET_DATASOURCE_RATE);
    switch (aCmd.iCmd)
    {
            /* node interface commands */
        case PVMF_SMFSP_NODE_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;
        case PVMF_SMFSP_NODE_INIT:
            DoInit(aCmd);
            break;
        case PVMF_SMFSP_NODE_PREPARE:
            DoPrepare(aCmd);
            break;
        case PVMF_SMFSP_NODE_REQUESTPORT:
            DoRequestPort(aCmd);
            break;
        case PVMF_SMFSP_NODE_RELEASEPORT:
            DoReleasePort(aCmd);
            break;
        case PVMF_SMFSP_NODE_START:
            DoStart(aCmd);
            break;
        case PVMF_SMFSP_NODE_STOP:
            DoStop(aCmd);
            break;
        case PVMF_SMFSP_NODE_FLUSH:
            DoFlush(aCmd);
            break;
        case PVMF_SMFSP_NODE_PAUSE:
            DoPause(aCmd);
            break;
        case PVMF_SMFSP_NODE_RESET:
            DoReset(aCmd);
            break;
        case PVMF_SMFSP_NODE_CANCELALLCOMMANDS:
            DoCancelAllCommands(aCmd);
            break;
        case PVMF_SMFSP_NODE_CANCELCOMMAND:
            DoCancelCommand(aCmd);
            break;

            /* add extention interface commands */
        case PVMF_SMFSP_NODE_SET_DATASOURCE_POSITION:
            if (iPlayListRepositioning == true)
            {
                DoSetDataSourcePositionPlayList(aCmd);
            }
            else
            {
                DoSetDataSourcePosition(aCmd);
            }
            break;

        case PVMF_SMFSP_NODE_QUERY_DATASOURCE_POSITION:
            DoQueryDataSourcePosition(aCmd);
            break;

        case PVMF_SMFSP_NODE_GETNODEMETADATAKEYS:
        {
            PVMFStatus status = DoGetMetadataKeys(aCmd);
            if (status != PVMFPending)
            {
                CommandComplete(iInputCommands, aCmd, status);
            }
            else
            {
                MoveCmdToCurrentQueue(aCmd);
            }
        }
        break;
        case PVMF_SMFSP_NODE_GETNODEMETADATAVALUES:
        {
            PVMFStatus status = DoGetMetadataValues(aCmd);
            if (status != PVMFPending)
            {
                CommandComplete(iInputCommands, aCmd, status);
            }
            else
            {
                MoveCmdToCurrentQueue(aCmd);
            }
        }
        break;
        case PVMF_SMFSP_NODE_GET_LICENSE_W:
        {
            PVMFStatus status = DoGetLicense(aCmd, true);
            if (status == PVMFPending)
            {
                MoveCmdToCurrentQueue(aCmd);
            }
            else
            {
                CommandComplete(iInputCommands, aCmd, status);
            }
        }
        break;
        case PVMF_SMFSP_NODE_GET_LICENSE:
        {
            PVMFStatus status = DoGetLicense(aCmd);
            if (status == PVMFPending)
            {
                MoveCmdToCurrentQueue(aCmd);
            }
            else
            {
                CommandComplete(iInputCommands, aCmd, status);
            }
        }
        break;
        case PVMF_SMFSP_NODE_CANCEL_GET_LICENSE:
            DoCancelGetLicense(aCmd);
            break;
        case PVMF_SMFSP_NODE_CAPCONFIG_SETPARAMS:
        {
            PvmiMIOSession session;
            PvmiKvp* aParameters;
            int num_elements;
            PvmiKvp** ppRet_kvp;
            aCmd.Parse(session, aParameters, num_elements, ppRet_kvp);
            setParametersSync(NULL, aParameters, num_elements, *ppRet_kvp);
            ciObserver->SignalEvent(aCmd.iId);
        }
        break;

        /* internal commands common to all types of streaming*/
        case PVMF_SMFSP_NODE_CONSTRUCT_SESSION:	//to construct the graph
        {
            PVMFStatus status = DoGraphConstruct();
            if (status != PVMFPending)
            {
                InternalCommandComplete(iInputCommands, aCmd, status);
            }
            else
            {
                MoveCmdToCurrentQueue(aCmd);
            }
        }
        break;

        /* unknown commands */
        default:
            /* unknown command type */
            CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
            break;
    }
    return true;
}

void PVMFSMFSPPVRBase::ReleaseChildNodesExtentionInterface()
{
    uint32 i, j;
    for (i = 0; i < iFSPChildNodeContainerVec.size(); i++)
    {
        for (j = 0; j < iFSPChildNodeContainerVec[i].iExtensions.size(); j++)
        {
            PVInterface* extIntf = iFSPChildNodeContainerVec[i].iExtensions[j];
            extIntf->removeRef();
            extIntf = NULL;
        }
        iFSPChildNodeContainerVec[i].iExtensions.clear();
    }
}

PVMFStatus PVMFSMFSPPVRBase::ReleaseNodeMetadataKeys(PVMFMetadataList& aKeyList,
        uint32 aStartingKeyIndex,
        uint32 aEndKeyIndex)
{
    //no allocation for any keys took in derived class so just calling base class release functions
    return ReleaseNodeMetadataKeysBase(aKeyList, aStartingKeyIndex, aEndKeyIndex);
}

PVMFStatus PVMFSMFSPPVRBase::ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
        uint32 aStartingValueIndex,
        uint32 aEndValueIndex)
{
    return ReleaseNodeMetadataValuesBase(aValueList, aStartingValueIndex, aEndValueIndex);
}

PVMFStatus PVMFSMFSPPVRBase::releaseParameters(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFSMFSPPVRBase::releaseParameters() In"));

    if (aParameters == NULL || num_elements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFSMFSPPVRBase::releaseParameters() KVP list is NULL or number of elements is 0"));
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
                        (0, "PVMFSMFSPPVRBase::releaseParameters() Unsupported key"));
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
                                    (0, "PVMFSMFSPPVRBase::releaseParameters() Valtype not specified in key string"));
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
                    (0, "PVMFSMFSPPVRBase::releaseParameters() Out"));
    return PVMFSuccess;
}

bool PVMFSMFSPPVRBase::RequestJitterBufferPorts(int32 portType,
        uint32 &numPortsRequested)
{
    PVMFSMFSPChildNodeContainer* nodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);

    if (nodeContainer == NULL)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:RequestJitterBufferPorts - getChildNodeContainer Failed"));
        return false;
    }

    numPortsRequested = 0;
    /*
     * Request port - all jitter buffer input ports
     * are even numbered and output and rtcp ports are odd numbered
     */
    int32 portTagStart = portType;

    if ((iSessionSourceInfo->_sessionType == PVMF_MIME_DATA_SOURCE_RTSP_URL)
            || (iSessionSourceInfo->_sessionType == PVMF_MIME_DATA_SOURCE_SDP_FILE)
       )
    {
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFPVRBaseTrackInfo trackInfo = iTrackInfoVec[i];

            PVMFSMFSPCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    nodeContainer->commandStartOffset +
                    PVMF_SM_FSP_NODE_INTERNAL_REQUEST_PORT_OFFSET;
                internalCmd->parentCmd = PVMF_SMFSP_NODE_CONSTRUCT_SESSION;
                internalCmd->portContext.trackID = trackInfo.trackID;
                internalCmd->portContext.portTag = portType;

                OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                PVMFNodeInterface* iNode = nodeContainer->iNode;

                OSCL_StackString<32> portConfig = trackInfo.iTransportType;
                portConfig += _STRLIT_CHAR("/");
                portConfig += trackInfo.iMimeType;
                iNode->RequestPort(nodeContainer->iSessionId,
                                   portTagStart,
                                   &(portConfig),
                                   cmdContextData);
                numPortsRequested++;
                nodeContainer->iNumRequestPortsPending++;
                nodeContainer->iNodeCmdState = PVMFSMFSP_NODE_CMD_PENDING;
            }
            else
            {
                PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:RequestJitterBufferPorts - RequestNewInternalCmd Failed"));
                return false;
            }
            portTagStart += 3;
        }
        return true;
    }
    return false;
}

bool PVMFSMFSPPVRBase::RequestMediaLayerPorts(int32 portType,
        uint32& numPortsRequested)
{
    PVMFSMFSPChildNodeContainer* nodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_MEDIA_LAYER_NODE);

    if (nodeContainer == NULL)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:RequestMediaLayerPorts - getChildNodeContainer Failed"));
        return false;
    }

    numPortsRequested = 0;
    /*
     * Request port - all media layer input ports
     * are even numbered and output are odd numbered
     */
    int32 portTagStart = portType;

    if ((iSessionSourceInfo->_sessionType == PVMF_MIME_DATA_SOURCE_RTSP_URL)
            || (iSessionSourceInfo->_sessionType == PVMF_MIME_DATA_SOURCE_SDP_FILE)
       )
    {
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFPVRBaseTrackInfo trackInfo = iTrackInfoVec[i];

            PVMFSMFSPCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    nodeContainer->commandStartOffset +
                    PVMF_SM_FSP_NODE_INTERNAL_REQUEST_PORT_OFFSET;
                internalCmd->parentCmd = PVMF_SMFSP_NODE_CONSTRUCT_SESSION;
                internalCmd->portContext.trackID = trackInfo.trackID;
                internalCmd->portContext.portTag = portType;

                OsclAny *cmdContextData =
                    OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                PVMFNodeInterface* iNode = nodeContainer->iNode;

                iNode->RequestPort(nodeContainer->iSessionId,
                                   portTagStart,
                                   &(trackInfo.iMimeType),
                                   cmdContextData);
                numPortsRequested++;
                nodeContainer->iNumRequestPortsPending++;
                nodeContainer->iNodeCmdState = PVMFSMFSP_NODE_CMD_PENDING;
            }
            else
            {
                PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:RequestMediaLayerPorts - RequestNewInternalCmd Failed"));
                return false;
            }
            portTagStart += 2;
        }

        return true;
    }

    //error
    return false;
}

void PVMFSMFSPPVRBase::ResetStopCompleteParams()
{
    iPlaylistPlayInProgress = false;
    iRepositionRequestedStartNPTInMS = 0;
}

PVMFStatus PVMFSMFSPPVRBase::SelectTracks(PVMFMediaPresentationInfo& aInfo)
{
    SDPInfo* sdpInfo = iSdpInfo.GetRep();
    if (sdpInfo == NULL)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:SelectTracks - SDP Not Available"));
        return PVMFErrArgument;
    }

    int32 numTracks = aInfo.getNumTracks();

    for (int32 i = 0; i < numTracks; i++)
    {
        PVMFTrackInfo* trackInfo = aInfo.getTrackInfo(i);

        uint32 trackID = trackInfo->getTrackID();

        mediaInfo* mInfo =
            sdpInfo->getMediaInfoBasedOnID(trackID);

        if (mInfo == NULL)
        {
            PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase:SelectTracks - Invalid SDP TrackID"));
            return PVMFErrArgument;
        }

        mInfo->setSelect();

        /* Set selected field in meta info */
        Oscl_Vector<PVMFSMTrackMetaDataInfo, OsclMemAllocator>::iterator it;
        for (it = iMetaDataInfo->iTrackMetaDataInfoVec.begin(); it != iMetaDataInfo->iTrackMetaDataInfoVec.end(); it++)
        {
            if (it->iTrackID == trackID)
            {
                it->iTrackSelected = true;
            }
        }
    }

    iSelectedMediaPresetationInfo = aInfo;
    return PVMFSuccess;
}

PVMFStatus PVMFSMFSPPVRBase::SetClientPlayBackClock(PVMFMediaClock* aClientClock)
{
    PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);

    if (iJitterBufferNodeContainer == NULL)
    {
        OSCL_LEAVE(OsclErrBadHandle);
        return PVMFFailure;
    }

    PVMFJitterBufferExtensionInterface* jbExtIntf =
        (PVMFJitterBufferExtensionInterface*)
        (iJitterBufferNodeContainer->iExtensions[0]);

    jbExtIntf->setClientPlayBackClock(aClientClock);

    PVMFSMFSPChildNodeContainer* iMediaLayerNodeContainer =
        getChildNodeContainer(PVMF_SM_FSP_MEDIA_LAYER_NODE);

    if (iMediaLayerNodeContainer == NULL)
    {
        OSCL_LEAVE(OsclErrBadHandle);
        return  PVMFFailure;
    }

    PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
        (PVMFMediaLayerNodeExtensionInterface*)
        (iMediaLayerNodeContainer->iExtensions[0]);

    mlExtIntf->setClientPlayBackClock(aClientClock);

    return PVMFSuccess;
}

void PVMFSMFSPPVRBase::setContextParameters(PvmiMIOSession aSession,
        PvmiCapabilityContext& aContext,
        PvmiKvp* aParameters,
        int num_parameter_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_parameter_elements);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

PVMFStatus PVMFSMFSPPVRBase::SetEstimatedServerClock(PVMFMediaClock* aClientClock)
{
    OSCL_UNUSED_ARG(aClientClock);
    return PVMFErrNotSupported;
}

void PVMFSMFSPPVRBase::setJitterBufferDurationInMilliSeconds(uint32 duration)
{
    iJitterBufferDurationInMilliSeconds = duration;
}

void PVMFSMFSPPVRBase::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    ciObserver = aObserver;
}

void PVMFSMFSPPVRBase::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
        int num_elements, PvmiKvp* &aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);

    if (aParameters == NULL || num_elements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFSMFSPPVRBase::setParametersSync() KVP list is NULL or number of elements is 0"));
        return;
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFSMFSPPVRBase::setParametersSync() In"));


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
                            (0, "PVMFSMFSPPVRBase::setParametersSync() Unsupported key"));
            return;
        }

        // Retrieve the second component from the key string
        pv_mime_string_extract_type(1, aParameters[paramind].key, compstr);

        // First check if it is key string for the streaming manager
        if (pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) >= 0)
        {
            if (compcount == 3)
            {
                pv_mime_string_extract_type(2, aParameters[paramind].key, compstr);
                uint i;
                for (i = 0; i < StreamingManagerConfig_NumBaseKeys; i++)
                {
                    if (pv_mime_strcmp(compstr, (char*)(StreamingManagerConfig_BaseKeys[i].iString)) >= 0)
                    {
                        break;
                    }
                }

                if (StreamingManagerConfig_NumBaseKeys == i)
                {
                    // invalid third component
                    aRet_kvp = &aParameters[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFSMFSPPVRBase::setParametersSync() Unsupported key"));
                    return;
                }

                // Verify and set the passed-in setting
                PVMFStatus retval = VerifyAndSetConfigParameter(i, aParameters[paramind], true);
                if (retval != PVMFSuccess)
                {
                    aRet_kvp = &aParameters[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFSMFSPPVRBase::setParametersSync() Setting "
                                     "parameter %d failed", paramind));
                    return;
                }
            }
            else
            {
                // Do not support more than 3 components right now
                aRet_kvp = &aParameters[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFSMFSPPVRBase::setParametersSync() Unsupported key"));
                return;
            }
        }
        else
        {
            // Unknown key string
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFSMFSPPVRBase::setParametersSync() Unsupported key"));
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFSMFSPPVRBase::setParametersSync() Out"));
}

PVMFStatus PVMFSMFSPPVRBase::verifyParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFSMFSPPVRBase::verifyParametersSync() In"));

    if (aParameters == NULL || num_elements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFSMFSPPVRBase::verifyParametersSync() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Go through each parameter and verify
    for (int32 paramind = 0; paramind < num_elements; ++paramind)
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
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::verifyParametersSync() Unsupported key"));
            return PVMFErrArgument;
        }

        // Retrieve the second component from the key string
        pv_mime_string_extract_type(1, aParameters[paramind].key, compstr);

        // First check if it is key string for this node
        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) >= 0) && (compcount == 3))
        {
            pv_mime_string_extract_type(2, aParameters[paramind].key, compstr);
            uint i;
            for (i = 0; i < StreamingManagerConfig_NumBaseKeys; i++)
            {
                if (pv_mime_strcmp(compstr, (char*)(StreamingManagerConfig_BaseKeys[i].iString)) >= 0)
                {
                    break;
                }
            }

            if (StreamingManagerConfig_NumBaseKeys == i)
            {
                return PVMFErrArgument;
            }

            // Verify the passed-in player setting
            PVMFStatus retval = VerifyAndSetConfigParameter(i, aParameters[paramind], false);
            if (retval != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFSMFSPPVRBase::DoCapConfigVerifyParameters() Verifying parameter %d failed", paramind));
                return retval;
            }
        }
        else
        {
            // Unknown key string
            return PVMFErrArgument;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFSMFSPPVRBase::DoCapConfigVerifyParameters() Out"));
    return PVMFSuccess;
}


bool PVMFSMFSPPVRBase::SkipThisNodeResume(int32 aNodeTag)
{
    if (iPVREnabled)
    {
        if ((aNodeTag == PVMF_SM_FSP_SOCKET_NODE)
                || (aNodeTag == PVMF_SM_FSP_RTSP_SESSION_CONTROLLER_NODE))
        {
            // SocketNode and Rtsp Engine were not paused
            return true;
        }
    }

    return false;
}

bool PVMFSMFSPPVRBase::SkipThisNodePause(int32 aNodeTag)
{
    if (iPVREnabled)
    {
        if ((aNodeTag == PVMF_SM_FSP_SOCKET_NODE)
                || (aNodeTag == PVMF_SM_FSP_RTSP_SESSION_CONTROLLER_NODE))
        {
            // SocketNode and Rtsp Engine were not paused
            return true;
        }
    }

    return false;

}

void PVMFSMFSPPVRBase::SetPVRTrackParams(uint32 aMediaTrackId, const PvmfMimeString* aMimeType,
        uint32 aTimescale, uint32 aBitrate)
{
    // If PVR is enabled, pass the track information to the extension interface
    if (iPVREnabled)
    {
        OSCL_ASSERT(iPVRExtInterface != NULL);
        iPVRExtInterface->setTrackParams(aMediaTrackId, aMimeType, aTimescale,
                                         aBitrate);
    }
}

void PVMFSMFSPPVRBase::SetPVRTrackRTPParams(const PvmfMimeString* aMimeType,
        bool   aSeqNumBasePresent, uint32 aSeqNumBase, bool   aRTPTimeBasePresent,
        uint32 aRTPTimeBase, uint32 aNPTInMS)
{
    if (iPVREnabled)
    {
        OSCL_ASSERT(iPVRExtInterface != NULL);
        iPVRExtInterface->setTrackRTPParams(aMimeType,
                                            aSeqNumBasePresent,
                                            aSeqNumBase,
                                            aRTPTimeBasePresent,
                                            aRTPTimeBase,
                                            aNPTInMS);
    }
}

void PVMFSMFSPPVRBase::SetPVRSdpText(OsclRefCounterMemFrag& aSDPText)
{
    if (iPVREnabled)
    {
        // For PVR, make sure we pass the sdp text
        OSCL_ASSERT(iPVRExtInterface != NULL);
        iPVRExtInterface->SetSdpText(aSDPText);
    }
}

//Compute Jitter buffer mem pool size
uint32 PVMFSMFSPPVRBase::GetJitterBufferMemPoolSize(PVMFJitterBufferNodePortTag aJBNodePortTag, PVMFPVRBaseTrackInfo& aRTSPTrackInfo)
{
    uint32 sizeInBytes = 0;
    uint32 byteRate = (aRTSPTrackInfo.bitRate) / 8;
    uint32 overhead = (byteRate * PVMF_JITTER_BUFFER_NODE_MEM_POOL_OVERHEAD) / 100;
    uint32 jitterBufferDuration;

    PVMFSMFSPChildNodeContainer* jitterBufferNodeContainer = getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
    if (jitterBufferNodeContainer == NULL)
    {
        OSCL_ASSERT(false);
        return false;
    }

    PVMFJitterBufferExtensionInterface* jbExtIntf = OSCL_STATIC_CAST(PVMFJitterBufferExtensionInterface*, jitterBufferNodeContainer->iExtensions[0]);
    if (jbExtIntf == NULL)
    {
        OSCL_ASSERT(false);
        return sizeInBytes;
    }

    jbExtIntf->getJitterBufferDurationInMilliSeconds(jitterBufferDuration);
    uint32 durationInSec = jitterBufferDuration / 1000;
    switch (aJBNodePortTag)
    {
        case PVMF_JITTER_BUFFER_PORT_TYPE_INPUT:
        {
            if (durationInSec > 0)
            {
                sizeInBytes = ((byteRate + overhead) * durationInSec);
                if (sizeInBytes < MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES)
                {
                    sizeInBytes = MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES;
                }
                sizeInBytes += (2 * MAX_SOCKET_BUFFER_SIZE);		//MAX_SOCKET_BUFFER_SIZE equals to SNODE_UDP_MULTI_MAX_BYTES_PER_RECV + MAX_UDP_PACKET_SIZE used in socket node
            }

        }
        break;
        case PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK:
        {
            if (durationInSec > 0)
            {
                sizeInBytes = MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES;
                sizeInBytes += (2 * MAX_SOCKET_BUFFER_SIZE);		//MAX_SOCKET_BUFFER_SIZE equals to SNODE_UDP_MULTI_MAX_BYTES_PER_RECV + MAX_UDP_PACKET_SIZE used in socket node
            }
        }
        break;
        default:
            sizeInBytes = 0;
    }
    return sizeInBytes;
}

bool PVMFSMFSPPVRBase::SetPVRPlaybackRange()
{
    // Send the repositioning point to PVR Extension Interface
    OSCL_ASSERT(iPVRExtInterface != NULL);
    if (iPVRExtInterface->SetDataSourcePosition(iRepositionRequestedStartNPTInMS) != PVMFSuccess)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "StreamingManagerNode:DoRepositioningStart - SetRequestPlayRange Failed for PVR"));
        return false;
    }

    return true;
}


PVMFStatus PVMFSMFSPPVRBase::GetConfigParameter(PvmiKvp*& aParameters,
        int& aNumParamElements,
        int32 aIndex, PvmiKvpAttr reqattr)
{
    PVMF_SM_FSP_PVR_BASE_LOGINFO((0, "PVMFSMFSPPVRBase::GetConfigParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));

    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(SMCONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() Memory allocation for key string failed"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, SMCONFIG_KEYSTRING_SIZE*sizeof(char));

    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/net/"), 17);
    oscl_strncat(aParameters[0].key, StreamingManagerConfig_BaseKeys[aIndex].iString,
                 oscl_strlen(StreamingManagerConfig_BaseKeys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype="), 20);
    switch (StreamingManagerConfig_BaseKeys[aIndex].iValueType)
    {
        case PVMI_KVPVALTYPE_RANGE_INT32:
            oscl_strncat(aParameters[0].key,
                         _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_INT32_STRING),
                         oscl_strlen(PVMI_KVPVALTYPE_RANGE_INT32_STRING));
            break;

        case PVMI_KVPVALTYPE_KSV:
            oscl_strncat(aParameters[0].key,
                         _STRLIT_CHAR(PVMI_KVPVALTYPE_KSV_STRING),
                         oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
            break;

        case PVMI_KVPVALTYPE_CHARPTR:
            oscl_strncat(aParameters[0].key,
                         _STRLIT_CHAR(PVMI_KVPVALTYPE_CHARPTR_STRING),
                         oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING));
            break;

        case PVMI_KVPVALTYPE_WCHARPTR:
            oscl_strncat(aParameters[0].key,
                         _STRLIT_CHAR(PVMI_KVPVALTYPE_WCHARPTR_STRING),
                         oscl_strlen(PVMI_KVPVALTYPE_WCHARPTR_STRING));
            break;

        case PVMI_KVPVALTYPE_BOOL:
            oscl_strncat(aParameters[0].key,
                         _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING),
                         oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
            break;

        case PVMI_KVPVALTYPE_UINT32:
        default:
            if (reqattr == PVMI_KVPATTR_CAP)
            {
                oscl_strncat(aParameters[0].key,
                             _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING),
                             oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            else
            {
                oscl_strncat(aParameters[0].key,
                             _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING),
                             oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
            }
            break;
    }
    aParameters[0].key[SMCONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case BASEKEY_DELAY:
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
                OSCL_ASSERT(iJitterBufferNodeContainer);
                if (!iJitterBufferNodeContainer)
                    return PVMFFailure;
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                OSCL_ASSERT(jbExtIntf);
                if (!jbExtIntf)
                    return PVMFFailure;
                if (jbExtIntf)
                    jbExtIntf->getJitterBufferDurationInMilliSeconds(aParameters[0].value.uint32_value);
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = DEFAULT_JITTER_BUFFER_DURATION_IN_MS;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                 "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = MIN_JITTER_BUFFER_DURATION_IN_MS;
                rui32->max = MAX_JITTER_BUFFER_DURATION_IN_MS;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;
        case BASEKEY_JITTERBUFFER_NUMRESIZE:
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                uint32 numResize, resizeSize;
                PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
                OSCL_ASSERT(iJitterBufferNodeContainer);
                if (!iJitterBufferNodeContainer)
                    return PVMFFailure;
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                OSCL_ASSERT(jbExtIntf);
                if (!jbExtIntf)
                    return PVMFFailure;
                jbExtIntf->GetSharedBufferResizeParams(numResize, resizeSize);
                aParameters[0].value.uint32_value = numResize;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = DEFAULT_MAX_NUM_SOCKETMEMPOOL_RESIZES;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                 "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = MIN_NUM_SOCKETMEMPOOL_RESIZES;
                rui32->max = MAX_NUM_SOCKETMEMPOOL_RESIZES;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;
        case BASEKEY_JITTERBUFFER_RESIZESIZE:
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                uint32 numResize, resizeSize;
                PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
                OSCL_ASSERT(iJitterBufferNodeContainer);
                if (!iJitterBufferNodeContainer)
                    return PVMFFailure;
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                OSCL_ASSERT(jbExtIntf);
                if (!jbExtIntf)
                    return PVMFFailure;
                jbExtIntf->GetSharedBufferResizeParams(numResize, resizeSize);
                aParameters[0].value.uint32_value = resizeSize;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = DEFAULT_MAX_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                 "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = MIN_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;
                rui32->max = MAX_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;
        case BASEKEY_JITTERBUFFER_MAX_INACTIVITY_DURATION:
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
                OSCL_ASSERT(iJitterBufferNodeContainer);
                if (!iJitterBufferNodeContainer)
                    return PVMFFailure;
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                OSCL_ASSERT(jbExtIntf);
                if (!jbExtIntf)
                    return PVMFFailure;
                uint32 inactivityDuration = 0;
                jbExtIntf->getMaxInactivityDurationForMediaInMs(inactivityDuration);
                aParameters[0].value.uint32_value = inactivityDuration;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = DEFAULT_MAX_INACTIVITY_DURATION_IN_MS;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                 "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = 0;
                rui32->max = DEFAULT_MAX_INACTIVITY_DURATION_IN_MS;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;
        case BASEKEY_SESSION_CONTROLLER_USER_AGENT:
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                aParameters[0].value.pWChar_value = NULL;
                /* As of now just RTSP node supports an external config of user agent */
                PVMFSMFSPChildNodeContainer* iSessionControllerNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                        (PVRTSPEngineNodeExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);

                    OSCL_wHeapString<OsclMemAllocator> userAgent;
                    if (rtspExtIntf->GetUserAgent(userAgent) == PVMFSuccess)
                    {
                        // Return current value
                        oscl_wchar* ptr = (oscl_wchar*)oscl_malloc(sizeof(oscl_wchar) * (userAgent.get_size()));
                        if (ptr)
                        {
                            oscl_memcpy(ptr, userAgent.get_cstr(), userAgent.get_size());
                            aParameters[0].value.pWChar_value = ptr;
                        }
                        else
                        {
                            oscl_free(aParameters[0].key);
                            oscl_free(aParameters);
                            PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                         "Memory allocation for user agent failed"));
                            return PVMFErrNoMemory;
                        }
                    }
                }
            }
            else
            {
                // Return capability - no concept of capability for user agent
                // do nothing
            }
            break;
        case BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_INTERVAL:
        case BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_DURING_PLAY:
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                if (aIndex == BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_INTERVAL)
                {
                    aParameters[0].value.uint32_value = PVRTSPENGINENODE_DEFAULT_KEEP_ALIVE_INTERVAL;
                }
                else
                {
                    aParameters[0].value.bool_value = false;
                }
                /* As of now just RTSP node supports an external config of user agent */
                PVMFSMFSPChildNodeContainer* iSessionControllerNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                        (PVRTSPEngineNodeExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);
                    uint32 timeout;
                    bool okeepalivemethod;
                    bool okeepaliveinplay;
                    rtspExtIntf->GetKeepAliveMethod((int32&)timeout, okeepalivemethod, okeepaliveinplay);
                    if (aIndex == BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_INTERVAL)
                    {
                        aParameters[0].value.uint32_value = timeout;
                    }
                    else
                    {
                        aParameters[0].value.bool_value = okeepaliveinplay;
                    }
                }
            }
            else
            {
                // Return capability - no concept of capability for keep alive interval
                // do nothing
            }
            break;
        case BASEKEY_REBUFFERING_THRESHOLD:
        {
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                PVMFSMFSPChildNodeContainer* iJitterBufferNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_JITTER_BUFFER_NODE);
                OSCL_ASSERT(iJitterBufferNodeContainer);
                if (!iJitterBufferNodeContainer)
                    return PVMFFailure;
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                OSCL_ASSERT(jbExtIntf);
                if (!jbExtIntf)
                    return PVMFFailure;
                jbExtIntf->getJitterBufferRebufferingThresholdInMilliSeconds(aParameters[0].value.uint32_value);
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                 "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
                rui32->max = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
        }
        break;
        case BASEKEY_SESSION_CONTROLLER_RTSP_TIMEOUT:
        {
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                /* As of now just RTSP node supports an external config of RTSP time out */
                PVMFSMFSPChildNodeContainer* iSessionControllerNodeContainer =
                    getChildNodeContainer(PVMF_SM_FSP_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                        (PVRTSPEngineNodeExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);
                    int32 timeout;
                    rtspExtIntf->GetRTSPTimeOut(timeout);
                    aParameters[0].value.uint32_value = OSCL_STATIC_CAST(uint32, timeout);
                }
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_FSP_PVR_BASE_LOGERR((0, "PVMFSMFSPPVRBase::GetConfigParameter() "
                                                 "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = MIN_RTSP_SERVER_INACTIVITY_TIMEOUT_IN_SEC;
                rui32->max = MAX_RTSP_SERVER_INACTIVITY_TIMEOUT_IN_SEC;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
        }
        break;
        case BASEKEY_DISABLE_FIREWALL_PACKETS:
        {
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                aParameters[0].value.bool_value = false;
            }
        }
        break;

        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Invalid index to player parameter"));
            return PVMFErrArgument;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlayerParameter() Out"));
    return PVMFSuccess;
}

void PVMFSMFSPPVRBase::DoQueryInterface(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoQueryInterface - In"));

    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.PVMFSMFSPBaseNodeCommandBase::Parse(uuid, ptr);

    *ptr = NULL;

    if (*uuid == PVMF_TRACK_SELECTION_INTERFACE_UUID)
    {
        PVMFTrackSelectionExtensionInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFTrackSelectionExtensionInterface*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else if (*uuid == PVMF_DATA_SOURCE_INIT_INTERFACE_UUID)
    {
        PVMFDataSourceInitializationExtensionInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFDataSourceInitializationExtensionInterface*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else if (*uuid == PvmfDataSourcePlaybackControlUuid)
    {
        PvmfDataSourcePlaybackControlInterface* interimPtr =
            OSCL_STATIC_CAST(PvmfDataSourcePlaybackControlInterface*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else if (*uuid == KPVMFMetadataExtensionUuid)
    {
        PVMFMetadataExtensionInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFMetadataExtensionInterface*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else if (*uuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* interimPtr =
            OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else if (*uuid == PVMFCPMPluginLicenseInterfaceUuid)
    {
        PVMFCPMPluginLicenseInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFCPMPluginLicenseInterface*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else if (*uuid == PVMF_DATA_SOURCE_PACKETSOURCE_INTERFACE_UUID)
    {
        PVMFDataSourcePacketSourceInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFDataSourcePacketSourceInterface*, this);
        *ptr = OSCL_STATIC_CAST(PVInterface*, interimPtr);
    }
    else
    {
        *ptr = NULL;
    }
    if (*ptr)
    {
        PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::QueryInterface() - CmdComplete - PVMFSuccess"));
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else
    {
        PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ((0, "PVMFSMFSPPVRBase::QueryInterface() - CmdFailed - PVMFErrNotSupported"));
        CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
    }

    PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE((0, "PVMFSMFSPPVRBase::DoQueryInterface - Out"));
    return;
}


PVMFStatus PVMFSMFSPPVRBase::GetPVRPluginSpecificValues(PVMFSMFSPBaseNodeCommand& aCmd)
{
    PVMFMetadataList* keylistptr = NULL;
    Oscl_Vector<PvmiKvp, OsclMemAllocator>* valuelistptr = NULL;
    uint32 starting_index;
    int32 max_entries;

    aCmd.PVMFSMFSPBaseNodeCommand::Parse(keylistptr, valuelistptr, starting_index, max_entries);

    // Check the parameters
    if (keylistptr == NULL || valuelistptr == NULL)
    {
        return PVMFErrArgument;
    }

    uint32 numkeys = keylistptr->size();

    if (numkeys <= 0 || max_entries == 0)
    {
        // Don't do anything
        return PVMFErrArgument;
    }

    uint32 numvalentries = 0;
    int32 numentriesadded = 0;

    for (uint32 lcv = 0; lcv < numkeys; lcv++)
    {
        PvmiKvp KeyVal;
        KeyVal.key = NULL;
        KeyVal.value.pWChar_value = NULL;
        KeyVal.value.pChar_value = NULL;

        if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_PAUSE_DENIED_KEY) != NULL)
        {
            // Increment the counter for the number of values found so far
            ++numvalentries;
            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                // check if duration is available first
                bool keyValue = iMetaDataInfo->iSessionDurationAvailable ? false : true;
                if (keyValue == true)
                    keyValue = !iPVREnabled;
                PVMFStatus retval = PVMFCreateKVPUtils::CreateKVPForBoolValue(KeyVal,
                                    PVMFSTREAMINGMGRNODE_PAUSE_DENIED_KEY,
                                    keyValue);
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }

        /* Check if the max number of value entries were added */
        if (max_entries > 0 && numentriesadded >= max_entries)
        {
            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
            return PVMFSuccess;
        }

        // Add the KVP to the list if the key string was created
        if (KeyVal.key != NULL)
        {
            if (PVMFSuccess != PushKVPToMetadataValueList(valuelistptr, KeyVal))
            {
                switch (GetValTypeFromKeyString(KeyVal.key))
                {
                    case PVMI_KVPVALTYPE_CHARPTR:
                        if (KeyVal.value.pChar_value != NULL)
                        {
                            OSCL_ARRAY_DELETE(KeyVal.value.pChar_value);
                            KeyVal.value.pChar_value = NULL;
                        }
                        break;

                    default:
                        // Add more case statements if other value types are returned
                        break;
                }

                OSCL_ARRAY_DELETE(KeyVal.key);
                KeyVal.key = NULL;
            }
            else
            {
                // Increment the counter for number of value entries added to the list
                ++numentriesadded;
            }

            // Check if the max number of value entries were added
            if (max_entries > 0 && numentriesadded >= max_entries)
            {
                // Maximum number of values added so break out of the loop
                break;
            }
        }
    }

    iNoOfValuesIteratedForValueVect = numvalentries;
    iNoOfValuesPushedInValueVect = numentriesadded;

    iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
    return PVMFSuccess;
}

