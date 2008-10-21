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
#ifndef PVMF_JITTER_BUFFER_H_INCLUDED
#define PVMF_JITTER_BUFFER_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif
#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_TIMESTAMP_H_INCLUDED
#include "pvmf_timestamp.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_MEDIA_FRAG_GROUP_H_INCLUDED
#include "pvmf_media_frag_group.h"
#endif
#ifndef PVMF_JITTER_BUFFER_EXT_INTERFACE_H_INCLUDED
#include "pvmf_jitter_buffer_ext_interface.h"
#endif
#ifndef PVMF_JITTER_BUFFER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif
#ifndef __MEDIA_CLOCK_CONVERTER_H
#include "media_clock_converter.h"
#endif
#ifndef PVMF_MEDIA_MSG_FORMAT_IDS_H_INCLUDED
#include "pvmf_media_msg_format_ids.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

#define PVMF_JITTER_BUFFER_ROLL_OVER_THRESHOLD_16BIT 2000
#define PVMF_JITTER_BUFFER_ROLL_OVER_THRESHOLD_32BIT 20000

/* RTP HEADER CONSTANTS */
#define SUPPORTED_RTP_HEADER_VERSION 2
#define RTP_FIXED_HEADER_SIZE        12
#define RTP_HEADER_V_BIT_MASK   0xC0
#define RTP_HEADER_V_BIT_OFFSET 6
#define RTP_HEADER_P_BIT_MASK   0x20
#define RTP_HEADER_P_BIT_OFFSET 5
#define RTP_HEADER_X_BIT_MASK   0x10
#define RTP_HEADER_X_BIT_OFFSET 4
#define RTP_HEADER_CC_BIT_MASK  0x0F
#define RTP_HEADER_M_BIT_MASK   0x80
#define RTP_HEADER_M_BIT_OFFSET 7
#define RTP_HEADER_PT_MASK      0x7F

/* ASF HEADER CONSTANTS */
#define ASF_DATA_PACKET_HEADER_ERROR_CORRECTION_PRESENT_MASK     0x80
#define ASF_DATA_PACKET_HEADER_ERROR_CORRECTION_DATA_LENGTH_MASK 0x07
#define ASF_DATA_PACKET_HEADER_SEQUENCE_LENGTH_TYPE_OFFSET       1
#define ASF_DATA_PACKET_HEADER_SEQUENCE_LENGTH_TYPE_MASK         0x06
#define ASF_DATA_PACKET_HEADER_PADDING_LENGTH_TYPE_OFFSET        3
#define ASF_DATA_PACKET_HEADER_PADDING_LENGTH_TYPE_MASK          0x18
#define ASF_DATA_PACKET_HEADER_PACKET_LENGTH_TYPE_OFFSET         5
#define ASF_DATA_PACKET_HEADER_PACKET_LENGTH_TYPE_MASK           0x60
/*
 * 1 - Error correction flags
 * 7 (worstcase) - Error correction data length
 * 1 - Length Type;
 * 1 - Property Flags;
 * 4 each(worst case) - PacketLength, Sequence, PaddingLength
 * 4 - Send Time
 * 2 - Duration
 */
#define MAX_ASF_DATA_PACKET_HEADER_SIZE_IN_BYTES   28

#define PVMF_JITTER_BUFFER_BUFFERING_STATUS_TIMER_ID 1

enum PVMFJitterBufferDataState
{
    PVMF_JITTER_BUFFER_STATE_UNKNOWN,
    PVMF_JITTER_BUFFER_READY,
    PVMF_JITTER_BUFFER_IN_TRANSITION
};

enum PVMFJitterBufferTransportHeaderFormat
{
    PVMF_JITTER_BUFFER_TRANSPORT_HEADER_FORMAT_UNKNOWN,
    PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP,
    PVMF_JITTER_BUFFER_TRANSPORT_HEADER_ASF
};

typedef enum
{
    PVMF_JITTER_BUFFER_ADD_ELEM_ERROR,
    PVMF_JITTER_BUFFER_ADD_ELEM_SUCCESS,
    PVMF_JITTER_BUFFER_ADD_ELEM_UNEXPECTED_DATA
} PVMFJitterBufferAddElemStatus;

typedef struct tagPVMFJitterBufferStats
{
    uint32 ssrc;
    uint32 totalNumPacketsReceived;
    uint32 totalNumPacketsRegistered;
    uint32 totalNumPacketsRetrieved;
    uint32 maxSeqNumReceived;
    uint32 maxSeqNumRegistered;
    uint32 totalPacketsLost;
    uint32 seqNumBase;
    uint32 currentOccupancy;
    PVMFTimestamp maxTimeStampRegistered;
    PVMFTimestamp maxTimeStampRetrieved;
    PVMFTimestamp maxTimeStampRetrievedWithoutRTPOffset;
    uint32 maxOccupancy;
    uint32 lastRegisteredSeqNum;
    uint32  lastRetrievedSeqNum;
    uint32 totalNumBytesRecvd;
    uint32 packetSizeInBytesLeftInBuffer;
} PVMFJitterBufferStats;

typedef struct tagPVMFRTPInfoParams
{
    tagPVMFRTPInfoParams()
    {
        seqNumBaseSet = false;
        seqNum = 0;
        rtpTimeBaseSet = false;
        rtpTime = 0;
        nptTimeInMS = 0;
        rtpTimeScale = 0;
        nptTimeInRTPTimeScale = 0;
    };

    bool   seqNumBaseSet;
    uint32 seqNum;
    bool   rtpTimeBaseSet;
    uint32 rtpTime;
    uint32 nptTimeInMS;
    uint32 rtpTimeScale;
    uint32 nptTimeInRTPTimeScale;
} PVMFRTPInfoParams;

class MediaCommandMsgHolder
{
    public:
        MediaCommandMsgHolder()
        {
            iPreceedingMediaMsgSeqNumber = 0xFFFFFFFF;
        };

        MediaCommandMsgHolder(const MediaCommandMsgHolder& a)
        {
            iPreceedingMediaMsgSeqNumber = a.iPreceedingMediaMsgSeqNumber;
            iCmdMsg = a.iCmdMsg ;
        }

