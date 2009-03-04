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
#include "pvmf_format_type.h"

#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif

#include "pv_mime_string_utils.h"

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()


OSCL_EXPORT_REF PvmfFormatIndex GetFormatIndex(char* mime_string, PvmfMediaTypeIndex aHint)
{
    if (!mime_string)
    {
        return PVMF_FORMAT_UNKNOWN;
    }

    //Parse to the last type component in the mimestring.
    //That way this routine can handle input like
    //  ".../audio/AMR" or ".../video/YUV420"
    char* lastcomp = mime_string;
#if 0
    int numcomps = pv_mime_string_compcnt(mime_string);
    if (numcomps <= 0)
    {
        return PVMF_FORMAT_UNKNOWN;
    }
    if (numcomps > 1)
    {
        int len = pv_mime_string_extract_type(numcomps, mime_string, lastcomp);
        if (len <= 0)
        {
            return PVMF_FORMAT_UNKNOWN;
        }
    }
#endif

    //compare last component to all the recognized formats.

    //uncompressed audio...
    if (aHint == PVMF_FORMAT_UNKNOWN ||
            aHint == PVMF_UNCOMPRESSED_AUDIO_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_PCM8) == 0)
        {
            return PVMF_PCM8;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_PCM16) == 0)
        {
            return PVMF_PCM16;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_PCM16_BE) == 0)
        {
            return PVMF_PCM16_BE;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ALAW) == 0)
        {
            return PVMF_PCM_ALAW;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ULAW) == 0)
        {
            return PVMF_PCM_ULAW;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_PCM) == 0)
        {
            return PVMF_PCM;
        }
    }

    //uncompressed video...
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_UNCOMPRESSED_VIDEO_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_YUV420) == 0)
        {
            return PVMF_YUV420;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_YUV422) == 0)
        {
            return PVMF_YUV422;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_RGB8) == 0)
        {
            return PVMF_RGB8;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_RGB12) == 0)
        {
            return PVMF_RGB12;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_RGB16) == 0)
        {
            return PVMF_RGB16;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_RGB24) == 0)
        {
            return PVMF_RGB24;
        }
    }

    //compressed audio...
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_COMPRESSED_AUDIO_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_AMR) == 0)
        {
            return PVMF_AMR_IETF_COMBINED;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_AMRWB) == 0)
        {
            return PVMF_AMRWB_IETF_PAYLOAD;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_AMR_IETF) == 0)
        {
            return PVMF_AMR_IETF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_AMRWB_IETF) == 0)
        {
            return PVMF_AMRWB_IETF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_AMR_IF2) == 0)
        {
            return PVMF_AMR_IF2;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_EVRC) == 0)
        {
            return PVMF_EVRC;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_MP3) == 0)
        {
            return PVMF_MP3;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ADIF) == 0)
        {
            return PVMF_ADIF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ADTS) == 0)
        {
            return PVMF_ADTS;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_LATM) == 0)
        {
            return PVMF_LATM;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_MPEG4_AUDIO) == 0)
        {
            return PVMF_MPEG4_AUDIO;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_G723) == 0)
        {
            return PVMF_G723;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_G726) == 0)
        {
            return PVMF_G726;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_WMA) == 0)
        {
            return PVMF_WMA;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ASF_AMR) == 0)
        {
            return PVMF_ASF_AMR;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_REAL_AUDIO) == 0)
        {
            return PVMF_REAL_AUDIO;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_3640) == 0)
        {
            return PVMF_MPEG4_AUDIO;
        }

        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ASF_MPEG4_AUDIO) == 0)
        {
            return PVMF_ASF_MPEG4_AUDIO;
        }
    }

    //compressed video...
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_COMPRESSED_VIDEO_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_M4V) == 0)
        {
            return PVMF_M4V;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_H2631998) == 0
                 || pv_mime_strcmp(lastcomp, PVMF_MIME_H2632000) == 0)
        {
            return PVMF_H263;//2 to 1 mapping.
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_H264_VIDEO_RAW) == 0)
        {
            return PVMF_H264_RAW;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_H264_VIDEO_MP4) == 0)
        {
            return PVMF_H264_MP4;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_H264_VIDEO) == 0)
        {
            return PVMF_H264;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_WMV) == 0)
        {
            return PVMF_WMV;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_REAL_VIDEO) == 0)
        {
            return PVMF_RV;
        }
    }

    //image...
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_IMAGE_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_M4V_IMAGE) == 0)
        {
            return PVMF_M4V_IMAGE;
        }
    }

    //multiplexed, stream or file formats...
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_MULTIPLEXED_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_MPEG4FF) == 0)
        {
            return PVMF_MPEG4FF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_H223) == 0)
        {
            return PVMF_H223;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_M4V_IMAGE) == 0)
        {
            return PVMF_M4V_IMAGE;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_RTP) == 0)
        {
            return PVMF_RTP;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_AMRFF) == 0)
        {
            return PVMF_AMRFF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_AACFF) == 0)
        {
            return PVMF_AACFF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_MP3FF) == 0)
        {
            return PVMF_MP3FF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_WAVFF) == 0)
        {
            return PVMF_WAVFF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_ASFFF) == 0)
        {
            return PVMF_ASFFF;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_RMFF) == 0)
        {
            return PVMF_RMFF;
        }
    }

    //raw data formats...
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_DATASTREAM_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_8BIT_RAW) == 0)
        {
            return PVMF_8BIT_RAW;
        }
    }

    // pvmf data source formats
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_DATA_SOURCE_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_RTSP_URL) == 0)
        {
            return PVMF_DATA_SOURCE_RTSP_URL;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_HTTP_URL) == 0)
        {
            return PVMF_DATA_SOURCE_HTTP_URL;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_SDP_FILE) == 0)
        {
            return PVMF_DATA_SOURCE_SDP_FILE;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_PVX_FILE) == 0)
        {
            return PVMF_DATA_SOURCE_PVX_FILE;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_MS_HTTP_STREAMING_URL) == 0)
        {
            return PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_REAL_HTTP_CLOAKING_URL) == 0)
        {
            return PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL;
        }
        else if (pv_mime_strcmp(lastcomp, PVMF_MIME_DATA_SOURCE_RTP_PACKET_SOURCE) == 0)
        {
            return PVMF_DATA_SOURCE_RTP_PACKET_SOURCE;
        }
    }

    //text.
    if (aHint == PVMF_FORMAT_UNKNOWN
            || aHint == PVMF_TEXT_FORMAT)
    {
        if (pv_mime_strcmp(lastcomp, PVMF_MIME_3GPP_TIMEDTEXT) == 0)
        {
            return PVMF_3GPP_TIMEDTEXT;
        }
    }

    //everything else...
    return PVMF_FORMAT_UNKNOWN;
}

