/* ------------------------------------------------------------------
 * Copyright (C) 2009 Android Open Source Project
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

#ifndef ANDROID_SURFACE_OUTPUT_FB_H_INCLUDED
#define ANDROID_SURFACE_OUTPUT_FB_H_INCLUDED

#include "android_surface_output.h"

// support for shared contiguous physical memory
#include <utils/MemoryHeapPmem.h>

// data structures for tunneling buffers
typedef struct PLATFORM_PRIVATE_PMEM_INFO
{
    /* pmem file descriptor */
    uint32 pmem_fd;
    uint32 offset;
} PLATFORM_PRIVATE_PMEM_INFO;

typedef struct PLATFORM_PRIVATE_ENTRY
{
    /* Entry type */
    uint32 type;

    /* Pointer to platform specific entry */
    OsclAny* entry;
} PLATFORM_PRIVATE_ENTRY;

typedef struct PLATFORM_PRIVATE_LIST
{
    /* Number of entries */
    uint32 nEntries;

    /* Pointer to array of platform specific entries *
     * Contiguous block of PLATFORM_PRIVATE_ENTRY elements */
    PLATFORM_PRIVATE_ENTRY* entryList;
} PLATFORM_PRIVATE_LIST;


class AndroidSurfaceOutputFB : public AndroidSurfaceOutput
{
public:
    AndroidSurfaceOutputFB();

    // frame buffer interface
    virtual bool initCheck();
    virtual PVMFStatus writeFrameBuf(uint8* aData, uint32 aDataLen, const PvmiMediaXferHeader& data_header_info);
    virtual void postLastFrame();
    virtual void closeFrameBuf();

    OSCL_IMPORT_REF ~AndroidSurfaceOutputFB();

private:
    bool getPmemFd(OsclAny *private_data_ptr, uint32 *pmemFD);
    bool getOffset(OsclAny *private_data_ptr, uint32 *offset);
    void convertFrame(void* src, void* dst, size_t len);

    // hardware frame buffer support
    sp<MemoryHeapPmem>          mHeapPmem;
    bool                        mHardwareCodec;
    uint32                      mOffset;
};

#endif // ANDROID_SURFACE_OUTPUT_FB_H_INCLUDED
