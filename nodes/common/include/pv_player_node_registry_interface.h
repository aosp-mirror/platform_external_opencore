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
 *  @file pv_player_node_registry.h
 *  @brief PVPlayerNodeRegistry maintains a list of nodes available which is queryable. The utility
 *   also allows the node specified by PVUuid to be created and returned
 *
 */


#ifndef PV_PLAYER_NODE_REGISTRY_INTERFACE_H_INCLUDED
#define PV_PLAYER_NODE_REGISTRY_INTERFACE_H_INCLUDED

#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#include "pvmf_format_type.h"
#endif

#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif

#ifndef PV_UUID_H_INCLUDED
#include "pv_uuid.h"
#endif

#ifdef HAS_OSCL_LIB_SUPPORT
#ifndef PVMF_RECOGNIZER_PLUGIN_H_INCLUDED
#include "pvmf_recognizer_plugin.h"
#endif
#endif

// FORWARD DECLARATION
class OsclSharedLibrary;


// CLASS DECLARATION
/**
 * PVPlayerNodeInfo is a class which will maintain node info
 **/
class PVPlayerNodeInfo
{
    public:
        /**
         * Object Constructor function
         **/
        PVPlayerNodeInfo()
        {
#ifdef HAS_OSCL_LIB_SUPPORT
            iSharedLibrary = NULL;
            iNodeCreateFunc = NULL;
            iNodeReleaseFunc = NULL;
#endif
        }

        /**
         * Copy Constructor function
         **/
        PVPlayerNodeInfo(const PVPlayerNodeInfo& aInfo)
        {
            iNodeUUID = aInfo.iNodeUUID;
            iNodeCreateFunc = aInfo.iNodeCreateFunc;
            iNodeReleaseFunc = aInfo.iNodeReleaseFunc;
            iInputTypes = aInfo.iInputTypes;
            iOutputType = aInfo.iOutputType;
            iSharedLibrary = aInfo.iSharedLibrary;
        }

        /**
         * Object destructor function
         **/
        ~PVPlayerNodeInfo()
        {
        }

        PVUuid iNodeUUID;
        PVMFNodeInterface*(*iNodeCreateFunc)(int32);
        bool (*iNodeReleaseFunc)(PVMFNodeInterface *);
        Oscl_Vector<PVMFFormatType, OsclMemAllocator> iInputTypes;
        Oscl_Vector<PVMFFormatType, OsclMemAllocator> iOutputType;
        OsclSharedLibrary* iSharedLibrary;
};


class PVPlayerNodeRegistryInterface
{
    public:
        /**
         * The QueryRegistry for PVPlayerNodeRegistry. Used mainly for Seaching of the UUID
         * whether it is available or not & returns Success if it is found else failure.
         *
         * @param aInputType Input Format Type
         *
         * @param aOutputType Output Format Type
         *
         * @param aUuids Reference to the UUID registered
         *
         * @returns Success or Failure
         **/
        virtual PVMFStatus QueryRegistry(PVMFFormatType& aInputType, PVMFFormatType& aOutputType, Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids) = 0;

        /**
         * The CreateNode for PVPlayerNodeRegistry. Used mainly for creating a node.
         *
         * @param aUuid UUID returned by the QueryRegistry
         *
         * @returns a pointer to node
         **/
        virtual PVMFNodeInterface* CreateNode(PVUuid& aUuid) = 0;

        /**
         * The ReleaseNode for PVPlayerNodeRegistry. Used for releasing a node.
         *
         * @param aUuid UUID recorded at the time of creation of the node.
         *
         * @param Pointer to the node to be released
         *
         * @returns True or False
         **/
        virtual bool ReleaseNode(PVUuid& aUuid, PVMFNodeInterface *aNode) = 0;

        /**
         * The RegisterNode for PVPlayerNodeRegistry. Used for registering nodes through the NodeInfo object.
         *
         * @param aNodeInfo NodeInfo object passed to the regisry class. This contains all nodes that need to be registered.
         *
         **/
        virtual void RegisterNode(const PVPlayerNodeInfo& aNodeInfo) = 0;

#ifdef HAS_OSCL_LIB_SUPPORT
        /**
         * The UnregisterNode for PVPlayerNodeRegistry. Used for unregistering nodes through the NodeInfo object.
         *
         * @param aNodeInfo NodeInfo object passed to the regisry class. This contains all nodes that need to be unregistered.
         *
         **/
        virtual void UnregisterNode(const PVPlayerNodeInfo& aNodeInfo) = 0;
#endif
};

#ifdef HAS_OSCL_LIB_SUPPORT
class PVPlayerRecognizerRegistryInterface
{
    public:
        /**
         * The RegisterRecognizer for PVPlayerRecognizerRegistry. Used for registering plugins through the PVMFRecognizerPluginFactory* object.
         *
         * @param PVMFRecognizerPluginFactory* object passed to the regisry class. This contains all nodes that need to be registered.
         *
         **/
        virtual void RegisterRecognizer(PVMFRecognizerPluginFactory* aRecognizerPluginFactory) = 0;

        /**
         * The UnregisterRecognizer for PVPlayerRecognizerRegistry. Used for unregistering plugins through the PVMFRecognizerPluginFactory* object.
         *
         * @param PVMFRecognizerPluginFactory* object passed to the regisry class. This contains all nodes that need to be unregistered.
         *
         **/
        virtual void UnregisterRecognizer(PVMFRecognizerPluginFactory* aRecognizerPluginFactory) = 0;

};
#endif

#endif // PV_PLAYER_NODE_REGISTRY_INTERFACE_H_INCLUDED