/* Returns the high level media type index of this format type */
OSCL_EXPORT_REF PvmfMediaTypeIndex GetMediaTypeIndex(PvmfFormatIndex aFormatIndex)
{
    if (aFormatIndex >= PVMF_FIRST_UNCOMPRESSED_AUDIO &&
            aFormatIndex <= PVMF_LAST_UNCOMPRESSED_AUDIO)
    {
        return PVMF_UNCOMPRESSED_AUDIO_FORMAT;
    }
    else if (aFormatIndex >= PVMF_FIRST_COMPRESSED_AUDIO &&
             aFormatIndex <= PVMF_LAST_COMPRESSED_AUDIO)
    {
        return PVMF_COMPRESSED_AUDIO_FORMAT;
    }
    else if (aFormatIndex >= PVMF_FIRST_UNCOMPRESSED_VIDEO &&
             aFormatIndex <= PVMF_LAST_UNCOMPRESSED_VIDEO)
    {
        return PVMF_UNCOMPRESSED_VIDEO_FORMAT;
    }
    else if (aFormatIndex >= PVMF_FIRST_COMPRESSED_VIDEO &&
             aFormatIndex <= PVMF_LAST_COMPRESSED_VIDEO)
    {
        return PVMF_COMPRESSED_VIDEO_FORMAT;
    }
    else if (aFormatIndex >= PVMF_FIRST_MULTIPLEXED &&
             aFormatIndex <= PVMF_LAST_MULTIPLEXED)
    {
        return PVMF_MULTIPLEXED_FORMAT;
    }
    else if (aFormatIndex >= PVMF_FIRST_IMAGE &&
             aFormatIndex <= PVMF_LAST_IMAGE)
    {
        return PVMF_IMAGE_FORMAT;
    }
    else if (aFormatIndex > PVMF_FIRST_DATA_SOURCETYPE &&
             aFormatIndex < PVMF_LAST_DATA_SOURCETYPE)
    {
        return PVMF_DATA_SOURCE_FORMAT;
    }
    else if (aFormatIndex >= PVMF_FIRST_TEXT &&
             aFormatIndex <= PVMF_LAST_TEXT)
    {
        return PVMF_TEXT_FORMAT;
    }

    return PVMF_FORMAT_UNKNOWN;
}

