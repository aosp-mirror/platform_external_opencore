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
//-*- c++ -*-
//#include <stdio.h>

#include "rtp_payload.h"
#include "access_unit_impl.h"
#include "au_utils.h"

const int NUM_BITS_RESYNC_MARKER = 17;

OSCL_EXPORT_REF RTPPacket * RtpPayloadHandler :: pack
(const GAU* gau,
 int & n,  // n is the number of AUs processed
 RtpPayloadStatus & status_code)
{
    OSCL_UNUSED_ARG(n);

    int num_aus = 0;
    RTPPacket * rtp_packet_chain = NULL;
    MediaStatusClass::status_t media_status;
    DefaultAUImplAllocator * au_impl_alloc = new DefaultAUImplAllocator;

    const AccessUnit * input_AU_chain =  Gau2AU(gau, au_impl_alloc, media_status);

    rtp_packet_chain = packAU(input_AU_chain, num_aus, status_code);

    // free the access units
    const AccessUnit* au = input_AU_chain, *au_next = NULL;
    while (au)
    {
        au_next = au->GetNext();
        OSCL_DELETE(au);
        au = au_next;
    }
    OSCL_DELETE(au_impl_alloc);
    return rtp_packet_chain;
}


//
// The following method searches backwards in the memory fragment group for a pattern, starting
// from (starting_index, starting_frag_offset). If pattern is found, last_frag_index points to the
// fragment containing the found pattern, and last_frag_offset is the pattern's offset.
//
// If no pattern is found between starting_index and (starting_index + search_range),
// last_frag_index will be set to the index of the fragment containing
// (starting_index + search_range); last_byte_offset will be set accordingly, too.
//
// It returns true if pattern is found, or boundary is reached.
// Note:
//    The pattern is assumed to start at beginning of some byte, but may have non-integral number of bytes.
//
bool SearchableRtpPayloadHandler::search_backwards_for_bit_pattern
(
    const AccessUnit & currentAU,
    const uint16 starting_index,
    const uint16 starting_frag_offset,
    const int search_range,
    const octet * pattern,
    const uint16 num_bits,
    uint16 & last_frag_index,
    uint16 & last_byte_offset
)
{
    int idx;
    int offset;
    octet * ptr;

    idx = starting_index;
    offset = starting_frag_offset;
    bool boundaryReached;
    if (!currentAU.seek(idx, offset, ptr, boundaryReached, search_range) && (NULL == ptr))
    {
        // UNCOND_LOGMSG(ERR_WARNING, ("currentAU.seek returned error. Some states are corrupted\n"));
        return true;
    }
    if (boundaryReached)
    {
        last_frag_index = currentAU.GetNumFrags() - 1;
        last_byte_offset = (uint16)(currentAU.GetMediaFragment(last_frag_index))->len;
        return true;
    }

    last_frag_index = idx;
    last_byte_offset = offset;

    octet *tmp_ptr;
    //
    // The following is a sequential search scheme with a persistent state across subsequent
    // invocations of the routine
    //
#define WITH_STATE 1
#ifdef WITH_STATE
    int match_state = num_bits / 8; // force all bits in pattern to be checked;
    if (num_bits % 8)
        match_state++;
    while (0 != (match_state = currentAU.match_bit_pattern_with_state(idx, offset, pattern, (uint8) num_bits, match_state)))
    {
        // printf("\n\n Got match_state %d, idx %d, offset %d\n", match_state, idx, offset);
        currentAU.seek(idx, offset, tmp_ptr, boundaryReached, -1*match_state);
        if (boundaryReached)
        {
            //  last_frag_index = idx;
            //      last_byte_offset = offset;
            return true;
        }

//      if (idx <= 0 && offset <= 0) {
        if (idx <= starting_index && offset <= starting_frag_offset)
        {
            // printf("back to begining idx <= 0 offset <= 0\n");
            return true;
        }

    };
#else
    while (! currentAU.match_bit_pattern_no_state(idx, offset, pattern, num_bits))
    {
        // printf("Search backwards for pattern: trying to match pattern at offset = %d\n", offset);
        if (!currentAU.seek(idx, offset, tmp_ptr, boundaryReached, -1))
        {
            // printf("Error encountered during seek \n");
            return true;
        };

        if (!idx && !offset)
        {
//        printf("back to begining idx = offset = 0\n");
            // last_frag_index = last_byte_offset = 0;
            return true;
        }

        if (boundaryReached)
        {
            // last_frag_index = idx;
            // last_byte_offset = offset;
            return true;
        }
    };
#endif

    // printf("matched at idx = %d, offset = %d\n", idx, offset);
    last_frag_index = idx;
    last_byte_offset = offset;

    return true;
}

