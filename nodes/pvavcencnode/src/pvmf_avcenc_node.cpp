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
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

#include "oscl_dll.h"
#include "oscl_error_codes.h"
#include "pvmf_avcenc_port.h"
#include "pvmf_avcenc_node_factory.h"
#include "pvmf_avcenc_node_types.h"
#include "pvmf_avcenc_node.h"
#include "pvavcencoderinterface.h"
#include "pvavcencoder_factory.h"

#ifndef PVMF_MEDIA_MSG_FORMAT_IDS_H_INCLUDED
#include "pvmf_media_msg_format_ids.h"
#endif
#ifndef PVMF_MEDIA_CMD_H_INCLUDED
#include "pvmf_media_cmd.h"
#endif
#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m);
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m);
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

// DLL entry point
OSCL_DLL_ENTRY_POINT_DEFAULT()

const uint32 DEFAULT_PARAMS_SET_LENGTH = 256; // should cover the output from avcencoder
const uint32 DEFAULT_PACKET_SIZE = 256;

////////////////////////////////////////////////////////////////////////////
//              PVMFAvcEncNodeFactory implementation
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFNodeInterface* PVMFAvcEncNodeFactory::CreateAvcEncNode(int32 aPriority)
{
    int32 err = 0;
    PVMFAvcEncNode* node = NULL;

    OSCL_TRY(err,
             node = OSCL_NEW(PVMFAvcEncNode, (aPriority));
             if (!node)
             OSCL_LEAVE(OsclErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err, return NULL;);

    return (PVMFNodeInterface*)node;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNodeFactory::DeleteAvcEncNode(PVMFNodeInterface* aNode)
{
    if (!aNode)
        return false;

    PVMFAvcEncNode* node = (PVMFAvcEncNode*)aNode;
    OSCL_DELETE(node);
    aNode = NULL;
    return true;
}

////////////////////////////////////////////////////////////////////////////
//              PVMFAvcEncNode implementation
////////////////////////////////////////////////////////////////////////////
PVMFAvcEncNode::PVMFAvcEncNode(int32 aPriority) :
        OsclTimerObject(aPriority, "PVMFAvcEncNode"),
//	iMediaBufferMemPool(PVAVCENC_MEDIADATA_POOLNUM, PVAVCENC_MEDIABUFFER_CHUNKSIZE),
        iMediaDataMemPool(PVAVCENC_MEDIADATA_POOLNUM, PVAVCENC_MEDIADATA_CHUNKSIZE),
        iCodec(PVMF_H264_RAW),
        iAvcEncoder(NULL),
        iSeqNum(0),
        iReadyForNextFrame(false),
        iNumSPSs(0),
        iNumPPSs(0),
        iExtensionRefCount(0)
{

#if PROFILING_ON
    iMinEncDuration = 0;
    iMaxEncDuration = 0;
    iAverageEncDuration = 0;
    iTotalFramesEncoded = 0;
    iTotalEncTime = 0;
    iNumFramesSkipped = 0;
    iDiagnosticsLogger = PVLogger::GetLoggerObject("pvauthordiagnostics.encoder.avc");
    oDiagnosticsLogged = false;

#endif

    iInterfaceState = EPVMFNodeCreated;

    // Allocate memory for parameters set
    uint refCounterSize = oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
    uint size = refCounterSize + DEFAULT_PARAMS_SET_LENGTH;
    uint8 *memBuffer = NULL;

    int32 err;
    OSCL_TRY(err,
             //Create the input command queue
             iCmdQueue.Construct(PVMF_AVCENC_NODE_CMD_ID_START, PVMF_AVCENC_NODE_CMD_QUEUE_RESERVE);
             iCurrentCmd.Construct(0, 1); // There's only 1 current command

             //Create the port vector.
             iInPort.Construct(PVMF_AVCENC_NODE_PORT_VECTOR_RESERVE);
             iOutPort.Construct(PVMF_AVCENC_NODE_PORT_VECTOR_RESERVE);

             // Reserve space for port activity queue
             iPortActivityQueue.reserve(PVMF_AVCENC_NODE_PORT_ACTIVITY_RESERVE);

             // Create media data allocator
             iMediaBufferMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (PVAVCENC_MEDIADATA_POOLNUM));
             if (!iMediaBufferMemPool)
             OSCL_LEAVE(OsclErrNoMemory);

             iMediaDataAlloc = OSCL_NEW(PVMFSimpleMediaBufferCombinedAlloc, (iMediaBufferMemPool));
             if (!iMediaDataAlloc)
                 OSCL_LEAVE(OsclErrNoMemory);

                 // The first allocate call will set the chunk size of the memory pool. Use the max
                 // frame size calculated earlier to set the chunk size.  The allocated data will be
                 // deallocated automatically as tmpPtr goes out of scope.
                 OsclSharedPtr<PVMFMediaDataImpl> tmpPtr = iMediaDataAlloc->allocate(MAX_OUTBUF_SIZE);

                 iMediaDataGroupImplMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (PVAVCENC_MEDIADATA_POOLNUM));
                 if (!iMediaDataGroupImplMemPool)
                     OSCL_LEAVE(OsclErrNoMemory);

                     iMediaDataGroupAlloc = OSCL_NEW(PVMFMediaFragGroupCombinedAlloc<OsclMemPoolFixedChunkAllocator>,
                                                     (10, 2048, iMediaDataGroupImplMemPool));
                     if (!iMediaDataGroupAlloc)
            {
                OSCL_LEAVE(OsclErrNoMemory);
                }
                else
                {
                    iMediaDataGroupAlloc->create();
                }

    // original from m4venc
    memBuffer = (uint8*)iAlloc.allocate(size);
    if (!memBuffer)
{
    OSCL_LEAVE(PVMFErrNoMemory);
    }
            );

    OSCL_FIRST_CATCH_ANY(err,
                         // If a leave happened, cleanup and re-throw the error
                         iCmdQueue.clear();
                         iInPort.clear();
                         iOutPort.clear();
                         OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
                         OSCL_CLEANUP_BASE_CLASS(OsclTimerObject);
                         OSCL_LEAVE(err);
                        );

    // Save default VOL header

    oscl_memset(memBuffer, 0, DEFAULT_PARAMS_SET_LENGTH);
    OsclMemoryFragment paramSet;
    OsclRefCounter* refCounter = new(memBuffer) OsclRefCounterDA(memBuffer,
            (OsclDestructDealloc*)&iAlloc);
    memBuffer += refCounterSize;
    paramSet.ptr = memBuffer;
//	oscl_memcpy(volHeader.ptr, (OsclAny*)DEFAULT_VOL_HEADER, DEFAULT_VOL_HEADER_LENGTH); no default param set
    paramSet.len = DEFAULT_PARAMS_SET_LENGTH;
    iParamSet = OsclRefCounterMemFrag(paramSet, refCounter, DEFAULT_PARAMS_SET_LENGTH);

    ConstructEncoderParams();

#if PROFILING_ON
    oscl_memset(&iStats, 0, sizeof(PVAvcEncNodeStats));
#endif
}