        MediaCommandMsgHolder& operator=(const MediaCommandMsgHolder& a)
        {
            if (&a != this)
            {
                iPreceedingMediaMsgSeqNumber = a.iPreceedingMediaMsgSeqNumber;
                iCmdMsg = a.iCmdMsg ;
            }
            return (*this);
        }

        uint32 iPreceedingMediaMsgSeqNumber;
        PVMFSharedMediaMsgPtr iCmdMsg;
};

template<class Alloc>
class PVMFDynamicCircularArray
{
    public:
        PVMFDynamicCircularArray(const PvmfMimeString* aMimeType = NULL)
        {
            numElems = 0;
            arraySize = 0;
            maxSeqNumAdded = 0;
            lastRetrievedSeqNum = 0;
            lastRetrievedTS = 0;
            readOffset = 0;
            firstSeqNumAdded = 0;

            sJitterBufferParams.ssrc = 0;
            sJitterBufferParams.totalNumPacketsReceived = 0;
            sJitterBufferParams.totalNumPacketsRegistered = 0;
            sJitterBufferParams.totalNumPacketsRetrieved = 0;
            sJitterBufferParams.maxSeqNumReceived = 0;
            sJitterBufferParams.maxSeqNumRegistered = 0;
            sJitterBufferParams.totalPacketsLost = 0;
            sJitterBufferParams.seqNumBase = 0;
            sJitterBufferParams.currentOccupancy = 0;
            sJitterBufferParams.maxTimeStampRegistered = 0;
            sJitterBufferParams.maxTimeStampRetrieved = 0;
            sJitterBufferParams.maxTimeStampRetrievedWithoutRTPOffset = 0;
            sJitterBufferParams.lastRegisteredSeqNum = 0;
            sJitterBufferParams.lastRetrievedSeqNum = lastRetrievedSeqNum;
            sJitterBufferParams.maxOccupancy = arraySize;
            sJitterBufferParams.totalNumBytesRecvd = 0;
            sJitterBufferParams.packetSizeInBytesLeftInBuffer = 0;

            iLogger = PVLogger::GetLoggerObject("PVMFDynamicCircularArray");
            iDataPathLoggerIn = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.in");
            iDataPathLoggerOut = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.out");
            if (aMimeType != NULL)
            {
                iMimeType = aMimeType->get_cstr();
            }
            iHeaderFormat = PVMF_JITTER_BUFFER_TRANSPORT_HEADER_FORMAT_UNKNOWN;
            iBroadCastSession = false;
        }

        PVMFDynamicCircularArray(uint32 n,
                                 const PvmfMimeString* aMimeType = NULL)
        {
            numElems = 0;
            arraySize = n;
            iMediaPtrVec.reserve(arraySize);
            InitVector(arraySize);
            maxSeqNumAdded = 0;
            lastRetrievedSeqNum = 0;
            lastRetrievedTS = 0;
            readOffset = 0;
            firstSeqNumAdded = 0;

            sJitterBufferParams.ssrc = 0;
            sJitterBufferParams.totalNumPacketsReceived = 0;
            sJitterBufferParams.totalNumPacketsRegistered = 0;
            sJitterBufferParams.totalNumPacketsRetrieved = 0;
            sJitterBufferParams.maxSeqNumReceived = 0;
            sJitterBufferParams.maxSeqNumRegistered = 0;
            sJitterBufferParams.totalPacketsLost = 0;
            sJitterBufferParams.seqNumBase = 0;
            sJitterBufferParams.currentOccupancy = 0;
            sJitterBufferParams.maxTimeStampRegistered = 0;
            sJitterBufferParams.maxTimeStampRetrieved = 0;
            sJitterBufferParams.maxTimeStampRetrievedWithoutRTPOffset = 0;
            sJitterBufferParams.lastRegisteredSeqNum = 0;
            sJitterBufferParams.lastRetrievedSeqNum = lastRetrievedSeqNum;
            sJitterBufferParams.maxOccupancy = arraySize;
            sJitterBufferParams.totalNumBytesRecvd = 0;
            sJitterBufferParams.packetSizeInBytesLeftInBuffer = 0;

            iLogger = PVLogger::GetLoggerObject("PVMFDynamicCircularArray");
            iDataPathLoggerIn = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.in");
            iDataPathLoggerOut = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.out");
            if (aMimeType != NULL)
            {
                iMimeType = aMimeType->get_cstr();
            }
            iHeaderFormat = PVMF_JITTER_BUFFER_TRANSPORT_HEADER_FORMAT_UNKNOWN;
            iBroadCastSession = false;
        }

        virtual ~PVMFDynamicCircularArray()
        {
            if (!iMediaPtrVec.empty())
            {
                typedef typename Oscl_Vector<PVMFSharedMediaDataPtr, Alloc>::iterator iterator_type;
                iterator_type it;

                for (it = iMediaPtrVec.begin(); it != iMediaPtrVec.end(); it++)
                {
                    if (it->GetRep() != NULL)
                    {
                        it->Unbind();
                    }
                }
            }
        }

        void Clear()
        {
            typedef typename Oscl_Vector<PVMFSharedMediaDataPtr, Alloc>::iterator iterator_type;
            iterator_type it;
            for (it = iMediaPtrVec.begin(); it != iMediaPtrVec.end(); it++)
            {
                if (it->GetRep() != NULL)
                {
                    it->Unbind();
                }
            }
            numElems = 0;
            iMediaPtrVec.clear();

            InitVector(arraySize);
            maxSeqNumAdded = 0;
            lastRetrievedSeqNum = 0;
            lastRetrievedTS = 0;
            readOffset = 0;
            firstSeqNumAdded = 0;
            sJitterBufferParams.currentOccupancy = 0;
        }

