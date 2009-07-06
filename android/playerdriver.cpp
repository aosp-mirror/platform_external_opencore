/* playerdriver.cpp
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "PlayerDriver"
#include <utils/Log.h>

#include <sys/prctl.h>
#include <sys/resource.h>
#include <media/mediaplayer.h>
#include <media/thread_init.h>
#include <utils/threads.h>
#include <utils/List.h>
#include <cutils/properties.h>
#include <binder/Parcel.h>

#include <media/MediaPlayerInterface.h>

#include "playerdriver.h"
#include <media/PVPlayer.h>

#include "oscl_base.h"
#include "oscl_exception.h"
#include "oscl_scheduler.h"
#include "oscl_scheduler_ao.h"
#include "oscl_exception.h"
#include "oscl_mem_basic_functions.h"  // oscl_memcpy
#include "oscl_mem.h"
#include "oscl_mem_audit.h"
#include "oscl_error.h"
#include "oscl_utf8conv.h"
#include "oscl_string_utils.h"

#include "media_clock_converter.h"

#include "pv_player_factory.h"
#include "pv_player_interface.h"
#include "pv_engine_observer.h"
#include "pvmi_media_io_fileoutput.h"
#include "pv_player_datasourceurl.h"
#include "pv_player_datasinkfilename.h"
#include "pv_player_datasinkpvmfnode.h"
#include "pvmf_errorinfomessage_extension.h"
#include "pvmf_duration_infomessage.h"
#include "android_surface_output.h"
#include "android_audio_output.h"
#include "android_audio_stream.h"
#include "pv_media_output_node_factory.h"
#include "pvmf_format_type.h"  // for PVMFFormatType
#include "pvmf_node_interface.h"
#include "pvmf_source_context_data.h"
#include "pvmf_download_data_source.h"
#include "OMX_Core.h"
#include "pv_omxcore.h"

// color converter
#include "cczoomrotation16.h"

//for KMJ DRM Plugin
#include "pvmf_local_data_source.h"
#include "pvmf_recognizer_registry.h"
#include "pvoma1_kmj_recognizer.h"
#include "pvmf_cpmplugin_kmj_oma1.h"


using namespace android;

# ifndef PAGESIZE
#  define PAGESIZE              4096
# endif

// library and function name to retrieve device-specific MIOs
static const char* MIO_LIBRARY_NAME = "libopencorehw.so";
static const char* VIDEO_MIO_FACTORY_NAME = "createVideoMio";
typedef AndroidSurfaceOutput* (*VideoMioFactory)();

namespace {


// For the event's buffer format is:
//                     1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                        buffering percent                      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// @param buffer Pointer to the start of the buffering status data.
// @param size Of the buffer
// @param[out] percentage On return contains the amout of buffering.
//                        The value is in [0,100]
// @return true if a valid buffering update was found. false otherwise.
bool GetBufferingPercentage(const void *buffer,
                            const size_t size,
                            int *percentage)
{
    if (buffer == NULL) {
        LOGE("Invalid buffer: NULL");
        return false;
    }
    if (sizeof(int) != size)
    {
        LOGE("Invalid percentage buffer size %d (expected %d)", size, sizeof(int));
        return false;
    }
    // TODO: The PVEvent class should expose a memcopy method
    // that does bound checking instead of having clients reaching
    // for its internal buffer.
    oscl_memcpy(percentage, buffer, sizeof(int));

    // Clamp the value and complain loudly.
    if (*percentage < 0 || *percentage > 100)
    {
        LOGE("Invalid percentage value %d", *percentage);
        return false;
    }
    return true;
}

// Macro used in a switch statement to convert a PlayerCommand code into its
// string representation.
#ifdef CONSIDER_CODE
#error "CONSIDER_CODE already defined!!"
#endif
#define CONSIDER_CODE(val) case ::PlayerCommand::val: return #val

// Convert a command code into a string for logging purposes.
// @param code Of the command.
// @return a string representation of the command type.
const char *PlayerCommandCodeToString(PlayerCommand::Code code) {
    switch (code) {
        CONSIDER_CODE(PLAYER_QUIT);
        CONSIDER_CODE(PLAYER_SETUP);
        CONSIDER_CODE(PLAYER_SET_DATA_SOURCE);
        CONSIDER_CODE(PLAYER_SET_VIDEO_SURFACE);
        CONSIDER_CODE(PLAYER_SET_AUDIO_SINK);
        CONSIDER_CODE(PLAYER_INIT);
        CONSIDER_CODE(PLAYER_PREPARE);
        CONSIDER_CODE(PLAYER_START);
        CONSIDER_CODE(PLAYER_STOP);
        CONSIDER_CODE(PLAYER_PAUSE);
        CONSIDER_CODE(PLAYER_RESET);
        CONSIDER_CODE(PLAYER_SET_LOOP);
        CONSIDER_CODE(PLAYER_SEEK);
        CONSIDER_CODE(PLAYER_GET_POSITION);
        CONSIDER_CODE(PLAYER_GET_DURATION);
        CONSIDER_CODE(PLAYER_GET_STATUS);
        CONSIDER_CODE(PLAYER_REMOVE_DATA_SOURCE);
        CONSIDER_CODE(PLAYER_CANCEL_ALL_COMMANDS);
        default: return "UNKNOWN PlayerCommand code";
    }
}
#undef CONSIDER_CODE

// Map a PV status code to a message type (error/info/nop)
// @param status PacketVideo status code as defined in pvmf_return_codes.h
// @return the corresponding android message type. MEDIA_NOP is used as a default.
::android::media_event_type MapStatusToEventType(const PVMFStatus status) {
    if (status <= PVMFErrFirst) {
        return ::android::MEDIA_ERROR;
    } else if (status >= PVMFInfoFirst) {
        return ::android::MEDIA_INFO;
    } else {
        return ::android::MEDIA_NOP;
    }
}

// Map a PV status to an error/info code.
// @param status PacketVideo status code as defined in pvmf_return_codes.h
// @return the corresponding android error/info code.
int MapStatusToEventCode(const PVMFStatus status) {
    switch (status) {
        case PVMFErrContentInvalidForProgressivePlayback:
            return ::android::MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK;

        default:
            // Takes advantage that both error and info codes are mapped to the
            // same value.
            assert(::android::MEDIA_ERROR_UNKNOWN == ::android::MEDIA_INFO_UNKNOWN);
            return ::android::MEDIA_ERROR_UNKNOWN;
    }
}

}  // anonymous namespace

class PlayerDriver :
        public OsclActiveObject,
        public PVCommandStatusObserver,
        public PVInformationalEventObserver,
        public PVErrorEventObserver
{
  public:
    PlayerDriver(PVPlayer* pvPlayer);
    ~PlayerDriver();

    PlayerCommand* dequeueCommand();
    status_t enqueueCommand(PlayerCommand* code);

    // Dequeues a code from MediaPlayer and gives it to PVPlayer.
    void Run();

    // Handlers for the various commands we can accept.
    void commandFailed(PlayerCommand* command);
    void handleSetup(PlayerSetup* command);
    void handleSetDataSource(PlayerSetDataSource* command);
    void handleSetVideoSurface(PlayerSetVideoSurface* command);
    void handleSetAudioSink(PlayerSetAudioSink* command);
    void handleInit(PlayerInit* command);
    void handlePrepare(PlayerPrepare* command);
    void handleStart(PlayerStart* command);
    void handleStop(PlayerStop* command);
    void handlePause(PlayerPause* command);
    void handleSeek(PlayerSeek* command);
    void handleCancelAllCommands(PlayerCancelAllCommands* command);
    void handleRemoveDataSource(PlayerRemoveDataSource* command);
    void handleReset(PlayerReset* command);
    void handleQuit(PlayerQuit* command);
    void handleGetPosition(PlayerGetPosition* command);
    void handleGetDuration(PlayerGetDuration* command);
    void handleGetStatus(PlayerGetStatus* command);

    void endOfData();

    void CommandCompleted(const PVCmdResponse& aResponse);
    void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
    void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

  private:
    // Finish up a non-async code in such a way that
    // the event loop will keep running.
    void FinishSyncCommand(PlayerCommand* command);

    void handleGetDurationComplete(PlayerGetDuration* cmd);

    int setupHttpStreamPre();
    int setupHttpStreamPost();

    // Starts the PV scheduler thread.
    static int startPlayerThread(void *cookie);
    int playerThread();

    // Callback for synchronous commands.
    static void syncCompletion(status_t s, void *cookie, bool cancelled);

    PVPlayer                *mPvPlayer;
    PVPlayerInterface       *mPlayer;
    PVPlayerDataSourceURL   *mDataSource;

    PVPlayerDataSink        *mAudioSink;
    PVMFNodeInterface       *mAudioNode;
    AndroidAudioMIO         *mAudioOutputMIO;

    PVPlayerDataSink        *mVideoSink;
    PVMFNodeInterface       *mVideoNode;
    AndroidSurfaceOutput    *mVideoOutputMIO;

    PvmiCapabilityAndConfig *mPlayerCapConfig;

    OSCL_wHeapString<OsclMemAllocator> mDownloadFilename;
    OSCL_HeapString<OsclMemAllocator> mDownloadProxy;
    OSCL_wHeapString<OsclMemAllocator> mDownloadConfigFilename;
    PVMFSourceContextData   *mDownloadContextData;

    PVPMetadataList mMetaKeyList;
    Oscl_Vector<PvmiKvp,OsclMemAllocator> mMetaValueList;
    int mNumMetaValues;

    // Semaphore used for synchronous commands.
    OsclSemaphore           *mSyncSem;
    // Status cached by syncCompletion for synchronous commands.
    status_t                mSyncStatus;

    // Command queue and its lock.
    List<PlayerCommand*>    mCommandQueue;
    Mutex                   mQueueLock;

    bool                    mIsLooping;
    bool                    mDoLoop;
    bool                    mDataReadyReceived;
    bool                    mPrepareDone;
    bool                    mEndOfData;
    int                     mRecentSeek;
    bool                    mSeekComp;
    bool                    mSeekPending;

    bool                    mEmulation;
    void*                   mLibHandle;

    // video display surface
    android::sp<android::ISurface> mSurface;
};

PlayerDriver::PlayerDriver(PVPlayer* pvPlayer) :
        OsclActiveObject(OsclActiveObject::EPriorityNominal, "PVPlayerPlayer"),
        mPvPlayer(pvPlayer),
        mIsLooping(false),
        mDoLoop(false),
        mDataReadyReceived(false),
        mPrepareDone(false),
        mEndOfData(false),
        mRecentSeek(0),
        mSeekComp(true),
        mSeekPending(false),
        mEmulation(false)
{
    LOGV("constructor");
    mSyncSem = new OsclSemaphore();
    mSyncSem->Create();

    mDataSource = NULL;
    mAudioSink = NULL;
    mAudioNode = NULL;
    mAudioOutputMIO = NULL;
    mVideoSink = NULL;
    mVideoNode = NULL;
    mVideoOutputMIO = NULL;
    mSurface = NULL;

    mPlayerCapConfig = NULL;
    mDownloadContextData = NULL;

    // running in emulation?
    mLibHandle = NULL;
    char value[PROPERTY_VALUE_MAX];
    if (property_get("ro.kernel.qemu", value, 0)) {
        mEmulation = true;
        LOGV("Emulation mode - using software codecs");
    } else {
        // attempt to open h/w specific library
        mLibHandle = ::dlopen(MIO_LIBRARY_NAME, RTLD_NOW);
        if (mLibHandle != NULL) {
            LOGV("OpenCore hardware module loaded");
        } else {
            LOGV("OpenCore hardware module not found");
        }
    }

    // start player thread
    LOGV("start player thread");
    createThreadEtc(PlayerDriver::startPlayerThread, this, "PV player");

    // mSyncSem will be signaled when the scheduler has started
    mSyncSem->Wait();
}

PlayerDriver::~PlayerDriver()
{
    LOGV("destructor");
    if (mLibHandle != NULL) {
        ::dlclose(mLibHandle);
    }
}

PlayerCommand* PlayerDriver::dequeueCommand()
{
    PlayerCommand* command;

    mQueueLock.lock();

    // XXX should we assert here?
    if (mCommandQueue.empty()) {
        PendForExec();
        mQueueLock.unlock();
        return NULL;
    }

    command = *(--mCommandQueue.end());
    mCommandQueue.erase(--mCommandQueue.end());
    if (mCommandQueue.size() > 0 )
    {
        RunIfNotReady();
    }
    else
    {
        PendForExec();
    }
    mQueueLock.unlock();

    return command;
}

status_t PlayerDriver::enqueueCommand(PlayerCommand* command)
{
    if (mPlayer == NULL) {
        // Only commands which can come in this use-case is PLAYER_SETUP and PLAYER_QUIT
        // The calling function should take responsibility to delete the command and cleanup
        return NO_INIT;
    }

    OsclSemaphore *syncsemcopy = NULL;

    // If the user didn't specify a completion callback, we
    // are running in synchronous mode.
    if (!command->hasCallback()) {
        command->set(PlayerDriver::syncCompletion, this);
        // make a copy of this semaphore for special handling of the PLAYER_QUIT code
        syncsemcopy = mSyncSem;
    }

    // Add the code to the queue.
    mQueueLock.lock();
    mCommandQueue.push_front(command);

    // save code, since command will be deleted by the standard completion function
    int code = command->code();

    // AO needs to be scheduled only if this is the first cmd after queue was empty
    if (mCommandQueue.size() == 1)
    {
        PendComplete(OSCL_REQUEST_ERR_NONE);
    }
    mQueueLock.unlock();

    // If we are in synchronous mode, wait for completion.
    if (syncsemcopy) {
        syncsemcopy->Wait();
        if (code == PlayerCommand::PLAYER_QUIT) {
            syncsemcopy->Close();
            delete syncsemcopy;
            return 0;
        }
        return mSyncStatus;
    }

    return OK;
}

void PlayerDriver::FinishSyncCommand(PlayerCommand* command)
{
    command->complete(0, false);
    delete command;
}

// The OSCL scheduler calls this when we get to run (this should happen only
// when a code has been enqueued for us).
void PlayerDriver::Run()
{
    if (mDoLoop) {
        mEndOfData = false;
        PVPPlaybackPosition begin, end;
        begin.iIndeterminate = false;
        begin.iPosUnit = PVPPBPOSUNIT_SEC;
        begin.iPosValue.sec_value = 0;
        begin.iMode = PVPPBPOS_MODE_NOW;
        end.iIndeterminate = true;
        mPlayer->SetPlaybackRange(begin, end, false, NULL);
        mPlayer->Resume();
        return;
    }

    PVPlayerState state = PVP_STATE_ERROR;
    if ((mPlayer->GetPVPlayerStateSync(state) == PVMFSuccess))
    {
        if (state == PVP_STATE_ERROR)
        {
            return;
        }
    }


    PlayerCommand* command;

    command = dequeueCommand();
    if (command) {
        LOGV("Send player code: %d", command->code());

        switch (command->code()) {
            case PlayerCommand::PLAYER_SETUP:
                handleSetup(static_cast<PlayerSetup*>(command));
                break;

            case PlayerCommand::PLAYER_SET_DATA_SOURCE:
                handleSetDataSource(static_cast<PlayerSetDataSource*>(command));
                break;

            case PlayerCommand::PLAYER_SET_VIDEO_SURFACE:
                handleSetVideoSurface(static_cast<PlayerSetVideoSurface*>(command));
                break;

            case PlayerCommand::PLAYER_SET_AUDIO_SINK:
                handleSetAudioSink(static_cast<PlayerSetAudioSink*>(command));
                break;

            case PlayerCommand::PLAYER_INIT:
                handleInit(static_cast<PlayerInit*>(command));
                break;

            case PlayerCommand::PLAYER_PREPARE:
                handlePrepare(static_cast<PlayerPrepare*>(command));
                break;

            case PlayerCommand::PLAYER_START:
                handleStart(static_cast<PlayerStart*>(command));
                break;

            case PlayerCommand::PLAYER_STOP:
                handleStop(static_cast<PlayerStop*>(command));
                break;

            case PlayerCommand::PLAYER_PAUSE:
                handlePause(static_cast<PlayerPause*>(command));
                break;

            case PlayerCommand::PLAYER_SEEK:
                handleSeek(static_cast<PlayerSeek*>(command));
                break;

            case PlayerCommand::PLAYER_GET_POSITION:
                handleGetPosition(static_cast<PlayerGetPosition*>(command));
                FinishSyncCommand(command);
                return;

            case PlayerCommand::PLAYER_GET_STATUS:
                handleGetStatus(static_cast<PlayerGetStatus*>(command));
                FinishSyncCommand(command);
                return;

            case PlayerCommand::PLAYER_GET_DURATION:
                handleGetDuration(static_cast<PlayerGetDuration*>(command));
                break;

            case PlayerCommand::PLAYER_REMOVE_DATA_SOURCE:
                handleRemoveDataSource(static_cast<PlayerRemoveDataSource*>(command));
                break;

            case PlayerCommand::PLAYER_CANCEL_ALL_COMMANDS:
                handleCancelAllCommands(static_cast<PlayerCancelAllCommands*>(command));
                break;

            case PlayerCommand::PLAYER_RESET:
                handleReset(static_cast<PlayerReset*>(command));
                break;

            case PlayerCommand::PLAYER_QUIT:
                handleQuit(static_cast<PlayerQuit*>(command));
                return;

            case PlayerCommand::PLAYER_SET_LOOP:
                mIsLooping = static_cast<PlayerSetLoop*>(command)->loop();
                FinishSyncCommand(command);
                return;

            default:
                LOGE("Unexpected code %d", command->code());
                break;
        }
    }

}

void PlayerDriver::commandFailed(PlayerCommand* command)
{
    if (command == NULL) {
        LOGV("async code failed");
        return;
    }

    LOGV("Command failed: %d", command->code());
    // FIXME: Ignore seek failure because it might not work when streaming
    if (mSeekPending) {
        LOGV("Ignoring failed seek");
        command->complete(NO_ERROR, false);
        mSeekPending = false;
    } else {
        command->complete(UNKNOWN_ERROR, false);
    }
    delete command;
}

void PlayerDriver::handleSetup(PlayerSetup* command)
{
    int error = 0;

    // Make sure we have the capabilities and config interface first.
    OSCL_TRY(error, mPlayer->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID,
                                            (PVInterface *&)mPlayerCapConfig, command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

int PlayerDriver::setupHttpStreamPre()
{
    mDataSource->SetDataSourceFormatType((char*)PVMF_MIME_DATA_SOURCE_HTTP_URL);

    delete mDownloadContextData;
    mDownloadContextData = NULL;

    mDownloadContextData = new PVMFSourceContextData();
    mDownloadContextData->EnableCommonSourceContext();
    mDownloadContextData->EnableDownloadHTTPSourceContext();

    // FIXME:
    // This mDownloadConfigFilename at /tmp/http-stream-cfg
    // should not exist. We need to clean it up later.
    mDownloadConfigFilename = _STRLIT_WCHAR("/tmp/http-stream-cfg");
    mDownloadFilename = NULL;
    mDownloadProxy = _STRLIT_CHAR("");

    mDownloadContextData->DownloadHTTPData()->iMaxFileSize = 0xFFFFFFFF;
    mDownloadContextData->DownloadHTTPData()->iPlaybackControl = PVMFSourceContextDataDownloadHTTP::ENoSaveToFile;
    mDownloadContextData->DownloadHTTPData()->iConfigFileName = mDownloadConfigFilename;
    mDownloadContextData->DownloadHTTPData()->iDownloadFileName = mDownloadFilename;
    mDownloadContextData->DownloadHTTPData()->iProxyName = mDownloadProxy;
    mDownloadContextData->DownloadHTTPData()->iProxyPort = 0;
    mDownloadContextData->DownloadHTTPData()->bIsNewSession = true;
    mDataSource->SetDataSourceContextData(mDownloadContextData);

    return 0;
}

int PlayerDriver::setupHttpStreamPost()
{
    PvmiKvp iKVPSetAsync;
    OSCL_StackString<64> iKeyStringSetAsync;
    PvmiKvp *iErrorKVP = NULL;

    int error = 0;

    iKeyStringSetAsync=_STRLIT_CHAR("x-pvmf/net/http-timeout;valtype=uint32");
    iKVPSetAsync.key=iKeyStringSetAsync.get_str();
    iKVPSetAsync.value.uint32_value=20;
    iErrorKVP=NULL;
    OSCL_TRY(error, mPlayerCapConfig->setParametersSync(NULL, &iKVPSetAsync, 1, iErrorKVP));
    OSCL_FIRST_CATCH_ANY(error, return -1);

    iKeyStringSetAsync=_STRLIT_CHAR("x-pvmf/net/num-redirect-attempts;valtype=uint32");
    iKVPSetAsync.key=iKeyStringSetAsync.get_str();
    iKVPSetAsync.value.uint32_value=4;
    iErrorKVP=NULL;
    OSCL_TRY(error, mPlayerCapConfig->setParametersSync(NULL, &iKVPSetAsync, 1, iErrorKVP));
    OSCL_FIRST_CATCH_ANY(error, return -1);

    iKeyStringSetAsync=_STRLIT_CHAR("x-pvmf/net/max-tcp-recv-buffer-size-download;valtype=uint32");
    iKVPSetAsync.key=iKeyStringSetAsync.get_str();
    iKVPSetAsync.value.uint32_value=64000;
    iErrorKVP=NULL;
    OSCL_TRY(error, mPlayerCapConfig->setParametersSync(NULL, &iKVPSetAsync, 1, iErrorKVP));
    OSCL_FIRST_CATCH_ANY(error, return -1);

    return 0;
}

void PlayerDriver::handleSetDataSource(PlayerSetDataSource* command)
{
    LOGV("handleSetDataSource");
    int error = 0;
    const char* url = command->url();
    int lengthofurl = strlen(url);
    oscl_wchar output[lengthofurl + 1];
    OSCL_wHeapString<OsclMemAllocator> wFileName;

    if (mDataSource) {
        delete mDataSource;
        mDataSource = NULL;
    }

    // Create a URL datasource to feed PVPlayer
    mDataSource = new PVPlayerDataSourceURL();
    oscl_UTF8ToUnicode(url, strlen(url), output, lengthofurl+1);
    wFileName.set(output, oscl_strlen(output));
    mDataSource->SetDataSourceURL(wFileName);
    LOGV("handleSetDataSource- scanning for extension");
    if (strncmp(url, "rtsp:", strlen("rtsp:")) == 0) {
        mDataSource->SetDataSourceFormatType((const char*)PVMF_MIME_DATA_SOURCE_RTSP_URL);
    } else if (strncmp(url, "http:", strlen("http:")) == 0) {
        if (0!=setupHttpStreamPre())
        {
            commandFailed(command);
            return;
        }
    } else {
        mDataSource->SetDataSourceFormatType((const char*)PVMF_MIME_FORMAT_UNKNOWN); // Let PV figure it out
    }

    OSCL_TRY(error, mPlayer->AddDataSource(*mDataSource, command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleInit(PlayerInit* command)
{
    int error = 0;

    if (mDownloadContextData) {
        setupHttpStreamPost();
    }

    {
        PvmiKvp iKVPSetAsync;
        PvmiKvp *iErrorKVP = NULL;

        int error = 0;
        iKVPSetAsync.key = _STRLIT_CHAR("x-pvmf/net/user-agent;valtype=wchar*");
        OSCL_wHeapString<OsclMemAllocator> userAgent = _STRLIT_WCHAR("CORE/6.506.4.1 OpenCORE/2.02 (Linux;Android 2.0)(AndroidMediaPlayer 2.0)");
        iKVPSetAsync.value.pWChar_value=userAgent.get_str();
        iErrorKVP=NULL;
        OSCL_TRY(error, mPlayerCapConfig->setParametersSync(NULL, &iKVPSetAsync, 1, iErrorKVP));
        OSCL_FIRST_CATCH_ANY(error,
                LOGE("handleInit- setParametersSync ERROR setting useragent");
        );
    }

    OSCL_TRY(error, mPlayer->Init(command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleSetVideoSurface(PlayerSetVideoSurface* command)
{

    // create video MIO if needed
    if (mVideoOutputMIO == NULL) {
        int error = 0;
        AndroidSurfaceOutput* mio = NULL;

        // attempt to load device-specific video MIO
        if (mLibHandle != NULL) {
            VideoMioFactory f = (VideoMioFactory) ::dlsym(mLibHandle, VIDEO_MIO_FACTORY_NAME);
            if (f != NULL) {
                mio = f();
            }
        }

        // if no device-specific MIO was created, use the generic one
        if (mio == NULL) {
            LOGW("Using generic video MIO");
            mio = new AndroidSurfaceOutput();
        }

        // initialize the MIO parameters
        status_t ret = mio->set(mPvPlayer, command->surface(), mEmulation);
        if (ret != NO_ERROR) {
            LOGE("Video MIO set failed");
            commandFailed(command);
            delete mio;
            return;
        }
        mVideoOutputMIO = mio;

        mVideoNode = PVMediaOutputNodeFactory::CreateMediaOutputNode(mVideoOutputMIO);
        mVideoSink = new PVPlayerDataSinkPVMFNode;

        ((PVPlayerDataSinkPVMFNode *)mVideoSink)->SetDataSinkNode(mVideoNode);
        ((PVPlayerDataSinkPVMFNode *)mVideoSink)->SetDataSinkFormatType((char*)PVMF_MIME_YUV420);

        OSCL_TRY(error, mPlayer->AddDataSink(*mVideoSink, command));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
    } else {
        // change display surface
        if (mVideoOutputMIO->setVideoSurface(command->surface()) == NO_ERROR) {
            FinishSyncCommand(command);
        } else {
            LOGE("Video MIO set failed");
            commandFailed(command);
        }
    }
}

void PlayerDriver::handleSetAudioSink(PlayerSetAudioSink* command)
{
    int error = 0;
    if (command->audioSink()->realtime()) {
        LOGV("Create realtime output");
        mAudioOutputMIO = new AndroidAudioOutput();
    } else {
        LOGV("Create stream output");
        mAudioOutputMIO = new AndroidAudioStream();
    }
    mAudioOutputMIO->setAudioSink(command->audioSink());

    mAudioNode = PVMediaOutputNodeFactory::CreateMediaOutputNode(mAudioOutputMIO);
    mAudioSink = new PVPlayerDataSinkPVMFNode;

    ((PVPlayerDataSinkPVMFNode *)mAudioSink)->SetDataSinkNode(mAudioNode);
    ((PVPlayerDataSinkPVMFNode *)mAudioSink)->SetDataSinkFormatType((char*)PVMF_MIME_PCM16);

    OSCL_TRY(error, mPlayer->AddDataSink(*mAudioSink, command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handlePrepare(PlayerPrepare* command)
{
    //Keep alive is sent during the play to prevent the firewall from closing ports while
    //streaming long clip
    PvmiKvp iKVPSetAsync;
    OSCL_StackString<64> iKeyStringSetAsync;
    PvmiKvp *iErrorKVP = NULL;
    int error=0;
    iKeyStringSetAsync=_STRLIT_CHAR("x-pvmf/net/keep-alive-during-play;valtype=bool");
    iKVPSetAsync.key=iKeyStringSetAsync.get_str();
    iKVPSetAsync.value.bool_value=true;
    iErrorKVP=NULL;
    OSCL_TRY(error, mPlayerCapConfig->setParametersSync(NULL, &iKVPSetAsync, 1, iErrorKVP));
    OSCL_TRY(error, mPlayer->Prepare(command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleStart(PlayerStart* command)
{
    int error = 0;

    // for video, set thread priority so we don't hog CPU
    if (mVideoOutputMIO) {
        int ret = setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
    }
    // for audio, set thread priority so audio isn't choppy
    else {
        int ret = setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    }

    // Signalling seek complete to continue obtaining the current position
    // from PVPlayer Engine
    mSeekComp = true;
    // if we are paused, just resume
    PVPlayerState state;
    if (mPlayer->GetPVPlayerStateSync(state) == PVMFSuccess
        && (state == PVP_STATE_PAUSED)) {
        if (mEndOfData) {
            // if we are at the end, seek to the beginning first
            mEndOfData = false;
            PVPPlaybackPosition begin, end;
            begin.iIndeterminate = false;
            begin.iPosUnit = PVPPBPOSUNIT_SEC;
            begin.iPosValue.sec_value = 0;
            begin.iMode = PVPPBPOS_MODE_NOW;
            end.iIndeterminate = true;
            mPlayer->SetPlaybackRange(begin, end, false, NULL);
        }
        OSCL_TRY(error, mPlayer->Resume(command));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
    } else {
        OSCL_TRY(error, mPlayer->Start(command));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
    }
}

void PlayerDriver::handleSeek(PlayerSeek* command)
{
    int error = 0;

    LOGV("handleSeek");
    // Cache the most recent seek request
    mRecentSeek = command->msec();
    // Seeking in the pause state
    PVPlayerState state;
    if (mPlayer->GetPVPlayerStateSync(state) == PVMFSuccess
        && (state == PVP_STATE_PAUSED)) {
        mSeekComp = false;
    }
    PVPPlaybackPosition begin, end;
    begin.iIndeterminate = false;
    begin.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    begin.iPosValue.millisec_value = command->msec();
    begin.iMode = PVPPBPOS_MODE_NOW;
    end.iIndeterminate = true;
    mSeekPending = true;
    OSCL_TRY(error, mPlayer->SetPlaybackRange(begin, end, false, command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));

    mEndOfData = false;
}

void PlayerDriver::handleGetPosition(PlayerGetPosition* command)
{
    PVPPlaybackPosition pos;
    pos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    PVPlayerState state;
    //  In the pause state, get the progress bar position from the recent seek value
    // instead of GetCurrentPosition() from PVPlayer Engine.
    if (mPlayer->GetPVPlayerStateSync(state) == PVMFSuccess
        && (state == PVP_STATE_PAUSED)
        && (mSeekComp == false)) {
        command->set(mRecentSeek);
    }
    else {
        if (mPlayer->GetCurrentPositionSync(pos) != PVMFSuccess) {
            command->set(-1);
        } else {
            LOGV("position=%d", pos.iPosValue.millisec_value);
            command->set((int)pos.iPosValue.millisec_value);
        }
    }
}

void PlayerDriver::handleGetStatus(PlayerGetStatus* command)
{
    PVPlayerState state;
    if (mPlayer->GetPVPlayerStateSync(state) != PVMFSuccess) {
        command->set(0);
    } else {
        command->set(state);
        LOGV("status=%d", state);
    }
}

void PlayerDriver::handleGetDuration(PlayerGetDuration* command)
{
    command->set(-1);
    mMetaKeyList.clear();
    mMetaKeyList.push_back(OSCL_HeapString<OsclMemAllocator>("duration"));
    mMetaValueList.clear();
    mNumMetaValues=0;
    int error = 0;
    OSCL_TRY(error, mPlayer->GetMetadataValues(mMetaKeyList,0,-1,mNumMetaValues,mMetaValueList, command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleStop(PlayerStop* command)
{
    int error = 0;
    // setting the looping boolean to false. MediaPlayer app takes care of setting the loop again before the start.
    mIsLooping = false;
    mDoLoop = false;
    PVPlayerState state;
    if ((mPlayer->GetPVPlayerStateSync(state) == PVMFSuccess)
        && ( (state == PVP_STATE_PAUSED) ||
             (state == PVP_STATE_PREPARED) ||
             (state == PVP_STATE_STARTED) ))
    {
        OSCL_TRY(error, mPlayer->Stop(command));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
    }
    else
    {
        LOGV("handleStop - Player State = %d - Sending Reset instead of Stop\n",state);
        // TODO: Previously this called CancelAllCommands and RemoveDataSource
        handleReset(new PlayerReset(command->callback(), command->cookie()));
        delete command;
    }
}

void PlayerDriver::handlePause(PlayerPause* command)
{
    LOGV("call pause");
    int error = 0;
    OSCL_TRY(error, mPlayer->Pause(command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleRemoveDataSource(PlayerRemoveDataSource* command)
{
    LOGV("handleRemoveDataSource");
    int error = 0;
    OSCL_TRY(error, mPlayer->RemoveDataSource(*mDataSource, command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleCancelAllCommands(PlayerCancelAllCommands* command)
{
    LOGV("handleCancelAllCommands");
    int error = 0;
    OSCL_TRY(error, mPlayer->CancelAllCommands(command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleReset(PlayerReset* command)
{
    LOGV("handleReset");
    int error = 0;

    // setting the looping boolean to false. MediaPlayer app takes care of setting the loop again before the start.
    mIsLooping = false;
    mDoLoop = false;
    mEndOfData = false;

    OSCL_TRY(error, mPlayer->Reset(command));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(command));
}

void PlayerDriver::handleQuit(PlayerQuit* command)
{
    OsclExecScheduler *sched = OsclExecScheduler::Current();
    sched->StopScheduler();
}

/*static*/ int PlayerDriver::startPlayerThread(void *cookie)
{
    LOGV("startPlayerThread");
    PlayerDriver *ed = (PlayerDriver *)cookie;
    return ed->playerThread();
}

