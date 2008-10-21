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
/**
 *  @file pvmf_format_type.h
 *  @brief This file defines known format types and MIME strings,
 *    and some utilities for converting between them.
 *
 */

#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#define PVMF_FORMAT_TYPE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

#ifdef __cplusplus
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif

/** PvmfMimeString is used in several PVMF APIs */
typedef OSCL_String PvmfMimeString;
#endif

/** PvmfMediaTypeIndex refers to one of the high-level format types*/
typedef uint32 PvmfMediaTypeIndex;

/** PvmfFormatIndex refers to one of the recognized formats*/
typedef uint32 PvmfFormatIndex;

/** PVMFFormatType maps to PvmfFormatIndex*/
typedef PvmfFormatIndex PVMFFormatType;


#define PVMF_FORMAT_UNKNOWN 0x00000000

// MIME string for unknown format
#define PVMF_MIME_FORMAT_UNKNOWN	"FORMATUNKNOWN"

/** The high level format types **/
#define PVMF_UNCOMPRESSED_AUDIO_FORMAT 0x1
#define PVMF_UNCOMPRESSED_VIDEO_FORMAT 0x2
#define PVMF_COMPRESSED_AUDIO_FORMAT 0x4
#define PVMF_COMPRESSED_VIDEO_FORMAT 0x8
#define PVMF_IMAGE_FORMAT 0x10
#define PVMF_MULTIPLEXED_FORMAT 0x20
#define PVMF_DATASTREAM_FORMAT 0x40
#define PVMF_USERINPUT_FORMAT 0x80
#define PVMF_DATA_SOURCE_FORMAT 0x100
#define PVMF_TEXT_FORMAT 0x200

/**
** Note on format MIME strings:
** The MIME strings that start with "X" were made up by PV.
** The others came from:
** http://www.iana.org/assignments/media-types/index.html.
** The names were chosen to match the names of the PV format
** index values.
**/

/* Following are the recognized formats  */

// Uncompressed audio formats
#define PVMF_FIRST_UNCOMPRESSED_AUDIO	1
#define PVMF_PCM						PVMF_FIRST_UNCOMPRESSED_AUDIO  /* this refers to general PCM support */
#define PVMF_PCM8						PVMF_FIRST_UNCOMPRESSED_AUDIO+1
#define PVMF_PCM16						PVMF_FIRST_UNCOMPRESSED_AUDIO+2
#define PVMF_PCM16_BE					PVMF_FIRST_UNCOMPRESSED_AUDIO+3
#define PVMF_PCM_ULAW					PVMF_FIRST_UNCOMPRESSED_AUDIO+4
#define PVMF_PCM_ALAW					PVMF_FIRST_UNCOMPRESSED_AUDIO+5
//When adding types to this range, please update the following
//value to equal the last defined format.
#define PVMF_LAST_UNCOMPRESSED_AUDIO	PVMF_FIRST_UNCOMPRESSED_AUDIO+5

// MIME strings for uncompressed audio formats
#define PVMF_MIME_PCM   	"X-PCM-GEN"
#define PVMF_MIME_PCM8		"audio/L8"
#define PVMF_MIME_PCM16		"audio/L16"
#define PVMF_MIME_PCM16_BE	"X-PCM16-BE"
#define PVMF_MIME_ULAW		"audio/PCMU"
#define PVMF_MIME_ALAW		"audio/PCMA"


// Uncompressed video formats
#define PVMF_FIRST_UNCOMPRESSED_VIDEO	200
#define PVMF_YUV420						PVMF_FIRST_UNCOMPRESSED_VIDEO
#define PVMF_YUV422						PVMF_FIRST_UNCOMPRESSED_VIDEO+1
#define PVMF_RGB8						PVMF_FIRST_UNCOMPRESSED_VIDEO+2
#define PVMF_RGB12						PVMF_FIRST_UNCOMPRESSED_VIDEO+3
#define PVMF_RGB16						PVMF_FIRST_UNCOMPRESSED_VIDEO+4
#define PVMF_RGB24						PVMF_FIRST_UNCOMPRESSED_VIDEO+5
#define PVMF_YUV420_PLANAR				PVMF_FIRST_UNCOMPRESSED_VIDEO+6
#define PVMF_YUV420_PACKEDPLANAR		PVMF_FIRST_UNCOMPRESSED_VIDEO+7
#define PVMF_YUV420_SEMIPLANAR			PVMF_FIRST_UNCOMPRESSED_VIDEO+8
#define PVMF_YUV420_PACKEDSEMIPLANAR	PVMF_FIRST_UNCOMPRESSED_VIDEO+9
#define PVMF_YUV420_SEMIPLANAR_YVU	    PVMF_FIRST_UNCOMPRESSED_VIDEO+10
#define PVMF_YUV422_PLANAR				PVMF_FIRST_UNCOMPRESSED_VIDEO+11
#define PVMF_YUV422_PACKEDPLANAR		PVMF_FIRST_UNCOMPRESSED_VIDEO+12
#define PVMF_YUV422_SEMIPLANAR			PVMF_FIRST_UNCOMPRESSED_VIDEO+13
#define PVMF_YUV422_PACKEDSEMIPLANAR	PVMF_FIRST_UNCOMPRESSED_VIDEO+14
//When adding types to this range, please update the following
//value to equal the last defined format.
#define PVMF_LAST_UNCOMPRESSED_VIDEO	PVMF_FIRST_UNCOMPRESSED_VIDEO+14


