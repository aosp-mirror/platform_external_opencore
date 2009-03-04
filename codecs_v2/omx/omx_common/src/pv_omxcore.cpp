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
#include "pv_omxregistry.h"

//Number of base instances
OMX_U32 NumBaseInstance = 0;
OMX_U32 g_ComponentIndex = 0;
// varaible that counts the number of instances for which OMX init is called
OMX_U32 NumOMXInitInstances = 0;
// Array of supported component types (e.g. MP4, AVC, AAC, etc.)
// they need to be registered
ComponentRegistrationType *pRegTemplateList[MAX_SUPPORTED_COMPONENTS] = {NULL};

#if PROXY_INTERFACE

#include "omx_proxy_interface.h"
extern OsclThreadLock ThreadLock;
ProxyApplication_OMX* pProxyTerm[MAX_INSTANTIATED_COMPONENTS] = {NULL};

#endif //PROXY_INTERFACE



/* Array to store component handles for future recognition of components etc.*/
OMX_HANDLETYPE ComponentHandle[MAX_INSTANTIATED_COMPONENTS] = {NULL};
// array of function pointers. For each component, a destructor function is assigned
OMX_ERRORTYPE(*ComponentDestructor[MAX_INSTANTIATED_COMPONENTS])(OMX_IN OMX_HANDLETYPE pHandle) = {NULL};

#if REGISTER_OMX_M4V_COMPONENT
#ifndef OMX_M4V_COMPONENT_INTERFACE_H_INCLUDED
#include "omx_m4v_component_interface.h"
#endif
OMX_ERRORTYPE Mpeg4Register(ComponentRegistrationType **);
#endif

#if REGISTER_OMX_H263_COMPONENT
#ifndef OMX_M4V_COMPONENT_INTERFACE_H_INCLUDED
#include "omx_m4v_component_interface.h"
#endif
OMX_ERRORTYPE H263Register(ComponentRegistrationType **);
#endif

#if REGISTER_OMX_AVC_COMPONENT
OMX_ERRORTYPE AvcRegister(ComponentRegistrationType **);
#endif

#if REGISTER_OMX_WMV_COMPONENT
#include "omx_wmv_component_interface.h"
OMX_ERRORTYPE WmvRegister(ComponentRegistrationType **);
#endif

#if REGISTER_OMX_AAC_COMPONENT
OMX_ERRORTYPE AacRegister(ComponentRegistrationType **);
#endif

#if REGISTER_OMX_AMR_COMPONENT
OMX_ERRORTYPE AmrRegister(ComponentRegistrationType **);
#endif

#if REGISTER_OMX_MP3_COMPONENT
OMX_ERRORTYPE Mp3Register(ComponentRegistrationType **);
#endif

