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
#ifndef PVMF_SM_FSP_PVR_BASE_IMPL_H
#define PVMF_SM_FSP_PVR_BASE_IMPL_H

#ifndef PVMF_SM_FSP_BASE_IMPL_H
#include "pvmf_sm_fsp_base_impl.h"
#endif

#ifndef __SDP_INFO_H__
#include "sdp_info.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef PVMF_SM_FSP_PVR_CAPANDCONFIG_H
#include "pvmf_sm_fsp_pvr_capandconfig.h"
#endif

#ifndef PVMF_JITTER_BUFFER_PORT_H_INCLUDED
#include "pvmf_jitter_buffer_port.h"
#endif

#ifndef PVMF_DATA_SOURCE_PACKETSOURCE_H_INCLUDED
#include "pvmf_data_source_packetsource.h"
#endif

#define PVMF_SM_FSP_PVR_BASE_LOGSTACKTRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iSMBaseLogger,PVLOGMSG_STACK_TRACE,m);
#define PVMF_SM_FSP_PVR_BASE_LOGDEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iSMBaseLogger,PVLOGMSG_DEBUG,m);
#define PVMF_SM_FSP_PVR_BASE_LOGERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iSMBaseLogger,PVLOGMSG_ERR,m);
#define PVMF_SM_FSP_PVR_BASE_LOGCMDSEQ(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iCommandSeqLogger,PVLOGMSG_STACK_TRACE,m);
#define PVMF_SM_FSP_PVR_BASE_LOGINFO(m) PVMF_SM_FSP_PVR_BASE_LOGINFOMED(m)
#define PVMF_SM_FSP_PVR_BASE_LOGINFOMED(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG,iLogger,PVLOGMSG_INFO,m);
#define PVMF_SM_FSP_PVR_BASE_LOGCOMMANDREPOS(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iLogger,PVLOGMSG_DEBUG,m);

class PVMFPVRBaseTrackInfo
{
    public:
        PVMFPVRBaseTrackInfo()
        {
            trackID = 0;
            rdtStreamID = 0;
            portTag = 0;
            bitRate = 0;
            trackTimeScale = 1;
            iNetworkNodePort = NULL;
            iJitterBufferInputPort = NULL;
            iJitterBufferOutputPort = NULL;
            iMediaLayerInputPort = NULL;
            iMediaLayerOutputPort = NULL;
            iJitterBufferRTCPPort = NULL;
            iNetworkNodeRTCPPort = NULL;
            iSessionControllerOutputPort = NULL;
            iSessionControllerFeedbackPort = NULL;
            iRTPSocketID = 0;
            iRTCPSocketID = 0;
            iRateAdaptation = false;
            iRateAdaptationFeedBackFrequency = 0;
            iFormatType = PVMF_MIME_FORMAT_UNKNOWN;
            iRTCPBwSpecified = false;
            iTrackDisable = false;
            iPVRNodeInputPort = NULL;
            iPVRNodeOutputPort = NULL;
            iPVRNodeRTCPPort = NULL;
        };

        PVMFPVRBaseTrackInfo(const PVMFPVRBaseTrackInfo& a)
        {
            trackID = a.trackID;
            rdtStreamID = a.rdtStreamID;
            portTag = a.portTag;
            bitRate = a.bitRate;
            trackTimeScale = a.trackTimeScale;
            iTrackConfig = a.iTrackConfig;
            iTransportType = a.iTransportType;
            iFormatType = a.iFormatType;
            iMimeType = a.iMimeType;
            iNetworkNodePort = a.iNetworkNodePort;
            iJitterBufferInputPort = a.iJitterBufferInputPort;
            iJitterBufferOutputPort = a.iJitterBufferOutputPort;
            iMediaLayerInputPort = a.iMediaLayerInputPort;
            iMediaLayerOutputPort = a.iMediaLayerOutputPort;
            iJitterBufferRTCPPort = a.iJitterBufferRTCPPort;
            iNetworkNodeRTCPPort = a.iNetworkNodeRTCPPort;
            iSessionControllerOutputPort = a.iSessionControllerOutputPort;
            iSessionControllerFeedbackPort = a.iSessionControllerFeedbackPort;
            iRTPSocketID = a.iRTPSocketID;
            iRTCPSocketID = a.iRTCPSocketID;
            iRateAdaptation = a.iRateAdaptation;
            iRateAdaptationFeedBackFrequency = a.iRateAdaptationFeedBackFrequency;
            iRTCPBwSpecified = a.iRTCPBwSpecified;
            iTrackDisable = a.iTrackDisable;
            iRR = a.iRR;
            iRS = a.iRS;
            iPVRNodeInputPort = a.iPVRNodeInputPort;
            iPVRNodeOutputPort = a.iPVRNodeOutputPort;
            iPVRNodeRTCPPort = a.iPVRNodeRTCPPort;

        };

