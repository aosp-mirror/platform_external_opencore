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
**   File:   rtp_encode.h
**
**   Description:
**      This module defines the RTP Encode class. This class is used to encode
**      RTP header. Please refer to the RTP/RTCP design document for details.
*/
#ifndef   RTP_ENCODE_H
#define   RTP_ENCODE_H

#include "rtp.h"
#include "oscl_bin_stream.h"

class RTP_Encode : public RTP_Base
{
    public:
        OSCL_IMPORT_REF RTP_Encode(const RtpSsrc SSRC,
                                   const RtpSeqType init_seq_offset,
                                   const RtpTimeStamp init_ts_offset,
                                   const RtpPayloadType in_payload_type,
                                   const uint8 version = DEFAULT_RTPRTCP_VERSION);

        OSCL_IMPORT_REF ~RTP_Encode();
        OSCL_IMPORT_REF RtpStatus stuffHeaderInMemory(const RtpTimeStamp timestamp,
                const bool marker_flag,
                uint8 * rtp_header_ptr
                                                     );
        OSCL_IMPORT_REF void setPayloadType(const RtpPayloadType new_payload_type)
        {
            payload_type = new_payload_type;
        };

        OSCL_IMPORT_REF void setSeqNum(const RtpSeqType seq)
        {
            currSeqNum = seq;
        };
        OSCL_IMPORT_REF RtpSeqType getSeqNum()
        {
            return currSeqNum;
        };
    private:
        OsclBinOStreamBigEndian outStream;
        RtpSeqType   currSeqNum;
        RtpTimeStamp currRtpTS;
        RtpPayloadType payload_type;
};

#endif
