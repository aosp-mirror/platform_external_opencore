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
 * @file pvmf_jitter_buffer_impl.cpp
 * @brief Jitter Buffer implementation
 */

#ifndef OSCL_EXCLUSIVE_PTR_H_INCLUDED
#include "oscl_exclusive_ptr.h"
#endif
#ifndef OSCL_BYTE_ORDER_H_INCLUDED
#include "oscl_byte_order.h"
#endif
#ifndef PVMF_JITTER_BUFFER_H_INCLUDED
#include "pvmf_jitter_buffer.h"
#endif
#ifndef PVMF_JITTER_BUFFER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif
#ifndef PVMF_STREAMING_MEM_CONFIG_H_INCLUDED
#include "pvmf_streaming_mem_config.h"
#endif
#ifndef OSCL_BIN_STREAM_H
#include "oscl_bin_stream.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

PVMFJitterBufferImpl::PVMFJitterBufferImpl(const PvmfMimeString* aMimeType,
        bool  aInPlaceProcessing)
{
    iLogger = NULL;
    iDataPathLoggerIn = NULL;
    iDataPathLoggerOut = NULL;
    iClockLogger = NULL;
    seqNumLock = false;
    oEOS = false;
    seqLockTimeStamp = 0;
    SSRCLock = 0;
    SSRCFromSetUpResponse = 0;
    oSSRCFromSetUpResponseAvailable = false;
    iMonotonicTimeStamp = 0;
    iPrevTSIn = 0;
    iPrevSeqNumBaseIn = 0;
    iPrevTSOut = 0;
    iPrevSeqNumBaseOut = 0;
    iMaxAdjustedRTPTS = 0;
    iPrevAdjustedRTPTS = 0;
    iEstimatedServerClock = NULL;
    iRTPTimeScale = 0;
    iFirstSeqNum = 0;
    iObserver = NULL;
    iObserverContext = NULL;
    iInPlaceProcessing = aInPlaceProcessing;
    iMediaDataImplMemPool = NULL;
    iMediaDataGroupAlloc = NULL;
    iMediaMsgMemPool = NULL;
    iServerClockUpdateNotificationObserver = NULL;

    iInterArrivalJitterD = 0;
    iPrevPacketTS = 0;
    iPrevPacketRecvTime = 0;

    iLogger = PVLogger::GetLoggerObject("JitterBuffer");
    iDataPathLoggerIn = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.in");
    iDataPathLoggerOut = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.out");
    iClockLogger = PVLogger::GetLoggerObject("clock.jitterbuffer");

    iHeaderFormat = PVMF_JITTER_BUFFER_TRANSPORT_HEADER_FORMAT_UNKNOWN;
    iSeqNum = 0;
    iFirstASFPacketAfterRepos = true;

    OSCL_StackString<8> rtp(_STRLIT_CHAR("RTP"));
    const char* mime = aMimeType->get_cstr();
    if (oscl_strncmp(mime, rtp.get_cstr(), 3) == 0)
    {
        iHeaderFormat = PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP;
    }
    iBroadCastSession = false;

    OsclExclusivePtr<OsclClock> arrivalClockAutoPtr;
    PVMF_JITTER_BUFFER_NEW(NULL, OsclClock, (), iPacketArrivalClock);
    arrivalClockAutoPtr.set(iPacketArrivalClock);

    uint32 numNodes = 0;
#if (PMVF_JITTER_BUFFER_NODE_USE_NO_RESIZE_ALLOC)
    numNodes = PVMF_JB_NO_RESIZE_ALLOC_NUM_CHUNKS_RTP;
#else
    numNodes = DEFAULT_NUM_MEDIA_MSGS_IN_JITTER_BUFFER;
#endif

    OsclExclusivePtr<PVMFDynamicCircularArrayType> jitterBufferAutoPtr;
    PVMF_JITTER_BUFFER_NEW(NULL,
                           PVMFDynamicCircularArrayType,
                           (numNodes, aMimeType),
                           iJitterBuffer);

    jitterBufferAutoPtr.set(iJitterBuffer) ;

    iJitterBuffer->setPacketHeaderFormat(iHeaderFormat);

    if (iInPlaceProcessing == false)
    {
        OsclExclusivePtr<PoolMemAlloc> mediaDataImplMemPoolAutoPtr;
        typedef OsclMemPoolFixedChunkAllocator osclmempoolAllocType;
        PVMF_JITTER_BUFFER_NEW(NULL,
                               osclmempoolAllocType,
                               (numNodes),
                               iMediaDataImplMemPool);
        mediaDataImplMemPoolAutoPtr.set(iMediaDataImplMemPool);

        typedef PVMFMediaFragGroupCombinedAlloc<PoolMemAlloc> mediaFragGroupType;
        OsclExclusivePtr<mediaFragGroupType> mediaDataGroupAutoPtr;
        PVMF_JITTER_BUFFER_NEW(NULL,
                               mediaFragGroupType,
                               (numNodes,
                                DEFAULT_NUM_FRAGMENTS_IN_MEDIA_MSG,
                                iMediaDataImplMemPool),
                               iMediaDataGroupAlloc);
        mediaDataGroupAutoPtr.set(iMediaDataGroupAlloc);

        iMediaDataGroupAlloc->create();

        OsclExclusivePtr<PoolMemAlloc> mediaMsgMemPoolAutoPtr;
        PVMF_JITTER_BUFFER_NEW(NULL,
                               osclmempoolAllocType,
                               (numNodes),
                               iMediaMsgMemPool);
        mediaMsgMemPoolAutoPtr.set(iMediaMsgMemPool);

        mediaDataImplMemPoolAutoPtr.release();
        mediaDataGroupAutoPtr.release();
        mediaMsgMemPoolAutoPtr.release();
    }

    if (aMimeType != NULL)
    {
        iMimeType = aMimeType->get_cstr();
    }

    arrivalClockAutoPtr.release();
    jitterBufferAutoPtr.release();

    /* Start arrival clock */
    iPacketArrivalClock->SetClockTimebase(iPacketArrivalTimeBase);
    iPacketArrivalClock->Start();
}