int PlayerDriver::playerThread()
{
    int error;

    LOGV("InitializeForThread");
    if (!InitializeForThread())
    {
        LOGV("InitializeForThread fail");
        mPlayer = NULL;
        mSyncSem->Signal();
        return -1;
    }

    LOGV("OMX_MasterInit");
    OMX_MasterInit();

    LOGV("OsclScheduler::Init");
    OsclScheduler::Init("AndroidPVWrapper");

    LOGV("CreatePlayer");
    OSCL_TRY(error, mPlayer = PVPlayerFactory::CreatePlayer(this, this, this));
    if (error) {
        // Just crash the first time someone tries to use it for now?
        mPlayer = NULL;
        mSyncSem->Signal();
        return -1;
    }

    LOGV("AddToScheduler");
    AddToScheduler();
    LOGV("PendForExec");
    PendForExec();

    LOGV("OsclActiveScheduler::Current");
    OsclExecScheduler *sched = OsclExecScheduler::Current();
    LOGV("StartScheduler");
    error = OsclErrNone;
    OSCL_TRY(error, sched->StartScheduler(mSyncSem));
    OSCL_FIRST_CATCH_ANY(error,
                         // Some AO did a leave, log it
                         LOGE("Player Engine AO did a leave, error=%d", error)
                        );

    LOGV("DeletePlayer");
    PVPlayerFactory::DeletePlayer(mPlayer);

    delete mDownloadContextData;
    mDownloadContextData = NULL;

    delete mDataSource;
    mDataSource = NULL;
    delete mAudioSink;
    PVMediaOutputNodeFactory::DeleteMediaOutputNode(mAudioNode);
    delete mAudioOutputMIO;
    delete mVideoSink;
    if (mVideoNode) {
        PVMediaOutputNodeFactory::DeleteMediaOutputNode(mVideoNode);
        delete mVideoOutputMIO;
    }

    mSyncStatus = OK;
    mSyncSem->Signal();
    // note that we only signal mSyncSem. Deleting it is handled
    // in enqueueCommand(). This is done because waiting for an
    // already-deleted OsclSemaphore doesn't work (it blocks),
    // and it's entirely possible for this thread to exit before
    // enqueueCommand() gets around to waiting for the semaphore.

    // do some of destructor's work here
    // goodbye cruel world
    delete this;

    //Moved after the delete this, as Oscl cleanup should be done in the end.
    //delete this was cleaning up OsclSemaphore objects, eventually causing a crash
    OsclScheduler::Cleanup();
    LOGV("OsclScheduler::Cleanup");

    OMX_MasterDeinit();
    UninitializeForThread();
    return 0;
}

