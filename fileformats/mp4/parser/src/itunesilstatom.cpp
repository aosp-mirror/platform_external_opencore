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
/*********************************************************************************/
/*
	This ITunesILSTAtom Class is used for Parsing, and Storing the tags from Meta data
	of ITune M4A file.
*/

#include "itunesilstatom.h"
#include "atomdefs.h"
#include "atomutils.h"
#include "oscl_int64_utils.h"
#include "oscl_utf8conv.h"

ITunesMetaDataAtom::ITunesMetaDataAtom(MP4_FF_FILE *fp, uint32 size, uint32 type): Atom(fp, size, type)
{

    iLogger = PVLogger::GetLoggerObject("mp4ffparser");
    Oscl_Int64_Utils::set_uint64(STRING_PREFIX, 0x00000001, 0);
    Oscl_Int64_Utils::set_uint64(INTEGER_PREFIX, 0x00000000, 0);
    Oscl_Int64_Utils::set_uint64(OTHER_PREFIX, 0x00000015, 0);
    Oscl_Int64_Utils::set_uint64(IMAGE_PREFIX_JFIF, 0x0000000D, 0);
    Oscl_Int64_Utils::set_uint64(IMAGE_PREFIX_PNG, 0x0000000E, 0);
}

ITunesMetaDataAtom::~ITunesMetaDataAtom()
{

}

//************************************ Title Class Starts  **********************************
ITunesTitleAtom::ITunesTitleAtom(MP4_FF_FILE *fp,
                                 uint32 size,
                                 uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTitleAtom::ITunesTitleAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _name = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesTitleAtom::~ITunesTitleAtom()
{
}


//************************************ Artist / Performer Class Starts  **********************************
ITunesArtistAtom::ITunesArtistAtom(MP4_FF_FILE *fp,
                                   uint32 size,
                                   uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesArtistAtom::ITunesArtistAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _artist = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesArtistAtom::~ITunesArtistAtom()
{
}

//************************************ Album Class Starts  **********************************
ITunesAlbumAtom::ITunesAlbumAtom(MP4_FF_FILE *fp,
                                 uint32 size,
                                 uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesAlbumAtom::ITunesAlbumAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _album = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesAlbumAtom::~ITunesAlbumAtom()
{
}

//************************************ Genre Class Starts  **********************************
ITunesGenreAtom::ITunesGenreAtom(MP4_FF_FILE *fp,
                                 uint32 size,
                                 uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    _gnreString = NULL;

    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            _gnreVersion = STRING_GENRE;
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesGenreAtom::ITunesGenreAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _gnreString = temp;
            }
        }
        else if (_prefix == INTEGER_PREFIX)
        {
            _gnreVersion = INTEGER_GENRE;
            if (!AtomUtils::read16(fp, _gnreID))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesGenreAtom::ITunesGenreAtom READ_ITUNES_ILST_META_DATA_FAILED  _prefix == INTEGER_PREFIX"));
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesGenreAtom::~ITunesGenreAtom()
{
}

//************************************ Day Class Starts  **********************************
ITunesYearAtom::ITunesYearAtom(MP4_FF_FILE *fp,
                               uint32 size,
                               uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesYearAtom::ITunesYearAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _day = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesYearAtom::~ITunesYearAtom()
{
}

//************************************ Tool Class Starts  **********************************
ITunesToolAtom::ITunesToolAtom(MP4_FF_FILE *fp,
                               uint32 size,
                               uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesToolAtom::ITunesToolAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _tool = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}

ITunesToolAtom::~ITunesToolAtom()
{
}

//************************************ Writer Class Starts  **********************************
ITunesWriterAtom::ITunesWriterAtom(MP4_FF_FILE *fp,
                                   uint32 size,
                                   uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesWriterAtom::ITunesWriterAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _writer = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesWriterAtom::~ITunesWriterAtom()
{
}

//************************************ Group Class Starts  **********************************
ITunesGroupAtom::ITunesGroupAtom(MP4_FF_FILE *fp,
                                 uint32 size,
                                 uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesYearAtom::ITunesYearAtom READ_ITUNES_ILST_META_DATA_FAILED"));
                return;
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _group = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesGroupAtom::~ITunesGroupAtom()
{
}

//************************************ Comment Class Starts  **********************************
ITunesCommentAtom::ITunesCommentAtom(MP4_FF_FILE *fp,
                                     uint32 size,
                                     uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesCommentAtom::ITunesCommentAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _comment = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesCommentAtom::~ITunesCommentAtom()
{

}


//************************************ Track Class Starts  **********************************
ITunesTracktAtom::ITunesTracktAtom(MP4_FF_FILE *fp,
                                   uint32 size,
                                   uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == INTEGER_PREFIX)
        {
            uint16 junk; // 2- Bytes representing 0x0000

            if (!AtomUtils::read16(fp, junk))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTracktAtom::ITunesTracktAtom READ_ITUNES_ILST_META_DATA_FAILED  !AtomUtils::read16(fp,junk)"));
                return;
            }

            if (!AtomUtils::read16read16(fp, _thisTrackNo, _totalTracks))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTracktAtom::ITunesTracktAtom READ_ITUNES_ILST_META_DATA_FAILED  !AtomUtils::read16read16(fp, _thisTrackNo, _totalTracks)"));
                return;
            }

            if (!AtomUtils::read16(fp, junk))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTracktAtom::ITunesTracktAtom READ_ITUNES_ILST_META_DATA_FAILED  !AtomUtils::read16(fp,junk)"));
                return;
            }
        }
        else
        {
            _success = false;
            _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTracktAtom::ITunesTracktAtom READ_ITUNES_ILST_META_DATA_FAILED  else "));
            return;
        }
    }
}

ITunesTracktAtom::~ITunesTracktAtom()
{
}

//************************************ Compile Part Class Starts  **********************************
ITunesCompileAtom::ITunesCompileAtom(MP4_FF_FILE *fp,
                                     uint32 size,
                                     uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == OTHER_PREFIX)
        {
            uint8 cplData;
            if (!AtomUtils::read8(fp, cplData))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesCompileAtom::ITunesCompileAtom READ_ITUNES_ILST_META_DATA_FAILED  if(_prefix == OTHER_PREFIX)"));
                return;
            }
            if (cplData)
                _compilationPart = true;
            else
                _compilationPart = false;
        }
        else
        {
            _success = false;
            _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesCompileAtom::ITunesCompileAtom READ_ITUNES_ILST_META_DATA_FAILED  else"));
            return;
        }
    }
}


