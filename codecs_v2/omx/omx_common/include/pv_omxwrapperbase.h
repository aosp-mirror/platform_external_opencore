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
#ifndef PV_OMXWRAPPERBASE_H_INCLUDED
#define PV_OMXWRAPPERBASE_H_INCLUDED



// get definitions of omx core methods
#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#include <stdio.h>
#include <stdlib.h>

// ptrs to all omx core methods
// will be set in the derived classes
typedef OMX_ERRORTYPE OMX_APIENTRY(*tpOMX_Init)(void);

typedef OMX_ERRORTYPE OMX_APIENTRY(*tpOMX_Deinit)(void);

typedef OMX_ERRORTYPE OMX_APIENTRY(*tpOMX_ComponentNameEnum)(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex);


typedef OMX_ERRORTYPE OMX_APIENTRY(*tpOMX_GetHandle)(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);


typedef OMX_ERRORTYPE OMX_APIENTRY(*tpOMX_FreeHandle)(
    OMX_IN  OMX_HANDLETYPE hComponent);

typedef OMX_ERRORTYPE(*tpOMX_GetComponentsOfRole)(
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames);


typedef OMX_ERRORTYPE(*tpOMX_GetRolesOfComponent)(
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles);


typedef OMX_ERRORTYPE OMX_APIENTRY(*tpOMX_SetupTunnel)(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput);


typedef OMX_ERRORTYPE(*tpOMX_GetContentPipe)(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI);


typedef class PV_OMX_WrapperBase
{
    public:
        PV_OMX_WrapperBase()
        {
            pOMX_Init = 0;
            pOMX_Deinit = 0;
            pOMX_ComponentNameEnum = 0;
            pOMX_GetHandle = 0;
            pOMX_FreeHandle = 0;
            pOMX_GetComponentsOfRole = 0;
            pOMX_GetRolesOfComponent = 0;
            pOMX_SetupTunnel = 0;
            pOMX_GetContentPipe = 0;
        };

        static PV_OMX_WrapperBase *New();

        void Delete()
        {
            this->~PV_OMX_WrapperBase();
            free(this);
        };

        ~PV_OMX_WrapperBase() {};

        tpOMX_Init GetpOMX_Init()
        {
            return pOMX_Init;
        };

        tpOMX_Deinit GetpOMX_Deinit()
        {
            return  pOMX_Deinit;
        };

        tpOMX_ComponentNameEnum GetpOMX_ComponentNameEnum()
        {
            return pOMX_ComponentNameEnum;
        };

        tpOMX_GetHandle GetpOMX_GetHandle()
        {
            return pOMX_GetHandle;
        };

        tpOMX_FreeHandle GetpOMX_FreeHandle()
        {
            return pOMX_FreeHandle;
        };

        tpOMX_GetComponentsOfRole GetpOMX_GetComponentsOfRole()
        {
            return pOMX_GetComponentsOfRole;
        };

        tpOMX_GetRolesOfComponent GetpOMX_GetRolesOfComponent()
        {
            return pOMX_GetRolesOfComponent;
        };

        tpOMX_SetupTunnel GetpOMX_SetupTunnel()
        {
            return pOMX_SetupTunnel;
        };

        tpOMX_GetContentPipe GetpOMX_GetContentPipe()
        {
            return pOMX_GetContentPipe;
        };


        // ptrs to all omx core methods
        // will be set in the derived classes
        OMX_ERRORTYPE OMX_APIENTRY(*pOMX_Init)(void);

        OMX_ERRORTYPE OMX_APIENTRY(*pOMX_Deinit)(void);

        OMX_ERRORTYPE OMX_APIENTRY(*pOMX_ComponentNameEnum)(
            OMX_OUT OMX_STRING cComponentName,
            OMX_IN  OMX_U32 nNameLength,
            OMX_IN  OMX_U32 nIndex);


        OMX_ERRORTYPE OMX_APIENTRY(*pOMX_GetHandle)(
            OMX_OUT OMX_HANDLETYPE* pHandle,
            OMX_IN  OMX_STRING cComponentName,
            OMX_IN  OMX_PTR pAppData,
            OMX_IN  OMX_CALLBACKTYPE* pCallBacks);


        OMX_ERRORTYPE OMX_APIENTRY(*pOMX_FreeHandle)(
            OMX_IN  OMX_HANDLETYPE hComponent);

        OMX_ERRORTYPE(*pOMX_GetComponentsOfRole)(
            OMX_IN      OMX_STRING role,
            OMX_INOUT   OMX_U32 *pNumComps,
            OMX_INOUT   OMX_U8  **compNames);


        OMX_ERRORTYPE(*pOMX_GetRolesOfComponent)(
            OMX_IN      OMX_STRING compName,
            OMX_INOUT   OMX_U32 *pNumRoles,
            OMX_OUT     OMX_U8 **roles);


        OMX_ERRORTYPE OMX_APIENTRY(*pOMX_SetupTunnel)(
            OMX_IN  OMX_HANDLETYPE hOutput,
            OMX_IN  OMX_U32 nPortOutput,
            OMX_IN  OMX_HANDLETYPE hInput,
            OMX_IN  OMX_U32 nPortInput);


        OMX_ERRORTYPE(*pOMX_GetContentPipe)(
            OMX_OUT OMX_HANDLETYPE *hPipe,
            OMX_IN OMX_STRING szURI);



} PV_OMX_WrapperBase;
#endif

