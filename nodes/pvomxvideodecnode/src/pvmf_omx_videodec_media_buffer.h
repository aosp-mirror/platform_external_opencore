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
#ifndef PV_OMX_VIDEODEC_MEDIA_BUFFER_H_INCLUDED
#define PV_OMX_VIDEODEC_MEDIA_BUFFER_H_INCLUDED


#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif

#ifndef OSCL_SCHEDULER_H_INCLUDED
#include "oscl_scheduler.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif

#ifndef OSCL_SHARED_PTR_H_INCLUDED
#include "oscl_shared_ptr.h"
#endif


#ifndef PVMF_SIMPLE_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_simple_media_buffer.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////
/////////////////////////
/////////////////////////
// CUSTOM DEALLOCATOR FOR MEDIA DATA SHARED PTR WRAPPER:
//						1) Deallocates the underlying output buffer
//						2) Deallocates the pvci buffer wrapper and the rest of accompanying structures
//					  Deallocator is created as part of the wrapper, and travels with the buffer wrapper

class PVOMXBufferSharedPtrWrapperCombinedCleanupDA : public OsclDestructDealloc
{
    public:
        PVOMXBufferSharedPtrWrapperCombinedCleanupDA(Oscl_DefAlloc* allocator, void *pMempoolData) :
                buf_alloc(allocator), ptr_to_data_to_dealloc(pMempoolData) {};
        virtual ~PVOMXBufferSharedPtrWrapperCombinedCleanupDA() {};

        virtual void destruct_and_dealloc(OsclAny* ptr)
        {
            // call buffer deallocator
            if (buf_alloc != NULL)
            {
                buf_alloc->deallocate(ptr_to_data_to_dealloc);
            }

            // finally, free the shared ptr wrapper memory
            oscl_free(ptr);
        }

    private:
        Oscl_DefAlloc* buf_alloc;
        void *ptr_to_data_to_dealloc;
};

#endif