ITunesCompileAtom::~ITunesCompileAtom()
{
}

//************************************ Tempo Class Starts  **********************************
ITunesTempoAtom::ITunesTempoAtom(MP4_FF_FILE *fp,
                                 uint32 size,
                                 uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == OTHER_PREFIX)
        {
            if (!AtomUtils::read16(fp, _beatsPerMin))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTempoAtom::ITunesTempoAtom READ_ITUNES_ILST_META_DATA_FAILED  if(_prefix == OTHER_PREFIX)"));
                return;
            }
        }
        else
        {
            _success = false;
            _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesTempoAtom::ITunesTempoAtom READ_ITUNES_ILST_META_DATA_FAILED  else"));
            return;
        }
    }
}


ITunesTempoAtom::~ITunesTempoAtom()
{

}

//************************************ Copyright Class Starts  **********************************
ITunesCopyrightAtom::ITunesCopyrightAtom(MP4_FF_FILE *fp,
        uint32 size,
        uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesCopyrightAtom::ITunesCopyrightAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _cprt = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesCopyrightAtom::~ITunesCopyrightAtom()
{

}
//************************************ Description Class Starts  **********************************
ITunesDescriptionAtom::ITunesDescriptionAtom(MP4_FF_FILE *fp,
        uint32 size,
        uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesDescriptionAtom::ITunesDescriptionAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _desc = temp;
            }
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesDescriptionAtom::~ITunesDescriptionAtom()
{
}

//************************************ Disk Data Starts  **********************************
ITunesDiskDatatAtom::ITunesDiskDatatAtom(MP4_FF_FILE *fp,
        uint32 size,
        uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == INTEGER_PREFIX)
        {
            uint16 junk; // 2- Bytes representing 0x0000

            if (!AtomUtils::read16(fp, junk))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesDiskDatatAtom::ITunesDiskDatatAtom READ_ITUNES_ILST_META_DATA_FAILED  if(!AtomUtils::read16(fp,junk))"));
                return;
            }

            if (!AtomUtils::read16read16(fp, _thisDiskNo, _totalDisks))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesDiskDatatAtm::ITunesDiskDatatAtom READ_ITUNES_ILST_META_DATA_FAILED  if(!AtomUtils::read16read16(fp, _thisDiskNo, _totalDisks))"));
                return;
            }
        }
        else
        {
            _success = false;
            _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesDiskDatatAtm::ITunesDiskDatatAtom READ_ITUNES_ILST_META_DATA_FAILED  else"));
            return;
        }
    }
}


