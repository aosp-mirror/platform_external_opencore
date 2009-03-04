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


#ifndef __ObjectDescriptor_H__
#define __ObjectDescriptor_H__

#include "a_atomdefs.h"
#include "basedescriptor.h"
#include "a_isucceedfail.h"

#include "esdescriptor.h"
#include "es_id_ref.h"

class PVA_FF_ObjectDescriptor : public PVA_FF_BaseDescriptor, public PVA_FF_ISucceedFail
{

    public:
        // Default Constructor
        PVA_FF_ObjectDescriptor(uint16 nextAvailableODID, uint8 tag = MP4_OD_TAG);

        virtual ~PVA_FF_ObjectDescriptor();

        void init(uint16 nextAvailableODID);

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        // Member gets and sets
        uint16 getObjectDescriptorID() const
        {
            return _objectDescriptorID;
        }
        void setObjectDescriptorID(uint16 id)
        {
            _objectDescriptorID = id;
        }

        bool getUrlFlag() const
        {
            return _urlFlag;
        }
        void setUrlFlag(bool flag)
        {
            _urlFlag = flag;
        }

        uint8 getUrlLength() const
        {
            return _urlLength;
        }
        PVA_FF_UTF8_STRING_PARAM getUrlString() const
        {
            return _urlString;
        }
        void setUrlString(PVA_FF_UTF8_STRING_PARAM url)
        {
            _urlString = url;
        }

        Oscl_Vector<PVA_FF_ES_ID_Ref*, OsclMemAllocator> &getESIDReferences()
        {
            return *_pES_ID_Ref_Vec;
        }
        void addESIDReference(PVA_FF_ES_ID_Ref *ref);
        PVA_FF_ES_ID_Ref* getESIDReferenceAt(int32 index);

    private:
        virtual void recomputeSize();

    protected:
        uint16 _objectDescriptorID; // (10)
        bool _urlFlag; // (1)
        uint8 _reserved; // 0b11111; // (5)

        uint8 _urlLength; // (8)
        PVA_FF_UTF8_HEAP_STRING _urlString; // (8)[_urlLength]

        Oscl_Vector<PVA_FF_ES_ID_Ref*, OsclMemAllocator> *_pES_ID_Ref_Vec; // PVA_FF_ESDescriptor[1 to 30]
};



#endif

