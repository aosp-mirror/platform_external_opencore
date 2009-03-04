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
/*                                                                               */
/*********************************************************************************/

/*
**   File:   rtp_encode.cpp
**
**   Description:
**      This module defines the RTP Encode class. This class is used to encode
**      RTP header. Please refer to the RTP/RTCP design document for details.
*/

#include <stdlib.h>
#include <math.h>
#include "rtp_encode.h"

OSCL_EXPORT_REF
RTP_Encode::RTP_Encode(const RtpSsrc ssrc,
                       const RtpSeqType seq_offset,
                       const RtpTimeStamp ts_offset,
                       const RtpPayloadType in_payload_type,
                       const uint8 version)
        : RTP_Base(ssrc, version)
{
    payload_type = in_payload_type;
    currSeqNum = seq_offset;
    currRtpTS = ts_offset;
}

OSCL_EXPORT_REF
RTP_Encode::~RTP_Encode()
{}

OSCL_EXPORT_REF
RTP_Base::RtpStatus RTP_Encode::stuffHeaderInMemory(const uint32 timestamp,
        const bool marker_flag,
        uint8 * rtp_header_ptr
                                                   )
{
    // User needs to allocate memory
    if (!rtp_header_ptr)
        return RTP_GENERAL_ERROR;

    // Okay to assume caller has allocated enough memory for header?
    outStream.Attach((void *)rtp_header_ptr, RTP_HEADER_SIZE);

    // Write version and csrc count
    uint8 tempChar = rtpVersion << RTPRTCP_VERSION_BIT_POSITION;
    outStream << tempChar;

    // Write marker flag and payload type
    tempChar = (marker_flag << RTP_MARKER_FLAG_POSITION) | (payload_type & RTP_PAYLOAD_TYPE_MASK);
    outStream << tempChar;

    // Write sequence number
    outStream << currSeqNum;

    // Write timestamp
    // RtpTimeStamp currTimeStamp = timestamp;
    currRtpTS = timestamp; // keep internal state updated;
    outStream << timestamp;

    // Write SSRC
    outStream << SSRC;

    // Increment sequence number
    ++currSeqNum;

    return RTP_OK;
}