/*static*/ void PlayerDriver::syncCompletion(status_t s, void *cookie, bool cancelled)
{
    PlayerDriver *ed = static_cast<PlayerDriver*>(cookie);
    ed->mSyncStatus = s;
    ed->mSyncSem->Signal();
}

void PlayerDriver::handleGetDurationComplete(PlayerGetDuration* cmd)
{
    cmd->set(-1);

    if (mMetaValueList.empty())
        return;

    MediaClockConverter mcc;

    for (uint32 i = 0; i < mMetaValueList.size(); ++i) {
        // Search for the duration
        const char* substr=oscl_strstr(mMetaValueList[i].key, _STRLIT_CHAR("duration;valtype=uint32;timescale="));
        if (substr!=NULL) {
            uint32 timescale=1000;
            if (PV_atoi((substr+34), 'd', timescale) == false) {
                // Retrieving timescale failed so default to 1000
                timescale=1000;
            }
            uint32 duration = mMetaValueList[i].value.uint32_value;
            if (duration > 0 && timescale > 0) {
                //set the timescale
                mcc.set_timescale(timescale);
                //set the clock to the duration as per the timescale
                mcc.set_clock(duration,0);
                //convert to millisec
                cmd->set(mcc.get_converted_ts(1000));
            }
        }
    }
}

