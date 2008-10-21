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
#include "pvmf_protocol_engine_node.h"
#include "pvmf_protocol_engine_node_progressive_streaming.h"
#include "pvmf_protocol_engine_progressive_download.h"

#include "pvlogger.h"

/**
//Macros for calling PVLogger
*/
#define LOGERROR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);
#define LOGINFOHI(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG,iLogger,PVLOGMSG_INFO,m);
#define LOGINFOMED(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG,iLogger,PVLOGMSG_INFO,m);
#define LOGINFOLOW(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iLogger,PVLOGMSG_INFO,m);
#define LOGINFO(m) LOGINFOMED(m)
#define LOGINFODATAPATH(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iDataPathLogger,PVLOGMSG_INFO,m);
#define LOGERRORDATAPATH(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iDataPathLogger,PVLOGMSG_ERR,m);
#define LOGINFOCLOCK(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iClockLogger,PVLOGMSG_INFO,m);
#define PVMF_PROTOCOL_ENGINE_LOGBIN(iPortLogger, m) PVLOGGER_LOGBIN(PVLOGMSG_INST_LLDBG, iPortLogger, PVLOGMSG_ERR, m);
#define	NODEDATAPATHLOGGER_TAG "datapath.sourcenode.protocolenginenode"

////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveStreamingContainerFactory implementation
////////////////////////////////////////////////////////////////////////////////////
ProtocolContainer* ProgressiveStreamingContainerFactory::create(PVMFProtocolEngineNode *aNode)
{
    return OSCL_NEW(ProgressiveStreamingContainer, (aNode));
}

////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveStreamingContainer implementation
////////////////////////////////////////////////////////////////////////////////////
ProgressiveStreamingContainer::ProgressiveStreamingContainer(PVMFProtocolEngineNode *aNode) :
        ProgressiveDownloadContainer(aNode),
        iEnableInfoUpdate(true)
{
    ;
}

bool ProgressiveStreamingContainer::createProtocolObjects()
{
    iNode->iProtocol		 = OSCL_NEW(ProgressiveStreaming, ());
    iNode->iNodeOutput		 = OSCL_NEW(pvProgressiveStreamingOutput, (iNode));
    iNode->iDownloadControl  = OSCL_NEW(progressiveStreamingControl, ());
    iNode->iDownloadProgess  = OSCL_NEW(ProgressiveStreamingProgress, ());
    iNode->iEventReport		 = OSCL_NEW(progressiveStreamingEventReporter, (iNode));
    iNode->iCfgFileContainer = OSCL_NEW(PVProgressiveStreamingCfgFileContainer, (iNode->iDownloadSource));
    iNode->iUserAgentField	 = OSCL_NEW(UserAgentFieldForProgDownload, ());
    iNode->iDownloadSource	 = OSCL_NEW(PVMFDownloadDataSourceContainer, ());

    if (!iNode->iProtocol		|| !iNode->iNodeOutput  || !iNode->iDownloadControl  ||
            !iNode->iDownloadProgess || !iNode->iEventReport || !iNode->iCfgFileContainer ||
            !iNode->iUserAgentField  || !iNode->iDownloadSource) return false;

    if (iNode->iNodeOutput)
    {
        iNode->iNodeOutput->setDataStreamSourceRequestObserver((PvmiDataStreamRequestObserver*)iNode);
    }

    return ProtocolContainer::createProtocolObjects();
}

PVMFStatus ProgressiveStreamingContainer::doStop()
{
    // For progressive streaming, tell the data stream to flush,
    // so that the socket buffer can be returned to socket node for reset
    iNode->iNodeOutput->flushDataStream();
    return PVMFSuccess;
}

