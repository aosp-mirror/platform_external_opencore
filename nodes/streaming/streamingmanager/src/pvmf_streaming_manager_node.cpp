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
#ifndef OSCL_SNPRINTF_H_INCLUDED
#include "oscl_snprintf.h"
#endif
#ifndef OSCL_EXCLUSIVE_PTR_H_INCLUDED
#include "oscl_exclusive_ptr.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif
#ifndef PVMF_SM_NODE_FACTORY_H_INCLUDED
#include "pvmf_sm_node_factory.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif
#ifndef PVMF_STREAMING_MANAGER_NODE_H_INCLUDED
#include "pvmf_streaming_manager_node.h"
#endif
#ifndef PVMF_STREAMING_MANAGER_INTERNAL_H_INCLUDED
#include "pvmf_streaming_manager_internal.h"
#endif
#ifndef PVMF_SM_CAPCONFIG_H_INCLUDED
#include "pvmf_sm_capconfig.h"
#endif
#ifndef PVMF_SOCKET_NODE_H_INCLUDED
#include "pvmf_socket_node.h"
#endif
#ifndef PVMF_RTSP_ENGINE_NODE_FACTORY_H_INCLUDED
#include "pvrtsp_client_engine_factory.h"
#endif
#ifndef PVMF_JITTER_BUFFER_NODE_H_INCLUDED
#include "pvmf_jitter_buffer_node.h"
#endif
#ifndef PVMF_MEDIALAYER_NODE_H_INCLUDED
#include "pvmf_medialayer_node.h"
#endif
#ifndef PVRTSP_ENGINE_NODE_EXTENSION_INTERFACE_H_INCLUDED
#include "pvrtspenginenodeextensioninterface.h"
#endif
#ifndef PVMF_MEDIA_PRESENTATION_INFO_H_INCLUDED
#include "pvmf_media_presentation_info.h"
#endif
#ifndef SDP_PARSER_H
#include "sdp_parser.h"
#endif
#ifndef PVMF_BASIC_ERRORINFOMESSAGE_H_INCLUDED
#include "pvmf_basic_errorinfomessage.h"
#endif
#ifndef PVMF_ERRORINFOMESSAGE_EXTENSION_H_INCLUDED
#include "pvmf_errorinfomessage_extension.h"
#endif
#ifndef OSCL_STRING_UTILS_H_INCLUDED
#include "oscl_string_utils.h"
#endif
#ifndef PVMI_KVP_UTIL_H_INCLUDED
#include "pvmi_kvp_util.h"
#endif
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif
#ifndef PVMF_PROTOCOLENGINE_FACTORY_H_INCLUDED
#include "pvmf_protocol_engine_factory.h"
#endif
#ifndef PVMF_PROTOCOLENGINE_DEFS_H_INCLUDED
#include "pvmf_protocol_engine_defs.h"
#endif
#ifndef PVMFPROTOCOLENGINENODE_EXTENSION_H_INCLUDED
#include "pvmf_protocol_engine_node_extension.h"
#endif
#ifndef PVMF_STREAMING_ASF_INTERFACES_INCLUDED
#include "pvmf_streaming_asf_interfaces.h"
#endif
#ifndef PVMI_DRM_KVP_H_INCLUDED
#include "pvmi_drm_kvp.h"
#endif

#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()


// Number of metadata keys supported in this node.
#define PVMFSTREAMINGMGRNODE_NUM_METADATAKEYS 16
// Constant character strings for metadata keys
static const char PVMFSTREAMINGMGRNODE_AUTHOR_KEY[] = "author";
static const char PVMFSTREAMINGMGRNODE_ARTIST_KEY[] = "artist";
static const char PVMFSTREAMINGMGRNODE_TITLE_KEY[] = "title";
static const char PVMFSTREAMINGMGRNODE_DESCRIPTION_KEY[] = "description";
static const char PVMFSTREAMINGMGRNODE_RATING_KEY[] = "rating";
static const char PVMFSTREAMINGMGRNODE_COPYRIGHT_KEY[] = "copyright";
static const char PVMFSTREAMINGMGRNODE_GENRE_KEY[] = "genre";
static const char PVMFSTREAMINGMGRNODE_LYRICS_KEY[] = "lyrics";
static const char PVMFSTREAMINGMGRNODE_CLASSIFICATION_KEY[] = "classification";
static const char PVMFSTREAMINGMGRNODE_KEYWORDS_KEY[] = "keywords";
static const char PVMFSTREAMINGMGRNODE_LOCATION_KEY[] = "location";
static const char PVMFSTREAMINGMGRNODE_DURATION_KEY[] = "duration";
static const char PVMFSTREAMINGMGRNODE_NUMTRACKS_KEY[] = "num-tracks";
static const char PVMFSTREAMINGMGRNODE_RANDOM_ACCESS_DENIED_KEY[] = "random-access-denied";
static const char PVMFSTREAMINGMGRNODE_NUM_GRAPHICS_KEY[] = "graphic/num-frames;format=APIC";
static const char PVMFSTREAMINGMGRNODE_GRAPHICS_KEY[] = "graphic;format=APIC";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_TYPE_KEY[] = "track-info/type";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_DURATION_KEY[] = "track-info/duration";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_BITRATE_KEY[] = "track-info/bit-rate";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_MAX_BITRATE_KEY[] = "track-info/max-bitrate";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_SELECTED_KEY[] = "track-info/selected";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_WIDTH_KEY[] = "track-info/video/width";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_HEIGHT_KEY[] = "track-info/video/height";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_SAMPLERATE_KEY[] = "track-info/sample-rate";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_NUMCHANNELS_KEY[] = "track-info/audio/channels";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_AUDIO_BITS_PER_SAMPLE_KEY[] = "track-info/audio/bits-per-sample";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_TRACKID_KEY[] = "track-info/track-id";

static const char PVMFSTREAMINGMGRNODE_SEMICOLON[] = ";";
static const char PVMFSTREAMINGMGRNODE_TIMESCALE[] = "timescale=";
static const char PVMFSTREAMINGMGRNODE_INDEX[] = "index=";
static const char PVMFSTREAMINGMGRNODE_CLIP_TYPE_KEY[] = "clip-type";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_FRAME_RATE_KEY[] = "track-info/frame-rate";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_NAME_KEY[] = "track-info/codec-name";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DESCRIPTION_KEY[] = "track-info/codec-description";
static const char PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DATA_KEY[] = "track-info/codec-specific-info";

static const char PVMFSTREAMINGMGRNODE_MAXSIZE[] = "maxsize=";
static const char PVMFSTREAMINGMGRNODE_REQSIZE[] = "reqsize=";
static const char PVMFSTREAMINGMGRNODE_TRUNCATE_FLAG[] = "truncate=";

///////////////////////////////////////////////////////////////////////////////
//
// Capability and config interface related constants and definitions
//   - based on pv_player_engine.h
//
///////////////////////////////////////////////////////////////////////////////

static const StreamingManagerKeyStringData StreamingManagerConfig_BaseKeys[] =
{
    {"delay", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"jitterBufferNumResize", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"jitterBufferResizeSize", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"user-agent", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_WCHARPTR},
    {"keep-alive-interval", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"keep-alive-during-play", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL},
    {"switch-streams", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_CHARPTR},
    {"speed", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"http-version", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_CHARPTR},
    {"num-redirect-attempts", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"protocol-extension-header", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_CHARPTR},
    {"http-timeout", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"http-streaming-logging-timeout", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"x-str-header", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL},
    {"max-streaming-asf-header-size", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"max-tcp-recv-buffer-size", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"rebuffering-threshold", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"accel-bitrate", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"accel-duration", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
    {"disable-firewall-packets", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_BOOL}
};

static const uint StreamingManagerConfig_NumBaseKeys =
    (sizeof(StreamingManagerConfig_BaseKeys) /
     sizeof(StreamingManagerKeyStringData));

enum BaseKeys_IndexMapType
{
    BASEKEY_DELAY = 0,
    BASEKEY_JITTERBUFFER_NUMRESIZE,
    BASEKEY_JITTERBUFFER_RESIZESIZE,
    BASEKEY_SESSION_CONTROLLER_USER_AGENT,
    BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_INTERVAL,
    BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_DURING_PLAY,
    BASEKEY_STREAMING_MGR_SWITCH_STREAMS,
    BASEKEY_STREAMING_SPEED,
    BASEKEY_HTTP_VERSION,
    BASEKEY_NUM_REDIRECT_ATTEMPTS,
    BASEKEY_PROTOCOL_EXTENSION_HEADER,
    BASEKEY_SESSION_CONTROLLER_HTTP_TIMEOUT,
    BASEKEY_SESSION_CONTROLLER_HTTP_STREAMING_LOGGING_TIMEOUT,
    BASEKEY_SESSION_CONTROLLER_XSTR_HTTP_HEADER,
    BASEKEY_MAX_STREAMING_ASF_HEADER_SIZE,
    BASEKEY_MAX_TCP_RECV_BUFFER_SIZE,
    BASEKEY_REBUFFERING_THRESHOLD,
    BASEKEY_ACCEL_BITRATE,
    BASEKEY_ACCEL_DURATION,
    BASEKEY_DISABLE_FIREWALL_PACKETS
};

///////////////////////////////////////////////////////////////////////////////

void
PVMFStreamingManagerNodeCommand::Copy(const PVMFGenericNodeCommand<OsclMemAllocator>& aCmd)
{
    PVMFGenericNodeCommand<OsclMemAllocator>::Copy(aCmd);
    switch (aCmd.iCmd)
    {
        case PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAKEYS:
            if (aCmd.iParam4)
            {
                /* copy the allocated string */
                OSCL_HeapString<OsclMemAllocator>* aStr =
                    (OSCL_HeapString<OsclMemAllocator>*)aCmd.iParam4;
                Oscl_TAlloc<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> str;
                iParam4 = str.ALLOC_AND_CONSTRUCT(*aStr);
            }
            break;
        default:
            break;
    }
}

/* need to overlaod the base Destroy routine to cleanup metadata key */
void PVMFStreamingManagerNodeCommand::Destroy()
{
    PVMFGenericNodeCommand<OsclMemAllocator>::Destroy();
    switch (iCmd)
    {
        case PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAKEYS:
            if (iParam4)
            {
                /* cleanup the allocated string */
                Oscl_TAlloc<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> str;
                str.destruct_and_dealloc(iParam4);
            }
            break;
        default:
            break;
    }
}

/**
//////////////////////////////////////////////////
// Node Constructor & Destructor
//////////////////////////////////////////////////
*/

OSCL_EXPORT_REF PVMFStreamingManagerNode::PVMFStreamingManagerNode(int32 aPriority)
        : OsclActiveObject(aPriority, "StreamingManagerNode")
{
    iLogger = NULL;
    iCmdSeqLogger = NULL;
    iReposLogger = NULL;

    iLogger = PVLogger::GetLoggerObject("StreamingManagerNode");

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::PVMFStreamingManagerNode - In"));

    iExtensionInterface = NULL;
    iExtensionInterfacePlaceholder = NULL;
    iQueryUUIDComplete = false;
    iQueryInterfaceComplete = false;
    iTotalNumRequestPortsComplete = 0;
    iNumRequestPortsPending = 0;
    iTotalNumInitPhaseRequestPortsComplete = 0;
    iTotalNumInitPhaseRequestPortsPending = 0;
    oGraphConstructComplete = false;
    oGraphConnectComplete = false;
    iSessionStartTime = 0;
    iSessionStopTime = 0;
    iSessionStopTimeAvailable = true;
    iSessionSeekAvailable = true;
    oRepositioning = false;
    oPlayListRepositioning = false;
    iRepositionRequestedStartNPTInMS = 0;
    iActualRepositionStartNPTInMS = 0;
    iActualMediaDataTS = 0;
    iActualRepositionStartNPTInMSPtr = NULL;
    iActualMediaDataTSPtr = NULL;
    iPVMFDataSourcePositionParamsPtr = NULL;
    iJumpToIFrame = false;
    iJitterBufferDurationInMilliSeconds = DEFAULT_JITTER_BUFFER_DURATION_IN_MS;
    iAutoPausePending = false;
    iAutoResumePending = 0;
    iAutoPaused = false;
    iStreamThinningInProgress = false;
    iPlaylistPlayInProgress = false;
    iSwitchStreamIFrameVideo = false;
    iErrorDuringProcess = SM_NO_ERROR;
    iErrorResponseInf = NULL;
    iCmdErrStatus = PVMFFailure;
    iEventData = NULL;

    ibRdtTransport = false;
    ibCloaking = false;

    ipRealChallengeGen = NULL;
    ipRdtParser = NULL;

    iStreamID = 0;

    iSourceContextDataValid    = false;
    iUseCPMPluginRegistry      = false;
    iPreviewMode               = false;
    iDRMResetPending		   = false;
    iCPMInitPending            = false;
    maxPacketSize			   = 0;
    iPVMFStreamingManagerNodeMetadataValueCount = 0;
    iCPMMetaDataExtensionInterface = NULL;
    iCPMLicenseInterface           = NULL;
    iCPMGetMetaDataKeysCmdId       = 0;
    iCPMGetMetaDataValuesCmdId     = 0;
    iCPMGetLicenseInterfaceCmdId   = 0;
    iCPMGetLicenseCmdId			   = 0;
    iCPMCancelGetLicenseCmdId	   = 0;
    iCPM                       = NULL;
    iCPMSessionID              = 0xFFFFFFFF;
    iCPMContentType            = PVMF_CPM_CONTENT_FORMAT_UNKNOWN;
    iCPMContentAccessFactory   = NULL;
    iDecryptionInterface       = NULL;
    iCPMInitCmdId              = 0;
    iCPMOpenSessionCmdId       = 0;
    iCPMRegisterContentCmdId   = 0;
    iCPMRequestUsageId         = 0;
    iCPMUsageCompleteCmdId     = 0;
    iCPMCloseSessionCmdId      = 0;
    iCPMResetCmdId             = 0;
    iRequestedUsage.key        = NULL;
    iApprovedUsage.key         = NULL;
    iAuthorizationDataKvp.key  = NULL;
    CleanupCPMdata();

    int32 err;
    OSCL_TRY(err,
             /*
              * Create the input command queue.  Use a reserve to avoid lots of
              * dynamic memory allocation.
              */
             iInputCommands.Construct(PVMF_STREAMING_MANAGER_NODE_COMMAND_ID_START,
                                      PVMF_STREAMING_MANAGER_VECTOR_RESERVE);

             /*
              * Create the "current command" queue.  It will only contain one
              * command at a time, so use a reserve of 1.
              */
             iCurrentCommand.Construct(0, 1);

             /*
              * Create the "cancel command" queue.  It will only contain one
              * command at a time, so use a reserve of 1.
              */
             iCancelCommand.Construct(0, 1);

             /*
              * Set the node capability data.
              * This node can support an unlimited number of ports.
              */
             iCapability.iCanSupportMultipleInputPorts = false;
             iCapability.iCanSupportMultipleOutputPorts = true;
             iCapability.iHasMaxNumberOfPorts = false;
             iCapability.iMaxNumberOfPorts = 0;//no maximum

             PVMF_STREAMING_MANAGER_NEW(NULL,
                                        PVMFSMSessionSourceInfo,
                                        (),
                                        iSessionSourceInfo);

             /*
              * Create Socket Node
              */
             OsclExclusivePtr<PVMFNodeInterface> socketNodeAutoPtr;
             PVMFNodeInterface* iSocketNode;
             PVMF_STREAMING_MANAGER_NEW(NULL,
                                        PVMFSocketNode,
                                        (OsclActiveObject::EPriorityNominal),
                                        iSocketNode);
             socketNodeAutoPtr.set(iSocketNode);

             PVMFSMNodeContainer sSocketNodeContainer;

             PVMFNodeSessionInfo socketNodeSession(this,
                                                   this,
                                                   OSCL_REINTERPRET_CAST(OsclAny*,
                                                                         iSocketNode),
                                                   this,
                                                   OSCL_REINTERPRET_CAST(OsclAny*,
                                                                         iSocketNode));

             sSocketNodeContainer.iNode = iSocketNode;
             sSocketNodeContainer.iSessionId =
                 iSocketNode->Connect(socketNodeSession);
             sSocketNodeContainer.iNodeTag =
                 PVMF_STREAMING_MANAGER_SOCKET_NODE;
             sSocketNodeContainer.commandStartOffset =
                 PVMF_STREAMING_MANAGER_SOCKET_NODE_COMMAND_START;
             /* Push back the known UUID in case there are no queries */
             PVUuid uuid(PVMF_SOCKET_NODE_EXTENSION_INTERFACE_UUID);
             sSocketNodeContainer.iExtensionUuids.push_back(uuid);
             iNodeContainerVec.push_back(sSocketNodeContainer);

             /*
              * Create Session Controller Node
              */
             OsclExclusivePtr<PVMFNodeInterface> sessionControllerAutoPtr;
             PVMFNodeInterface* iSessionControllerNode = PVMFRrtspEngineNodeFactory::CreatePVMFRtspEngineNode(OsclActiveObject::EPriorityNominal);
             sessionControllerAutoPtr.set(iSessionControllerNode);

             PVMFSMNodeContainer sSessionControllerNodeContainer;

             PVMFNodeSessionInfo sessionControllerSession(this,
                     this,
                     OSCL_REINTERPRET_CAST(OsclAny*,
                                           iSessionControllerNode),
                     this,
                     OSCL_REINTERPRET_CAST(OsclAny*,
                                           iSessionControllerNode));

             sSessionControllerNodeContainer.iNode = iSessionControllerNode;
             sSessionControllerNodeContainer.iSessionId =
                 iSessionControllerNode->Connect(sessionControllerSession);
             sSessionControllerNodeContainer.iNodeTag =
                 PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE;
             sSessionControllerNodeContainer.commandStartOffset =
                 PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_COMMAND_START;
             /* Push back the known UUID in case there are no queries */
             sSessionControllerNodeContainer.iExtensionUuids.push_back(KPVRTSPEngineNodeExtensionUuid);
             iNodeContainerVec.push_back(sSessionControllerNodeContainer);

             /*
              * Create jitter buffer node
              */
             OsclExclusivePtr<PVMFNodeInterface> jitterBufferNodeAutoPtr;
             PVMFNodeInterface* iJitterBufferNode;
             PVMF_STREAMING_MANAGER_NEW(NULL,
                                        PVMFJitterBufferNode,
                                        (OsclActiveObject::EPriorityNominal),
                                        iJitterBufferNode);
             jitterBufferNodeAutoPtr.set(iJitterBufferNode);

             PVMFSMNodeContainer sJitterBufferNodeContainer;

             PVMFNodeSessionInfo jitterBufferSession(this,
                                                     this,
                                                     OSCL_REINTERPRET_CAST(OsclAny*,
                                                                           iJitterBufferNode),
                                                     this,
                                                     OSCL_REINTERPRET_CAST(OsclAny*,
                                                                           iJitterBufferNode));

             sJitterBufferNodeContainer.iNode = iJitterBufferNode;
             sJitterBufferNodeContainer.iSessionId =
                 iJitterBufferNode->Connect(jitterBufferSession);
             sJitterBufferNodeContainer.iNodeTag =
                 PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE;
             sJitterBufferNodeContainer.commandStartOffset =
                 PVMF_STREAMING_MANAGER_JITTER_BUFFER_CONTROLLER_COMMAND_START;
             /* Push back the known UUID in case there are no queries */
             sJitterBufferNodeContainer.iExtensionUuids.push_back(PVMF_JITTERBUFFERNODE_EXTENSIONINTERFACE_UUID);
             iNodeContainerVec.push_back(sJitterBufferNodeContainer);

             /*
              * Create media layer node
              */
             OsclExclusivePtr<PVMFNodeInterface> mediaLayerNodeAutoPtr;
             PVMFNodeInterface* iMediaLayerNode;
             PVMF_STREAMING_MANAGER_NEW(NULL,
                                        PVMFMediaLayerNode,
                                        (OsclActiveObject::EPriorityNominal),
                                        iMediaLayerNode);
             mediaLayerNodeAutoPtr.set(iMediaLayerNode);

             PVMFSMNodeContainer sMediaLayerNodeContainer;

             PVMFNodeSessionInfo mediaLayerSession(this,
                                                   this,
                                                   OSCL_REINTERPRET_CAST(OsclAny*,
                                                                         iMediaLayerNode),
                                                   this,
                                                   OSCL_REINTERPRET_CAST(OsclAny*,
                                                                         iMediaLayerNode));

             sMediaLayerNodeContainer.iNode = iMediaLayerNode;
             sMediaLayerNodeContainer.iSessionId =
                 iMediaLayerNode->Connect(mediaLayerSession);
             sMediaLayerNodeContainer.iNodeTag =
                 PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE;
             sMediaLayerNodeContainer.commandStartOffset =
                 PVMF_STREAMING_MANAGER_MEDIA_LAYER_COMMAND_START;
             /* Push back the known UUID in case there are no queries */
             sMediaLayerNodeContainer.iExtensionUuids.push_back(PVMF_MEDIALAYERNODE_EXTENSIONINTERFACE_UUID);
             iNodeContainerVec.push_back(sMediaLayerNodeContainer);

             for (int32 i = 0; i < PVMF_STREAMING_MANAGER_INTERNAL_CMDQ_SIZE; i++)
{
    iInternalCmdPool[i].cmd = PVMF_STREAMING_MANAGER_INTERNAL_COMMAND_NONE;
        iInternalCmdPool[i].oFree = true;
    }

    sessionControllerAutoPtr.release();
    socketNodeAutoPtr.release();
    jitterBufferNodeAutoPtr.release();
    mediaLayerNodeAutoPtr.release();

    // Allocate memory for metadata key list
    iAvailableMetadataKeys.reserve(PVMFSTREAMINGMGRNODE_NUM_METADATAKEYS);
    iAvailableMetadataKeys.clear();
            );
    if (err != OsclErrNone)
    {
        CleanUp();
        OSCL_LEAVE(err);
    }

    if (err != OsclErrNone)
    {
        CleanUp();
        OSCL_LEAVE(err);
    }

    if (err != OsclErrNone)
    {
        CleanUp();
        OSCL_LEAVE(err);
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::PVMFStreamingManagerNode - Out"));
}

void PVMFStreamingManagerNode::CleanUp()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CleanUp - In"));

    /*
     * if a leave happened, cleanup and re-throw the error
     */
    iInputCommands.clear();
    iCurrentCommand.clear();

    PVMF_STREAMING_MANAGER_DELETE(NULL,
                                  PVMFSMSessionSourceInfo,
                                  iSessionSourceInfo);
    /*
     * Clean up all children nodes
     */
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_SOCKET_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFSocketNode,
                                          ((PVMFSocketNode*)(iNodeContainerVec[i].iNode)));
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE)
        {
            PVMFRrtspEngineNodeFactory::DeletePVMFRtspEngineNode(iNodeContainerVec[i].iNode);
            iNodeContainerVec[i].iNode = NULL;
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFJitterBufferNode,
                                          ((PVMFJitterBufferNode*)(iNodeContainerVec[i].iNode)));
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFMediaLayerNode,
                                          ((PVMFMediaLayerNode*)(iNodeContainerVec[i].iNode)));
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_RTPPACKETSOURCE_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFMediaLayerNode,
                                          ((PVMFMediaLayerNode*)(iNodeContainerVec[i].iNode)));
        }

    }
    iNodeContainerVec.clear();
    iCapability.iInputFormatCapability.clear();
    iCapability.iOutputFormatCapability.clear();
    OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
    OSCL_CLEANUP_BASE_CLASS(OsclActiveObject);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CleanUp - Out"));
    return;
}

OSCL_EXPORT_REF PVMFStreamingManagerNode::~PVMFStreamingManagerNode()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::~PVMFStreamingManagerNode - In"));

    Cancel();

    /* Reset the metadata key list */
    /* Clean up CPM related variables */
    CleanupCPMdata();
    /*Cleanup CPM instance*/
    if (iCPM != NULL)
    {
        iCPM->ThreadLogoff();
        PVMFCPMFactory::DestroyContentPolicyManager(iCPM);
        iCPM = NULL;
    }

    /* thread logoff */
    if (IsAdded())
        RemoveFromScheduler();

    /* Cleanup any old key */
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

    /* Cleanup allocated interfaces */
    if (iExtensionInterface)
    {
        /*
         * clear the interface container
         * the interface can't function without the node.
         */
        iExtensionInterface->iContainer = NULL;
        iExtensionInterface->removeRef();
    }

    /*
     * Cleanup commands
     * The command queues are self-deleting, but we want to
     * notify the observer of unprocessed commands.
     */
    while (iCurrentCommand.size() > 0)
    {
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFFailure);
    }
    while (iInputCommands.size() > 0)
    {
        CommandComplete(iInputCommands, iInputCommands.front(), PVMFFailure);
    }

    PVMF_STREAMING_MANAGER_DELETE(NULL,
                                  PVMFSMSessionSourceInfo,
                                  iSessionSourceInfo);

    /*
     * Clean up all children nodes & interfaces
     */
    uint32 i, j;
    for (i = 0; i < iNodeContainerVec.size(); i++)
    {
        for (j = 0; j < iNodeContainerVec[i].iExtensions.size(); j++)
        {
            PVInterface* extIntf = iNodeContainerVec[i].iExtensions[j];
            extIntf->removeRef();
        }

        if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_SOCKET_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFSocketNode,
                                          ((PVMFSocketNode*)(iNodeContainerVec[i].iNode)));
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE)
        {
            PVMFRrtspEngineNodeFactory::DeletePVMFRtspEngineNode(iNodeContainerVec[i].iNode);
            iNodeContainerVec[i].iNode = NULL;
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFJitterBufferNode,
                                          ((PVMFJitterBufferNode*)(iNodeContainerVec[i].iNode)));
        }
        else if (iNodeContainerVec[i].iNodeTag == PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE)
        {
            PVMF_STREAMING_MANAGER_DELETE(NULL,
                                          PVMFMediaLayerNode,
                                          ((PVMFMediaLayerNode*)(iNodeContainerVec[i].iNode)));
        }

    }
    iNodeContainerVec.clear();

    // destroy the payload parser registry
    destroyPayloadParserRegistry();


    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::~PVMFStreamingManagerNode - Out"));
}

void PVMFStreamingManagerNode::CleanupCPMdata()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CleanupCPMdata() Called"));

    /* Clean up CPM related variables */
    iSourceContextDataValid = false;
    iUseCPMPluginRegistry   = false;
    if (iCPMContentAccessFactory != NULL)
    {
        if (iDecryptionInterface != NULL)
        {
            iDecryptionInterface->Reset();
            /* Remove the decrpytion interface */
            PVUuid uuid = PVMFCPMPluginDecryptionInterfaceUuid;
            iCPMContentAccessFactory->DestroyPVMFCPMPluginAccessInterface(uuid, iDecryptionInterface);
            iDecryptionInterface = NULL;
        }
        iCPMContentAccessFactory->removeRef();
        iCPMContentAccessFactory = NULL;
    }
    iCPMContentType = PVMF_CPM_CONTENT_FORMAT_UNKNOWN;
    iPreviewMode = false;
    iCPMInitPending = false;
    iCPMMetadataKeys.clear();
}

/* Called during a Reset */
void PVMFStreamingManagerNode::ResetNodeParams()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ResetNodeParams - In"));

    iTotalNumRequestPortsComplete = 0;
    iNumRequestPortsPending = 0;
    iTotalNumInitPhaseRequestPortsComplete = 0;
    iTotalNumInitPhaseRequestPortsPending = 0;
    oGraphConstructComplete = false;
    oGraphConnectComplete = false;
    iAutoPausePending = false;
    iAutoResumePending = 0;
    iAutoPaused = false;
    iStreamThinningInProgress = false;
    iPlaylistPlayInProgress = false;
    iSwitchStreamIFrameVideo = false;

    iMetaDataInfo.Reset();
    iTrackInfoVec.clear();

    PVMFSMNodeContainerVector::iterator it;
    for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
    {
        it->Reset();
    }

    /* Delete current session info and recreate a new one */
    PVMF_STREAMING_MANAGER_DELETE(NULL,
                                  PVMFSMSessionSourceInfo,
                                  iSessionSourceInfo);
    iSessionSourceInfo = NULL;

    PVMF_STREAMING_MANAGER_NEW(NULL,
                               PVMFSMSessionSourceInfo,
                               (),
                               iSessionSourceInfo);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ResetNodeParams - Out"));
}

/**
 * Do thread-specific node creation and go to "Idle" state.
 */
OSCL_EXPORT_REF PVMFStatus PVMFStreamingManagerNode::ThreadLogon()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ThreadLogon - In"));

    PVMFStatus status = PVMFSuccess;
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
        {
            if (!IsAdded())
                AddToScheduler();

            iCmdSeqLogger = PVLogger::GetLoggerObject("pvplayercmdseq.streamingmanager");
            iReposLogger = PVLogger::GetLoggerObject("pvplayerrepos.sourcenode.streamingmanager");

            /*
             * Call thread logon for all the children nodes
             */
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                if (iNodeContainerVec[i].iNode->ThreadLogon() != PVMFSuccess)
                {
                    PVMF_SM_LOGERROR((0, "StreamingManagerNode - Child Node:ThreadLogon Failed, Node Tag %d", iNodeContainerVec[i].iNodeTag));
                    status = PVMFFailure;
                }
            }
            if (status == PVMFSuccess)
            {
                PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::ThreadLogon() - State - EPVMFNodeIdle"));
                SetState(EPVMFNodeIdle);
            }
        }
        break;
        default:
            status = PVMFErrInvalidState;
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ThreadLogon - Out"));
    return (status);
}

/**
 * Do thread-specific node cleanup and go to "Created" state.
 */
OSCL_EXPORT_REF PVMFStatus PVMFStreamingManagerNode::ThreadLogoff()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ThreadLogoff - In"));

    PVMFStatus status = PVMFSuccess;
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
        {
            /* Reset the metadata key list */
            /* Clean up CPM related variables */
            CleanupCPMdata();

            if (IsAdded())
                RemoveFromScheduler();
            iLogger = NULL;
            iCmdSeqLogger = NULL;
            iReposLogger = NULL;

            /*
             * Call thread logon for all the children nodes
             */
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                PVMFNodeInterface* node = iNodeContainerVec[i].iNode;
                if (node->GetState() != EPVMFNodeCreated)
                {
                    if (node->ThreadLogoff() != PVMFSuccess)
                    {
                        PVMF_SM_LOGERROR((0, "StreamingManagerNode - Child Node:ThreadLogoff Failed, Node Tag %d", iNodeContainerVec[i].iNodeTag));
                        status = PVMFFailure;
                    }
                }
            }
            if (status == PVMFSuccess)
            {
                SetState(EPVMFNodeCreated);
                PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::ThreadLogoff() - State - EPVMFNodeIdle"));
            }
            else
            {
                SetState(EPVMFNodeError);
                PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::ThreadLogoff() - State - EPVMFNodeError"));
            }
        }
        break;

        case EPVMFNodeCreated:
            status = PVMFSuccess;
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ThreadLogoff - Out"));
    return (status);
}

/**
 * retrieve node capabilities.
 */
OSCL_EXPORT_REF PVMFStatus PVMFStreamingManagerNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    PVMF_SM_LOGINFO((0, "StreamingManagerNode:GetCapability"));
    aNodeCapability = iCapability;
    return PVMFSuccess;
}

/**
 * retrive a port iterator.
 */
