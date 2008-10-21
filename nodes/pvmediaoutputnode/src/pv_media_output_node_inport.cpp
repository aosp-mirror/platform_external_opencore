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
 *
 * @file pvmi_io_interface_node_inport.cpp
 * @brief Input port for media io interface wrapper node
 *
 */

#include "pv_media_output_node_inport.h"
#include "pv_media_output_node.h"
#include "pvmf_common_audio_decnode.h"
#include "pvmf_video.h"
#include "pv_mime_string_utils.h"
#include "pvmf_media_cmd.h"
#include "pvmf_media_msg_format_ids.h"
#include "latmpayloadparser.h"
#include "media_clock_converter.h"
#include "time_comparison_utils.h"

#define LOGDATAPATH(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iDatapathLogger, PVLOGMSG_INFO, x);

#define PVMF_MOPORT_LOGDATAPATH(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iDatapathLogger, PVLOGMSG_INFO, x);
#define PVMF_MOPORT_LOGDATAPATH_IN(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iDatapathLoggerIn, PVLOGMSG_INFO, x);
#define PVMF_MOPORT_LOGDATAPATH_OUT(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iDatapathLoggerOut, PVLOGMSG_INFO, x);
#define PVMF_MOPORT_LOGREPOS(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, x);
#define PVMF_MOPORT_LOGDEBUG(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, x);
#define PVMF_MOPORT_LOGERROR(x)	PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, x);


////////////////////////////////////////////////////////////////////////////
PVMediaOutputNodePortTimer::PVMediaOutputNodePortTimer(PVMediaOutputNodePortTimerObserver* aObserver)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVMediaOutputNodePortTimer"),
        iDurationInMS(0),
        iObserver(aObserver),
        iStarted(false)
{
    AddToScheduler();
}

