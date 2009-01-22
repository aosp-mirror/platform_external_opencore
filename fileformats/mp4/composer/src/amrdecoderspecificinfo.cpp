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
/*
    This PVA_FF_DecoderSpecificInfo Class that holds the Mpeg4 VOL header for the
	video stream
*/

#define __IMPLEMENT_AMRDecoderSpecificInfo__

#include "amrdecoderspecificinfo.h"

#include "atom.h"
#include "atomutils.h"

// Default constructor
PVA_FF_AMRDecoderSpecificInfo::PVA_FF_AMRDecoderSpecificInfo()
{
    _sizeOfClass = 12;
    _sizeOfSizeField = PVA_FF_AtomUtils::
                       getNumberOfBytesUsedToStoreSizeOfClass(_sizeOfClass);

    _VendorCode = PACKETVIDEO_FOURCC;
    _encoder_version = 0;
    _band_mode = 0;
    _mode_set = 0;
    _mode_change_period = 0;
    _mode_change_neighbour = 0;
    _reserved = 0;
}

bool
PVA_FF_AMRDecoderSpecificInfo::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    uint8 temp1 = 0;
    int32  temp2 = 0;
    int32 rendered = 0;

    // Render base descriptor packed size and tag
    if (!renderBaseDescriptorMembers(fp))
    {
        return false;
    }

    rendered += getSize();

    // Render decoder specific info payload
    if (!PVA_FF_AtomUtils::render32(fp, _VendorCode))
    {
        return false;
    }
    rendered += 4;

    if (!PVA_FF_AtomUtils::render8(fp, _encoder_version))
    {
        return false;
    }
    rendered += 1;

    temp1 = (uint8)(((_band_mode << 7) & 0x80) | (_frame_type & 0x0F));
    if (!PVA_FF_AtomUtils::render8(fp, temp1))
    {
        return false;
    }
    rendered += 1;

    if (!PVA_FF_AtomUtils::render16(fp, _mode_set))
    {
        return false;
    }
    rendered += 2;

    temp2 = ((_mode_change_period << 1) | (_mode_change_neighbour & 0x01));
    if (!PVA_FF_AtomUtils::render32(fp, temp2))
    {
        return false;
    }
    rendered += 4;

    return true;
}


