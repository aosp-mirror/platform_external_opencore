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
/*     -------------------------------------------------------------------       */
/*                        MPEG-4 CompositionOffsetAtom Class                          */
/*     -------------------------------------------------------------------       */
/*********************************************************************************/

/*
This atom gives difference between decoding time and composition time on each
sample basis. This atom is optional and must be present only if Decoding time
and Composition Time differ for any samples. As understood that Decoding time
is always less than composition time, the offsets are termed as unsigned
numbers such.

*/


#define IMPLEMENT_CompositionOffsetAtom

#include "compositionoffsetatom.h"
#include "atomutils.h"

// Stream-in ctor
CompositionOffsetAtom::CompositionOffsetAtom(MP4_FF_FILE *fp,
        uint32 mediaType,
        uint32 size,
        uint32 type,
        OSCL_wString& filename,
        uint32 parsingMode)
        : FullAtom(fp, size, type)
{
    _psampleCountVec = NULL;
    _psampleOffsetVec = NULL;

    _currGetSampleCount = 0;
    _currGetIndex = -1;
    _currGetTimeOffset = 0;
    _currPeekSampleCount = 0;
    _currPeekIndex = -1;
    _currPeekTimeOffset = 0;

    _mediaType = mediaType;
    //_head_offset = 0;
    _parsed_entry_cnt = 0;
    _fileptr = NULL;
    _parsing_mode = 0;
    _parsing_mode = parsingMode;

    _stbl_buff_size = 4096;
    _next_buff_number = 0;
    _curr_buff_number = 0;
    _curr_entry_point = 0;
    _stbl_fptr_vec = NULL;

    iLogger = PVLogger::GetLoggerObject("mp4ffparser");
    iStateVarLogger = PVLogger::GetLoggerObject("mp4ffparser_mediasamplestats");
    iParsedDataLogger = PVLogger::GetLoggerObject("mp4ffparser_parseddata");


    if (_success)
    {
        if (!AtomUtils::read32(fp, _entryCount))
        {
            _success = false;
        }

        PVMF_MP4FFPARSER_LOGPARSEDINFO((0, "CompositionOffsetAtom::CompositionOffsetAtom- _entryCount =%d", _entryCount));
        uint32 dataSize = _size - (DEFAULT_FULL_ATOM_SIZE + 4);

        uint32 entrySize = (4 + 4);

        if ((_entryCount*entrySize) > dataSize)
        {
            _success = false;
        }

        if (_success)
        {
            if (_entryCount > 0)
            {
                if (parsingMode == 1)
                {
                    if ((_entryCount > 4096)) // cahce size is 4K so that optimization
                        // should work if entry_count is greater than 4K
                    {
                        uint32 fptrBuffSize = (_entryCount / _stbl_buff_size) + 1;

                        PV_MP4_FF_ARRAY_NEW(NULL, uint32, (fptrBuffSize), _stbl_fptr_vec);
                        if (_stbl_fptr_vec == NULL)
                        {
                            _success = false;
                            _mp4ErrorCode = MEMORY_ALLOCATION_FAILED;
                            return;
                        }

                        PV_MP4_FF_ARRAY_NEW(NULL, uint32, (_stbl_buff_size), _psampleCountVec);
                        if (_psampleCountVec == NULL)
                        {
                            _success = false;
                            _mp4ErrorCode = MEMORY_ALLOCATION_FAILED;
                            return;
                        }

                        PV_MP4_FF_ARRAY_NEW(NULL, uint32, (_stbl_buff_size), _psampleOffsetVec);
                        if (_psampleOffsetVec == NULL)
                        {
                            PV_MP4_ARRAY_DELETE(NULL, _psampleOffsetVec);
                            _psampleOffsetVec = NULL;
                            _success = false;
                            _mp4ErrorCode = MEMORY_ALLOCATION_FAILED;
                            return;
                        }

                        for (uint32 idx = 0; idx < _stbl_buff_size; idx++)  //initialization
                        {
                            _psampleCountVec[idx] = 0;
                            _psampleOffsetVec[idx] = 0;
                        }

                        OsclAny* ptr = (MP4_FF_FILE *)(oscl_malloc(sizeof(MP4_FF_FILE)));
                        if (ptr == NULL)
                        {
                            _success = false;
                            _mp4ErrorCode = MEMORY_ALLOCATION_FAILED;
                            return;
                        }

                        _fileptr = OSCL_PLACEMENT_NEW(ptr, MP4_FF_FILE());
                        _fileptr->_fileServSession = fp->_fileServSession;
                        _fileptr->_pvfile.SetCPM(fp->_pvfile.GetCPM());

                        if (AtomUtils::OpenMP4File(filename,
                                                   Oscl_File::MODE_READ | Oscl_File::MODE_BINARY,
                                                   _fileptr) != 0)
                        {
                            _success = false;
                            _mp4ErrorCode = FILE_OPEN_FAILED;
                        }

                        _fileptr->_fileSize = fp->_fileSize;

                        int32 _head_offset = AtomUtils::getCurrentFilePosition(fp);
                        AtomUtils::seekFromCurrPos(fp, dataSize);
                        AtomUtils::seekFromStart(_fileptr, _head_offset);
                        return;
                    }
                    else
                    {
                        _parsing_mode = 0;
                        _stbl_buff_size = _entryCount;
                    }
                }
                else
                {
                    _stbl_buff_size = _entryCount;
                }

                PV_MP4_FF_ARRAY_NEW(NULL, uint32, (_entryCount), _psampleCountVec);
                if (_psampleCountVec == NULL)
                {
                    _success = false;
                    _mp4ErrorCode = MEMORY_ALLOCATION_FAILED;
                    return;
                }

                PV_MP4_FF_ARRAY_NEW(NULL, uint32, (_entryCount), _psampleOffsetVec);
                if (_psampleOffsetVec == NULL)
                {
                    PV_MP4_ARRAY_DELETE(NULL, _psampleOffsetVec);
                    _psampleOffsetVec = NULL;
                    _success = false;
                    _mp4ErrorCode = MEMORY_ALLOCATION_FAILED;
                    return;
                }

                for (uint32 idx = 0; idx < _entryCount; idx++)  //initialization
                {
                    _psampleCountVec[idx] = 0;
                    _psampleOffsetVec[idx] = 0;
                }

                uint32 number = 0;
                uint32 offset = 0;
                for (_parsed_entry_cnt = 0; _parsed_entry_cnt < _entryCount; _parsed_entry_cnt++)
                {
                    if (!AtomUtils::read32(fp, number))
                    {
                        _success = false;
                        break;
                    }
                    if (!AtomUtils::read32(fp, offset))
                    {
                        _success = false;
                        break;
                    }
                    _psampleCountVec[_parsed_entry_cnt] = (number);
                    _psampleOffsetVec[_parsed_entry_cnt] = (offset);
                }
            }
        }

        if (!_success)
        {
            _mp4ErrorCode = READ_TIME_TO_SAMPLE_ATOM_FAILED;
        }
    }
    else
    {
        if (_mp4ErrorCode != ATOM_VERSION_NOT_SUPPORTED)
            _mp4ErrorCode = READ_TIME_TO_SAMPLE_ATOM_FAILED;
    }
}

