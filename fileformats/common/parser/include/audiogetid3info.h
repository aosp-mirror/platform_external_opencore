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

//           A U D I O   G E T    I D 3    I N F O

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/**
 *  @file audiogetid3info.h
 *  @brief This include file contains the definitions and classes needed
 *  to use and understand the ID3 library correctly.
 */


#ifndef AUDIOGETID3INFO_H_INCLUDED
#define AUDIOGETID3INFO_H_INCLUDED

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif
#ifndef PVFILE_H_INCLUDED
#include "pvfile.h"
#endif

#ifndef AUDIOMETADATA_H_INCLUDED
#include "audiometadata.h"
#endif

//----------------------------------------------------------------------------
// FORWARD CLASS DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

// Error codes returned
#define PVID3PARSER_EVERYTHING_FINE                        0
#define PVID3PARSER_FILE_OPEN_FAILED                      -1
#define PVID3PARSER_FILE_OPERATION_FAILED				  -2

#define ID3V1_TAG_NUM_BYTES_HEADER             3
#define ID3V1_MAX_NUM_BYTES_TITLE             30
#define ID3V1_MAX_NUM_BYTES_ARTIST            30
#define ID3V1_MAX_NUM_BYTES_ALBUM             30
#define ID3V1_MAX_NUM_BYTES_YEAR               4
#define ID3V1_MAX_NUM_BYTES_COMMENT           30
#define ID3V1_MAX_NUM_BYTES_GENRE              1
#define ID3V1_MAX_NUM_BYTES_TOTAL            128
#define ID3V1_MAX_NUM_BYTES_FIELD_SIZE        30

#define ID3V2_TAG_NUM_BYTES_HEADER            10
#define ID3V2_TAG_NUM_BYTES_FOOTER            10
#define ID3V2_TAG_NUM_BYTES_ID                 3
#define ID3V2_TAG_NUM_BYTES_VERSION            2
#define ID3V2_TAG_NUM_BYTES_FLAG               1
#define ID3V2_TAG_NUM_BYTES_SIZE               4

#define ID3V2_FRAME_NUM_BYTES_HEADER          10
#define ID3V2_FRAME_NUM_BYTES_ID               4
#define ID3V2_FRAME_NUM_BYTES_SIZE             4
#define ID3V2_FRAME_NUM_BYTES_FLAG             2

#define ID3V2_TAG_EXTENDED_HEADER_TOTAL_SIZE   6
#define ID3V2_TAG_EXTENDED_HEADER_SIZE         4
#define ID3V2_TAG_EXTENDED_HEADER_NUM          1
#define ID3V2_TAG_EXTENDED_HEADER_FLAG         1

#define UNICODE_LITTLE_ENDIAN                  1
#define UNICODE_BIG_ENDIAN                     0

#define FLAGMASK                              64 // 0x40
#define FOOTERMASK                            16 // Bit 4
#define MASK127                              127 // 0111 1111

#define VALID_BITS_IN_SYNC_SAFE_BYTE           7
#define UNICODE_LITTLE_ENDIAN_INDICATOR      255 // 0xff
#define UNICODE_BIG_ENDIAN_INDICATOR         254 // 0xfe

//for the ID3 Version 2.2
#define ID3V22_FRAME_NUM_BYTES_ID               3
#define ID3V22_HEADER_SIZE                      3
#define ID3V22_FRAME_NUM_BYTES_HEADER           6

enum TTextType
{
    EISO88591,
    EUTF16,
    EUTF16BE,
    EUTF8,
    ENoType
};

// Frame Header ID Types for ID3v2 frames
enum TID3V2FrameType
{
    ETIT2, // Title
    ETPE1, // Artist
    ETALB, // Album
    ETCOP, // Copyright
    ETCON, // Content Type
    ETRCK, // Track Number on Album
    ETLEN, // Length of the audiofile in milliseconds
    ETDAT, // Recording Time
    ETYER, // Year
    ETPOS, // Part Of Set
    ECOMT, // Comment
    EEND,  // Frame ID was 0's
    EFrameNotSupported
};


