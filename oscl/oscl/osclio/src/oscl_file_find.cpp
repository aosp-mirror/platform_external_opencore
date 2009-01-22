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
/*! \file oscl_file_io.cpp
    \brief This file contains file io APIs
*/

#include "oscl_file_find.h"
#include "oscl_file_types.h"
#include "oscl_utf8conv.h"
#include "oscl_stdstring.h"
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif

OSCL_EXPORT_REF Oscl_FileFind::Oscl_FileFind()
{
#if   (OSCL_HAS_GLOB)
    count = 0;
    haveGlob = false;
    lastError = Oscl_FileFind::E_OK;
#else
#define OSCL_FILEFIND_NUMBER_OF_FILES_ENTRY 256

    count = 0;
    int err = 0;
    haveGlob = false;
    OSCL_TRY(err,
             iDirEntVec.reserve(OSCL_FILEFIND_NUMBER_OF_FILES_ENTRY);
            );
    if (err)
    {
        iDirEntVec.clear();
        OSCL_LEAVE(err);
        lastError = Oscl_FileFind::E_OTHER;
    }
    lastError = Oscl_FileFind::E_OK;
#endif
    type = Oscl_FileFind::INVALID_TYPE;
}

OSCL_EXPORT_REF Oscl_FileFind::~Oscl_FileFind()
{
    Close();
}

