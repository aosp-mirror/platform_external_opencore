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
#ifndef PVMF_JITTER_BUFFER_NODE_H_INCLUDED
#include "pvmf_jitter_buffer_node.h"
#endif
#ifndef PVMF_JITTER_BUFFER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif
#ifndef PV_LOGGGER_H_INCLUDED
#include "pvlogger.h"
#endif

PVMFJitterBufferExtensionInterfaceImpl::PVMFJitterBufferExtensionInterfaceImpl(PVMFJitterBufferNode*c)
        : PVInterfaceImpl<PVMFJitterBufferNodeAllocator>(PVUuid(PVMF_JITTERBUFFERNODE_EXTENSIONINTERFACE_UUID))
        , iContainer(c)
{}

PVMFJitterBufferExtensionInterfaceImpl::~PVMFJitterBufferExtensionInterfaceImpl()
{}

OSCL_EXPORT_REF
bool PVMFJitterBufferExtensionInterfaceImpl::setPortParams(PVMFPortInterface* aPort,
        uint32 aTimeScale,
        uint32 aBitRate,
        OsclRefCounterMemFrag& aConfig,
        bool aRateAdaptation,
        uint32 aRateAdaptationFeedBackFrequency)
{
    return (iContainer->setPortParams(aPort,
                                      aTimeScale,
                                      aBitRate,
                                      aConfig,
                                      aRateAdaptation,
                                      aRateAdaptationFeedBackFrequency));
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::setPlayBackThresholdInMilliSeconds(uint32 threshold)
{
    iContainer->setPlayBackThresholdInMilliSeconds(threshold);
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::setJitterBufferRebufferingThresholdInMilliSeconds(uint32 aThreshold)
{
    iContainer->setJitterBufferRebufferingThresholdInMilliSeconds(aThreshold);
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::getJitterBufferRebufferingThresholdInMilliSeconds(uint32& aThreshold)
{
    iContainer->getJitterBufferRebufferingThresholdInMilliSeconds(aThreshold);
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::setJitterBufferDurationInMilliSeconds(uint32 duration)
{
    iContainer->setJitterBufferDurationInMilliSeconds(duration);
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::getJitterBufferDurationInMilliSeconds(uint32& duration)
{
    iContainer->getJitterBufferDurationInMilliSeconds(duration);
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::setClientPlayBackClock(OsclClock* clientClock)
{
    iContainer->setClientPlayBackClock(clientClock);
}

OSCL_EXPORT_REF
void PVMFJitterBufferExtensionInterfaceImpl::setRTCPIntervalInMicroSecs(uint32 aRTCPInterval)
{
    iContainer->setRTCPIntervalInMicroSecs(aRTCPInterval);
}

OSCL_EXPORT_REF
bool PVMFJitterBufferExtensionInterfaceImpl::setPlayRange(int32 aStartTimeInMS,
        int32 aStopTimeInMS,
        bool oPlayAfterASeek,
        bool aStopTimeAvailable)
{
    return (iContainer->setPlayRange(aStartTimeInMS,
                                     aStopTimeInMS,
                                     oPlayAfterASeek,
                                     aStopTimeAvailable));
}

OSCL_EXPORT_REF
bool PVMFJitterBufferExtensionInterfaceImpl::PrepareForRepositioning(bool oUseExpectedClientClockVal,
        uint32 aExpectedClientClockVal)
{
    return (iContainer->PrepareForRepositioning(oUseExpectedClientClockVal, aExpectedClientClockVal));
}

OSCL_EXPORT_REF
bool PVMFJitterBufferExtensionInterfaceImpl::setPortSSRC(PVMFPortInterface* aPort, uint32 aSSRC)
{
    return (iContainer->setPortSSRC(aPort, aSSRC));
}


OSCL_EXPORT_REF
bool PVMFJitterBufferExtensionInterfaceImpl::setPortRTPParams(PVMFPortInterface* aPort,
        bool   aSeqNumBasePresent,
        uint32 aSeqNumBase,
        bool   aRTPTimeBasePresent,
        uint32 aRTPTimeBase,
        uint32 aNPTInMS,
        bool oPlayAfterASeek)
{
    return (iContainer->setPortRTPParams(aPort,
                                         aSeqNumBasePresent,
                                         aSeqNumBase,
                                         aRTPTimeBasePresent,
                                         aRTPTimeBase,
                                         aNPTInMS,
                                         oPlayAfterASeek));
}

OSCL_EXPORT_REF
bool PVMFJitterBufferExtensionInterfaceImpl::setPortRTCPParams(PVMFPortInterface* aPort,
        int aNumSenders,
        uint32 aRR,
        uint32 aRS)
{
    return iContainer->setPortRTCPParams(aPort, aNumSenders, aRR, aRS);
}

OSCL_EXPORT_REF
PVMFTimestamp PVMFJitterBufferExtensionInterfaceImpl::getActualMediaDataTSAfterSeek()
{
    return (iContainer->getActualMediaDataTSAfterSeek());
}

OSCL_EXPORT_REF PVMFStatus
PVMFJitterBufferExtensionInterfaceImpl::setServerInfo(PVMFJitterBufferFireWallPacketInfo& aServerInfo)
{
    return (iContainer->setServerInfo(aServerInfo));
}

OSCL_EXPORT_REF PVMFStatus
PVMFJitterBufferExtensionInterfaceImpl::NotifyOutOfBandEOS()
{
    return (iContainer->NotifyOutOfBandEOS());
}

OSCL_EXPORT_REF PVMFStatus
PVMFJitterBufferExtensionInterfaceImpl::SendBOSMessage(uint32 aStreamID)
{
    return (iContainer->SendBOSMessage(aStreamID));
}

OSCL_EXPORT_REF OsclSharedPtr<PVMFSharedSocketDataBufferAlloc>
PVMFJitterBufferExtensionInterfaceImpl::CreateResizablePortAllocator(uint32 aSize, OSCL_String& aName)
{
    return (iContainer->CreateResizablePortAllocator(aSize, aName));
}

OSCL_EXPORT_REF void
PVMFJitterBufferExtensionInterfaceImpl::SetSharedBufferResizeParams(uint32 maxNumResizes, uint32 resizeSize)
{
    iContainer->SetSharedBufferResizeParams(maxNumResizes, resizeSize);
}

OSCL_EXPORT_REF void
PVMFJitterBufferExtensionInterfaceImpl::GetSharedBufferResizeParams(uint32& maxNumResizes, uint32& resizeSize)
{
    iContainer->GetSharedBufferResizeParams(maxNumResizes, resizeSize);
}
OSCL_EXPORT_REF bool
PVMFJitterBufferExtensionInterfaceImpl::ClearJitterBuffer(PVMFPortInterface* aPort, uint32 aSeqNum)
{
    return (iContainer->ClearJitterBuffer(aPort, aSeqNum));
}

OSCL_EXPORT_REF bool
PVMFJitterBufferExtensionInterfaceImpl::NotifyAutoPauseComplete()
{
    return (iContainer->NotifyAutoPauseComplete());
}

OSCL_EXPORT_REF bool
PVMFJitterBufferExtensionInterfaceImpl::NotifyAutoResumeComplete()
{
    return (iContainer->NotifyAutoResumeComplete());
}

OSCL_EXPORT_REF PVMFStatus
PVMFJitterBufferExtensionInterfaceImpl::SetTransportType(PVMFPortInterface* aPort,
        OSCL_String& aTransportType)
{
    return (iContainer->SetTransportType(aPort, aTransportType));
}

OSCL_EXPORT_REF PVMFStatus
PVMFJitterBufferExtensionInterfaceImpl::HasSessionDurationExpired(bool& aExpired)
{
    return (iContainer->HasSessionDurationExpired(aExpired));
}
OSCL_EXPORT_REF bool
PVMFJitterBufferExtensionInterfaceImpl::PurgeElementsWithNPTLessThan(NptTimeFormat &aNPTTime)
{
    return (iContainer->PurgeElementsWithNPTLessThan(aNPTTime));
}
OSCL_EXPORT_REF void
PVMFJitterBufferExtensionInterfaceImpl::SetBroadCastSession()
{
    iContainer->SetBroadCastSession();
}
OSCL_EXPORT_REF void
PVMFJitterBufferExtensionInterfaceImpl::DisableFireWallPackets()
{
    iContainer->DisableFireWallPackets();
}