void PlayerDriver::CommandCompleted(const PVCmdResponse& aResponse)
{
    LOGV("CommandCompleted");
    PVMFStatus status = aResponse.GetCmdStatus();

    if (mDoLoop) {
        mDoLoop = false;
        RunIfNotReady();
        return;
    }

    PlayerCommand* command = static_cast<PlayerCommand*>(aResponse.GetContext());
    LOGV("Completed command %s status=%s", command ? command->toString(): "<null>", PVMFStatusToString(status));
    if (command == NULL) return;

    // FIXME: Ignore non-fatal seek errors because pvPlayerEngine returns these errors and retains it's state.
    if (mSeekPending) {
        mSeekPending = false;
        if ( ( (status == PVMFErrArgument) || (status == PVMFErrInvalidState) || (status == PVMFErrNotSupported) ) ) {
            LOGV("Ignoring error during seek");
            status = PVMFSuccess;
        }
    }

    if (status == PVMFSuccess) {
        switch (command->code()) {
            case PlayerCommand::PLAYER_PREPARE:
                LOGV("PLAYER_PREPARE complete mDownloadContextData=%p, mDataReadyReceived=%d", mDownloadContextData, mDataReadyReceived);
                mPrepareDone = true;
                // If we are streaming from the network, we
                // have to wait until the first PVMFInfoDataReady
                // is sent to notify the user that it is okay to
                // begin playback.  If it is a local file, just
                // send it now at the completion of Prepare().
                if ((mDownloadContextData == NULL) || mDataReadyReceived) {
                    mPvPlayer->sendEvent(MEDIA_PREPARED);
                }
                break;

            case PlayerCommand::PLAYER_GET_DURATION:
                handleGetDurationComplete(static_cast<PlayerGetDuration*>(command));
                break;

            case PlayerCommand::PLAYER_PAUSE:
                LOGV("pause complete");
                break;

            case PlayerCommand::PLAYER_SEEK:
                mPvPlayer->sendEvent(MEDIA_SEEK_COMPLETE);
                break;

            default: /* shut up gcc */
                break;
        }

        // Call the user's requested completion function
        command->complete(NO_ERROR, false);
    } else if (status == PVMFErrCancelled) {
        // Ignore cancelled code return status (PVMFErrCancelled), since it is not an error.
        LOGE("Command (%d) was cancelled", command->code());
        status = PVMFSuccess;
        command->complete(NO_ERROR, true);
    } else {
        // Try to map the PV error code to an Android one.
        LOGE("Command %s completed with an error or info %s", command->toString(), PVMFStatusToString(status));
        const media_event_type event_type = MapStatusToEventType(status);

        if (MEDIA_NOP != event_type) {
            mPvPlayer->sendEvent(event_type, MapStatusToEventCode(status), status);
        } else {
            LOGE("Ignoring: %d", status);
        }
        command->complete(UNKNOWN_ERROR, false);
    }

    delete command;
}