ITunesDiskDatatAtom::~ITunesDiskDatatAtom()
{

}

//************************************ Free Form Data Class Starts  **********************************
ITunesFreeFormDataAtom::ITunesFreeFormDataAtom(MP4_FF_FILE *fp,
        uint32 size,
        uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType = type;
    uint32 atomSize = size;
    uint32 nSize;
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));
    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesFreeFormdataAtom::ITunesFreeFormDataAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _StringData = temp;
            }
        }
        else if (_prefix == OTHER_PREFIX)
        {
            // reading the data to keep atom alignment
            uint32 readData;
            if (!AtomUtils::read32(fp, readData))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesFreeFormDataAtom::ITunesFreeFormDataAtom Read four bytes (%d) in OTHER_PREFIX", readData));
            }
        }
        else
        {
            _success = false;
            _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesFreeFormDataAtom::ITunesFreeFormDataAtom READ_ITUNES_ILST_META_DATA_FAILED  else"));
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesFreeFormDataAtom::~ITunesFreeFormDataAtom()
{

}


//************************************ Lyrics Class Starts  **********************************
ITunesLyricsAtom::ITunesLyricsAtom(MP4_FF_FILE *fp,
                                   uint32 size,
                                   uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    AtomUtils::getNextAtomType(fp, atomSize, atomType);
    nSize = atomSize - 16;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        if (_prefix == STRING_PREFIX)
        {
            if (!AtomUtils::readByteData(fp, nSize, buf))
            {
                _success = false;
                _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesLyricsAtom::ITunesLyricsAtom READ_ITUNES_ILST_META_DATA_FAILED"));
            }
            else
            {
                oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                _lyrics = temp;
            }
        }
        else
        {
            _success = false;
            _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesLyricsAtom::ITunesLyricsAtom READ_ITUNES_ILST_META_DATA_FAILED  else"));
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesLyricsAtom::~ITunesLyricsAtom()
{
}

//************************************ CoverImage Class Starts  **********************************
ITunesCoverImageAtom::ITunesCoverImageAtom(MP4_FF_FILE *fp,
        uint32 size,
        uint32 type)
        : ITunesMetaDataAtom(fp, size, type)
{
    uint32 atomType;
    uint32 atomSize;
    uint32 nSize;
    _ImageData = NULL;
    nSize = 3;
    uint8* buf = (uint8*)OSCL_MALLOC(nSize);
    uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));
    AtomUtils::getNextAtomType(fp, atomSize, atomType);

    uint32 count = atomSize;

    count -= DEFAULT_ATOM_SIZE;

    if (atomType == ITUNES_ILST_DATA_ATOM && AtomUtils::read64(fp, _prefix))
    {
        count -= 8;
        if (count < ITUNES_MAX_COVER_IMAGE_SIZE)
        {
            //treat rest of the atom as image
            _ImageData = OSCL_NEW(PvmfApicStruct, ());
            _ImageData->iGraphicData = (uint8*)OSCL_MALLOC(count);
            AtomUtils::readByteData(fp, count, _ImageData->iGraphicData);
            _ImageData->iGraphicDataLen = count;
        }
        else
        {
            // skip rest of the atom
            fp->_pvfile.Seek(count, Oscl_File::SEEKCUR);
            count = 0;
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesCoverImageAtom::ITunesCoverImageAtom READ_ITUNES_ILST_META_DATA_FAILED  else[if (_prefix == IMAGE_PREFIX_PNG)]  )"));
        }
    }
    if (buf)
    {
        OSCL_FREE(buf);
        buf = NULL;
    }
    if (outbuf)
    {
        OSCL_FREE(outbuf);
        outbuf = NULL;
    }
}