        void ResetJitterBufferStats()
        {
            sJitterBufferParams.ssrc = 0;
            sJitterBufferParams.totalNumPacketsReceived = 0;
            sJitterBufferParams.totalNumPacketsRegistered = 0;
            sJitterBufferParams.totalNumPacketsRetrieved = 0;
            sJitterBufferParams.maxSeqNumReceived = 0;
            sJitterBufferParams.maxSeqNumRegistered = 0;
            sJitterBufferParams.totalPacketsLost = 0;
            sJitterBufferParams.seqNumBase = 0;
            sJitterBufferParams.currentOccupancy = 0;
            sJitterBufferParams.maxTimeStampRegistered = 0;
            sJitterBufferParams.maxTimeStampRetrieved = 0;
            sJitterBufferParams.maxTimeStampRetrievedWithoutRTPOffset = 0;
            sJitterBufferParams.lastRetrievedSeqNum = lastRetrievedSeqNum;
            sJitterBufferParams.maxOccupancy = arraySize;
            sJitterBufferParams.totalNumBytesRecvd = 0;
            sJitterBufferParams.lastRegisteredSeqNum = 0;
            sJitterBufferParams.totalNumBytesRecvd = 0;
            sJitterBufferParams.packetSizeInBytesLeftInBuffer = 0;
        }

        uint32 getNumElements()
        {
            return numElems;
        }

        uint32 getArraySize()
        {
            return arraySize;
        }

        void growCircularArray(uint32 newSize)
        {
            if (newSize > arraySize)
            {
                /*
                 * This transfers the existing contents
                 * as well
                 */
                iMediaPtrVec.reserve(newSize);
                /* Initialize the new space */
                InitVector((newSize - arraySize));
                arraySize = newSize;
            }
        }

        void setFirstSeqNumAdded(uint32 aFirstSeqNumAdded)
        {
            firstSeqNumAdded = aFirstSeqNumAdded;
        }

