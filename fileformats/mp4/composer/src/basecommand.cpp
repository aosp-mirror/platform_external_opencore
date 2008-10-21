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
    This PVA_FF_BaseCommand Class
*/

#define IMPLEMENT_BaseCommand

#include "basecommand.h"
#include "atomutils.h"
#include "a_atomdefs.h"


// Constructor
PVA_FF_BaseCommand::PVA_FF_BaseCommand(uint8 tag)
{
    _sizeOfClass = 0;
    _sizeOfSizeField = DEFAULT_COMMAND_SIZE; // 1
    _tag = tag;
}

// Destructor
PVA_FF_BaseCommand::~PVA_FF_BaseCommand()
{
    // Empty
}


// Rendering the PVA_FF_BaseCommand members in proper format (bitlengths, etc.)
// to an ostream
int
PVA_FF_BaseCommand::renderBaseCommandMembers(MP4_AUTHOR_FF_FILE_IO_WRAP *fp) const
{
    if (!PVA_FF_AtomUtils::render8(fp, getTag()))
    {
        return 0;
    }
    int numBytesRendered = 1;

    // Render attributes of the PVA_FF_BaseCommand class
    int32 numBytes = renderSizeOfClassToFileStream(fp);

    if (numBytes > 0)
    {
        numBytesRendered += numBytes;
    }
    else
    {
        numBytesRendered = 0;
    }

    return numBytesRendered;
}