PVMFJitterBufferImpl::~PVMFJitterBufferImpl()
{
    iLogger = NULL;
    iDataPathLoggerIn = NULL;
    iDataPathLoggerOut = NULL;
    iClockLogger = NULL;
    iPacketArrivalClock->Stop();
    iServerClockUpdateNotificationObserver = NULL;

    PVMF_JITTER_BUFFER_TEMPLATED_DELETE(NULL, PVMFDynamicCircularArray<PVMFJitterBufferNodeAllocator>, PVMFDynamicCircularArray, iJitterBuffer);
    PVMF_JITTER_BUFFER_DELETE(NULL, OsclClock, iPacketArrivalClock);

    if (iInPlaceProcessing == false)
    {
        if (iMediaDataGroupAlloc != NULL)
        {
            iMediaDataGroupAlloc->CancelFreeChunkAvailableCallback();
            iMediaDataGroupAlloc->removeRef();
            iMediaDataGroupAlloc = NULL;
        }
        if (iMediaDataImplMemPool != NULL)
        {
            iMediaDataImplMemPool->removeRef();
            iMediaDataImplMemPool = NULL;
        }
        if (iMediaMsgMemPool != NULL)
        {
            iMediaMsgMemPool->removeRef();
            iMediaMsgMemPool = NULL;
        }
    }
}

void
PVMFJitterBufferImpl::ResetJitterBuffer()
{
    FlushJitterBuffer();
    iJitterBuffer->ResetJitterBufferStats();
    oEOS = false;
    iRTPInfoParamsVec.clear();
    seqNumLock = false;
    iMonotonicTimeStamp = 0;
    iPrevPacketTS = 0;
    iPrevTSOut = 0;
    iSeqNum = 0;
    iMaxAdjustedRTPTS = 0;
    UpdateEstimatedServerClockDiscrete(true);
    iFirstASFPacketAfterRepos = true;
}

