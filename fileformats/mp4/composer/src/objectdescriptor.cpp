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
    This PVA_FF_ObjectDescriptor Class
*/

#define IMPLEMENT_ObjectDescriptor

#include "objectdescriptor.h"
#include "atomutils.h"
#include "a_atomdefs.h"

typedef Oscl_Vector<PVA_FF_ES_ID_Ref*, OsclMemAllocator> PVA_FF_ES_ID_RefVecType;

// Default Constructor
PVA_FF_ObjectDescriptor::PVA_FF_ObjectDescriptor(uint16 nextAvailableODID, uint8 tag)
        : PVA_FF_BaseDescriptor(tag)
{
    init(nextAvailableODID);
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_ES_ID_RefVecType, (), _pES_ID_Ref_Vec);

    recomputeSize();
}

// Destructor
PVA_FF_ObjectDescriptor::~PVA_FF_ObjectDescriptor()
{
    // Cleanup vector of ES_ID_Refs
    if (_pES_ID_Ref_Vec != NULL)
    {
        for (uint32 i = 0; i < _pES_ID_Ref_Vec->size(); i++)
        {
            PVA_FF_ES_ID_Ref* ref = (*_pES_ID_Ref_Vec)[i];
            PV_MP4_FF_DELETE(NULL, PVA_FF_ES_ID_Ref, ref);
        }
        PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_ES_ID_RefVecType, Oscl_Vector, _pES_ID_Ref_Vec);
    }
}

void
PVA_FF_ObjectDescriptor::init(uint16 nextAvailableODID)
{
    _objectDescriptorID = nextAvailableODID;

    _urlFlag = false;
    _urlLength = 0;
    _reserved = 0x1f;
}

// Rendering the Descriptor in proper format (bitlengths, etc.) to an ostream
bool
PVA_FF_ObjectDescriptor::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    int32 rendered = 0; // Keep track of number of bytes rendered

    // Render the base class members
    int32 numBytes = renderBaseDescriptorMembers(fp);

    if (numBytes == 0)
    {
        return false;
    }
    rendered += numBytes;

    // Pack and render ODID, urlFlag, and reserved
    uint16 data = (uint16)((getObjectDescriptorID() & 0x0fffff) << 6); // (10 bits)

    if (getUrlFlag())
    {
        data |= 0x20; // Set urlFlag bit
    }

    data |= _reserved; // (5 bits) reserved 0b11111
    if (!PVA_FF_AtomUtils::render16(fp, data))
    {
        return false;
    }
    rendered += 2;

    if (getUrlFlag())
    {
        // Render _urlLength
        if (!PVA_FF_AtomUtils::render8(fp, getUrlLength()))
        {
            return false;
        }
        rendered += 1;

        // Render url string
        if (getUrlLength() > 0)
        {
            if (!PVA_FF_AtomUtils::renderString(fp, getUrlString()))
            {
                return false;
            }
        }
        rendered += getUrlLength();

    }
    else
    {
        // Render the vector of ESDescriptors - actually render their ESIDs
        if (_pES_ID_Ref_Vec != NULL)
        {
            for (uint32 i = 0; i < _pES_ID_Ref_Vec->size(); i++)
            {
                PVA_FF_ES_ID_Ref* ref = (*_pES_ID_Ref_Vec)[i];
                if (!ref->renderToFileStream(fp))
                {
                    return false;
                }
                rendered += ref->getSizeOfDescriptorObject();
            }
        }
    }

    return true;
}


// Adding an PVA_FF_ES_ID_Ref to the vector of ES_ID_References
// Then recomputing the size of the descriptor with the newly added PVA_FF_ES_ID_Ref
void
PVA_FF_ObjectDescriptor::addESIDReference(PVA_FF_ES_ID_Ref *ref)
{
    _pES_ID_Ref_Vec->push_back(ref);
    recomputeSize();
}


// Returning an PVA_FF_ES_ID_Ref pointer at location index in the array - if
// the array fp that big
PVA_FF_ES_ID_Ref*
PVA_FF_ObjectDescriptor::getESIDReferenceAt(int32 index)
{
    if (_pES_ID_Ref_Vec != NULL)
    {
        if (index < (int32)_pES_ID_Ref_Vec->size())
        {
            return (*_pES_ID_Ref_Vec)[index];
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}


void
PVA_FF_ObjectDescriptor::recomputeSize()
{
    int32 contents = 0;
    contents += 2; // (2 bytes = 16 bits) Packed ODID, URLFlag, and Reserved

    if (getUrlFlag())
    {
        contents += 1; // 1 byte for URL Length variable
        contents += _urlLength; // length of URL string
    }
    else
    {

        if (_pES_ID_Ref_Vec != NULL)
        {
            // Renders the PVA_FF_ES_ID_Ref instead of the actual PVA_FF_ESDescriptor
            int32 SIZE_OF_ES_ID_REF_OBJECT = 4;
            contents += _pES_ID_Ref_Vec->size() * SIZE_OF_ES_ID_REF_OBJECT;
        }
    }

    _sizeOfClass = contents;
    _sizeOfSizeField = PVA_FF_AtomUtils::getNumberOfBytesUsedToStoreSizeOfClass(contents);

    if (_pparent != NULL)
    {
        _pparent->recomputeSize();
    }
}

