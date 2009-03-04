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
#ifndef PVMF_MEDIA_FRAG_GROUP_IN_SIMPLE_BUFFER_H_INCLUDED
#define PVMF_MEDIA_FRAG_GROUP_IN_SIMPLE_BUFFER_H_INCLUDED

#ifndef PVMF_MEDIA_DATA_IMPL_H_INCLUDED
#include "pvmf_media_data_impl.h"
#endif
#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif
#ifndef OSCL_DEFALLOC_H_INCLUDED
#include "Oscl_defalloc.h"
#endif
#ifndef OSCL_EXCEPTION_H_INCLUDED
#include "oscl_exception.h"
#endif
#ifndef OSCL_ERROR_CODES_H_INCLUDED
#include "oscl_error_codes.h"
#endif

/**
 * The PVMFMediaFragGroupInSimpleBuffer class is a MediaDataImpl implementation that
 * stores multiple OsclRefCounterMemFrag in one chunk of memory with one shared reference counter,
 * especially for multiple AMR fremes encoding. It takes an allocator as a templatized parameter to
 * allow the user to determine how memory for storing the fragments internally is allocated.
 * The PVMFMediaFragGroupInSimpleBuffer is created with a fixed capacity that is passed in at
 * construction time.
 */
template <class Alloc>
class PVMFMediaFragGroupInSimpleBuffer : public PVMFMediaDataImpl
{

    public:
        OSCL_IMPORT_REF virtual uint32 getMarkerInfo();
        OSCL_IMPORT_REF virtual bool isRandomAccessPoint();
        OSCL_IMPORT_REF virtual uint32 getErrorsFlag();
        OSCL_IMPORT_REF virtual uint32 getNumFragments();
        OSCL_IMPORT_REF virtual bool getMediaFragment(uint32 index, OsclRefCounterMemFrag& memfrag);
        OSCL_IMPORT_REF virtual bool getMediaFragmentSize(uint32, uint32&);
        OSCL_IMPORT_REF virtual uint32 getFilledSize();
        OSCL_IMPORT_REF virtual uint32 getCapacity();
        OSCL_IMPORT_REF virtual void setCapacity(uint32 aCapacity);
        OSCL_IMPORT_REF virtual bool setMediaFragFilledLen(uint32 index, uint32 len);
        OSCL_IMPORT_REF virtual bool setMarkerInfo(uint32 marker);
        OSCL_IMPORT_REF virtual bool setRandomAccessPoint(bool flag);
        OSCL_IMPORT_REF virtual bool setErrorsFlag(uint32 flag);

        OSCL_IMPORT_REF virtual bool appendMediaFragment(OsclRefCounterMemFrag& memfrag);
        OSCL_IMPORT_REF virtual bool clearMediaFragments();

        /* NOTE!!:  The constructor assumes the refcnt has already been incremented
         * to reflect this class holding a reference to the buffer. Increment it
         * externally if you aren't simply passing ownership of a reference
         */
        OSCL_IMPORT_REF PVMFMediaFragGroupInSimpleBuffer(void * ptr, uint32 buffer_length,
                OsclRefCounter *my_refcnt,
                uint32 group_capacity,
                uint32 frag_capacity_in);

        OSCL_IMPORT_REF virtual ~PVMFMediaFragGroupInSimpleBuffer();

    private:
        uint32 marker_info;
        bool random_access_point;
        uint32 errors_flag;
        OsclMemoryFragment buffer;
        Oscl_Vector<uint32, Alloc> iFragLengths;
        uint32 group_capacity;
        uint32 frag_capacity;
        uint32 filled_size;
        OsclRefCounter* refcnt;

};


template<class Alloc>
OSCL_EXPORT_REF PVMFMediaFragGroupInSimpleBuffer<Alloc>::~PVMFMediaFragGroupInSimpleBuffer()
{
    // clear all the fragments
    iFragLengths.clear();
}