PVMFJitterBufferAddPktStatus
PVMFJitterBufferImpl::addPacket(PVMFSharedMediaDataPtr& dataPacket)
{
    if (oEOS)
    {
        /* EOS received - do not register packet */
        PVMF_JBNODE_LOGWARNING((0, "0x%x PVMFJitterBufferImpl::addPacket: After EOS Reached!!!", this));
        return PVMF_JITTER_BUFFER_ADD_PKT_EOS_REACHED;
    }

    if (seqNumLock)
    {
        if (oSSRCFromSetUpResponseAvailable == false)
        {
            oSSRCFromSetUpResponseAvailable = true;
            SSRCLock = dataPacket->getStreamID();
            PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::addPacket: Ser No SSRC, set to 1st pkt SSRC %d", SSRCLock));
        }
        /* Filter based on SSRC */
        if (dataPacket->getStreamID() == SSRCLock)
        {
            PVMFJitterBufferAddElemStatus status;

            status = iJitterBuffer->addElement(dataPacket, iFirstSeqNum);
            if (status == PVMF_JITTER_BUFFER_ADD_ELEM_SUCCESS)
            {
                PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::addPacket: MimeType=%s TS=%d, SEQNUM= %d",
                                               iMimeType.get_cstr(), dataPacket->getTimestamp(), dataPacket->getSeqNum()));

                if (iRTPInfoParamsVec.size() > 0)
                {
                    /*
                     * Calculate adjusted RTP TS - Will be used to update
                     * the estimated server clock, if any only if some rtp-info
                     * params have been set
                     */
                    ComputeMaxAdjustedRTPTS();
                }
            }
            else if (status == PVMF_JITTER_BUFFER_ADD_ELEM_UNEXPECTED_DATA)
            {
                return PVMF_JITTER_BUFFER_ADD_PKT_UNEXPECTED_DATA;
            }

            /* Calculate inter arrival jitter */
            UpdateInterArrivalJitter(dataPacket->getTimestamp());
        }
    }
    else
    {
        if (oSSRCFromSetUpResponseAvailable && (dataPacket->getStreamID() != SSRCFromSetUpResponse))
        {//discard packet
            PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::addPacket: ERROR wrong ssrc %d", dataPacket->getStreamID()));
            return PVMF_JITTER_BUFFER_ADD_PKT_SUCCESS;
        }
        // Add packet to temporary array
        iFirstDataPackets.push_back(dataPacket);

        const uint cPktNeededForVote = 2;
        if (iFirstDataPackets.size() < cPktNeededForVote)
            return PVMF_JITTER_BUFFER_ADD_PKT_SUCCESS;

        //Majortiy vote for SSRC first
        //0 count; 1 ssrc; 2 seqbase; 3 timestamp; uint32 PVMFTimestamp;
        uint32 my_ssrc[cPktNeededForVote][4];
        for (uint32 i = 0; i < cPktNeededForVote; i++)
            my_ssrc[i][0] = my_ssrc[i][1] = my_ssrc[i][2] = my_ssrc[i][3] = 0;

        {
            // 1. vote
            for (uint32 i = 0; i < cPktNeededForVote; i++)
            {
                uint32 ssrc = iFirstDataPackets[i]->getStreamID();
                for (uint32 j = 0; j < cPktNeededForVote; j++)
                {
                    if (my_ssrc[j][0] > 0)
                    {
                        if (ssrc ==  my_ssrc[j][1])
                        {
                            my_ssrc[j][0]++;
                            if (iFirstDataPackets[i]->getSeqNum() < my_ssrc[j][2])
                            {
                                my_ssrc[j][2] = iFirstDataPackets[i]->getSeqNum();
                                my_ssrc[j][3] = iFirstDataPackets[i]->getTimestamp();
                            }
                            break;
                        }
                    }
                    else
                    {
                        my_ssrc[j][0]++; //my_ssrc[j][0]=1
                        my_ssrc[j][1] = ssrc;
                        my_ssrc[j][2] = iFirstDataPackets[i]->getSeqNum();
                        my_ssrc[j][3] = iFirstDataPackets[i]->getTimestamp();
                        break;
                    }
                }
            }
        }

        {// 2. poll ssrc
            uint32 first_ssrc_index = 0, second_ssrc_index = 1;
            if (my_ssrc[0][0] < my_ssrc[1][0])
            {
                first_ssrc_index = 1;
                second_ssrc_index = 0;
            }
            for (uint32 i = 2; i < cPktNeededForVote; i++)
            {
                if (my_ssrc[i][0] > first_ssrc_index)
                {
                    second_ssrc_index = first_ssrc_index;
                    first_ssrc_index = i;
                }
                else if (my_ssrc[i][0] > second_ssrc_index)
                {
                    second_ssrc_index =  i;
                }
            }
            if (my_ssrc[first_ssrc_index][0] <= my_ssrc[second_ssrc_index][0])
            {
                //got a tie. should rarely happen
                //for now, just pick the first one
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::addPacket: ERROR first %d second %d", first_ssrc_index, second_ssrc_index));
            }
            SSRCLock = my_ssrc[first_ssrc_index][1];
            seqNumLock = true;

            //if we got RTPInfo by now, we should still use it
            if (iRTPInfoParamsVec.size() > 0)
            {
                Oscl_Vector<PVMFRTPInfoParams, PVMFJitterBufferNodeAllocator>::iterator it;
                it = iRTPInfoParamsVec.begin();
                iFirstSeqNum = (it->seqNumBaseSet) ? it->seqNum : my_ssrc[first_ssrc_index][2];
                seqLockTimeStamp = (it->rtpTimeBaseSet) ? it->rtpTime : my_ssrc[first_ssrc_index][3];
            }
            // iFirstSeqNum must be initialized when we come here
            iJitterBuffer->setFirstSeqNumAdded(iFirstSeqNum);
            CheckForRTPTimeAndRTPSeqNumberBase();
        }
        // 3.throw away the pkt not belong to current session and register packets

        bool bNoErr = true;
        Oscl_Vector<PVMFSharedMediaDataPtr, PVMFJitterBufferNodeAllocator>::iterator it;
        for (it = iFirstDataPackets.begin(); it != iFirstDataPackets.end();		it++)
        {
            if ((*it)->getStreamID() == SSRCLock)
            {
                if (! iJitterBuffer->addElement(*it, iFirstSeqNum))
                {
                    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::addPacket: addElement failed"));
                    bNoErr = false;
                }
                else
                {
                    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::addPacket: MimeType=%s TS=%d, SEQNUM= %d",
                                                   iMimeType.get_cstr(), (*it)->getTimestamp(), (*it)->getSeqNum()));
                }
            }
        }

        iFirstDataPackets.clear();
        // Calculate inter arrival jitter
        UpdateInterArrivalJitter(dataPacket->getTimestamp());
        if (iRTPInfoParamsVec.size() > 0)
        {
            /*
            * Calculate adjusted RTP TS - Will be used to update
            * the estimated server clock, if any only if some rtp-info
            * params have been set
            	*/
            ComputeMaxAdjustedRTPTS();
        }

        if (!bNoErr)
            return PVMF_JITTER_BUFFER_ADD_PKT_ERROR;

    }

    PVMFJitterBufferStats stats = getJitterBufferStats();
    if (stats.totalPacketsLost > 0)
    {
        PVMF_JBNODE_LOGWARNING((0, "Packet Loss: MimeType=%s, NumPacketsLost=%d", iMimeType.get_cstr(), stats.totalPacketsLost));
    }
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::addPacket - JB Occup Stats - MimeType=%s, MaxSize=%d, CurrOccupany=%d", iMimeType.get_cstr(), iJitterBuffer->getArraySize(), iJitterBuffer->getNumElements()));
    return PVMF_JITTER_BUFFER_ADD_PKT_SUCCESS;
}

