/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2008 HTC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "CameraInput"
#include <utils/Log.h>
#include <camera/CameraParameters.h>
#include <utils/Errors.h>
#include <media/mediarecorder.h>
#include <surfaceflinger/ISurface.h>
#include <camera/ICamera.h>
#include <camera/Camera.h>

#include "pv_mime_string_utils.h"
#include "oscl_dll.h"
#include "oscl_tickcount.h"

#include "android_camera_input.h"

using namespace android;

static const int VIEW_FINDER_FREEZE_DETECTION_TOLERANCE = 250;  // ms

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

PVRefBufferAlloc::~PVRefBufferAlloc()
{
    if(numAllocated != 0)
    {
        LOGE("Ln %d ERROR PVRefBufferAlloc numAllocated %d",__LINE__, numAllocated );
    }
}

// camera MIO
AndroidCameraInput::AndroidCameraInput()
    : OsclTimerObject(OsclActiveObject::EPriorityNominal, "AndroidCameraInput"),
    iWriteState(EWriteOK),
    iAuthorClock(NULL),
    iClockNotificationsInf(NULL),
    iAudioFirstFrameTs(0),
    pPmemInfo(NULL)
{
    LOGV("constructor(%p)", this);
    iCmdIdCounter = 0;
    iPeer = NULL;
    iThreadLoggedOn = false;
    iDataEventCounter = 0;
    iTimeStamp = 0;
    iMilliSecondsPerDataEvent = 0;
    iMicroSecondsPerDataEvent = 0;
    iState = STATE_IDLE;
    mFrameWidth = ANDROID_DEFAULT_FRAME_WIDTH;
    mFrameHeight= ANDROID_DEFAULT_FRAME_HEIGHT;
    mFrameRate  = ANDROID_DEFAULT_FRAME_RATE;
    mCamera = NULL;

    mFlags = 0;
    iFrameQueue.reserve(5);
    iFrameQueueMutex.Create();

    // setup callback listener
    mListener = new AndroidCameraInputListener(this);
}

void AndroidCameraInput::ReleaseQueuedFrames()
{
    LOGV("ReleaseQueuedFrames");
    iFrameQueueMutex.Lock();
    while (!iFrameQueue.empty()) {
        AndroidCameraInputMediaData data = iFrameQueue[0];
        iFrameQueue.erase(iFrameQueue.begin());
#if (LOG_NDEBUG == 0)
        ssize_t offset = 0;
        size_t size = 0;
        sp<IMemoryHeap> heap = data.iFrameBuffer->getMemory(&offset, &size);
        LOGD("writeComplete: ID = %d, base = %p, offset = %ld, size = %d ReleaseQueuedFrames", heap->getHeapID(), heap->base(), offset, size);
#endif
        mCamera->releaseRecordingFrame(data.iFrameBuffer);
    }
    iFrameQueueMutex.Unlock();
}

AndroidCameraInput::~AndroidCameraInput()
{
    LOGV("destructor");
    if (mCamera != NULL) {
        mCamera->setListener(NULL);
        ReleaseQueuedFrames();
        if ((mFlags & FLAGS_HOT_CAMERA) == 0) {
            LOGV("camera was cold when we started, stopping preview");
            mCamera->stopPreview();
        }
        if (mFlags & FLAGS_SET_CAMERA) {
            LOGV("unlocking camera to return to app");
            mCamera->unlock();
        } else {
            LOGV("disconnect from camera");
            mCamera->disconnect();
        }
        mFlags = 0;
        mCamera.clear();
    }
    iFrameQueueMutex.Close();
    mListener.clear();
    if(pPmemInfo)
    {
        delete pPmemInfo;
        pPmemInfo = NULL;
    }
}

