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
#ifndef PVMF_JITTER_BUFFER_EXT_INTERFACE_H_INCLUDED
#define PVMF_JITTER_BUFFER_EXT_INTERFACE_H_INCLUDED

#ifndef OSCL_REFCOUNTER_MEMFRAG_H_INCLUDED
#include "oscl_refcounter_memfrag.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif
#ifndef PVMF_MEDIA_CLOCK_H_INCLUDED
#include "pvmf_media_clock.h"
#endif
#ifndef PV_UUID_H_INCLUDED
#include "pv_uuid.h"
#endif
#ifndef PV_INTERFACE_H
#include "pv_interface.h"
#endif
#ifndef PVMF_PORT_INTERFACE_H_INCLUDED
#include "pvmf_port_interface.h"
#endif
#ifndef RTSP_TIME_FORMAT_H
#include "rtsp_time_formats.h"
#endif
#ifndef PVMF_STREAMING_BUFFER_ALLOCATORS_H_INCLUDED
#include "pvmf_streaming_buffer_allocators.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

class PvmfPortBaseImpl;
//memory allocator type for this node.
typedef OsclMemAllocator PVMFJitterBufferNodeAllocator;

class PVMFSharedSocketDataBufferAlloc;

enum PVMFJitterBufferFireWallPacketFormat
{
    PVMF_JB_FW_PKT_FORMAT_RTP,
    PVMF_JB_FW_PKT_FORMAT_PV
};

class PVMFJitterBufferFireWallPacketInfo
{
    public:
        PVMFJitterBufferFireWallPacketInfo()
        {
            iServerRoundTripDelayInMS =
                PVMF_JITTER_BUFFER_NODE_FIREWALL_PKT_DEFAULT_SERVER_RESPONSE_TIMEOUT_IN_MS;
            iNumAttempts =
                PVMF_JITTER_BUFFER_NODE_DEFAULT_FIREWALL_PKT_ATTEMPTS;
            iFormat = PVMF_JB_FW_PKT_FORMAT_RTP;
        };

        virtual ~PVMFJitterBufferFireWallPacketInfo()
        {
        };

        uint32 iServerRoundTripDelayInMS;
        uint32 iNumAttempts;
        PVMFJitterBufferFireWallPacketFormat iFormat;
};

class PVMFJitterBufferExtensionInterface : public PVInterface
{
    public:
        OSCL_IMPORT_REF virtual void setRTCPIntervalInMicroSecs(uint32 aRTCPInterval) = 0;
        OSCL_IMPORT_REF virtual bool setPortParams(PVMFPortInterface* aPort,
                uint32 aTimeScale,
                uint32 aBitRate,
                OsclRefCounterMemFrag& aConfig,
                bool aRateAdaptation = false,
                uint32 aRateAdaptationFeedBackFrequency = 0) = 0;
        OSCL_IMPORT_REF virtual bool setPlayRange(int32 aStartTimeInMS,
                int32 aStopTimeInMS,
                bool oPlayAfterASeek,
                bool aStopTimeAvailable = true) = 0;
        OSCL_IMPORT_REF virtual void setPlayBackThresholdInMilliSeconds(uint32 threshold) = 0;
        OSCL_IMPORT_REF virtual void setJitterBufferRebufferingThresholdInMilliSeconds(uint32 aThreshold) = 0;
        OSCL_IMPORT_REF virtual void getJitterBufferRebufferingThresholdInMilliSeconds(uint32& aThreshold) = 0;
        OSCL_IMPORT_REF virtual void setJitterBufferDurationInMilliSeconds(uint32 duration) = 0;
        OSCL_IMPORT_REF virtual void getJitterBufferDurationInMilliSeconds(uint32& duration) = 0;

        OSCL_IMPORT_REF virtual void setEarlyDecodingTimeInMilliSeconds(uint32 duration) = 0;
        OSCL_IMPORT_REF virtual void setBurstThreshold(float burstThreshold) = 0;

        //While in buffering/start state, Jitter Buffer node expects its upstream peer node to send media msg at its input port in duration < inactivity duration
        OSCL_IMPORT_REF virtual void setMaxInactivityDurationForMediaInMs(uint32 duration) = 0;
        OSCL_IMPORT_REF virtual void getMaxInactivityDurationForMediaInMs(uint32& duration) = 0;