void PlayerDriver::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    PVMFStatus status = aEvent.GetEventType();

    // Errors use negative codes (see pvmi/pvmf/include/pvmf_return_codes.h)
    if (status > PVMFErrFirst) {
        LOGE("HandleErrorEvent called with an non-error event [%d]!!", status);
    }
    LOGE("HandleErrorEvent: %s", PVMFStatusToString(status));
    // TODO: Map more of the PV error code into the Android Media Player ones.
    mPvPlayer->sendEvent(MEDIA_ERROR, ::android::MEDIA_ERROR_UNKNOWN, status);
}

void PlayerDriver::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    PVMFStatus status = aEvent.GetEventType();

    // Errors use negative codes (see pvmi/pvmf/include/pvmf_return_codes.h)
    if (status <= PVMFErrFirst) {
        // Errors should go to the HandleErrorEvent handler, not the
        // informational one.
        LOGE("HandleInformationalEvent called with an error event [%d]!!", status);
    }

    LOGV("HandleInformationalEvent: %s", PVMFStatusToString(status));

    switch (status) {
        case PVMFInfoEndOfData:
            mEndOfData = true;
            if (mIsLooping) {
                mDoLoop = true;
                Cancel();
                RunIfNotReady();
            } else {
                mPvPlayer->sendEvent(MEDIA_PLAYBACK_COMPLETE);
            }
            break;

        case PVMFInfoErrorHandlingComplete:
            LOGW("PVMFInfoErrorHandlingComplete");
            RunIfNotReady();
            break;

        case PVMFInfoBufferingStart:
            mPvPlayer->sendEvent(MEDIA_BUFFERING_UPDATE, 0);
            break;

        case PVMFInfoBufferingStatus:
            {
                const void *buffer = aEvent.GetLocalBuffer();
                const size_t size = aEvent.GetLocalBufferSize();
                int percentage;

                if (GetBufferingPercentage(buffer, size, &percentage))
                {
                    LOGD("buffering (%d)", percentage);
                    mPvPlayer->sendEvent(MEDIA_BUFFERING_UPDATE, percentage);
                }
            }
            break;

        case PVMFInfoDurationAvailable:
            {
                PVUuid infomsguuid = PVMFDurationInfoMessageInterfaceUUID;
                PVMFDurationInfoMessageInterface* eventMsg = NULL;
                PVInterface* infoExtInterface = aEvent.GetEventExtensionInterface();
                if (infoExtInterface &&
                    infoExtInterface->queryInterface(infomsguuid, (PVInterface*&)eventMsg))
                {
                    PVUuid eventuuid;
                    int32 infoCode;
                    eventMsg->GetCodeUUID(infoCode, eventuuid);
                    if (eventuuid == infomsguuid)
                    {
                        uint32 SourceDurationInMS = eventMsg->GetDuration();
                        LOGV(".... with duration = %u ms",SourceDurationInMS);
                    }
                }
            }
            break;

        case PVMFInfoDataReady:
            if (mDataReadyReceived)
                break;
            mDataReadyReceived = true;
            // If this is a network stream, we are now ready to play.
            if (mDownloadContextData && mPrepareDone) {
                mPvPlayer->sendEvent(MEDIA_PREPARED);
            }
            break;

        case PVMFInfoVideoTrackFallingBehind:
            // TODO: This event should not be passed to the user in the ERROR channel.
            LOGW("Video track fell behind");
            mPvPlayer->sendEvent(MEDIA_INFO, ::android::MEDIA_INFO_VIDEO_TRACK_LAGGING,
                                 PVMFInfoVideoTrackFallingBehind);
            break;

        case PVMFInfoPoorlyInterleavedContent:
            // TODO: This event should not be passed to the user in the ERROR channel.
            LOGW("Poorly interleaved content.");
            mPvPlayer->sendEvent(MEDIA_INFO, ::android::MEDIA_INFO_BAD_INTERLEAVING,
                                 PVMFInfoPoorlyInterleavedContent);
            break;

        case PVMFInfoContentTruncated:
            LOGE("Content is truncated.");
            break;

        /* Certain events we don't really care about, but don't
         * want log spewage, so just no-op them here.
         */
        case PVMFInfoPositionStatus:
        case PVMFInfoBufferingComplete:
        case PVMFInfoContentLength:
        case PVMFInfoContentType:
        case PVMFInfoUnderflow:
        case PVMFInfoDataDiscarded:
            break;

        default:
            LOGV("HandleInformationalEvent: type=%d UNHANDLED", status);
            break;
    }
}