        PVMFJitterBufferAddElemStatus addElement(PVMFSharedMediaDataPtr& elem, uint32 aSeqNumBase)
        {
            PVMFJitterBufferAddElemStatus oRet = PVMF_JITTER_BUFFER_ADD_ELEM_SUCCESS;

            sJitterBufferParams.totalNumPacketsReceived++;
            sJitterBufferParams.ssrc = elem->getStreamID();
            uint32 seqNum = elem->getSeqNum();
            /* Get packet size */
            uint32 size = 0;
            uint32 numFragments = elem->getNumFragments();
            for (uint32 i = 0; i < numFragments; i++)
            {
                OsclRefCounterMemFrag memFragIn;
                elem->getMediaFragment(i, memFragIn);
                size += memFragIn.getMemFrag().len;
            }
            sJitterBufferParams.totalNumBytesRecvd += size;
            sJitterBufferParams.packetSizeInBytesLeftInBuffer += size;

            /* If it is the first packet always register it */
            if (seqNum != aSeqNumBase)
            {
                if (elem->getTimestamp() < lastRetrievedTS)
                {
                    if (seqNum > lastRetrievedSeqNum)
                    {
                        lastRetrievedSeqNum = seqNum;
                        readOffset = (seqNum - aSeqNumBase) % arraySize;
                    }
                    return PVMF_JITTER_BUFFER_ADD_ELEM_ERROR;
                }

                if ((iHeaderFormat == PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP) &&
                        (iBroadCastSession == true))
                {
                    /*
                     * This can happen when using prerecorded transport streams that loop
                     * If this happens, just signal an unexpected data event
                     */
                    if (elem->getTimestamp() < sJitterBufferParams.maxTimeStampRegistered)
                    {
                        return PVMF_JITTER_BUFFER_ADD_ELEM_UNEXPECTED_DATA;
                    }
                }

                if (seqNum <= lastRetrievedSeqNum)
                {
                    /*
                     * check for seqnum roll over. For RTP packets, seqnum is 16 bits
                     * therefore a typecast is needed before we check for rollover
                     */
                    if (iHeaderFormat == PVMF_JITTER_BUFFER_TRANSPORT_HEADER_RTP)
                    {
                        uint16 seqNum16 = (uint16)seqNum;
                        uint16 lastRetrievedSeqNum16 = (uint16)(lastRetrievedSeqNum + 1);
                        uint16 diff16 = (seqNum16 - lastRetrievedSeqNum16);
                        if (diff16 > PVMF_JITTER_BUFFER_ROLL_OVER_THRESHOLD_16BIT)
                        {
                            /* too late - discard the packet */
                            oRet = PVMF_JITTER_BUFFER_ADD_ELEM_ERROR;
                            return (oRet);
                        }
                        else
                        {
                            /* Reset these variables to acct for seqnum rollover */
                            sJitterBufferParams.maxSeqNumRegistered = seqNum;
                            sJitterBufferParams.maxSeqNumReceived = seqNum;
                            sJitterBufferParams.maxTimeStampRegistered = elem->getTimestamp();
                        }
                    }
                    else
                    {
                        /* Seqnum is 32 bits, no typecase needed */
                        if ((seqNum - lastRetrievedSeqNum) > PVMF_JITTER_BUFFER_ROLL_OVER_THRESHOLD_32BIT)
                        {
                            /* too late - discard the packet */
                            oRet = PVMF_JITTER_BUFFER_ADD_ELEM_ERROR;
                            return (oRet);
                        }
                        else
                        {
                            /* Reset these variables to acct for seqnum rollover */
                            sJitterBufferParams.maxSeqNumRegistered = seqNum;
                            sJitterBufferParams.maxSeqNumReceived = seqNum;
                            sJitterBufferParams.maxTimeStampRegistered = elem->getTimestamp();
                        }
                    }
                }
            }

            if (seqNum > sJitterBufferParams.maxSeqNumRegistered)
            {
                sJitterBufferParams.maxSeqNumReceived = seqNum;
            }
            uint32 offset = (seqNum - aSeqNumBase) % arraySize;

            PVMFSharedMediaDataPtr currElem = iMediaPtrVec[offset];
            if (currElem.GetRep() == NULL)
            {
                /* Register Packet */
                iMediaPtrVec[offset] = elem;
                numElems++;
                sJitterBufferParams.totalNumPacketsRegistered++;
                sJitterBufferParams.lastRegisteredSeqNum = seqNum;
                if (seqNum > sJitterBufferParams.maxSeqNumRegistered)
                {
                    sJitterBufferParams.maxSeqNumRegistered = seqNum;
                    sJitterBufferParams.maxTimeStampRegistered = elem->getTimestamp();
                }
                sJitterBufferParams.currentOccupancy = numElems;
                return (oRet);
            }
            else if (currElem->getSeqNum() != seqNum)
            {
                PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "JB OW: MimeType=%s, ReadOffset=%d, NumElemsLeft=%d, lastRetrievedSeqNum=%d, old seqNum=%d, new seqNum=%d",
                                               iMimeType.get_cstr(),
                                               readOffset,
                                               numElems,
                                               lastRetrievedSeqNum,
                                               seqNum,
                                               currElem->getSeqNum()));
                /* Overwrite existing data */
                currElem.Unbind();
                /* Register Packet */
                iMediaPtrVec[offset] = elem;
                sJitterBufferParams.totalNumPacketsRegistered++;
                sJitterBufferParams.lastRegisteredSeqNum = seqNum;
                if (seqNum > sJitterBufferParams.maxSeqNumRegistered)
                {
                    sJitterBufferParams.maxSeqNumRegistered = seqNum;
                    sJitterBufferParams.maxTimeStampRegistered = elem->getTimestamp();
                }
                sJitterBufferParams.currentOccupancy = numElems;
                return (oRet);
            }
            /* Duplicate Packet - Ignore */
            return (oRet);
        }

        PVMFSharedMediaDataPtr retrieveElement()
        {
            PVMFSharedMediaDataPtr dataPkt;
            uint32 count = 0;
            while (dataPkt.GetRep() == NULL)
            {
                /* No data */
                if (count > arraySize)
                {
                    dataPkt.Unbind();
                    return dataPkt;
                }
                /* Wrap around */
                if (readOffset >= arraySize)
                {
                    readOffset = 0;
                }
                dataPkt = iMediaPtrVec[readOffset];
                readOffset++;
                count++;
            }
            numElems--;
            /* Mark the retrieved element location as free */
            PVMFSharedMediaDataPtr retElem = iMediaPtrVec[readOffset-1];
            retElem.Unbind();
            iMediaPtrVec[readOffset-1] = retElem;
            lastRetrievedSeqNum = (int32)(dataPkt.GetRep()->getSeqNum());
            /* Check and register packet loss */
            sJitterBufferParams.totalPacketsLost += (count - 1);
            sJitterBufferParams.maxTimeStampRetrieved = dataPkt->getTimestamp();
            sJitterBufferParams.currentOccupancy = numElems;
            sJitterBufferParams.totalNumPacketsRetrieved++;
            sJitterBufferParams.lastRetrievedSeqNum = lastRetrievedSeqNum;
            /* Get packet size */
            uint32 size = 0;
            uint32 numFragments = dataPkt->getNumFragments();
            for (uint32 i = 0; i < numFragments; i++)
            {
                OsclRefCounterMemFrag memFragIn;
                dataPkt->getMediaFragment(i, memFragIn);
                size += memFragIn.getMemFrag().len;
            }
            sJitterBufferParams.packetSizeInBytesLeftInBuffer -= size;
            PVMF_JBNODE_LOGINFO((0, "PVMFDynamicCircularArray::retrieveElement: MimeType=%s, ReadOffset=%d, NumElemsLeft=%d, SeqNum=%d",
                                 iMimeType.get_cstr(),
                                 readOffset,
                                 numElems,
                                 lastRetrievedSeqNum));

            return (dataPkt);
        }

        PVMFJitterBufferStats getStats()
        {
            return sJitterBufferParams;
        }
        PVMFJitterBufferStats* getStatsPtr()
        {
            return &sJitterBufferParams;
        }

        void peekNextElementTimeStamp(PVMFTimestamp& aTS,
                                      uint32& aSeqNum)
        {
            PVMFSharedMediaDataPtr dataPkt;

            uint32 peekOffset = readOffset;
            uint32 count = 0;

            while (dataPkt.GetRep() == NULL)
            {
                if (count > numElems)
                {
                    aTS = 0xFFFFFFFF;
                }
                if (peekOffset >= arraySize)
                {
                    peekOffset = 0;
                }
                dataPkt = iMediaPtrVec[peekOffset];
                peekOffset++;
                count++;
            }
            aTS = dataPkt.GetRep()->getTimestamp();
            aSeqNum = dataPkt.GetRep()->getSeqNum();
            return;
        }

        bool CheckCurrentReadPosition()
        {
            uint32 offset = readOffset;

            if (offset >= arraySize)
            {
                offset = 0;
            }

            PVMFSharedMediaDataPtr dataPkt =
                iMediaPtrVec[offset];

            if (dataPkt.GetRep() == NULL)
            {
                return false;
            }
            return true;
        }

        bool CheckSpaceAvailability()
        {
            if (numElems < arraySize)
            {
                return true;
            }
            return false;
        }

        void setRTPInfoParams(PVMFRTPInfoParams rtpInfo)
        {
            sJitterBufferParams.seqNumBase = rtpInfo.seqNum;
        }

        PVMFSharedMediaDataPtr getElementAt(uint32 aIndex)
        {
            if (aIndex > arraySize) OSCL_LEAVE(OsclErrArgument);
            return (iMediaPtrVec[aIndex]);
        }

        void AddElementAt(PVMFSharedMediaDataPtr aMediaPtr,
                          uint32 aIndex)
        {
            if (aIndex > arraySize) OSCL_LEAVE(OsclErrArgument);
            iMediaPtrVec[aIndex] = aMediaPtr;
        }

        void PurgeElementsWithSeqNumsLessThan(uint32 aSeqNum)
        {
            if (!iMediaPtrVec.empty())
            {
                if (aSeqNum < lastRetrievedSeqNum)
                {
                    typedef typename Oscl_Vector<PVMFSharedMediaDataPtr, Alloc>::iterator iterator_type;
                    iterator_type it;
                    for (it = iMediaPtrVec.begin(); it != iMediaPtrVec.end(); it++)
                    {
                        if (it->GetRep() != NULL)
                        {
                            it->Unbind();
                        }
                    }
                    numElems = 0;
                    //iMediaPtrVec.clear();
                }
                else if (aSeqNum > lastRetrievedSeqNum)
                {
                    /*
                     * Start from last retrieved seq num and offset it by
                     * first seq number added
                     * This guarantees that we deallocate in the allocation
                     * sequence.
                     */
                    uint32 startoffset = ((lastRetrievedSeqNum - firstSeqNumAdded) + 1) % arraySize;

                    PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFDynamicCircularArray::PurgeElementsWithSeqNumsLessThan: MimeType=%s, SeqNum=%d, StartOffset=%d, ArraySize=%d",
                                                    iMimeType.get_cstr(),
                                                    aSeqNum,
                                                    startoffset,
                                                    arraySize));

                    for (uint32 i = 0; i < aSeqNum - (lastRetrievedSeqNum + 1); i++)
                    {
                        uint32 offset = (startoffset + i) % arraySize;
                        /* Mark the retrieved element location as free */
                        PVMFSharedMediaDataPtr elem = iMediaPtrVec[offset];
                        if (elem.GetRep() != NULL)
                        {
                            if (elem->getSeqNum() < aSeqNum)
                            {
                                elem.Unbind();
                                iMediaPtrVec[offset] = elem;
                                numElems--;
                            }
                        }
                    }
                }
            }
            /* To prevent us from registering any old packets */
            lastRetrievedSeqNum = aSeqNum - 1;
            sJitterBufferParams.lastRetrievedSeqNum = lastRetrievedSeqNum;
            sJitterBufferParams.currentOccupancy = numElems;

            SetReadOffset(aSeqNum);
        }

        void PurgeElementsWithTimestampLessThan(PVMFTimestamp aTS)
        {
            if (!iMediaPtrVec.empty())
            {
                while (numElems > 0)
                {
                    /* Wrap around */
                    if (readOffset >= arraySize)
                    {
                        readOffset = 0;
                    }
                    PVMFSharedMediaDataPtr dataPkt = iMediaPtrVec[readOffset];
                    if (dataPkt.GetRep() != NULL)
                    {
                        PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "JB Purge: MimeType=%s, ReadOffset=%d, NumElemsLeft=%d, lastRetrievedSeqNum=%d, seqNum=%d",
                                                       iMimeType.get_cstr(),
                                                       readOffset,
                                                       numElems,
                                                       lastRetrievedSeqNum,
                                                       dataPkt->getSeqNum()));
                        PVMFTimestamp tmpTS = dataPkt.GetRep()->getTimestamp();
                        if (tmpTS >= aTS)
                            break;

                        (iMediaPtrVec[readOffset]).Unbind();

                        numElems--;
                    }
                    readOffset++;
                }
            }
            /* To prevent us from registering any old packets */
            lastRetrievedTS = aTS;
            sJitterBufferParams.currentOccupancy = numElems;
        }

        void SetReadOffset(uint32 aSeqNum)
        {
            readOffset = (aSeqNum - firstSeqNumAdded) % arraySize;
        }

        void setPacketHeaderFormat(PVMFJitterBufferTransportHeaderFormat aHeaderFormat)
        {
            iHeaderFormat = aHeaderFormat;
        }

        void SetBroadCastSession()
        {
            iBroadCastSession = true;
        }

    private:
        Oscl_Vector<PVMFSharedMediaDataPtr, Alloc> iMediaPtrVec;

        uint32 numElems;
        uint32 arraySize;
        uint32 readOffset;
        bool iBroadCastSession;

        void InitVector(uint32 size)
        {
            for (uint32 i = 0; i < size; i++)
            {
                PVMFSharedMediaDataPtr sharedMediaPtr;
                iMediaPtrVec.push_back(sharedMediaPtr);
            }
        }

        uint32 lastRetrievedSeqNum;
        PVMFTimestamp lastRetrievedTS;
        uint32 maxSeqNumAdded;
        uint32 firstSeqNumAdded;

        /* buffer statistics related params */
        PVMFJitterBufferStats sJitterBufferParams;
        PVLogger* iLogger;
        PVLogger *iDataPathLoggerIn;
        PVLogger *iDataPathLoggerOut;
        OSCL_HeapString<PVMFJitterBufferNodeAllocator> iMimeType;

        PVMFJitterBufferTransportHeaderFormat iHeaderFormat;
};

