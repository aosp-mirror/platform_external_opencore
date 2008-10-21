/* authordriver.h
 * 
 * The glue between the Android MediaRecorder and PVAuthorInterface
 */

#ifndef _AUTHORDRIVER_PRIV_H
#define _AUTHORDRIVER_PRIV_H

#include <utils.h>

#include <media/mediarecorder.h>

#include "oscl_scheduler.h"
#include "oscl_scheduler_ao.h"
#include "oscl_exception.h"
#include "pvlogger.h"
#include "pvlogger_file_appender.h"
#include "pvlogger_stderr_appender.h"
#include "pvlogger_time_and_id_layout.h"
#include "pvauthorenginefactory.h"
#include "pvauthorengineinterface.h"
#include "pv_engine_observer.h"
#include "pvmf_errorinfomessage_extension.h"
#include "oscl_mem.h"
#include "oscl_mem_audit.h"
#include "oscl_error.h"
#include "oscl_error_panic.h"
#include "oscl_utf8conv.h"
#include "oscl_string_utils.h"
#include "android_camera_input.h"
#include "android_audio_input.h"
#include "pvmf_media_input_node_factory.h"
#include "pvmf_node_interface.h"
#include "pvmp4h263encextension.h"
#include "pvmp4ffcn_clipconfig.h"
#include "pvmfamrencnode_extension.h"

namespace android {

template<class DestructClass>
class LogAppenderDestructDealloc : public OsclDestructDealloc 
{
public:
    virtual void destruct_and_dealloc(OsclAny *ptr) 
    { 
        delete((DestructClass*)ptr); 
    }
};

// Commands that MediaAuthor sends to the AuthorDriver.
//
enum author_command_type {
    AUTHOR_INIT = 1,
    AUTHOR_SET_VIDEO_SOURCE,
    AUTHOR_SET_AUDIO_SOURCE,
    AUTHOR_SET_OUTPUT_FORMAT,
    AUTHOR_SET_VIDEO_ENCODER,
    AUTHOR_SET_AUDIO_ENCODER,
    AUTHOR_SET_VIDEO_SIZE,
    AUTHOR_SET_VIDEO_FRAME_RATE,
    AUTHOR_SET_PREVIEW_SURFACE,
    AUTHOR_SET_OUTPUT_FILE,
    AUTHOR_PREPARE,
    AUTHOR_START,
    AUTHOR_STOP,
    AUTHOR_RESET,
    AUTHOR_QUIT = 100
};

struct author_command
{
    author_command(author_command_type which) {
        this->which = which;
    }

    author_command_type which;
    media_completion_f comp;
    void *cookie;
};

struct set_audio_source_command : author_command
{
    set_audio_source_command() : author_command(AUTHOR_SET_AUDIO_SOURCE) {};
    audio_source                 as;
};

struct set_video_source_command : author_command
{
    set_video_source_command() : author_command(AUTHOR_SET_VIDEO_SOURCE) {};
    video_source                 vs;
};

struct set_output_format_command : author_command
{
    set_output_format_command() : author_command(AUTHOR_SET_OUTPUT_FORMAT) {};
    output_format                 of;
};

struct set_audio_encoder_command : author_command
{
    set_audio_encoder_command() : author_command(AUTHOR_SET_AUDIO_ENCODER) {};
    audio_encoder                 ae;
};

struct set_video_encoder_command : author_command
{
    set_video_encoder_command() : author_command(AUTHOR_SET_VIDEO_ENCODER) {};
    video_encoder                 ve;
};

struct set_output_file_command : author_command
{
    set_output_file_command() : author_command(AUTHOR_SET_OUTPUT_FILE) {};
    char                        *path;
};

struct set_video_size_command : author_command
{
    set_video_size_command() : author_command(AUTHOR_SET_VIDEO_SIZE) {};
    int                        width;
    int                        height;
};

struct set_video_frame_rate_command : author_command
{
    set_video_frame_rate_command() : author_command(AUTHOR_SET_VIDEO_FRAME_RATE) {};
    int                              rate;
};

struct set_preview_surface_command : author_command
{
    set_preview_surface_command() : author_command(AUTHOR_SET_PREVIEW_SURFACE) {};
    sp<Surface>                      surface;
};

class AuthorDriver :
    public OsclActiveObject,
    public PVCommandStatusObserver,
    public PVInformationalEventObserver,
    public PVErrorEventObserver
{
public:
    AuthorDriver();
    ~AuthorDriver();

    author_command *dequeueCommand();
    status_t enqueueCommand(author_command *ec, media_completion_f comp, void *cookie);

    // Dequeues a command from MediaRecorder and gives it to PVAuthorEngine.
    void Run();

    // Handlers for the various commands we can accept.
    void commandFailed(author_command *ac);
    void handleInit(author_command *ac);
    void handleSetAudioSource(set_audio_source_command *ac);
    void handleSetVideoSource(set_video_source_command *ac);
    void handleSetOutputFormat(set_output_format_command *ac);
    void handleSetAudioEncoder(set_audio_encoder_command *ac);
    void handleSetVideoEncoder(set_video_encoder_command *ac);
    void handleSetVideoSize(set_video_size_command *ac);
    void handleSetVideoFrameRate(set_video_frame_rate_command *ac);
    void handleSetPreviewSurface(set_preview_surface_command *ac);
    void handleSetOutputFile(set_output_file_command *ac);
    void handlePrepare(author_command *ac);
    void handleStart(author_command *ac);
    void handleStop(author_command *ac);
    void handleReset(author_command *ac);
    void handleQuit(author_command *ac);

    void endOfData();

    void CommandCompleted(const PVCmdResponse& aResponse);
    void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
    void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);
    
    status_t getMaxAmplitude(int *max);

private:
    // Finish up a non-async command in such a way that
    // the event loop will keep running.
    void FinishNonAsyncCommand(author_command *ec);

    // Starts the PV scheduler thread.
    static int startAuthorThread(void *cookie);
    int authorThread();

    static void media_track_added(status_t s, void *cookie);

    // Callback for synchronous commands.
    static void syncCompletion(status_t s, void *cookie);

    PVAuthorEngineInterface    *mAuthor;

    PvmiMIOControl            *mVideoInputMIO;
    PVMFNodeInterface        *mVideoNode;
    PvmiMIOControl            *mAudioInputMIO;
    PVMFNodeInterface        *mAudioNode;

    void                    *mSelectedComposer;
    PVInterface                *mComposerConfig;
    PVInterface                *mVideoEncoderConfig;
    PVInterface                *mAudioEncoderConfig;

    int                     mVideoWidth;
    int                     mVideoHeight;
    int                     mVideoFrameRate;
    //int                     mVideoBitRate;
    video_encoder           mVideoEncoder;

    //int                     mAudioBitRate;
    audio_encoder           mAudioEncoder;

    // Semaphore used for synchronous commands.
    OsclSemaphore           *mSyncSem;
    // Status cached by syncCompletion for synchronous commands.
    status_t                mSyncStatus;

    // Command queue and its lock.
    List<author_command *>  mCommandQueue;
    Mutex                   mQueueLock;
};

class AuthorDriverWrapper
{
public:
    AuthorDriverWrapper();
    ~AuthorDriverWrapper();
    status_t enqueueCommand(author_command *ec, media_completion_f comp, void *cookie);
    status_t getMaxAmplitude(int *max);

private:
    AuthorDriver    *mAuthorDriver;
};

}; // namespace android

#endif // _AUTHORDRIVER_PRIV_H

