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
#ifndef QC_OMXCORE_H_INCLUDED
#define QC_OMXCORE_H_INCLUDED

#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#ifndef PV_OMXWRAPPERBASE_H_INCLUDED
#include "pv_omxwrapperbase.h"
#endif

#ifndef PV_OMXDEFS_H_INCLUDED
#include "pv_omxdefs.h"
#endif

#include <new>

#if HARDWARE_OMX

typedef class QC_OMX_Wrapper
            : public PV_OMX_WrapperBase
{
    public:
        QC_OMX_Wrapper();
        static PV_OMX_WrapperBase *New();
        void Delete();
        ~QC_OMX_Wrapper() {};

        tpOMX_Init GetpOMX_Init();

        tpOMX_Deinit GetpOMX_Deinit();

        tpOMX_ComponentNameEnum GetpOMX_ComponentNameEnum();

        tpOMX_GetHandle GetpOMX_GetHandle();

        tpOMX_FreeHandle GetpOMX_FreeHandle();

        tpOMX_GetComponentsOfRole GetpOMX_GetComponentsOfRole();

        tpOMX_GetRolesOfComponent GetpOMX_GetRolesOfComponent();

        tpOMX_SetupTunnel GetpOMX_SetupTunnel();

        tpOMX_GetContentPipe GetpOMX_GetContentPipe();

} QC_OMX_Wrapper;
#endif // HARDWARE_OMX

#endif