static const uint8 KTAGLIT[] = {"TAG"};
static const uint8 KID3LIT[] = {"ID3"};

//for the Version 2.3 and 2.4
static const uint8 KTIT2[]   = {"TIT2"}; // Title
static const uint8 KTPE1[]   = {"TPE1"}; // Artist
static const uint8 KTALB[]   = {"TALB"}; // Album
static const uint8 KTCOP[]   = {"TCOP"}; // Copyright
static const uint8 KTCON[]   = {"TCON"}; // Content Type
static const uint8 KTRCK[]   = {"TRCK"}; // Track Number on Album
static const uint8 KTLEN[]   = {"TLEN"}; // Length of the audiofile in milliseconds
static const uint8 KTYER[]   = {"TYER"}; // Year (version 2.3)
static const uint8 KTPOS[]   = {"TPOS"}; // Part Of Set
static const uint8 KTDRC[]   = {"TDRC"}; // Year (version 2.4)
static const uint8 KTDAT[]   = {"TDAT"}; // Recording Time
static const uint8 KCOMM[]   = {"COMM"}; // Comment


//for the Version 2.2
static const uint8 KTT2[]   = {"TT2"}; // Title
static const uint8 KTP1[]   = {"TP1"}; // Artist
static const uint8 KTAL[]   = {"TAL"}; // Album
static const uint8 KTCR[]   = {"TCR"}; // Copyright
static const uint8 KTCO[]   = {"TCO"}; // Content Type
static const uint8 KTRK[]   = {"TRK"}; // Track Number on Album
static const uint8 KTLE[]   = {"TLE"}; // Length of the audiofile in milliseconds
static const uint8 KTDA[]   = {"TDA"}; // Recording Time
static const uint8 KTYE[]   = {"TYE"}; // Year
static const uint8 KCOM[]   = {"COM"}; // Comment
static const uint8 KTPA[]   = {"TPA"}; // Part Of Set

//id3v1 Genre Lookup table

static const uint8 ID3V1_GENRE[150][64]   = {	  "Blues", "Classic Rock", "Country", "Dance", "Disco",
        "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age",
        "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
        "Techno", "Industrial", "Alternative", "Ska", "Death Metal",
        "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
        "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical",
        "Instrumental", "Acid", "House", "Game", "Sound Clip",
        "Gospel", "Noise", "Alternative Rock", "Bass", "Punk",
        "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
        "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
        "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock",
        "Comedy", "Cult", "Gangsta", "Top 40", "Christian Rap",
        "Pop/Funk", "Jungle", "Native US", "Cabaret", "New Wave", "Psychadelic",
        "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk",
        "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll",
        "Hard Rock", "Folk", "Folk-Rock", "National Folk", "Swing", "Fast Fusion",
        "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
        "Gothic Rock", "Progressive Rock", "Psychedelic Rock",
        "Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening",
        "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music",
        "Sonata", "Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire",
        "Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",
        "Rhytmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo", "Acapella",
        "Euro-House", "Dance Hall", "Goa", "Drum & Bass", "Club-House", "Hardcore",
        "Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk", "Beat",
        "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
        "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop", "SynthPop"

                                            }; //ID3V1 Genre

//----------------------------------------------------------------------------
// EXTERNAL VARIABLES REFERENCES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CLASS DEFINITIONS
//----------------------------------------------------------------------------

/**
 *  @brief The TID3TagInfo class acts as temporary storage for information
 *  extracted from the different types of ID3 tags to a given file. This
 *  class is created to keep track of certain qualities of an ID3 tag that
 *  the caller of the ID3 library would not need to know.
 */
class TID3TagInfo
{
    public:
        uint32      iPositionInFile;
        TTextType   iTextType;

