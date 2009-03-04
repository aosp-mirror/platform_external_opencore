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


#ifndef __ObjectDescriptorUpdate_H__
#define __ObjectDescriptorUpdate_H__

#include "basecommand.h"
#include "objectdescriptor.h"

class PVA_FF_ObjectDescriptorUpdate : public PVA_FF_BaseCommand
{

    public:
        PVA_FF_ObjectDescriptorUpdate(); // Constructor
        virtual ~PVA_FF_ObjectDescriptorUpdate();

        // Adding elements to and getting from ObjDescrVec
        void addObjectDescriptor(PVA_FF_ObjectDescriptor *pdescr);
        const PVA_FF_ObjectDescriptor *getObjectDescriptorAt(int32 index) const;
        const Oscl_Vector<PVA_FF_ObjectDescriptor*, OsclMemAllocator> &getObjDescrVec()
        {
            return *_pobjDescrVec;
        }

        // Rendering the Descriptor in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        virtual void recomputeSize();

        // Dumps the objects members to the ostream os
        virtual MP4_AUTHOR_FF_FILE_IO_WRAP* dump(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

    private:
        Oscl_Vector<PVA_FF_ObjectDescriptor*, OsclMemAllocator> *_pobjDescrVec;
        uint32 _entryCount; // DO NOT RENDER THIS ATTRIBUTE - internal ONLY

};

#endif