// MIME strings for uncompressed video formats
#define PVMF_MIME_YUV420	"X-YUV-420"
#define PVMF_MIME_YUV422	"X-YUV-422"
#define PVMF_MIME_RGB8		"X-RGB-8"
#define PVMF_MIME_RGB12		"X-RGB-12"
#define PVMF_MIME_RGB16		"X-RGB-16"
#define PVMF_MIME_RGB24		"X-RGB-24"

// Compressed audio formats
#define PVMF_FIRST_COMPRESSED_AUDIO		400
#define PVMF_AMR_IETF					PVMF_FIRST_COMPRESSED_AUDIO
#define PVMF_AMR_IF2					PVMF_FIRST_COMPRESSED_AUDIO+1
#define PVMF_EVRC						PVMF_FIRST_COMPRESSED_AUDIO+2
#define PVMF_G726						PVMF_FIRST_COMPRESSED_AUDIO+3
#define PVMF_G723						PVMF_FIRST_COMPRESSED_AUDIO+4
#define PVMF_MP3						PVMF_FIRST_COMPRESSED_AUDIO+5
#define PVMF_ADIF						PVMF_FIRST_COMPRESSED_AUDIO+6
#define PVMF_ADTS						PVMF_FIRST_COMPRESSED_AUDIO+7
#define PVMF_LATM						PVMF_FIRST_COMPRESSED_AUDIO+8
#define PVMF_MPEG4_AUDIO				PVMF_FIRST_COMPRESSED_AUDIO+9
#define PVMF_AMR_ETS                    PVMF_FIRST_COMPRESSED_AUDIO+10
#define PVMF_AMR_IETF_COMBINED          PVMF_FIRST_COMPRESSED_AUDIO+11
#define PVMF_AMRWB_IETF_PAYLOAD			PVMF_FIRST_COMPRESSED_AUDIO+12
#define PVMF_WMA                        PVMF_FIRST_COMPRESSED_AUDIO+13
#define PVMF_ASF_AMR                    PVMF_FIRST_COMPRESSED_AUDIO+14
#define PVMF_REAL_AUDIO                 PVMF_FIRST_COMPRESSED_AUDIO+15
#define PVMF_AMRWB_IETF					PVMF_FIRST_COMPRESSED_AUDIO+16
#define PVMF_ASF_MPEG4_AUDIO			PVMF_FIRST_COMPRESSED_AUDIO+17
#define PVMF_AAC_SIZEHDR				PVMF_FIRST_COMPRESSED_AUDIO+18
//When adding types to this range, please update the following
//value to equal the last defined format.
#define PVMF_LAST_COMPRESSED_AUDIO		PVMF_FIRST_COMPRESSED_AUDIO+18

// MIME strings for compressed audio formats
#define PVMF_MIME_AMR		 "audio/AMR" // Streaming AMR format, aka IETF_COMBINED_TOC
#define PVMF_MIME_AMRWB		 "audio/AMR-WB" // AMR Wide Band
#define PVMF_MIME_AMR_IETF	 "X-AMR-IETF-SEPARATE" // Today's IETF
#define PVMF_MIME_AMRWB_IETF "X-AMRWB-IETF-SEPARATE" // Today's IETF
#define PVMF_MIME_AMR_IF2	 "X-AMR-IF2"
#define PVMF_MIME_EVRC		 "audio/EVRC" // Streaming EVRC format
#define PVMF_MIME_MP3		 "audio/MPEG"
#define PVMF_MIME_ADIF		 "X-AAC-ADIF" //.aac file format
#define PVMF_MIME_ADTS		 "X-AAC-ADTS" //.aac file format
#define PVMF_MIME_LATM		 "audio/MP4A-LATM" // Streaming AAC format
#define PVMF_MIME_MPEG4_AUDIO "X-MPEG4-AUDIO"// MPEG4 Audio (AAC) as stored in MPEG4 File
#define PVMF_MIME_G723          "audio/G723"
#define PVMF_MIME_G726       "x-pvmf/audio/g726"
//WMA Audio
#define PVMF_MIME_WMA        "audio/x-ms-wma"
// AMR Audio from a asf file
#define PVMF_MIME_ASF_AMR   "x-pvmf/audio/asf-amr"
// real audio
#define PVMF_MIME_REAL_AUDIO "audio/vnd.rn-realaudio"
// MPEG4 Audio from a asf file
#define PVMF_MIME_ASF_MPEG4_AUDIO   "x-pvmf/audio/asf-mpeg4-audio"
#define PVMF_MIME_3640		 "audio/mpeg4-generic" // Streaming AAC format

