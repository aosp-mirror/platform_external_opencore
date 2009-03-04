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
// -*- c++ -*-
#ifndef RTP_PACKET_H
#define RTP_PACKET_H


#ifdef PV_OS_ZREX
#include "zrex_os.h"
#endif

#include "oscl_media_data.h"
#include "oscl_media_status.h"

#include "access_unit.h"
#include "rtp.h"


// Constants controlling the size of the RTP packet container
const int RTP_PACKET_MAX_FRAGMENTS = 30;
const int RTP_PACKET_IMMEDIATE_DATA = 1500;
const int DEF_RTP_MAX_PACKET_SIZE = 1024;

class RTPPacketImplementation;
typedef MemAllocator<RTPPacketImplementation> RTPImplAllocator;



class RTPPacket
{

    public:


        OSCL_IMPORT_REF RTPPacket(RTPImplAllocator* alloc, void * hint = 0);
        OSCL_IMPORT_REF ~RTPPacket();

        OSCL_IMPORT_REF MediaStatusClass::status_t AddRTPFrag(const BufferFragment& frag, BufferState* buffer_state,
                int32 location_offset = APPEND_MEDIA_AT_END);
        void Clear();

        OSCL_IMPORT_REF BufferFragment * GetRTPHeader();

        OSCL_IMPORT_REF BufferFragment * GetFragments();

        OSCL_IMPORT_REF MediaStatusClass::status_t GetLocalFragment(BufferFragment& fragment);

        OSCL_IMPORT_REF MediaStatusClass::status_t AddLocalRTPFrag(BufferFragment& fragment, int32 location_offset = APPEND_MEDIA_AT_END);

        OSCL_IMPORT_REF MediaStatusClass::status_t AddAU(const AccessUnit* au);

        OSCL_IMPORT_REF int GetAvailableBufferSize();

        MediaTimestamp GetTimestamp();  // this method gets timestamp from header
        RtpSeqType GetSN();              // gets Seq Num from header
        MediaTimestamp GetTimestampMS();  // this method gets timestamp from member variable based on serverRealTimeClock

        OSCL_IMPORT_REF void SetTimestamp(MediaTimestamp& rtp_timestamp);
        OSCL_IMPORT_REF void SetTimestampMS(MediaTimestamp& rtp_timestamp);

        OSCL_IMPORT_REF int32 GetNumFrags() const;
        OSCL_IMPORT_REF int32 GetMaxFrags() const;
        OSCL_IMPORT_REF uint32 GetLength() const;


        OSCL_IMPORT_REF int GetMediaSize() const;

        OSCL_IMPORT_REF void AppendNext(RTPPacket *next_ptr);

        OSCL_IMPORT_REF RTPPacket* GetNext() const;

        void SetFreeBufferStatesWhenDone();

        void SetMBit(const bool flag);
    private:
        RTPImplAllocator * alloc;
        RTPPacketImplementation *rep;



};

typedef MemAllocator<RTPPacket> RTPPacketAlloc;




#endif