        uint32    iID3V2FrameSize;
        uint8     iID3V2TagFlagsV2;
        uint32    iID3V2ExtendedHeader;
        uint32    iID3V2TagSize;
        uint8     iID3V2FrameID[ID3V2_FRAME_NUM_BYTES_ID];
        uint8     iID3V2FrameFlag[ID3V2_FRAME_NUM_BYTES_FLAG];
};


/**
 *  @brief The CID3TagParser class extracts the ID3 tag information from a
 *  given file. The class reads frame information from a file, determined if
 *  it is a supported tag and frame type, and extracts the frame information
 *  and fills in T-class that stores the ID3 file information.
 */
class CID3TagParser
{
    public:
        CID3TagParser();
        OSCL_IMPORT_REF ~CID3TagParser();

    public:

        /**
        * @brief Checks for the existence of an ID3V1 or ID3V2 tag
        * associated with the soecified file. Retrieves the information from the tag
        * to populate aID3MetaData.
        *
        * @param aFile Pointer to file to parse
        * @param aID3MetaData Reference to a pre-allocated TAudioMetaData structure
        * @returns Result of operation: PVID3PARSER_EVERYTHING_FINE, etc.
        */
        OSCL_IMPORT_REF int32 ParseID3TagInfo(PVFile *aFile, TAudioMetaData& aID3MetaData);

        /**
        * @brief Constructor. This constructor is called if no file server session
        * exists
        *
        * @param None
        * @returns None
        */
        OSCL_IMPORT_REF static CID3TagParser* NewL();

        /**
        * @brief Destructor for use with NewL.
        * exists
        *
        * @param None
        * @returns None
        */
        OSCL_IMPORT_REF static void Delete(CID3TagParser*);

    private:
        /**
        * @brief Constructor.
        *
        * @param None
        * @returns None
        */
        void ConstructL();

        /**

        * @brief Checks the file for the presence of ID3V1 Tag.
        *
        * @param aID3MetaData Reference to a pre-allocated TAudioMetaData structure
        * @returns True if Id3V1 tag present otherwise False
        */
        bool CheckForTagID3V1(TAudioMetaData& aID3MetaData);

        /**
        * @brief Checks the file for the presence of ID3V2 Tag attached to
        * the beginning of the file.
        *
        * @param aID3MetaData Reference to a pre-allocated TAudioMetaData structure
        * @returns True if Id3V2 tag present otherwise False
        */
        bool CheckForTagID3V2(TAudioMetaData& aID3MetaData);

        /**
        * @brief Parses the file and populates the aID3MetaData structure when
        * an ID3V1 tag is found and no ID3V2 tag was found.
        *
        * @param aID3MetaData Reference to a pre-allocated TAudioMetaData structure
        * @param aTitleOnlyFlag Indicates whether to read title only or read
        * other metadata information.
        * @returns None
        */
        void ReadID3V1Tag(TAudioMetaData& aID3MetaData, bool aTitleOnlyFlag);

        /**
        * @brief Parses the ID3 tag frames from the file and populates the
        * aID3MetaData structure when an ID3V2 tag is found
        *
        * @param aID3MetaData Reference to a pre-allocated TAudioMetaData structure
        * @returns None
        */
        void ReadHeaderID3V2(TAudioMetaData& aID3MetaData);

        /**
        * @brief Parses the frame and populates the iID3TagInfo structure
        * when an ID3V22 tag is found.
        *
        * @param aPos Position of frame in file
        * @returns None
        */
        void ReadFrameHeaderID3V22(uint32 aPos);

        /**
        * @brief Parses the frame and populates the iID3TagInfo structure
        * when an ID3V2 tag is found.
        *
        * @param aPos Position of frame in file
        * @returns None
        */
        void ReadFrameHeaderID3V2(uint32 aPos);