ITunesCoverImageAtom::~ITunesCoverImageAtom()
{
    if (_ImageData != NULL)
    {
        if (_ImageData->iGraphicData != NULL)
        {
            OSCL_FREE(_ImageData->iGraphicData);
        }
        OSCL_DELETE(_ImageData);
        _ImageData = NULL;
    }
}
//************************************ Lyrics Class Ends  **********************************
ITunesILSTAtom::ITunesILSTAtom(MP4_FF_FILE *fp, uint32 size, uint32 type): Atom(fp, size, type)
{
    _success = true;

    _pITunesTitleAtom = NULL;
    _pITunesCompileAtom = NULL;
    _pITunesTempoAtom =  NULL;
    _pITunesCopyrightAtom =  NULL;
    _pITunesDescriptionAtom =  NULL;

    _pITunesToolAtom = NULL;
    _pITunesNormalizationFreeFormDataAtom = NULL;
    _pITunesNormalizationFreeFormDataToolAtom = NULL;
    _iITunesCDIdentifierFreeFormDataAtomNum = 0;
    for (uint8 i = 0; i < MAX_CD_IDENTIFIER_FREE_DATA_ATOM; i++)
    {
        _pITunesCDIdentifierFreeFormDataAtom[i] = NULL;
    }
    _pITunesAlbumAtom = NULL;
    _pITunesArtistAtom = NULL;
    _pITunesGenreAtom = NULL;
    _pITunesYearAtom = NULL;
    _pITunesWriterAtom = NULL;
    _pITunesGroupAtom = NULL;
    _pITunesCommentAtom = NULL;
    _pITunesTracktAtom = NULL;
    _pITunesDiskDatatAtom = NULL;
    _pITunesLyricsAtom = NULL;
    _pITunesCoverImageAtom = NULL;

    uint32 count = _size - DEFAULT_ATOM_SIZE;

    iLogger = PVLogger::GetLoggerObject("mp4ffparser");
    while (count > 0)
    {
        uint32 atomsize = 0;
        uint32 atomType = 0;
        uint32 currPtr = AtomUtils::getCurrentFilePosition(fp);
        AtomUtils::getNextAtomType(fp, atomsize, atomType);

        if (count < atomsize)
        {
            AtomUtils::seekFromStart(fp, currPtr);
            AtomUtils::seekFromCurrPos(fp, count);
            count = 0;
            return;
        }

        if (atomType == ITUNES_TITLE_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesTitleAtom, (fp, atomsize, atomType), _pITunesTitleAtom);

            if (!_pITunesTitleAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesTitleAtom, _pITunesTitleAtom);
                _pITunesTitleAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesTitleAtom->getSize();
        }
        else if (atomType == ITUNES_COMPILATION_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesCompileAtom, (fp, atomsize, atomType), _pITunesCompileAtom);

            if (!_pITunesCompileAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesCompileAtom, _pITunesCompileAtom);
                _pITunesCompileAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesCompileAtom->getSize();
        }
        else if (atomType == ITUNES_BPM_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesTempoAtom, (fp, atomsize, atomType), _pITunesTempoAtom);

            if (!_pITunesTempoAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                if (_pITunesTempoAtom != NULL)
                {
                    PV_MP4_FF_DELETE(NULL, ITunesTempoAtom, _pITunesTempoAtom);
                    _pITunesTempoAtom = NULL;
                }
                count -= atomsize;
            }
            else
                count -= _pITunesTempoAtom->getSize();
        }
        else if (atomType == ITUNES_COPYRIGHT_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesCopyrightAtom, (fp, atomsize, atomType), _pITunesCopyrightAtom);

            if (!_pITunesCopyrightAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesCopyrightAtom, _pITunesCopyrightAtom);
                _pITunesCopyrightAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesCopyrightAtom->getSize();
        }
        else if (atomType == ITUNES_DESCRIPTION_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesDescriptionAtom, (fp, atomsize, atomType), _pITunesDescriptionAtom);

            if (!_pITunesDescriptionAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesDescriptionAtom, _pITunesDescriptionAtom);
                _pITunesDescriptionAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesDescriptionAtom->getSize();
        }
        else if (atomType == ITUNES_ENCODER_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesToolAtom, (fp, atomsize, atomType), _pITunesToolAtom);

            if (!_pITunesToolAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesToolAtom, _pITunesToolAtom);
                _pITunesToolAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesToolAtom->getSize();
        }
