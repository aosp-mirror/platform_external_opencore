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
#include "pvmf_node_shared_lib_interface.h"

#include "pv_omx_shared_lib_interface.h"

#include "pvmf_mp4ffparser_factory.h"

#include "pv_omxdefs.h"
#include "omx_mpeg4_component.h"
#include "omx_m4v_component_interface.h"

// external factory functions needed for creation of each component (or stubs for testing)
extern OMX_ERRORTYPE Mpeg4OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE Mpeg4OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

extern OMX_ERRORTYPE H263OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData);
extern OMX_ERRORTYPE H263OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle);

class Mp4NodesInterface: public OsclSharedLibraryInterface,
            public NodeSharedLibraryInterface,
            public OmxSharedLibraryInterface
{
    public:
        // From NodeSharedLibraryInterface
        OsclAny* QueryNodeInterface(const PVUuid& aNodeUuid, const OsclUuid& aInterfaceId)
        {
            if (KPVMFMP4FFParserNodeUuid == aNodeUuid)
            {
                if (PV_CREATE_NODE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(PVMFMP4FFParserNodeFactory::CreatePVMFMP4FFParserNode));
                }
                else if (PV_RELEASE_NODE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(PVMFMP4FFParserNodeFactory::DeletePVMFMP4FFParserNode));
                }
            }

            return NULL;
        };

        // From OmxSharedLibraryInterface
        OsclAny* QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId)
        {
            if (PV_OMX_M4V_TYPE == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&Mpeg4OmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&Mpeg4OmxComponentDestructor));
                }
            }
            else if (PV_OMX_H263_TYPE == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&H263OmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&H263OmxComponentDestructor));
                }
            }

            return NULL;
        };

        // From OsclSharedLibraryInterface
        OsclAny* SharedLibraryLookup(const OsclUuid& aInterfaceId)
        {
            if (aInterfaceId == PV_NODE_INTERFACE)
            {
                return OSCL_STATIC_CAST(NodeSharedLibraryInterface*, this);
            }
            else if (aInterfaceId == PV_OMX_SHARED_INTERFACE)
            {
                return OSCL_STATIC_CAST(OmxSharedLibraryInterface*, this);
            }

            return NULL;
        };

        static Mp4NodesInterface* Instance()
        {
            static Mp4NodesInterface nodeInterface;
            return &nodeInterface;
        };

    private:

        Mp4NodesInterface() {};

};


extern "C"
{
    OsclAny *GetInterface(void)
    {
        return Mp4NodesInterface::Instance();
    }
}

