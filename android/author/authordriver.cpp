/*
 * Copyright (c) 2008, The Android Open Source Project
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

#include "authordriver.h"
#include <sys/prctl.h>

using namespace android;

AuthorDriverWrapper::AuthorDriverWrapper()
{
    mAuthorDriver = new AuthorDriver();
}

AuthorDriverWrapper::~AuthorDriverWrapper()
{
    author_command *ac = new author_command(AUTHOR_QUIT);
    enqueueCommand(ac, NULL, NULL); // will wait on mSyncSem, signaled by author thread
    delete ac; // have to delete this manually because CommandCompleted won't be called
}

status_t AuthorDriverWrapper::getMaxAmplitude(int *max)
{
    return mAuthorDriver->getMaxAmplitude(max);
}

status_t AuthorDriverWrapper::enqueueCommand(author_command *ac, media_completion_f comp, void *cookie)
{
    return mAuthorDriver->enqueueCommand(ac, comp, cookie);
}

static void pv_global_init()
{
    OsclBase::Init();
    OsclErrorTrap::Init();
    OsclMem::Init();
    PVLogger::Init();
}

static void pv_global_cleanup()
{
    OsclScheduler::Cleanup();
    PVLogger::Cleanup();
    OsclMem::Cleanup();
    OsclErrorTrap::Cleanup();
    OsclBase::Cleanup();
}

AuthorDriver::AuthorDriver()
             : OsclActiveObject(OsclActiveObject::EPriorityNominal, "AuthorDriver"),
               mAuthor(NULL),
               mVideoInputMIO(NULL),
               mVideoNode(NULL),
               mAudioInputMIO(NULL),
               mAudioNode(NULL),
               mSelectedComposer(NULL),
               mComposerConfig(NULL),
               mVideoEncoderConfig(NULL),
               mAudioEncoderConfig(NULL),
               mVideoWidth(DEFAULT_FRAME_WIDTH),
               mVideoHeight(DEFAULT_FRAME_HEIGHT),
               mVideoFrameRate((int)DEFAULT_FRAME_RATE),
               mVideoEncoder(VIDEO_ENCODER_DEFAULT),
               mAudioEncoder(AUDIO_ENCODER_DEFAULT)
{
    mSyncSem = new OsclSemaphore();
    mSyncSem->Create();

    createThread(AuthorDriver::startAuthorThread, this);
    // mSyncSem will be signaled when the scheduler has started
    mSyncSem->Wait();
}

AuthorDriver::~AuthorDriver()
{
}

author_command *AuthorDriver::dequeueCommand()
{
    author_command *ac;
    mQueueLock.lock();

    // XXX should we assert here?
    if (mCommandQueue.empty()) {
        PendForExec();
        mQueueLock.unlock();
        return NULL;
    }

    ac = *(--mCommandQueue.end());
    mCommandQueue.erase(--mCommandQueue.end());
    if (mCommandQueue.size() > 0) {
        RunIfNotReady();
    } else {
        PendForExec();
    }
    mQueueLock.unlock();

    return ac;
}

status_t AuthorDriver::enqueueCommand(author_command *ac, media_completion_f comp, void *cookie)
{
    int sync_wait = 0;

    // If the user didn't specify a completion callback, we
    // are running in synchronous mode.
    if (comp == NULL) {
        ac->comp = AuthorDriver::syncCompletion;
        ac->cookie = this;
        sync_wait = 1;
    } else {
        ac->comp = comp;
        ac->cookie = cookie;
    }

    // Add the command to the queue.
    mQueueLock.lock();
    mCommandQueue.push_front(ac);

    // make a copy of this semaphore for special handling of the AUTHOR_QUIT command
    OsclSemaphore *syncsemcopy = mSyncSem;
    // make a copy of ac->which, since ac will be deleted by the standard completion function
    author_command_type whichcopy = ac->which;

    // Wake up the author thread so it can dequeue the command.
    if (mCommandQueue.size() == 1) {
        PendComplete(OSCL_REQUEST_ERR_NONE);
    }
    mQueueLock.unlock();
    // If we are in synchronous mode, wait for completion.
    if (syncsemcopy) {
        syncsemcopy->Wait();
        if (whichcopy == AUTHOR_QUIT) {
            syncsemcopy->Close();
            delete syncsemcopy;
            return 0;
        }
        return mSyncStatus;
    }

    return OK;
}

void AuthorDriver::FinishNonAsyncCommand(author_command *ac)
{
    ac->comp(0, ac->cookie); // this signals the semaphore for synchronous calls
}

// The OSCL scheduler calls this when we get to run (this should happen only
// when a command has been enqueued for us).
void AuthorDriver::Run()
{
    author_command *ac;

    ac = dequeueCommand();
    if (ac == NULL) {
        // assert?
        return;
    }

    switch(ac->which) {
    case AUTHOR_INIT:
        handleInit(ac);
        break; 

    case AUTHOR_SET_AUDIO_SOURCE:
        handleSetAudioSource((set_audio_source_command *)ac);
        break;

    case AUTHOR_SET_VIDEO_SOURCE:
        handleSetVideoSource((set_video_source_command *)ac);
        break;

    case AUTHOR_SET_OUTPUT_FORMAT:
        handleSetOutputFormat((set_output_format_command *)ac);
        break;

    case AUTHOR_SET_AUDIO_ENCODER:
        handleSetAudioEncoder((set_audio_encoder_command *)ac);
        break;

    case AUTHOR_SET_VIDEO_ENCODER:
        handleSetVideoEncoder((set_video_encoder_command *)ac);
        break;

    case AUTHOR_SET_VIDEO_SIZE:
        handleSetVideoSize((set_video_size_command *)ac);
        FinishNonAsyncCommand(ac);
        return;

    case AUTHOR_SET_VIDEO_FRAME_RATE:
        handleSetVideoFrameRate((set_video_frame_rate_command *)ac);
        FinishNonAsyncCommand(ac);
        return;

    case AUTHOR_SET_PREVIEW_SURFACE:
        handleSetPreviewSurface((set_preview_surface_command *)ac);
        FinishNonAsyncCommand(ac);
        return;

    case AUTHOR_SET_OUTPUT_FILE:
        handleSetOutputFile((set_output_file_command *)ac);
        FinishNonAsyncCommand(ac);
        return;

    case AUTHOR_PREPARE: handlePrepare(ac); break;
    case AUTHOR_START: handleStart(ac); break;
    case AUTHOR_STOP: handleStop(ac); break;
    case AUTHOR_RESET: handleReset(ac); break;
    case AUTHOR_QUIT: handleQuit(ac); return;

    default:
        assert(0);
        break;
    }

 }

void AuthorDriver::commandFailed(author_command *ac)
{
    ac->comp(UNKNOWN_ERROR, ac->cookie);
    delete ac;
}

void AuthorDriver::handleInit(author_command *ac)
{
    int error = 0;
    OSCL_TRY(error, mAuthor->Open(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetAudioSource(set_audio_source_command *ac)
{
    int error = 0;

    switch(ac->as) {
    case AUDIO_SOURCE_DEFAULT:
    case AUDIO_SOURCE_MIC:
        mAudioInputMIO = new AndroidAudioInput();
        if(mAudioInputMIO){
            LOGV("create mio input audio");
            mAudioNode = PvmfMediaInputNodeFactory::Create(mAudioInputMIO);
            if(mAudioNode){
                break;
            }
            else{
            // do nothing, let it go in default case
            }
        }
        else{
        // do nothing, let it go in default case
        }
    default:
        commandFailed(ac);
        return;
    }

    OSCL_TRY(error, mAuthor->AddDataSource(*mAudioNode, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetVideoSource(set_video_source_command *ac)
{
    int error = 0;

    switch(ac->vs) {
    case VIDEO_SOURCE_DEFAULT:
    case VIDEO_SOURCE_CAMERA:
        mVideoInputMIO = new AndroidCameraInput();
        if(mVideoInputMIO){
            LOGV("create mio input video");
            mVideoNode = PvmfMediaInputNodeFactory::Create(mVideoInputMIO);
            if(mVideoNode){
                break;
            }
            else{
            // do nothing, let it go in default case
            }
        }
        else{
        // do nothing, let it go in default case
        }
    default:
        commandFailed(ac);
        return;
    }

    OSCL_TRY(error, mAuthor->AddDataSource(*mVideoNode, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetOutputFormat(set_output_format_command *ac)
{
    int error = 0;
    OSCL_HeapString<OsclMemAllocator> iComposerMimeType;

    if (ac->of == OUTPUT_FORMAT_DEFAULT)
        ac->of = OUTPUT_FORMAT_THREE_GPP;

    switch(ac->of) {
    case OUTPUT_FORMAT_THREE_GPP:
        iComposerMimeType = "/x-pvmf/ff-mux/3gp";
        break;

    case OUTPUT_FORMAT_MPEG_4:
        iComposerMimeType = "/x-pvmf/ff-mux/mp4";
        break;

    default:
        commandFailed(ac);
        return;
    }

    OSCL_TRY(error, mAuthor->SelectComposer(iComposerMimeType, mComposerConfig, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::media_track_added(status_t status, void *cookie)
{
    AuthorDriver *ad = (AuthorDriver *)cookie;

    ad->mSyncSem->Signal();
}

void AuthorDriver::handleSetAudioEncoder(set_audio_encoder_command *ac)
{
    int error = 0;
    OSCL_HeapString<OsclMemAllocator> iAudioEncoderMimeType;

    if (ac->ae == AUDIO_ENCODER_DEFAULT)
        ac->ae = AUDIO_ENCODER_AMR_NB;

    switch(ac->ae) {
    case AUDIO_ENCODER_AMR_NB:
        iAudioEncoderMimeType = "/x-pvmf/audio/encode/amr-nb";
        break;

    default:
        commandFailed(ac);
        return;
    }

    mAudioEncoder = ac->ae;

    OSCL_TRY(error, mAuthor->AddMediaTrack(*mAudioNode, iAudioEncoderMimeType, mSelectedComposer, mAudioEncoderConfig, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetVideoEncoder(set_video_encoder_command *ac)
{
    int error = 0;
    OSCL_HeapString<OsclMemAllocator> iVideoEncoderMimeType;

    if (ac->ve == VIDEO_ENCODER_DEFAULT)
        ac->ve = VIDEO_ENCODER_H263;

    switch(ac->ve) {
    case VIDEO_ENCODER_H263:
        iVideoEncoderMimeType = "/x-pvmf/video/encode/h263";
        break;

    case VIDEO_ENCODER_H264:
        iVideoEncoderMimeType = "/x-pvmf/video/encode/h264";
        break;

    case VIDEO_ENCODER_MPEG_4_SP:
        iVideoEncoderMimeType = "/x-pvmf/video/encode/mp4";
        break;

    default:
        commandFailed(ac);
        return;
    }
   
    mVideoEncoder = ac->ve;
 
    OSCL_TRY(error, mAuthor->AddMediaTrack(*mVideoNode, iVideoEncoderMimeType, mSelectedComposer, mVideoEncoderConfig, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetVideoSize(set_video_size_command *ac)
{
    if (mVideoInputMIO == NULL)
        return;

    mVideoWidth = ac->width;
    mVideoHeight = ac->height;

    ((AndroidCameraInput *)mVideoInputMIO)->SetFrameSize(ac->width, ac->height);
}

void AuthorDriver::handleSetVideoFrameRate(set_video_frame_rate_command *ac)
{
    if (mVideoInputMIO == NULL)
        return;

    mVideoFrameRate = ac->rate;

    ((AndroidCameraInput *)mVideoInputMIO)->SetFrameRate(ac->rate);
}

void AuthorDriver::handleSetPreviewSurface(set_preview_surface_command *ac)
{
    if (ac->surface == NULL) {
        commandFailed(ac);
        return;
    }

    if (mVideoInputMIO == NULL) {
        commandFailed(ac);
        return;
    }

    ((AndroidCameraInput *)mVideoInputMIO)->SetPreviewSurface(ac->surface);
}

void AuthorDriver::handleSetOutputFile(set_output_file_command *ac)
{
    int error = 0;

    if (!mComposerConfig) {
        commandFailed(ac);
        return;
    }

    PVMp4FFCNClipConfigInterface *config = OSCL_STATIC_CAST(PVMp4FFCNClipConfigInterface*, mComposerConfig);
    if (!config) {
        commandFailed(ac);
        return;
    }

    config->SetPresentationTimescale(1000);

    oscl_wchar output[512];
    OSCL_wHeapString<OsclMemAllocator> wFileName;
    oscl_UTF8ToUnicode(ac->path, strlen(ac->path), output, 512);
    wFileName.set(output, oscl_strlen(output));
    PVMFStatus ret = config->SetOutputFileName(wFileName);
    free(ac->path);
    if (ret != PVMFSuccess) {
        commandFailed(ac);
        return;
    }
}

void AuthorDriver::handlePrepare(author_command *ac)
{
    int error = 0;
    OSCL_TRY(error, mAuthor->Init(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleStart(author_command *ac)
{
    int error = 0;
    OSCL_TRY(error, mAuthor->Start(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleStop(author_command *ac)
{
    int error = 0;
    OSCL_TRY(error, mAuthor->Stop());
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
    OSCL_TRY(error, mAuthor->Reset());
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
    if(mAudioNode){
        OSCL_TRY(error, mAuthor->RemoveDataSource(*mAudioNode));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
    }
    if(mVideoNode){
        OSCL_TRY(error, mAuthor->RemoveDataSource(*mVideoNode));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
    }
    OSCL_TRY(error, mAuthor->Close(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleReset(author_command *ac)
{
    int error = 0;
    OSCL_TRY(error, mAuthor->Reset(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleQuit(author_command *ac)
{
    OsclExecScheduler *sched = OsclExecScheduler::Current();
    sched->StopScheduler();
}

/*static*/ int AuthorDriver::startAuthorThread(void *cookie)
{
    prctl(PR_SET_NAME, (unsigned long) "PV author", 0, 0, 0);
    AuthorDriver *ed = (AuthorDriver *)cookie;
    return ed->authorThread();
}