// ----------------------------------------------------------------------------
// PlayerCommand implementation
// ----------------------------------------------------------------------------
const char* PlayerCommand::toString() const {
    return PlayerCommandCodeToString(code());
}

namespace android {

#undef LOG_TAG
#define LOG_TAG "PVPlayer"

#ifdef MAX_OPENCORE_INSTANCES
/*static*/ volatile int32_t PVPlayer::sNumInstances = 0;
#endif

// ----------------------------------------------------------------------------
// implement the Packet Video player
// ----------------------------------------------------------------------------
PVPlayer::PVPlayer()
{
    LOGV("PVPlayer constructor");
    mDataSourcePath = NULL;
    mSharedFd = -1;
    mIsDataSourceSet = false;
    mDuration = -1;
    mPlayerDriver = NULL;

#ifdef MAX_OPENCORE_INSTANCES
    if (android_atomic_inc(&sNumInstances) >= MAX_OPENCORE_INSTANCES) {
        LOGW("Exceeds maximum number of OpenCore instances");
        mInit = -EBUSY;
        return;
    }
#endif

    LOGV("construct PlayerDriver");
    mPlayerDriver = new PlayerDriver(this);
    LOGV("send PLAYER_SETUP");
    PlayerSetup* setup = new PlayerSetup(0,0);
    mInit = mPlayerDriver->enqueueCommand(setup);
    if (mInit == NO_INIT) {
        delete setup;
    }
}

status_t PVPlayer::initCheck()
{
    return mInit;
}

PVPlayer::~PVPlayer()
{
    LOGV("PVPlayer destructor");
    if (mPlayerDriver != NULL) {
        PlayerQuit quit = PlayerQuit(0,0);
        mPlayerDriver->enqueueCommand(&quit); // will wait on mSyncSem, signaled by player thread
    }
    free(mDataSourcePath);
    if (mSharedFd >= 0) {
        close(mSharedFd);
    }
#ifdef MAX_OPENCORE_INSTANCES
    android_atomic_dec(&sNumInstances);
#endif
}

status_t PVPlayer::setDataSource(const char *url)
{
    LOGV("setDataSource(%s)", url);
    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }
    free(mDataSourcePath);
    mDataSourcePath = NULL;

