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
    This PVA_FF_UserDataAtom Class is a container atom for informative user-data.
*/


#ifndef __PVUserDataAtom_H__
#define __PVUserDataAtom_H__

#include "atom.h"
#include "a_isucceedfail.h"
#include "atomutils.h"

class PVA_FF_PVUserDataAtom : public PVA_FF_Atom, public PVA_FF_ISucceedFail
{

    public:
        PVA_FF_PVUserDataAtom(); // Constructor
        virtual ~PVA_FF_PVUserDataAtom();

        // Member sets
        void setVersion(PVA_FF_UNICODE_STRING_PARAM version);
        void setTitle(PVA_FF_UNICODE_STRING_PARAM title);
        void setAuthor(PVA_FF_UNICODE_STRING_PARAM author);
        void setCopyright(PVA_FF_UNICODE_STRING_PARAM copyright);
        void setDescription(PVA_FF_UNICODE_STRING_PARAM description);
        void setRating(PVA_FF_UNICODE_STRING_PARAM ratingInfo);
        void setCreationDate(PVA_FF_UNICODE_STRING_PARAM creationdate);

        // Member gets
        uint32 getFourCC() const
        {
            return _fourCC;    // Special member get
        }
        virtual void recomputeSize();

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        uint32          _fourCC;
        PVA_FF_UNICODE_HEAP_STRING _version; // Static for the version of the file format code
        PVA_FF_UNICODE_HEAP_STRING _title;
        PVA_FF_UNICODE_HEAP_STRING _author;
        PVA_FF_UNICODE_HEAP_STRING _copyright;
        PVA_FF_UNICODE_HEAP_STRING _description;
        PVA_FF_UNICODE_HEAP_STRING _rating;
        PVA_FF_UNICODE_HEAP_STRING _creationDate;
};

class PVA_FF_PVEntityTagAtom : public PVA_FF_Atom, public PVA_FF_ISucceedFail
{

    public:
        PVA_FF_PVEntityTagAtom(); // Constructor
        virtual ~PVA_FF_PVEntityTagAtom() {};

        // Member sets
        void setEntityTag(PVA_FF_UNICODE_STRING_PARAM etag);

        virtual void recomputeSize();

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        PVA_FF_UNICODE_HEAP_STRING _etag;
};

class PVA_FF_PVContentTypeAtom : public PVA_FF_Atom
{
    public:
        PVA_FF_PVContentTypeAtom();
        virtual ~PVA_FF_PVContentTypeAtom() {};

        void setContentType(uint32 typeFlags);

        virtual void recomputeSize();

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

    private:
        uint32 _contentType;
};

#endif