bool CompositionOffsetAtom::ParseEntryUnit(uint32 entry_cnt)
{
    const uint32 threshold = 1024;
    entry_cnt += threshold;

    if (entry_cnt > _entryCount)
        entry_cnt = _entryCount;

    uint32 number, offset;
    while (_parsed_entry_cnt < entry_cnt)
    {
        _curr_entry_point = _parsed_entry_cnt % _stbl_buff_size;
        _curr_buff_number = _parsed_entry_cnt / _stbl_buff_size;

        if (_curr_buff_number  == _next_buff_number)
        {
            uint32 currFilePointer = AtomUtils::getCurrentFilePosition(_fileptr);
            _stbl_fptr_vec[_curr_buff_number] = currFilePointer;
            _next_buff_number++;
        }

        if (!_curr_entry_point)
        {
            uint32 currFilePointer = _stbl_fptr_vec[_curr_buff_number];
            AtomUtils::seekFromStart(_fileptr, currFilePointer);
        }

        if (!AtomUtils::read32(_fileptr, number))
        {
            return false;
        }

        if (!AtomUtils::read32(_fileptr, offset))
        {
            return false;
        }
        _psampleCountVec[_curr_entry_point] = (number);
        _psampleOffsetVec[_curr_entry_point] = (offset);
        _parsed_entry_cnt++;
    }
    return true;
}

