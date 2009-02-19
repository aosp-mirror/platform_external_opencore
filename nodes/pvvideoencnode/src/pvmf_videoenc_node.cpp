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
#ifndef PVMF_VIDEOENC_NODE_H_INCLUDED
#include "pvmf_videoenc_node.h"
#endif
#ifndef OSCL_ERROR_CODES_H_INCLUDED
#include "oscl_error_codes.h"
#endif
#ifndef PVMF_VIDEOENC_PORT_H_INCLUDED
#include "pvmf_videoenc_port.h"
#endif
#ifndef PVMF_VIDEOENC_NODE_FACTORY_H_INCLUDED
#include "pvmf_videoenc_node_factory.h"
#endif
#ifndef PVMF_VIDEOENC_NODE_TYPES_H_INCLUDED
#include "pvmf_videoenc_node_types.h"
#endif
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif
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

const uint32 DEFAULT_VOL_HEADER_LENGTH = 28;
const uint8 DEFAULT_VOL_HEADER[DEFAULT_VOL_HEADER_LENGTH] =
{
    0x00, 0x00, 0x01, 0xB0, 0x08, 0x00, 0x00, 0x01,
    0xB5, 0x09,	0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x20, 0x00, 0x84, 0x40, 0xFA, 0x28, 0x2C,
    0x20, 0x90, 0xA2, 0x1F
};

