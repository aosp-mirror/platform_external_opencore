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
#include "pv_omxdefs.h"
#include "omx_component.h"
#include "pv_omxcore.h"
#include "pv_omxwrapperbase.h"

#include "pv_omxmastercore.h"
#include "oscl_mutex.h"

#include "qc_omxcore.h"

//Number of base instances
OsclMutex g_OMX_Mutex;
OMX_U32 g_NumMasterOMXInstances = 0;
void *g_Wrapper = NULL;
void *g_MasterRegistry = NULL;
void *g_OMXCompHandles = NULL;
OMX_U32 g_TotalNumOMXComponents = 0;

typedef struct PVOMXMasterRegistryStruct
{
    OMX_U8 CompName[PV_OMX_MAX_COMPONENT_NAME_LENGTH];
    OMX_U8 CompRole[PV_OMX_MAX_COMPONENT_NAME_LENGTH];
    OMX_U32 OMXCoreIndex;
} PVOMXMasterRegistryStruct;

typedef struct PVOMXCompHandles
{
    OMX_HANDLETYPE handle;
    OMX_U32 OMXCoreIndex;
} PVOMXCompHandles;

// this is the number of vendor OMX cores
#if HARDWARE_OMX
#define NUMBER_OF_OMX_CORES 2
#else
#define NUMBER_OF_OMX_CORES 1
#endif

#define MAX_NUMBER_OF_OMX_COMPONENTS 50