/* Initializes the component */
OMX_ERRORTYPE PVOMX_Init()
{
    OMX_ERRORTYPE Status = OMX_ErrorNone;
    OMX_U32 ii;
    NumOMXInitInstances ++;

    if (NumOMXInitInstances == 1)
    {
        /* Initialize template list to NULL at the beginning */
        for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
        {
            pRegTemplateList[ii] = NULL;
        }

        for (ii = 0; ii < MAX_INSTANTIATED_COMPONENTS; ii++)
        {
            ComponentHandle[ii] = NULL;
            ComponentDestructor[ii] = NULL;
#if PROXY_INTERFACE
            pProxyTerm[ii] = NULL;
#endif
        }

        // REGISTER COMPONENT TYPES (ONE BY ONE)
#if REGISTER_OMX_M4V_COMPONENT
        // MPEG4
        Status = Mpeg4Register(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif

#if REGISTER_OMX_H263_COMPONENT
        //H263
        Status = H263Register(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif

#if REGISTER_OMX_AVC_COMPONENT
        // AVC
        Status = AvcRegister(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif

#if REGISTER_OMX_WMV_COMPONENT
        // WMV
        Status = WmvRegister(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif

#if REGISTER_OMX_AAC_COMPONENT
        // AAC
        Status = AacRegister(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif

#if REGISTER_OMX_AMR_COMPONENT
        // AMR
        Status = AmrRegister(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif

#if REGISTER_OMX_MP3_COMPONENT
        // MP3
        Status = Mp3Register(pRegTemplateList);
        if (Status != OMX_ErrorNone)
            return Status;
#endif
    }
    return OMX_ErrorNone;
}



/* De-initializes the component*/
OMX_ERRORTYPE PVOMX_Deinit()
{
    OMX_S32 ii;
    NumOMXInitInstances--;
    if (NumOMXInitInstances == 0)
    {

        // go through all component instnaces and delete leftovers if necessary
        for (ii = 0; ii < MAX_INSTANTIATED_COMPONENTS; ii++)
        {

#if PROXY_INTERFACE
            if (pProxyTerm[ii])
            {
                // delete leftover components
                // call the OMX_FreeHandle through the proxy
                if ((ComponentHandle[ii] != NULL) && (ComponentDestructor[ii] != NULL))
                    pProxyTerm[ii]->ProxyFreeHandle(ComponentHandle[ii]);

                // exit thread
                pProxyTerm[ii]->Exit();
                delete pProxyTerm[ii];
                // delete array entries associated with pProxyTerm and Component handle
                pProxyTerm[ii] = NULL;
                ComponentHandle[ii] = NULL;
                ComponentDestructor[ii] = NULL;
            }
#else
            if ((ComponentHandle[ii] != NULL) && (ComponentDestructor[ii] != NULL))
            {

                // call destructor with the corresponding handle as argument
                ComponentDestructor[ii](ComponentHandle[ii]);
            }

            ComponentHandle[ii] = NULL;
            ComponentDestructor[ii] = NULL;
#endif
        }

        //Finally de-register all the components
        for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
        {

            if (pRegTemplateList[ii])
            {
                oscl_free(pRegTemplateList[ii]);
                pRegTemplateList[ii] = NULL;
            }
            else
            {
                break;
            }
        }
    }
    return OMX_ErrorNone;
}


OMX_API OMX_ERRORTYPE OMX_APIENTRY PVOMX_GetHandle(OMX_OUT OMX_HANDLETYPE* pHandle,
        OMX_IN  OMX_STRING cComponentName,
        OMX_IN  OMX_PTR pAppData,
        OMX_IN  OMX_CALLBACKTYPE* pCallBacks)
{
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;

    // First, find an empty slot in the proxy/component handle array to store the component/proxy handle
    OMX_U32 jj;
    for (jj = 0; jj < MAX_INSTANTIATED_COMPONENTS; jj++)
    {
        if (ComponentHandle[jj] == NULL)
            break;
    }
    // can't find a free slot
    if (jj == MAX_INSTANTIATED_COMPONENTS)
    {
        return OMX_ErrorInsufficientResources;
    }
    else
    {
        g_ComponentIndex = jj;
    }

#if PROXY_INTERFACE

    pProxyTerm[g_ComponentIndex] = new ProxyApplication_OMX;

    if (pProxyTerm[g_ComponentIndex]->GetMemPoolPtr() == NULL)
    {
        return OMX_ErrorInsufficientResources;
    }
    pProxyTerm[g_ComponentIndex]->Start();

    ErrorType = pProxyTerm[g_ComponentIndex]->ProxyGetHandle(pHandle, cComponentName, pAppData, pCallBacks);

#else

    OMX_S32 ii;

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii ++)
    {
        // go through the list of supported components and find the component based on its name (identifier)
        if (pRegTemplateList[ii] != NULL)
        {
            if (!strcmp(pRegTemplateList[ii]->ComponentName, cComponentName))
            {
                // found a matching name
                // call the factory for the component
                if ((pRegTemplateList[ii]->FunctionPtrCreateComponent)(pHandle, pAppData) == OMX_ErrorNone)
                {
                    // now that we got the component handle, store the handle in the ComponentHandle array
                    ComponentHandle[g_ComponentIndex] = *pHandle;
                    // also, record the component destructor function ptr
                    ComponentDestructor[g_ComponentIndex] = pRegTemplateList[ii]->FunctionPtrDestroyComponent;

                    NumBaseInstance++;

                    if (NumBaseInstance > MAX_INSTANTIATED_COMPONENTS)
                    {
                        return OMX_ErrorInsufficientResources;
                    }

                    ((OMX_COMPONENTTYPE*)*pHandle)->SetCallbacks(*pHandle, pCallBacks, pAppData);
                }
                else
                {
                    return OMX_ErrorInsufficientResources;
                }
            }

        }
        else
        {
            break;
        }

    }
    // can't find the component after going through all of them
    if (ComponentHandle[g_ComponentIndex] == NULL)
    {
        return OMX_ErrorComponentNotFound;
    }

#endif

    return ErrorType;

}


OMX_API OMX_ERRORTYPE OMX_APIENTRY PVOMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;


    OMX_S32 ii, ComponentNumber = 0;

    // Find the component index in the array of handles
    for (ii = 0; ii < MAX_INSTANTIATED_COMPONENTS ; ii++)
    {
        if (hComponent == ComponentHandle[ii])
        {
            ComponentNumber = ii;
            break;
        }
    }

    // cannot find the component handle
    if (ii == MAX_INSTANTIATED_COMPONENTS)
        return OMX_ErrorInvalidComponent;


#if PROXY_INTERFACE
    // call the OMX_FreeHandle through the proxy
    ErrorType = pProxyTerm[ComponentNumber]->ProxyFreeHandle(hComponent);

    // exit thread
    pProxyTerm[ComponentNumber]->Exit();
    delete pProxyTerm[ComponentNumber];
    // delete array entries associated with pProxyTerm and Component handle
    pProxyTerm[ComponentNumber] = NULL;
    ComponentHandle[ComponentNumber] = NULL;
    ComponentDestructor[ComponentNumber] = NULL;

#else


    // call the component AO destructor through the function pointer
    ErrorType = ComponentDestructor[ComponentNumber](hComponent);

    ComponentHandle[ComponentNumber] = NULL;
    ComponentDestructor[ComponentNumber] = NULL;

    NumBaseInstance--;

#endif

    return ErrorType;

}

#if PROXY_INTERFACE

// Note that g_ComponentIndex was determined outside this function (prior to using the proxy)

OMX_API OMX_ERRORTYPE OMX_APIENTRY GlobalProxyComponentGetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks)
{
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;
    OMX_S32 ii;

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii ++)
    {

        if (pRegTemplateList[ii] != NULL)
        {
            if (!oscl_strcmp(pRegTemplateList[ii]->ComponentName, cComponentName))
            {
                // found a matching name
                // call the factory for the component
                if ((pRegTemplateList[ii]->FunctionPtrCreateComponent)(pHandle, pAppData) == OMX_ErrorNone)
                {
                    NumBaseInstance++;

                    if (NumBaseInstance > MAX_INSTANTIATED_COMPONENTS)
                    {
                        return OMX_ErrorInsufficientResources;
                    }

                    //Store handle to identify the corresponding proxy later on
                    ComponentHandle[g_ComponentIndex] = *pHandle;

                    // record the component destructor function ptr;
                    ComponentDestructor[g_ComponentIndex] = pRegTemplateList[ii]->FunctionPtrDestroyComponent;

                    ((OMX_COMPONENTTYPE*)*pHandle)->SetCallbacks(*pHandle, pCallBacks, pAppData);
                }
                else
                {
                    return OMX_ErrorInsufficientResources;
                }
            }

        }
        else
        {
            break;
        }
    }

    // can't find the component after going through all of them
    if (ComponentHandle[g_ComponentIndex] == NULL)
    {
        return OMX_ErrorComponentNotFound;
    }

    return ErrorType;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY GlobalProxyComponentFreeHandle(OMX_IN OMX_HANDLETYPE hComponent)
{

    //ThreadLock.Lock();
    OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)hComponent;
    OMX_U32 ii;

    // find the component index based on handle
    for (ii = 0; ii < MAX_INSTANTIATED_COMPONENTS; ii++)
    {
        if (pHandle == ComponentHandle[ii])
            break;
    }
    // cannot find the component handle
    if (ii == MAX_INSTANTIATED_COMPONENTS)
        return OMX_ErrorInvalidComponent;


    // call the component destructor through the function pointer recorder earlier
    // using hComponent as argument
    ComponentDestructor[ii](hComponent);



    NumBaseInstance--;
    //ThreadLock.Unlock();

    return OMX_ErrorNone;
}
#endif


//This is a method to be called directly under testapp thread
OMX_API OMX_ERRORTYPE OMX_APIENTRY PVOMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    OMX_U32 Index = 0;

    while (pRegTemplateList[Index] != NULL)
    {
        if (Index == nIndex)
        {
            break;
        }
        Index++;
    }

    if (pRegTemplateList[Index] != NULL)
    {
        strncpy(cComponentName, pRegTemplateList[Index]->ComponentName, nNameLength);
    }
    else
    {
        return OMX_ErrorNoMore;
    }

    return OMX_ErrorNone;

}

OMX_API OMX_ERRORTYPE PVOMX_SetupTunnel(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput)
{
    OSCL_UNUSED_ARG(hOutput);
    OSCL_UNUSED_ARG(nPortOutput);
    OSCL_UNUSED_ARG(hInput);
    OSCL_UNUSED_ARG(nPortInput);
    return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE   PVOMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI)
{
    OSCL_UNUSED_ARG(hPipe);
    OSCL_UNUSED_ARG(szURI);
    return OMX_ErrorNotImplemented;
}

/////////////////////////////////////////////////////
/////////////// Given a compName, find the component and then return its role(s)
///////////////// It's the caller's responsibility to provide enough space for the role(s)
////////////////////////////////////////////////////////////////////////////
OMX_API OMX_ERRORTYPE PVOMX_GetRolesOfComponent(
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32* pNumRoles,
    OMX_OUT     OMX_U8** roles)
{
    OMX_STRING RoleString = NULL;
    OMX_S32 ii;

    // first check if there is a component with the correct name
    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii ++)
    {
        if (pRegTemplateList[ii])
        {
            if (!strcmp(pRegTemplateList[ii]->ComponentName, compName))
            {
                pRegTemplateList[ii]->GetRolesOfComponent(&RoleString);
                break;
            }
            //else
            //{
            //	break;
            //}
        }
    }

    if (ii == MAX_SUPPORTED_COMPONENTS)
    {
        // component not found
        *pNumRoles = 0;
        return OMX_ErrorInvalidComponent;
    }


    // for simplicity all our components have 1 role only
    // if a component has more than 1 role, still register it for each role separately, and use
    //	factory/destructor separately
    *pNumRoles = 1;
    if (roles != NULL)
    {
        strcpy((OMX_STRING) roles[0], (OMX_STRING)RoleString);
    }

    return OMX_ErrorNone;
}