int AuthorDriver::authorThread()
{
    int error;

    pv_global_init();

    OsclScheduler::Init("AndroidAuthorDriver");
    LOGV("Create author ...");
    OSCL_TRY(error, mAuthor = PVAuthorEngineFactory::CreateAuthor(this, this, this));
    if (error) {
        // Just crash the first time someone tries to use it for now?
        mAuthor = NULL;
        mSyncSem->Signal();
        return -1;
    }

    AddToScheduler();
    PendForExec();

    OsclExecScheduler *sched = OsclExecScheduler::Current();
    sched->StartScheduler(mSyncSem);
    LOGV("Delete Author");
    PVAuthorEngineFactory::DeleteAuthor(mAuthor);

    if (mVideoNode) {
        PvmfMediaInputNodeFactory::Delete(mVideoNode);
        mVideoNode = NULL;
        delete mVideoInputMIO;
        mVideoInputMIO = NULL;
    }

    if (mAudioNode) {
        PvmfMediaInputNodeFactory::Delete(mAudioNode);
        mAudioNode = NULL;
        delete mAudioInputMIO;
        mAudioInputMIO = NULL;
    }

    // Let the destructor know that we're out
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

   //moved below delete this, similar code on playerdriver.cpp caused a crash.
   //cleanup of oscl should happen at the end.
    pv_global_cleanup();
    return 0;
}