OMX_ERRORTYPE OMX_APIENTRY  PV_MasterOMX_Init()
{
    OMX_ERRORTYPE Status = OMX_ErrorNone;
    OMX_U32 jj;
    OMX_U32 index;
    OMX_U32 master_index = 0;

    g_OMX_Mutex.Lock();
    g_NumMasterOMXInstances++;

    if (g_NumMasterOMXInstances == 1)
    {

        // array of ptrs to various cores
        PV_OMX_WrapperBase **pWrapper = (PV_OMX_WrapperBase **)malloc(NUMBER_OF_OMX_CORES * sizeof(PV_OMX_WrapperBase *));
        if (pWrapper == NULL)
        {
            g_OMX_Mutex.Unlock();
            return OMX_ErrorInsufficientResources;
        }
        // set the global ptr to this array
        g_Wrapper = (void*)pWrapper;

        PVOMXMasterRegistryStruct *pOMXMasterRegistry = (PVOMXMasterRegistryStruct *)malloc(MAX_NUMBER_OF_OMX_COMPONENTS * sizeof(PVOMXMasterRegistryStruct));
        if (pOMXMasterRegistry == NULL)
        {
            g_OMX_Mutex.Unlock();
            return OMX_ErrorInsufficientResources;
        }
        g_MasterRegistry = (void*)pOMXMasterRegistry;

        PVOMXCompHandles *pOMXCompHandles = (PVOMXCompHandles *)malloc(MAX_NUMBER_OF_OMX_COMPONENTS * sizeof(PVOMXCompHandles));
        if (pOMXCompHandles == NULL)
        {
            g_OMX_Mutex.Unlock();
            return OMX_ErrorInsufficientResources;
        }
        g_OMXCompHandles = (void*)pOMXCompHandles;
        // init the array
        memset(pOMXCompHandles, 0, MAX_NUMBER_OF_OMX_COMPONENTS*sizeof(PVOMXCompHandles));


        // initialize pointers to omx methods for different vendor OMX cores

        //pWrapper[0] = Vendor0_Wrapper::New();
        //pWrapper[1] = Vendor1_Wrapper::New();

        //NOTE: Instantiate PV as the last core
#if HARDWARE_OMX
        pWrapper[0] = QC_OMX_Wrapper::New();
#endif
        pWrapper[NUMBER_OF_OMX_CORES-1] = PV_OMX_Wrapper::New(); // initialize pointers to omx methods

        // loop over all cores
        master_index = 0;
        OMX_STRING ComponentName = (OMX_STRING) malloc(PV_OMX_MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8));

        for (jj = 0; jj < NUMBER_OF_OMX_CORES; jj++)
        {
            // first call OMX_Init
            Status = (*(pWrapper[jj]->GetpOMX_Init()))();
            if (Status == OMX_ErrorNone)
            {
                // enumerate components to get their number etc.
                OMX_ERRORTYPE stat = OMX_ErrorNone;

                index = 0;
                while (stat != OMX_ErrorNoMore)
                {
                    // clear, then get next  component name
                    memset(ComponentName, 0, PV_OMX_MAX_COMPONENT_NAME_LENGTH*sizeof(OMX_U8));

                    stat = (*(pWrapper[jj]->GetpOMX_ComponentNameEnum()))(
                               ComponentName,
                               PV_OMX_MAX_COMPONENT_NAME_LENGTH,
                               index);
                    if (stat == OMX_ErrorNoMore)
                        break;

                    // check number roles of the component
                    OMX_U32 numRoles;
                    numRoles = 0;
                    stat = (*(pWrapper[jj]->GetpOMX_GetRolesOfComponent()))(
                               ComponentName,
                               &numRoles,
                               NULL);
                    if ((numRoles > 0) && (stat == OMX_ErrorNone))
                    {
                        // allocate space for roles of the component
                        OMX_U32 role;
                        OMX_U8 **ComponentRoles = (OMX_U8 **) malloc(numRoles * sizeof(OMX_U8 *));

                        for (role = 0;role < numRoles; role++)
                            ComponentRoles[role] = (OMX_U8 *) malloc(PV_OMX_MAX_COMPONENT_NAME_LENGTH * sizeof(OMX_U8));


                        // get the array of strings with component roles
                        stat = (*(pWrapper[jj]->GetpOMX_GetRolesOfComponent()))(
                                   ComponentName,
                                   &numRoles,
                                   ComponentRoles);

                        //register all components separately in master registry
                        if (stat == OMX_ErrorNone)
                        {
                            for (role = 0;role < numRoles; role++)
                            {
                                strncpy((OMX_STRING)pOMXMasterRegistry[master_index].CompName, ComponentName, PV_OMX_MAX_COMPONENT_NAME_LENGTH);
                                strncpy((OMX_STRING)pOMXMasterRegistry[master_index].CompRole, (OMX_STRING)ComponentRoles[role], PV_OMX_MAX_COMPONENT_NAME_LENGTH);
                                pOMXMasterRegistry[master_index].OMXCoreIndex = jj;
                                master_index++;
                            }
                        }

                        // dealloc space for component roles
                        for (role = 0;role < numRoles; role++)
                            free(ComponentRoles[role]);

                        free(ComponentRoles);


                    } // done with roles of component represented by index

                    index++; // get next component from the jj-th core

                } // done with all components from jj-th core

            } // end of if(Status==OMX_ErrorNone)

        }//end of for(jj=...
        // at this point, all cores were init, and all components registered

        free(ComponentName);
        g_TotalNumOMXComponents = master_index;
    }

    g_OMX_Mutex.Unlock();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMX_APIENTRY PV_MasterOMX_Deinit()
{
    OMX_U32 jj;
    OMX_ERRORTYPE Status = OMX_ErrorNone;

    g_OMX_Mutex.Lock();
    g_NumMasterOMXInstances--;
    if (g_NumMasterOMXInstances == 0)
    {
        //free master registry
        g_TotalNumOMXComponents = 0;
        PVOMXMasterRegistryStruct *pOMXMasterRegistry = (PVOMXMasterRegistryStruct *) g_MasterRegistry;
        if (pOMXMasterRegistry)
            free(pOMXMasterRegistry);
        g_MasterRegistry = NULL;

        PVOMXCompHandles *pOMXCompHandles = (PVOMXCompHandles *)g_OMXCompHandles;
        if (pOMXCompHandles)
            free(pOMXCompHandles);
        g_OMXCompHandles = NULL;

        PV_OMX_WrapperBase **pWrapper = (PV_OMX_WrapperBase **) g_Wrapper;
        //call Deinit for each core
        if (pWrapper)
        {
            for (jj = 0; jj < NUMBER_OF_OMX_CORES; jj++)
            {
                Status = (*(pWrapper[jj]->GetpOMX_Deinit()))();
                pWrapper[jj]->Delete();
            }

            free(pWrapper);
            g_Wrapper = NULL;
        }


    }
    g_OMX_Mutex.Unlock();
    return OMX_ErrorNone;
}

// look for the component
OMX_API OMX_ERRORTYPE PV_MasterOMX_GetComponentsOfRole(
    OMX_IN	OMX_STRING role,
    OMX_INOUT	OMX_U32	*pNumComps,
    OMX_INOUT	OMX_U8	**compNames)
{

    OMX_U32 ii;
    // initialize
    *pNumComps = 0;
    g_OMX_Mutex.Lock();
    PVOMXMasterRegistryStruct *pOMXMasterRegistry = (PVOMXMasterRegistryStruct *) g_MasterRegistry;
    if (pOMXMasterRegistry == NULL)
    {
        g_OMX_Mutex.Unlock();
        return OMX_ErrorNone;
    }


    // go through all components and check if they support the given role
    for (ii = 0; ii < g_TotalNumOMXComponents; ii ++)
    {
        // if the role matches, increment the counter and record the comp. name
        if (!strcmp((OMX_STRING)pOMXMasterRegistry[ii].CompRole, role))
        {
            // if a placeholder for compNames is provided, copy the component name into it
            if (compNames != NULL)
            {
                strcpy((OMX_STRING) compNames[*pNumComps], (OMX_STRING)pOMXMasterRegistry[ii].CompName);
            }
            // increment the counter
            *pNumComps = (*pNumComps + 1);

        }
    }

    g_OMX_Mutex.Unlock();
    return OMX_ErrorNone;

}


