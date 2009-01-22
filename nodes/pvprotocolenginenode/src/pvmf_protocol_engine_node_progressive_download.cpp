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
#include "pvmf_protocol_engine_node_progressive_download.h"
#include "pvmf_protocolengine_node_tunables.h"
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

#define GET_10_PERCENT(x) ( ((x)>>3)-((x)>>6) ) // 1/8 - 1/64 = 0.109

////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveDownloadContainerFactory implementation
////////////////////////////////////////////////////////////////////////////////////
ProtocolContainer* ProgressiveDownloadContainerFactory::create(PVMFProtocolEngineNode *aNode)
{
    return OSCL_NEW(ProgressiveDownloadContainer, (aNode));
}


////////////////////////////////////////////////////////////////////////////////////
//////	ProgessiveDownloadContainer implementation
////////////////////////////////////////////////////////////////////////////////////
ProgressiveDownloadContainer::ProgressiveDownloadContainer(PVMFProtocolEngineNode *aNode) :
        DownloadContainer(aNode),
        iNumCheckExtraDataComeIn(0),
        iNumCheckEOSAfterDisconnectSocket(0)
{
    ;
}

bool ProgressiveDownloadContainer::createProtocolObjects()
{
    if (!ProtocolContainer::createProtocolObjects()) return false;

    iNode->iProtocol		 = OSCL_NEW(ProgressiveDownload, ());
    iNode->iNodeOutput		 = OSCL_NEW(pvHttpDownloadOutput, (iNode));
    iNode->iDownloadControl  = OSCL_NEW(progressiveDownloadControl, ());
    iNode->iDownloadProgess  = OSCL_NEW(ProgressiveDownloadProgress, ());
    iNode->iEventReport		 = OSCL_NEW(downloadEventReporter, (iNode));
    iNode->iCfgFileContainer = OSCL_NEW(PVProgressiveDownloadCfgFileContainer, (iNode->iDownloadSource));
    iNode->iUserAgentField	 = OSCL_NEW(UserAgentFieldForProgDownload, ());
    iNode->iDownloadSource	 = OSCL_NEW(PVMFDownloadDataSourceContainer, ());

    if (!iNode->iProtocol		|| !iNode->iNodeOutput  || !iNode->iDownloadControl  ||
            !iNode->iDownloadProgess || !iNode->iEventReport || !iNode->iCfgFileContainer ||
            !iNode->iUserAgentField  || !iNode->iDownloadSource) return false;
    return true;
}

bool ProgressiveDownloadContainer::needSocketReconnect()
{
    // currently, only disallow socket reconnect for head request disabled during prepare->start
    if (iNode->iInterfaceState == EPVMFNodePrepared &&
            iNode->iInterfacingObjectContainer->getHttpHeadRequestDisabled() &&
            !iForceSocketReconnect) return false;
    return true;
}

PVMFStatus ProgressiveDownloadContainer::initImpl()
{
    if (!iNode->iInterfacingObjectContainer->getHttpHeadRequestDisabled()) return ProtocolContainer::initImpl();

    if (!isObjectsReady())
    {
        return PVMFErrNotReady;
    }

    // initialize output object
    int32 status = initNodeOutput();
    if (status != PVMFSuccess) return status;

    // initialize protocol object
    if (!initProtocol()) return PVMFFailure;

    // initialize download control object
    initDownloadControl();

    return PVMFSuccess;
}

bool ProgressiveDownloadContainer::initProtocol_SetConfigInfo()
{
    OsclSharedPtr<PVDlCfgFile> aCfgFile = iNode->iCfgFileContainer->getCfgFile();
    if (aCfgFile.GetRep() == NULL) return false;
    aCfgFile->setHttpHeadRequestDisabled(iNode->iInterfacingObjectContainer->getHttpHeadRequestDisabled());
    return DownloadContainer::initProtocol_SetConfigInfo();
}


////////////////////////////////////////////////////////////////////////////////////
//////	progressiveDownloadControl implementation
////////////////////////////////////////////////////////////////////////////////////
bool progressiveDownloadControl::isDlAlgoPreConditionMet(const uint32 aDownloadRate,
        const uint32 aDurationMsec,
        const uint32 aCurrDownloadSize,
        const uint32 aFileSize)
{
    // first make sure initial download pre-conditions should be met
    if (!pvDownloadControl::isDlAlgoPreConditionMet(aDownloadRate, aDurationMsec, aCurrDownloadSize, aFileSize)) return false;

    // then heck the parser consuming rate is close to clip bitrate
    int32 status = isPlaybackRateCloseToClipBitrate(aDurationMsec, aCurrDownloadSize, aFileSize);
    if (status == 0) return true;  // parser node data consumption rate is close to clip bitrate
    if (status == -1) return true; // duration is not available, then don't wait and kicks off algo running

    return false; // parser node data consumption rate is not close to clip bitrate
}

