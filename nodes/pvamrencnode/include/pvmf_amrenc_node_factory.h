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
#ifndef PVMF_AMRENC_NODE_FACTORY_H_INCLUDED
#define PVMF_AMRENC_NODE_FACTORY_H_INCLUDED

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

#define PvmfAmrEncNodeUuid PVUuid(0x8274e1d1, 0xd604, 0x43e6, 0xa8, 0xd4, 0xb9, 0x7a, 0x14, 0x0e, 0x14, 0x4c)

/**
 * Factory class for PvmfAmrEncNode
 */
class PvmfAmrEncNodeFactory
{
    public:
        /**
         * Creates an instance of a PV AMR Encoder node
         *
         * @param aPriority Priority of active objects of PvmfAmrEncNode
         * @returns A pointer to an author or NULL if instantiation fails
         **/
        OSCL_IMPORT_REF static PVMFNodeInterface* Create(int32 aPriority = OsclActiveObject::EPriorityNominal);

        /**
         * This function allows the application to delete an instance of file input node
         * and reclaim all allocated resources.  An instance can be deleted only in
         * the idle state. An attempt to delete in any other state will fail and return false.
         *
         * @param aNode The file input node to be deleted.
         * @returns A status code indicating success or failure.
         **/
        OSCL_IMPORT_REF static bool Delete(PVMFNodeInterface* aNode);
};

#endif // PVMF_AMRENC_NODE_FACTORY_H_INCLUDED