#if 0
/* Disable this code for now, as there is an off-by-X bug in the free form atom
 * parsing that causes subsequent atoms to be misparsed, which makes us not see
 * metadata for some files purchased from the itunes music store
 */
        else if (atomType == ITUNES_FREE_FORM_ATOM)
        {
            uint32 FreeAtomsize = 0;
            uint32 FreeAtomType = 0;
            uint32 nSize = 0;
            count -= DEFAULT_ATOM_SIZE;
            AtomUtils::getNextAtomType(fp, FreeAtomsize, FreeAtomType);
            count -= 8;

            nSize = 16;
            uint8* buf = (uint8*)OSCL_MALLOC(nSize);
            uint8* outbuf = (uint8*)OSCL_MALLOC((nSize + 1) * sizeof(oscl_wchar));

            if (FreeAtomType == ITUENES_MEAN_ATOM)
            {
                uint32 _data;
                OSCL_wHeapString<OsclMemAllocator> meanData;

                if (!AtomUtils::read32(fp, _data))
                {
                    _success = false;
                    _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                    PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesILSTAtom::ITunesILSTAtom READ_ITUNES_ILST_META_DATA_FAILED  if(!AtomUtils::read32(fp,_data))  )"));
                    if (buf)
                    {
                        OSCL_FREE(buf);
                        buf = NULL;
                    }
                    if (outbuf)
                    {
                        OSCL_FREE(outbuf);
                        outbuf = NULL;
                    }
                    return;
                }
                count -= 4;

                if (!AtomUtils::readByteData(fp, nSize, buf))
                {
                    _success = false;
                    _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                    PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesCommentAtom::ITunesCommentAtom READ_ITUNES_ILST_META_DATA_FAILED"));
                    if (buf)
                    {
                        OSCL_FREE(buf);
                        buf = NULL;
                    }
                    if (outbuf)
                    {
                        OSCL_FREE(outbuf);
                        outbuf = NULL;
                    }
                    return;
                }
                else
                {
                    oscl_UTF8ToUnicode((const char *)buf, nSize, (oscl_wchar*)outbuf, nSize + 1);
                    OSCL_wHeapString<OsclMemAllocator> temp((const oscl_wchar *)outbuf);
                    meanData = temp;
                }
                count -= 16;
            }
            if (buf)
            {
                OSCL_FREE(buf);
                buf = NULL;
            }
            if (outbuf)
            {
                OSCL_FREE(outbuf);
                outbuf = NULL;
            }

            uint32 FDNAtomsize = 0;
            uint32 FDNAtomType = 0;
            uint32 read_count = 0;

            AtomUtils::getNextAtomType(fp, FDNAtomsize, FDNAtomType);
            count -= 8;

            if (FDNAtomType == ITUNES_FREE_FORM_DATA_NAME_ATOM)
            {
                uint32 _data;
                if (!AtomUtils::read32(fp, _data))
                {
                    _success = false;
                    _mp4ErrorCode = READ_ITUNES_ILST_META_DATA_FAILED;
                    PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesILSTAtom::ITunesILSTAtom READ_ITUNES_ILST_META_DATA_FAILED  if(!AtomUtils::read32(fp,_data))  )"));
                    return;
                }
                count -= 4;
                read_count += 4;

                uint32 FreeFormType_Part1 = 0;
                uint32 FreeFormType_Part2 = 0;

                if (!AtomUtils::read32(fp, FreeFormType_Part1))
                {
                    _success = false;
                    _mp4ErrorCode = READ_META_DATA_FAILED;
                    PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesILSTAtom::ITunesILSTAtom READ_ITUNES_ILST_META_DATA_FAILED  if(!AtomUtils::read32(fp,_data))  )"));
                    return;
                }
                count -= 4;
                read_count += 4;

                if (FreeFormType_Part1 == ITUNES_FREE_FORM_DATA_ATOM_TYPE_TOOL)
                {
                    uint32 normDataAtomSize = 0;
                    uint32 normDataAtomType = 0;
                    uint32 Currpos = AtomUtils::getCurrentFilePosition(fp);
                    AtomUtils::getNextAtomType(fp, normDataAtomSize, normDataAtomType);
                    PV_MP4_FF_NEW(fp->auditCB,
                                  ITunesFreeFormDataAtom,
                                  (fp, normDataAtomSize, normDataAtomType),
                                  _pITunesNormalizationFreeFormDataToolAtom);
                    if (!_pITunesNormalizationFreeFormDataToolAtom->MP4Success())
                    {
                        AtomUtils::seekFromStart(fp, Currpos);
                        AtomUtils::seekFromCurrPos(fp, atomsize);
                    }
                    count -= _pITunesNormalizationFreeFormDataToolAtom->getSize();
                }
                else
                {
                    if (!AtomUtils::read32(fp, FreeFormType_Part2))
                    {
                        _success = false;
                        _mp4ErrorCode = READ_META_DATA_FAILED;
                        PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>ITunesILSTAtom::ITunesILSTAtom READ_ITUNES_ILST_META_DATA_FAILED  if(!AtomUtils::read32(fp,_data))  )"));
                        return;
                    }
                    count -= 4;
                    read_count += 4;

                    if (FreeFormType_Part1 == ITUNES_FREE_FORM_DATA_ATOM_TYPE_PART1 &&
                            FreeFormType_Part2 == ITUNES_FREE_FORM_DATA_ATOM_TYPE_PART2)
                    {
                        uint32 normDataAtomSize = 0;
                        uint32 normDataAtomType = 0;
                        uint32 Currpos = AtomUtils::getCurrentFilePosition(fp);
                        AtomUtils::getNextAtomType(fp, normDataAtomSize, normDataAtomType);
                        PV_MP4_FF_NEW(fp->auditCB,
                                      ITunesFreeFormDataAtom,
                                      (fp, normDataAtomSize, normDataAtomType),
                                      _pITunesNormalizationFreeFormDataAtom);
                        if (!_pITunesNormalizationFreeFormDataAtom->MP4Success())
                        {
                            AtomUtils::seekFromStart(fp, Currpos);
                            AtomUtils::seekFromCurrPos(fp, atomsize);
                        }
                        count -= _pITunesNormalizationFreeFormDataAtom->getSize();
                    }
                    else if (FreeFormType_Part2 == ITUNES_FREE_FORM_DATA_ATOM_TYPE_CDDB)
                    {
                        //skipping rest of "iTunes_CDDB_Ids"
                        uint32 temp_size = FDNAtomsize - (read_count + DEFAULT_ATOM_SIZE);
                        uint8 *temp_bytes;
                        uint8 index = 0;
                        temp_bytes = (uint8*)OSCL_MALLOC(temp_size);
                        if (temp_bytes)	// malloc can fail
                        {
                            while (index < temp_size)
                            {
                                AtomUtils::read8(fp, temp_bytes[index++]);
                            }
                            OSCL_FREE(temp_bytes);
                            temp_bytes = NULL;
                        }
                        count -= temp_size;

                        uint32 cdIDDataAtomSize = 0;
                        uint32 cdIDDataAtomType = 0;
                        uint32 Currpos = AtomUtils::getCurrentFilePosition(fp);
                        AtomUtils::getNextAtomType(fp, cdIDDataAtomSize, cdIDDataAtomType);
                        PV_MP4_FF_NEW(fp->auditCB,
                                      ITunesFreeFormDataAtom,
                                      (fp, cdIDDataAtomSize, cdIDDataAtomType),
                                      _pITunesCDIdentifierFreeFormDataAtom[_iITunesCDIdentifierFreeFormDataAtomNum]);

                        if (!_pITunesCDIdentifierFreeFormDataAtom[_iITunesCDIdentifierFreeFormDataAtomNum]->MP4Success())
                        {
                            AtomUtils::seekFromStart(fp, Currpos);
                            AtomUtils::seekFromCurrPos(fp, atomsize);
                        }
                        count -= _pITunesCDIdentifierFreeFormDataAtom[_iITunesCDIdentifierFreeFormDataAtomNum]->getSize();
                        _iITunesCDIdentifierFreeFormDataAtomNum++;
                    }
                    else
                    {
                        uint32 unknownFormAtomSize = 0;
                        uint32 unknownFormAtomType = 0;
                        AtomUtils::getNextAtomType(fp, unknownFormAtomSize, unknownFormAtomType);
                        //skip
                        if (unknownFormAtomSize > DEFAULT_ATOM_SIZE)
                        {
                            count -= unknownFormAtomSize;
                            unknownFormAtomSize -= DEFAULT_ATOM_SIZE;
                            fp->_pvfile.Seek(unknownFormAtomSize, Oscl_File::SEEKCUR);
                        }
                        else
                        {
                            // skip rest of the atom
                            fp->_pvfile.Seek(count, Oscl_File::SEEKCUR);
                            count = 0;
                        }

                    }
                }
            }
        }
