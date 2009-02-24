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
#include "pvmf_protocol_engine_node.h"
#include "pvmf_protocol_engine_node_download_common.h"
#include "pvmf_protocolengine_node_tunables.h"
#include "pvmf_protocol_engine_command_format_ids.h"


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
//////	DownloadContainer implementation
////////////////////////////////////////////////////////////////////////////////////

// constructor
DownloadContainer::DownloadContainer(PVMFProtocolEngineNode *aNode) :
        ProtocolContainer(aNode),
        iForceSocketReconnect(false),
        iNeedCheckResumeNotificationManually(false)
{
    ;
}

void DownloadContainer::deleteProtocolObjects()
{
    if (iNode->iCfgFileContainer)
    {
        iNode->iCfgFileContainer->saveConfig();
        OSCL_DELETE(iNode->iCfgFileContainer);
        iNode->iCfgFileContainer = NULL;
    }

    if (iNode->iDownloadSource) OSCL_DELETE(iNode->iDownloadSource);
    iNode->iDownloadSource = NULL;
    ProtocolContainer::deleteProtocolObjects();
}

int32 DownloadContainer::doPreStart()
{
    // check the case: resume download after download is complete
    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    LOGINFODATAPATH((0, "DownloadContainer::doPreStart(), currFileSizeFromCfgFile=%d, totalFileSizeFromCfgFile=%d, aResumeDownload=%d, rangeStartTime=%d",
                     aCfgFile->GetCurrentFileSize(), aCfgFile->GetOverallFileSize(), (uint32)(!aCfgFile->IsNewSession()), aCfgFile->GetRangeStartTime()));

    if (!aCfgFile->IsNewSession() && aCfgFile->GetCurrentFileSize() >= aCfgFile->GetOverallFileSize())
    {
        iNode->iInterfacingObjectContainer->setFileSize(aCfgFile->GetOverallFileSize());
        iNode->SetState(EPVMFNodeStarted);
        iNode->iNodeTimer->clear();
        iNode->iEventReport->startRealDataflow();
        iNode->iEventReport->checkReportEvent(PROCESS_SUCCESS_END_OF_MESSAGE);
        iNode->iDownloadControl->checkResumeNotification();
        iNode->iInterfacingObjectContainer->setInputDataUnwanted();
        return PROCESS_SUCCESS_END_OF_MESSAGE;
    }
    return PROCESS_SUCCESS;
}

bool DownloadContainer::doPause()
{
    if (iNode->iCfgFileContainer) iNode->iCfgFileContainer->saveConfig();
    return true;
}

PVMFStatus DownloadContainer::doStop()
{
    ProtocolContainer::doStop();

    // set resume download mode for stop and play
    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    aCfgFile->SetNewSession(false); // set resume download session for the next time

    iForceSocketReconnect = true;
    return PVMFSuccess;
}

void DownloadContainer::doClear(const bool aNeedDelete)
{
    // save config
    if (iNode->iCfgFileContainer) iNode->iCfgFileContainer->saveConfig();
    ProtocolContainer::doClear(aNeedDelete);
}

void DownloadContainer::doCancelClear()
{
    // save config
    if (iNode->iCfgFileContainer) iNode->iCfgFileContainer->saveConfig();
    ProtocolContainer::doCancelClear();
}

bool DownloadContainer::addSourceData(OsclAny* aSourceData)
{
    if (!aSourceData) return false;
    if (!iNode->iDownloadSource->addSource(aSourceData)) return false;
    iNode->iCfgFileContainer->setDataSource(iNode->iDownloadSource);
    return true;
}

bool DownloadContainer::createCfgFile(OSCL_String& aUri)
{
    // create and set iCfgFile
    if (!iNode->iCfgFileContainer) return false;
    return (iNode->iCfgFileContainer->createCfgFile(aUri) == PVMFSuccess);
}

bool DownloadContainer::getProxy(OSCL_String& aProxyName, uint32 &aProxyPort)
{
    // download proxy
    // try proxy name/port from the context data at first, and then config file
    // normally proxy name/port in the context data is stored in iCfgFileContainer,
    // for resume download, the proxy name/port in iCfgFileContainer will be updated
    // by the information from the actual config file (i.e., LoadConfig())
    if (iNode->iDownloadSource->iProxyName.get_size() > 0 && iNode->iDownloadSource->iProxyPort > 0)
    {
        aProxyName = iNode->iDownloadSource->iProxyName;
        aProxyPort = iNode->iDownloadSource->iProxyPort;
    }
    else
    {
        OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
        if (aCfgFile->GetProxyName().get_size() == 0 || aCfgFile->GetProxyPort() == 0) return false;
        aProxyName = aCfgFile->GetProxyName();
        aProxyPort = aCfgFile->GetProxyPort();
    }
    return true;
}

void DownloadContainer::setHttpVersion(const uint32 aHttpVersion)
{
    if (!iNode->iCfgFileContainer->isEmpty())
    {
        iNode->iCfgFileContainer->getCfgFile()->setHttpVersion(aHttpVersion);
    }
}

void DownloadContainer::setHttpExtensionHeaderField(OSCL_String &aFieldKey,
        OSCL_String &aFieldValue,
        const HttpMethod aMethod,
        const bool aPurgeOnRedirect)
{
    iNode->iCfgFileContainer->getCfgFile()->SetExtensionHeaderKey(aFieldKey);
    iNode->iCfgFileContainer->getCfgFile()->SetExtensionHeaderValue(aFieldValue);
    iNode->iCfgFileContainer->getCfgFile()->SetHTTPMethodMaskForExtensionHeader(getBitMaskForHTTPMethod(aMethod));
    iNode->iCfgFileContainer->getCfgFile()->SetExtensionHeaderPurgeOnRediect(aPurgeOnRedirect);
}

bool DownloadContainer::handleContentRangeUnmatch()
{
    // re-issue GET request with range from zero -- meaning starting download from beginning
    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    aCfgFile->SetCurrentFileSize(0);
    aCfgFile->SetOverallFileSize(aCfgFile->GetMaxAllowedFileSize());
    aCfgFile->SetNewSession();
    iNode->iProtocol->stop();

    DownloadOutputConfig config;
    config.isNeedOpenDataStream = true;
    config.isRangeSupport		= false;
    config.isResumeDownload		= true;
    if (iNode->iNodeOutput->initialize((OsclAny*)(&config)) != PVMFSuccess) return false;
    iNode->iNodeOutput->discardData(true); // true means closing and reopening the data stream object
    iNode->iEventReport->startRealDataflow();
    startDataFlowByCommand();
    return true;
}

bool DownloadContainer::downloadUpdateForHttpHeaderAvailable()
{
    if (!iNode->iCfgFileContainer->getCfgFile()->IsNewSession())
    {
        // for resume download session, open data stream with append mode
        DownloadOutputConfig config;
        config.isNeedOpenDataStream = true;
        config.isRangeSupport		= true;
        config.isResumeDownload		= true;
        iNode->iNodeOutput->setCurrentOutputSize(iNode->iProtocol->getDownloadSize());
        iNode->iDownloadControl->setPrevDownloadSize(iNode->iProtocol->getDownloadSize());
        if (iNode->iNodeOutput->initialize((OsclAny*)(&config)) != PVMFSuccess) return false;
    }
    return true;
}

bool DownloadContainer::isStreamingPlayback()
{
    return (iNode->iDownloadSource->iPlaybackControl ==
            (uint32)PVMFSourceContextDataDownloadHTTP::ENoSaveToFile);
}

int32 DownloadContainer::initNodeOutput()
{
    // pass objects to node output object
    iNode->iNodeOutput->setOutputObject((OsclAny*)iNode->iPortInForData);
    iNode->iNodeOutput->setOutputObject((OsclAny*)iInterfacingObjectContainer->getDataStreamFactory(), NodeOutputType_DataStreamFactory);
    iNode->iInterfacingObjectContainer->setOutputPortConnect();  // for sending disconnect after download complete

    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    DownloadOutputConfig config;
    config.isResumeDownload     = !aCfgFile->IsNewSession();
    // for resume download, initially no need to open data stream, because we don't know open mode determined by one of several factors: range support (not available)
    // at this point, will call initialize again once we know if range is supported or not
    config.isNeedOpenDataStream = !config.isResumeDownload;
    if (config.isResumeDownload && (aCfgFile->GetCurrentFileSize() >= aCfgFile->GetOverallFileSize()))
    {
        config.isNeedOpenDataStream = true;
    }

    return iNode->iNodeOutput->initialize((OsclAny*)(&config));
}

