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
#ifndef OMX_M4V_COMPONENT_INTERFACE_H_INCLUDED
#define OMX_M4V_COMPONENT_INTERFACE_H_INCLUDED

#include "omx_core.h"

#define PV_OMX_M4V_TYPE OsclUuid(0x1d4769f0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x6b)
#define PV_OMX_H263_TYPE OsclUuid(0x1d4769f0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x6c)

class OsclSharedLibrary;

class OmxM4vComponentFactory
{
    public:
        static OMX_ERRORTYPE M4vCreate(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
        static OMX_ERRORTYPE M4vDestructor(OMX_IN OMX_HANDLETYPE pHandle);
        static OMX_ERRORTYPE H263Create(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
        static OMX_ERRORTYPE H263Destructor(OMX_IN OMX_HANDLETYPE pHandle);

    private:
        static OsclSharedLibrary* iOmxLib;
        static int iRefCount;
};

#endif // OMX_M4V_COMPONENT_INTERFACE_H_INCLUDED

