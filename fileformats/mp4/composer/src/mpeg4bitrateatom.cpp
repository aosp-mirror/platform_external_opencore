/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#include "atom.h"
#include "atomutils.h"
#include "oscl_file_io.h"
#include "mpeg4bitrateatom.h"
#include "a_atomdefs.h"

PVA_FF_Mpeg4Bitrate::PVA_FF_Mpeg4Bitrate(uint32 BufferSizeDB, uint32 MaxBitRate, uint32 AvgBitRate)
        : PVA_FF_Atom(MPEG4_BITRATE_BOX)
        , _bufferSizeDB(BufferSizeDB)
        , _maxBitRate(MaxBitRate)
        , _avgBitRate(AvgBitRate)
{
    recomputeSize();
}

bool PVA_FF_Mpeg4Bitrate::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    return (renderAtomBaseMembers(fp)
            && PVA_FF_AtomUtils::render32(fp, _bufferSizeDB)
            && PVA_FF_AtomUtils::render32(fp, _maxBitRate)
            && PVA_FF_AtomUtils::render32(fp, _avgBitRate));
}

void PVA_FF_Mpeg4Bitrate::recomputeSize()
{
    _size = 0x14;

    // Update size of parent
    if (_pparent != NULL)
    {
        _pparent->recomputeSize();
    }
}

uint32 PVA_FF_Mpeg4Bitrate::getSize()
{
    recomputeSize();
    return (_size);
}

