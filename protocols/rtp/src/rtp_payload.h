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
#ifndef __RTP_PAYLOAD_H
#define __RTP_PAYLOAD_H
#if defined(PV_OS_UNIX)
#include <dlfcn.h>
#endif

#ifdef PV_OS_ZREX
#include "zrex_os.h"
#endif

#include "oscl_string.h"

#include "oscl_media_data.h"
#include "media_clock_converter.h"

#include "access_unit.h"
#include "rtp_packet.h"
#include "rtp_encode.h"
#include "pv_gau.h"
#include "oscl_time.h"

/* - - - - - - - - - - - - - - - -  -- - - - - - -  - - - - - - - - - - - - - -*/
/* --------  Constants ----------------------*/
const int INVALID_SAMPLING_RATE = 0;
const int DEF_INPUT_SAMPLING_RATE = 1000;

#define DEF_TOC_CHUNK_SIZE 10

typedef int * (* malloc_function_ptr_t)(const size_t size);

/** @name RtpPayloadHandler packs a chain of AccessUnit into the payload of one or more RTP packets.
    One instance of this class is associated with one streaming client session.
*/

class RtpPayloadHandler
{
    public:
        typedef enum
        {
            RPH_SUCCESS = 0,
            BAD_INPUT_FORMAT = 1,
            MEM_ALLOC_FAILURE = 2,
            MEDIA_DATA_ERROR = 3,
            TOO_BIG_FOR_PACKET = 4,
            ERROR_ADDING_TO_PACKET = 5,
            ERROR_GETTING_LOCAL_BUFFER = 6,
            CONFIG_HEADER_MUST_START_A_PACKET = 7
        } RtpPayloadStatus;



        //
        // Method "pack" returns a pointer to RTPPacket which links to other RTPPackets via next pointer.
        // It also updates a status code.
        //
        OSCL_IMPORT_REF virtual RTPPacket * pack(const GAU * gau,
                int& out_number_processed,
                RtpPayloadStatus & status_code);

        virtual RTPPacket * packAU(const AccessUnit * input_AU_chain,
                                   int& out_number_processed,
                                   RtpPayloadStatus & status_code) = 0;

        virtual RTPPacket* flush(RtpPayloadStatus & status_code) = 0;

        inline void set_max_packet_size(const uint16 n)
        {
            max_packet_size = n;
        };
        inline void set_payload_type(const uint8 in_PT)
        {
            rtp_encoder.setPayloadType(in_PT);
        };
        inline void set_rtp_seqnum(const RtpSeqType n)
        {
            rtp_encoder.setSeqNum(n);
        };
        inline void set_rtp_timestamp(const MediaTimestamp t)
        {
            currRtpTime = t;
        };
        void set_SSRC(const RtpSsrc n)
        {
            rtp_encoder.SetSSRC(n);
        };


        inline void set_malloc_function_ptr(malloc_function_ptr_t fn)
        {
            malloc_fn_ptr = fn;
        };

        RtpPayloadHandler(const RtpSsrc ssrc,
                          const RtpPayloadType pt,
                          RTPPacketAlloc * rtpAlloc = NULL,
                          AccessUnitAlloc * accessUnitAlloc = NULL,
                          const RtpSeqType init_seq_offset = 0,
                          const MediaTimestamp init_ts_offset = 0,
                          const uint16 max_pckt_size = DEF_RTP_MAX_PACKET_SIZE
                         ) :

                rtp_encoder(ssrc, init_seq_offset, init_ts_offset,  pt)
        {
            rtp_alloc = rtpAlloc;
            access_unit_alloc = accessUnitAlloc;
            max_packet_size = max_pckt_size;

            requiredOutputSamplingRate = INVALID_SAMPLING_RATE;

            rtpRtcOffset = 0;

            init_rtp_ts_offset = init_ts_offset;
            currRtpTime = init_ts_offset;

            config_au_flag = false;
            current_packet_ptr = NULL;
        };

        virtual ~RtpPayloadHandler()
        {
        };

        void setInputSamplingRate(const int inputRate)
        {
            if (inputRate > 0)
                media_clock_converter.set_timescale(inputRate);
        }


        // the following should be overloaded by derived classes
        inline virtual int getRequiredRTPSamplingRate()
        {
            return INVALID_SAMPLING_RATE;
        }

        void setRtpRtcOffset(const MediaTimestamp offset)
        {
            rtpRtcOffset = offset;
        }

        void OpenPlayRange(const MediaTimestamp start_timeMS)
        {
            media_clock_converter.set_clock_other_timescale(start_timeMS, 1000);

        }
        void ClosePlayRange(const MediaTimestamp stop_timeMS)
        {
            uint32 timediff =
                media_clock_converter.get_timediff_and_update_clock(stop_timeMS, 1000,
                        requiredOutputSamplingRate);
            currRtpTime += timediff;

        }