////////////////////////////////////////////////////////////////////////////
PVMediaOutputNodePortTimer::~PVMediaOutputNodePortTimer()
{
    Stop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePortTimer::Start()
{
    RunIfNotReady(iDurationInMS*1000);
    iStarted = true;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePortTimer::setTimerDurationInMS(uint32 duration)
{
    iDurationInMS = duration;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePortTimer::Stop()
{
    Cancel();
    iStarted = false;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePortTimer::Run()
{
    if (!iStarted)
        return;

    if (!iObserver)
    {
        return;
    }

    iObserver->PVMediaOutputNodePortTimerEvent();
    /*
     * Do not reschudule the AO here. Observer would reschedule this AO
     * once it is done processing the timer event.
     */
}

//for logging media data info
void PVMediaOutputNodePort::LogMediaDataInfo(const char* msg, PVMFSharedMediaDataPtr mediaData)
{
    if (!mediaData.GetRep())
        return;
    LOGDATAPATH(
        (0, "MOUT %s %s MediaData SeqNum %d, SId %d, TS %d"
         , PortName()
         , msg
         , mediaData->getSeqNum()
         , mediaData->getStreamID()
         , mediaData->getTimestamp()
        ));
}


//for logging media data info plus write ID and cleanup q depth
void PVMediaOutputNodePort::LogMediaDataInfo(const char* msg, PVMFSharedMediaDataPtr mediaData, int32 cmdid, int32 qdepth)
{
    if (!mediaData.GetRep())
        return;
    LOGDATAPATH(
        (0, "MOUT %s %s, Write Id %d, MediaData SeqNum %d, SId %d, TS %d, Cleanup Q-depth %d"
         , PortName()
         , msg
         , cmdid
         , mediaData->getSeqNum()
         , mediaData->getStreamID()
         , mediaData->getTimestamp()
         , qdepth
        ));
}


//for logging media xfer info
void PVMediaOutputNodePort::LogDatapath(const char*msg)
{
    LOGDATAPATH(
        (0, "MOUT %s %s"
         , PortName()
         , msg
        ));
}


////////////////////////////////////////////////////////////////////////////
PVMediaOutputNodePort::PVMediaOutputNodePort(PVMediaOutputNode* aNode)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVMediaOutputNodePort")
        , PvmfPortBaseImpl(PVMF_MEDIAIO_NODE_INPUT_PORT_TAG
                           //this port handles its own port activity
                           , this
                           , 10, 10, 70
                           //output queue isn't needed.
                           , 0, 0, 0, "MediaOut")
        , iNode(aNode)
{
    AddToScheduler();

    iWaitForConfig = true;
    isUnCompressedMIO = false;

    iExtensionRefCount = 0;
    iPortFormat = PVMF_FORMAT_UNKNOWN;

    iMediaTransfer = NULL;
    iMioInfoErrorCmdId = 0;
    iMediaTypeIndex = PVMF_FORMAT_UNKNOWN;
    iWriteState = EWriteOK;
    iCleanupQueue.reserve(1);
    iWriteAsyncContext = 0;
    iWriteAsyncEOSContext = 0;
    iWriteAsyncReConfigContext = 0;
    iClock = NULL;
    iClockRate = 1;
    iEarlyMargin = 0;
    iLateMargin = 0;
    iDelayTimer = NULL;
    // By default we treat the MIO's to be active. For active MIO's MIO node will not
    // wait for clock to start before it sends out the data to MIO comp. For passive MIO's
    // oActiveMediaOutputComp and oProcessIncomingMessage will be set to false in EnableMediaSync.
    // MIO node will wait for clock to start before sending data to passive MIO's.
    oActiveMediaOutputComp = true;
    oProcessIncomingMessage = true;
    iConsecutiveFramesDropped = 0;
    iLateFrameEventSent = false;
    iFragIndex = 0;

    iSkipTimestamp = 0;
    iRecentStreamID = 0;
    iSendStartOfDataEvent = false;

    iFrameStepMode = false;
    iClockFrameCount = 0;
    iSyncFrameCount = 0;
    iEOSStreamId  = 0;

    iOsclErrorTrapImp = OsclErrorTrap::GetErrorTrapImp();
    iLogger = PVLogger::GetLoggerObject("PVMediaOutputNodePort");
    iDatapathLogger = PVLogger::GetLoggerObject("datapath.sinknode");
    iDatapathLoggerIn = PVLogger::GetLoggerObject("datapath.sinknode.in");
    iDatapathLoggerOut = PVLogger::GetLoggerObject("datapath.sinknode.out");
    iReposLogger = PVLogger::GetLoggerObject("pvplayerrepos.mionode");
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::ClearCleanupQueue()
//clear the media transfer cleanup queue and log all messages.
{
    while (!iCleanupQueue.empty())
    {
        PVMFSharedMediaDataPtr mediaData = iCleanupQueue.begin()->iData;
        PVMFCommandId cmdId = iCleanupQueue.begin()->iCmdId;
        iCleanupQueue.erase(iCleanupQueue.begin());
        LogMediaDataInfo("Cleared"
                         , mediaData
                         , cmdId
                         , iCleanupQueue.size()
                        );
    }
}

////////////////////////////////////////////////////////////////////////////
PVMediaOutputNodePort::~PVMediaOutputNodePort()
{
    Disconnect();
    PvmfPortBaseImpl::ClearMsgQueues();
    //cancel any pending write operations
    if (!iCleanupQueue.empty())
    {
        int32 err;
        OSCL_TRY(err, iMediaTransfer->cancelAllCommands(););
        ClearCleanupQueue();
    }
    if (iDelayTimer != NULL)
    {
        iDelayTimer->Stop();
        OSCL_DELETE(iDelayTimer);
        iDelayTimer = NULL;
    }
    if (iClock != NULL)
    {
        iClock->RemoveClockObserver(*this);
        iClock->RemoveClockStateObserver(*this);
        iClock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePort::Configure(OSCL_String& fmtstr)
{
    //This is called when the format is being set.
    if (iConnectedPort)
    {
        // Must disconnect before changing port properties, so return error
        return PVMFFailure;
    }
    PVMFFormatType fmt = GetFormatIndex(fmtstr.get_str());
    //set port name for datapath logging.
    int32 mediaindex = GetMediaTypeIndex(fmt);

    if (mediaindex == PVMF_COMPRESSED_AUDIO_FORMAT ||
            mediaindex == PVMF_UNCOMPRESSED_AUDIO_FORMAT)
    {
        SetName("MediaOutIn(Audio)");
    }
    else if (mediaindex == PVMF_COMPRESSED_VIDEO_FORMAT ||
             mediaindex == PVMF_UNCOMPRESSED_VIDEO_FORMAT)
    {
        SetName("MediaOutIn(Video)");
    }
    else
    {
        SetName("MediaOutIn");
    }

    if ((mediaindex == PVMF_UNCOMPRESSED_AUDIO_FORMAT) ||
            (mediaindex == PVMF_UNCOMPRESSED_VIDEO_FORMAT))
    {
        isUnCompressedMIO = true;
    }

    if (IsFormatSupported(fmt))
    {
        iPortFormat = fmt;
        iSinkFormat = fmt;
        iSinkFormatString = fmtstr;
        FormatUpdated();
        return PVMFSuccess;
    }
    else
    {
        iPortFormat = PVMF_FORMAT_UNKNOWN;
        iSinkFormat = PVMF_FORMAT_UNKNOWN;
        iSinkFormatString = fmtstr;
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
// override the PvmfPortBaseImpl routine
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::Connect(PVMFPortInterface* aPort)
{
    PVMFStatus status = PvmfPortBaseImpl::Connect(aPort);
    if (status != PVMFSuccess)
    {
        return status;
    }

    if (iMediaTransfer == NULL)
    {
        iMediaTransfer = iNode->iMIOControl->createMediaTransfer(iNode->iMIOSession);
        if (iMediaTransfer)
        {
            iMediaTransfer->setPeer(this);
        }
        else
        {
            return PVMFFailure;
        }
    }

    iSkipTimestamp = 0;

    PvmiCapabilityAndConfig *config;
    aPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID,
                          (OsclAny*&)config);
    if (config != NULL)
    {
        PvmiKvp* configKvp = NULL;
        PvmiKvp* aRet_kvp = NULL;
        int num_elements = 0;
        PVMFStatus status = config->getParametersSync(NULL, NULL, configKvp, num_elements, NULL);

        if (status == PVMFSuccess)
        {
            iNode->iMIOConfig->setParametersSync(iNode->iMIOSession,
                                                 configKvp,
                                                 num_elements,
                                                 aRet_kvp);
            config->releaseParameters(NULL, configKvp, num_elements);
        }
    }
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
// override the PvmfPortBaseImpl routine
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::PeerConnect(PVMFPortInterface* aPort)
{
    PVMFStatus status = PvmfPortBaseImpl::PeerConnect(aPort);
    if (status != PVMFSuccess)
    {
        return status;
    }

    if (iMediaTransfer == NULL)
    {
        iMediaTransfer = iNode->iMIOControl->createMediaTransfer(iNode->iMIOSession);
        if (iMediaTransfer)
        {
            iMediaTransfer->setPeer(this);
        }
        else
        {
            return PVMFFailure;
        }
    }
    return PVMFSuccess;
}

void PVMediaOutputNodePort::CleanupMediaTransfer()
{
    int32 err;
    //Just in case the media transfer did not report all the async write
    //completes, cancel any pending write operations and clear up the cleanup queue
    if (!iCleanupQueue.empty())
    {
        err = 0;
        OSCL_TRY(err, iMediaTransfer->cancelAllCommands(););
        ClearCleanupQueue();
    }
    if (iNode && iNode->iMIOControl && iMediaTransfer)
    {
        iMediaTransfer->setPeer(NULL);
        err = 0;
        OSCL_TRY(err, iNode->iMIOControl->deleteMediaTransfer(iNode->iMIOSession, iMediaTransfer));
        OSCL_FIRST_CATCH_ANY(err,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                             (0, "PVMediaOutputNodePort::CleanupMediaTransfer Caught a leave when calling deleteMediaTransfer!"));
                            );
        iMediaTransfer = NULL;
    }
    if (iClock != NULL)
    {
        iClock->RemoveClockObserver(*this);
        iClock->RemoveClockStateObserver(*this);
        iClock = NULL;
    }
    if (iCurrentMediaMsg.GetRep() != NULL)
    {
        iCurrentMediaMsg.Unbind();
    }
}

////////////////////////////////////////////////////////////////////////////
// override the PvmfPortBaseImpl routine
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::Disconnect()
{
    CleanupMediaTransfer();
    return PvmfPortBaseImpl::Disconnect();
}

////////////////////////////////////////////////////////////////////////////
// override the PvmfPortBaseImpl routine
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::PeerDisconnect()
{
    PVMFStatus status = PvmfPortBaseImpl::PeerDisconnect();
    if (status != PVMFSuccess)
    {
        return status;
    }
    CleanupMediaTransfer();
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
// override the PvmfPortBaseImpl routine
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::ClearMsgQueues()
{
    if (iCurrentMediaMsg.GetRep() != NULL)
        iCurrentMediaMsg.Unbind();

    PvmfPortBaseImpl::ClearMsgQueues();
    //cancel any pending write operations
    if (!iCleanupQueue.empty())
    {
        int32 err;
        OSCL_TRY(err, iMediaTransfer->cancelAllCommands(););
        ClearCleanupQueue();
    }

    //may need to generate port flow control now
    PvmfPortBaseImpl::EvaluateIncomingBusy();

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::addRef()
// for PVInterface
{
    ++iExtensionRefCount;
}

///////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::removeRef()
// for PVInterface
{
    if (iExtensionRefCount > 0)
    {
        --iExtensionRefCount;
    }
}

////////////////////////////////////////////////////////////////////////////
bool PVMediaOutputNodePort::queryInterface(const PVUuid& uuid, PVInterface*& iface)
// for PVInterface
{
    if (uuid == PvmfNodesSyncControlUuid)
    {
        PvmfNodesSyncControlInterface* myInterface = OSCL_STATIC_CAST(PvmfNodesSyncControlInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        addRef();
    }
    else
    {
        iface = NULL;
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::NodeStarted()
{
    RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
//for sync control interface
PVMFStatus PVMediaOutputNodePort::SetClock(OsclClock* aClock)
{
    if (aClock == NULL)
    {
        return PVMFErrArgument;
    }
    iClock = aClock;
    iClock->SetClockObserver(*this);
    iClock->SetClockStateObserver(*this);
    return PVMFSuccess;
}

void PVMediaOutputNodePort::EnableMediaSync()
{
    //wait on play clock
    oProcessIncomingMessage = false;
    oActiveMediaOutputComp = false;
    if (iDelayTimer != NULL)
    {
        OSCL_DELETE(iDelayTimer);
        iDelayTimer = NULL;
    }
    iDelayTimer = OSCL_NEW(PVMediaOutputNodePortTimer, (this));
}

////////////////////////////////////////////////////////////////////////////
//for sync control interface
PVMFStatus PVMediaOutputNodePort::ChangeClockRate(int32 aRate)
{
    iClockRate = aRate;

    PVMFStatus status;
    status = SetMIOParameterInt32(MOUT_MEDIAXFER_OUTPUT_RATE, aRate);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,
                    "PVMediaOutputNodePort::ChangeClockRate rate %d",
                    aRate));

    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePort::SetMargins(int32 aEarlyMargin, int32 aLateMargin)
//for sync control interface
{
    iEarlyMargin = aEarlyMargin;
    iLateMargin = aLateMargin;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::ClockStarted()
//for sync control interface
{
    if (IsBusy())
    {
        // Cancel any long waits due to data synchronization done
        // before clock was set and started
        Cancel();
    }
    RunIfNotReady();
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::ClockStopped()
//for sync control interface
{
    ;//ignore
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::setPeer(PvmiMediaTransfer *aPeer)
//for PvmiMediaTransfer
{
    OSCL_UNUSED_ARG(aPeer);
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::useMemoryAllocators(OsclMemAllocator* write_alloc)
//for PvmiMediaTransfer
{
    OSCL_UNUSED_ARG(write_alloc);
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMediaOutputNodePort::writeAsync(uint8 format_type, int32 format_index,
        uint8* data, uint32 data_len,
        const PvmiMediaXferHeader& data_header_info,
        OsclAny* context)
//for PvmiMediaTransfer
{
    OSCL_UNUSED_ARG(data_header_info);
    OSCL_UNUSED_ARG(context);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVMediaOutputNodePort::writeAsync: format_type=%d, format_index=%d, data=0x%x, data_len=%d",
                     format_type, format_index, data, data_len));

    PVMFAsyncEvent* event = NULL;
    switch (format_type)
    {
        case PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION:
            switch (format_index)
            {
                case PVMI_MEDIAXFER_FMT_INDEX_INFO_EVENT:
                    OSCL_ASSERT(iNode);
                    OSCL_ASSERT(iMediaTransfer);

                    if (!data)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMediaOutputNodePort::writeAsync: Error - data is NULL"));
                        OSCL_LEAVE(OsclErrArgument);
                        return -1;
                    }

                    if (data_len != sizeof(PVMFAsyncEvent))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMediaOutputNodePort::writeAsync: Error - data length is not size of PVMFAsyncEvent"));
                        OSCL_LEAVE(OsclErrArgument);
                        return -1;
                    }

                    event = OSCL_STATIC_CAST(PVMFAsyncEvent*, data);
                    iNode->ReportInfoEvent((*event));

                    // Not really processing this asynchronously. Just call writeComplete
                    // synchronously
                    iMediaTransfer->writeComplete(PVMFSuccess, iMioInfoErrorCmdId, context);
                    return iMioInfoErrorCmdId++;

                case PVMI_MEDIAXFER_FMT_INDEX_ERROR_EVENT:
                    OSCL_ASSERT(iNode);
                    OSCL_ASSERT(iMediaTransfer);

                    if (!data)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMediaOutputNodePort::writeAsync: Error - data is NULL"));
                        OSCL_LEAVE(OsclErrArgument);
                        return -1;
                    }

                    if (data_len != sizeof(PVMFAsyncEvent))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                        (0, "PVMediaOutputNodePort::writeAsync: Error - data length is not size of PVMFAsyncEvent"));
                        OSCL_LEAVE(OsclErrArgument);
                        return -1;
                    }

                    event = OSCL_STATIC_CAST(PVMFAsyncEvent*, data);
                    iNode->ReportErrorEvent((*event));

                    // Not really processing this asynchronously. Just call writeComplete
                    // synchronously
                    iMediaTransfer->writeComplete(PVMFSuccess, iMioInfoErrorCmdId, context);
                    return iMioInfoErrorCmdId++;

                default:
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PVMediaOutputNodePort::writeAsync: Error - Unsupported format_index"));
                    OSCL_LEAVE(OsclErrNotSupported);
                    break;
            }

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVMediaOutputNodePort::writeAsync: Error - Unsupported format_type"));
            OSCL_LEAVE(OsclErrNotSupported);
            break;
    }

    return -1;
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::writeComplete(PVMFStatus status, PVMFCommandId aCmdId, OsclAny* aContext)
//for PvmiMediaTransfer
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO,
                    (0, "PVMediaOutputNodePort::writeComplete status %d cmdId %d context 0x%x", status, aCmdId, aContext));

    //we don't expect any error status to be returned here
    //except possibly cancelled
    if (status != PVMFSuccess && status != PVMFErrCancelled)
    {
        iNode->ReportErrorEvent(PVMFErrPortProcessing, NULL, PVMFMoutNodeErr_WriteComplete);
    }

    // Check if the writeComplete is in response to EOS message
    if (&iWriteAsyncEOSContext == (uint32*)aContext)
    {
        if (iWriteState == EWriteBusy)
        {
            // Synchronous completion
            // Let EndOfData info event be sent out in EndOfData() function
            iWriteState = EWriteOK;
        }
        else
        {
            // Asynchronous completion
            if (status == PVMFSuccess && iNode->IsMioRequestPending() == false)
            {
                // Report End of Data to the user of media output node
                // only if the EOS media transfer completes successfully and
                // there is no pending MIO control request
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::writeComplete For EOS - Fmt=%s",
                                         iSinkFormatString.get_str()));
                uint32 iStreamID = iEOSStreamId;
                iNode->ReportInfoEvent(PVMFInfoEndOfData, (OsclAny*)&iStreamID);
            }
            else
            {
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::writeComplete EOS media transfer completed but PVMFInfoEndOfData not sent"));
            }
        }
    }
    // Check if the writeComplete is in response to Reconfig message
    else if (&iWriteAsyncReConfigContext == (uint32*)aContext)
    {
        if (iWriteState == EWriteBusy)
        {
            // Synchronous completion
            iWriteState = EWriteOK;
        }
        else
        {
            // Asynchronous completion
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::writeComplete For RECONFIG - Fmt=%s",
                                     iSinkFormatString.get_str()));
        }
    }
    //detect cases where the current SendData call is completing synchronously.
    else if (iWriteState == EWriteBusy && (uint32)aContext == iWriteAsyncContext)
    {
        //synchronous completion
        iWriteState = EWriteOK;
    }
    else
    {
        //asynchronous completion.
        //do any memory cleanup
        bool oCmdIdFound = false;
        uint32 i;
        for (i = 0;i < iCleanupQueue.size();i++)
        {
            if (iCleanupQueue[i].iCmdId == aCmdId)
            {
                PVMFSharedMediaDataPtr mediaData = iCleanupQueue[i].iData;
                iCleanupQueue.erase(&iCleanupQueue[i]);
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::writeComplete - Fmt=%s, Seq=%d, TS=%d, FIdx=%d, ClnUpQSize=%d",
                                         iSinkFormatString.get_str(),
                                         mediaData->getSeqNum(),
                                         mediaData->getTimestamp(),
                                         iFragIndex,
                                         iCleanupQueue.size()));
                oCmdIdFound = true;
                break;
            }
        }
        if (oCmdIdFound == false)
        {
            PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::writeComplete - CmdId Not Found - Fmt=%s, FIdx=%d, ClnUpQSize=%d",
                                  iSinkFormatString.get_str(),
                                  iFragIndex,
                                  iCleanupQueue.size()));
        }
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFCommandId PVMediaOutputNodePort::readAsync(uint8* data, uint32 max_data_len, OsclAny* context,
        int32* formats, uint16 num_formats)
//for PvmiMediaTransfer
{
    OSCL_UNUSED_ARG(data);
    OSCL_UNUSED_ARG(max_data_len);
    OSCL_UNUSED_ARG(context);
    OSCL_UNUSED_ARG(formats);
    OSCL_UNUSED_ARG(num_formats);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::readComplete(PVMFStatus status, PVMFCommandId read_cmd_id,
        int32 format_index, const PvmiMediaXferHeader& data_header_info,
        OsclAny* context)
//for PvmiMediaTransfer
{
    OSCL_UNUSED_ARG(status);
    OSCL_UNUSED_ARG(read_cmd_id);
    OSCL_UNUSED_ARG(format_index);
    OSCL_UNUSED_ARG(data_header_info);
    OSCL_UNUSED_ARG(context);
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::statusUpdate(uint32 status_flags)
//for PvmiMediaTransfer
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO,
                    (0, "PVMediaOutputNodePort::statusUpdate flags %d", status_flags));

    if (status_flags & PVMI_MEDIAXFER_STATUS_WRITE)
    {
        //recover from a previous async write error.
        if (iWriteState == EWriteWait && (iClock->GetState() != OsclClock::STOPPED))
        {
            iWriteState = EWriteOK;
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::statusUpdate - WriteAsync Enabled - Fmt=%s, iWriteState=%d",
                                     iSinkFormatString.get_str(),
                                     iWriteState));
            oProcessIncomingMessage = true;
            if (iCurrentMediaMsg.GetRep() != NULL)
            {
                //attempt to send data current media msg if any
                SendData();
            }
            //reschedule if there is more stuff waiting and
            //if we can process more data
            if (IncomingMsgQueueSize() > 0)
            {
                RunIfNotReady();
            }
        }
    }
    else
    {
        //disable write
        iWriteState = EWriteWait;
        oProcessIncomingMessage = false;
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::statusUpdate - WriteAsync Disabled - Fmt=%s, iWriteState=%d",
                                 iSinkFormatString.get_str(),
                                 iWriteState));
    }
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::cancelCommand(PVMFCommandId command_id)
//for PvmiMediaTransfer
{
    OSCL_UNUSED_ARG(command_id);
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::cancelAllCommands()
//for PvmiMediaTransfer
{
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
bool PVMediaOutputNodePort::IsFormatSupported(PVMFFormatType aFmt)
// for PvmiCapabilityAndConfigPortFormatImpl interface
{
    //Try to get supported formats from the media I/O component.
    PvmiKvp* kvp = NULL;
    int numParams = 0;
    PVMFStatus status = iNode->iMIOConfig->getParametersSync(NULL, INPUT_FORMATS_CAP_QUERY, kvp, numParams, NULL);
    if (status == PVMFSuccess)
    {
        bool found = false;
        for (int32 i = 0;i < numParams;i++)
        {
            if (aFmt == kvp[i].value.uint32_value)
            {
                found = true;
                break;
            }
        }
        iNode->iMIOConfig->releaseParameters(0, kvp, numParams);
        return found;
    }
    else
    {
        //if media I/O did not report its formats, then assume all formats
        //are OK.  do this since some MIO may not have the query support.
        //at least verify it's an audio or video format...
        int32 mt = GetMediaTypeIndex(aFmt);
        switch (mt)
        {
            case PVMF_COMPRESSED_AUDIO_FORMAT:
            case PVMF_COMPRESSED_VIDEO_FORMAT:
            case PVMF_UNCOMPRESSED_AUDIO_FORMAT:
            case PVMF_UNCOMPRESSED_VIDEO_FORMAT:
            case PVMF_TEXT_FORMAT:
                return true;
            default:
                return false;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::FormatUpdated()
// for PvmiCapabilityAndConfigPortFormatImpl interface
{
    //called when format was just set, either through capability and config,
    //for by node during port request.
    //save the media type index.
    iMediaTypeIndex = GetMediaTypeIndex(iSinkFormat);

    switch (iMediaTypeIndex)
    {
        case PVMF_COMPRESSED_AUDIO_FORMAT:
            //pass the selected format to the MIO component.
            //ignore any failure since not all MIO may support the feature.
            SetMIOParameterPchar(MOUT_AUDIO_FORMAT_KEY, iSinkFormatString.get_str());
            break;

        case PVMF_COMPRESSED_VIDEO_FORMAT:
            //pass the selected format to the MIO component.
            //ignore any failure since not all MIO may support the feature.
            SetMIOParameterPchar(MOUT_VIDEO_FORMAT_KEY, iSinkFormatString.get_str());
            break;

        case PVMF_UNCOMPRESSED_AUDIO_FORMAT:
            //pass the selected format to the MIO component.
            //ignore any failure since not all MIO may support the feature.
            SetMIOParameterPchar(MOUT_AUDIO_FORMAT_KEY, iSinkFormatString.get_str());
            break;

        case PVMF_UNCOMPRESSED_VIDEO_FORMAT:
            //pass the selected format to the MIO component.
            //ignore any failure since not all MIO may support the feature.
            SetMIOParameterPchar(MOUT_VIDEO_FORMAT_KEY, iSinkFormatString.get_str());
            break;

        case PVMF_TEXT_FORMAT:
            //pass the selected format to the MIO component.
            //ignore any failure since not all MIO may support the feature.
            SetMIOParameterPchar(MOUT_TEXT_FORMAT_KEY, iSinkFormatString.get_str());
            break;

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::SendData()
//send data to the MIO componenent.
{
    if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
    {
        SendEndOfData();
    }
    else if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_RE_CONFIG_FORMAT_ID)
    {
        SendReConfigNotification();
    }
    else if (iCurrentMediaMsg->getFormatID() < PVMF_MEDIA_CMD_FORMAT_IDS_START)
    {
        if (oActiveMediaOutputComp)
        {
            SendMediaData();
        }
        else if (iFrameStepMode == false)
        {
            uint32 delta = 0;
            PVMFMediaOutputNodePortMediaTimeStatus status = CheckMediaTimeStamp(delta);
            if (status == PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ON_TIME)
            {
                SendMediaData();
            }
            else if (status == PVMF_MEDIAOUTPUTNODEPORT_MEDIA_LATE)
            {
                iCurrentMediaMsg.Unbind();
                iFragIndex = 0;
            }
            else if (status == PVMF_MEDIAOUTPUTNODEPORT_MEDIA_EARLY)
            {
                //stop processing input, since we are not done with the current media msg
                oProcessIncomingMessage = false;
                if (iDelayTimer != NULL)
                {
                    iDelayTimer->setTimerDurationInMS(delta);
                    iDelayTimer->Start();
                }
                else
                {
                    PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::SendData - Fmt=%s, No Delay Timer",
                                          iSinkFormatString.get_str()));
                    OSCL_ASSERT(false);
                }
            }
        }
        else if (iFrameStepMode == true)
        {
            PVMFMediaOutputNodePortMediaTimeStatus status = CheckMediaFrameStep();
            if (status == PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ON_TIME)
            {
                SendMediaData();
            }
            else if (status == PVMF_MEDIAOUTPUTNODEPORT_MEDIA_LATE)
            {
                iCurrentMediaMsg.Unbind();
                iFragIndex = 0;
            }
            else if (status == PVMF_MEDIAOUTPUTNODEPORT_MEDIA_EARLY)
            {
                //stop processing input, since we are not done with the current media msg
                oProcessIncomingMessage = false;
                //wait on ClockCountUpdated call back from oscl clock
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::SendMediaData()
//send media data to the MIO componenent.
{
    if (iWriteState != EWriteOK)
    {
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendMediaData - Fmt=%s - Invalid WriteAsync State - State=%d",
                                 iSinkFormatString.get_str(),
                                 iWriteState));
        PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::SendMediaData - Fmt=%s - Invalid WriteAsync State - State=%d",
                              iSinkFormatString.get_str(),
                              iWriteState));
        OSCL_ASSERT(false);
    }

    PVMFSharedMediaDataPtr mediaData;
    convertToPVMFMediaData(mediaData, iCurrentMediaMsg);

    uint32 duration = 0;
    if (mediaData->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_DURATION_AVAILABLE_BIT)
    {
        duration = mediaData->getDuration();
    }

    // extract private data (if it is in fsi)
    OsclAny *privatedataptr = NULL;
    uint32 privatedatalength = 0;

    OsclRefCounterMemFrag fsifrag;
    mediaData->getFormatSpecificInfo(fsifrag);

    uint32 *data = (uint32 *)fsifrag.getMemFragPtr();
    uint32 data_len = fsifrag.getMemFragSize();
    if (data && data_len > 0)
    {
        //data to extract
        privatedataptr = (OsclAny*)(*data);
        privatedatalength = data_len;
    }


    for (uint32 fragindex = iFragIndex; fragindex < mediaData->getNumFragments();)
    {
        OsclRefCounterMemFrag frag;
        mediaData->getMediaFragment(fragindex, frag);

        ++iWriteAsyncContext;
        iWriteState = EWriteBusy;
        int32 err;
        int32 cmdId = 0;

        uint32 flags = PVMI_MEDIAXFER_MEDIA_DATA_FLAG_NONE;

        // The marker bit should only be set for the final frag to allow MIO to reassemble data properly
        if ((mediaData->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_M_BIT) &&
                (fragindex == (mediaData->getNumFragments() - 1)))
        {
            flags |= PVMI_MEDIAXFER_MEDIA_DATA_FLAG_MARKER_BIT;
        }
        if (mediaData->getMarkerInfo() & PVMF_MEDIA_DATA_MARKER_INFO_NO_RENDER_BIT)
        {
            flags |= PVMI_MEDIAXFER_MEDIA_DATA_FLAG_NO_RENDER_BIT;
        }
        PvmiMediaXferHeader mediaxferhdr;
        mediaxferhdr.seq_num = mediaData->getSeqNum();
        mediaxferhdr.timestamp = mediaData->getTimestamp();
        mediaxferhdr.duration = duration;
        mediaxferhdr.flags = flags;
        mediaxferhdr.stream_id = mediaData->getStreamID();
        mediaxferhdr.private_data_length = privatedatalength;
        mediaxferhdr.private_data_ptr = privatedataptr;
        OSCL_TRY_NO_TLS(iOsclErrorTrapImp, err,
                        cmdId = iMediaTransfer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_DATA,  /*format_type*/
                                                           PVMI_MEDIAXFER_FMT_INDEX_DATA, /*format_index*/
                                                           (uint8*)frag.getMemFragPtr(),
                                                           frag.getMemFragSize(),
                                                           mediaxferhdr,
                                                           (OsclAny*)iWriteAsyncContext);
                       );

        if (err != OsclErrNone)
        {
            //if a leave occurs in the writeAsync call, we suspend data
            //transfer until a statusUpdate call from the MIO component
            //tells us to resume.
            //this is not an error-- it's the normal flow control mechanism.
            iWriteState = EWriteWait;

            //stop processing input, since we are not done with the current media msg
            oProcessIncomingMessage = false;

            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendMediaData - WriteAsyncLeave - Fmt=%s, LeaveCode=%d",
                                     iSinkFormatString.get_str(),
                                     err));

            return ;//wait on statusUpdate call from the MIO component.
        }
        else
        {
            fragindex++;
            iFragIndex++;
            if (fragindex == mediaData->getNumFragments())
            {
                //all fragments have been sent.  see whether completion
                //is synchronous or asynchronous.
                if (iWriteState == EWriteBusy)
                {
                    //asynchronous completion.
                    //push the data onto the cleanup stack so it won't get cleaned
                    //up until the component consumes it.
                    iCleanupQueue.push_back(CleanupQueueElement(mediaData, cmdId));
                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendMediaData - AsyncWrite - Fmt=%s, Seq=%d, TS=%d, Dur=%d, FIdx=%d, ClnUpQSize=%d",
                                             iSinkFormatString.get_str(),
                                             mediaData->getSeqNum(),
                                             mediaData->getTimestamp(),
                                             duration,
                                             iFragIndex,
                                             iCleanupQueue.size()));
                }
                //else the write already completed synchronously.
                else
                {
                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendMediaData - SyncWrite - Fmt=%s, Seq=%d, TS=%d, Dur=%d, FIdx=%d, ClnUpQSize=%d",
                                             iSinkFormatString.get_str(),
                                             mediaData->getSeqNum(),
                                             mediaData->getTimestamp(),
                                             duration,
                                             iFragIndex,
                                             iCleanupQueue.size()));
                }
                //we are done with current media msg - either we are truly done (sync write complete) or we are
                //waiting on async write complete in which case media msg has been pushed into the cleanup queue
                iCurrentMediaMsg.Unbind();
                iFragIndex = 0;
            }
            iWriteState = EWriteOK;
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::SendEndOfData()
//send end of data notice to the MIO componenent.
{
    if (iWriteState != EWriteOK)
    {
        PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::SendEndOfData - Invalid WriteAsync State - State=%d", iWriteState));
        OSCL_ASSERT(false);
    }

    uint32 eosseqnum = iCurrentMediaMsg->getSeqNum();
    uint32 eostimestamp = iCurrentMediaMsg->getTimestamp();

    iWriteState = EWriteBusy;

    int32 err;
    int32 cmdId = 0;

    PvmiMediaXferHeader mediaxferhdr;
    mediaxferhdr.seq_num = eosseqnum;
    mediaxferhdr.timestamp = eostimestamp;
    mediaxferhdr.duration = 0;
    mediaxferhdr.flags = PVMI_MEDIAXFER_MEDIA_DATA_FLAG_NONE;
    mediaxferhdr.stream_id = iCurrentMediaMsg->getStreamID();
    iEOSStreamId = mediaxferhdr.stream_id;
    OSCL_TRY(err,
             cmdId = iMediaTransfer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION,  /*format_type*/
                                                PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM, /*format_index*/
                                                NULL,
                                                0,
                                                mediaxferhdr,
                                                (OsclAny*) & iWriteAsyncEOSContext);
            );

    if (err != OsclErrNone)
    {
        //if a leave occurs in the writeAsync call, we suspend data
        //transfer until a statusUpdate call from the MIO component
        //tells us to resume.
        //this is not an error-- it's the normal flow control mechanism.
        iWriteState = EWriteWait;

        //stop processing input, since we are not done with the current media msg
        oProcessIncomingMessage = false;

        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendEndOfData - WriteAsyncLeave - Fmt=%s, LeaveCode=%d",
                                 iSinkFormatString.get_str(),
                                 err));

        return ;//wait on statusUpdate call from the MIO component
    }
    else
    {
        if (iWriteState == EWriteBusy)
        {
            //asynchronous completion.
            //wait for writeComplete to be called for this request
            //log media data info.
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendEndOfData - AsyncWrite - Fmt=%s, Seq=%d, TS=%d, ClnUpQSize=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     iCleanupQueue.size()));

            iWriteState = EWriteOK;

        }
        //else the write already completed synchronously.
        else
        {
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendEndOfData - SyncWrite - Fmt=%s, Seq=%d, TS=%d, ClnUpQSize=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     iCleanupQueue.size()));

            // Report End of Data to the user of media output node
            uint32 iStreamID = iEOSStreamId;
            iNode->ReportInfoEvent(PVMFInfoEndOfData, (OsclAny*)&iStreamID);
        }
        //we are done with current media msg - either we are truly done (sync write complete) or we are
        //waiting on async write complete. there is no need to push this msg into cleanup queue since it
        //is a notification as opossed to media data
        iCurrentMediaMsg.Unbind();
        iFragIndex = 0;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::SendReConfigNotification()
