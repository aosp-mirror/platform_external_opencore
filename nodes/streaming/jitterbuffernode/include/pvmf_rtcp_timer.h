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
#ifndef PVMF_RTCP_TIMER_H_INCLUDED
#define PVMF_RTCP_TIMER_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef PVMF_RETURN_CODES_H_INCLUDED
#include "pvmf_return_codes.h"
#endif
#ifndef PVMF_SIMPLE_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_simple_media_buffer.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_STREAMING_BUFFER_ALLOCATORS_H_INCLUDED
#include "pvmf_streaming_buffer_allocators.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif

/*
oscl_mem_aligned_size(sizeof(PVMFMediaData)) +
oscl_mem_aligned_size(sizeof(OsclRefCounterDA)) +
oscl_mem_aligned_size(sizeof(MediaDataCleanupDA)) +
sizeof(PVMFMediaMsgHeader));
*/
#define PVMF_MEDIA_DATA_CLASS_SIZE 128

/* RTCP Packet Mem Pool Allocator */
class PVMFRTCPMemPool
{
    public:
        PVMFRTCPMemPool(uint32 aNumRTCPBufs = DEFAULT_RTCP_MEM_POOL_BUFFERS)
        {
            iMediaDataMemPool = NULL;
            iMediaDataMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (aNumRTCPBufs, PVMF_MEDIA_DATA_CLASS_SIZE));
        }

        ~PVMFRTCPMemPool()
        {
            if (iMediaDataMemPool)
            {
                iMediaDataMemPool->removeRef();
            }
        }
        OsclSharedPtr<PVMFMediaDataImpl> getMediaDataImpl(uint32 size)
        {
            return (iRTCPRRMsgBufAlloc->createSharedBuffer(size));
        }

        OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> iRTCPRRMsgBufAlloc;
        /* Memory pool for media data objects */
        OsclMemPoolFixedChunkAllocator* iMediaDataMemPool;
};

class PVMFRTCPStats
{
    public:
        PVMFRTCPStats()
        {
            lastSenderReportNTP = 0;
            lastSenderReportRTP = 0;
            lastSenderReportTS = 0;
            lastSenderReportRecvTime = 0;
            packetLossUptoThisRR = 0;
            maxSeqNumReceivedUptoThisRR = 0;
            lastRRGenTime = 0;
            oSRRecvd = false;
            oRTCPByeRecvd = false;
        };

        PVMFRTCPStats(const PVMFRTCPStats& aSrc)
        {
            lastSenderReportNTP = aSrc.lastSenderReportNTP;
            lastSenderReportRTP = aSrc.lastSenderReportRTP;
            lastSenderReportTS = aSrc.lastSenderReportTS;
            lastSenderReportRecvTime = aSrc.lastSenderReportRecvTime;
            packetLossUptoThisRR = aSrc.packetLossUptoThisRR;
            maxSeqNumReceivedUptoThisRR = aSrc.maxSeqNumReceivedUptoThisRR;
            lastRRGenTime = aSrc.lastRRGenTime;
            oSRRecvd = aSrc.oSRRecvd;
            oRTCPByeRecvd = aSrc.oRTCPByeRecvd;
        };

        PVMFRTCPStats& operator=(const PVMFRTCPStats& a)
        {
            if (&a != this)
            {
                lastSenderReportNTP = a.lastSenderReportNTP;
                lastSenderReportRTP = a.lastSenderReportRTP;
                lastSenderReportTS = a.lastSenderReportTS;
                lastSenderReportRecvTime = a.lastSenderReportRecvTime;
                packetLossUptoThisRR = a.packetLossUptoThisRR;
                maxSeqNumReceivedUptoThisRR = a.maxSeqNumReceivedUptoThisRR;
                lastRRGenTime = a.lastRRGenTime;
                oSRRecvd = a.oSRRecvd;
                oRTCPByeRecvd = a.oRTCPByeRecvd;
            }
            return *this;
        };

        /* these are for feedback statistics. */
        uint64	lastSenderReportNTP;
        uint32	lastSenderReportRTP;
        uint32	lastSenderReportTS;
        uint64	lastSenderReportRecvTime;
        int32	packetLossUptoThisRR;
        int32   maxSeqNumReceivedUptoThisRR;
        uint64	lastRRGenTime;
        bool    oSRRecvd;
        bool    oRTCPByeRecvd;
};


class PvmfRtcpTimer;

/**
 * Observer class for the rtcp timer AO
 */
class PvmfRtcpTimerObserver
{
    public:
        virtual ~PvmfRtcpTimerObserver() {}
        /**
         * A timer event, indicating that the RTCP timer has expired.
         */
        virtual void RtcpTimerEvent(PvmfRtcpTimer* pTimer) = 0;
};

/**
 * RTCP timer object to Jitter Buffer node. This object generates events at
 * specified RTCP time intervals
 */
class PvmfRtcpTimer : public OsclTimerObject
{
    public:
        PvmfRtcpTimer(PvmfRtcpTimerObserver* aObserver);

        virtual ~PvmfRtcpTimer();

        /** Start RTCP Timer */
        PVMFStatus Start();

        /** Reset RTCP Timer */
        PVMFStatus setRTCPInterval(uint32 rtcpTimeIntervalInMicroSecs);

        /** Stop Timer events */
        PVMFStatus Stop();

        PVMFRTCPMemPool* getRTCPBuffAlloc()
        {
            return &iRTCPBufAlloc;
        }

    private:
        void Run();

        OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> createRTCPRRBufAllocReSize();

        uint32 iRTCPTimeIntervalInMicroSecs;
        PvmfRtcpTimerObserver* iObserver;
        PVLogger* iLogger;
        bool iStarted;

        PVMFRTCPMemPool iRTCPBufAlloc;
        PVMFSMSharedBufferAllocWithReSize* iBufAlloc;
};
#endif // PVMF_RTCP_TIMER_H_INCLUDED