        PVMFPVRBaseTrackInfo& operator=(const PVMFPVRBaseTrackInfo& a)
        {
            if (&a != this)
            {
                trackID = a.trackID;
                rdtStreamID = a.rdtStreamID;
                portTag = a.portTag;
                bitRate = a.bitRate;
                trackTimeScale = a.trackTimeScale;
                iTrackConfig = a.iTrackConfig;
                iTransportType = a.iTransportType;
                iFormatType = a.iFormatType;
                iMimeType = a.iMimeType;
                iNetworkNodePort = a.iNetworkNodePort;
                iJitterBufferInputPort = a.iJitterBufferInputPort;
                iJitterBufferOutputPort = a.iJitterBufferOutputPort;
                iMediaLayerInputPort = a.iMediaLayerInputPort;
                iMediaLayerOutputPort = a.iMediaLayerOutputPort;
                iJitterBufferRTCPPort = a.iJitterBufferRTCPPort;
                iNetworkNodeRTCPPort = a.iNetworkNodeRTCPPort;
                iSessionControllerOutputPort = a.iSessionControllerOutputPort;
                iSessionControllerFeedbackPort = a.iSessionControllerFeedbackPort;
                iRTPSocketID = a.iRTPSocketID;
                iRTCPSocketID = a.iRTCPSocketID;
                iRateAdaptation = a.iRateAdaptation;
                iRateAdaptationFeedBackFrequency = a.iRateAdaptationFeedBackFrequency;
                iRTCPBwSpecified = a.iRTCPBwSpecified;
                iTrackDisable = a.iTrackDisable;
                iRR = a.iRR;
                iRS = a.iRS;
                iPVRNodeInputPort = a.iPVRNodeInputPort;
                iPVRNodeOutputPort = a.iPVRNodeOutputPort;
                iPVRNodeRTCPPort = a.iPVRNodeRTCPPort;

            }
            return *this;
        };

        virtual ~PVMFPVRBaseTrackInfo() {};

        uint32 trackID;
        uint32 rdtStreamID;
        uint32 portTag;
        uint32 bitRate;
        uint32 trackTimeScale;
        OsclRefCounterMemFrag iTrackConfig;
        OSCL_HeapString<OsclMemAllocator> iTransportType;
        PVMFFormatType iFormatType;
        OSCL_HeapString<OsclMemAllocator> iMimeType;
        PVMFPortInterface* iNetworkNodePort;
        PVMFPortInterface* iJitterBufferInputPort;
        PVMFPortInterface* iJitterBufferOutputPort;
        PVMFPortInterface* iMediaLayerInputPort;
        PVMFPortInterface* iMediaLayerOutputPort;
        PVMFPortInterface* iJitterBufferRTCPPort;
        PVMFPortInterface* iNetworkNodeRTCPPort;
        PVMFPortInterface* iSessionControllerOutputPort;
        PVMFPortInterface* iSessionControllerFeedbackPort;

        uint32 iRTPSocketID;
        uint32 iRTCPSocketID;
        bool   iRateAdaptation;
        uint32 iRateAdaptationFeedBackFrequency;

        // RTCP bandwidth related
        bool   iRTCPBwSpecified;
        uint32 iRR;
        uint32 iRS;

        // PVR Related
        PVMFPortInterface* iPVRNodeInputPort;
        PVMFPortInterface* iPVRNodeOutputPort;
        PVMFPortInterface* iPVRNodeRTCPPort;

        //Check track disable or not
        bool iTrackDisable;


};
typedef Oscl_Vector<PVMFPVRBaseTrackInfo, OsclMemAllocator> PVMFPVRBaseTrackInfoVector;

class PVMFPVRExtInterface;

