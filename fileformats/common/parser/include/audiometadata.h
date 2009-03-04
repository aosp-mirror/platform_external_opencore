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
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//                  A U D I O   M E T A   D A T A

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/**
 *  @file audiometadata.h
 *  @brief This include file contains the class definition that handles
 *  the storage of meta data for an audio file.
 */


#ifndef AUDIOMETADATA_H_INCLUDED
#define AUDIOMETADATA_H_INCLUDED

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

const uint32 KMaxFileNameMp3 = 0x100;

//----------------------------------------------------------------------------
// FORWARD CLASS DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_UNICODE_BUF_LEN_TITLE     KMaxFileNameMp3
#define MAX_UNICODE_BUF_LEN_ARTIST       256
#define MAX_UNICODE_BUF_LEN_ALBUM        256
#define MAX_UNICODE_BUF_LEN_SET           32
#define MAX_UNICODE_BUF_LEN_YEAR          32
#define MAX_UNICODE_BUF_LEN_COMMENT      256
#define MAX_UNICODE_BUF_LEN_COPYRIGHT    256
#define MAX_UNICODE_BUF_LEN_GENRE        256
#define MAX_UNICODE_BUF_LEN_TRACK_NUMBER   4
#define MAX_UNICODE_BUF_LEN_TRACK_LENGTH   8
#define MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE 4
#define MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION 256


// ID3 Version Types for MP3 files
enum TTagType
{
    ENoTag,
    EID3V1,
    EID3V2,
};

//----------------------------------------------------------------------------
// EXTERNAL VARIABLES REFERENCES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CLASS DEFINITIONS
//----------------------------------------------------------------------------

/**
 *  @brief The TAudioMetaData class encapsulates information about any audio
 *  clip found embedded in the file. The information is meta data to the audio
 *  file in that it is not part of the audio frames, but is extra information
 *  such as: artist, title, album, etc...
 */
class TAudioMetaData
{
    public:

        enum FormatType
        {
            EISO88591_CHAR,
            EUTF16_WCHAR,
            EUTF16BE_WCHAR,
            EUTF8_CHAR,
            EBYTEVALUE_UINT8,
            FORMAT_INVALID
        };

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_TITLE];
                char pChar[MAX_UNICODE_BUF_LEN_TITLE];
            };
            FormatType iFormat;
        } iTitle;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_ARTIST];
                char pChar[MAX_UNICODE_BUF_LEN_ARTIST];
            };
            FormatType iFormat;
        } iArtist;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_ALBUM];
                char pChar[MAX_UNICODE_BUF_LEN_ALBUM];
            };
            FormatType iFormat;
        } iAlbum;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_SET];
                char pChar[MAX_UNICODE_BUF_LEN_SET];
            };
            FormatType iFormat;
        } iSet;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_YEAR];
                char pChar[MAX_UNICODE_BUF_LEN_YEAR];
            };
            FormatType iFormat;
        } iYear;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_COMMENT];
                char pChar[MAX_UNICODE_BUF_LEN_COMMENT];
            };
            FormatType iFormat;

            union
            {
                oscl_wchar langcode_pWChar[MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE];
                char langcode_pChar[MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE];
            };
            union
            {
                oscl_wchar description_pWChar[MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION];
                char description_pChar[MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION];
            };
        } iComment;


        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_COPYRIGHT];
                char pChar[MAX_UNICODE_BUF_LEN_COPYRIGHT];
            };
            FormatType iFormat;
        } iCopyright;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_GENRE];
                char pChar[MAX_UNICODE_BUF_LEN_GENRE];
                uint8 uint8_value;
            };
            FormatType iFormat;
        } iGenre;

        struct
        {
            union
            {
                oscl_wchar pWChar[MAX_UNICODE_BUF_LEN_TRACK_NUMBER];
                char pChar[MAX_UNICODE_BUF_LEN_TRACK_NUMBER];
                uint8 uint8_value;
            };
            FormatType iFormat;
        } iTrackNumber;


        TTagType iID3VersionType;
        uint32   iMajorVersion;
        uint32   iMinorVersion;
        int64    iTrackLength;
        uint32   iFileSizeInBytes;
        uint32   iByteOffsetToStartOfAudioFrames;
};

#endif  // AUDIOMETADATA_H_INCLUDED