OSCL_EXPORT_REF
VariableAuSizePacketizer::
VariableAuSizePacketizer(const RtpSsrc ssrc,
                         const RtpPayloadType pt,
                         RTPPacketAlloc * rtpAlloc,
                         AccessUnitAlloc * accessUnitAlloc,
                         const RtpSeqType init_seq_offset,
                         const MediaTimestamp init_ts_offset,
                         const int max_pckt_size,
                         const int input_sample_rate,
                         const int output_sample_rate,
                         const bool mustSearchStartCode)
        : SearchableRtpPayloadHandler(ssrc, pt, rtpAlloc, accessUnitAlloc, init_seq_offset, init_ts_offset, max_pckt_size),
        needToSearchForStartCode(mustSearchStartCode),
        packet_list_tail(NULL),
        list_of_packets_to_send(NULL)
{
    resync_marker[0] = 0x00;
    resync_marker[1] = 0x00;
    resync_marker[2] = 0x80;
    resync_marker[3] = 0x00;
    uint32 inputSamplingRate;
    if (input_sample_rate > 0)
    {
        inputSamplingRate = input_sample_rate;
    }
    else
    {
        //    int generate_a_log_here;
        inputSamplingRate = DEF_INPUT_SAMPLING_RATE;
    }

    if (output_sample_rate > 0)
    {
        requiredOutputSamplingRate = output_sample_rate;
    }
    else
    {
        requiredOutputSamplingRate = inputSamplingRate;
    }

    init_timescale_conversion(inputSamplingRate, requiredOutputSamplingRate);

    config_au_flag = false;
};

//uint8 VariableAuSizePacketizer::resync_marker[4] = {0x00, 0x00, 0x80, 0x00};


MediaStatusClass::status_t VariableAuSizePacketizer::AddAU(const AccessUnit * au)
{
    if (!au)
    {
        //printf("Error: AddAU: au is NULL\n");
        return MediaStatusClass::NULL_INPUT;
    }

    if (NULL == current_packet_ptr)
    {
        return MediaStatusClass::INTERNAL_ERROR;  //
    }

    // determine the number of access unit fragments and the total length
    int32 au_frags = au->GetNumFrags();
    BufferFragment tmp_fragment, frag, firstFrag;
    BufferFragment* headerFrag;
    BufferState* state_ptr;
    MediaStatusClass::status_t status;
    uint8 resync_marker[4] = {0x00, 0x00, 0x80, 0x00};

    au->GetMediaFragment(0, firstFrag, state_ptr);
    tmp_fragment.ptr = firstFrag.ptr;
    tmp_fragment.len = firstFrag.len;
    if (current_packet_ptr->GetMediaSize() == Rfc2429_PAYLOAD_HDR_SIZE)
    {
        headerFrag = &current_packet_ptr->GetFragments()[1]; // the first fragment is RTP header, the 2nd is Payload hdr
        // set payload header
        if ((oscl_memcmp(firstFrag.ptr, resync_marker, 2)) == 0 && ((uint8*)firstFrag.ptr)[2] & 0x80)
        {
            // its a start code. set the P bit
            ((uint8*)headerFrag->ptr)[0] = 0x04;

            // This format doesn't work on HP, because, strictly
            // speaking, the left side is not a modifiable lvalue.
            // ((uint8*)tmp_fragment.ptr)+=2;
            // so, we do this:
            tmp_fragment.ptr = ((uint8*)tmp_fragment.ptr) + 2;
            tmp_fragment.len -= 2;
        }
    }
    // check if it is local
    if (au->IsLocalData(firstFrag))
    {
        // add as a local fragment
        if ((status = current_packet_ptr->AddLocalRTPFrag(tmp_fragment, APPEND_MEDIA_AT_END)) != MediaStatusClass::BFG_SUCCESS)
        {
            return status;
        }
    }
    else
    {
        // add as a regular fragment
        if ((status = current_packet_ptr->AddRTPFrag(tmp_fragment, state_ptr, APPEND_MEDIA_AT_END)) != MediaStatusClass::BFG_SUCCESS)
        {
            return status;
        }
    }

    for (int ii = 1; ii < au_frags; ++ii)
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
                if ((status = current_packet_ptr->AddLocalRTPFrag(frag, APPEND_MEDIA_AT_END)) !=
                        MediaStatusClass::BFG_SUCCESS)
                {
                    return status;
                }
                continue;
            }
        }

        // add as a regular fragment
        if ((status = current_packet_ptr->AddRTPFrag(frag, state_ptr, APPEND_MEDIA_AT_END)) !=
                MediaStatusClass::BFG_SUCCESS)
        {
            return status;
        }
    }

    return MediaStatusClass::BFG_SUCCESS;
}
;  // AddAU


