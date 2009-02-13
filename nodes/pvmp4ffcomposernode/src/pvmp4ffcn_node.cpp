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
/**
 * @file pvmp4ffcn_node.cpp
 * @brief Node for PV MPEG4 file format composer
 */

#ifndef PVMP4FFCN_NODE_H_INCLUDED
#include "pvmp4ffcn_node.h"
#endif
#ifndef PVMP4FFCN_FACTORY_H_INCLUDED
#include "pvmp4ffcn_factory.h"
#endif
#ifndef PVMP4FFCN_PORT_H_INCLUDED
#include "pvmp4ffcn_port.h"
#endif
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif
#ifndef OSCL_MEM_BASIC_FUNCTIONS_H
#include "oscl_mem_basic_functions.h"
#endif

#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m);
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m);
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

#define SLASH '/'

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFNodeInterface* PVMp4FFComposerNodeFactory::CreateMp4FFComposer(int32 aPriority)
{
    int32 err = 0;
    PVMFNodeInterface* node = NULL;

    OSCL_TRY(err,
             node = (PVMFNodeInterface*)OSCL_NEW(PVMp4FFComposerNode, (aPriority));
             if (!node)
             OSCL_LEAVE(OsclErrNoMemory);
            );

    OSCL_FIRST_CATCH_ANY(err, return NULL;);
    return node;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMp4FFComposerNodeFactory::DeleteMp4FFComposer(PVMFNodeInterface* aComposer)
{
    if (aComposer)
    {
        PVMp4FFComposerNode* node = (PVMp4FFComposerNode*)aComposer;
        OSCL_DELETE(node);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////
PVMp4FFComposerNode::PVMp4FFComposerNode(int32 aPriority)
        : OsclActiveObject(aPriority, "PVMp4FFComposerNode")
        , iMpeg4File(NULL)
        , iFileType(0)
        , iAuthoringMode(PVMP4FF_3GPP_DOWNLOAD_MODE)
        , iPresentationTimescale(1000)
        , iMovieFragmentDuration(2000)
        , iClockConverter(8000)
        , iExtensionRefCount(0)
        , iRealTimeTS(false)
        , iInitTSOffset(false)
        , iTSOffset(0)
        , iMaxFileSizeEnabled(false)
        , iMaxDurationEnabled(false)
        , iMaxFileSize(0)
        , iMaxTimeDuration(0)
        , iFileSizeReportEnabled(false)
        , iDurationReportEnabled(false)
        , iFileSizeReportFreq(0)
        , iDurationReportFreq(0)
        , iNextDurationReport(0)
        , iNextFileSizeReport(0)
        , iCacheSize(0)
        , iConfigSize(0)
        , pConfig(NULL)
        , iTrackId_H264(0)
        , iTrackId_Text(0)
        , iSyncSample(0)
        , iNodeEndOfDataReached(false)
        , iformat_h264(PVMF_FORMAT_UNKNOWN)
        , iformat_text(PVMF_FORMAT_UNKNOWN)
        , iOutputFileHandle(NULL)
{
    iInterfaceState = EPVMFNodeCreated;
    iNum_PPS_Set = 0;
    iNum_SPS_Set = 0;
    iText_sdIndex = 0;
#if PROFILING_ON
    iMaxSampleAddTime = 0;
    iMinSampleAddTime = 0;
    iMinSampleSize = 0;
    iMaxSampleSize = 0;
    iNumSamplesAdded = 0;
    oDiagnosticsLogged = false;
    iDiagnosticsLogger = PVLogger::GetLoggerObject("pvauthordiagnostics.composer.mp4");

#endif

    iLogger = PVLogger::GetLoggerObject("PVMp4FFComposerNode");

    int32 err;
    OSCL_TRY(err,
             //Create the input command queue.  Use a reserve to avoid lots of
             //dynamic memory allocation.
             iCmdQueue.Construct(PVMF_MP4FFCN_COMMAND_ID_START, PVMF_MP4FFCN_COMMAND_VECTOR_RESERVE);
             iCurrentCmd.Construct(0, 1); // There's only 1 current command


             //Create the port vector.
             iInPorts.Construct(PVMF_MP4FFCN_PORT_VECTOR_RESERVE);
            );

    OSCL_FIRST_CATCH_ANY(err,
                         //if a leave happened, cleanup and re-throw the error
                         iCmdQueue.clear();
                         iCurrentCmd.clear();
                         iInPorts.clear();
                         memvector_sps.clear();
                         memvector_pps.clear();
                         OSCL_CLEANUP_BASE_CLASS(PVMFNodeInterface);
                         OSCL_CLEANUP_BASE_CLASS(OsclActiveObject);
                         OSCL_LEAVE(err);
                        );

#if PROFILING_ON
    // Statistics
    for (uint32 i = 0; i < 3; i++)
        oscl_memset(&(iStats[i]), 0, sizeof(PVMp4FFCNStats));
#endif
}

////////////////////////////////////////////////////////////////////////////
PVMp4FFComposerNode::~PVMp4FFComposerNode()
{
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif
    if (iMpeg4File)
    {
        PVA_FF_IMpeg4File::DestroyMP4FileObject(iMpeg4File);
    }
    if(iOutputFileHandle)
    {
        OSCL_DELETE( iOutputFileHandle );
        iOutputFileHandle = NULL;
    }

    if (pConfig != NULL)
    {
        OSCL_FREE(pConfig);
        iConfigSize = 0;
    }

    // Cleanup allocated ports
    while (!iInPorts.empty())
    {
        iInPorts.Erase(&iInPorts.front());

    }
    //Cleanup commands
    //The command queues are self-deleting, but we want to
    //notify the observer of unprocessed commands.
    while (!iCmdQueue.empty())
    {
        CommandComplete(iCmdQueue, iCmdQueue[0], PVMFFailure);
        iCmdQueue.Erase(&iCmdQueue.front());
    }

    while (!iCurrentCmd.empty())
    {
        CommandComplete(iCurrentCmd, iCurrentCmd[0], PVMFFailure);
        iCmdQueue.Erase(&iCurrentCmd.front());
    }
    iNodeEndOfDataReached = false;

    Cancel();
    if (iInterfaceState != EPVMFNodeCreated)
        iInterfaceState = EPVMFNodeIdle;
    ThreadLogoff();
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::ThreadLogon()
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
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::ThreadLogoff()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO, (0, "PVMp4FFComposerNode:ThreadLogoff"));
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
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::GetCapability(PVMFNodeCapability& aNodeCapability)
{
    aNodeCapability.iCanSupportMultipleInputPorts = true;
    aNodeCapability.iCanSupportMultipleOutputPorts = false;
    aNodeCapability.iHasMaxNumberOfPorts = true;
    aNodeCapability.iMaxNumberOfPorts = PVMF_MP4FFCN_MAX_INPUT_PORT + PVMF_MP4FFCN_MAX_OUTPUT_PORT;
    aNodeCapability.iInputFormatCapability.push_back(PVMF_M4V);
    aNodeCapability.iInputFormatCapability.push_back(PVMF_H264_MP4);
    aNodeCapability.iInputFormatCapability.push_back(PVMF_H263);
    aNodeCapability.iInputFormatCapability.push_back(PVMF_AMR_IETF);
    aNodeCapability.iInputFormatCapability.push_back(PVMF_3GPP_TIMEDTEXT);

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFPortIter* PVMp4FFComposerNode::GetPorts(const PVMFPortFilter* aFilter)
{
    OSCL_UNUSED_ARG(aFilter);
    iInPorts.Reset();
    return &iInPorts;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::QueryUUID(PVMFSessionId aSession, const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::QueryUUID"));

    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYUUID, aMimeType, aUuids, aExactUuidsOnly, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::QueryInterface(PVMFSessionId aSession, const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMp4FFComposerNode::QueryInterface"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_QUERYINTERFACE, aUuid, aInterfacePtr, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Init(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Init"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_INIT, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Prepare(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Prepare"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PREPARE, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::RequestPort(PVMFSessionId aSession,
        int32 aPortTag,
        const PvmfMimeString* aPortConfig,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::RequestPort"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_REQUESTPORT, aPortTag, aPortConfig, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::ReleasePort(PVMFSessionId aSession,
        PVMFPortInterface& aPort,
        const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::ReleasePort"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RELEASEPORT, aPort, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Start(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Start"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_START, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Stop(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Stop"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_STOP, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Pause(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Pause"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_PAUSE, aContext);
    return QueueCommandL(cmd);
}

OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Flush(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Flush"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_FLUSH, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::Reset(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::Reset"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_RESET, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::CancelAllCommands(PVMFSessionId aSession, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::CancelAllCommands"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELALLCOMMANDS, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PVMp4FFComposerNode::CancelCommand(PVMFSessionId aSession, PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::CancelCommand"));
    PVMp4FFCNCmd cmd;
    cmd.Construct(aSession, PVMF_GENERIC_NODE_CANCELCOMMAND, aCmdId, aContext);
    return QueueCommandL(cmd);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMp4FFComposerNode::addRef()
{
    ++iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMp4FFComposerNode::removeRef()
{
    if (iExtensionRefCount > 0)
        --iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVMp4FFComposerNode::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    if (uuid == KPVMp4FFCNClipConfigUuid)
    {
        PVMp4FFCNClipConfigInterface* myInterface = OSCL_STATIC_CAST(PVMp4FFCNClipConfigInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else if (uuid == KPVMp4FFCNTrackConfigUuid)
    {
        PVMp4FFCNTrackConfigInterface* myInterface = OSCL_STATIC_CAST(PVMp4FFCNTrackConfigInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else if (uuid == PvmfComposerSizeAndDurationUuid)
    {
        PvmfComposerSizeAndDurationInterface* myInterface = OSCL_STATIC_CAST(PvmfComposerSizeAndDurationInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else if (uuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
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
//            PVMp4FFCNClipConfigInterface routines
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetOutputFile(OsclFileHandle* aFileHandle)
{
    if(iInterfaceState != EPVMFNodeIdle && iInterfaceState != EPVMFNodeInitialized)
        return PVMFFailure;
    
    iFileName = _STRLIT("");//wipe out file name if file handle is in use
    
    if(iOutputFileHandle)
        OSCL_DELETE( iOutputFileHandle );
    iOutputFileHandle = OSCL_NEW( Oscl_File, () );
    if( iOutputFileHandle )
    {
        if(0 == iOutputFileHandle->SetFileHandle(aFileHandle) )
        {
            uint32 mode = Oscl_File::MODE_READWRITE | Oscl_File::MODE_BINARY;
            if(!iOutputFileHandle->Open("", mode, iFs ))
                return PVMFSuccess;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, 
                            (0, "PVMp4FFComposerNode::SetOutputFile: Open() Error Ln %d", __LINE__));
        }
        OSCL_DELETE( iOutputFileHandle );
        iOutputFileHandle = NULL;
    }
    
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, 
                    (0, "PVMp4FFComposerNode::SetOutputFile() Error Ln %d", __LINE__));
    return PVMFFailure;
}


OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetOutputFileName(const OSCL_wString& aFileName)
{
    if (iInterfaceState != EPVMFNodeIdle && iInterfaceState != EPVMFNodeInitialized)
        return PVMFFailure;

    iFileName = aFileName;
	if(iOutputFileHandle)
	{
	    OSCL_DELETE( iOutputFileHandle );
	    iOutputFileHandle = NULL;
	}

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetAuthoringMode(PVMp4FFCN_AuthoringMode aAuthoringMode)
{
    if (iInterfaceState != EPVMFNodeIdle && iInterfaceState != EPVMFNodeInitialized)
        return PVMFErrInvalidState;

    iAuthoringMode = aAuthoringMode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetPresentationTimescale(uint32 aTimescale)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iPresentationTimescale = aTimescale;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetVersion(const OSCL_wString& aVersion, uint16 aLangCode)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iVersion.iDataString = aVersion;
    iVersion.iLangCode = aLangCode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetTitle(const OSCL_wString& aTitle, uint16 aLangCode)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iTitle.iDataString = aTitle;
    iTitle.iLangCode = aLangCode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetAuthor(const OSCL_wString& aAuthor, uint16 aLangCode)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iAuthor.iDataString = aAuthor;
    iAuthor.iLangCode = aLangCode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetCopyright(const OSCL_wString& aCopyright, uint16 aLangCode)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iCopyright.iDataString = aCopyright;
    iCopyright.iLangCode = aLangCode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetDescription(const OSCL_wString& aDescription, uint16 aLangCode)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iDescription.iDataString = aDescription;
    iDescription.iLangCode = aLangCode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetRating(const OSCL_wString& aRating, uint16 aLangCode)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iRating.iDataString = aRating;
    iRating.iLangCode = aLangCode;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetCreationDate(const OSCL_wString& aCreationDate)
{
    if (iInterfaceState != EPVMFNodeIdle &&
            iInterfaceState != EPVMFNodeInitialized &&
            iInterfaceState != EPVMFNodePrepared)
        return PVMFErrInvalidState;

    iCreationDate = aCreationDate;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::SetRealTimeAuthoring(const bool aRealTime)
{
    if (iInterfaceState != EPVMFNodeIdle && iInterfaceState != EPVMFNodeInitialized)
        return PVMFErrInvalidState;

    iRealTimeTS = aRealTime;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
//                PVMp4FFCNTrackConfigInterface routines
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetTrackReference(const PVMFPortInterface& aPort,
        const PVMFPortInterface& aReferencePort)
{
    if (iInterfaceState != EPVMFNodeInitialized)
        return PVMFErrInvalidState;

    int32 portIndex = -1;
    int32 refPortIndex = -1;
    PVMp4FFComposerPort* port = OSCL_REINTERPRET_CAST(PVMp4FFComposerPort*, &aPort);
    PVMp4FFComposerPort* refPort = OSCL_REINTERPRET_CAST(PVMp4FFComposerPort*, &aReferencePort);

    for (uint32 i = 0; i < iInPorts.size(); i++)
    {
        if (iInPorts[i] == port)
            portIndex = i;
        if (iInPorts[i] == refPort)
            refPortIndex = i;
    }

    if (portIndex > 0 && refPortIndex > 0)
    {
        iInPorts[portIndex]->SetReferencePort(iInPorts[refPortIndex]);
        return PVMFSuccess;
    }
    else
        return PVMFFailure;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetCodecSpecificInfo(const PVMFPortInterface& aPort,
        uint8* aInfo, int32 aSize)
{
    PVMFStatus status = PVMFFailure;

    if ((status == PVMFSuccess) &&
            (iInterfaceState == EPVMFNodeStarted))
    {
        PVMp4FFComposerPort* port = OSCL_STATIC_CAST(PVMp4FFComposerPort*, &aPort);
        iMpeg4File->setDecoderSpecificInfo(aInfo, aSize, port->GetTrackId());
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
//          PvmfComposerSizeAndDurationInterface routines
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetMaxFileSize(bool aEnable, uint32 aMaxFileSizeBytes)
{
    iMaxFileSizeEnabled = aEnable;
    if (iMaxFileSizeEnabled)
    {
        iMaxFileSize = aMaxFileSizeBytes;
    }
    else
    {
        iMaxFileSize = 0;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMp4FFComposerNode::GetMaxFileSizeConfig(bool& aEnable, uint32& aMaxFileSizeBytes)
{
    aEnable = iMaxFileSizeEnabled;
    aMaxFileSizeBytes = iMaxFileSize;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetMaxDuration(bool aEnable, uint32 aMaxDurationMilliseconds)
{
    iMaxDurationEnabled = aEnable;
    if (iMaxDurationEnabled)
    {
        iMaxTimeDuration = aMaxDurationMilliseconds;
    }
    else
    {
        iMaxTimeDuration = 0;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMp4FFComposerNode::GetMaxDurationConfig(bool& aEnable, uint32& aMaxDurationMilliseconds)
{
    aEnable = iMaxDurationEnabled;
    aMaxDurationMilliseconds = iMaxTimeDuration;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetFileSizeProgressReport(bool aEnable, uint32 aReportFrequency)
{
    iFileSizeReportEnabled = aEnable;
    if (iFileSizeReportEnabled)
    {
        iFileSizeReportFreq = aReportFrequency;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMp4FFComposerNode::GetFileSizeProgressReportConfig(bool& aEnable, uint32& aReportFrequency)
{
    aEnable = iFileSizeReportEnabled;
    aReportFrequency = iFileSizeReportFreq;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMp4FFComposerNode::SetDurationProgressReport(bool aEnable, uint32 aReportFrequency)
{
    iDurationReportEnabled = aEnable;
    if (iDurationReportEnabled)
    {
        iDurationReportFreq = aReportFrequency;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMp4FFComposerNode::GetDurationProgressReportConfig(bool& aEnable, uint32& aReportFrequency)
{
    aEnable = iDurationReportEnabled;
    aReportFrequency = iDurationReportFreq;
}

////////////////////////////////////////////////////////////////////////////
//            PVMFPortActivityHandler routines
////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::HandlePortActivity(const PVMFPortActivity& aActivity)
{
    OSCL_UNUSED_ARG(aActivity);
    // Scheduling to process port activities are handled in the port itself
}

////////////////////////////////////////////////////////////////////////////
//                    OsclActiveObject routines
////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::Run()
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode::Run: iInterfaceState=%d", iInterfaceState));

    if (!iCmdQueue.empty())
    {
        if (ProcessCommand(iCmdQueue.front()))
        {
            //note: need to check the state before re-scheduling
            //since the node could have been reset in the ProcessCommand
            //call.
            if (iInterfaceState != EPVMFNodeCreated)
                RunIfNotReady();
            return;
        }
    }

    LOG_STACK_TRACE((0, "PVMp4FFComposerNode::Run: Out. iInterfaceState=%d", iInterfaceState));
}


////////////////////////////////////////////////////////////////////////////
//                   Command Processing routines
////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMp4FFComposerNode::QueueCommandL(PVMp4FFCNCmd& aCmd)
{
    int32 err = 0;
    PVMFCommandId id = 0;

    OSCL_TRY(err, id = iCmdQueue.AddL(aCmd););
    OSCL_FIRST_CATCH_ANY(err,
                         OSCL_LEAVE(err);
                         return 0;
                        );

    // Wakeup the AO
    RunIfNotReady();
    return id;
}

////////////////////////////////////////////////////////////////////////////
bool PVMp4FFComposerNode::ProcessCommand(PVMp4FFCNCmd& aCmd)
{
    //normally this node will not start processing one command
    //until the prior one is finished.  However, a hi priority
    //command such as Cancel must be able to interrupt a command
    //in progress.
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
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            break;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::CommandComplete(PVMp4FFCNCmdQueue& aCmdQueue, PVMp4FFCNCmd& aCmd,
        PVMFStatus aStatus, OsclAny* aEventData)
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode:CommandComplete: Id %d Cmd %d Status %d Context %d Data %d"
                     , aCmd.iId, aCmd.iCmd, aStatus, aCmd.iContext, aEventData));

    //create response
    PVMFCmdResp resp(aCmd.iId, aCmd.iContext, aStatus, aEventData);
    PVMFSessionId session = aCmd.iSession;

    //Erase the command from the queue.
    aCmdQueue.Erase(&aCmd);

    //Report completion to the session observer.
    ReportCmdCompleteEvent(session, resp);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoQueryUuid(PVMp4FFCNCmd& aCmd)
{
    OSCL_String* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;
    aCmd.Parse(mimetype, uuidvec, exactmatch);

    uuidvec->push_back(KPVMp4FFCNClipConfigUuid);
    uuidvec->push_back(KPVMp4FFCNTrackConfigUuid);
    uuidvec->push_back(PvmfComposerSizeAndDurationUuid);

    CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoQueryInterface(PVMp4FFCNCmd& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMp4FFComposerNode::DoQueryInterface"));

    PVUuid* uuid;
    PVInterface** ptr;
    aCmd.Parse(uuid, ptr);

    if (queryInterface(*uuid, *ptr))
    {
        CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
    }
    else
    {
        CommandComplete(iCmdQueue, aCmd, PVMFFailure);
    }
}


//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoRequestPort(PVMp4FFCNCmd& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMp4FFComposerNode::DoRequestPort() In"));

    int32 tag;
    OSCL_String* portconfig;
    aCmd.Parse(tag, portconfig);

    //validate the tag...
    switch (tag)
    {
        case PVMF_MP4FFCN_PORT_TYPE_SINK:
            if (iInPorts.size() >= PVMF_MP4FFCN_MAX_INPUT_PORT)
            {
                LOG_ERR((0, "PVMp4FFComposerNode::DoRequestPort: Error - Max number of input port already allocated"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }
            break;

        default:
            //bad port tag
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMp4FFComposerNode::DoRequestPort: Error - Invalid port tag"));
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
            return;
    }

    //Allocate a new port
    OsclAny *ptr = NULL;
    int32 err;
    OSCL_TRY(err,
             ptr = iInPorts.Allocate();
             if (!ptr)
             OSCL_LEAVE(OsclErrNoMemory);
            );

    OSCL_FIRST_CATCH_ANY(err,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                         (0, "PVMp4FFComposerNode::DoRequestPort: Error - iInPorts Out of memory"));
                         CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                         return;
                        );

    OSCL_StackString<20> portname;
    portname = "PVMP4ComposerIn";

    PVMp4FFComposerPort* port = OSCL_PLACEMENT_NEW(ptr, PVMp4FFComposerPort(tag, this, Priority(), portname.get_cstr()));

    // if format was provided in mimestring, set it now.
    if (portconfig)
    {
        PVMFFormatType format = GetFormatIndex(portconfig->get_str());
        switch (format)
        {
            case PVMF_3GPP_TIMEDTEXT:
            case PVMF_H264_MP4:
            case PVMF_M4V:
            case PVMF_H263:
            case PVMF_AMR_IETF:
                port->SetFormat(format);
                break;
            default:
                CommandComplete(iCmdQueue, aCmd, PVMFErrNotSupported);
                return;
        }
    }

    //Add the port to the port vector.
    OSCL_TRY(err, iInPorts.AddL(port););
    OSCL_FIRST_CATCH_ANY(err,
                         iInPorts.DestructAndDealloc(port);
                         CommandComplete(iCmdQueue, aCmd, PVMFErrNoMemory);
                         return;
                        );

    // Return the port pointer to the caller.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess, (OsclAny*)port);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoReleasePort(PVMp4FFCNCmd& aCmd)
{
    //Find the port in the port vector
    PVMp4FFComposerPort* port;

    for (uint32 i = 0; i < iInPorts.size(); i++)
    {
        aCmd.Parse((PVMFPortInterface*&)port);
        PVMp4FFComposerPort** portPtr = iInPorts.FindByValue(port);
        if (portPtr)
        {
            //delete the port.
            iInPorts.Erase(portPtr);
            CommandComplete(iCmdQueue, aCmd, PVMFSuccess);

        }
        else
        {
            //port not found.
            CommandComplete(iCmdQueue, aCmd, PVMFFailure);
        }

    }
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoInit(PVMp4FFCNCmd& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMp4FFComposerNode::DoInitNode() In"));

    switch (iInterfaceState)
    {
        case EPVMFNodeIdle:
            // Creation of file format library is done in DoStart. Nothing to do here.
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

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoPrepare(PVMp4FFCNCmd& aCmd)
{
    switch (iInterfaceState)
    {
        case EPVMFNodeInitialized:
            // Creation of file format library is done in DoStart. Nothing to do here.
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

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoStart(PVMp4FFCNCmd& aCmd)
{
    PVMFStatus status = PVMFSuccess;
    uint32 i = 0;

    switch (iInterfaceState)
    {
        case EPVMFNodePrepared:
        {
            iPostfix = _STRLIT("00");
            iOutputPath = _STRLIT("");
            int32 pos = 0;
            for (pos = iFileName.get_size() - 1; pos >= 0; pos--)
            {
                if (iFileName[pos] == SLASH)
                    break;
            }

            if (pos == -1)
            {
                iOutputPath = _STRLIT(".");
            }
            else
            {
                for (i = 0; i <= (uint32) pos; i++)
                    iOutputPath += iFileName[i];
            }



            iFileType = 0;
            for (i = 0; i < iInPorts.size(); i++)
            {
                switch (iInPorts[i]->GetFormat())
                {
                    case PVMF_H264_MP4:
                    case PVMF_M4V:
                    case PVMF_H263:
                        iFileType |= FILE_TYPE_VIDEO;
                        break;

                    case PVMF_AMR_IETF:
                        iFileType |= FILE_TYPE_AUDIO;
                        break;
                    case PVMF_3GPP_TIMEDTEXT:
                        iFileType |= FILE_TYPE_TIMED_TEXT;
                        break;
                    default:
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMp4FFComposerNode::DoStart: Error - Unsupported format"));
                        return;
                }
            }

            if (iMpeg4File)
            {
                LOG_ERR((0, "PVMp4FFComposerNode::DoStart: Error - File format library already exists"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMp4FFComposerNode::DoStart: Calling PVA_FF_IMpeg4File::createMP4File(%d,0x%x,%d)",
                             iFileType, &iFs, iAuthoringMode));
			if(iOutputFileHandle)
			{
			    OSCL_ASSERT(NULL != iOutputFileHandle->Handle());

			    iMpeg4File = PVA_FF_IMpeg4File::createMP4File(iFileType, iAuthoringMode, iOutputFileHandle, iCacheSize);
			    if( iMpeg4File )
			    {
				iOutputFileHandle = NULL; //let ff take care of the file close and dealloc
			    }
			}
			else
			{
			    iMpeg4File = PVA_FF_IMpeg4File::createMP4File(iFileType, iOutputPath, iPostfix, 
				    (void*)&iFs, iAuthoringMode, iFileName, iCacheSize);
			}

            if (!iMpeg4File)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "PVMp4FFComposerNode::DoStart: Error - PVA_FF_IMpeg4File::createMP4File failed"));
                CommandComplete(iCmdQueue, aCmd, PVMFFailure);
                return;
            }

            iMpeg4File->setPresentationTimescale(iPresentationTimescale);
            iMpeg4File->setVersion(iVersion.iDataString, iVersion.iLangCode);
            iMpeg4File->setTitle(iTitle.iDataString, iTitle.iLangCode);
            iMpeg4File->setAuthor(iAuthor.iDataString, iAuthor.iLangCode);
            iMpeg4File->setCopyright(iCopyright.iDataString, iCopyright.iLangCode);
            iMpeg4File->setDescription(iDescription.iDataString, iDescription.iLangCode);
            iMpeg4File->setRating(iRating.iDataString, iRating.iLangCode);
            iMpeg4File->setCreationDate(iCreationDate);
            iMpeg4File->setMovieFragmentDuration(iMovieFragmentDuration);

            for (i = 0; i < iInPorts.size(); i++)
            {
                status = AddTrack(iInPorts[i]);
                if (status != PVMFSuccess)
                {
                    CommandComplete(iCmdQueue, aCmd, status);
                    return;
                }
            }

            // Check for and set reference tracks after track IDs are assigned
            PVMp4FFComposerPort* refPort = NULL;
            for (i = 0; i < iInPorts.size(); i++)
            {
                refPort = OSCL_STATIC_CAST(PVMp4FFComposerPort*, iInPorts[i]->GetReferencePort());
                if (refPort)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                    (0, "PVMp4FFComposerNode::DoStart: Calling addTrackReference(%d, %d)",
                                     iInPorts[i]->GetTrackId(), refPort->GetTrackId()));
                    iMpeg4File->addTrackReference(iInPorts[i]->GetTrackId(), refPort->GetTrackId());
                }
            }

            iMpeg4File->prepareToEncode();

            iInitTSOffset = true;
            iTSOffset = 0;
            SetState(EPVMFNodeStarted);
            break;
        }

        case EPVMFNodePaused:
            SetState(EPVMFNodeStarted);
            for (i = 0; i < iInPorts.size(); i++)
                ((PVMp4FFComposerPort*)iInPorts[i])->ProcessIncomingMsgReady();
            break;
        case EPVMFNodeStarted:
            status = PVMFSuccess;
            break;
        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

//////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::AddTrack(PVMp4FFComposerPort *aPort)
{
    int32 codecType = 0;
    int32 mediaType = 0;
    int32 trackId = 0;
    PVMP4FFCNFormatSpecificConfig* config = aPort->GetFormatSpecificConfig();
    if (!config)
    {
        LOG_ERR((0, "PVMp4FFComposerNode::AddTrack: Error - GetFormatSpecificConfig failed"));
        return PVMFFailure;
    }

    switch (aPort->GetFormat())
    {
        case PVMF_3GPP_TIMEDTEXT:
            codecType = CODEC_TYPE_TIMED_TEXT;
            mediaType = MEDIA_TYPE_TEXT;
            break;
        case PVMF_H264_MP4:
            codecType = CODEC_TYPE_AVC_VIDEO;
            mediaType = MEDIA_TYPE_VISUAL;
            break;
        case PVMF_M4V:
            codecType = CODEC_TYPE_MPEG4_VIDEO;
            mediaType = MEDIA_TYPE_VISUAL;
            break;

        case PVMF_H263:
            codecType = CODEC_TYPE_BASELINE_H263_VIDEO;
            mediaType = MEDIA_TYPE_VISUAL;
            break;

        case PVMF_AMR_IETF:
            codecType = CODEC_TYPE_AMR_AUDIO;
            mediaType = MEDIA_TYPE_AUDIO;
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                            (0, "PVMp4FFComposerNode::AddTrack: Error - Unsupported format"));
            return PVMFFailure;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                    (0, "PVMp4FFComposerNode::AddTrack: Calling PVA_FF_IMpeg4File::addTrack(0x%x,0x%x)",
                     mediaType, codecType));
    trackId = iMpeg4File->addTrack(mediaType, codecType);
    if (trackId == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "PVMp4FFComposerNode::AddTrack: Error - PVA_FF_IMpeg4File::addTrack failed"));
        return PVMFFailure;
    }
    aPort->SetTrackId(trackId);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                    (0, "PVMp4FFComposerNode::AddTrack: PVA_FF_IMpeg4File::addTrack success. trackID=%d", trackId));
#if PROFILING_ON

    for (uint32 k = 0; k < 3; k++)
    {
        if (iStats[k].iTrackId == 0)
        {
            iStats[k].iTrackId = trackId;
            break;
        }
    }
#endif

    switch (mediaType)
    {
        case MEDIA_TYPE_AUDIO:
            iMpeg4File->setTargetBitRate(trackId, config->iBitrate);
            iMpeg4File->setTimeScale(trackId, config->iTimescale);
            break;

        case MEDIA_TYPE_VISUAL:
            switch (codecType)
            {
                case CODEC_TYPE_BASELINE_H263_VIDEO:
                    iMpeg4File->setH263ProfileLevel(trackId, config->iH263Profile, config->iH263Level);
                    // Don't break here. Continue to set other video properties
                case CODEC_TYPE_AVC_VIDEO:
                case CODEC_TYPE_MPEG4_VIDEO:
                    iMpeg4File->setTargetBitRate(trackId, config->iBitrate);
                    iMpeg4File->setTimeScale(trackId, config->iTimescale);
                    iMpeg4File->setVideoParams(trackId, (float)config->iFrameRate,
                                               (uint16)config->iIFrameInterval, config->iWidth, config->iHeight);
                    break;
            }
            break;
        case MEDIA_TYPE_TEXT:
            iMpeg4File->setTargetBitRate(trackId, config->iBitrate);
            iMpeg4File->setTimeScale(trackId, config->iTimescale);
            break;

    }

    return PVMFSuccess;
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoStop(PVMp4FFCNCmd& aCmd)
{
    PVMFStatus status = PVMFSuccess;
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
            if (!iNodeEndOfDataReached)
            {
                WriteDecoderSpecificInfo();
                status = RenderToFile();
            }

            iNodeEndOfDataReached = false;
            for (uint32 ii = 0; ii < iInPorts.size(); ii++)
            {
                iInPorts[ii]->iEndOfDataReached = false;
            }

        }
        break;
        case EPVMFNodePrepared:
            status = PVMFSuccess;
            break;
        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::WriteDecoderSpecificInfo()
{
    uint32 i;
    uint32 offset = 0;
    iConfigSize = 0;
    int32 trackId;

    if (iformat_h264 == PVMF_H264_MP4)
    {
        trackId = iTrackId_H264;

        for (i = 0;i < memvector_sps.size();i++)
        {
            iConfigSize += 2;//2 bytes for SPS_len
            iConfigSize += memvector_sps[i]->len;
        }

        for (i = 0;i < memvector_pps.size();i++)
        {
            iConfigSize += 2;//2 bytes for PPS_len
            iConfigSize += memvector_pps[i]->len;
        }
        iConfigSize = iConfigSize + 2;//extra two bytes for nunSPS and NumPPS
        pConfig = (uint8*)(OSCL_MALLOC(sizeof(uint8) * iConfigSize));


        //currently we are ignoring NAL Length information
        oscl_memcpy((void*)(pConfig + offset), (const void*)&iNum_SPS_Set, 1);//Writing Number of SPS sets
        offset += 1;

        for (i = 0;i < memvector_sps.size();i++)
        {
            oscl_memcpy((void*)(pConfig + offset), (const void*)&memvector_sps[i]->len, 2);//Writing length of SPS
            offset += 2;
            oscl_memcpy((void*)(pConfig + offset), memvector_sps[i]->ptr, memvector_sps[i]->len);
            offset = offset + memvector_sps[i]->len;
        }

        oscl_memcpy((void*)(pConfig + offset), (const void*)&iNum_PPS_Set, 1);//Writing Number of PPS sets
        offset += 1;

        for (i = 0;i < memvector_pps.size();i++)
        {
            oscl_memcpy((void*)(pConfig + offset), (const void*)&memvector_pps[i]->len, 2);//Writing length of PPS
            offset += 2;//2 bytes for PPS Length
            oscl_memcpy((void*)(pConfig + offset), memvector_pps[i]->ptr, memvector_pps[i]->len);
            offset = offset + memvector_pps[i]->len;
        }
        iMpeg4File->setDecoderSpecificInfo(pConfig, iConfigSize, trackId);
    }

    if (iformat_text == PVMF_3GPP_TIMEDTEXT)
    {
        for (uint32 ii = 0;ii < textdecodervector.size();ii++)
        {
            trackId = iTrackId_Text;
            iMpeg4File->setTextDecoderSpecificInfo(textdecodervector[ii], trackId);
        }
    }

}
//////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::RenderToFile()
{
    PVMFStatus status = PVMFSuccess;

    // Clear queued messages in ports
    uint32 i;
    for (i = 0; i < iInPorts.size(); i++)
        iInPorts[i]->ClearMsgQueues();

    if (!iMpeg4File || !iMpeg4File->renderToFile(iFileName))
    {
        LOG_ERR((0, "PVMp4FFComposerNode::RenderToFile: Error - renderToFile failed"));
        ReportErrorEvent(PVMF_MP4FFCN_ERROR_FINALIZE_OUTPUT_FILE_FAILED);
        status = PVMFFailure;
    }
    else
    {
#if PROFILING_ON
        // Statistics

        for (i = 0; i < 3; i++)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "PVMp4FFComposerNode Stats: TrackId=%d, NumFrame=%d, Duration=%d",
                             iStats[i].iTrackId, iStats[i].iNumFrames, iStats[i].iDuration));
            oscl_memset(&(iStats[i]), 0, sizeof(PVMp4FFCNStats));
        }
#endif
        // Delete file format library
        if (iMpeg4File)
        {
            PVA_FF_IMpeg4File::DestroyMP4FileObject(iMpeg4File);
            iMpeg4File = NULL;
        }

        // Change state
        SetState(EPVMFNodePrepared);
        status = PVMFSuccess;
    }


    return status;
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoFlush(PVMp4FFCNCmd& aCmd)
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode::DoFlush() iInterfaceState:%d", iInterfaceState));

    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
        case EPVMFNodePaused:
            int32 err;
            uint32 i;
            bool msgPending;
            msgPending = false;

            for (i = 0; i < iInPorts.size(); i++)
            {
                if (iInPorts[i]->IncomingMsgQueueSize() > 0)
                    msgPending = true;
                iInPorts[i]->SuspendInput();
                if (iInterfaceState != EPVMFNodeStarted)
                {
                    // Port is in idle if node state is not started. Call ProcessIncomingMsgReady
                    // to wake up port AO
                    ((PVMp4FFComposerPort*)iInPorts[i])->ProcessIncomingMsgReady();
                }
            }

            // Move the command from the input command queue to the current command, where
            // it will remain until the flush completes.
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

            break;
    }
}

////////////////////////////////////////////////////////////////////////////
bool PVMp4FFComposerNode::IsFlushPending()
{
    return (iCurrentCmd.size() > 0
            && iCurrentCmd.front().iCmd == PVMF_GENERIC_NODE_FLUSH);
}

////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::FlushComplete()
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode::FlushComplete"));
    uint32 i = 0;
    PVMFStatus status = PVMFSuccess;
    // Flush is complete only when all queues of all ports are clear.
    // Other wise, just return from this method and wait for FlushComplete
    // from the remaining ports.
    for (i = 0; i < iInPorts.size(); i++)
    {
        if (iInPorts[i]->IncomingMsgQueueSize() > 0 ||
                iInPorts[i]->OutgoingMsgQueueSize() > 0)
        {
            return;
        }
    }
    if (!iNodeEndOfDataReached)
    {
        WriteDecoderSpecificInfo();
        // Finalize output file
        status = RenderToFile();
        if (status != PVMFSuccess)
            LOG_ERR((0, "PVMp4FFComposerNode::FlushComplete: Error - RenderToFile failed"));
    }

    // Resume port input so the ports can be re-started.
    for (i = 0; i < iInPorts.size(); i++)
        iInPorts[i]->ResumeInput();

    if (iCurrentCmd.empty())
    {
        LOG_ERR((0, "PVMp4FFComposerNode::FlushComplete: Error - iCurrentCmd is empty"));
        status = PVMFFailure;
    }

    CommandComplete(iCurrentCmd, iCurrentCmd[0], status);

    if (!iCmdQueue.empty())
    {
        // If command queue is not empty, schedule to process the next command
        RunIfNotReady();
    }


}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoPause(PVMp4FFCNCmd& aCmd)
{
    PVMFStatus status = PVMFSuccess;
    switch (iInterfaceState)
    {
        case EPVMFNodeStarted:
            SetState(EPVMFNodePaused);
            break;
        case EPVMFNodePaused:
            break;
        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoReset(PVMp4FFCNCmd& aCmd)
{
    PVMFStatus status = PVMFSuccess;
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif

    if (IsAdded())
    {
        //delete all ports and notify observer.
        while (!iInPorts.empty())
            iInPorts.Erase(&iInPorts.front());

        //restore original port vector reserve.
        iInPorts.Reconstruct();
        iNodeEndOfDataReached = false;

        //logoff & go back to Created state.
        SetState(EPVMFNodeIdle);
        status = ThreadLogoff();
    }
    else
    {
        OSCL_LEAVE(OsclErrInvalidState);
    }

    CommandComplete(iCmdQueue, aCmd, status);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoCancelAllCommands(PVMp4FFCNCmd& aCmd)
{
    //first cancel the current command if any
    while (!iCurrentCmd.empty())
        CommandComplete(iCurrentCmd, iCurrentCmd[0], PVMFErrCancelled);

    //next cancel all queued commands
    //start at element 1 since this cancel command is element 0.
    while (iCmdQueue.size() > 1)
        CommandComplete(iCmdQueue, iCmdQueue[1], PVMFErrCancelled);

    //finally, report cancel complete.
    CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
}

//////////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::DoCancelCommand(PVMp4FFCNCmd& aCmd)
{
    //extract the command ID from the parameters.
    PVMFCommandId id;
    aCmd.Parse(id);

    //first check "current" command if any
    PVMp4FFCNCmd* cmd = iCurrentCmd.FindById(id);
    if (cmd)
    {
        //cancel the queued command
        CommandComplete(iCurrentCmd, *cmd, PVMFErrCancelled);
        //report cancel success
        CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
        return;
    }

    //next check input queue.
    //start at element 1 since this cancel command is element 0.
    cmd = iCmdQueue.FindById(id, 1);
    if (cmd)
    {
        //cancel the queued command
        CommandComplete(iCmdQueue, *cmd, PVMFErrCancelled);
        //report cancel success
        CommandComplete(iCmdQueue, aCmd, PVMFSuccess);
        return;
    }

    //if we get here the command isn't queued so the cancel fails.
    CommandComplete(iCmdQueue, aCmd, PVMFFailure);
}

//////////////////////////////////////////////////////////////////////////////////
//                  Port activity processing routines
//////////////////////////////////////////////////////////////////////////////////
bool PVMp4FFComposerNode::IsProcessIncomingMsgReady()
{
    if (iInterfaceState == EPVMFNodeStarted || IsFlushPending())
        return true;
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::ProcessIncomingMsg(PVMFPortInterface* aPort)
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode::ProcessIncomingMsg: aPort=0x%x", aPort));
    PVMFStatus status = PVMFSuccess;

    switch (aPort->GetPortTag())
    {
        case PVMF_MP4FFCN_PORT_TYPE_SINK:
        {
            PVMp4FFComposerPort* port = OSCL_REINTERPRET_CAST(PVMp4FFComposerPort*, aPort);
            if (!IsProcessIncomingMsgReady())
            {
                LOG_ERR((0, "PVMp4FFComposerNode::ProcessIncomingMsg: Error - Not ready."));
                return PVMFErrBusy;
            }

            PVMFSharedMediaMsgPtr msg;
            status = port->DequeueIncomingMsg(msg);
            if (status != PVMFSuccess)
            {
                LOG_ERR((0, "PVMp4FFComposerNode::ProcessIncomingMsg: Error - DequeueIncomingMsg failed"));
                return status;
            }
            if (msg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                port->iEndOfDataReached = true;
                //check if EOS has been received on all connected ports.
                uint32 ii = 0;
                iNodeEndOfDataReached = true;
                for (ii = 0; ii < iInPorts.size(); ii++)
                {
                    if (!iInPorts[ii]->iEndOfDataReached)
                    {
                        iNodeEndOfDataReached = false;
                    }
                }

                if (iNodeEndOfDataReached)
                {
                    //Close the file since EOS is received on every connected port
                    WriteDecoderSpecificInfo();
                    status = RenderToFile();

                    //report EOS info to engine
                    ReportInfoEvent(PVMF_COMPOSER_EOS_REACHED);
                }

                //since we do not have data to process, we can safely break here.
                break;
            }
            PVMFSharedMediaDataPtr mediaDataPtr;
            convertToPVMFMediaData(mediaDataPtr, msg);

            int32 trackId = port->GetTrackId();
            if ((mediaDataPtr->getSeqNum() == 0) && (port->GetFormat() == PVMF_M4V))
            {
                // Set VOL Header
                OsclRefCounterMemFrag volHeader;
                if (mediaDataPtr->getFormatSpecificInfo(volHeader) == false ||
                        volHeader.getMemFragSize() == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMp4FFComposerNode::ProcessIncomingMsg: Error - VOL Header not available"));
                    return PVMFFailure;
                }

                iMpeg4File->setDecoderSpecificInfo((uint8*)volHeader.getMemFragPtr(),
                                                   (int32)volHeader.getMemFragSize(), trackId);
            }
            if ((mediaDataPtr->getSeqNum() == 0) && (port->GetFormat() == PVMF_H264_MP4))
            {
                iTrackId_H264 = port->GetTrackId();
                iformat_h264 = port->GetFormat();
            }
            if (port->GetFormat() == PVMF_3GPP_TIMEDTEXT)
            {
                iTrackId_Text = port->GetTrackId();
                iformat_text = port->GetFormat();
                OsclRefCounterMemFrag textconfiginfo;

                if (mediaDataPtr->getFormatSpecificInfo(textconfiginfo) == false ||
                        textconfiginfo.getMemFragSize() == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMp4FFComposerNode::ProcessIncomingMsg: Error - VOL Header not available"));
                    return PVMFFailure;
                }
                int32* pVal = (int32*)textconfiginfo.getMemFragPtr();
                iText_sdIndex = *pVal;
            }
            if ((port->GetFormat() == PVMF_AMR_IETF) && mediaDataPtr->getErrorsFlag())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE,
                                (0, "PVMp4FFComposerNode::ProcessIncomingMsg: Error flag set for AMR!"));
                return PVMFSuccess;
            }

            // Retrieve data from incoming queue
            OsclRefCounterMemFrag memFrag;
            uint32 numFrags = mediaDataPtr->getNumFragments();
            uint32 timestamp = mediaDataPtr->getTimestamp();
            iSyncSample = mediaDataPtr->getMarkerInfo(); //gives the I frame info

            Oscl_Vector<OsclMemoryFragment, OsclMemAllocator> pFrame; //vector to store the nals in the particular case of AVC
            for (uint32 i = 0; (i < numFrags) && status == PVMFSuccess; i++)
            {
                if (!mediaDataPtr->getMediaFragment(i, memFrag))
                {
                    status = PVMFFailure;
                }
                else
                {
                    OsclMemoryFragment memfragment;
                    memfragment.len = memFrag.getMemFragSize();
                    memfragment.ptr = memFrag.getMemFragPtr();
                    pFrame.push_back(memfragment);
                }
            }
            status = AddMemFragToTrack(pFrame, memFrag, port->GetFormat(), timestamp,
                                       trackId, (PVMp4FFComposerPort*)aPort);

            if (status == PVMFFailure)
                ReportErrorEvent(PVMF_MP4FFCN_ERROR_ADD_SAMPLE_TO_TRACK_FAILED, (OsclAny*)aPort);
        }
        break;

        default:
            LOG_ERR((0, "PVMp4FFComposerNode::ProcessIncomingMsg: Error - Invalid port tag"));
            ReportErrorEvent(PVMF_MP4FFCN_ERROR_ADD_SAMPLE_TO_TRACK_FAILED, (OsclAny*)aPort);
            status = PVMFFailure;
            break;
    }

    return status;
}

//////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::AddMemFragToTrack(Oscl_Vector<OsclMemoryFragment, OsclMemAllocator> aFrame, OsclRefCounterMemFrag& aMemFrag,
        PVMFFormatType aFormat,
        uint32& aTimestamp,
        int32 aTrackId,
        PVMp4FFComposerPort *aPort)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMp4FFComposerNode::AddMemFragToTrack: aFormat=0x%x, aTimestamp=%d, aTrackId=%d",
                     aFormat, aTimestamp, aTrackId));

    if (iRealTimeTS)
    {
        if (iInitTSOffset && (aMemFrag.getMemFragSize() > 0))
        {
            iTSOffset = aTimestamp;
            iInitTSOffset = false;
        }

        aTimestamp = aTimestamp - iTSOffset;
    }

    uint32 i = 0;
#if PROFILING_ON
    PVMp4FFCNStats* stats = NULL;
    for (i = 0; i < 3; i++)
    {
        if (aTrackId == iStats[i].iTrackId)
        {
            stats = &(iStats[i]);
            break;
        }
    }
#endif

    PVMFStatus status = PVMFSuccess;
    uint8 flags = 0;
    uint32 size = 0;
    uint8* data = NULL;
    for (i = 0;i < aFrame.size();i++)
    {
        size = aFrame[i].len;
        data = OSCL_REINTERPRET_CAST(uint8*, aFrame[i].ptr);
        if (!data || size == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                            (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - Invalid data or data size"));
            return PVMFFailure;
        }
    }

    switch (aFormat)
    {
        case PVMF_3GPP_TIMEDTEXT:
        case PVMF_H264_MP4:
        case PVMF_M4V:
        case PVMF_H263:
        {
            status = CheckMaxDuration(aTimestamp);
            if (status == PVMFFailure)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - CheckMaxDuration failed"));
                return status;
            }
            else if (status == PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVMp4FFComposerNode::AddMemFragToTrack: Maxmimum duration reached"));
                return status;
            }

            for (i = 0; i < aFrame.size(); i++)
            {
                size = aFrame[i].len;
                status = CheckMaxFileSize(size);
                if (status == PVMFFailure)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - CheckMaxFileSize failed"));
                    return status;
                }
                else if (status == PVMFSuccess)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                                    (0, "PVMp4FFComposerNode::AddMemFragToTrack: Maxmimum file size reached"));
                    return status;
                }

                //No data for some reason.
                if (size == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_NOTICE,
                                    (0, "PVMp4FFComposerNode::AddMemFragToTrack: no data in frag!"));
                    return PVMFSuccess;
                }
            }
            uint8 codeType = 1;

            if (iRealTimeTS)
            {
                if (aTimestamp <= aPort->GetLastTS())
                {
                    aTimestamp = aPort->GetLastTS() + 1;
                }

                aPort->SetLastTS(aTimestamp);
            }

            // FIXME:
            //
            // First of all, this parsing logic should not be here, since composer does not need
            // to parse the output stream and encoder should tell composer what type of picture
            // is coming
            //
            // Second, this parsing code is looking for the PTYPE or PLUSPTYPE field through
            // all the bits in the output picture, and there is no error handling here
            //
            // The big assumption is that anything is not marked as I-frame, it is a P-frame.
            switch (aFormat)
            {
                case PVMF_H264_MP4:
                {
                    if (iSyncSample) //iSyncSample is obtained from the marker info
                        codeType = 0;  //to identify the I Frame in the case of AVC
                }
                break;
                case PVMF_M4V:
                    for (i = 0; i < aFrame.size(); i++)
                    {
                        data = OSCL_REINTERPRET_CAST(uint8*, aFrame[i].ptr);
                        if (data[4] <= 0x3F)
                            codeType = 0; // I-frame
                    }

                    break;
                case PVMF_H263:
                    for (i = 0; i < aFrame.size(); i++)
                    {
                        data = OSCL_REINTERPRET_CAST(uint8*, aFrame[i].ptr);
                        bool isIFrameFromPTypeField = ((data[4] & 0x02) == 0x00);      // PTYPE field must contain a single 0 bit
                        bool isExtendedPicCodingType = ((data[4] & 0x1C) == 0x1C) && ((data[5] & 0x80) == 0x80);
                        bool isIFrameFromPlusPTypeField = ((data[7] & 0x1C) == 0x00);  // PLUSPTYPE field must contain three 0 bits
                        if ((isExtendedPicCodingType && isIFrameFromPlusPTypeField) ||
                            (!isExtendedPicCodingType && isIFrameFromPTypeField)) {
                           codeType = 0;  // I-frame
                        }
                    }
                    break;
            }

            // Format: mtb (1) | layer_id (3) | coding_type (2) | ref_select_code (2)
            // flags |= ((stream->iHintTrack.MTB & 0x01) << 7);
            // flags |= ((stream->iHintTrack.LayerID & 0x07) << 4);
            flags |= ((codeType & 0x03) << 2);
            // flags |= (stream->iHintTrack.RefSelCode & 0x03);

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVMp4FFComposerNode::AddMemFragToTrack: Calling addSampleToTrack(%d, 0x%x, %d, %d, %d)",
                             aTrackId, data, size, aTimestamp, flags));

            if (aFormat == PVMF_3GPP_TIMEDTEXT)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVMp4FFComposerNode::AddMemFragToTrack: Calling addtextSampleToTrack(%d, 0x%x, %d, %d, %d)",
                                 aTrackId, data, size, aTimestamp, flags));
                int32 index = iText_sdIndex;

                if (index >= 0)
                {
#if PROFILING_ON
                    uint32 start = OsclTickCount::TickCount();
#endif
                    if (!iMpeg4File->addTextSampleToTrack(aTrackId, aFrame, aTimestamp, flags, index, NULL))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - addTextSampleToTrack for Timed Text failed"));
                        return PVMFFailure;
                    }
#if PROFILING_ON
                    uint32 stop = OsclTickCount::TickCount();
                    uint32 comptime = OsclTickCount::TicksToMsec(stop - start);
                    uint32 dataSize = 0;
                    for (uint32 ii = 0; ii < aFrame.size(); ii++)
                    {
                        dataSize += aFrame[ii].len;
                    }
                    GenerateDiagnostics(comptime, dataSize);
#endif
                }
            }
            else
            {

#if PROFILING_ON
                uint32 start = OsclTickCount::TickCount();
#endif

                if (!iMpeg4File->addSampleToTrack(aTrackId, aFrame, aTimestamp, flags))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - addSampleToTrack failed"));
                    return PVMFFailure;
                }

#if PROFILING_ON
                uint32 stop = OsclTickCount::TickCount();
                uint32 comptime = OsclTickCount::TicksToMsec(stop - start);
                uint32 dataSize = 0;
                for (uint32 ii = 0; ii < aFrame.size(); ii++)
                {
                    dataSize += aFrame[ii].len;
                }
                GenerateDiagnostics(comptime, dataSize);
#endif
            }


            // Send progress report after sample is successfully added
            SendProgressReport(aTimestamp);

#if PROFILING_ON
            ++(stats->iNumFrames);
            stats->iDuration = aTimestamp;
#endif
        }
        break;

        case PVMF_AMR_IETF:
        {
            if (iRealTimeTS)
            {
                if (((int32) aTimestamp - (int32) aPort->GetLastTS()) < 20)
                {
                    aTimestamp = aPort->GetLastTS() + 20;
                }

                aPort->SetLastTS(aTimestamp);
            }

            uint32 bytesProcessed = 0;
            uint32 frameSize = 0;
            Oscl_Vector<OsclMemoryFragment, OsclMemAllocator> amrfrags;
            for (i = 0; i < aFrame.size(); i++)
            {
                bytesProcessed = 0;
                size = aFrame[i].len;
                data = OSCL_REINTERPRET_CAST(uint8*, aFrame[i].ptr);
                // Parse audio data and add one 20ms frame to track at a time
                while (bytesProcessed < size)
                {
                    // Check for max duration
                    status = CheckMaxDuration(aTimestamp);
                    if (status == PVMFFailure)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - CheckMaxDuration failed"));
                        return status;
                    }
                    else if (status == PVMFSuccess)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMp4FFComposerNode::AddMemFragToTrack: Maxmimum duration reached"));
                        return status;
                    }

                    // Update clock converter
                    iClockConverter.set_clock_other_timescale(aTimestamp, 1000);

                    // Check max file size
                    frameSize = GetIETFFrameSize(data[0]);
                    status = CheckMaxFileSize(frameSize);
                    if (status == PVMFFailure)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - CheckMaxFileSize failed"));
                        return status;
                    }
                    else if (status == PVMFSuccess)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                                        (0, "PVMp4FFComposerNode::AddMemFragToTrack: Maxmimum file size reached"));
                        return status;
                    }

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVMp4FFComposerNode::AddMemFragToTrack: Calling addSampleToTrack(%d, 0x%x, %d, %d, %d)",
                                     aTrackId, data, frameSize, iClockConverter.get_current_timestamp(), flags));


                    OsclMemoryFragment amr_memfrag;
                    amr_memfrag.len = frameSize;
                    amr_memfrag.ptr = data;
                    amrfrags.push_back(amr_memfrag);

#if PROFILING_ON
                    uint32 start = OsclTickCount::TickCount();
#endif

                    if (!iMpeg4File->addSampleToTrack(aTrackId, amrfrags,
                                                      iClockConverter.get_current_timestamp(), flags))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMp4FFComposerNode::AddMemFragToTrack: Error - addSampleToTrack failed"));
                        return PVMFFailure;
                    }

#if PROFILING_ON
                    uint32 stop = OsclTickCount::TickCount();
                    uint32 comptime = OsclTickCount::TicksToMsec(stop - start);
                    uint32 dataSize = 0;
                    for (uint32 ii = 0; ii < amrfrags.size(); ii++)
                    {
                        dataSize += amrfrags[ii].len;
                    }
                    GenerateDiagnostics(comptime, dataSize);

#endif

                    // Send progress report after sample is successfully added
                    SendProgressReport(aTimestamp);

#if PROFILING_ON
                    ++(stats->iNumFrames);
                    stats->iDuration = aTimestamp;
#endif
                    data += frameSize;
                    bytesProcessed += frameSize;
                    aTimestamp += 20;
                    amrfrags.clear();
                }
            }
            if (iRealTimeTS)
            {
                aPort->SetLastTS(aTimestamp - 20);
            }
        }
        break;

        default:
            break;
    }

    return PVMFSuccess;
}

void PVMp4FFComposerNode::GenerateDiagnostics(uint32 aTime, uint32 aSize)
{
#if PROFILING_ON
    if ((iMinSampleAddTime > aTime) || (0 == iMinSampleAddTime))
    {
        iMinSampleAddTime = aTime;
    }
    if (iMaxSampleAddTime < aTime)
    {
        iMaxSampleAddTime = aTime;
    }

    if ((iMinSampleSize > aSize) || (0 == iMinSampleSize))
    {
        iMinSampleSize = aSize;
    }
    if (iMaxSampleSize < aSize)
    {
        iMaxSampleSize = aSize;
    }
    iNumSamplesAdded++;
#endif
}
//////////////////////////////////////////////////////////////////////////////////
int32 PVMp4FFComposerNode::GetIETFFrameSize(uint8 aFrameType)
{
    uint8 frameType = (uint8)(aFrameType >> 3);

    // Find frame size for each frame type
    switch (frameType)
    {
        case 0: // AMR 4.75 Kbps
            return 13;
        case 1: // AMR 5.15 Kbps
            return 14;
        case 2: // AMR 5.90 Kbps
            return 16;
        case 3: // AMR 6.70 Kbps
            return 18;
        case 4: // AMR 7.40 Kbps
            return 20;
        case 5: // AMR 7.95 Kbps
            return 21;
        case 6: // AMR 10.2 Kbps
            return 27;
        case 7: // AMR 12.2 Kbps
            return 32;
        case 15: // AMR Frame No Data
            return 1;
        default: // Error - For Future Use
            return -1;
    }
}

//////////////////////////////////////////////////////////////////////////////////
//                 Progress and max size / duration routines
//////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::SendProgressReport(uint32 aTimestamp)
{
    if (iDurationReportEnabled &&
            aTimestamp >= iNextDurationReport)
    {
        iNextDurationReport = aTimestamp - (aTimestamp % iDurationReportFreq) + iDurationReportFreq;
        ReportInfoEvent(PVMF_COMPOSER_DURATION_PROGRESS, (OsclAny*)aTimestamp);
    }
    else if (iFileSizeReportEnabled)
    {
        uint32 metaDataSize = 0;
        uint32 mediaDataSize = 0;
        uint32 fileSize = 0;

        iMpeg4File->getTargetFileSize(metaDataSize, mediaDataSize);
        fileSize = metaDataSize + mediaDataSize;

        if (fileSize >= iNextFileSizeReport)
        {
            iNextFileSizeReport = fileSize - (fileSize % iFileSizeReportFreq) + iFileSizeReportFreq;
            ReportInfoEvent(PVMF_COMPOSER_FILESIZE_PROGRESS, (OsclAny*)fileSize);
        }
    }

    return PVMFSuccess;
}

//////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::CheckMaxFileSize(uint32 aFrameSize)
{
    if (iMaxFileSizeEnabled)
    {
        uint32 metaDataSize = 0;
        uint32 mediaDataSize = 0;
        iMpeg4File->getTargetFileSize(metaDataSize, mediaDataSize);

        if ((metaDataSize + mediaDataSize + aFrameSize) >= iMaxFileSize)
        {
            // Finalized output file
            if (RenderToFile() != PVMFSuccess)
                return PVMFFailure;

            ReportInfoEvent(PVMF_COMPOSER_MAXFILESIZE_REACHED, NULL);
            return PVMFSuccess;
        }

        return PVMFPending;
    }

    return PVMFErrNotSupported;
}

//////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMp4FFComposerNode::CheckMaxDuration(uint32 aTimestamp)
{
    if (iMaxDurationEnabled)
    {
        if (aTimestamp >= iMaxTimeDuration)
        {
            // Finalize output file
            if (RenderToFile() != PVMFSuccess)
                return PVMFFailure;

            ReportInfoEvent(PVMF_COMPOSER_MAXDURATION_REACHED, NULL);
            return PVMFSuccess;
        }

        return PVMFPending;
    }

    return PVMFErrNotSupported;
}

////////////////////////////////////////////////////////////////////////////
//                   Event reporting routines.
////////////////////////////////////////////////////////////////////////////
void PVMp4FFComposerNode::SetState(TPVMFNodeInterfaceState aState)
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode::SetState: aState=%d", aState));
    PVMFNodeInterface::SetState(aState);
}

void PVMp4FFComposerNode::ReportErrorEvent(PvmfMp4FFCNError aErrorEvent, OsclAny* aEventData)
{
    LOG_ERR((0, "PVMp4FFComposerNode:ReportErrorEvent: aEventType=%d, aEventData=0x%x", aErrorEvent, aEventData));
    switch (aErrorEvent)
    {
        case PVMF_MP4FFCN_ERROR_FINALIZE_OUTPUT_FILE_FAILED:
        case PVMF_MP4FFCN_ERROR_ADD_SAMPLE_TO_TRACK_FAILED:
            PVMFNodeInterface::ReportErrorEvent(PVMFErrResourceConfiguration, aEventData);
            break;
        default:
            PVMFNodeInterface::ReportErrorEvent(PVMFFailure, aEventData);
            break;
    }
}

void PVMp4FFComposerNode::ReportInfoEvent(PVMFEventType aEventType, OsclAny* aEventData)
{
    LOG_STACK_TRACE((0, "PVMp4FFComposerNode:ReportInfoEvent: aEventType=%d, aEventData=0x%x", aEventType, aEventData));
    PVMFNodeInterface::ReportInfoEvent(aEventType, aEventData);
}



void PVMp4FFComposerNode::LogDiagnostics()
{
#if PROFILING_ON
    oDiagnosticsLogged = true;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_DEBUG, (0, "PVMp4FFComposerNode Stats:Sample Add time (Min:%d, Max:%d), Sample Size(Min:%d, Max:%d), number of samples added:%d\n", iMinSampleAddTime, iMaxSampleAddTime, iMinSampleSize, iMaxSampleSize, iNumSamplesAdded));
#endif
}







