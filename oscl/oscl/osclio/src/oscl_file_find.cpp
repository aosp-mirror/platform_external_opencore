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
/*! \file oscl_file_io.cpp
    \brief This file contains file io APIs
*/

#include "oscl_file_find.h"
#include "oscl_file_types.h"
#include "oscl_utf8conv.h"
#include "oscl_stdstring.h"

OSCL_EXPORT_REF Oscl_FileFind::Oscl_FileFind()
{
    lastError = Oscl_FileFind::E_NOT_IMPLEMENTED;
    type = Oscl_FileFind::INVALID_TYPE;
}

OSCL_EXPORT_REF Oscl_FileFind::~Oscl_FileFind()
{
    Close();
}

OSCL_EXPORT_REF const char *Oscl_FileFind::FindFirst(const char *directory, const char *pattern, char *buf, uint32 buflen)
{
    const char *def_pattern = "*";

    if (pattern == NULL) pattern = def_pattern;

    OSCL_UNUSED_ARG(directory);
    OSCL_UNUSED_ARG(buf);
    OSCL_UNUSED_ARG(buflen);
    return NULL;

}

OSCL_EXPORT_REF const oscl_wchar *Oscl_FileFind::FindFirst(const oscl_wchar *directory, const oscl_wchar *pattern, oscl_wchar *buf, uint32 buflen)
{
    const oscl_wchar *def_pattern = _STRLIT_WCHAR("*");

    if (pattern == NULL) pattern = def_pattern;

    OSCL_UNUSED_ARG(directory);
    OSCL_UNUSED_ARG(buf);
    OSCL_UNUSED_ARG(buflen);

    return NULL;
}

OSCL_EXPORT_REF char *Oscl_FileFind::FindNext(char *buf, uint32 buflen)
{
    OSCL_UNUSED_ARG(buf);
    OSCL_UNUSED_ARG(buflen);
    return NULL;
}

OSCL_EXPORT_REF oscl_wchar *Oscl_FileFind::FindNext(oscl_wchar *buf, uint32 buflen)
{
    OSCL_UNUSED_ARG(buf);
    OSCL_UNUSED_ARG(buflen);
    return NULL;
}

OSCL_EXPORT_REF void Oscl_FileFind::Close()
{
}

OSCL_EXPORT_REF Oscl_FileFind::element_type Oscl_FileFind::GetElementType()
{
    return type;
}

OSCL_EXPORT_REF Oscl_FileFind::error_type Oscl_FileFind::GetLastError()
{
    return lastError;
}


