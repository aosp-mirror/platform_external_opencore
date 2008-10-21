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
**      This module defines the RTP Decode class. This class is used to decode
**      RTP header. Please refer to the RTP/RTCP design document for details.
*/

#include <stdlib.h>
#include <math.h>
#include "rtp_decode.h"

RTP_Decode::RTP_Decode(const RtpSsrc expected_ssrc,
                       const uint8 version)
        : RTP_Base(expected_ssrc, version)
{
    savedNumCSRC = 0;
    savedRtpMsgPtr = NULL;
    savedRtpMsgSize = 0;
    savedRtpExtPtr = NULL;
    savedRtpExtSize = 0;
}

RTP_Decode::~RTP_Decode()
{}

RTP_Base::RtpStatus RTP_Decode::decode(const uint8* msg, uint16 size,
                                       RtpSsrc& ssrc, RtpTimeStamp & timestamp,
                                       RtpSeqType & seq_num, bool & marker_flag,
                                       bool & extension_flag,  RtpPayloadType & payload_type,
                                       uint8* & payload_ptr, uint16 & payload_size,
                                       uint8 & num_of_csrc
                                      )
{
    OSCL_UNUSED_ARG(extension_flag);

    uint8 tempChar;
    uint8 rcvdVersion;
    int pad_bit;
    int ext_bit;
    uint8 pad_bytes = 0;
    uint16 ext_size = 0;



    // Reset saved Message pointers
    savedNumCSRC = 0;
    savedRtpMsgPtr = NULL;
    savedRtpMsgSize = 0;
    savedRtpExtPtr = NULL;
    savedRtpExtSize = 0;

    OsclBinIStreamBigEndian inStream;
    inStream.Attach((void *)msg, size);

    inStream >> tempChar;
    rcvdVersion = tempChar >> RTPRTCP_VERSION_BIT_POSITION;
    pad_bit = (tempChar >> RTPRTCP_PAD_FLAG_BIT_POSITION) & 0x1;
    ext_bit = (tempChar >> RTP_EXT_FLAG_BIT_POSITION) & 0x1;
    num_of_csrc = tempChar & RTP_CSRC_COUNT_MASK;

    inStream >> tempChar;
    if (tempChar >> RTP_MARKER_FLAG_POSITION)
        marker_flag = true;
    else
        marker_flag = false;
    payload_type = tempChar & RTP_PAYLOAD_TYPE_MASK;

    inStream >> seq_num;
    inStream >> timestamp;
    inStream >> ssrc;

    if (rcvdVersion != rtpVersion)
        return RTP_NOT_SUPPORTED;


    if (ext_bit)
    {
        const int RTP_EXT_TYPE_BYTES = 2;
        const int RTP_EXT_HDR_WORDS = 1;
        const int RTP_EXT_LEN_TO_BYTES = 4;
        inStream.Seek(RTP_HEADER_SIZE + (num_of_csrc * NUM_BYTES_IN_UINT_32) + RTP_EXT_TYPE_BYTES);
        inStream >> ext_size;


        // add the size of the extension header (in 32-bit words)
        ext_size += RTP_EXT_HDR_WORDS;

        // Save pointer to extension information in case the caller decides to
        // retrieve it. Pointer is valid only until
        // the next call to decode is made
        savedRtpExtPtr = const_cast<uint8 *>(msg + RTP_HEADER_SIZE + (num_of_csrc * NUM_BYTES_IN_UINT_32));
        savedRtpExtSize = ext_size;

        // convert size to bytes
        ext_size *= RTP_EXT_LEN_TO_BYTES;
    }

    if (pad_bit)
    {
        // figure how many pad bytes there are
        inStream.Seek(size - 1);
        inStream >> pad_bytes;
    }

    payload_size = size - RTP_HEADER_SIZE - (num_of_csrc * NUM_BYTES_IN_UINT_32) - pad_bytes - ext_size;

    // Skip the CSRCs and any extensions
    payload_ptr = const_cast<uint8 *>(msg + RTP_HEADER_SIZE + (num_of_csrc * NUM_BYTES_IN_UINT_32) + ext_size);

    // Save pointer to message in case the caller decides to
    // retrieve the CSRCs. Pointer is valid only until
    // the next call to decode is made
    if (num_of_csrc > 0)
    {
        savedNumCSRC = num_of_csrc;
        savedRtpMsgPtr = const_cast<uint8 *>(msg);
        savedRtpMsgSize = size;
    }

    return RTP_OK;
}


int RTP_Decode::getExt(uint32 * const Ext_buffer, int max_words)
{ // number of 32-bit words in the extension

    int words_to_copy = (savedRtpExtSize < max_words) ? savedRtpExtSize : max_words;
    int cnt = 0;

    if (savedRtpExtPtr != NULL && savedRtpExtSize > 0)
    {
        const int RTP_EXT_BYTES_PER_WORD = 4;
        OsclBinIStreamBigEndian inStream;
        inStream.Attach((void *)savedRtpExtPtr, savedRtpExtSize*RTP_EXT_BYTES_PER_WORD);

        for (cnt = 0 ; cnt < words_to_copy; ++cnt)
        {
            inStream >> Ext_buffer[cnt];
        }
    }

    return cnt;

}

int RTP_Decode::getCSRCs(uint32 * const CSRC_buffer)
{
    int cnt = 0;
    if ((savedRtpMsgPtr != NULL) && savedRtpMsgSize > RTP_HEADER_SIZE)
    {
        OsclBinIStreamBigEndian inStream;
        inStream.Attach((void *)savedRtpMsgPtr, savedRtpMsgSize);

        // Seek to first CSRC
        inStream.seekFromCurrentPosition(RTP_HEADER_SIZE);

        for (cnt = 0;cnt < savedNumCSRC ; ++cnt)
        {
            inStream >> CSRC_buffer[cnt];
        }
    }

    return cnt;
}
