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

#include "oscl_file_dir_utils.h"
#include "oscl_file_types.h"
#include "oscl_utf8conv.h"
#include "oscl_stdstring.h"
#include "oscl_int64_utils.h"
#include "oscl_file_io.h"
#include "osclconfig_error.h"
#include "oscl_file_dir_utils.h"
#include "oscl_mem_basic_functions.h"

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_getcwd(char *path, uint32 size)
{
    if (getcwd(path, size) != NULL)
        return OSCL_FILEMGMT_E_OK;
    return OSCL_FILEMGMT_E_PATH_NOT_FOUND;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_getcwd(oscl_wchar *path, uint32 size)
{
    char convpathname[OSCL_IO_FILENAME_MAXLEN];
    if (oscl_getcwd(convpathname, size > OSCL_IO_FILENAME_MAXLEN ? OSCL_IO_FILENAME_MAXLEN : size) == OSCL_FILEMGMT_E_OK)
    {
        if (0 == oscl_UTF8ToUnicode(convpathname, oscl_strlen(convpathname), path, size) && oscl_strlen(convpathname) != 0)
        {
            return OSCL_FILEMGMT_E_PATH_TOO_LONG;
        }
        return OSCL_FILEMGMT_E_OK;
    }
    return OSCL_FILEMGMT_E_PATH_NOT_FOUND;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_stat(const oscl_wchar *path, OSCL_STAT_BUF *statbuf)
{
    char convpathname[OSCL_IO_FILENAME_MAXLEN];
    if (0 == oscl_UnicodeToUTF8(path, oscl_strlen(path), convpathname, OSCL_IO_FILENAME_MAXLEN) && oscl_strlen(path) != 0)
    {
        return OSCL_FILEMGMT_E_PATH_TOO_LONG;
    }
    return  oscl_stat(convpathname, statbuf);


}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_stat(const char *path, OSCL_STAT_BUF *statbuf)
{
    oscl_memset(statbuf, 0, sizeof(OSCL_STAT_BUF));
    struct stat buf;
    if (stat(path, &buf) == 0)
    {
        if (buf.st_mode & S_IRUSR)
            statbuf->perms |= OSCL_FILEMGMT_PERMS_READ;
        if (buf.st_mode & S_IWUSR)
            statbuf->perms |= OSCL_FILEMGMT_PERMS_WRITE;
        if (buf.st_mode & S_IFDIR)
            statbuf->mode |= OSCL_FILEMGMT_MODE_DIR;
        return OSCL_FILEMGMT_E_OK;
    }
    return OSCL_FILEMGMT_E_PATH_NOT_FOUND;
}


OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_statfs(OSCL_FSSTAT *stats, const char *path)
{
    OSCL_UNUSED_ARG(stats);
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_statfs(OSCL_FSSTAT *stats, const oscl_wchar *path)
{
    OSCL_UNUSED_ARG(stats);
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}


OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_mkdir(const oscl_wchar *path)
{
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_mkdir(const char *path)
{
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_rmdir(const oscl_wchar *path)
{
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_rmdir(const char *path)
{
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_chdir(const oscl_wchar *path)
{
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_chdir(const char *path)
{
    OSCL_UNUSED_ARG(path);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_rename(const oscl_wchar *oldpath, const oscl_wchar *newpath)
{
    OSCL_UNUSED_ARG(oldpath);
    OSCL_UNUSED_ARG(newpath);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}

OSCL_EXPORT_REF OSCL_FILEMGMT_ERR_TYPE oscl_rename(const char *oldpath, const char *newpath)
{
    OSCL_UNUSED_ARG(oldpath);
    OSCL_UNUSED_ARG(newpath);
    return OSCL_FILEMGMT_E_NOT_IMPLEMENTED;
}