////////////////////////////////////////////////////////////////////////////
//              PVMFVideoEncNodeFactory implementation
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFNodeInterface* PVMFVideoEncNodeFactory::CreateVideoEncNode(int32 aPriority)
{
    int32 err = 0;
    PVMFVideoEncNode* node = NULL;

    OSCL_TRY(err,
             node = OSCL_NEW(PVMFVideoEncNode, (aPriority));
             if (!node)
             OSCL_LEAVE(OsclErrNoMemory);
            );
    OSCL_FIRST_CATCH_ANY(err, return NULL;);

    return (PVMFNodeInterface*)node;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNodeFactory::DeleteVideoEncNode(PVMFNodeInterface* aNode)
{
    if (!aNode)
        return false;

    PVMFVideoEncNode* node = (PVMFVideoEncNode*)aNode;
    OSCL_DELETE(node);
    aNode = NULL;
    return true;
}

////////////////////////////////////////////////////////////////////////////
//              PVMFVideoEncNode implementation
////////////////////////////////////////////////////////////////////////////
PVMFVideoEncNode::PVMFVideoEncNode(int32 aPriority) :
        OsclTimerObject(aPriority, "PVMFVideoEncNode"),
        iMediaBufferMemPool(PVVIDENC_MEDIADATA_POOLNUM, PVVIDENC_MEDIABUFFER_CHUNKSIZE),
        iMediaDataMemPool(PVVIDENC_MEDIADATA_POOLNUM, PVVIDENC_MEDIADATA_CHUNKSIZE),
        iVideoEncoder(NULL),
        iSeqNum(0),
        iDiagnosticsLogger(NULL),
        total_ticks(0),
        iExtensionRefCount(0)
{
    iInterfaceState = EPVMFNodeCreated;

    // Allocate memory for VOL header
    uint refCounterSize = oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
    uint size = refCounterSize + DEFAULT_VOL_HEADER_LENGTH;
    uint8 *memBuffer = NULL;

    int32 err;
    OSCL_TRY(err,
             //Create the input command queue
             iCmdQueue.Construct(PVMF_VIDEOENC_NODE_CMD_ID_START, PVMF_VIDEOENC_NODE_CMD_QUEUE_RESERVE);
             iCurrentCmd.Construct(0, 1); // There's only 1 current command

             //Create the port vector.
             iInPort.Construct(PVMF_VIDEOENC_NODE_PORT_VECTOR_RESERVE);
             iOutPort.Construct(PVMF_VIDEOENC_NODE_PORT_VECTOR_RESERVE);

             // Reserve space for port activity queue
             iPortActivityQueue.reserve(PVMF_VIDEOENC_NODE_PORT_ACTIVITY_RESERVE);

             // Create media data allocator
             iMediaDataAlloc = OSCL_NEW(PVMFSimpleMediaBufferCombinedAlloc, (&iMediaBufferMemPool));
             if (!iMediaDataAlloc)
             OSCL_LEAVE(OsclErrNoMemory);

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
    oscl_memset(memBuffer, 0, DEFAULT_VOL_HEADER_LENGTH);
    OsclMemoryFragment volHeader;
    OsclRefCounter* refCounter = new(memBuffer) OsclRefCounterDA(memBuffer,
            (OsclDestructDealloc*)&iAlloc);
    memBuffer += refCounterSize;
    volHeader.ptr = memBuffer;
    oscl_memcpy(volHeader.ptr, (OsclAny*)DEFAULT_VOL_HEADER, DEFAULT_VOL_HEADER_LENGTH);
    volHeader.len = DEFAULT_VOL_HEADER_LENGTH;
    iVolHeader = OsclRefCounterMemFrag(volHeader, refCounter, DEFAULT_VOL_HEADER_LENGTH);

    ConstructEncoderParams();

#if PROFILING_ON
    oscl_memset(&iStats, 0, sizeof(PVVideoEncNodeStats));
    iStats.iMinEncTime = 0;
    oDiagnosticsLogged = false;
#endif
    iLogger = PVLogger::GetLoggerObject("PVMFVideoEncNode");
    iDiagnosticsLogger = PVLogger::GetLoggerObject("pvauthordiagnostics.encnode.h263encnode");

}

////////////////////////////////////////////////////////////////////////////
PVMFVideoEncNode::~PVMFVideoEncNode()
{
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif

    if (iMediaDataAlloc)
    {
        OSCL_DELETE(iMediaDataAlloc);
        iMediaDataAlloc = NULL;
    }

    DeleteVideoEncoder();

    while (!iInPort.empty())
        iInPort.Erase(&iInPort.front());
    while (!iOutPort.empty())
        iOutPort.Erase(&iOutPort.front());

    // Clean up command queues
    while (!iCmdQueue.empty())
    {
        CommandComplete(iCmdQueue, iCmdQueue[0], PVMFFailure);
    }

    while (!iCurrentCmd.empty())
    {
        CommandComplete(iCurrentCmd, iCurrentCmd[0], PVMFFailure);
    }


    Cancel();
    SetState(EPVMFNodeIdle);
    ThreadLogoff();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFVideoEncNode::ThreadLogon()
{
    switch (iInterfaceState)
    {
        case EPVMFNodeCreated:
            if (!IsAdded())
                AddToScheduler();
            SetState(EPVMFNodeIdle);
            return PVMFSuccess;

        default:
            return PVMFErrInvalidState;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMFVideoEncNode::ThreadLogoff()
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
OSCL_EXPORT_REF PVMFStatus PVMFVideoEncNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::GetCapability"));

    aNodeCapability.iInputFormatCapability.push_back(PVMF_YUV420);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_M4V);
    aNodeCapability.iOutputFormatCapability.push_back(PVMF_H263);
    aNodeCapability.iCanSupportMultipleOutputPorts = false;
    aNodeCapability.iCanSupportMultipleInputPorts = false;
    aNodeCapability.iHasMaxNumberOfPorts = true;
    aNodeCapability.iMaxNumberOfPorts = 2;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFPortIter* PVMFVideoEncNode::GetPorts(const PVMFPortFilter* aFilter)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::GetPorts"));
    OSCL_UNUSED_ARG(aFilter);//port filter is not implemented.

    int32 err = 0;
    PVMFVideoEncPortVector* port = NULL;
    OSCL_TRY(err,
             port = OSCL_NEW(PVMFVideoEncPortVector, ());
             if (!port)
             return NULL;

             uint32 i;
             for (i = 0; i < iInPort.size(); i++)
                 port->AddL(iInPort[i]);
                 for (i = 0; i < iOutPort.size(); i++)
                     port->AddL(iOutPort[i]);
                     port->Reset();
                    );

    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PVMFVideoEncNode::GetPorts: Error - Out of memory"));
                        );

    return port;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::QueryUUID(PVMFSessionId aSession,
        const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::QueryUUID"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYUUID,
                  aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::QueryInterface(PVMFSessionId aSession,
        const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::QueryInterface"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYINTERFACE,
                  aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::RequestPort(PVMFSessionId aSession,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::RequestPort: aPortTag=%d", aPortTag));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_REQUESTPORT,
                  aPortTag, aPortConfig, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::ReleasePort(PVMFSessionId aSession,
        PVMFPortInterface& aPort,
        const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::ReleasePort"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Init(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Init"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_INIT, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Prepare(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Prepare"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PREPARE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Start(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Start"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_START, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Stop(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Stop"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_STOP, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Flush(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Flush"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_FLUSH, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Pause(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Pause"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PAUSE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::Reset(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Reset"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RESET, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::CancelAllCommands"));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELALLCOMMANDS, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMFVideoEncNode::CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::CancelCommand: aCmdId=%d", aCmdId));
    PVMFVideoEncNodeCommand cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELCOMMAND, aCmdId, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::HandlePortActivity(const PVMFPortActivity &aActivity)
{
    // Only outgoing queue ready activity are forwarded on to the node
    // by the output port.  The rest of the events for output port and all events
    // for input port are handled within the port AO.
    if (aActivity.iPort->GetPortTag() == PVMF_VIDEOENC_NODE_PORT_TYPE_OUTPUT)
    {
        switch (aActivity.iType)
        {
            case PVMF_PORT_ACTIVITY_OUTGOING_QUEUE_READY:
                // Notifies the node that the output queue is ready, and the node would
                // resume encoding incoming data
                uint32 i;
                for (i = 0; i < iInPort.size(); i++)
                    ((PVMFVideoEncPort*)iInPort[i])->ProcessIncomingMsgReady();
                break;

            default:
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFVideoEncNode::addRef()
{
    ++iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMFVideoEncNode::removeRef()
{
    if (iExtensionRefCount > 0)
        --iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
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
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetNumLayers(uint32 aNumLayers)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetNumLayers: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if (aNumLayers > MAX_LAYER) // MAX_LAYER defined in cvei.h
    {
        LOG_ERR((0, "PVMFVideoEncNode::SetNumLayers: Error Max num layers is %d", MAX_LAYER));
        return false;
    }

    iEncodeParam.iNumLayer = aNumLayers;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetOutputBitRate(uint32 aLayer, uint32 aBitRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetOutputBitRate: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFVideoEncNode::SetOutputBitRate: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iBitRate[aLayer] = aBitRate;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetOutputFrameSize(uint32 aLayer, uint32 aWidth, uint32 aHeight)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetOutputFrameSize: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFVideoEncNode::SetOutputFrameSize: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iFrameWidth[aLayer] = aWidth;
    iEncodeParam.iFrameHeight[aLayer] = aHeight;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetOutputFrameRate(uint32 aLayer, OsclFloat aFrameRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetOutputFrameRate: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFVideoEncNode::SetOutputFrameRate: Error Invalid layer number"));
        return false;
    }

    iEncodeParam.iFrameRate[aLayer] = OSCL_STATIC_CAST(float, aFrameRate);
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetSegmentTargetSize(uint32 aLayer, uint32 aSizeBytes)
{
    OSCL_UNUSED_ARG(aLayer);

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetSegmentTargetSize: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    iEncodeParam.iPacketSize = aSizeBytes;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetRateControlType(uint32 aLayer, PVMFVENRateControlType aRateControl)
{
    OSCL_UNUSED_ARG(aLayer);

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetRateControlType: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    switch (aRateControl)
    {
        case PVMFVEN_RATE_CONTROL_CONSTANT_Q:
            iEncodeParam.iRateControlType = ECONSTANT_Q;
            break;
        case PVMFVEN_RATE_CONTROL_CBR:
            iEncodeParam.iRateControlType = ECBR_1;
            break;
        case PVMFVEN_RATE_CONTROL_VBR:
            iEncodeParam.iRateControlType = EVBR_1;
            break;
        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetDataPartitioning(bool aDataPartitioning)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetDataPartitioning: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    if (iEncodeParam.iContentType == ECVEI_H263)
    {
        LOG_ERR((0, "PVMFVideoEncNode::SetDataPartitioning: Error data partitioning not supported for H263"));
        return false;
    }

    if (aDataPartitioning)
        iEncodeParam.iContentType = ECVEI_STREAMING;
    else
        iEncodeParam.iContentType = ECVEI_DOWNLOAD;

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetIFrameInterval(uint32 aIFrameInterval)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetIFrameInterval: Error iInterfaceState=%d", iInterfaceState));
            return false;

        default:
            break;
    }

    iEncodeParam.iIFrameInterval = aIFrameInterval;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetRVLC(bool aRVLC)
{
    OSCL_UNUSED_ARG(aRVLC);

    LOG_STACK_TRACE((0, "PVMFVideoEncNode::SetRVLC"));
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::GetVolHeader(OsclRefCounterMemFrag& aVolHeader)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::GetVolHeader"));

    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
        case EPVMFNodePrepared:
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            break;

        default:
            LOG_ERR((0, "PVMFVideoEncNode::GetVolHeader: Error - Wrong state"));
            return false;
    }

    if (iEncodeParam.iContentType == ECVEI_H263)
    {
        LOG_ERR((0, "PVMFVideoEncNode::GetVolHeader: Error - VOL header only for M4V encode"));
        return false;
    }

    uint8 *ptr = (uint8 *) iVolHeader.getMemFragPtr();
    //If data partioning mode
    if (iEncodeParam.iContentType == ECVEI_STREAMING)
    {
        ptr[iVolHeader.getMemFragSize() - 1] = 0x8F;
    }
    //else combined mode
    else
    {
        ptr[iVolHeader.getMemFragSize() - 1] = 0x1F;
    }

    aVolHeader = iVolHeader;
    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::RequestIFrame()
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::RequestIFrame"));

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            break;
        default:
            LOG_ERR((0, "PVMFVideoEncNode::RequestIFrame: Error - Wrong state"));
            return false;
    }

    if (!iVideoEncoder || iVideoEncoder->IFrameRequest() == ECVEI_FAIL)
    {
        LOG_ERR((0, "PVMFVideoEncNode::RequestIFrame: Error - IFrameRequest failed"));
        return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMFVideoEncNode::SetCodec(PVMFFormatType aCodec)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::SetCodec %d", aCodec));

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
void PVMFVideoEncNode::HandlePVCVEIEvent(uint32 aId, uint32 aEvent, uint32 aParam1)
{
    OSCL_UNUSED_ARG(aId);
    OSCL_UNUSED_ARG(aEvent);
    OSCL_UNUSED_ARG(aParam1);
}

////////////////////////////////////////////////////////////////////////////
//                        Private methods
////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::ConstructEncoderParams()
{
    iInputFormat.iVideoFormat = ECVEI_YUV420;
    iInputFormat.iFrameWidth = DEFAULT_FRAME_WIDTH;
    iInputFormat.iFrameHeight = DEFAULT_FRAME_HEIGHT;
    iInputFormat.iFrameRate = (float)DEFAULT_FRAME_RATE;

    oscl_memset(&iEncodeParam, 0, sizeof(TPVVideoEncodeParam));
    iEncodeParam.iEncodeID = 0;
    iEncodeParam.iNumLayer = 1;
    iEncodeParam.iFrameWidth[0] = DEFAULT_FRAME_WIDTH;
    iEncodeParam.iFrameHeight[0] = DEFAULT_FRAME_HEIGHT;
    iEncodeParam.iBitRate[0] = DEFAULT_BITRATE;
    iEncodeParam.iFrameRate[0] = (float)DEFAULT_FRAME_RATE;
    iEncodeParam.iFrameQuality = 10;
    iEncodeParam.iIFrameInterval = 10;
    iEncodeParam.iBufferDelay = (float)0.2;
    iEncodeParam.iContentType = ECVEI_H263;
    iEncodeParam.iRateControlType = ECBR_1;
    iEncodeParam.iIquant[0] = 15;
    iEncodeParam.iPquant[0] = 12;
    iEncodeParam.iBquant[0] = 12;
    iEncodeParam.iSearchRange = 16;
    iEncodeParam.iMV8x8 = false;
    iEncodeParam.iPacketSize = 256;
    iEncodeParam.iNoCurrentSkip = false;
    iEncodeParam.iNoFrameSkip = false;
    iEncodeParam.iClipDuration = 0;
    iEncodeParam.iProfileLevel = ECVEI_CORE_LEVEL2;
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::Run()
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Run"));

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

    LOG_STACK_TRACE((0, "PVMFVideoEncNode::Run: Out"));
}

/////////////////////////////////////////////////////
//     Command processing routines
/////////////////////////////////////////////////////
PVMFCommandId PVMFVideoEncNode::QueueCommandL(PVMFVideoEncNodeCommand& aCmd)
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
bool PVMFVideoEncNode::ProcessCommand(PVMFVideoEncNodeCommand& aCmd)
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
            LOG_ERR((0, "PVMFVideoEncNode::ProcessCommand: Error - Unknown command type %d", aCmd.iCmd));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            break;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::CommandComplete(PVMFVideoEncNodeCmdQueue& aCmdQueue, PVMFVideoEncNodeCommand& aCmd,
                                       PVMFStatus aStatus, OsclAny* aData)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::CommandComplete: Id=%d, Type=%d, Status=%d, Context=0x%x, Data0x%x"
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
void PVMFVideoEncNode::DoQueryUuid(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::DoQueryUuid"));
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
void PVMFVideoEncNode::DoQueryInterface(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::DoQueryInterface"));
    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.Parse(uuid, ptr);

    PVMFStatus status = PVMFSuccess;
    if (!queryInterface(*uuid, *ptr))
        status = PVMFFailure;

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::DoRequestPort(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::DoRequestPort"));
    int32 tag;
    OSCL_String* mimetype;
    PVMFVideoEncPort* port = NULL;
    aCmd.Parse(tag, mimetype);

    switch (tag)
    {
        case PVMF_VIDEOENC_NODE_PORT_TYPE_INPUT:
        {
            if (iInPort.size() >= PVMF_VIDEOENC_NODE_MAX_INPUT_PORT)
            {
                LOG_ERR((0, "PVMFVideoEncNode::DoRequestPort: Error - Max number of input port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVVideoEncIn";

            port = AllocatePort(iInPort, tag, mimetype, portname.get_cstr());
            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }

        break;

        case PVMF_VIDEOENC_NODE_PORT_TYPE_OUTPUT:
        {
            if (iOutPort.size() >= PVMF_VIDEOENC_NODE_MAX_OUTPUT_PORT)
            {
                LOG_ERR((0, "PVMFVideoEncNode::DoRequestPort: Error - Max number of output port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            OSCL_StackString<20> portname;
            portname = "PVVideoEncOut";

            port = AllocatePort(iOutPort, tag, mimetype, portname.get_cstr());
            if (!port)
            {
                CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                return;
            }
        }
        break;

        default:
            LOG_ERR((0, "PVMFVideoEncNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            return;
    }

    //Return the port pointer to the caller.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess, (OsclAny*)port);
}

////////////////////////////////////////////////////////////////////////////
PVMFVideoEncPort* PVMFVideoEncNode::AllocatePort(PVMFVideoEncPortVector& aPortVector, int32 aTag,
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
                         LOG_ERR((0, "PVMFVideoEncNode::AllocatePort: Error - iPortVector Out of memory"));
                         return NULL;
                        );
    PVMFVideoEncPort* port = OSCL_PLACEMENT_NEW(ptr, PVMFVideoEncPort(aTag, this, Priority(), aName));

    // if format was provided in mimestring, set it now.
    if (aMimeType)
    {
        PVMFFormatType format = GetFormatIndex(aMimeType->get_str());
        if ((port->SetFormat(format) != PVMFSuccess) ||
                ((aTag == PVMF_VIDEOENC_NODE_PORT_TYPE_OUTPUT) && (SetCodecType(format) != PVMFSuccess)))
        {
            aPortVector.DestructAndDealloc(port);
            LOG_ERR((0, "PVMFVideoEncNode::AllocatePort: Error - port->SetFormat or SetCodecType failed"));
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
void PVMFVideoEncNode::DoReleasePort(PVMFVideoEncNodeCommand& aCmd)
{
    //Find the port in the port vector
    PVMFVideoEncPort* port = NULL;
    PVMFVideoEncPort** portPtr = NULL;
    aCmd.Parse((PVMFPortInterface*&)port);

    if (!port)
    {
        LOG_ERR((0, "PVMFVideoEncNode::DoReleasePort: Error - Invalid port pointer"));
        CommandComplete(iCmdQueue, aCmd, PVMFFailure);
        return;
    }

    PVMFStatus status = PVMFSuccess;
    switch (port->GetPortTag())
    {
        case PVMF_VIDEOENC_NODE_PORT_TYPE_INPUT:
            portPtr = iInPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PVMFVideoEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iInPort.Erase(portPtr);
            }
            break;

        case PVMF_VIDEOENC_NODE_PORT_TYPE_OUTPUT:
            portPtr = iOutPort.FindByValue(port);
            if (!portPtr)
            {
                LOG_ERR((0, "PVMFVideoEncNode::DoReleasePort: Error - Port not found"));
                status = PVMFFailure;
            }
            else
            {
                iOutPort.Erase(portPtr);
            }
            break;

        default:
            LOG_ERR((0, "PVMFVideoEncNode::DoReleasePort: Error - Invalid port tag"));
            status = PVMFFailure;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::DoInit(PVMFVideoEncNodeCommand& aCmd)
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
void PVMFVideoEncNode::DoPrepare(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFVIdeoEncNode::DoPrepare"));
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
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
void PVMFVideoEncNode::DoStart(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::DoStart"));
    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
            iVideoEncoder = CPVM4VEncoder::New(0);
            if (!iVideoEncoder)
            {
                LOG_ERR((0, "PVMFVideoEncNode::DoPrepare: new CommonVideoEncoder failed"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            iVideoEncoder->SetObserver(this);

            if (iVideoEncoder->Initialize(&iInputFormat, &iEncodeParam) == ECVEI_FAIL)
            {
                LOG_ERR((0, "PVMFVideoEncNode::DoPrepare: CommonVideoEncoder::Initialize failed"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            if (iEncodeParam.iContentType == ECVEI_STREAMING)
            {
                // M4V output, get VOL header
                uint refCounterSize = oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
                OsclMemoryFragment volHeader;
                volHeader.ptr = NULL;
                volHeader.len = 32; // Encoder requires that buffer size is greater than vol header size (28)
                uint8* memBuffer = (uint8*)iAlloc.allocate(refCounterSize + volHeader.len);
                oscl_memset(memBuffer, 0, refCounterSize + volHeader.len);
                OsclRefCounter* refCounter = OSCL_PLACEMENT_NEW(memBuffer, OsclRefCounterDA(memBuffer, (OsclDestructDealloc*) & iAlloc));
                memBuffer += refCounterSize;
                volHeader.ptr = (OsclAny*)memBuffer;

                int32 length = volHeader.len;
                if (iVideoEncoder->GetVolHeader((uint8*)volHeader.ptr, &length, /*layer*/0) == ECVEI_FAIL)
                {
                    LOG_ERR((0, "PVMFVideoEncNode::DoPrepare: Error - CPVM4VEncoder::GetVolHeader failed"));
                    CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                    return;
                }
                volHeader.len = length;
                iVolHeader = OsclRefCounterMemFrag(volHeader, refCounter, 32);
            }

            SetState(EPVMFNodeStarted);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
            break;

        case EPVMFNodePaused:
            SetState(EPVMFNodeStarted);

            // Notify input port that the node is ready to process incoming msg again
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                ((PVMFVideoEncPort*)iInPort[i])->ProcessIncomingMsgReady();

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
void PVMFVideoEncNode::DoStop(PVMFVideoEncNodeCommand& aCmd)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::DoStop"));
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
        {
            // Clear queued messages in ports
            uint32 i;
            for (i = 0; i < iInPort.size(); i++)
                iInPort[i]->ClearMsgQueues();
            for (i = 0; i < iOutPort.size(); i++)
                iOutPort[i]->ClearMsgQueues();

            // Video encoder is created on Start, so in parallel it's deleted in Stop
            DeleteVideoEncoder();
            //transition to Prepared state
            SetState(EPVMFNodePrepared);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
        }
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
void PVMFVideoEncNode::DeleteVideoEncoder()
{
    if (iVideoEncoder)
    {
        if (iMediaDataAlloc)
        {
            PVMFVideoEncNodeOutputData outputData;
            PVMFStatus status = outputData.Allocate(iMediaDataAlloc, &iMediaDataMemPool);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PVMFVideoEncNode::SyncEncodeAndSend: Error - outputData.Allocate failed"));
                return;
            }

            // Flush and discard output data
            while (iVideoEncoder->FlushOutput(&(outputData.iEncoderOutput)) == ECVEI_FLUSH)
            {
                outputData.iEncoderOutput.iBitStreamSize = MAX_OUTBUF_SIZE;
            }
        }

        iVideoEncoder->Terminate();
        OSCL_DELETE(iVideoEncoder);
        iVideoEncoder = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::DoFlush(PVMFVideoEncNodeCommand& aCmd)
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
                    ((PVMFVideoEncPort*)iInPort[i])->ProcessIncomingMsgReady();
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
                    ((PVMFVideoEncPort*)iOutPort[i])->ProcessOutgoingMsgReady();
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
bool PVMFVideoEncNode::IsFlushPending()
{
    return (iCurrentCmd.size() > 0
            && iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH);
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::FlushComplete()
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::FlushComplete"));
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
    DeleteVideoEncoder();
    //resume port input so the ports can be re-started.
    for (i = 0; i < iInPort.size(); i++)
        iInPort[i]->ResumeInput();
    for (i = 0; i < iOutPort.size(); i++)
        iOutPort[i]->ResumeInput();

    // When the current cmd queue is empty, simply return.
    if (iCurrentCmd.empty())
    {
        LOG_ERR((0, "PVMp4FFComposerNode::FlushComplete: Error - iCurrentCmd is empty"));
        return;
    }

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
void PVMFVideoEncNode::DoPause(PVMFVideoEncNodeCommand& aCmd)
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
void PVMFVideoEncNode::DoReset(PVMFVideoEncNodeCommand& aCmd)
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
        CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
    }
    else
    {
        OSCL_LEAVE(OsclErrInvalidState);

    }
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::DoCancelAllCommands(PVMFVideoEncNodeCommand& aCmd)
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
void PVMFVideoEncNode::DoCancelCommand(PVMFVideoEncNodeCommand& aCmd)
{
    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.Parse(id);

    //first check "current" command if any
    {
        PVMFVideoEncNodeCommand* cmd = iCurrentCmd.FindById(id);
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
        PVMFVideoEncNodeCommand* cmd = iCmdQueue.FindById(id, 1);
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
bool PVMFVideoEncNode::IsProcessOutgoingMsgReady()
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
bool PVMFVideoEncNode::IsProcessIncomingMsgReady()
{
    if (iInterfaceState == EPVMFNodeStarted || IsFlushPending())
    {
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
PVMFStatus PVMFVideoEncNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::ProcessIncomingMsg: aPort=0x%x", aPort));
    PVMFStatus status = PVMFFailure;

    switch (aPort->GetPortTag())
    {
        case PVMF_VIDEOENC_NODE_PORT_TYPE_INPUT:
        {
            if (!IsProcessIncomingMsgReady())
            {
                LOG_ERR((0, "PVMFVideoEncNode::ProcessIncomingMsg: Error - Not ready."));
                return PVMFErrBusy;
            }

            PVMFSharedMediaMsgPtr msg;
            status = aPort->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PVMFVideoEncNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed"));
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
            status = SyncEncodeAndSend(mediaData);
            switch (status)
            {
                case PVMFSuccess:
                    break;
                case PVMFErrBusy:
                    LOG_DEBUG((0, "PVMFVideoEncNode::ProcessIncomingMsg: Outgoing queue busy. This should not happen."));
                    break;
                default:
                    ReportErrorEvent(PVMF_VIDEOENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)aPort);
                    break;
            }
        }
        break;

        case PVMF_VIDEOENC_NODE_PORT_TYPE_OUTPUT:
            // Nothing to be done
            status = PVMFSuccess;
            break;

        default:
            LOG_ERR((0, "PVMFVideoEncNode::ProcessIncomingMsg: Error - Invalid port tag"));
            ReportErrorEvent(PVMF_VIDEOENC_NODE_ERROR_ENCODE_ERROR, (OsclAny*)aPort);
            status = PVMFFailure;
            break;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNode::SyncEncodeAndSend(PVMFSharedMediaDataPtr& aMediaData)
{
    // Get the next data fragment
    OsclRefCounterMemFrag frag;
    aMediaData->getMediaFragment(0, frag);

    TPVVideoInputData inputData;
    inputData.iSource = (uint8*)frag.getMemFragPtr();
    inputData.iTimeStamp = aMediaData->getTimestamp();

    PVMFVideoEncNodeOutputData outputData;
    PVMFStatus status = outputData.Allocate(iMediaDataAlloc, &iMediaDataMemPool);
    if (status != PVMFSuccess)
    {
        LOG_ERR((0, "PVMFVideoEncNode::SyncEncodeAndSend: Error - outputData.Allocate failed"));
        return status;
    }

#if	PROFILING_ON
    uint32 startTicks = OsclTickCount::TickCount();
#endif
    if (iVideoEncoder->EncodeFrame(&inputData, &(outputData.iEncoderOutput)) == ECVEI_SUCCESS)
    {
        if (outputData.iEncoderOutput.iBitStreamSize > 0)
        {
#if PROFILING_ON
            uint32 endTicks = OsclTickCount::TickCount();
            uint32 enctime = OsclTickCount::TicksToMsec(endTicks - startTicks);
            total_ticks += (endTicks - startTicks);

            ++iStats.iNumFrames;
            iStats.iDuration = outputData.iEncoderOutput.iVideoTimeStamp;
            if ((iStats.iMinEncTime > enctime) || (0 == iStats.iMinEncTime))
            {
                iStats.iMinEncTime = enctime;
            }
            if (iStats.iMaxEncTime < enctime)
            {
                iStats.iMaxEncTime = enctime;
            }
#endif
            return SendEncodedBitstream(outputData);
        }
        else
        {
#if PROFILING_ON
            ++iStats.iNumFramesSkipped;
#endif
            LOG_DEBUG((0, "PVMFVideoEncNode::SyncEncodeAndSend(): Skipped frame"));
            return PVMFSuccess;
        }
    }
    else
    {
        LOG_ERR((0, "PVMFVideoEncNode::SyncEncodeAndSend(): Error - EncodeFrame failed"));
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNode::SendEncodedBitstream(PVMFVideoEncNodeOutputData& aOutputData)
{
    PVMFStatus status = PVMFSuccess;

    // update the filled length of the fragment
    aOutputData.iMediaData->setMediaFragFilledLen(0, aOutputData.iEncoderOutput.iBitStreamSize);

    // Set timestamp
    aOutputData.iMediaData->setTimestamp(aOutputData.iEncoderOutput.iVideoTimeStamp);

    // Set sequence number
    aOutputData.iMediaData->setSeqNum(iSeqNum++);

    // Send vol header for m4v bitstream
    if (aOutputData.iMediaData->getSeqNum() == 0 && iEncodeParam.iContentType == ECVEI_STREAMING)
        aOutputData.iMediaData->setFormatSpecificInfo(iVolHeader);

    // Send bitstream data to downstream node
    PVMFSharedMediaMsgPtr mediaMsgOut;
    convertToPVMFMediaMsg(mediaMsgOut, aOutputData.iMediaData);

    for (uint32 i = 0; i < iOutPort.size(); i++)
    {
        status = iOutPort[i]->QueueOutgoingMsg(mediaMsgOut);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFVideoEncNode::SendEncodedBitstream: Error - QueueOutgoingMsg failed. status=%d", status));
            return status;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
//                 Encoder settings routines
////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNode::SetCodecType(PVMFFormatType aCodec)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetCodecType: Error iInterfaceState=%d", iInterfaceState));
            return PVMFErrInvalidState;
        default:
            break;
    }

    switch (aCodec)
    {
        case PVMF_H263:
            iEncodeParam.iContentType = ECVEI_H263;
            break;
        case PVMF_M4V:
            iEncodeParam.iContentType = ECVEI_STREAMING;
            break;
        default:
            return PVMFErrNotSupported;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNode::SetInputFormat(PVMFFormatType aFormat)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::SetInputFormat: aFormat=%d", aFormat));
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetInputFormat: Error - iInterfaceState=%d", iInterfaceState));
            return PVMFErrInvalidState;
        default:
            break;
    }

    switch (aFormat)
    {
        case PVMF_YUV420:
            iInputFormat.iVideoFormat = ECVEI_YUV420;
            break;
        case PVMF_YUV422:
            iInputFormat.iVideoFormat = ECVEI_UYVY;
            break;
        case PVMF_RGB24:
            iInputFormat.iVideoFormat = ECVEI_RGB24;
            break;
        case PVMF_RGB12:
            iInputFormat.iVideoFormat = ECVEI_RGB12;
            break;
        default:
            return PVMFFailure;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNode::SetInputFrameSize(uint32 aWidth, uint32 aHeight, uint8 aFrmOrient)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetInputFrameSize: Error iInterfaceState=%d", iInterfaceState));
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
PVMFStatus PVMFVideoEncNode::SetInputFrameRate(OsclFloat aFrameRate)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            LOG_ERR((0, "PVMFVideoEncNode::SetInputFrameRate: Error iInterfaceState=%d", iInterfaceState));
            return false;
        default:
            break;
    }

    iInputFormat.iFrameRate = OSCL_STATIC_CAST(float, aFrameRate);
    iEncodeParam.iNoFrameSkip = iEncodeParam.iNoCurrentSkip = false;
    return true;
}

////////////////////////////////////////////////////////////////////////////
PVMFFormatType PVMFVideoEncNode::GetCodecType()
{
    switch (iEncodeParam.iContentType)
    {
        case ECVEI_H263:
            return PVMF_H263;
        case ECVEI_STREAMING:
        case ECVEI_DOWNLOAD:
            return PVMF_M4V;
        default:
            return PVMF_FORMAT_UNKNOWN;
    }
}

////////////////////////////////////////////////////////////////////////////
uint32 PVMFVideoEncNode::GetOutputBitRate(uint32 aLayer)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFVideoEncNode::GetOutputBitRate: Error - Invalid layer number"));
        return 0;
    }

    return iEncodeParam.iBitRate[aLayer];
}

////////////////////////////////////////////////////////////////////////////
OsclFloat PVMFVideoEncNode::GetOutputFrameRate(uint32 aLayer)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFVideoEncNode::GetOutputFrameRate: Error Invalid layer number"));
        return 0;
    }

    return (OsclFloat)iEncodeParam.iFrameRate[aLayer];
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNode::GetOutputFrameSize(uint32 aLayer, uint32& aWidth, uint32& aHeight)
{
    if ((int32)aLayer >= iEncodeParam.iNumLayer)
    {
        LOG_ERR((0, "PVMFVideoEncNode::GetOutputFrameSize: Error Invalid layer number"));
        return PVMFFailure;
    }

    aWidth = iEncodeParam.iFrameWidth[aLayer];
    aHeight = iEncodeParam.iFrameHeight[aLayer];
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
uint32 PVMFVideoEncNode::GetIFrameInterval()
{
    return iEncodeParam.iIFrameInterval;
}

////////////////////////////////////////////////////////////////////////////
//                 Event reporting routines.
////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::SetState(TPVMFNodeInterfaceState aState)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::SetState %d", aState));
    PVMFNodeInterface::SetState(aState);
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::ReportErrorEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_ERR((0, "PVMFVideoEncNode::ReportErrorEvent: aEventType=%d aEventData=0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportErrorEvent(aEventType, aEventData);
}

////////////////////////////////////////////////////////////////////////////
void PVMFVideoEncNode::ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_STACK_TRACE((0, "PVMFVideoEncNode::ReportInfoEvent: aEventType=%d, aEventData0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData);
}

////////////////////////////////////////////////////////////////////////////
//            PVMFVideoEncNodeOutputData implementation
////////////////////////////////////////////////////////////////////////////
PVMFVideoEncNodeOutputData::PVMFVideoEncNodeOutputData()
{
    oscl_memset(&iEncoderOutput, 0, sizeof(TPVVideoOutputData));
}

////////////////////////////////////////////////////////////////////////////
PVMFVideoEncNodeOutputData::PVMFVideoEncNodeOutputData(const PVMFVideoEncNodeOutputData& aData)
{
    iEncoderOutput.iFrame = aData.iEncoderOutput.iFrame;
    iEncoderOutput.iLayerNumber = aData.iEncoderOutput.iLayerNumber;
    iEncoderOutput.iBitStream = aData.iEncoderOutput.iBitStream;
    iEncoderOutput.iBitStreamSize = aData.iEncoderOutput.iBitStreamSize;
    iEncoderOutput.iVideoTimeStamp = aData.iEncoderOutput.iVideoTimeStamp;
    iEncoderOutput.iExternalTimeStamp = aData.iEncoderOutput.iExternalTimeStamp;
    iEncoderOutput.iHintTrack.MTB = aData.iEncoderOutput.iHintTrack.MTB;
    iEncoderOutput.iHintTrack.LayerID = aData.iEncoderOutput.iHintTrack.LayerID;
    iEncoderOutput.iHintTrack.CodeType = aData.iEncoderOutput.iHintTrack.CodeType;
    iEncoderOutput.iHintTrack.RefSelCode = aData.iEncoderOutput.iHintTrack.RefSelCode;
    iMediaData = aData.iMediaData;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMFVideoEncNodeOutputData::Allocate(PVMFSimpleMediaBufferCombinedAlloc* aBufferAlloc,
        PVMFVideoEncNodeMemPool* aMemPool)
{
    int32 err = 0;
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImplOut;
    OsclRefCounterMemFrag refCtrMemFragOut;

    OSCL_TRY(err, mediaDataImplOut = aBufferAlloc->allocate(MAX_OUTBUF_SIZE););
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    OSCL_TRY(err, iMediaData = PVMFMediaData::createMediaData(mediaDataImplOut, aMemPool););
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    // Retrieve memory fragment to write to
    iMediaData->getMediaFragment(0, refCtrMemFragOut);
    iEncoderOutput.iBitStream = (uint8*)refCtrMemFragOut.getMemFrag().ptr;
    oscl_memset(iEncoderOutput.iBitStream, 0, MAX_OUTBUF_SIZE);
    iEncoderOutput.iBitStreamSize = MAX_OUTBUF_SIZE;
    return PVMFSuccess;
}

PVMFStatus PVMFVideoEncNode::SendEndOfTrackCommand(PVMFSharedMediaMsgPtr& aMsg)
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

void PVMFVideoEncNode::LogDiagnostics()
{

#if	PROFILING_ON
    oDiagnosticsLogged = true;
    uint32 frame_rate =  0;
    if (iStats.iDuration > 0)
    {
        frame_rate = (iStats.iNumFrames * 1000) / iStats.iDuration;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_DEBUG, (0, "PVMFVideoEncNode Stats: Frame Rate = %d, NumFrames=%d, NumSkippedFrames=%d, Final Timestamp=%d, Total Encode Time(in ms)=%d",
                    frame_rate, iStats.iNumFrames, iStats.iNumFramesSkipped, iStats.iDuration, OsclTickCount::TicksToMsec(total_ticks)));

    if (iStats.iNumFrames > 0)
    {
        iStats.iAverageEncTime = (OsclTickCount::TicksToMsec(total_ticks)) / iStats.iNumFrames;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFVideoEncNode Stats: EncTime(max:%d, min:%d, average:%d)/n",
                    iStats.iMaxEncTime, iStats.iMinEncTime, iStats.iAverageEncTime));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFVideoEncNode Stats: NumFrames=%d, NumSkippedFrames=%d, Final Timestamp=%d, Time to encode(in ms)=%d",
                    iStats.iNumFrames, iStats.iNumFramesSkipped, iStats.iDuration, OsclTickCount::TicksToMsec(total_ticks)));

    oscl_memset(&iStats, 0, sizeof(PVVideoEncNodeStats));
#endif

}
