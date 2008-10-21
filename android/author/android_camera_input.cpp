#include "oscl_base.h"
//#include "android_camera_input_factory.h"
#include "android_camera_input.h"
#include "pv_mime_string_utils.h"
#include "oscl_dll.h"
#include "oscl_tickcount.h"

//#include <hardware/camera.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#undef LOG_TAG
#define LOG_TAG "CameraInput"
#include <utils/Log.h>

using namespace android;

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

// Logging macros
#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m)
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m)
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m)

#if 0
#include "yuv_data.h"
#endif

////////////////////////////////////////////////////////////////////////////
AndroidCameraInput::~AndroidCameraInput()
{
    if(iMediaBufferMemPool)
    {
        OSCL_TEMPLATED_DELETE(iMediaBufferMemPool, OsclMemPoolFixedChunkAllocator, OsclMemPoolFixedChunkAllocator);
        iMediaBufferMemPool = NULL;
    }

    if(camera_output_buf)
    {
        free(camera_output_buf);
        camera_output_buf = NULL;
    }

    if(iYuv422toYuv420)
    {
	    OSCL_DELETE(iYuv422toYuv420);
	    iYuv422toYuv420 = NULL;
    }

    if (iCameraFd >= 0) {
        //android::camera_close_driver(iCameraFd);
        iCameraFd = -1;
    }

    delete iColorConverter;
    iColorConverter = NULL;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus AndroidCameraInput::connect(PvmiMIOSession& aSession, PvmiMIOObserver* aObserver)
{
    if(!aObserver)
    {
        return PVMFFailure;
    }

    int32 err = 0;
    OSCL_TRY(err, iObservers.push_back(aObserver));
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);
    aSession = (PvmiMIOSession)(iObservers.size() - 1); // Session ID is the index of observer in the vector
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus AndroidCameraInput::disconnect(PvmiMIOSession aSession)
{
    uint32 index = (uint32)aSession;
    if(index >= iObservers.size())
    {
        // Invalid session ID
        return PVMFFailure;
    }

    iObservers.erase(iObservers.begin()+index);
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PvmiMediaTransfer* AndroidCameraInput::createMediaTransfer(PvmiMIOSession& aSession, 
                                                                       PvmiKvp* read_formats,
                                                                       int32 read_flags,
                                                                       PvmiKvp* write_formats,
                                                                       int32 write_flags)
{
    OSCL_UNUSED_ARG(read_formats);
    OSCL_UNUSED_ARG(read_flags);
    OSCL_UNUSED_ARG(write_formats);
    OSCL_UNUSED_ARG(write_flags);

    uint32 index = (uint32)aSession;
    if(index >= iObservers.size())
    {
        // Invalid session ID
        OSCL_LEAVE(OsclErrArgument);
        return NULL;
    }

    return (PvmiMediaTransfer*)this;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::deleteMediaTransfer(PvmiMIOSession& aSession,
                                                         PvmiMediaTransfer* media_transfer)
{
    uint32 index = (uint32)aSession;
    if(!media_transfer || index >= iObservers.size())
    {
        // Invalid session ID
        OSCL_LEAVE(OsclErrArgument);
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::QueryUUID(const PvmfMimeString& aMimeType, 
                                    Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                    bool aExactUuidsOnly,
                                    const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aMimeType);
    OSCL_UNUSED_ARG(aExactUuidsOnly);

    int32 err = 0;
    OSCL_TRY(err, aUuids.push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID););
    OSCL_FIRST_CATCH_ANY(err, OSCL_LEAVE(OsclErrNoMemory););

    return AddCmdToQueue(CMD_QUERY_UUID, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::QueryInterface(const PVUuid& aUuid,
                                                             PVInterface*& aInterfacePtr,
                                                             const OsclAny* aContext)
{
    if(aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*,this);
        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else
    {
        aInterfacePtr = NULL;
    }

    return AddCmdToQueue(CMD_QUERY_INTERFACE, aContext, (OsclAny*)&aInterfacePtr);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput:: Init(const OsclAny* aContext)
{
    if(iState != STATE_IDLE)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_INIT, aContext);
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::Start(const OsclAny* aContext)
{
    if(iState != STATE_INITIALIZED && iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_START, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::Pause(const OsclAny* aContext)
{
    if(iState != STATE_STARTED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_PAUSE, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::Flush(const OsclAny* aContext)
{
    if(iState != STATE_STARTED || iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_FLUSH, aContext);
}

OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::Reset(const OsclAny* aContext)
{
    if(iState != STATE_STARTED || iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_RESET, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::DiscardData(PVMFTimestamp aTimestamp, const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aTimestamp);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::DiscardData(const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::Stop(const OsclAny* aContext)
{
    if(iState != STATE_STARTED && iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_STOP, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::ThreadLogon()
{
    if(!iThreadLoggedOn)
    {
        AddToScheduler();
        iLogger = PVLogger::GetLoggerObject("AndroidCameraInput");
        iThreadLoggedOn = true;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::ThreadLogoff()
{
    if(iThreadLoggedOn)
    {
        RemoveFromScheduler();
        iLogger = NULL;
        iThreadLoggedOn = false;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::CancelAllCommands( const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::CancelCommand( PVMFCommandId aCmdId, const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::setPeer(PvmiMediaTransfer* aPeer)
{
    if(iPeer || !aPeer)
    {
        OSCL_LEAVE(OsclErrGeneral);
        return;
    }

    iPeer = aPeer;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::useMemoryAllocators(OsclMemAllocator* write_alloc)
{
    OSCL_UNUSED_ARG(write_alloc);
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::writeAsync(uint8 aFormatType, int32 aFormatIndex,
                                                         uint8* aData, uint32 aDataLen,
                                                         const PvmiMediaXferHeader& data_header_info, 
                                                         OsclAny* aContext)
{
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

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::writeComplete(PVMFStatus aStatus, PVMFCommandId write_cmd_id,
                                                   OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    if(aStatus != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
            (0,"AndroidCameraInput::writeComplete: Error - writeAsync failed. aStatus=%d", aStatus));
        return;
    }

    if(iSentMediaData.empty())
    {
        // Error: Nothing to complete
        return;
    }

    if(iSentMediaData[0].iId != write_cmd_id)
    {
        // Error: unmatching ID. They should be in sequence
        return;
    }

    if (mSurface != NULL) {
        Surface::SurfaceInfo info;
        mSurface->lock(&info);

        // If the surface dimensions have changed, reinitialize the color converter.
        if (info.w != mSurfaceWidth || info.h != mSurfaceHeight) {
            iColorConverter->Init(mFrameWidth,mFrameHeight,mFrameWidth,mFrameWidth,mFrameHeight,info.w);
            mSurfaceWidth = info.w;
            mSurfaceHeight = info.h;
        }

        iColorConverter->Convert((uint8 *)iSentMediaData[0].iData, (uint8 *)info.bits);
        //iColorConverter->Convert( camera_output_buf, (uint8 *)info.bits);
        mSurface->unlockAndPost();
    }

    iMediaBufferMemPool->deallocate(iSentMediaData[0].iData);
    iSentMediaData.erase(iSentMediaData.begin());
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::readAsync(uint8* data, uint32 max_data_len,
                                                        OsclAny* aContext, int32* formats, uint16 num_formats)
{
    OSCL_UNUSED_ARG(data);
    OSCL_UNUSED_ARG(max_data_len);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(formats);
    OSCL_UNUSED_ARG(num_formats);
    // This is an active data source. readAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::readComplete(PVMFStatus aStatus, PVMFCommandId read_cmd_id,
                                                  int32 format_index, const PvmiMediaXferHeader& data_header_info,
                                                  OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(read_cmd_id);
    OSCL_UNUSED_ARG(format_index);
    OSCL_UNUSED_ARG(data_header_info);
    OSCL_UNUSED_ARG(aContext);
    // This is an active data source. readComplete is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::statusUpdate(uint32 status_flags)
{
    OSCL_UNUSED_ARG(status_flags);
    // Ideally this routine should update the status of media input component.
    // It should check then for the status. If media input buffer is consumed,
    // media input object should be resheduled.
    // Since the Media fileinput component is designed with single buffer, two
    // asynchronous reads are not possible. So this function will not be required
    // and hence not been implemented.
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::cancelCommand(PVMFCommandId aCmdId)
{
    OSCL_UNUSED_ARG(aCmdId);
    // This cancel command ( with a small "c" in cancel ) is for the media transfer interface.
    // implementation is similar to the cancel command of the media I/O interface.
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::cancelAllCommands()
{
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    OSCL_UNUSED_ARG(aObserver);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus AndroidCameraInput::getParametersSync(PvmiMIOSession session,
                                                             PvmiKeyType identifier,
                                                             PvmiKvp*& parameters,
                                                             int& num_parameter_elements,
                                                             PvmiCapabilityContext context)
{
    LOG_STACK_TRACE((0,"AndroidCameraInput::getParametersSync"));
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);

    parameters = NULL;
    num_parameter_elements = 0;
    PVMFStatus status = PVMFFailure;
    
    if( pv_mime_strcmp(identifier, OUTPUT_FORMATS_CAP_QUERY) == 0 ||
        pv_mime_strcmp(identifier, OUTPUT_FORMATS_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, OUTPUT_FORMATS_VALTYPE, num_parameter_elements);
        if(status != PVMFSuccess)
        {
            LOG_ERR((0,"AndroidCameraInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",status));
        }
        else
        {
            //parameters[0].value.uint32_value = ANDROID_VIDEO_FORMAT;
            parameters[0].value.uint32_value = PVMF_YUV420;
        }
    }
    else if(pv_mime_strcmp(identifier, VIDEO_OUTPUT_WIDTH_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_WIDTH_CUR_VALUE, num_parameter_elements);
        if(status != PVMFSuccess)
        {
            LOG_ERR((0,"AndroidCameraInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",status));
            return status;
        }

        parameters[0].value.uint32_value = mFrameWidth;
    }
    else if(pv_mime_strcmp(identifier, VIDEO_OUTPUT_HEIGHT_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_HEIGHT_CUR_VALUE, num_parameter_elements);
        if(status != PVMFSuccess)
        {
            LOG_ERR((0,"AndroidCameraInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",status));
            return status;
        }
        
        parameters[0].value.uint32_value = mFrameHeight;
    }
    else if(pv_mime_strcmp(identifier, VIDEO_OUTPUT_FRAME_RATE_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, VIDEO_OUTPUT_FRAME_RATE_CUR_VALUE, num_parameter_elements);
        if(status != PVMFSuccess)
        {
            LOG_ERR((0,"AndroidCameraInput::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",status));
            return status;
        }

        parameters[0].value.float_value = mFrameRate;
    }
    else if(pv_mime_strcmp(identifier, OUTPUT_TIMESCALE_CUR_QUERY) == 0)
    {
        num_parameter_elements = 1;
        status = AllocateKvp(parameters, OUTPUT_TIMESCALE_CUR_VALUE, num_parameter_elements);
        if(status != PVMFSuccess)
        {
            LOG_ERR((0,"PVMFVideoEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d",status));
            return status;
        }
    
        // XXX is it okay to hardcode this as the timescale?    
        parameters[0].value.uint32_value = 1000;
    }

    return status;}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus AndroidCameraInput::releaseParameters(PvmiMIOSession session, 
                        PvmiKvp* parameters, 
                        int num_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(num_elements);

    if(parameters)
    {
        iAlloc.deallocate((OsclAny*)parameters);
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::createContext(PvmiMIOSession session, PvmiCapabilityContext& context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::setContextParameters(PvmiMIOSession session,
                                                          PvmiCapabilityContext& context,
                                                          PvmiKvp* parameters, int num_parameter_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_parameter_elements);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::DeleteContext(PvmiMIOSession session, PvmiCapabilityContext& context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(context);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void AndroidCameraInput::setParametersSync(PvmiMIOSession session, PvmiKvp* parameters, 
                                                       int num_elements, PvmiKvp*& ret_kvp)
{
    OSCL_UNUSED_ARG(session);
    PVMFStatus status = PVMFSuccess;
    ret_kvp = NULL;

    for(int32 i = 0; i < num_elements; i++)
    {
        status = VerifyAndSetParameter(&(parameters[i]), true);
        if(status != PVMFSuccess)
        {
            LOG_ERR((0,"AndroidCameraInput::setParametersSync: Error - VerifiyAndSetParameter failed on parameter #%d", i));
            ret_kvp = &(parameters[i]);
            OSCL_LEAVE(OsclErrArgument);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId AndroidCameraInput::setParametersAsync(PvmiMIOSession session, 
                                                                 PvmiKvp* parameters, 
                                                                 int num_elements, 
                                                                 PvmiKvp*& ret_kvp,
                                                                 OsclAny* context)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_elements);
    OSCL_UNUSED_ARG(ret_kvp);
    OSCL_UNUSED_ARG(context);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 AndroidCameraInput::getCapabilityMetric (PvmiMIOSession session)
{
    OSCL_UNUSED_ARG(session);
    return 0;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus AndroidCameraInput::verifyParametersSync(PvmiMIOSession session, 
                                                                PvmiKvp* parameters, int num_elements)
{
    OSCL_UNUSED_ARG(session);
    OSCL_UNUSED_ARG(parameters);
    OSCL_UNUSED_ARG(num_elements);
    return PVMFErrNotSupported;
}

////////////////////////////////////////////////////////////////////////////
//                            Private methods
////////////////////////////////////////////////////////////////////////////
AndroidCameraInput::AndroidCameraInput()
 : OsclTimerObject(OsclActiveObject::EPriorityNominal, "AndroidCameraInput"),
   iCmdIdCounter(0),
   iPeer(NULL),
   camera_output_buf(NULL),
   iThreadLoggedOn(false),
   iCameraFd(-1),
   mSurfaceWidth(-1),
   mSurfaceHeight(-1),
   mFrameWidth(DEFAULT_FRAME_WIDTH),
   mFrameHeight(DEFAULT_FRAME_HEIGHT),
   mFrameRate(DEFAULT_FRAME_RATE),
   iDataEventCounter(0),
   iTimeStamp(0),
   iMediaBufferMemPool(NULL),
   iLogger(NULL),
   iState(STATE_IDLE),
   iStartTickCount(0)
{
    iColorConverter = ColorConvert16::NewL();
    //iColorConverter = ColorConvert16::NewL();
    iYuv422toYuv420 = CCYUV422toYUV420::New();

    FrameSizeChanged();
}

void AndroidCameraInput::SetFrameSize(int w, int h)
{
    if (iState != STATE_IDLE)
        return;

    mFrameWidth = w;
    mFrameHeight = h;
    FrameSizeChanged();
}

void AndroidCameraInput::SetFrameRate(int fps)
{
    if (iState != STATE_IDLE)
        return;

    mFrameRate = (float)fps;
}

void AndroidCameraInput::FrameSizeChanged()
{
    if (iState != STATE_IDLE)
        return;

#if (ANDROID_VIDEO_FORMAT == PVMF_RGB16) || (ANDROID_VIDEO_FORMAT == PVMF_YUV422)
    iFrameSize = (uint32)(mFrameHeight * mFrameWidth * 2);
#elif ANDROID_VIDEO_FORMAT == PVMF_YUV420
#error "This doesn't work"
    iFrameSize = (uint32)(mFrameHeight * mFrameWidth * 3 / 2);
#else
#error Undefined video format
#endif

    // Reinitialize the preview surface in case it was set up before now
    if (mSurface != NULL) {
        SetPreviewSurface(mSurface);
    }
}

////////////////////////////////////////////////////////////////////////////
void AndroidCameraInput::Run()
{
    if(!iCmdQueue.empty())
    {
        AndroidCameraInputCmd cmd = iCmdQueue[0];
        iCmdQueue.erase(iCmdQueue.begin());
        
        switch(cmd.iType)
        {

        case CMD_INIT:
            DoRequestCompleted(cmd, DoInit());
            break;

        case CMD_START:
            DoRequestCompleted(cmd, DoStart());
            break;

        case CMD_PAUSE:
            DoRequestCompleted(cmd, DoPause());
            break;

        case CMD_FLUSH:
            DoRequestCompleted(cmd, DoFlush());
            break;

        case CMD_RESET:
            DoRequestCompleted(cmd, DoReset());
            break;

        case CMD_STOP:
            DoRequestCompleted(cmd, DoStop());
            break;

        case DATA_EVENT:
            DoRead();
            break;

        case CMD_QUERY_UUID:
        case CMD_QUERY_INTERFACE:
            DoRequestCompleted(cmd, PVMFSuccess);
            break;

        case CMD_CANCEL_ALL_COMMANDS:
        case CMD_CANCEL_COMMAND:
            DoRequestCompleted(cmd, PVMFFailure);
            break;
    
        default:
            break;
        }
    }
    
    if(!iCmdQueue.empty())
    {
        // Run again if there are more things to process
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFCommandId AndroidCameraInput::AddCmdToQueue(AndroidCameraInputCmdType aType, 
                                              const OsclAny* aContext, OsclAny* aData1)
{
    if(aType == DATA_EVENT)
        OSCL_LEAVE(OsclErrArgument);

    AndroidCameraInputCmd cmd;
    cmd.iType = aType;
    cmd.iContext = OSCL_STATIC_CAST(OsclAny*, aContext);
    cmd.iData1 = aData1;
    cmd.iId = iCmdIdCounter;
    ++iCmdIdCounter;
    iCmdQueue.push_back(cmd);
    RunIfNotReady();
    return cmd.iId;
}

////////////////////////////////////////////////////////////////////////////
void AndroidCameraInput::AddDataEventToQueue(uint32 aMicroSecondsToEvent)
{
    AndroidCameraInputCmd cmd;
    cmd.iType = DATA_EVENT;
    iCmdQueue.push_back(cmd);
    RunIfNotReady(aMicroSecondsToEvent);
}

////////////////////////////////////////////////////////////////////////////
void AndroidCameraInput::DoRequestCompleted(const AndroidCameraInputCmd& aCmd, PVMFStatus aStatus, OsclAny* aEventData)
{
    PVMFCmdResp response(aCmd.iId, aCmd.iContext, aStatus, aEventData);

    for(uint32 i = 0; i < iObservers.size(); i++)
        iObservers[i]->RequestCompleted(response);
}

#ifndef HAVE_MALLOC_H
void*   memalign(size_t  alignment, size_t  bytesize) {
    return malloc(bytesize);
}
#endif


////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::DoInit()
{
    //calculate time for a buffer to fill
    iMilliSecondsPerDataEvent = (int32)(1000 / mFrameRate);
    iMicroSecondsPerDataEvent = (int32)(1000000 / mFrameRate);

    iDataEventCounter = 0;

    // Create memory pool for the media data, using the maximum frame size found earlier
    int32 err = 0;
    OSCL_TRY(err,
        if(iMediaBufferMemPool)
        {
            OSCL_TEMPLATED_DELETE(iMediaBufferMemPool, OsclMemPoolFixedChunkAllocator, OsclMemPoolFixedChunkAllocator);
            iMediaBufferMemPool = NULL;
        }
        iMediaBufferMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator, (4));
        if(!iMediaBufferMemPool)
            OSCL_LEAVE(OsclErrNoMemory);

    );
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);
    
    uint32 yuv420FrameSize = (uint32)(mFrameHeight * mFrameWidth * 3 / 2);
    OSCL_TRY(err,
	if(camera_output_buf)
	{
		free(camera_output_buf);
		camera_output_buf = NULL;
	}
	camera_output_buf = (uint8*)memalign(4096, iFrameSize);

        if(!camera_output_buf)
            OSCL_LEAVE(OsclErrNoMemory);
    );
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);
    
    if(!iYuv422toYuv420->Init(mFrameWidth, mFrameHeight, mFrameWidth, mFrameWidth, mFrameHeight, mFrameWidth ) )
	    return PVMFFailure;

    // The first allocate call will set the chunk size of the memory pool. Use the max
    // frame size calculated earlier to set the chunk size.  The allocated data will be
    // deallocated automatically as tmpPtr goes out of scope.
    OsclAny* tmpPtr = iMediaBufferMemPool->allocate(yuv420FrameSize);
    iMediaBufferMemPool->deallocate(tmpPtr);
    
    iState = STATE_INITIALIZED;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::DoStart()
{
    iState = STATE_STARTED;

//     android::app_cap_params params;
// 
//     params.type = 1; // PREVIEW
//     params.source_width = 1280;
//     params.source_height = 960;
//     params.pixel_left = 0;
//     params.pixel_top = 0;
//     params.output_width = mFrameWidth;
//     params.output_height = mFrameHeight;
// 
//     // XXX YUV422_PACKED = 0, PLANAR = 1, RGB16 = 2
// #if ANDROID_VIDEO_FORMAT == PVMF_RGB16
//     params.data_format = 2;
// #elif ANDROID_VIDEO_FORMAT == PVMF_YUV422
//     params.data_format = 1;
// #else
// #error Undefined video format
// #endif
// 
//     iCameraFd = android::camera_open_driver();
//     android::camera_set_capture_params(iCameraFd, &params);
//     
//     AddDataEventToQueue(0);
//     return PVMFSuccess;

    // TODO: Reimplement this class with the new camera API
    return PVMFFailure;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::DoPause()
{
    iState = STATE_PAUSED;
    return PVMFSuccess;
}

PVMFStatus AndroidCameraInput::DoReset()
{
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::DoFlush()
{
    // This method should stop capturing media data but continue to send captured
    // media data that is already in buffer and then go to stopped state.
    // However, in this case of file input we do not have such a buffer for 
    // captured data, so this behaves the same way as stop.
    return DoStop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::DoStop()
{
    iDataEventCounter = 0;
    iState = STATE_STOPPED;
    if (iCameraFd >= 0) {
        //android::camera_close_driver(iCameraFd);
        iCameraFd = -1;
    }
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::DoRead()
{
    // Just copy from PVMFFileInputNode::HandleEventPortActivity.  The only difference
    // is that data buffer is allocated by calling iMediaBufferMemPool->allocate(bytesToRead)
    // and there's no need to wrap it in a PVMFSharedMediaDataPtr.  Also, you'll need to
    // keep track of the data pointer and the write command id received from peer->writeAsync
    // and put it in the iSentMediaData queue

    if(iState != STATE_STARTED)
    {
        return PVMFSuccess;
    }

    uint32 timeStamp = 0;
    // allocate bytesToRead number of bytes
    uint8* data = NULL;
    uint32 writeAsyncID = 0;

    uint32 bytesToRead = iFrameSize;
    if(iDataEventCounter == 0)
    {
       iStartTickCount = OsclTickCount::TickCount();
    }
    else
    {	    
    	timeStamp = OsclTickCount::TicksToMsec(OsclTickCount::TickCount()-iStartTickCount);
    }	
    ++iDataEventCounter;
    
    // Create new media data buffer
    uint32 yuv420FrameSize = (uint32)(mFrameHeight * mFrameWidth * 3 / 2);
    int32 error = 0;
    OSCL_TRY(error, 
                data = (uint8*)iMediaBufferMemPool->allocate(yuv420FrameSize);
            );

    if (error)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
            (0,"AndroidCameraInput::No buffer available, wait till next data event"));

        AddDataEventToQueue(iMicroSecondsPerDataEvent);
        return PVMFSuccess;
    }

    //android::camera_capture_image(iCameraFd, (unsigned char *)camera_output_buf);

    //do yuv422 interleaved to YUV420 planar conversion
    //    Swap bytes Yi Cbi Yi+1 Cri  
    if(!iYuv422toYuv420->Convert(camera_output_buf, data))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
            (0,"AndroidCameraInput::CC failure"));

        return PVMFFailure;
    }

    // send data to Peer & store the id
    PvmiMediaXferHeader data_hdr;
    data_hdr.seq_num=iDataEventCounter-1;
    data_hdr.timestamp=timeStamp;
    data_hdr.flags=0;
    data_hdr.duration=0;
    data_hdr.stream_id=0;
    writeAsyncID = iPeer->writeAsync(0, 0, data, yuv420FrameSize, data_hdr);

    // Save the id and data pointer on iSentMediaData queue for writeComplete call
    AndroidCameraInputMediaData sentData;
    sentData.iId = writeAsyncID;
    sentData.iData = data;
    iSentMediaData.push_back(sentData);

    // Queue the next data event
    AddDataEventToQueue(iMicroSecondsPerDataEvent);

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::AllocateKvp(PvmiKvp*& aKvp, PvmiKeyType aKey, int32 aNumParams)
{
    LOG_STACK_TRACE((0, "AndroidCameraInput::AllocateKvp"));
    uint8* buf = NULL;
    uint32 keyLen = oscl_strlen(aKey) + 1;
    int32 err = 0;

    OSCL_TRY(err, 
        buf = (uint8*)iAlloc.allocate(aNumParams * (sizeof(PvmiKvp) + keyLen));
        if(!buf)
            OSCL_LEAVE(OsclErrNoMemory);
    );
    OSCL_FIRST_CATCH_ANY(err, 
        LOG_ERR((0,"AndroidCameraInput::AllocateKvp: Error - kvp allocation failed"));
        return PVMFErrNoMemory;
    );
    
    int32 i = 0;
    PvmiKvp* curKvp = aKvp = new (buf) PvmiKvp;
    buf += sizeof(PvmiKvp);
    for(i = 1; i < aNumParams; i++)
    {
        curKvp += i;
        curKvp = new (buf) PvmiKvp;
        buf += sizeof(PvmiKvp);
    }

    for(i = 0; i < aNumParams; i++)
    {
        aKvp[i].key = (char*)buf;
        oscl_strncpy(aKvp[i].key, aKey, keyLen);
        buf += keyLen;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus AndroidCameraInput::VerifyAndSetParameter(PvmiKvp* aKvp, bool aSetParam)
{
    LOG_STACK_TRACE((0,"AndroidCameraInput::VerifyAndSetParameter: aKvp=0x%x, aSetParam=%d", aKvp, aSetParam));

    if(!aKvp)
    {
        LOG_ERR((0, "AndroidCameraInput::VerifyAndSetParameter: Error - Invalid key-value pair"));
        return PVMFFailure;
    }

    if(pv_mime_strcmp(aKvp->key, OUTPUT_FORMATS_VALTYPE) == 0)
    {
        //if(aKvp->value.uint32_value == ANDROID_VIDEO_FORMAT)
        if(aKvp->value.uint32_value == PVMF_YUV420)
        {
            return PVMFSuccess;
        }
        else
        {
            LOG_ERR((0,"AndroidCameraInput::VerifyAndSetParameter: Error - Unsupported format %d",
                aKvp->value.uint32_value));
            return PVMFFailure;
        }
    }

    LOG_ERR((0,"AndroidCameraInput::VerifyAndSetParameter: Error - Unsupported parameter"));
    return PVMFFailure;
}

void AndroidCameraInput::SetPreviewSurface(const sp<Surface>& surface)
{
    mSurface = surface;

    // currently we need to locl the surface to access its size
    // note that technically the Surface's size is only valid while
    // it's locked. So I soon as we unlock it, the data in obsolete.
    
    Surface::SurfaceInfo info;
    surface->lock(&info);
    
    // Cache the surface dimensions so we can detect if it changes later.
    mSurfaceWidth = info.w;
    mSurfaceHeight = info.h;

    surface->unlockAndPost();

    //this Init could fail, check the return code!!!
    //iColorConverter->Init(mFrameWidth,mFrameHeight,mFrameWidth,info.w,info.h,info.w,CCROTATE_NONE);
    //iColorConverter->SetMode(CCSUPPORT_ROTATION);
    if(!iColorConverter->Init(mFrameWidth,mFrameHeight,mFrameWidth,mFrameWidth,mFrameHeight,info.w))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
            (0,"AndroidCameraInput::SetPreviewSurface: CC Init Error."));
	OSCL_ASSERT(false);
    }
}