OSCL_EXPORT_REF PVMFPortIter* PVMFStreamingManagerNode::GetPorts(const PVMFPortFilter* aFilter)
{
    PVMF_SM_LOGINFO((0, "StreamingManagerNode:GetPorts"));
    OSCL_UNUSED_ARG(aFilter);//port filter is not implemented.
    return NULL;
}

/**
 * Queue an asynchronous QueryUUID command
 * QueryUUID for the streaming manager node is not complete until QueryUUIDs
 * are complete for all the children node (viz. session controller, jitter buffer
 * controller etc)
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::QueryUUID(PVMFSessionId s, const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryUUID - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_QUERYUUID, aMimeType, aUuids, aExactUuidsOnly, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryUUID - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::QueryUUID() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the Query UUID
 */
void PVMFStreamingManagerNode::DoQueryUuid(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoQueryUuid - In"));
    if (iQueryUUIDComplete == false)
    {
        /*
         * QueryUUID for streaming manager cannot be completed unless
         * QueryUUID for all the children nodes are complete
         */

        PVMFSMCommandContext* internalCmd = NULL;
        OsclAny *cmdContextData  = NULL;
        /*
         * QueryUUID from socket node
         */
        OSCL_StackString<50> socketNodeExtMimeType(PVMF_SOCKET_NODE_EXTENSION_INTERFACE_MIMETYPE);

        internalCmd = RequestNewInternalCmd();
        if (internalCmd == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoQueryUuid:RequestNewInternalCmd - Failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
            return;
        }
        internalCmd->cmd = PVMF_STREAMING_MANAGER_SOCKET_NODE_QUERY_UUID;
        internalCmd->parentCmd = aCmd.iCmd;
        cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

        PVMFSMNodeContainer* iSocketNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
        if (iSocketNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFNodeInterface* iSocketNode = iSocketNodeContainer->iNode;

        iSocketNode->QueryUUID(iSocketNodeContainer->iSessionId,
                               socketNodeExtMimeType,
                               iSocketNodeContainer->iExtensionUuids,
                               true,
                               cmdContextData);
        iSocketNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_PENDING;

        /*
         * QueryUUID from rtsp session controller
         */
        OSCL_StackString<50> sessionControllerExtMimeType(PVMF_RTSPENGINENODE_CUSTOM1_MIMETYPE);

        internalCmd = RequestNewInternalCmd();
        if (internalCmd == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoQueryUuid:RequestNewInternalCmd - Failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
            return;
        }
        internalCmd->cmd = PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_QUERY_UUID;
        internalCmd->parentCmd = aCmd.iCmd;
        cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFNodeInterface* iSessionControllerNode = iSessionControllerNodeContainer->iNode;

        iSessionControllerNode->QueryUUID(iSessionControllerNodeContainer->iSessionId,
                                          sessionControllerExtMimeType,
                                          iSessionControllerNodeContainer->iExtensionUuids,
                                          true,
                                          cmdContextData);
        iSessionControllerNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_PENDING;

        /*
         * QueryUUID from jitter buffer node
         */
        OSCL_StackString<50> jitterBufferExtMimeType(PVMF_JITTERBUFFER_CUSTOMINTERFACE_MIMETYPE);

        internalCmd = RequestNewInternalCmd();
        if (internalCmd == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoQueryUuid:RequestNewInternalCmd - Failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
            return;
        }
        internalCmd->cmd = PVMF_STREAMING_MANAGER_JITTER_BUFFER_QUERY_UUID;
        internalCmd->parentCmd = aCmd.iCmd;
        cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

        PVMFSMNodeContainer* iJitterBufferNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
        if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFNodeInterface* iJitterBufferNode = iJitterBufferNodeContainer->iNode;

        iJitterBufferNode->QueryUUID(iJitterBufferNodeContainer->iSessionId,
                                     jitterBufferExtMimeType,
                                     iJitterBufferNodeContainer->iExtensionUuids,
                                     true,
                                     cmdContextData);
        iJitterBufferNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_PENDING;

        /*
         * QueryUUID from media layer node
         */
        OSCL_StackString<50> mediaLayerExtMimeType(PVMF_MEDIALAYER_CUSTOMINTERFACE_MIMETYPE);

        internalCmd = RequestNewInternalCmd();
        if (internalCmd == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoQueryUuid:RequestNewInternalCmd - Failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
            return;
        }
        internalCmd->cmd = PVMF_STREAMING_MANAGER_MEDIA_LAYER_QUERY_UUID;
        internalCmd->parentCmd = aCmd.iCmd;
        cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

        PVMFSMNodeContainer* iMediaLayerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
        if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFNodeInterface* iMediaLayerNode = iMediaLayerNodeContainer->iNode;

        iMediaLayerNode->QueryUUID(iMediaLayerNodeContainer->iSessionId,
                                   mediaLayerExtMimeType,
                                   iMediaLayerNodeContainer->iExtensionUuids,
                                   true,
                                   cmdContextData);
        iMediaLayerNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
        /*
         * QueryUUID from HTTP PE node
         */
        OSCL_StackString<50> httpNodeExtMimeType(PVMF_PROTOCOL_ENGINE_MSHTTP_STREAMING_EXTENSION_MIMETYPE);

        internalCmd = RequestNewInternalCmd();
        if (internalCmd == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoQueryUuid:RequestNewInternalCmd - Failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
            return;
        }
        internalCmd->cmd = PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_QUERY_UUID;
        internalCmd->parentCmd = aCmd.iCmd;
        cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

        PVMFSMNodeContainer* iHTTPNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE);
        if (iHTTPNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFNodeInterface* iHTTPNode = iHTTPNodeContainer->iNode;

        iHTTPNode->QueryUUID(iHTTPNodeContainer->iSessionId,
                             httpNodeExtMimeType,
                             iHTTPNodeContainer->iExtensionUuids,
                             true,
                             cmdContextData);
        iHTTPNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_PENDING;

        /*
         * This node supports Query UUID from any state
         * Query UUID is asynchronous. move the command from
         * the input command queue to the current command, where
         * it will remain until the Query UUID completes.
         */
        MoveCmdToCurrentQueue(aCmd);
    }
    else if (iQueryUUIDComplete == true)
    {
        /*
         * We have already queried the child nodes for UUID
         */
        MoveCmdToCurrentQueue(aCmd);
        CompleteQueryUuid();
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoQueryUuid - Out"));
    return;
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesQueryUuid()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
        {
            return false;
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteQueryUuid()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteQueryUuid - In"));
    if (CheckChildrenNodesQueryUuid() || iQueryUUIDComplete == true)
    {
        PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();

        OSCL_String* mimetype;
        Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
        bool exactmatch;
        aCmd.PVMFStreamingManagerNodeCommandBase::Parse(mimetype, uuidvec, exactmatch);

        /*
         * Try to match the input mimetype against any of
         * the custom interfaces for this node
         * Match against extension interface...
         * also match against base mimetypes for extension interface,
         * unless exactmatch is set.
         */
        if (*mimetype == PVMF_STREAMINGMANAGER_CUSTOMINTERFACE_MIMETYPE)
        {
            PVUuid uuid(PVMF_STREAMINGMANAGERNODE_EXTENSIONINTERFACE_UUID);
            uuidvec->push_back(uuid);
        }
        else if (*mimetype == PVMF_DATA_SOURCE_INIT_INTERFACE_MIMETYPE)
        {
            PVUuid uuid(PVMF_DATA_SOURCE_INIT_INTERFACE_UUID);
            uuidvec->push_back(uuid);
        }
        else if (*mimetype == PVMF_TRACK_SELECTION_INTERFACE_MIMETYPE)
        {
            PVUuid uuid(PVMF_TRACK_SELECTION_INTERFACE_UUID);
            uuidvec->push_back(uuid);
        }
        else if (*mimetype == PVMF_DATA_SOURCE_PLAYBACK_CONTROL_INTERFACE_MIMETYPE)
        {
            PVUuid uuid(PvmfDataSourcePlaybackControlUuid);
            uuidvec->push_back(uuid);
        }
        else if (*mimetype == PVMF_META_DATA_EXTENSION_INTERFACE_MIMETYPE)
        {
            PVUuid uuid(KPVMFMetadataExtensionUuid);
            uuidvec->push_back(uuid);
        }

        PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::QueryUUID() - CmdComplete - PVMFSuccess"));
        iQueryUUIDComplete = true;

        CommandComplete(aCmd, PVMFSuccess);
        /* Erase the command from the current queue */
        iCurrentCommand.Erase(&iCurrentCommand.front());
        PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode::QueryUuid Complete"));
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteQueryUuid - Out"));
}

/**
 * Queue an asynchronous node command QueryInterface
 */
OSCL_EXPORT_REF PVMFCommandId
PVMFStreamingManagerNode::QueryInterface(PVMFSessionId s,
        const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryInterface - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_QUERYINTERFACE, aUuid, aInterfacePtr, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryInterface - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::QueryInterface() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the Query Interface.
 */
void PVMFStreamingManagerNode::DoQueryInterface(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoQueryInterface - In"));
    if (iQueryInterfaceComplete == false)
    {
        /*
         * QueryInterface for streaming manager cannot be completed unless
         * QueryInterface for all the children nodes are complete
         */
        for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
        {
            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    iNodeContainerVec[i].commandStartOffset +
                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_QUERY_INTERFACE_CMD_OFFSET;
                internalCmd->parentCmd = aCmd.iCmd;

                OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                iNode->QueryInterface(iNodeContainerVec[i].iSessionId,
                                      iNodeContainerVec[i].iExtensionUuids.front(),
                                      iExtensionInterfacePlaceholder,
                                      cmdContextData);
                iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
            }
            else
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoQueryInterface:RequestNewInternalCmd - Failed"));
                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        /*
         * This node supports QueryInterface from any state
         * QueryInterface is asynchronous. move the command from
         * the input command queue to the current command, where
         * it will remain until the QueryInterface completes.
         */
        MoveCmdToCurrentQueue(aCmd);
    }
    else if (iQueryInterfaceComplete == true)
    {
        /*
         * We have already queried the child nodes for interface
         */
        MoveCmdToCurrentQueue(aCmd);
        CompleteQueryInterface();
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoQueryInterface - Out"));
    return;
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesQueryInterface()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
        {
            return false;
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteQueryInterface()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteQueryInterface - In"));
    if (CheckChildrenNodesQueryInterface() || iQueryInterfaceComplete == true)
    {
        PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();

        PVUuid* uuid;
        PVInterface** ptr;
        aCmd.PVMFStreamingManagerNodeCommandBase::Parse(uuid, ptr);

        if ((*uuid == PVUuid(PVMF_STREAMINGMANAGERNODE_EXTENSIONINTERFACE_UUID)) ||
                (*uuid == PVUuid(PVMF_DATA_SOURCE_INIT_INTERFACE_UUID)) ||
                (*uuid == PVUuid(PVMF_TRACK_SELECTION_INTERFACE_UUID)) ||
                (*uuid == PVUuid(PvmfDataSourcePlaybackControlUuid)) ||
                (*uuid == PVUuid(KPVMFMetadataExtensionUuid)) ||
                (*uuid == PVUuid(PVMI_CAPABILITY_AND_CONFIG_PVUUID)) ||
                (*uuid == PVUuid(PVMFCPMPluginLicenseInterfaceUuid)))
        {
            if (!iExtensionInterface)
            {
                PVMFStreamingManagerNodeAllocator alloc;
                int32 err;
                OsclAny*ptr = NULL;
                OSCL_TRY(err,
                         ptr = alloc.ALLOCATE(sizeof(PVMFStreamingManagerExtensionInterfaceImpl));
                        );
                if (err != OsclErrNone || !ptr)
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode CompleteQueryInterface Failed"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                    return;
                }
                iExtensionInterface =
                    new(ptr) PVMFStreamingManagerExtensionInterfaceImpl(this,
                            aCmd.iSession);
            }

            iQueryInterfaceComplete = true;
            iExtensionInterface->queryInterface(*uuid, *ptr);
            iExtensionInterface->addRef();
            PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::QueryInterface() - CmdComplete - PVMFSuccess"));
            CommandComplete(aCmd, PVMFSuccess);
            /* Erase the command from the current queue */
            iCurrentCommand.Erase(&iCurrentCommand.front());

            PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode CompleteQueryInterface Success"));
        }
        else
        {
            PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::QueryInterface() - CmdFailed - PVMFErrNotSupported"));
            //not supported
            *ptr = NULL;
            CommandComplete(aCmd, PVMFErrNotSupported);
            /* Erase the command from the current queue */
            iCurrentCommand.Erase(&iCurrentCommand.front());
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteQueryInterface - Out"));
}

/**
 * Queue an asynchronous node command - RequestPort
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::RequestPort(PVMFSessionId s,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::RequestPort - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s,
            PVMF_STREAMING_MANAGER_NODE_REQUESTPORT,
            aPortTag,
            aPortConfig,
            aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::RequestPort - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::RequestPort() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the port request
 */
void PVMFStreamingManagerNode::DoRequestPort(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoRequestPort - In"));
    /*
     * This node supports port request only after the graph
     * has been fully constructed
     */
    if (oGraphConstructComplete)
    {
        /*
         * retrieve port tag
         */
        OSCL_String* mimetype;
        int32 tag;
        aCmd.PVMFStreamingManagerNodeCommandBase::Parse(tag, mimetype);
        /*
         * Do not Allocate a new port. Streaming manager treats the output
         * port from the media layer as its own output port. Find the media
         * layer output port corresponding to the input mimetype and hand the
         * same out
         */
        PVMFSMTrackInfo* trackInfo = FindTrackInfo(tag);

        PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
        int32 errcode = PVMFStreamingManagerNodeErrorInvalidRequestPortTag;

        if (trackInfo == NULL)
        {
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoRequestPort: FindTrackInfo failed"));
            CommandComplete(iInputCommands, aCmd, PVMFErrArgument, NULL, &eventuuid, &errcode);
            return;
        }
        if (trackInfo->iMediaLayerOutputPort == NULL)
        {
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoRequestPort: iMediaLayerOutputPort NULL"));
            CommandComplete(iInputCommands, aCmd, PVMFFailure, NULL, &eventuuid, &errcode);
            return;
        }
        PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::RequestPort() - CmdComplete - PVMFSuccess"));
        /*
         * Return the port pointer to the caller.
         */
        CommandComplete(iInputCommands,
                        aCmd,
                        PVMFSuccess,
                        (OsclAny*)(trackInfo->iMediaLayerOutputPort));

        PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode::DoRequestPort Success"));
    }
    else
    {
        PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::RequestPort() - CmdFailed - PVMFErrInvalidState"));
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoRequestPort Failed - InvalidState"));
        CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoRequestPort - Out"));
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::ReleasePort(PVMFSessionId s, PVMFPortInterface& aPort, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ReleasePort - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_RELEASEPORT, aPort, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::ReleasePort - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::ReleasePort() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the port release
 */
void PVMFStreamingManagerNode::DoReleasePort(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoReleasePort - In"));
    /*
     * Since the streaming manager does not have ports of its own,
     * a release port command typically translates to disconnecting
     * the underlying media layer port.
     */
    PVMFPortInterface* port;
    aCmd.PVMFStreamingManagerNodeCommandBase::Parse((PVMFPortInterface*&)port);

    /*
     * Find TrackInfo that corresponds to the Media Layer Output port
     * on which the current relase is being called.
     */
    PVMFSMTrackInfoVector::iterator it;
    PVMFSMTrackInfo* trackInfo = NULL;

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
        PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::ReleasePort() - CmdFailed - PVMFErrArgument"));
        /* invalid port */
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoReleasePort Failed - Invalid Port"));
        int32 errcode = PVMFStreamingManagerNodeErrorInvalidPort;
        CommandComplete(iInputCommands, aCmd, PVMFErrArgument, NULL, &eventuuid, &errcode);
        return;
    }
    PVMFStatus status = it->iMediaLayerOutputPort->Disconnect();

    if (status != PVMFSuccess)
    {
        PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode::DoReleasePort Success"));
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    else
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoReleasePort Failed"));
        CommandComplete(iInputCommands, aCmd, PVMFErrPortProcessing);
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoReleasePort - Out"));
}

/**
 * Queue an asynchronous node command - Init
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Init(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Init - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_INIT, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Init - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Init() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Call by DoInit as a prep step
 */
PVMFStatus PVMFStreamingManagerNode::DoPreInit(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoPreInit - In"));
    PVMFStatus status = PVMFSuccess;

    if (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE)
    {
    }
    else if (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE)
    {
        status = ProcessSDP();
        if (status == PVMFSuccess)
        {
            PVMFSMNodeContainer* iSessionControllerNodeContainer =
                getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);

            if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

            PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                (PVRTSPEngineNodeExtensionInterface*)
                (iSessionControllerNodeContainer->iExtensions[0]);

            /*
             * This vector is intentionally left uninitialized.
             * Streaming manager does not have any track selection info
             * at this stage. "SetSDPInfo" would be called again before
             * prepare complete to set up all the selected tracks. This
             * call is needed here to indicate to Session Controller node
             * that it is NOT a RTSP URL based session
             */
            Oscl_Vector<StreamInfo, OsclMemAllocator> aSelectedStream;

            status = rtspExtIntf->SetSDPInfo(iSessionSourceInfo->_sdpInfo,
                                             aSelectedStream);
            if (status != PVMFSuccess)
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoPreInit - SetSDPInfo Failed"));
            }
        }
        else
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoPreInit - ProcessSDP Failed"));
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoPreInit - Out"));
    return status;
}

void PVMFStreamingManagerNode::CompletePreInit()
{
    OSCL_ASSERT(iSessionSourceInfo->_sessionType ==
                PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL);

    Asf_CompletePreInit();
}

/**
 * Called by the command handler AO to do the node Init
 */
void PVMFStreamingManagerNode::DoInit(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoInit - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
        {
            /*
             * At first Init, PVMFErrLicneseRequest is replied from Janus.
             * Then iCPMInitPending is set into true.
             * If second Init is called, just to check license authentication is required.
             */
            if (iCPMInitPending == true)
            {
                MoveCmdToCurrentQueue(aCmd);
                {
                    CommandComplete(iCurrentCommand,
                                    iCurrentCommand.front(),
                                    PVMFFailure,
                                    NULL, NULL, NULL);
                    return;
                }
            }
            else
            {
                /* An asynchronous method that prepare's the node for init */
                PVMFStatus status = DoPreInit(aCmd);

                DeleteUnusedSessionControllerNode();

                if (status == PVMFSuccess)
                {
                    /*
                     * Init for streaming manager cannot be completed unless Init
                     * for all the children nodes are complete
                     */
                    PVMFSMNodeContainerVector::iterator it;
                    for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
                    {
                        {
                            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                            if (internalCmd != NULL)
                            {
                                internalCmd->cmd =
                                    it->commandStartOffset +
                                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_INIT_CMD_OFFSET;
                                internalCmd->parentCmd = aCmd.iCmd;

                                OsclAny *cmdContextData =
                                    OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                                PVMFNodeInterface* iNode = it->iNode;

                                iNode->Init(it->iSessionId, cmdContextData);
                                it->iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                            }
                            else
                            {
                                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoInit:RequestNewInternalCmd - Failed"));
                                CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                                return;
                            }
                        }
                    }
                    MoveCmdToCurrentQueue(aCmd);
                }
                else if (status == PVMFPending)
                {
                    MoveCmdToCurrentQueue(aCmd);
                }
                else
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoInit: DoPreInit() - Failed"));
                    PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
                    int32 errcode = PVMFStreamingManagerNodeErrorParseSDPFailed;
                    CommandComplete(iInputCommands, aCmd, PVMFFailure, NULL, &eventuuid, &errcode);
                    return;
                }
            }
        }
        break;

        default:
        {
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoInit Failed - Invalid State"));
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
        }
        break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoInit - Out"));
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesInit()
{
    {
        for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
        {
            if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
            {
                return false;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteInit()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteInit - In"));
    if (CheckChildrenNodesInit() && iErrorDuringProcess == SM_NO_ERROR)
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
            if (aCmd.iCmd == PVMF_STREAMING_MANAGER_NODE_INIT)
            {
                // create and pass the payload parser registry on to the media layer node
                PopulatePayloadParserRegistry();

                PVMFStatus status = PVMFSuccess;

                if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL))
                {
                    status = ProcessSDP();
                }
                if (status == PVMFSuccess)
                {
                    status = InitMetaData();
                    if (status == PVMFSuccess)
                    {
                    }
                    else
                    {
                        PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteInit - InitMetaData fail"));
                    }

                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Init() - CmdComplete - PVMFSuccess"));
                    //Init is completed at unprotected clip
                    SetState(EPVMFNodeInitialized);
                    PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode::CompleteInit Success"));
                    CommandComplete(aCmd, PVMFSuccess);
                }
                else
                {
                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Init() - Cmd Failed - PVMFStreamingManagerNodeErrorParseSDPFailed"));
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::CompleteInit Failure"));
                    PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
                    int32 errcode = PVMFStreamingManagerNodeErrorParseSDPFailed;
                    CommandComplete(aCmd, status, NULL, &eventuuid, &errcode);
                }
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteInit - Out"));
    return;
}

/**
 * Queue an asynchronous node command - Prepare
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Prepare(PVMFSessionId s,
        const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Prepare - In"));
    /* Queue an internal command for Graph construct */
    PVMFStreamingManagerNodeCommand cmdGC;
    cmdGC.PVMFStreamingManagerNodeCommandBase::Construct(s,
            PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION,
            NULL);

    QueueCommandL(cmdGC);

    PVMFStreamingManagerNodeCommand cmdPrep;
    cmdPrep.PVMFStreamingManagerNodeCommandBase::Construct(s,
            PVMF_STREAMING_MANAGER_NODE_PREPARE,
            aContext);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Prepare - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Prepare() - Cmd Recvd"));
    return QueueCommandL(cmdPrep);
}

/**
 * Called by the command handler AO to do the node Prepare
 */
void PVMFStreamingManagerNode::DoPrepare(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoPrepare - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
        {
            if (oGraphConstructComplete)
            {
                /*
                 * Connect the graph here. This is needed since we would send firewall packets
                 * as part of Prepare.
                 */
                if (GraphConnect() == false)
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoPrepare - GraphConnect Failed"));
                    SetState(EPVMFNodeError);
                    PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
                    int32 errcode = PVMFStreamingManagerNodeGraphConnectFailed;
                    CommandComplete(aCmd, PVMFFailure, NULL, &eventuuid, &errcode);
                    return;
                }
                {
                    /*
                     * Prepare for streaming manager cannot be completed unless Prepare
                     * for all the children nodes are complete
                     */
                    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
                    {
                        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                        if (internalCmd != NULL)
                        {
                            internalCmd->cmd =
                                iNodeContainerVec[i].commandStartOffset +
                                PVMF_STREAMING_MANAGER_NODE_INTERNAL_PREPARE_CMD_OFFSET;
                            internalCmd->parentCmd = aCmd.iCmd;

                            OsclAny *cmdContextData =
                                OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                            PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                            iNode->Prepare(iNodeContainerVec[i].iSessionId, cmdContextData);
                            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                        }
                        else
                        {
                            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoPrepare:RequestNewInternalCmd - Failed"));
                            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                            return;
                        }
                    }
                }
                MoveCmdToCurrentQueue(aCmd);
            }
            else
            {
                /* Graph construction not complete, so cant prep */
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoPrepare Failed - Incomplete Graph"));
                CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            }
        }
        break;

        default:
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoPrepare Failed - Invalid State"));
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoPrepare - Out"));
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesPrepare()
{
    {
        for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
        {
            if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
            {
                return false;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompletePrepare()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompletePrepare - In"));
    if ((CheckChildrenNodesPrepare()) && (oGraphConstructComplete) && iErrorDuringProcess == SM_NO_ERROR)
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
            if (aCmd.iCmd == PVMF_STREAMING_MANAGER_NODE_PREPARE)
            {
                SetState(EPVMFNodePrepared);
                PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Prepare() - CmdComplete - PVMFSuccess"));
                CommandComplete(aCmd, PVMFSuccess);
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompletePrepare - Out"));
    return;
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Start(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Start - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_START, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Start - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Start() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the node Start
 */
void PVMFStreamingManagerNode::DoStart(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoStart - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
        {
            /*
             * Connect the graph if not already connected. Usually the graph is
             * disconnected as part of Stop. In case we are doing a start after
             * stop, we would need to connect the graph again.
             */
            if (GraphConnect() == false)
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:CompleteStart - GraphConnect Failed"));
                SetState(EPVMFNodeError);
                PVUuid eventuuid = PVMFStreamingManagerNodeEventTypeUUID;
                int32 errcode = PVMFStreamingManagerNodeGraphConnectFailed;
                CommandComplete(aCmd, PVMFFailure, NULL, &eventuuid, &errcode);
                return;
            }
            /*
             * Start for streaming manager cannot be completed unless
             * Start for all the children nodes are complete
             */
            {
                if (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE)
                {
                    uint32 duration = 2000;
                    PVMFSMNodeContainer* iJitterBufferNodeContainer =
                        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                    PVMFJitterBufferExtensionInterface* jbExtIntf =
                        (PVMFJitterBufferExtensionInterface*)
                        (iJitterBufferNodeContainer->iExtensions[0]);

                    /* Set jitter buffer duration */
                    jbExtIntf->setJitterBufferDurationInMilliSeconds(duration);
                }
                /*
                 * Start for streaming manager cannot be completed unless
                 * Start for all the children nodes are complete
                 */
                for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
                {
                    PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                    if (internalCmd != NULL)
                    {
                        internalCmd->cmd =
                            iNodeContainerVec[i].commandStartOffset +
                            PVMF_STREAMING_MANAGER_NODE_INTERNAL_START_CMD_OFFSET;
                        internalCmd->parentCmd = aCmd.iCmd;

                        OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                        PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                        iNode->Start(iNodeContainerVec[i].iSessionId, cmdContextData);
                        iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                    }
                    else
                    {
                        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoStart:RequestNewInternalCmd - Failed"));
                        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                        return;
                    }
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        break;

        /*
         * GraphConnect() not needed if starting from a paused state
         */
        case EPVMFNodePaused:
        {
            /*
             * Start for streaming manager cannot be completed unless
             * Start for all the children nodes are complete
             */
            {
                for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
                {
                    if (iNodeContainerVec[i].iAutoPaused == false)
                    {
                        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                        if (internalCmd != NULL)
                        {
                            internalCmd->cmd =
                                iNodeContainerVec[i].commandStartOffset +
                                PVMF_STREAMING_MANAGER_NODE_INTERNAL_START_CMD_OFFSET;
                            internalCmd->parentCmd = aCmd.iCmd;

                            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                            PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                            iNode->Start(iNodeContainerVec[i].iSessionId, cmdContextData);
                            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                        }
                        else
                        {
                            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoStart:RequestNewInternalCmd - Failed"));
                            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                            return;
                        }
                    }
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        break;

        case EPVMFNodeStarted:
            //Ignore start if already started
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoStart - Out"));
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesStart()
{
    {
        for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
        {
            PVMFSMNodeCmdState tmp = iNodeContainerVec[i].iNodeCmdState;
            uint32 tag = iNodeContainerVec[i].iNodeTag;
            if (iNodeContainerVec[i].iNodeCmdState == PVMFSM_NODE_CMD_PENDING)
            {
                return false;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteStart()
{
    int32 localMode = 0;
    if (iPVMFDataSourcePositionParamsPtr != NULL)
    {
        localMode = iPVMFDataSourcePositionParamsPtr->iMode;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteStart - In"));
    if (CheckChildrenNodesStart() && iErrorDuringProcess == SM_NO_ERROR)
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
            if ((aCmd.iCmd == PVMF_STREAMING_MANAGER_NODE_START) ||
                    (aCmd.iCmd == PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION))
            {
                if (oRepositioning)
                {
                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::SetDataSourcePosition() - CmdComplete - PMVFSuccess"));
                    oRepositioning = false;
                    oPlayListRepositioning = false;

                    if ((localMode == 0) || (localMode == -1))
                    {
                        GetAcutalMediaTSAfterSeek();
                    }

                    iPVMFDataSourcePositionParamsPtr = NULL;
                }
                if ((iAutoResumePending == 2) && (iAutoPaused == true))
                {
                    iAutoResumePending = 1;
                    /* internal command - session id does not matter */
                    PVMFSessionId s = 0;
                    PVMFStreamingManagerNodeCommand cmdAutoResume;
                    cmdAutoResume.PVMFStreamingManagerNodeCommandBase::Construct(s,
                            PVMF_STREAMING_MANAGER_NODE_AUTO_RESUME,
                            NULL);
                    QueueCommandL(cmdAutoResume);
                    PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:CompleteStart: - AutoResume Queued"));
                }
                if ((localMode == 0) || (localMode == -1))
                {
                    SetState(EPVMFNodeStarted);
                    if (IsAdded())
                    {
                        /* wakeup the AO */
                        RunIfNotReady();
                    }
                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Start() - CmdComplete - PMVFSuccess"));
                }
                CommandComplete(aCmd, PVMFSuccess);
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteStart - Out"));
    return;
}

/**
 * Called by the command handler AO to do the Auto Resume
 */
void PVMFStreamingManagerNode::DoAutoResume(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoAutoResume - In"));

    if (iSessionStopTimeAvailable == false)
    {
        /* Implies an open ended session - no pause or reposition */
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoAutoResume in live  - PVMFErrNotSupported"));
        InternalCommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
        return;
    }

    bool oCmdSent = false;
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        {
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                /* We only pause session controller and socket nodes in auto-pause */
                int32 nodeTag = iNodeContainerVec[i].iNodeTag;
                PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                if ((nodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE) ||
                        (nodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE))
                {
                    if (iNode->GetState() != EPVMFNodeStarted)
                    {
                        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                        if (internalCmd != NULL)
                        {
                            internalCmd->cmd =
                                iNodeContainerVec[i].commandStartOffset +
                                PVMF_STREAMING_MANAGER_NODE_INTERNAL_START_CMD_OFFSET;
                            internalCmd->parentCmd = aCmd.iCmd;

                            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);
                            iNode->Start(iNodeContainerVec[i].iSessionId, cmdContextData);
                            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                            oCmdSent = true;
                        }
                        else
                        {
                            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoAutoResume:RequestNewInternalCmd - Failed"));
                            InternalCommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                            return;
                        }
                    }
                }
            }
            if (oCmdSent)
            {
                MoveCmdToCurrentQueue(aCmd);
                PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoResume: - Start Sent"));
            }
            else
            {
                iAutoResumePending = 0;
                InternalCommandComplete(iInputCommands, aCmd, PVMFSuccess);
                PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoResume: - Done"));
            }
        }
        break;

        case EPVMFNodePaused:
        {
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                int32 nodeTag = iNodeContainerVec[i].iNodeTag;
                if (nodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE)
                {
                    iNodeContainerVec[i].iAutoPaused = false;
                }
            }
            iAutoResumePending = 0;
            InternalCommandComplete(iInputCommands, aCmd, PVMFSuccess);
            PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoResume in EPVMFNodePaused: - Done"));
        }
        break;
        default:
        {
            iAutoResumePending = 0;
            InternalCommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
        }
        break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoAutoResume - Out"));
    return;
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesAutoResume()
{
    PVMFSMNodeContainerVector::iterator it;
    for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
    {
        int32 nodeTag = it->iNodeTag;
        if ((nodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE))
        {
            if (it->iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE
                    && it->iNodeCmdState != PVMFSM_NODE_CMD_NO_PENDING)
            {
                return false;
            }
            else
            {
                it->iAutoPaused = false;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteAutoResume()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteAutoResume - In"));
    if (CheckChildrenNodesAutoResume() && iErrorDuringProcess == SM_NO_ERROR)
    {
        iAutoResumePending = 0;
        iAutoPaused = false;
        /* Notify jitter buffer so that it may pause its estimated server clock */
        PVMFSMNodeContainer* iJitterBufferNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
        if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFJitterBufferExtensionInterface* jbExtIntf =
            (PVMFJitterBufferExtensionInterface*)
            (iJitterBufferNodeContainer->iExtensions[0]);
        if (jbExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);
        jbExtIntf->NotifyAutoResumeComplete();

        PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
        InternalCommandComplete(aCmd, PVMFSuccess);
        /* Erase the command from the current queue */
        iCurrentCommand.Erase(&iCurrentCommand.front());
        PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoResume: - Done"));
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteAutoResume - Out"));
    return;
}


/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Stop(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Stop - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_STOP, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Stop - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Stop() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the node Stop
 */
void PVMFStreamingManagerNode::DoStop(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoStop - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            {
                /*
                 * Stop for streaming manager cannot be completed unless
                 * Stop for all the children nodes are complete
                 */
                for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
                {
                    PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                    if (internalCmd != NULL)
                    {
                        internalCmd->cmd =
                            iNodeContainerVec[i].commandStartOffset +
                            PVMF_STREAMING_MANAGER_NODE_INTERNAL_STOP_CMD_OFFSET;
                        internalCmd->parentCmd = aCmd.iCmd;

                        OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                        PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                        iNode->Stop(iNodeContainerVec[i].iSessionId, cmdContextData);
                        iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                    }
                    else
                    {
                        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoStop:RequestNewInternalCmd - Failed"));
                        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                        return;
                    }
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        break;

        default:
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoStop Failure - Invalid State"));
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoStop - Out"));
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesStop()
{
    {
        for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
        {
            if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
            {
                return false;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteStop()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteStop - In"));
    if (CheckChildrenNodesStop() && iErrorDuringProcess == SM_NO_ERROR)
    {
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
            if (aCmd.iCmd == PVMF_STREAMING_MANAGER_NODE_STOP)
            {
                /* transition to Prepared state */
                ResetStopCompleteParams();
                SetState(EPVMFNodePrepared);
                PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Stop() - CmdComplete - PVMFSuccess"));
                CommandComplete(aCmd, PVMFSuccess);
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteStop - Out"));
    return;
}

void PVMFStreamingManagerNode::ResetStopCompleteParams()
{
    iStreamThinningInProgress = false;
    iPlaylistPlayInProgress = false;
    iSwitchStreamIFrameVideo = false;
    iRepositionRequestedStartNPTInMS = 0;
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Flush(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Flush - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_FLUSH, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Flush - Out"));
    return QueueCommandL(cmd);
}
/**
 * Called by the command handler AO to do the node Flush
 */
void PVMFStreamingManagerNode::DoFlush(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoFlush - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
        {
            /*
             * Flush for streaming manager cannot be completed unless
             * Flush for all the children nodes are complete
             */
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                if (internalCmd != NULL)
                {
                    internalCmd->cmd =
                        iNodeContainerVec[i].commandStartOffset +
                        PVMF_STREAMING_MANAGER_NODE_INTERNAL_FLUSH_CMD_OFFSET;
                    internalCmd->parentCmd = aCmd.iCmd;

                    OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                    PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                    iNode->Flush(iNodeContainerVec[i].iSessionId, cmdContextData);
                    iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                }
                else
                {
                    PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoFlush:RequestNewInternalCmd - Failed"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                    return;
                }
            }
            MoveCmdToCurrentQueue(aCmd);
            /*
             * Notify all ports to suspend their input - TBD
             */
            /*
             * If the node is not running we need to wakeup the
             * AO to further complete the flush, which means all
             * port activity needs to be completed.
             */
            if (IsAdded())
            {
                RunIfNotReady();
            }
        }
        break;

        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoFlush - Out"));
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesFlush()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
        {
            return false;
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteFlush()
{
    /*
     * If the node is not running we need to wakeup the
     * AO to further complete the flush, which means all
     * port activity needs to be completed.
     */
    if (iInterfaceState != EPVMFNodeStarted)
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    return;
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Pause(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Pause - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_PAUSE, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Pause - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Pause() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the node Pause
 */
void PVMFStreamingManagerNode::DoPause(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoPause - In"));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        {
            {
                for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
                {
                    /*
                     * Pause only if not already paused - could happen that
                     * some of the nodes could be paused due to flow control
                     */
                    if ((iNodeContainerVec[i].iNode->GetState()) != EPVMFNodePaused)
                    {
                        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                        if (internalCmd != NULL)
                        {
                            internalCmd->cmd =
                                iNodeContainerVec[i].commandStartOffset +
                                PVMF_STREAMING_MANAGER_NODE_INTERNAL_PAUSE_CMD_OFFSET;
                            internalCmd->parentCmd = aCmd.iCmd;

                            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                            PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                            iNode->Pause(iNodeContainerVec[i].iSessionId, cmdContextData);
                            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                        }
                        else
                        {
                            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoPause:RequestNewInternalCmd - Failed"));
                            CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                            return;
                        }
                    }
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        break;
        default:
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoPause - Out"));
    return;
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesPause()
{
    {
        for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
        {
            if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE
                    && iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_NO_PENDING)
            {
                return false;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompletePause()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompletePause - In"));
    if (CheckChildrenNodesPause() && iErrorDuringProcess == SM_NO_ERROR)
    {
        SetState(EPVMFNodePaused);
        if (oRepositioning)
        {
            if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
                    (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
            {
                /*
                 * Pause request generated by a reposition command
                 * complete. Issue a start.
                 */
                if (iPVMFDataSourcePositionParamsPtr == NULL)
                {
                    DoRepositioningStart3GPPStreaming();
                }
            }
        }
        else
        {
            if (!iCurrentCommand.empty() && iCancelCommand.empty())
            {
                PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
                PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Pause() - CmdComplete - PVMFSuccess"));
                CommandComplete(aCmd, PVMFSuccess);
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }
        }

    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompletePause - Out"));
    return;
}

/**
 * Called by the command handler AO to do the Auto Pause
 */
void PVMFStreamingManagerNode::DoAutoPause(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoAutoPause - In"));

    if (iSessionStopTimeAvailable == false)
    {
        /* Implies an open ended session - no pause or reposition */
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoAutoPause in live  - PVMFErrNotSupported"));
        InternalCommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
        return;
    }
    bool oCmdSent = false;
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        {
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                /* We only pause session controller and socket nodes in auto-pause */
                int32 nodeTag = iNodeContainerVec[i].iNodeTag;
                PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                if ((nodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE) ||
                        (nodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE))
                {
                    if (iNode->GetState() != EPVMFNodePaused)
                    {
                        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                        if (internalCmd != NULL)
                        {
                            internalCmd->cmd =
                                iNodeContainerVec[i].commandStartOffset +
                                PVMF_STREAMING_MANAGER_NODE_INTERNAL_PAUSE_CMD_OFFSET;
                            internalCmd->parentCmd = aCmd.iCmd;

                            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);
                            iNode->Pause(iNodeContainerVec[i].iSessionId, cmdContextData);
                            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                            oCmdSent = true;
                        }
                        else
                        {
                            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoAutoPause:RequestNewInternalCmd - Failed"));
                            InternalCommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                            return;
                        }
                    }
                }
            }
            if (oCmdSent == true)
            {
                MoveCmdToCurrentQueue(aCmd);
                PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoPause: - Pause Sent"));
            }
            else
            {
                iAutoPausePending = false;
                InternalCommandComplete(iInputCommands, aCmd, PVMFSuccess);
                PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoPause: - Done"));
            }
        }
        break;
        case EPVMFNodePaused:
        {
            /*
             * If state is EPVMFNodePaused when Highwatermark event is reported, we ignore this event.
             * But if Highwatermark event is reported while pause cmd is pending,
             * we call DoAutoPause and JB node expect SM node go to autopause. So we set iAutoPaused to true.
             */
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                int32 nodeTag = iNodeContainerVec[i].iNodeTag;
                if (nodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE)
                {
                    iNodeContainerVec[i].iAutoPaused = true;
                }
            }
            iAutoPausePending = false;
            iAutoPaused = true;

            InternalCommandComplete(iInputCommands, aCmd, PVMFSuccess);
            PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoPause in EPVMFNodePaused: - Done"));
        }
        break;
        default:
        {
            iAutoPausePending = false;
            InternalCommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
        }
        break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoAutoPause - Out"));
    return;
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesAutoPause()
{
    PVMFSMNodeContainerVector::iterator it;
    for (it = iNodeContainerVec.begin(); it !=  iNodeContainerVec.end(); it++)
    {
        int32 nodeTag = it->iNodeTag;
        if ((nodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE))
        {
            if (it->iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE
                    && it->iNodeCmdState != PVMFSM_NODE_CMD_NO_PENDING)
            {
                return false;
            }
            else
            {
                it->iAutoPaused = true;
            }
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteAutoPause()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteAutoPause - In"));
    if (CheckChildrenNodesAutoPause() && iErrorDuringProcess == SM_NO_ERROR)
    {
        iAutoPausePending = false;
        iAutoPaused = true;
        /* Notify jitter buffer so that it may pause its estimated server clock */
        PVMFSMNodeContainer* iJitterBufferNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
        if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFJitterBufferExtensionInterface* jbExtIntf =
            (PVMFJitterBufferExtensionInterface*)
            (iJitterBufferNodeContainer->iExtensions[0]);
        if (jbExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);
        jbExtIntf->NotifyAutoPauseComplete();

        PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
        InternalCommandComplete(aCmd, PVMFSuccess);
        /* Erase the command from the current queue */
        iCurrentCommand.Erase(&iCurrentCommand.front());
        PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:DoAutoPause: - Done"));
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteAutoPause - Out"));
    return;
}


/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::Reset(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Reset - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_RESET, aContext);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::Reset - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Reset() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Called by the command handler AO to do the node Reset.
 */
void PVMFStreamingManagerNode::DoReset(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoReset - In"));
    /* this node allows a reset from any idle or error state */
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
            /*
             * Reset for streaming manager cannot be completed unless
             * Reset for all the children nodes are complete
             */
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                if (internalCmd != NULL)
                {
                    internalCmd->cmd =
                        iNodeContainerVec[i].commandStartOffset +
                        PVMF_STREAMING_MANAGER_NODE_INTERNAL_RESET_CMD_OFFSET;
                    internalCmd->parentCmd = aCmd.iCmd;

                    OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                    PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                    iNode->Reset(iNodeContainerVec[i].iSessionId, cmdContextData);
                    iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
                }
                else
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoReset:RequestNewInternalCmd - Failed"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                    return;
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        break;

        default:
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::DoReset Failure - Invalid State"));
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            break;
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoReset - Out"));
}

bool
PVMFStreamingManagerNode::CheckChildrenNodesReset()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_COMPLETE)
        {
            return false;
        }
    }
    return true;
}

void PVMFStreamingManagerNode::CompleteReset()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteReset - In"));
    if (CheckChildrenNodesReset() && iDRMResetPending == false)
    {
        ResetNodeContainerCmdState();
        if (!iCurrentCommand.empty() && iCancelCommand.empty())
        {
            /* Indicates that the init for Children Nodes was successfull */
            /* At protected clip, Reset CPM also was successfull */
            PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
            if (aCmd.iCmd == PVMF_STREAMING_MANAGER_NODE_RESET)
            {

                /* Reset Params */
                ResetNodeParams();

                /* Reset the metadata key list */
                /* Clean up CPM related variables */
                CleanupCPMdata();

                /* logoff & go back to Created state */
                SetState(EPVMFNodeIdle);
                PVMFStatus status = ThreadLogoff();

                if (iErrorDuringProcess == SM_NO_ERROR)
                {
                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::Reset() - CmdComplete - Status=%d", status));
                    CommandComplete(aCmd, status);
                }
                else
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::CompleteReset -- CPM Reset error"));
                    CommandComplete(aCmd, PVMFFailure, iErrorResponseInf);

                    if (iErrorResponseInf != NULL)
                    {
                        iErrorResponseInf->removeRef();
                        iErrorResponseInf = NULL;
                    }
                    iErrorDuringProcess = SM_NO_ERROR;
                }

                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
                // destroy the payload parser registry
                destroyPayloadParserRegistry();
            }
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteReset - Out"));
    return;
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::CancelAllCommands(PVMFSessionId s, const OsclAny* aContext)
{
    PVMF_SM_LOGINFO((0, "StreamingManagerNode:CancelAllCommands"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS, aContext);
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::CancelAllCommands() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * Queue an asynchronous node command
 */
OSCL_EXPORT_REF PVMFCommandId PVMFStreamingManagerNode::CancelCommand(PVMFSessionId s, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVMF_SM_LOGINFO((0, "StreamingManagerNode:CancelCommand"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(s, PVMF_STREAMING_MANAGER_NODE_CANCELCOMMAND, aCmdId, aContext);
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::CancelCommand() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

/**
 * This routine is called by various command APIs to queue an
 * asynchronous command for processing by the command handler AO.
 * This function may leave if the command can't be queued due to
 * memory allocation failure.
 */
PVMFCommandId PVMFStreamingManagerNode::QueueCommandL(PVMFStreamingManagerNodeCommand& aCmd)
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
 * Called by the command handler AO to process a command from
 * the input queue.
 * Return true if a command was processed, false if the command
 * processor is busy and can't process another command now.
 */
bool PVMFStreamingManagerNode::ProcessCommand(PVMFStreamingManagerNodeCommand& aCmd)
{
    /*
     * normally this node will not start processing one command
     * until the prior one is finished.  However, a hi priority
     * command such as Cancel must be able to interrupt a command
     * in progress.
     */
    if ((iCurrentCommand.size() > 0 && !aCmd.hipri()
            && aCmd.iCmd != PVMF_STREAMING_MANAGER_NODE_CANCEL_GET_LICENSE)
            || iCancelCommand.size() > 0)
        return false;

    switch (aCmd.iCmd)
    {
        case PVMF_STREAMING_MANAGER_NODE_QUERYUUID:
            DoQueryUuid(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_REQUESTPORT:
            DoRequestPort(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_RELEASEPORT:
            DoReleasePort(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_INIT:
            DoInit(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_PREPARE:
            DoPrepare(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_START:
            DoStart(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_STOP:
            DoStop(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_FLUSH:
            DoFlush(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_PAUSE:
            DoPause(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_RESET:
            DoReset(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS:
            DoCancelAllCommands(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_CANCELCOMMAND:
            DoCancelCommand(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION:
        {
            if (!GraphConstruct())
            {
                InternalCommandComplete(iInputCommands,
                                        aCmd,
                                        PVMFFailure);
            }
            else
            {
                /*
                 * Move it to current command queue where it would
                 * stay until "GraphConstruct" is complete
                 */
                MoveCmdToCurrentQueue(aCmd);
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION:
        {
            {
                DoSetDataSourcePosition(aCmd);
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_NODE_QUERY_DATASOURCE_POSITION:
            DoQueryDataSourcePosition(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAKEYS:
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

        case PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAVALUES:
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
        case PVMF_STREAMING_MANAGER_NODE_CAPCONFIG_SETPARAMS:
        {
            PvmiMIOSession session;
            PvmiKvp* aParameters;
            int num_elements;
            PvmiKvp** ppRet_kvp;
            aCmd.Parse(session, aParameters, num_elements, ppRet_kvp);
            setParametersSync(NULL, aParameters, num_elements, *ppRet_kvp);
            ciObserver->SignalEvent(aCmd.iId);
        }

        case PVMF_STREAMING_MANAGER_NODE_AUTO_PAUSE:
            DoAutoPause(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_AUTO_RESUME:
            DoAutoResume(aCmd);
            break;

        case PVMF_STREAMING_MANAGER_NODE_GET_LICENSE_W:
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

        case PVMF_STREAMING_MANAGER_NODE_GET_LICENSE:
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

        case PVMF_STREAMING_MANAGER_NODE_CANCEL_GET_LICENSE:
            DoCancelGetLicense(aCmd);
            break;

        default:
            /* unknown command type */
            CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
            break;
    }

    return true;
}

/**
 * The various command handlers call this when a command is complete.
 */
void PVMFStreamingManagerNode::CommandComplete(PVMFStreamingManagerNodeCmdQ& aCmdQ,
        PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode,
        PVInterface* aExtMsg)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    PVInterface* extif = NULL;
    PVMFBasicErrorInfoMessage* errormsg = NULL;
    if (aExtMsg)
    {
        extif = aExtMsg;
    }
    else if (aEventUUID && aEventCode)
    {
        errormsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL));
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
    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other error implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

void PVMFStreamingManagerNode::CommandComplete(PVMFStreamingManagerNodeCmdQ& aCmdQ,
        PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    PVInterface* extif = NULL;
    PVMFBasicErrorInfoMessage* errormsg = NULL;
    if (aEventUUID && aEventCode)
    {
        errormsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL));
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
    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty() && IsAdded())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other error implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

void PVMFStreamingManagerNode::CommandComplete(PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    PVInterface* extif = NULL;
    PVMFBasicErrorInfoMessage* errormsg = NULL;
    if (aEventUUID && aEventCode)
    {
        errormsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL));
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
    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

/**
 * The various command handlers call this when a command is complete.
 */
void PVMFStreamingManagerNode::CommandComplete(PVMFStreamingManagerNodeCmdQ& aCmdQ,
        PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        PVInterface* aErrorExtIntf)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext));

    /* create response */
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aErrorExtIntf, NULL);
    PVMFSessionId session = aCmd.iSession;

    /* Erase the command from the queue */
    aCmdQ.Erase(&aCmd);

    /* Report completion to the session observer */
    ReportCmdCompleteEvent(session, resp);

    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other error implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

void PVMFStreamingManagerNode::CommandComplete(PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        PVInterface* aErrorExtIntf,
        OsclAny* aData)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext));

    /* create response */
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aErrorExtIntf, aData);
    PVMFSessionId session = aCmd.iSession;

    /* Report completion to the session observer */
    ReportCmdCompleteEvent(session, resp);

    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

/**
 * The various command handlers call this when an internal command is complete.
 * Does not report events to the observer
 */
void
PVMFStreamingManagerNode::InternalCommandComplete(PVMFStreamingManagerNodeCmdQ& aCmdQ,
        PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    /* Erase the command from the queue */
    aCmdQ.Erase(&aCmd);

    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }

    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

void
PVMFStreamingManagerNode::InternalCommandComplete(PVMFStreamingManagerNodeCommand& aCmd,
        PVMFStatus aStatus,
        OsclAny* aEventData)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:CommandComplete Id %d Cmd %d Status %d Context %d Data %d"
                           , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    /* Reschedule AO if input command queue is not empty */
    if (!iInputCommands.empty())
    {
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
    /*
     * Transition to error state in case of select errors only, viz.
     * PVMFFailure, PVMFErrNoMemory, PVMFErrNoResources
     * Any other status implies that the node is probably in a recoverable
     * state
     */
    if (IsFatalErrorEvent(aStatus))
    {
        SetState(EPVMFNodeError);
    }
}

/**
//A routine to tell if a flush operation is in progress.
*/
bool PVMFStreamingManagerNode::FlushPending()
{
    if ((iCurrentCommand.size() > 0) &&
            (iCurrentCommand.front().iCmd == PVMF_STREAMING_MANAGER_NODE_FLUSH) &&
            (CheckChildrenNodesFlush() == false))
    {
        return true;
    }
    return false;
}

/**
//Called by the command handler AO to do the Cancel All
*/
void PVMFStreamingManagerNode::DoCancelAllCommands(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFStreamingManagerNode::DoCancelAllCommands In"));
    //first cancel the current command if any
    if (iCurrentCommand.size() > 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFStreamingManagerNode::DoCancelAllCommands iCurrentCommand.size()>0"));
        /*
         * As of now we only allow cancelling the start command. Also only jitter buffer start
         * needs to be cancelled. Rest of the nodes dont do anything in start.
         */
        {
            ResetNodeContainerCmdState();
            for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
            {
                PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
                if (internalCmd != NULL)
                {
                    internalCmd->cmd =
                        iNodeContainerVec[i].commandStartOffset +
                        PVMF_STREAMING_MANAGER_NODE_INTERNAL_CANCEL_ALL_OFFSET;
                    internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS;

                    OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                    PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                    iNode->CancelAllCommands(iNodeContainerVec[i].iSessionId, cmdContextData);
                    iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_CANCEL_PENDING;
                }
                else
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoCancelAllCommands:RequestNewInternalCmd - Failed"));
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                    return;
                }
            }
            /* Wait for jitter buffer cancel all to complete */
            /* If current command is not start then just wait for current command to complete */
        }
        MoveCmdToCancelQueue(aCmd);
    }
    else
    {
        //no current command - just cancel all queued commands, if any
        {
            //start at element 1 since this cancel command is element 0.
            while (iInputCommands.size() > 1)
            {
                if (IsInternalCmd(iInputCommands.front().iCmd) == false)
                {
                    CommandComplete(iInputCommands.front(), PVMFErrCancelled);
                }
                /* Erase the command from the input queue */
                iInputCommands.Erase(&iInputCommands.front());
            }
        }

        //finally, report cancel complete.
        CommandComplete(iInputCommands, aCmd, PVMFSuccess);
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFStreamingManagerNode::DoCancelAllCommands Out"));
}

bool PVMFStreamingManagerNode::CheckChildrenNodesCancelAll()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeCmdState != PVMFSM_NODE_CMD_CANCEL_COMPLETE)
        {
            return false;
        }
    }
    ResetNodeContainerCmdState();
    return true;
}

void PVMFStreamingManagerNode::CompleteCancelAll()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteCancelAll - In"));
    if (CheckChildrenNodesCancelAll())
    {
        if (iCancelCommand.front().iContext)
        {
            /*
             * CancelAllCommands is issued by upper layer
             */
            PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::CancelCommand() - CmdComplete - PVMFErrCancelled"));
            if (!iCurrentCommand.empty())
            {
                //not need to send commandcmp during GraphConstruct
                if (IsInternalCmd(iCurrentCommand.front().iCmd) == false)
                {
                    CommandComplete(iCurrentCommand.front(), PVMFErrCancelled);
                }
                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }

            /*
             * cancel all queued commands, if any
             */
            while (iInputCommands.size() > 0)
            {
                if (IsInternalCmd(iInputCommands.front().iCmd) == false)
                    CommandComplete(iInputCommands, iInputCommands.front(), PVMFErrCancelled);
            }
            /* finally send command complete for the cancel all command */
            if (iErrorDuringProcess == SM_NO_ERROR)
            {
                PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteCancelAll - CancelAllCommands complete"));
                CommandComplete(iCancelCommand,
                                iCancelCommand.front(),
                                PVMFSuccess);
            }
            else
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::CompleteCancelAll - CancelAllCommands complete error"));
                CommandComplete(iCancelCommand, iCancelCommand.front(), PVMFFailure, iErrorResponseInf);

                if (iErrorResponseInf != NULL)
                {
                    iErrorResponseInf->removeRef();
                    iErrorResponseInf = NULL;
                }
                iErrorDuringProcess = SM_NO_ERROR;
            }
        }
        else
        {
            /*
             * CancelAllCommands is issued due to error
             */
            if (iCurrentCommand.front().iCmd == PVMF_STREAMING_MANAGER_NODE_RESET)
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::CompleteCancelAll Shound not issue CancelAllCommands durint Reset"));
                OSCL_ASSERT(false);
            }
            PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::CompleteCancelAll() due to error - CmdComplete"));
            /* Erase the command from the cancel queue */
            iCancelCommand.Erase(&iCancelCommand.front());
            if (!iCurrentCommand.empty())
            {
                /*
                 * not need to send commandcmp for internal purpose
                 */
                if (IsInternalCmd(iCurrentCommand.front().iCmd) == false)
                {
                    CommandComplete(iCurrentCommand.front(), iCmdErrStatus, iErrorResponseInf, iEventData);
                }
                if (iErrorResponseInf != NULL)
                {
                    iErrorResponseInf->removeRef();
                    iErrorResponseInf = NULL;
                }
                iCmdErrStatus = PVMFFailure;
                iEventData = NULL;

                /* Erase the command from the current queue */
                iCurrentCommand.Erase(&iCurrentCommand.front());
            }

            /*
             * cancel all queued commands, if any
             */
            if (iErrorDuringProcess == SM_NODE_COMMAND_COMPLETION)
            {
                while (iInputCommands.size() > 0)
                {
                    if (IsInternalCmd(iInputCommands.front().iCmd) == false)
                    {
                        CommandComplete(iInputCommands.front(), PVMFErrCancelled);
                    }
                    /* Erase the command from the input queue */
                    iInputCommands.Erase(&iInputCommands.front());
                }
            }
            iErrorDuringProcess = SM_NO_ERROR;
        }
    }
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteCancelAll - Out"));
    return;
}

/**
//Called by the command handler AO to do the Cancel single command
*/
void PVMFStreamingManagerNode::DoCancelCommand(PVMFStreamingManagerNodeCommand& aCmd)
{
    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.PVMFStreamingManagerNodeCommandBase::Parse(id);

    //first check "current" command if any
    {
        PVMFStreamingManagerNodeCommand* cmd = iCurrentCommand.FindById(id);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iCurrentCommand, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            return;
        }
    }
    //next check input queue.
    {
        //start at element 1 since this cancel command is element 0.
        PVMFStreamingManagerNodeCommand* cmd = iInputCommands.FindById(id, 1);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iInputCommands, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
            return;
        }
    }
    //if we get here the command isn't queued so the cancel fails.
    CommandComplete(iInputCommands, aCmd, PVMFFailure);
}

/////////////////////////////////////////////////////
// Event reporting routines.
/////////////////////////////////////////////////////
void PVMFStreamingManagerNode::SetState(TPVMFNodeInterfaceState s)
{
    PVMF_SM_LOGINFO((0, "StreamingManagerNode:SetState %d", s));
    PVMFNodeInterface::SetState(s);
}

void PVMFStreamingManagerNode::ReportErrorEvent(PVMFEventType aEventType,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)
{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:NodeErrorEvent Type %d Data %d"
                           , aEventType, aEventData));

    if (aEventUUID && aEventCode)
    {
        PVMFBasicErrorInfoMessage* eventmsg =
            OSCL_NEW(PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL));
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

void PVMFStreamingManagerNode::ReportInfoEvent(PVMFEventType aEventType,
        OsclAny* aEventData,
        PVUuid* aEventUUID,
        int32* aEventCode)

{
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:NodeInfoEvent Type %d Data %d"
                           , aEventType, aEventData));

    if (aEventUUID && aEventCode)
    {
        PVMFBasicErrorInfoMessage* eventmsg =
            OSCL_NEW(PVMFBasicErrorInfoMessage, (*aEventCode, *aEventUUID, NULL));
        PVMFAsyncEvent asyncevent(PVMFInfoEvent,
                                  aEventType,
                                  NULL,
                                  OSCL_STATIC_CAST(PVInterface*, eventmsg),
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

void
PVMFStreamingManagerNode::HandlePortActivity(const PVMFPortActivity& aActivity)
{
    OSCL_UNUSED_ARG(aActivity);
    PVMF_SM_LOGSTACKTRACE((0, "StreamingManagerNode:HandlePortActivity - Not Implemented"));
}

/**
 * Active object implementation
 * The AO will either process one command or service one connected
 * port per call.  It will re-schedule itself and run continuously
 * until it runs out of things to do.
 */
void PVMFStreamingManagerNode::Run()
{
    /* Process commands */
    if (!iInputCommands.empty())
    {
        if (ProcessCommand(iInputCommands.front()))
        {
            /*
             * re-schedule if more commands to do
             * and node isn't reset.
             */
            if (!iInputCommands.empty() && iInterfaceState != EPVMFNodeCreated)
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
     * If we get here we did not process any commands.
     * Check for completion of a flush command...
     */
    if (FlushPending())
    {
        /*
         * Flush is complete.  Go to initialized state.
         */
        SetState(EPVMFNodeInitialized);
        CommandComplete(iCurrentCommand, iCurrentCommand.front(), PVMFSuccess);
        if (IsAdded())
        {
            RunIfNotReady();
        }
    }
}

void PVMFStreamingManagerNode::DoCancel()
{
    /* the base class cancel operation is sufficient */
    OsclActiveObject::DoCancel();
}

void
PVMFStreamingManagerNode::NodeCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode::NodeCommandCompleted"));

    bool oResponseOverRide = false;

    PVMFSMCommandContext *cmdContextData =
        OSCL_REINTERPRET_CAST(PVMFSMCommandContext*, aResponse.GetContext());

    PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:NodeCommandCompleted: %d", cmdContextData->cmd));

    if ((cmdContextData->cmd >=
            PVMF_STREAMING_MANAGER_SOCKET_NODE_COMMAND_START) &&
            (cmdContextData->cmd <
             PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_COMMAND_START))

    {
        HandleSocketNodeCommandCompleted(aResponse);
    }
    else if ((cmdContextData->cmd >=
              PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_COMMAND_START) &&
             (cmdContextData->cmd <
              PVMF_STREAMING_MANAGER_JITTER_BUFFER_CONTROLLER_COMMAND_START))

    {
        HandleRTSPSessionControllerCommandCompleted(aResponse, oResponseOverRide);
    }
    else if ((cmdContextData->cmd >=
              PVMF_STREAMING_MANAGER_JITTER_BUFFER_CONTROLLER_COMMAND_START) &&
             (cmdContextData->cmd <
              PVMF_STREAMING_MANAGER_MEDIA_LAYER_COMMAND_START))

    {
        HandleJitterBufferCommandCompleted(aResponse);
    }
    else if ((cmdContextData->cmd >=
              PVMF_STREAMING_MANAGER_MEDIA_LAYER_COMMAND_START) &&
             (cmdContextData->cmd <
              PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_COMMAND_START))

    {
        HandleMediaLayerCommandCompleted(aResponse);
    }
    else if (cmdContextData->cmd >=
             PVMF_STREAMING_MANAGER_RTPPACKETSOURCE_NODE_COMMAND_START)
    {
    }

    if ((aResponse.GetCmdStatus() != PVMFSuccess) &&
            (aResponse.GetCmdStatus() != PVMFErrCancelled) &&
            (oResponseOverRide == false))
    {
        if (iCancelCommand.size() > 0)
        {
            if (cmdContextData->parentCmd == PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS)
            {
                /*
                 * During CancelAllComands, error happen from child node.
                 * Then we save this err and after all current cmd is canceled in child node, we send this err code.
                 * If CancelAllCommands is issued by internal error handling, iErrorDuringProcess should be true.
                 */
                if (iErrorDuringProcess == SM_NO_ERROR)
                {
                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode:NodeCommandCompleted: - Reset Error during CancelAll"));
                    if (iErrorResponseInf != NULL)
                    {
                        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::NodeCommandCompleted - iErrorResponseInf is not NULL"));
                        OSCL_ASSERT(false);
                    }
                    /*
                     * Only first error msg from cancelall completion in child node is stored.
                     */
                    if (aResponse.GetEventExtensionInterface())
                    {
                        iErrorResponseInf = aResponse.GetEventExtensionInterface();
                        iErrorResponseInf->addRef();
                    }
                    iErrorDuringProcess = SM_NODE_COMMAND_COMPLETION;
                }
                CompleteCancelAll();
            }
            /*
             * If Error is recieved for Current Cmd during cancelling, we just ignore.
             */
        }
        else if (iCurrentCommand.size() > 0)
        {
            /*
             * If error is recieved while current cmd is ongoing, we save this error code and issue CancelAllCommands.
             * After all current cmd is canceled in child node,
             * we send this error code if current cmd is not internal use.
             */
            if (iErrorDuringProcess == SM_NO_ERROR)
            {
                if (iErrorResponseInf != NULL)
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::NodeCommandCompleted - iErrorResponseInf is not NULL"));
                    OSCL_ASSERT(false);
                }
                if (aResponse.GetEventExtensionInterface())
                {
                    iErrorResponseInf = aResponse.GetEventExtensionInterface();
                    iErrorResponseInf->addRef();
                }
                iCmdErrStatus = aResponse.GetCmdStatus();
                iEventData = aResponse.GetEventData();
                iErrorDuringProcess = SM_NODE_COMMAND_COMPLETION;

                /*
                 * We don't interupt any Reset cmd for child node.
                 * After all Reset cmd is completed in child node, we send this error.
                 */
                if (cmdContextData->parentCmd != PVMF_STREAMING_MANAGER_NODE_RESET)
                {
                    PVMFSessionId s = 0;
                    PVMFStreamingManagerNodeCommand cmdCancelPendingCmd;
                    cmdCancelPendingCmd.PVMFStreamingManagerNodeCommandBase::Construct(s,
                            PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS,
                            NULL);
                    QueueCommandL(cmdCancelPendingCmd);
                    PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:NodeCommandCompleted: - CancelAllCommands Queued"));

                    ResetNodeContainerCmdState();
                }
            }
            else
            {
                PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:NodeCommandCompleted: - CancelAllCommands already Queued"));
            }
        }
    }
    return;
}

void
PVMFStreamingManagerNode::HandleSocketNodeCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVMFSMNodeContainer* iSocketNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
    if (iSocketNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVMFSMCommandContext *cmdContextData =
        OSCL_REINTERPRET_CAST(PVMFSMCommandContext*, aResponse.GetContext());

    if (iSocketNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_PENDING)
    {
        iSocketNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_COMPLETE;
    }
    else if (iSocketNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_CANCEL_PENDING)
    {
        if (cmdContextData->parentCmd == PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS)
            iSocketNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_CANCEL_COMPLETE;
    }

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::HandleSocketNodeCommandCompleted - Command failed - context=0x%x, status=0x%x", aResponse.GetContext(), aResponse.GetCmdStatus()));
        if (IsBusy())
        {
            Cancel();
            RunIfNotReady();
        }
        return;
    }
    cmdContextData->oFree = true;

    switch (cmdContextData->cmd)
    {
        case PVMF_STREAMING_MANAGER_SOCKET_NODE_QUERY_UUID:
            CompleteQueryUuid();
            break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_QUERY_INTERFACE:
        {
            if (iExtensionInterfacePlaceholder == NULL) OSCL_LEAVE(OsclErrBadHandle);
            iSocketNodeContainer->iExtensions.push_back(iExtensionInterfacePlaceholder);
            CompleteQueryInterface();
        }
        break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_INIT:
        {
            {
                CompleteInit();
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_PREPARE:
        {
            {
                CompletePrepare();
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_START:
        {
            {
                {
                    CompleteStart();
                }
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_STOP:
        {
            CompleteStop();
        }
        break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_FLUSH:
            CompleteFlush();
            break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_PAUSE:
        {
            {
                CompletePause();
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_RESET:
            CompleteReset();
            break;

        case PVMF_STREAMING_MANAGER_SOCKET_NODE_REQUEST_PORT:
        {
            PVMFPortInterface* port =
                (PVMFPortInterface*)aResponse.GetEventData();
            {
                /*
                 * Save the port in TrackInfo
                 */
                PVMFSMTrackInfo* trackInfo =
                    FindTrackInfo(cmdContextData->portContext.trackID);

                if (cmdContextData->portContext.portTag ==
                        PVMF_SOCKET_NODE_PORT_TYPE_SOURCE)
                {
                    trackInfo->iNetworkNodePort = port;
                    iSocketNodeContainer->iOutputPorts.push_back(port);
                }
                else if (cmdContextData->portContext.portTag ==
                         PVMF_SOCKET_NODE_PORT_TYPE_SINK)
                {
                    trackInfo->iNetworkNodeRTCPPort = port;
                    iSocketNodeContainer->iInputPorts.push_back(port);
                }
                CompleteGraphConstruct();
            }
        }
        break;
        case PVMF_STREAMING_MANAGER_SOCKET_NODE_CANCEL_ALL_COMMANDS:
        {
            CompleteCancelAll();
        }
        break;

        default:
            break;
    }
    return;
}

void
PVMFStreamingManagerNode::HandleRTSPSessionControllerCommandCompleted(const PVMFCmdResp& aResponse,
        bool& aResponseOverRide)
{
    aResponseOverRide = false;

    PVMFSMNodeContainer* iSessionControllerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
    if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVMFSMCommandContext *cmdContextData =
        OSCL_REINTERPRET_CAST(PVMFSMCommandContext*, aResponse.GetContext());

    if (iSessionControllerNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_PENDING)
    {
        iSessionControllerNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_COMPLETE;
    }
    else if (iSessionControllerNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_CANCEL_PENDING)
    {
        if (cmdContextData->parentCmd == PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS)
            iSessionControllerNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_CANCEL_COMPLETE;
    }

    cmdContextData->oFree = true;

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        if (cmdContextData->cmd == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_PAUSE)
        {
            /*
             * Check if it is a pause failure - suppress pause failures if they
             * happen after a session is complete
             */
            PVMFSMNodeContainer* iJitterBufferNodeContainer =
                getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
            if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
            PVMFJitterBufferExtensionInterface* jbExtIntf =
                (PVMFJitterBufferExtensionInterface*)
                (iJitterBufferNodeContainer->iExtensions[0]);
            bool oSessionExpired = false;
            jbExtIntf->HasSessionDurationExpired(oSessionExpired);
            if (oSessionExpired == true)
            {
                aResponseOverRide = true;
            }
        }
        /* if a failure has been overridden just fall thru */
        if (aResponseOverRide == false)
        {
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::HandleRTSPSessionControllerCommandCompleted - Command failed - context=0x%x, status=0x%x", aResponse.GetContext(), aResponse.GetCmdStatus()));
            if (IsBusy())
            {
                Cancel();
                RunIfNotReady();
            }
            return;
        }
    }

    switch (cmdContextData->cmd)
    {
        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_QUERY_UUID:
            CompleteQueryUuid();
            break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_QUERY_INTERFACE:
        {
            if (iExtensionInterfacePlaceholder == NULL) OSCL_LEAVE(OsclErrBadHandle);
            iSessionControllerNodeContainer->iExtensions.push_back(iExtensionInterfacePlaceholder);
            CompleteQueryInterface();
        }
        break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_INIT:
            CompleteInit();
            break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_PREPARE:
        {
            if (iSessionSourceInfo->iRTSPTunnelling == false)
            {
                /* Complete set up of feedback channels */
                CompleteFeedBackPortsSetup();
            }
            /*
             * Send start complete params to child nodes
             * viz. SSRC etc
             */
            SendSessionControlPrepareCompleteParams();
            CompletePrepare();
        }
        break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_START:
        {
            {
                /*
                 * Send start complete params to child nodes
                 * viz. actual play range, rtp info params etc
                 */
                SendSessionControlStartCompleteParams();
                CompleteStart();
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_STOP:
        {
            CompleteStop();
        }
        break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_FLUSH:
            CompleteFlush();
            break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_PAUSE:
        {
            {
                CompletePause();
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_RESET:
            CompleteReset();
            break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_REQUEST_PORT:
        {
            /*
             * Save the port in TrackInfo
             */
            PVMFSMTrackInfo* trackInfo =
                FindTrackInfo(cmdContextData->portContext.trackID);

            PVMFPortInterface* port =
                (PVMFPortInterface*)aResponse.GetEventData();

            if (cmdContextData->portContext.portTag ==
                    PVMF_RTSP_NODE_PORT_TYPE_OUTPUT)
            {
                trackInfo->iSessionControllerOutputPort = port;
                iSessionControllerNodeContainer->iOutputPorts.push_back(port);

            }
            else if (cmdContextData->portContext.portTag ==
                     PVMF_RTSP_NODE_PORT_TYPE_INPUT)
            {
                iSessionControllerNodeContainer->iInputPorts.push_back(port);
            }
            else if (cmdContextData->portContext.portTag ==
                     PVMF_RTSP_NODE_PORT_TYPE_INPUT_OUTPUT)
            {
                trackInfo->iSessionControllerFeedbackPort = port;
                iSessionControllerNodeContainer->iFeedBackPorts.push_back(port);
            }
            CompleteGraphConstruct();
        }
        break;

        case PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_CANCEL_ALL_COMMANDS:
        {
            CompleteCancelAll();
        }
        break;

        default:
            break;
    }
    return;
}

void
PVMFStreamingManagerNode::HandleJitterBufferCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVMFSMNodeContainer* iJitterBufferNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
    if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVMFSMCommandContext *cmdContextData =
        OSCL_REINTERPRET_CAST(PVMFSMCommandContext*, aResponse.GetContext());

    if (iJitterBufferNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_PENDING)
    {
        iJitterBufferNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_COMPLETE;
    }
    else if (iJitterBufferNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_CANCEL_PENDING)
    {
        if (cmdContextData->parentCmd == PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS)
            iJitterBufferNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_CANCEL_COMPLETE;
    }

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::HandleJitterBufferCommandCompleted - Command failed - context=0x%x, status=0x%x", aResponse.GetContext(), aResponse.GetCmdStatus()));
        if (IsBusy())
        {
            Cancel();
            RunIfNotReady();
        }
        return;
    }

    cmdContextData->oFree = true;

    switch (cmdContextData->cmd)
    {
        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_QUERY_UUID:
            CompleteQueryUuid();
            break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_QUERY_INTERFACE:
        {
            if (iExtensionInterfacePlaceholder == NULL) OSCL_LEAVE(OsclErrBadHandle);
            iJitterBufferNodeContainer->iExtensions.push_back(iExtensionInterfacePlaceholder);
            CompleteQueryInterface();
        }
        break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_INIT:
        {
            PVMFJitterBufferExtensionInterface* jbExtIntf =
                (PVMFJitterBufferExtensionInterface*)
                (iJitterBufferNodeContainer->iExtensions[0]);
            if (jbExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);
            if (IsRTPPacketSourcePresent())
            {
                jbExtIntf->SetBroadCastSession();
            }
            bool disableFireWallPackets = true;
            if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
                    (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
            {
                if (iSessionSourceInfo->iRTSPTunnelling == false)
                {
                    //do not disable fw pkts run time
                    //apps can still disable it compile time or using KVP
                    //for UDP sessions ofcourse
                    disableFireWallPackets = false;
                }
            }
            if (disableFireWallPackets == true)
            {
                jbExtIntf->DisableFireWallPackets();
            }
            CompleteInit();
        }
        break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_PREPARE:
            CompletePrepare();
            break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_START:
        {
            /* If start has been cancelled wait for cancel success */
            if (aResponse.GetCmdStatus() != PVMFErrCancelled)
            {
                CompleteStart();
            }
        }
        break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_STOP:
        {
            CompleteStop();
        }
        break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_FLUSH:
            CompleteFlush();
            break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_PAUSE:
            CompletePause();
            break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_RESET:
            CompleteReset();
            break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_REQUEST_PORT:
        {
            PVMFJitterBufferExtensionInterface* jbExtIntf =
                (PVMFJitterBufferExtensionInterface*)
                (iJitterBufferNodeContainer->iExtensions[0]);

            if (jbExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);

            /*
             * Save the port in TrackInfo
             */
            PVMFSMTrackInfo* trackInfo =
                FindTrackInfo(cmdContextData->portContext.trackID);

            PVMFPortInterface* port =
                (PVMFPortInterface*)aResponse.GetEventData();

            uint32 bitrate = 0;

            if (cmdContextData->portContext.portTag ==
                    PVMF_JITTER_BUFFER_PORT_TYPE_INPUT)
            {
                bitrate = trackInfo->bitRate;
                trackInfo->iJitterBufferInputPort = port;
                iJitterBufferNodeContainer->iInputPorts.push_back(port);
            }
            else if (cmdContextData->portContext.portTag ==
                     PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT)
            {
                trackInfo->iJitterBufferOutputPort = port;
                iJitterBufferNodeContainer->iOutputPorts.push_back(port);
            }
            else if (cmdContextData->portContext.portTag ==
                     PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK)
            {
                trackInfo->iJitterBufferRTCPPort = port;
                iJitterBufferNodeContainer->iFeedBackPorts.push_back(port);

                //if RTP packet source is present, it implies a DVBH session
                //and we disable RTCP RRs for DVB sessions
                if (IsRTPPacketSourcePresent())
                {
                    //The packet source does not process RTCP reader reports.
                    //So, disable them by setting the RR bandwidth to zero.
                    //The RS bandwidth is set to the RFC default of 800 bps but
                    //actual bandwidth is allocated by the fraction RS/(RR+RS), so
                    //the number used for RS in this case is arbitrary because
                    //sender reports will get 100% of the RTCP bandwidth as RS/RS = 100%.
                    jbExtIntf->setPortRTCPParams(port,
                                                 iTrackInfoVec.size(),
                                                 0,
                                                 800);
                }
                else
                {
                    if (trackInfo->iRTCPBwSpecified)
                    {
                        jbExtIntf->setPortRTCPParams(port, iTrackInfoVec.size(), trackInfo->iRR, trackInfo->iRS);
                    }
                }
            }
            jbExtIntf->setPortParams(port,
                                     trackInfo->trackTimeScale,
                                     bitrate,
                                     trackInfo->iTrackConfig,
                                     trackInfo->iRateAdaptation,
                                     trackInfo->iRateAdaptationFeedBackFrequency);
            CompleteGraphConstruct();
        }
        break;

        case PVMF_STREAMING_MANAGER_JITTER_BUFFER_CANCEL_ALL_COMMANDS:
        {
            CompleteCancelAll();
        }
        break;

        default:
            break;
    }
    return;
}

void
PVMFStreamingManagerNode::HandleMediaLayerCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVMFSMNodeContainer* iMediaLayerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVMFSMCommandContext *cmdContextData =
        OSCL_REINTERPRET_CAST(PVMFSMCommandContext*, aResponse.GetContext());

    if (iMediaLayerNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_PENDING)
    {
        iMediaLayerNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_COMPLETE;
    }
    else if (iMediaLayerNodeContainer->iNodeCmdState == PVMFSM_NODE_CMD_CANCEL_PENDING)
    {
        if (cmdContextData->parentCmd == PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS)
            iMediaLayerNodeContainer->iNodeCmdState = PVMFSM_NODE_CMD_CANCEL_COMPLETE;
    }

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::HandleMediaLayerCommandCompleted - Command failed - context=0x%x, status=0x%x", aResponse.GetContext(), aResponse.GetCmdStatus()));
        if (IsBusy())
        {
            Cancel();
            RunIfNotReady();
        }
        return;
    }

    cmdContextData->oFree = true;

    switch (cmdContextData->cmd)
    {
        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_QUERY_UUID:
            CompleteQueryUuid();
            break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_QUERY_INTERFACE:
        {
            if (iExtensionInterfacePlaceholder == NULL) OSCL_LEAVE(OsclErrBadHandle);
            iMediaLayerNodeContainer->iExtensions.push_back(iExtensionInterfacePlaceholder);
            CompleteQueryInterface();
        }
        break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_INIT:
            CompleteInit();
            break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_PREPARE:
            CompletePrepare();
            break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_START:
        {
            CompleteStart();
        }
        break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_STOP:
        {
            CompleteStop();
        }
        break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_FLUSH:
            CompleteFlush();
            break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_PAUSE:
            CompletePause();
            break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_RESET:
            CompleteReset();
            break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_REQUEST_PORT:
        {
            PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
                (PVMFMediaLayerNodeExtensionInterface*)
                (iMediaLayerNodeContainer->iExtensions[0]);

            if (mlExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);

            /*
             * Save the port in TrackInfo
             */
            PVMFSMTrackInfo* trackInfo =
                FindTrackInfo(cmdContextData->portContext.trackID);

            PVMFPortInterface* port =
                (PVMFPortInterface*)aResponse.GetEventData();

            if (cmdContextData->portContext.portTag ==
                    PVMF_MEDIALAYER_PORT_TYPE_INPUT)
            {
                trackInfo->iMediaLayerInputPort = port;
                iMediaLayerNodeContainer->iInputPorts.push_back(port);
            }
            else if (cmdContextData->portContext.portTag ==
                     PVMF_MEDIALAYER_PORT_TYPE_OUTPUT)
            {
                trackInfo->iMediaLayerOutputPort = port;
                iMediaLayerNodeContainer->iOutputPorts.push_back(port);
            }
            mediaInfo* mInfo = NULL;
            if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
                    (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
                    (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
            {
                SDPInfo* sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();
                if (sdpInfo == NULL) OSCL_LEAVE(OsclErrBadHandle);
                mInfo = sdpInfo->getMediaInfoBasedOnID(trackInfo->trackID);
            }
            mlExtIntf->setPortMediaParams(port, trackInfo->iTrackConfig, mInfo);
            CompleteGraphConstruct();
        }
        break;

        case PVMF_STREAMING_MANAGER_MEDIA_LAYER_CANCEL_ALL_COMMANDS:
        {
            CompleteCancelAll();
        }
        break;

        default:
            break;
    }
    return;
}

bool PVMFStreamingManagerNode::IsFatalErrorEvent(const PVMFEventType& event)
{
    bool retval = false;
    switch (event)
    {
        case PVMFErrCorrupt:
        case PVMFErrOverflow:
        case PVMFErrResource:
        case PVMFErrProcessing:
        case PVMFErrUnderflow:
        case PVMFErrNoResources:
        case PVMFErrResourceConfiguration:
        case PVMFErrTimeout:
        case PVMFErrNoMemory:
        case PVMFFailure:
            retval = true;
            break;
        default:
            retval = false;
    }
    return retval;
}

void
PVMFStreamingManagerNode::HandleNodeErrorEvent(const PVMFAsyncEvent& aEvent)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::HandleNodeErrorEvent - In iCancelCommand.empty() [%d] current command id[%d] event type [%d]", iCancelCommand.empty(), iCurrentCommand.front().iCmd, aEvent.GetEventType()));

    //Check if fatal error occurred, then we may need to enque the cancelallcommand due to error
    if (IsFatalErrorEvent(aEvent.GetEventType())
            && !iCurrentCommand.empty()
            && iCancelCommand.empty())
    {
        if ((IsInternalCmd(iCurrentCommand.front().iCmd) == true))
        {
            /*
             * cancel all queued commands, if any
             */
            while (iInputCommands.size() > 0)
            {
                if (IsInternalCmd(iInputCommands.front().iCmd) == false)
                {
                    CommandComplete(iInputCommands.front(), PVMFErrCancelled);
                }
                /* Erase the command from the input queue */
                iInputCommands.Erase(&iInputCommands.front());
            }

            /*
             * After the completion of the cancel due to error, we will complete the current command in which failure occurred.
             */
            iErrorDuringProcess = SM_ERROR_EVENT;
            PVMFSessionId s = 0;
            PVMFStreamingManagerNodeCommand cmdCancelPendingCmd;
            cmdCancelPendingCmd.PVMFStreamingManagerNodeCommandBase::Construct(s,
                    PVMF_STREAMING_MANAGER_NODE_CANCELALLCOMMANDS,
                    NULL);
            QueueCommandL(cmdCancelPendingCmd);
            PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode:HandleNodeErrorEvent: - CancelAllCommands Queued"));
        }
    }
    //just pass the error event up
    PVMFAsyncEvent event = OSCL_CONST_CAST(PVMFAsyncEvent, aEvent);
    PVMFNodeInterface::ReportErrorEvent(event);

    return;
}

void
PVMFStreamingManagerNode::HandleNodeInformationalEvent(const PVMFAsyncEvent& aEvent)
{
    PVMFAsyncEvent event = OSCL_CONST_CAST(PVMFAsyncEvent, aEvent);
    PVMFEventType infoEvent = aEvent.GetEventType();
    if (infoEvent == PVMFInfoEndOfData)
    {
        /* Notify jitter buffer */
        PVMFSMNodeContainer* iJitterBufferNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
        if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFJitterBufferExtensionInterface* jbExtIntf =
            (PVMFJitterBufferExtensionInterface*)
            (iJitterBufferNodeContainer->iExtensions[0]);
        jbExtIntf->NotifyOutOfBandEOS();
    }
    else
    {
        /* Just pass the info event up */
        PVMFNodeInterface::ReportInfoEvent(event);
    }
}

PVMFSMCommandContext* PVMFStreamingManagerNode::RequestNewInternalCmd()
{
    int32 i = 0;
    /* Search for the next free node command in the pool */
    while (i < PVMF_STREAMING_MANAGER_INTERNAL_CMDQ_SIZE)
    {
        if (iInternalCmdPool[i].oFree)
        {
            iInternalCmdPool[i].oFree = false;
            return &(iInternalCmdPool[i]);
        }
        ++i;
    }
    /* Free one not found so return NULL */
    return NULL;
}

void
PVMFStreamingManagerNode::MoveCmdToCurrentQueue(PVMFStreamingManagerNodeCommand& aCmd)
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

void
PVMFStreamingManagerNode::MoveCmdToCancelQueue(PVMFStreamingManagerNodeCommand& aCmd)
{
    /*
     * note: the StoreL cannot fail since the queue is never more than 1 deep
     * and we reserved space.
     */
    iCancelCommand.StoreL(aCmd);
    iInputCommands.Erase(&aCmd);
    return;
}

PVMFSMNodeContainer*
PVMFStreamingManagerNode::getNodeContainer(int32 tag)
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeTag == tag)
        {
            return (&(iNodeContainerVec[i]));
        }
    }
    return NULL;
}

bool PVMFStreamingManagerNode::PopulateTrackInfoVec()
{
    if (iSelectedMediaPresetationInfo.getNumTracks() == 0)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:PopulateTrackInfoVec - Selected Track List Empty"));
        return false;
    }

    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        /*
         * Get selected tracks
         */
        SDPInfo* sdpInfo = iSessionSourceInfo->_sdpInfo;

        int32 numTracks = sdpInfo->getNumMediaObjects();

        if (numTracks > 0)
        {
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
                        return false;
                    }

                    if (mInfo->getSelect())
                    {
                        PVMFSMTrackInfo trackInfo;

                        {
                            trackInfo.iTransportType += _STRLIT_CHAR("RTP");
                        }

                        trackInfo.trackID = mInfo->getMediaInfoID();

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
                        OSCL_StackString<32> h263(_STRLIT_CHAR("H263"));
                        {
                            trackInfo.iMimeType += mimeType;
                        }

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

                            uint8* my_ptr = NULL;
                            int32 errcode = 0;
                            OSCL_TRY(errcode, my_ptr = (uint8*) my_alloc.ALLOCATE(aligned_refcnt_size + configSize));

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
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:PopulateTrackInfoVec - Selected Track List Empty"));
            return false;
        }
    }
    return true;
}

PVMFSMTrackInfo*
PVMFStreamingManagerNode::FindTrackInfo(uint32 trackID)
{
    PVMFSMTrackInfoVector::iterator it;

    for (it = iTrackInfoVec.begin();
            it != iTrackInfoVec.end();
            it++)
    {
        if (it->trackID == trackID)
        {
            return (it);
        }
    }
    return NULL;
}

PVMFSMTrackInfo*
PVMFStreamingManagerNode::FindTrackInfo(PvmfMimeString* trackMimeType)
{
    PVMFSMTrackInfoVector::iterator it;

    for (it = iTrackInfoVec.begin();
            it != iTrackInfoVec.end();
            it++)
    {
        if (!(oscl_CIstrcmp(it->iMimeType.get_cstr(),
                            trackMimeType->get_cstr())))
        {
            return (it);
        }
    }
    return NULL;
}

bool PVMFStreamingManagerNode::ReserveSockets()
{
    uint32 sockid = 0;
    char portConfigBuf[64];
    oscl_memset((OsclAny*)portConfigBuf, 0, 64);
    oscl_snprintf(portConfigBuf, 64, "%d", sockid);
    OSCL_StackString<128> portConfig("UDP");
    portConfig += _STRLIT_CHAR("/remote_address=0.0.0.0");
    portConfig += _STRLIT_CHAR(";client_port=");
    portConfig += portConfigBuf;

    PVMFSMNodeContainer* iSocketNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
    if (iSocketNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFSocketNode* socketNode =
        (PVMFSocketNode*)(iSocketNodeContainer->iNode);
    PVMFSMTrackInfoVector::iterator it;
    uint32 startPortNum = 0;
    {
        TimeValue current_time;
        current_time.set_to_current_time();
        uint32 my_seed = current_time.get_sec();

        OsclRand random_num;
        random_num.Seed(my_seed);
        int32 first = random_num.Rand();
        uint32 myport = (first & 0x1FFF) + 0x2000;	//start from 8192
        startPortNum = (myport >> 1) << 1;	//start from even
    }

    for (it = iTrackInfoVec.begin();
            it != iTrackInfoVec.end();
            it++)
    {
        OSCL_StackString<128> portConfigWithMime;
        portConfigWithMime += portConfig;
        portConfigWithMime += _STRLIT_CHAR(";mime=");
        portConfigWithMime += it->iMimeType;

        PVMFStatus status = socketNode->AllocateConsecutivePorts(&portConfigWithMime,
                            it->iRTPSocketID,
                            it->iRTCPSocketID, startPortNum);
        if (status != PVMFSuccess)
        {
            return false;
        }
    }
    return true;
}

bool PVMFStreamingManagerNode::RequestNetworkNodePorts(int32 portTag,
        uint32& numPortsRequested)
{
    numPortsRequested = 0;

    PVMFSMNodeContainer* nodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);

    if (nodeContainer == NULL)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestNetworkNodePorts - getNodeContainer Failed"));
        return false;
    }

    for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
    {
        PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
        if (internalCmd != NULL)
        {
            internalCmd->cmd =
                nodeContainer->commandStartOffset +
                PVMF_STREAMING_MANAGER_NODE_INTERNAL_REQUEST_PORT_OFFSET;
            internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION;
            internalCmd->portContext.trackID = trackInfo.trackID;
            internalCmd->portContext.portTag = portTag;

            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

            PVMFNodeInterface* iNode = nodeContainer->iNode;

            uint32 sockid = 0;
            bool oRTCP = false;

            if (portTag == PVMF_SOCKET_NODE_PORT_TYPE_SOURCE)
            {
                sockid = trackInfo.iRTPSocketID;
            }
            else if (portTag == PVMF_SOCKET_NODE_PORT_TYPE_SINK)
            {
                sockid = trackInfo.iRTCPSocketID;
                oRTCP = true;
            }

            char portConfigBuf[64];
            oscl_memset((OsclAny*)portConfigBuf, 0, 64);
            oscl_snprintf(portConfigBuf, 64, "%d", sockid);
            OSCL_StackString<128> portConfig("UDP");
            portConfig += _STRLIT_CHAR("/remote_address=0.0.0.0");
            portConfig += _STRLIT_CHAR(";client_port=");
            portConfig += portConfigBuf;
            portConfig += _STRLIT_CHAR(";mime=");
            portConfig += trackInfo.iMimeType.get_cstr();
            if (oRTCP == true)
            {
                portConfig += _STRLIT_CHAR("/rtp");
            }
            else
            {
                portConfig += _STRLIT_CHAR("/rtcp");
            }

            iNode->RequestPort(nodeContainer->iSessionId,
                               internalCmd->portContext.portTag,
                               &portConfig,
                               cmdContextData);
            numPortsRequested++;
        }
        else
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestNetworkNodePorts - RequestNewInternalCmd Failed"));
            return false;
        }
    }
    return true;
}

bool PVMFStreamingManagerNode::RequestRTSPNodePorts(int32 portTag,
        uint32& numPortsRequested)
{
    numPortsRequested = 0;

    PVMFSMNodeContainer* nodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);

    if (nodeContainer == NULL)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestNetworkNodePorts - getNodeContainer Failed"));
        return false;
    }

    for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
    {
        PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
        if (internalCmd != NULL)
        {
            internalCmd->cmd =
                nodeContainer->commandStartOffset +
                PVMF_STREAMING_MANAGER_NODE_INTERNAL_REQUEST_PORT_OFFSET;
            internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION;
            internalCmd->portContext.trackID = trackInfo.trackID;
            internalCmd->portContext.portTag = portTag;

            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

            PVMFNodeInterface* iNode = nodeContainer->iNode;

            char portConfigBuf[64];
            oscl_memset((OsclAny*)portConfigBuf, 0, 64);
            oscl_snprintf(portConfigBuf, 64, "sdpTrackIndex=%d", trackInfo.trackID);
            OSCL_StackString<128> portConfig;
            portConfig += portConfigBuf;

            if (portTag == PVMF_RTSP_NODE_PORT_TYPE_OUTPUT)
            {
                portConfig += _STRLIT_CHAR("/media");
            }
            if (portTag == PVMF_RTSP_NODE_PORT_TYPE_INPUT_OUTPUT)
            {
                portConfig += _STRLIT_CHAR("/feedback");
            }

            iNode->RequestPort(nodeContainer->iSessionId,
                               internalCmd->portContext.portTag,
                               &portConfig,
                               cmdContextData);
            numPortsRequested++;
        }
        else
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestNetworkNodePorts - RequestNewInternalCmd Failed"));
            return false;
        }
    }
    return true;
}

bool
PVMFStreamingManagerNode::RequestJitterBufferPorts(int32 portType,
        uint32 &numPortsRequested)
{
    PVMFSMNodeContainer* nodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);

    if (nodeContainer == NULL)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestJitterBufferPorts - getNodeContainer Failed"));
        return false;
    }

    numPortsRequested = 0;
    /*
     * Request port - all jitter buffer input ports
     * are even numbered and output and rtcp ports are odd numbered
     */
    int32 portTagStart = portType;

    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    nodeContainer->commandStartOffset +
                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_REQUEST_PORT_OFFSET;
                internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION;
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
            }
            else
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestJitterBufferPorts - RequestNewInternalCmd Failed"));
                return false;
            }
            portTagStart += 3;
        }
        return true;
    }
    // error
    return false;
}

bool
PVMFStreamingManagerNode::RequestMediaLayerPorts(int32 portType,
        uint32& numPortsRequested)
{
    PVMFSMNodeContainer* nodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);

    if (nodeContainer == NULL)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestMediaLayerPorts - getNodeContainer Failed"));
        return false;
    }

    numPortsRequested = 0;
    /*
     * Request port - all media layer input ports
     * are even numbered and output are odd numbered
     */
    int32 portTagStart = portType;

    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    nodeContainer->commandStartOffset +
                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_REQUEST_PORT_OFFSET;
                internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION;
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
            }
            else
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:RequestMediaLayerPorts - RequestNewInternalCmd Failed"));
                return false;
            }
            portTagStart += 2;
        }

        return true;
    }
    //error
    return false;
}

bool PVMFStreamingManagerNode::ConstructGraphFor3GPPUDPStreaming()
{
    uint32 numPortsRequested = 0;

    /*
     * Reserve UDP socket pairs
     */
    if (!ReserveSockets())
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - ReserveSockets() Failed"));
        return false;
    }

    /*
     * Request data UDP ports
     */
    if (!RequestNetworkNodePorts(PVMF_SOCKET_NODE_PORT_TYPE_SOURCE,
                                 numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestNetworkNodePorts(PVMF_SOCKET_NODE_PORT_TYPE_SOURCE) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    /*
     * Request feedback (RTCP) UDP ports
     */
    if (!RequestNetworkNodePorts(PVMF_SOCKET_NODE_PORT_TYPE_SINK,
                                 numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestNetworkNodePorts(PVMF_SOCKET_NODE_PORT_TYPE_SINK) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_INPUT,
                                  numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_INPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT,
                                  numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK,
                                  numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_INPUT,
                                numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_INPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_OUTPUT,
                                numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPUDPStreaming - RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_OUTPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    return true;
}

bool PVMFStreamingManagerNode::ConstructGraphFor3GPPTCPStreaming()
{
    uint32 numPortsRequested = 0;

    /*
     * Request media ports
     */
    if (!RequestRTSPNodePorts(PVMF_RTSP_NODE_PORT_TYPE_OUTPUT,
                              numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestRTSPNodePorts(PVMF_RTSP_NODE_PORT_TYPE_OUTPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    /*
     * Request feedback (RTCP) ports
     */
    if (!RequestRTSPNodePorts(PVMF_RTSP_NODE_PORT_TYPE_INPUT_OUTPUT,
                              numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestRTSPNodePorts(PVMF_RTSP_NODE_PORT_TYPE_INPUT_OUTPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_INPUT,
                                  numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_INPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT,
                                  numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK,
                                  numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestJitterBufferPorts(PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_INPUT,
                                numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_INPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    if (!RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_OUTPUT,
                                numPortsRequested))
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConstructGraphFor3GPPTCPStreaming - RequestMediaLayerPorts(PVMF_MEDIALAYER_PORT_TYPE_OUTPUT) Failed"));
        return false;
    }
    iNumRequestPortsPending += numPortsRequested;

    return true;
}


bool
PVMFStreamingManagerNode::GraphConstruct()
{
    /*
     * Session source info must have been set
     */
    if (iSessionSourceInfo->_sessionType == PVMF_FORMAT_UNKNOWN)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:GraphConstruct - Invalid Session Type"));
        return false;
    }

    if (!PopulateTrackInfoVec())
    {
        return false;
    }

    if (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE)
    {
    }
    else if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
             (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        if (iSessionSourceInfo->iRTSPTunnelling == true)
        {
            return (ConstructGraphFor3GPPTCPStreaming());
        }
        else
        {
            return (ConstructGraphFor3GPPUDPStreaming());
        }
    }
    //error
    return false;
}

/*
 * Called by the call back routine whenever a "RequestPort" call
 * completes successfully.
 */
void PVMFStreamingManagerNode::CompleteGraphConstruct()
{
    iTotalNumRequestPortsComplete++;
    /*
     * Once all port requests are complete, connect the graph
     */
    if (iTotalNumRequestPortsComplete == iNumRequestPortsPending)
    {
        PVMFStreamingManagerNodeCommand aCmd = iCurrentCommand.front();
        if (!SendSessionSourceInfoToSessionController())
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:CompleteGraphConstruct - SendSessionSourceInfoToSessionController Failed"));
            InternalCommandComplete(aCmd, PVMFFailure);
            iCurrentCommand.Erase(&iCurrentCommand.front());
        }
        else
        {
            oGraphConstructComplete = true;
            InternalCommandComplete(aCmd, PVMFSuccess);
            /* Erase the command from the current queue */
            iCurrentCommand.Erase(&iCurrentCommand.front());
        }
    }
}

PVMFStatus
PVMFStreamingManagerNode::ConnectPortPairs(PVMFPortInterface* aPort1,
        PVMFPortInterface* aPort2)
{
    PVMFStatus status;

    status = aPort1->Connect(aPort2);

    if (status != PVMFSuccess)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:ConnectPortPairs - Connect Failed"));
        return status;
    }

    return status;
}

bool PVMFStreamingManagerNode::GraphConnectFor3GPPUDPStreaming()
{
    if (oGraphConnectComplete == false)
    {
        /*
         * Go over the track list and connect:
         * network_node_port -> jitter_buffer_node_input_port;
         * jitter_buffer_node_output_port -> media_layer_input_port
         */
        PVMFStatus status;
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

            if ((trackInfo.iNetworkNodePort == NULL) ||
                    (trackInfo.iNetworkNodeRTCPPort == NULL) ||
                    (trackInfo.iJitterBufferInputPort == NULL) ||
                    (trackInfo.iJitterBufferOutputPort == NULL) ||
                    (trackInfo.iJitterBufferRTCPPort == NULL) ||
                    (trackInfo.iMediaLayerInputPort == NULL) ||
                    (trackInfo.iMediaLayerOutputPort == NULL))
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:GraphConnectFor3GPPUDPStreaming - Invalid Ports"));
                return false;
            }

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnect - Track MimeType %s", trackInfo.iMimeType.get_cstr()));

            /* connect network_node_port <-> jitter_buffer_node_input_port */
            status = ConnectPortPairs(trackInfo.iJitterBufferInputPort,
                                      trackInfo.iNetworkNodePort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - Connected Network - JB Input"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - NetworkPort=0x%x - JBInputPort=0x%x", trackInfo.iNetworkNodePort, trackInfo.iJitterBufferInputPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

            /*
             * connect jitter_buffer_node_output_port <->
             * media_layer_input_port
             */
            status = ConnectPortPairs(trackInfo.iJitterBufferOutputPort,
                                      trackInfo.iMediaLayerInputPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - JB Output - ML Input"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - JB Output=0x%x - ML Input=0x%x", trackInfo.iJitterBufferOutputPort, trackInfo.iMediaLayerInputPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

            /*
             * connect network_rtcp_port <-> jitter_buffer_rtcp_port
             */
            status = ConnectPortPairs(trackInfo.iJitterBufferRTCPPort,
                                      trackInfo.iNetworkNodeRTCPPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - NetworkRTCPPort - JBRTCPPort"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - NetworkRTCPPort=0x%x - JBRTCPPort=0x%x", trackInfo.iNetworkNodeRTCPPort, trackInfo.iJitterBufferRTCPPort));

            if (status != PVMFSuccess)
            {
                return false;
            }
        }
        oGraphConnectComplete = true;
    }
    return true;
}

bool PVMFStreamingManagerNode::GraphConnectForRTPPacketSource()
{
    if (oGraphConnectComplete == false)
    {
        /*
         * Go over the track list and connect:
         * network_node_port -> jitter_buffer_node_input_port;
         * jitter_buffer_node_output_port -> media_layer_input_port
         */
        PVMFStatus status;
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

            if ((trackInfo.iNetworkNodePort == NULL) ||
                    (trackInfo.iJitterBufferInputPort == NULL) ||
                    (trackInfo.iJitterBufferOutputPort == NULL) ||
                    (trackInfo.iMediaLayerInputPort == NULL) ||
                    (trackInfo.iMediaLayerOutputPort == NULL))
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:GraphConnectForRTPPacketSource - Invalid Ports"));
                return false;
            }

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnect - Track MimeType %s", trackInfo.iMimeType.get_cstr()));

            /* connect network_node_port <-> jitter_buffer_node_input_port */
            status = ConnectPortPairs(trackInfo.iJitterBufferInputPort,
                                      trackInfo.iNetworkNodePort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectForRTPPacketSource - Connected Network - JB Input"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectForRTPPacketSource - NetworkPort=0x%x - JBInputPort=0x%x", trackInfo.iNetworkNodePort, trackInfo.iJitterBufferInputPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

            /*
             * connect jitter_buffer_node_output_port <->
             * media_layer_input_port
             */
            status = ConnectPortPairs(trackInfo.iJitterBufferOutputPort,
                                      trackInfo.iMediaLayerInputPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectForRTPPacketSource - JB Output - ML Input"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectForRTPPacketSource - JB Output=0x%x - ML Input=0x%x", trackInfo.iJitterBufferOutputPort, trackInfo.iMediaLayerInputPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

            /*
             * connect network_rtcp_port <-> jitter_buffer_rtcp_port
             */
            status = ConnectPortPairs(trackInfo.iJitterBufferRTCPPort,
                                      trackInfo.iNetworkNodeRTCPPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - NetworkRTCPPort - JBRTCPPort"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPUDPStreaming - NetworkRTCPPort=0x%x - JBRTCPPort=0x%x", trackInfo.iNetworkNodeRTCPPort, trackInfo.iJitterBufferRTCPPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

        }
        oGraphConnectComplete = true;
    }
    return true;
}


bool PVMFStreamingManagerNode::GraphConnectFor3GPPTCPStreaming()
{
    if (oGraphConnectComplete == false)
    {
        /*
         * Go over the track list and connect:
         * network_node_port -> jitter_buffer_node_input_port;
         * jitter_buffer_node_output_port -> media_layer_input_port
         */
        PVMFStatus status;
        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

            if ((trackInfo.iSessionControllerOutputPort == NULL) ||
                    (trackInfo.iSessionControllerFeedbackPort == NULL) ||
                    (trackInfo.iJitterBufferInputPort == NULL) ||
                    (trackInfo.iJitterBufferOutputPort == NULL) ||
                    (trackInfo.iJitterBufferRTCPPort == NULL) ||
                    (trackInfo.iMediaLayerInputPort == NULL) ||
                    (trackInfo.iMediaLayerOutputPort == NULL))
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:GraphConnectFor3GPPTCPStreaming - Invalid Ports"));
                return false;
            }

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - Track MimeType %s", trackInfo.iMimeType.get_cstr()));

            /* connect sessioncontroller_node_output_port <-> jitter_buffer_node_input_port */
            status = ConnectPortPairs(trackInfo.iJitterBufferInputPort,
                                      trackInfo.iSessionControllerOutputPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - Connected SessionController Output - JB Input"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - SessionControllerPort=0x%x - JBInputPort=0x%x", trackInfo.iSessionControllerOutputPort, trackInfo.iJitterBufferInputPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

            /*
             * connect jitter_buffer_node_output_port <->
             * media_layer_input_port
             */
            status = ConnectPortPairs(trackInfo.iJitterBufferOutputPort,
                                      trackInfo.iMediaLayerInputPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - JB Output - ML Input"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - JB Output=0x%x - ML Input=0x%x", trackInfo.iJitterBufferOutputPort, trackInfo.iMediaLayerInputPort));

            if (status != PVMFSuccess)
            {
                return false;
            }

            /*
             * connect sessioncontroller_node_feedback_port <-> jitter_buffer_rtcp_port
             */
            status = ConnectPortPairs(trackInfo.iJitterBufferRTCPPort,
                                      trackInfo.iSessionControllerFeedbackPort);

            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - SessionControllerFeedbackPort - JBRTCPPort"));
            PVMF_SM_LOGINFO((0, "PVMFSM:GraphConnectFor3GPPTCPStreaming - SessionControllerFeedbackPort=0x%x - JBRTCPPort=0x%x", trackInfo.iSessionControllerFeedbackPort, trackInfo.iJitterBufferRTCPPort));

            if (status != PVMFSuccess)
            {
                return false;
            }
        }
        oGraphConnectComplete = true;
    }
    return true;
}


/*
 * Called by command handler AO once all request ports are complete
 */
bool PVMFStreamingManagerNode::GraphConnect()
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        if (iSessionSourceInfo->iRTSPTunnelling == true)
        {
            return (GraphConnectFor3GPPTCPStreaming());
        }
        else
        {
            return (GraphConnectFor3GPPUDPStreaming());
        }
    }
    else if (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE)
    {
        return (GraphConnectForRTPPacketSource());
    }

    //error
    return false;
}


/*
 * Called when all port requests are complete, in order to send the
 * UDP port information to RTSP
 */
bool
PVMFStreamingManagerNode::SendSessionSourceInfoToSessionController()
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        PVMFSMNodeContainer* iSocketNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
        if (iSocketNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

        PVMFSocketNode* socketNode =
            (PVMFSocketNode*)(iSocketNodeContainer->iNode);

        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);

        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        Oscl_Vector<StreamInfo, OsclMemAllocator> aSelectedStream;

        for (uint32 i = 0; i < iTrackInfoVec.size(); i++)
        {
            PVMFSMTrackInfo trackInfo = iTrackInfoVec[i];

            OsclNetworkAddress localAdd;
            OsclNetworkAddress remoteAdd;
            StreamInfo sInfo;

            sInfo.iSDPStreamId = trackInfo.trackID;

            if (iSessionSourceInfo->iRTSPTunnelling == false)
            {
                if (trackInfo.iNetworkNodePort == NULL)
                {
                    PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendSessionSourceInfoToSessionController - Invalid Port"));
                    return false;
                }

                socketNode->GetPortConfig(*trackInfo.iNetworkNodePort,
                                          localAdd,
                                          remoteAdd);

                sInfo.iCliRTPPort = localAdd.port;

                socketNode->GetPortConfig(*trackInfo.iNetworkNodeRTCPPort,
                                          localAdd,
                                          remoteAdd);

                sInfo.iCliRTCPPort = localAdd.port;
            }

            /* Set Rate Adaptation parameters */
            sInfo.b3gppAdaptationIsSet = false;
            if (trackInfo.iRateAdaptation)
            {
                sInfo.b3gppAdaptationIsSet = true;
                /* Compute buffer size based on bitrate and jitter duration*/
                uint32 sizeInBytes = MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES;
                if (((int32)iJitterBufferDurationInMilliSeconds > 0) &&
                        ((int32)trackInfo.bitRate > 0))
                {
                    uint32 byteRate = trackInfo.bitRate / 8;
                    uint32 overhead = (byteRate * PVMF_JITTER_BUFFER_NODE_MEM_POOL_OVERHEAD) / 100;
                    uint32 durationInSec = iJitterBufferDurationInMilliSeconds / 1000;
                    sizeInBytes = ((byteRate + overhead) * durationInSec);
                    if (sizeInBytes < MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES)
                    {
                        sizeInBytes = MIN_RTP_SOCKET_MEM_POOL_SIZE_IN_BYTES;
                    }
                    sizeInBytes += 2 * MAX_SOCKET_BUFFER_SIZE;
                }
                sInfo.iBufSize = sizeInBytes;
                sInfo.iTargetTime = iJitterBufferDurationInMilliSeconds;
            }
            aSelectedStream.push_back(sInfo);
        }

        if (rtspExtIntf->SetSDPInfo(iSessionSourceInfo->_sdpInfo,
                                    aSelectedStream) != PVMFSuccess)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendSessionSourceInfoToSessionController - SetSDPInfo Failed"));
            return false;
        }

        /* Set play range from SDP */
        sessionDescription* sessionInfo = iSessionSourceInfo->_sdpInfo->getSessionInfo();
        RtspRangeType *rtspRange = OSCL_CONST_CAST(RtspRangeType*, (sessionInfo->getRange()));
        rtspRange->convertToMilliSec((int32&)iSessionStartTime, (int32&)iSessionStopTime);

        if (rtspRange->end_is_set == false)
        {
            iSessionStopTime = -1;
            iSessionStopTimeAvailable = false;
        }

        if ((rtspRange->format != RtspRangeType::INVALID_RANGE) &&
                (rtspRange->start_is_set != false))
        {
            if (rtspExtIntf->SetRequestPlayRange(*rtspRange) != PVMFSuccess)
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendRequestPlayRangeToSessionController - SetRequestPlayRange Failed"));
                return false;
            }
        }
    }
    return true;
}

bool
PVMFStreamingManagerNode::SendSessionControlPrepareCompleteParams()
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        PVMFSMNodeContainer* iJitterBufferNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
        if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFJitterBufferExtensionInterface* jbExtIntf =
            (PVMFJitterBufferExtensionInterface*)
            (iJitterBufferNodeContainer->iExtensions[0]);

        Oscl_Vector<StreamInfo, OsclMemAllocator> aSelectedStream;

        if (rtspExtIntf->GetStreamInfo(aSelectedStream) != PVMFSuccess)
        {
            OSCL_LEAVE(OsclErrGeneral);
        }

        for (uint32 i = 0; i < aSelectedStream.size(); i++)
        {
            StreamInfo streamInfo = aSelectedStream[i];

            PVMFSMTrackInfo* trackInfo = FindTrackInfo(streamInfo.iSDPStreamId);

            if (trackInfo == NULL)
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendSessionControlPrepareCompleteParams - FindTrackInfo Failed"));
                return false;
            }

            if (trackInfo->iJitterBufferInputPort == NULL)
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendSessionControlPrepareCompleteParams - Invalid Port"));
                return false;
            }

            if (streamInfo.ssrcIsSet)
            {
                jbExtIntf->setPortSSRC(trackInfo->iJitterBufferInputPort,
                                       streamInfo.iSSRC);
            }
        }

        /* Set server info */
        PVRTSPEngineNodeServerInfo rtspServerInfo;
        PVMFJitterBufferFireWallPacketInfo fireWallPktInfo;

        rtspExtIntf->GetServerInfo(rtspServerInfo);

        if (rtspServerInfo.iIsPVServer)
        {
            fireWallPktInfo.iFormat = PVMF_JB_FW_PKT_FORMAT_PV;
        }
        fireWallPktInfo.iServerRoundTripDelayInMS = rtspServerInfo.iRoundTripDelayInMS;
        fireWallPktInfo.iNumAttempts = PVMF_JITTER_BUFFER_NODE_DEFAULT_FIREWALL_PKT_ATTEMPTS;

        jbExtIntf->setServerInfo(fireWallPktInfo);
    }
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL))
    {
        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        PVMFSMNodeContainer* iJitterBufferNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
        if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFJitterBufferExtensionInterface* jbExtIntf =
            (PVMFJitterBufferExtensionInterface*)
            (iJitterBufferNodeContainer->iExtensions[0]);

    }
    return true;
}

bool
PVMFStreamingManagerNode::SendSessionControlStartCompleteParams()
{
    PVMFSMNodeContainer* iJitterBufferNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
    if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFJitterBufferExtensionInterface* jbExtIntf =
        (PVMFJitterBufferExtensionInterface*)
        (iJitterBufferNodeContainer->iExtensions[0]);

    PVMFSMNodeContainer* iMediaLayerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
        (PVMFMediaLayerNodeExtensionInterface*)
        (iMediaLayerNodeContainer->iExtensions[0]);

    bool end_is_set = true;
    int32 startTime = 0;
    int32 stopTime  = 0;

    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        /* Get Actual Play Range */
        RtspRangeType rangeType;
        if (rtspExtIntf->GetActualPlayRange(rangeType) != PVMFSuccess)
        {
            return false;
        }

        rangeType.convertToMilliSec(startTime, stopTime);

        /* Use from SDP if not set */
        end_is_set = rangeType.end_is_set;
        if (end_is_set == false)
        {
            stopTime = iSessionStopTime;
        }

        if (oRepositioning)
        {
            iActualRepositionStartNPTInMS = startTime;
            if (iActualRepositionStartNPTInMSPtr != NULL)
            {
                *iActualRepositionStartNPTInMSPtr = startTime;
            }
            if (iPVMFDataSourcePositionParamsPtr != NULL)
            {
                iPVMFDataSourcePositionParamsPtr->iActualNPT = startTime;
            }
        }

        Oscl_Vector<StreamInfo, OsclMemAllocator> aSelectedStream;

        if (rtspExtIntf->GetStreamInfo(aSelectedStream) != PVMFSuccess)
        {
            OSCL_LEAVE(OsclErrGeneral);
        }

        for (uint32 i = 0; i < aSelectedStream.size(); i++)
        {
            StreamInfo streamInfo = aSelectedStream[i];

            PVMFSMTrackInfo* trackInfo = FindTrackInfo(streamInfo.iSDPStreamId);

            if (trackInfo == NULL)
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendStartCompleteSessionControlParams - FindTrackInfo Failed"));
                return false;
            }

            if (trackInfo->iJitterBufferInputPort == NULL)
            {
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:SendStartCompleteSessionControlParams - Invalid Port"));
                return false;
            }

            if (streamInfo.seqIsSet != true)
            {
                streamInfo.seqIsSet = false;
                streamInfo.seq = 0;
            }
            if (streamInfo.rtptimeIsSet != true)
            {
                streamInfo.rtptimeIsSet = false;
                streamInfo.rtptime = 0;
            }
            jbExtIntf->setPortRTPParams(trackInfo->iJitterBufferInputPort,
                                        streamInfo.seqIsSet,
                                        streamInfo.seq,
                                        streamInfo.rtptimeIsSet,
                                        streamInfo.rtptime,
                                        startTime,
                                        oRepositioning);

        }
    }
    /* Send actual stop time to Jitter Buffer */
    if (jbExtIntf->setPlayRange(startTime,
                                stopTime,
                                oRepositioning,
                                end_is_set) != true)
    {
        return false;
    }

    if (mlExtIntf->setPlayRange(startTime, stopTime) != true)
    {
        return false;
    }
    return true;
}

bool
PVMFStreamingManagerNode::SendPacketSourceStartCompleteParams()
{
    PVMFSMNodeContainer* iJitterBufferNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
    if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFJitterBufferExtensionInterface* jbExtIntf =
        (PVMFJitterBufferExtensionInterface*)
        (iJitterBufferNodeContainer->iExtensions[0]);

    PVMFSMNodeContainer* iMediaLayerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
        (PVMFMediaLayerNodeExtensionInterface*)
        (iMediaLayerNodeContainer->iExtensions[0]);

    bool end_is_set = true;
    int32 startTime = 0;
    int32 stopTime  = 0;

    /* Get Actual Play Range */
    RtspRangeType rangeType;
    rangeType.convertToMilliSec(startTime, stopTime);

    /* Use from SDP if not set */
    end_is_set = false;
    stopTime = -1;

    PVMFSMTrackInfoVector::iterator it;

    for (it = iTrackInfoVec.begin();
            it != iTrackInfoVec.end();
            it++)
    {
        StreamInfo streamInfo;

        streamInfo.seqIsSet = false;
        streamInfo.seq = 0;

        streamInfo.rtptimeIsSet = false;
        streamInfo.rtptime = 0;
        jbExtIntf->setPortRTPParams(it->iJitterBufferInputPort,
                                    streamInfo.seqIsSet,
                                    streamInfo.seq,
                                    streamInfo.rtptimeIsSet,
                                    streamInfo.rtptime,
                                    startTime,
                                    oRepositioning);

    }

    /* Send actual stop time to Jitter Buffer */
    if (jbExtIntf->setPlayRange(startTime, stopTime, false, end_is_set) != true)
    {
        return false;
    }

    if (mlExtIntf->setPlayRange(startTime, stopTime) != true)
    {
        return false;
    }
    return true;
}


bool
PVMFStreamingManagerNode::CompleteFeedBackPortsSetup()
{
    PVMFSMNodeContainer* iSocketNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
    if (iSocketNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVMFSocketNode* socketNode =
        (PVMFSocketNode*)(iSocketNodeContainer->iNode);

    PVMFSMNodeContainer* iSessionControllerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);

    if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
        (PVRTSPEngineNodeExtensionInterface*)
        (iSessionControllerNodeContainer->iExtensions[0]);

    Oscl_Vector<StreamInfo, OsclMemAllocator> aSelectedStream;

    if (rtspExtIntf->GetStreamInfo(aSelectedStream) != PVMFSuccess)
    {
        OSCL_LEAVE(OsclErrGeneral);
    }

    for (uint32 i = 0; i < aSelectedStream.size(); i++)
    {
        StreamInfo streamInfo = aSelectedStream[i];

        PVMFSMTrackInfo* trackInfo = FindTrackInfo(streamInfo.iSDPStreamId);

        if (trackInfo == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:CompleteFeedBackPortsSetup - FindTrackInfo Failed"));
            return false;
        }

        if (trackInfo->iNetworkNodeRTCPPort == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:CompleteFeedBackPortsSetup - Invalid RTCP Port"));
            return false;
        }

        OsclNetworkAddress localAddRTCP;
        OsclNetworkAddress remoteAddRTCP;

        localAddRTCP.port = streamInfo.iCliRTCPPort;
        remoteAddRTCP.port = streamInfo.iSerRTCPPort;
        remoteAddRTCP.ipAddr = streamInfo.iSerIpAddr;

        socketNode->SetPortConfig(*(trackInfo->iNetworkNodeRTCPPort),
                                  localAddRTCP,
                                  remoteAddRTCP);

        if (trackInfo->iNetworkNodePort == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:CompleteFeedBackPortsSetup - Invalid RTP Port"));
            return false;
        }

        OsclNetworkAddress localAddRTP;
        OsclNetworkAddress remoteAddRTP;

        localAddRTP.port = streamInfo.iCliRTPPort;
        remoteAddRTP.port = streamInfo.iSerRTPPort;
        remoteAddRTP.ipAddr = streamInfo.iSerIpAddr;

        socketNode->SetPortConfig(*(trackInfo->iNetworkNodePort),
                                  localAddRTP,
                                  remoteAddRTP);

    }

    return true;
}

PVMFStatus
PVMFStreamingManagerNode::DeleteUnusedSessionControllerNode()
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        /* Delete HTTP PE Node */
        PVMFSMNodeContainerVector::iterator it;
        for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
        {
            if (it->iNodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE)
            {
                for (uint32 j = 0; j < it->iExtensions.size(); j++)
                {
                    PVInterface* extIntf = it->iExtensions[j];
                    extIntf->removeRef();
                }
                PVMFProtocolEngineNodeFactory::DeletePVMFProtocolEngineNode(it->iNode);
                it->iNode = NULL;
                iNodeContainerVec.erase(it);
                break;
            }
        }
    }
    DeleteUnusedNodes();

    return PVMFSuccess;
}

PVMFStatus
PVMFStreamingManagerNode::DeleteUnusedNodes()
{
    if (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE)
    {
        PVMFSMNodeContainerVector::iterator it;
        for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
        {
            /* Delete Unused Socket Node */
            if (it->iNodeTag == PVMF_STREAMING_MANAGER_SOCKET_NODE)
            {
                for (uint32 j = 0; j < it->iExtensions.size(); j++)
                {
                    PVInterface* extIntf = it->iExtensions[j];
                    extIntf->removeRef();
                }
                PVMF_STREAMING_MANAGER_DELETE(NULL,
                                              PVMFSocketNode,
                                              ((PVMFSocketNode*)(it->iNode)));
                it->iNode = NULL;
                iNodeContainerVec.erase(it);
                break;
            }
        }
        for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
        {
            /* Delete Unused RTSP Session Controller Node */
            if (it->iNodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE)
            {
                for (uint32 j = 0; j < it->iExtensions.size(); j++)
                {
                    PVInterface* extIntf = it->iExtensions[j];
                    extIntf->removeRef();
                }
                PVMFRrtspEngineNodeFactory::DeletePVMFRtspEngineNode(it->iNode);
                it->iNode = NULL;
                iNodeContainerVec.erase(it);
                break;
            }
        }
        for (it = iNodeContainerVec.begin(); it != iNodeContainerVec.end(); it++)
        {
            /* Delete Unused HTTP Session Controller Node */
            if (it->iNodeTag == PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE)
            {
                for (uint32 j = 0; j < it->iExtensions.size(); j++)
                {
                    PVInterface* extIntf = it->iExtensions[j];
                    extIntf->removeRef();
                }
                PVMFProtocolEngineNodeFactory::DeletePVMFProtocolEngineNode(it->iNode);
                it->iNode = NULL;
                iNodeContainerVec.erase(it);
                break;
            }
        }
    }
    else
    {
    }
    return PVMFSuccess;
}

PVMFStatus
PVMFStreamingManagerNode::setSessionSourceInfo(OSCL_wString& aSourceURL,
        PVMFFormatType aSourceFormatType,
        OsclAny* aSourceData)
{

    if (((aSourceFormatType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (aSourceFormatType == PVMF_DATA_SOURCE_SDP_FILE)) && (aSourceData != NULL))
    {
        PVInterface* pvInterface = OSCL_STATIC_CAST(PVInterface*, aSourceData);

        PVInterface* sourceDataContext = NULL;
        PVUuid sourceContextUuid(PVMF_SOURCE_CONTEXT_DATA_UUID);
        if (pvInterface->queryInterface(sourceContextUuid, sourceDataContext))
        {
            if (sourceDataContext)
            {
                PVInterface* streamingDataContext = NULL;
                PVUuid streamingContextUuid(PVMF_SOURCE_CONTEXT_DATA_STREAMING_UUID);

                if (sourceDataContext->queryInterface(streamingContextUuid, streamingDataContext))
                {
                    if (streamingDataContext)
                    {
                        PVMFSourceContextDataStreaming* sContext =
                            OSCL_STATIC_CAST(PVMFSourceContextDataStreaming*, streamingDataContext);
                        if (sContext->iProxyName.get_size() > 0)
                        {
                            PVMFSMNodeContainer* iSessionControllerNodeContainer = NULL;
                            iSessionControllerNodeContainer = getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
                            if (iSessionControllerNodeContainer)
                            {
                                PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                                    (PVRTSPEngineNodeExtensionInterface*)(iSessionControllerNodeContainer->iExtensions[0]);
                                if (rtspExtIntf)
                                {//the proxyname doesn't need to be unicode
                                    OsclMemAllocator alloc;
                                    char *buf = (char*)alloc.allocate(sContext->iProxyName.get_size() + 1);
                                    if (!buf)
                                        return PVMFErrNoMemory;
                                    uint32 size = oscl_UnicodeToUTF8(sContext->iProxyName.get_cstr(), sContext->iProxyName.get_size(), buf, sContext->iProxyName.get_size() + 1);
                                    if (size == 0)
                                    {
                                        alloc.deallocate(buf);
                                        return PVMFErrNoMemory;
                                    }

                                    OSCL_FastString myProxyName(buf, size);

                                    rtspExtIntf->SetRtspProxy(myProxyName, sContext->iProxyPort);
                                    alloc.deallocate(buf);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if ((aSourceFormatType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (aSourceFormatType == PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL))
    {
        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        OSCL_wStackString<8> rtspScheme(_STRLIT_WCHAR("rtsp"));
        OSCL_wStackString<8> rtsptScheme(_STRLIT_WCHAR("rtspt"));
        OSCL_wStackString<8> schemeDelimiter(_STRLIT_WCHAR("://"));
        oscl_wchar* actualURL = NULL;
        if (oscl_strncmp(rtsptScheme.get_cstr(), aSourceURL.get_cstr(), 5) == 0)
        {
            iSessionSourceInfo->iRTSPTunnelling = true;
            actualURL = oscl_strstr(aSourceURL.get_cstr(), schemeDelimiter.get_cstr());
            if (actualURL == NULL)
            {
                return PVMFErrArgument;
            }
            //skip over ://
            actualURL += schemeDelimiter.get_size();
            iSessionSourceInfo->_sessionType = aSourceFormatType;
            iSessionSourceInfo->_sessionURL  += rtspScheme.get_str();
            iSessionSourceInfo->_sessionURL  += schemeDelimiter.get_str();
            iSessionSourceInfo->_sessionURL  += actualURL;
        }
        else
        {
            iSessionSourceInfo->_sessionType = aSourceFormatType;
            iSessionSourceInfo->_sessionURL  = aSourceURL;
        }

        if (iSessionSourceInfo->iRTSPTunnelling == true)
        {
            rtspExtIntf->SetStreamingType(PVRTSP_3GPP_TCP);
        }
        else
        {
            /* default to UDP */
            rtspExtIntf->SetStreamingType(PVRTSP_3GPP_UDP);
        }

        return (rtspExtIntf->SetSessionURL(iSessionSourceInfo->_sessionURL));
    }
    else if (aSourceFormatType == PVMF_DATA_SOURCE_SDP_FILE)
    {
        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        iSessionSourceInfo->_sessionType = aSourceFormatType;
        iSessionSourceInfo->_sessionURL  = aSourceURL;

        rtspExtIntf->SetStreamingType(PVRTSP_3GPP_UDP);

        return PVMFSuccess;
    }
    else if (aSourceFormatType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE)
    {
        iSessionSourceInfo->_sessionType = aSourceFormatType;
        iSessionSourceInfo->_sessionURL  = aSourceURL;
        return PVMFSuccess;
    }
    return PVMFErrNotSupported;
}

PVMFStatus PVMFStreamingManagerNode::ProcessSDP()
{
    PVMFStatus status;
    OsclRefCounterMemFrag iSDPText;

    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL))
    {
        PVMFSMNodeContainer* iSessionControllerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);

        if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

        PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
            (PVRTSPEngineNodeExtensionInterface*)
            (iSessionControllerNodeContainer->iExtensions[0]);

        status = rtspExtIntf->GetSDP(iSDPText);

        if (status != PVMFSuccess)
        {
            return status;
        }
    }
    else if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
             (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        /* Parse SDP file contents into a buffer */
        Oscl_FileServer fileServ;
        Oscl_File osclFile;
        fileServ.Connect();

        if (osclFile.Open(iSessionSourceInfo->_sessionURL.get_cstr(),
                          Oscl_File::MODE_READ,
                          fileServ) != 0)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:ProcessSDP - Unable to open SDP file"));
            return PVMFFailure;
        }

        /* Get File Size */
        osclFile.Seek(0, Oscl_File::SEEKEND);
        int32 fileSize = osclFile.Tell();
        osclFile.Seek(0, Oscl_File::SEEKSET);

        if (fileSize <= 0)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:ProcessSDP - Corrupt SDP file"));
            return PVMFFailure;
        }

        OsclMemAllocDestructDealloc<uint8> my_alloc;
        OsclRefCounter* my_refcnt;
        uint aligned_refcnt_size =
            oscl_mem_aligned_size(sizeof(OsclRefCounterSA< OsclMemAllocDestructDealloc<uint8> >));
        uint8* my_ptr = NULL;
        int32 errcode = 0;
        /*
         * To acct for null char, as SDP buffer is treated akin to a string by the
         * SDP parser lib.
         */
        uint allocsize = oscl_mem_aligned_size(aligned_refcnt_size + fileSize + 2);
        OSCL_TRY(errcode, my_ptr = (uint8*) my_alloc.ALLOCATE(allocsize));

        if (errcode != OsclErrNone)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:ProcessSDP - Unable to process SDP file"));
            return PVMFFailure;
        }

        my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterSA< OsclMemAllocDestructDealloc<uint8> >(my_ptr));
        my_ptr += aligned_refcnt_size;

        OsclMemoryFragment memfrag;
        memfrag.len = fileSize;
        memfrag.ptr = my_ptr;

        OsclRefCounterMemFrag tmpRefcntMemFrag(memfrag, my_refcnt, memfrag.len);
        iSDPText = tmpRefcntMemFrag;

        osclFile.Read(memfrag.ptr, 1, fileSize);

        osclFile.Close();
        fileServ.Close();

    }

    PVMFSMSharedPtrAlloc<SDPInfo> sdpAlloc;
    SDPInfo* sdpInfo = sdpAlloc.allocate();

    SDP_Parser *sdpParser;

    PVMF_STREAMING_MANAGER_NEW(NULL, SDP_Parser, (), sdpParser);

    int32 sdpRetVal =
        sdpParser->parseSDP((const char*)(iSDPText.getMemFragPtr()),
                            iSDPText.getMemFragSize(),
                            sdpInfo);

    // save the SDP file name - the packet source node will need this
    sdpInfo->setSDPFilename(iSessionSourceInfo->_sessionURL);

    PVMF_STREAMING_MANAGER_DELETE(NULL, SDP_Parser, sdpParser);

    OsclRefCounterSA< PVMFSMSharedPtrAlloc<SDPInfo> > *refcnt =
        new OsclRefCounterSA< PVMFSMSharedPtrAlloc<SDPInfo> >(sdpInfo);

    OsclSharedPtr<SDPInfo> sharedSDPInfo(sdpInfo, refcnt);

    if (sdpRetVal != SDP_SUCCESS)
    {
        return PVMFFailure;
    }

    iSessionSourceInfo->_sdpInfo = sharedSDPInfo;

    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::RecognizeAndProcessHeader()
{
    OSCL_ASSERT(iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL);

    return Asf_RecognizeAndProcessHeader();
}

PVMFStatus
PVMFStreamingManagerNode::GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo)
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        SDPInfo* sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();

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

        PVMF_TRACK_INFO_TRACK_ALTERNATE_TYPE iAltType = PVMF_TRACK_ALTERNATE_TYPE_UNDEFINED;

        if (sdpAltGroupType == SDP_ALT_GROUP_LANGUAGE)
        {
            iAltType = PVMF_TRACK_ALTERNATE_TYPE_LANGUAGE;
        }
        else if (sdpAltGroupType == SDP_ALT_GROUP_BANDWIDTH)
        {
            iAltType = PVMF_TRACK_ALTERNATE_TYPE_BANDWIDTH;
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
                    return false;
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
                {
                    mimeString += mimeType;
                }
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

                    uint8* my_ptr = NULL;
                    int32 errcode = 0;
                    OSCL_TRY(errcode, my_ptr = (uint8*) my_alloc.ALLOCATE(aligned_refcnt_size + configSize));

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

                if (iAltType != PVMF_TRACK_ALTERNATE_TYPE_UNDEFINED)
                {
                    /* Expose alternate track ids */
                    trackInfo.setTrackAlternates(iAltType);
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
    return PVMFErrNotSupported;
}

PVMFStatus
PVMFStreamingManagerNode::SelectTracks(PVMFMediaPresentationInfo& aInfo,
                                       PVMFSessionId s)
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        SDPInfo* sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();
        if (sdpInfo == NULL)
        {
            PVMF_SM_LOGERROR((0, "StreamingManagerNode:SelectTracks - SDP Not Available"));
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
                PVMF_SM_LOGERROR((0, "StreamingManagerNode:SelectTracks - Invalid SDP TrackID"));
                return PVMFErrArgument;
            }

            mInfo->setSelect();

            /* Set selected field in meta info */
            Oscl_Vector<PVMFSMTrackMetaDataInfo, PVMFStreamingManagerNodeAllocator>::iterator it;
            for (it = iMetaDataInfo.iTrackMetaDataInfoVec.begin(); it != iMetaDataInfo.iTrackMetaDataInfoVec.end(); it++)
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
    return PVMFErrNotSupported;
}

PVMFCommandId
PVMFStreamingManagerNode::SetDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aActualNPT,
        PVMFTimestamp& aActualMediaDataTS,
        bool aJumpToIFrame,
        uint32 aStreamID,
        OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::SetDataSourcePosition - In"));

    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION,
            aTargetNPT,
            &aActualNPT,
            &aActualMediaDataTS,
            aJumpToIFrame,
            aStreamID,
            aContext);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::SetDataSourcePosition - Out"));
    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::SetDataSourcePosition() - Cmd Recvd"));
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFStreamingManagerNode::SetDataSourcePosition(PVMFSessionId aSessionId,
        PVMFDataSourcePositionParams& aPVMFDataSourcePositionParams,
        OsclAny* aContext)
{
    PVMFStreamingManagerNodeCommand cmd;
    return QueueCommandL(cmd);
}


void PVMFStreamingManagerNode::DoSetDataSourcePosition(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoSetDataSourcePosition - In"));

    iActualRepositionStartNPTInMSPtr = NULL;
    iActualMediaDataTSPtr = NULL;
    iPVMFDataSourcePositionParamsPtr = NULL;
    iJumpToIFrame = false;
    uint32 streamID = 0;

    aCmd.PVMFStreamingManagerNodeCommand::Parse(iRepositionRequestedStartNPTInMS,
            iActualRepositionStartNPTInMSPtr,
            iActualMediaDataTSPtr,
            iJumpToIFrame,
            streamID);

    PVMF_SM_LOG_COMMAND_REPOS((0, "PVMFStreamingManagerNode::DoSetDataSourcePosition - TargetNPT = %d", iRepositionRequestedStartNPTInMS));

    PVMFSMNodeContainer* iJitterBufferNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
    if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFJitterBufferExtensionInterface* jbExtIntf =
        (PVMFJitterBufferExtensionInterface*)
        (iJitterBufferNodeContainer->iExtensions[0]);
    bool retVal = false;

    // duplicate bos has been received
    // dont perform reposition at source node
    if (iStreamID == streamID)	retVal = true; // data is already present in the graph

    iStreamID = streamID;
    jbExtIntf->SendBOSMessage(iStreamID);


    *iActualRepositionStartNPTInMSPtr = 0;
    *iActualMediaDataTSPtr = 0;


    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        if (iInterfaceState == EPVMFNodePrepared)
        {
            /*
             * SetDataSource from a prepared state could mean two things:
             *	- In Play-Stop-Play usecase engine does a SetDataSourcePosition
             *    to get the start media TS to set its playback clock
             *  - Engine is trying to do a play with a non-zero start offset
             */
            if (iRepositionRequestedStartNPTInMS < iSessionStopTime && iRepositionRequestedStartNPTInMS != iSessionStartTime)
            {

                /* Set Requested Play Range */
                PVMFSMNodeContainer* iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
                PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                    (PVRTSPEngineNodeExtensionInterface*)
                    (iSessionControllerNodeContainer->iExtensions[0]);

                RtspRangeType rtspRange;
                rtspRange.format = RtspRangeType::NPT_RANGE;
                rtspRange.start_is_set = true;
                rtspRange.npt_start.npt_format = NptTimeFormat::NPT_SEC;
                rtspRange.npt_start.npt_sec.sec = iRepositionRequestedStartNPTInMS / 1000;
                rtspRange.npt_start.npt_sec.milli_sec =
                    (iRepositionRequestedStartNPTInMS - ((iRepositionRequestedStartNPTInMS / 1000) * 1000));
                rtspRange.end_is_set = true;
                rtspRange.npt_end.npt_format = NptTimeFormat::NPT_SEC;
                rtspRange.npt_end.npt_sec.sec = iSessionStopTime / 1000;
                rtspRange.npt_end.npt_sec.milli_sec =
                    (iSessionStopTime - ((iSessionStopTime / 1000) * 1000));

                if (rtspExtIntf->SetRequestPlayRange(rtspRange) != PVMFSuccess)
                {
                    PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoSetDataSourcePosition - SetRequestPlayRange Failed"));
                    PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::SetDataSourcePosition() - Cmd Failed - PVMFFailure"));
                    CommandComplete(iInputCommands, aCmd, PVMFFailure);
                    return;
                }

                // we need to use part of the logic of repositioning to start
                // streaming from a non-zero offset. Enabled only for 3gpp streaming
                if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
                        (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
                {
                    oRepositioning = true;
                    /* Start the nodes */
                    if (!DoRepositioningStart3GPPStreaming())
                    {
                        CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                        return;
                    }

                    MoveCmdToCurrentQueue(aCmd);
                    return;
                }
            }

            GetAcutalMediaTSAfterSeek();
            PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::SetDataSourcePosition() - CmdComplete"));
            CommandComplete(iInputCommands, aCmd, PVMFSuccess);
        }
        else if ((iInterfaceState == EPVMFNodeStarted) || (iInterfaceState == EPVMFNodePaused))
        {
            bool oRandAccessDenied = true;

            sessionDescription* sessionInfo =
                iSessionSourceInfo->_sdpInfo->getSessionInfo();
            oRandAccessDenied = sessionInfo->getRandomAccessDenied();


            if ((oRandAccessDenied == true) ||
                    (iSessionStopTimeAvailable == false) ||
                    (((int32)iRepositionRequestedStartNPTInMS < (int32)iSessionStartTime) ||
                     ((int32)iRepositionRequestedStartNPTInMS >= (int32)iSessionStopTime)))
            {
                /*
                 * Implies an open ended session or invalid request time
                 * - no pause or reposition
                 */
                CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
                return;
            }

            oRepositioning = true;

            /* Put the jitter buffer into a state of transition */
            PVMFSMNodeContainer* iJitterBufferNodeContainer =
                getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
            if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
            PVMFJitterBufferExtensionInterface* jbExtIntf =
                (PVMFJitterBufferExtensionInterface*)
                (iJitterBufferNodeContainer->iExtensions[0]);
            jbExtIntf->PrepareForRepositioning();

            /* If node is running, pause first */
            if (iInterfaceState == EPVMFNodeStarted)
            {
                if (!DoRepositioningPause3GPPStreaming())
                {
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                }
            }
            /* If already paused do not pause */
            else if (iInterfaceState == EPVMFNodePaused)
            {
                /* Start the nodes */
                if (!DoRepositioningStart3GPPStreaming())
                {
                    CommandComplete(iInputCommands, aCmd, PVMFErrNoMemory);
                }
            }
            MoveCmdToCurrentQueue(aCmd);
        }
        else
        {
            CommandComplete(iInputCommands, aCmd, PVMFErrInvalidState);
            return;
        }
    }
    else
    {
        PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode::SetDataSourcePosition() - Cmd Failed - PVMFErrArgument"));
        CommandComplete(iInputCommands, aCmd, PVMFErrArgument);
        return;
    }
    return;
}

bool PVMFStreamingManagerNode::DoRepositioningPause3GPPStreaming()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        int32 nodeTag = iNodeContainerVec[i].iNodeTag;

        if ((nodeTag == PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_SOCKET_NODE))
        {
            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    iNodeContainerVec[i].commandStartOffset +
                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_PAUSE_CMD_OFFSET;
                internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION;

                OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                iNode->Pause(iNodeContainerVec[i].iSessionId, cmdContextData);
                iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
            }
            else
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoRepositioningPause:RequestNewInternalCmd - Failed"));
                return false;
            }
        }
    }
    return true;
}

bool PVMFStreamingManagerNode::DoRepositioningPauseMSHTTPStreaming()
{
    /* Pause session controller */
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        int32 nodeTag = iNodeContainerVec[i].iNodeTag;
        if ((nodeTag == PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE))
        {
            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    iNodeContainerVec[i].commandStartOffset +
                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_PAUSE_CMD_OFFSET;
                internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION;

                OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                iNode->Pause(iNodeContainerVec[i].iSessionId, cmdContextData);
                iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
            }
            else
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoRepositioningPause:RequestNewInternalCmd - Failed"));
                return false;
            }
        }
    }
    return true;
}


bool PVMFStreamingManagerNode::DoRepositioningStart3GPPStreaming()
{
    /* Set Requested Play Range */
    PVMFSMNodeContainer* iSessionControllerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);

    if (iSessionControllerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
        (PVRTSPEngineNodeExtensionInterface*)
        (iSessionControllerNodeContainer->iExtensions[0]);

    RtspRangeType rtspRange;
    rtspRange.format = RtspRangeType::NPT_RANGE;
    rtspRange.start_is_set = true;
    rtspRange.npt_start.npt_format = NptTimeFormat::NPT_SEC;
    rtspRange.npt_start.npt_sec.sec = iRepositionRequestedStartNPTInMS / 1000;
    rtspRange.npt_start.npt_sec.milli_sec =
        (iRepositionRequestedStartNPTInMS - ((iRepositionRequestedStartNPTInMS / 1000) * 1000));
    rtspRange.end_is_set = true;
    rtspRange.npt_end.npt_format = NptTimeFormat::NPT_SEC;
    rtspRange.npt_end.npt_sec.sec = iSessionStopTime / 1000;
    rtspRange.npt_end.npt_sec.milli_sec =
        (iSessionStopTime - ((iSessionStopTime / 1000) * 1000));

    if (rtspExtIntf->SetRequestPlayRange(rtspRange) != PVMFSuccess)
    {
        PVMF_SM_LOGERROR((0, "StreamingManagerNode:DoRepositioningStart - SetRequestPlayRange Failed"));
        return false;
    }
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
        if (internalCmd != NULL)
        {
            internalCmd->cmd =
                iNodeContainerVec[i].commandStartOffset +
                PVMF_STREAMING_MANAGER_NODE_INTERNAL_START_CMD_OFFSET;
            internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION;

            OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

            PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

            iNode->Start(iNodeContainerVec[i].iSessionId, cmdContextData);
            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
        }
        else
        {
            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoRepositioningStart:RequestNewInternalCmd - Failed"));
            return false;
        }
    }
    return true;
}


bool PVMFStreamingManagerNode::DoRepositioningStartMSHTTPStreaming()
{
    PVMFSMNodeContainer* iMediaLayerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
        (PVMFMediaLayerNodeExtensionInterface*)
        (iMediaLayerNodeContainer->iExtensions[0]);
    if (mlExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);

    PVMFPortInterface* mlInPort = iMediaLayerNodeContainer->iInputPorts[0];
    mlExtIntf->setInPortReposFlag(mlInPort);

    //Check Track Config. If error is detected, MediaLayer send EOS.
    PVMFSMTrackInfoVector::iterator it;
    for (it = iTrackInfoVec.begin(); it != iTrackInfoVec.end(); it++)
    {
        if (it->iTrackDisable == true)
            mlExtIntf->setTrackDisable(it->iMediaLayerOutputPort);
    }

    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        int32 nodeTag = iNodeContainerVec[i].iNodeTag;

        if ((nodeTag == PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE) ||
                (nodeTag == PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE) ||
                (iInterfaceState == EPVMFNodePrepared && nodeTag != PVMF_STREAMING_MANAGER_SOCKET_NODE))
        {
            PVMFSMCommandContext* internalCmd = RequestNewInternalCmd();
            if (internalCmd != NULL)
            {
                internalCmd->cmd =
                    iNodeContainerVec[i].commandStartOffset +
                    PVMF_STREAMING_MANAGER_NODE_INTERNAL_START_CMD_OFFSET;
                internalCmd->parentCmd = PVMF_STREAMING_MANAGER_NODE_SET_DATASOURCE_POSITION;

                OsclAny *cmdContextData = OSCL_REINTERPRET_CAST(OsclAny*, internalCmd);

                PVMFNodeInterface* iNode = iNodeContainerVec[i].iNode;

                iNode->Start(iNodeContainerVec[i].iSessionId, cmdContextData);
                iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_PENDING;
            }
            else
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode:DoRepositioningStartMSHTTPStreaming:RequestNewInternalCmd - Failed"));
                return false;
            }
        }
    }
    return true;
}

void PVMFStreamingManagerNode::GetAcutalMediaTSAfterSeek()
{
    PVMFSMNodeContainer* iJitterBufferNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
    if (iJitterBufferNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFJitterBufferExtensionInterface* jbExtIntf =
        (PVMFJitterBufferExtensionInterface*)
        (iJitterBufferNodeContainer->iExtensions[0]);

    PVMFSMNodeContainer* iMediaLayerNodeContainer =
        getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
    if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
        (PVMFMediaLayerNodeExtensionInterface*)
        (iMediaLayerNodeContainer->iExtensions[0]);
    if (mlExtIntf == NULL) OSCL_LEAVE(OsclErrBadHandle);
    PVMFPortInterface* mlInPort = iMediaLayerNodeContainer->iInputPorts[0];

    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
    {
        iActualMediaDataTS = jbExtIntf->getActualMediaDataTSAfterSeek();
        if (iActualMediaDataTSPtr != NULL)
        {
            *iActualMediaDataTSPtr = iActualMediaDataTS;
            PVMF_SM_LOG_COMMAND_REPOS((0, "PVMFStreamingManagerNode::GetAcutalMediaTSAfterSeek - TargetNPT = %d, ActualNPT=%d, ActualMediaDataTS=%d",
                                       iRepositionRequestedStartNPTInMS, *iActualRepositionStartNPTInMSPtr, *iActualMediaDataTSPtr));
        }
        if (iPVMFDataSourcePositionParamsPtr != NULL)
        {
            iPVMFDataSourcePositionParamsPtr->iActualMediaDataTS = iActualMediaDataTS;
            PVMF_SM_LOG_COMMAND_REPOS((0, "PVMFStreamingManagerNode::GetAcutalMediaTSAfterSeek - ActualMediaDataTS=%d",
                                       iPVMFDataSourcePositionParamsPtr->iActualMediaDataTS));
        }
    }
}

PVMFCommandId
PVMFStreamingManagerNode::QueryDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aActualNPT,
        bool aSeekToSyncPoint,
        OsclAny* aContext)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryDataSourcePosition - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_QUERY_DATASOURCE_POSITION,
            aTargetNPT,
            &aActualNPT,
            aSeekToSyncPoint,
            aContext);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryDataSourcePosition - Out"));
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFStreamingManagerNode::QueryDataSourcePosition(PVMFSessionId aSessionId,
        PVMFTimestamp aTargetNPT,
        PVMFTimestamp& aSeekPointBeforeTargetNPT,
        PVMFTimestamp& aSeekPointAfterTargetNPT,
        OsclAny* aContext,
        bool aSeekToSyncPoint)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryDataSourcePosition - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_QUERY_DATASOURCE_POSITION,
            aTargetNPT,
            &aSeekPointBeforeTargetNPT,
            &aSeekPointAfterTargetNPT,
            aContext,
            aSeekToSyncPoint);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::QueryDataSourcePosition - Out"));
    return QueueCommandL(cmd);
}

void PVMFStreamingManagerNode::DoQueryDataSourcePosition(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoQueryDataSourcePosition - In"));

    PVMFTimestamp repositionrequestedstartnptinms = 0;
    PVMFTimestamp* actualrepositionstartnptinmsptr = NULL;
    bool seektosyncpoint = false;

    aCmd.PVMFStreamingManagerNodeCommand::Parse(repositionrequestedstartnptinms,
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
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoQueryDataSourcePosition - Out"));
    return;
}

PVMFCommandId
PVMFStreamingManagerNode::SetDataSourceRate(PVMFSessionId aSessionId,
        int32 aRate,
        OsclTimebase* aTimebase,
        OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aSessionId);
    OSCL_UNUSED_ARG(aRate);
    OSCL_UNUSED_ARG(aTimebase);
    OSCL_UNUSED_ARG(aContext);

    OSCL_LEAVE(OsclErrNotSupported);
    return 0;
}

void PVMFStreamingManagerNode::DoSetDataSourceRate(PVMFStreamingManagerNodeCommand& aCmd)
{
    CommandComplete(iInputCommands, aCmd, PVMFErrNotSupported);
}

uint32
PVMFStreamingManagerNode::GetNumMetadataKeys(char* aQueryKeyString)
{
    uint32 num_entries = 0;

    if (aQueryKeyString == NULL)
    {
        // No query key so just return all the available keys
        num_entries = iAvailableMetadataKeys.size();
    }
    else
    {
        // Determine the number of metadata keys based on the query key string provided
        uint32 i;
        for (i = 0; i < iAvailableMetadataKeys.size(); i++)
        {
            // Check if the key matches the query key
            if (pv_mime_strcmp(iAvailableMetadataKeys[i].get_cstr(), aQueryKeyString) >= 0)
            {
                num_entries++;
            }
        }
        for (i = 0; i < iCPMMetadataKeys.size(); i++)
        {
            /* Check if the key matches the query key */
            if (pv_mime_strcmp(iCPMMetadataKeys[i].get_cstr(),
                               aQueryKeyString) >= 0)
            {
                num_entries++;
            }
        }
    }
    return num_entries;
}

uint32
PVMFStreamingManagerNode::GetNumMetadataValues(PVMFMetadataList& aKeyList)
{
    uint32 numkeys = aKeyList.size();

    if (numkeys == 0)
    {
        return 0;
    }

    SDPInfo* sdpInfo = NULL;
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();
        if (sdpInfo == NULL)
        {
            return 0;
        }
    }
    // Get Num Tracks
    uint32 numtracks = iMetaDataInfo.iNumTracks;

    uint32 numvalentries = 0;
    for (uint32 lcv = 0; lcv < numkeys; lcv++)
    {
        if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_AUTHOR_KEY) == 0 &&
                iMetaDataInfo.iAuthorPresent)
        {
            // Author
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_ARTIST_KEY) == 0 &&
                 iMetaDataInfo.iPerformerPresent)
        {
            // Artist/performer
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TITLE_KEY) == 0 &&
                 iMetaDataInfo.iTitlePresent)
        {
            // Title
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_DESCRIPTION_KEY) == 0 &&
                 iMetaDataInfo.iDescriptionPresent)
        {
            // Description
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_RATING_KEY) == 0 &&
                 iMetaDataInfo.iRatingPresent)
        {
            // Rating
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_COPYRIGHT_KEY) == 0 &&
                 iMetaDataInfo.iCopyRightPresent)
        {
            // Copyright
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_GENRE_KEY) == 0 &&
                 iMetaDataInfo.iGenrePresent)
        {
            // Genre
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_LYRICS_KEY) == 0 &&
                 iMetaDataInfo.iLyricsPresent)
        {
            // Lyrics
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_NUM_GRAPHICS_KEY) == 0 &&
                 iMetaDataInfo.iWMPicturePresent)
        {
            // Num Picture
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_GRAPHICS_KEY) == 0 &&
                 iMetaDataInfo.iWMPicturePresent)
        {
            // Picture
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_CLASSIFICATION_KEY) == 0 &&
                 iMetaDataInfo.iClassificationPresent)
        {
            // Classification
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_KEYWORDS_KEY) == 0 &&
                 iMetaDataInfo.iKeyWordsPresent)
        {
            // Keywords
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_LOCATION_KEY) == 0 &&
                 iMetaDataInfo.iLocationPresent)
        {
            // Location
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_DURATION_KEY) == 0)
        {
            // Session Duration
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_NUMTRACKS_KEY) == 0 &&
                 numtracks > 0)
        {
            // Number of tracks
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_RANDOM_ACCESS_DENIED_KEY) == 0)
        {
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strcmp(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_CLIP_TYPE_KEY) == 0)
        {
            // Increment the counter for the number of values found so far
            ++numvalentries;
        }
        else if (oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_TYPE_KEY) != NULL)
        {
            // Track type
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = numtracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= (uint32)numtracks || endindex >= (uint32)numtracks)
            {
                continue;
            }
            // Increment the counter for the number of values found so far
            numvalentries += (endindex + 1 - startindex);
        }
        else if (oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_DURATION_KEY) != NULL)
        {
            // Track duration
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = numtracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= (uint32)numtracks || endindex >= (uint32)numtracks)
            {
                continue;
            }

            // Increment the counter for the number of values found so far
            numvalentries += (endindex + 1 - startindex);
        }
        else if (oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_BITRATE_KEY) != NULL)
        {
            // Track bitrate
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = numtracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= (uint32)numtracks || endindex >= (uint32)numtracks)
            {
                continue;
            }
            // Increment the counter for the number of values found so far
            numvalentries += (endindex + 1 - startindex);
        }
        else if (oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_MAX_BITRATE_KEY) != NULL)
        {
            // Track bitrate
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = numtracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= (uint32)numtracks || endindex >= (uint32)numtracks)
            {
                continue;
            }
            // Increment the counter for the number of values found so far
            numvalentries += (endindex + 1 - startindex);
        }
        else if (oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_SELECTED_KEY) != NULL)
        {
            // Track selected
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = numtracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= (uint32)numtracks || endindex >= (uint32)numtracks)
            {
                continue;
            }
            // Increment the counter for the number of values found so far
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_NAME_KEY) != NULL))
        {
            /*
             * Codec Name
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }

            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DESCRIPTION_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DATA_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_WIDTH_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_HEIGHT_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_FRAME_RATE_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_SAMPLERATE_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_NUMCHANNELS_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_AUDIO_BITS_PER_SAMPLE_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else if ((oscl_strstr(aKeyList[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_TRACKID_KEY) != NULL))
        {
            /*
             * Codec Description
             * Determine the index requested.
             */
            uint32 startindex = 0;
            uint32 endindex   = 0;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr(aKeyList[lcv].get_cstr(),
                                         PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if ((startindex > endindex) ||
                    (startindex >= (uint32)numtracks) ||
                    (endindex >= (uint32)numtracks))
            {
                continue;
            }
            numvalentries += (endindex + 1 - startindex);
        }
        else
        {
            /* Check the extended meta data list */
            for (uint32 i = 0; i < iMetaDataInfo.iExtendedMetaDataNameVec.size(); i++)
            {
                OSCL_HeapString<PVMFStreamingManagerNodeAllocator> extMetaDataName =
                    iMetaDataInfo.iExtendedMetaDataNameVec[i];
                if (oscl_strcmp(aKeyList[lcv].get_cstr(), extMetaDataName.get_cstr()) == 0)
                {
                    /*
                     * Increment the counter for the number of values found so far
                     */
                    ++numvalentries;
                }
            }
        }
    }
    return numvalentries; // Number of elements
}