// Compressed video formats
#define PVMF_FIRST_COMPRESSED_VIDEO		600
#define PVMF_M4V						PVMF_FIRST_COMPRESSED_VIDEO
#define PVMF_H263						PVMF_FIRST_COMPRESSED_VIDEO+1
//Raw 264 files (.264 - stored as per byte stream format)
#define PVMF_H264_RAW					PVMF_FIRST_COMPRESSED_VIDEO+2
//H264 tracks as stored in MP4/3GP files
#define PVMF_H264_MP4                   PVMF_FIRST_COMPRESSED_VIDEO+3
//H263 streamed as per RFC 3984
#define PVMF_H264                       PVMF_FIRST_COMPRESSED_VIDEO+4
// WMV7, WMV8, WMV9
#define PVMF_WMV                        PVMF_FIRST_COMPRESSED_VIDEO+5
// RV8, RV9
#define PVMF_RV							PVMF_FIRST_COMPRESSED_VIDEO+6
//When adding types to this range, please update the following
//value to equal the last defined format.
#define PVMF_LAST_COMPRESSED_VIDEO		PVMF_FIRST_COMPRESSED_VIDEO+6

// MIME strings for Compressed video formats
#define PVMF_MIME_M4V		"video/MP4V-ES"// MPEG4 Video
#define PVMF_MIME_H2631998	"video/H263-1998"
#define PVMF_MIME_H2632000	"video/H263-2000"
//Raw 264 files (.264 - stored as per byte stream format)
#define PVMF_MIME_H264_VIDEO_RAW "X-H264-BYTE-STREAM"
//H264 tracks as stored in MP4/3GP files
#define PVMF_MIME_H264_VIDEO_MP4  "X-H264-VIDEO"
//H263 streamed as per RFC 3984
#define PVMF_MIME_H264_VIDEO "video/H264"
// WMV7, WMV8, WMV9
#define PVMF_MIME_WMV         "video/x-ms-wmv"
// RV8, RV0
#define PVMF_MIME_REAL_VIDEO		  "video/vnd.rn-realvideo"

// Still image formats
#define PVMF_FIRST_IMAGE				800
#define PVMF_M4V_IMAGE					PVMF_FIRST_IMAGE
#define PVMF_H263_IMAGE					PVMF_FIRST_IMAGE+1
#define PVMF_LAST_IMAGE					999

// MIME strings for still image formats
#define PVMF_MIME_M4V_IMAGE	"X-M4V-IMAGE" // PV Proprietary Still Image

// Multiplexed formats
#define PVMF_FIRST_MULTIPLEXED			1000
#define PVMF_MPEG4FF					PVMF_FIRST_MULTIPLEXED
#define PVMF_H223						PVMF_FIRST_MULTIPLEXED+1
#define PVMF_RTP						PVMF_FIRST_MULTIPLEXED+2
#define PVMF_AMRFF						PVMF_FIRST_MULTIPLEXED+3
#define PVMF_AACFF						PVMF_FIRST_MULTIPLEXED+4
#define PVMF_MP3FF						PVMF_FIRST_MULTIPLEXED+5
#define PVMF_WAVFF						PVMF_FIRST_MULTIPLEXED+6
#define PVMF_ASFFF                      PVMF_FIRST_MULTIPLEXED+7
#define PVMF_RMFF						PVMF_FIRST_MULTIPLEXED+8
#define	PVMF_MIDIFF						PVMF_FIRST_MULTIPLEXED+9
#define	PVMF_AVIFF						PVMF_FIRST_MULTIPLEXED+10
#define PVMF_LAST_MULTIPLEXED			1199