PVMFSharedMediaDataPtr PVMFJitterBufferImpl::retrievePacket()
{
    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::retrievePacket - JB Occup Stats - MimeType=%s, MaxSize=%d, CurrOccupany=%d", iMimeType.get_cstr(), iJitterBuffer->getArraySize(), iJitterBuffer->getNumElements()));

    PVMFSharedMediaDataPtr elem = iJitterBuffer->retrieveElement();
    if (elem.GetRep() != NULL)
    {
        /*
         * Adjust TimeStamp - Goal is to provide a monotonically increasing
         * timestamp.
         */
        PVMFTimestamp currTS = elem->getTimestamp();
        DeterminePrevTimeStamp(elem->getSeqNum());
        iMonotonicTimeStamp += (currTS - iPrevTSOut);
        PVMFTimestamp adjustedTS =
            (PVMFTimestamp)(Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp));
        elem->setTimestamp(adjustedTS);

        PVMFJitterBufferStats* jbStatsPtr = iJitterBuffer->getStatsPtr();
        jbStatsPtr->maxTimeStampRetrievedWithoutRTPOffset = adjustedTS;

        PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferImpl::retrievePacket: MimeType=%s, TS=%d, SEQNUM= %d",
                                        iMimeType.get_cstr(),
                                        elem->getTimestamp(),
                                        elem->getSeqNum()));
        iPrevTSOut = currTS;
    }
    return elem;
}

PVMFTimestamp PVMFJitterBufferImpl::peekNextElementTimeStamp()
{
    if (iJitterBuffer->getNumElements() > 0)
    {
        PVMFTimestamp currTS;
        PVMFTimestamp prevTS;
        uint32 aSeqNum;
        iJitterBuffer->peekNextElementTimeStamp(currTS, aSeqNum);
        DeterminePrevTimeStampPeek(aSeqNum, prevTS);
        uint64 ts64 = iMonotonicTimeStamp;
        ts64 += (currTS - prevTS);
        PVMFTimestamp adjTS =
            (PVMFTimestamp)(Oscl_Int64_Utils::get_uint64_lower32(ts64));
        return (adjTS);
    }
    else
    {
        PVMFTimestamp adjTS =
            (PVMFTimestamp)(Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp));
        return (adjTS);
    }
}

void PVMFJitterBufferImpl::UpdateInterArrivalJitter(PVMFTimestamp currPacketTS)
{
    /* D(i-1,i) = (RecvT(i) - RTP_TS(i)) -
    			  (RecvT(i-1) - RTP_TS(i-1)) */
    uint64 currPacketRecvTime;
    iPacketArrivalClock->GetCurrentTime64(currPacketRecvTime,
                                          OSCLCLOCK_MSEC);
    uint32 currPacketRecvTime32 =
        Oscl_Int64_Utils::get_uint64_lower32(currPacketRecvTime);

    int32 ts_diff = currPacketTS - iPrevPacketTS;
    int32 arrival_diff = currPacketRecvTime32 - iPrevPacketRecvTime;

    int32 arrivalJitter = ts_diff - arrival_diff;
    if (ts_diff < arrival_diff)
        arrivalJitter = arrival_diff - ts_diff;

    /* J(i) = J(i-1) + (ABS(D(i-1,i)) - J(i-1))/16.0 */
    iInterArrivalJitterD += (arrivalJitter - iInterArrivalJitterD) / 16.0;
    /* Round up */
    iInterArrivalJitter = (uint32)(iInterArrivalJitterD + 0.5);

    /* Update variables */
    iPrevPacketTS = currPacketTS;
    iPrevPacketRecvTime = currPacketRecvTime32;

    return;
}

bool PVMFJitterBufferImpl::ParsePacketHeader(PVMFSharedMediaDataPtr& inDataPacket,
        PVMFSharedMediaDataPtr& outDataPacket,
        uint32 aFragIndex)
{
    if (iHeaderFormat == PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP)
    {
        return (ParseRTPHeader(inDataPacket, outDataPacket, aFragIndex));
    }
    PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::ParsePacketHeader: Unknown Header Format"));
    return false;
}

