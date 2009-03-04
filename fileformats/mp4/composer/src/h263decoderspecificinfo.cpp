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
    This PVA_FF_DecoderSpecificInfo Class that holds the Mpeg4 VOL header for the
	video stream
*/

#define __IMPLEMENT_AMRDecoderSpecificInfo__

#include "h263decoderspecificinfo.h"

#include "atomutils.h"

PVA_FF_H263DecoderSpecificInfo::PVA_FF_H263DecoderSpecificInfo()
{
    _sizeOfClass = 12;
    _sizeOfSizeField = PVA_FF_AtomUtils::
                       getNumberOfBytesUsedToStoreSizeOfClass(_sizeOfClass);
}

bool
PVA_FF_H263DecoderSpecificInfo::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
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

    if (!PVA_FF_AtomUtils::render8(fp, _codec_profile))
    {
        return false;
    }
    rendered += 1;

    if (!PVA_FF_AtomUtils::render8(fp, _codec_level))
    {
        return false;
    }
    rendered += 1;

    if (!PVA_FF_AtomUtils::render8(fp, 0))
    {
        return false;
    }
    rendered += 1;

    if (!PVA_FF_AtomUtils::render16(fp, _max_width))
    {
        return false;
    }
    rendered += 2;

    if (!PVA_FF_AtomUtils::render16(fp, _max_height))
    {
        return false;
    }
    rendered += 2;

    return true;
}