//send reconfig notice to the MIO componenent.
{
    if (iWriteState != EWriteOK)
    {
        PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::SendReConfigNotification - Invalid WriteAsync State - State=%d", iWriteState));
        OSCL_ASSERT(false);
    }

    iWriteState = EWriteBusy;
    int32 err;
    int32 cmdId = 0;
    PvmiMediaXferHeader mediaxferhdr;
    mediaxferhdr.seq_num = iCurrentMediaMsg->getSeqNum();
    mediaxferhdr.timestamp = 0;
    mediaxferhdr.duration = 0;
    mediaxferhdr.flags = PVMI_MEDIAXFER_MEDIA_DATA_FLAG_NONE;
    mediaxferhdr.stream_id = iCurrentMediaMsg->getStreamID();
    OSCL_TRY(err,
             cmdId = iMediaTransfer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION,  /*format_type*/
                                                PVMI_MEDIAXFER_FMT_INDEX_RE_CONFIG_NOTIFICATION, /*format_index*/
                                                NULL, 0,
                                                mediaxferhdr,
                                                (OsclAny*) & iWriteAsyncReConfigContext);
            );

    if (err != OsclErrNone)
    {
        PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::SendReConfigNotification SendReConfigNotification leave code %d", err));
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendReConfigNotification SendReConfigNotification leave code %d", err));
        //we don't handle a leave here-- it's an error
        //media output comps should be open to recving reconfig anytime
        //they are free to process it at a later point in time and they may
        //choose to accept the reconfig and reject it. but there should never
        //be a case where cannot even queue the reconfig request
        iNode->ReportErrorEvent(PVMFErrResource, NULL, PVMFMoutNodeErr_WriteAsync);
    }
    else
    {
        if (iWriteState == EWriteBusy)
        {
            //asynchronous completion.
            //wait for writeComplete to be called for this request
            //log media data info.
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendReConfigNotification - AsyncWrite - Fmt=%s, Seq=%d, TS=%d, ClnUpQSize=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     iCleanupQueue.size()));

            iWriteState = EWriteOK;

        }
        //else the write already completed synchronously.
        else
        {
            //log media data info.
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SendEndOfData - SyncWrite - Fmt=%s, Seq=%d, TS=%d, ClnUpQSize=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     iCleanupQueue.size()));
        }
        //we are done with current media msg - either we are truly done (sync write complete) or we are
        //waiting on async write complete. there is no need to push this msg into cleanup queue since it
        //is a notification as opossed to media data
        iCurrentMediaMsg.Unbind();
        iFragIndex = 0;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFMediaOutputNodePortMediaTimeStatus
