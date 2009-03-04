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
/**
 * @file pvaenodefactoryutility.h
 * PVAuthorEngine Node Factory Utility for single core builds
 */

#ifndef PVAE_NODE_FACTORY_UTILITY_H_INCLUDED
#define PVAE_NODE_FACTORY_UTILITY_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif
#ifndef PVAUTHORENGINE_H_INCLUDED
#include "pvauthorengine.h"
#endif
#ifndef PVMF_OMX_VIDEOENC_NODE_FACTORY_H_INCLUDED
#include "pvmf_omx_videoenc_node_factory.h"
#endif
#ifndef PVMP4FFCN_FACTORY_H_INCLUDED
#include "pvmp4ffcn_factory.h"
#endif
#ifndef PVMF_FILEOUTPUT_FACTORY_H_INCLUDED
#include "pvmf_fileoutput_factory.h"
#endif
#ifndef PVMF_AMRENC_NODE_FACTORY_H_INCLUDED
#include "pvmf_amrenc_node_factory.h"
#endif
#ifndef PVMP4FFCN_CLIPCONFIG_H_INCLUDED
#include "pvmp4ffcn_clipconfig.h"
#endif
#ifndef PVMP4FFCN_TRACKCONFIG_H_INCLUDED
#include "pvmp4ffcn_trackconfig.h"
#endif
#ifndef PVMF_FILEOUTPUT_CONFIG_H_INCLUDED
#include "pvmf_fileoutput_config.h"
#endif
#ifndef PV_MP4_H263_ENC_EXTENSION_H_INCLUDED
#include "pvmp4h263encextension.h"
#endif
#ifndef PVMFAMRENCNODE_EXTENSION_H_INCLUDED
#include "pvmfamrencnode_extension.h"
#endif

class PVAuthorEngineNodeFactoryUtility
{
    public:
        static PVMFNodeInterface* CreateEncoder(const PVUuid& aUuid)
        {
            PVMFNodeInterface* node = NULL;
            if (aUuid == PVMFOMXVideoEncNodeUuid)
            {
                node = PVMFOMXVideoEncNodeFactory::CreateVideoEncNode();
            }
            else if (aUuid == PvmfAmrEncNodeUuid)
            {
                node = PvmfAmrEncNodeFactory::Create();
            }
            return node;
        }

        static PVMFNodeInterface* CreateComposer(const PVUuid& aUuid)
        {
            PVMFNodeInterface* node = NULL;
            if (aUuid == KPVMp4FFComposerNodeUuid)
            {
                node = PVMp4FFComposerNodeFactory::CreateMp4FFComposer();
            }
            else if (aUuid == KPVFileOutputNodeUuid)
            {
                node = PVFileOutputNodeFactory::CreateFileOutput();
            }

            return node;
        }

        static bool Delete(const PVUuid& aUuid, PVMFNodeInterface* aNode)
        {
            if(!aNode)
            {
                return false;
            }

            if (aUuid == PVMFOMXVideoEncNodeUuid)
            {
                return PVMFOMXVideoEncNodeFactory::DeleteVideoEncNode(aNode);
            }
            else if (aUuid == KPVMp4FFComposerNodeUuid)
            {
                return PVMp4FFComposerNodeFactory::DeleteMp4FFComposer(aNode);
            }
            else if (aUuid == KPVFileOutputNodeUuid)
            {
                return PVFileOutputNodeFactory::DeleteFileOutput(aNode);
            }
            else if (aUuid == PvmfAmrEncNodeUuid)
            {
                return PvmfAmrEncNodeFactory::Delete(aNode);
            }

            return false;
        }

        static bool QueryRegistry(const PvmfMimeString& aMimeType, PVUuid& aUuid)
        {
            if (CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KMp4ComposerMimeType)) ||
                CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(K3gpComposerMimeType)))
            {
                aUuid = KPVMp4FFComposerNodeUuid;
            }
            else if (CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KMp4EncMimeType)) ||
                     CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KH263EncMimeType)))
            {
                aUuid = PVMFOMXVideoEncNodeUuid;
            }
            else if (CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KAmrNbEncMimeType)))
            {
                aUuid = PvmfAmrEncNodeUuid;
            }
            else if (CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KAMRNbComposerMimeType)) ||
                     CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KAACADIFComposerMimeType)) ||
                     CompareMimeTypes(aMimeType, OSCL_HeapString<OsclMemAllocator>(KAACADTSComposerMimeType)))
            {
                aUuid = KPVFileOutputNodeUuid;
            }
            else
            {
                return false;
            }

            return true;
        }

        static bool QueryNodeConfigUuid(const PVUuid& aNodeUuid, PVUuid& aConfigUuid)
        {
            bool status = false;

            if (aNodeUuid == KPVMp4FFComposerNodeUuid)
            {
                aConfigUuid = KPVMp4FFCNClipConfigUuid;
                status = true;
            }
            else if (aNodeUuid == KPVFileOutputNodeUuid)
            {
                aConfigUuid = PvmfFileOutputNodeConfigUuid;
                status = true;
            }
            else if (aNodeUuid == PVMFOMXVideoEncNodeUuid)
            {
                aConfigUuid = PVMp4H263EncExtensionUUID;
                status = true;
            }
            else if (aNodeUuid == PvmfAmrEncNodeUuid)
            {
                aConfigUuid = PVAMREncExtensionUUID;
                status = true;
            }
            ////////////////////////////////////////////////////////////////////////////
            // When implementing support for a new file format composer or encoder node,
            // add code to return config uuid of the new node here if necessary
            ////////////////////////////////////////////////////////////////////////////

            return status;
        }

        static bool CompareMimeTypes(const PvmfMimeString& a, const PvmfMimeString& b)
        {
            return (oscl_strncmp(a.get_cstr(), b.get_cstr(), oscl_strlen(a.get_cstr())) == 0);
        }

};

#endif // PVAE_NODE_FACTORY_UTILITY_H_INCLUDED
