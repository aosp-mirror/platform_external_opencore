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
* @file pv_id3_parcom_constants.h
* @brief Constant definitions for ID3 Parser-Composer
*/

#ifndef PV_ID3_PARCOM_CONSTANTS_H_INCLUDED
#define PV_ID3_PARCOM_CONSTANTS_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif

#define ID3V1_TAG_NUM_BYTES_HEADER             3
#define ID3V1_MAX_NUM_BYTES_TITLE             30
#define ID3V1_MAX_NUM_BYTES_ARTIST            30
#define ID3V1_MAX_NUM_BYTES_ALBUM             30
#define ID3V1_MAX_NUM_BYTES_YEAR               4
#define ID3V1_MAX_NUM_BYTES_COMMENT           30
#define ID3V1_MAX_NUM_BYTES_GENRE              1
#define ID3V1_MAX_NUM_BYTES_TOTAL            128
#define ID3V1_MAX_NUM_BYTES_FIELD_SIZE        30

#define ID3V2_NUM_BYTES_SIZE                   4
#define ID3V2_TAG_NUM_BYTES_HEADER            10
#define ID3V2_TAG_NUM_BYTES_ID                 3
#define ID3V2_TAG_NUM_BYTES_VERSION            2
#define ID3V2_TAG_NUM_BYTES_FLAG               1
#define ID3V2_TAG_NUM_BYTES_SIZE               ID3V2_NUM_BYTES_SIZE

#define ID3V2_FRAME_NUM_BYTES_HEADER          10

#define ID3V2_2_FRAME_NUM_BYTES_HEADER		   6
#define ID3V2_FRAME_NUM_BYTES_ID               4

#define ID3V2_2_FRAME_NUM_BYTES_ID			   3
#define ID3V2_FRAME_NUM_BYTES_SIZE             ID3V2_NUM_BYTES_SIZE

#define ID3V2_2_FRAME_NUM_BYTES_SIZE		   ID3V2_2_NUM_BYTES_SIZE
#define ID3V2_FRAME_NUM_BYTES_FLAG             2

#define ID3V2_TAG_EXTENDED_HEADER_TOTAL_SIZE   6
#define ID3V2_TAG_EXTENDED_HEADER_SIZE         ID3V2_NUM_BYTES_SIZE
#define ID3V2_TAG_EXTENDED_HEADER_NUM          1
#define ID3V2_TAG_EXTENDED_HEADER_FLAG         1
#define ID3V2_COMMENT_LANGUAGE_SIZE			   4

//text encoding(1)+  picture type(1)  = 2
#define ID3V2_APIC_CONST_SIZE				   2
//text encoding(1)+image format(3) + picture type(1)  = 5
#define ID3V2_PIC_CONST_SIZE		            5
#define ID3V2_IMAGE_FORMAT_SIZE				    3
#define	ID3V2_LANGUAGE_SIZE						3
#define UNICODE_LITTLE_ENDIAN                   1
#define UNICODE_BIG_ENDIAN                      0
#define UNICODE_BOM_SIZE						2
#define VERSION_SIZE							2

//Max length for null terminated strings in text frames
#define ID3_MAX_STRING_FRAME_LEN				256

#define	ID3V2_FLAGS							0x00
#define EXT_FLAGMASK                         64 // 0x40
#define FTR_FLAGMASK                         16 // 0x10
#define MASK127                              127 // 0111 1111
#define EXTHDR_UPDMASK						 64 // 0x40
#define ENCR_COMP_3_FLAGMASK					192//0xC0
#define ENCR_COMP_4_FLAGMASK                    12 //0x0C

#define VALID_BITS_IN_SYNC_SAFE_BYTE           7
#define UNICODE_LITTLE_ENDIAN_INDICATOR       255 // 0xff
#define UNICODE_BIG_ENDIAN_INDICATOR          254 // 0xfe

#define UNICODE_LITTLE_ENDIAN_INDICATOR_FULL   0xfffe
#define UNICODE_BIG_ENDIAN_INDICATOR_FULL	   0xfeff

#define CRC_ON	0
#if CRC_ON
#define CRC_BYTES					5
#else
#define CRC_BYTES					0
#endif

#define TAG_RESTRICTION_ON	0
#if TAG_RESTRICTION_ON
#define TAG_RESTRICTION_BYTES		1
#else
#define TAG_RESTRICTION_BYTES		0
#endif

