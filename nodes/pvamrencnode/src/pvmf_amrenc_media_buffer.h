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
 * @file pvmf_amrenc_media_buffer.h
 * @brief Media buffer to hold video encoder MDF output
 *
 */

#ifndef PVMF_AMRENC_MEDIA_BUFFER_H_INCLUDED
#define PVMF_AMRENC_MEDIA_BUFFER_H_INCLUDED

#ifndef PVMF_MEDIA_DATA_IMPL_H_INCLUDED
#include "pvmf_media_data_impl.h"
#endif
#ifndef OSCL_SHARED_PTR_H_INCLUDED
#include "oscl_shared_ptr.h"
#endif

/*
  This class is specially for creating PVMFMediaFragGroup object that contains multiple fragments (OsclRefCounterMemFrag),
  with the specific structure (see code)
  Note that PVMFSimpleMediaBuffer only contains 1 fragment.
*/
class PvmfAmrEncBufferAlloc
{
    public:

        //! constructor
        OSCL_IMPORT_REF PvmfAmrEncBufferAlloc(Oscl_DefAlloc* opt_gen_alloc = 0);

        /**
         * Create a PVMFMediaFragGroup object that contains multiple fragments
         * @param aNumOfFragments, the number of fragments within the generated PVMFMediaFragGroup object
         * @param aFragLen, the length of each fragment (usually the maximum length)
         * @return the shared pointer of the PVMFMediaFragGroup object
         */
        virtual OsclSharedPtr<PVMFMediaDataImpl> allocate(uint32 aFragLen, uint32 aNumOfFragments);

    private:
        Oscl_DefAlloc* gen_alloc;
};

#endif // PVMF_AMRENC_MEDIA_BUFFER_H_INCLUDED