PVMFStatus ProgressiveStreamingContainer::doSeek(PVMFProtocolEngineNodeCommand& aCmd)
{
    uint32 newOffset = getSeekOffset(aCmd);

    LOGINFODATAPATH((0, "PVMFProtocolEngineNode::DoReposition()->ProgressiveStreamingContainer::DoSeek : reposition offset=%d, iInterfaceState=%d",
                     newOffset, (int32)iNode->iInterfaceState));

    return doSeekBody(newOffset);
}

uint32 ProgressiveStreamingContainer::getSeekOffset(PVMFProtocolEngineNodeCommand& aCmd)
{
    //extract the parameters.
    OsclAny* aRequestData;
    aCmd.PVMFProtocolEngineNodeCommand::Parse(aRequestData);
    uint32 newOffset = (uint32)aRequestData;
    return newOffset;
}

PVMFStatus ProgressiveStreamingContainer::doSeekBody(uint32 aNewOffset)
{
    // reset streaming done and session done flag to restart streaming
    ProtocolStateCompleteInfo aInfo;
    iNode->iInterfacingObjectContainer.setProtocolStateCompleteInfo(aInfo, true);

    // HTTP GET request looks at the current file size to determine is Range header is needed
    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    aCfgFile->SetCurrentFileSize(aNewOffset);

    // Reconnect and send new GET request
    iNode->iProtocol->seek(aNewOffset);
    iNode->StartDataFlowByCommand();

    return PVMFPending;
}

bool ProgressiveStreamingContainer::completeRepositionRequest()
{
    PVMFProtocolEngineNodeCommand *pCmd = iNode->FindCmd(iNode->iCurrentCommand, PVPROTOCOLENGINE_NODE_CMD_DATASTREAM_REQUEST_REPOSITION);
    if (pCmd == NULL) return false;

    OsclAny* aRequestData;
    PvmiDataStreamCommandId aDataStreamCmdId;
    pCmd->PVMFProtocolEngineNodeCommand::Parse(aRequestData, aDataStreamCmdId);

    // set current file offset to the byte range request offset
    uint32 newOffset = (uint32)(aRequestData);
    iNode->iNodeOutput->seekDataStream(newOffset);
    iNode->iNodeOutput->setCurrentOutputSize(newOffset);
    iNode->iDownloadControl->setPrevDownloadSize(newOffset);

    // reset initial buffering algo variables
    iNode->iDownloadControl->clearPerRequest();

    // Form a command response
    PVMFCmdResp resp(aDataStreamCmdId, pCmd->iContext, PVMFSuccess, NULL, NULL);
    // Make the Command Complete notification
    iNode->iNodeOutput->dataStreamCommandCompleted(resp);
    iNode->iCurrentCommand.Erase(pCmd);
    return true;
}

void ProgressiveStreamingContainer::updateDownloadControl(const bool isDownloadComplete)
{
    bool downloadComplete = isDownloadComplete;
    if (downloadComplete && iNode->IsRepositioningRequestPending())
    {
        // if there is a repositioning request pending for progressive streaming,
        // do not send resume notification due to download complete
        downloadComplete = false;
    }

    // check resume notification
    if (iNode->iDownloadControl->checkResumeNotification(downloadComplete) == 1)
    {
        LOGINFODATAPATH((0, "ProgressiveStreamingContainer::updateDownloadControl, send data ready event to parser node, downloadComplete=false"));

        // report data ready event
        iNode->iEventReport->sendDataReadyEvent();
    }

    // update download progress
    iNode->iDownloadProgess->update(isDownloadComplete);
}

void ProgressiveStreamingContainer::checkSendResumeNotification()
{
    // check the special case to trigger node running to send back resume notification to parser node if there is
    if (iNode->iNodeOutput->getAvailableOutputSize() == 0 && iEnableInfoUpdate)
    {
        //!iNode->IsRepositioningRequestPending()) {
        // form PVProtocolEngineNodeInternalEventType_CheckResumeNotificationMaually event
        PVProtocolEngineNodeInternalEvent aEvent(PVProtocolEngineNodeInternalEventType_CheckResumeNotificationMaually);
        iNode->iInternalEventQueue.clear();
        iNode->iInternalEventQueue.push_back(aEvent);
        iNode->SetProcessingState(ProcessingState_NormalDataflow);
        iNode->RunIfNotReady();
    }
}