PVMediaOutputNodePort::CheckMediaTimeStamp(uint32& aDelta)
{
    PVMFTimestamp aTimeStamp = iCurrentMediaMsg->getTimestamp();
    aDelta = 0;
    //aTimeStamp is assumed to be in milliseconds
    if (iClock != NULL)
    {
        uint64 clock_msec64;
        iClock->GetCurrentTime64(clock_msec64, OSCLCLOCK_MSEC);
        uint32 clock_msec32 = Oscl_Int64_Utils::get_uint64_lower32(clock_msec64);

        uint32 clock_adjforearlymargin = clock_msec32 + iEarlyMargin;
        uint32 ts_adjforlatemargin = aTimeStamp + iLateMargin;

        if ((clock_adjforearlymargin - aTimeStamp) > WRAP_THRESHOLD)
        {
            // This condition indicates that the timestamp is ahead of the adjusted playback clock
            // Note that since the computation is based on 32-bit values, it has a limitation that
            // it will not work for durations exceeding 2^32 milliseconds = 49+ days which is an acceptable
            // limit for this application.
            // Say clock is 1000ms, early margin is 200ms, then any timestamp later than 1200ms should be
            // held back. If ts is greater than 1200ms, then (1200 - ts) will be larger than wrap threshold
            uint32 deltaInMS = (aTimeStamp - clock_msec32);
            deltaInMS -= iEarlyMargin;
            if (iClockRate > 1)
            {
                //In case of rate change, calculate delta to acct
                //for clock rate
                MediaClockConverter mcc;
                mcc.set_timescale(iClockRate);
                //delta is in milliseconds
                mcc.set_clock_other_timescale(deltaInMS, 1000);
                //get the value in milliseconds
                aDelta = mcc.get_converted_ts(1000);
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaTimeStamp - Early - Fmt=%s, Seq=%d, Ts=%d, Clock=%d, DeltaInMS=%d, ClkRate=%d, DeltaInClkUnits=%d",
                                         iSinkFormatString.get_str(),
                                         iCurrentMediaMsg->getSeqNum(),
                                         aTimeStamp,
                                         clock_msec32,
                                         deltaInMS,
                                         iClockRate,
                                         aDelta));
            }
            else
            {
                aDelta = deltaInMS;
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaTimeStamp - Early - Fmt=%s, Seq=%d, Ts=%d, Clock=%d, Delta=%d",
                                         iSinkFormatString.get_str(),
                                         iCurrentMediaMsg->getSeqNum(),
                                         aTimeStamp,
                                         clock_msec32,
                                         aDelta));
            }
            iConsecutiveFramesDropped = 0;
            return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_EARLY;
        }
        else if ((ts_adjforlatemargin - clock_msec32) > WRAP_THRESHOLD)
        {
            // This condition indicates that the clock is ahead of the adjusted timestamp
            // Note that since the computation is based on 32-bit values, it has a limitation that
            // it will not work for durations exceeding 2^32 milliseconds = 49+ days which is an acceptable
            // limit for this application.
            // Say clock is 1000ms, late margin is 200ms, then any timestamp earlier than 800ms should be
            // dropped, as late. If ts is less than 800ms, then (ts + 200 - 1000) will be larger than wrap threshold
            aDelta = (clock_msec32 - aTimeStamp);
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaTimeStamp - Late - Fmt=%s, Seq=%d, Ts=%d, Clock=%d, Delta=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     aTimeStamp,
                                     clock_msec32,
                                     aDelta));
            iConsecutiveFramesDropped++;
            if (iMediaTypeIndex == PVMF_UNCOMPRESSED_VIDEO_FORMAT)
            {
                if ((iConsecutiveFramesDropped >= THRESHOLD_FOR_DROPPED_VIDEO_FRAMES) && (iLateFrameEventSent == false))
                {
                    iLateFrameEventSent = true;
                    iNode->ReportInfoEvent(PVMFInfoVideoTrackFallingBehind);
                }
            }
            return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_LATE;
        }
        else
        {
            //ts falls in the interval (clock-latermargin, clock+earlymargin)
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaTimeStamp - OnTime - Fmt=%s, Seq=%d, Ts=%d, Clock=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     aTimeStamp,
                                     clock_msec32));
            iConsecutiveFramesDropped = 0;
            return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ON_TIME;
        }
    }
    else
    {

        PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::CheckMediaTimeStamp - Fmt=%s, No PlayBack Clock!!!",
                              iSinkFormatString.get_str()));
        OSCL_ASSERT(false);
        return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ERROR;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFMediaOutputNodePortMediaTimeStatus