#endif
        else if (atomType == ITUNES_ALBUM_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesAlbumAtom, (fp, atomsize, atomType), _pITunesAlbumAtom);

            if (!_pITunesAlbumAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesAlbumAtom, _pITunesAlbumAtom);
                _pITunesAlbumAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesAlbumAtom->getSize();
        }
        else if (atomType == ITUNES_ARTIST_ATOM || atomType == ITUNES_ALBUM_ARTIST_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesArtistAtom, (fp, atomsize, atomType), _pITunesArtistAtom);

            if (!_pITunesArtistAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesArtistAtom, _pITunesArtistAtom);
                _pITunesArtistAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesArtistAtom->getSize();
        }
        else if (atomType == ITUNES_GENRE1_ATOM || atomType == ITUNES_GENRE2_ATOM)
        {
            if (_pITunesGenreAtom == NULL)
            {
                PV_MP4_FF_NEW(fp->auditCB, ITunesGenreAtom, (fp, atomsize, atomType), _pITunesGenreAtom);
                if (!_pITunesGenreAtom->MP4Success())
                {
                    AtomUtils::seekFromStart(fp, currPtr);
                    AtomUtils::seekFromCurrPos(fp, atomsize);
                    PV_MP4_FF_DELETE(NULL, ITunesGenreAtom, _pITunesGenreAtom);
                    _pITunesGenreAtom = NULL;
                    count -= atomsize;
                }
                else
                    count -= _pITunesGenreAtom->getSize();
            }
            else
            {
                count -= atomsize;
                atomsize -= DEFAULT_ATOM_SIZE;
                fp->_pvfile.Seek(atomsize, Oscl_File::SEEKCUR);
            }
        }
        else if (atomType == ITUNES_YEAR_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesYearAtom, (fp, atomsize, atomType), _pITunesYearAtom);

            if (!_pITunesYearAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesYearAtom, _pITunesYearAtom);
                _pITunesYearAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesYearAtom->getSize();
        }
        else if (atomType == ITUNES_COMPOSER_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesWriterAtom, (fp, atomsize, atomType), _pITunesWriterAtom);

            if (!_pITunesWriterAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesWriterAtom, _pITunesWriterAtom);
                _pITunesWriterAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesWriterAtom->getSize();
        }
        else if (atomType == ITUNES_GROUPING_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesGroupAtom, (fp, atomsize, atomType), _pITunesGroupAtom);

            if (!_pITunesGroupAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesGroupAtom, _pITunesGroupAtom);
                _pITunesGroupAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesGroupAtom->getSize();
        }
        else if (atomType == ITUNES_COMMENT_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesCommentAtom, (fp, atomsize, atomType), _pITunesCommentAtom);

            if (!_pITunesCommentAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesCommentAtom, _pITunesCommentAtom);
                _pITunesCommentAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesCommentAtom->getSize();
        }
        else if (atomType == ITUNES_TRACK_NUMBER_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesTracktAtom, (fp, atomsize, atomType), _pITunesTracktAtom);

            if (!_pITunesTracktAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesTracktAtom, _pITunesTracktAtom);
                _pITunesTracktAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesTracktAtom->getSize();
        }
        else if (atomType == ITUNES_ART_WORK_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesCoverImageAtom, (fp, atomsize, atomType), _pITunesCoverImageAtom);

            if (!_pITunesCoverImageAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesCoverImageAtom, _pITunesCoverImageAtom);
                _pITunesCoverImageAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesCoverImageAtom->getSize();
        }
        else if (atomType == ITUNES_DISK_NUMBER_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesDiskDatatAtom, (fp, atomsize, atomType), _pITunesDiskDatatAtom);

            if (!_pITunesDiskDatatAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesDiskDatatAtom, _pITunesDiskDatatAtom);
                _pITunesDiskDatatAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesDiskDatatAtom->getSize();
        }
        else if (atomType == ITUNES_LYRICS_ATOM)
        {
            PV_MP4_FF_NEW(fp->auditCB, ITunesLyricsAtom, (fp, atomsize, atomType), _pITunesLyricsAtom);

            if (!_pITunesLyricsAtom->MP4Success())
            {
                AtomUtils::seekFromStart(fp, currPtr);
                AtomUtils::seekFromCurrPos(fp, atomsize);
                PV_MP4_FF_DELETE(NULL, ITunesLyricsAtom, _pITunesLyricsAtom);
                _pITunesLyricsAtom = NULL;
                count -= atomsize;
            }
            else
                count -= _pITunesLyricsAtom->getSize();
        }
        else
        {
            if (atomsize > DEFAULT_ATOM_SIZE)
            {
                count -= atomsize;
                atomsize -= DEFAULT_ATOM_SIZE;
                fp->_pvfile.Seek(atomsize, Oscl_File::SEEKCUR);
            }
            else
            {
                // skip rest of the atom
                fp->_pvfile.Seek(count, Oscl_File::SEEKCUR);
                count = 0;
            }
        }
    }
}

