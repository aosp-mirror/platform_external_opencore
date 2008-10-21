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
    This PVA_FF_InitialObjectDescriptor Class
*/


#define IMPLEMENT_InitialObjectDescriptor

#include "initialobjectdescriptor.h"
#include "atomutils.h"
#include "a_atomdefs.h"

typedef Oscl_Vector<PVA_FF_ES_ID_Inc*, OsclMemAllocator> PVA_FF_ES_ID_IncVecType;
// Constructor
PVA_FF_InitialObjectDescriptor::PVA_FF_InitialObjectDescriptor()
        : PVA_FF_ObjectDescriptor(1, MP4_IOD_TAG)
{
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_ES_ID_IncVecType, (), _pES_ID_Inc_Vec);
    init();
    recomputeSize();
}

// Initializations
void
PVA_FF_InitialObjectDescriptor::init()
{
    _urlLength = 0;
    _urlFlag = false;
    _reserved = 0x0f; // Now only (4)
    _includeInlineProfilesFlag = false;

    setODProfileLevelIndication(0xFE);
    setSceneProfileLevelIndication(0xFE);
    setAudioProfileLevelIndication(0xFE);
    setVisualProfileLevelIndication(0xFE);
    setGraphicsProfileLevelIndication(0xFE);

}

// Destructor
PVA_FF_InitialObjectDescriptor::~PVA_FF_InitialObjectDescriptor()
{
    if (_pES_ID_Inc_Vec != NULL)
    {
        for (uint32 i = 0; i < _pES_ID_Inc_Vec->size(); i++)
        {
            PV_MP4_FF_DELETE(NULL, PVA_FF_ES_ID_Inc, (*_pES_ID_Inc_Vec)[i]);
        }
        PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_ES_ID_IncVecType, Oscl_Vector, _pES_ID_Inc_Vec);
    }

    return;

}


void
PVA_FF_InitialObjectDescriptor::addESIDInclude(PVA_FF_ES_ID_Inc *inc)
{
    _pES_ID_Inc_Vec->push_back(inc);
    recomputeSize();
}

PVA_FF_ES_ID_Inc*
PVA_FF_InitialObjectDescriptor::getESIDIncludeAt(int32 index)
{
    if (_pES_ID_Inc_Vec != NULL)
    {
        if (index < (int32)_pES_ID_Inc_Vec->size())
        {
            return (*_pES_ID_Inc_Vec)[index];
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
PVA_FF_InitialObjectDescriptor::recomputeSize()
{
    int32 contents = 2; // (2 bytes = 16 bits) Packed ODID, URLFlag, InlineProfFlag, and Reserved

    if (_urlFlag)
    {
        contents += 1; // 1 byte for URL Length variable
        contents += _urlLength; // length of URL string
    }
    else
    {
        contents += 5;  // ODProfileLevelIndication
        // sceneProfileLevelIndication
        // audioProfileLevelIndication
        // visualProfileLevelIndication
        // graphicsProfileLevelIndication

        // IOD renders ONLy the ESIDs (referencing the tracks) and not the actual ESDescr
        if (_pES_ID_Inc_Vec != NULL)
        {
            // Renders the ESID instead of the actual PVA_FF_ESDescriptor
            int32 SIZE_OF_ES_ID_INC_OBJECT = 6;
            contents += _pES_ID_Inc_Vec->size() * SIZE_OF_ES_ID_INC_OBJECT;
        }
    }

    _sizeOfClass = contents;
    _sizeOfSizeField = PVA_FF_AtomUtils::getNumberOfBytesUsedToStoreSizeOfClass(contents);

    // Have the parent descriptor recompute its size based on this update
    if (_pparent != NULL)
    {
        _pparent->recomputeSize();
    }
}

// Rendering the Descriptor in proper format (bitlengths, etc.) to an ostream
bool
PVA_FF_InitialObjectDescriptor::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    int32 rendered = 0; // Keep track of number of bytes rendered

    // Render the base class members
    int32 numBytes = renderBaseDescriptorMembers(fp);

    if (numBytes == 0)
    {
        return false;
    }
    rendered += numBytes;

    // Pack and render ODID, urlFlag, and
    uint16 data = (uint16)((_objectDescriptorID && 0x03ff) << 6); // (10 bits)

    if (getUrlFlag())
    {
        data |= 0x20; // Set urlFlag bit
    }
    if (getIncludeInlineProfilesFlag())
    {
        data |= 0x10; // Set includeInlineProfilesFlag bit
    }
    data |= (_reserved & 0x000f); // (4 bits)

    if (!PVA_FF_AtomUtils::render16(fp, data))
    {
        return false;
    }
    rendered += 2;

    if (getUrlFlag())
    {
        // Render _urlLength
        if (!PVA_FF_AtomUtils::render8(fp, _urlLength))
        {
            return false;
        }
        rendered += 1;

        // Render url string
        if (_urlLength > 0)
        {
            if (!PVA_FF_AtomUtils::renderString(fp, _urlString))
            {
                return false;
            }
        }
        rendered += _urlLength;
    }
    else
    {
        if (!PVA_FF_AtomUtils::render8(fp, _ODProfileLevelIndication))
        {
            return false;
        }
        if (!PVA_FF_AtomUtils::render8(fp, _sceneProfileLevelIndication))
        {
            return false;
        }
        if (!PVA_FF_AtomUtils::render8(fp, _audioProfileLevelIndication))
        {
            return false;
        }
        if (!PVA_FF_AtomUtils::render8(fp, _visualProfileLevelIndication))
        {
            return false;
        }
        if (!PVA_FF_AtomUtils::render8(fp, _graphicsProfileLevelIndication))
        {
            return false;
        }
        rendered += 5;

        // Render the vector of ESDescriptors (ES_ID_Incs)
        if (_pES_ID_Inc_Vec != NULL)
        {
            for (uint32 i = 0; i < _pES_ID_Inc_Vec->size(); i++)
            {
                PVA_FF_ES_ID_Inc *inc = (*_pES_ID_Inc_Vec)[i];
                if (!inc->renderToFileStream(fp))
                {
                    return false;
                }
                rendered += inc->getSizeOfDescriptorObject();
            }
        }
    }

    return true;
}