typedef enum
{
    PVMF_JITTER_BUFFER_ADD_PKT_ERROR,
    PVMF_JITTER_BUFFER_ADD_PKT_SUCCESS,
    PVMF_JITTER_BUFFER_ADD_PKT_UNEXPECTED_DATA,
    PVMF_JITTER_BUFFER_ADD_PKT_EOS_REACHED
} PVMFJitterBufferAddPktStatus;

class PVMFJitterBufferObserver
{
    public:
        virtual ~PVMFJitterBufferObserver() {}

        virtual void JitterBufferFreeSpaceAvailable(OsclAny* aContext) = 0;
        virtual void EstimatedServerClockUpdated(OsclAny* aContext) = 0;
};

class PVMFJitterBuffer
{
    public:
        virtual ~PVMFJitterBuffer() {}

        virtual void SetEstimatedServerClock(OsclClock* aEstServClock) = 0;
        virtual bool ParsePacketHeader(PVMFSharedMediaDataPtr& inDataPacket,
                                       PVMFSharedMediaDataPtr& outDataPacket,
                                       uint32 aFragIndex = 0) = 0;
        virtual PVMFJitterBufferAddPktStatus addPacket(PVMFSharedMediaDataPtr& dataPacket) = 0;
        virtual PVMFSharedMediaDataPtr retrievePacket() = 0;
        virtual PVMFJitterBufferStats getJitterBufferStats() = 0;
        virtual void FlushJitterBuffer() = 0;
        virtual void ResetJitterBuffer() = 0;
        virtual void setSSRC(uint32 aSSRC) = 0;
        virtual void setRTPInfoParams(PVMFRTPInfoParams rtpInfoParams) = 0;
        virtual bool CheckSpaceAvailability() = 0;
        virtual bool CheckForMemoryAvailability() = 0;
        virtual bool CheckCurrentReadPosition() = 0;
        virtual PVMFTimestamp peekNextElementTimeStamp() = 0;
        virtual bool IsEmpty() = 0;
        virtual void SetEOS(bool aVal) = 0;
        virtual bool GetEOS() = 0;
        virtual uint32 getInterArrivalJitter() = 0;
        virtual void setPlayRange(int32 aStartTimeInMS, int32 aStopTimeInMS) = 0;
        virtual bool CheckForHighWaterMark() = 0;
        virtual bool CheckForLowWaterMark() = 0;
        virtual bool CheckNumElements() = 0;
        virtual void PurgeElementsWithSeqNumsLessThan(uint32 aSeqNum, uint32 aPlayerClockMS) = 0;
        virtual void PurgeElementsWithTimestampLessThan(PVMFTimestamp aTS) = 0;
        virtual bool NotifyFreeSpaceAvailable(PVMFJitterBufferObserver* aObserver,
                                              OsclAny* aContext) = 0;
        virtual bool NotifyServerClockUpdates(PVMFJitterBufferObserver* aObserver,
                                              OsclAny* aContext) = 0;
        virtual bool CancelServerClockNotificationUpdates() = 0;
        virtual void SetInPlaceProcessingMode(bool aInPlaceProcessingMode) = 0;
        virtual bool addMediaCommand(PVMFSharedMediaMsgPtr& aMediaCmd) = 0;
        virtual bool CheckForPendingCommands(PVMFSharedMediaMsgPtr& aCmdMsg) = 0;
        virtual void SetAdjustedTSInMS(PVMFTimestamp aAdjustedTS) = 0;
        virtual uint32 GetRTPTimeStampOffset(void) = 0;
        virtual void   SetRTPTimeStampOffset(uint32 newTSBase) = 0;
        virtual PVMFSharedMediaDataPtr& GetFirstDataPacket(void) = 0;
        virtual void AdjustRTPTimeStamp() = 0;
        virtual void SetBroadCastSession() = 0;
};

