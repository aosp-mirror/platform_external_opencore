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


#include "rtp_packet.h"
#include "rtp_packet_impl.h"

OSCL_EXPORT_REF
RTPPacket::RTPPacket(RTPImplAllocator* in_alloc, void * hint)
{
    alloc = in_alloc;
    rep = in_alloc->allocate(hint);
}

OSCL_EXPORT_REF
RTPPacket::~RTPPacket()
{
    alloc->deallocate(rep);
}

OSCL_EXPORT_REF
BufferFragment * RTPPacket::GetRTPHeader()
{
    return rep->GetRTPHeader();
};

OSCL_EXPORT_REF
BufferFragment * RTPPacket::GetFragments()
{
    return rep->GetFragments();
};

MediaStatusClass::status_t RTPPacket::AddRTPFrag(const BufferFragment& frag, BufferState* buffer_state,
        int32 location_offset)
{
    return rep->AddRTPFrag(frag, buffer_state, location_offset);
}

OSCL_EXPORT_REF MediaStatusClass::status_t RTPPacket::AddLocalRTPFrag(BufferFragment& fragment, int32 location_offset)
{
    return rep->AddLocalRTPFrag(fragment, location_offset);
}


OSCL_EXPORT_REF
MediaStatusClass::status_t RTPPacket::GetLocalFragment(BufferFragment& fragment)
{
    return rep->GetLocalFragment(fragment);
}

OSCL_EXPORT_REF
int RTPPacket::GetAvailableBufferSize()
{
    return rep->GetAvailableBufferSize();
}

void RTPPacket::Clear()
{
    rep->Clear();
}

MediaTimestamp RTPPacket::GetTimestamp()
{
    return rep->GetHdrTimestamp();
};

RtpSeqType RTPPacket::GetSN()
{
    return rep->GetSN();
}

MediaTimestamp RTPPacket::GetTimestampMS()
{
    return rep->GetTimestamp();
}

OSCL_EXPORT_REF
void RTPPacket::SetTimestamp(MediaTimestamp& rtp_timestamp)
{
    rep->SetTimestamp(rtp_timestamp);
}

OSCL_EXPORT_REF
void RTPPacket::SetTimestampMS(MediaTimestamp& rtp_timestamp)
{
    rep->SetTimestamp(rtp_timestamp);
}


OSCL_EXPORT_REF
MediaStatusClass::status_t RTPPacket::AddAU(const AccessUnit *au)
{
    return rep->AddAU(au);
}

OSCL_EXPORT_REF
uint32 RTPPacket::GetLength() const
{
    return rep->GetLength();
}

OSCL_EXPORT_REF
void RTPPacket::AppendNext(RTPPacket *next_ptr)
{
    rep->AppendNext(next_ptr);
}

OSCL_EXPORT_REF
RTPPacket* RTPPacket::GetNext() const
{
    return rep->GetNext();
}

OSCL_EXPORT_REF
int32 RTPPacket::GetNumFrags() const
{
    return rep->GetNumFrags();
}

OSCL_EXPORT_REF
int32 RTPPacket::GetMaxFrags() const
{
    return rep->GetMaxFrags();
}

OSCL_EXPORT_REF
int RTPPacket::GetMediaSize() const
{
    return rep->GetMediaSize();
}


void RTPPacket::SetFreeBufferStatesWhenDone()
{
    rep->SetFreeBufferStatesWhenDone();
}


const uint8 RTP_MBIT_MASK = 0x01 << RTP_MARKER_FLAG_POSITION;
void RTPPacket::SetMBit(const bool flag)
{
    BufferFragment * hdrFrag = GetRTPHeader();
    uint8 tmpOctet = *(((uint8 *) hdrFrag->ptr) + 1);
    if (flag && hdrFrag->ptr)
    {
        tmpOctet |= RTP_MBIT_MASK;
    }
    else
    {
        tmpOctet &= (~RTP_MBIT_MASK);
    }
    oscl_memcpy(((uint8 *) hdrFrag->ptr) + 1, &tmpOctet, sizeof(uint8));
}
