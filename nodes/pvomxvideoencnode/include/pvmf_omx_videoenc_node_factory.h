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
#ifndef PVMF_OMX_VIDEOENC_NODE_FACTORY_H_INCLUDED
#define PVMF_OMX_VIDEOENC_NODE_FACTORY_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif

// Forward declarations
class PVMFNodeInterface;

/** Uuid of PV Video Encoder Node */
#define PVMFOMXVideoEncNodeUuid PVUuid(0xbb4d38d1,0xcc90,0x44e7,0x9a,0x38,0xbd,0x97,0x67,0x24,0xbe,0xef)

/**
 * Factory class for PVMFVideoEncNode
 */
class PVMFOMXVideoEncNodeFactory
{
public:
	/**
     * Creates an instance of a PV video encoder node. If the creation fails, this function will leave.
     *
	 * @param aPriority The active object priority for the node. Default is standard priority if not specified
     * @returns A pointer to an author or leaves if instantiation fails
     **/
	OSCL_IMPORT_REF static PVMFNodeInterface* CreateVideoEncNode(int32 aPriority=OsclActiveObject::EPriorityNominal);

    /**
     * This function allows the application to delete an instance of file input node
     * and reclaim all allocated resources.  An instance can be deleted only in
     * the idle state. An attempt to delete in any other state will fail and return false.
     *
     * @param aNode The file input node to be deleted.
     * @returns A status code indicating success or failure.
     **/
	OSCL_IMPORT_REF static bool DeleteVideoEncNode(PVMFNodeInterface* aNode);
};

#endif // PVMF_OMX_VIDEOENC_NODE_FACTORY_H_INCLUDED