PVMFStatus AndroidCameraInput::connect(PvmiMIOSession& aSession,
        PvmiMIOObserver* aObserver)
{
    LOGV("connect");
    if (!aObserver) {
        LOGE("observer is a NULL pointer");
        return PVMFFailure;
    }

    int32 err = 0;
    OSCL_TRY(err, iObservers.push_back(aObserver));
    OSCL_FIRST_CATCH_ANY(err,
        LOGE("Out of memory"); return PVMFErrNoMemory);

    // Session ID is the index of observer in the vector
    aSession = (PvmiMIOSession)(iObservers.size() - 1);
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::disconnect(PvmiMIOSession aSession)
{
    LOGV("disconnect");
    uint32 index = (uint32) aSession;
    uint32 size  = iObservers.size();
    if (index >= size) {
        LOGE("Invalid session ID %d. Valid range is [0, %d]", index, size - 1);
        return PVMFFailure;
    }

    iObservers.erase(iObservers.begin() + index);
    return PVMFSuccess;
}

PvmiMediaTransfer* AndroidCameraInput::createMediaTransfer(
        PvmiMIOSession& aSession,
        PvmiKvp* read_formats,
        int32 read_flags,
        PvmiKvp* write_formats,
        int32 write_flags)
{
    LOGV("createMediaTransfer");
    OSCL_UNUSED_ARG(read_formats);
    OSCL_UNUSED_ARG(read_flags);
    OSCL_UNUSED_ARG(write_formats);
    OSCL_UNUSED_ARG(write_flags);

    uint32 index = (uint32) aSession;
    uint32 size  = iObservers.size();
    if (index >= size) {
        LOGE("Invalid session ID %d. Valid range is [0, %d]", index, size - 1);
        OSCL_LEAVE(OsclErrArgument);
        return NULL;
    }

    return (PvmiMediaTransfer*)this;
}

void AndroidCameraInput::deleteMediaTransfer(PvmiMIOSession& aSession,
        PvmiMediaTransfer* media_transfer)
{
    LOGV("deleteMediaTransfer");
    uint32 index = (uint32) aSession;
    uint32 size  = iObservers.size();
    if (index >= size) {
        LOGE("Invalid session ID %d. Valid range is [0, %d]", index, size - 1);
        OSCL_LEAVE(OsclErrArgument);
        return;
    }
    if (!media_transfer) {
        LOGE("media transfer is a NULL pointer");
        OSCL_LEAVE(OsclErrArgument);
    }

    // TODO:
    // 1. I did not see how the media transfer session has been terminated
    //    after this method call.
    // 2. according to pvmi_mio_control.h, this should also check with there
    //    is any outstanding buffer?
}

PVMFCommandId AndroidCameraInput::QueryUUID(const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid,
        OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    LOGV("QueryUUID");
    OSCL_UNUSED_ARG(aMimeType);
    OSCL_UNUSED_ARG(aExactUuidsOnly);

    int32 err = 0;
    OSCL_TRY(err, aUuids.push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID););
    OSCL_FIRST_CATCH_ANY(err,
        LOGE("Out of memory"); OSCL_LEAVE(OsclErrNoMemory));

    return AddCmdToQueue(CMD_QUERY_UUID, aContext);
}

PVMFCommandId AndroidCameraInput::QueryInterface(const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    LOGV("QueryInterface");
    if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID) {
        PvmiCapabilityAndConfig*
           myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*,this);

        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
    } else {
        aInterfacePtr = NULL;
    }

    return AddCmdToQueue(CMD_QUERY_INTERFACE,
                         aContext,
                         (OsclAny*)&aInterfacePtr);
}

PVMFCommandId AndroidCameraInput::Init(const OsclAny* aContext)
{
    LOGV("Init");
    if (iState != STATE_IDLE) {
        LOGE("Init called in an invalid state(%d)", iState);
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_INIT, aContext);
}


PVMFCommandId AndroidCameraInput::Start(const OsclAny* aContext)
{
    LOGV("Start");
    if (iState != STATE_INITIALIZED && iState != STATE_PAUSED) {
        LOGE("Start called in an invalid state(%d)", iState);
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_START, aContext);
}

PVMFCommandId AndroidCameraInput::Pause(const OsclAny* aContext)
{
    LOGV("Pause");
    if (iState != STATE_STARTED) {
        LOGE("Pause called in an invalid state(%d)", iState);
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_PAUSE, aContext);
}

PVMFCommandId AndroidCameraInput::Flush(const OsclAny* aContext)
{
    LOGV("Flush");
    if (iState != STATE_STARTED && iState != STATE_PAUSED) {
        LOGE("Flush called in an invalid state(%d)", iState);
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_FLUSH, aContext);
}

PVMFCommandId AndroidCameraInput::Reset(const OsclAny* aContext)
{
    LOGV("Reset");
    return AddCmdToQueue(CMD_RESET, aContext);
}

PVMFCommandId AndroidCameraInput::DiscardData(PVMFTimestamp aTimestamp,
        const OsclAny* aContext)
{
    LOGV("DiscardData with time stamp");
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aTimestamp);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