// ret_val: 0,  success,
//			1,  playback rate is not close to clip bitrate, but the information is all available
//			-1, related information, e.g. duration=0, size2time conversion is not available, is not available
int32 progressiveDownloadControl::isPlaybackRateCloseToClipBitrate(const uint32 aDurationMsec,
        const uint32 aCurrDownloadSize,
        const uint32 aFileSize)
{
    if (aFileSize == 0 || aDurationMsec == 0 || iProgDownloadSI == NULL) return -1;

    uint32 aNPTInMS = 0;

    if (iProgDownloadSI->convertSizeToTime(aCurrDownloadSize, aNPTInMS) == 0)
    {

        if (aNPTInMS == 0) return 1;
        if (iClipByterate == 0) iClipByterate = divisionInMilliSec(aFileSize, aDurationMsec);	// aFileSize*1000/aDurationMsec
        uint32 aInstantByterate = divisionInMilliSec(aCurrDownloadSize, aNPTInMS);			// aCurrDownloadSize*1000/aNPTInMS
        LOGINFODATAPATH((0, "progressiveDownloadControl::isPlaybackRateCloseToClipBitrate, check Instant rate=%d(currDLSize=%d, NPTTimeMs=%d), clip bitrate=%d",
                         (aInstantByterate << 3), aCurrDownloadSize, aNPTInMS, (iClipByterate << 3)));
        uint32 diffByterate = (aInstantByterate >= iClipByterate ? aInstantByterate - iClipByterate : iClipByterate - aInstantByterate);
        if (diffByterate < GET_10_PERCENT(iClipByterate) || // OSCL_ABS(aInstantByterate-iClipByterate)/iClipByterate < 1/8-1/64=0.109
                isBufferingEnoughTime(aCurrDownloadSize, PVPROTOCOLENGINE_JITTER_BUFFER_SIZE_TIME, aNPTInMS))
        {
            if (isBufferingEnoughTime(aCurrDownloadSize, PVPROTOCOLENGINE_JITTER_BUFFER_SIZE_TIME, aNPTInMS))
            {
                return 0;
            }
            return 1;
        }
    }
    else
    {
        // in case of convertSizeToTime() not supported in parser node, but duration can be estimated and provided
        if (iClipByterate == 0) iClipByterate = divisionInMilliSec(aFileSize, aDurationMsec);
        if (isBufferingEnoughTime(aCurrDownloadSize, PVPROTOCOLENGINE_INIT_DOWNLOAD_TIME_THRESHOLD_WITH_CLIPBITRATE)) return 0;
    }

    return 1;
}

bool progressiveDownloadControl::isBufferingEnoughTime(const uint32 aCurrDownloadSize,
        const uint32 aBufferTimeLimitInSec,
        const uint32 aNPTInMS)
{
    if (aNPTInMS == 0xFFFFFFFF)
    {
        // use clip bitrate to calculate buffering time, instead of using convertSizeToTime()
        uint32 deltaSizeLimit = iClipByterate * aBufferTimeLimitInSec;
        return (aCurrDownloadSize >= iPrevDownloadSize + deltaSizeLimit);
    }
    else if (aNPTInMS > 0)
    {
        // convertSizeToTime() should be available
        if (iPrevDownloadSize == 0)
        {
            return (aNPTInMS >= aBufferTimeLimitInSec*1000);
        }
        else
        {
            uint32 aPrevNPTInMS = 0;
            if (iProgDownloadSI->convertSizeToTime(iPrevDownloadSize, aPrevNPTInMS) == 0)
            {
                return (aNPTInMS > aPrevNPTInMS && (aNPTInMS - aPrevNPTInMS) >= aBufferTimeLimitInSec*1000);
            }
        }
    }
    return false;
}