PVMediaOutputNodePort::CheckMediaFrameStep()
{
    if (iClock != NULL)
    {
        if (iClockFrameCount > iSyncFrameCount)
        {
            //this condition implies that clock count has
            //gone past the sync frame count, late frame, drop it
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaFrameStep - Late - Fmt=%s, Seq=%d, Ts=%d, iClockFrameCount=%d, iSyncFrameCount=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     Oscl_Int64_Utils::get_int64_lower32(iClockFrameCount),
                                     Oscl_Int64_Utils::get_int64_lower32(iSyncFrameCount)));
            iSyncFrameCount++;
            return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_LATE;
        }
        else if (iClockFrameCount < iSyncFrameCount)
        {
            //this condition implies that clock count is less than
            //the sync frame count, early frame, hold it
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaFrameStep - Early - Fmt=%s, Seq=%d, Ts=%d, iClockFrameCount=%d, iSyncFrameCount=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     Oscl_Int64_Utils::get_int64_lower32(iClockFrameCount),
                                     Oscl_Int64_Utils::get_int64_lower32(iSyncFrameCount)));
            return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_EARLY;
        }
        else
        {
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CheckMediaFrameStep - OnTime - Fmt=%s, Seq=%d, Ts=%d, iClockFrameCount=%d, iSyncFrameCount=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getSeqNum(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     Oscl_Int64_Utils::get_int64_lower32(iClockFrameCount),
                                     Oscl_Int64_Utils::get_int64_lower32(iSyncFrameCount)));
            iSyncFrameCount++;
            return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ON_TIME;
        }
    }
    else
    {

        PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::CheckMediaTimeStamp - Fmt=%s, No PlayBack Clock!!!",
                              iSinkFormatString.get_str()));
        OSCL_ASSERT(false);
        return PVMF_MEDIAOUTPUTNODEPORT_MEDIA_ERROR;
    }
}

