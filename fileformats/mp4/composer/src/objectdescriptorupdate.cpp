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
    This PVA_FF_ObjectDescriptorUpdate Class conveys a new list of ObjectDescriptors
    OD streams
*/


#define IMPLEMENT_BaseCommand

#include "objectdescriptorupdate.h"
#include "atomutils.h"

typedef Oscl_Vector<PVA_FF_ObjectDescriptor*, OsclMemAllocator> PVA_FF_ObjectDescriptorVecType;

// Constructor
PVA_FF_ObjectDescriptorUpdate::PVA_FF_ObjectDescriptorUpdate()
        : PVA_FF_BaseCommand(0x01)
{
    _entryCount = 0;
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_ObjectDescriptorVecType, (), _pobjDescrVec);

    recomputeSize();
}

// Desctructor
PVA_FF_ObjectDescriptorUpdate::~PVA_FF_ObjectDescriptorUpdate()
{
    for (uint32 i = 0; i < _pobjDescrVec->size(); i++)
    {
        PV_MP4_FF_DELETE(NULL, PVA_FF_ObjectDescriptor, (*_pobjDescrVec)[i]);
    }
    PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_ObjectDescriptorVecType, Oscl_Vector, _pobjDescrVec);
}

void
PVA_FF_ObjectDescriptorUpdate::addObjectDescriptor(PVA_FF_ObjectDescriptor *pdescr)
{
    _entryCount += 1;
    _pobjDescrVec->push_back(pdescr);
    recomputeSize();
}

const PVA_FF_ObjectDescriptor *
PVA_FF_ObjectDescriptorUpdate::getObjectDescriptorAt(int32 index) const
{
    if (index < (int32)_entryCount)
    {
        return (*_pobjDescrVec)[index];
    }
    else
    {
        return NULL;
    }
}


// Rendering the Descriptor in proper format (bitlengths, etc.) to an ostream
bool
PVA_FF_ObjectDescriptorUpdate::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    int32 rendered = renderBaseCommandMembers(fp);

    if (rendered > 0)
    {
        if (_pobjDescrVec != NULL)
        {
            for (int32 i = 0; i < (int32)_entryCount; i++)
            {
                if (!((*_pobjDescrVec)[i]->renderToFileStream(fp)))
                {
                    return false;
                }
                rendered += (*_pobjDescrVec)[i]->getSizeOfDescriptorObject();
            }
        }
    }

    return true;
}


MP4_AUTHOR_FF_FILE_IO_WRAP *
PVA_FF_ObjectDescriptorUpdate::dump(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    //return fp << *this << endl;
    return fp;
}

void
PVA_FF_ObjectDescriptorUpdate::recomputeSize()
{
    int32 contents = 0;

    for (int32 i = 0; i < (int32)_entryCount; i++)   // Get sizes of all contained ODs
    {
        contents += (*_pobjDescrVec)[i]->getSizeOfDescriptorObject();
    }
    _sizeOfClass = contents;
    _sizeOfSizeField = PVA_FF_AtomUtils::getNumberOfBytesUsedToStoreSizeOfClass(contents);


    if (_pparent != NULL)
    {
        _pparent->recomputeSize();
    }
}
