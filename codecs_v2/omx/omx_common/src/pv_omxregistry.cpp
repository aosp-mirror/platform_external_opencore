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
#include "oscl_types.h"
#include "pv_omxregistry.h"

extern ComponentRegistrationType *pRegTemplateList[];
extern OMX_HANDLETYPE ComponentHandle[];
extern OMX_ERRORTYPE(*ComponentDestructor[])(OMX_IN OMX_HANDLETYPE pHandle);
extern OMX_U32 NumOMXInitInstances;


#if PROXY_INTERFACE
#include "omx_proxy_interface.h"
extern ProxyApplication_OMX* pProxyTerm[];
#endif

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
OMX_ERRORTYPE OMX_Init()
{
    OMX_ERRORTYPE Status = OMX_ErrorNone;
    OMX_U32 ii;

    NumOMXInitInstances++;

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


#if REGISTER_OMX_M4V_COMPONENT
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE Mpeg4Register(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));

    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.mpeg4dec";
        pCRT->RoleString = "video_decoder.mpeg4";
        pCRT->FunctionPtrCreateComponent = OmxM4vComponentFactory::M4vCreate;
        pCRT->FunctionPtrDestroyComponent = OmxM4vComponentFactory::M4vDestructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif
//////////////////////////////////////////////////////////////////////////////

#if REGISTER_OMX_H263_COMPONENT

/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE H263Register(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));


    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.h263dec";
        pCRT->RoleString = "video_decoder.h263";
        pCRT->FunctionPtrCreateComponent = OmxM4vComponentFactory::H263Create;
        pCRT->FunctionPtrDestroyComponent = OmxM4vComponentFactory::H263Destructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif
////////////////////////////////////////////////////////////////////////////////////

#if REGISTER_OMX_AVC_COMPONENT
extern OMX_ERRORTYPE AvcOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE AvcOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

/////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE AvcRegister(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));

    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.avcdec";
        pCRT->RoleString = "video_decoder.avc";
        pCRT->FunctionPtrCreateComponent = &AvcOmxComponentFactory;
        pCRT->FunctionPtrDestroyComponent = &AvcOmxComponentDestructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif
////////////////////////////////////////////////////////////////////////////////////

#if REGISTER_OMX_WMV_COMPONENT
/////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE WmvRegister(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));

    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.wmvdec";
        pCRT->RoleString = "video_decoder.wmv";
        pCRT->FunctionPtrCreateComponent = OmxWmvComponentFactory::Create;
        pCRT->FunctionPtrDestroyComponent = OmxWmvComponentFactory::Destructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////
#if REGISTER_OMX_AAC_COMPONENT
// external factory functions needed for creation of each component (or stubs for testing)
extern OMX_ERRORTYPE AacOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE AacOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE AacRegister(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));

    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.aacdec";
        pCRT->RoleString = "audio_decoder.aac";
        pCRT->FunctionPtrCreateComponent = &AacOmxComponentFactory;
        pCRT->FunctionPtrDestroyComponent = &AacOmxComponentDestructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
#if REGISTER_OMX_AMR_COMPONENT
// external factory functions needed for creation of each component (or stubs for testing)
extern OMX_ERRORTYPE AmrOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE AmrOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE AmrRegister(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));

    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.amrdec";
        pCRT->RoleString = "audio_decoder.amr";
        pCRT->FunctionPtrCreateComponent = &AmrOmxComponentFactory;
        pCRT->FunctionPtrDestroyComponent = &AmrOmxComponentDestructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
#if REGISTER_OMX_MP3_COMPONENT
// external factory functions needed for creation of each component (or stubs for testing)
extern OMX_ERRORTYPE Mp3OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE Mp3OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE Mp3Register(ComponentRegistrationType **aTemplateList)
{

    OMX_S32 ii;
    ComponentRegistrationType *pCRT = (ComponentRegistrationType *) oscl_malloc(sizeof(ComponentRegistrationType));

    if (pCRT)
    {
        pCRT->ComponentName = "OMX.PV.mp3dec";
        pCRT->RoleString = "audio_decoder.mp3";
        pCRT->FunctionPtrCreateComponent = &Mp3OmxComponentFactory;
        pCRT->FunctionPtrDestroyComponent = &Mp3OmxComponentDestructor;

    }
    else
    {
        return OMX_ErrorInsufficientResources;
    }

    for (ii = 0; ii < MAX_SUPPORTED_COMPONENTS; ii++)
    {
        if (NULL == aTemplateList[ii])
        {
            aTemplateList[ii] = pCRT;
            break;
        }
    }

    if (MAX_SUPPORTED_COMPONENTS == ii)
    {
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}
#endif

