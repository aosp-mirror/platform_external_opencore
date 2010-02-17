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

/*
 * This is a sample video sink using frame buffer push model. The
 * video output is NV21 (YUV 420 semi-planar with VUVU ordering).
 * This requires re-ordering the YUV 420 planar output from the
 * software codec to match the hardware color converter. The
 * encoder outputs its frames in NV21, so no re-order is necessary.
 *
 * The hardware decoder and hardware video unit share pmem buffers
 * (tunneling mode). For software codecs, we allocate a pmem buffer
 * and convert the decoded YUV frames while copying to them to the
 * pmem frame buffers used by the hardware output.
 * 
 * This code should be compiled into a libopencorehw.so module.
 * Here is a sample makefile to build the library:
 *
 * LOCAL_PATH := $(call my-dir)
 * include $(CLEAR_VARS)
 *
 * # Set up the OpenCore variables.
 * include external/opencore/Config.mk
 * LOCAL_C_INCLUDES := $(PV_INCLUDES)
 *
 * LOCAL_SRC_FILES := android_surface_output_fb.cpp
 * 
 * LOCAL_CFLAGS := $(PV_CFLAGS)
 * 
 * LOCAL_SHARED_LIBRARIES := \
 *     libutils \
 *     libcutils \
 *     libui \
 *     libhardware\
 *     libandroid_runtime \
 *     libmedia \
 *     libsgl \
 *     libopencorecommon \
 *     libicuuc \
 *     libopencoreplayer
 * 
 * LOCAL_MODULE := libopencorehw
 * 
 * LOCAL_LDLIBS += 
 * 
 * include $(BUILD_SHARED_LIBRARY)
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "VideoMioFB"
#include <utils/Log.h>

#include "android_surface_output_fb.h"
#include <media/PVPlayer.h>

#define PLATFORM_PRIVATE_PMEM 1

#if HAVE_ANDROID_OS
#include <linux/android_pmem.h>
#endif

using namespace android;

static const char* pmem_adsp = "/dev/pmem_adsp";
static const char* pmem = "/dev/pmem";

OSCL_EXPORT_REF AndroidSurfaceOutputFB::AndroidSurfaceOutputFB() :
    AndroidSurfaceOutput()
{
    mHardwareCodec = false;
}

OSCL_EXPORT_REF AndroidSurfaceOutputFB::~AndroidSurfaceOutputFB()
{
}

// create a frame buffer for software codecs
OSCL_EXPORT_REF bool AndroidSurfaceOutputFB::initCheck()
{

    // initialize only when we have all the required parameters
    if (((iVideoParameterFlags & VIDEO_SUBFORMAT_VALID) == 0) || !checkVideoParameterFlags())
        return mInitialized;

    // release resources if previously initialized
    closeFrameBuf();

    // reset flags in case display format changes in the middle of a stream
    resetVideoParameterFlags();

    // copy parameters in case we need to adjust them
    int displayWidth = iVideoDisplayWidth;
    int displayHeight = iVideoDisplayHeight;
    int frameWidth = iVideoWidth;
    int frameHeight = iVideoHeight;
    int frameSize;

    // MSM72xx hardware codec uses semi-planar format
    if (iVideoSubFormat == PVMF_YUV420_SEMIPLANAR_YVU) {
        LOGV("using hardware codec");
        mHardwareCodec = true;
    } else {
        LOGV("using software codec");

        // YUV420 frames are 1.5 bytes/pixel
        frameSize = (frameWidth * frameHeight * 3) / 2;

        // create frame buffer heap
        sp<MemoryHeapBase> master = new MemoryHeapBase(pmem_adsp, frameSize * kBufferCount);
        if (master->heapID() < 0) {
            LOGE("Error creating frame buffer heap");
            return false;
        }
        master->setDevice(pmem);
        mHeapPmem = new MemoryHeapPmem(master, 0);
        mHeapPmem->slap();
        master.clear();
        ISurface::BufferHeap buffers(displayWidth, displayHeight, 
                frameWidth, frameHeight, HAL_PIXEL_FORMAT_YCbCr_420_SP, mHeapPmem);
        mSurface->registerBuffers(buffers);

        // create frame buffers
        for (int i = 0; i < kBufferCount; i++) {
            mFrameBuffers[i] = i * frameSize;
        }

        LOGV("video = %d x %d", displayWidth, displayHeight);
        LOGV("frame = %d x %d", frameWidth, frameHeight);
        LOGV("frame #bytes = %d", frameSize);

        // register frame buffers with SurfaceFlinger
        mFrameBufferIndex = 0;
    }

    mInitialized = true;
    LOGV("sendEvent(MEDIA_SET_VIDEO_SIZE, %d, %d)", iVideoDisplayWidth, iVideoDisplayHeight);
    mPvPlayer->sendEvent(MEDIA_SET_VIDEO_SIZE, iVideoDisplayWidth, iVideoDisplayHeight);
    return mInitialized;
}

PVMFStatus AndroidSurfaceOutputFB::writeFrameBuf(uint8* aData, uint32 aDataLen, const PvmiMediaXferHeader& data_header_info)
{
    if (mSurface == 0) return PVMFFailure;

    // hardware codec
    if (mHardwareCodec) {

        // initialize frame buffer heap
        if (mHeapPmem == 0) {
            LOGV("initializing for hardware");
            LOGV("private data pointer is 0%p\n", data_header_info.private_data_ptr);

            // check for correct video format
            if (iVideoSubFormat != PVMF_YUV420_SEMIPLANAR_YVU) return PVMFFailure;

            uint32 fd;
            if (!getPmemFd(data_header_info.private_data_ptr, &fd)) {
                LOGE("Error getting pmem heap from private_data_ptr");
                return PVMFFailure;
            }
            sp<MemoryHeapBase> master = (MemoryHeapBase *) fd;
            master->setDevice(pmem);
            mHeapPmem = new MemoryHeapPmem(master, 0);
            mHeapPmem->slap();
            master.clear();

            // register frame buffers with SurfaceFlinger
            ISurface::BufferHeap buffers(iVideoDisplayWidth, iVideoDisplayHeight, 
                    iVideoWidth, iVideoHeight,
                    HAL_PIXEL_FORMAT_YCbCr_420_SP, mHeapPmem);
            mSurface->registerBuffers(buffers);
        }

        // get pmem offset and post to SurfaceFlinger
        if (!getOffset(data_header_info.private_data_ptr, &mOffset)) {
            LOGE("Error getting pmem offset from private_data_ptr");
            return PVMFFailure;
        }
        mSurface->postBuffer(mOffset);
    } else {

        // software codec
        convertFrame(aData, static_cast<uint8*>(mHeapPmem->base()) + mFrameBuffers[mFrameBufferIndex], aDataLen);
        // post to SurfaceFlinger
        if (++mFrameBufferIndex == kBufferCount) mFrameBufferIndex = 0;
        mSurface->postBuffer(mFrameBuffers[mFrameBufferIndex]);
    }

    return PVMFSuccess;
}

// post the last video frame to refresh screen after pause
void AndroidSurfaceOutputFB::postLastFrame()
{
    mSurface->postBuffer(mOffset);
}

void AndroidSurfaceOutputFB::closeFrameBuf()
{
    LOGV("CloseFrameBuf");
    if (!mInitialized) return;

    mInitialized = false;
    if (mSurface.get()) {
        LOGV("unregisterBuffers");
        mSurface->unregisterBuffers();
        mSurface.clear();
    }

    // free frame buffers
    LOGV("free frame buffers");
    for (int i = 0; i < kBufferCount; i++) {
        mFrameBuffers[i] = 0;
    }

    // free heaps
    LOGV("free mFrameHeap");
    mFrameHeap.clear();
    LOGV("free mHeapPmem");
    mHeapPmem.clear();
}

bool AndroidSurfaceOutputFB::getPmemFd(OsclAny *private_data_ptr, uint32 *pmemFD)
{
    PLATFORM_PRIVATE_LIST *listPtr = NULL;
    PLATFORM_PRIVATE_PMEM_INFO *pmemInfoPtr = NULL;
    bool returnType = false;
    LOGV("in getPmemfd - privatedataptr=%p\n",private_data_ptr);
    listPtr = (PLATFORM_PRIVATE_LIST*) private_data_ptr;

    for (uint32 i=0;i<listPtr->nEntries;i++)
    {
        if(listPtr->entryList->type == PLATFORM_PRIVATE_PMEM)
        {
            LOGV("in getPmemfd - entry type = %d\n",listPtr->entryList->type);
          pmemInfoPtr = (PLATFORM_PRIVATE_PMEM_INFO*) (listPtr->entryList->entry);
          returnType = true;
          if(pmemInfoPtr){
            *pmemFD = pmemInfoPtr->pmem_fd;
            LOGV("in getPmemfd - pmemFD = %d\n",*pmemFD);
          }
          break;
        }
    }
    return returnType;
}

bool AndroidSurfaceOutputFB::getOffset(OsclAny *private_data_ptr, uint32 *offset)
{
    PLATFORM_PRIVATE_LIST *listPtr = NULL;
    PLATFORM_PRIVATE_PMEM_INFO *pmemInfoPtr = NULL;
    bool returnType = false;

    listPtr = (PLATFORM_PRIVATE_LIST*) private_data_ptr;
    LOGV("in getOffset: listPtr = %p\n",listPtr);
    for (uint32 i=0;i<listPtr->nEntries;i++)
    {
        if(listPtr->entryList->type == PLATFORM_PRIVATE_PMEM)
        {
            LOGV(" in getOffset: entrytype = %d\n",listPtr->entryList->type);

          pmemInfoPtr = (PLATFORM_PRIVATE_PMEM_INFO*) (listPtr->entryList->entry);
          returnType = true;
          if(pmemInfoPtr){
            *offset = pmemInfoPtr->offset;
            LOGV("in getOffset: offset = %d\n",*offset);
          }
          break;
        }
    }
    return returnType;
}

static inline void* byteOffset(void* p, size_t offset) { return (void*)((uint8_t*)p + offset); }

void AndroidSurfaceOutputFB::convertFrame(void* src, void* dst, size_t len)
{
    // copy the Y plane
    size_t y_plane_size = iVideoWidth * iVideoHeight;
    //LOGV("len=%u, y_plane_size=%u", len, y_plane_size);
    memcpy(dst, src, y_plane_size + iVideoWidth);

    // re-arrange U's and V's
    uint16_t* pu = (uint16_t*)byteOffset(src, y_plane_size);
    uint16_t* pv = (uint16_t*)byteOffset(pu, y_plane_size / 4);
    uint32_t* p = (uint32_t*)byteOffset(dst, y_plane_size);

    int count = y_plane_size / 8;
    //LOGV("u = %p, v = %p, p = %p, count = %d", pu, pv, p, count);
    do {
        uint32_t u = *pu++;
        uint32_t v = *pv++;
        *p++ = ((u & 0xff) << 8) | ((u & 0xff00) << 16) | (v & 0xff) | ((v & 0xff00) << 8);
    } while (--count);
}

// factory function for playerdriver linkage
extern "C" AndroidSurfaceOutput* createVideoMio()
{
    return new AndroidSurfaceOutputFB();
}