    // Don't let somebody trick us in to reading some random block of memory
    if (strncmp("sharedfd://", url, 11) == 0)
        return android::UNKNOWN_ERROR;
    mDataSourcePath = strdup(url);
    return OK;
}

status_t PVPlayer::setDataSource(int fd, int64_t offset, int64_t length) {

    // This is all a big hack to allow PV to play from a file descriptor.
    // Eventually we'll fix PV to use a file descriptor directly instead
    // of using mmap().
    LOGV("setDataSource(%d, %lld, %lld)", fd, offset, length);
    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }
    free(mDataSourcePath);
    mDataSourcePath = NULL;

    char buf[80];
    mSharedFd = dup(fd);
    sprintf(buf, "sharedfd://%d:%lld:%lld", mSharedFd, offset, length);
    mDataSourcePath = strdup(buf);
    return OK;
}

status_t PVPlayer::setVideoSurface(const sp<ISurface>& surface)
{
    LOGV("setVideoSurface(%p)", surface.get());
    mSurface = surface;
    return OK;
}

status_t PVPlayer::prepare()
{
    status_t ret;

    // We need to differentiate the two valid use cases for prepare():
    // 1. new PVPlayer/reset()->setDataSource()->prepare()
    // 2. new PVPlayer/reset()->setDataSource()->prepare()/prepareAsync()
    //    ->start()->...->stop()->prepare()
    // If data source has already been set previously, no need to run
    // a sequence of commands and only the PLAYER_PREPARE code needs
    // to be run.
    if (!mIsDataSourceSet) {
        // set data source
        LOGV("prepare");
        LOGV("  data source = %s", mDataSourcePath);
        ret = mPlayerDriver->enqueueCommand(new PlayerSetDataSource(mDataSourcePath,0,0));
        if (ret != OK)
            return ret;

        // init
        LOGV("  init");
        ret = mPlayerDriver->enqueueCommand(new PlayerInit(0,0));
        if (ret != OK)
            return ret;

        // set video surface, if there is one
        if (mSurface != NULL) {
            LOGV("  set video surface");
            ret = mPlayerDriver->enqueueCommand(new PlayerSetVideoSurface(mSurface,0,0));
            if (ret != OK)
                return ret;
        }

        // set audio output
        // If we ever need to expose selectable audio output setup, this can be broken
        // out.  In the meantime, however, system audio routing APIs should suffice.
        LOGV("  set audio sink");
        ret = mPlayerDriver->enqueueCommand(new PlayerSetAudioSink(mAudioSink,0,0));
        if (ret != OK)
            return ret;

        // New data source has been set successfully.
        mIsDataSourceSet = true;
    }

    // prepare
    LOGV("  prepare");
    return mPlayerDriver->enqueueCommand(new PlayerPrepare(0,0));
}