ITunesILSTAtom::~ITunesILSTAtom()
{
    if (_pITunesTitleAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesTitleAtom, _pITunesTitleAtom);
    }
    if (_pITunesCompileAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesCompileAtom, _pITunesCompileAtom);
    }
    if (_pITunesTempoAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesTempoAtom, _pITunesTempoAtom);
    }
    if (_pITunesCopyrightAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesCopyrightAtom, _pITunesCopyrightAtom);
    }
    if (_pITunesDescriptionAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesDescriptionAtom, _pITunesDescriptionAtom);
    }
    if (_pITunesToolAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesToolAtom, _pITunesToolAtom);
    }
    if (_pITunesNormalizationFreeFormDataAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesFreeFormDataAtom, _pITunesNormalizationFreeFormDataAtom);
    }
    if (_pITunesNormalizationFreeFormDataToolAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesFreeFormDataAtom, _pITunesNormalizationFreeFormDataToolAtom);
    }
    for (uint8 ii = 0; ii < _iITunesCDIdentifierFreeFormDataAtomNum; ii++)
    {
        if (_pITunesCDIdentifierFreeFormDataAtom[ii] != NULL)
        {
            PV_MP4_FF_DELETE(NULL, ITunesFreeFormDataAtom, _pITunesCDIdentifierFreeFormDataAtom[ii]);
        }
    }
    if (_pITunesAlbumAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesAlbumAtom, _pITunesAlbumAtom);
    }
    if (_pITunesArtistAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesArtistAtom, _pITunesArtistAtom);
    }
    if (_pITunesGenreAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesGenreAtom, _pITunesGenreAtom);
    }
    if (_pITunesYearAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesYearAtom, _pITunesYearAtom);
    }
    if (_pITunesWriterAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesWriterAtom, _pITunesWriterAtom);
    }
    if (_pITunesGroupAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesGroupAtom, _pITunesGroupAtom);
    }
    if (_pITunesCommentAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesCommentAtom, _pITunesCommentAtom);
    }
    if (_pITunesTracktAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesTracktAtom, _pITunesTracktAtom);
    }
    if (_pITunesDiskDatatAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesDiskDatatAtom, _pITunesDiskDatatAtom);
    }
    if (_pITunesLyricsAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesLyricsAtom, _pITunesLyricsAtom);
    }
    if (_pITunesCoverImageAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, ITunesCoverImageAtom, _pITunesCoverImageAtom);
    }
}