bool PVMFJitterBufferImpl::ParseRTPHeader(PVMFSharedMediaDataPtr& rtpPacket,
        PVMFSharedMediaDataPtr& dataPacket,
        uint32 aFragIndex)
{
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataIn;
    if (!rtpPacket->getMediaDataImpl(mediaDataIn))
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: corrupt input media msg", this));
        return false;
    }

    OsclRefCounterMemFrag memFragIn;
    rtpPacket->getMediaFragment(aFragIndex, memFragIn);

    /* Get start of RTP packet */
    uint8* rtpHeader    = (uint8*)(memFragIn.getMemFrag().ptr);
    uint32 rtpPacketLen = memFragIn.getMemFrag().len;

    /* is this a legal data packet? */
    if (rtpPacketLen <= RTP_FIXED_HEADER_SIZE)
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: illegal pkt size", this));
        return false;
    }

    /* Parse RTP version */
    uint8  rtpVersion = (((*rtpHeader) & RTP_HEADER_V_BIT_MASK) >> RTP_HEADER_V_BIT_OFFSET);
    if (rtpVersion != SUPPORTED_RTP_HEADER_VERSION)
    {
        PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: illegal rtp version", this));
        return false;
    }

    /* Check for padding */
    uint8 pbit = (((*rtpHeader) & RTP_HEADER_P_BIT_MASK) >> RTP_HEADER_P_BIT_OFFSET);
    uint8 numPaddingOctets = 0;
    if (pbit)
    {
        numPaddingOctets = *(rtpHeader + (rtpPacketLen - 1));
    }

    /* Check for header extension */
    uint8 xbit = (((*rtpHeader) & RTP_HEADER_X_BIT_MASK) >> RTP_HEADER_X_BIT_OFFSET);

    /* Check for CSRC */
    uint8 csrcCount = ((*rtpHeader) & RTP_HEADER_CC_BIT_MASK);

    rtpHeader++;

    /* Parse M bit */
    uint8 mbit = (((*rtpHeader) & RTP_HEADER_M_BIT_MASK) >> RTP_HEADER_M_BIT_OFFSET);

    rtpHeader++;

    /* Parse sequence number */
    uint16 seqNum16 = 0;
    oscl_memcpy((char *)&seqNum16, rtpHeader, sizeof(seqNum16));
    big_endian_to_host((char *)&seqNum16, sizeof(seqNum16));
    uint32 seqNum = (uint32)seqNum16;
    rtpHeader += 2;

    /* Parse rtp time stamp */
    uint32 ts32 = 0;
    oscl_memcpy((char *)&ts32, rtpHeader, sizeof(ts32));
    big_endian_to_host((char *)&ts32, sizeof(ts32));
    PVMFTimestamp rtpTimeStamp = (PVMFTimestamp)ts32;
    rtpHeader += 4;

    /* Parse SSRC */
    uint32 ssrc32 = 0;
    oscl_memcpy((char *)&ssrc32, rtpHeader, sizeof(ssrc32));
    big_endian_to_host((char *)&ssrc32, sizeof(ssrc32));
    uint32 SSRC = ssrc32;
    rtpHeader += 4;

    rtpPacketLen -= RTP_FIXED_HEADER_SIZE;

    /* Check for CSRC list - If present skip over */
    if (csrcCount)
    {
        if ((uint32)(csrcCount*4) > rtpPacketLen)
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: Corrupt CSRC", this));
            return false;
        }
        rtpHeader += (csrcCount * 4);
        rtpPacketLen -= (csrcCount * 4);
    }

    /* Check for extended RTP header - If present skip over */
    if (xbit)
    {
        rtpHeader += 2;
        uint16 len16 = 0;
        oscl_memcpy((char *)&len16, rtpHeader, sizeof(len16));
        big_endian_to_host((char *)&len16, sizeof(len16));
        uint32 extensionHeaderLen = (uint32)len16;
        rtpPacketLen -= 4;
        if ((extensionHeaderLen*4) > rtpPacketLen)
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: Corrupt Extended Header Length", this));
            return false;
        }
        rtpHeader += (extensionHeaderLen * 4);
        rtpPacketLen -= (extensionHeaderLen * 4);
    }

    /* Ignore padding octets */
    if (numPaddingOctets)
    {
        if ((uint32)numPaddingOctets > rtpPacketLen)
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: Corrupt Padding Length", this));
            return false;
        }
        rtpPacketLen -= numPaddingOctets;
    }

    if (iInPlaceProcessing == false)
    {
        OsclSharedPtr<PVMFMediaDataImpl> mediaDataOut;
        int32 err;
        OSCL_TRY(err,
                 mediaDataOut = iMediaDataGroupAlloc->allocate());

        if (err != OsclErrNone)
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: Jitter Buffer Full", this));
            return false;
        }

        OsclRefCounterMemFrag memFragOut(memFragIn);
        memFragOut.getMemFrag().ptr = rtpHeader;
        memFragOut.getMemFrag().len = rtpPacketLen;

        mediaDataOut->appendMediaFragment(memFragOut);

        uint32 markerInfo = 0;
        if (mbit == 1) markerInfo |= PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
        mediaDataOut->setMarkerInfo(markerInfo);

        OSCL_TRY(err,
                 dataPacket = PVMFMediaData::createMediaData(mediaDataOut,
                              iMediaMsgMemPool););

        if (err != OsclErrNone)
        {
            PVMF_JBNODE_LOGERROR((0, "0x%x PVMFJitterBufferImpl::ParseRTPHeader: Jitter Buffer Full", this));
            return false;
        }

        dataPacket->setTimestamp(rtpTimeStamp);
        dataPacket->setStreamID(SSRC);
        dataPacket->setSeqNum(seqNum);
    }
    else
    {
        memFragIn.getMemFrag().ptr = rtpHeader;
        memFragIn.getMemFrag().len = rtpPacketLen;
        mediaDataIn->clearMediaFragments();
        mediaDataIn->appendMediaFragment(memFragIn);
        uint32 markerInfo = 0;
        if (mbit == 1) markerInfo |= PVMF_MEDIA_DATA_MARKER_INFO_M_BIT;
        mediaDataIn->setMarkerInfo(markerInfo);

        rtpPacket->setTimestamp(rtpTimeStamp);
        rtpPacket->setStreamID(SSRC);
        rtpPacket->setSeqNum(seqNum);
    }

    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::ParseRTPHeader: SSRC=%d, rtpSeqNum=%d, rtpTs=%d, rtpPacketLen=%d",
                                   SSRC, seqNum, rtpTimeStamp, rtpPacketLen));

    return true;
}