        MediaTimestamp GetCurrentRtpTS()
        {
            return currRtpTime;
        };
        RtpSeqType GetCurrentSeqNum()
        {
            return rtp_encoder.getSeqNum();
        };

        MediaTimestamp elapsedTime2RtpTS(const MediaTimestamp init_rtp_ts,  // RTP TS at begining of play session
                                         const TimeValue elapsedTime)
        {
            MediaTimestamp tmp = init_rtp_ts;
            tmp += elapsedTime.to_msec() * (uint32)(requiredOutputSamplingRate / 1000);
            return tmp;
        }

        OSCL_IMPORT_REF virtual RtpPayloadStatus packConfigAU(const AccessUnit * config_au);

    protected:
        //
        // call this if output rate is fixed. For example, for AMR/EVRC, output has to be 8K Hz
        void init_with_input_sampling_rate(const int input_sample_rate)
        {
            uint32 inputSamplingRate;
            if (input_sample_rate > 0)
            {
                inputSamplingRate = input_sample_rate;
            }
            else
            {
                inputSamplingRate = DEF_INPUT_SAMPLING_RATE;
                //      int look_here_to_generate_error_log;
            }

            media_clock_converter.set_timescale(inputSamplingRate);

        }

        //
        // call this if output rate is not fixed. For example, for RFC3016, output does not have to be 90000
        void init_timescale_conversion(const int input_sample_rate,
                                       const int output_sample_rate)
        {
            uint32 inputSamplingRate;
            // if (inputSamplingRate > 0) {
            if (input_sample_rate > 0)
            {
                inputSamplingRate = input_sample_rate;
            }
            else
            {
                //      int generate_a_log_here;
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

            media_clock_converter.set_timescale(inputSamplingRate);
        }

        void StuffRtpHeaderNRtpRtc
        (
            RTPPacket * packet,
            const MediaTimestamp normalPlayTime, // from AU
            const bool MBit
        )
        {
            BufferFragment * rtp_header = packet->GetRTPHeader();
            uint32 timediff =
                media_clock_converter.get_timediff_and_update_clock(normalPlayTime,
                        requiredOutputSamplingRate);

            /* printf("Normal play time = %ld, timediff = %ld, currRtpTime = %ld\n",
               normalPlayTime, timediff, currRtpTime); */
            currRtpTime += timediff;

            rtp_encoder.stuffHeaderInMemory(currRtpTime, MBit, (uint8 *)rtp_header->ptr);

            // SetTimestamp in RTPPacket as well, after adding the RTP_RTC offset
            RtpTimeStamp tsMS = media_clock_converter.get_converted_ts(1000);
            tsMS += rtpRtcOffset;
            packet->SetTimestamp(tsMS);
        }

        RTP_Encode rtp_encoder;
        uint16 max_packet_size;

        // MediaTimestamp timestamp_offset;
        MediaTimestamp rtpRtcOffset; // RTP Real Time Clock offset at the begining of "play" command

        // uint16 next_seqnum;

        AccessUnitAlloc      * access_unit_alloc;
        RTPPacketAlloc     * rtp_alloc;
        malloc_function_ptr_t malloc_fn_ptr;

        uint32 requiredOutputSamplingRate;  // as required by RFCs. For instance, AMR RFC requires 8K sampling rate



        MediaClockConverter media_clock_converter;  // current normal play time in inputSamplingRate

        MediaTimestamp init_rtp_ts_offset;

        MediaTimestamp currRtpTime;

        bool config_au_flag;
        RTPPacket * current_packet_ptr;
};

typedef RtpPayloadHandler * (* maker_of_rtp_payload_module_t)();

class SearchableRtpPayloadHandler : public RtpPayloadHandler
{
    public:
        SearchableRtpPayloadHandler(const RtpSsrc ssrc,
                                    const RtpPayloadType pt,
                                    RTPPacketAlloc * rtpAlloc = NULL,
                                    AccessUnitAlloc * accessUnitAlloc = NULL,
                                    const RtpSeqType init_seq_offset = 0,
                                    const MediaTimestamp init_ts_offset = 0,
                                    const uint16 max_pckt_size = DEF_RTP_MAX_PACKET_SIZE) :
                RtpPayloadHandler(ssrc, pt, rtpAlloc, accessUnitAlloc, init_seq_offset, init_ts_offset, max_pckt_size)
        {};

        virtual RTPPacket * packAU(const AccessUnit * input_AU_chain,
                                   int& out_number_processed,
                                   RtpPayloadStatus & status_code) = 0;


        virtual RTPPacket * flush(RtpPayloadStatus & status_code) = 0;

    protected:

        //
        // The following method searches backwards in the memory fragment group for a pattern, starting
        // from (starting_index, starting_frag_offset). If pattern is found, last_frag_index points to the
        // fragment containing the found pattern, and last_byte_offset is the pattern's offset.
        //
        // If no pattern is found between starting_index and (starting_index + max_packet_size),
        // last_frag_index will be set to the index of the fragment containing
        // (starting_index + max_packet_size); last_byte_offset will be set accordingly, too.
        //
        // It returns the number of fragments between starting_index and last_frag_index.
        // Note:
        //    The pattern is assumed to start at beginning of some byte, but may have non-integral number of bytes.
        //
        bool search_backwards_for_bit_pattern
        (
            const AccessUnit & currentAU,
            const uint16 starting_index,
            const uint16 starting_frag_offset,
            const int max_packet_size,
            const octet * pattern,
            const uint16 num_bits,
            uint16 & last_frag_index,
            uint16 & last_byte_offset
        );
};

const int DEFAULT_IETF_VIDEO_SAMPLING_RATE = 90000;  // in HZ
const int DEFAULT_PVA_VIDEO_INPUT_SAMPLING_RATE = 1000;  // in HZ
const int Rfc2429_PAYLOAD_HDR_SIZE = 2;

class VariableAuSizePacketizer : public SearchableRtpPayloadHandler
{
    public:
        OSCL_IMPORT_REF
        VariableAuSizePacketizer(const RtpSsrc ssrc,
                                 const RtpPayloadType pt,
                                 RTPPacketAlloc * rtpAlloc,
                                 AccessUnitAlloc * accessUnitAlloc = NULL,
                                 const RtpSeqType init_seq_offset = 0,
                                 const MediaTimestamp init_ts_offset = 0,
                                 const int max_pckt_size = DEF_RTP_MAX_PACKET_SIZE,
                                 const int input_sample_rate = DEFAULT_PVA_VIDEO_INPUT_SAMPLING_RATE,
                                 const int output_sample_rate = DEFAULT_IETF_VIDEO_SAMPLING_RATE,
                                 const bool mustSearchStartCode = true);

        virtual RTPPacket * packAU(const AccessUnit * input_AU_chain,
                                   int& out_number_processed,
                                   RtpPayloadStatus & status_code) = 0;



        virtual RTPPacket * flush(RtpPayloadStatus & status_code)
        {
            RTPPacket * packetPtr;
            status_code = RtpPayloadHandler::RPH_SUCCESS;
            if (!list_of_packets_to_send)
            {
                if (!current_packet_ptr)
                {
                    packetPtr = NULL;
                }
                else if (current_packet_ptr->GetMediaSize() > 0)   // this is the only packet created
                {
                    packetPtr = current_packet_ptr;
                    current_packet_ptr = NULL;
                }
                else
                {
                    packetPtr = NULL;
                }
            }
            else
            {
                if (NULL != current_packet_ptr)
                {
                    if (current_packet_ptr->GetMediaSize() > 0)
                    {
                        packet_list_tail->AppendNext(current_packet_ptr);
                        packet_list_tail = current_packet_ptr;
                    }
                }
                packetPtr = list_of_packets_to_send;
                list_of_packets_to_send = NULL;
                packet_list_tail = NULL;
                current_packet_ptr = NULL;
            }
            return packetPtr;
        };

        void closeCurrentPacket(
            RTPPacket * & current_packet_ptr,
            RTPPacket * & packet_list_tail,
            RTPPacket * & packet_list_head
        );

        OSCL_IMPORT_REF RTPPacket * packAU(const AccessUnit * input_AU_chain,
                                           const bool isRfc2429,
                                           int& out_number_processed,
                                           RtpPayloadStatus & status_code);


    protected:


        RTPPacket * getNewRtpPacket(const bool isRfc2429, RtpPayloadStatus & status_code);
        MediaStatusClass::status_t AddAU(const AccessUnit * au);

        bool needToSearchForStartCode;
        bool needToChopTwoBytesFromStartCode;
        // static
        uint8 resync_marker[4];


        RTPPacket * packet_list_tail;
        RTPPacket * list_of_packets_to_send;

        // only useful for Rfc2429 at the present, TZ Setp. 2001
        BufferFragment payload_header_fragment;
}; // VariableAuSizePacketizer


inline void VariableAuSizePacketizer :: closeCurrentPacket
(
    RTPPacket * & current_packet_ptr,
    RTPPacket * & packet_list_tail,
    RTPPacket * & packet_list_head
)
{
    if (!packet_list_head)   // this is the first packet created
    {
        packet_list_tail = packet_list_head = current_packet_ptr;

    }
    else
    {
        packet_list_tail->AppendNext(current_packet_ptr);
        packet_list_tail = current_packet_ptr;
    }

    current_packet_ptr = NULL;
};

#endif  // __RTP_PAYLOAD_H
