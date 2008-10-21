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
#ifndef PVMF_NODE_SHARED_LIB_INTERFACE_H_INCLUDED
#define PVMF_NODE_SHARED_LIB_INTERFACE_H_INCLUDED

#ifndef OSCL_SHARED_LIB_INTERFACE_H_INCLUDED
#include "oscl_shared_lib_interface.h"
#endif
#ifndef OSCL_SHARED_LIBRARY_H_INCLUDED
#include "oscl_shared_library.h"
#endif
#ifndef PV_PLAYER_NODE_REGISTRY_INTERFACE_H_INCLUDED
#include "pv_player_node_registry_interface.h"
#endif

#define PV_NODE_REGISTRY_INTERFACE OsclUuid(0x1d4769f0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x66)
#define PV_RECOGNIZER_INTERFACE OsclUuid(0x6d3413a0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x66)
#define PV_CREATE_NODE_INTERFACE OsclUuid(0xac8703a0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x66)
#define PV_RELEASE_NODE_INTERFACE OsclUuid(0xac8703a1,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x66)
#define PV_NODE_INTERFACE OsclUuid(0xac8703a2,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x66)

class NodeRegistrySharedLibraryInterface
{
    public:
        virtual void RegisterAllNodes(PVPlayerNodeRegistryInterface* aRegistry, OsclAny*& aContext) = 0;

        virtual void UnregisterAllNodes(PVPlayerNodeRegistryInterface* aRegistry, OsclAny* aContext) = 0;
};

// A simple macro that facilitates the packaging of a node plugin
// The usage would be
// DECLARE_NODE_SHARED_LIBRARY(new NodeSharedLibraryInterface());
//
#define DECLARE_NODE_SHARED_LIBRARY(INTERFACE_OBJECT) \
    extern "C" {\
        OsclAny *GetInterface (void) { \
        printf("\nIN THE SUPPORTED LIBS GET INTERFACE METHOD\n"); \
        return INTERFACE_OBJECT; \
        }\
    }

class NodeSharedLibraryInterface
{
    public:
        /**
         * Query for the instance of a particular interface based on the request the node UUID
         *
         * @param aNodeUuid This is the UUID to identify which node to retrieve the interface for.
         *
         * @param aInterfacePtr - output parameter filled in with the requested interface
         *                        pointer or NULL if the interface pointer is not supported.
         **/
        virtual OsclAny* QueryNodeInterface(const PVUuid& aNodeUuid, const OsclUuid& aInterfaceId) = 0;
};

class RecognizerSharedLibraryInterface
{
    public:
        virtual void RegisterAllRecognizers(PVPlayerRecognizerRegistryInterface* aRegistry, OsclAny*& aContext) = 0;

        virtual void UnregisterAllRecognizers(PVPlayerRecognizerRegistryInterface* aRegistry, OsclAny* aContext) = 0;
};

// A simple macro that facilitates the packaging of a plugin
// The usage would be
// DECLARE_SHARED_LIBRARY(new MyCustomSharedLibraryInterface());
//
#define DECLARE_SHARED_LIBRARY(INTERFACE_OBJECT) \
        extern "C" {\
        OsclAny *GetInterface (void) { \
                printf("\nIN THE SUPPORTED LIBS GET INTERFACE METHOD\n"); \
                return INTERFACE_OBJECT; \
            }\
    }


#endif // PVMF_NODE_SHARED_LIB_INTERFACE_H_INCLUDED