bool ProgressiveStreamingContainer::doInfoUpdate(const uint32 downloadStatus)
{
    // For pending reposition request, don't do auto-resume checking
    //if(iNode->IsRepositioningRequestPending()) return true;
    if (!iEnableInfoUpdate) return true;

    return DownloadContainer::doInfoUpdate(downloadStatus);
}

////////////////////////////////////////////////////////////////////////////////////
//////	pvProgressiveStreamingOutput implementation
///////////////////////////////////////////////////////////////////////////////////
pvProgressiveStreamingOutput::pvProgressiveStreamingOutput(PVMFProtocolEngineNodeOutputObserver *aObserver) :
        pvHttpDownloadOutput(aObserver),
        iSourceRequestObserver(NULL)
{
    ;
}

int32 pvProgressiveStreamingOutput::openDataStream(OsclAny* aInitInfo)
{
    int32 status = pvHttpDownloadOutput::openDataStream(aInitInfo);
    if (status == PVMFSuccess && isOpenDataStream)
    {
        // protocol engine node is the observer
        PvmiDataStreamStatus dsStatus = iDataStream->SetSourceRequestObserver(*iSourceRequestObserver);
        if ((dsStatus != PVDS_NOT_SUPPORTED) && (dsStatus != PVDS_SUCCESS)) return PROCESS_DATA_STREAM_OPEN_FAILURE;
    }
    return status;
}

int32 pvProgressiveStreamingOutput::flushData(const uint32 aOutputType)
{
    int32 status = PVMFProtocolEngineNodeOutput::flushData(aOutputType);
    if (status != PROCESS_SUCCESS) return status;

    while (!iOutputFramesQueue.empty())
    {
        if (writeToDataStream(iOutputFramesQueue[0], iPendingOutputDataQueue) == 0xffffffff) return PROCESS_OUTPUT_TO_DATA_STREAM_FAILURE;
        iOutputFramesQueue.erase(iOutputFramesQueue.begin());
    }
    return PROCESS_SUCCESS;
}

uint32 pvProgressiveStreamingOutput::writeToDataStream(OUTPUT_DATA_QUEUE &aOutputQueue, PENDING_OUTPUT_DATA_QUEUE &aPendingOutputQueue)
{
    uint32 totalFragSize = 0;

    // Memory Buffer Data Stream takes memory fragments
    // Go through the queue, remove the frags, write them to the data stream
    // If the data stream is holding onto the frags, add the frags to a different queue, to be deleted later
    // Otherwise the frags are deleted in here
    while (!aOutputQueue.empty())
    {
        OsclRefCounterMemFrag frag = aOutputQueue.front();
        // make a copy otherwise erase will destroy it

        OsclRefCounterMemFrag* copyFrag = OSCL_NEW(OsclRefCounterMemFrag, (frag));

        uint32 fragSize = 0;
        PvmiDataStreamStatus status = iDataStream->Write(iSessionID, copyFrag, fragSize);
        if (PVDS_PENDING == status)
        {
            // This buffer is now part of the data stream cache
            // and cannot be freed until it is returned later on
            // Move the mem frag to the pending queue
            aPendingOutputQueue.push_back(copyFrag);
        }
        else
        {
            // Done with this frag
            // free the reference
            OSCL_DELETE(copyFrag);
        }

        // Remove from output queue
        aOutputQueue.erase(aOutputQueue.begin());

        if ((PVDS_SUCCESS != status) && (PVDS_PENDING != status))
        {
            // An error has occurred
            return ~0;
        }

        totalFragSize += fragSize;
    }

    LOGINFODATAPATH((0, "pvProgressiveStreamingOutput::writeToDataStream() SIZE= %d , SEQNUM=%d", totalFragSize, iCounter++));
    iCurrTotalOutputSize += totalFragSize;
    return totalFragSize;
}