/////////////////////////////////////////////////////////////////////////
////////// Given a role (say "video_decoder.avc") give the number (and a list) of
///////////components that support the role
/////////// It is the callers responsibility to provide enough space for component names,
//////////// so it may need to make the call twice. Once to find number of components, and 2nd time
//////////// to find their actual names
//////////////////////////////////////////////////////////////////////////////////
OMX_API OMX_ERRORTYPE PVOMX_GetComponentsOfRole(
    OMX_IN		OMX_STRING role,
    OMX_INOUT	OMX_U32	*pNumComps,
    OMX_INOUT	OMX_U8	**compNames)
{


    OMX_U32 ii;
    OMX_STRING RoleString;
    // initialize
    *pNumComps = 0;

    // go through all components and check if they support the given role
    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii ++)
    {
        if (pRegTemplateList[ii])
        {
            // get the component role
            pRegTemplateList[ii]->GetRolesOfComponent(&RoleString);

            // if the role matches, increment the counter and record the comp. name
            if (!strcmp(RoleString, role))
            {
                // if a placeholder for compNames is provided, copy the component name into it
                if (compNames != NULL)
                {
                    strcpy((OMX_STRING) compNames[*pNumComps], pRegTemplateList[ii]->ComponentName);
                }
                // increment the counter
                *pNumComps = (*pNumComps + 1);

            }
        }
    }

    return OMX_ErrorNone;

}

