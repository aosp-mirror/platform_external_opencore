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
    This PVA_FF_ObjectDescriptorAtom Class contains an PVA_FF_ObjectDescriptor or an
    PVA_FF_InitialObjectDescriptor.
*/


#define IMPLEMENT_ObjectDescriptorAtom

#include "objectdescriptoratom.h"
#include "atomutils.h"
#include "a_atomdefs.h"

//extern pvostream& operator<<(pvostream& fp, PVA_FF_InitialObjectDescriptor &iod);
//extern pvostream& operator<<(pvostream& fp, const PVA_FF_InitialObjectDescriptor &iod);

// Constructor
PVA_FF_ObjectDescriptorAtom::PVA_FF_ObjectDescriptorAtom(uint8 version, uint32 flags)
        : PVA_FF_FullAtom(OBJECT_DESCRIPTOR_ATOM, version, flags)
{
    _success = true;

    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_InitialObjectDescriptor, (), _pOD);

    _pOD->setParent(this);
    recomputeSize();
}

// Destructor
PVA_FF_ObjectDescriptorAtom::~PVA_FF_ObjectDescriptorAtom()
{
    // Cleanup PVA_FF_InitialObjectDescriptor
    PV_MP4_FF_DELETE(NULL, PVA_FF_InitialObjectDescriptor, _pOD);
}

void
PVA_FF_ObjectDescriptorAtom::recomputeSize()
{
    _size = getDefaultSize() + _pOD->getSizeOfDescriptorObject();

    // Update size of parent item (continue up the tree to root)
    if (getParent() != NULL)
    {
        _pparent->recomputeSize();
    }
}


// Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
bool
PVA_FF_ObjectDescriptorAtom::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    int32 rendered = 0; // Keep track of number of bytes rendered

    // Render PVA_FF_Atom type and size
    if (!renderAtomBaseMembers(fp))
    {
        return false;
    }
    rendered += getDefaultSize();

    // Render the OD
    if (!_pOD->renderToFileStream(fp))
    {
        return false;
    }
    rendered += _pOD->getSizeOfDescriptorObject();

    return true;
}