////////////////////////////////////////////////////////////////////////////
PVMFAvcEncNode::~PVMFAvcEncNode()
{
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif

    if (iMediaDataGroupAlloc)
    {
        iMediaDataGroupAlloc->removeRef();
    }

    if (iMediaDataGroupImplMemPool)
    {
        OSCL_DELETE(iMediaDataGroupImplMemPool);
        iMediaDataGroupImplMemPool = 0;
    }

    if (iMediaDataAlloc)
    {
        OSCL_DELETE(iMediaDataAlloc);
        iMediaDataAlloc = NULL;
    }

    if (iMediaBufferMemPool)
    {
        OSCL_DELETE(iMediaBufferMemPool);
        iMediaBufferMemPool = 0;
    }

    DeleteAvcEncoder();

    while (!iInPort.empty())		iInPort.Erase(&iInPort.front());
    while (!iOutPort.empty())	iOutPort.Erase(&iOutPort.front());

    Cancel();
    SetState(EPVMFNodeIdle);
    ThreadLogoff();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncNode::ThreadLogon()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
                AddToScheduler();
            iLogger = PVLogger::GetLoggerObject("PVMFAvcEncNode");
            SetState(EPVMFNodeIdle);
            return PVMFSuccess;

        default:
            return PVMFErrInvalidState;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncNode::ThreadLogoff()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            if (IsAdded())
                RemoveFromScheduler();
            iLogger = NULL;
            SetState(EPVMFNodeCreated);
            return PVMFSuccess;

        default:
            return PVMFErrInvalidState;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFAvcEncNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::GetCapability"));

    aNodeCapability.iInputFormatCapability.push_back(PVMF_YUV420);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_H264_RAW);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_H264_MP4);
    // not supported yet NodeCapability.iOutputFormatCapability.push_back(PVMF_H264);
    aNodeCapability.iCanSupportMultipleOutputPorts = false;
    aNodeCapability.iCanSupportMultipleInputPorts = false;
    aNodeCapability.iHasMaxNumberOfPorts = true;
    aNodeCapability.iMaxNumberOfPorts = 2;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFPortIter* PVMFAvcEncNode::GetPorts(const PVMFPortFilter* aFilter)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::GetPorts"));
    OSCL_UNUSED_ARG(aFilter);//port filter is not implemented.

    int32 err = 0;
    PVMFAvcEncPortVector* port = NULL;
    OSCL_TRY(err,
             port = OSCL_NEW(PVMFAvcEncPortVector, ());
             if (!port)
             return NULL;

             int32 i;
             for (i = 0; i < (int)iInPort.size(); i++)
                 port->AddL(iInPort[i]);
                 for (i = 0; i < (int) iOutPort.size(); i++)
                     port->AddL(iOutPort[i]);
                     port->Reset();
                    );

    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PVMFAvcEncNode::GetPorts: Error - Out of memory"));
                        );

    return port;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::QueryUUID(PVMFSessionId aSession,
        const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::QueryUUID"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYUUID,
                  aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::QueryInterface(PVMFSessionId aSession,
        const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::QueryInterface"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYINTERFACE,
                  aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::RequestPort(PVMFSessionId aSession,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::RequestPort: aPortTag=%d", aPortTag));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_REQUESTPORT,
                  aPortTag, aPortConfig, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::ReleasePort(PVMFSessionId aSession,
        PVMFPortInterface& aPort,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::ReleasePort"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Init(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Init"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_INIT, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Prepare(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Prepare"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PREPARE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Start(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Start"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_START, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Stop(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Stop"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_STOP, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Flush(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Flush"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_FLUSH, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Pause(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Pause"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PAUSE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::Reset(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Reset"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RESET, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::CancelAllCommands"));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELALLCOMMANDS, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFAvcEncNode::CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::CancelCommand: aCmdId=%d", aCmdId));
    PVMFAvcEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELCOMMAND, aCmdId, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    // Only outgoing queue ready activity are forwarded on to the node
    // by the output port.  The rest of the events for output port and all events
    // for input port are handled within the port AO.
    if (aActivity.iPort->GetPortTag() == PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aActivity.iType)
        {
            case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
                // Notifies the node that the output queue is ready, and the node would
                // resume encoding incoming data
                uint32 i;
                for (i = 0; i < iInPort.size(); i++)
                    ((PVMFAvcEncPort*)iInPort[i])->ProcessIncomingMsgReady();
                break;

            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFAvcEncNode::addRef()
{
    ++iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFAvcEncNode::removeRef()
{
    if (iExtensionRefCount > 0)
        --iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    if (uuid == PVMp4H263EncExtensionUUID)
    {
        PVMp4H263EncExtensionInterface* myInterface = OSCL_STATIC_CAST(PVMp4H263EncExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else if (uuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = 	OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else
    {
        iface = NULL;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetNumLayers(uint32 aNumLayers)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetNumLayers: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if (aNumLayers > MAX_AVC_LAYER) // MAX_AVC_LAYER defined in pvmf_avcenc_tunables.h
    {
        LOG_ERR((0, "PVMFAvcEncNode::SetNumLayers: Error Max num layers is %d", MAX_AVC_LAYER));
        return false;
    }

    iEncodeParam.iNumLayer = aNumLayers;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetOutputBitRate(uint32 aLayer, uint32 aBitRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetOutputBitRate: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFAvcEncNode::SetOutputBitRate: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iBitRate[aLayer] = aBitRate;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetOutputFrameSize(uint32 aLayer, uint32 aWidth, uint32 aHeight)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetOutputFrameSize: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFAvcEncNode::SetOutputFrameSize: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iFrameWidth[aLayer] = aWidth;
    iEncodeParam.iFrameHeight[aLayer] = aHeight;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetOutputFrameRate(uint32 aLayer, OsclFloat aFrameRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetOutputFrameRate: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFAvcEncNode::SetOutputFrameRate: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iFrameRate[aLayer] = OSCL_STATIC_CAST(float, aFrameRate);
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetSegmentTargetSize(uint32 aLayer, uint32 aSizeBytes)
{
    OSCL_UNUSED_ARG(aLayer);

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetSegmentTargetSize: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    iEncodeParam.iPacketSize = aSizeBytes;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetRateControlType(uint32 aLayer, PVMFVENRateControlType aRateControl)
{
    OSCL_UNUSED_ARG(aLayer);

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetRateControlType: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    switch (aRateControl)
    {
        case PVMFVEN_RATE_CONTROL_CONSTANT_Q:
            iEncodeParam.iRateControlType = EAVCEI_RC_CONSTANT_Q;
            break;
        case PVMFVEN_RATE_CONTROL_CBR:
            iEncodeParam.iRateControlType = EAVCEI_RC_CBR_1;
            break;
        case PVMFVEN_RATE_CONTROL_VBR:
            iEncodeParam.iRateControlType = EAVCEI_RC_VBR_1;
            break;
        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetDataPartitioning(bool aDataPartitioning)
{
    OSCL_UNUSED_ARG(aDataPartitioning);

    LOG_STACK_TRACE((0, "PVMFAvcEncNode::SetDataPartitioning, do nothing"));

    return true; // do nothing for now.
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetIFrameInterval(uint32 aIFrameInterval)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetIFrameInterval: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    iEncodeParam.iIFrameInterval = aIFrameInterval;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetRVLC(bool aRVLC)
{
    OSCL_UNUSED_ARG(aRVLC);

    LOG_STACK_TRACE((0, "PVMFAvcEncNode::SetRVLC do nothing"));

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::GetVolHeader(OsclRefCounterMemFrag& aVolHeader)
{
    OSCL_UNUSED_ARG(aVolHeader);
    // this function is utility. It returns vol header to whomever using it.
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::GetVolHeader not supported"));

    return false; // just return false for now
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::RequestIFrame()
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::RequestIFrame"));

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            break;
        default:
            LOG_ERR((0, "PVMFAvcEncNode::RequestIFrame: Error - Wrong state"));
            return false;
    }

    if (!iAvcEncoder || iAvcEncoder->IDRRequest() == EAVCEI_FAIL)
    {
        LOG_ERR((0, "PVMFAvcEncNode::RequestIFrame: Error - IFrameRequest failed"));
        return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFAvcEncNode::SetCodec(PVMFFormatType aCodec)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::SetCodec %d", aCodec));

    if (SetCodecType(aCodec) == PVMFSuccess)
    {
        return true;
    }
    else
    {
        return false;
    }

}

////////////////////////////////////////////////////////////////////////////
//                        Private methods
////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::ConstructEncoderParams()
{
    iInputFormat.iVideoFormat = EAVCEI_VDOFMT_YUV420;
    iInputFormat.iFrameWidth = DEFAULT_FRAME_WIDTH;
    iInputFormat.iFrameHeight = DEFAULT_FRAME_HEIGHT;
    iInputFormat.iFrameRate = (float)DEFAULT_FRAME_RATE;

    oscl_memset(&iEncodeParam, 0, sizeof(TAVCEIEncodeParam));
    iEncodeParam.iEncodeID = 0;
    iEncodeParam.iNumLayer = 1;
    iEncodeParam.iFrameWidth[0] = DEFAULT_FRAME_WIDTH;
    iEncodeParam.iFrameHeight[0] = DEFAULT_FRAME_HEIGHT;
    iEncodeParam.iBitRate[0] = DEFAULT_BITRATE;
    iEncodeParam.iFrameRate[0] = (float)DEFAULT_FRAME_RATE;
    iEncodeParam.iIFrameInterval = 10;
    iEncodeParam.iBufferDelay = (float)5.0;
    iEncodeParam.iEncMode = EAVCEI_ENCMODE_RECORDER;
    iEncodeParam.iRateControlType = EAVCEI_RC_CBR_1;
    iEncodeParam.iIquant[0] = 0; // to enable automatic init QP selection
    iEncodeParam.iPquant[0] = 35; // doesn't matter
    iEncodeParam.iBquant[0] = 35; // doesn't matter
    iEncodeParam.iPacketSize = DEFAULT_PACKET_SIZE;
    iEncodeParam.iProfile = EAVCEI_PROFILE_BASELINE;
    iEncodeParam.iLevel = EAVCEI_LEVEL_11;
    iEncodeParam.iOutOfBandParamSet = true;
}

void PVMFAvcEncNode::freechunkavailable(OsclAny* aContextData)
{
    OSCL_UNUSED_ARG(aContextData);

    iWaitingOnFreeChunk = false;
    RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::Run()
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Run"));

    if (!iCmdQueue.empty())
    {
        if (ProcessCommand(iCmdQueue.front()))
        {
            // Need to check the state before re-scheduling since the node could have
            // been reset in the ProcessCommand call
            if (iInterfaceState != EPVMFNodeCreated)
                RunIfNotReady();
        }
    }


    if (iInterfaceState == EPVMFNodeStarted && iReadyForNextFrame == false)
    {
        /* there are bitstreams to be retrieved. */
        PVMFAvcEncNodeOutputData outputData;
        PVMFStatus status = outputData.Allocate(iMediaDataAlloc, &iMediaDataMemPool, iMediaDataGroupAlloc);
        if (status != PVMFSuccess)
        {
            LOG_DEBUG((0, "PVMFAvcEncNode::Run: Error - outputData.Allocate failed"));
            //register to receive notice when memory is available.
            iMediaBufferMemPool->notifyfreechunkavailable(*this);
            iWaitingOnFreeChunk = true;

            return ;
        }

        int32 max_length = outputData.iEncoderOutput.iBitstreamSize;
        int32 length = 0;
        uint8* firstPtr = outputData.iEncoderOutput.iBitstream;

        while (1)
        {
            OsclRefCounterMemFrag memFragOut(outputData.iRefCtrMemFragOut);

            /* need to add some code to separate between NALs of the same frame */
            if (iCodec == PVMF_H264_RAW)
            {
                outputData.iEncoderOutput.iBitstream[0] = 0;
                outputData.iEncoderOutput.iBitstream[1] = 0;
                outputData.iEncoderOutput.iBitstream[2] = 0;
                outputData.iEncoderOutput.iBitstream[3] = 1;
                outputData.iEncoderOutput.iBitstream += 4;
                outputData.iEncoderOutput.iBitstreamSize -= 4;
                length += 4;
            }

            TAVCEI_RETVAL avcStatus = iAvcEncoder->GetOutput(&(outputData.iEncoderOutput));

            if (avcStatus == EAVCEI_FAIL || avcStatus == EAVCEI_INPUT_ERROR)
            {
                LOG_ERR((0, "PVMFAvcEncNode::Run: Error iAvcEncoder->GetOutput failed"));
                return ;
            }
            else
            {
                if (iCodec == PVMF_H264_RAW)
                {
                    outputData.iEncoderOutput.iBitstream -= 4;
                    outputData.iEncoderOutput.iBitstreamSize += 4;
                }

                memFragOut.getMemFrag().ptr = (OsclAny*)outputData.iEncoderOutput.iBitstream;
                memFragOut.getMemFrag().len = outputData.iEncoderOutput.iBitstreamSize;
                outputData.iFragGroupmediaDataImpl->appendMediaFragment(memFragOut);

                outputData.iEncoderOutput.iBitstream += outputData.iEncoderOutput.iBitstreamSize;
                length += outputData.iEncoderOutput.iBitstreamSize;
                outputData.iEncoderOutput.iBitstreamSize = max_length - length;
                if (avcStatus == EAVCEI_SUCCESS) // we get the whole frame
                {
                    break;
                }
            }
        }

#if PROFILING_ON
        ++iStats.iNumFrames;
        iStats.iDuration = outputData.iEncoderOutput.iTimeStamp;
#endif
        outputData.iEncoderOutput.iBitstreamSize = length;
        outputData.iEncoderOutput.iBitstream = firstPtr;

        iReadyForNextFrame = true;

        // KL added, make sure port is activated after done with current frame.
        uint32 i;
        for (i = 0; i < iInPort.size(); i++)
            ((PVMFAvcEncPort*)iInPort[i])->ProcessIncomingMsgReady();

        status = SendEncodedBitstream(outputData);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncNode::Run: Error - SendEncodeBitstream fail"));
            return ;
        }

    }


    LOG_STACK_TRACE((0, "PVMFAvcEncNode::Run: Out"));
}

/////////////////////////////////////////////////////
//     Command processing routines
/////////////////////////////////////////////////////
PVMFCommandId PVMFAvcEncNode::QueueCommandL(PVMFAvcEncNodeCommand& aCmd)
{
    if (iInterfaceState == EPVMFNodeCreated)
    {
        OSCL_LEAVE(OsclErrNotReady);
        // return 0;	This statement was removed to avoid compiler warning for Unreachable Code
    }

    PVMFCommandId id = iCmdQueue.AddL(aCmd);
    RunIfNotReady();
    return id;
}

////////////////////////////////////////////////////////////////////////////
bool PVMFAvcEncNode::ProcessCommand(PVMFAvcEncNodeCommand& aCmd)
{
    // If a command is active, only high priority commands can interrupt
    // the processing
    if (!iCurrentCmd.empty() && !aCmd.hipri())
        return false;

    switch (aCmd.iCmd)
    {
        case PVMF_GENERIC_NODE_QUERYUUID:
            DoQueryUuid(aCmd);
            break;

        case PVMF_GENERIC_NODE_QUERYINTERFACE:
            DoQueryInterface(aCmd);
            break;

        case PVMF_GENERIC_NODE_REQUESTPORT:
            DoRequestPort(aCmd);
            break;

        case PVMF_GENERIC_NODE_RELEASEPORT:
            DoReleasePort(aCmd);
            break;

        case PVMF_GENERIC_NODE_INIT:
            DoInit(aCmd);
            break;

        case PVMF_GENERIC_NODE_PREPARE:
            DoPrepare(aCmd);
            break;

        case PVMF_GENERIC_NODE_START:
            DoStart(aCmd);
            break;

        case PVMF_GENERIC_NODE_STOP:
            DoStop(aCmd);
            break;

        case PVMF_GENERIC_NODE_FLUSH:
            DoFlush(aCmd);
            break;

        case PVMF_GENERIC_NODE_PAUSE:
            DoPause(aCmd);
            break;

        case PVMF_GENERIC_NODE_RESET:
            DoReset(aCmd);
            break;

        case PVMF_GENERIC_NODE_CANCELALLCOMMANDS:
            DoCancelAllCommands(aCmd);
            break;

        case PVMF_GENERIC_NODE_CANCELCOMMAND:
            DoCancelCommand(aCmd);
            break;

        default://unknown command type
            LOG_ERR((0, "PVMFAvcEncNode::ProcessCommand: Error - Unknown command type %d", aCmd.iCmd));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            break;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::CommandComplete(PVMFAvcEncNodeCmdQueue& aCmdQueue, PVMFAvcEncNodeCommand& aCmd,
                                     PVMFStatus aStatus, OsclAny* aData)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::CommandComplete: Id=%d, Type=%d, Status=%d, Context=0x%x, Data0x%x"
                     , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aData));

    //create response
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aData);
    PVMFSessionId session = aCmd.iSession;

    //Erase the command from the queue.
    aCmdQueue.Erase(&aCmd);

    //Report completion to the session observer.
    ReportCmdCompleteEvent(session, resp);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoQueryUuid(PVMFAvcEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::DoQueryUuid"));
    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator>* uuidvec;
    bool exactMatch;
    aCmd.Parse(mimetype, uuidvec, exactMatch);

    int32 err = 0;
    PVMFStatus status = PVMFSuccess;
    OSCL_TRY(err, uuidvec->push_back(PVMp4H263EncExtensionUUID););
    OSCL_FIRST_CATCH_ANY(err, status = PVMFErrNoMemory;);
    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoQueryInterface(PVMFAvcEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::DoQueryInterface"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.Parse(uuid, ptr);

    PVMFStatus status = PVMFSuccess;
    if (!queryInterface(*uuid, *ptr))
        status = PVMFFailure;

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoRequestPort(PVMFAvcEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::DoRequestPort"));
    int32 tag;
    OSCL_String* mimetype;
    PVMFAvcEncPort* port = NULL;
    aCmd.Parse(tag, mimetype);

    switch (tag)
    {
        case PVMF_AVCENC_NODE_PORT_TYPE_INPUT:
        {
            if (iInPort.size() >= PVMF_AVCENC_NODE_MAX_INPUT_PORT)
            {
                LOG_ERR((0, "PVMFAvcEncNode::DoRequestPort: Error - Max number of input port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVAvcEncIn";

            port = AllocatePort(iInPort, tag, mimetype, portname.get_cstr());

            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        break;

        case PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT:
        {
            if (iOutPort.size() >= PVMF_AVCENC_NODE_MAX_OUTPUT_PORT)
            {
                LOG_ERR((0, "PVMFAvcEncNode::DoRequestPort: Error - Max number of output port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVAvcEncOut";

            port = AllocatePort(iOutPort, tag, mimetype, portname.get_cstr());

            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        break;

        default:
            LOG_ERR((0, "PVMFAvcEncNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            return;
    }

    //Return the port pointer to the caller.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess, (OsclAny*)port);
}

////////////////////////////////////////////////////////////////////////////
PVMFAvcEncPort* PVMFAvcEncNode::AllocatePort(PVMFAvcEncPortVector& aPortVector, int32 aTag,
        OSCL_String* aMimeType, const char* aName)
{
    int32 err = 0;
    OsclAny* ptr = NULL;

    // Allocate a new port
    OSCL_TRY(err,
             ptr = aPortVector.Allocate();
             if (!ptr)
             OSCL_LEAVE(PVMFErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PVMFAvcEncNode::AllocatePort: Error - iPortVector Out of memory"));
                         return NULL;
                        );
    PVMFAvcEncPort* port = new(ptr) PVMFAvcEncPort(aTag, this, Priority(), aName);

    // if format was provided in mimestring, set it now.
    if (aMimeType)
    {
        PVMFFormatType format = GetFormatIndex(aMimeType->get_str());
        if ((port->SetFormat(format) != PVMFSuccess) ||
                ((aTag == PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT) && (SetCodecType(format) != PVMFSuccess)))
        {
            aPortVector.DestructAndDealloc(port);
            LOG_ERR((0, "PVMFAvcEncNode::AllocatePort: Error - port->SetFormat or SetCodecType failed"));
            return NULL;
        }
    }

    OSCL_TRY(err, aPortVector.AddL(port););
    OSCL_FIRST_CATCH_ANY(err,
                         aPortVector.DestructAndDealloc(port);
                         return NULL;
                        );

    return port;
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoReleasePort(PVMFAvcEncNodeCommand& aCmd)
{
    //Find the port in the port vector
    PVMFAvcEncPort* port = NULL;
    PVMFAvcEncPort** portPtr = NULL;
    aCmd.Parse((PVMFPortInterface*&)port);

    if (!port)
    {
        LOG_ERR((0, "PVMFAvcEncNode::DoReleasePort: Error - Invalid port pointer"));
        CommandComplete(iCmdQueue, aCmd, PVMFFailure);
        return;
    }

    PVMFStatus status = PVMFSuccess;
    switch (port->GetPortTag())
    {
        case PVMF_AVCENC_NODE_PORT_TYPE_INPUT:
            portPtr = iInPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PVMFAvcEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iInPort.Erase(portPtr);
            }
            break;

        case PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT:
            portPtr = iOutPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PVMFAvcEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iOutPort.Erase(portPtr);
            }
            break;

        default:
            LOG_ERR((0, "PVMFAvcEncNode::DoReleasePort: Error - Invalid port tag"));
            status = PVMFFailure;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoInit(PVMFAvcEncNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            SetState(EPVMFNodeInitialized);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodeInitialized:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoPrepare(PVMFAvcEncNodeCommand& aCmd)
{

    LOG_STACK_TRACE((0, "PVMFVIdeoEncNode::DoPrepare"));
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
            /* moved from DoStart to here */
            int32 leavecode;
            int32 length, new_length;
            uint8* start_ptr;
            int nalType;

            leavecode = 0;

            OSCL_TRY(leavecode, iAvcEncoder = PVAVCEncoderFactory::CreatePVAVCEncoder());

            if (iAvcEncoder->Initialize(&iInputFormat, &iEncodeParam) == EAVCEI_FAIL)
            {
                LOG_ERR((0, "PVMFAvcEncNode::DoPrepare: CommonAvcEncoder::Initialize failed"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }


            if (iCodec == PVMF_H264_MP4) // if MP4 output, get the SPS and PPS up front.
            {
                iNumSPSs = iNumPPSs = 0; // reset here

                start_ptr = (uint8*)iParamSet.getMemFragPtr();

                oscl_memset(start_ptr, 0, DEFAULT_PARAMS_SET_LENGTH); // reset to zero

                length = new_length = DEFAULT_PARAMS_SET_LENGTH;

                while (1 || length < 0)
                {

                    TAVCEI_RETVAL avcStatus = iAvcEncoder->GetParameterSet(start_ptr, &new_length, &nalType);

                    if (avcStatus == EAVCEI_FAIL)
                    {
                        break;
                    }
                    else if (avcStatus == EAVCEI_SUCCESS)
                    {
                        if (nalType == 7) // SPS
                        {
                            iSPSs[iNumSPSs].ptr = start_ptr;
                            iSPSs[iNumSPSs++].len = new_length;
                        }
                        else //if(nalType == 8) // PPS
                        {
                            iPPSs[iNumPPSs].ptr = start_ptr;
                            iPPSs[iNumPPSs++].len = new_length;
                        }

                        start_ptr += new_length;
                        length -= new_length;
                        new_length = length;
                    }
                    else
                    {
                        LOG_ERR((0, "PVMFAvcEncNode::DoPrepare: Error - PVAVCEncoder::GetParameterSet failed"));
                        CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                        return;
                    }
                }
                //iParamSet = OsclRefCounterMemFrag(paramSet, refCounter, DEFAULT_PARAMS_SET_LENGTH);

                if (length < 0)
                {
                    LOG_ERR((0, "PVMFAvcEncNode::DoPrepare: Error - Not enough memory for parameter sets"));
                    CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                    return;
                }

                if (iOutPort[0])
                {
                    iOutPort[0]->SendSPS_PPS(iSPSs, iNumSPSs, iPPSs, iNumPPSs);
                }
                else
                {
                    LOG_ERR((0, "PVMFAvcEncNode::DoPrepare: Error - cannot find output port"));
                    CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                    return;
                }
            }

            iReadyForNextFrame = true;
            /************************/
            SetState(EPVMFNodePrepared);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodePrepared:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoStart(PVMFAvcEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::DoStart"));

    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
            SetState(EPVMFNodeStarted);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;

        case EPVMFNodePaused:
            SetState(EPVMFNodeStarted);

            // Notify input port that the node is ready to process incoming msg again
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                ((PVMFAvcEncPort*)iInPort[i])->ProcessIncomingMsgReady();

            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodeStarted:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoStop(PVMFAvcEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::DoStop"));
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            // Clear queued messages in ports
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                iInPort[i]->ClearMsgQueues();
            for (i = 0; i < iOutPort.size(); i++)
                iOutPort[i]->ClearMsgQueues();

            // Video encoder is created on Start, so in parallel it's deleted in Stop
            DeleteAvcEncoder();

            //transition to Prepared state
            SetState(EPVMFNodePrepared);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;

        case EPVMFNodePrepared:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DeleteAvcEncoder()
{
    if (iAvcEncoder)
    {

        iAvcEncoder->FlushInput(); /* signal input flush */
        iAvcEncoder->CleanupEncoder();
        delete iAvcEncoder;
        iAvcEncoder = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoFlush(PVMFAvcEncNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            int32 err;
            uint32 i;
            bool msgPending;
            msgPending = false;

            for (i = 0; i < iInPort.size(); i++)
            {
                if (iInPort[i]->IncomingMsgQueueSize() > 0)
                    msgPending = true;
                iInPort[i]->SuspendInput();
                if (iInterfaceState != EPVMFNodeStarted)
                {
                    // Port is in idle if node state is not started. Call ProcessIncomingMsgReady
                    // to wake up port AO
                    ((PVMFAvcEncPort*)iInPort[i])->ProcessIncomingMsgReady();
                }
            }

            for (i = 0; i < iOutPort.size(); i++)
            {
                if (iOutPort[i]->OutgoingMsgQueueSize() > 0)
                    msgPending = true;
                iOutPort[i]->SuspendInput();
                if (iInterfaceState != EPVMFNodeStarted)
                {
                    // Port is in idle if node state is not started. Call ProcessOutgoingMsgReady
                    // to wake up port AO
                    ((PVMFAvcEncPort*)iOutPort[i])->ProcessOutgoingMsgReady();
                }
            }

            //the flush is asynchronous.  move the command from
            //the input command queue to the current command, where
            //it will remain until the flush completes.
            OSCL_TRY(err, iCurrentCmd.StoreL(aCmd););
            OSCL_FIRST_CATCH_ANY(err,
                                 CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                                 return;
                                );
            iCmdQueue.Erase(&aCmd);
            if (!msgPending)
            {
                FlushComplete();
                return;
            }
            break;

        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
bool PVMFAvcEncNode::IsFlushPending()
{
    return (iCurrentCmd.size() > 0
            && iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::FlushComplete()
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::FlushComplete"));
    uint32 i = 0;

    // Flush is complete only when all queues of all ports are clear.
    // Other wise, just return from this method and wait for FlushComplete
    // from the remaining ports.
    for (i = 0; i < iInPort.size(); i++)
    {
        if (iInPort[i]->IncomingMsgQueueSize() > 0 ||
                iInPort[i]->OutgoingMsgQueueSize() > 0)
        {
            return;
        }
    }

    for (i = 0; i < iOutPort.size(); i++)
    {
        if (iOutPort[i]->IncomingMsgQueueSize() > 0 ||
                iOutPort[i]->OutgoingMsgQueueSize() > 0)
        {
            return;
        }
    }

    // Video encoder is created on Start, so in parallel it's deleted when Flush is completed
    DeleteAvcEncoder();
#if PROFILING_ON
    LOG_DEBUG((0, "PVMFAvcEncNode Stats: NumFrames=%d, NumSkippedFrames=%d, Duration=%d",
               iStats.iNumFrames, iStats.iNumFramesSkipped, iStats.iDuration));
    oscl_memset(&iStats, 0, sizeof(PVAvcEncNodeStats));
#endif

    //resume port input so the ports can be re-started.
    for (i = 0; i < iInPort.size(); i++)
        iInPort[i]->ResumeInput();
    for (i = 0; i < iOutPort.size(); i++)
        iOutPort[i]->ResumeInput();

    // Flush is complete.  Go to prepared state.
    SetState(EPVMFNodePrepared);
    CommandComplete(iCurrentCmd, iCurrentCmd.front(), PVMFSuccess);

    if (!iCmdQueue.empty())
    {
        // If command queue is not empty, schedule to process the next command
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoPause(PVMFAvcEncNodeCommand& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
            SetState(EPVMFNodePaused);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        case EPVMFNodePaused:
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;
        default:
            CommandComplete(iCmdQueue, aCmd, PVMFErrInvalidState);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoReset(PVMFAvcEncNodeCommand& aCmd)
{
    //This example node allows a reset from any idle state.
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif

    if (IsAdded())
    {
        while (!iInPort.empty())
            iInPort.Erase(&iInPort.front());
        while (!iOutPort.empty())
            iOutPort.Erase(&iOutPort.front());

        //restore original port vector reserve.
        iInPort.Reconstruct();
        iOutPort.Reconstruct();

        //logoff & go back to Created state.
        SetState(EPVMFNodeIdle);
        PVMFStatus status = ThreadLogoff();

        CommandComplete(iCmdQueue, aCmd, status);
    }
    else
    {
        OSCL_LEAVE(OsclErrInvalidState);
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoCancelAllCommands(PVMFAvcEncNodeCommand& aCmd)
{
    //first cancel the current command if any
    {
        while (!iCurrentCmd.empty())
            CommandComplete(iCurrentCmd, iCurrentCmd[0], PVMFErrCancelled);
    }

    //next cancel all queued commands
    {
        //start at element 1 since this cancel command is element 0.
        while (iCmdQueue.size() > 1)
            CommandComplete(iCmdQueue, iCmdQueue[1], PVMFErrCancelled);
    }

    //finally, report cancel complete.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::DoCancelCommand(PVMFAvcEncNodeCommand& aCmd)
{
    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.Parse(id);

    //first check "current" command if any
    {
        PVMFAvcEncNodeCommand* cmd = iCurrentCmd.FindById(id);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iCurrentCmd, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            return;
        }
    }

    //next check input queue.
    {
        //start at element 1 since this cancel command is element 0.
        PVMFAvcEncNodeCommand* cmd = iCmdQueue.FindById(id, 1);
        if (cmd)
        {
            //cancel the queued command
            CommandComplete(iCmdQueue, *cmd, PVMFErrCancelled);
            //report cancel success
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            return;
        }
    }
    //if we get here the command isn't queued so the cancel fails.
    CommandComplete(iCmdQueue, aCmd, PVMFFailure);
}

////////////////////////////////////////////////////////////////////////////
//               Port activity processing routines
////////////////////////////////////////////////////////////////////////////
bool PVMFAvcEncNode::IsProcessOutgoingMsgReady()
{
    if (iInterfaceState == EPVMFNodeStarted || IsFlushPending())
    {
        for (uint32 i = 0; i < iOutPort.size(); i++)
        {
            if (iOutPort[i]->IsConnectedPortBusy())
                return false;
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
bool PVMFAvcEncNode::IsProcessIncomingMsgReady()
{
    if (iInterfaceState == EPVMFNodeStarted || IsFlushPending())
    {
        if (!iReadyForNextFrame) // still encoding the current frame.
        {
            return false;
        }

        for (uint32 i = 0; i < iOutPort.size(); i++)
        {
            if (iOutPort[i]->IsOutgoingQueueBusy())
                return false;
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::ProcessIncomingMsg: aPort=0x%x", aPort));
    PVMFStatus status = PVMFFailure;

    switch (aPort->GetPortTag())
    {
        case PVMF_AVCENC_NODE_PORT_TYPE_INPUT:
        {
            PVMFSharedMediaMsgPtr msg;
            status = aPort->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PVMFAvcEncNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed"));
                return status;
            }

            if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                status = SendEndOfTrackCommand(msg);
                return status;
            }
            // Put the data on the data queue
            PVMFSharedMediaDataPtr mediaData;
            convertToPVMFMediaData(mediaData, msg);
            status = AsyncEncode(mediaData); // set it to encode asynchronously
            switch (status)
            {
                case PVMFSuccess:
                    break;
                    /*case PVMFErrBusy:  // this is treated as error.
                    	LOG_DEBUG((0,"PVMFAvcEncNode::ProcessIncomingMsg: Outgoing queue busy. This should not happen."));
                    	break;*/
                default:
                    ReportErrorEvent(PVMF_AVCENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)aPort);
                    break;
            }
        }
        break;

        case PVMF_AVCENC_NODE_PORT_TYPE_OUTPUT:
            // Nothing to be done
            status = PVMFSuccess;
            break;

        default:
            LOG_ERR((0, "PVMFAvcEncNode::ProcessIncomingMsg: Error - Invalid port tag"));
            ReportErrorEvent(PVMF_AVCENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)aPort);
            status = PVMFFailure;
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::AsyncEncode(PVMFSharedMediaDataPtr& aMediaData)
{
    TAVCEI_RETVAL status;

    // Get the next data fragment
    OsclRefCounterMemFrag frag;
    aMediaData->getMediaFragment(0, frag);

    TAVCEIInputData inputData;
    inputData.iSource = (uint8*)frag.getMemFragPtr();
    inputData.iTimeStamp = aMediaData->getTimestamp();

    LOG_DEBUG((0, "PVMFAvcEncNode::SyncEncodeAndSend(): Encoding frame %d", inputData.iTimeStamp));

#if PROFILING_ON
    uint32 start = OsclTickCount::TickCount();
#endif

    status = iAvcEncoder->Encode(&inputData) ;
#if PROFILING_ON

    uint32 stop = OsclTickCount::TickCount();
    uint32 enctime = OsclTickCount::TicksToMsec(stop - start);
    if ((iMinEncDuration > enctime) || (0 == iMinEncDuration))
    {
        iMinEncDuration = enctime;
    }

    if (iMaxEncDuration < enctime)
    {
        iMaxEncDuration = enctime;
    }

    if (status == EAVCEI_SUCCESS)
    {
        ++iTotalFramesEncoded;
    }

    iTotalEncTime += enctime;

#endif

    if (status == EAVCEI_FRAME_DROP)
    {

#if	 PROFILING_ON
        ++iStats.iNumFramesSkipped;
        ++iNumFramesSkipped;
#endif
        LOG_DEBUG((0, "PVMFAvcEncNode::SyncEncodeAndSend(): Skipped frame"));

        // KL added, make sure port is activated after the dropped frame
        uint32 i = 0;
        for (i = 0; i < iInPort.size(); i++)
            ((PVMFAvcEncPort*)iInPort[i])->ProcessIncomingMsgReady();

        return PVMFSuccess; // do nothing, move on to the next frame
    }
    else if (status != EAVCEI_SUCCESS) // this should not happen
    {
        LOG_ERR((0, "PVMFAvcEncNode::ASyncEncode(): Error - EncodeFrame failed"));
        return PVMFFailure;
    }

    // else set Run to get output
    iReadyForNextFrame = false;
    RunIfNotReady();

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::SendEncodedBitstream(PVMFAvcEncNodeOutputData& aOutputData)
{
    PVMFStatus status = PVMFSuccess;

    aOutputData.iMediaData->setTimestamp(aOutputData.iEncoderOutput.iTimeStamp);
    aOutputData.iMediaData->setSeqNum(iSeqNum++);

    OsclSharedPtr<PVMFMediaDataImpl> media_data_impl;
    aOutputData.iMediaData->getMediaDataImpl(media_data_impl);

    if (aOutputData.iEncoderOutput.iKeyFrame == true)
        media_data_impl->setMarkerInfo(1);
    else
        media_data_impl->setMarkerInfo(0);

    /*** old code
    // update the filled length of the fragment
    //aOutputData.iMediaData->setMediaFragFilledLen(0, aOutputData.iEncoderOutput.iBitstreamSize);

    // Set timestamp
    //aOutputData.iMediaData->setTimestamp(aOutputData.iEncoderOutput.iTimeStamp);

    // Set sequence number
    //aOutputData.iMediaData->setSeqNum(iSeqNum++);

    // Send paramSet for avc bitstream // sent via Kvp instead
    //if(aOutputData.iMediaData->getSeqNum() == 0 && iCodec == PVMF_H264_MP4)
    	//aOutputData.iMediaData->setFormatSpecificInfo(iParamSet);
    ****/

    // Send bitstream data to downstream node
    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaMsg(mediaMsgOut, aOutputData.iMediaData);

    for (uint32 i = 0; i < iOutPort.size(); i++)
    {
        status = iOutPort[i]->QueueOutgoingMsg(mediaMsgOut);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFAvcEncNode::SendEncodedBitstream: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
//                 Encoder settings routines
////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::SetCodecType(PVMFFormatType aCodec)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetCodecType: Error iInterfaceState=%d", iInterfaceState));
            return PVMFErrInvalidState;
        default:
            break;
    }

    iCodec = aCodec;

    switch (iCodec)
    {
        case PVMF_H264_RAW:
            iEncodeParam.iOutOfBandParamSet = false;
            break;
        case PVMF_H264_MP4:
            iEncodeParam.iOutOfBandParamSet = true;
            break;
        default:
            LOG_ERR((0, "PVMFAvcEncNode::SetCodecType: RFC 3984 not supported"));
            return PVMFErrNotSupported;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::SetInputFormat(PVMFFormatType aFormat)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::SetInputFormat: aFormat=%d", aFormat));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetInputFormat: Error - iInterfaceState=%d", iInterfaceState));
            return PVMFErrInvalidState;
        default:
            break;
    }

    switch (aFormat)
    {
        case PVMF_YUV420:
            iInputFormat.iVideoFormat = EAVCEI_VDOFMT_YUV420;
            break;
        case PVMF_YUV422:
            iInputFormat.iVideoFormat = EAVCEI_VDOFMT_UYVY;
            break;
        case PVMF_RGB24:
            iInputFormat.iVideoFormat = EAVCEI_VDOFMT_RGB24;
            break;
        case PVMF_RGB12:
            iInputFormat.iVideoFormat = EAVCEI_VDOFMT_RGB12;
            break;
        default:
            return PVMFFailure;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::SetInputFrameSize(uint32 aWidth, uint32 aHeight, uint8 aFrmOrient)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetInputFrameSize: Error iInterfaceState=%d", iInterfaceState));
            return false;
        default:
            break;
    }

    iInputFormat.iFrameWidth = aWidth;
    iInputFormat.iFrameHeight = aHeight;
    iInputFormat.iFrameOrientation = aFrmOrient;
    return true;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::SetInputFrameRate(OsclFloat aFrameRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFAvcEncNode::SetInputFrameRate: Error iInterfaceState=%d", iInterfaceState));
            return false;
        default:
            break;
    }

    iInputFormat.iFrameRate = aFrameRate;
    return true;
}

////////////////////////////////////////////////////////////////////////////
PVMFFormatType PVMFAvcEncNode::GetCodecType()
{
    return iCodec;
}

////////////////////////////////////////////////////////////////////////////
uint32 PVMFAvcEncNode::GetOutputBitRate(uint32 aLayer)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFAvcEncNode::GetOutputBitRate: Error - Invalid layer number"));
        return 0;
    }

    return iEncodeParam.iBitRate[aLayer];
}

////////////////////////////////////////////////////////////////////////////
OsclFloat PVMFAvcEncNode::GetOutputFrameRate(uint32 aLayer)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFAvcEncNode::GetOutputFrameRate: Error Invalid layer number"));
        return 0;
    }

    return iEncodeParam.iFrameRate[aLayer];
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::GetOutputFrameSize(uint32 aLayer, uint32& aWidth, uint32& aHeight)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFAvcEncNode::GetOutputFrameSize: Error Invalid layer number"));
        return PVMFFailure;
    }

    aWidth = iEncodeParam.iFrameWidth[aLayer];
    aHeight = iEncodeParam.iFrameHeight[aLayer];
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
uint32 PVMFAvcEncNode::GetIFrameInterval()
{
    return iEncodeParam.iIFrameInterval;
}

////////////////////////////////////////////////////////////////////////////
//                 Event reporting routines.
////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::SetState(TPVMFNodeInterfaceState aState)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::SetState %d", aState));
    PVMFNodeInterface::SetState(aState);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_ERR((0, "PVMFAvcEncNode::ReportErrorEvent: aEventType=%d aEventData=0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData);
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_STACK_TRACE((0, "PVMFAvcEncNode::ReportInfoEvent: aEventType=%d, aEventData0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData);
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNode::SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg)
{
    PVMFSharedMediaCmdPtr sharedMediaCmdPtr = PVMFMediaCmd::createMediaCmd();

    sharedMediaCmdPtr->setFormatID(PVMF_MEDIA_CMD_EOS_FORMAT_ID);

    // Set the timestamp
    sharedMediaCmdPtr->setTimestamp(aMsg->getTimestamp());

    // Set the sequence number
    sharedMediaCmdPtr->setSeqNum(aMsg->getSeqNum());

    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaCmdMsg(mediaMsgOut, sharedMediaCmdPtr);

    for (uint32 ii = 0; ii < iOutPort.size(); ii++)
    {
        PVMFStatus status = iOutPort[ii]->QueueOutgoingMsg(mediaMsgOut);

        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFVideoEncNode::SendEndOfTrackCommand: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PVMFAvcEncNode::LogDiagnostics()
{
#if PROFILING_ON
    oDiagnosticsLogged = true;
    if (iTotalFramesEncoded > 0)
    {
        iAverageEncDuration = iTotalEncTime / iTotalFramesEncoded;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_DEBUG,
                    (0, "PVMFAvcEncNodeEnc Stats: Encode Duration(Min: %d ,Max: %d, Average: %d), Total Enc Time: %d\n", iMinEncDuration, iMaxEncDuration, iAverageEncDuration, iTotalEncTime));
#endif
}

////////////////////////////////////////////////////////////////////////////
//            PVMFAvcEncNodeOutputData implementation
////////////////////////////////////////////////////////////////////////////
PVMFAvcEncNodeOutputData::PVMFAvcEncNodeOutputData()
{
    oscl_memset(&iEncoderOutput, 0, sizeof(TAVCEIOutputData));
}

////////////////////////////////////////////////////////////////////////////
PVMFAvcEncNodeOutputData::PVMFAvcEncNodeOutputData(const PVMFAvcEncNodeOutputData& aData)
{
    iEncoderOutput.iBitstream = aData.iEncoderOutput.iBitstream;
    iEncoderOutput.iBitstreamSize = aData.iEncoderOutput.iBitstreamSize;
    iEncoderOutput.iTimeStamp = aData.iEncoderOutput.iTimeStamp;
    iEncoderOutput.iFragment = aData.iEncoderOutput.iFragment;
    iEncoderOutput.iLastFragment = aData.iEncoderOutput.iLastFragment;
    iEncoderOutput.iKeyFrame = aData.iEncoderOutput.iKeyFrame;
    iEncoderOutput.iLastNAL = aData.iEncoderOutput.iLastNAL;

    iMediaData = aData.iMediaData;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFAvcEncNodeOutputData::Allocate(PVMFSimpleMediaBufferCombinedAlloc* aBufferAlloc,
        PVMFAvcEncNodeMemPool* aMemPool,
        PVMFMediaFragGroupCombinedAlloc<OsclMemPoolFixedChunkAllocator>* aGroupAlloc)
{
    int32 err = 0;
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImplOut;

    OSCL_TRY(err, mediaDataImplOut = aBufferAlloc->allocate(MAX_OUTBUF_SIZE);); // 8192
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    OSCL_TRY(err, iFragGroupmediaDataImpl = aGroupAlloc->allocate(););
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    /* Now create a PVMF media data from pool */
    OSCL_TRY(err, iMediaData = PVMFMediaData::createMediaData(iFragGroupmediaDataImpl, aMemPool););
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    // Retrieve memory fragment to write to
    //iMediaData->getMediaFragment(0, refCtrMemFragOut);
    mediaDataImplOut->getMediaFragment(0, iRefCtrMemFragOut);
    iEncoderOutput.iBitstream = (uint8*)iRefCtrMemFragOut.getMemFrag().ptr;
    oscl_memset(iEncoderOutput.iBitstream, 0, MAX_OUTBUF_SIZE);
    iEncoderOutput.iBitstreamSize = MAX_OUTBUF_SIZE;
    return PVMFSuccess;
}