bool DownloadContainer::initProtocol_SetConfigInfo()
{
    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    if (iNode->iUserAgentField)
    {
        OSCL_FastString aUserAgent;
        if (!iNode->iUserAgentField->getUserAgent(aUserAgent)) return false;
        aCfgFile->SetUserAgent(aUserAgent);
    }
    iNode->iProtocol->setConfigInfo((OsclAny*)(&aCfgFile));
    return true;
}

void DownloadContainer::initDownloadControl()
{
    iNode->iDownloadControl->setSupportObject((OsclAny*)iNode->iProtocol, DownloadControlSupportObjectType_ProtocolEngine);
    iNode->iDownloadControl->setSupportObject((OsclAny*)iNode->iDownloadProgess, DownloadControlSupportObjectType_DownloadProgress);
    iNode->iDownloadControl->setSupportObject((OsclAny*)iNode->iNodeOutput, DownloadControlSupportObjectType_OutputObject);
    iNode->iDownloadControl->setSupportObject((OsclAny*)iNode->iCfgFileContainer, DownloadControlSupportObjectType_ConfigFileContainer);

    iNode->iDownloadProgess->setSupportObject((OsclAny*)iNode->iProtocol, DownloadControlSupportObjectType_ProtocolEngine);
    iNode->iDownloadProgess->setSupportObject((OsclAny*)iNode->iCfgFileContainer, DownloadControlSupportObjectType_ConfigFileContainer);
    iNode->iDownloadProgess->setSupportObject((OsclAny*)iNode->iNodeOutput, DownloadControlSupportObjectType_OutputObject);
}

bool DownloadContainer::doInfoUpdate(uint32 downloadStatus)
{
    if (downloadStatus == PROCESS_SUCCESS_GOT_EOS ||
            downloadStatus == PROCESS_WAIT_FOR_INCOMING_DATA) return true;

    if (iNode->iInterfaceState == EPVMFNodeStarted)
    {
        updateDownloadControl(isDownloadComplete(downloadStatus));
    }

    // centralize sending info events for download
    return iNode->iEventReport->checkReportEvent(downloadStatus);
}

void DownloadContainer::updateDownloadControl(const bool isDownloadComplete)
{
    // check resume notification
    if (iNode->iDownloadControl->checkResumeNotification(isDownloadComplete) == 1)
    {
        // report data ready event
        iNode->iEventReport->sendDataReadyEvent();
    }

    // update download progress
    iNode->iDownloadProgess->update(isDownloadComplete);
}

bool DownloadContainer::handleProtocolStateComplete(PVProtocolEngineNodeInternalEvent &aEvent, PVProtocolEngineNodeInternalEventHandler *aEventHandler)
{
    iNode->iNodeTimer->clear();
    return ProtocolContainer::handleProtocolStateComplete(aEvent, aEventHandler);
}

void DownloadContainer::checkSendResumeNotification()
{
    iNode->iNodeTimer->start(WALL_CLOCK_TIMER_ID);

    if (needToCheckResumeNotificationMaually())
    {
        iNeedCheckResumeNotificationManually = false;
        // form PVProtocolEngineNodeInternalEventType_CheckResumeNotificationMaually event
        PVProtocolEngineNodeInternalEvent aEvent(PVProtocolEngineNodeInternalEventType_CheckResumeNotificationMaually);
        iNode->iInternalEventQueue.clear();
        iNode->iInternalEventQueue.push_back(aEvent);
        iNode->SetProcessingState(ProcessingState_NormalDataflow);
        iNode->RunIfNotReady();
    }
}

