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
#include "rtp_packet_impl.h"


/* **********************************************
 *
 *  NOTE: Throughout this implementation, fragment number 0
 *  is reserved for the RTP Header.
 *
 * **********************************************/

OSCL_EXPORT_REF
RTPPacketImplementation::RTPPacketImplementation()
        : free_buffer_states_when_done(false)
{
    // the default constructor for BufFragGroup takes care of other things
    timestamp = 0;

    num_fragments = 1;


    fragments[0].ptr = localbuf;
    length = fragments[0].len = RTP_HEADER_SIZE;
    available_localbuf -= RTP_HEADER_SIZE;
    buffer_states[0] = NULL;
}


RTPPacketImplementation::~RTPPacketImplementation()
{
    // decrement the ref count on all buffers
    for (uint ii = 0; ii < num_fragments; ++ii)
    {
        if (buffer_states[ii])
        {
            buffer_states[ii]->decrement_refcnt();

            if (free_buffer_states_when_done)
            {
                if (buffer_states[ii]->get_refcount() == 0)
                {
                    OSCL_DELETE(buffer_states[ii]);
                    buffer_states[ii] = NULL;
                }
            }
        }
    }
}


MediaStatusClass::status_t RTPPacketImplementation::AddRTPFrag(const BufferFragment& frag, BufferState* buffer_state,
        int32 location_offset)
{
    location_offset = (location_offset >= 0) ? location_offset + 1 : location_offset;
    return AddFragment(frag, buffer_state, location_offset);
}

MediaStatusClass::status_t RTPPacketImplementation::AddLocalRTPFrag(const BufferFragment& frag, int32 location_offset)
{
    location_offset = (location_offset >= 0) ? location_offset + 1 : location_offset;
    return AddLocalFragment(frag, location_offset);
}

MediaStatusClass::status_t RTPPacketImplementation::AddAU(const AccessUnit* au)
{

    if (!au)
    {
        return MediaStatusClass::NULL_INPUT;
    }

    // determine the number of access unit fragments and the total length
    int32 au_frags = au->GetNumFrags();
    BufferFragment frag;
    BufferState* state_ptr;
    MediaStatusClass::status_t status;

    for (int ii = 0; ii < au_frags; ++ii)
    {
        au->GetMediaFragment(ii, frag, state_ptr);
        if (frag.ptr == NULL || frag.len == 0)
        {
            continue;
        }

        if (!state_ptr)
        {
            // check if it is local
            if (au->IsLocalData(frag))
            {
                // add as a local fragment
                if ((status = AddLocalFragment(frag, APPEND_MEDIA_AT_END)) !=
                        MediaStatusClass::BFG_SUCCESS)
                {
                    return status;
                }
                continue;
            }
        }

        // add as a regular fragment
        if ((status = AddFragment(frag, state_ptr, APPEND_MEDIA_AT_END)) !=
                MediaStatusClass::BFG_SUCCESS)
        {
            return status;
        }

    }

    return MediaStatusClass::BFG_SUCCESS;
}

const MediaTimestamp INVALID_TIMESTAMP = 0xFFFFFFFF;
MediaTimestamp RTPPacketImplementation::GetHdrTimestamp()
{
    BufferFragment * hdr = GetRTPHeader();

    if (NULL == hdr || NULL == hdr->ptr)
        return 0xffff;

    return * ((uint32 *) hdr->ptr + 1);
}

