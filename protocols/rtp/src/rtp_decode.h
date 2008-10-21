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
**   File:   rtp_decode.h
**
**   Description:
**      This module defines the RTP Decode class. This class is used to decode the
**      RTP header. Please refer to the RTP/RTCP design document for details.
*/
#ifndef   RTP_DECODE_H
#define   RTP_DECODE_H

#include "rtp.h"
#include "oscl_bin_stream.h"

class RTP_Decode : public RTP_Base
{
    public:
        RTP_Decode(const RtpSsrc expected_ssrc = 0,
                   const uint8 version = DEFAULT_RTPRTCP_VERSION);
        ~RTP_Decode();
    public:
        RtpStatus decode(const uint8 * msg,
                         const uint16 size,
                         RtpSsrc& ssrc,
                         RtpTimeStamp & timestamp,
                         RtpSeqType & seq_num,
                         bool & marker_flag,
                         bool & extension_flag,
                         RtpPayloadType & payload_type,
                         uint8 * & payload_ptr,
                         uint16 & payload_size,
                         uint8 & num_of_csrc);

        int getExtSize()
        {
            return savedRtpExtSize;    // number of 32-bit words in the extension
        }

        int getExt(uint32 * const Ext_buffer, int max_words); // get up to max_words from the extension

        int getCSRCs(uint32 * const CSRC_buffer);

    private:
        // Pointer to RTP message which
        // is used in case CSRCs are found
        // and maybe retrieved by the caller
        // later
        uint16  savedRtpMsgSize;
        uint8*  savedRtpMsgPtr;
        uint8   savedNumCSRC;
        uint16  savedRtpExtSize;
        uint8*  savedRtpExtPtr;
};

#endif