bool DownloadContainer::ignoreThisTimeout(const int32 timerID)
{
    if (timerID != (int32)WALL_CLOCK_TIMER_ID && timerID != BUFFER_STATUS_TIMER_ID)
    {
        return ProtocolContainer::ignoreThisTimeout(timerID);
    }

    // in case of WALL_CLOCK_TIMER_ID
    if (timerID == (int32)WALL_CLOCK_TIMER_ID)
    {
        iNeedCheckResumeNotificationManually = true;
        checkSendResumeNotification();
    }
    else if (timerID == BUFFER_STATUS_TIMER_ID)
    {
        iNode->iEventReport->sendBufferStatusEvent();
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////
//////	pvHttpDownloadOutput implementation
////////////////////////////////////////////////////////////////////////////////////

// constructor
pvHttpDownloadOutput::pvHttpDownloadOutput(PVMFProtocolEngineNodeOutputObserver *aObserver) :
        PVMFProtocolEngineNodeOutput(aObserver),
        iDataStreamFactory(NULL),
        iDataStream(NULL),
        iSessionID(0),
        isOpenDataStream(false),
        iCounter(0)
{
    ;
}

// destructor
pvHttpDownloadOutput::~pvHttpDownloadOutput()
{
    reset();
}

void pvHttpDownloadOutput::reset()
{
    PVMFProtocolEngineNodeOutput::reset();

    // delete data stream object
    if (iDataStreamFactory && iDataStream)
    {
        iDataStream->CloseSession(iSessionID);
        PVUuid uuid = PVMIDataStreamSyncInterfaceUuid;
        iDataStreamFactory->DestroyPVMFCPMPluginAccessInterface(uuid, iDataStream);
        iDataStream = NULL;
        iDataStreamFactory = NULL;
    }
}

void pvHttpDownloadOutput::setOutputObject(OsclAny* aOutputObject, const uint32 aObjectType)
{
    if (aObjectType == NodeOutputType_DataStreamFactory && aOutputObject) iDataStreamFactory = (PVMFDataStreamFactory *)aOutputObject;
    PVMFProtocolEngineNodeOutput::setOutputObject(aOutputObject, aObjectType);
}

uint32 pvHttpDownloadOutput::writeToDataStream(OUTPUT_DATA_QUEUE &aOutputQueue)
{
    uint32 totalFragSize = 0, i;
    for (i = 0; i < aOutputQueue.size();i++)
    {
        uint32 fragSize = aOutputQueue[i].getMemFragSize();
        if (!writeToDataStream((uint8*)(aOutputQueue[i].getMemFragPtr()), fragSize)) return ~0;
        totalFragSize += fragSize;
    }

    LOGINFODATAPATH((0, "pvHttpDownloadOutput::writeToDataStream() SIZE= %d , SEQNUM=%d", totalFragSize, iCounter++));
    iCurrTotalOutputSize += totalFragSize;
    return totalFragSize;
}

bool pvHttpDownloadOutput::writeToDataStream(uint8 *aBuffer, uint32 aBufferLen)
{
    if (iDataStream->Write(iSessionID, aBuffer, sizeof(uint8), aBufferLen) != PVDS_SUCCESS) return false;
    return true;
}

int32 pvHttpDownloadOutput::flushData(const uint32 aOutputType)
{
    int32 status = PVMFProtocolEngineNodeOutput::flushData(aOutputType);
    if (status != PROCESS_SUCCESS) return status;

    while (!iOutputFramesQueue.empty())
    {
        if (writeToDataStream(iOutputFramesQueue[0]) == 0xffffffff) return PROCESS_OUTPUT_TO_DATA_STREAM_FAILURE;
        iOutputFramesQueue.erase(iOutputFramesQueue.begin());
    }
    return PROCESS_SUCCESS;
}

int32 pvHttpDownloadOutput::initialize(OsclAny* aInitInfo)
{
    // open data stream object
    if (!iDataStreamFactory || !iPortIn) return PVMFFailure;
    if (!iDataStream)
    {
        PVUuid uuid = PVMIDataStreamSyncInterfaceUuid;
        iDataStream = (PVMIDataStreamSyncInterface*)iDataStreamFactory->CreatePVMFCPMPluginAccessInterface(uuid);
        if (!iDataStream) return PVMFFailure;

        // create memory pool
        int32 status = createMemPool();
        if (status != PVMFSuccess) return status;
    }

    // open data stream object if needed
    return openDataStream(aInitInfo);
}

int32 pvHttpDownloadOutput::openDataStream(OsclAny* aInitInfo)
{
    DownloadOutputConfig* config = (DownloadOutputConfig*)aInitInfo;
    if (config->isNeedOpenDataStream && !isOpenDataStream)
    {
        PvmiDataStreamMode aMode = PVDS_WRITE_ONLY;
        if (config->isResumeDownload && config->isRangeSupport) aMode = PVDS_APPEND;

        if (iDataStream->OpenSession(iSessionID, aMode) != PVDS_SUCCESS) return PROCESS_DATA_STREAM_OPEN_FAILURE;
        isOpenDataStream = true;
    }
    return PVMFSuccess;
}

void pvHttpDownloadOutput::discardData(const bool aNeedReopen)
{
    // discard the existing data inside the data stream object
    if (iDataStream && isOpenDataStream)
    {
        if (aNeedReopen)
        {
            iDataStream->CloseSession(iSessionID);
            iDataStream->OpenSession(iSessionID, PVDS_REWRITE);
        }
    }
    PVMFProtocolEngineNodeOutput::discardData();
}

uint32 pvHttpDownloadOutput::getAvailableOutputSize()
{
    uint32 writeCapacity = 0xFFFFFFFF;
    if (iDataStream)
    {
        iDataStream->QueryWriteCapacity(iSessionID, writeCapacity);
    }
    return writeCapacity;
}

uint32 pvHttpDownloadOutput::getMaxAvailableOutputSize()
{
    return (iDataStream == NULL ? 0 : iDataStream->QueryBufferingCapacity());
}

////////////////////////////////////////////////////////////////////////////////////
//////	pvDownloadControl implementation
////////////////////////////////////////////////////////////////////////////////////

pvDownloadControl::pvDownloadControl() :
        iCurrentPlaybackClock(NULL),
        iProgDownloadSI(NULL),
        iProtocol(NULL),
        iDownloadProgress(NULL),
        iNodeOutput(NULL),
        iCfgFileContainer(NULL),
        iFirstResumeNotificationSent(false),
        iClipDurationMsec(0),
        iFileSize(0)
{
    clearBody();
    createDownloadClock(); // may leave
    iDataPathLogger = PVLogger::GetLoggerObject(NODEDATAPATHLOGGER_TAG);
}

void pvDownloadControl::clear()
{
    // check whether there is still pending resume request
    if (iProgDownloadSI) sendResumeNotification(true);
    clearBody();
}

void pvDownloadControl::clearBody()
{
    iPlaybackUnderflow			 = true;
    iDownloadComplete			 = false;
    iRequestResumeNotification	 = false;
    iCurrentNPTReadPosition		 = 0;
    iPlaybackByteRate			 = 0;
    iClipByterate				 = 0;
    iPrevDownloadSize			 = 0;
    iDlAlgoPreConditionMet		 = false;
    iSetFileSize				 = false;
    iSendDownloadCompleteNotification = false;
}


// requst resume notification, implementation of PVMFDownloadProgressInterface API
void pvDownloadControl::requestResumeNotification(const uint32 currentNPTReadPosition, bool& aDownloadComplete, bool& aNeedSendUnderflowEvent)
{
    LOGINFODATAPATH((0, "pvDownloadControl::requestResumeNotification() IN, iPlaybackUnderflow=%d, iRequestResumeNotification=%d, iDownloadComplete=%d",
                     (uint32)iPlaybackUnderflow, (uint32)iRequestResumeNotification, (uint32)iDownloadComplete));

    if (iFirstResumeNotificationSent) aNeedSendUnderflowEvent = !iRequestResumeNotification;
    else aNeedSendUnderflowEvent = false;

    if (!(aDownloadComplete = iDownloadComplete))
    {
        iPlaybackUnderflow = true;
        iCurrentNPTReadPosition = currentNPTReadPosition;
        iDownloadComplete = false;
    }

    iRequestResumeNotification = true;

    // save the download size at the underflow point
    iPrevDownloadSize = iNodeOutput->getCurrentOutputSize();

    // estimate playback rate and save the download size at the underflow point
    if (currentNPTReadPosition > 0 && currentNPTReadPosition < 0xFFFFFFFF)
    {
        // estimate playback rate
        iPlaybackByteRate = divisionInMilliSec(iProtocol->getDownloadSize(), currentNPTReadPosition);

        uint32 iPrevDownloadSizeOrig = 0;
        iPrevDownloadSizeOrig = iPrevDownloadSize;
        if (iClipByterate == 0 && iClipDurationMsec > 0) iClipByterate = divisionInMilliSec(iFileSize, iClipDurationMsec);
        iPrevDownloadSize = OSCL_MAX(iPrevDownloadSize, currentNPTReadPosition / 1000 * iClipByterate);

        LOGINFODATAPATH((0, "pvDownloadControl::requestResumeNotification(), currentNPTReadPosition=%d, playbackRate=%dbps, prevDownloadSize=%d, iPrevDownloadSizeOrig=%d, iClipByterate=%dbps",
                         currentNPTReadPosition, (iPlaybackByteRate << 3), iPrevDownloadSize, iPrevDownloadSizeOrig, (iClipByterate << 3)));
    }
}

void pvDownloadControl::setSupportObject(OsclAny *aDLSupportObject, DownloadControlSupportObjectType aType)
{
    switch (aType)
    {
        case DownloadControlSupportObjectType_SupportInterface:
            iProgDownloadSI = (PVMFFormatProgDownloadSupportInterface*)aDLSupportObject;
            // in high bandwidth conditions, iProgDownloadSI gets set AFTER download is complete, then
            // need to check resume notification again if something is pending
            if (iDownloadComplete) checkResumeNotification(iDownloadComplete);
            else checkSendingNotification();
            break;

        case DownloadControlSupportObjectType_ProgressInterface:
        {
            PVMFDownloadProgressInterface *aProgDownload = (PVMFDownloadProgressInterface *)aDLSupportObject;
            if (iProgDownloadSI) iProgDownloadSI->setDownloadProgressInterface(aProgDownload);
            break;
        }
        case DownloadControlSupportObjectType_EnginePlaybackClock:
            iCurrentPlaybackClock = (PVMFMediaClock *)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_ProtocolEngine:
            iProtocol = (HttpBasedProtocol *)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_DownloadProgress:
            iDownloadProgress = (DownloadProgressInterface *)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_OutputObject:
            iNodeOutput = (PVMFProtocolEngineNodeOutput *)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_ConfigFileContainer:
            iCfgFileContainer = (PVDlCfgFileContainer *)aDLSupportObject;
            if (!iCfgFileContainer->getCfgFile()->IsNewSession())
            {
                if (iCfgFileContainer->getCfgFile()->HasContentLength())
                {
                    iFileSize = iCfgFileContainer->getCfgFile()->GetOverallFileSize();
                }
            }
            break;

        default:
            break;
    }
}

// check whether to make resume notification; if needed, then make resume notification
// Return value: 1 means making resume notification normally (underflow->auto resume),
//				 2 means making resume notification for download complete
//				 0 means anything else
int32 pvDownloadControl::checkResumeNotification(const bool aDownloadComplete)
{
    //LOGINFODATAPATH((0, "pvDownloadControl::checkResumeNotification() IN, iPlaybackUnderflow=%d, iRequestResumeNotification=%d, aDownloadComplete=%d",
    //	(uint32)iPlaybackUnderflow, (uint32)iRequestResumeNotification, (uint32)aDownloadComplete));

    // short-cut: download complete
    // check sending file size, protocol info or download complete notification
    if (!checkSendingNotification(aDownloadComplete))
    {
        LOGINFODATAPATH((0, "pvDownloadControl::checkResumeNotification()->checkDownloadCompleteForResumeNotification() return false, iProgDownloadSI=0x%x", iProgDownloadSI));
        return 0;
    }

    // real work that causes some PDL and PS differences
    if (!iPlaybackUnderflow && iRequestResumeNotification)
    {
        sendResumeNotification(iDownloadComplete);
        return 2;
    }

    // check if need to resume playback
    if (iPlaybackUnderflow &&
            isResumePlayback(iProtocol->getDownloadRate(),
                             iNodeOutput->getCurrentOutputSize(),
                             iFileSize))
    {
        iPlaybackUnderflow = false;
        sendResumeNotification(iDownloadComplete);
        iFirstResumeNotificationSent = true;
        return 1;
    }

    return 0;
}

bool pvDownloadControl::checkSendingNotification(const bool aDownloadComplete)
{
    if (aDownloadComplete)
    {
        LOGINFODATAPATH((0, "pvDownloadControl::checkDownloadCompleteForResumeNotification()  Download is complete, final download rate = %dbps", (iProtocol->getDownloadRate() << 3)));
    }
    iDownloadComplete = aDownloadComplete;
    // update iFileSize to minimize dependency on protocol object and improve the efficiency
    updateFileSize();

    if (!isInfoReady()) return false;

    // set file size to parser node
    setFileSize(iFileSize);

    // set protocol info to parser node if needed
    setProtocolInfo();

    // send download complete notification to parser node
    if (aDownloadComplete) sendDownloadCompleteNotification();

    // update download clock
    if (!iDownloadComplete) updateDownloadClock();
    return true;
}

void pvDownloadControl::updateFileSize()
{
    if (iProtocol)
    {
        if (iProtocol->getContentLength() > 0 && iFileSize == 0) iFileSize = iProtocol->getContentLength();
    }
}

void pvDownloadControl::setFileSize(const uint32 aFileSize)
{
    if (iSetFileSize) return;
    if (aFileSize == 0 || !iProgDownloadSI) return;

    iProgDownloadSI->setFileSize(aFileSize);
    iSetFileSize = true;
}

void pvDownloadControl::sendResumeNotification(bool aDownloadComplete)
{
    if (iRequestResumeNotification && iProgDownloadSI)
    {
        iProgDownloadSI->playResumeNotification(aDownloadComplete);
        iRequestResumeNotification = false;
        iFirstResumeNotificationSent = true;
        if (aDownloadComplete) iPlaybackUnderflow = false;

        // sync up with actual download complete
        if (aDownloadComplete && !iDownloadComplete) iDownloadComplete = aDownloadComplete;
    }
}

void pvDownloadControl::sendDownloadCompleteNotification()
{
    if (!iProgDownloadSI || iSendDownloadCompleteNotification) return;

    // send download complete notification
    LOGINFODATAPATH((0, "pvDownloadControl::sendDownloadCompleteNotification() - Notify download complete"));
    iProgDownloadSI->notifyDownloadComplete();
    iSendDownloadCompleteNotification = true;
}

// create iDlProgressClock, will leave when memory allocation fails
void pvDownloadControl::createDownloadClock()
{
    // create shared PVMFMediaClock
    PVDlSharedPtrAlloc<PVMFMediaClock> alloc;
    PVMFMediaClock* myClock = alloc.allocate();
    OsclRefCounterSA< PVDlSharedPtrAlloc<PVMFMediaClock> > *refcnt = new OsclRefCounterSA< PVDlSharedPtrAlloc<PVMFMediaClock> >(myClock);
    OsclSharedPtr<PVMFMediaClock> myHandle(myClock, refcnt);
    iDlProgressClock = myHandle;

    // set the clock base
    iDlProgressClock->SetClockTimebase(iEstimatedServerClockTimeBase);
    uint32 startTime = 0; // for type conversion
    bool bOverflowFlag = false;
    iDlProgressClock->SetStartTime32(startTime, PVMF_MEDIA_CLOCK_SEC, bOverflowFlag);
}


// auto-resume playback decision
bool pvDownloadControl::isResumePlayback(const uint32 aDownloadRate,
        const uint32 aCurrDownloadSize,
        const uint32 aFileSize)
{
    // check download complete, for download complete, no need to run the following algorithm
    if (iDownloadComplete || isOutputBufferOverflow())
    {
        if (!iDownloadComplete)
        {
            LOGINFODATAPATH((0, "pvDownloadControl::isResumePlayback(), output buffer (MBDS is full) overflows!! Then auto-resume kicks off!!"));
        }
        return true;
    }

    // check playback clock, if not available, then switch to the old algorithm
    if (!iCurrentPlaybackClock) return isResumePlaybackWithOldAlg(aDownloadRate, aFileSize - aCurrDownloadSize);


    // check the pre-conditins including initial download time/size for download rate estimation purpose
    if (!isDlAlgoPreConditionMet(aDownloadRate, iClipDurationMsec, aCurrDownloadSize, aFileSize)) return false;

    // get the playback clock time
    if (iClipDurationMsec > 0 && aFileSize > 0)
    {
        return checkAutoResumeAlgoWithConstraint(aDownloadRate, aFileSize - aCurrDownloadSize, iClipDurationMsec, aFileSize);
    }

    return checkAutoResumeAlgoNoConstraint(aCurrDownloadSize, aFileSize, iClipDurationMsec);
}


bool pvDownloadControl::isDlAlgoPreConditionMet(const uint32 aDownloadRate,
        const uint32 aDurationMsec,
        const uint32 aCurrDownloadSize,
        const uint32 aFileSize)
{
    if (iDlAlgoPreConditionMet) return iDlAlgoPreConditionMet;

    LOGINFODATAPATH((0, "pvDownloadControl::isResumePlayback()->isDlAlgoPreConditionMet(), download rate = %d , clip duration = %dms, download size = %d",
                     aDownloadRate, aDurationMsec, aCurrDownloadSize));
    OSCL_UNUSED_ARG(aDurationMsec);
    if (aDownloadRate == 0) return false;

    // check initial download time for download rate estimation
    uint32 downloadTimeMsec = iProtocol->getDownloadTimeForEstimation();
    LOGINFODATAPATH((0, "pvDownloadControl::isResumePlayback()->isDlAlgoPreConditionMet(), check dl_time(%dms) > 1sec, OR download size(%d) >= 10 percent of file size(%d)=%d",
                     downloadTimeMsec, aCurrDownloadSize, aFileSize, aFileSize / PVPROTOCOLENGINE_INIT_DOWNLOAD_SIZE_PERCENTAGE_THRESHOLD));
    iDlAlgoPreConditionMet = (downloadTimeMsec >= PVPROTOCOLENGINE_INIT_DOWNLOAD_TIME_THRESHOLD);
    if (iDlAlgoPreConditionMet) return true;

    // check initial download size for download rate estimation
    uint32 initDownloadThreshold = (aFileSize > 0 ? aFileSize / PVPROTOCOLENGINE_INIT_DOWNLOAD_SIZE_PERCENTAGE_THRESHOLD : PVPROTOCOLENGINE_INIT_DOWNLOAD_SIZE_THRESHOLD);
    iDlAlgoPreConditionMet = (aCurrDownloadSize >= initDownloadThreshold);
    return iDlAlgoPreConditionMet;
}

// with contraint: file size and clip duration are both available
bool pvDownloadControl::checkAutoResumeAlgoWithConstraint(const uint32 aDownloadRate,
        const uint32 aRemainingDownloadSize,
        const uint32 aDurationMsec,
        const uint32 aFileSize)
{
    // get the playback clock time
    uint32 playbackTimeMec32 = 0;
    if (!getPlaybackTimeFromEngineClock(playbackTimeMec32)) return false;

    LOGINFODATAPATH((0, "pvDownloadControl::isResumePlayback()->checkAutoResumeAlgowithConstraint(), algorithm: RemainingDownloadSize < 0.0009 * dl_rate * remaining_playback_time: remaining_dl_size= %d, dl_rate=%dByte/s,  playback_remaining_time=%dms",
                     aRemainingDownloadSize, aDownloadRate, aDurationMsec - playbackTimeMec32));
    // the basic algorithm is, remaining download time (remaining download size/download rate) <
    //						   remaining playback time (duration - current playback time) * 0.9

    uint32 newDurationMsec = aDurationMsec;
    if (!checkNewDuration(aDurationMsec, newDurationMsec)) return false;
    uint32 playbackRemainingTimeMsec = newDurationMsec - playbackTimeMec32;
    // 4sec buffering time
    if (approveAutoResumeDecisionShortCut(aFileSize - aRemainingDownloadSize, newDurationMsec, playbackTimeMec32, playbackRemainingTimeMsec))
    {
        LOGINFODATAPATH((0, "pvDownloadControl::isResumePlayback()->checkAutoResumeAlgowithConstraint(), 4sec extra buffering time"));
        return true;
    }

    if (approveAutoResumeDecision(aRemainingDownloadSize, aDownloadRate, playbackRemainingTimeMsec))
    {
        LOGINFODATAPATH((0, "pvDownloadControl::isResumePlayback()->checkAutoResumeAlgowithConstraint(), BytesLeft = %d, dl_rate = %dbps, duration = %d(orig_duration=%d), playback_time=%d",
                         aRemainingDownloadSize, (aDownloadRate << 3), newDurationMsec, aDurationMsec, playbackTimeMec32));
        return true;
    }

    return false;
}


bool pvDownloadControl::getPlaybackTimeFromEngineClock(uint32 &aPlaybackTime)
{
    aPlaybackTime = 0;
    bool isPbOverflow = false;
    iCurrentPlaybackClock->GetCurrentTime32(aPlaybackTime, isPbOverflow, PVMF_MEDIA_CLOCK_MSEC);
    if (isPbOverflow)
    {
        LOGERRORDATAPATH((0, "pvDownloadControl::getPlaybackTimeFromEngineClock(), Playback clock overflow %d", isPbOverflow));
        return false;
    }

    LOGINFODATAPATH((0, "pvDownloadControl::getPlaybackTimeFromEngineClock(), aPlaybackTime=%d, iCurrentNPTReadPosition=%d",
                     aPlaybackTime, iCurrentNPTReadPosition));
    aPlaybackTime = OSCL_MAX(aPlaybackTime, iCurrentNPTReadPosition);
    return true;
}

// use fixed-point calculation to replace the float-point calculation: aRemainingDLSize<0.0009*aDownloadRate*aRemainingPlaybackTime
bool pvDownloadControl::approveAutoResumeDecision(const uint32 aRemainingDLSize,
        const uint32 aDownloadRate,
        const uint32 aRemainingPlaybackTime)
{

#if 0
    // the float-point calculation: aRemainingDLSize<0.0009*aDownloadRate*aRemainingPlaybackTime
    return (aRemainingDLSize < 0.0009*aDownloadRate*aRemainingPlaybackTime);
#else
    // fixed-point calculation
    // 0.0009 = 1/1111 ~= 1/1024 = 1/2^10 = right shift 10 bits
    // aRemainingDLSize<(aDownloadRate*aRemainingPlaybackTime>>10)

    uint32 max = OSCL_MAX(aDownloadRate, aRemainingPlaybackTime);
    if ((max >> PVPROTOCOLENGINE_AUTO_RESUME_FIXED_CALCULATION_MAX_LIMIT_RIGHT_SHIFT_FACTOR) == 0)   // right shift 16 bits, 2^16= 65536
    {
        return (aRemainingDLSize < (aDownloadRate*aRemainingPlaybackTime >>
                                    PVPROTOCOLENGINE_AUTO_RESUME_FIXED_CALCULATION_RIGHT_SHIFT)); // right shift 10 bits
    }
    else
    {
        uint32 min = OSCL_MIN(aDownloadRate, aRemainingPlaybackTime);
        uint32 maxRightShift10 = max >> PVPROTOCOLENGINE_AUTO_RESUME_FIXED_CALCULATION_RIGHT_SHIFT; // right shift 10 bits
        return (aRemainingDLSize / maxRightShift10 < min);
    }
#endif
}

// result = x*1000/y
uint32 pvDownloadControl::divisionInMilliSec(const uint32 x, const uint32 y)
{
    // result = x*1000/y
    // handle overflow issue
    if (x >> PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_LIMIT_RIGHT_SHIFT_FACTOR == 0) return x*1000 / y; // no overflow

    // x*1000 overflows
    uint32 result = (x >> PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_RIGHTSHIFT_FACTOR) * 1000;
    if (result < y) result /= (y >> PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_RIGHTSHIFT_FACTOR);
    else
    {
        uint32 resultTmp = result / y;
        if (resultTmp >> PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_LIMIT_RIGHT_SHIFT_FACTOR) /* overflow */  result = 0xffffffff;
        else
        {
            // check the accuracy of result/y
            uint32 halfRightShift = PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_RIGHTSHIFT_FACTOR >> 1;
            if (resultTmp >> halfRightShift)
                result = resultTmp << PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_RIGHTSHIFT_FACTOR;
            else
            {
                result /= (y >> halfRightShift);
                result <<= (PVPROTOCOLENGINE_DOWNLOAD_DURATION_CALCULATION_RIGHTSHIFT_FACTOR - halfRightShift);
            }
        }
    }
    return result;
}

bool pvDownloadControl::isResumePlaybackWithOldAlg(const uint32 aDownloadRate, const uint32 aRemainingDownloadSize)
{
    // get the download progress clock time
    uint32 download_time;
    bool overflowFlag = false;
    iDlProgressClock->GetCurrentTime32(download_time, overflowFlag, PVMF_MEDIA_CLOCK_MSEC);
    uint32 currentNPTDownloadPosition = Oscl_Int64_Utils::get_uint64_lower32(download_time);

    LOGINFODATAPATH((0, "pvDownloadControl::isResumePlaybackWithOldAlg(), download_time=%dms, download_complete=%d\n", download_time, iDownloadComplete));

    if (iCurrentNPTReadPosition < currentNPTDownloadPosition)
    {
        // bytes_remaining < (0.9 * (received data rate * file duration ))

        uint32 BytesTobeDownloadedForAutoResume = (uint32)((currentNPTDownloadPosition - iCurrentNPTReadPosition) *
                aDownloadRate * 0.0009);

        LOGINFODATAPATH((0, "pvDownloadControl::isResumePlaybackWithOldAlg(), currentDLPosition=%dms, iCurrentReadPosition=%dms, downloadRate=%d, remainingSize %d\n", \
                         currentNPTDownloadPosition, iCurrentNPTReadPosition, aDownloadRate, aRemainingDownloadSize));

        if (BytesTobeDownloadedForAutoResume > aRemainingDownloadSize)
        {
            return true;
        }
    }

    return false;
}

void pvDownloadControl::cancelResumeNotification()
{
    // Just reset the boolean iRequestResumeNotification, so that download control sends no resume notification.
    iRequestResumeNotification = false;
}


////////////////////////////////////////////////////////////////////////////////////
//////	DownloadProgress implementation
////////////////////////////////////////////////////////////////////////////////////
// constructor
DownloadProgress::DownloadProgress() :
        iProtocol(NULL),
        iProgDownloadSI(NULL),
        iNodeOutput(NULL)
{
    reset();
}

void DownloadProgress::reset()
{
    //for progress reports
    iCurrProgressPercent  = 0;
    iPrevProgressPercent  = 0;
    iDownloadNPTTime	  = 0;
    iDurationMsec		  = 0;
}

void DownloadProgress::setSupportObject(OsclAny *aDLSupportObject, DownloadControlSupportObjectType aType)
{
    switch (aType)
    {
        case DownloadControlSupportObjectType_SupportInterface:
            iProgDownloadSI = (PVMFFormatProgDownloadSupportInterface*)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_ProtocolEngine:
            iProtocol = (HttpBasedProtocol *)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_OutputObject:
            iNodeOutput = (PVMFProtocolEngineNodeOutput *)aDLSupportObject;
            break;

        default:
            break;
    }
}

bool DownloadProgress::update(const bool aDownloadComplete)
{
    updateDownloadClock(aDownloadComplete);

    // update download progress
    uint32 newProgressPercent = 0;
    if (!calculateDownloadPercent(newProgressPercent)) return false;

    //Report 0... 100 percent complete.
    // In progressive streaming, repositioning is allowed during download
    // so the download percentage does not always increase
    if (newProgressPercent != iCurrProgressPercent)
    {
        //avoid sending the same percentage update
        iCurrProgressPercent = newProgressPercent;
        return true;
    }
    return false;
}

bool DownloadProgress::getNewProgressPercent(uint32 &aProgressPercent)
{
    aProgressPercent = iCurrProgressPercent;
    // in progressive streaming, after repositioning esp rewind, the current download percentage
    // may be smaller than the previous percentage and current percentage may not be 0 unless rewind back to
    // // beginning of clip
    if (((iCurrProgressPercent < iPrevProgressPercent) && iPrevProgressPercent > 0) ||
            iCurrProgressPercent > iPrevProgressPercent)
    {
        iPrevProgressPercent = iCurrProgressPercent;
        return true;
    }
    return false;
}

bool DownloadProgress::calculateDownloadPercent(uint32 &aDownloadProgressPercent)
{
    // clip duration
    uint32 clipDuration = getClipDuration();
    if (clipDuration == 0) return false;

    // download progress, convert to percent complete.
    aDownloadProgressPercent = iDownloadNPTTime * 100 / clipDuration;
    if (aDownloadProgressPercent > 100) aDownloadProgressPercent = 100;
    return true;
}

uint32 DownloadProgress::getClipDuration()
{
    return iDurationMsec;
}

////////////////////////////////////////////////////////////////////////////////////
//////	PVMFDownloadDataSourceContainer implementation
////////////////////////////////////////////////////////////////////////////////////
PVMFDownloadDataSourceContainer::PVMFDownloadDataSourceContainer(OsclAny* aSourceData)
{
    addSource(aSourceData);
}

bool PVMFDownloadDataSourceContainer::addSource(OsclAny* aSourceData)
{
    PVInterface* pvInterface = OSCL_STATIC_CAST(PVInterface*, aSourceData);

    PVInterface* aDownloadSourceInterface = NULL;
    PVUuid downloadHTTPDataUuid(PVMF_DOWNLOAD_DATASOURCE_HTTP_UUID);
    if (pvInterface->queryInterface(downloadHTTPDataUuid, aDownloadSourceInterface))
    {
        PVMFDownloadDataSourceHTTP* aInterface = OSCL_STATIC_CAST(PVMFDownloadDataSourceHTTP*, aDownloadSourceInterface);
        copy(*aInterface);
        return true;
    }

    PVUuid downloadPVXDataUuid(PVMF_DOWNLOAD_DATASOURCE_PVX_UUID);
    if (pvInterface->queryInterface(downloadPVXDataUuid, aDownloadSourceInterface))
    {
        PVMFDownloadDataSourcePVX* aInterface = OSCL_STATIC_CAST(PVMFDownloadDataSourcePVX*, aDownloadSourceInterface);
        copy(*aInterface);
        return true;
    }

    PVInterface* sourceDataContext = NULL;
    PVUuid sourceContextUuid(PVMF_SOURCE_CONTEXT_DATA_UUID);
    if (pvInterface->queryInterface(sourceContextUuid, sourceDataContext))
    {

        PVUuid dlHTTPContextUuid(PVMF_SOURCE_CONTEXT_DATA_DOWNLOAD_HTTP_UUID);
        if (sourceDataContext->queryInterface(dlHTTPContextUuid, aDownloadSourceInterface))
        {
            PVMFSourceContextDataDownloadHTTP* aInterface = OSCL_STATIC_CAST(PVMFSourceContextDataDownloadHTTP*, aDownloadSourceInterface);
            copy(*aInterface);
            return true;
        }

        PVUuid dlPVXContextUuid(PVMF_SOURCE_CONTEXT_DATA_DOWNLOAD_PVX_UUID);
        if (sourceDataContext->queryInterface(dlPVXContextUuid, aDownloadSourceInterface))
        {
            PVMFSourceContextDataDownloadPVX* aInterface = OSCL_STATIC_CAST(PVMFSourceContextDataDownloadPVX*, aDownloadSourceInterface);
            copy(*aInterface);
            return true;
        }
    }
    return false;
}

void PVMFDownloadDataSourceContainer::copy(const PVMFDownloadDataSourceHTTP& aSourceData)
{
    iHasDataSource	  = true;
    iIsNewSession	  = aSourceData.bIsNewSession;
    iMaxFileSize	  = aSourceData.iMaxFileSize;
    iPlaybackControl  = (uint32)convert(aSourceData.iPlaybackControl);
    if (aSourceData.iPlaybackControl == PVMFDownloadDataSourceHTTP::ENoSaveToFile) iIsNewSession = true; // always use new download session for progressive streaming
    iConfigFileName   = aSourceData.iConfigFileName;
    iDownloadFileName = aSourceData.iDownloadFileName;
    iProxyName		  = aSourceData.iProxyName;
    iProxyPort		  = aSourceData.iProxyPort;
    iPvxInfo		  = NULL;
}

void PVMFDownloadDataSourceContainer::copy(const PVMFDownloadDataSourcePVX& aSourceData)
{
    iHasDataSource	  = true;
    iIsNewSession	  = aSourceData.bIsNewSession;
    iMaxFileSize	  = aSourceData.iMaxFileSize;
    iPlaybackControl  = 0;
    iConfigFileName   = aSourceData.iConfigFileName;
    iDownloadFileName = aSourceData.iDownloadFileName;
    iProxyName		  = aSourceData.iProxyName;
    iProxyPort		  = aSourceData.iProxyPort;
    iPvxInfo		  = &aSourceData.iPvxInfo;
}

void PVMFDownloadDataSourceContainer::copy(const PVMFSourceContextDataDownloadHTTP& aSourceData)
{
    iHasDataSource	  = true;
    iIsNewSession	  = aSourceData.bIsNewSession;
    iMaxFileSize	  = aSourceData.iMaxFileSize;
    iPlaybackControl  = (uint32)aSourceData.iPlaybackControl;
    if (aSourceData.iPlaybackControl == PVMFSourceContextDataDownloadHTTP::ENoSaveToFile) iIsNewSession = true; // always use new download session for progressive streaming
    iConfigFileName   = aSourceData.iConfigFileName;
    iDownloadFileName = aSourceData.iDownloadFileName;
    iProxyName		  = aSourceData.iProxyName;
    iProxyPort		  = aSourceData.iProxyPort;
    iUserID			  = aSourceData.iUserID;
    iUserPasswd		  = aSourceData.iUserPasswd;
    iPvxInfo		  = NULL;
}

void PVMFDownloadDataSourceContainer::copy(const PVMFSourceContextDataDownloadPVX& aSourceData)
{
    iHasDataSource	  = true;
    iIsNewSession	  = aSourceData.bIsNewSession;
    iMaxFileSize	  = aSourceData.iMaxFileSize;
    iPlaybackControl  = 0;
    iConfigFileName   = aSourceData.iConfigFileName;
    iDownloadFileName = aSourceData.iDownloadFileName;
    iProxyName		  = aSourceData.iProxyName;
    iProxyPort		  = aSourceData.iProxyPort;
    iPvxInfo		  = aSourceData.iPvxInfo;
}

PVMFSourceContextDataDownloadHTTP::TPVPlaybackControl PVMFDownloadDataSourceContainer::convert(const PVMFDownloadDataSourceHTTP::TPVPlaybackControl aPlaybackControl)
{
    switch (aPlaybackControl)
    {
        case PVMFDownloadDataSourceHTTP::ENoPlayback:
            return PVMFSourceContextDataDownloadHTTP::ENoPlayback;
        case PVMFDownloadDataSourceHTTP::EAfterDownload:
            return PVMFSourceContextDataDownloadHTTP::EAfterDownload;
        case PVMFDownloadDataSourceHTTP::EAsap:
            return PVMFSourceContextDataDownloadHTTP::EAsap;
        case PVMFDownloadDataSourceHTTP::ENoSaveToFile:
            return PVMFSourceContextDataDownloadHTTP::ENoSaveToFile;
        case PVMFDownloadDataSourceHTTP::EReserve:
            return PVMFSourceContextDataDownloadHTTP::EReserve;
        default:
            break;
    };
    return PVMFSourceContextDataDownloadHTTP::EAsap;
}


////////////////////////////////////////////////////////////////////////////////////
//////	PVDlCfgFileContainer implementation
////////////////////////////////////////////////////////////////////////////////////
PVMFStatus PVDlCfgFileContainer::createCfgFile(OSCL_String &aUrl)
{
    // iDataSource should be ready at this point
    if (!iDataSource) return PVMFFailure;

    // create iCfgFileObj
    PVDlSharedPtrAlloc<PVDlCfgFile> alloc;
    PVDlCfgFile* myCfg = alloc.allocate();
    OsclRefCounterSA< PVDlSharedPtrAlloc<PVDlCfgFile> > *refcnt = new OsclRefCounterSA< PVDlSharedPtrAlloc<PVDlCfgFile> >(myCfg);
    OsclSharedPtr<PVDlCfgFile> myHandle(myCfg, refcnt);
    iCfgFileObj = myHandle;

    // set common stuff for progressive download and fasttrack
    OSCL_FastString player_version(_STRLIT_CHAR("4.0"));
    iCfgFileObj->SetPlayerVersion(player_version);

    OSCL_FastString user_network(_STRLIT_CHAR("UNKNOWN"));
    iCfgFileObj->SetUserNetwork(user_network);

    OSCL_FastString deviceInfo(_STRLIT_CHAR("MANUF=UNKNOWN;PROC=WINS EMULATOR;MEM=UNKNOWN;OS=EPOC;DISPLAY=TRUECOLOR16"));
    iCfgFileObj->SetDeviceInfo(deviceInfo);

    iCfgFileObj->SetNetworkTimeouts(30000, 30000, -1);

    iCfgFileObj->SetRangeStartTime(0);

    return configCfgFile(aUrl);
}

PVMFStatus PVDlCfgFileContainer::loadOldConfig()
{
    int32 status = iCfgFileObj->LoadConfig();
    LOGINFODATAPATH((0, "PVDlCfgFileContainer::loadOldConfig() status=%d(-1 critical, -2 non-critical), currFileSize=%d, totalFileSize=%d, rangeStartTime=%d",
                     status, iCfgFileObj->GetCurrentFileSize(), iCfgFileObj->GetOverallFileSize(), iCfgFileObj->GetRangeStartTime()));

    if (status == PVDlCfgFile::LoadConfigStatus_CriticalError) return PVMFFailure;
    if (status == PVDlCfgFile::LoadConfigStatus_NonCriticalError)
    {
        // set up a new download session
        iCfgFileObj->SetCurrentFileSize(0);
        iCfgFileObj->SetOverallFileSize(iCfgFileObj->GetMaxAllowedFileSize());
        iCfgFileObj->SetNewSession();
    }

    PVDlCfgFile::TPVDLPlaybackMode tmpPlaybackMode = iCfgFileObj->GetPlaybackMode();
    if (tmpPlaybackMode == PVDlCfgFile::EPVDL_ASAP)
    {
        iPlaybackMode = PVMFDownloadDataSourceHTTP::EAsap;;
    }
    else if (tmpPlaybackMode == PVDlCfgFile::EPVDL_PLAYBACK_AFTER_DOWNLOAD)
    {
        iPlaybackMode = PVMFDownloadDataSourceHTTP::EAfterDownload;
    }
    else if (tmpPlaybackMode == PVDlCfgFile::EPVDL_DOWNLOAD_ONLY)
    {
        iPlaybackMode = PVMFDownloadDataSourceHTTP::ENoPlayback;
    }
    else //if(tmpPlaybackMode == PVDlCfgFile::EReserve )
    {
        //iPlaybackMode = PVMFDownloadDataSourceHTTP::EReserve;
        return PVMFFailure;
    }
    return PVMFSuccess;
}

PVMFStatus PVDlCfgFileContainer::configCfgFile(OSCL_String &aUrl)
{
    if (iDataSource->isEmpty()) return PVMFFailure;
    if (iDataSource->iMaxFileSize <= 0) return PVMFFailure;
    iCfgFileObj->SetOverallFileSize(iDataSource->iMaxFileSize);
    iCfgFileObj->SetMaxAllowedFileSize(iDataSource->iMaxFileSize);

    iCfgFileObj->SetConfigFileName(iDataSource->iConfigFileName);
    iCfgFileObj->SetDownloadFileName(iDataSource->iDownloadFileName);

    // save URL
    iCfgFileObj->SetUrl(aUrl);

    // for old session, load the config file
    if (!iDataSource->iIsNewSession) return loadOldConfig();
    return PVMFSuccess;
}



////////////////////////////////////////////////////////////////////////////////////
//////	downloadEventReporter implementation
////////////////////////////////////////////////////////////////////////////////////
downloadEventReporter::downloadEventReporter(PVMFProtocolEngineNode *aNode) : EventReporter(aNode)
{
    clear();
}

void downloadEventReporter::clear()
{
    iSendBufferStartInfoEvent		= false;
    iSendBufferCompleteInfoEvent	= false;
    iSendMovieAtomCompleteInfoEvent = false;
    iSendInitialDataReadyEvent		= false;
    iSendContentLengthEvent			= false;
    iSendContentTruncateEvent		= false;
    iSendContentTypeEvent			= false;
    iSendUnexpectedDataEvent		= false;
    iSendServerDisconnectEvent		= false;
    iPrevDownloadProgress = 0;

    EventReporter::clear();
}

void downloadEventReporter::sendDataReadyEvent()
{
    iNode->ReportInfoEvent(PVMFInfoDataReady, (OsclAny*)iNode->iProtocol->getDownloadRate());
    iSendInitialDataReadyEvent = true;
    iNode->iNodeTimer->cancel(WALL_CLOCK_TIMER_ID);
}

void downloadEventReporter::enableBufferingCompleteEvent()
{
    iSendBufferCompleteInfoEvent = false;
}

void downloadEventReporter::sendBufferStatusEvent()
{
    sendBufferStatusEventBody(true);
}

void downloadEventReporter::sendBufferStatusEventBody(const bool aForceToSend)
{
    if (!iStarted || !iNode->iDownloadProgess) return;

    uint32 aProgessPercent = 0;
    bool status = iNode->iDownloadProgess->getNewProgressPercent(aProgessPercent);
    if (!status && aForceToSend) aProgessPercent = iPrevDownloadProgress;

    if ((status || aForceToSend))
    {
        reportBufferStatusEvent(aProgessPercent);
        iPrevDownloadProgress = aProgessPercent;
        if (iPrevDownloadProgress < 100) iNode->iNodeTimer->start(BUFFER_STATUS_TIMER_ID);
    }
}

void downloadEventReporter::reportBufferStatusEvent(const uint32 aDownloadPercent)
{
    iNode->ReportInfoEvent(PVMFInfoBufferingStatus,
                           NULL,
                           PVMFPROTOCOLENGINENODEInfo_BufferingStatus,
                           (uint8*)(&aDownloadPercent),
                           sizeof(aDownloadPercent));
    LOGINFODATAPATH((0, "downloadEventReporter::reportBufferStatusEvent() DOWNLOAD PERCENTAGE: %d", aDownloadPercent));
}


bool downloadEventReporter::checkReportEvent(const uint32 downloadStatus)
{
    // PVMFInfoContentLength, PVMFErrContentTooLarge and PVMFInfoContentTruncated
    if (!checkContentInfoEvent(downloadStatus)) return false;

    // PVMFInfoBufferingStart, PVMFInfoBufferingStatus and PVMFInfoBufferingComplete event
    return checkBufferInfoEvent(downloadStatus);
}

bool downloadEventReporter::checkBufferInfoEvent(const uint32 downloadStatus)
{
    // PVMFInfoBufferingStart event
    if (!iSendBufferStartInfoEvent)
    {
        iNode->ReportInfoEvent(PVMFInfoBufferingStart); // first coming media data triggers sending PVMFInfoBufferingStart event
        iSendBufferStartInfoEvent = true;
        if (!isDownloadComplete(downloadStatus))
        {
            iNode->iNodeTimer->start(BUFFER_STATUS_TIMER_ID);
            return true;
        }
    }

    // PVMFInfoBufferingStatus and PVMFInfoBufferingComplete event
    if (iStarted && iNode->iDownloadProgess)
    {
        sendBufferStatusEventBody();

        // check and send buffer complete, data ready and unexpected data events
        checkBufferCompleteEvent(downloadStatus);
    }

    return true;
}

// check and send buffer complete, data ready and unexpected data events
void downloadEventReporter::checkBufferCompleteEvent(const uint32 downloadStatus)
{
    if (!iSendBufferCompleteInfoEvent && isDownloadComplete(downloadStatus))
    {
        uint32 aProgessPercent = 0;
        iNode->iDownloadProgess->getNewProgressPercent(aProgessPercent);
        if (aProgessPercent < 100)
        {
            aProgessPercent = 100;
            reportBufferStatusEvent(aProgessPercent);
        }

        // send buffer complete event
        iNode->ReportInfoEvent(PVMFInfoBufferingComplete, (OsclAny*)iNode->iProtocol->getDownloadSize());
        iSendBufferCompleteInfoEvent = true;

        // check and send data ready event
        if (!iSendInitialDataReadyEvent)
        {
            iNode->ReportInfoEvent(PVMFInfoDataReady);
            iSendInitialDataReadyEvent = true;
        }

        // clear the timer again because of download completion
        iNode->iNodeTimer->clear();
    }

    checkUnexpectedDataAndServerDisconnectEvent(downloadStatus);
}

void downloadEventReporter::checkUnexpectedDataAndServerDisconnectEvent(const uint32 downloadStatus)
{
    if (!iSendBufferCompleteInfoEvent && isDownloadComplete(downloadStatus))
    {
        // check and send unexpected data event
        checkUnexpectedDataEvent(downloadStatus);
    }

    // check and send unexpected data event, PVMFInfoUnexpectedData
    checkUnexpectedDataEvent(downloadStatus);

    // check and send server disconnect event, PVMFInfoSessionDisconnect
    // especially for no content length case
    checkServerDisconnectEvent(downloadStatus);
}

void downloadEventReporter::checkUnexpectedDataEvent(const uint32 downloadStatus)
{
    if (!iSendUnexpectedDataEvent && !iSendServerDisconnectEvent && // if PVMFInfoSessionDisconnect is sent, no need to send PVMFInfoUnexpectedData
            downloadStatus == PROCESS_SUCCESS_END_OF_MESSAGE_WITH_EXTRA_DATA)
    {
        iNode->ReportInfoEvent(PVMFInfoUnexpectedData);
        iSendUnexpectedDataEvent = true;
    }
}

void downloadEventReporter::checkServerDisconnectEvent(const uint32 downloadStatus)
{
    if (!iSendServerDisconnectEvent)
    {
        if (downloadStatus == PROCESS_SUCCESS_END_OF_MESSAGE_BY_SERVER_DISCONNECT ||
                (!iNode->iCfgFileContainer->getCfgFile()->IsNewSession() &&
                 downloadStatus == PROCESS_SUCCESS_END_OF_MESSAGE &&
                 iSendBufferCompleteInfoEvent))   // resume download and previous complete download
        {
            iNode->ReportInfoEvent(PVMFInfoSessionDisconnect);
            iSendServerDisconnectEvent = true;
        }
    }
}


bool downloadEventReporter::checkContentInfoEvent(const uint32 downloadStatus)
{
    // short-cut
    if (!needToCheckContentInfoEvent()) return true;

    // PVMFInfoContentType
    if (!iSendContentTypeEvent)
    {
        OSCL_HeapString<OsclMemAllocator> aContentType;
        if (iNode->iProtocol->getContentType(aContentType))
        {
            iNode->ReportInfoEvent(PVMFInfoContentType, (void*)(aContentType.get_cstr()));
            iSendContentTypeEvent = true;
        }
    }

    // check and send PVMFInfoContentLength or PVMFErrContentTooLarge
    if (!checkContentLengthOrTooLarge()) return false;

    // check and send PVMFInfoContentTruncated
    return checkContentTruncated(downloadStatus);
}

bool downloadEventReporter::checkContentLengthOrTooLarge()
{
    // PVMFInfoContentLength
    uint32 fileSize = iInterfacingObjectContainer->getFileSize();
    uint32 maxAllowedFileSize = iNode->iCfgFileContainer->getCfgFile()->GetMaxAllowedFileSize();

    if (!iSendContentLengthEvent && fileSize > 0)
    {
        iNode->ReportInfoEvent(PVMFInfoContentLength, (OsclAny*)fileSize);
        iSendContentLengthEvent = true;

        // PVMFErrContentTooLarge
        if (fileSize > maxAllowedFileSize)
        {
            // before error out, settle down the interaction with parser node
            iNode->iDownloadControl->checkResumeNotification(false);

            ProtocolStateErrorInfo aInfo(PVMFErrContentTooLarge);
            PVProtocolEngineNodeInternalEvent aEvent(PVProtocolEngineNodeInternalEventType_ProtocolStateError, (OsclAny*)(&aInfo));
            iNode->DispatchInternalEvent(&aEvent);
            return false;
        }
    }
    return true;
}

// check and send PVMFInfoContentTruncated
bool downloadEventReporter::checkContentTruncated(const uint32 downloadStatus)
{
    if (!iStarted) return true;
    if (!iSendContentTruncateEvent)
    {
        int32 status = isDownloadFileTruncated(downloadStatus);
        if (status > 0)
        {
            if (status == 1) iNode->ReportInfoEvent(PVMFInfoContentTruncated, (OsclAny*)iNode->iProtocol->getDownloadSize());
            if (status == 2)
            {
                iNode->ReportInfoEvent(PVMFInfoContentTruncated,
                                       (OsclAny*)iNode->iProtocol->getDownloadSize(),
                                       PVMFPROTOCOLENGINENODEInfo_TruncatedContentByServerDisconnect);
            }
            //iNode->Clear();
            iSendContentTruncateEvent = true;
        }
    }
    return true;
}

// return code: 0 - no truncation,
//				1 - truncation (download size >= maximum file size)
//				2 - truncation (download size < content length, server disconnect)
int32 downloadEventReporter::isDownloadFileTruncated(const uint32 downloadStatus)
{
    // 1. connection shutdown case with content length
    // if no content length and connection is done, this case should be download complete,
    // and the download size should be file size (this is NOT truncated case)
    uint32 currDownloadSize = iNode->iProtocol->getDownloadSize();
    uint32 contentLength = iInterfacingObjectContainer->getFileSize();
    if (isDownloadComplete(downloadStatus))
    {
        // short-cut: for resume download, if previous download is complete download, then return 0 (no truncation)
        if (!iNode->iCfgFileContainer->getCfgFile()->IsNewSession() && downloadStatus == PROCESS_SUCCESS_END_OF_MESSAGE) return 0;
        if (currDownloadSize < contentLength) return 2;
    }

    // 2. no content length case : download size > maximum file size (storage size)
    if (contentLength == 0)
    {
        if (downloadStatus == PROCESS_SUCCESS_END_OF_MESSAGE_TRUNCATED) return 1;
        uint32 maxFileSize = iNode->iCfgFileContainer->getCfgFile()->GetMaxAllowedFileSize();
        if (currDownloadSize <= maxFileSize) return 0;
        if (currDownloadSize > maxFileSize)  return 1;
    }
    return 0;
}


