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
#ifndef PV_OMXCORE_H_INCLUDED
#define PV_OMXCORE_H_INCLUDED

#ifndef PV_OMXDEFS_H_INCLUDED
#include "pv_omxdefs.h"
#endif

#ifndef PV_OMX_QUEUE_H_INCLUDED
#include "pv_omx_queue.h"
#endif

#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#ifndef PV_OMXWRAPPERBASE_H_INCLUDED
#include "pv_omxwrapperbase.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    OMX_API OMX_ERRORTYPE PVOMX_GetComponentsOfRole(
        OMX_IN		OMX_STRING role,
        OMX_INOUT	OMX_U32	*pNumComps,
        OMX_INOUT	OMX_U8	**compNames);

    OMX_API OMX_ERRORTYPE OMX_APIENTRY PVOMX_ComponentNameEnum(
        OMX_OUT OMX_STRING cComponentName,
        OMX_IN  OMX_U32 nNameLength,
        OMX_IN  OMX_U32 nIndex);

    OMX_API OMX_ERRORTYPE OMX_APIENTRY PVOMX_FreeHandle(OMX_IN OMX_HANDLETYPE hComponent);

    OMX_API OMX_ERRORTYPE OMX_APIENTRY PVOMX_GetHandle(OMX_OUT OMX_HANDLETYPE* pHandle,
            OMX_IN  OMX_STRING cComponentName,
            OMX_IN  OMX_PTR pAppData,
            OMX_IN  OMX_CALLBACKTYPE* pCallBacks);

    OMX_API OMX_ERRORTYPE PVOMX_GetRolesOfComponent(
        OMX_IN      OMX_STRING compName,
        OMX_INOUT   OMX_U32* pNumRoles,
        OMX_OUT     OMX_U8** roles);

#ifdef __cplusplus
}
#endif

class ComponentRegistrationType
{
    public:
        // name of the component used as identifier
        OMX_STRING		ComponentName;
        OMX_STRING		RoleString;
        // pointer to factory function to be called when component needs to be instantiated
        OMX_ERRORTYPE(*FunctionPtrCreateComponent)(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
        // pointer to function that destroys the component and its AO
        OMX_ERRORTYPE(*FunctionPtrDestroyComponent)(OMX_IN OMX_HANDLETYPE pHandle);
        //This function will return the role string
        void GetRolesOfComponent(OMX_STRING* aRole_string)
        {
            *aRole_string = RoleString;
        }

};


OMX_API OMX_ERRORTYPE OMX_APIENTRY GlobalProxyComponentGetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName, OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);

OMX_API OMX_ERRORTYPE OMX_APIENTRY GlobalProxyComponentFreeHandle(
    OMX_IN OMX_HANDLETYPE hComponent);



typedef struct CoreDescriptorType
{
    QueueType* pMessageQueue; // The output queue for the messages to be send to the components
} CoreDescriptorType;


/** This structure contains all the fields of a message handled by the core */
struct CoreMessage
{
    OMX_COMPONENTTYPE* pComponent; /// A reference to the main structure that defines a component. It represents the target of the message
    OMX_S32 MessageType; /// the flag that specifies if the message is a command, a warning or an error
    OMX_S32 MessageParam1; /// the first field of the message. Its use is the same as specified for the command in OpenMAX spec
    OMX_S32 MessageParam2; /// the second field of the message. Its use is the same as specified for the command in OpenMAX spec
    OMX_PTR pCmdData; /// This pointer could contain some proprietary data not covered by the standard
};

typedef struct PV_OMXComponentCapabilityFlagsType
{
    ////////////////// OMX COMPONENT CAPABILITY RELATED MEMBERS
    OMX_BOOL iIsOMXComponentMultiThreaded;
    OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
    OMX_BOOL iOMXComponentSupportsMovableInputBuffers;

} PV_OMXComponentCapabilityFlagsType;


typedef class PV_OMX_Wrapper
            : public PV_OMX_WrapperBase
{
    public:
        PV_OMX_Wrapper();
        static PV_OMX_WrapperBase *New();
        void Delete();
        ~PV_OMX_Wrapper() {};

        tpOMX_Init GetpOMX_Init();

        tpOMX_Deinit GetpOMX_Deinit();

        tpOMX_ComponentNameEnum GetpOMX_ComponentNameEnum();

        tpOMX_GetHandle GetpOMX_GetHandle();

        tpOMX_FreeHandle GetpOMX_FreeHandle();

        tpOMX_GetComponentsOfRole GetpOMX_GetComponentsOfRole();

        tpOMX_GetRolesOfComponent GetpOMX_GetRolesOfComponent();

        tpOMX_SetupTunnel GetpOMX_SetupTunnel();

        tpOMX_GetContentPipe GetpOMX_GetContentPipe();

} PV_OMX_Wrapper;


#endif