        /**
        * @brief Receives a ID3V2 frame and handles it accordingly. All of the frames
        * sent to this function have ASCII strings in them.
        *
        * @param aFrameType Enumerated TFrameTypeID3V2 type; used to determine if
        * Frame is supported
        * @param aPos Position in file where frame data begins
        * @param aSize Size of the frame data
        * @param aID3MetaData Reference to repository for parsed data
        * @returns None
        */
        void HandleID3V2FrameDataASCII(TID3V2FrameType      aframeType,
                                       uint32               aPos,
                                       uint32               aSize,
                                       TAudioMetaData&      aID3MetaData);

        /**
        * @brief Receives a ID3V2 frame and handles it accordingly. All of the frames
        * sent to this function have unicode strings in them.
        *
        * @param aFrameType Enumerated TFrameTypeID3V2 type; used to determine if
        * Frame is supported
        * @param aPos Position in file where frame data begins
        * @param aSize Size of the frame data
        * @param aEndianType Flag indicator regarding the text endian type
        * @param aID3MetaData Reference to repository for parsed data
        * @returns None
        */
        void HandleID3V2FrameDataUnicode16(TID3V2FrameType      aframeType,
                                           uint32               aPos,
                                           uint32               aSize,
                                           uint32               aEndianType,
                                           TAudioMetaData&      aID3MetaData);

        /**
        * @brief Receives a ID3V2 frame and handles it accordingly. All of the frames
        * sent to this function have unicode strings in them.
        *
        * @param aFrameType Enumerated TFrameTypeID3V2 type; used to determine if
        * Frame is supported
        * @param aPos Position in file where frame data begins
        * @param aSize Size of the frame data
        * @param aTextType Flag indicator regarding the text type
        * @param aID3MetaData Reference to repository for parsed data
        * @returns None
        */
        void HandleID3V2FrameDataUTF16(TID3V2FrameType      aFrameType,
                                       uint32               aPos,
                                       uint32               aSize,
                                       TTextType            aTextType,
                                       TAudioMetaData&      aID3MetaData);

        /**
        * @brief Receives a ID3V2 frame and handles it accordingly. All of the frames
        * sent to this function have UTF8 strings in them.
        *
        * @param aFrameType Enumerated TFrameTypeID3V2 type; used to determine if
        * Frame is supported
        * @param aPos Position in file where frame data begins
        * @param aSize Size of the frame data
        * @param aID3MetaData Pointer to repository for parsed data
        * @returns None
        */
        void HandleID3V2FrameDataUTF8(TID3V2FrameType      aframeType,
                                      uint32               aPos,
                                      uint32               aSize,
                                      TAudioMetaData&      aID3MetaData);

        /**
        * @brief Detects the ID3V2FrameType and returns the enum value that
        * corresponds to the current frame.
        *
        * @param None
        * @returns Value that describes the current frame of type TID3V2FrameType
        */
        TID3V2FrameType FrameSupportedID3V2(void);

        /**
        * @brief Detects the ID3V2FrameType and returns the enum value that
        * corresponds to the current frame.
        *
        * @param None
        * @returns Value that describes the current frame of type TID3V2FrameType
        */
        TID3V2FrameType FrameSupportedID3V22(void);


        /**
        * @brief Converts the data in the uint8* buffer and puts the data into the
        * oscl_wchar* buffer
        *
        * @param aPtrFrameData Pointer to intput string format of either big
        * or little endian.
        * @param aSize Number of character elements in aPtrFrameData
        * @param aEndianType Indicates if the encoded Unicode text in the
        * aPtrFrameData buffer is in big or little endian.
        * @param aPtrFrameData16 Pointer to the output string in unicode format.
        * @returns None
        */
        void EightBitToWideCharBufferTransfer(
            const uint8 *   aPtrFrameData,
            uint32         aSize,
            uint32         aEndianType,
            oscl_wchar *      aPtrFrameData16);


    private:
        bool iTitleFoundFlag;
        TID3TagInfo iID3TagInfo;
        PVFile* iFilePtr;
};

#endif  // AUDIOMP3GETID3INFO_H_INCLUDED