void PVMFJitterBufferImpl::DeterminePrevTimeStampPeek(uint32 aSeqNum,
        PVMFTimestamp& aPrevTS)
{
    if (iHeaderFormat == PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP)
    {
        PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(aSeqNum);
        if (rtpInfoParams == NULL)
        {
            if (iRTPInfoParamsVec.size() == 0)
            {
                PVMF_JBNODE_LOGWARNING((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: RTPInfoVec Empty"));
                /* Use the value from the first packet */
                aPrevTS = seqLockTimeStamp;
            }
            else
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::DeterminePrevTimeStampPeek: illegal seq number"));
                OSCL_LEAVE(OsclErrArgument);
            }
            return; // no need to continue here
        }
        else
        {
            if (rtpInfoParams->seqNum > iPrevSeqNumBaseOut)
            {
                aPrevTS = rtpInfoParams->rtpTime;
            }
            else
            {
                aPrevTS = iPrevTSOut;
            }
        }
    }
    else
    {
        aPrevTS = iPrevTSOut;
    }
}

void PVMFJitterBufferImpl::DeterminePrevTimeStamp(uint32 aSeqNum)
{
    if (iHeaderFormat == PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP)
    {
        PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(aSeqNum);
        if (rtpInfoParams == NULL)
        {
            if (iRTPInfoParamsVec.size() == 0)
            {
                PVMF_JBNODE_LOGWARNING((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: RTPInfoVec Empty"));
                /* Use the value from the first packet */
                iPrevTSOut = seqLockTimeStamp;
                iPrevTSIn = seqLockTimeStamp;
                iPrevAdjustedRTPTS = seqLockTimeStamp;
                iPrevSeqNumBaseOut = iFirstSeqNum;
                iPrevSeqNumBaseIn = iFirstSeqNum;

                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: illegal seq number = %d", aSeqNum));
                return;
            }
            else
            {
                PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: illegal seq number = %d", aSeqNum));
                OSCL_LEAVE(OsclErrArgument);
            }
        }
        if (rtpInfoParams->seqNum > iPrevSeqNumBaseOut)
        {
            iPrevSeqNumBaseOut = rtpInfoParams->seqNum;
            iPrevTSOut = rtpInfoParams->rtpTime;
        }
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::DeterminePrevTimeStamp: RTPInfoSeqNum=%d, iPrevSeqNumBaseOut=%d, iPrevTSOut=%d", rtpInfoParams->seqNum, iPrevSeqNumBaseOut, iPrevTSOut));
    }
}

void PVMFJitterBufferImpl::ComputeMaxAdjustedRTPTS()
{
    PVMFJitterBufferStats jbStats = getJitterBufferStats();

    uint32 aSeqNum = jbStats.maxSeqNumReceived;
    PVMFTimestamp aTS = jbStats.maxTimeStampRegistered;

    PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(aSeqNum);
    if (rtpInfoParams == NULL)
    {
        PVMF_JBNODE_LOGERROR((0, "PVMFJitterBufferImpl::ComputeMaxAdjustedRTPTS: illegal seq number"));
        OSCL_LEAVE(OsclErrArgument);
    }

    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::ComputeMaxAdjustedRTPTS - maxSeqNumReceived=%d, rtpInfoParams->seqNum=%d, iPrevSeqNumBaseIn=%d, Mime=%s",
                         aSeqNum, rtpInfoParams->seqNum, iPrevSeqNumBaseIn, iMimeType.get_cstr()));

    if (rtpInfoParams->seqNum > iPrevSeqNumBaseIn)
    {
        iPrevSeqNumBaseIn = rtpInfoParams->seqNum;
        iPrevTSIn = rtpInfoParams->rtpTime;
    }
    iMaxAdjustedRTPTS += (aTS - iPrevAdjustedRTPTS);
    iPrevAdjustedRTPTS = aTS;

    PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::ComputeMaxAdjustedRTPTS - maxTimeStampRegistered=%d, iPrevAdjustedRTPTS=%d, iMaxAdjustedRTPTS=%d, Mime=%s",
                         aTS, iPrevAdjustedRTPTS, iMaxAdjustedRTPTS, iMimeType.get_cstr()));

    UpdateEstimatedServerClock();
}