RTPPacket * VariableAuSizePacketizer::getNewRtpPacket(const bool isRfc2429, RtpPayloadStatus & status_code)
{
    RTPPacket * new_packet_ptr = rtp_alloc->allocate();

    if (NULL == new_packet_ptr)
    {
        // UNCOND_LOGMSG(ERR_ERROR, ("Packet allocation error. \n"));
        return NULL;
    }

    if (isRfc2429)
    {
        // allocate payload header fragment
        payload_header_fragment.len = Rfc2429_PAYLOAD_HDR_SIZE;
        if (new_packet_ptr->GetLocalFragment(payload_header_fragment) != MediaStatusClass::BFG_SUCCESS)
        {
            //printf("error: failed to get local frag for ph \n");
            status_code = MEM_ALLOC_FAILURE;
            return NULL;
        }
        // set the header bytes to 0's
        ((uint8*)payload_header_fragment.ptr)[0] = ((uint8*)payload_header_fragment.ptr)[1] = 0;

        MediaStatusClass::status_t status;

        if ((status = new_packet_ptr->AddLocalRTPFrag(payload_header_fragment, APPEND_MEDIA_AT_END)) !=
                MediaStatusClass::BFG_SUCCESS)
        {
            status_code = ERROR_ADDING_TO_PACKET;
            return NULL;
        }
        else
        {
            return new_packet_ptr;
        }
    }
    else    // not Rfc2429, hence no payload header
    {
        return new_packet_ptr;
    }
}

