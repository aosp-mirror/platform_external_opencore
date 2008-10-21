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
#include "oscl_shared_library.h"
#include "oscl_string.h"
#include "pvlogger.h"
#include "oscl_utf8conv.h"
#include "oscl_uuid.h"
#include "oscl_file_types.h"

#define BUFFER_SIZE 256

typedef void* (*GetInterface_t)();


OSCL_EXPORT_REF OsclSharedLibrary::OsclSharedLibrary()
{
    ipLogger = PVLogger::GetLoggerObject("oscllib");

    iRefCount = 0;
    ipHandle = NULL;

}

OSCL_EXPORT_REF OsclSharedLibrary::OsclSharedLibrary(const OSCL_String& aPath)
{
    ipLogger = PVLogger::GetLoggerObject("oscllib");

    iRefCount = 0;
    ipHandle = NULL;
    iLibPath.set(aPath.get_cstr(), oscl_strlen(aPath.get_cstr()));
}

OSCL_EXPORT_REF OsclSharedLibrary::~OsclSharedLibrary()
{
    ipLogger = NULL;

    if (NULL != ipHandle)
    {
        Close();
    }
}

OSCL_EXPORT_REF OsclLibStatus OsclSharedLibrary::LoadLib(const OSCL_String& aPath)
{
    iLibPath.set(aPath.get_cstr(), oscl_strlen(aPath.get_cstr()));

    // Clear any errors
    dlerror();
    // Open the library
    void* library = dlopen(aPath.get_cstr(), RTLD_NOW);
    // dlopen() returns NULL if there were any issues opening the library
    if (NULL == library)
    {
        // check for errors
        const char* pErr = dlerror();
        if (NULL == pErr)
        {
            // No error reported, but no handle to the library
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_WARNING,
                            (0, "OsclLib::LoadLibrary: Error opening "
                             "library (%s) but no error reported\n",
                             aPath.get_cstr(), pErr));
        }
        else
        {
            // Error reported
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_WARNING,
                            (0, "OsclLib::LoadLibrary: Error opening "
                             "library (%s): %s\n", aPath.get_cstr(), pErr));
        }
        return OsclLibFail;
    }
    ipHandle = library;
    return OsclLibSuccess;

}

OSCL_EXPORT_REF OsclLibStatus OsclSharedLibrary::LoadLib()
{
    return LoadLib(iLibPath);
}

OSCL_EXPORT_REF void OsclSharedLibrary::SetLibPath(const OSCL_String& aPath)
{
    iLibPath.set(aPath.get_cstr(), oscl_strlen(aPath.get_cstr()));
}

OSCL_EXPORT_REF OsclLibStatus OsclSharedLibrary::QueryInterface(const OsclUuid& aInterfaceId,
        OsclAny*& aInterfacePtr)
{
    OsclSharedLibraryInterface *pSharedLibInterface = NULL;
    aInterfacePtr = NULL;
    // Look up the GetInterface method
    if (NULL == ipHandle)
    {
        return OsclLibFail;
    }
    GetInterface_t GetInterface =
        (GetInterface_t)dlsym(ipHandle, "GetInterface");
    // dlsym() returns NULL if there were any issues getting the
    // address of the symbol
    if (NULL == GetInterface)
    {
        // check for errors
        const char* pErr = dlerror();
        if (NULL == pErr)
        {
            // No error reported, but no symbol was found
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_INFO,
                            (0, "OsclLib::QueryInterface: Could not access GetInterface "
                             "symbol in library but no error reported\n"));
        }
        else
        {
            // Error reported
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_INFO,
                            (0, "OsclLib::QueryInterface: Could not access GetInterface "
                             "symbol in library: %s\n", pErr));
        }
        return OsclLibFail;
    }

    // Get the OsclSharedLibraryInterface
    pSharedLibInterface = (OsclSharedLibraryInterface*)GetInterface();
    if (NULL == pSharedLibInterface)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_INFO,
                        (0, "OsclSharedLibrary::QueryInterface: Could not access the "
                         "library pointer function"));
        return OsclLibFail;
    }
    // Lookup the interface ID
    aInterfacePtr = pSharedLibInterface->SharedLibraryLookup(aInterfaceId);
    if (NULL == aInterfacePtr)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_INFO,
                        (0, "OsclLib::QueryInterface: NO library interface found for aInterfaceId."));
        return OsclLibFail;
    }
    return OsclLibSuccess;
}

OSCL_EXPORT_REF OsclLibStatus OsclSharedLibrary::Close()
{
    if (iRefCount > 0)
    {
        return OsclLibFail;
    }

    if (NULL == ipHandle)
    {
        return OsclLibFail;
    }
    if (0 != dlclose(ipHandle))
    {
        // dlclose() returns non-zero value if close failed, check for errors
        const char* pErr = dlerror();
        if (NULL != pErr)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_WARNING,
                            (0, "OsclSharedLibrary::Close: Error closing library: %s\n", pErr));
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, ipLogger, PVLOGMSG_WARNING,
                            (0, "OsclSharedLibrary::Close: Error closing library, no error reported"
                             "\n"));
        }
        return OsclLibFail;
    }
    ipHandle = NULL;
    return OsclLibSuccess;
}

OSCL_EXPORT_REF void OsclSharedLibrary::AddRef()
{
    ++iRefCount;
}

OSCL_EXPORT_REF void OsclSharedLibrary::RemoveRef()
{
    --iRefCount;
}