PVMFStatus PVMFStreamingManagerNode::InitMetaData()
{
    // Init Metadata
    {
        // Clear out the existing key list
        iAvailableMetadataKeys.clear();
        iCPMMetadataKeys.clear();

        int32 leavecode = 0;

        if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
                (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL) ||
                (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
                (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
        {
            // Get the SDP info
            SDPInfo* sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();
            if (sdpInfo == NULL)
            {
                return PVMFErrInvalidState;
            }

            // Get Asset Info
            sessionDescription* sessionInfo = sdpInfo->getSessionInfo();
            if (sessionInfo != NULL)
            {
                iMetaDataInfo.iRandomAccessDenied = sessionInfo->getRandomAccessDenied();

                AssetInfoType assetInfo = sessionInfo->getAssetInfo();

                iMetaDataInfo.iTitlePresent = assetInfo.oTitlePresent;
                iMetaDataInfo.iDescriptionPresent = assetInfo.oDescriptionPresent;
                iMetaDataInfo.iCopyRightPresent = assetInfo.oCopyRightPresent;
                iMetaDataInfo.iPerformerPresent = assetInfo.oPerformerPresent;
                iMetaDataInfo.iAuthorPresent = assetInfo.oAuthorPresent;
                iMetaDataInfo.iGenrePresent = assetInfo.oGenrePresent;
                iMetaDataInfo.iRatingPresent = assetInfo.oRatingPresent;
                iMetaDataInfo.iClassificationPresent = assetInfo.oClassificationPresent;
                iMetaDataInfo.iKeyWordsPresent = assetInfo.oKeyWordsPresent;
                iMetaDataInfo.iLocationPresent = assetInfo.oLocationPresent;

                if (iMetaDataInfo.iTitlePresent)
                {
                    iMetaDataInfo.iTitle = assetInfo.Box[AssetInfoType::TITLE];
                }
                if (iMetaDataInfo.iDescriptionPresent)
                {
                    iMetaDataInfo.iDescription = assetInfo.Box[AssetInfoType::DESCRIPTION];
                }
                if (iMetaDataInfo.iCopyRightPresent)
                {
                    iMetaDataInfo.iCopyright = assetInfo.Box[AssetInfoType::COPYRIGHT];
                }
                if (iMetaDataInfo.iPerformerPresent)
                {
                    iMetaDataInfo.iPerformer = assetInfo.Box[AssetInfoType::PERFORMER];
                }
                if (iMetaDataInfo.iAuthorPresent)
                {
                    iMetaDataInfo.iAuthor = assetInfo.Box[AssetInfoType::AUTHOR];
                }
                if (iMetaDataInfo.iRatingPresent)
                {
                    iMetaDataInfo.iRating = assetInfo.Box[AssetInfoType::RATING];
                }
                if (iMetaDataInfo.iClassificationPresent)
                {
                    iMetaDataInfo.iClassification = assetInfo.Box[AssetInfoType::CLASSIFICATION];
                }
                if (iMetaDataInfo.iKeyWordsPresent)
                {
                    iMetaDataInfo.iKeyWords = assetInfo.Box[AssetInfoType::KEYWORDS];
                }
                if (iMetaDataInfo.iLocationPresent)
                {
                    iMetaDataInfo.iLocation = assetInfo.Box[AssetInfoType::LOCATION];
                }

                RtspRangeType *sessionRange = OSCL_CONST_CAST(RtspRangeType*, (sessionInfo->getRange()));
                if (sessionRange->end_is_set == true)
                {
                    iMetaDataInfo.iSessionDurationAvailable = true;

                    int32 sessionStartTime = 0, sessionStopTime = 0;
                    sessionRange->convertToMilliSec(sessionStartTime, sessionStopTime);
                    uint32 duration = 0;
                    if (sessionStopTime > sessionStartTime && sessionStartTime >= 0)
                    {
                        duration = (uint32)(sessionStopTime - sessionStartTime);
                    }
                    Oscl_Int64_Utils::set_uint64(iMetaDataInfo.iSessionDuration, 0, duration);
                    iMetaDataInfo.iSessionDurationTimeScale = 1000;
                }

            }
            iMetaDataInfo.iNumTracks = sdpInfo->getNumMediaObjects();

            for (uint32 i = 0; i < iMetaDataInfo.iNumTracks; i++)
            {
                Oscl_Vector<mediaInfo*, SDPParserAlloc> mediaInfoVec = sdpInfo->getMediaInfo(i);
                for (uint32 j = 0; j < mediaInfoVec.size(); ++j)
                {
                    mediaInfo* mInfo = mediaInfoVec[j];
                    if (mInfo != NULL)
                    {
                        PVMFSMTrackMetaDataInfo trackMetaDataInfo;

                        trackMetaDataInfo.iTrackID = mInfo->getMediaInfoID();
                        const char* mimeType = mInfo->getMIMEType();
                        OSCL_StackString<32> h263(_STRLIT_CHAR("H263"));
                        {
                            trackMetaDataInfo.iMimeType += mimeType;
                        }

                        Oscl_Vector<PayloadSpecificInfoTypeBase*, SDPParserAlloc> payloadVector;
                        payloadVector = mInfo->getPayloadSpecificInfoVector();
                        if (payloadVector.size() != 0)
                        {
                            /*
                             * There can be multiple payloads per media segment.
                             * We only support one for now, so
                             * use just the first payload
                             */
                            PayloadSpecificInfoTypeBase* payloadInfo = payloadVector[0];
                            if (oscl_strstr(mimeType, h263.get_cstr()) != NULL)
                            {
                                H263PayloadSpecificInfoType* h263PayloadInfo =
                                    OSCL_STATIC_CAST(H263PayloadSpecificInfoType*, payloadInfo);
                                trackMetaDataInfo.iTrackWidth = h263PayloadInfo->getFrameWidth();
                                trackMetaDataInfo.iTrackHeight = h263PayloadInfo->getFrameHeight();
                            }
                        }

                        trackMetaDataInfo.iTrackBitRate = (uint32)(mInfo->getBitrate());

                        RtspRangeType *mediaRange = mInfo->getRtspRange();
                        if (mediaRange->end_is_set == true)
                        {
                            int32 mediaStartTime = 0, mediaStopTime = 0;
                            mediaRange->convertToMilliSec(mediaStartTime, mediaStopTime);
                            uint32 trackduration = 0;
                            if (mediaStopTime > mediaStartTime && mediaStartTime >= 0)
                            {
                                trackduration = (uint32)(mediaStopTime - mediaStartTime);
                            }
                            uint64 trackduration64 = 0;
                            Oscl_Int64_Utils::set_uint64(trackduration64, 0, trackduration);
                            trackMetaDataInfo.iTrackDuration = trackduration64;
                            trackMetaDataInfo.iTrackDurationTimeScale = 1000;
                            trackMetaDataInfo.iTrackDurationAvailable = true;
                        }
                        else
                        {
                            trackMetaDataInfo.iTrackDurationAvailable = false;
                        }
                        iMetaDataInfo.iTrackMetaDataInfoVec.push_back(trackMetaDataInfo);
                    }
                }
            }
        }
        leavecode = 0;
        OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_DURATION_KEY));

        if (iMetaDataInfo.iTitlePresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_TITLE_KEY));
        }
        if (iMetaDataInfo.iDescriptionPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_DESCRIPTION_KEY));
        }
        if (iMetaDataInfo.iCopyRightPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_COPYRIGHT_KEY));
        }
        if (iMetaDataInfo.iPerformerPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_ARTIST_KEY));
        }
        if (iMetaDataInfo.iAuthorPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_AUTHOR_KEY));
        }
        if (iMetaDataInfo.iGenrePresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_GENRE_KEY));
        }
        if (iMetaDataInfo.iLyricsPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_LYRICS_KEY));
        }
        if (iMetaDataInfo.iWMPicturePresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_NUM_GRAPHICS_KEY));
        }
        if (iMetaDataInfo.iWMPicturePresent)
        {
            // Create the parameter string for the index range
            char indexparam[18];
            oscl_snprintf(indexparam, 18, ";index=0...%d", (iMetaDataInfo.iWMPictureIndexVec.size() - 1));
            indexparam[17] = NULL_TERM_CHAR;

            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_GRAPHICS_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);
        }
        if (iMetaDataInfo.iRatingPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_RATING_KEY));
        }
        if (iMetaDataInfo.iClassificationPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_CLASSIFICATION_KEY));
        }
        if (iMetaDataInfo.iKeyWordsPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_KEYWORDS_KEY));
        }
        if (iMetaDataInfo.iLocationPresent)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_LOCATION_KEY));
        }

        leavecode = 0;
        OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_RANDOM_ACCESS_DENIED_KEY));

        leavecode = 0;
        OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_CLIP_TYPE_KEY));

        if (iMetaDataInfo.iNumTracks > 0)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(PVMFSTREAMINGMGRNODE_NUMTRACKS_KEY));

            // Create the parameter string for the index range
            char indexparam[18];
            oscl_snprintf(indexparam, 18, ";index=0...%d", (iMetaDataInfo.iNumTracks - 1));
            indexparam[17] = NULL_TERM_CHAR;

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_TYPE_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_DURATION_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_SELECTED_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_NAME_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DESCRIPTION_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DATA_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            leavecode = 0;
            OSCL_TRY(leavecode,
                     iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_TRACKID_KEY);
                     iAvailableMetadataKeys[0] += indexparam;);

            if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
                    (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL) ||
                    (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE))
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_BITRATE_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
        }

        uint32 i;
        for (i = 0; i < iMetaDataInfo.iExtendedMetaDataDescriptorCount; i++)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, iAvailableMetadataKeys.push_back(iMetaDataInfo.iExtendedMetaDataNameVec[i]));
        }

        for (i = 0; i < iMetaDataInfo.iTrackMetaDataInfoVec.size(); i++)
        {
            PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

            char indexparam[18];
            oscl_snprintf(indexparam, 18, ";index=%d", (i));
            indexparam[17] = NULL;

            if (trackMetaDataInfo.iTrackWidth > 0)
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_WIDTH_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
            if (trackMetaDataInfo.iTrackHeight > 0)
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_HEIGHT_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
            if (trackMetaDataInfo.iVideoFrameRate > 0)
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_FRAME_RATE_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
            if (trackMetaDataInfo.iAudioSampleRate > 0)
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_SAMPLERATE_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
            if (trackMetaDataInfo.iAudioNumChannels > 0)
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_NUMCHANNELS_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
            if (trackMetaDataInfo.iAudioBitsPerSample > 0)
            {
                leavecode = 0;
                OSCL_TRY(leavecode,
                         iAvailableMetadataKeys.push_front(PVMFSTREAMINGMGRNODE_TRACKINFO_AUDIO_BITS_PER_SAMPLE_KEY);
                         iAvailableMetadataKeys[0] += indexparam;);
            }
        }
        return PVMFSuccess;
    }
}