//
// packAU returns the chain of RTP packets ready to be sent
//
OSCL_EXPORT_REF
RTPPacket* VariableAuSizePacketizer::packAU(const AccessUnit * input_AU_chain,
        const bool isRfc2429,
        int & num_AU_processed,
        RtpPayloadStatus & status_code)
{
#ifdef DEBUG_VOP_START_CODE
    if (GetCurrentSeqNum() >= 15)
    {
        int tbd = 1;
    }
    else
    {
        //	printf ("PackAU: seq num is: %d\n", GetCurrentSeqNum());
    }
#endif

    RTPPacket * tmp_packet_chain = NULL;
    const AccessUnit * currentAU = input_AU_chain;
    num_AU_processed = 0;
    int k;
    uint16 num_packets_made = 0; // was m: num of packets
    MediaTimestamp rtp_timestamp;
    status_code = RPH_SUCCESS;
    bool mtbFlag = false;
    packet_list_tail = list_of_packets_to_send = NULL;

    { // base layer
        while (currentAU)
        {
            rtp_timestamp = currentAU->GetTimestamp();
            if (!isRfc2429)
            {
                // BufferFragment *tmp_buf = currentAU->GetCodecInfo();
                // void *tmp_ptr = currentAU->GetCodecInfo()->ptr;
                if (currentAU->GetCodecInfo() && currentAU->GetCodecInfo()->ptr &&
                        ((* (uint32 *) currentAU->GetCodecInfo()->ptr) & 0x00010000))
                    mtbFlag	= true;
                else
                    mtbFlag = false;

                if (mtbFlag && !config_au_flag)
                {

#ifdef DEBUG_MTB
//				printf("Found a positive MTB flag at seq num = %d\n", GetCurrentSeqNum());
#endif
                    // start a new packet if the AU has an MTB
                    if (current_packet_ptr && current_packet_ptr->GetMediaSize() > 0)
                    {
                        num_packets_made++;
                        this->closeCurrentPacket(current_packet_ptr,
                                                 packet_list_tail,
                                                 list_of_packets_to_send
                                                );
                    }
                }
                else
                {
#ifdef DEBUG_MTB
                    //		printf("Found a negative MTB flag at seq num = %d\n", GetCurrentSeqNum());
#endif
                }
            };


            uint32 new_media_size = currentAU->GetLength();
            if (config_au_flag && current_packet_ptr) // treat config_au as part of the first data AU
                new_media_size += current_packet_ptr->GetMediaSize();

            //      if (currentAU->GetLength() > (uint32) max_packet_size) {
            if (new_media_size > (uint32) max_packet_size)
            {
                // if necessary, close the current packet
                if (current_packet_ptr &&
                        current_packet_ptr->GetMediaSize() > 0  &&
                        !config_au_flag)
                {
                    num_packets_made++;
                    this->closeCurrentPacket(current_packet_ptr,
                                             packet_list_tail,
                                             list_of_packets_to_send
                                            );
                }

                // Try to break AU just before resync_marker
                // This involves a pretty expensive search process
                uint8 num_bits = NUM_BITS_RESYNC_MARKER;

                rtp_timestamp = currentAU->GetTimestamp();

                uint16 last_byte_offset;

                uint16 last_frag_index;

                uint16 starting_index = 0;
                uint16 starting_frag_offset = 0;

                //        uint16 num_of_fragments_in_AU = currentAU->GetNumFragments();
                //  uint16 k; // stores the number of fragments in next packet

                int32 num_bytes_left_in_AU = (int32) currentAU->GetLength();
                int32 adjusted_max_packet_size; // used in seeking

                //  printf("in the begining, num_bytes_left_in_AU is %d\n", num_bytes_left_in_AU);
                while (num_bytes_left_in_AU > 0)
                {
                    if (NULL == current_packet_ptr)
                    {
                        current_packet_ptr = this->getNewRtpPacket(isRfc2429, status_code);
                        if (NULL == current_packet_ptr)
                        {
                            tmp_packet_chain = list_of_packets_to_send;
                            list_of_packets_to_send = NULL;
                            packet_list_tail = NULL;
                            return tmp_packet_chain;
                        }
                    }

                    if (config_au_flag)
                        adjusted_max_packet_size = max_packet_size - current_packet_ptr->GetMediaSize();
                    else
                        adjusted_max_packet_size = max_packet_size;

                    if (needToSearchForStartCode)
                    {
                        //
                        // The following search routine will return the position of the resync_marker;
                        // If no resync_marker is found between starting_p and (starting_p + max_packet_size),
                        // last_frag_index will be set to the index of the fragment containing
                        // (starting_index + max_packet_size); last_frag_size will be set accordingly, too.
                        //
                        this->search_backwards_for_bit_pattern
                        (*currentAU,
                         starting_index,
                         starting_frag_offset,
                         adjusted_max_packet_size - sizeof(resync_marker),
                         (octet *) & resync_marker,
                         num_bits,
                         last_frag_index,
                         last_byte_offset
                        );
                    }
                    else    // no need to search for start code
                    {
                        bool boundaryReached;
                        int last_frag_index = starting_index;
                        int last_byte_offset = starting_frag_offset;
                        octet * tmp_ptr;  // should i inistialize tmp_ptr? No!
                        boundaryReached = false;
                        if (!currentAU->seek(last_frag_index, last_byte_offset, tmp_ptr, boundaryReached, adjusted_max_packet_size - sizeof(resync_marker)))
                        {
                            // printf("VariableAuSizePacketizer::packetAU(): currentAU.seek returned error. Some states are corrupted\n");
                            return NULL;
                        }

                        if (boundaryReached)
                        {
                            last_frag_index = currentAU->GetNumFrags() - 1;
                            last_byte_offset = (currentAU->GetMediaFragment(last_frag_index))->len;
                        }
                    }


                    //     printf("\n\nreturned from search_backwards_for_bit_pattern()\n");
                    BufferFragment tmp_fragment;
                    BufferState * tmp_buf_state;
                    currentAU->GetMediaFragment(starting_index, tmp_fragment, tmp_buf_state);
                    tmp_fragment.ptr = (uint8 *)tmp_fragment.ptr + starting_frag_offset;
                    if (starting_index == last_frag_index)
                    {
                        tmp_fragment.len = last_byte_offset - starting_frag_offset;
                    }
                    else
                    {
                        tmp_fragment.len += starting_frag_offset;
                    }

                    if (tmp_fragment.len <= 0)
                    {
                        // UNCOND_LOGMSG(ERR_ERROR, ("tmp_fragment.size = %d < 0. Force it to zero \n", tmp_fragment.len));
                        tmp_fragment.len = 0;
                    }

                    num_bytes_left_in_AU -= tmp_fragment.len;

                    // printf("When numPacketCreated = %d, Breaking a big AU of size %d, num_bytes_left_in_AU is %ld, config_au_flag is %d\n", num_packets_made, currentAU->GetLength(), num_bytes_left_in_AU, config_au_flag );

                    if (isRfc2429 && starting_index == 0 &&
                            starting_frag_offset == 0 &&
                            tmp_fragment.len >= 3)
                    {
                        // set the h.263 payload header
                        if ((oscl_memcmp(tmp_fragment.ptr, resync_marker, 2)) == 0 &&
                                ((uint8*)tmp_fragment.ptr)[2] & 0x80)
                        {
                            BufferFragment*  headerFrag = &current_packet_ptr->GetFragments()[1];
                            // its a start code. set the P bit
                            ((uint8*)headerFrag->ptr)[0] = 0x04;

                            // This format doesn't work on HP, because, strictly
                            // speaking, the left side is not a modifiable lvalue.
                            // ((uint8*)tmp_fragment.ptr)+=2;
                            // so, we do this:
                            tmp_fragment.ptr = ((uint8*)tmp_fragment.ptr) + 2;
                            tmp_fragment.len -= 2;
                        }
                    } // end isRfc2429

                    /*printf("When m = %d, Breaking a big AU of size %d, num_bytes_left_in_AU is %d, this payload size is now %d\n",m, currentAU->GetLength(),
                    num_bytes_left_in_AU, payload_size);*/

                    current_packet_ptr->AddRTPFrag(tmp_fragment, tmp_buf_state);

                    if (last_frag_index > starting_index)
                    {
                        k = last_frag_index - starting_index + 1;

                        for (int ii = 1; ii < k - 1; ii++)
                        {
                            currentAU->GetMediaFragment(starting_index + ii, tmp_fragment, tmp_buf_state);
                            current_packet_ptr->AddRTPFrag(tmp_fragment, tmp_buf_state);
                            num_bytes_left_in_AU -= tmp_fragment.len;
                        }

                        if (last_frag_index > starting_index && last_byte_offset > 0)
                        {
                            currentAU->GetMediaFragment(last_frag_index, tmp_fragment, tmp_buf_state);
                            tmp_fragment.len = last_byte_offset;
                            num_bytes_left_in_AU -= tmp_fragment.len;
                            current_packet_ptr->AddRTPFrag(tmp_fragment, tmp_buf_state);
                        }
                    }

                    starting_index = last_frag_index;
                    starting_frag_offset = last_byte_offset;

                    // num_bytes_left_in_AU -= current_packet_ptr->GetMediaSize(); this logic erroneously counted H263 header bytes

                    //printf("Closing a packet with payload size = %d, , num_bytes_left_in_AU = %d\n", current_packet_ptr->GetMediaSize(), num_bytes_left_in_AU);

                    // current_packet_ptr->SetTimestamp(rtp_timestamp);
                    if (!config_au_flag)
                    {
                        if (num_bytes_left_in_AU <= 0)
                        {
                            this->StuffRtpHeaderNRtpRtc(current_packet_ptr,
                                                        rtp_timestamp,
                                                        true); // MBIT is 1
                        }
                        else
                        {
                            this->StuffRtpHeaderNRtpRtc(current_packet_ptr,
                                                        rtp_timestamp,
                                                        false); // MBIT is 0
                        }
                    }
                    else
                    {
                        if (num_bytes_left_in_AU <= 0)
                        {
                            current_packet_ptr->SetMBit(true);
                        }
                        else
                        {
                            current_packet_ptr->SetMBit(false);
                        }
                        current_packet_ptr->SetTimestamp(rtp_timestamp);
                    }
                    num_packets_made++;
                    this->closeCurrentPacket(current_packet_ptr,
                                             packet_list_tail,
                                             list_of_packets_to_send
                                            );

                    if (config_au_flag)
                        config_au_flag = false;
                }      // while loop
            }
            else   // no need to fragment the AU
            {
                // put AU in current RTP packet
                if (config_au_flag && current_packet_ptr)
                {
                    /*
                    this->StuffRtpHeaderNRtpRtc(current_packet_ptr,
                    						  rtp_timestamp,
                    						  true
                    						  ); // MBIT is 1
                    */
                    current_packet_ptr->SetMBit(true);

                    config_au_flag = false;
                }

                if (current_packet_ptr)
                {
                    if (((uint32) max_packet_size - current_packet_ptr->GetMediaSize()) < currentAU->GetLength())
                    {
                        num_packets_made++;
                        this->closeCurrentPacket(current_packet_ptr,
                                                 packet_list_tail,
                                                 list_of_packets_to_send);
                    }
                }


                if (NULL == current_packet_ptr)
                {
                    current_packet_ptr = this->getNewRtpPacket(isRfc2429, status_code);

                    if (NULL == current_packet_ptr)
                    {
                        tmp_packet_chain = list_of_packets_to_send;
                        list_of_packets_to_send = NULL;
                        packet_list_tail = NULL;
                        return tmp_packet_chain;
                    }

                    this->StuffRtpHeaderNRtpRtc(current_packet_ptr,
                                                rtp_timestamp,
                                                true); // MBIT is 1
                }
                if (isRfc2429)
                {
                    this->AddAU(currentAU);
                }
                else
                {
                    current_packet_ptr->AddAU(currentAU);
                }
            }// AU size is less than max packet size

            num_AU_processed++;   // increment the number of AU's processed
            currentAU = currentAU->GetNext();
            if (config_au_flag)
                config_au_flag = false;
        } // while (currentAU != NULL)
    } // else (is_base_layer == true)

    // when only one packect is made, Ling
    if (list_of_packets_to_send == 0 && current_packet_ptr != 0)
    {
        num_packets_made++;
        this->closeCurrentPacket(current_packet_ptr,
                                 packet_list_tail,
                                 list_of_packets_to_send);
    }

    tmp_packet_chain = list_of_packets_to_send;
    list_of_packets_to_send = NULL;
    packet_list_tail = NULL;
    return tmp_packet_chain;
};


#if 0
// need a single routine to dynamically load packers
RtpPayloadHandler * maker_of_rtp_packer
(RTPPacketAlloc * rtpAlloc,
 AccessUnitAlloc * accessUnitAlloc,
 const uint16 max_pckt_size
)
{
    return new Rfc3016VideoPacker(rtpAlloc,
                                  accessUnitAlloc,
                                  max_pckt_size);
}
#endif

OSCL_EXPORT_REF
RtpPayloadHandler::RtpPayloadStatus  RtpPayloadHandler :: packConfigAU(const AccessUnit * config_au)
{

    if (NULL != current_packet_ptr)
    {
        return CONFIG_HEADER_MUST_START_A_PACKET;
    }


    current_packet_ptr = rtp_alloc->allocate();

    if (NULL == current_packet_ptr)
    {

        return MEM_ALLOC_FAILURE ;
    }

    this->StuffRtpHeaderNRtpRtc(current_packet_ptr,
                                config_au->GetTimestamp(),
                                false); // MBIT is 0

    current_packet_ptr->AddAU(config_au);

    this->config_au_flag = true;

    return RPH_SUCCESS;

}
