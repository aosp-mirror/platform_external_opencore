/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
 * Copyright (C) 2008 HTC Inc.
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

// WRAPPER CLASS METHODS VISIBLE FROM OUTSIDE THE LIBRARY
#include "qc_omxcore.h"

#if HARDWARE_OMX

QC_OMX_Wrapper::QC_OMX_Wrapper()
{
	// initialize f. ptrs
	pOMX_Init = OMX_Init;
	pOMX_Deinit = OMX_Deinit;
	pOMX_ComponentNameEnum= OMX_ComponentNameEnum;
	pOMX_GetHandle = OMX_GetHandle;
	pOMX_FreeHandle = OMX_FreeHandle;
	pOMX_GetComponentsOfRole= OMX_GetComponentsOfRole;
	pOMX_GetRolesOfComponent= OMX_GetRolesOfComponent;
	pOMX_SetupTunnel = OMX_SetupTunnel;
	pOMX_GetContentPipe= OMX_GetContentPipe;
};

PV_OMX_WrapperBase *QC_OMX_Wrapper::New()
{
   void *tmp = malloc(sizeof(QC_OMX_Wrapper));
   PV_OMX_WrapperBase *x = (PV_OMX_WrapperBase *) new (tmp) QC_OMX_Wrapper();
   return x;	
}

void QC_OMX_Wrapper::Delete()
{
	
	this->~QC_OMX_Wrapper();
	free(this);
}

tpOMX_Init QC_OMX_Wrapper::GetpOMX_Init()
{
	return pOMX_Init;
};
  
tpOMX_Deinit QC_OMX_Wrapper::GetpOMX_Deinit()
{
	return pOMX_Deinit;
};
	
tpOMX_ComponentNameEnum QC_OMX_Wrapper::GetpOMX_ComponentNameEnum()
{
	return pOMX_ComponentNameEnum;
};

tpOMX_GetHandle QC_OMX_Wrapper::GetpOMX_GetHandle()
{
	return pOMX_GetHandle;
};

tpOMX_FreeHandle QC_OMX_Wrapper::GetpOMX_FreeHandle()
{
	return pOMX_FreeHandle;
};

tpOMX_GetComponentsOfRole QC_OMX_Wrapper::GetpOMX_GetComponentsOfRole()
{
	return pOMX_GetComponentsOfRole;
};

tpOMX_GetRolesOfComponent QC_OMX_Wrapper::GetpOMX_GetRolesOfComponent()
{
	return pOMX_GetRolesOfComponent;
};

tpOMX_SetupTunnel QC_OMX_Wrapper::GetpOMX_SetupTunnel()
{
	return pOMX_SetupTunnel;
};

tpOMX_GetContentPipe QC_OMX_Wrapper::GetpOMX_GetContentPipe()
{
	return pOMX_GetContentPipe;
};
#endif // HARDWARE_OMX