PVMFStatus
PVMFStreamingManagerNode::DoGetMetadataKeys(PVMFStreamingManagerNodeCommand& aCmd)
{
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        SDPInfo* sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();
        if (sdpInfo == NULL)
        {
            return PVMFErrInvalidState;
        }
    }
    return (CompleteGetMetadataKeys(aCmd));
}

PVMFStatus
PVMFStreamingManagerNode::CompleteGetMetadataKeys(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMFMetadataList* keylistptr = NULL;
    uint32 starting_index;
    int32 max_entries;
    char* query_key = NULL;

    aCmd.PVMFStreamingManagerNodeCommand::Parse(keylistptr, starting_index, max_entries, query_key);

    // Check parameters
    if (keylistptr == NULL)
    {
        // The list pointer is invalid
        return PVMFErrArgument;
    }

    if ((starting_index > (iAvailableMetadataKeys.size() - 1)) || max_entries == 0)
    {
        // Invalid starting index and/or max entries
        return PVMFErrArgument;
    }

    // Copy the requested keys
    uint32 num_entries = 0;
    int32 num_added = 0;
    int32 leavecode = 0;
    uint32 lcv = 0;
    for (lcv = 0; lcv < iAvailableMetadataKeys.size(); lcv++)
    {
        if (query_key == NULL)
        {
            // No query key so this key is counted
            ++num_entries;
            if (num_entries > starting_index)
            {
                // Past the starting index so copy the key
                leavecode = 0;
                OSCL_TRY(leavecode, keylistptr->push_back(iAvailableMetadataKeys[lcv]));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::DoGetMetadataKeys() Memory allocation failure when copying metadata key"));
                                     return PVMFErrNoMemory);
                num_added++;
            }
        }
        else
        {
            // Check if the key matche the query key
            if (pv_mime_strcmp(iAvailableMetadataKeys[lcv].get_cstr(), query_key) >= 0)
            {
                // This key is counted
                ++num_entries;
                if (num_entries > starting_index)
                {
                    // Past the starting index so copy the key
                    leavecode = 0;
                    OSCL_TRY(leavecode, keylistptr->push_back(iAvailableMetadataKeys[lcv]));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::DoGetMetadataKeys() Memory allocation failure when copying metadata key"));
                                         return PVMFErrNoMemory);
                    num_added++;
                }
            }
        }
    }
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
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::CompleteGetMetadataKeys() Memory allocation failure when copying metadata key"));
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
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::CompleteGetMetadataKeys() Memory allocation failure when copying metadata key"));
                                         return PVMFErrNoMemory);
                    num_added++;
                }
            }
        }
        // Check if max number of entries have been copied
        if (max_entries > 0 && num_added >= max_entries)
        {
            break;
        }
    }

    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::GetIndexParamValues(char* aString, uint32& aStartIndex, uint32& aEndIndex)
{
    // This parses a string of the form "index=N1...N2" and extracts the integers N1 and N2.
    // If string is of the format "index=N1" then N2=N1

    if (aString == NULL)
    {
        return PVMFErrArgument;
    }

    // Go to end of "index="
    char* n1string = aString + 6;

    PV_atoi(n1string, 'd', oscl_strlen(n1string), aStartIndex);

    char* n2string = oscl_strstr(aString, _STRLIT_CHAR("..."));

    if (n2string == NULL)
    {
        aEndIndex = aStartIndex;
    }
    else
    {
        // Go to end of "index=N1..."
        n2string += 3;

        PV_atoi(n2string, 'd', oscl_strlen(n2string), aEndIndex);
    }

    return PVMFSuccess;
}

