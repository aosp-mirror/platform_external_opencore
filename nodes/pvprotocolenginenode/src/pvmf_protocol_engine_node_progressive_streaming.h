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

#ifndef PVMF_PROTOCOLENGINE_NODE_PROGRESSIVE_STREAMING_H_INCLUDED
#define PVMF_PROTOCOLENGINE_NODE_PROGRESSIVE_STREAMING_H_INCLUDED

#ifndef PVMF_PROTOCOLENGINE_NODE_DOWNLOAD_COMMON_H_INCLUDED
#include "pvmf_protocol_engine_node_progressive_download.h"
#endif


////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveStreamingContainer
////////////////////////////////////////////////////////////////////////////////////
class ProgressiveStreamingContainer : public ProgressiveDownloadContainer
{
    public:
        bool createProtocolObjects();
        PVMFStatus doStop();
        PVMFStatus doSeek(PVMFProtocolEngineNodeCommand& aCmd);
        bool completeRepositionRequest();
        bool doInfoUpdate(const uint32 downloadStatus);
        void enableInfoUpdate(const bool aEnabled = true)
        {
            iEnableInfoUpdate = aEnabled;
        }

        // constructor
        ProgressiveStreamingContainer(PVMFProtocolEngineNode *aNode = NULL);

    private:
        // called by DoSeek()
        uint32 getSeekOffset(PVMFProtocolEngineNodeCommand& aCmd);
        PVMFStatus doSeekBody(uint32 aNewOffset);
        void updateDownloadControl(const bool isDownloadComplete = false);
        bool needToCheckResumeNotificationMaually();

    private:
        bool iEnableInfoUpdate;
};


////////////////////////////////////////////////////////////////////////////////////
//////	pvProgressiveStreamingOutput
////////////////////////////////////////////////////////////////////////////////////
class pvProgressiveStreamingOutput : public pvHttpDownloadOutput
{
    public:
        int32 flushData(const uint32 aOutputType = NodeOutputType_InputPortForData);
        void discardData(const bool aNeedReopen = false)
        {
            OSCL_UNUSED_ARG(aNeedReopen);
            return;
        }
        bool releaseMemFrag(OsclRefCounterMemFrag* aFrag);
        // for new data stream APIs
        void setContentLength(uint32 aLength);
        void dataStreamCommandCompleted(const PVMFCmdResp& aResponse);
        void setDataStreamSourceRequestObserver(PvmiDataStreamRequestObserver* aObserver)
        {
            iSourceRequestObserver = aObserver;
        }
        void flushDataStream();
        bool seekDataStream(const uint32 aSeekOffset);

        // constructor and destructor
        pvProgressiveStreamingOutput(PVMFProtocolEngineNodeOutputObserver *aObserver = NULL);
        virtual ~pvProgressiveStreamingOutput()
        {
            flushDataStream();
        }

    private:
        int32 openDataStream(OsclAny* aInitInfo);
        // write data to data stream object
        // return~0=0xffffffff for error.
        uint32 writeToDataStream(OUTPUT_DATA_QUEUE &aOutputQueue, PENDING_OUTPUT_DATA_QUEUE &aPendingOutputQueue);

    private:
        PvmiDataStreamRequestObserver* iSourceRequestObserver;
};


////////////////////////////////////////////////////////////////////////////////////
//////	progressiveStreamingControl
////////////////////////////////////////////////////////////////////////////////////
class progressiveStreamingControl : public progressiveDownloadControl
{
    public:
        void requestResumeNotification(const uint32 currentNPTReadPosition, bool& aDownloadComplete, bool& aNeedSendUnderflowEvent);

        // clear several fields for progressive playback repositioning
        void clearPerRequest();

        // constructor
        progressiveStreamingControl();
};

////////////////////////////////////////////////////////////////////////////////////
//////	ProgressiveStreamingProgress
////////////////////////////////////////////////////////////////////////////////////
class ProgressiveStreamingProgress : public ProgressiveDownloadProgress
{
    public:
        // constructor
        ProgressiveStreamingProgress() : ProgressiveDownloadProgress(), iContentLength(0)
        {
            ;
        }

    private:
        bool calculateDownloadPercent(uint32 &aDownloadProgressPercent);

    private:
        uint32 iContentLength;
};

////////////////////////////////////////////////////////////////////////////////////
//////	PVProgressiveStreamingCfgFileContainer
////////////////////////////////////////////////////////////////////////////////////
class PVProgressiveStreamingCfgFileContainer : public PVProgressiveDownloadCfgFileContainer
{
    public:
        PVProgressiveStreamingCfgFileContainer(PVMFDownloadDataSourceContainer *aDataSource) : PVProgressiveDownloadCfgFileContainer(aDataSource)
        {
            ;
        }

    private:
        // no need to save data to config file
        void saveConfig()
        {
            ;
        }
};

////////////////////////////////////////////////////////////////////////////////////
//////	progressiveStreamingEventReporter
////////////////////////////////////////////////////////////////////////////////////
class progressiveStreamingEventReporter : public downloadEventReporter
{
    public:
        // constructor
        progressiveStreamingEventReporter(PVMFProtocolEngineNode *aNode) : downloadEventReporter(aNode)
        {
            ;
        }

    private:
        // in case of progressive streaming, currently do not send PVMFInfoSessionDisconnect event
        void checkServerDisconnectEvent(const uint32 downloadStatus)
        {
            OSCL_UNUSED_ARG(downloadStatus);
        }
        // in case of progressive streaming, add buffer fullness information into buffer status report
        void reportBufferStatusEvent(const uint32 aDownloadPercent);
        // called by reportBufferStatusEvent
        uint32 getBufferFullness();
};


#endif