bool progressiveDownloadControl::checkNewDuration(const uint32 aCurrDurationMsec, uint32 &aNewDurationMsec)
{
    aNewDurationMsec = aCurrDurationMsec;
    if (aCurrDurationMsec > 0 && iClipByterate == 0)
    {
        if (iFileSize > 0) iClipByterate = divisionInMilliSec(iFileSize, aCurrDurationMsec);
    }

    if (iPlaybackByteRate > 0)
    {
        if (iPlaybackByteRate > iClipByterate)
        {
            uint32 averPlaybackRate = (iClipByterate + iPlaybackByteRate) / 2;
            aNewDurationMsec = divisionInMilliSec(iFileSize, averPlaybackRate); // aFileSize/averPlaybackRate*1000
        }
    }
    return true;
}

bool progressiveDownloadControl::approveAutoResumeDecisionShortCut(const uint32 aCurrDownloadSize,
        const uint32 aDurationMsec,
        const uint32 aPlaybackTimeMsec,
        uint32 &aPlaybackRemainingTimeMsec)
{
    if (!iProgDownloadSI || aDurationMsec == 0) return false;

    uint32 aNPTInMS = 0;
    if (iProgDownloadSI->convertSizeToTime(aCurrDownloadSize, aNPTInMS) == 0)
    {
        aPlaybackRemainingTimeMsec = aDurationMsec - aNPTInMS;
        if (aNPTInMS > PVPROTOCOLENGINE_JITTER_BUFFER_SIZE_TIME*2000 + aPlaybackTimeMsec)
            return true;
    }
    return false;
}

// No constraint: for file size/clip duration/clip bitrate(i.e. playback rate), one of them must be unavailable, except
// file size and clip duration are available, but clip bitrate is unavailable
bool progressiveDownloadControl::checkAutoResumeAlgoNoConstraint(const uint32 aCurrDownloadSize,
        const uint32 aFileSize,
        uint32 &aDurationMsec)
{
    // first check one exception: file size>0, duration=0, playbackRate>0, then we need to estimate the clip duration
    if (checkEstDurationAvailable(aFileSize, aDurationMsec)) return false;

    uint32 currDownloadSizeOfInterest = aCurrDownloadSize - iPrevDownloadSize; // use download size as the jitter buffer size
    if (iPlaybackByteRate > 0) currDownloadSizeOfInterest /= iPlaybackByteRate; // use playback time as the jitter buffer size
    else if (aFileSize > 0) currDownloadSizeOfInterest /= (aFileSize / PVPROTOCOLENGINE_DOWNLOAD_BYTE_PERCENTAGE_CONVERTION_100); // use download percentage as the jitter buffer size

    uint32 aJitterBufferSize = (iPlaybackByteRate > 0 ? PVPROTOCOLENGINE_JITTER_BUFFER_SIZE_TIME :
                                (aFileSize > 0 ? PVPROTOCOLENGINE_JITTER_BUFFER_SIZE_DLPERCENTAGE :
                                 PVPROTOCOLENGINE_JITTER_BUFFER_SIZE_BYTES));
    bool resumeOK = (currDownloadSizeOfInterest >= aJitterBufferSize);
    LOGINFODATAPATH((0, "progressiveDownloadControl::isResumePlayback()->checkAutoResumeAlgoNoConstraint(), resumeOK=%d, currDownloadSize=%d, prevDownloadSize=%d, currDownloadSizeOfInterest=%d, aJitterBufferSize=%d, file size=%d",
                     (uint32)resumeOK, aCurrDownloadSize, iPrevDownloadSize, currDownloadSizeOfInterest, aJitterBufferSize, aFileSize));

    return resumeOK;
}

bool progressiveDownloadControl::checkEstDurationAvailable(const uint32 aFileSize, uint32 &aDurationMsec)
{
    // check file size>0, duration=0, playbackRate>0, then we need to estimate the clip duration
    // and use the original algorithm
    if (iPlaybackByteRate > 0 && aFileSize > 0 && aDurationMsec == 0)
    {
        // calculate estimated duration
        aDurationMsec = divisionInMilliSec(aFileSize, iPlaybackByteRate);
        LOGINFODATAPATH((0, "progressiveDownloadControl::isResumePlayback()->checkEstDurationAvailable(), aFileSize=%d, aPlaybackByteRate=%d, estDurationMsec=%d",
                         aFileSize, iPlaybackByteRate, aDurationMsec));
        return true;
    }
    return false;
}