void
PVMFJitterBufferImpl::PurgeElementsWithSeqNumsLessThan(uint32 aSeqNum,
        uint32 aPlayerClockMS)
{
    iJitterBuffer->PurgeElementsWithSeqNumsLessThan(aSeqNum);

    {
        iMaxAdjustedRTPTS =
            Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp);
        /*
         * In case of 3GPP streaming this clock adjustment is performed
         * at a later point, via the "AdjustRTPTimeStamp" API call from
         * jitter buffer node.
         */
    }
    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::PurgeElementsWithSeqNumsLessThan - SeqNum=%d",
                                   aSeqNum));
}

void
PVMFJitterBufferImpl::PurgeElementsWithTimestampLessThan(PVMFTimestamp aTS)
{
    PVMFRTPInfoParams* rtpInfoParams = FindRTPInfoParams(0x80000000);
    PVMFTimestamp rtpTS = rtpInfoParams->rtpTime + aTS;
    iJitterBuffer->PurgeElementsWithTimestampLessThan(rtpTS);
    iMaxAdjustedRTPTS = aTS;
    UpdateEstimatedServerClock(true);
    iMonotonicTimeStamp += (rtpTS - iPrevTSOut);
    iPrevTSOut = rtpTS;
    PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::PurgeElementsWithTimestampLessThan - ntpTS=%d, rtpTS=%d",
                                   aTS, rtpTS));
}

void PVMFJitterBufferImpl::UpdateEstimatedServerClock(bool oFreshStart)
{
    uint32 rtpTSInMS;
    uint64 currentTime64 = 0;
    uint64 currentTimeBase64 = 0;
    uint64 adjustTime64 = 0;

    if (oFreshStart)
    {
        uint32 in_wrap_count = 0;
        iEstServClockMediaClockConvertor.set_clock(iMaxAdjustedRTPTS, in_wrap_count);
        rtpTSInMS = iEstServClockMediaClockConvertor.get_converted_ts(1000);
        iEstimatedServerClock->Stop();
        iEstimatedServerClock->SetStartTime32(rtpTSInMS,
                                              OSCLCLOCK_MSEC);
        iEstimatedServerClock->Start();
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Setting start time - MaxAdjustedRTPTS=%d, StartTime=%d",
                              iMaxAdjustedRTPTS, rtpTSInMS));
    }
    else
    {
        iEstServClockMediaClockConvertor.update_clock(iMaxAdjustedRTPTS);
        rtpTSInMS = iEstServClockMediaClockConvertor.get_converted_ts(1000);
        Oscl_Int64_Utils::set_uint64(adjustTime64, 0, rtpTSInMS);
        iEstimatedServerClock->GetCurrentTime64(currentTime64,
                                                OSCLCLOCK_MSEC,
                                                currentTimeBase64);
        {
            iEstimatedServerClock->AdjustClockTime64(currentTime64,
                    currentTimeBase64,
                    adjustTime64,
                    OSCLCLOCK_MSEC);
        }
        iEstimatedServerClock->GetCurrentTime64(currentTime64,
                                                OSCLCLOCK_MSEC,
                                                currentTimeBase64);

        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Mime=%s",
                             iMimeType.get_cstr()));
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - EstServClock=%2d",
                             currentTime64));
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - RTPTime64=%2d",
                             adjustTime64));
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Adjusting Clock - iMaxAdjustedRTPTS=%d, currentTimeBase64=%2d",
                             iMaxAdjustedRTPTS, currentTimeBase64));
    }
    if (iServerClockUpdateNotificationObserver != NULL)
    {
        iServerClockUpdateNotificationObserver->EstimatedServerClockUpdated(iServerClockUpdateNotificationObserverContext);
    }
}