static bool oscl_strglob(const char *str, const char *p);
OSCL_EXPORT_REF const char *Oscl_FileFind::FindFirst(const char *directory, const char *pattern, char *buf, uint32 buflen)
{
    const char *def_pattern = "*";
    lastError = Oscl_FileFind::E_OK;
    type = Oscl_FileFind::INVALID_TYPE;
    if (directory == NULL || buf == NULL || buflen <= 0)
    {
        lastError = E_INVALID_ARG;
        return NULL;
    }
    if (pattern == NULL) pattern = def_pattern;

#if   (OSCL_HAS_GLOB)
    int retval;
    char path[OSCL_IO_FILENAME_MAXLEN];
    lastError = Oscl_FileFind::E_OK;

    if (haveGlob || !directory || !buf || buflen <= 0)
    {
        lastError = (haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }

    if ((oscl_strlen(directory) + oscl_strlen(pattern) + 1) > OSCL_IO_FILENAME_MAXLEN)
    {
        lastError = Oscl_FileFind::E_PATH_TOO_LONG;
        return NULL;
    }

    path[0] = '\0';
    if (oscl_strlen(directory) > 0)
    {
        oscl_strcat(path, directory);

        // Check whether path ends with delimiter
        int32 pathLength = oscl_strlen(path);
        bool appendPathDelimiter = true;
        if (pathLength >= 1)
        {
            if (oscl_strncmp((path + pathLength - 1/*length of path delimiter is 1*/),
                             OSCL_FILE_CHAR_PATH_DELIMITER, 1) == 0)
            {
                // path ends with delimiter
                appendPathDelimiter = false;
            }
        }

        if (appendPathDelimiter)
        {
            oscl_strcat(path, OSCL_FILE_CHAR_PATH_DELIMITER);
        }
        oscl_strcat(path, pattern);
    }
    else
    {
        oscl_strcat(path, pattern);
    }

    if ((retval = glob(path, GLOB_ERR | GLOB_NOSORT , NULL, &hFind)) == 0)
    {
        haveGlob = true;
        if (hFind.gl_pathc > 0)
        {
            if (strlen(hFind.gl_pathv[count]) >= buflen)
            {
                lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
                return NULL;
            }
            oscl_strncpy(buf, hFind.gl_pathv[count++], buflen);
            struct stat statbuf;
            if (stat(buf, &statbuf) == 0)
            {
                type = (S_ISDIR(statbuf.st_mode)) ? DIR_TYPE : FILE_TYPE;
            }
            else
            {
                type = FILE_TYPE;
            }
            return buf;
        }
        else
        {
            Close();
        }
    }
    else
    {
        if (GLOB_NOMATCH == retval)
        {
            lastError = Oscl_FileFind::E_NO_MATCH;
        }
        else if (GLOB_ABORTED)
        {
            lastError = Oscl_FileFind::E_PATH_NOT_FOUND;
        }
        else
        {
            lastError = Oscl_FileFind::E_OTHER;
        }
        return NULL;
    }

#else
    // support linux having no glob.h support in glob pattern matching
    DIR* pDir;
    struct dirent* pEnt;
    uint32 itr = 0;
    struct stat statbuf;
    if (haveGlob || !directory || !buf || buflen <= 0)
    {
        lastError = (haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }
    if (oscl_strlen(directory) > 0)
    {
        pDir = opendir(directory);
    }
    else
    {
        // empty directory, replaced with searching current dir
        // make the behavior consistent with the glob-based implementation
        OSCL_HeapString<OsclMemAllocator> curpath(".");
        curpath += OSCL_FILE_CHAR_PATH_DELIMITER;
        pDir = opendir(curpath.get_cstr());
    }
    if (pDir == NULL)
    {

        lastError = Oscl_FileFind::E_PATH_NOT_FOUND;
        return NULL;
    }
    // parsing thru dirent structure
    while ((pEnt = readdir(pDir)) != NULL)
    {
        if (oscl_strglob(pEnt->d_name, pattern) &&
                oscl_strcmp(pEnt->d_name, ".") &&
                oscl_strcmp(pEnt->d_name, ".."))	// excluded out '.' and '..' from readdir
        {	// pattern matched
            iDirEntVec.push_back(pEnt->d_name);
            // d_type is not all available on all lunix system, using stat() instead
            if (itr == 0)
            {
                if (strlen(pEnt->d_name) >= buflen)
                {
                    lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
                    return NULL;
                }
                // need to return the first found element
                oscl_strncpy(buf, pEnt->d_name, buflen);
                if (stat(pEnt->d_name, &statbuf) == 0)
                {
                    type = (S_ISDIR(statbuf.st_mode)) ? DIR_TYPE : FILE_TYPE;
                }
                else
                {
                    type = FILE_TYPE;
                }
            }
            itr++;
        }
    }
    closedir(pDir);
    if (iDirEntVec.size())
    {
        haveGlob = true;
        count = 1; // advance to next element, used for findnext()
        return iDirEntVec[0].get_str();
    }
    lastError = Oscl_FileFind::E_NO_MATCH;
#endif
    return NULL;
}

OSCL_EXPORT_REF const oscl_wchar *Oscl_FileFind::FindFirst(const oscl_wchar *directory, const oscl_wchar *pattern, oscl_wchar *buf, uint32 buflen)
{
    const oscl_wchar *def_pattern = _STRLIT_WCHAR("*");
    lastError = Oscl_FileFind::E_OK;
    type = Oscl_FileFind::INVALID_TYPE;
    if (directory == NULL || buf == NULL || buflen <= 0)
    {
        lastError = E_INVALID_ARG;
        return NULL;
    }

    if (pattern == NULL) pattern = def_pattern;

#if   (OSCL_HAS_GLOB)
    char convpattern[OSCL_IO_EXTENSION_MAXLEN];
    char convdir[OSCL_IO_FILENAME_MAXLEN];
    char utf8buf[OSCL_IO_FILENAME_MAXLEN];
    const char *retval;

    if (haveGlob || !directory || !buf || buflen <= 0)
    {
        lastError = (haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }

    if ((0 == oscl_UnicodeToUTF8(directory, oscl_strlen(directory), convdir, OSCL_IO_FILENAME_MAXLEN)
            && oscl_strlen(directory))
            || (0 == oscl_UnicodeToUTF8(pattern, oscl_strlen(pattern), convpattern, OSCL_IO_EXTENSION_MAXLEN)
                && oscl_strlen(pattern)))
    {
        lastError = Oscl_FileFind::E_PATH_TOO_LONG;
        return NULL;
    }

    retval = FindFirst(convdir, convpattern, utf8buf, OSCL_IO_FILENAME_MAXLEN);

    if (retval != NULL)
    {
        if (0 == oscl_UTF8ToUnicode(retval, oscl_strlen(retval), buf, buflen) && oscl_strlen(retval))
        {
            lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
            return NULL;
        }
        return buf;
    }
#else
    char convpattern[OSCL_IO_EXTENSION_MAXLEN];
    char convdir[OSCL_IO_FILENAME_MAXLEN];
    char utf8buf[OSCL_IO_FILENAME_MAXLEN];
    const char *retval;

    if (haveGlob || !directory || !buf || buflen <= 0)
    {
        lastError = (haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }
    if ((0 == oscl_UnicodeToUTF8(directory, oscl_strlen(directory), convdir, OSCL_IO_FILENAME_MAXLEN) &&
            oscl_strlen(directory) != 0) ||
            (0 == oscl_UnicodeToUTF8(pattern, oscl_strlen(pattern), convpattern, OSCL_IO_EXTENSION_MAXLEN) &&
             oscl_strlen(pattern) != 0))
    {
        lastError = Oscl_FileFind::E_PATH_TOO_LONG;
        return NULL;
    }

    retval = FindFirst(convdir, convpattern, utf8buf, OSCL_IO_FILENAME_MAXLEN);

    if (retval != NULL)
    {
        if (0 == oscl_UTF8ToUnicode(retval, oscl_strlen(retval), buf, buflen) && oscl_strlen(retval))
        {
            lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
            return NULL;
        }
        return buf;
    }
#endif

    return NULL;
}

OSCL_EXPORT_REF char *Oscl_FileFind::FindNext(char *buf, uint32 buflen)
{
    lastError = Oscl_FileFind::E_OK;
    type = Oscl_FileFind::INVALID_TYPE;
#if   (OSCL_HAS_GLOB)
    if (!haveGlob || !buf || buflen <= 0)
    {
        lastError = (!haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }
    if (count >= hFind.gl_pathc)
    {
        lastError = Oscl_FileFind::E_PATH_NOT_FOUND;
        return NULL;
    }
    if (oscl_strlen(hFind.gl_pathv[count]) >= buflen)
    {
        lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
        return NULL;
    }
    else
    {
        oscl_strncpy(buf, hFind.gl_pathv[count++], buflen);
        struct stat statbuf;
        if (stat(buf, &statbuf) == 0)
        {
            type = (S_ISDIR(statbuf.st_mode)) ? DIR_TYPE : FILE_TYPE;
        }
        else
        {
            type = FILE_TYPE;
        }
        return buf;
    }
#else
    if (!haveGlob || !buf || buflen <= 0)
    {
        lastError = (!haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }
    if (count >= iDirEntVec.size())
    {
        lastError = Oscl_FileFind::E_PATH_NOT_FOUND;
        return NULL;
    }
    if (oscl_strlen(iDirEntVec[count].get_cstr()) > buflen)
    {
        lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
    }
    else
    {
        oscl_strncpy(buf, iDirEntVec[count++].get_cstr(), buflen);
        struct stat statbuf;
        if (stat(buf, &statbuf) == 0)
        {
            type = (S_ISDIR(statbuf.st_mode)) ? DIR_TYPE : FILE_TYPE;
        }
        else
        {
            type = FILE_TYPE;
        }
        return buf;
    }
#endif
    return NULL;
}

OSCL_EXPORT_REF oscl_wchar *Oscl_FileFind::FindNext(oscl_wchar *buf, uint32 buflen)
{
    lastError = Oscl_FileFind::E_OK;
    type = Oscl_FileFind::INVALID_TYPE;
#if   (OSCL_HAS_GLOB)
    char utf8buf[OSCL_IO_FILENAME_MAXLEN];
    char *retval;

    if (!haveGlob || !buf || buflen <= 0)
    {
        lastError = (!haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }

    retval = FindNext(utf8buf, OSCL_IO_FILENAME_MAXLEN);

    if (retval != NULL)
    {
        if (0 == oscl_UTF8ToUnicode(retval, oscl_strlen(retval), buf, buflen) && oscl_strlen(retval))
        {
            lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
            return NULL;
        }
        return buf;
    }
#else
    char utf8buf[OSCL_IO_FILENAME_MAXLEN];
    char *retval;
    if (!haveGlob || !buf || buflen <= 0)
    {
        lastError = (!haveGlob) ?
                    Oscl_FileFind::E_INVALID_STATE :
                    Oscl_FileFind::E_INVALID_ARG;
        return NULL;
    }
    if (count >= iDirEntVec.size())
    {
        lastError = Oscl_FileFind::E_PATH_NOT_FOUND;
        return NULL;
    }
    retval = FindNext(utf8buf, OSCL_IO_FILENAME_MAXLEN);

    if (retval != NULL)
    {
        if (0 == oscl_UTF8ToUnicode(retval, oscl_strlen(retval), buf, buflen))
        {
            lastError = Oscl_FileFind::E_BUFFER_TOO_SMALL;
            return NULL;
        }
        return buf;
    }
#endif
    return NULL;
}

OSCL_EXPORT_REF void Oscl_FileFind::Close()
{
#if   (OSCL_HAS_GLOB)
    if (haveGlob)
        globfree(&hFind);
    haveGlob = false;
    count = 0;
    lastError = Oscl_FileFind::E_OK;
#else
    iDirEntVec.clear();
    count = 0;
    haveGlob = false;
    lastError = Oscl_FileFind::E_OK;
#endif
}

OSCL_EXPORT_REF Oscl_FileFind::element_type Oscl_FileFind::GetElementType()
{
    return type;
}

OSCL_EXPORT_REF Oscl_FileFind::error_type Oscl_FileFind::GetLastError()
{
    return lastError;
}

// globmatch matches pattern strings p from str, follows linux glob.c man spec.
static bool oscl_strglob(const char *str, const char *p)
{
#define NEGATE	'^'			/* std cset negation char */
    int negate;
    int match;
    int c;

    while (*p)
    {
        if (!*str && *p != '*')
            return false;

        switch (c = *p++)
        {

            case '*':
                while (*p == '*')
                    p++;

                if (!*p)
                    return true;

                if (*p != '?' && *p != '[' && *p != '\\')
                    while (*str && *p != *str)
                        str++;

                while (*str)
                {
                    if (oscl_strglob(str, p))
                        return true;
                    str++;
                }
                return false;

            case '?':
                if (*str)
                    break;
                return false;
                /*
                 * set specification is inclusive, that is [a-z] is a, z and
                 * everything in between. this means [z-a] may be interpreted
                 * as a set that contains z, a and nothing in between.
                 */
            case '[':
                if (*p != NEGATE)
                    negate = false;
                else
                {
                    negate = true;
                    p++;
                }

                match = false;

                while (!match && (c = *p++))
                {
                    if (!*p)
                        return false;
                    if (*p == '-')  	/* c-c */
                    {
                        if (!*++p)
                            return false;
                        if (*p != ']')
                        {
                            if (*str == c || *str == *p ||
                                    (*str > c && *str < *p))
                                match = true;
                        }
                        else  		/* c-] */
                        {
                            if (*str >= c)
                                match = true;
                            break;
                        }
                    }
                    else  			/* cc or c] */
                    {
                        if (c == *str)
                            match = true;
                        if (*p != ']')
                        {
                            if (*p == *str)
                                match = true;
                        }
                        else
                            break;
                    }
                }

                if (negate == match)
                    return false;
                /*
                 * if there is a match, skip past the cset and continue on
                 */
                while (*p && *p != ']')
                    p++;
                if (!*p++)
                    return false;
                break;

            case '\\':
                if (*p)
                    c = *p++;
            default:
                if (c != *str)
                    return false;
                break;

        }
        str++;
    }

    return !*str;
}

