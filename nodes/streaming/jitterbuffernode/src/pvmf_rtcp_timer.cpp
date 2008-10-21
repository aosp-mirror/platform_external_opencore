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
 * @file pvmf_rtcp_timer.cpp
 * @brief RTCP timer to Jitter Buffer Node
 */
#ifndef PVMF_RTCP_TIMER_H_INCLUDED
#include "pvmf_rtcp_timer.h"
#endif
#ifndef PVMF_JITTER_BUFFER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif

#define RTCP_HOLD_DATA_SIZE 2

////////////////////////////////////////////////////////////////////////////
PvmfRtcpTimer::PvmfRtcpTimer(PvmfRtcpTimerObserver* aObserver)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmfRtcpTimer"),
        iRTCPTimeIntervalInMicroSecs(DEFAULT_RTCP_INTERVAL_USEC),
        iObserver(aObserver),
        iStarted(false)
{
    iBufAlloc = NULL;
    iLogger = PVLogger::GetLoggerObject("PvmfRtcpTimer");
    AddToScheduler();
    iRTCPBufAlloc.iRTCPRRMsgBufAlloc = createRTCPRRBufAllocReSize();
}

////////////////////////////////////////////////////////////////////////////
PvmfRtcpTimer::~PvmfRtcpTimer()
{
    Stop();
    if (iBufAlloc != NULL)
    {
        iBufAlloc->DecrementKeepAliveCount();
        if (iBufAlloc->getNumOutStandingBuffers() == 0)
        {
            OSCL_DELETE((iBufAlloc));
        }
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfRtcpTimer::Start()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfRtcpTimer::Start"));
    if (iRTCPTimeIntervalInMicroSecs > 0)
    {
        RunIfNotReady(iRTCPTimeIntervalInMicroSecs);
        iStarted = true;
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfRtcpTimer::setRTCPInterval(uint32 rtcpTimeIntervalInMicroSecs)
{
    PVMF_JBNODE_LOGINFO((0, "PvmfRtcpTimer::ResetRTCPInterval"));
    iRTCPTimeIntervalInMicroSecs = rtcpTimeIntervalInMicroSecs;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfRtcpTimer::Stop()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfRtcpTimer::Stop"));
    Cancel();
    iStarted = false;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmfRtcpTimer::Run()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfRtcpTimer::Run"));

    if (!iStarted)
        return;

    if (!iObserver)
    {
        PVMF_JBNODE_LOGERROR((0, "PvmfRtcpTimer::Run: Error - Observer not set"));
        return;
    }

    iObserver->RtcpTimerEvent(this);
    /*
     * Do not reschudule the AO here. Observer would reschedule this AO
     * once it is done processing the timer event.
     */
}

OsclSharedPtr<PVMFSharedSocketDataBufferAlloc>
PvmfRtcpTimer::createRTCPRRBufAllocReSize()
{
    uint8* my_ptr;
    OsclRefCounter* my_refcnt;
    OsclMemAllocator my_alloc;
    PVMFSharedSocketDataBufferAlloc *alloc_ptr = NULL;

    uint aligned_socket_alloc_size =
        oscl_mem_aligned_size(sizeof(PVMFSMSharedBufferAllocWithReSize));

    uint aligned_refcnt_size =
        oscl_mem_aligned_size(sizeof(OsclRefCounterSA<PVMFSharedSocketDataBufferAllocCleanupSA>));

    my_ptr = (uint8*) my_alloc.ALLOCATE(aligned_refcnt_size +
                                        aligned_socket_alloc_size);

    my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterSA<PVMFSharedSocketDataBufferAllocCleanupSA>(my_ptr));
    my_ptr += aligned_refcnt_size;

    iBufAlloc =
        OSCL_NEW(PVMFSMSharedBufferAllocWithReSize, (DEFAULT_RTCP_SOCKET_MEM_POOL_SIZE_IN_BYTES, "PVMFRTCPRRMemPool"));

    alloc_ptr = OSCL_PLACEMENT_NEW(my_ptr, PVMFSharedSocketDataBufferAlloc(iBufAlloc));

    OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> shared_alloc(alloc_ptr, my_refcnt);

    return shared_alloc;
}