/* This routine returns the mime string corresponding to the format index,
** of NULL if not recognized.
*/
OSCL_EXPORT_REF void GetFormatString(PvmfFormatIndex aFormatIndex, OSCL_String&str)
{
    switch (aFormatIndex)
    {
        case PVMF_FORMAT_UNKNOWN:
            str = PVMF_MIME_FORMAT_UNKNOWN;
            break ;

            //uncompressed audio...
        case PVMF_PCM8:
            str = PVMF_MIME_PCM8;
            break;

        case PVMF_PCM16:
            str = PVMF_MIME_PCM16;
            break;

            //uncompressed video...
        case PVMF_YUV420:
            str = PVMF_MIME_YUV420;
            break;
        case PVMF_YUV422:
            str = PVMF_MIME_YUV422;
            break;
        case PVMF_RGB8:
            str = PVMF_MIME_RGB8;
            break;
        case PVMF_RGB12:
            str = PVMF_MIME_RGB12;
            break;
        case PVMF_RGB16:
            str = PVMF_MIME_RGB16;
            break;
        case PVMF_RGB24:
            str = PVMF_MIME_RGB24;
            break;

            //compressed audio...
        case PVMF_AMR_IETF_COMBINED:
            str = PVMF_MIME_AMR;
            break;
        case PVMF_AMRWB_IETF_PAYLOAD:
            str = PVMF_MIME_AMRWB;
            break;
        case PVMF_AMR_IETF:
            str = PVMF_MIME_AMR_IETF;
            break;
        case PVMF_AMRWB_IETF:
            str = PVMF_MIME_AMRWB_IETF;
            break;
        case PVMF_AMR_IF2:
            str = PVMF_MIME_AMR_IF2;
            break;
        case PVMF_EVRC:
            str = PVMF_MIME_EVRC;
            break;
        case PVMF_MP3:
            str = PVMF_MIME_MP3;
            break;
        case PVMF_ADIF:
            str = PVMF_MIME_ADIF;
            break;
        case PVMF_ADTS:
            str = PVMF_MIME_ADTS;
            break;
        case PVMF_LATM:
            str = PVMF_MIME_LATM;
            break;
        case PVMF_MPEG4_AUDIO:
            str = PVMF_MIME_MPEG4_AUDIO;
            break;
        case PVMF_G723:
            str = PVMF_MIME_G723;
            break;
        case PVMF_G726:
            str = PVMF_MIME_G726;
            break;
        case PVMF_WMA:
            str = PVMF_MIME_WMA;
            break;
        case PVMF_ASF_AMR:
            str = PVMF_MIME_ASF_AMR;
            break;
        case PVMF_REAL_AUDIO:
            str = PVMF_MIME_REAL_AUDIO;
            break;
        case PVMF_ASF_MPEG4_AUDIO:
            str = PVMF_MIME_ASF_MPEG4_AUDIO;
            break;

            //compressed video...
        case PVMF_M4V:
            str = PVMF_MIME_M4V;
            break;
            //case PVMF_H263:str=PVMF_MIME_H2631998;break; //arbitrarily prefer PVMF_MIME_H2632000
        case PVMF_H263:
            str = PVMF_MIME_H2632000;
            break;
        case PVMF_H264_RAW:
            str = PVMF_MIME_H264_VIDEO_RAW;
            break;
        case PVMF_H264_MP4:
            str = PVMF_MIME_H264_VIDEO_MP4;
            break;
        case PVMF_H264:
            str = PVMF_MIME_H264_VIDEO;
            break;
        case PVMF_WMV:
            str = PVMF_MIME_WMV;
            break;
        case PVMF_RV:
            str = PVMF_MIME_REAL_VIDEO;
            break;

            //image...
        case PVMF_M4V_IMAGE:
            str = PVMF_MIME_M4V_IMAGE;
            break;

            //multiplexed, stream or file formats...
        case PVMF_MPEG4FF:
            str = PVMF_MIME_MPEG4FF;
            break;
        case PVMF_H223:
            str = PVMF_MIME_H223;
            break;
            //case PVMF_M4V_IMAGE:str=PVMF_MIME_M4V_IMAGE;break; //already covered under "image"
        case PVMF_RTP:
            str = PVMF_MIME_RTP;
            break;
        case PVMF_AMRFF:
            str = PVMF_MIME_AMRFF;
            break;
        case PVMF_MP3FF:
            str = PVMF_MIME_MP3FF;
            break;
        case PVMF_WAVFF:
            str = PVMF_MIME_WAVFF;
            break;
        case PVMF_ASFFF:
            str = PVMF_MIME_ASFFF;
            break;
        case PVMF_RMFF:
            str = PVMF_MIME_RMFF;
            break;

            //raw data formats...
        case PVMF_8BIT_RAW:
            str = PVMF_MIME_8BIT_RAW;
            break;

            // pvmf data source formats
        case PVMF_DATA_SOURCE_RTSP_URL:
            str = PVMF_MIME_DATA_SOURCE_RTSP_URL;
            break;
        case PVMF_DATA_SOURCE_HTTP_URL:
            str = PVMF_MIME_DATA_SOURCE_HTTP_URL;
            break;
        case PVMF_DATA_SOURCE_SDP_FILE:
            str = PVMF_MIME_DATA_SOURCE_SDP_FILE;
            break;
        case PVMF_DATA_SOURCE_PVX_FILE:
            str = PVMF_MIME_DATA_SOURCE_PVX_FILE;
            break;
        case PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL:
            str = PVMF_MIME_DATA_SOURCE_MS_HTTP_STREAMING_URL;
            break;
        case PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL:
            str = PVMF_MIME_DATA_SOURCE_REAL_HTTP_CLOAKING_URL;
            break;

            // misc data formats
        case PVMF_3GPP_TIMEDTEXT:
            str = PVMF_MIME_3GPP_TIMEDTEXT;
            break;

        default:
            str = PVMF_MIME_FORMAT_UNKNOWN;
            break;
    }
}


