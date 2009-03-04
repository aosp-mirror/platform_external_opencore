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
**   File:   rtp.h
**
**   Description:
**      This module defines the RTP class. This is the base class common to the
**      encode and decode RTP classes. Please refer to the RTP/RTCP design document
**      for details.
*/

#ifndef   RTP_H
#define   RTP_H
#include "rtprtcp.h"

#define RTP_HEADER_SIZE		12
#define RTP_CSRC_COUNT_MASK 0x0f
#define RTP_MARKER_FLAG_POSITION 7
#define RTP_PAYLOAD_TYPE_MASK 0x7f
#define RTP_SEQ_NUM_MASK 0x00ff
const int16 RTP_EXT_FLAG_BIT_POSITION = 4;


class RTP_Base
{
    public:
        typedef enum
        {
            RTP_OK = 0,
            RTP_NOT_SUPPORTED = -1,
            RTP_INDEX_OUT_OF_RANGE = -2,
            RTP_GENERAL_ERROR = -3
        } RtpStatus;

        RTP_Base(const RtpSsrc ssrc = 0,
                 const uint8 version = DEFAULT_RTPRTCP_VERSION) :
                SSRC(ssrc),
                rtpVersion(version)
        {};

        virtual ~RTP_Base() {};

        RtpStatus GetSSRC(RtpSsrc& returnSSRC)
        {
            returnSSRC = SSRC;
            return RTP_OK;
        }

        void SetSSRC(const RtpSsrc& in_SSRC)
        {
            SSRC = in_SSRC;
        };
        // void SetITS(const RtpTimeStamp& ITS) {timeStampOffset = ITS;};
        // void SetISN(const RtpSeqType& ISN) {seqNumOffset = ISN;};

    protected:
        RtpSsrc SSRC; // synchronization source ID of the caller or
        // expected synchronization source ID of the
        // RTP sender
        // RtpSeqType  seqNumOffset;
        // RtpTimeStamp  timeStampOffset;
        uint8 rtpVersion;

};

#endif