void PVPlayer::run_init(status_t s, void *cookie, bool cancelled)
{
    LOGV("run_init s=%d, cancelled=%d", s, cancelled);
    if (s == NO_ERROR && !cancelled) {
        PVPlayer *p = (PVPlayer*)cookie;
        p->mPlayerDriver->enqueueCommand(new PlayerInit(run_set_video_surface, cookie));
    }
}

void PVPlayer::run_set_video_surface(status_t s, void *cookie, bool cancelled)
{
    LOGV("run_set_video_surface s=%d, cancelled=%d", s, cancelled);
    if (s == NO_ERROR && !cancelled) {
        // If we don't have a video surface, just skip to the next step.
        PVPlayer *p = (PVPlayer*)cookie;
        if (p->mSurface == NULL) {
            run_set_audio_output(s, cookie, false);
        } else {
            p->mPlayerDriver->enqueueCommand(new PlayerSetVideoSurface(p->mSurface, run_set_audio_output, cookie));
        }
    }
}

void PVPlayer::run_set_audio_output(status_t s, void *cookie, bool cancelled)
{
    LOGV("run_set_audio_output s=%d, cancelled=%d", s, cancelled);
    if (s == NO_ERROR && !cancelled) {
        PVPlayer *p = (PVPlayer*)cookie;
        p->mPlayerDriver->enqueueCommand(new PlayerSetAudioSink(p->mAudioSink, run_prepare, cookie));
    }
}

void PVPlayer::run_prepare(status_t s, void *cookie, bool cancelled)
{
    LOGV("run_prepare s=%d, cancelled=%d", s, cancelled);
    if (s == NO_ERROR && !cancelled) {
        PVPlayer *p = (PVPlayer*)cookie;
        p->mPlayerDriver->enqueueCommand(new PlayerPrepare(do_nothing,0));
    }
}

status_t PVPlayer::prepareAsync()
{
    LOGV("prepareAsync");
    status_t ret = OK;

    if (!mIsDataSourceSet) {  // If data source has NOT been set.
        // Set our data source as cached in setDataSource() above.
        LOGV("  data source = %s", mDataSourcePath);
        ret = mPlayerDriver->enqueueCommand(new PlayerSetDataSource(mDataSourcePath,run_init,this));
        mIsDataSourceSet = true;
    } else {  // If data source has been already set.
        // No need to run a sequence of commands.
        // The only code needed to run is PLAYER_PREPARE.
        ret = mPlayerDriver->enqueueCommand(new PlayerPrepare(do_nothing, NULL));
    }

    return ret;
}

status_t PVPlayer::start()
{
    LOGV("start");
    return mPlayerDriver->enqueueCommand(new PlayerStart(0,0));
}

status_t PVPlayer::stop()
{
    LOGV("stop");
    return mPlayerDriver->enqueueCommand(new PlayerStop(0,0));
}

status_t PVPlayer::pause()
{
    LOGV("pause");
    return mPlayerDriver->enqueueCommand(new PlayerPause(0,0));
}

bool PVPlayer::isPlaying()
{
    int status = 0;
    if (mPlayerDriver->enqueueCommand(new PlayerGetStatus(&status,0,0)) == NO_ERROR) {
        return (status == PVP_STATE_STARTED);
    }
    return false;
}

status_t PVPlayer::getCurrentPosition(int *msec)
{
    return mPlayerDriver->enqueueCommand(new PlayerGetPosition(msec,0,0));
}

status_t PVPlayer::getDuration(int *msec)
{
    status_t ret = mPlayerDriver->enqueueCommand(new PlayerGetDuration(msec,0,0));
    if (ret == NO_ERROR) mDuration = *msec;
    return ret;
}

status_t PVPlayer::seekTo(int msec)
{
    LOGV("seekTo(%d)", msec);
    // can't always seek to end of streams - so we fudge a little
    if ((msec == mDuration) && (mDuration > 0)) {
        msec--;
        LOGV("Seek adjusted 1 msec from end");
    }
    return mPlayerDriver->enqueueCommand(new PlayerSeek(msec,do_nothing,0));
}

status_t PVPlayer::reset()
{
    LOGV("reset");
    status_t ret = mPlayerDriver->enqueueCommand(new PlayerCancelAllCommands(0,0));

    // Log failure from CancelAllCommands() and call Reset() regardless.
    if (ret != NO_ERROR) {
        LOGE("failed to cancel all exiting PV player engine commands with error code (%d)", ret);
    }
    ret = mPlayerDriver->enqueueCommand(new PlayerReset(0,0));

    // We should never fail in Reset(), but logs the failure just in case.
    if (ret != NO_ERROR) {
        LOGE("failed to reset PV player engine with error code (%d)", ret);
    } else {
        ret = mPlayerDriver->enqueueCommand(new PlayerRemoveDataSource(0,0));
    }

    mSurface.clear();
    LOGV("unmap file");
    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }
    mIsDataSourceSet = false;
    return ret;
}

status_t PVPlayer::setLooping(int loop)
{
    LOGV("setLooping(%d)", loop);
    return mPlayerDriver->enqueueCommand(new PlayerSetLoop(loop,0,0));
}

// This is a stub for the direct invocation API.
// From include/media/MediaPlayerInterface.h where the abstract method
// is declared:
//
//   Invoke a generic method on the player by using opaque parcels
//   for the request and reply.
//   @param request Parcel that must start with the media player
//   interface token.
//   @param[out] reply Parcel to hold the reply data. Cannot be null.
//   @return OK if the invocation was made successfully.
//
// This stub should be replaced with a concrete implementation.
//
// Typically the request parcel will contain an opcode to identify an
// operation to be executed. There might also be a handle used to
// create a session between the client and the player.
//
// The concrete implementation can then dispatch the request
// internally based on the double (opcode, handle).
status_t PVPlayer::invoke(const Parcel& request, Parcel *reply)
{
    return INVALID_OPERATION;
}

}; // namespace android