class PVMFJitterBufferImpl : public PVMFJitterBuffer,
            public OsclMemPoolFixedChunkAllocatorObserver
{
    public:
        PVMFJitterBufferImpl(const PvmfMimeString* aMimeType = NULL,
                             bool aInPlaceProcessing = true);
        virtual ~PVMFJitterBufferImpl();

        void SetEstimatedServerClock(OsclClock* aEstServClock)
        {
            iEstimatedServerClock = aEstServClock;
        }

        PVMFJitterBufferAddPktStatus addPacket(PVMFSharedMediaDataPtr& dataPacket);

        PVMFSharedMediaDataPtr retrievePacket();

        PVMFJitterBufferStats getJitterBufferStats()
        {
            return (iJitterBuffer->getStats());
        }

        void FlushJitterBuffer()
        {
            iFirstDataPackets.clear();
            iJitterBuffer->Clear();
        };

        void ResetJitterBuffer();

        void setSSRC(uint32 aSSRC)
        {
            PVMF_JBNODE_LOGDATATRAFFIC_IN((0, "PVMFJitterBufferImpl::setSSRC: Setting SSRCFromSetUpResponse - MimeType=%s, SSRC=%d", iMimeType.get_cstr(), aSSRC));
            oSSRCFromSetUpResponseAvailable = true;
            SSRCFromSetUpResponse = aSSRC;
            SSRCLock = aSSRC;
        }

        void setRTPInfoParams(PVMFRTPInfoParams rtpInfo)
        {
            iJitterBuffer->setRTPInfoParams(rtpInfo);
            PVMFRTPInfoParams iRTPInfoParams;
            iRTPInfoParams.seqNumBaseSet = rtpInfo.seqNumBaseSet;
            if (rtpInfo.seqNumBaseSet == true)
            {
                iRTPInfoParams.seqNum = rtpInfo.seqNum;
            }
            iRTPInfoParams.rtpTimeBaseSet = rtpInfo.rtpTimeBaseSet;
            if (rtpInfo.rtpTimeBaseSet == true)
            {
                iRTPInfoParams.rtpTime = rtpInfo.rtpTime;
            }
            iRTPInfoParams.nptTimeInMS = rtpInfo.nptTimeInMS;
            iRTPInfoParams.rtpTimeScale = rtpInfo.rtpTimeScale;
            iRTPTimeScale = rtpInfo.rtpTimeScale;
            iEstServClockMediaClockConvertor.set_timescale(iRTPTimeScale);
            iMediaClockConvertor.set_timescale(1000);
            iMediaClockConvertor.set_clock_other_timescale(0, iRTPInfoParams.rtpTimeScale);
            iMediaClockConvertor.update_clock(iRTPInfoParams.nptTimeInMS);
            iRTPInfoParams.nptTimeInRTPTimeScale =
                iMediaClockConvertor.get_converted_ts(iRTPInfoParams.rtpTimeScale);
            /* In case this is the first rtp info set TS calc variables */
            if (iRTPInfoParamsVec.size() == 0)
            {
                if (iRTPInfoParams.rtpTimeBaseSet)
                {
                    iPrevTSOut = iRTPInfoParams.rtpTime;
                    iPrevTSIn = iRTPInfoParams.rtpTime;
                    iPrevAdjustedRTPTS = iRTPInfoParams.rtpTime;
                }
                else
                {
                    /* Use the value from the first packet */
                    if (seqNumLock)
                    {
                        iPrevTSOut = seqLockTimeStamp;
                        iPrevTSIn = seqLockTimeStamp;
                        iPrevAdjustedRTPTS = seqLockTimeStamp;
                    }
                }
                if (iRTPInfoParams.seqNumBaseSet)
                {
                    iPrevSeqNumBaseOut = iRTPInfoParams.seqNum;
                    iPrevSeqNumBaseIn = iRTPInfoParams.seqNum;
                }
                else
                {
                    /* Use the value from the first packet */
                    if (seqNumLock)
                    {
                        iPrevSeqNumBaseOut = iFirstSeqNum;
                        iPrevSeqNumBaseIn = iFirstSeqNum;
                    }
                }
            }
            if (iRTPInfoParams.rtpTimeBaseSet)
            {
                iPrevAdjustedRTPTS = iRTPInfoParams.rtpTime;
            }
            iRTPInfoParamsVec.push_back(iRTPInfoParams);
        }

        bool IsEmpty()
        {
            uint32 elems = iJitterBuffer->getNumElements();
            if (elems == 0)
            {
                return true;
            }
            return false;
        };

        bool CheckSpaceAvailability()
        {
            if (iJitterBuffer)
            {
                return (iJitterBuffer->CheckSpaceAvailability());
            }
            return false;
        }

        bool CheckForMemoryAvailability()
        {

            if (iInPlaceProcessing == false)
            {
                if (iMediaDataGroupAlloc)
                {
                    return (iMediaDataGroupAlloc->IsMsgAvailable());
                }
                return false;
            }
            else
            {
                return true;
            }
        };

        bool CheckCurrentReadPosition()
        {
            return iJitterBuffer->CheckCurrentReadPosition();
        };

        bool CheckNumElements()
        {
            if (iJitterBuffer->getNumElements() > 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        };

        PVMFTimestamp peekNextElementTimeStamp();

        void SetEOS(bool aVal)
        {
            oEOS = aVal;
        }

        bool GetEOS()
        {
            return (oEOS);
        }

        uint32 getInterArrivalJitter()
        {
            return iInterArrivalJitter;
        }

        bool ParsePacketHeader(PVMFSharedMediaDataPtr& inDataPacket,
                               PVMFSharedMediaDataPtr& outDataPacket,
                               uint32 aFragIndex = 0);

        bool ParseRTPHeader(PVMFSharedMediaDataPtr& rtpPacket,
                            PVMFSharedMediaDataPtr& dataPacket,
                            uint32 aFragIndex = 0);


        void setPlayRange(int32 aStartTimeInMS, int32 aStopTimeInMS)
        {
            iStartTimeInMS = aStartTimeInMS;
            iStopTimeInMS = aStopTimeInMS;
        }

        bool CheckForHighWaterMark()
        {
            PVMFJitterBufferStats stats = iJitterBuffer->getStats();
            uint32 currOccupancy = stats.currentOccupancy;
            uint32 maxOccupancy = stats.maxOccupancy;

            PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::CheckForHighWaterMark: CurrOccupancy = %d", currOccupancy));
            if (currOccupancy >=
                    maxOccupancy*DEFAULT_JITTER_BUFFER_HIGH_WATER_MARK)
            {
                return true;
            }
            return false;
        }

        bool CheckForLowWaterMark()
        {
            PVMFJitterBufferStats stats = iJitterBuffer->getStats();
            uint32 currOccupancy = stats.currentOccupancy;
            uint32 maxOccupancy = stats.maxOccupancy;

            PVMF_JBNODE_LOGINFO((0, "PVMFJitterBufferImpl::CheckForLowWaterMark: CurrOccupancy = %d", currOccupancy));
            if (currOccupancy <=
                    maxOccupancy*DEFAULT_JITTER_BUFFER_LOW_WATER_MARK)
            {
                return true;
            }
            return false;
        }

        void PurgeElementsWithSeqNumsLessThan(uint32 aSeqNum, uint32 aPlayerClockMS);
        void PurgeElementsWithTimestampLessThan(PVMFTimestamp aTS);

        bool NotifyServerClockUpdates(PVMFJitterBufferObserver* aObserver,
                                      OsclAny* aContext)
        {
            iServerClockUpdateNotificationObserver = aObserver;
            iServerClockUpdateNotificationObserverContext = aContext;
            return true;
        }

        bool CancelServerClockNotificationUpdates()
        {
            iServerClockUpdateNotificationObserver = NULL;
            iServerClockUpdateNotificationObserverContext = NULL;
            return true;
        }

        bool NotifyFreeSpaceAvailable(PVMFJitterBufferObserver* aObserver,
                                      OsclAny* aContext)
        {
            iObserver = aObserver;
            iObserverContext = aContext;
            if (iMediaDataGroupAlloc)
            {
                iMediaDataGroupAlloc->notifyfreechunkavailable(*this);
                return true;
            }
            return false;
        }

        void freechunkavailable(OsclAny*)
        {
            if (iObserver)
            {
                iObserver->JitterBufferFreeSpaceAvailable(iObserverContext);
            }
        }

        void SetInPlaceProcessingMode(bool aInPlaceProcessingMode);

        bool addMediaCommand(PVMFSharedMediaMsgPtr& aMediaCmd)
        {
            PVMFJitterBufferStats stats = getJitterBufferStats();
            MediaCommandMsgHolder cmdHolder;
            cmdHolder.iPreceedingMediaMsgSeqNumber = stats.lastRegisteredSeqNum;
            cmdHolder.iCmdMsg = aMediaCmd;
            iMediaCmdVec.push_back(cmdHolder);
            return true;
        }

        bool CheckForPendingCommands(PVMFSharedMediaMsgPtr& aCmdMsg)
        {
            /*
             * Parse the command queue to see if it is time to send out
             * any pending ones. if last retrieved seq num is same as
             * the seq num of media msg that preceeds this cmd, then
             * it is time to send this cmd out
             */
            PVMFJitterBufferStats stats = getJitterBufferStats();
            Oscl_Vector<MediaCommandMsgHolder, OsclMemAllocator>::iterator it;
            for (it = iMediaCmdVec.begin(); it != iMediaCmdVec.end(); it++)
            {
                if (it->iCmdMsg->getFormatID() == PVMF_MEDIA_CMD_BOS_FORMAT_ID)
                {
                    aCmdMsg = it->iCmdMsg;
                    iMediaCmdVec.erase(it);
                    return true;
                }
                if (it->iPreceedingMediaMsgSeqNumber == stats.lastRetrievedSeqNum)
                {
                    aCmdMsg = it->iCmdMsg;
                    iMediaCmdVec.erase(it);
                    return true;
                }
            }
            return false;
        }

        void SetAdjustedTSInMS(PVMFTimestamp aAdjustedTSInMS)
        {
            // convert adjustedTS to RTP Timescale
            uint32 in_wrap_count = 0;
            MediaClockConverter clockConvertor;
            clockConvertor.set_timescale(1000);
            clockConvertor.set_clock(aAdjustedTSInMS, in_wrap_count);
            uint32 adjustedTSInRTPTS = clockConvertor.get_converted_ts(iRTPTimeScale);
            iMonotonicTimeStamp = adjustedTSInRTPTS;
            PVMF_JBNODE_LOGDATATRAFFIC_OUT((0, "PVMFJitterBufferImpl::SetAdjustedTS(): adjustedTSInRTPTS=%d, iMonotonicTS=%d", adjustedTSInRTPTS, Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp)));

        }
        uint32 GetRTPTimeStampOffset(void)
        {
            return seqLockTimeStamp;
        }
        void   SetRTPTimeStampOffset(uint32 newTSBase)
        {
            seqLockTimeStamp = newTSBase;
            iMonotonicTimeStamp = 0;
        }
        PVMFSharedMediaDataPtr& GetFirstDataPacket(void)
        {
            return firstDataPacket;
        }


        void AdjustRTPTimeStamp()
        {
            // By the time this function is called, iMonotonicTimeStamp should be already normalized
            // with the corresponding values from other jitterbuffers
            iMaxAdjustedRTPTS = Oscl_Int64_Utils::get_uint64_lower32(iMonotonicTimeStamp);
            UpdateEstimatedServerClock(true);
        }

        void SetBroadCastSession()
        {
            iBroadCastSession = true;
            if (iJitterBuffer != NULL)
            {
                iJitterBuffer->SetBroadCastSession();
            }
        }

    private:
        void UpdateInterArrivalJitter(PVMFTimestamp currPacketTS);
        PVMFRTPInfoParams *FindRTPInfoParams(uint32 aSeqNum)
        {
            if (iRTPInfoParamsVec.size() == 1)
            {
                return (iRTPInfoParamsVec.begin());
            }

            PVMFRTPInfoParams* retVal = NULL;
            Oscl_Vector<PVMFRTPInfoParams, PVMFJitterBufferNodeAllocator>::iterator it;

            for (it = iRTPInfoParamsVec.begin();
                    it != iRTPInfoParamsVec.end();
                    it++)
            {
                if (it->seqNum <= aSeqNum)
                {
                    retVal = it;
                }
            }
            return retVal;
        }

        void CheckForRTPTimeAndRTPSeqNumberBase()
        {
            if (iRTPInfoParamsVec.size() > 0)
            {
                Oscl_Vector<PVMFRTPInfoParams, PVMFJitterBufferNodeAllocator>::iterator it;
                it = iRTPInfoParamsVec.begin();
                if (it->rtpTimeBaseSet == false)
                {
                    /* Use the value from the first packet */
                    if (seqNumLock)
                    {
                        iPrevTSOut = seqLockTimeStamp;
                        iPrevTSIn = seqLockTimeStamp;
                        iPrevAdjustedRTPTS = seqLockTimeStamp;
                    }
                }
                if (it->seqNumBaseSet == false)
                {
                    /* Use the value from the first packet */
                    if (seqNumLock)
                    {
                        iPrevSeqNumBaseOut = iFirstSeqNum;
                        iPrevSeqNumBaseIn = iFirstSeqNum;
                    }
                }
            }
        }

        void DeterminePrevTimeStamp(uint32);
        void DeterminePrevTimeStampPeek(uint32, PVMFTimestamp&);
        void ComputeMaxAdjustedRTPTS();
        void UpdateEstimatedServerClock(bool oFreshStart = false);
        void UpdateEstimatedServerClockDiscrete(bool oFreshStart = false);

        bool   oFirstPacket;
        bool   seqNumLock;
        bool   oEOS;
        uint32 SSRCLock;
        bool   oSSRCFromSetUpResponseAvailable;
        uint32 SSRCFromSetUpResponse;
        uint32 iFirstSeqNum;

        PVMFSharedMediaDataPtr firstDataPacket;
        Oscl_Vector<PVMFSharedMediaDataPtr, PVMFJitterBufferNodeAllocator> iFirstDataPackets;

        typedef PVMFDynamicCircularArray<PVMFJitterBufferNodeAllocator> PVMFDynamicCircularArrayType;

        PVMFDynamicCircularArrayType* iJitterBuffer;

        typedef OsclMemPoolFixedChunkAllocator PoolMemAlloc;
        PVMFMediaFragGroupCombinedAlloc<PoolMemAlloc>* iMediaDataGroupAlloc;
        PoolMemAlloc* iMediaDataImplMemPool;
        PoolMemAlloc* iMediaMsgMemPool;

        Oscl_Vector<PVMFRTPInfoParams, PVMFJitterBufferNodeAllocator> iRTPInfoParamsVec;

        MediaClockConverter iMediaClockConvertor;
        PVMFTimestamp seqLockTimeStamp;
        PVLogger *iLogger;
        PVLogger *iDataPathLoggerIn;
        PVLogger *iDataPathLoggerOut;
        PVLogger *iClockLogger;

        OSCL_HeapString<PVMFJitterBufferNodeAllocator> iMimeType;

        uint32 iPrevPacketRecvTime;
        PVMFTimestamp iPrevPacketTS;
        double iInterArrivalJitterD;
        int32  iInterArrivalJitter;

        OsclClock *iPacketArrivalClock;
        OsclTimebase_Tickcount iPacketArrivalTimeBase;

        int32 iStartTimeInMS;
        int32 iStopTimeInMS;

        uint64 iMonotonicTimeStamp;
        PVMFTimestamp  iPrevTSOut;
        uint32 iPrevSeqNumBaseOut;

        PVMFTimestamp iMaxAdjustedRTPTS;
        PVMFTimestamp iPrevAdjustedRTPTS;
        PVMFTimestamp iPrevTSIn;
        uint32 iPrevSeqNumBaseIn;

        OsclClock* iEstimatedServerClock;
        OsclTimebase_Tickcount iTickCount;
        uint32 iRTPTimeScale;
        MediaClockConverter iEstServClockMediaClockConvertor;
        PVMFJitterBufferObserver* iObserver;
        OsclAny* iObserverContext;

        bool iInPlaceProcessing;

        PVMFJitterBufferTransportHeaderFormat iHeaderFormat;

        /* ASF Related */
        uint32 iSeqNum;
        bool iFirstASFPacketAfterRepos;

        /* Media Command related */
        Oscl_Vector<MediaCommandMsgHolder, OsclMemAllocator> iMediaCmdVec;

        PVMFJitterBufferObserver* iServerClockUpdateNotificationObserver;
        OsclAny* iServerClockUpdateNotificationObserverContext;

        bool iBroadCastSession;
};

#endif


