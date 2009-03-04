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
*
* @file Pvmf_amrenc_media_buffer.cpp
* @brief Media buffer for pv amr encoder node
*
*/


#ifndef PVVIDEOENCMDF_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_amrenc_media_buffer.h"
#endif

#ifndef PVMF_MEDIA_FRAG_GROUP_IN_SIMPLE_BUFFER_H_INCLUDED
#include "pvmf_media_fraggroup_in_simple_buffer.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef PVMF_RETURN_CODES_H_INCLUDED
#include "pvmf_return_codes.h"
#endif

// Constants used in this file are defined in the following h file
#ifndef PVMF_AMRENC_TUNEABLES_INCLUDED
#include "pvmf_amrenc_tuneables.h"
#endif


template <class Alloc>
class AMREncBufferCleanupDA : public OsclDestructDealloc
{
    public:
        AMREncBufferCleanupDA(Oscl_DefAlloc* in_gen_alloc) :
                gen_alloc(in_gen_alloc), mediaFragPtr(NULL) {};
        virtual ~AMREncBufferCleanupDA() {};

        virtual void destruct_and_dealloc(OsclAny* ptr)
        {
            // 1. destruct mediaFragPtr, because the vector used in mediaFragPtr needs to be deleted, otherwise there will be a memory leak
            if (mediaFragPtr) mediaFragPtr->OSCL_TEMPLATED_DESTRUCTOR_CALL(PVMFMediaFragGroupInSimpleBuffer<Alloc>,  PVMFMediaFragGroupInSimpleBuffer);

            // 2. free the memory. No need to call destructors to free the actual memory because memory pool is used
            // So real memory deallocation is done in the destructor of the memory pool
            gen_alloc->deallocate(ptr);
        }

        // For PvmfAmrEncBufferAlloc::allocate() to call private function setPointer()
        friend class PvmfAmrEncBufferAlloc;

    private:
        void setPointer(PVMFMediaFragGroupInSimpleBuffer<Alloc> *ptr)
        {
            mediaFragPtr = ptr;
        }

    private:
        Oscl_DefAlloc* gen_alloc;
        PVMFMediaFragGroupInSimpleBuffer<Alloc> *mediaFragPtr;
};


OSCL_EXPORT_REF PvmfAmrEncBufferAlloc::PvmfAmrEncBufferAlloc(Oscl_DefAlloc* opt_gen_alloc) : gen_alloc(opt_gen_alloc)
{
    if (!gen_alloc)  // No support for this static allocator(SA)
    {
        OSCL_LEAVE(PVMFErrArgument);
    }
}

OsclSharedPtr<PVMFMediaDataImpl> PvmfAmrEncBufferAlloc::allocate(uint32 aFragLen, uint32 aNumOfFragments)
{
    // Note that gen_alloc shouldn't be zero, because we put leave in constructor

    // Sanity check
    if (aNumOfFragments == 0) aNumOfFragments = PVMF_AMRENC_MEDIA_BUF_DEFAULT_NUMOFFRAGMENTS;
    if (aFragLen == 0)		 aFragLen = PVMF_AMRENC_MEDIA_BUF_DEFAULT_FRAGMENT_LENGTH;

    uint aligned_cleanup_size			= oscl_mem_aligned_size(sizeof(AMREncBufferCleanupDA<OsclMemAllocator>));
    uint aligned_refcnt_size			= oscl_mem_aligned_size(sizeof(OsclRefCounterDA));
    uint aligned_frag_group_class_size  = oscl_mem_aligned_size(sizeof(PVMFMediaFragGroupInSimpleBuffer<OsclMemAllocator>));
    //uint aligned_frag_group_class_size  = oscl_mem_aligned_size(sizeof(PVMFMediaFragGroupInSimpleBuffer));

    OsclRefCounter* my_refcnt = NULL;
    uint32 total_size, capacity_frag;
    uint8* my_ptr = NULL;
    AMREncBufferCleanupDA<OsclMemAllocator>* cleanup_ptr = NULL;


    // 1.calculate the total size and do memory allocation
    capacity_frag = aFragLen;
    total_size = aligned_refcnt_size + aligned_cleanup_size + aligned_frag_group_class_size +
                 aNumOfFragments * capacity_frag;
    my_ptr = (uint8*) gen_alloc->allocate(total_size);

    // 1.5 create clean up object (for the reference count object). Note that the first pointer should be for reference count object
    cleanup_ptr = OSCL_PLACEMENT_NEW(my_ptr + aligned_refcnt_size, AMREncBufferCleanupDA<OsclMemAllocator>(gen_alloc));

    // 2. create the recounter after the cleanup object
    my_refcnt = OSCL_PLACEMENT_NEW(my_ptr, OsclRefCounterDA(my_ptr, cleanup_ptr));
    my_ptr += (aligned_refcnt_size + aligned_cleanup_size);


    // 3. create PVMFMediaFragGroup object at the beginning (not sure if the memory position would cause problem)
    void *ptr;
    ptr = my_ptr + aligned_frag_group_class_size;
    PVMFMediaFragGroupInSimpleBuffer<OsclMemAllocator>* media_data_ptr = new(my_ptr) PVMFMediaFragGroupInSimpleBuffer<OsclMemAllocator>(
        ptr, aNumOfFragments*capacity_frag,
        my_refcnt, aNumOfFragments, capacity_frag);

    // will destruct media_data_ptr in AMREncBufferCleanupDA::destruct_and_dealloc()
    if (cleanup_ptr) cleanup_ptr->setPointer(media_data_ptr);

    // create OsclSharedPtr<PVMFMediaDataImpl> object
    OsclSharedPtr<PVMFMediaDataImpl> shared_media_data(media_data_ptr, my_refcnt);
    return shared_media_data;
}

