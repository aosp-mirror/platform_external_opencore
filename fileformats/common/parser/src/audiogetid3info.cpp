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

//           A U D I O    M P 3   G E T    I D 3    I N F O

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/**
 * @file audiogetid3info.cpp
 * @brief This include file contains the methods to obtain the required
 * ID3V1 or ID3V2 information from a specified file. The various tasks it does
 * to get the ID3 data are reading from the file, determining the ID3
 * version, checking for a valid ID3 frame type, and extracting the ID3
 * frame information.
 */

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "audiogetid3info.h"

#include "oscl_mem.h"
#include "oscl_stdstring.h"
#include "oscl_utf8conv.h"

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
static const uint8 ID3Tag[] = "ID3";
static const uint8 TAGTag[] = "TAG";

static uint32 SafeSynchIntToInt32(uint32 SafeSynchInteger)
{
    uint8 * pBuf = (uint8 *) & (SafeSynchInteger);
    uint8 tmpByte = 0;
    int32 i = 0;
    uint32 Integer = 0;

    // This loop will calculate the correct size from the bytes designated for size
    // It is stored as a SynchSafe Integer. This means the 8th bit is reserved in
    // each byte and not used for integer precision.. The number is effectively 28
    // bits in length/precision.
    // Assumes: sizeof(uint32) = 4 Bytes
    for (i = 0; i < 4; i++)
    {
#if (OSCL_BYTE_ORDER_LITTLE_ENDIAN)
        tmpByte = (uint8)(pBuf[i] & MASK127);
#elif (OSCL_BYTE_ORDER_BIG_ENDIAN)
        tmpByte = pBuf[4-i-1] & MASK127;
#else
#error "Must Specify ENDIANNESS"
#endif
        // now shift the data to it's correct place
        Integer += tmpByte << VALID_BITS_IN_SYNC_SAFE_BYTE * i;
    }

    return Integer;
}

// Read in byte data and take most significant byte first
static bool readByteData(PVFile *fp, uint32 length, uint8 *data)
{
    uint32 bytesRead;
    bytesRead = fp->Read(data, 1, length);

    if (bytesRead < (uint32)length) // read byte data failed
    {
        return false;
    }

    return true;
}

// Read in the 32 bits byte by byte and take most significant byte first
static bool read32(PVFile *fp, uint32 &data)
{
    const int32 N = 4;
    uint8 bytes[N];
    data = 0;

    int32 retVal = (int32)(fp->Read((void*)bytes, 1, N));

    if (retVal < N)
    {
        return false;
    }

    for (int32 i = 0; i < N; i++)
    {
        data = (data << 8) | bytes[i];
    }

    return true;
}


// Read in the 32 bits byte by byte and take most significant byte first
static bool read24(PVFile *fp, uint32 &data)
{
    const int32 N = 3;
    uint8 bytes[N];
    data = 0;

    int32 retVal = (int32)(fp->Read((void*)bytes, 1, N));

    if (retVal < N)
    {
        return false;
    }

    for (int32 i = 0; i < N; i++)
    {
        data = (data << 8) | bytes[i];
    }

    return true;
}