// Destructor
CompositionOffsetAtom::~CompositionOffsetAtom()
{
    if (_psampleCountVec != NULL)
        PV_MP4_ARRAY_DELETE(NULL, _psampleCountVec);

    if (_psampleOffsetVec != NULL)
        PV_MP4_ARRAY_DELETE(NULL, _psampleOffsetVec);

    if (_stbl_fptr_vec != NULL)
        PV_MP4_ARRAY_DELETE(NULL, _stbl_fptr_vec);

    if (_fileptr != NULL)
    {
        if (_fileptr->IsOpen())
        {
            AtomUtils::CloseMP4File(_fileptr);
        }
        oscl_free(_fileptr);
    }
}

// Return the number of samples  at index
uint32
CompositionOffsetAtom::getSampleCountAt(int32 index)
{
    if (_psampleCountVec == NULL)
    {
        return (uint32) PV_ERROR;
    }

    if (index < (int32)_entryCount)
    {
        if (_parsing_mode == 1)
            CheckAndParseEntry(index);

        return (int32)(_psampleCountVec[index%_stbl_buff_size]);
    }
    else
    {
        PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>CompositionOffsetAtom::getSampleCountAt index = %d", index));
        return (uint32) PV_ERROR;
    }
}

// Return sample offset at index
int32
CompositionOffsetAtom::getSampleOffsetAt(int32 index)
{
    if (_psampleOffsetVec == NULL)
    {
        return PV_ERROR;
    }

    if (index < (int32)_entryCount)
    {
        if (_parsing_mode == 1)
            CheckAndParseEntry(index);

        return (int32)(_psampleOffsetVec[index%_stbl_buff_size]);
    }
    else
    {
        PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>CompositionOffsetAtom::getSampleOffsetAt index = %d", index));
        return PV_ERROR;
    }
}

int32
CompositionOffsetAtom::getTimeOffsetForSampleNumberPeek(uint32 sampleNum)
{
    // It is assumed that sample 0 has a ts of 0 - i.e. the first
    // entry in the table starts with the delta between sample 1 and sample 0
    if ((_psampleOffsetVec == NULL) ||
            (_psampleCountVec == NULL) ||
            (_entryCount == 0))
    {
        return PV_ERROR;
    }

    // note that sampleNum is a zero based index while _currGetSampleCount is 1 based index
    if (sampleNum < _currPeekSampleCount)
    {
        return (_currPeekTimeOffset);
    }
    else
    {
        do
        {
            _currPeekIndex++;
            if (_parsing_mode)
                CheckAndParseEntry(_currPeekIndex);

            _currPeekSampleCount += _psampleCountVec[_currPeekIndex%_stbl_buff_size];
            _currPeekTimeOffset    = _psampleOffsetVec[_currPeekIndex%_stbl_buff_size];
        }
        while (_currPeekSampleCount == 0);

        PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimeOffsetForSampleNumberPeek- _currPeekIndex =%d", _currPeekIndex));
        PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimeOffsetForSampleNumberPeek- _currPeekSampleCount =%d", _currPeekSampleCount));
        PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimeOffsetForSampleNumberPeek- _currPeekTimeOffset =%d", _currPeekTimeOffset));

        if (sampleNum < _currPeekSampleCount)
        {
            return (_currPeekTimeOffset);
        }
        else
        {
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>CompositionOffsetAtom::getTimeOffsetForSampleNumberPeek sampleNum = %d", sampleNum));
            return (PV_ERROR);
        }
    }
}

// This is the most widely used API
// Returns the offset (ms) for the  sample given by num.  This is used when
// randomly accessing a frame and the timestamp has not been accumulated.
int32 CompositionOffsetAtom::getTimeOffsetForSampleNumberGet(uint32 sampleNum)
{
    if ((_psampleOffsetVec == NULL) ||
            (_psampleCountVec == NULL) ||
            (_entryCount == 0))
    {
        return PV_ERROR;
    }

    // note that sampleNum is a zero based index while _currGetSampleCount is 1 based index
    if (sampleNum < _currGetSampleCount)
    {
        return (_currGetTimeOffset);
    }
    else
    {

        do
        {
            _currGetIndex++;
            if (_parsing_mode)
                CheckAndParseEntry(_currGetIndex);
            _currGetSampleCount += _psampleCountVec[_currGetIndex%_stbl_buff_size];
            _currGetTimeOffset    = _psampleOffsetVec[_currGetIndex%_stbl_buff_size];
        }
        while (_currGetSampleCount == 0);

        PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimeOffsetForSampleNumberGet- _currGetIndex =%d", _currGetIndex));
        PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimeOffsetForSampleNumberGet- _currGetSampleCount =%d", _currGetSampleCount));
        PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimeOffsetForSampleNumberGet- _currGetTimeOffset =%d", _currGetTimeOffset));

        if (sampleNum < _currGetSampleCount)
        {
            return (_currGetTimeOffset);
        }
        else
        {
            PVMF_MP4FFPARSER_LOGERROR((0, "ERROR =>TimeToSampleAtom::getTimeDeltaForSampleNumberGet sampleNum = %d", sampleNum));
            return (PV_ERROR);
        }
    }
}