////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::Run()
{
    /*
     * Media Output node inport does not process any more incoming messages
     * under following circumstances:
     * 1) Playback clock is NOT in RUNNING state
     * 2) Media output comp is in a STARTED state
     * 3) When waiting on media output comp to go ready from a busy state
     * Boolean "oProcessIncomingMessage" will be false if any of the above
     * conditions is true.
     * This while loop is intentional. This port doesnt do much in terms of
     * data processing. So we attempt to send as much data as we can in a
     * single Run call. Since we could process multiple media msgs per Run
     * we need to account for BOS and Skip as well.
     */
    while (IncomingMsgQueueSize() > 0)
    {
        bool oMsgDqd = false;
        if (iCurrentMediaMsg.GetRep() == NULL)
        {
            //we are starting to process a new media msg
            iFragIndex = 0;
            if (DequeueIncomingMsg(iCurrentMediaMsg) != PVMFSuccess)
            {
                PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::Run: DequeueIncomingMsg Failed - Fmt=%s",
                                      iSinkFormatString.get_str()));
                OsclError::Panic("PVMOUT", 2);
            }
            else
            {
                //this boolean avoids duplicate logs in case we are skipping data
                oMsgDqd = true;
            }
        }
        if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_BOS_FORMAT_ID)
        {
            uint32 msgStreamId = iCurrentMediaMsg->getStreamID();
            iBOSStreamIDVec.push_back(msgStreamId);

            PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::Run: BOS Recvd - Fmt=%s, TS=%d, StreamID=%d, Qs=%d",
                                  iSinkFormatString.get_str(),
                                  iCurrentMediaMsg->getTimestamp(),
                                  msgStreamId,
                                  IncomingMsgQueueSize()));

            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run: BOS Recvd - Fmt=%s, TS=%d, StreamID=%d, Qs=%d",
                                     iSinkFormatString.get_str(),
                                     iCurrentMediaMsg->getTimestamp(),
                                     msgStreamId,
                                     IncomingMsgQueueSize()));

            iNode->ReportBOS();
            iCurrentMediaMsg.Unbind();
            iFragIndex = 0;
        }
        else if (DataToSkip(iCurrentMediaMsg) == true)
        {
            //this condition implies that a skip timestamp has been set
            //and we are looking for a mediamsg whole timestamp is equal or greater than
            //iSkipTimestamp. All media msgs whose timestamps are less than iSkipTimestamp
            //will be dropped. There is no need for any additional checks. Just dequeue and toss
            if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::Run: EOSSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                      iCurrentMediaMsg->getStreamID(),
                                      iCurrentMediaMsg->getSeqNum(),
                                      iCurrentMediaMsg->getTimestamp(),
                                      iSinkFormatString.get_str()));

                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run: EOSSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                         iCurrentMediaMsg->getStreamID(),
                                         iCurrentMediaMsg->getSeqNum(),
                                         iCurrentMediaMsg->getTimestamp(),
                                         iSinkFormatString.get_str()));
            }
            else
            {
                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::Run: MsgSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                      iCurrentMediaMsg->getStreamID(),
                                      iCurrentMediaMsg->getSeqNum(),
                                      iCurrentMediaMsg->getTimestamp(),
                                      iSinkFormatString.get_str()));

                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run: MsgSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                         iCurrentMediaMsg->getStreamID(),
                                         iCurrentMediaMsg->getSeqNum(),
                                         iCurrentMediaMsg->getTimestamp(),
                                         iSinkFormatString.get_str()));
            }

            iCurrentMediaMsg.Unbind();
            iFragIndex = 0;
        }
        else
        {
            //implies that this message needs to be consumed
            if (iSendStartOfDataEvent == true)
            {
                //implies that we are attempting to process the first media msg during
                //after skip, a mediamsg whose timestamp is equal to or greater than
                //iSkipTimestamp
                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::Run: PVMFInfoStartOfData - Fmt=%s",
                                      iSinkFormatString.get_str()));
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run: PVMFInfoStartOfData - Fmt=%s",
                                         iSinkFormatString.get_str()));
                uint32 iStreamID = iRecentStreamID;
                iNode->ReportInfoEvent(PVMFInfoStartOfData, (OsclAny*)&iStreamID);

                iSendStartOfDataEvent = false;
            }
            if (oMsgDqd == true)
            {
                //logging
                if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
                {
                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run - EOS Recvd - StreamID=%d, Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                             iCurrentMediaMsg->getStreamID(),
                                             iCurrentMediaMsg->getSeqNum(),
                                             iCurrentMediaMsg->getTimestamp(),
                                             iSinkFormatString.get_str(),
                                             IncomingMsgQueueSize()));
                }
                else if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_RE_CONFIG_FORMAT_ID)
                {
                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run - RECONFIG Recvd - Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                             iCurrentMediaMsg->getSeqNum(),
                                             iCurrentMediaMsg->getTimestamp(),
                                             iSinkFormatString.get_str(),
                                             IncomingMsgQueueSize()));
                }
                else if (iCurrentMediaMsg->getFormatID() < PVMF_MEDIA_CMD_FORMAT_IDS_START)
                {
                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::Run - MediaMsg Recvd - Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                             iCurrentMediaMsg->getSeqNum(),
                                             iCurrentMediaMsg->getTimestamp(),
                                             iSinkFormatString.get_str(),
                                             IncomingMsgQueueSize()));
                }
            }
            if (oProcessIncomingMessage == true)
            {
                SendData();
            }
            else
            {
                //implies that we cannot send out any more data
                //no point continuing, just return
                //Do not reschedule here. We need to wait for
                //oProcessIncomingMessage to be true again
                return;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
void PVMediaOutputNodePort::HandlePortActivity(const PVMFPortActivity& aActivity)
//from PVMFPortActivityHandler.  this port handles its own activity.
{
    switch (aActivity.iType)
    {
        case PVMF_PORT_ACTIVITY_CREATED:
            iNode->ReportInfoEvent(PVMFInfoPortCreated, (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_DELETED:
            iNode->ReportInfoEvent(PVMFInfoPortDeleted, (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_CONNECT:
            iNode->ReportInfoEvent(PVMFInfoPortConnected, (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_DISCONNECT:
            iNode->ReportInfoEvent(PVMFInfoPortDisconnected, (OsclAny*)aActivity.iPort);
            break;

        case PVMF_PORT_ACTIVITY_INCOMING_MSG:
            if (IncomingMsgQueueSize() > 0)
            {
                PVMFSharedMediaMsgPtr peekMsgPtr;
                bool oBos = false;
                if (peekHead(peekMsgPtr, oBos))
                {
                    /*
                     * Media Output node inport does not process any more incoming messages
                     * under following circumstances:
                     * 1) Playback clock is NOT in RUNNING state
                     * 2) Media output comp is in a STARTED state
                     * 3) When waiting on media output comp to go ready from a busy state
                     * Boolean "oProcessIncomingMessage" will be false if any of the above
                     * conditions is true.
                     * However BOS is an exception to all of these. BOS is not sent to media output comp,
                     * nor should we worry about playback clock state or media output comp state while
                     * processing BOS. When we get BOS we always send a notification to mediaoutput node
                     * via the BOSObserver with BOS stream-id.
                     */
                    if (oBos)
                    {
                        if (DequeueIncomingMsg(iCurrentMediaMsg) == PVMFSuccess)
                        {
                            uint32 msgStreamId = peekMsgPtr->getStreamID();
                            iBOSStreamIDVec.push_back(msgStreamId);

                            PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::HPA: BOS Recvd - Fmt=%s, TS=%d, StreamID=%d, Qs=%d",
                                                  iSinkFormatString.get_str(),
                                                  peekMsgPtr->getTimestamp(),
                                                  msgStreamId,
                                                  IncomingMsgQueueSize()));

                            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA: BOS Recvd - Fmt=%s, TS=%d, StreamID=%d, Qs=%d",
                                                     iSinkFormatString.get_str(),
                                                     peekMsgPtr->getTimestamp(),
                                                     msgStreamId,
                                                     IncomingMsgQueueSize()));

                            iNode->ReportBOS();

                            iCurrentMediaMsg.Unbind();
                            iFragIndex = 0;
                        }
                        else
                        {
                            PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::HPA: DequeueIncomingMsg Failed - Fmt=%s",
                                                  iSinkFormatString.get_str()));
                            OSCL_ASSERT(false);
                        }
                    }
                    else if (DataToSkip(peekMsgPtr) == true)
                    {
                        //this condition implies that a skip timestamp has been set
                        //and we are looking for a mediamsg whole timestamp is equal or greater than
                        //iSkipTimestamp. All media msgs whose timestamps are less than iSkipTimestamp
                        //will be dropped. There is no need for any additional checks. Just dequeue and toss
                        iCurrentMediaMsg.Unbind();
                        iFragIndex = 0;
                        if (DequeueIncomingMsg(iCurrentMediaMsg) == PVMFSuccess)
                        {
                            if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
                            {
                                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::HPA: EOSSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                                      iCurrentMediaMsg->getStreamID(),
                                                      iCurrentMediaMsg->getSeqNum(),
                                                      iCurrentMediaMsg->getTimestamp(),
                                                      iSinkFormatString.get_str()));

                                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA: EOSSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                                         iCurrentMediaMsg->getStreamID(),
                                                         iCurrentMediaMsg->getSeqNum(),
                                                         iCurrentMediaMsg->getTimestamp(),
                                                         iSinkFormatString.get_str()));
                            }
                            else
                            {
                                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::HPA: MsgSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                                      iCurrentMediaMsg->getStreamID(),
                                                      iCurrentMediaMsg->getSeqNum(),
                                                      iCurrentMediaMsg->getTimestamp(),
                                                      iSinkFormatString.get_str(),
                                                      IncomingMsgQueueSize()));

                                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA: MsgSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                                         iCurrentMediaMsg->getStreamID(),
                                                         iCurrentMediaMsg->getSeqNum(),
                                                         iCurrentMediaMsg->getTimestamp(),
                                                         iSinkFormatString.get_str(),
                                                         IncomingMsgQueueSize()));
                            }
                            iCurrentMediaMsg.Unbind();
                            iFragIndex = 0;
                        }
                        else
                        {
                            PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::HPA: DequeueIncomingMsg Failed - Fmt=%s",
                                                  iSinkFormatString.get_str()));
                            OSCL_ASSERT(false);
                        }
                    }
                    else
                    {
                        if (iSendStartOfDataEvent == true)
                        {
                            //implies that we are attempting to process the first media msg during
                            //after skip, a mediamsg whose timestamp is equal to or greater than
                            //iSkipTimestamp
                            PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::HPA: PVMFInfoStartOfData - Fmt=%s",
                                                  iSinkFormatString.get_str()));
                            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA: PVMFInfoStartOfData - Fmt=%s",
                                                     iSinkFormatString.get_str()));
                            uint32 iStreamID = iRecentStreamID;
                            iNode->ReportInfoEvent(PVMFInfoStartOfData, (OsclAny*)&iStreamID);

                            iSendStartOfDataEvent = false;
                        }
                        if (oProcessIncomingMessage)
                        {
                            //we are starting to process a new media msg
                            if ((iCurrentMediaMsg.GetRep() != NULL) ||
                                    (iFragIndex != 0))
                            {
                                PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::HPA: DequeueIncomingMsg oProcessIncomingMessage Invalid - Fmt=%s",
                                                      iSinkFormatString.get_str()));
                                OSCL_ASSERT(false);
                            }
                            if (DequeueIncomingMsg(iCurrentMediaMsg) == PVMFSuccess)
                            {
                                if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
                                {
                                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA - EOS Recvd - StreamID=%d, Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                                             iCurrentMediaMsg->getStreamID(),
                                                             iCurrentMediaMsg->getSeqNum(),
                                                             iCurrentMediaMsg->getTimestamp(),
                                                             iSinkFormatString.get_str(),
                                                             IncomingMsgQueueSize()));
                                }
                                else if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_RE_CONFIG_FORMAT_ID)
                                {
                                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA - RECONFIG Recvd - Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                                             iCurrentMediaMsg->getSeqNum(),
                                                             iCurrentMediaMsg->getTimestamp(),
                                                             iSinkFormatString.get_str(),
                                                             IncomingMsgQueueSize()));
                                }
                                else if (iCurrentMediaMsg->getFormatID() < PVMF_MEDIA_CMD_FORMAT_IDS_START)
                                {
                                    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::HPA - MediaMsg Recvd - Seq=%d, TS=%d, Fmt=%s, Qs=%d",
                                                             iCurrentMediaMsg->getSeqNum(),
                                                             iCurrentMediaMsg->getTimestamp(),
                                                             iSinkFormatString.get_str(),
                                                             IncomingMsgQueueSize()));
                                }

                                SendData();
                                if ((oProcessIncomingMessage == true) &&
                                        (IncomingMsgQueueSize() > 0))
                                {
                                    //implies that we can process more data, so reschedule
                                    //we could process all the messages in the incoming msg q
                                    //in a while loop here, but that might not be advisable
                                    //since this call is sort of an observer callback and
                                    //it might not be good idea to process more than one msg
                                    RunIfNotReady();
                                }
                            }
                            else
                            {
                                PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::HPA: DequeueIncomingMsg Failed - Fmt=%s",
                                                      iSinkFormatString.get_str()));
                                OSCL_ASSERT(false);
                            }
                        }
                    }
                }
            }
            break;

        default:
            break;
    }
}