// This is a base class for plugins: PVR File Playback, Broadcast PVR, and Unicast + PVR
class PVMFSMFSPPVRBase : public PVMFSMFSPBaseNode
{
    public:
        PVMFSMFSPPVRBase(int32 aPriority);
        // Pure Virtual functions. Implementations are done in the plugins: PVR File Playback, Broadcast, and Unicast + PVR
        virtual void CompleteGraphConstruct() = 0;
        virtual void CompletePause() = 0;
        virtual void CompleteInit() = 0;
        virtual	void CompleteStart() = 0;
        virtual void CreateChildNodes() = 0;
        virtual void DestroyChildNodes() = 0;
        virtual PVMFStatus DoGraphConstruct() = 0;
        virtual void DoSetDataSourcePosition(PVMFSMFSPBaseNodeCommand&) = 0;
        virtual void DoSetDataSourcePositionPlayList(PVMFSMFSPBaseNodeCommand&) = 0;

        virtual void HandleChildNodeCommandCompletion(const PVMFCmdResp& , bool&) = 0;

        virtual void HandleJitterBufferCommandCompleted(const PVMFCmdResp&, bool& aPerformErrHandling) = 0;

        virtual PVMFStatus InitMetaData() = 0;
        virtual PVMFStatus ProcessSDP() = 0;

        virtual PVMFStatus SetSourceInitializationData(OSCL_wString& aSourceURL,
                PVMFFormatType& aSourceFormat,
                OsclAny* aSourceData) = 0;

        virtual PVMFStatus DoPreInit(PVMFSMFSPBaseNodeCommand& aCmd) = 0;
        virtual void DoInit(PVMFSMFSPBaseNodeCommand&) = 0;
        virtual void DoPause(PVMFSMFSPBaseNodeCommand&) = 0;
        virtual void DoStart(PVMFSMFSPBaseNodeCommand&) = 0;
        virtual void HandleNodeInformationalEvent(const PVMFAsyncEvent& aEvent) = 0;
        virtual void QueryChildNodesExtentionInterface() = 0;
        virtual	PVMFStatus VerifyAndSetConfigParameter(int index, PvmiKvp& aParameter, bool set) = 0;
        virtual bool GraphConnect() = 0;

        // These functions are common to the plugins: PVR File Playback, Broadcast PVR, and Unicast + PVR

        void BypassError();
        bool CheckChildrenNodesInit();
        bool CheckChildrenNodesPause();
        bool CheckChildrenNodesPrepare();
        bool CheckChildrenNodesStart();
        bool CheckChildrenNodesStop();
        void CleanUp();
        void PopulatePayloadParserRegistry();
        void ResetNodeParams(bool aReleaseMemory = true);

        void CompletePrepare();

        void CompleteStop();
        PVMFStatus ComputeSkipTimeStamp(PVMFTimestamp aTargetNPT,
                                        PVMFTimestamp aActualNPT,
                                        PVMFTimestamp aActualMediaDataTS,
                                        PVMFTimestamp& aSkipTimeStamp,
                                        PVMFTimestamp& aStartNPT);
        PVMFStatus ConnectPortPairs(PVMFPortInterface* aPort1, PVMFPortInterface* aPort2);
        void Construct();
        virtual void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        virtual void DeleteContext(PvmiMIOSession aSession,
                                   PvmiCapabilityContext& aContext);
        void DestroyPayloadParserRegistry();
        PVMFCommandId DoGetMetadataKeys(PVMFSMFSPBaseNodeCommand& aCmd);
        PVMFCommandId DoGetMetadataValues(PVMFSMFSPBaseNodeCommand& aCmd);


        void DoPrepare(PVMFSMFSPBaseNodeCommand&);
        void DoQueryDataSourcePosition(PVMFSMFSPBaseNodeCommand&);


        void DoReleasePort(PVMFSMFSPBaseNodeCommand&);
        void DoRequestPort(PVMFSMFSPBaseNodeCommand&);

