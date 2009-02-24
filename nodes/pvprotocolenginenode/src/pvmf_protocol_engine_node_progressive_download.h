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

#ifndef PVMF_PROTOCOLENGINE_NODE_PROGRESSIVE_DOWNLOAD_H_INCLUDED
#define PVMF_PROTOCOLENGINE_NODE_PROGRESSIVE_DOWNLOAD_H_INCLUDED

#ifndef PVMF_PROTOCOLENGINE_NODE_DOWNLOAD_COMMON_H_INCLUDED
#include "pvmf_protocol_engine_node_download_common.h"
#endif

#define DEFAULT_STATE_NUMBER_FOR_DOWNLOAD_GET_REQUEST 1 // for progressive streaming or shoutcast


////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveDownloadContainer
////////////////////////////////////////////////////////////////////////////////////
class ProgressiveDownloadContainer : public DownloadContainer
{
    public:
        bool createProtocolObjects();
        bool needSocketReconnect();
        bool needCheckExtraDataComeIn()
        {
            return ((++iNumCheckExtraDataComeIn) == 1);
        }
        bool needCheckEOSAfterDisconnectSocket()
        {
            bool needCheck = ((++iNumCheckEOSAfterDisconnectSocket) == 1);
            if (needCheck && iNumCheckExtraDataComeIn == 0)
            {
                // if EOS happens after socket disconnect, then disable checking any extra data after this EOS
                // no need to send unexpected data event after server disconnect event gets sent
                iNumCheckExtraDataComeIn = 1;
            }
            return needCheck;
        }

        // constructor
        ProgressiveDownloadContainer(PVMFProtocolEngineNode *aNode = NULL);

    protected:
        PVMFStatus initImpl();
        bool initProtocol_SetConfigInfo();

    protected:
        uint32 iNumCheckExtraDataComeIn;
        uint32 iNumCheckEOSAfterDisconnectSocket;
};

////////////////////////////////////////////////////////////////////////////////////
//////	progressiveDownloadControl
////////////////////////////////////////////////////////////////////////////////////
class progressiveDownloadControl : public pvDownloadControl
{
    protected:
        bool isDlAlgoPreConditionMet(const uint32 aDownloadRate,
                                     const uint32 aDurationMsec,
                                     const uint32 aCurrDownloadSize,
                                     const uint32 aFileSize);

        // ret_val: 0 success, 1 playback rate is not close to clip bitrate, but the information is all available
        //			-1, related information, e.g. duration=0, size2time conversion is not available, is not available
        // called by isDlAlgoPreConditionMet()
        virtual int32 isPlaybackRateCloseToClipBitrate(const uint32 aDurationMsec,
                const uint32 aCurrDownloadSize,
                const uint32 aFileSize);

        // called by isPlaybackRateCloseToClipBitrate, using clip bitrate to do buffer time calculation
        bool isBufferingEnoughTime(const uint32 aCurrDownloadSize,
                                   const uint32 aBufferTimeLimitInSec,
                                   const uint32 aNPTInMS = 0xFFFFFFFF);

        // update duration by new playback rate, called by checkAutoResumeAlgoWithConstraint
        bool checkNewDuration(const uint32 aCurrDurationMsec, uint32 &aNewDurationMsec);

        // extra 4 sec buffering time
        bool approveAutoResumeDecisionShortCut(const uint32 aCurrDownloadSize,
                                               const uint32 aDurationMsec,
                                               const uint32 aPlaybackTimeMsec,
                                               uint32 &aPlaybackRemainingTimeMsec);

        // No constraint: for file size/clip duration/clip bitrate(i.e. playback rate), one of them must be unavailable, except
        // file size and clip duration are available, but clip bitrate is unavailable. This only applies on PDL
        bool checkAutoResumeAlgoNoConstraint(const uint32 aCurrDownloadSize,
                                             const uint32 aFileSize,
                                             uint32 &aDurationMsec);

        bool checkEstDurationAvailable(const uint32 aFileSize,
                                       uint32 &aDurationMsec);

        // updata download clock
        bool updateDownloadClock();

        bool isOutputBufferOverflow()
        {
            if (!iNodeOutput) return false;
            return (iNodeOutput->getAvailableOutputSize() == 0);
        }
};

////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveDownloadProgress
////////////////////////////////////////////////////////////////////////////////////
class PVDlCfgFileContainer;
class ProgressiveDownloadProgress : public DownloadProgress
{
    public:
        // constructor and destructor
        ProgressiveDownloadProgress()
        {
            reset();
        }
        ~ProgressiveDownloadProgress()
        {
            reset();
        }
        void setSupportObject(OsclAny *aDLSupportObject, DownloadControlSupportObjectType aType);

        // set download progress mode, time-based or byte-based
        void setDownloadProgressMode(DownloadProgressMode aMode = DownloadProgressMode_TimeBased)
        {
            iDownloadProgressMode = (uint32)aMode;
            if (aMode == DownloadProgressMode_TimeBased) iTimeBasedDownloadPercent = true;
        }

    protected:
        virtual bool calculateDownloadPercent(uint32 &aDownloadProgressPercent);
        bool updateDownloadClock(const bool aDownloadComplete);
        bool calculateDownloadPercentBody(uint32 &aDownloadProgressPercent, const uint32 aFileSize);
        // determine if download percent is time-based or byte-based
        bool checkDownloadPercentModeAndUpdateDLClock();
        uint32 getDownloadBytePercent(const uint32 aDownloadSize, const uint32 aFileSize);
        void reset();


    protected:
        PVDlCfgFileContainer *iCfgFileContainer;
        PVMFFormatProgDownloadSupportInterface *iProgDownloadSI;

        // user-specified download progress mode, 0 - time-based, 1 - byte-based
        uint32 iDownloadProgressMode;
        bool iTimeBasedDownloadPercent; // false means byte-based download percent
        uint32 iDownloadSize;
};

////////////////////////////////////////////////////////////////////////////////////
//////	UserAgentField and its derived class definition
////////////////////////////////////////////////////////////////////////////////////
class UserAgentFieldForProgDownload : public UserAgentField
{
    public:
        // constructor
        UserAgentFieldForProgDownload()
        {
            ;
        }
        UserAgentFieldForProgDownload(OSCL_wString &aUserAgent, const bool isOverwritable = false);
        UserAgentFieldForProgDownload(OSCL_String &aUserAgent, const bool isOverwritable = false);

    private:
        void getDefaultUserAgent(OSCL_String &aUserAgent);
};

////////////////////////////////////////////////////////////////////////////////////
//////	PVProgressiveDownloadCfgFileContainer
////////////////////////////////////////////////////////////////////////////////////
class PVProgressiveDownloadCfgFileContainer : public PVDlCfgFileContainer
{
    public:
        PVProgressiveDownloadCfgFileContainer(PVMFDownloadDataSourceContainer *aDataSource) : PVDlCfgFileContainer(aDataSource)
        {
            ;
        }

    private:
        PVMFStatus configCfgFile(OSCL_String &aUrl);
        PVDlCfgFile::TPVDLPlaybackMode convertToConfigFilePlaybackMode(PVMFDownloadDataSourceHTTP::TPVPlaybackControl aPlaybackMode);
};

#endif

