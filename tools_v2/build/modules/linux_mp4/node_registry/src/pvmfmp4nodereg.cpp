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
/* This file defines a PV NodeSharedLibrary interface that populates the
   registry with the nodes required for mp4 playback.
*/
#include "pvmf_node_shared_lib_interface.h"

#include "pvmf_mp4ffparser_factory.h"

#include "pvmp4ffrec_factory.h"

#include "pvmf_recognizer_registry.h"

class Mp4NodesRegistryInterface: public OsclSharedLibraryInterface,
            public NodeRegistrySharedLibraryInterface,
            public RecognizerSharedLibraryInterface
{
    public:

        // From NodeRegistrySharedLibraryInterface
        void RegisterAllNodes(PVPlayerNodeRegistryInterface* aRegistry, OsclAny*& aContext)
        {
            PVPlayerNodeInfo nodeinfo;

#define MP4_LIB_PATH "libopencoremp4.so"
            OSCL_HeapString<OsclMemAllocator> libname = MP4_LIB_PATH;

            Oscl_Vector<PVPlayerNodeInfo, OsclMemAllocator>* nodeList = new Oscl_Vector<PVPlayerNodeInfo, OsclMemAllocator>;

            //For PVMFMP4FFParserNode
            nodeinfo.iInputTypes.clear();
            nodeinfo.iInputTypes.push_back(PVMF_MPEG4FF);
            nodeinfo.iNodeUUID = KPVMFMP4FFParserNodeUuid;
            nodeinfo.iOutputType.clear();
            nodeinfo.iOutputType.push_back(PVMF_FORMAT_UNKNOWN);
            nodeinfo.iSharedLibrary = OSCL_NEW(OsclSharedLibrary, (libname));
            aRegistry->RegisterNode(nodeinfo);

            nodeList->push_back(nodeinfo);

            aContext = (OsclAny *)nodeList;

        };

        // From NodeRegistrySharedLibraryInterface
        void UnregisterAllNodes(PVPlayerNodeRegistryInterface* aRegistry, OsclAny* aContext)
        {
            if (NULL != aContext)
            {
                Oscl_Vector<PVPlayerNodeInfo, OsclMemAllocator>* nodeList = (Oscl_Vector<PVPlayerNodeInfo, OsclMemAllocator> *)aContext;

                while (!nodeList->empty())
                {
                    PVPlayerNodeInfo tmpnode = nodeList->front();
                    OSCL_DELETE(tmpnode.iSharedLibrary);
                    aRegistry->UnregisterNode(tmpnode);
                    nodeList->erase(nodeList->begin());
                }

                delete nodeList;
            }
        };


        // From RecognizerSharedLibraryInterface
        void RegisterAllRecognizers(PVPlayerRecognizerRegistryInterface* aRegistry, OsclAny*& aContext)
        {
            PVMFRecognizerPluginFactory* tmpfac = NULL;

            Oscl_Vector<PVMFRecognizerPluginFactory*, OsclMemAllocator>* pluginList =
                new Oscl_Vector<PVMFRecognizerPluginFactory*, OsclMemAllocator>;

            tmpfac =
                OSCL_STATIC_CAST(PVMFRecognizerPluginFactory*, OSCL_NEW(PVMP4FFRecognizerFactory, ()));
            aRegistry->RegisterRecognizer(tmpfac);

            pluginList->push_back(tmpfac);

            aContext = (OsclAny *)pluginList;
        };


        void UnregisterAllRecognizers(PVPlayerRecognizerRegistryInterface* aRegistry, OsclAny* aContext)
        {
            if (NULL != aContext)
            {
                Oscl_Vector<PVMFRecognizerPluginFactory*, OsclMemAllocator>* pluginList = (Oscl_Vector<PVMFRecognizerPluginFactory*, OsclMemAllocator>*)aContext;

                while (!pluginList->empty())
                {
                    PVMFRecognizerPluginFactory* tmpfac = pluginList->front();

                    aRegistry->UnregisterRecognizer(tmpfac);

                    pluginList->erase(pluginList->begin());

                    OSCL_DELETE(tmpfac);
                }

                delete pluginList;
            }
        };

        // From OsclSharedLibraryInterface
        OsclAny* SharedLibraryLookup(const OsclUuid& aInterfaceId)
        {
            if (aInterfaceId == PV_NODE_REGISTRY_INTERFACE)
            {
                return OSCL_STATIC_CAST(NodeRegistrySharedLibraryInterface*, this);
            }
            else if (aInterfaceId == PV_RECOGNIZER_INTERFACE)
            {
                return OSCL_STATIC_CAST(RecognizerSharedLibraryInterface*, this);
            }
            return NULL;
        };

        static Mp4NodesRegistryInterface* Instance()
        {
            static Mp4NodesRegistryInterface nodeInterface;
            return &nodeInterface;
        };

    private:

        Mp4NodesRegistryInterface() {};

};


extern "C"
{
    OsclAny *GetInterface(void)
    {
        return Mp4NodesRegistryInterface::Instance();
    }
}