PVMFStatus
PVMFStreamingManagerNode::DoGetMetadataValues(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFStreamingManagerNode::DoGetMetadataValues() In"));

    SDPInfo* sdpInfo = NULL;
    if ((iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTSP_URL) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_SDP_FILE) ||
            (iSessionSourceInfo->_sessionType == PVMF_DATA_SOURCE_RTP_PACKET_SOURCE))
    {
        sdpInfo = iSessionSourceInfo->_sdpInfo.GetRep();
        if (sdpInfo == NULL)
        {
            return PVMFErrInvalidState;
        }
    }
    PVMFMetadataList* keylistptr = NULL;
    Oscl_Vector<PvmiKvp, OsclMemAllocator>* valuelistptr = NULL;
    uint32 starting_index;
    int32 max_entries;

    aCmd.PVMFStreamingManagerNodeCommand::Parse(keylistptr, valuelistptr, starting_index, max_entries);

    // Check the parameters
    if (keylistptr == NULL || valuelistptr == NULL)
    {
        return PVMFErrArgument;
    }

    uint32 numkeys = keylistptr->size();

    if (starting_index > (numkeys - 1) || numkeys <= 0 || max_entries == 0)
    {
        // Don't do anything
        return PVMFErrArgument;
    }

    uint32 numvalentries = 0;
    int32 numentriesadded = 0;
    for (uint32 lcv = 0; lcv < numkeys; lcv++)
    {
        int32 leavecode = 0;
        PvmiKvp KeyVal;
        KeyVal.key = NULL;
        KeyVal.value.pWChar_value = NULL;
        KeyVal.value.pChar_value = NULL;

        if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_AUTHOR_KEY) == 0 &&
                iMetaDataInfo.iAuthorPresent)
        {
            // Author
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsAuthorUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_AUTHOR_KEY,
                                                         iMetaDataInfo.iAuthor.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_AUTHOR_KEY,
                             iMetaDataInfo.iAuthorUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_ARTIST_KEY) == 0 &&
                 iMetaDataInfo.iPerformerPresent)
        {
            // Artist/performer
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsPerformerUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_ARTIST_KEY,
                                                         iMetaDataInfo.iPerformer.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_ARTIST_KEY,
                             iMetaDataInfo.iPerformerUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TITLE_KEY) == 0 &&
                 iMetaDataInfo.iTitlePresent)
        {
            // Title
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsTitleUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_TITLE_KEY,
                                                         iMetaDataInfo.iTitle.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_TITLE_KEY,
                             iMetaDataInfo.iTitleUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_DESCRIPTION_KEY) == 0 &&
                 iMetaDataInfo.iDescriptionPresent)
        {
            // Description
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsDescriptionUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_DESCRIPTION_KEY,
                                                         iMetaDataInfo.iDescription.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_DESCRIPTION_KEY,
                             iMetaDataInfo.iDescriptionUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_RATING_KEY) == 0 &&
                 iMetaDataInfo.iRatingPresent)
        {
            // Rating
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsRatingUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_RATING_KEY,
                                                         iMetaDataInfo.iRating.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_RATING_KEY,
                             iMetaDataInfo.iRatingUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_COPYRIGHT_KEY) == 0 &&
                 iMetaDataInfo.iCopyRightPresent)
        {
            // Copyright
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsCopyRightUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_COPYRIGHT_KEY,
                                                         iMetaDataInfo.iCopyright.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_COPYRIGHT_KEY,
                             iMetaDataInfo.iCopyrightUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_GENRE_KEY) == 0 &&
                 iMetaDataInfo.iGenrePresent)
        {
            // Genre
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsGenreUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_GENRE_KEY,
                                                         iMetaDataInfo.iGenre.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_GENRE_KEY,
                             iMetaDataInfo.iGenreUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_LYRICS_KEY) == 0 &&
                 iMetaDataInfo.iLyricsPresent)
        {
            // Lyrics
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsLyricsUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_LYRICS_KEY,
                                                         iMetaDataInfo.iLyrics.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_LYRICS_KEY,
                             iMetaDataInfo.iLyricsUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_NUM_GRAPHICS_KEY) == 0 &&
                 iMetaDataInfo.iWMPicturePresent)
        {
            /*
             * Num Picture
             * Increment the counter for the number of values found so far
             */
            ++numvalentries;

            /* Create a value entry if past the starting index */
            if (numvalentries > (uint32)starting_index)
            {
                PVMFStatus retval =
                    PVMFCreateKVPUtils::CreateKVPForUInt32Value(KeyVal,
                            PVMFSTREAMINGMGRNODE_NUM_GRAPHICS_KEY,
                            iMetaDataInfo.iNumWMPicture,
                            NULL);
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_CLASSIFICATION_KEY) == 0 &&
                 iMetaDataInfo.iClassificationPresent)
        {
            // Classification
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsClassificationUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_CLASSIFICATION_KEY,
                                                         iMetaDataInfo.iClassification.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_CLASSIFICATION_KEY,
                             iMetaDataInfo.iClassificationUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_KEYWORDS_KEY) == 0 &&
                 iMetaDataInfo.iKeyWordsPresent)
        {
            // Keywords
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsKeyWordsUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_KEYWORDS_KEY,
                                                         iMetaDataInfo.iKeyWords.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_KEYWORDS_KEY,
                             iMetaDataInfo.iKeyWordUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_LOCATION_KEY) == 0 &&
                 iMetaDataInfo.iLocationPresent)
        {
            // Location
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval;
                if (iMetaDataInfo.iIsLocationUnicode == false)
                {
                    retval = CreateKVPForCharStringValue(KeyVal,
                                                         PVMFSTREAMINGMGRNODE_LOCATION_KEY,
                                                         iMetaDataInfo.iLocation.get_cstr());
                }
                else
                {
                    retval = CreateKVPForWideCharStringValue(KeyVal,
                             PVMFSTREAMINGMGRNODE_LOCATION_KEY,
                             iMetaDataInfo.iLocationUnicode.get_cstr());
                }
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_DURATION_KEY) == 0)
        {
            // Session Duration
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                if (iMetaDataInfo.iSessionDurationAvailable == true)
                {
                    uint32 duration = Oscl_Int64_Utils::get_uint64_lower32(iMetaDataInfo.iSessionDuration);
                    char timescalestr[20];
                    oscl_snprintf(timescalestr, 20, ";%s%d", PVMFSTREAMINGMGRNODE_TIMESCALE, iMetaDataInfo.iSessionDurationTimeScale);
                    timescalestr[19] = NULL_TERM_CHAR;
                    PVMFStatus retval = CreateKVPForUInt32Value(KeyVal,
                                        PVMFSTREAMINGMGRNODE_DURATION_KEY,
                                        duration,
                                        timescalestr);
                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }
                }
                else
                {
                    OSCL_StackString<32> unknownDuration(_STRLIT_CHAR("unknown"));
                    PVMFStatus retval = CreateKVPForCharStringValue(KeyVal,
                                        PVMFSTREAMINGMGRNODE_DURATION_KEY,
                                        unknownDuration.get_cstr());
                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_NUMTRACKS_KEY) == 0 &&
                 iMetaDataInfo.iNumTracks > 0)
        {
            // Number of tracks
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval = CreateKVPForUInt32Value(KeyVal,
                                    PVMFSTREAMINGMGRNODE_NUMTRACKS_KEY,
                                    iMetaDataInfo.iNumTracks);
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_RANDOM_ACCESS_DENIED_KEY) == 0)
        {
            // random access
            // Increment the counter for the number of values found so far
            ++numvalentries;

            // Create a value entry if past the starting index
            if (numvalentries > starting_index)
            {
                PVMFStatus retval = CreateKVPForBoolValue(KeyVal,
                                    PVMFSTREAMINGMGRNODE_RANDOM_ACCESS_DENIED_KEY,
                                    iMetaDataInfo.iRandomAccessDenied);
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }
            }
        }
        else if (oscl_strcmp((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_CLIP_TYPE_KEY) == 0)
        {
            /*
             * Clip Type
             * Increment the counter for the number of values found so far
             */
            ++numvalentries;

            /* Create a value entry if past the starting index */
            if (numvalentries > (uint32)starting_index)
            {
                uint32 len = 0;
                char* clipType = NULL;
                len = oscl_strlen("streaming");
                clipType = OSCL_ARRAY_NEW(char, len + 1);
                oscl_memset(clipType, 0, len + 1);
                oscl_strncpy(clipType, ("streaming"), len);
                PVMFStatus retval =
                    PVMFCreateKVPUtils::CreateKVPForCharStringValue(KeyVal,
                            PVMFSTREAMINGMGRNODE_CLIP_TYPE_KEY,
                            clipType);

                OSCL_ARRAY_DELETE(clipType);
                if (retval != PVMFSuccess && retval != PVMFErrArgument)
                {
                    break;
                }

            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_TYPE_KEY) != NULL)
        {
            // Track type

            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex   = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;
                    trackkvp.value.pChar_value = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL_TERM_CHAR;

                        const char* mimeType = trackMetaDataInfo.iMimeType.get_cstr();
                        retval = CreateKVPForCharStringValue(trackkvp,
                                                             PVMFSTREAMINGMGRNODE_TRACKINFO_TYPE_KEY,
                                                             mimeType,
                                                             indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            if (trackkvp.value.pChar_value != NULL)
                            {
                                OSCL_ARRAY_DELETE(trackkvp.value.pChar_value);
                                trackkvp.value.pChar_value = NULL;
                            }

                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_DURATION_KEY) != NULL)
        {
            // Track duration

            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        if (trackMetaDataInfo.iTrackDurationAvailable == true)
                        {
                            char indextimescaleparam[36];
                            oscl_snprintf(indextimescaleparam, 36, ";%s%d;%s%d", PVMFSTREAMINGMGRNODE_INDEX, i, PVMFSTREAMINGMGRNODE_TIMESCALE, trackMetaDataInfo.iTrackDurationTimeScale);
                            indextimescaleparam[35] = NULL_TERM_CHAR;

                            uint32 trackduration =
                                Oscl_Int64_Utils::get_uint64_lower32(trackMetaDataInfo.iTrackDuration);
                            retval = CreateKVPForUInt32Value(trackkvp,
                                                             PVMFSTREAMINGMGRNODE_TRACKINFO_DURATION_KEY,
                                                             trackduration,
                                                             indextimescaleparam);
                        }
                        else
                        {
                            char indextimescaleparam[36];
                            oscl_snprintf(indextimescaleparam, 36, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                            indextimescaleparam[35] = NULL_TERM_CHAR;

                            OSCL_StackString<32> unknownDuration(_STRLIT_CHAR("unknown"));
                            PVMFStatus retval = CreateKVPForCharStringValue(trackkvp,
                                                PVMFSTREAMINGMGRNODE_TRACKINFO_DURATION_KEY,
                                                unknownDuration.get_cstr(),
                                                indextimescaleparam);
                            if (retval != PVMFSuccess && retval != PVMFErrArgument)
                            {
                                break;
                            }
                        }

                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_BITRATE_KEY) != NULL)
        {
            // Track bitrate

            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL_TERM_CHAR;

                        uint32 trackbitrate = trackMetaDataInfo.iTrackBitRate;
                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_BITRATE_KEY,
                                                         trackbitrate,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_MAX_BITRATE_KEY) != NULL)
        {
            // Track bitrate

            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL_TERM_CHAR;

                        uint32 trackbitrate = trackMetaDataInfo.iTrackMaxBitRate;
                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_MAX_BITRATE_KEY,
                                                         trackbitrate,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_SELECTED_KEY) != NULL)
        {
            // Track bitrate

            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);

                        indexparam[15] = NULL_TERM_CHAR;

                        bool selected = trackMetaDataInfo.iTrackSelected;
                        retval = CreateKVPForBoolValue(trackkvp,
                                                       PVMFSTREAMINGMGRNODE_TRACKINFO_SELECTED_KEY,
                                                       selected,
                                                       indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_WIDTH_KEY) != NULL)
        {
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL;

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_WIDTH_KEY,
                                                         trackMetaDataInfo.iTrackWidth,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_HEIGHT_KEY) != NULL)
        {
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL;

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_HEIGHT_KEY,
                                                         trackMetaDataInfo.iTrackHeight,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_FRAME_RATE_KEY) != NULL)
        {
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL;

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_FRAME_RATE_KEY,
                                                         trackMetaDataInfo.iVideoFrameRate,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_SAMPLERATE_KEY) != NULL)
        {
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL;

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_SAMPLERATE_KEY,
                                                         trackMetaDataInfo.iAudioSampleRate,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_NUMCHANNELS_KEY) != NULL)
        {
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL;

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_NUMCHANNELS_KEY,
                                                         trackMetaDataInfo.iAudioNumChannels,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_AUDIO_BITS_PER_SAMPLE_KEY) != NULL)
        {
            // Determine the index requested. Default to all tracks
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            // Check if the index parameter is present
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                // Retrieve the index values
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            // Validate the indices
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }

            // Return a KVP for each index
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;

                    // Increment the counter for the number of values found so far
                    ++numvalentries;
                    // Add the value entry if past the starting index
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > starting_index)
                    {
                        char indexparam[16];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);
                        indexparam[15] = NULL;

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_AUDIO_BITS_PER_SAMPLE_KEY,
                                                         trackMetaDataInfo.iAudioBitsPerSample,
                                                         indexparam);
                    }

                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }

                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_NAME_KEY) != NULL)
        {
            /* Codec Name */
            /* Determine the index requested. Default to all tracks */
            uint32 startindex = 0;
            uint32 endindex = iMetaDataInfo.iNumTracks - 1;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }
            /* Return a KVP for each index */
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {

                    PVMFSMTrackMetaDataInfo trackMetaDataInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];

                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;
                    /* Increment the counter for the number of values found so far */
                    ++numvalentries;
                    /* Add the value entry if past the starting index */
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > (uint32)starting_index)
                    {
                        char indexparam[29];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);

                        char* maxsizestr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_MAXSIZE);
                        char* truncatestr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRUNCATE_FLAG);
                        uint32 maxSize = 0xFFFFFFFF;
                        if (maxsizestr != NULL)
                        {
                            if (GetMaxSizeValue(maxsizestr, maxSize) != PVMFSuccess)
                            {
                                break;
                            }
                        }

                        uint32 truncateFlag = true;

                        if (truncatestr != NULL)
                        {
                            if (GetTruncateFlagValue(truncatestr, truncateFlag) != PVMFSuccess)
                            {
                                break;
                            }
                        }

                        uint32 codecNameLen = trackMetaDataInfo.iCodecName.get_size();
                        char maxsizemessage[13];
                        maxsizemessage[0] = '\0';
                        if (maxSize < codecNameLen)

                        {
                            if (!truncateFlag)
                            {
                                oscl_snprintf(maxsizemessage, 13, ";%s%d", PVMFSTREAMINGMGRNODE_REQSIZE, codecNameLen);
                                oscl_strncat(indexparam, maxsizemessage, oscl_strlen(maxsizemessage));
                            }
                        }

                        retval =
                            PVMFCreateKVPUtils::CreateKVPForWStringValue(trackkvp,
                                    PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_NAME_KEY,
                                    (trackMetaDataInfo.iCodecName),
                                    indexparam, maxSize, truncateFlag);
                    }
                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }
                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }

                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DESCRIPTION_KEY) != NULL)
        {
            /* Codec Description */
            /* Determine the index requested. Default to all tracks */
            uint32 startindex = 0;
            uint32 endindex = (uint32)iMetaDataInfo.iNumTracks - 1;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }
            /* Return a KVP for each index */
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];
                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;
                    /* Increment the counter for the number of values found so far */
                    ++numvalentries;
                    /* Add the value entry if past the starting index */
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > (uint32)starting_index)
                    {
                        char indexparam[29];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);

                        char* maxsizestr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_MAXSIZE);
                        char* truncatestr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRUNCATE_FLAG);
                        uint32 maxSize = 0xFFFFFFFF;
                        if (maxsizestr != NULL)
                        {
                            if (GetMaxSizeValue(maxsizestr, maxSize) != PVMFSuccess)
                            {
                                break;
                            }
                        }

                        uint32 truncateFlag = true;

                        if (truncatestr != NULL)
                        {
                            if (GetTruncateFlagValue(truncatestr, truncateFlag) != PVMFSuccess)
                            {
                                break;
                            }
                        }

                        uint32 codecDescLen = trackInfo.iCodecDescription.get_size();
                        char maxsizemessage[13];
                        maxsizemessage[0] = '\0';
                        if (maxSize < codecDescLen)
                        {
                            if (!truncateFlag)
                            {
                                oscl_snprintf(maxsizemessage, 13, ";%s%d", PVMFSTREAMINGMGRNODE_REQSIZE, codecDescLen);
                                oscl_strncat(indexparam, maxsizemessage, oscl_strlen(maxsizemessage));
                            }
                        }

                        retval =
                            PVMFCreateKVPUtils::CreateKVPForWStringValue(trackkvp,
                                    PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DESCRIPTION_KEY,
                                    (trackInfo.iCodecDescription),
                                    indexparam, maxSize, truncateFlag);
                    }
                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }
                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DATA_KEY) != NULL)
        {
            /* Codec Description */
            /* Determine the index requested. Default to all tracks */
            uint32 startindex = 0;
            uint32 endindex = (uint32)iMetaDataInfo.iNumTracks - 1;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }
            /* Return a KVP for each index */
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];
                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;
                    /* Increment the counter for the number of values found so far */
                    ++numvalentries;
                    /* Add the value entry if past the starting index */
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > (uint32)starting_index)
                    {
                        char indexparam[29];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);

                        retval =
                            PVMFCreateKVPUtils::CreateKVPForByteArrayValue(trackkvp,
                                    PVMFSTREAMINGMGRNODE_TRACKINFO_CODEC_DATA_KEY,
                                    (uint8*)(trackInfo.iCodecSpecificInfo.getMemFragPtr()),
                                    (uint32)trackInfo.iCodecSpecificInfo.getMemFragSize()
                                    , indexparam);
                    }
                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }
                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        else if (oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_TRACKINFO_TRACKID_KEY) != NULL)
        {
            /* Codec Description */
            /* Determine the index requested. Default to all tracks */
            uint32 startindex = 0;
            uint32 endindex = (uint32)iMetaDataInfo.iNumTracks - 1;
            /* Check if the index parameter is present */
            char* indexstr = oscl_strstr((*keylistptr)[lcv].get_cstr(), PVMFSTREAMINGMGRNODE_INDEX);
            if (indexstr != NULL)
            {
                /* Retrieve the index values */
                GetIndexParamValues(indexstr, startindex, endindex);
            }
            /* Validate the indices */
            if (startindex > endindex || startindex >= iMetaDataInfo.iNumTracks || endindex >= iMetaDataInfo.iNumTracks)
            {
                continue;
            }
            /* Return a KVP for each index */
            for (uint32 i = startindex; i <= endindex; ++i)
            {
                if (i < iMetaDataInfo.iTrackMetaDataInfoVec.size())
                {
                    PVMFSMTrackMetaDataInfo trackInfo = iMetaDataInfo.iTrackMetaDataInfoVec[i];
                    PvmiKvp trackkvp;
                    trackkvp.key = NULL;
                    /* Increment the counter for the number of values found so far */
                    ++numvalentries;
                    /* Add the value entry if past the starting index */
                    PVMFStatus retval = PVMFErrArgument;
                    if (numvalentries > (uint32)starting_index)
                    {
                        char indexparam[29];
                        oscl_snprintf(indexparam, 16, ";%s%d", PVMFSTREAMINGMGRNODE_INDEX, i);

                        retval = CreateKVPForUInt32Value(trackkvp,
                                                         PVMFSTREAMINGMGRNODE_TRACKINFO_TRACKID_KEY,
                                                         trackInfo.iTrackID,
                                                         indexparam);
                    }
                    if (retval != PVMFSuccess && retval != PVMFErrArgument)
                    {
                        break;
                    }
                    if (trackkvp.key != NULL)
                    {
                        leavecode = 0;
                        OSCL_TRY(leavecode, (*valuelistptr).push_back(trackkvp));
                        if (leavecode != 0)
                        {
                            OSCL_ARRAY_DELETE(trackkvp.key);
                            trackkvp.key = NULL;
                        }
                        else
                        {
                            // Increment the value list entry counter
                            ++numentriesadded;
                        }

                        // Check if the max number of value entries were added
                        if (max_entries > 0 && numentriesadded >= max_entries)
                        {
                            iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();
                            return PVMFSuccess;
                        }
                    }
                }
            }
        }
        // Add the KVP to the list if the key string was created
        if (KeyVal.key != NULL)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, (*valuelistptr).push_back(KeyVal));
            if (leavecode != 0)
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

    iPVMFStreamingManagerNodeMetadataValueCount = (*valuelistptr).size();

    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::CreateKVPForCharStringValue(PvmiKvp& aKeyVal, const char* aKeyTypeString, const char* aValString, char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR) + 1; // for "char*" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    uint32 valuelen = oscl_strlen(aValString) + 1;

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
             aKeyVal.value.pChar_value = OSCL_ARRAY_NEW(char, valuelen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMFSTREAMINGMGRNODE_SEMICOLON, oscl_strlen(PVMFSTREAMINGMGRNODE_SEMICOLON));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = NULL_TERM_CHAR;
        // Copy the value
        oscl_strncpy(aKeyVal.value.pChar_value, aValString, valuelen);
        aKeyVal.value.pChar_value[valuelen-1] = NULL_TERM_CHAR;
        // Set the length and capacity
        aKeyVal.length = valuelen;
        aKeyVal.capacity = valuelen;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }
        if (aKeyVal.value.pChar_value)
        {
            OSCL_ARRAY_DELETE(aKeyVal.value.pChar_value);
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}


