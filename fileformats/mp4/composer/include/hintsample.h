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
    This PVA_FF_HintSample Class is the simple structure that hold all the data of
    individual HintTrack entries.  This class alsp provides access methods to
    directly access the HintTrack entry fields - this class takes care of parsing
    the packed bytes of the hint entry
*/

#ifndef __HintSample_H__
#define __HintSample_H__

#include "atomutils.h"
#include "a_atomdefs.h"

class PVA_FF_HintSample
{

    public:
        PVA_FF_HintSample(uint8 type)
        {
            _type = type;    // Constructor
        }
        virtual ~PVA_FF_HintSample() {} // Destructor

        virtual int32 renderToBuffer(void *mem) = 0;
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp) = 0;
        virtual uint32 getSize() const = 0; // Size of hint sample in bytes

        uint8 getType() const
        {
            return _type;
        }

        void SetMediaOffset(uint32 offset)
        {
            _Offset = offset;
        }

    protected:
        bool renderBaseMembersToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp) const
        {
            if (!PVA_FF_AtomUtils::render8(fp, _type))
            {
                return false;
            }

            return true;
        }
        uint32 _Offset;
        uint8  _Flag_Type;

    private:
        uint8 _type;

};

#endif