bool progressiveDownloadControl::updateDownloadClock()
{
    if (!iProgDownloadSI || !iProtocol) return false;

    // using size2time conversion to update download clock in PDL only applies to the case where
    // old algorithm gets running without engine clock input
    if (!iCurrentPlaybackClock)
    {
        uint32 aDownloadNPTTime = 0;
        // get download size from output object instead of protocol engine.
        // The download size from output object is the actual size for the data written to the file, and is exposed to user,
        // while the download size time from protocol engine is internally counted from socket and most likely
        // larger than that from output object

        if (iProgDownloadSI->convertSizeToTime(iNodeOutput->getCurrentOutputSize(), aDownloadNPTTime) != 0) return false;
        bool bOverflowFlag = false;
        iDlProgressClock->SetStartTime32(aDownloadNPTTime, PVMF_MEDIA_CLOCK_MSEC, bOverflowFlag);
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveDownloadProgress implementation
////////////////////////////////////////////////////////////////////////////////////
void ProgressiveDownloadProgress::setSupportObject(OsclAny *aDLSupportObject, DownloadControlSupportObjectType aType)
{
    switch (aType)
    {
        case DownloadControlSupportObjectType_ConfigFileContainer:
            iCfgFileContainer = (PVDlCfgFileContainer *)aDLSupportObject;
            break;

        case DownloadControlSupportObjectType_SupportInterface:
            iProgDownloadSI = (PVMFFormatProgDownloadSupportInterface*)aDLSupportObject;
            break;

        default:
            break;
    }

    DownloadProgress::setSupportObject(aDLSupportObject, aType);
}

bool ProgressiveDownloadProgress::updateDownloadClock(const bool aDownloadComplete)
{
    OSCL_UNUSED_ARG(aDownloadComplete);
    if (iProtocol) iDownloadSize = iNodeOutput->getCurrentOutputSize();
    if (iDownloadSize == 0) return false;
    return checkDownloadPercentModeAndUpdateDLClock();
}

bool ProgressiveDownloadProgress::checkDownloadPercentModeAndUpdateDLClock()
{
    // (1) if user specifies byte-based download percentage or no content length case,
    // no need to update download clock and download time
    if (iDownloadProgressMode > 0 ||
            iProtocol->getContentLength() == 0)
    {
        iTimeBasedDownloadPercent = false;
        return true;
    }

    // (2) if download and playback mode is not asap mode, i.e. download and play or download only,
    // then use byte-based download percent
    if (!iProgDownloadSI && iCfgFileContainer)
    {
        if (iCfgFileContainer->getPlaybackMode() != PVMFDownloadDataSourceHTTP::EAsap) iTimeBasedDownloadPercent = false;
        return true;
    }

    // (3) if parser node is not ready to do conversion from size to time,
    // then choose byte-based download percent
    if (iProgDownloadSI->convertSizeToTime(iDownloadSize, iDownloadNPTTime) != 0)
    {
        iTimeBasedDownloadPercent = false;
        return true;
    }
    return true;
}

bool ProgressiveDownloadProgress::calculateDownloadPercent(uint32 &aDownloadProgressPercent)
{
    return calculateDownloadPercentBody(aDownloadProgressPercent, iProtocol->getContentLength());
}

bool ProgressiveDownloadProgress::calculateDownloadPercentBody(uint32 &aDownloadProgressPercent, const uint32 aFileSize)
{
    if (iTimeBasedDownloadPercent)
    {
        return DownloadProgress::calculateDownloadPercent(aDownloadProgressPercent);
    }
    else
    {
        // byte-based download percentage
        aDownloadProgressPercent = iDownloadSize;
        if (aFileSize > 0)
        {
            aDownloadProgressPercent = getDownloadBytePercent(iDownloadSize, aFileSize);
            if (aDownloadProgressPercent > 100) aDownloadProgressPercent = 100;
            if (aDownloadProgressPercent == 100) iDownloadSize = aFileSize;
        }
        else
        {
            uint32 aMaxFileSize = iCfgFileContainer->getCfgFile()->GetMaxAllowedFileSize();
            if (aDownloadProgressPercent > aMaxFileSize) aDownloadProgressPercent = aMaxFileSize;
        }
    }
    return true;
}

// handle overflow issue for integer multiplication: downloadSize*100/fileSize
uint32 ProgressiveDownloadProgress::getDownloadBytePercent(const uint32 aDownloadSize, const uint32 aFileSize)
{
    uint32 aDownloadProgressPercent = aDownloadSize * PVPROTOCOLENGINE_DOWNLOAD_BYTE_PERCENTAGE_CONVERTION_100 / aFileSize; // 100
    if ((aDownloadSize >> PVPROTOCOLENGINE_DOWNLOAD_BYTE_PERCENTAGE_DLSIZE_LIMIT_RIGHT_SHIFT_FACTOR) > 0)   // right shift 25 bits => larger than 2^25, then *100 may cause overflow
    {
        aDownloadProgressPercent = (aDownloadSize >> PVPROTOCOLENGINE_DOWNLOAD_BYTE_PERCENTAGE_DLSIZE_RIGHTSHIFT_FACTOR)* // right shift 7 bits, 2^7>100 to avoid overflow
                                   PVPROTOCOLENGINE_DOWNLOAD_BYTE_PERCENTAGE_CONVERTION_100 /
                                   (aFileSize >> PVPROTOCOLENGINE_DOWNLOAD_BYTE_PERCENTAGE_DLSIZE_RIGHTSHIFT_FACTOR);
    }
    return aDownloadProgressPercent;
}

void ProgressiveDownloadProgress::reset()
{
    DownloadProgress::reset();

    iCfgFileContainer = NULL;
    iProgDownloadSI   = NULL;
    iTimeBasedDownloadPercent = false;
    iDownloadProgressMode = (uint32)DownloadProgressMode_ByteBased;
}

////////////////////////////////////////////////////////////////////////////////////
//////	UserAgentField implementation
////////////////////////////////////////////////////////////////////////////////////
UserAgentFieldForProgDownload::UserAgentFieldForProgDownload(OSCL_wString &aUserAgent, const bool isOverwritable) :
        UserAgentField(aUserAgent, isOverwritable)
{
    ;
}

UserAgentFieldForProgDownload::UserAgentFieldForProgDownload(OSCL_String &aUserAgent, const bool isOverwritable) :
        UserAgentField(aUserAgent, isOverwritable)
{
    ;
}

void UserAgentFieldForProgDownload::getDefaultUserAgent(OSCL_String &aUserAgent)
{
    OSCL_HeapString<OsclMemAllocator> defaultUserAgent(PDL_HTTP_USER_AGENT); // defined in pvmf_protocolengine_node_tunables.h
    aUserAgent = defaultUserAgent;
}


////////////////////////////////////////////////////////////////////////////////////
//////	PVProgressiveDownloadCfgFileContainer implementation
////////////////////////////////////////////////////////////////////////////////////

PVMFStatus PVProgressiveDownloadCfgFileContainer::configCfgFile(OSCL_String &aUrl)
{
    // playback mode and proxy
    iPlaybackMode = (PVMFDownloadDataSourceHTTP::TPVPlaybackControl)iDataSource->iPlaybackControl;
    iCfgFileObj->SetPlaybackMode(convertToConfigFilePlaybackMode(iPlaybackMode));
    iCfgFileObj->SetProxyName(iDataSource->iProxyName);
    iCfgFileObj->SetProxyPort(iDataSource->iProxyPort);

    // user agent
    OSCL_FastString user_agent(PDL_HTTP_USER_AGENT); // defined in pvmf_protocolengine_node_tunables.h
    iCfgFileObj->SetUserAgent(user_agent);

    // user-id and password
    if (iDataSource->iUserID.get_size() > 0)	  iCfgFileObj->SetUserId(iDataSource->iUserID);
    if (iDataSource->iUserPasswd.get_size() > 0) iCfgFileObj->SetUserAuth(iDataSource->iUserPasswd);

    iCfgFileObj->SetDownloadType(false);//not fasttrack
    return PVDlCfgFileContainer::configCfgFile(aUrl);
}

PVDlCfgFile::TPVDLPlaybackMode PVProgressiveDownloadCfgFileContainer::convertToConfigFilePlaybackMode(PVMFDownloadDataSourceHTTP::TPVPlaybackControl aPlaybackMode)
{
    OSCL_UNUSED_ARG(aPlaybackMode);

    PVDlCfgFile::TPVDLPlaybackMode mode = PVDlCfgFile::EPVDL_ASAP;
    switch (iPlaybackMode)
    {
        case PVMFDownloadDataSourceHTTP::EAsap:
            mode = PVDlCfgFile::EPVDL_ASAP;
            break;
        case PVMFDownloadDataSourceHTTP::EAfterDownload:
            mode = PVDlCfgFile::EPVDL_PLAYBACK_AFTER_DOWNLOAD;
            break;
        case PVMFDownloadDataSourceHTTP::ENoPlayback:
            mode = PVDlCfgFile::EPVDL_DOWNLOAD_ONLY;
            break;
        default:
            break;
    }
    return mode;
}

