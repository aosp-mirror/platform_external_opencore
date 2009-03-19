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
#include "omx_m4v_component_interface.h"

#include "pv_omxdefs.h"
#include "omx_mpeg4_component.h"

#ifdef HAS_OSCL_LIB_SUPPORT
#include "oscl_shared_library.h"

#include "pvmf_node_shared_lib_interface.h"
#include "pv_omx_shared_lib_interface.h"

#define M4V_MAX_LIB_PATH 32

#define OMX_M4V_LIB_NAME "libopencoremp4.so"

// Since iOmxLib is a static member variable, it must be assigned here before
// it can be used.
OsclSharedLibrary* OmxM4vComponentFactory::iOmxLib = NULL;
int OmxM4vComponentFactory::iRefCount = 0;

#else
// External factory functions needed for the creation of each component. Defined
// in codec_v2/omx/omx_m4v/src/omx_mpeg4_component.cpp.
extern OMX_ERRORTYPE Mpeg4OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE Mpeg4OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

extern OMX_ERRORTYPE H263OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE H263OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

#endif  // HAS_OSCL_LIB_SUPPORT

// This function is called by OMX_GetHandle and it creates an instance of the m4v component AO
// by loading the library where libomx_m4v_component_lib is located
OMX_ERRORTYPE OmxM4vComponentFactory::M4vCreate(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData)
{
#ifdef HAS_OSCL_LIB_SUPPORT
    OSCL_StackString<M4V_MAX_LIB_PATH> omxLibName(OMX_M4V_LIB_NAME);
    OsclSharedLibrary* lib = NULL;

    // If iOmxLib is NULL, this is the first time this has been called.
    // Theoretically, there shouldn't ever be a case where iOmxLib is opened
    // twice, but this check is in here just in case.
    if (NULL == OmxM4vComponentFactory::iOmxLib)
    {
        lib = OSCL_NEW(OsclSharedLibrary, (omxLibName));
    }
    else
    {
        lib = OmxM4vComponentFactory::iOmxLib;
    }

    // Keep track of the number of times iOmxLib is accessed
    OmxM4vComponentFactory::iRefCount++;

    // Load the associated library. If successful, call the corresponding
    // create function for OMX M4V.
    if (OsclLibSuccess == lib->LoadLib())
    {
        OsclAny* interfacePtr = NULL;
        lib->QueryInterface(PV_OMX_SHARED_INTERFACE, (OsclAny*&)interfacePtr);

        OmxSharedLibraryInterface* omxIntPtr =
            OSCL_DYNAMIC_CAST(OmxSharedLibraryInterface*, interfacePtr);

        OsclAny* createCompTemp =
            omxIntPtr->QueryOmxComponentInterface(PV_OMX_M4V_TYPE, PV_OMX_CREATE_INTERFACE);

        // createComp is the function pointer to store the creation function
        // for the omx component
        OMX_ERRORTYPE(*createComp)(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
        createComp =
            OSCL_DYNAMIC_CAST(OMX_ERRORTYPE(*)(OMX_OUT OMX_HANDLETYPE*, OMX_IN OMX_PTR), createCompTemp);

        OMX_ERRORTYPE returnStatus = (*createComp)(pHandle, pAppData);

        // Store the shared library so it can be closed later
        OmxM4vComponentFactory::iOmxLib = lib;

        return returnStatus;
    }
    else
    {
        lib->Close();

        // If this is the last time to close the library, delete the
        // OsclSharedLibrary object and be sure to set iOmxLib back to NULL
        OmxM4vComponentFactory::iRefCount--;
        if (0 == OmxM4vComponentFactory::iRefCount)
        {
            OSCL_DELETE(lib);
            OmxM4vComponentFactory::iOmxLib = NULL;
        }
    }
    return OMX_ErrorUndefined;
#else
    return Mpeg4OmxComponentFactory(pHandle, pAppData);
#endif
}

OMX_ERRORTYPE OmxM4vComponentFactory::M4vDestructor(OMX_IN OMX_HANDLETYPE pHandle)
{
#ifdef HAS_OSCL_LIB_SUPPORT
    OsclSharedLibrary* lib = OmxM4vComponentFactory::iOmxLib;

    // lib must not be NULL at this point. If the omx component has been
    // created, lib is necessary to destroy the component correctly and
    // cleanup used memory.
    OSCL_ASSERT(NULL != lib);

    // Need to get the function pointer for the destroy function through the
    // shared library interface.
    OsclAny* interfacePtr = NULL;
    lib->QueryInterface(PV_OMX_SHARED_INTERFACE, (OsclAny*&)interfacePtr);

    OmxSharedLibraryInterface* omxIntPtr =
        OSCL_DYNAMIC_CAST(OmxSharedLibraryInterface*, interfacePtr);

    OsclAny* destroyCompTemp =
        omxIntPtr->QueryOmxComponentInterface(PV_OMX_M4V_TYPE, PV_OMX_DESTROY_INTERFACE);

    // destroyComp is the function pointer to store the function for
    // destroying the omx m4v component.
    OMX_ERRORTYPE(*destroyComp)(OMX_IN OMX_HANDLETYPE pHandle);
    destroyComp =
        OSCL_DYNAMIC_CAST(OMX_ERRORTYPE(*)(OMX_IN OMX_HANDLETYPE), destroyCompTemp);

    OMX_ERRORTYPE returnStatus = (*destroyComp)(pHandle);

    // Finish memory cleanup by closing the shared library and deleting
    lib->Close();

    // If this is the last time to close the library, delete the
    // OsclSharedLibrary object and be sure to set iOmxLib back to NULL
    OmxM4vComponentFactory::iRefCount--;
    if (0 == OmxM4vComponentFactory::iRefCount)
    {
        OSCL_DELETE(lib);
        OmxM4vComponentFactory::iOmxLib = NULL;
    }

    return returnStatus;
#else
    return Mpeg4OmxComponentDestructor(pHandle);
#endif
}

// This function is called by OMX_GetHandle and it creates an instance of the h263 component AO
// by loading the library where libomx_m4v_component_lib is located
OMX_ERRORTYPE OmxM4vComponentFactory::H263Create(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData)
{
#ifdef HAS_OSCL_LIB_SUPPORT
    OSCL_StackString<M4V_MAX_LIB_PATH> omxLibName(OMX_M4V_LIB_NAME);
    OsclSharedLibrary* lib = NULL;

    // If iOmxLib is NULL, this is the first time this has been called.
    // Theoretically, there shouldn't ever be a case where iOmxLib is opened
    // twice, but this check is in here just in case.
    if (NULL == OmxM4vComponentFactory::iOmxLib)
    {
        lib = OSCL_NEW(OsclSharedLibrary, (omxLibName));
    }
    else
    {
        lib = OmxM4vComponentFactory::iOmxLib;
    }

    // Keep track of the number of times iOmxLib is accessed
    OmxM4vComponentFactory::iRefCount++;

    // Load the associated library. If successful, call the corresponding
    // create function for OMX H263.
    if (OsclLibSuccess == lib->LoadLib())
    {
        OsclAny* interfacePtr = NULL;
        lib->QueryInterface(PV_OMX_SHARED_INTERFACE, (OsclAny*&)interfacePtr);

        OmxSharedLibraryInterface* omxIntPtr =
            OSCL_DYNAMIC_CAST(OmxSharedLibraryInterface*, interfacePtr);

        OsclAny* createCompTemp =
            omxIntPtr->QueryOmxComponentInterface(PV_OMX_H263_TYPE, PV_OMX_CREATE_INTERFACE);

        // createComp is the function pointer to store the creation function
        // for the omx component
        OMX_ERRORTYPE(*createComp)(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
        createComp =
            OSCL_DYNAMIC_CAST(OMX_ERRORTYPE(*)(OMX_OUT OMX_HANDLETYPE*, OMX_IN OMX_PTR), createCompTemp);

        OMX_ERRORTYPE returnStatus = (*createComp)(pHandle, pAppData);

        // Store the shared library so it can be closed later
        OmxM4vComponentFactory::iOmxLib = lib;

        return returnStatus;
    }
    else
    {
        lib->Close();

        // If this is the last time to close the library, delete the
        // OsclSharedLibrary object and be sure to set iOmxLib back to NULL
        OmxM4vComponentFactory::iRefCount--;
        if (0 == OmxM4vComponentFactory::iRefCount)
        {
            OSCL_DELETE(lib);
            OmxM4vComponentFactory::iOmxLib = NULL;
        }
    }

    return OMX_ErrorUndefined;
#else
    return H263OmxComponentFactory(pHandle, pAppData);
#endif
}

OMX_ERRORTYPE OmxM4vComponentFactory::H263Destructor(OMX_IN OMX_HANDLETYPE pHandle)
{
#ifdef HAS_OSCL_LIB_SUPPORT

    OsclSharedLibrary* lib = OmxM4vComponentFactory::iOmxLib;

    // lib must not be NULL at this point. If the omx component has been
    // created, lib is necessary to destroy the component correctly and
    // cleanup used memory.
    OSCL_ASSERT(NULL != lib);

    // Need to get the function pointer for the destroy function through the
    // shared library interface.
    OsclAny* interfacePtr = NULL;
    lib->QueryInterface(PV_OMX_SHARED_INTERFACE, (OsclAny*&)interfacePtr);

    OmxSharedLibraryInterface* omxIntPtr =
        OSCL_DYNAMIC_CAST(OmxSharedLibraryInterface*, interfacePtr);

    OsclAny* destroyCompTemp =
        omxIntPtr->QueryOmxComponentInterface(PV_OMX_H263_TYPE, PV_OMX_DESTROY_INTERFACE);

    // destroyComp is the function pointer to store the function for
    // destroying the omx h263 component.
    OMX_ERRORTYPE(*destroyComp)(OMX_IN OMX_HANDLETYPE pHandle);
    destroyComp =
        OSCL_DYNAMIC_CAST(OMX_ERRORTYPE(*)(OMX_IN OMX_HANDLETYPE), destroyCompTemp);

    OMX_ERRORTYPE returnStatus = (*destroyComp)(pHandle);

    // Finish memory cleanup by closing the shared library and deleting
    lib->Close();

    // If this is the last time to close the library, delete the
    // OsclSharedLibrary object and be sure to set iOmxLib back to NULL
    OmxM4vComponentFactory::iRefCount--;
    if (0 == OmxM4vComponentFactory::iRefCount)
    {
        OSCL_DELETE(lib);
        OmxM4vComponentFactory::iOmxLib = NULL;
    }

    return returnStatus;
#else
    return H263OmxComponentDestructor(pHandle);
#endif
}