////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePort::ConfigMIO(PvmiKvp* aParameters, PvmiKvp* &aRetParameters)
//send config info to media I/O component.
{

    if (iMediaTransfer == NULL)
    {
        iMediaTransfer = iNode->iMIOControl->createMediaTransfer(iNode->iMIOSession);
        if (iMediaTransfer)
        {
            iMediaTransfer->setPeer(this);
        }
        else
        {
            return PVMFFailure;
        }
    }

    if (0 == aParameters)
    {
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO: No format-specific info for this track."));
        if (iWaitForConfig == true)
        {
            iNode->MioConfigured();
            iWaitForConfig = false;
        }
        return PVMFSuccess;//no format-specific info, so nothing needed.
    }
    uint8* data = (uint8*)aParameters->value.key_specific_value;
    uint32 data_len = aParameters->capacity;

    PVMFStatus status;

    //formatted data gets sent to the MIO on first arrival
    switch (iMediaTypeIndex)
    {
        case PVMF_UNCOMPRESSED_AUDIO_FORMAT:
        {
            channelSampleInfo* pcm16Info = (channelSampleInfo*)data;
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO - SamplingRate=%d, NumChannels=%d",
                                     pcm16Info->samplingRate, pcm16Info->desiredChannels));
            //Send pcm16 info.
            //ignore any failures since not all MIO may suppport the parameters.
            //Note: send the parameters individually since some MIO may not know
            //how to process arrays.
            status = SetMIOParameterUint32(MOUT_AUDIO_SAMPLING_RATE_KEY,
                                           pcm16Info->samplingRate);
            if (status == PVMFSuccess)
            {
                SetMIOParameterUint32(MOUT_AUDIO_NUM_CHANNELS_KEY,
                                      pcm16Info->desiredChannels);
            }
        }
        break;

        case PVMF_UNCOMPRESSED_VIDEO_FORMAT:
        {
            PVMFYuvFormatSpecificInfo0* yuvInfo = (PVMFYuvFormatSpecificInfo0*)data;
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO - Width=%d, Height=%d, DispWidth=%d, DispHeight=%d",
                                     yuvInfo->width, yuvInfo->height, yuvInfo->display_width, yuvInfo->display_height));
            //Send yuv info.
            //ignore any failures since not all MIO may suppport the parameters.
            //Note: send the parameters individually since some MIO may not know
            //how to process arrays.
            status = SetMIOParameterUint32(MOUT_VIDEO_WIDTH_KEY,
                                           yuvInfo->width);
            if (status == PVMFSuccess)
            {
                status = SetMIOParameterUint32(MOUT_VIDEO_HEIGHT_KEY,
                                               yuvInfo->height);
            }
            if (status == PVMFSuccess)
            {
                status = SetMIOParameterUint32(MOUT_VIDEO_DISPLAY_WIDTH_KEY,
                                               yuvInfo->display_width);
            }
            if (status == PVMFSuccess)
            {
                status = SetMIOParameterUint32(MOUT_VIDEO_DISPLAY_HEIGHT_KEY,
                                               yuvInfo->display_height);
            }
            if (status == PVMFSuccess)
            {
                // ignore status here
                SetMIOParameterUint32(MOUT_VIDEO_SUBFORMAT_KEY,
                                      yuvInfo->video_format);
            }
        }
        break;

        case PVMF_COMPRESSED_AUDIO_FORMAT:
        case PVMF_COMPRESSED_VIDEO_FORMAT:
            //for compressed formats, the format-specific info is sent as kvp
        {
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO Setting Codec Header - Len=%d, Fmt=%s",
                                     data_len, iSinkFormatString.get_str()));

            // We will enable the following line after source node fix KVP issue
            //oscl_assert(aParameters->length == aParameters->capacity)

            // We will remove the following line after source node fix KVP issue
            aParameters->length = aParameters->capacity;

            int32 err;
            OSCL_TRY(err, iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, aParameters, 1, aRetParameters););

            if (err != OsclErrNone || aRetParameters)
            {
                PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::ConfigMIO setParametersSync of PVMF_FORMAT_SPECIFIC_INFO_KEY failed "));
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO setParametersSync of PVMF_FORMAT_SPECIFIC_INFO_KEY failed "));
                //this is the first call, we don't handle a leave here-- it's an error
                iNode->ReportErrorEvent(PVMFErrPortProcessing, NULL, PVMFMoutNodeErr_MediaIOSetParameterSync);
                return PVMFFailure;
            }
            status = PVMFSuccess;

        }
        break;

        case PVMF_TEXT_FORMAT:
            //for text formats, the format-specific info is sent as kvp
        {
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO Setting Codec Header - Len=%d, Fmt=%s",
                                     data_len, iSinkFormatString.get_str()));

            // We will enable the following line after source node fix KVP issue
            //oscl_assert(aParameters->length == aParameters->capacity)

            // We will remove the following line after source node fix KVP issue
            aParameters->length = aParameters->capacity;

            int32 err;
            OSCL_TRY(err, iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, aParameters, 1, aRetParameters););

            if (err != OsclErrNone || aRetParameters)
            {
                PVMF_MOPORT_LOGERROR((0, "PVMediaOutputNodePort::ConfigMIO setParametersSync of PVMF_FORMAT_SPECIFIC_INFO_KEY failed "));
                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ConfigMIO setParametersSync of PVMF_FORMAT_SPECIFIC_INFO_KEY failed "));
                //this is the first call, we don't handle a leave here-- it's an error
                iNode->ReportErrorEvent(PVMFErrPortProcessing, NULL, PVMFMoutNodeErr_MediaIOSetParameterSync);
                return PVMFFailure;
            }
            status = PVMFSuccess;
        }
        break;

        default:
            status = PVMFErrNotSupported;
            iNode->Assert(false);
            break;
    }
    if (status != PVMFSuccess)
    {
        iNode->ReportErrorEvent(PVMFErrResource, NULL, PVMFMoutNodeErr_MediaIOSetParameterSync);
    }
    else
    {
        if (iWaitForConfig == true)
        {
            iNode->MioConfigured();
            iWaitForConfig = false;
        }
    }
    return status;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePort::SetMIOParameterInt32(PvmiKeyType aKey, int32 aValue)
//to set parameters to the MIO component through its config interface.
{
    OsclMemAllocator alloc;
    PvmiKvp kvp;
    PvmiKvp* retKvp = NULL; // for return value

    kvp.key = NULL;
    kvp.length = oscl_strlen(aKey) + 1; // +1 for \0
    kvp.capacity = kvp.length;

    kvp.key = (PvmiKeyType)alloc.ALLOCATE(kvp.length);
    if (!kvp.key)
        return PVMFErrNoMemory;

    oscl_strncpy(kvp.key, aKey, kvp.length);
    kvp.value.int32_value = aValue;

    int32 err;
    OSCL_TRY(err, iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, &kvp, 1, retKvp););
    alloc.deallocate(kvp.key);

    if (err != OsclErrNone || retKvp)
        return PVMFFailure;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePort::SetMIOParameterUint32(PvmiKeyType aKey, uint32 aValue)
//to set parameters to the MIO component through its config interface.
{
    OsclMemAllocator alloc;
    PvmiKvp kvp;
    PvmiKvp* retKvp = NULL; // for return value

    kvp.key = NULL;
    kvp.length = oscl_strlen(aKey) + 1; // +1 for \0
    kvp.capacity = kvp.length;

    kvp.key = (PvmiKeyType)alloc.ALLOCATE(kvp.length);
    if (!kvp.key)
        return PVMFErrNoMemory;

    oscl_strncpy(kvp.key, aKey, kvp.length);
    kvp.value.uint32_value = aValue;

    int32 err;
    OSCL_TRY(err, iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, &kvp, 1, retKvp););
    alloc.deallocate(kvp.key);

    if (err != OsclErrNone || retKvp)
        return PVMFFailure;

    return PVMFSuccess;
}


////////////////////////////////////////////////////////////////////////////
PVMFStatus PVMediaOutputNodePort::SetMIOParameterPchar(PvmiKeyType aKey, char* aValue)
//to set parameters to the MIO component through its config interface.
{
    OsclMemAllocator alloc;
    PvmiKvp kvp;
    PvmiKvp* retKvp = NULL; // for return value

    kvp.key = NULL;
    kvp.length = oscl_strlen(aKey) + 1; // +1 for \0
    kvp.capacity = kvp.length;

    kvp.key = (PvmiKeyType)alloc.ALLOCATE(kvp.length);
    if (!kvp.key)
        return PVMFErrNoMemory;

    oscl_strncpy(kvp.key, aKey, kvp.length);
    kvp.value.pChar_value = aValue;

    int32 err;
    OSCL_TRY(err, iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, &kvp, 1, retKvp););
    alloc.deallocate(kvp.key);

    if (err != OsclErrNone || retKvp)
        return PVMFFailure;

    return PVMFSuccess;
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
        PvmiKvp*& aParameters, int& num_parameter_elements,
        PvmiCapabilityContext aContext)
{
    PVMFStatus status = iNode->iMIOConfig->getParametersSync(aSession, aIdentifier, aParameters, num_parameter_elements,
                        aContext);
    return status;
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    if (aParameters && iNode)
    {
        return iNode->iMIOConfig->releaseParameters(aSession, aParameters, num_elements);
    }
    return PVMFFailure;
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PVMediaOutputNodePort::setParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements,
        PvmiKvp * &aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);
    for (int32 i = 0; i < num_elements; i++)
    {
        if (pv_mime_strcmp(aParameters[i].key, PVMF_FORMAT_SPECIFIC_INFO_KEY) == 0)
        {
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::setParametersSync - FSI - Fmt=%s",
                                     iSinkFormatString.get_str()));
            ConfigMIO(&aParameters[i], aRet_kvp);
        }
        else if ((pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_SAMPLING_RATE_KEY) == 0) ||
                 (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_NUM_CHANNELS_KEY) == 0) ||
                 (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_WIDTH_KEY) == 0) ||
                 (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_HEIGHT_KEY) == 0) ||
                 (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_DISPLAY_WIDTH_KEY) == 0) ||
                 (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_DISPLAY_HEIGHT_KEY) == 0))
        {
            PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::setParametersSync - FSI - Fmt=%s",
                                     iSinkFormatString.get_str()));
            //Null parameters, because we just notify that MIO has been configured.
            ConfigMIO(0, aRet_kvp);
            iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, &aParameters[i], 1, aRet_kvp);
        }
        else
            iNode->iMIOConfig->setParametersSync(iNode->iMIOSession, &aParameters[i], 1, aRet_kvp);
    }
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PVMediaOutputNodePort::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    OSCL_UNUSED_ARG(aSession);
    return (iNode->iMIOConfig->verifyParametersSync(iNode->iMIOSession, aParameters, num_elements));
}