PVMFStatus PVMFStreamingManagerNode::CreateKVPForWideCharStringValue(PvmiKvp& aKeyVal, const char* aKeyTypeString, const oscl_wchar* aValString, char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR) + 1; // for "wchar*" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    uint32 valuelen = oscl_strlen(aValString) + 1;

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
             aKeyVal.value.pWChar_value = OSCL_ARRAY_NEW(oscl_wchar, valuelen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMFSTREAMINGMGRNODE_SEMICOLON, oscl_strlen(PVMFSTREAMINGMGRNODE_SEMICOLON));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = NULL_TERM_CHAR;
        // Copy the value
        oscl_strncpy(aKeyVal.value.pWChar_value, aValString, valuelen);
        aKeyVal.value.pWChar_value[valuelen-1] = NULL_TERM_CHAR;
        // Set the length and capacity
        aKeyVal.length = valuelen;
        aKeyVal.capacity = valuelen;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }
        if (aKeyVal.value.pChar_value)
        {
            OSCL_ARRAY_DELETE(aKeyVal.value.pWChar_value);
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::CreateKVPForUInt32Value(PvmiKvp& aKeyVal, const char* aKeyTypeString, uint32& aValueUInt32, char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR) + 1; // for "uint32" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMFSTREAMINGMGRNODE_SEMICOLON, oscl_strlen(PVMFSTREAMINGMGRNODE_SEMICOLON));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = NULL_TERM_CHAR;
        // Copy the value
        aKeyVal.value.uint32_value = aValueUInt32;
        // Set the length and capacity
        aKeyVal.length = 1;
        aKeyVal.capacity = 1;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::CreateKVPForBoolValue(PvmiKvp& aKeyVal, const char* aKeyTypeString, bool& aValueBool, char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR) + 1; // for "bool" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMFSTREAMINGMGRNODE_SEMICOLON, oscl_strlen(PVMFSTREAMINGMGRNODE_SEMICOLON));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = NULL_TERM_CHAR;
        // Copy the value
        aKeyVal.value.bool_value = aValueBool;
        // Set the length and capacity
        aKeyVal.length = 1;
        aKeyVal.capacity = 1;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}