/*static*/ void AuthorDriver::syncCompletion(status_t s, void *cookie)
{
    AuthorDriver *ed = (AuthorDriver *)cookie;
    ed->mSyncStatus = s;
    ed->mSyncSem->Signal();
}

void AuthorDriver::CommandCompleted(const PVCmdResponse& aResponse)
{
    author_command *ac = (author_command *)aResponse.GetContext();
    status_t s = 0;

    // XXX do we want to make this illegal?  combo commands like
    // SETUP_DEFAULT_SINKS will need some changing
    if (ac == NULL) {
        // LOGD("ac == NULL\n");
        return;
    }

    if (ac->which == AUTHOR_SET_OUTPUT_FORMAT) {
        mSelectedComposer = aResponse.GetResponseData();
    }

    if (ac->which == AUTHOR_SET_VIDEO_ENCODER) {
        switch(mVideoEncoder) {
        case VIDEO_ENCODER_H263: {
            PVMp4H263EncExtensionInterface *config = OSCL_STATIC_CAST(PVMp4H263EncExtensionInterface*,
                                                                      mVideoEncoderConfig);
            if (config) {
                config->SetNumLayers(1);
                config->SetOutputBitRate(0, 58850);    //XXX
                config->SetOutputFrameSize(0, mVideoWidth, mVideoHeight);
                config->SetOutputFrameRate(0, mVideoFrameRate);
                config->SetIFrameInterval(mVideoFrameRate);
            }
            } break;

        default:
            break;
        }
    }

    if (ac->which == AUTHOR_SET_AUDIO_ENCODER) {
        switch(mAudioEncoder) {
        case AUDIO_ENCODER_AMR_NB: {
            PVAMREncExtensionInterface *config = OSCL_STATIC_CAST(PVAMREncExtensionInterface*,
                                                                  mAudioEncoderConfig);
            if (config) {
                config->SetMaxNumOutputFramesPerBuffer(10);
                config->SetOutputBitRate(GSM_AMR_5_15); // 5150 bps XXX
            }
            } break;

        default:
            break;
        }
    }

    // Translate the PVMF error codes into Android ones 
    switch(aResponse.GetCmdStatus()) {
    case PVMFSuccess: s = android::OK; break;
    case PVMFPending: *(char *)0 = 0; break; /* XXX assert */
    default: s = android::UNKNOWN_ERROR;
    }

    // Call the user's requested completion function
    ac->comp(s, ac->cookie);

    delete ac;
}

void AuthorDriver::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    printf("HandleErrorEvent\n");
}

void AuthorDriver::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    PVInterface* iface = (PVInterface*)(aEvent.GetEventExtensionInterface());

    if (iface == NULL)
        return;

    switch(aEvent.GetEventType()) {
    case PVMFInfoPositionStatus:
    default:
        break;
    }
}

status_t AuthorDriver::getMaxAmplitude(int *max)
{
    if (!mAudioInputMIO) {
        return android::UNKNOWN_ERROR;
    }
    AndroidAudioInput *audioInput = static_cast<AndroidAudioInput*>(mAudioInputMIO);
    *max = audioInput->maxAmplitude();
    return android::OK;
}




