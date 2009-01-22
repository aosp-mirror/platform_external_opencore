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
    This PVA_FF_ObjectDescriptorAtom Class contains an PVA_FF_ObjectDescriptor or an
    PVA_FF_InitialObjectDescriptor.
*/


#ifndef __ObjectDescriptorAtom_H__
#define __ObjectDescriptorAtom_H__

#include "fullatom.h"
#include "a_isucceedfail.h"

#include "initialobjectdescriptor.h"

class PVA_FF_ObjectDescriptorAtom : public PVA_FF_FullAtom, public PVA_FF_ISucceedFail
{

    public:
        PVA_FF_ObjectDescriptorAtom(uint8 version, uint32 flags); // Constructor

        virtual ~PVA_FF_ObjectDescriptorAtom();

        // Member gets and sets
        PVA_FF_InitialObjectDescriptor &getMutableInitialObjectDescriptor()
        {
            return *_pOD;
        }
        const PVA_FF_InitialObjectDescriptor &getInitialObjectDescriptor() const
        {
            return *_pOD;
        }
        void setInitialObjectDescriptor(PVA_FF_InitialObjectDescriptor *piod)
        {
            _pOD = piod;
        }

        virtual void recomputeSize();

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

    private:
        PVA_FF_InitialObjectDescriptor *_pOD;
};



#endif

