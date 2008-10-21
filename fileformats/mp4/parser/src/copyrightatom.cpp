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
#include "copyrightatom.h"
#include "atomutils.h"
#include "atomdefs.h"

#define BYTE_ORDER_MASK 0xFEFF

CopyRightAtom::CopyRightAtom(MP4_FF_FILE *fp, uint32 size, uint32 type)
        : FullAtom(fp, size, type)
{
    uint32 count = getDefaultSize();

    if (_success)
    {
        if (!AtomUtils::read16(fp, _language_code))
        {
            _success = false;
            _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
        }

        count += 2;

        if (count < _size)
        {
            uint32 temp = AtomUtils::peekNextNthBytes(fp, 1);

            uint16 byteOrderMask = (uint16)((temp >> 16) & 0xFFFF);

            if (byteOrderMask == BYTE_ORDER_MASK)
            {
                if (!AtomUtils::read16(fp, byteOrderMask))
                {
                    _success = false;
                    _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                    return;
                }
                count += 2;

                if (count < _size)
                {
                    // Check to see if the string is actually null-terminated
                    uint32 delta = (_size - count);

                    int32 filePos = AtomUtils::getCurrentFilePosition(fp);

                    AtomUtils::seekFromCurrPos(fp, (delta - 2));

                    uint16 strEnd = 0;

                    if (!AtomUtils::read16(fp, strEnd))
                    {
                        _success = false;
                        _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                        return;
                    }

                    if (strEnd == 0)
                    {
                        AtomUtils::seekFromStart(fp, filePos);

                        if (!AtomUtils::readNullTerminatedUnicodeString(fp, _copyRightNotice))
                        {
                            _success = false;
                            _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                            return;
                        }
                        {
                            int32 newfilePos =
                                AtomUtils::getCurrentFilePosition(fp);

                            if (newfilePos != (int32)(filePos + delta))
                            {
                                AtomUtils::seekFromStart(fp, filePos + delta);
                            }
                        }
                    }
                    count += delta;
                }
                else
                {
                    _success = false;
                    _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                    return;
                }
            }
            else
            {
                if (count < _size)
                {
                    // Check to see if the string is actually null-terminated

                    uint32 delta = (_size - count);

                    int32 filePos = AtomUtils::getCurrentFilePosition(fp);

                    AtomUtils::seekFromCurrPos(fp, (delta - 1));

                    uint8 strEnd = 0;

                    if (!AtomUtils::read8(fp, strEnd))
                    {
                        _success = false;
                        _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                        return;
                    }

                    if (strEnd == 0)
                    {
                        AtomUtils::seekFromStart(fp, filePos);

                        if (!AtomUtils::readNullTerminatedString(fp, _copyRightNotice))
                        {
                            _success = false;
                            _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                            return;
                        }
                        {
                            int32 newfilePos =
                                AtomUtils::getCurrentFilePosition(fp);

                            if (newfilePos != (int32)(filePos + delta))
                            {
                                AtomUtils::seekFromStart(fp, filePos + delta);
                            }
                        }
                    }
                    count += delta;
                }
                else
                {
                    _success = false;
                    _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
                    return;
                }
            }

            while (count < _size)
            {
                uint8 data;
                if (!AtomUtils::read8(fp, data))
                {
                    _success = false;
                    _mp4ErrorCode = READ_USER_DATA_ATOM_FAILED;
                    return;
                }
                count++;
            }
        }
        else if (count > _size)
        {
            _success = false;
            _mp4ErrorCode = READ_USER_DATA_ATOM_FAILED;
            return;
        }
    }
    else
    {
        if (_mp4ErrorCode != ATOM_VERSION_NOT_SUPPORTED)
            _mp4ErrorCode = READ_COPYRIGHT_ATOM_FAILED;
    }
}


// Destructor
CopyRightAtom::~CopyRightAtom()
{
}