// WRAPPER CLASS METHODS VISIBLE FROM OUTSIDE THE LIBRARY

PV_OMX_Wrapper::PV_OMX_Wrapper()
{
    // initialize f. ptrs
    pOMX_Init = PVOMX_Init;
    pOMX_Deinit = PVOMX_Deinit;
    pOMX_ComponentNameEnum = PVOMX_ComponentNameEnum;
    pOMX_GetHandle = PVOMX_GetHandle;
    pOMX_FreeHandle = PVOMX_FreeHandle;
    pOMX_GetComponentsOfRole = PVOMX_GetComponentsOfRole;
    pOMX_GetRolesOfComponent = PVOMX_GetRolesOfComponent;
    pOMX_SetupTunnel = PVOMX_SetupTunnel;
    pOMX_GetContentPipe = PVOMX_GetContentPipe;
};

PV_OMX_WrapperBase *PV_OMX_Wrapper::New()
{
    void *tmp = malloc(sizeof(PV_OMX_Wrapper));
    PV_OMX_WrapperBase *x = (PV_OMX_WrapperBase *) new(tmp) PV_OMX_Wrapper();
    return x;
}

void PV_OMX_Wrapper::Delete()
{

    this->~PV_OMX_Wrapper();
    free(this);
}

tpOMX_Init PV_OMX_Wrapper::GetpOMX_Init()
{
    return pOMX_Init;
};

tpOMX_Deinit PV_OMX_Wrapper::GetpOMX_Deinit()
{
    return pOMX_Deinit;
};

tpOMX_ComponentNameEnum PV_OMX_Wrapper::GetpOMX_ComponentNameEnum()
{
    return pOMX_ComponentNameEnum;
};

tpOMX_GetHandle PV_OMX_Wrapper::GetpOMX_GetHandle()
{
    return pOMX_GetHandle;
};

tpOMX_FreeHandle PV_OMX_Wrapper::GetpOMX_FreeHandle()
{
    return pOMX_FreeHandle;
};

tpOMX_GetComponentsOfRole PV_OMX_Wrapper::GetpOMX_GetComponentsOfRole()
{
    return pOMX_GetComponentsOfRole;
};

tpOMX_GetRolesOfComponent PV_OMX_Wrapper::GetpOMX_GetRolesOfComponent()
{
    return pOMX_GetRolesOfComponent;
};

tpOMX_SetupTunnel PV_OMX_Wrapper::GetpOMX_SetupTunnel()
{
    return pOMX_SetupTunnel;
};

tpOMX_GetContentPipe PV_OMX_Wrapper::GetpOMX_GetContentPipe()
{
    return pOMX_GetContentPipe;
};