bool pvProgressiveStreamingOutput::releaseMemFrag(OsclRefCounterMemFrag* aFrag)
{
    bool bFound = false;
    LOGINFODATAPATH((0, "pvProgressiveStreamingOutput::releaseMemFrag(), frag=%x", aFrag->getMemFragPtr()));
    for (uint32 i = 0; i < iPendingOutputDataQueue.size(); i++)
    {
        // Find the frag in the queue and remove it
        OsclRefCounterMemFrag* frag = iPendingOutputDataQueue[i];
        if (aFrag->getMemFragPtr() == frag->getMemFragPtr())
        {
            LOGINFODATAPATH((0, "pvProgressiveStreamingOutput::releaseMemFrag(), found frag %x in pending Q", aFrag->getMemFragPtr()));
            iPendingOutputDataQueue.erase(&iPendingOutputDataQueue[i]);
            OSCL_DELETE(frag);
            bFound = true;
            break;
        }
        // TBD, how do we free the reference
    }
    return bFound;
}

void pvProgressiveStreamingOutput::setContentLength(uint32 aLength)
{
    if (iDataStream) iDataStream->SetContentLength(aLength);
}

void pvProgressiveStreamingOutput::dataStreamCommandCompleted(const PVMFCmdResp& aResponse)
{
    // propagate the command complete
    if (iDataStream) iDataStream->SourceRequestCompleted(aResponse);
}

void pvProgressiveStreamingOutput::flushDataStream()
{
    // tell the data stream to flush all buffered data
    // for MBDS, empty temp cache and release mem buffers
    if (iDataStream) iDataStream->Flush(iSessionID);
}

bool pvProgressiveStreamingOutput::seekDataStream(const uint32 aSeekOffset)
{
    if (!iDataStream) return false;
    return (iDataStream->Seek(iSessionID, aSeekOffset, PVDS_SEEK_SET) == PVDS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////////
//////	progressiveStreamingControl implementation
////////////////////////////////////////////////////////////////////////////////////
progressiveStreamingControl::progressiveStreamingControl() : progressiveDownloadControl()
{
    ;
}

void progressiveStreamingControl::requestResumeNotification(const uint32 currentNPTReadPosition, bool& aDownloadComplete, bool& aNeedSendUnderflowEvent)
{
    LOGINFODATAPATH((0, "progressiveStreamingControl::requestResumeNotification() IN, iPlaybackUnderflow=%d, iRequestResumeNotification=%d, iDownloadComplete=%d, will manually set iDownloadComplete=false",
                     (uint32)iPlaybackUnderflow, (uint32)iRequestResumeNotification, (uint32)iDownloadComplete));

    iDownloadComplete = aDownloadComplete = false;
    pvDownloadControl::requestResumeNotification(currentNPTReadPosition, aDownloadComplete, aNeedSendUnderflowEvent);
}


void progressiveStreamingControl::clearPerRequest()
{
    // for progressive playback
    // after each repositioning (aka new GET request)
    // the following variables must be reset
    // to enable auto pause and resume to function properly
    iDlAlgoPreConditionMet         = false;
    iDownloadComplete              = false;
}

////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveStreamingProgress implementation
////////////////////////////////////////////////////////////////////////////////////
bool ProgressiveStreamingProgress::calculateDownloadPercent(uint32 &aDownloadProgressPercent)
{
    // in progessive streaming, the getContentLength will change after new GET request
    // from known to 0 and then to known again
    uint32 fileSize = iProtocol->getContentLength();
    if (!fileSize && iContentLength)
    {
        fileSize = iContentLength;
    }
    if (fileSize) iContentLength = fileSize;

    return ProgressiveDownloadProgress::calculateDownloadPercentBody(aDownloadProgressPercent, fileSize);
}