#define EXT_HEADER_FLAG_BYTES						(1 + CRC_BYTES + TAG_RESTRICTION_BYTES)

#define EXT_FLAG_TAG_UPDATE							0x40
#define EXT_FLAG_CRC_DATA							0x20
#define EXT_FLAG_TAG_RESTRICTION					0x10

#define EXT_FLAG_TAG_SIZE_RESTRICTION				0x00
#define EXT_FLAG_TEXT_ENCODING_RESTRICTION			0x00
#define EXT_FLAG_TEXT_FEILD_SIZE_RESTRICTION		0x00
#define EXT_FLAG_IMAGE_ENCODING_RESTRICTION			0x00
#define IMAGE_SIZE_RESTRICTION						0x00

//frame status flags
#define FRAME_TAG_ALTER_PRESERVATION				0x00
#define FRAME_FILE_ALTER_PRESERVATION				0x00
#define FRAME_READ_ONLY								0x00

//frame format description
#define FRAME_GROUP_IDENTITY						0x00
#define FRAME_COMPRESSION							0x00
#define FRAME_ENCRYPTION							0x00
#define FRAME_UNSYNCHRONIZATION						0x00
#define FRAME_DATA_LENGTH_INDICATOR					0x00

#define BYTE_FOR_CHARACTERSET_FORMAT				1

//frame IDs
#define ID3_FRAME_ID_TITLE			"TIT2"
#define ID3_FRAME_ID_ARTIST			"TPE1"
#define ID3_FRAME_ID_ALBUM			"TALB"
#define ID3_FRAME_ID_TRACK_NUMBER	"TRCK"
#define ID3_FRAME_ID_PART_OF_SET    "TPOS"
#define ID3_FRAME_ID_GENRE			"TCON"
#define ID3_FRAME_ID_YEAR			"TYER"
#define ID3_FRAME_ID_COPYRIGHT		"TCOP"
#define ID3_FRAME_ID_COMMENT		"COMM"
#define ID3_FRAME_ID_TRACK_LENGTH	"TLEN"
#define ID3_FRAME_ID_DATE			"TDAT"
#define ID3_FRAME_ID_ALBUMART		"APIC"
#define ID3_FRAME_ID_LYRICIST		"USLT"
#define ID3_FRAME_ID_SEEK			"SEEK"
#define ID3_FRAME_ID_RECORDING_TIME	"TDRC"
#define ID3_FRAME_ID_AUTHOR			"TEXT"
#define ID3_FRAME_ID_COMPOSER		"TCOM"
#define ID3_FRAME_ID_DESCRIPTION	"TIT3"
#define ID3_FRAME_ID_VERSION		"TENC"


#define ID3_V1_IDENTIFIER			"TAG"
#define ID3_V2_IDENTIFIER			"ID3"
#define ID3_V2_4_SEEK_FRAME			"SEEK"
#define ID3_V2_4_TAG_FOOTER			"3DI"

//ID3V2_2 frame IDs
#define ID3V2_2_FRAME_ID_TITLE			"TT2"
#define ID3V2_2_FRAME_ID_ARTIST			"TP1"
#define ID3V2_2_FRAME_ID_ALBUM			"TAL"
#define ID3V2_2_FRAME_ID_TRACK_NUMBER	"TRK"
#define ID3V2_2_FRAME_ID_PART_OF_SET    "TPA"
#define ID3V2_2_FRAME_ID_GENRE			"TCO"
#define ID3V2_2_FRAME_ID_YEAR			"TYE"
#define ID3V2_2_FRAME_ID_COPYRIGHT		"TCR"
#define ID3V2_2_FRAME_ID_COMMENT		"COM"
#define ID3V2_2_FRAME_ID_TRACK_LENGTH	"TLE"
#define ID3V2_2_FRAME_ID_DATE			"TDA"
#define ID3V2_2_FRAME_ID_ALBUMART		"PIC"
#define ID3V2_2_FRAME_ID_LYRICIST		"ULT"
#define ID3V2_2_FRAME_ID_AUTHOR			"TXT"
#define ID3V2_2_FRAME_ID_COMPOSER		"TCM"
#define ID3V2_2_FRAME_ID_DESCRIPTION	"TT3"
#define ID3V2_2_FRAME_ID_VERSION		"TEN"


#define SIZE_OF_TRACK_LENGTH 8