PVMFCommandId
PVMFStreamingManagerNode::GetNodeMetadataKeys(PVMFSessionId aSessionId,
        PVMFMetadataList& aKeyList,
        uint32 starting_index,
        int32 max_entries,
        char* query_key,
        const OsclAny* aContextData)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetNodeMetadataKeys - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAKEYS,
            aKeyList,
            starting_index,
            max_entries,
            query_key,
            aContextData);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetNodeMetadataKeys - Out"));
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFStreamingManagerNode::GetNodeMetadataValues(PVMFSessionId aSessionId,
        PVMFMetadataList& aKeyList,
        Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
        uint32 starting_index,
        int32 max_entries,
        const OsclAny* aContextData)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetNodeMetadataValues - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAVALUES,
            aKeyList,
            aValueList,
            starting_index,
            max_entries,
            aContextData);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetNodeMetadataValues - Out"));
    return QueueCommandL(cmd);
}

// From PVMFMetadataExtensionInterface
PVMFStatus PVMFStreamingManagerNode::ReleaseNodeMetadataKeys(PVMFMetadataList& ,
        uint32 ,
        uint32)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFStreamingManagerNode::ReleaseNodeMetadataKeys() called"));
    //nothing needed-- there's no dynamic allocation in this node's key list
    return PVMFSuccess;
}