// Read in the 8 bit byte
static bool read8(PVFile *fp, uint8 &data)
{
    data = 0;

    int32 retVal = (int32)(fp->Read((void*) & data, 1, 1));

    if (retVal < 1)
    {
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::CID3TagParser()
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: None
//
//  Outputs: None
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function is the constructor for the CID3TagParser class.
//
//------------------------------------------------------------------------------

CID3TagParser::CID3TagParser()
{
    iTitleFoundFlag = false;
    oscl_memset(&iID3TagInfo, 0, sizeof(iID3TagInfo));
    iFilePtr = NULL;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::~CID3TagParser
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: None
//
//  Outputs: None
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function is the destructor for the CID3TagParser class.
//
//------------------------------------------------------------------------------
OSCL_EXPORT_REF CID3TagParser::~CID3TagParser()
{
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::NewL()
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: None
//
//  Outputs: None
//
//  Returns: Returns a pointer to an instance of this class
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function is the 2nd phase constructor for this object.It also
// is to be available to application programs and other DLLs.
//
//------------------------------------------------------------------------------
OSCL_EXPORT_REF CID3TagParser* CID3TagParser::NewL()
{
    OsclAuditCB myCB;
    OsclMemInit(myCB);
    CID3TagParser* self = OSCL_AUDIT_NEW(myCB, CID3TagParser, ());
    self->ConstructL();
    return self;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::Delete()
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: Pointer to an instance of this class
//
//  Outputs: None
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// Destructor for use with NewL
//
//------------------------------------------------------------------------------
OSCL_EXPORT_REF  void CID3TagParser::Delete(CID3TagParser* aObject)
{
    OSCL_DELETE(aObject);
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::ParseTagInfo(
//                                            TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aID3MetaData - Reference to a pre-allocated TAudioMetaData structure
//
//  Outputs:
//     aID3MetaData - Reference to a pre-allocated TAudioMetaData structure
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function causes the checks for the existance of an ID3V1 or ID3V2 tag
// associated with the file under investigation. It then makes a function
// call to get the information from the tag to populate aID3MetaData.
//
//------------------------------------------------------------------------------
// CAUTION
// Leaves the File Pointer a the start of the first audio frame
//------------------------------------------------------------------------------
OSCL_EXPORT_REF int32 CID3TagParser::ParseID3TagInfo(PVFile * aFile,
        TAudioMetaData& aID3MetaData)
{
    iTitleFoundFlag = false;
    int32 iCurrentFilePosn = 0;

    // Initialize the ID3 elements to be unknown
    aID3MetaData.iTitle.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iArtist.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iAlbum.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iSet.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iYear.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iComment.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iCopyright.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iGenre.iFormat = TAudioMetaData::FORMAT_INVALID;
    aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::FORMAT_INVALID;

    if (aFile != NULL)
    {
        iFilePtr = aFile;
        // SEEK TO THE END OF THE FILE AND GET FILE SIZE
        iCurrentFilePosn = iFilePtr->Tell();
        if (iCurrentFilePosn == -1)
        {
            return PVID3PARSER_FILE_OPERATION_FAILED;
        }

        if (iFilePtr->Seek(0, Oscl_File::SEEKEND) == -1)
        {
            return PVID3PARSER_FILE_OPERATION_FAILED;
        }

        aID3MetaData.iFileSizeInBytes = iFilePtr->Tell();
        if (aID3MetaData.iFileSizeInBytes == (uint32) - 1)
        {
            return PVID3PARSER_FILE_OPERATION_FAILED;
        }
        if (iFilePtr->Seek(iCurrentFilePosn, Oscl_File::SEEKSET) == -1)
        {
            return PVID3PARSER_FILE_OPERATION_FAILED;
        }

        // Initialize the ID3 tag
        aID3MetaData.iID3VersionType = ENoTag;

        if (CheckForTagID3V2(aID3MetaData))
        {
            aID3MetaData.iID3VersionType = EID3V2;
            iCurrentFilePosn = iFilePtr->Tell();
            if (iCurrentFilePosn == -1)
            {
                return PVID3PARSER_FILE_OPERATION_FAILED;
            }

            ReadHeaderID3V2(aID3MetaData);

            if (!iTitleFoundFlag)
            {
                if (CheckForTagID3V1(aID3MetaData))
                {
                    ReadID3V1Tag(aID3MetaData, true);
                }
            }

            if (iFilePtr->Seek(iCurrentFilePosn, Oscl_File::SEEKSET) == -1)
            {
                return PVID3PARSER_FILE_OPERATION_FAILED;
            }
        }
        else
        {
            if (CheckForTagID3V1(aID3MetaData))
            {
                aID3MetaData.iID3VersionType = EID3V1;
                ReadID3V1Tag(aID3MetaData, false);

                aID3MetaData.iByteOffsetToStartOfAudioFrames = 0;
            }
            else // No ID3 tag was found.
            {
                aID3MetaData.iID3VersionType = ENoTag;
                aID3MetaData.iByteOffsetToStartOfAudioFrames = 0;
            }

            if (iFilePtr->Seek(iCurrentFilePosn, Oscl_File::SEEKSET) == -1)
            {
                return PVID3PARSER_FILE_OPERATION_FAILED;
            }
        }

        return PVID3PARSER_EVERYTHING_FINE;
    }
    else
    {
        return PVID3PARSER_FILE_OPEN_FAILED;
    }
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::ConstructL()
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: None
//
//  Outputs: None
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function is the constructor for the CID3TagParser class.
//
//------------------------------------------------------------------------------
void CID3TagParser::ConstructL()
{
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::CheckForTagID3V2(TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aID3MetaData - reference to a TAudioMetaData structure
//
//  Outputs: None
//
//  Returns:
//     bool - true if Id3V2 tag present otherwise false.
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function checks the file for the presence of ID3V2 Tag attached to
//  the beginning of the file. This tag is  denoted by the string "ID3"
//  at the very beginning of the file.
//------------------------------------------------------------------------------

bool CID3TagParser::CheckForTagID3V2(TAudioMetaData& aID3MetaData)
{
    uint8 id3Header[ID3V2_TAG_NUM_BYTES_HEADER+1] = {0};

    // Make sure file is big enough to contain a tag
    if (aID3MetaData.iFileSizeInBytes >= ID3V2_TAG_NUM_BYTES_HEADER)
    {
        if (!readByteData(iFilePtr, ID3V2_TAG_NUM_BYTES_HEADER, id3Header))
        {
            return false;
        }

        // Read in ID3 Tags at the front of the file.
        if (oscl_memcmp(ID3Tag, id3Header, 3) == 0)
        {
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::CheckForTagID3V1(TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aID3MetaData - reference to a TAudioMetaData structure
//
//  Outputs: None
//
//  Returns:
//     bool - true if Id3V1 tag present otherwise false
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function checks the file for the presence of ID3V1 Tag. This tag is
// denoted by the string "TAG" starting 128 bytes in from the end-of-file.
//
//------------------------------------------------------------------------------
bool CID3TagParser::CheckForTagID3V1(TAudioMetaData& aID3MetaData)
{
    uint8 tagHeader[ID3V1_TAG_NUM_BYTES_HEADER+1] = {0};

    // Make sure file is big enough to contain a tag
    if (aID3MetaData.iFileSizeInBytes >= ID3V1_MAX_NUM_BYTES_TOTAL)
    {
        uint32 nBytes = 0;
        // Read the value at the tag position

        nBytes = aID3MetaData.iFileSizeInBytes - ID3V1_MAX_NUM_BYTES_TOTAL;
        if (iFilePtr->Seek(nBytes, Oscl_File::SEEKSET) == -1)
        {
            return false;
        }

        if (!readByteData(iFilePtr, ID3V1_TAG_NUM_BYTES_HEADER, tagHeader))
        {
            return false;
        }

        // Read in ID3 Tags at the front of the file.
        if (oscl_memcmp(TAGTag, tagHeader, 3) == 0)
        {
            aID3MetaData.iMajorVersion = 1;
            aID3MetaData.iMinorVersion = 0;
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::ReadID3V1Tag(TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aID3MetaData - reference to a TAudioMetaData structure
//
//  Outputs:
//     aID3MetaData - reference to a TAudioMetaData structure
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
// This function parses the file and populates the aID3MetaData structure when
// an ID3V1 tag is found to be attached to the file and no ID3V2 tag was
// found. The data in the ID3V1 tag can only be ASCII, so the ConvertToUnicode
// must be called for all fields read from the ID3V1 tag.
//
//----------------------------------------------------------------------------
// REQUIREMENTS
//
//  The track must be declared in an ASCII character and not a numeric string.
//------------------------------------------------------------------------------

void CID3TagParser::ReadID3V1Tag(TAudioMetaData& aID3MetaData,
                                 bool           aTitleOnlyFlag)
{
    char Buf[ID3V1_MAX_NUM_BYTES_FIELD_SIZE+1] = {0};

    // Read and convert title
    oscl_memset(Buf, 0, ID3V1_MAX_NUM_BYTES_FIELD_SIZE + 1);
    if (readByteData(iFilePtr, ID3V1_MAX_NUM_BYTES_TITLE, (uint8 *)Buf) == false)
    {
        return;
    }
    Buf[ID3V1_MAX_NUM_BYTES_FIELD_SIZE] = 0;

    oscl_strncpy(aID3MetaData.iTitle.pChar, Buf, oscl_strlen(Buf));
    aID3MetaData.iTitle.pChar[oscl_strlen(Buf)] = 0;

    if ((aID3MetaData.iID3VersionType == EID3V2) && (aTitleOnlyFlag == true))
    {
        aID3MetaData.iTitle.iFormat = TAudioMetaData::EISO88591_CHAR;

        iTitleFoundFlag = true;
        return;
    }

    iTitleFoundFlag = true;

    if (!aTitleOnlyFlag)
    {
        // Set the format for title
        aID3MetaData.iTitle.iFormat = TAudioMetaData::EISO88591_CHAR;

        // Read and convert artist
        oscl_memset(Buf, 0, ID3V1_MAX_NUM_BYTES_FIELD_SIZE + 1);
        if (readByteData(iFilePtr, ID3V1_MAX_NUM_BYTES_ARTIST, (uint8*)Buf) == false)
        {
            return;
        }
        Buf[ID3V1_MAX_NUM_BYTES_ARTIST] = 0;

        oscl_strncpy(aID3MetaData.iArtist.pChar, Buf, oscl_strlen(Buf));
        aID3MetaData.iArtist.pChar[oscl_strlen(Buf)] = 0;
        aID3MetaData.iArtist.iFormat = TAudioMetaData::EISO88591_CHAR;

        // Read and convert album
        oscl_memset(Buf, 0, ID3V1_MAX_NUM_BYTES_FIELD_SIZE + 1);
        if (readByteData(iFilePtr, ID3V1_MAX_NUM_BYTES_ALBUM, (uint8*)Buf) == false)
        {
            return;
        }
        Buf[ID3V1_MAX_NUM_BYTES_ALBUM] = 0;

        oscl_strncpy(aID3MetaData.iAlbum.pChar, Buf, oscl_strlen(Buf));
        aID3MetaData.iAlbum.pChar[oscl_strlen(Buf)] = 0;
        aID3MetaData.iAlbum.iFormat = TAudioMetaData::EISO88591_CHAR;

        // Read and convert year
        oscl_memset(Buf, 0, ID3V1_MAX_NUM_BYTES_FIELD_SIZE + 1);
        if (readByteData(iFilePtr, ID3V1_MAX_NUM_BYTES_YEAR, (uint8*)Buf) == false)
        {
            return;
        }
        Buf[ID3V1_MAX_NUM_BYTES_YEAR] = 0;

        oscl_strncpy(aID3MetaData.iYear.pChar, Buf, oscl_strlen(Buf));
        aID3MetaData.iYear.pChar[oscl_strlen(Buf)] = 0;
        aID3MetaData.iYear.iFormat = TAudioMetaData::EISO88591_CHAR;

        // Read and convert comment & possibly track number
        oscl_memset(Buf, 0, ID3V1_MAX_NUM_BYTES_FIELD_SIZE + 1);
        if (readByteData(iFilePtr, ID3V1_MAX_NUM_BYTES_COMMENT, (uint8*)Buf) == false)
        {
            return;
        }
        if ((Buf[ID3V1_MAX_NUM_BYTES_COMMENT-2] == 0)
                && (Buf[ID3V1_MAX_NUM_BYTES_COMMENT-1] != 0))
        {
            // This would mean its an ID3v1.1 tag and hence has the
            // the track number also, so extract it
            aID3MetaData.iTrackNumber.uint8_value = (uint8)Buf[ID3V1_MAX_NUM_BYTES_COMMENT-1];
            aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EBYTEVALUE_UINT8;
            aID3MetaData.iMinorVersion = 1;
        }
        else
        {
            // Track number is not available
            aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::FORMAT_INVALID;
        }
        Buf[ID3V1_MAX_NUM_BYTES_COMMENT] = 0;

        oscl_strncpy(aID3MetaData.iComment.pChar, Buf, oscl_strlen(Buf));
        aID3MetaData.iComment.pChar[oscl_strlen(Buf)] = 0;
        aID3MetaData.iComment.iFormat = TAudioMetaData::EISO88591_CHAR;
        // Comment language code and comment not available in ID3v1
        oscl_memset(aID3MetaData.iComment.description_pChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);
        oscl_memset(aID3MetaData.iComment.langcode_pChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);

        // Read the genre value
        oscl_memset(Buf, 0, ID3V1_MAX_NUM_BYTES_FIELD_SIZE + 1);
        if (readByteData(iFilePtr, ID3V1_MAX_NUM_BYTES_GENRE, (uint8*)Buf) == false)
        {
            return;
        }
        aID3MetaData.iGenre.uint8_value = (uint8)Buf[0];
        aID3MetaData.iGenre.iFormat = TAudioMetaData::EBYTEVALUE_UINT8;
    }
}


//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::ReadHeaderID3V2(TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aID3MetaData - pointer to repository for parsed data
//
//  Outputs:
//     aID3MetaData - pointer to repository for parsed data
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function parses the ID3 tag frames from the file and populates the
//  aID3MetaData structure when an ID3V2 tag is found attached to the start
//  of a file.
//
//------------------------------------------------------------------------------

void CID3TagParser::ReadHeaderID3V2(TAudioMetaData& aID3MetaData)
{
    if (iFilePtr->Seek(0, Oscl_File::SEEKSET) == -1)
    {
        return;
    }

    // Read "ID3" tag at start of file.
    uint8 identifierBuf[ID3V2_TAG_NUM_BYTES_ID];
    if (readByteData(iFilePtr, ID3V2_TAG_NUM_BYTES_ID, identifierBuf) == false)
    {
        return;
    }

    // Read and convert tag versions, Major and Minor
    uint8 ID3V2MajorVer, ID3V2MinorVer;
    if (read8(iFilePtr, ID3V2MajorVer) == false)
    {
        return;
    }
    if (read8(iFilePtr, ID3V2MinorVer) == false)
    {
        return;
    }
    aID3MetaData.iMajorVersion = ID3V2MajorVer;
    aID3MetaData.iMinorVersion = ID3V2MinorVer;

    // Read and convert tag flags
    if (read8(iFilePtr, iID3TagInfo.iID3V2TagFlagsV2) == false)
    {
        return;
    }

    // Read and convert tag size
    if (read32(iFilePtr, iID3TagInfo.iID3V2TagSize) == false)
    {
        return;
    }

    // Now check if an extended header exists
    bool extHeaderFlag = false;
    if (iID3TagInfo.iID3V2TagFlagsV2 & FLAGMASK)
    {
        extHeaderFlag = true;
        uint8 extHeader[ID3V2_TAG_EXTENDED_HEADER_TOTAL_SIZE];
        if (readByteData(iFilePtr, ID3V2_TAG_EXTENDED_HEADER_TOTAL_SIZE, extHeader) == false)
        {
            return;
        }
    }

    // tagSize will store the file's Id3v2 tag size
    uint32 tagSize = 0;
    uint32 i;

    tagSize = SafeSynchIntToInt32(iID3TagInfo.iID3V2TagSize);

    // set iByteOffsetToStartOfAudioFrames and it must account for the frame header
    aID3MetaData.iByteOffsetToStartOfAudioFrames =
        tagSize + ID3V2_TAG_NUM_BYTES_HEADER;

    // Add Footer Header Size if present
    if (iID3TagInfo.iID3V2TagFlagsV2 & FOOTERMASK)
    {
        aID3MetaData.iByteOffsetToStartOfAudioFrames += ID3V2_TAG_NUM_BYTES_FOOTER;
    }

    uint32  tagExtHeaderSize = 0;
    // Calculate the length of the extended header.
    if (extHeaderFlag)
    {
        tagExtHeaderSize = SafeSynchIntToInt32(iID3TagInfo.iID3V2ExtendedHeader);
        aID3MetaData.iByteOffsetToStartOfAudioFrames +=
            tagExtHeaderSize + ID3V2_TAG_EXTENDED_HEADER_TOTAL_SIZE;
    }

    TID3V2FrameType frameType;
    uint32 currFrameLength;
    uint32 aIDV2FrameNumBytesHeader;

    i = ID3V2_TAG_NUM_BYTES_HEADER; // must account for the tag header
    if (extHeaderFlag)
    {
        i += tagExtHeaderSize + ID3V2_TAG_EXTENDED_HEADER_TOTAL_SIZE;
    }

    if (ID3V2MajorVer == 0x02)
        aIDV2FrameNumBytesHeader = ID3V22_FRAME_NUM_BYTES_HEADER;
    else
        aIDV2FrameNumBytesHeader = ID3V2_FRAME_NUM_BYTES_HEADER;

    while (i <= tagSize)
    {
        // Read the frame header
        if (iFilePtr->Seek(i, Oscl_File::SEEKSET) == -1)
        {
            return;
        }

        if (ID3V2MajorVer == 0x02)
            ReadFrameHeaderID3V22(i);
        else
            ReadFrameHeaderID3V2(i);

        if (ID3V2MajorVer == 0x04)
            iID3TagInfo.iID3V2FrameSize = SafeSynchIntToInt32(iID3TagInfo.iID3V2FrameSize);


        currFrameLength = 0;
        currFrameLength = iID3TagInfo.iID3V2FrameSize;
        if (currFrameLength > tagSize)
        {
            break;
        }
        // handle the frame header
        if (ID3V2MajorVer == 0x02)
            frameType = FrameSupportedID3V22();
        else
            frameType = FrameSupportedID3V2();

        /*
         * Check the byte after the frame header for indication of unicode.
         * If the frame is type TLEN then it has a numeric string and do not
         * make a check for unicode type.
         */
        if ((frameType != ETLEN) &&
                ((currFrameLength > 1) && (frameType != EFrameNotSupported)))
        {
            uint8 unicodeCheck;

            if (read8(iFilePtr, unicodeCheck) == false)
            {
                return;
            }

            if (unicodeCheck == EISO88591)
            {
                // This frame contains normal ASCII text strings. (ISO-8859-1)
                iID3TagInfo.iTextType = EISO88591;
                HandleID3V2FrameDataASCII(frameType,
                                          i + aIDV2FrameNumBytesHeader + 1,
                                          currFrameLength - 1,
                                          aID3MetaData);
            }
            else if (unicodeCheck == EUTF16)
            {
                uint8 endianCheck;

                // Special case for comments. The first 3 bytes are the have
                // language code, which does not have any BOM
                if (frameType == ECOMT)
                {
                    uint8 endianData[4];

                    if (readByteData(iFilePtr, 4, endianData) == false)
                    {
                        return;
                    }
                    endianCheck = endianData[3];
                }
                else
                {
                    if (read8(iFilePtr, endianCheck) == false)
                    {
                        return;
                    }
                }

                // This frame's text strings are Unicode and the frame
                // does include a BOM value. (UTF-16)
                iID3TagInfo.iTextType = EUTF16;

                if ((endianCheck != UNICODE_LITTLE_ENDIAN_INDICATOR) &&
                        (endianCheck != UNICODE_BIG_ENDIAN_INDICATOR))
                {
                    frameType = EFrameNotSupported;
                    break;
                }
                HandleID3V2FrameDataUTF16(frameType,
                                          i + aIDV2FrameNumBytesHeader + 1,
                                          currFrameLength - 1,
                                          EUTF16,
                                          aID3MetaData);
            }
            else if (unicodeCheck == EUTF16BE)
            {
                // This frame's text strings are Unicode but the frame
                // does not contain a BOM(byte order mark) (UTF-16BE)

                iID3TagInfo.iTextType = EUTF16BE;

                // Big Endian is assumed since the frame did not specify the endian type.
                HandleID3V2FrameDataUTF16(frameType,
                                          i + aIDV2FrameNumBytesHeader + 1,
                                          currFrameLength - 1,
                                          EUTF16BE,
                                          aID3MetaData);
            }
            else if (unicodeCheck == EUTF8)
            {
                // This frame's text strings are Unicode (UTF-8)
                iID3TagInfo.iTextType = EUTF8;
                HandleID3V2FrameDataUTF8(frameType,
                                         i + aIDV2FrameNumBytesHeader + 1,
                                         currFrameLength - 1,
                                         aID3MetaData);
            }
            else
            {
                iID3TagInfo.iTextType = ENoType;

                // This case is when no text type is defined in the frame.
                HandleID3V2FrameDataASCII(frameType,
                                          i + aIDV2FrameNumBytesHeader,
                                          currFrameLength,
                                          aID3MetaData);
            }
        }
        else
        {
            if (frameType == EEND)
            {
                i = tagSize + 1;
            }
            else
            {
                iID3TagInfo.iTextType = ENoType;
                HandleID3V2FrameDataASCII(frameType,
                                          i + aIDV2FrameNumBytesHeader,
                                          currFrameLength,
                                          aID3MetaData);
            }
        }
        i += currFrameLength + aIDV2FrameNumBytesHeader;
    }
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::ReadFrameHeaderID3V2(uint32 aPos)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aPos - position in file where frame
//
//  Outputs: None
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function parses the frame and populates the iID3TagInfo structure
//  when an ID3V2 tag is found to be attached to the file.
//
//------------------------------------------------------------------------------

void CID3TagParser::ReadFrameHeaderID3V2(uint32 aPos)
{
    if (readByteData(iFilePtr, ID3V2_FRAME_NUM_BYTES_ID, iID3TagInfo.iID3V2FrameID) == false)
    {
        return;
    }
    if (read32(iFilePtr, iID3TagInfo.iID3V2FrameSize) == false)
    {
        return;
    }
    if (readByteData(iFilePtr, ID3V2_FRAME_NUM_BYTES_FLAG, iID3TagInfo.iID3V2FrameFlag) == false)
    {
        return;
    }

    iID3TagInfo.iPositionInFile = aPos;

    return;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::ReadFrameHeaderID3V22(uint32 aPos)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aPos - position in file where frame
//
//  Outputs: None
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function parses the frame and populates the iID3TagInfo structure
//  when an ID3V2 tag is found to be attached to the file.
//
//------------------------------------------------------------------------------

void CID3TagParser::ReadFrameHeaderID3V22(uint32 aPos)
{
    if (readByteData(iFilePtr, ID3V22_FRAME_NUM_BYTES_ID, iID3TagInfo.iID3V2FrameID) == false)
    {
        return;
    }
    if (read24(iFilePtr, iID3TagInfo.iID3V2FrameSize) == false)
    {
        return;
    }

    iID3TagInfo.iPositionInFile = aPos;

    return;
}


//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::HandleID3V2FrameDataASCII(
//                                               TFrameTypeID3V2 aFrameType,
//                                               uint32            aPos,
//                                               uint32            aSize,
//                                               TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aFrameType   - enumerated TFrameTypeID3V2 type - used to determine if
//                    Frame is supported
//     aPos         - position in file where frame data begins
//     aSize        - size of the frame data
//     aID3MetaData - pointer to repository for parsed data
//
//  Outputs:
//     aID3MetaData - pointer to repository for parsed data
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function receives a ID3V2 frame and handles it accordingly. Each
//  frame header is composed of 10 bytes - 4 bytes for FrameID, 4 bytes for
//  FrameSize, and 2 bytes for FrameFlags, respectively. Once we have a
//  frame's unique FrameID, we can handle the frame accordingly. (eg. The
//  frame with FrameID = "TIT2" contains data describing the title.
//  Clearly, the data in that frame should be copied to it respective
//  location in aID3MetaData. The location for the data with FrameID = "TIT2"
//  goes in the aID3MetaData.iTitle buffer. This line of code accomplishes our
//  objective: aID3MetaData.iTitle.Copy(ptrFrameData16);
//
//------------------------------------------------------------------------------

void CID3TagParser::HandleID3V2FrameDataASCII(TID3V2FrameType      aFrameType,
        uint32               aPos,
        uint32               aSize,
        TAudioMetaData&      aID3MetaData)
{
    if (iFilePtr->Seek(aPos, Oscl_File::SEEKSET) == -1)
    {
        return;
    }

    // create buffers to store frame data
    uint8* frameData = OSCL_ARRAY_NEW(uint8, aSize + 1);
    if (frameData == NULL)
    {
        return;
    }

    uint8* ptrFrameData = frameData;
    oscl_memset(frameData,  0, aSize + 1);

    switch (aFrameType)
    {
        case ETIT2:
        {
            // Handle TIT2 frame
            // The TIT2 frame contains data referring to the title.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iTitle.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_TITLE);
            aID3MetaData.iTitle.pChar[MAX_UNICODE_BUF_LEN_TITLE-1] = 0;
            aID3MetaData.iTitle.iFormat = TAudioMetaData::EISO88591_CHAR;

            iTitleFoundFlag = true;
        }
        break;

        case ETPE1:
        {
            // Handle TPE1 frame
            // The TPE1 frame contains data referring to the artist.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iArtist.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_ARTIST);
            aID3MetaData.iArtist.pChar[MAX_UNICODE_BUF_LEN_ARTIST-1] = 0;
            aID3MetaData.iArtist.iFormat = TAudioMetaData::EISO88591_CHAR;


        }
        break;

        case ETPOS:
        {
            // Handle TPOS/TPA frame
            // The TPOS frame contains data defining if this is part of
            // a multi disc set.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iSet.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_SET);
            aID3MetaData.iSet.pChar[MAX_UNICODE_BUF_LEN_SET-1] = 0;
            aID3MetaData.iSet.iFormat = TAudioMetaData::EISO88591_CHAR;

        }
        break;

        case ETALB:
        {
            // Handle TALB frame
            // The TALB frame contains data referring to the album.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iAlbum.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_ALBUM);
            aID3MetaData.iAlbum.pChar[MAX_UNICODE_BUF_LEN_ALBUM-1] = 0;
            aID3MetaData.iAlbum.iFormat = TAudioMetaData::EISO88591_CHAR;

        }
        break;

        case ETCOP:
        {
            // Handle TCOP frame
            // The TCOP frame contains data referring to the copyright.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iCopyright.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_COPYRIGHT);
            aID3MetaData.iCopyright.pChar[MAX_UNICODE_BUF_LEN_COPYRIGHT-1] = 0;
            aID3MetaData.iCopyright.iFormat = TAudioMetaData::EISO88591_CHAR;

        }
        break;

        case ETCON:
        {
            // Handle TCON frame
            // The TCON frame contains data referring to the genre.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iGenre.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_GENRE);
            aID3MetaData.iGenre.pChar[MAX_UNICODE_BUF_LEN_GENRE-1] = 0;
            aID3MetaData.iGenre.iFormat = TAudioMetaData::EISO88591_CHAR;


        }
        break;

        case ETRCK:
        {
            // Handle TRCK frame
            // The TRCK frame contains data referring to the track number.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iTrackNumber.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_TRACK_NUMBER);
            aID3MetaData.iTrackNumber.pChar[MAX_UNICODE_BUF_LEN_TRACK_NUMBER-1] = 0;
            aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EISO88591_CHAR;

        }
        break;

        case ETLEN:
        {
            // Handle TLEN frame
            // The TLEN frame contains data referring to the recording time.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            // The ID3 Track Length is a numeric string in milliseconds.
            uint32 temp32Low  = 0;
            uint32 temp32High = 0;
            int32 numericStringLen = oscl_strlen((const char*)ptrFrameData);
            int32 i;
            if (numericStringLen > 4)
            {
                for (i = 0; i < 4; i++)
                {
                    temp32Low  |= ptrFrameData[numericStringLen-4+i] << (24 - (i * 8));
                }
                for (i = 0; i < (numericStringLen - 4); i++)
                {
                    temp32High |= ptrFrameData[i] << (((numericStringLen - 5) * 8) - (i * 8));
                }
            }
            else
            {
                for (i = 0; i < numericStringLen; i++)
                {
                    temp32Low  |= ptrFrameData[i] <<
                                  (((numericStringLen - 1) * 8) - (i * 8));
                }
            }

            // The next 3 lines is equivalent to: aID3MetaData.iTrackLength = (temp32High << 32) | temp32Low;
            // It is written thus because the behavior of "temp32High << 32" (where temp32High has a width <= 32)
            // is undefined in some processors.
            aID3MetaData.iTrackLength = temp32High;
            aID3MetaData.iTrackLength <<= 32;
            aID3MetaData.iTrackLength += temp32Low;
            // Go from milliseconds to microseconds.
            aID3MetaData.iTrackLength *= 1000;
        }
        break;

        case ETYER:
        {
            // Handle TYER frame
            // The TYER frame contains data referring to the year.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iYear.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_YEAR);
            aID3MetaData.iYear.pChar[MAX_UNICODE_BUF_LEN_YEAR-1] = 0;
            aID3MetaData.iYear.iFormat = TAudioMetaData::EISO88591_CHAR;
        }
        break;

        case ECOMT:
        {
            // Handle COMM frame
            // The COMM frame contains data referring to comments
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(ptrFrameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            char* strptr = (char*)ptrFrameData;
            oscl_memset(aID3MetaData.iComment.langcode_pChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            oscl_memset(aID3MetaData.iComment.description_pChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);

            // Copy language code
            oscl_strncpy(aID3MetaData.iComment.langcode_pChar, strptr, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            aID3MetaData.iComment.langcode_pChar[MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE-1] = 0;

            // Move the string pointer past the language code
            strptr += 3;

            // Copy comment description string
            uint32 commentstrlen = oscl_strlen(strptr);
            if (commentstrlen < (aSize - 5))
            {
                oscl_strncpy(aID3MetaData.iComment.description_pChar, strptr, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);
                aID3MetaData.iComment.description_pChar[MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION-1] = 0;
                strptr += (commentstrlen + 1);
            }

            // Finally copy the comment string
            oscl_strncpy(aID3MetaData.iComment.pChar, strptr, MAX_UNICODE_BUF_LEN_COMMENT);
            aID3MetaData.iComment.pChar[MAX_UNICODE_BUF_LEN_COMMENT-1] = 0;

            aID3MetaData.iComment.iFormat = TAudioMetaData::EISO88591_CHAR;
        }
        break;

        default:
        {
            // Frame is not yet supported
            break;
        }
    }

    OSCL_ARRAY_DELETE(ptrFrameData);
    ptrFrameData = NULL;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::HandleID3V2FrameDataUnicode16(
//                                               TFrameTypeID3V2 aFrameType,
//                                               uint32            aPos,
//                                               uint32            aSize,
//                                               uint32            aEndianType,
//                                               TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aFrameType   - enumerated TFrameTypeID3V2 type - used to determine if
//                    Frame is supported
//     aPos         - position in file where frame data begins
//     aSize        - size of the frame data
//     aEndianType  - flag indicator regarding the text endian type
//     aID3MetaData - pointer to repository for parsed data
//
//  Outputs:
//     aID3MetaData - pointer to repository for parsed data
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function receives a ID3V2 frame and handles it accordingly. Each
//  frame header is composed of 10 bytes - 4 bytes for FrameID, 4 bytes for
//  FrameSize, and 2 bytes for FrameFlags, respectively. Once we have a
//  frame's unique FrameID, we can handle the frame accordingly. (eg. The
//  frame with FrameID = "TIT2" contains data describing the title.
//  Clearly, the data in that frame should be copied to it respective
//  location in aID3MetaData. The location for the data with FrameID = "TIT2"
//  goes in the aID3MetaData.iTitle buffer. This line of code accomplishes our
//  objective: aID3MetaData.iTitle.Copy(ptrFrameData16); All of the frames
//  sent to this function have unicode strings in them that will need to be
//  read different than the ASCII strings.
//
//------------------------------------------------------------------------------
void CID3TagParser::HandleID3V2FrameDataUnicode16(TID3V2FrameType      aFrameType,
        uint32                 aPos,
        uint32                 aSize,
        uint32                 aEndianType,
        TAudioMetaData&      aID3MetaData)
{
    // seek to the beginning of the current frame data
    if (iFilePtr->Seek(aPos, Oscl_File::SEEKSET) == -1)
    {
        return;
    }

    // create buffers to store frame data
    uint8* frameData = OSCL_ARRAY_NEW(uint8, aSize + 2);
    if (frameData == NULL)
    {
        return;
    }
    oscl_wchar* frameDataWC = OSCL_ARRAY_NEW(oscl_wchar, aSize + 2);
    if (frameDataWC == NULL)
    {
        OSCL_ARRAY_DELETE(frameData);
        return;
    }
    uint8* ptrFrameData  = frameData;
    oscl_wchar* ptrFrameDataWC = frameDataWC;

    oscl_memset(frameData,  0, aSize + 2);
    oscl_memset(frameDataWC, 0, sizeof(oscl_wchar)*(aSize + 1));

    switch (aFrameType)
    {
        case ETIT2:
        {   // Handle TIT2 frame
            // The TIT2 frame contains data referring to the title.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }

            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);

            oscl_strncpy(aID3MetaData.iTitle.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_TITLE);
            aID3MetaData.iTitle.pWChar[MAX_UNICODE_BUF_LEN_TITLE-1] = NULL_TERM_CHAR;

            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iTitle.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iTitle.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
            iTitleFoundFlag = true;
        }
        break;

        case ETPE1:
        {
            // Handle TPE1 frame
            // The TPE1 frame contains data referring to the artist.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iArtist.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_ARTIST);
            aID3MetaData.iArtist.pWChar[MAX_UNICODE_BUF_LEN_ARTIST-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iArtist.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iArtist.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETPOS:
        {
            // Handle TPOS/TPA frame
            // The TPOS frame contains data defining if this is part of
            // a multi disc set.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iSet.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_SET);
            aID3MetaData.iSet.pWChar[MAX_UNICODE_BUF_LEN_SET-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iSet.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iSet.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETALB:
        {
            // Handle TALB frame
            // The TALB frame contains data referring to the album.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iAlbum.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_ALBUM);
            aID3MetaData.iAlbum.pWChar[MAX_UNICODE_BUF_LEN_ALBUM-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iAlbum.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iAlbum.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETCOP:
        {
            // Handle TCOP frame
            // The TCOP frame contains data referring to the copyright.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iCopyright.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_COPYRIGHT);
            aID3MetaData.iCopyright.pWChar[MAX_UNICODE_BUF_LEN_COPYRIGHT-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iCopyright.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iCopyright.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETCON:
        {
            // Handle TCON frame
            // The TCON frame contains data referring to the genre.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iGenre.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_GENRE);
            aID3MetaData.iGenre.pWChar[MAX_UNICODE_BUF_LEN_GENRE-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iGenre.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iGenre.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETRCK:
        {
            // Handle TRCK frame
            // The TRCK frame contains data referring to the track number.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iTrackNumber.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_TRACK_NUMBER);
            aID3MetaData.iTrackNumber.pWChar[MAX_UNICODE_BUF_LEN_TRACK_NUMBER-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETYER:
        {
            // Handle TYER frame
            // The TYER frame contains data referring to the year.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);
            oscl_strncpy(aID3MetaData.iYear.pWChar, ptrFrameDataWC, MAX_UNICODE_BUF_LEN_YEAR);
            aID3MetaData.iYear.pWChar[MAX_UNICODE_BUF_LEN_YEAR-1] = NULL_TERM_CHAR;
            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iYear.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iYear.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;
        case ECOMT:
        {
            // Handle COMM frame
            // The COMM frame contains data referring to comments
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                OSCL_ARRAY_DELETE(frameDataWC);
                return;
            }
            ptrFrameData[aSize] = 0;
            EightBitToWideCharBufferTransfer(ptrFrameData,
                                             aSize,
                                             aEndianType,
                                             ptrFrameDataWC);

            oscl_wchar* wstrptr = ptrFrameDataWC;
            oscl_memset(aID3MetaData.iComment.langcode_pWChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            oscl_memset(aID3MetaData.iComment.description_pWChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);

            // Check for language code
            oscl_strncpy(aID3MetaData.iComment.langcode_pWChar, wstrptr, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            aID3MetaData.iComment.langcode_pWChar[MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE-1] = 0;

            // Move the string pointer past the language code
            wstrptr += 3;

            // Check for comment description string
            uint32 commentstrlen = oscl_strlen(wstrptr);
            if (commentstrlen < (aSize - 5))
            {
                oscl_strncpy(aID3MetaData.iComment.description_pWChar, wstrptr, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);
                aID3MetaData.iComment.description_pWChar[MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION-1] = 0;
                wstrptr += (commentstrlen + 1);
            }

            // Finally copy the comment string
            oscl_strncpy(aID3MetaData.iComment.pWChar, wstrptr, MAX_UNICODE_BUF_LEN_COMMENT);
            aID3MetaData.iComment.pWChar[MAX_UNICODE_BUF_LEN_COMMENT-1] = NULL_TERM_CHAR;

            if (aEndianType == UNICODE_BIG_ENDIAN)
            {
                aID3MetaData.iComment.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iComment.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;
        default:
        {
            // Frame is not yet supported
            break;
        }
    }

    OSCL_ARRAY_DELETE(ptrFrameData);
    ptrFrameData = NULL;

    OSCL_ARRAY_DELETE(ptrFrameDataWC);
    ptrFrameDataWC = NULL;
}


//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::HandleID3V2FrameDataUTF16(
//                                               TFrameTypeID3V2 aFrameType,
//                                               uint32            aPos,
//                                               uint32            aSize,
//                                               uint32            aEndianType,
//                                               TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aFrameType   - enumerated TFrameTypeID3V2 type - used to determine if
//                    Frame is supported
//     aPos         - position in file where frame data begins
//     aSize        - size of the frame data
//     aTextType    - flag indicator regarding the text type
//     aID3MetaData - pointer to repository for parsed data
//
//  Outputs:
//     aID3MetaData - pointer to repository for parsed data
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function receives a ID3V2 frame and handles it accordingly. Each
//  frame header is composed of 10 bytes - 4 bytes for FrameID, 4 bytes for
//  FrameSize, and 2 bytes for FrameFlags, respectively. Once we have a
//  frame's unique FrameID, we can handle the frame accordingly. (eg. The
//  frame with FrameID = "TIT2" contains data describing the title.
//  Clearly, the data in that frame should be copied to it respective
//  location in aID3MetaData. The location for the data with FrameID = "TIT2"
//  goes in the aID3MetaData.iTitle buffer. This line of code accomplishes our
//  objective: aID3MetaData.iTitle.Copy(ptrFrameData16); All of the frames
//  sent to this function have unicode strings in them that will need to be
//  read different than the ASCII strings.
//
//------------------------------------------------------------------------------
void CID3TagParser::HandleID3V2FrameDataUTF16(TID3V2FrameType      aFrameType,
        uint32                 aPos,
        uint32                 aSize,
        TTextType              aTextType,
        TAudioMetaData&      aID3MetaData)
{
    // seek to the beginning of the current frame data
    if (iFilePtr->Seek(aPos, Oscl_File::SEEKSET) == -1)
    {
        return;
    }

    // create buffers to store frame data
    uint8* frameData = OSCL_ARRAY_NEW(uint8, aSize + 2);
    if (frameData == NULL)
    {
        return;
    }

    uint8* ptrFrameData  = frameData;

    oscl_memset(frameData,  0, aSize + 2);

    switch (aFrameType)
    {
        case ETIT2:
        {   // Handle TIT2 frame
            // The TIT2 frame contains data referring to the title.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }

            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iTitle.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_TITLE);
            aID3MetaData.iTitle.pWChar[MAX_UNICODE_BUF_LEN_TITLE-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iTitle.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iTitle.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
            iTitleFoundFlag = true;
        }
        break;

        case ETPE1:
        {
            // Handle TPE1 frame
            // The TPE1 frame contains data referring to the artist.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iArtist.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_ARTIST);
            aID3MetaData.iArtist.pWChar[MAX_UNICODE_BUF_LEN_ARTIST-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iArtist.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iArtist.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETPOS:
        {
            // Handle TPOS/TPA frame
            // The TPOS frame contains data defining if this is part of
            // a multi disc set.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iSet.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_SET);
            aID3MetaData.iSet.pWChar[MAX_UNICODE_BUF_LEN_SET-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iSet.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iSet.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETALB:
        {
            // Handle TALB frame
            // The TALB frame contains data referring to the album.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iAlbum.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_ALBUM);
            aID3MetaData.iAlbum.pWChar[MAX_UNICODE_BUF_LEN_ALBUM-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iAlbum.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iAlbum.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETCOP:
        {
            // Handle TCOP frame
            // The TCOP frame contains data referring to the copyright.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iCopyright.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_COPYRIGHT);
            aID3MetaData.iCopyright.pWChar[MAX_UNICODE_BUF_LEN_COPYRIGHT-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iCopyright.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iCopyright.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETCON:
        {
            // Handle TCON frame
            // The TCON frame contains data referring to the genre.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iGenre.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_GENRE);
            aID3MetaData.iGenre.pWChar[MAX_UNICODE_BUF_LEN_GENRE-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iGenre.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iGenre.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETRCK:
        {
            // Handle TRCK frame
            // The TRCK frame contains data referring to the track number.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iTrackNumber.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_TRACK_NUMBER);
            aID3MetaData.iTrackNumber.pWChar[MAX_UNICODE_BUF_LEN_TRACK_NUMBER-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;

        case ETYER:
        {
            // Handle TYER frame
            // The TYER frame contains data referring to the year.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;
            ptrFrameData[aSize+1] = 0;

            oscl_strncpy(aID3MetaData.iYear.pWChar, (oscl_wchar*)ptrFrameData, MAX_UNICODE_BUF_LEN_YEAR);
            aID3MetaData.iYear.pWChar[MAX_UNICODE_BUF_LEN_YEAR-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iYear.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iYear.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;
        case ECOMT:
        {
            // Handle COMM frame
            // The COMM frame contains data referring to comments

            // Since data in ptrFrameData has to be aligned one cannot read the
            // whole comments frame at once, since the language code is 3 byte
            // long and accessing the remaining part after that as a wide char
            // will cause misalignment. So the reading is split into two part.

            // Read Language Code (3 bytes)
            if (readByteData(iFilePtr, 3, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[3] = 0;

            oscl_wchar* wstrptr = (oscl_wchar*)ptrFrameData;
            oscl_memset(aID3MetaData.iComment.langcode_pWChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            oscl_memset(aID3MetaData.iComment.description_pWChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);

            // Check for language code
            // NOTE: language code is NEVER wide char - and it is always 3 bytes long
            oscl_strncpy(aID3MetaData.iComment.langcode_pWChar, wstrptr, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            aID3MetaData.iComment.langcode_pWChar[MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE-1] = 0;

            // Reset the part of ptrFrameData to zero, which was used for language code
            oscl_memset(ptrFrameData,  0, 3);

            // Read the remaning part of the comments frame
            if (readByteData(iFilePtr, aSize - 3, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }

            ptrFrameData[aSize-3] = 0;
            ptrFrameData[aSize-2] = 0; // aSize-3+1

            // Move the pointer to the beginning of the short content description
            wstrptr = (oscl_wchar*)ptrFrameData;

            // Check for short content description string
            uint32 commentstrlen = oscl_strlen(wstrptr);
            if (commentstrlen < (aSize - 5))
            {
                oscl_strncpy(aID3MetaData.iComment.description_pWChar, wstrptr, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);
                aID3MetaData.iComment.description_pWChar[MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION-1] = 0;

                // Move the pointer to the beginning of the actual comment text
                wstrptr += (commentstrlen + 1);
            }

            // Finally copy the comment string
            oscl_strncpy(aID3MetaData.iComment.pWChar, wstrptr, MAX_UNICODE_BUF_LEN_COMMENT);
            aID3MetaData.iComment.pWChar[MAX_UNICODE_BUF_LEN_COMMENT-1] = NULL_TERM_CHAR;

            if (aTextType == EUTF16BE)
            {
                aID3MetaData.iComment.iFormat = TAudioMetaData::EUTF16BE_WCHAR;
            }
            else
            {
                aID3MetaData.iComment.iFormat = TAudioMetaData::EUTF16_WCHAR;
            }
        }
        break;
        default:
        {
            // Frame is not yet supported
            break;
        }
    }

    OSCL_ARRAY_DELETE(ptrFrameData);
    ptrFrameData = NULL;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::HandleID3V2FrameDataUTF8(
//                                               TFrameTypeID3V2 aFrameType,
//                                               uint32            aPos,
//                                               uint32            aSize,
//                                               TAudioMetaData& aID3MetaData)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//     aFrameType   - enumerated TFrameTypeID3V2 type - used to determine if
//                    Frame is supported
//     aPos         - position in file where frame data begins
//     aSize        - size of the frame data
//     aID3MetaData - pointer to repository for parsed data
//
//  Outputs:
//     aID3MetaData - pointer to repository for parsed data
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function receives a ID3V2 frame and handles it accordingly. Each
//  frame header is composed of 10 bytes - 4 bytes for FrameID, 4 bytes for
//  FrameSize, and 2 bytes for FrameFlags, respectively. Once we have a
//  frame's unique FrameID, we can handle the frame accordingly. (eg. The
//  frame with FrameID = "TIT2" contains data describing the title.
//  Clearly, the data in that frame should be copied to it respective
//  location in aID3MetaData. The location for the data with FrameID = "TIT2"
//  goes in the aID3MetaData.iTitle buffer. This line of code accomplishes our
//  objective: aID3MetaData.iTitle.Copy(ptrFrameData16); All of the frames
//  sent to this function have UTF8 strings in them that will need to be
//  read different than the ASCII strings.
//
//------------------------------------------------------------------------------
void CID3TagParser::HandleID3V2FrameDataUTF8(TID3V2FrameType      aFrameType,
        uint32               aPos,
        uint32               aSize,
        TAudioMetaData&      aID3MetaData)
{
    // seek to the beginning of the current frame data
    if (iFilePtr->Seek(aPos, Oscl_File::SEEKSET) == -1)
    {
        return;
    }

    // create buffers to store frame data
    uint8* frameData = OSCL_ARRAY_NEW(uint8, aSize + 1);
    if (frameData == NULL)
    {
        return;
    }
    uint8* ptrFrameData = frameData;
    oscl_memset(frameData,  0, aSize + 1);

    switch (aFrameType)
    {
        case ETIT2:
        {
            // Handle TIT2 frame
            // The TIT2 frame contains data referring to the title.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iTitle.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_TITLE);
            aID3MetaData.iTitle.pChar[MAX_UNICODE_BUF_LEN_TITLE-1] = 0;
            aID3MetaData.iTitle.iFormat = TAudioMetaData::EUTF8_CHAR;

            iTitleFoundFlag = true;
        }
        break;

        case ETPE1:
        {
            // Handle TPE1 frame
            // The TPE1 frame contains data referring to the artist.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iArtist.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_ARTIST);
            aID3MetaData.iArtist.pChar[MAX_UNICODE_BUF_LEN_ARTIST-1] = 0;
            aID3MetaData.iArtist.iFormat = TAudioMetaData::EUTF8_CHAR;
        }
        break;

        case ETPOS:
        {
            // Handle TPOS/TPA frame
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iSet.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_SET);
            aID3MetaData.iSet.pChar[MAX_UNICODE_BUF_LEN_ALBUM-1] = 0;
            aID3MetaData.iSet.iFormat = TAudioMetaData::EUTF8_CHAR;
        }
        break;

        case ETALB:
        {
            // Handle TALB frame
            // The TALB frame contains data referring to the album.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iAlbum.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_ALBUM);
            aID3MetaData.iAlbum.pChar[MAX_UNICODE_BUF_LEN_ALBUM-1] = 0;
            aID3MetaData.iAlbum.iFormat = TAudioMetaData::EUTF8_CHAR;
        }
        break;

        case ETCOP:
        {
            // Handle TCOP frame
            // The TCOP frame contains data referring to the copyright.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iCopyright.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_COPYRIGHT);
            aID3MetaData.iCopyright.pChar[MAX_UNICODE_BUF_LEN_COPYRIGHT-1] = 0;
            aID3MetaData.iCopyright.iFormat = TAudioMetaData::EUTF8_CHAR;

        }
        break;

        case ETCON:
        {
            // Handle TCON frame
            // The TCON frame contains data referring to the genre.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iGenre.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_GENRE);
            aID3MetaData.iGenre.pChar[MAX_UNICODE_BUF_LEN_GENRE-1] = 0;
            aID3MetaData.iGenre.iFormat = TAudioMetaData::EUTF8_CHAR;

        }
        break;

        case ETRCK:
        {
            // Handle TRCK frame
            // The TRCK frame contains data referring to the track number.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iTrackNumber.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_TRACK_NUMBER);
            aID3MetaData.iTrackNumber.pChar[MAX_UNICODE_BUF_LEN_TRACK_NUMBER-1] = 0;
            aID3MetaData.iTrackNumber.iFormat = TAudioMetaData::EUTF8_CHAR;
        }
        break;

        case ETYER:
        {
            // Handle TYER frame
            // The TYER frame contains data referring to the year.
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            oscl_strncpy(aID3MetaData.iYear.pChar, (char*)ptrFrameData, MAX_UNICODE_BUF_LEN_YEAR);
            aID3MetaData.iYear.pChar[MAX_UNICODE_BUF_LEN_YEAR-1] = 0;
            aID3MetaData.iYear.iFormat = TAudioMetaData::EUTF8_CHAR;
        }
        break;

        case ECOMT:
        {
            // Handle COMM frame
            // The COMM frame contains data referring to comments
            if (readByteData(iFilePtr, aSize, ptrFrameData) == false)
            {
                OSCL_ARRAY_DELETE(frameData);
                return;
            }
            ptrFrameData[aSize] = 0;

            char* strptr = (char*)ptrFrameData;
            oscl_memset(aID3MetaData.iComment.langcode_pChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            oscl_memset(aID3MetaData.iComment.description_pChar, 0, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);

            // Copy the language code
            oscl_strncpy(aID3MetaData.iComment.langcode_pChar, strptr, MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE);
            aID3MetaData.iComment.langcode_pChar[MAX_UNICODE_BUF_LEN_COMMENT_LANGCODE-1] = 0;

            // Move the string pointer past the language code
            strptr += 3;

            // Check for comment description string
            uint32 commentstrlen = oscl_strlen(strptr);
            if (commentstrlen < (aSize - 5))
            {
                oscl_strncpy(aID3MetaData.iComment.description_pChar, strptr, MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION);
                aID3MetaData.iComment.description_pChar[MAX_UNICODE_BUF_LEN_COMMENT_DESCRIPTION-1] = 0;
                strptr += (commentstrlen + 1);
            }

            // Finally copy the comment string
            oscl_strncpy(aID3MetaData.iComment.pChar, strptr, MAX_UNICODE_BUF_LEN_COMMENT);
            aID3MetaData.iComment.pChar[MAX_UNICODE_BUF_LEN_COMMENT-1] = 0;

            aID3MetaData.iComment.iFormat = TAudioMetaData::EUTF8_CHAR;
        }
        break;

        default:
        {
            // Frame is not yet supported
            break;
        }
    }

    OSCL_ARRAY_DELETE(ptrFrameData);
    ptrFrameData = NULL;
}

//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::FrameSupportedID3V2(void)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: None
//
//  Outputs: None
//
//  Returns:
//    ID3V2FrameTypeReturnValue - The value that describes the current frame.
//                                of type enum TID3V2FrameType
//
//  Global Variables Used:
//    TID3V2FrameType - The enum table containing the supported frame types
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function detects the ID3V2FrameType and returns the enum value that
//  corresponds to the current frame.
//
//------------------------------------------------------------------------------

TID3V2FrameType CID3TagParser::FrameSupportedID3V2(void)
{
    uint8 endTestBuf[ID3V2_FRAME_NUM_BYTES_ID] = {0};
    TID3V2FrameType ID3V2FrameTypeReturnValue;

    if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTIT2, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETIT2;
        iTitleFoundFlag           = true;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTPE1, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETPE1;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTALB, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETALB;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTCOP, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETCOP;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTCON, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETCON;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTRCK, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETRCK;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTDAT, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETDAT;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTLEN, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETLEN;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KCOMM, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ECOMT;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTYER, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETYER;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTDRC, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETYER;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, endTestBuf, ID3V2_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = EEND;
    }
    else
    {
        ID3V2FrameTypeReturnValue = EFrameNotSupported;
    }

    return ID3V2FrameTypeReturnValue;
}


//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::FrameSupportedID3V22(void)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs: None
//
//  Outputs: None
//
//  Returns:
//    ID3V2FrameTypeReturnValue - The value that describes the current frame.
//                                of type enum TID3V2FrameType
//
//  Global Variables Used:
//    TID3V2FrameType - The enum table containing the supported frame types
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function detects the ID3V2FrameType and returns the enum value that
//  corresponds to the current frame.
//
//------------------------------------------------------------------------------

TID3V2FrameType CID3TagParser::FrameSupportedID3V22(void)
{
    uint8 endTestBuf[ID3V22_FRAME_NUM_BYTES_ID] = {0};
    TID3V2FrameType ID3V2FrameTypeReturnValue;

    if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTT2, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETIT2;
        iTitleFoundFlag           = true;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTP1, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETPE1;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTAL, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETALB;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTCR, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETCOP;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTCO, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETCON;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTRK, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETRCK;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KCOM, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ECOMT;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTYE, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETYER;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTDA, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETDAT;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, KTLE, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = ETLEN;
    }
    else if (oscl_memcmp(iID3TagInfo.iID3V2FrameID, endTestBuf, ID3V22_FRAME_NUM_BYTES_ID) == 0)
    {
        ID3V2FrameTypeReturnValue = EEND;
    }
    else
    {
        ID3V2FrameTypeReturnValue = EFrameNotSupported;
    }

    return ID3V2FrameTypeReturnValue;
}


//----------------------------------------------------------------------------
// FUNCTION NAME: CID3TagParser::EightBitToWideCharBufferTransfer(
//                                               const uint8* aPtrFrameData8,
//                                               uint32         aSize,
//                                               uint32         aEndianType,
//                                               oscl_wchar*    aPtrFrameData16)
//----------------------------------------------------------------------------
// INPUT AND OUTPUT DEFINITIONS
//
//  Inputs:
//    aPtrFrameData8  - pointer to input string format of either big
//                      or little endian.
//    aSize           - number of character elements in aPtrFrameData8
//    aEndianType     - This describes if the encoded Unicode text in the
//                      aPtrFrameData8 buffer is in big or little endian.
//    aPtrFrameData16 - pointer to the output string in unicode format.
//
//  Outputs:
//    aPtrFrameData16 - pointer to the output string in unicode format.
//
//  Returns: None
//
//  Global Variables Used: None
//
//----------------------------------------------------------------------------
// FUNCTION DESCRIPTION
//
//  This function takes in two buffers. One is of type uint8* and the other is
//  of type oscl_wchar*. This function converts the data in the uint8* buffer and
//  puts the data into the oscl_wchar* buffer. The endian is taken care of by this
//  function as well.
//
//------------------------------------------------------------------------------
void CID3TagParser::EightBitToWideCharBufferTransfer(const uint8 * aPtrFrameData8,
        uint32        aSize,
        uint32        aEndianType,
        oscl_wchar  * aPtrFrameDataWCBase)
{
    if (aPtrFrameData8 == NULL || aPtrFrameDataWCBase == NULL)
    {
        return;
    }
    oscl_wchar * aPtrFrameDataWC = aPtrFrameDataWCBase;
    uint16 tempFrameData16;
    oscl_wchar tempFrameDataWC = 0;

    if (aEndianType != 0)
        // Indication check of big-endian vs. little endian
    {
        uint32 z = 0;
        for (uint32 x = 0; x < (aSize >> 1); x++)
        {
            z = x << 1;
            uint8 tempByteOne = aPtrFrameData8[z];
            uint8 tempByteTwo = aPtrFrameData8[z + 1];
            if ((tempByteOne == 0) && (tempByteTwo == 0))
            {
                x++;
                // End of string here and skip to start of next string.
                *aPtrFrameDataWC++ = ((oscl_wchar) ' ');
            }
            else
            {
                tempFrameData16 = ((((uint16)(tempByteTwo << 8)) | tempByteOne));
                tempFrameDataWC = tempFrameData16;
                *aPtrFrameDataWC++ = tempFrameDataWC;
            }
        }
    }
    else
    {
        uint32 z = 0;
        for (uint32 x = 0; x < (aSize >> 1); x++)
        {
            z = x << 1;
            uint8 tempByteOne = aPtrFrameData8[z];
            uint8 tempByteTwo = aPtrFrameData8[z + 1];
            if ((tempByteTwo == 0) && (tempByteOne == 0))
            {
                x++;
                // End of string here and skip to start of next string.
                *aPtrFrameDataWC++ = ((oscl_wchar) ' ');
            }
            else
            {
                tempFrameData16 = ((((uint16)(tempByteOne << 8)) | tempByteTwo));
                tempFrameDataWC = tempFrameData16;
                *aPtrFrameDataWC++ = tempFrameDataWC;
            }
        }
    }
    return;
}