        OSCL_IMPORT_REF virtual void setClientPlayBackClock(PVMFMediaClock* clientClock) = 0;
        OSCL_IMPORT_REF virtual bool PrepareForRepositioning(bool oUseExpectedClientClockVal = false,
                uint32 aExpectedClientClockVal = 0) = 0;
        OSCL_IMPORT_REF virtual bool setPortSSRC(PVMFPortInterface* aPort, uint32 aSSRC) = 0;
        OSCL_IMPORT_REF virtual bool setPortRTPParams(PVMFPortInterface* aPort,
                bool   aSeqNumBasePresent,
                uint32 aSeqNumBase,
                bool   aRTPTimeBasePresent,
                uint32 aRTPTimeBase,
                bool   aNPTTimeBasePresent,
                uint32 aNPTInMS,
                bool oPlayAfterASeek = false) = 0;
        OSCL_IMPORT_REF virtual bool setPortRTCPParams(PVMFPortInterface* aPort,
                int aNumSenders,
                uint32 aRR,
                uint32 aRS) = 0;
        OSCL_IMPORT_REF virtual PVMFTimestamp getActualMediaDataTSAfterSeek() = 0;
        OSCL_IMPORT_REF virtual PVMFTimestamp getMaxMediaDataTS() = 0;
        OSCL_IMPORT_REF virtual void addRef() = 0;
        OSCL_IMPORT_REF virtual void removeRef() = 0;
        OSCL_IMPORT_REF virtual bool queryInterface(const PVUuid& uuid, PVInterface*& iface) = 0;
        OSCL_IMPORT_REF virtual PVMFStatus setServerInfo(PVMFJitterBufferFireWallPacketInfo& aServerInfo) = 0;
        OSCL_IMPORT_REF virtual PVMFStatus NotifyOutOfBandEOS() = 0;
        OSCL_IMPORT_REF virtual PVMFStatus SendBOSMessage(uint32 aStramID) = 0;

        OSCL_IMPORT_REF virtual void SetJitterBufferChunkAllocator(OsclMemPoolResizableAllocator* aDataBufferAllocator, const PVMFPortInterface* aPort) = 0;

        OSCL_IMPORT_REF virtual void SetJitterBufferMemPoolInfo(const PvmfPortBaseImpl* aPort, uint32 aSize, uint32 aResizeSize, uint32 aMaxNumResizes, uint32 aExpectedNumberOfBlocksPerBuffer) = 0;
        OSCL_IMPORT_REF virtual void GetJitterBufferMemPoolInfo(const PvmfPortBaseImpl* aPort, uint32& aSize, uint32& aResizeSize, uint32& aMaxNumResizes, uint32& aExpectedNumberOfBlocksPerBuffer) const = 0;

        OSCL_IMPORT_REF virtual void SetSharedBufferResizeParams(uint32 maxNumResizes, uint32 resizeSize) = 0;
        OSCL_IMPORT_REF virtual void GetSharedBufferResizeParams(uint32& maxNumResizes, uint32& resizeSize) = 0;

        OSCL_IMPORT_REF virtual bool ClearJitterBuffer(PVMFPortInterface* aPort,
                uint32 aSeqNum) = 0;
        OSCL_IMPORT_REF virtual void FlushJitterBuffer() = 0;

        OSCL_IMPORT_REF virtual bool NotifyAutoPauseComplete() = 0;
        OSCL_IMPORT_REF virtual bool NotifyAutoResumeComplete() = 0;
        OSCL_IMPORT_REF virtual PVMFStatus SetTransportType(PVMFPortInterface* aPort,
                OSCL_String& aTransportType) = 0;
        OSCL_IMPORT_REF virtual PVMFStatus HasSessionDurationExpired(bool& aExpired) = 0;
        OSCL_IMPORT_REF virtual bool PurgeElementsWithNPTLessThan(NptTimeFormat &aNPTTime) = 0;

        OSCL_IMPORT_REF virtual void SetBroadCastSession() = 0;
        OSCL_IMPORT_REF virtual void DisableFireWallPackets() = 0;
        OSCL_IMPORT_REF virtual void UpdateJitterBufferState() = 0;
        OSCL_IMPORT_REF virtual void StartOutputPorts() = 0;
        OSCL_IMPORT_REF virtual void StopOutputPorts() = 0;
        OSCL_IMPORT_REF virtual bool PrepareForPlaylistSwitch() = 0;
};

//Mimetype and Uuid for the extension interface
#define PVMF_JITTERBUFFER_CUSTOMINTERFACE_MIMETYPE "pvxxx/PVMFJitterBufferNode/CustomInterface"
#define PVMF_JITTERBUFFER_MIMETYPE "pvxxx/PVMFJitterBufferNode"
#define PVMF_JITTERBUFFER_BASEMIMETYPE "pvxxx"
#define PVMF_JITTERBUFFERNODE_EXTENSIONINTERFACE_UUID PVUuid(0x440af38b,0xde8d,0x4d61,0xab,0x2a,0x84,0x11,0x07,0x3c,0x60,0x35)

#endif