PVMFCommandId AndroidCameraInput::DiscardData(const OsclAny* aContext)
{
    LOGV("DiscardData");
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

PVMFCommandId AndroidCameraInput::Stop(const OsclAny* aContext)
{
    LOGV("Stop");
    if (iState != STATE_STARTED && iState != STATE_PAUSED) {
        LOGE("Stop called in an invalid state(%d)", iState);
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_STOP, aContext);
}

void AndroidCameraInput::ThreadLogon()
{
    LOGV("ThreadLogon");
    if (!iThreadLoggedOn) {
        AddToScheduler();
        iThreadLoggedOn = true;
    }
}

void AndroidCameraInput::ThreadLogoff()
{
    LOGV("ThreadLogoff");
    if (iThreadLoggedOn) {
        RemoveFromScheduler();
        iThreadLoggedOn = false;
    }
}

PVMFCommandId AndroidCameraInput::CancelAllCommands(const OsclAny* aContext)
{
    LOGV("CancelAllCommands");
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

PVMFCommandId AndroidCameraInput::CancelCommand(PVMFCommandId aCmdId,
        const OsclAny* aContext)
{
    LOGV("CancelCommand");
    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

void AndroidCameraInput::setPeer(PvmiMediaTransfer* aPeer)
{
    LOGV("setPeer iPeer %p aPeer %p", iPeer, aPeer);
    if(iPeer && aPeer){
        LOGE("setPeer iPeer %p aPeer %p", iPeer, aPeer);
        OSCL_LEAVE(OsclErrGeneral);
        return;
    }

    iPeer = aPeer;
}

void AndroidCameraInput::useMemoryAllocators(OsclMemAllocator* write_alloc)
{
    LOGV("useMemoryAllocators");
    OSCL_UNUSED_ARG(write_alloc);
    OSCL_LEAVE(OsclErrNotSupported);
}

PVMFCommandId AndroidCameraInput::writeAsync(uint8 aFormatType,
        int32 aFormatIndex,
        uint8* aData,
        uint32 aDataLen,
        const PvmiMediaXferHeader& data_header_info,
        OsclAny* aContext)
{
    LOGV("writeAsync");
    OSCL_UNUSED_ARG(aFormatType);
    OSCL_UNUSED_ARG(aFormatIndex);
    OSCL_UNUSED_ARG(aData);
    OSCL_UNUSED_ARG(aDataLen);
    OSCL_UNUSED_ARG(data_header_info);
    OSCL_UNUSED_ARG(aContext);
    // This is an active data source. writeAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

void AndroidCameraInput::writeComplete(PVMFStatus aStatus,
       PVMFCommandId write_cmd_id,
       OsclAny* aContext)
{
    LOGV("writeComplete");
    OSCL_UNUSED_ARG(aContext);

    iFrameQueueMutex.Lock();
    if (iSentMediaData.empty()) {
        LOGE("Nothing to complete");
        iFrameQueueMutex.Unlock();
        return;
    }

    AndroidCameraInputMediaData data = iSentMediaData[0];
#if (LOG_NDEBUG == 0)
    ssize_t offset = 0;
    size_t size = 0;
    sp<IMemoryHeap> heap = data.iFrameBuffer->getMemory(&offset, &size);
    LOGD("writeComplete: ID = %d, base = %p, offset = %ld, size = %d", heap->getHeapID(), heap->base(), offset, size);
#endif
    // View finder freeze detection
    // Note for low frame rate, we don't bother to log view finder freezes
    int processingTimeInMs = (systemTime()/1000000L - iAudioFirstFrameTs) - data.iXferHeader.timestamp;
    if (processingTimeInMs >= VIEW_FINDER_FREEZE_DETECTION_TOLERANCE && mFrameRate >= 10.0) {
        LOGW("Frame %p takes too long (%d ms) to process, staring at %d", data.iFrameBuffer.get(), processingTimeInMs, iAudioFirstFrameTs);
    }
    mCamera->releaseRecordingFrame(data.iFrameBuffer);

    iSentMediaData.erase(iSentMediaData.begin());
    iFrameQueueMutex.Unlock();

    // reference count is always updated, even if the write fails
    if (aStatus != PVMFSuccess) {
        LOGE("writeAsync failed. aStatus=%d", aStatus);
    }
}

PVMFCommandId AndroidCameraInput::readAsync(uint8* data,
        uint32 max_data_len,
        OsclAny* aContext,
        int32* formats,
        uint16 num_formats)
{
    LOGV("readAsync");
    OSCL_UNUSED_ARG(data);
    OSCL_UNUSED_ARG(max_data_len);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(formats);
    OSCL_UNUSED_ARG(num_formats);
    // This is an active data source. readAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

void AndroidCameraInput::readComplete(PVMFStatus aStatus,
        PVMFCommandId read_cmd_id,
        int32 format_index,
        const PvmiMediaXferHeader& data_header_info,
        OsclAny* aContext)
{
    LOGV("readComplete");
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(read_cmd_id);
    OSCL_UNUSED_ARG(format_index);
    OSCL_UNUSED_ARG(data_header_info);
    OSCL_UNUSED_ARG(aContext);
    // This is an active data source. readComplete is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return;
}

void AndroidCameraInput::statusUpdate(uint32 status_flags)
{
    LOGV("statusUpdate");
    if (status_flags != PVMI_MEDIAXFER_STATUS_WRITE)
    {
        OSCL_LEAVE(OsclErrNotSupported);
    }
    else
    {
        // Restart the flow of data
        iWriteState = EWriteOK;
    }
}

void AndroidCameraInput::cancelCommand(PVMFCommandId aCmdId)
{
    LOGV("cancelCommand");
    OSCL_UNUSED_ARG(aCmdId);

    // This cancel command ( with a small "c" in cancel ) is for the media
    // transfer interface. implementation is similar to the cancel command
    // of the media I/O interface.
    OSCL_LEAVE(OsclErrNotSupported);
}

void AndroidCameraInput::cancelAllCommands()
{
    LOGV("cancelAllCommands");
    OSCL_LEAVE(OsclErrNotSupported);
}

void AndroidCameraInput::setObserver(
        PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    LOGV("setObserver");
    OSCL_UNUSED_ARG(aObserver);
}

PVMFStatus AndroidCameraInput::getParametersSync(PvmiMIOSession session,
        PvmiKeyType identifier,
        PvmiKvp*& params,
        int& num_params,
        PvmiCapabilityContext context)
{
    LOGV("getParametersSync");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);

    params = NULL;
    num_params = 0;
    PVMFStatus status = PVMFFailure;

    if (!pv_mime_strcmp(identifier, OUTPUT_FORMATS_CAP_QUERY) ||
        !pv_mime_strcmp(identifier, OUTPUT_FORMATS_CUR_QUERY)) {
        num_params = 1;
        status = AllocateKvp(params, (PvmiKeyType)OUTPUT_FORMATS_VALTYPE, num_params);
        if (status != PVMFSuccess) {
            LOGE("AllocateKvp failed for OUTPUT_FORMATS_VALTYP");
            return status;
        }
        params[0].value.pChar_value = (char*)ANDROID_VIDEO_FORMAT;
    } else if (!pv_mime_strcmp(identifier, VIDEO_OUTPUT_WIDTH_CUR_QUERY)) {
        num_params = 1;
        status = AllocateKvp(params, (PvmiKeyType)VIDEO_OUTPUT_WIDTH_CUR_VALUE, num_params);
        if (status != PVMFSuccess) {
            LOGE("AllocateKvp failed for VIDEO_OUTPUT_WIDTH_CUR_VALUE");
            return status;
        }
        params[0].value.uint32_value = mFrameWidth;
    } else if (!pv_mime_strcmp(identifier, VIDEO_OUTPUT_HEIGHT_CUR_QUERY)) {
        num_params = 1;
        status = AllocateKvp(params, (PvmiKeyType)VIDEO_OUTPUT_HEIGHT_CUR_VALUE, num_params);
        if (status != PVMFSuccess) {
            LOGE("AllocateKvp failed for VIDEO_OUTPUT_HEIGHT_CUR_VALUE");
            return status;
        }
        params[0].value.uint32_value = mFrameHeight;
    } else if (!pv_mime_strcmp(identifier, VIDEO_OUTPUT_FRAME_RATE_CUR_QUERY)) {
        num_params = 1;
        status = AllocateKvp(params,
            (PvmiKeyType)VIDEO_OUTPUT_FRAME_RATE_CUR_VALUE, num_params);
        if (status != PVMFSuccess) {
            LOGE("AllocateKvp failed for VIDEO_OUTPUT_FRAME_RATE_CUR_VALUE");
            return status;
        }
        params[0].value.float_value = mFrameRate;
    } else if (!pv_mime_strcmp(identifier, OUTPUT_TIMESCALE_CUR_QUERY)) {
        num_params = 1;
        status = AllocateKvp(params, (PvmiKeyType)OUTPUT_TIMESCALE_CUR_VALUE, num_params);
        if (status != PVMFSuccess) {
            LOGE("AllocateKvp failed for OUTPUT_TIMESCALE_CUR_VALUE");
            return status;
        }
        // TODO:
        // is it okay to hardcode this as the timescale?
        params[0].value.uint32_value = 1000;
    } else if (!pv_mime_strcmp(identifier, PVMF_BUFFER_ALLOCATOR_KEY)) {
        /*
         * if( camera MIO does NOT allocate YUV buffers )
         * {
         *      OSCL_LEAVE(OsclErrNotSupported);
         *      return PVMFErrNotSupported;
         * }
         */

        params = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
        if (!params )
        {
            OSCL_LEAVE(OsclErrNoMemory);
            return PVMFErrNoMemory;
        }
        params [0].value.key_specific_value = (PVInterface*)&mbufferAlloc;
        status = PVMFSuccess;
    }

    return status;
}

PVMFStatus AndroidCameraInput::releaseParameters(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements)
{
    LOGV("releaseParameters");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(num_elements);
    
    if (!parameters) {
        LOGE("parameters is a NULL pointer");
        return PVMFFailure;
    }
    iAlloc.deallocate((OsclAny*)parameters);
    return PVMFSuccess;
}

void AndroidCameraInput::createContext(PvmiMIOSession session,
        PvmiCapabilityContext& context)
{
    LOGV("createContext");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

void AndroidCameraInput::setContextParameters(PvmiMIOSession session,
        PvmiCapabilityContext& context,
        PvmiKvp* parameters,
        int num_parameter_elements)
{
    LOGV("setContextParameters");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_parameter_elements);
}

void AndroidCameraInput::DeleteContext(PvmiMIOSession session,
        PvmiCapabilityContext& context)
{
    LOGV("DeleteContext");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

void AndroidCameraInput::setParametersSync(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements,
        PvmiKvp*& ret_kvp)
{
    LOGV("setParametersSync");
    OSCL_UNUSED_ARG(session);
    PVMFStatus status = PVMFSuccess;
    ret_kvp = NULL;
    for (int32 i = 0; i < num_elements; ++i) {
        status = VerifyAndSetParameter(&(parameters[i]), true);
        if (status != PVMFSuccess) {
            LOGE("VerifiyAndSetParameter failed on parameter #%d", i);
            ret_kvp = &(parameters[i]);
            OSCL_LEAVE(OsclErrArgument);
            return;
        }
    }
}

PVMFCommandId AndroidCameraInput::setParametersAsync(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements,
        PvmiKvp*& ret_kvp,
        OsclAny* context)
{
    LOGV("setParametersAsync");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_elements);
    OSCL_UNUSED_ARG(ret_kvp);
    OSCL_UNUSED_ARG(context);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

uint32 AndroidCameraInput::getCapabilityMetric (PvmiMIOSession session)
{
    LOGV("getCapabilityMetric");
    OSCL_UNUSED_ARG(session);
    return 0;
}

PVMFStatus AndroidCameraInput::verifyParametersSync(PvmiMIOSession session,
        PvmiKvp* parameters,
        int num_elements)
{
    LOGV("verifyParametersSync");
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_elements);
    return PVMFErrNotSupported;
}


void AndroidCameraInput::SetFrameSize(int w, int h)
{
    LOGV("SetFrameSize");
    if (iState != STATE_IDLE) {
        LOGE("SetFrameSize called in an invalid state(%d)", iState);
        return;
    }

    mFrameWidth = w;
    mFrameHeight = h;
    FrameSizeChanged();
}

void AndroidCameraInput::SetFrameRate(int fps)
{
    LOGV("SetFrameRate");
    if (iState != STATE_IDLE) {
        LOGE("SetFrameRate called in an invalid state(%d)", iState);
        return;
    }

    mFrameRate = (float)fps;
}

void AndroidCameraInput::FrameSizeChanged()
{
    LOGV("FrameSizeChanged");
    if (iState != STATE_IDLE) {
        LOGE("FrameSizeChanged called in an invalid state(%d)", iState);
        return;
    }

    // Reinitialize the preview surface in case it was set up before now
    if (mSurface != NULL) {
        SetPreviewSurface(mSurface);
    }
}

void AndroidCameraInput::Run()
{
    LOGV("Run");

    // dequeue frame buffers and write to peer
    if (NULL != iPeer) {
        iFrameQueueMutex.Lock();
        while (!iFrameQueue.empty()) {
            AndroidCameraInputMediaData data = iFrameQueue[0];

            uint32 writeAsyncID = 0;
            OsclLeaveCode error = OsclErrNone;
            uint8 *ptr = (uint8*) (data.iFrameBuffer->pointer());
            if (ptr) {
                OSCL_TRY(error,writeAsyncID = iPeer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_DATA, 0, ptr,
                            data.iFrameSize, data.iXferHeader););
            } else {
                //FIXME Check why camera sends NULL frames
                LOGE("Ln %d ERROR null pointer", __LINE__);
                error = OsclErrBadHandle;
            }

            if (OsclErrNone == error) {
                iFrameQueue.erase(iFrameQueue.begin());
                data.iId = writeAsyncID;
                iSentMediaData.push_back(data);
                LOGV("Ln %d Run writeAsync writeAsyncID %d", __LINE__, writeAsyncID);
            } else {
                //FIXME resend the frame later if ( OsclErrBusy == error)
                LOGE("Ln %d Run writeAsync error %d", __LINE__, error);
                //release buffer immediately if write fails
                mCamera->releaseRecordingFrame(data.iFrameBuffer);
                iFrameQueue.erase(iFrameQueue.begin());
                iWriteState = EWriteBusy;
                break;
            }
        }
        iFrameQueueMutex.Unlock();
    }

    PVMFStatus status = PVMFFailure;

    if (!iCmdQueue.empty()) {
        AndroidCameraInputCmd cmd = iCmdQueue[0];
        iCmdQueue.erase(iCmdQueue.begin());

        switch(cmd.iType) {

        case CMD_INIT:
            status = DoInit();
            break;

        case CMD_START:
            status = DoStart();
            break;

        case CMD_PAUSE:
            status = DoPause();
            break;

        case CMD_FLUSH:
            status = DoFlush(cmd);
            break;

        case CMD_RESET:
            status = DoReset();
            break;

        case CMD_STOP:
            status = DoStop(cmd);
            break;

        case DATA_EVENT:
            // this is internal only - don't send RequestCompleted
            DoRead();
            status = PVMFPending;
            break;

        case CMD_QUERY_UUID:
        case CMD_QUERY_INTERFACE:
            status = PVMFSuccess;
            break;

        // these commands all fail
        case CMD_CANCEL_ALL_COMMANDS:
        case CMD_CANCEL_COMMAND:
        default:
            break;
        }
        // do RequestCompleted unless command is still pending
        if (status != PVMFPending) {
            DoRequestCompleted(cmd, status);
        }
    }

    if (!iCmdQueue.empty()) {
        // Run again if there are more things to process
        RunIfNotReady();
    }
}

PVMFCommandId AndroidCameraInput::AddCmdToQueue(AndroidCameraInputCmdType aType,
        const OsclAny* aContext,
        OsclAny* aData)
{
    LOGV("AddCmdToQueue");
    if (aType == DATA_EVENT) {
        LOGE("Invalid argument");
        OSCL_LEAVE(OsclErrArgument);
        return -1;
    }

    AndroidCameraInputCmd cmd;
    cmd.iType = aType;
    cmd.iContext = OSCL_STATIC_CAST(OsclAny*, aContext);
    cmd.iData = aData;
    cmd.iId = iCmdIdCounter;
    ++iCmdIdCounter;

    // TODO:
    // Check against out of memory failure
    int err = 0;
    OSCL_TRY(err, iCmdQueue.push_back(cmd));
    OSCL_FIRST_CATCH_ANY(err, LOGE("Out of memory"); return -1;);
    RunIfNotReady();
    return cmd.iId;
}

void AndroidCameraInput::AddDataEventToQueue(uint32 aMicroSecondsToEvent)
{
    LOGV("AddDataEventToQueue");
    AndroidCameraInputCmd cmd;
    cmd.iType = DATA_EVENT;

    int err = 0;
    OSCL_TRY(err, iCmdQueue.push_back(cmd));
    OSCL_FIRST_CATCH_ANY(err, LOGE("Out of memory"); return;);
    RunIfNotReady(aMicroSecondsToEvent);
}

void AndroidCameraInput::DoRequestCompleted(const AndroidCameraInputCmd& aCmd, PVMFStatus aStatus, OsclAny* aEventData)
{
    LOGV("DoRequestCompleted");
    PVMFCmdResp response(aCmd.iId, aCmd.iContext, aStatus, aEventData);

    for (uint32 i = 0; i < iObservers.size(); i++) {
        iObservers[i]->RequestCompleted(response);
    }
}

PVMFStatus AndroidCameraInput::DoInit()
{
    LOGV("DoInit()");
    iState = STATE_INITIALIZED;
    iMilliSecondsPerDataEvent = (int32)(1000 / mFrameRate);
    iMicroSecondsPerDataEvent = (int32)(1000000 / mFrameRate);
    iDataEventCounter = 0;

    // create a camera if the app didn't supply one
    if (mCamera == 0) {
        mCamera = Camera::connect();
    }

    // always call setPreviewDisplay() regardless whether mCamera is just created or not
    // return failure if no display surface is available
    if (mCamera != NULL && mSurface != NULL) {
        mCamera->setPreviewDisplay(mSurface);
    } else {
        if (mCamera == NULL) {
            LOGE("Camera is not available");
        } else if (mSurface == NULL) {
            LOGE("No surface is available for display");
        }
        return PVMFFailure;
    }

    LOGD("Intended mFrameWidth=%d, mFrameHeight=%d ",mFrameWidth, mFrameHeight);
    String8 s = mCamera->getParameters();
    if (s.length() == 0) {
        LOGE("Failed to get camera(%p) parameters", mCamera.get());
        return PVMFFailure;
    }
    CameraParameters p(s);
    p.setPreviewSize(mFrameWidth, mFrameHeight);
    p.setPreviewFrameRate(mFrameRate);
    s = p.flatten();
    if (mCamera->setParameters(s) != NO_ERROR) {
        LOGE("Failed to set camera(%p) parameters", mCamera.get());
        return PVMFFailure;
    }

    // Since we may not honor the preview size that app has requested
    // It is a good idea to get the actual preview size and used it
    // for video recording.
    CameraParameters newCameraParam(mCamera->getParameters());
    int32 width, height;
    newCameraParam.getPreviewSize(&width, &height);
    if (width < 0 || height < 0) {
        LOGE("Failed to get camera(%p) preview size", mCamera.get());
        return PVMFFailure;
    }
    if (width != mFrameWidth || height != mFrameHeight) {
        LOGE("Mismatch between the intended frame size (%dx%d) and the available frame size (%dx%d)", mFrameWidth, mFrameHeight, width, height);
        return PVMFFailure;
    }
    LOGD("Actual mFrameWidth=%d, mFrameHeight=%d ",mFrameWidth, mFrameHeight);
    if (mCamera->startPreview() != NO_ERROR) {
    LOGE("Failed to start camera(%p) preview", mCamera.get());
        return PVMFFailure;
    }
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::DoStart()
{
    LOGV("DoStart");

    iAudioFirstFrameTs = 0;
    // Set the clock state observer
    if (iAuthorClock) {
        iAuthorClock->ConstructMediaClockNotificationsInterface(iClockNotificationsInf, *this);

        if (iClockNotificationsInf == NULL) {
             return PVMFErrNoMemory;
        }

        iClockNotificationsInf->SetClockStateObserver(*this);
    }

    PVMFStatus status = PVMFFailure;
    iWriteState = EWriteOK;
    if (mCamera == NULL) {
        status = PVMFFailure;
        LOGE("mCamera is not initialized yet");
    } else {
        mCamera->setListener(mListener);
        if (mCamera->startRecording() != NO_ERROR) {
            LOGE("mCamera start recording failed");
            status = PVMFFailure;
        } else {
            iState = STATE_STARTED;
            status = PVMFSuccess;
        }
    }
    AddDataEventToQueue(iMilliSecondsPerDataEvent);
    return status;
}

PVMFStatus AndroidCameraInput::DoPause()
{
    LOGV("DoPause");
    iState = STATE_PAUSED;
    return PVMFSuccess;
}

// Does this work for reset?
PVMFStatus AndroidCameraInput::DoReset()
{
    LOGV("DoReset");
    // Remove and destroy the clock state observer
    RemoveDestroyClockObs();
    iDataEventCounter = 0;
    iWriteState = EWriteOK;
    if ( (iState == STATE_STARTED) || (iState == STATE_PAUSED) ) {
    if (mCamera != NULL) {
        mCamera->setListener(NULL);
        mCamera->stopRecording();
        ReleaseQueuedFrames();
    }
    }
    while(!iCmdQueue.empty())
    {
        AndroidCameraInputCmd cmd = iCmdQueue[0];
        iCmdQueue.erase(iCmdQueue.begin());
    }
    Cancel();
    iState = STATE_IDLE;
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::DoFlush(const AndroidCameraInputCmd& aCmd)
{
    LOGV("DoFlush");
    // This method should stop capturing media data but continue to send
    // captured media data that is already in buffer and then go to
    // stopped state.
    // However, in this case of file input we do not have such a buffer for
    // captured data, so this behaves the same way as stop.
    return DoStop(aCmd);
}

PVMFStatus AndroidCameraInput::DoStop(const AndroidCameraInputCmd& aCmd)
{
    LOGV("DoStop");

    // Remove and destroy the clock state observer
    RemoveDestroyClockObs();

    iDataEventCounter = 0;
    iWriteState = EWriteOK;
    if (mCamera != NULL) {
    mCamera->setListener(NULL);
    mCamera->stopRecording();
    ReleaseQueuedFrames();
    if(pPmemInfo)
    {
        delete pPmemInfo;
        pPmemInfo = NULL;
    }
    }
    iState = STATE_STOPPED;
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::DoRead()
{
    LOGV("DoRead");
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::AllocateKvp(PvmiKvp*& aKvp,
        PvmiKeyType aKey,
        int32 aNumParams)
{
    LOGV("AllocateKvp");
    uint8* buf = NULL;
    uint32 keyLen = oscl_strlen(aKey) + 1;
    int32 err = 0;

    OSCL_TRY(err,
        buf = (uint8*)iAlloc.allocate(aNumParams * (sizeof(PvmiKvp) + keyLen));
        if (!buf) {
            LOGE("Failed to allocate memory to Kvp");
            OSCL_LEAVE(OsclErrNoMemory);
        }
    );
    OSCL_FIRST_CATCH_ANY(err,
        LOGE("kvp allocation failed");
        return PVMFErrNoMemory;
    );

    PvmiKvp* curKvp = aKvp = new (buf) PvmiKvp;
    buf += sizeof(PvmiKvp);
    for (int32 i = 1; i < aNumParams; ++i) {
        curKvp += i;
        curKvp = new (buf) PvmiKvp;
        buf += sizeof(PvmiKvp);
    }

    for (int32 i = 0; i < aNumParams; ++i) {
        aKvp[i].key = (char*)buf;
        oscl_strncpy(aKvp[i].key, aKey, keyLen);
        buf += keyLen;
    }

    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::VerifyAndSetParameter(PvmiKvp* aKvp,
        bool aSetParam)
{
    LOGV("VerifyAndSetParameter");

    if (!aKvp) {
        LOGE("Invalid key-value pair");
        return PVMFFailure;
    }

    if (!pv_mime_strcmp(aKvp->key, OUTPUT_FORMATS_VALTYPE)) {
    if(pv_mime_strcmp(aKvp->value.pChar_value, ANDROID_VIDEO_FORMAT) == 0) {
            return PVMFSuccess;
        } else  {
            LOGE("Unsupported format %d", aKvp->value.uint32_value);
            return PVMFFailure;
        }
    }
    else if (pv_mime_strcmp(aKvp->key, PVMF_AUTHORING_CLOCK_KEY) == 0)
    {
        LOGV("AndroidCameraInput::VerifyAndSetParameter() PVMF_AUTHORING_CLOCK_KEY value %p", aKvp->value.key_specific_value);
        if( (NULL == aKvp->value.key_specific_value) && ( iAuthorClock ) )
        {
            RemoveDestroyClockObs();
        }
        iAuthorClock = (PVMFMediaClock*)aKvp->value.key_specific_value;
        return PVMFSuccess;
    }

    LOGE("Unsupported parameter(%s)", aKvp->key);
    return PVMFFailure;
}

void AndroidCameraInput::SetPreviewSurface(const sp<android::ISurface>& surface)
{
    LOGV("SetPreviewSurface");
    mSurface = surface;

    if (mCamera != NULL) {
        mCamera->setPreviewDisplay(surface);
    }
}

PVMFStatus AndroidCameraInput::SetCamera(const sp<android::ICamera>& camera)
{
    LOGV("SetCamera");
    mFlags &= ~ FLAGS_SET_CAMERA | FLAGS_HOT_CAMERA;
    if (camera == NULL) {
        LOGV("camera is NULL");
        return PVMFErrArgument;
    }

    // Connect our client to the camera remote
    mCamera = Camera::create(camera);
    if (mCamera == NULL) {
        LOGE("Unable to connect to camera");
        return PVMFErrNoResources;
    }

    LOGV("Connected to camera");
    mFlags |= FLAGS_SET_CAMERA;
    if (mCamera->previewEnabled()) {
        mFlags |= FLAGS_HOT_CAMERA;
        LOGV("camera is hot");
    }
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::postWriteAsync(nsecs_t timestamp, const sp<IMemory>& frame)
{
    LOGV("postWriteAsync");

    if (frame == NULL) {
        LOGE("frame is a NULL pointer");
        return PVMFFailure;
    }

    if((!iPeer) || (!isRecorderStarting()) || (iWriteState == EWriteBusy) || (NULL == iAuthorClock) || (iAuthorClock->GetState() != PVMFMediaClock::RUNNING)) {
        if( NULL == iAuthorClock )
        {
            LOGE("Recording is not ready (iPeer %p iState %d iWriteState %d iAuthorClock NULL), frame dropped", iPeer, iState, iWriteState);
        }
        else
        {
            LOGE("Recording is not ready (iPeer %p iState %d iWriteState %d iClockState %d), frame dropped", iPeer, iState, iWriteState, iAuthorClock->GetState());
        }
        mCamera->releaseRecordingFrame(frame);
        return PVMFSuccess;
    }

    // Now compare the video timestamp with the AudioFirstTimestamp
    // if video timestamp is earlier to audio drop it
    // or else send it downstream with correct timestamp
    uint32 ts = (uint32)(timestamp / 1000000L);

    // In cases of Video Only recording iAudioFirstFrameTs will always be zero,
    // so for such cases assign iAudioFirstFrameTs to Video's first sample TS
    // which will make Video samples to start with Timestamp zero.
    if (iAudioFirstFrameTs == 0)
        iAudioFirstFrameTs = ts;

    if (ts < iAudioFirstFrameTs) {
        // Drop the frame
        mCamera->releaseRecordingFrame(frame);
        return PVMFSuccess;
    } else {
         // calculate timestamp as offset from start time
         ts -= iAudioFirstFrameTs;
    }

    // Make sure that no two samples have the same timestamp
    if (iDataEventCounter != 0) {
        if (iTimeStamp != ts) {
            iTimeStamp = ts;
        } else {
            ++iTimeStamp;
        }
    }

    // get memory offset for frame buffer
    ssize_t offset = 0;
    size_t size = 0;
    sp<IMemoryHeap> heap = frame->getMemory(&offset, &size);
    LOGV("postWriteAsync: ID = %d, base = %p, offset = %ld, size = %d pointer %p", heap->getHeapID(), heap->base(), offset, size, frame->pointer());

    // queue data to be sent to peer
    AndroidCameraInputMediaData data;
    data.iXferHeader.seq_num = iDataEventCounter++;
    data.iXferHeader.timestamp = iTimeStamp;
    data.iXferHeader.flags = 0;
    data.iXferHeader.duration = 0;
    data.iXferHeader.stream_id = 0;

    {//compose private data
        //could size be zero?
        if(NULL == pPmemInfo)
        {
            int iCalculateNoOfCameraPreviewBuffer = heap->getSize() / size;
            LOGV("heap->getSize() = %d, size of each frame= %d, iCalculateNoOfCameraPreviewBuffer = %d", heap->getSize(), size, iCalculateNoOfCameraPreviewBuffer);
            pPmemInfo = new CAMERA_PMEM_INFO[iCalculateNoOfCameraPreviewBuffer];
            if(NULL == pPmemInfo)
            {
                LOGE("Failed to allocate the camera pmem info buffer array. iCalculateNoOfCameraPreviewBuffer %d",iCalculateNoOfCameraPreviewBuffer);
                return PVMFFailure;
            }
        }

        int iIndex = offset / size;
        pPmemInfo[iIndex].pmem_fd = heap->getHeapID();
        pPmemInfo[iIndex].offset = offset;
        data.iXferHeader.private_data_ptr = ((OsclAny*)(&pPmemInfo[iIndex]));
        LOGV("struct size %d, pmem_info - %x, &pmem_info[iIndex] - %x, iIndex =%d, pmem_info.pmem_fd = %d, pmem_info.offset = %d", sizeof(CAMERA_PMEM_INFO), pPmemInfo, &pPmemInfo[iIndex], iIndex, pPmemInfo[iIndex].pmem_fd, pPmemInfo[iIndex].offset );
    }

    data.iFrameBuffer = frame;
    data.iFrameSize = size;

    // lock mutex and queue frame buffer
    iFrameQueueMutex.Lock();
    iFrameQueue.push_back(data);
    iFrameQueueMutex.Unlock();
    RunIfNotReady();

    return PVMFSuccess; 
}

// camera callback interface
void AndroidCameraInputListener::postData(int32_t msgType, const sp<IMemory>& dataPtr)
{
}

void AndroidCameraInputListener::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
{
    if ((mCameraInput != NULL) && (msgType == CAMERA_MSG_VIDEO_FRAME)) {
        mCameraInput->postWriteAsync(timestamp, dataPtr);
    }
}

void AndroidCameraInput::NotificationsInterfaceDestroyed()
{
    iClockNotificationsInf = NULL;
}

void AndroidCameraInput::ClockStateUpdated()
{
    PVMFMediaClock::PVMFMediaClockState iClockState = iAuthorClock->GetState();
    if ((iClockState == PVMFMediaClock::RUNNING) && (iAudioFirstFrameTs == 0)) {
        // Get the clock time here
        // this will be the time of first audio frame capture
        bool tmpbool = false;
        iAuthorClock->GetCurrentTime32(iAudioFirstFrameTs, tmpbool, PVMF_MEDIA_CLOCK_MSEC);
        LOGV("Audio first ts %d", iAudioFirstFrameTs);
    }
}

void AndroidCameraInput::RemoveDestroyClockObs()
{
    if (iAuthorClock != NULL) {
        if (iClockNotificationsInf != NULL) {
            iClockNotificationsInf->RemoveClockStateObserver(*this);
            iAuthorClock->DestroyMediaClockNotificationsInterface(iClockNotificationsInf);
            iClockNotificationsInf = NULL;
        }
    }
}