template<class Alloc>
OSCL_EXPORT_REF uint32 PVMFMediaFragGroupInSimpleBuffer<Alloc>::getMarkerInfo()
{
    return marker_info;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::isRandomAccessPoint()
{
    return random_access_point;
}

template<class Alloc>
OSCL_EXPORT_REF uint32 PVMFMediaFragGroupInSimpleBuffer<Alloc>::getErrorsFlag()
{
    return errors_flag;
}

template<class Alloc>
OSCL_EXPORT_REF uint32 PVMFMediaFragGroupInSimpleBuffer<Alloc>::getNumFragments()
{
    return iFragLengths.size();
}

template<class Alloc>
OSCL_EXPORT_REF uint32 PVMFMediaFragGroupInSimpleBuffer<Alloc>::getFilledSize()
{
    return filled_size;
}

template<class Alloc>
OSCL_EXPORT_REF uint32 PVMFMediaFragGroupInSimpleBuffer<Alloc>::getCapacity()
{
    return group_capacity;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::getMediaFragment(uint32 index, OsclRefCounterMemFrag& memfrag)
{
    if (index != 0 && index >= iFragLengths.size())
    {
        return false;
    }

    if (index == 0 && iFragLengths.empty())   // at this time, iFragLengths have no element, then send out buffer to let
    {
        OsclMemoryFragment raw_memFrag;			// the user get the buffer pointer
        uint8 *ptr = (uint8 *)buffer.ptr;
        raw_memFrag.ptr = (void *)ptr;
        raw_memFrag.len = frag_capacity;
        memfrag = OsclRefCounterMemFrag(raw_memFrag, refcnt, iFragLengths[index]);
    }
    else    // index < iFragLengths.size()
    {
        uint32 i = 0, offset = 0;
        for (i = 0; i < index; i++) offset += iFragLengths[i];

        OsclMemoryFragment raw_memFrag;
        uint8 *ptr = (uint8 *)buffer.ptr;
        raw_memFrag.ptr = (void *)(&ptr[offset]);
        raw_memFrag.len = iFragLengths[index];
        memfrag = OsclRefCounterMemFrag(raw_memFrag, refcnt, iFragLengths[index]);
    }

    // explicitly addref
    refcnt->addRef();

    return true;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::getMediaFragmentSize(uint32 index, uint32& size)
{
    size = 0;
    if (index >= iFragLengths.size())
    {
        return false;
    }

    size = iFragLengths[index];

    return true;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::setMediaFragFilledLen(uint32 index, uint32 len)
{
    if (index > iFragLengths.size() || index >= group_capacity)
    {
        return false;
    }

    if (len > frag_capacity)
    {
        return false;
    }

    if (index == iFragLengths.size())
    {
        iFragLengths.push_back(len);
        filled_size += len;
    }
    else
    {
        filled_size -= iFragLengths[index];
        iFragLengths[index] = len;
        filled_size += len;
    }

    return true;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::setMarkerInfo(uint32 in_marker)
{
    marker_info = in_marker;
    return true;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::setRandomAccessPoint(bool flag)
{
    random_access_point = flag;
    return true;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::setErrorsFlag(uint32 flag)
{
    errors_flag = flag;
    return true;
}

template<class Alloc>
OSCL_EXPORT_REF PVMFMediaFragGroupInSimpleBuffer<Alloc>::PVMFMediaFragGroupInSimpleBuffer(void * ptr, uint32 buffer_length,
        OsclRefCounter *my_refcnt,
        uint32 group_capacity_in,
        uint32 frag_capacity_in) :
        marker_info(0), random_access_point(false), errors_flag(0),
        group_capacity(group_capacity_in), frag_capacity(frag_capacity_in), filled_size(0), refcnt(my_refcnt)
{
    buffer.ptr = ptr;
    buffer.len = buffer_length;
    iFragLengths.reserve(group_capacity);
}


// Not supported, because there is one chunck of memory shared for all the fragments
template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::appendMediaFragment(OsclRefCounterMemFrag& memfrag)
{
    OSCL_UNUSED_ARG(memfrag);
    return false;
}

template<class Alloc>
OSCL_EXPORT_REF bool PVMFMediaFragGroupInSimpleBuffer<Alloc>::clearMediaFragments()
{
    iFragLengths.clear();
    group_capacity = 0;
    frag_capacity  = 0;
    filled_size = 0;
    return true;
}

template<class Alloc>
OSCL_EXPORT_REF void PVMFMediaFragGroupInSimpleBuffer<Alloc>::setCapacity(uint32 aCapacity)
{
    OSCL_UNUSED_ARG(aCapacity);
    OSCL_LEAVE(OsclErrNotSupported);
}
#endif // PVMF_MEDIA_FRAG_GROUP_IN_SIMPLE_BUFFER_H_INCLUDED