// MIME strings for Multiplexed formats and single-media file formats.
#define PVMF_MIME_MPEG4FF		"video/MP4"
#define PVMF_MIME_H223			"X-H223"
#define PVMF_MIME_RTP			"X-RTP"
#define PVMF_MIME_AMRFF			"X-AMR-FF"
#define PVMF_MIME_AACFF			"X-AAC-FF"
#define PVMF_MIME_MP3FF			"X-MP3-FF"
#define PVMF_MIME_WAVFF			"X-WAV-FF"
#define PVMF_MIME_ASFFF			"x-pvmf/mux/asf"
#define PVMF_MIME_RMFF			"x-pvmf/mux/rm"

// RAW data formats
#define PVMF_FIRST_DATASTREAM           1200
#define PVMF_8BIT_RAW                   1201
#define PVMF_LAST_DATASTREAM            1299

// MIME strings for RAW data formats
#define PVMF_MIME_8BIT_RAW   "X-RAW-8"

// reserved for 2way
#define PVMF_FIRST_USERINPUT_2WAY       1300
#define PVMF_USERINPUT_BASIC_STRING     PVMF_FIRST_USERINPUT_2WAY
#define PVMF_USERINPUT_IA5_STRING       PVMF_FIRST_USERINPUT_2WAY+1
#define PVMF_USERINPUT_GENERAL_STRING   PVMF_FIRST_USERINPUT_2WAY+2
#define PVMF_USERINPUT_DTMF             PVMF_FIRST_USERINPUT_2WAY+3
#define PVMF_LAST_USERINPUT_2WAY        1309

#define PVMF_INET_UDP					1400
#define PVMF_INET_TCP					1401
#define PVMF_RTSP						1402
#define PVMF_RTCP                       1403

// PVMF data source types
#define PVMF_FIRST_DATA_SOURCETYPE              1500
#define PVMF_DATA_SOURCE_RTSP_URL               1501
#define PVMF_DATA_SOURCE_HTTP_URL               1502
#define PVMF_DATA_SOURCE_SDP_FILE               1503
#define PVMF_DATA_SOURCE_PVX_FILE               1504
#define PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL  1505
#define PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL 1506
#define PVMF_DATA_SOURCE_RTP_PACKET_SOURCE      1507
#define PVMF_DATA_SOURCE_UNKNOWN_URL            1508
#define PVMF_LAST_DATA_SOURCETYPE               1599

// MIME strings for PVMF data source types
#define PVMF_MIME_DATA_SOURCE_RTSP_URL  "X-PVMF-DATA-SRC-RTSP-URL"
#define PVMF_MIME_DATA_SOURCE_HTTP_URL  "X-PVMF-DATA-SRC-HTTP-URL"
#define PVMF_MIME_DATA_SOURCE_SDP_FILE  "X-PVMF-DATA-SRC-SDP-FILE"
#define PVMF_MIME_DATA_SOURCE_PVX_FILE  "X-PVMF-DATA-SRC-PVX-FILE"
#define PVMF_MIME_DATA_SOURCE_MS_HTTP_STREAMING_URL "X-PVMF-DATA-SRC-MS-HTTP-STREAMING-URL"
#define PVMF_MIME_DATA_SOURCE_REAL_HTTP_CLOAKING_URL "X-PVMF-DATA-SRC-REAL-HTTP-CLOAKING-URL"
#define PVMF_MIME_DATA_SOURCE_UNKNOWN_URL "X-PVMF-DATA-SRC-UNKNOWN-URL"
#define PVMF_MIME_DATA_SOURCE_RTP_PACKET_SOURCE "X-PVMF-DATA-SRC-RTP-PACKET"

// Text media data format
#define PVMF_FIRST_TEXT					1600
#define PVMF_3GPP_TIMEDTEXT				PVMF_FIRST_TEXT+1
#define PVMF_LAST_TEXT					1799

// MIME string for miscellaneous media data formats
#define PVMF_MIME_3GPP_TIMEDTEXT		"video/3gpp-tt"

#ifdef __cplusplus
/* This routine returns the unique index of the format specified in the mime string,
** or PVMF_UNKNOWN_FORMAT if not recognized.
** aHint is an optional media type index to narrow the search.
*/
OSCL_IMPORT_REF PvmfFormatIndex GetFormatIndex(char* mime_string, PvmfMediaTypeIndex aHint = PVMF_FORMAT_UNKNOWN);

/* Returns the high level media type index of this format type,
** or PVMF_UNKNOWN_FORMAT if not recognized
*/
OSCL_IMPORT_REF PvmfMediaTypeIndex GetMediaTypeIndex(PvmfFormatIndex aFormatIndex);

/* This routine returns the mime string corresponding to the format index,
** or an empty string if not recognized.
*/
OSCL_IMPORT_REF void GetFormatString(PvmfFormatIndex aFormatIndex, OSCL_String&str);
#endif // __cplusplus


#endif