OMX_API OMX_ERRORTYPE OMX_APIENTRY 	PV_MasterOMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks)
{
    OMX_ERRORTYPE Status = OMX_ErrorNone;
    OMX_U32 ii, kk;

    g_OMX_Mutex.Lock();
    PVOMXMasterRegistryStruct *pOMXMasterRegistry = (PVOMXMasterRegistryStruct *) g_MasterRegistry;
    if (pOMXMasterRegistry == NULL)
    {
        g_OMX_Mutex.Unlock();
        return OMX_ErrorComponentNotFound;
    }
    PVOMXCompHandles *pOMXCompHandles = (PVOMXCompHandles *)g_OMXCompHandles;
    if (pOMXCompHandles == NULL)
    {
        g_OMX_Mutex.Unlock();
        return OMX_ErrorComponentNotFound;
    }

    for (ii = 0; ii < g_TotalNumOMXComponents; ii ++)
    {
        // go through the list of supported components and find the component based on its name (identifier)
        if (!strcmp((OMX_STRING) pOMXMasterRegistry[ii].CompName, cComponentName))
        {
            // found a matching name
            break;
        }

    }

    if (ii == g_TotalNumOMXComponents)
    {
        // could not find a component with the given name
        g_OMX_Mutex.Unlock();
        return OMX_ErrorComponentNotFound;
    }

    // call the appropriate GetHandle for the component
    PV_OMX_WrapperBase **pWrapper = (PV_OMX_WrapperBase **) g_Wrapper;
    if (pWrapper)
    {
        //save component handle with the OMX core index, so it can be retrieved
        // later when freehandle is called
        // find an empty slot to write the pair handle/index:
        for (kk = 0;kk < MAX_NUMBER_OF_OMX_COMPONENTS; kk++)
        {
            if (pOMXCompHandles[kk].handle == NULL)
            {
                break;
            }
        }
        if (kk == MAX_NUMBER_OF_OMX_COMPONENTS)
        {
            g_OMX_Mutex.Unlock();
            return OMX_ErrorComponentNotFound;
        }

        OMX_U32 index = pOMXMasterRegistry[ii].OMXCoreIndex;
        Status = (*(pWrapper[index]->GetpOMX_GetHandle()))(pHandle, cComponentName, pAppData, pCallBacks);
        if (Status == OMX_ErrorNone)
        {
            // write the pair handle/index
            pOMXCompHandles[kk].handle = *pHandle;
            pOMXCompHandles[kk].OMXCoreIndex = index;
        }
        g_OMX_Mutex.Unlock();
        return Status;
    }
    else
    {
        g_OMX_Mutex.Unlock();
        return OMX_ErrorInsufficientResources;
    }

}

OMX_API OMX_ERRORTYPE OMX_APIENTRY PV_MasterOMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent)
{

    OMX_ERRORTYPE Status = OMX_ErrorNone;
    OMX_U32 ii;
// here, we need to first find the handle among instantiated components
// then we retrieve the core based on component handle
// finally, call the OMX_FreeHandle for appropriate core
    g_OMX_Mutex.Lock();
    PVOMXCompHandles *pOMXCompHandles = (PVOMXCompHandles *)g_OMXCompHandles;
    if (pOMXCompHandles == NULL)
    {
        g_OMX_Mutex.Unlock();
        return OMX_ErrorComponentNotFound;
    }
    for (ii = 0; ii < MAX_NUMBER_OF_OMX_COMPONENTS; ii ++)
    {
        // go through the list of supported components and find the component handle
        if (pOMXCompHandles[ii].handle == hComponent)
        {
            // found a matching handle
            break;
        }

    }
    if (ii == MAX_NUMBER_OF_OMX_COMPONENTS)
    {
        // could not find a component with the given name
        g_OMX_Mutex.Unlock();
        return OMX_ErrorComponentNotFound;
    }

    // call the appropriate GetHandle for the component
    PV_OMX_WrapperBase **pWrapper = (PV_OMX_WrapperBase **) g_Wrapper;
    if (pWrapper)
    {
        OMX_U32 index = pOMXCompHandles[ii].OMXCoreIndex;
        Status = (*(pWrapper[index]->GetpOMX_FreeHandle()))(hComponent);
        //we're done with this, so get rid of the component handle
        pOMXCompHandles[ii].handle = NULL;
        g_OMX_Mutex.Unlock();
        return Status;
    }
    else
    {
        g_OMX_Mutex.Unlock();
        return OMX_ErrorInsufficientResources;
    }

}