void PVMFJitterBufferImpl::UpdateEstimatedServerClockDiscrete(bool oFreshStart)
{
    uint32 rtpTSInMS;
    uint64 currentTime64 = 0;
    uint64 currentTimeBase64 = 0;
    uint64 adjustTime64 = 0;

    if (oFreshStart)
    {
        uint32 in_wrap_count = 0;
        iEstServClockMediaClockConvertor.set_clock(iMaxAdjustedRTPTS, in_wrap_count);
        rtpTSInMS = iEstServClockMediaClockConvertor.get_converted_ts(1000);
        iEstimatedServerClock->Stop();
        iEstimatedServerClock->SetStartTime32(rtpTSInMS,
                                              OSCLCLOCK_MSEC);
        iEstimatedServerClock->Start();
        iEstimatedServerClock->Pause();
        PVMF_JBNODE_LOGCLOCK((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClock - Setting start time - MaxAdjustedRTPTS=%d, StartTime=%d",
                              iMaxAdjustedRTPTS, rtpTSInMS));
    }
    else
    {
        iEstServClockMediaClockConvertor.update_clock(iMaxAdjustedRTPTS);
        rtpTSInMS = iEstServClockMediaClockConvertor.get_converted_ts(1000);
        Oscl_Int64_Utils::set_uint64(adjustTime64, 0, rtpTSInMS);
        iEstimatedServerClock->GetCurrentTime64(currentTime64,
                                                OSCLCLOCK_MSEC,
                                                currentTimeBase64);
        iEstimatedServerClock->Stop();
        iEstimatedServerClock->SetStartTime64(adjustTime64, OSCLCLOCK_MSEC);
        iEstimatedServerClock->Start();
        iEstimatedServerClock->Pause();
        iEstimatedServerClock->GetCurrentTime64(currentTime64,
                                                OSCLCLOCK_MSEC,
                                                currentTimeBase64);
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClockDiscrete - Mime=%s",
                             iMimeType.get_cstr()));
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClockDiscrete - EstServClock=%2d",
                             currentTime64));
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClockDiscrete - RTPTime64=%2d",
                             adjustTime64));
        PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::UpdateEstimatedServerClockDiscrete - Adjusting Clock - iMaxAdjustedRTPTS=%d, currentTimeBase64=%2d",
                             iMaxAdjustedRTPTS, currentTimeBase64));
    }
    if (iServerClockUpdateNotificationObserver != NULL)
    {
        iServerClockUpdateNotificationObserver->EstimatedServerClockUpdated(iServerClockUpdateNotificationObserverContext);
    }
}

void PVMFJitterBufferImpl::SetInPlaceProcessingMode(bool aInPlaceProcessingMode)
{
    iInPlaceProcessing = aInPlaceProcessingMode;
    if (iInPlaceProcessing == false)
    {
        /* Create allocators if not already present */
        uint32 numNodes = 0;
#if (PMVF_JITTER_BUFFER_NODE_USE_NO_RESIZE_ALLOC)
        numNodes = PVMF_JB_NO_RESIZE_ALLOC_NUM_CHUNKS_RTP;
#else
        numNodes = DEFAULT_NUM_MEDIA_MSGS_IN_JITTER_BUFFER;
#endif
        if ((iMediaDataImplMemPool == NULL) && (iMediaDataGroupAlloc == NULL))
        {
            OsclExclusivePtr<PoolMemAlloc> mediaDataImplMemPoolAutoPtr;
            typedef OsclMemPoolFixedChunkAllocator osclmempoolAllocType;
            PVMF_JITTER_BUFFER_NEW(NULL,
                                   osclmempoolAllocType,
                                   (numNodes),
                                   iMediaDataImplMemPool);
            mediaDataImplMemPoolAutoPtr.set(iMediaDataImplMemPool);

            typedef PVMFMediaFragGroupCombinedAlloc<PoolMemAlloc> mediaFragGroupType;
            OsclExclusivePtr<mediaFragGroupType> mediaDataGroupAutoPtr;
            PVMF_JITTER_BUFFER_NEW(NULL,
                                   mediaFragGroupType,
                                   (numNodes,
                                    DEFAULT_NUM_FRAGMENTS_IN_MEDIA_MSG,
                                    iMediaDataImplMemPool),
                                   iMediaDataGroupAlloc);
            mediaDataGroupAutoPtr.set(iMediaDataGroupAlloc);

            iMediaDataGroupAlloc->create();

            OsclExclusivePtr<PoolMemAlloc> mediaMsgMemPoolAutoPtr;
            PVMF_JITTER_BUFFER_NEW(NULL,
                                   osclmempoolAllocType,
                                   (numNodes),
                                   iMediaMsgMemPool);
            mediaMsgMemPoolAutoPtr.set(iMediaMsgMemPool);

            mediaDataImplMemPoolAutoPtr.release();
            mediaDataGroupAutoPtr.release();
            mediaMsgMemPoolAutoPtr.release();
        }
    }
    else
    {
        /* Destroy allocators if present */
        if (iMediaDataGroupAlloc != NULL)
        {
            iMediaDataGroupAlloc->removeRef();
            iMediaDataGroupAlloc = NULL;
        }
        if (iMediaDataImplMemPool != NULL)
        {
            iMediaDataImplMemPool->removeRef();
            iMediaDataImplMemPool = NULL;
        }
        if (iMediaMsgMemPool != NULL)
        {
            iMediaMsgMemPool->removeRef();
            iMediaMsgMemPool = NULL;
        }
    }
}

