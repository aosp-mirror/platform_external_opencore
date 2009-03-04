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
#ifndef RTP_PACKET_IMPL_H
#define RTP_PACKET_IMPL_H

#include <stdlib.h>

#ifdef PV_OS_ZREX
#include "zrex_os.h"
#endif

#include "oscl_media_data.h"
#include "oscl_media_status.h"

#include "rtp_packet.h"
//#include "pvserver_tunable.h"
#include "access_unit.h"

#include "rtp.h"

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

class RTPPacketImplementation : public MediaData<RTPPacket, RTP_PACKET_MAX_FRAGMENTS, RTP_PACKET_IMMEDIATE_DATA>
{

    public:
        OSCL_IMPORT_REF // \todo: Check whether this is needed
        RTPPacketImplementation();
        virtual ~RTPPacketImplementation();

        MediaStatusClass::status_t AddRTPFrag(const BufferFragment& frag, BufferState* buffer_state,
                                              int32 location_offset);

        MediaStatusClass::status_t AddLocalRTPFrag(const BufferFragment& frag, int32 location_offset);

        BufferFragment * GetFragments()
        {
            return fragments;
        };

        BufferFragment * GetRTPHeader()
        {
            return fragments;
        };

        MediaStatusClass::status_t AddAU(const AccessUnit* au);

        int GetMediaSize()
        {
            return (GetLength() - GetRTPHeader()->len);
        }

        MediaTimestamp GetHdrTimestamp();

        RtpSeqType GetSN()
        {
            BufferFragment * hdr = GetRTPHeader();

            if (NULL == hdr || NULL == hdr->ptr)
                return 0xff;

            return ((* ((uint32 *) hdr->ptr)) & RTP_SEQ_NUM_MASK);
        }

        void SetFreeBufferStatesWhenDone()
        {
            free_buffer_states_when_done = true;
        }

    private:

        bool free_buffer_states_when_done;

};


class DefaultRTPPacketAlloc : public RTPPacketAlloc
{
    public:
        DefaultRTPPacketAlloc(RTPImplAllocator * impl_alloc)
        {
            rtp_impl_alloc = impl_alloc;
            num_freed = num_allocated = 0;
        };

        int get_num_allocated()
        {
            return num_allocated;
        }
        int get_num_freed()
        {
            return num_freed;
        }

        //
        // The following will allocate a buffer to hold both RTPPacket object and the
        // correspondign RTPPacketImplementation object.
        //

        // WINCE doesn't seem to support placement new in 3.0 so we are using plain old new and delete for now.
        virtual RTPPacket* allocate(void * hint = 0, const int num_reserved_frags = 1)
        {
            num_allocated++;
            return OSCL_NEW(RTPPacket, (rtp_impl_alloc, 0));
        }

        virtual void deallocate(RTPPacket* packet)
        {
            OSCL_DELETE(packet);
            num_freed ++;
        }
    private:
        RTPImplAllocator * rtp_impl_alloc;
        int num_allocated;
        int num_freed;
};

class DefaultRTPImplAllocator : public RTPImplAllocator
{

    public:

        virtual RTPPacketImplementation * allocate(void * hint, const int num_reserved_frags = 1)
        {
            return new RTPPacketImplementation;
        }

        virtual void deallocate(pointer rtp_impl)
        {
            // do nothing: recall it was a placement new;
            OSCL_DELETE(rtp_impl);
        }


};



#endif