// Returns the offset (ms) for the  sample given by num.  This is used when
// randomly accessing a frame and the timestamp has not been accumulated.

int32 CompositionOffsetAtom::getTimeOffsetForSampleNumber(uint32 num)
{
    if ((_psampleOffsetVec == NULL) ||
            (_psampleCountVec == NULL) ||
            (_entryCount == 0))
    {
        return PV_ERROR;
    }

    uint32 sampleCount = 0;
    int32 offset = 0; // Offset value to return

    for (uint32 i = 0; i < _entryCount; i++)
    {
        if (_parsing_mode == 1)
            CheckAndParseEntry(i);

        if (num < (sampleCount + _psampleCountVec[i%_stbl_buff_size]))
        { // Sample num within current entry
            int32 count = num - sampleCount;
            PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "CompositionOffsetAtom::getTimestampForSampleNumber- Time StampOffset = %d", _psampleOffsetVec[i%_stbl_buff_size]));
            return _psampleOffsetVec[i%_stbl_buff_size];
        }
        else
        {
            sampleCount += _psampleCountVec[i%_stbl_buff_size];
        }
    }

    // Went past end of list - not a valid sample number
    return PV_ERROR;
}

int32
CompositionOffsetAtom::resetStateVariables()
{
    uint32 sampleNum = 0;
    return (resetStateVariables(sampleNum));
}

int32
CompositionOffsetAtom::resetStateVariables(uint32 sampleNum)
{
    _currGetSampleCount = 0;
    _currGetIndex = -1;
    _currGetTimeOffset = 0;
    _currPeekSampleCount = 0;
    _currPeekIndex = -1;
    _currPeekTimeOffset = 0;

    // It is assumed that sample 0 has a ts of 0 - i.e. the first
    // entry in the table starts with the delta between sample 1 and sample 0
    if ((_psampleOffsetVec == NULL) ||
            (_psampleCountVec == NULL) ||
            (_entryCount == 0))
    {
        return PV_ERROR;
    }

    if (_parsing_mode)
    {
        if (_parsed_entry_cnt == 0)
        {
            ParseEntryUnit(sampleNum);
        }
    }

    for (uint32 i = 0; i < _entryCount; i++)
    {
        _currPeekIndex++;
        _currPeekSampleCount += _psampleCountVec[i%_stbl_buff_size];
        _currPeekTimeOffset    = _psampleOffsetVec[i%_stbl_buff_size];

        _currGetIndex++;
        _currGetSampleCount += _psampleCountVec[i%_stbl_buff_size];
        _currGetTimeOffset    = _psampleOffsetVec[i%_stbl_buff_size];

        if (sampleNum <= _currPeekSampleCount)
        {
            return (EVERYTHING_FINE);
        }
    }

    // Went past end of list - not a valid sample number
    return PV_ERROR;
}

int32 CompositionOffsetAtom::resetPeekwithGet()
{
    _currPeekSampleCount = _currGetSampleCount;
    _currPeekIndex = _currGetIndex;
    _currPeekTimeOffset = _currGetTimeOffset;
    return (EVERYTHING_FINE);
}

void CompositionOffsetAtom::CheckAndParseEntry(uint32 i)
{
    if (i >= _parsed_entry_cnt)
    {
        ParseEntryUnit(i);
    }
    else
    {
        uint32 entryLoc = i / _stbl_buff_size;
        if (_curr_buff_number != entryLoc)
        {
            _parsed_entry_cnt = entryLoc * _stbl_buff_size;
            while (_parsed_entry_cnt <= i)
                ParseEntryUnit(_parsed_entry_cnt);
        }
    }
}