        void DoStop(PVMFSMFSPBaseNodeCommand&);
        PVMFPVRBaseTrackInfo* FindTrackInfo(uint32 tag);
        void GetAcutalMediaTSAfterSeek();
        virtual uint32 getCapabilityMetric(PvmiMIOSession aSession);
        virtual PVMFStatus GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo);
        virtual uint32 GetNumMetadataKeys(char* aQueryKeyString = NULL);
        virtual uint32 GetNumMetadataValues(PVMFMetadataList& aKeyList);
        virtual PVMFStatus getParametersSync(PvmiMIOSession aSession,
                                             PvmiKeyType aIdentifier,
                                             PvmiKvp*& aParameters,
                                             int& aNumParamElements,
                                             PvmiCapabilityContext aContext);
        virtual PVMFStatus GetPVRPluginSpecificValues(PVMFSMFSPBaseNodeCommand& aCmd);

        void HandleMediaLayerCommandCompleted(const PVMFCmdResp&, bool& aPerformErrHandling);

        bool IsFSPInternalCmd(PVMFCommandId aId);
        virtual void NodeCommandCompleted(const PVMFCmdResp& aResponse);
        PVMFStatus PopulateAvailableMetadataKeys();
        virtual void PopulateDRMInfo();
        bool PopulateTrackInfoVec();
        virtual bool ProcessCommand(PVMFSMFSPBaseNodeCommand&);

        void ReleaseChildNodesExtentionInterface();
        virtual PVMFStatus ReleaseNodeMetadataKeys(PVMFMetadataList& aKeyList,
                uint32 aStartingKeyIndex,
                uint32 aEndKeyIndex);
        virtual PVMFStatus ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
                uint32 aStartingValueIndex,
                uint32 aEndValueIndex);
        virtual PVMFStatus releaseParameters(PvmiMIOSession aSession,
                                             PvmiKvp* aParameters,
                                             int num_elements);

        bool RequestJitterBufferPorts(int32 portType, uint32 &numPortsRequested);
        bool RequestMediaLayerPorts(int32 portType, uint32& numPortsRequested);


        void ResetStopCompleteParams();
        virtual PVMFStatus SelectTracks(PVMFMediaPresentationInfo& aInfo);
        virtual PVMFStatus SetClientPlayBackClock(PVMFMediaClock* aClientClock);
        virtual void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                          PvmiKvp* aParameters, int num_parameter_elements);
        virtual PVMFStatus SetEstimatedServerClock(PVMFMediaClock* aClientClock);
        void setJitterBufferDurationInMilliSeconds(uint32 duration);
        virtual void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver);
        virtual void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                       int num_elements, PvmiKvp * & aRet_kvp);

        virtual PVMFStatus verifyParametersSync(PvmiMIOSession aSession,
                                                PvmiKvp* aParameters,
                                                int num_elements);
        bool SkipThisNodeResume(int32 aNodeTag);
        bool SkipThisNodePause(int32 aNodeTag);
        void SetPVRTrackParams(uint32 aMediaTrackId, const PvmfMimeString* aMimeType,
                               uint32 aTimescale, uint32 aBitrate);
        void SetPVRTrackRTPParams(const PvmfMimeString* aMimeType, bool   aSeqNumBasePresent,
                                  uint32 aSeqNumBase, bool   aRTPTimeBasePresent,
                                  uint32 aRTPTimeBase, uint32 aNPTInMS);
        void SetPVRSdpText(OsclRefCounterMemFrag& aSDPText);
        uint32 GetJitterBufferMemPoolSize(PVMFJitterBufferNodePortTag aJBNodePortTag, PVMFPVRBaseTrackInfo& aRTSPPlusPVRTrackInfo);
        bool SetPVRPlaybackRange();
        PVMFStatus GetConfigParameter(PvmiKvp*& aParameters, int& aNumParamElements,
                                      int32 aIndex, PvmiKvpAttr reqattr);
        void DoQueryInterface(PVMFSMFSPBaseNodeCommand&);

    protected:
        virtual bool RequestUsageComplete()
        {
            return true;
        };

    protected:

        PVMFPVRBaseTrackInfoVector	iTrackInfoVec;
        OsclSharedPtr<SDPInfo> iSdpInfo;

        uint32 iJitterBufferDurationInMilliSeconds;

        PVMFMediaPresentationInfo iCompleteMediaPresetationInfo;
        PVMFMediaPresentationInfo iSelectedMediaPresetationInfo;
        bool iPVREnabled;
        bool iOutOfBandEOS;
        PVMFPVRExtInterface* iPVRExtInterface;
        PVMFPVRControl* iPVRControl;
        PVInterface* iPVRQueryInterface;
        bool oAutoReposition;
        bool iJumpingToLive;

};


#endif