// ID3 meta data keys
#define KVP_KEY_TITLE "title"
#define KVP_KEY_TITLE_LEN 5
#define KVP_KEY_ARTIST "artist"
#define KVP_KEY_ARTIST_LEN 6
#define KVP_KEY_PART_OF_SET "set"
#define KVP_KEY_SET_PART_OF_SET_LEN 3
#define KVP_KEY_ALBUM "album"
#define KVP_KEY_ALBUM_LEN 5
#define KVP_KEY_YEAR "year"
#define KVP_KEY_DATE "date"
#define KVP_KEY_YEAR_LEN 4
#define KVP_KEY_COMMENT "comment"
#define KVP_KEY_COMMENT_LEN 7
#define KVP_KEY_TRACK_NUMBER "tracknumber"
#define KVP_KEY_TRACK_NUMBER_LEN 11
#define KVP_KEY_GENRE "genre"
#define KVP_KEY_GENRE_LEN 5
#define KVP_KEY_COPYRIGHT "copyright"
#define KVP_KEY_COPYRIGHT_LEN 9
#define KVP_KEY_ID3DURATION "duration-from-metadata"
#define KVP_KEY_ID3DURATION_LEN 20
#define KVP_KEY_RECORDING_TIME	"year"
#define KVP_KEY_AUTHOR		"author"
#define KVP_KEY_COMPOSER	"composer"
#define KVP_KEY_DESCRIPTION	"description"
#define KVP_KEY_VERSION		"version"

#define KVP_KEY_LYRICS "lyrics"
#define KVP_KEY_LYRICS_LEN 6
#define KVP_KEY_ALBUMART "graphic"
#define KVP_KEY_ALBUMART_LEN 8

#define SEMI_COLON _STRLIT_CHAR(";")
#define NULL_CHARACTER _STRLIT_CHAR("\0")
#define FORWARD_SLASH _STRLIT_CHAR("/")

// ID3 meta data value types
#define KVP_VALTYPE_ISO88591_CHAR "valtype=char*"
#define KVP_VALTYPE_ISO88591_CHAR_LEN 13
#define KVP_VALTYPE_UTF8_CHAR "valtype=char*;char-encoding=UTF8"
#define KVP_VALTYPE_UTF8_CHAR_LEN 32
#define KVP_VALTYPE_UTF16_WCHAR "valtype=wchar*"
#define KVP_VALTYPE_UTF16_WCHAR_LEN 14
#define KVP_VALTYPE_UTF16BE_WCHAR "valtype=wchar*;char-encoding=UTF16BE"
#define KVP_VALTYPE_UTF16BE_WCHAR_LEN 36
#define KVP_VALTYPE_BYTEVALUE_UINT8 "valtype=uint8;format=id3v1"
#define KVP_VALTYPE_BYTEVALUE_UINT8_LEN 26
#define KVP_VALTYPE_UINT32 "valtype=uint32"
#define KVP_VALTYPE_UINT32_LEN 14
#define KVP_VALTYPE_UINT8PTR_STRING "valtype=uint8*"
#define KVP_VALTYPE_UINT8PTR_STRING_LEN 14
#define KVP_PARAM_LANGUAGE_CODE "iso-639-2-lang="
#define KVP_PARAM_LANGUAGE_CODE_LEN 15
#define KVP_VALTYPE_DURATION "timescale=1000;valtype=uint32"
#define KVP_PARAM_CHAR_ENCODING_UTF16BE "char-encoding=UTF16BE"
#define KVP_PARAM_CHAR_ENCODING_UTF8 "char-encoding=UTF8"
#define KVP_FORMAT_ALBUMART  "format=APIC"
#define KVP_FORMAT_ID3V1	"format=id3v1"
#define KVP_VALTYPE_ALBUMART "valtype=ksv"
#define KVP_ID3V2_VALUE "id3v2"
#define KVP_VALTYPE_LYRICS   "valtype=ksv"
#define KVP_VALTYPE_REQSIZE  "reqsize="

static const int  MAX_RANGE_INT_SIZE  = 10;

#define PERIOD_CHARACTER_ASCII_VALUE 46
#define COMMA_CHARACTER_ASCII_VALUE 44
#define ZERO_CHARACTER_ASCII_VALUE 48
#define NINE_CHARACTER_ASCII_VALUE 57

#endif // PV_ID3_PARCOM_CONSTANTS_H_INCLUDED


