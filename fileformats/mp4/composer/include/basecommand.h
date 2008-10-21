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
    This PVA_FF_BaseCommand Class is the base class for all PVA_FF_ObjectDescriptor streams
    that are represented as PVA_FF_ObjectDescriptor commands - the AccessUnits for all
    OD streams
*/


#ifndef __BaseCommand_H__
#define __BaseCommand_H__

#include "oscl_types.h"

#include "expandablebaseclass.h"

const uint32 DEFAULT_COMMAND_SIZE = 1; // 8 bits for the tag ONLY
// _sizeOfClass is computed explicitly elsewhere!

class PVA_FF_BaseCommand : public PVA_FF_ExpandableBaseClass
{

    public:
        PVA_FF_BaseCommand(uint8 tag); // Constructor
        virtual ~PVA_FF_BaseCommand();

        // Rendering the Descriptor in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
        {
            OSCL_UNUSED_ARG(fp);
            return true;
        }
        // Rendering only the members of the PVA_FF_BaseDescriptor class
        int renderBaseCommandMembers(MP4_AUTHOR_FF_FILE_IO_WRAP *fp) const;
        virtual void recomputeSize() = 0; // Should get overridden
        uint32 getSizeOfClass() const
        {
            return _sizeOfClass;
        }
        uint32 getDefaultCommandSize() const
        {
            return DEFAULT_COMMAND_SIZE;
        }

    private:
        PVA_FF_BaseCommand() {} // Disabling public default constructor

};



#endif