////////////////////////////////////////////////////////////////////////////
bool PVMediaOutputNodePort::peekHead(PVMFSharedMediaMsgPtr& aMsgPtr,
                                     bool& bBos)
{
    if (iIncomingQueue.iQ.empty())
    {
        return false;
    }
    // get a pointer to the queue head
    aMsgPtr = iIncomingQueue.iQ.front();
    // check the format id first - treat BOS special
    if (aMsgPtr->getFormatID() == PVMF_MEDIA_CMD_BOS_FORMAT_ID)
    {
        bBos = true;
    }
    else
    {
        bBos = false;
    }

    // This check is needed to handle cases where the media output node is waiting on configuration,
    // but for some reason the upstream node does not have a config to send.
    // (Think of cases where we have empty tracks in a presentation). In such cases, if we get EOS we want
    // to notify the media output node that configuration is complete, so that media output node can complete its pending start
    if (aMsgPtr->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
    {
        if (iWaitForConfig == true)
        {
            iNode->MioConfigured();
            iWaitForConfig = false;
        }
    }
    return true;
}

void PVMediaOutputNodePort::PVMediaOutputNodePortTimerEvent()
{
    //timer expires, reset the boolean so that we can start processing more data if need be
    oProcessIncomingMessage = true;
    if (iCurrentMediaMsg.GetRep() != NULL)
    {
        //attempt to send data current media msg if any
        SendData();
    }
    //reschedule if there is more stuff waiting and
    //if we can process more data
    if ((oProcessIncomingMessage == true) &&
            (IncomingMsgQueueSize() > 0))
    {
        RunIfNotReady();
    }
}

void PVMediaOutputNodePort::SetSkipTimeStamp(uint32 aSkipTS,
        uint32 aStreamID)
{
    PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::SetSkipTimeStamp: TS=%d, Fmt=%s",
                          aSkipTS,
                          iSinkFormatString.get_str()));
    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SetSkipTimeStamp: TS=%d, Fmt=%s",
                             aSkipTS,
                             iSinkFormatString.get_str()));
    iSkipTimestamp = aSkipTS;
    iRecentStreamID = aStreamID;
    iSendStartOfDataEvent = true;
    //cancel any outstanding delay timer
    if (iDelayTimer != NULL)
    {
        iDelayTimer->Stop();
    }
    //release the current media msg right here instead of
    //waiting on a reschedule. this is to avoid deadlocks
    //in case the upstream node is just operating with a
    //single media msg
    if (iCurrentMediaMsg.GetRep() != NULL)
    {
        if (DataToSkip(iCurrentMediaMsg) == true)
        {
            if (iCurrentMediaMsg->getFormatID() == PVMF_MEDIA_CMD_EOS_FORMAT_ID)
            {
                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::SetSkipTimeStamp: EOSSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                      iCurrentMediaMsg->getStreamID(),
                                      iCurrentMediaMsg->getSeqNum(),
                                      iCurrentMediaMsg->getTimestamp(),
                                      iSinkFormatString.get_str()));

                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SetSkipTimeStamp: EOSSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                         iCurrentMediaMsg->getStreamID(),
                                         iCurrentMediaMsg->getSeqNum(),
                                         iCurrentMediaMsg->getTimestamp(),
                                         iSinkFormatString.get_str()));
            }
            else
            {
                PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::SetSkipTimeStamp: MsgSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                      iCurrentMediaMsg->getStreamID(),
                                      iCurrentMediaMsg->getSeqNum(),
                                      iCurrentMediaMsg->getTimestamp(),
                                      iSinkFormatString.get_str()));

                PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::SetSkipTimeStamp: MsgSkip - StreamId=%d, Seq=%d, TS=%d, Fmt=%s",
                                         iCurrentMediaMsg->getStreamID(),
                                         iCurrentMediaMsg->getSeqNum(),
                                         iCurrentMediaMsg->getTimestamp(),
                                         iSinkFormatString.get_str()));
            }
            iCurrentMediaMsg.Unbind();
            iFragIndex = 0;
        }
    }
    //wake up the AO to start processing messages
    RunIfNotReady();
}

void PVMediaOutputNodePort::CancelSkip()
{
    PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::CancelSkip: Fmt=%s",
                          iSinkFormatString.get_str()));
    PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::CancelSkip: Fmt=%s",
                             iSinkFormatString.get_str()));
    iSkipTimestamp = 0;
    iRecentStreamID = 0;
    iSendStartOfDataEvent = false;
}

bool PVMediaOutputNodePort::DataToSkip(PVMFSharedMediaMsgPtr& aMsg)
{
    //iRecentStreamID reflects the latest stream
    //that is being played. Msg with a streamid less than
    //iRecentStreamID belongs to an older stream and should
    //be skipped. Do not skip Msg that belong to the current
    //stream or a future stream (future being the case where
    //msg arrives before a skip request. Pls note that we
    //assume that the stream ids are a montonically increasing
    //sequence
    uint32 delta = 0;
    bool oOldStream = IsEarlier(aMsg->getStreamID(), iRecentStreamID, delta);
    if (oOldStream && delta > 0)
    {
        //a zero delta could mean the stream ids are equal
        return true;
    }
    //we typically do not do timestamp checks on EOS.
    //old EOSes that needed to be discarded during a repositioning
    //would not pass the stream id check above. If we get this far
    //it means that the EOS belong to the current stream id and its
    //timestamp does not really matter.
    if (aMsg->getFormatID() != PVMF_MEDIA_CMD_EOS_FORMAT_ID)
    {
        //if we get here it means that the msg belong to current or a
        //future media stream. check against skip timestamp if we are
        //required to send PVMFInfoStartOfData. If we are not reqd to
        //send the event, we intentionally by pass any timestamp checks
        if (iSendStartOfDataEvent == true)
        {
            delta = 0;
            bool tsEarly = IsEarlier(aMsg->getTimestamp(), iSkipTimestamp, delta);
            if (tsEarly && delta > 0)
            {
                //a zero delta could mean the timestamps are equal
                return true;
            }
        }
    }
    return false;
}

void PVMediaOutputNodePort::ClockTimebaseUpdated()
{
    if (iClock == NULL)
        return;

    if (iClock->GetCountTimebase())
    {
        PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::ClockTimebaseUpdated: CountTimeBase Added - Fmt=%s",
                              iSinkFormatString.get_str()));
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ClockTimebaseUpdated: CountTimeBase Added - Fmt=%s",
                                 iSinkFormatString.get_str()));
        //Reset the frame step delta to zero.
        iFrameStepMode = true;
        iClock->GetCountTimebase()->GetCount(iClockFrameCount);
        iSyncFrameCount = iClockFrameCount;
    }
    else
    {
        PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::ClockTimebaseUpdated: CountTimeBase Removed - Fmt=%s",
                              iSinkFormatString.get_str()));
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ClockTimebaseUpdated: CountTimeBase Removed - Fmt=%s",
                                 iSinkFormatString.get_str()));
        //reset frame step variables.
        iFrameStepMode = false;
        iSyncFrameCount = 0;
        iClockFrameCount = 0;
    }
    if (iDelayTimer != NULL)
    {
        iDelayTimer->Stop();
    }
    oProcessIncomingMessage = true;
    if (iCurrentMediaMsg.GetRep() != NULL)
    {
        //attempt to send data current media msg if any
        SendData();
    }
    //reschedule if there is more stuff waiting and
    //if we can process more data
    if ((oProcessIncomingMessage == true) &&
            (IncomingMsgQueueSize() > 0))
    {
        RunIfNotReady();
    }
}

void PVMediaOutputNodePort::ClockCountUpdated()
{
    if (iClock && iClock->GetCountTimebase())
    {
        //read the new framecount
        iClock->GetCountTimebase()->GetCount(iClockFrameCount);
        //wake up the AO to process data
        oProcessIncomingMessage = true;
        if (iCurrentMediaMsg.GetRep() != NULL)
        {
            //attempt to send data current media msg if any
            SendData();
        }
        //reschedule if there is more stuff waiting and
        //if we can process more data
        if ((oProcessIncomingMessage == true) &&
                (IncomingMsgQueueSize() > 0))
        {
            RunIfNotReady();
        }
    }
}

void PVMediaOutputNodePort::ClockAdjusted()
{
}

void PVMediaOutputNodePort::ClockStateUpdated()
{
    if (iClock == NULL)
        return;

    if (iClock->GetState() == OsclClock::PAUSED)
    {
        PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::ClockStateUpdated: Clock Paused - Fmt=%s",
                              iSinkFormatString.get_str()));
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ClockStateUpdated: Clock Paused - Fmt=%s",
                                 iSinkFormatString.get_str()));
        //stop processing input msgs only for passive mediaoutput comps
        //for active ones continue to send, since the mediaoutput comp
        //is responsible for pacing of the data
        if (oActiveMediaOutputComp == false)
        {
            oProcessIncomingMessage = false;
            if (iDelayTimer != NULL)
            {
                iDelayTimer->Stop();
            }
        }
    }
    else if (iClock->GetState() == OsclClock::RUNNING)
    {
        PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::ClockStateUpdated: Clock Running - Fmt=%s",
                              iSinkFormatString.get_str()));
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ClockStateUpdated: Clock Running - Fmt=%s",
                                 iSinkFormatString.get_str()));
        //start processing input msgs
        oProcessIncomingMessage = true;
        //reset write state as well, in case mo comp is still busy
        //it will leave again, so that state will get reset
        iWriteState = EWriteOK;
        if (iCurrentMediaMsg.GetRep() != NULL)
        {
            //attempt to send data current media msg if any
            SendData();
        }
        //reschedule if there is more stuff waiting and
        //if we can process more data
        if ((oProcessIncomingMessage == true) &&
                (IncomingMsgQueueSize() > 0))
        {
            RunIfNotReady();
        }
    }
    else if (iClock->GetState() == OsclClock::STOPPED)
    {
        PVMF_MOPORT_LOGREPOS((0, "PVMediaOutputNodePort::ClockStateUpdated: Clock Stopped - Fmt=%s",
                              iSinkFormatString.get_str()));
        PVMF_MOPORT_LOGDATAPATH((0, "PVMediaOutputNodePort::ClockStateUpdated: Clock Stopped - Fmt=%s",
                                 iSinkFormatString.get_str()));
        //stop processing input msgs only for passive mediaoutput comps
        //for active ones continue to send, since the mediaoutput comp
        //is responsible for pacing of the data, this is to account for a case when clock is stopped
        //after repositioning.
        if (oActiveMediaOutputComp == false)
        {
            oProcessIncomingMessage = false;
            if (iDelayTimer != NULL)
            {
                iDelayTimer->Stop();
            }
        }
    }
    RunIfNotReady();
}

void PVMediaOutputNodePort::ClearPreviousBOSStreamIDs(uint32 aID)
{
    //Pls note that we
    //assume that the stream ids are a montonically increasing
    //sequence
    Oscl_Vector<uint32, OsclMemAllocator>::iterator it;
    it = iBOSStreamIDVec.begin();
    while (it != iBOSStreamIDVec.end())
    {
        // iBOSStreamIDVec will contain the stream ID of the current stream being Played. On each skip call
        // all the previous streamid's will be removed from the vector leaving the current stream id. The
        // current stream id will be erased in the next skip media data call.
        if (*it < aID)
        {
            it = iBOSStreamIDVec.erase(it);
        }
        else
        {
            it++;
        }
    }
}











