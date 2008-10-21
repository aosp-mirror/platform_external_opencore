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
#ifndef PV_OMXMASTERCORE_H_INCLUDED
#define PV_OMXMASTERCORE_H_INCLUDED

// get definitions of omx core methods
#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#ifndef PV_OMXWRAPPERBASE_H_INCLUDED
#include "pv_omxwrapperbase.h"
#endif

OMX_ERRORTYPE OMX_APIENTRY PV_MasterOMX_Init();

OMX_ERRORTYPE OMX_APIENTRY PV_MasterOMX_Deinit();

OMX_API OMX_ERRORTYPE PV_MasterOMX_GetComponentsOfRole(
    OMX_IN	OMX_STRING role,
    OMX_INOUT	OMX_U32	*pNumComps,
    OMX_INOUT	OMX_U8	**compNames);

OMX_API OMX_ERRORTYPE OMX_APIENTRY 	PV_MasterOMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);

OMX_API OMX_ERRORTYPE OMX_APIENTRY PV_MasterOMX_FreeHandle(
    OMX_IN OMX_HANDLETYPE hComponent);


#endif