// From PVMFMetadataExtensionInterface
PVMFStatus PVMFStreamingManagerNode::ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
        uint32 start,
        uint32 end)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFStreamingManagerNode::ReleaseNodeMetadataValues() called"));

    if (start > end || aValueList.size() == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::ReleaseNodeMetadataValues() Invalid start/end index"));
        return PVMFErrArgument;
    }

    //Only CPM related metadata is retrived. Then this one should be 0.
    if (iPVMFStreamingManagerNodeMetadataValueCount == 0) return PVMFSuccess;

    //To remove madatada related with un-drm value
    end = iPVMFStreamingManagerNodeMetadataValueCount - 1;
    if (end >= aValueList.size())
    {
        end = aValueList.size() - 1;
    }

    for (uint32 i = start; i <= end; i++)
    {
        if (aValueList[i].key != NULL)
        {
            switch (GetValTypeFromKeyString(aValueList[i].key))
            {
                case PVMI_KVPVALTYPE_WCHARPTR:
                    if (aValueList[i].value.pWChar_value != NULL)
                    {
                        OSCL_ARRAY_DELETE(aValueList[i].value.pWChar_value);
                        aValueList[i].value.pWChar_value = NULL;
                    }
                    break;

                case PVMI_KVPVALTYPE_CHARPTR:
                    if (aValueList[i].value.pChar_value != NULL)
                    {
                        OSCL_ARRAY_DELETE(aValueList[i].value.pChar_value);
                        aValueList[i].value.pChar_value = NULL;
                    }
                    break;

                case PVMI_KVPVALTYPE_UINT8PTR:
                    if (aValueList[i].value.pUint8_value != NULL)
                    {
                        OSCL_ARRAY_DELETE(aValueList[i].value.pUint8_value);
                        aValueList[i].value.pUint8_value = NULL;
                    }
                    break;

                case PVMI_KVPVALTYPE_UINT32:
                case PVMI_KVPVALTYPE_FLOAT:
                case PVMI_KVPVALTYPE_BOOL:
                    // No need to free memory for this valtype
                    break;

                default:
                    // Should not get a value that wasn't created from this node
                    OSCL_ASSERT(false);
                    break;
            }

            OSCL_ARRAY_DELETE(aValueList[i].key);
            aValueList[i].key = NULL;
        }
    }

    return PVMFSuccess;
}




PVMFStatus PVMFStreamingManagerNode::getParametersSync(
    PvmiMIOSession aSession, PvmiKeyType aIdentifier,
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
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::getParametersSync() Invalid key string"));
        return PVMFErrArgument;
    }

    // Retrieve the second component from the key string
    pv_mime_string_extract_type(1, aIdentifier, compstr);

    // Check if it is key string for streaming manager
    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("net")) < 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::getParametersSync() Unsupported key"));
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
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::getParametersSync() Memory allocation for KVP failed"));
            return PVMFErrNoMemory;
        }
        oscl_memset(aParameters, 0, StreamingManagerConfig_NumBaseKeys*sizeof(PvmiKvp));
        // Allocate memory for the key strings in each KVP
        PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(StreamingManagerConfig_NumBaseKeys * SMCONFIG_KEYSTRING_SIZE * sizeof(char));
        if (memblock == NULL)
        {
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::getParametersSync() Memory allocation for key string failed"));
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
                            (0, "PVMFStreamingManagerNode::getParametersSync() Unsupported key"));
            return PVMFErrNoMemory;
        }

        PVMFStatus retval = GetConfigParameter(aParameters, aNumParamElements, i, reqattr);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFStreamingManagerNode::getParametersSync() "
                             "Retrieving streaming manager parameter failed"));
            return retval;
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::getParametersSync() Unsupported key"));
        return PVMFErrArgument;
    }

    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::releaseParameters(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFStreamingManagerNode::releaseParameters() In"));

    if (aParameters == NULL || num_elements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFStreamingManagerNode::releaseParameters() KVP list is NULL or number of elements is 0"));
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
                        (0, "PVMFStreamingManagerNode::releaseParameters() Unsupported key"));
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
                                    (0, "PVMFStreamingManagerNode::releaseParameters() Valtype not specified in key string"));
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
                    (0, "PVMFStreamingManagerNode::releaseParameters() Out"));
    return PVMFSuccess;
}

void PVMFStreamingManagerNode::createContext(PvmiMIOSession aSession,
        PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFStreamingManagerNode::setContextParameters(PvmiMIOSession aSession,
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

void PVMFStreamingManagerNode::DeleteContext(PvmiMIOSession aSession,
        PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFStreamingManagerNode::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
        int num_elements, PvmiKvp* &aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFStreamingManagerNode::setParametersSync() In"));


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
                            (0, "PVMFStreamingManagerNode::setParametersSync() Unsupported key"));
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
                                    (0, "PVMFStreamingManagerNode::setParametersSync() Unsupported key"));
                    return;
                }

                // Verify and set the passed-in setting
                PVMFStatus retval = VerifyAndSetConfigParameter(i, aParameters[paramind], true);
                if (retval != PVMFSuccess)
                {
                    aRet_kvp = &aParameters[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMFStreamingManagerNode::setParametersSync() Setting "
                                     "parameter %d failed", paramind));
                    return;
                }
            }
            else
            {
                // Do not support more than 3 components right now
                aRet_kvp = &aParameters[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVMFStreamingManagerNode::setParametersSync() Unsupported key"));
                return;
            }
        }
        else
        {
            // Unknown key string
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMFStreamingManagerNode::setParametersSync() Unsupported key"));
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFStreamingManagerNode::setParametersSync() Out"));
}

PVMFCommandId PVMFStreamingManagerNode::setParametersAsync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements,
        PvmiKvp*& aRet_kvp,
        OsclAny* context)
{

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::setParametersAsync - In"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(0,
            PVMF_STREAMING_MANAGER_NODE_CAPCONFIG_SETPARAMS,
            aSession, aParameters, num_elements, aRet_kvp, context);
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::setParametersAsync - Out"));
    return QueueCommandL(cmd);
}

uint32 PVMFStreamingManagerNode::getCapabilityMetric(PvmiMIOSession aSession)
{
    OSCL_UNUSED_ARG(aSession);
    return 0;
}

PVMFStatus PVMFStreamingManagerNode::verifyParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFStreamingManagerNode::verifyParametersSync() In"));

    if (aParameters == NULL || num_elements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PVMFStreamingManagerNode::verifyParametersSync() Passed in parameter invalid"));
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
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::verifyParametersSync() Unsupported key"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Verifying parameter %d failed", paramind));
                return retval;
            }
        }
        else
        {
            // Unknown key string
            return PVMFErrArgument;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Out"));
    return PVMFSuccess;
}

PVMFStatus PVMFStreamingManagerNode::GetConfigParameter(PvmiKvp*& aParameters,
        int& aNumParamElements,
        int32 aIndex, PvmiKvpAttr reqattr)
{
    PVMF_SM_LOGINFO((0, "PVMFStreamingManagerNode::GetConfigParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));

    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(SMCONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() Memory allocation for key string failed"));
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
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
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
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() "
                                      "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = MIN_JITTER_BUFFER_DURATION_IN_MS;
                rui32->max = MAX_JITTER_BUFFER_DURATION_IN_MS;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;
        case BASEKEY_REBUFFERING_THRESHOLD:
        {
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
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
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() "
                                      "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
                rui32->max = DEFAULT_JITTER_BUFFER_UNDERFLOW_THRESHOLD_IN_MS;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
        }
        break;
        case BASEKEY_JITTERBUFFER_NUMRESIZE:
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                uint32 numResize, resizeSize;
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
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
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() "
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
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
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
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() "
                                      "Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = MIN_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;
                rui32->max = MAX_SOCKETMEMPOOL_RESIZELEN_INPUT_PORT;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;
        case BASEKEY_SESSION_CONTROLLER_USER_AGENT:
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                aParameters[0].value.pWChar_value = NULL;
                /* As of now just RTSP node supports an external config of user agent */
                PVMFSMNodeContainer* iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
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
                            PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter() "
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
                PVMFSMNodeContainer* iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
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

        case BASEKEY_STREAMING_MGR_SWITCH_STREAMS:
            /* Do nothing - things like current, default, and capability do not make sense here */
            break;

        case BASEKEY_STREAMING_SPEED:
        {
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                {
                    aParameters[0].value.uint32_value = 1;
                }
            }
            else
            {
                // Return capability - no concept of capability for streaming speed
                // do nothing
            }
        }
        break;

        case BASEKEY_HTTP_VERSION:
        {
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::GetConfigParameter - HTTP Version Not Supported"));
                    return PVMFErrArgument;
                }
            }
            else
            {
                // Return capability - no concept of capability for http version
                // do nothing
            }
        }
        break;

        case BASEKEY_NUM_REDIRECT_ATTEMPTS:
        {
            if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
            {
                {
                    //seems like there is no reasonable default here ??
                    aParameters[0].value.uint32_value = 0;
                }
            }
            else
            {
                // Return capability - no concept of capability for num redirect attempts
                // do nothing
            }
        }
        break;

        case BASEKEY_PROTOCOL_EXTENSION_HEADER:
            /* Do nothing - things like current, default, and capability do not make sense here */
            break;

        case BASEKEY_SESSION_CONTROLLER_HTTP_TIMEOUT:
            break;

        case BASEKEY_SESSION_CONTROLLER_HTTP_STREAMING_LOGGING_TIMEOUT:
            break;

        case BASEKEY_ACCEL_BITRATE:
            /* Do nothing - things like current, default, and capability do not make sense here */
            break;

        case BASEKEY_ACCEL_DURATION:
            /* Do nothing - things like current, default, and capability do not make sense here */
            break;
        case BASEKEY_MAX_TCP_RECV_BUFFER_SIZE:
        {
            if (IsHttpExtensionHeaderValid(aParameters[0]))
            {
                PVMFSMNodeContainer* iSocketNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
                if (iSocketNodeContainer != NULL)
                {
                    PVMFSocketNode* socketNode =
                        (PVMFSocketNode*)(iSocketNodeContainer->iNode);
                    if ((reqattr == PVMI_KVPATTR_CUR) || (reqattr == PVMI_KVPATTR_DEF))
                    {
                        uint32 size = 0;
                        socketNode->GetMaxTCPRecvBufferSize(size);
                        aParameters[0].value.uint32_value = size;
                    }
                }
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

PVMFStatus PVMFStreamingManagerNode::VerifyAndSetConfigParameter(int index, PvmiKvp& aParameter, bool set)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::VerifyAndSetConfigParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::VerifyAndSetConfigParameter() "
                          "Valtype in key string unknown"));
        return PVMFErrArgument;
    }

    // Verify the valtype
    if (keyvaltype != StreamingManagerConfig_BaseKeys[index].iValueType)
    {
        PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::VerifyAndSetConfigParameter() "
                          "Valtype does not match for key"));
        return PVMFErrArgument;
    }

    switch (index)
    {
        case BASEKEY_DELAY:
        {
            // Validate
            if ((aParameter.value.uint32_value < MIN_JITTER_BUFFER_DURATION_IN_MS) ||
                    (aParameter.value.uint32_value > MAX_JITTER_BUFFER_DURATION_IN_MS))
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::VerifyAndSetConfigParameter() "
                                  "Trying to set delay to 0"));
                return PVMFErrArgument;
            }

            if (set)
            {
                // save value locally
                setJitterBufferDurationInMilliSeconds(aParameter.value.uint32_value);

                // pass the value on to the jitter buffer node
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                jbExtIntf->setJitterBufferDurationInMilliSeconds(aParameter.value.uint32_value);
            }
        }
        break;
        case BASEKEY_REBUFFERING_THRESHOLD:
        {
            uint32 jbDuration = 0;
            PVMFSMNodeContainer* iJitterBufferNodeContainer =
                getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
            PVMFJitterBufferExtensionInterface* jbExtIntf =
                (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
            jbExtIntf->getJitterBufferDurationInMilliSeconds(jbDuration);
            // Validate
            if (aParameter.value.uint32_value >= jbDuration)
            {
                PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::VerifyAndSetConfigParameter() "
                                  "Trying to set rebuffering threshold greater than equal to jitter buffer duration"));
                return PVMFErrArgument;
            }
            if (set)
            {
                // pass the value on to the jitter buffer node
                jbExtIntf->setJitterBufferRebufferingThresholdInMilliSeconds(aParameter.value.uint32_value);
            }
        }
        break;
        case BASEKEY_JITTERBUFFER_NUMRESIZE:
        {
            if (set)
            {
                // retrieve and update
                uint32 numResize, resizeSize;
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                jbExtIntf->GetSharedBufferResizeParams(numResize, resizeSize);
                jbExtIntf->SetSharedBufferResizeParams(aParameter.value.uint32_value, resizeSize);
            }
        }
        break;
        case BASEKEY_JITTERBUFFER_RESIZESIZE:
        {
            if (set)
            {
                // retrieve and update
                uint32 numResize, resizeSize;
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                jbExtIntf->GetSharedBufferResizeParams(numResize, resizeSize);
                jbExtIntf->SetSharedBufferResizeParams(numResize, aParameter.value.uint32_value);
            }
        }
        break;
        case BASEKEY_SESSION_CONTROLLER_USER_AGENT:
        {
            if (set)
            {
                // user agent update
                PVMFSMNodeContainer* iSessionControllerNodeContainer = NULL;
                iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                        (PVRTSPEngineNodeExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);

                    OSCL_wHeapString<OsclMemAllocator> userAgent;
                    OSCL_wHeapString<OsclMemAllocator> dummy;
                    userAgent = aParameter.value.pWChar_value;
                    rtspExtIntf->SetClientParameters(userAgent, dummy, dummy);
                }
                iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE);
                OSCL_wHeapString<OsclMemAllocator> userAgent = aParameter.value.pWChar_value;
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVMFProtocolEngineNodeMSHTTPStreamingExtensionInterface* httpExtIntf =
                        (PVMFProtocolEngineNodeMSHTTPStreamingExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);

                    httpExtIntf->SetUserAgent(userAgent);
                }
                // save user-agent kvp for cpm
                if (iCPM)
                {
                    PVMFStatus status = iCPMKvpStore.addKVPString(aParameter.key, userAgent);
                    if (status != PVMFSuccess) return status;
                }

            }
        }
        break;

        case BASEKEY_HTTP_VERSION:
        {
            if (set)
            {
                PVMFSMNodeContainer* iSessionControllerNodeContainer = NULL;
                iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::VerifyAndSetConfigParameter() "
                                      "Setting HTTP Protocol Version Not supported"));
                    return PVMFErrNotSupported;
                }
            }
        }
        break;

        case BASEKEY_NUM_REDIRECT_ATTEMPTS:
        {
            if (set)
            {
                if (IsHttpExtensionHeaderValid(aParameter))
                {
                    PVMFSMNodeContainer* iSessionControllerNodeContainer = NULL;
                    iSessionControllerNodeContainer =
                        getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
                    if (iSessionControllerNodeContainer != NULL)
                    {
                        //do nothing for now - till RTSP node has an api to set this
                    }
                    iSessionControllerNodeContainer =
                        getNodeContainer(PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE);
                    if (iSessionControllerNodeContainer != NULL)
                    {
                        PVMFProtocolEngineNodeMSHTTPStreamingExtensionInterface* httpExtIntf =
                            (PVMFProtocolEngineNodeMSHTTPStreamingExtensionInterface*)
                            (iSessionControllerNodeContainer->iExtensions[0]);

                        httpExtIntf->SetNumRedirectTrials(aParameter.value.uint32_value);
                    }
                }
                else
                {
                    OSCL_StackString<32> dlamode(_STRLIT_CHAR("mode=dla"));
                    bool isDlaMode  = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), dlamode.get_cstr())  != NULL);
                    if (isDlaMode)
                    {
                        if (iCPM)
                        {
                            PVMFStatus status = iCPMKvpStore.addKVPuint32Value(aParameter.key, aParameter.value.uint32_value);
                            if (status != PVMFSuccess)
                                return status;
                        }
                    }
                }
            }
        }
        break;

        case BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_INTERVAL:
        {
            if (set)
            {
                // user agent update
                /* As of now just RTSP node supports an external config of user agent */
                PVMFSMNodeContainer* iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                        (PVRTSPEngineNodeExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);
                    rtspExtIntf->SetKeepAliveMethod_timeout(aParameter.value.uint32_value);
                }
                iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_HTTP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVMFProtocolEngineNodeMSHTTPStreamingExtensionInterface* httpExtIntf =
                        (PVMFProtocolEngineNodeMSHTTPStreamingExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);
                    httpExtIntf->SetKeepAliveTimeout(aParameter.value.uint32_value);
                }

            }
        }
        break;

        case BASEKEY_SESSION_CONTROLLER_KEEP_ALIVE_DURING_PLAY:
        {
            if (set)
            {
                // keep-alive during play update
                /* As of now just RTSP node supports an external config of keep-alive during play */
                PVMFSMNodeContainer* iSessionControllerNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_RTSP_SESSION_CONTROLLER_NODE);
                if (iSessionControllerNodeContainer != NULL)
                {
                    PVRTSPEngineNodeExtensionInterface* rtspExtIntf =
                        (PVRTSPEngineNodeExtensionInterface*)
                        (iSessionControllerNodeContainer->iExtensions[0]);
                    rtspExtIntf->SetKeepAliveMethod_keep_alive_in_play(aParameter.value.bool_value);
                }
            }
        }
        break;



        case BASEKEY_MAX_TCP_RECV_BUFFER_SIZE:
        {
            if (IsHttpExtensionHeaderValid(aParameter))
            {
                if (set)
                {
                    PVMFSMNodeContainer* iSocketNodeContainer =
                        getNodeContainer(PVMF_STREAMING_MANAGER_SOCKET_NODE);
                    if (iSocketNodeContainer != NULL)
                    {
                        PVMFSocketNode* socketNode =
                            (PVMFSocketNode*)(iSocketNodeContainer->iNode);
                        socketNode->SetMaxTCPRecvBufferSize(aParameter.value.uint32_value);
                    }
                }
            }
        }
        break;

        case BASEKEY_DISABLE_FIREWALL_PACKETS:
        {
            if (set)
            {
                PVMFSMNodeContainer* iJitterBufferNodeContainer =
                    getNodeContainer(PVMF_STREAMING_MANAGER_JITTER_BUFFER_NODE);
                PVMFJitterBufferExtensionInterface* jbExtIntf =
                    (PVMFJitterBufferExtensionInterface*)iJitterBufferNodeContainer->iExtensions[0];
                jbExtIntf->DisableFireWallPackets();
            }
        }
        break;

        default:
            return PVMFErrNotSupported;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMFStreamingManagerNode::VerifyAndSetPlayerParameter() Out"));

    return PVMFSuccess;
}

// remove the ending ';', ',' or ' ' and calulate value length
uint32 PVMFStreamingManagerNode::getItemLen(char *ptrItemStart, char *ptrItemEnd)
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


bool PVMFStreamingManagerNode::IsInternalCmd(PVMFCommandId aId)
{
    if ((aId == PVMF_STREAMING_MANAGER_NODE_CONSTRUCT_SESSION) ||
            (aId == PVMF_STREAMING_MANAGER_NODE_AUTO_PAUSE) ||
            (aId == PVMF_STREAMING_MANAGER_NODE_AUTO_RESUME) ||
            (aId == PVMF_STREAMING_MANAGER_NODE_THIN_STREAM))
    {
        return true;
    }
    return false;
}

bool PVMFStreamingManagerNode::IsHttpExtensionHeaderValid(PvmiKvp &aParameter)
{
    OSCL_StackString<32> downloadMode(_STRLIT_CHAR("mode=download"));
    OSCL_StackString<32> streamingMode(_STRLIT_CHAR("mode=streaming"));
    OSCL_StackString<32> dlaMode(_STRLIT_CHAR("mode=dla"));

    bool isDownloadMode  = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), downloadMode.get_cstr())  != NULL);
    bool isStreamingMode = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), streamingMode.get_cstr()) != NULL);
    bool isDlaMode = (oscl_strstr(OSCL_CONST_CAST(char*, aParameter.key), dlaMode.get_cstr()) != NULL);


    // download mode only would fail, streaming mode specified or not specified will be viewed as true
    if (isDownloadMode && !isStreamingMode) return false;

    // dla mode only would fail, streaming mode specified or not specified will be viewed as true
    if (isDlaMode && !isStreamingMode) return false;


    return true;
}




PVMFStatus
PVMFStreamingManagerNode::GetMaxSizeValue(char* aString, uint32& aMaxSize)
{
    aMaxSize = 0xFFFFFFFF;
    /*
     * This parses a string of the form "maxsize=N1" and extracts the integer N1.
     */
    if (aString == NULL)
    {
        return PVMFErrArgument;
    }

    /* Go to end of "maxsize=" */
    char* n1string = aString + 8;
    char* truncatestr = oscl_strstr(n1string, PVMFSTREAMINGMGRNODE_TRUNCATE_FLAG);

    uint32 maxsizelen = 0;

    if (truncatestr != NULL)
    {
        maxsizelen = oscl_strlen(n1string) - (oscl_strlen(truncatestr) + 1);
        n1string[maxsizelen] = '\0';
    }

    if (PV_atoi(n1string, 'd', oscl_strlen(n1string), aMaxSize))
    {
        return PVMFSuccess;
    }
    return PVMFFailure;
}

PVMFStatus
PVMFStreamingManagerNode::GetTruncateFlagValue(char* aString, uint32& aTruncateFlag)
{
    aTruncateFlag = 0;
    /*
     * This parses a string of the form "truncate=N1" and extracts the integer N1.
     */
    if (aString == NULL)
    {
        return PVMFErrArgument;
    }

    /* Go to end of "truncate=" */
    char* n1string = aString + 9;

    if (!oscl_strcmp(n1string, "true"))
    {
        aTruncateFlag = true;
    }
    else if (!oscl_strcmp(n1string, "false"))
    {
        aTruncateFlag = false;
    }
    else
    {
        return PVMFFailure;
    }
    return PVMFSuccess;

}





void
PVMFStreamingManagerNode::ResetNodeContainerCmdState()
{
    for (uint32 i = 0; i < iNodeContainerVec.size(); i++)
    {
        if (iNodeContainerVec[i].iNodeCmdState == PVMFSM_NODE_CMD_COMPLETE)
        {
            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_NO_PENDING;
        }
        else if (iNodeContainerVec[i].iNodeCmdState == PVMFSM_NODE_CMD_CANCEL_PENDING)
        {
            iNodeContainerVec[i].iNodeCmdState = PVMFSM_NODE_CMD_CANCEL_COMPLETE;
        }
    }
}

//////////////////// PVMFSMNodeKVPStore Implementation /////////////////////////////////////////////////////////

// add kvp string with W-string value
PVMFStatus PVMFSMNodeKVPStore::addKVPString(const char* aKeyTypeString, OSCL_wString& aValString)
{
    PvmiKvp aKeyVal;
    aKeyVal.key = NULL;
    PVMFStatus status = PVMFCreateKVPUtils::CreateKVPForWStringValue(aKeyVal, aKeyTypeString, aValString);
    if (status != PVMFSuccess) return status;

    int32 err = 0;
    OSCL_TRY(err,
             iKvpVector.push_back(aKeyVal);
             KVPValueTypeForMemoryRelease valType = KVPValueTypeForMemoryRelease_WString;
             iKVPValueTypeForMemoryRelease.push_back((uint32)valType);
            );
    return (err == 0 ? PVMFSuccess : PVMFErrNoMemory);
}

// add kvp string with normal string value
PVMFStatus PVMFSMNodeKVPStore::addKVPString(const char* aKeyTypeString, const char* aValString)
{
    PvmiKvp aKeyVal;
    aKeyVal.key = NULL;
    PVMFStatus status = PVMFCreateKVPUtils::CreateKVPForCharStringValue(aKeyVal, aKeyTypeString, aValString);
    if (status != PVMFSuccess) return status;

    int32 err = 0;
    OSCL_TRY(err,
             iKvpVector.push_back(aKeyVal);
             KVPValueTypeForMemoryRelease valType = KVPValueTypeForMemoryRelease_String;
             iKVPValueTypeForMemoryRelease.push_back((uint32)valType);
            );
    return (err == 0 ? PVMFSuccess : PVMFErrNoMemory);
}

void PVMFSMNodeKVPStore::releaseMemory()
{
    OSCL_ASSERT(iKvpVector.size() == iKVPValueTypeForMemoryRelease.size());

    for (uint32 i = 0; i < iKvpVector.size(); i++)
    {
        if (iKvpVector[i].key) OSCL_ARRAY_DELETE(iKvpVector[i].key);

        // release memory for appropriate types of KVP value
        if ((KVPValueTypeForMemoryRelease)iKVPValueTypeForMemoryRelease[i] == KVPValueTypeForMemoryRelease_WString &&
                iKvpVector[i].value.pWChar_value) OSCL_ARRAY_DELETE(iKvpVector[i].value.pWChar_value);
        if ((KVPValueTypeForMemoryRelease)iKVPValueTypeForMemoryRelease[i] == KVPValueTypeForMemoryRelease_String &&
                iKvpVector[i].value.pChar_value) OSCL_ARRAY_DELETE(iKvpVector[i].value.pChar_value);
    }
}



PVMFStatus PVMFSMNodeKVPStore::addKVPuint32Value(const char* aKeyTypeString, uint32 aValue)
{
    PvmiKvp aKeyVal;
    aKeyVal.key = NULL;
    PVMFStatus status = PVMFCreateKVPUtils::CreateKVPForUInt32Value(aKeyVal, aKeyTypeString, aValue);
    if (status != PVMFSuccess) return status;

    int32 err = 0;
    OSCL_TRY(err,
             iKvpVector.push_back(aKeyVal);
             KVPValueTypeForMemoryRelease valType = KVPValueTypeForMemoryRelease_NoInterest;
             iKVPValueTypeForMemoryRelease.push_back((uint32)valType);
            );
    return (err == 0 ? PVMFSuccess : PVMFErrNoMemory);
}

//Check for RTP packet source
bool
PVMFStreamingManagerNode::IsRTPPacketSourcePresent()
{
    return false;
}



