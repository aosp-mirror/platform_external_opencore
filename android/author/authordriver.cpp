/*
 * Copyright (C) 2008, The Android Open Source Project
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
#define LOG_TAG "AuthorDriver"

#include <unistd.h>
#include <media/thread_init.h>
#include <ui/ISurface.h>
#include <ui/ICamera.h>
#include <cutils/properties.h> // for property_get
#include "authordriver.h"
#include "pv_omxcore.h"
#include <sys/prctl.h>
#include "pvmf_composer_size_and_duration.h"
#include "android_camera_input.h"

using namespace android;

AuthorDriverWrapper::AuthorDriverWrapper()
{
    mAuthorDriver = new AuthorDriver();
}

void AuthorDriverWrapper::resetAndClose()
{
    LOGV("resetAndClose");
    mAuthorDriver->enqueueCommand(new author_command(AUTHOR_RESET), NULL, NULL);
    mAuthorDriver->enqueueCommand(new author_command(AUTHOR_REMOVE_VIDEO_SOURCE), NULL, NULL);
    mAuthorDriver->enqueueCommand(new author_command(AUTHOR_REMOVE_AUDIO_SOURCE), NULL, NULL);
    mAuthorDriver->enqueueCommand(new author_command(AUTHOR_CLOSE), NULL, NULL);
}

AuthorDriverWrapper::~AuthorDriverWrapper()
{
    LOGV("Destructor");
    if (mAuthorDriver) {
        // set the authoring engine to the IDLE state.
        PVAEState state = mAuthorDriver->getAuthorEngineState();
        LOGV("state(%d)", state);
        switch (state) {
        case PVAE_STATE_IDLE:
            break;

        case PVAE_STATE_RECORDING:
            mAuthorDriver->enqueueCommand(new author_command(AUTHOR_STOP),  NULL, NULL);
            resetAndClose();
            break;

        default:
            resetAndClose();
            break;
        }

        // now it is safe to quit.
        author_command *ac = new author_command(AUTHOR_QUIT);
        enqueueCommand(ac, NULL, NULL); // will wait on mSyncSem, signaled by author thread
        delete ac; // have to delete this manually because CommandCompleted won't be called
    }
}

status_t AuthorDriverWrapper::getMaxAmplitude(int *max)
{
    if (mAuthorDriver) {
        return mAuthorDriver->getMaxAmplitude(max);
    }
    return NO_INIT;
}

status_t AuthorDriverWrapper::enqueueCommand(author_command *ac, media_completion_f comp, void *cookie)
{
    if (mAuthorDriver) {
        return mAuthorDriver->enqueueCommand(ac, comp, cookie);
    }
    return NO_INIT;
}

status_t AuthorDriverWrapper::setListener(const sp<IMediaPlayerClient>& listener) {
    if (mAuthorDriver) {
    return mAuthorDriver->setListener(listener);
    }
    return NO_INIT;
}

AuthorDriver::AuthorDriver()
    : OsclActiveObject(OsclActiveObject::EPriorityNominal, "AuthorDriver"),
    mAuthor(NULL),
    mVideoInputMIO(NULL),
    mVideoNode(NULL),
    mAudioNode(NULL),
    mSelectedComposer(NULL),
    mComposerConfig(NULL),
    mVideoEncoderConfig(NULL),
    mAudioEncoderConfig(NULL),
    mVideoWidth(ANDROID_DEFAULT_FRAME_WIDTH),
    mVideoHeight(ANDROID_DEFAULT_FRAME_HEIGHT),
    mVideoFrameRate((int)ANDROID_DEFAULT_FRAME_RATE),
    mVideoEncoder(VIDEO_ENCODER_DEFAULT),
    mOutputFormat(OUTPUT_FORMAT_DEFAULT),
    mAudioEncoder(AUDIO_ENCODER_DEFAULT),
    mSamplingRate(0),
    mNumberOfChannels(0),
    mAudio_bitrate_setting(0),
    mVideo_bitrate_setting(0),
    ifpOutput(NULL)
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
    if (mAuthor == NULL) {
        return NO_INIT;
    }
    // If the user didn't specify a completion callback, we
    // are running in synchronous mode.
    if (comp == NULL) {
        ac->comp = AuthorDriver::syncCompletion;
        ac->cookie = this;
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
    delete ac;
}

// The OSCL scheduler calls this when we get to run (this should happen only
// when a command has been enqueued for us).
void AuthorDriver::Run()
{
    author_command *ac = dequeueCommand();
    if (ac == NULL) {
    LOGE("Unexpected NULL command");
    OSCL_LEAVE(PVMFErrArgument);
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
        return;

    case AUTHOR_SET_VIDEO_FRAME_RATE:
        handleSetVideoFrameRate((set_video_frame_rate_command *)ac);
        return;

    case AUTHOR_SET_PREVIEW_SURFACE:
        handleSetPreviewSurface((set_preview_surface_command *)ac);
        return;

    case AUTHOR_SET_CAMERA:
        handleSetCamera((set_camera_command *)ac);
        return;

    case AUTHOR_SET_OUTPUT_FILE:
        handleSetOutputFile((set_output_file_command *)ac);
        return;

    case AUTHOR_SET_PARAMETERS:
        handleSetParameters((set_parameters_command *)ac);
        return;

    case AUTHOR_REMOVE_VIDEO_SOURCE:
        handleRemoveVideoSource(ac);
        return;

    case AUTHOR_REMOVE_AUDIO_SOURCE:
        handleRemoveAudioSource(ac);
        return;

    case AUTHOR_PREPARE: handlePrepare(ac); break;
    case AUTHOR_START: handleStart(ac); break;
    case AUTHOR_STOP: handleStop(ac); break;
    case AUTHOR_CLOSE: handleClose(ac); break;
    case AUTHOR_RESET: handleReset(ac); break;
    case AUTHOR_QUIT: handleQuit(ac); return;

    default:
    LOGE("Unknown author command: %d", ac->which);
    OSCL_LEAVE(PVMFErrArgument);
        break;
    }

 }

void AuthorDriver::commandFailed(author_command *ac)
{
    LOGE("Command (%d) failed", ac->which);
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

    mAudioInputMIO = new AndroidAudioInput(ac->as);
    if (mAudioInputMIO != NULL) {
        LOGV("create mio input audio");
        mAudioNode = PvmfMediaInputNodeFactory::Create(static_cast<PvmiMIOControl *>(mAudioInputMIO.get()));
        if (mAudioNode == NULL) {
            commandFailed(ac);
            return;
        }
    }

    OSCL_TRY(error, mAuthor->AddDataSource(*mAudioNode, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetVideoSource(set_video_source_command *ac)
{
    int error = 0;

    switch(ac->vs) {
    case VIDEO_SOURCE_DEFAULT:
    case VIDEO_SOURCE_CAMERA: {
        AndroidCameraInput* cameraInput = new AndroidCameraInput();
        if (cameraInput) {
            LOGV("create mio input video");
            mVideoNode = PvmfMediaInputNodeFactory::Create(cameraInput);
            if (mVideoNode) {
                // pass in the application supplied camera object
                if (mCamera == 0 ||
                    (mCamera != 0 && cameraInput->SetCamera(mCamera) == PVMFSuccess)) {
                    mVideoInputMIO = cameraInput;
                    break;
                }
            }
            delete cameraInput;
        }
        commandFailed(ac);
        return;
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

    if (ac->of == OUTPUT_FORMAT_DEFAULT) {
        ac->of = OUTPUT_FORMAT_THREE_GPP;
    }

    OSCL_HeapString<OsclMemAllocator> mComposerMimeType;

    switch(ac->of) {
    case OUTPUT_FORMAT_THREE_GPP:
        mComposerMimeType = "/x-pvmf/ff-mux/3gp";
        break;

    case OUTPUT_FORMAT_MPEG_4:
        mComposerMimeType = "/x-pvmf/ff-mux/mp4";
        break;

    //case OUTPUT_FORMAT_RAW_AMR: //"duplicate case value" keep this to be backward compatible
    case OUTPUT_FORMAT_AMR_NB:
        mComposerMimeType = "/x-pvmf/ff-mux/amr-nb";
        break;

    case OUTPUT_FORMAT_AMR_WB:
        mComposerMimeType = "/x-pvmf/ff-mux/amr-wb";
        break;

    case OUTPUT_FORMAT_AAC_ADIF:
        mComposerMimeType = "/x-pvmf/ff-mux/adif";
        break;

    case OUTPUT_FORMAT_AAC_ADTS:
        mComposerMimeType = "/x-pvmf/ff-mux/adts";
        break;

    default:
        LOGE("Ln %d unsupported file format: %d", __LINE__, ac->of);
        commandFailed(ac);
        return;
    }

    mOutputFormat = ac->of;

    OSCL_TRY(error, mAuthor->SelectComposer(mComposerMimeType, mComposerConfig, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::media_track_added(status_t status, void *cookie)
{
    AuthorDriver *ad = (AuthorDriver *)cookie;

    ad->mSyncSem->Signal();
}

void AuthorDriver::handleSetAudioEncoder(set_audio_encoder_command *ac)
{
    LOGV("AuthorDriver::handleSetAudioEncoder(%d)", ac->ae);

    int error = 0;
    OSCL_HeapString<OsclMemAllocator> iAudioEncoderMimeType;

    if (ac->ae == AUDIO_ENCODER_DEFAULT)
        ac->ae = AUDIO_ENCODER_AMR_NB;

    switch(ac->ae) {
    case AUDIO_ENCODER_AMR_NB:
        iAudioEncoderMimeType = "/x-pvmf/audio/encode/amr-nb";
        // AMR_NB only supports 8kHz sampling rate
        if (mSamplingRate == 0)
        {
            // Sampling rate not set, use the default
            mSamplingRate = 8000;
        }
        else if (mSamplingRate != 8000)
        {
            LOGE("Only valid sampling rate for AMR_NB is 8kHz.");
            commandFailed(ac);
            return;
        }

        // AMR_NB only supports mono (IE 1 channel)
        if (mNumberOfChannels == 0)
        {
            // Number of channels not set, use the default
            mNumberOfChannels = 1;
        }
        else if (mNumberOfChannels != 1)
        {
            LOGE("Only valid number of channels for ANR_NB is 1.");
            commandFailed(ac);
            return;
        }
        break;

    case AUDIO_ENCODER_AMR_WB:
        iAudioEncoderMimeType = "/x-pvmf/audio/encode/amr-wb";
        // AMR_WB only supports 16kHz sampling rate
        if (mSamplingRate == 0)
        {
            // Sampling rate not set, use the default
            mSamplingRate = 16000;
        }
        else if (mSamplingRate != 16000)
        {
            LOGE("Only valid sampling rate for AMR_WB is 16kHz.");
            commandFailed(ac);
            return;
        }

        // AMR_WB only supports mono (IE 1 channel)
        if (mNumberOfChannels == 0)
        {
            // Number of channels not set, use the default
            mNumberOfChannels = 1;
        }
        else if (mNumberOfChannels != 1)
        {
            LOGE("Only valid number of channels for ANR_WB is 1.");
            commandFailed(ac);
            return;
        }
        break;

    case AUDIO_ENCODER_AAC:
        // Check the sampling rate
        if (mSamplingRate == 0)
        {
            // No sampling rate set, use the default
            mSamplingRate = DEFAULT_AUDIO_SAMPLING_RATE;
        }
        // Check the number of channels
        if (mNumberOfChannels == 0)
        {
            // Number of channels not set, use the default
            mNumberOfChannels = DEFAULT_AUDIO_NUMBER_OF_CHANNELS;
        }

        // Is file container type AAC-ADIF?
        if(mOutputFormat == OUTPUT_FORMAT_AAC_ADIF)
        {
            // This is an audio only file container, set the correct encoder
            iAudioEncoderMimeType = "/x-pvmf/audio/encode/aac/adif";
        }
        // AAC-ADTS?
        else if (mOutputFormat == OUTPUT_FORMAT_AAC_ADTS)
        {
            // This is an audio only file container, set the correct encoder
            iAudioEncoderMimeType = "/x-pvmf/audio/encode/aac/adts";
        }
        // else MPEG4 or 3GPP container ... use AAC-RAW
        else
        {
            // AAC for mixed audio/video containers
            iAudioEncoderMimeType = "/x-pvmf/audio/encode/X-MPEG4-AUDIO";
        }
        break;

    case AUDIO_ENCODER_AAC_PLUS:
    case AUDIO_ENCODER_EAAC_PLUS:
        // Added for future use.  Not currently supported by pvauthor
        LOGE("AAC_PLUS and EAAC_PLUS audio formats are currently not supported");
        // NO BREAK!  Fall through from the unsupported AAC_PLUS and EAAC_PLUS cases into default case
    default:
        commandFailed(ac);
        return;
    }

    LOGV("AuthorDriver::handleSetAudioEncoder() set %d %d \"%s\"", mSamplingRate, mNumberOfChannels, iAudioEncoderMimeType.get_cstr());

    // Set the sampling rate and number of channels
    if (!mAudioInputMIO->setAudioSamplingRate(mSamplingRate))
    {
        LOGE("Failed to set the sampling rate %d", mSamplingRate);
        commandFailed(ac);
        return;
    }
    if (!mAudioInputMIO->setAudioNumChannels(mNumberOfChannels))
    {
        LOGE("Failed to set the number of channels %d", mNumberOfChannels);
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
    // Set video encoding frame rate and video frame size only when the
    // video input MIO is set.
    if (mVideoInputMIO) {
        if (mVideoFrameRate == 0) {
            mVideoFrameRate = DEFAULT_VIDEO_FRAME_RATE;
        }
        clipVideoFrameRate();
        ((AndroidCameraInput *)mVideoInputMIO)->SetFrameRate(mVideoFrameRate);

        if (mVideoWidth == 0) {
            mVideoWidth = DEFAULT_VIDEO_WIDTH;
        }
        if (mVideoHeight == 0) {
            mVideoHeight = DEFAULT_VIDEO_HEIGHT;
        }
        clipVideoFrameSize();
        ((AndroidCameraInput *)mVideoInputMIO)->SetFrameSize(mVideoWidth, mVideoHeight);
    }

    OSCL_TRY(error, mAuthor->AddMediaTrack(*mVideoNode, iVideoEncoderMimeType, mSelectedComposer, mVideoEncoderConfig, ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleSetVideoSize(set_video_size_command *ac)
{
    if (mVideoInputMIO == NULL) {
        LOGE("camera MIO is NULL");
        commandFailed(ac);
        return;
    }

    mVideoWidth = ac->width;
    mVideoHeight = ac->height;
    FinishNonAsyncCommand(ac);
}

void AuthorDriver::handleSetVideoFrameRate(set_video_frame_rate_command *ac)
{
    if (mVideoInputMIO == NULL) {
        LOGE("camera MIO is NULL");
        commandFailed(ac);
        return;
    }

    mVideoFrameRate = ac->rate;
    FinishNonAsyncCommand(ac);
}

void AuthorDriver::handleSetCamera(set_camera_command *ac)
{
    LOGV("mCamera = %p", ac->camera.get());
    mCamera = ac->camera;
    FinishNonAsyncCommand(ac);
}

void AuthorDriver::handleSetPreviewSurface(set_preview_surface_command *ac)
{
    if (mVideoInputMIO == NULL) {
        LOGE("camera MIO is NULL");
        commandFailed(ac);
        return;
    }

    ((AndroidCameraInput *)mVideoInputMIO)->SetPreviewSurface(ac->surface);
    FinishNonAsyncCommand(ac);
}

void AuthorDriver::handleSetOutputFile(set_output_file_command *ac)
{
    PVMFStatus ret = PVMFFailure;
    PvmfFileOutputNodeConfigInterface *config = NULL;

    if (!mComposerConfig) goto exit;

    config = OSCL_STATIC_CAST(PvmfFileOutputNodeConfigInterface*, mComposerConfig);
    if (!config) goto exit;

    ifpOutput = fdopen(ac->fd, "wb");
    if (NULL == ifpOutput) {
        LOGE("Ln %d fopen() error", __LINE__);
        goto exit;
    }

    if (( OUTPUT_FORMAT_AMR_NB == mOutputFormat ) || ( OUTPUT_FORMAT_AMR_WB == mOutputFormat ) ||
        ( OUTPUT_FORMAT_AAC_ADIF == mOutputFormat ) || ( OUTPUT_FORMAT_AAC_ADTS == mOutputFormat )) {
        PvmfFileOutputNodeConfigInterface *config = OSCL_DYNAMIC_CAST(PvmfFileOutputNodeConfigInterface*, mComposerConfig);
        if (!config) goto exit;

        ret = config->SetOutputFileDescriptor(&OsclFileHandle(ifpOutput));
    }  else if((OUTPUT_FORMAT_THREE_GPP == mOutputFormat) || (OUTPUT_FORMAT_MPEG_4 == mOutputFormat)){
        PVMp4FFCNClipConfigInterface *config = OSCL_DYNAMIC_CAST(PVMp4FFCNClipConfigInterface*, mComposerConfig);
        if (!config) goto exit;

        config->SetPresentationTimescale(1000);
        ret = config->SetOutputFileDescriptor(&OsclFileHandle(ifpOutput));
    }


exit:
    if (ret == PVMFSuccess) {
        FinishNonAsyncCommand(ac);
    } else {
        LOGE("Ln %d SetOutputFile() error", __LINE__);
    if (ifpOutput) {
        fclose(ifpOutput);
        ifpOutput = NULL;
    }
        commandFailed(ac);
    }
}

PVMFStatus AuthorDriver::setMaxDurationOrFileSize(
        int64_t limit, bool limit_is_duration) {
    PVInterface *interface;
    PvmfComposerSizeAndDurationInterface *durationConfig;

    if (limit > 0xffffffff) {
        // PV API expects this to fit in a uint32.
        return PVMFErrArgument;
    }

    if (!mComposerConfig) {
        return PVMFFailure;
    }

    mComposerConfig->queryInterface(
            PvmfComposerSizeAndDurationUuid, interface);

    durationConfig =
        OSCL_DYNAMIC_CAST(PvmfComposerSizeAndDurationInterface *, interface);

    if (!durationConfig) {
        return PVMFFailure;
    }

    PVMFStatus ret;
    if (limit_is_duration) {
        // SetMaxDuration's first parameter is a boolean "enable", we enable
        // enforcement of the maximum duration if it's (strictly) positive,
        // otherwise we take it to imply disabling.
        ret = durationConfig->SetMaxDuration(
                limit > 0, static_cast<uint32>(limit));
    } else {
        // SetMaxFileSize's first parameter is a boolean "enable", we enable
        // enforcement of the maximum filesize if it's (strictly) positive,
        // otherwise we take it to imply disabling.
        ret = durationConfig->SetMaxFileSize(
                limit > 0, static_cast<uint32>(limit));
    }

    durationConfig->removeRef();
    durationConfig = NULL;

    return ret;
}

PVMFStatus AuthorDriver::setParamAudioSamplingRate(int64_t aSamplingRate)
{
    // Do a rough check on the incoming sampling rate
    if ((aSamplingRate < MIN_AUDIO_SAMPLING_RATE) || (aSamplingRate > MAX_AUDIO_SAMPLING_RATE))
    {
        LOGE("setParamAudioSamplingRate() invalid sampling rate.");
        return PVMFErrArgument;
    }

    mSamplingRate = aSamplingRate;
    LOGV("setParamAudioSamplingRate() set sampling rate %d", mSamplingRate);
    return PVMFSuccess;
}


PVMFStatus AuthorDriver::setParamAudioNumberOfChannels(int64_t aNumberOfChannels)
{
    // Check the number of channels
    if ((aNumberOfChannels < MIN_AUDIO_NUMBER_OF_CHANNELS) || (aNumberOfChannels > MAX_AUDIO_NUMBER_OF_CHANNELS))
    {
        LOGE("setParamAudioNumberOfChannels() invalid number of channels.");
        return PVMFErrArgument;
    }

    mNumberOfChannels = aNumberOfChannels;
    LOGV("setParamAudioNumberOfChannels() set num channels %d", mNumberOfChannels);
    return PVMFSuccess;
}

PVMFStatus AuthorDriver::setParamAudioEncodingBitrate(int64_t aAudioBitrate)
{
    // Map the incoming audio bitrate settings
    if ((aAudioBitrate < MIN_AUDIO_BITRATE_SETTING) || (aAudioBitrate > MAX_AUDIO_BITRATE_SETTING))
    {
        LOGE("setParamAudioEncodingBitrate() invalid audio bitrate.  Set call ignored.");
        return PVMFErrArgument;
    }

    // Set the audio bitrate
    mAudio_bitrate_setting = aAudioBitrate;

    LOGV("setParamAudioEncodingBitrate() %d", mAudio_bitrate_setting);
    return PVMFSuccess;
}

// Attempt to parse an int64 literal optionally surrounded by whitespace,
// returns true on success, false otherwise.
static bool safe_strtoi64(const char *s, int64 *val) {
    char *end;
    *val = strtoll(s, &end, 10);

    if (end == s || errno == ERANGE) {
        return false;
    }

    // Skip trailing whitespace
    while (isspace(*end)) {
        ++end;
    }

    // For a successful return, the string must contain nothing but a valid
    // int64 literal optionally surrounded by whitespace.

    return *end == '\0';
}

// Trim both leading and trailing whitespace from the given string.
static void TrimString(String8 *s) {
    size_t num_bytes = s->bytes();
    const char *data = s->string();

    size_t leading_space = 0;
    while (leading_space < num_bytes && isspace(data[leading_space])) {
        ++leading_space;
    }

    size_t i = num_bytes;
    while (i > leading_space && isspace(data[i - 1])) {
        --i;
    }

    s->setTo(String8(&data[leading_space], i - leading_space));
}

PVMFStatus AuthorDriver::setParameter(
        const String8& key, const String8& value) {
    if (key == "max-duration") {
        int64_t max_duration_ms;
        if (safe_strtoi64(value.string(), &max_duration_ms)) {
            return setMaxDurationOrFileSize(
                    max_duration_ms, true /* limit_is_duration */);
        }
    } else if (key == "max-filesize") {
        int64_t max_filesize_bytes;
        if (safe_strtoi64(value.string(), &max_filesize_bytes)) {
            return setMaxDurationOrFileSize(
                    max_filesize_bytes, false /* limit is filesize */);
        }
    } else if (key == "audio-param-sampling-rate") {
        int64_t sampling_rate;
        if (safe_strtoi64(value.string(), &sampling_rate)) {
            return setParamAudioSamplingRate(sampling_rate);
        }
    } else if (key == "audio-param-number-of-channels") {
        int64_t number_of_channels;
        if (safe_strtoi64(value.string(), &number_of_channels)) {
            return setParamAudioNumberOfChannels(number_of_channels);
        }
    } else if (key == "audio-param-encoding-bitrate") {
        int64_t audio_bitrate;
        if (safe_strtoi64(value.string(), &audio_bitrate)) {
            return setParamAudioEncodingBitrate(audio_bitrate);
        }
    } else if (key == "video-param-encoding-bitrate") {
        int64_t video_bitrate;
        if (safe_strtoi64(value.string(), &video_bitrate)) {
            return setParamVideoEncodingBitrate(video_bitrate);
        }
    }

    // Return error if the key wasnt found
    LOGE("AuthorDriver::setParameter() unrecognized key \"%s\"", key.string());
    return PVMFErrArgument;
}

PVMFStatus AuthorDriver::setParamVideoEncodingBitrate(int64_t aVideoBitrate)
{
    if (aVideoBitrate <= 0)
    {
        LOGE("setParamVideoEncodingBitrate() invalid video bitrate (%lld).  Set call ignored.", aVideoBitrate);
        return PVMFErrArgument;
    }

    mVideo_bitrate_setting = aVideoBitrate;
    LOGD("setParamVideoEncodingBitrate() %d", mVideo_bitrate_setting);
    return PVMFSuccess;
}

// Applies the requested parameters, completes either successfully or stops
// application of parameters upon encountering the first error, finishing the
// transaction with the failure result of that initial failure.
void AuthorDriver::handleSetParameters(set_parameters_command *ac) {
    PVMFStatus ret = PVMFSuccess;

    const char *params = ac->params().string();
    const char *key_start = params;
    for (;;) {
        const char *equal_pos = strchr(key_start, '=');
        if (equal_pos == NULL) {
            // This key is missing a value.

            ret = PVMFErrArgument;
            break;
        }

        String8 key(key_start, equal_pos - key_start);
        TrimString(&key);

        if (key.length() == 0) {
            ret = PVMFErrArgument;
            break;
        }

        const char *value_start = equal_pos + 1;
        const char *semicolon_pos = strchr(value_start, ';');
        String8 value;
        if (semicolon_pos == NULL) {
            value.setTo(value_start);
        } else {
            value.setTo(value_start, semicolon_pos - value_start);
        }

        ret = setParameter(key, value);

        if (ret != PVMFSuccess) {
            LOGE("setParameter(%s = %s) failed with result %d",
                 key.string(), value.string(), ret);
            break;
        }

        if (semicolon_pos == NULL) {
            break;
        }

        key_start = semicolon_pos + 1;
    }

    if (ret == PVMFSuccess) {
        FinishNonAsyncCommand(ac);
    } else {
        LOGE("Ln %d handleSetParameters(\"%s\") error", __LINE__, params);
    commandFailed(ac);
    }
}

void AuthorDriver::handlePrepare(author_command *ac)
{
    LOGV("handlePrepare");
    int error = 0;
    OSCL_TRY(error, mAuthor->Init(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleStart(author_command *ac)
{
    LOGV("handleStart");
    int error = 0;
    OSCL_TRY(error, mAuthor->Start(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleStop(author_command *ac)
{
    LOGV("handleStop");
    int error = 0;
    OSCL_TRY(error, mAuthor->Stop(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleClose(author_command *ac)
{
    LOGV("handleClose");
    int error = 0;
    OSCL_TRY(error, mAuthor->Close(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleReset(author_command *ac)
{
    LOGV("handleReset");
    removeConfigRefs(ac);
    int error = 0;
    OSCL_TRY(error, mAuthor->Reset(ac));
    OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
}

void AuthorDriver::handleRemoveVideoSource(author_command *ac)
{
    LOGV("handleRemoveVideoSource");
    if (mVideoNode) {
        int error = 0;
        OSCL_TRY(error, mAuthor->RemoveDataSource(*mVideoNode, ac));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
    } else {
       FinishNonAsyncCommand(ac);
    }
}

void AuthorDriver::handleRemoveAudioSource(author_command *ac)
{
    LOGV("handleRemoveAudioSource");
    if (mAudioNode) {
        int error = 0;
        OSCL_TRY(error, mAuthor->RemoveDataSource(*mAudioNode, ac));
        OSCL_FIRST_CATCH_ANY(error, commandFailed(ac));
    } else {
        FinishNonAsyncCommand(ac);
    }
}

void AuthorDriver::removeConfigRefs(author_command *ac)
{
    LOGV("removeConfigRefs");

    if (mComposerConfig) {
        mComposerConfig->removeRef();
        mComposerConfig = NULL;
    }
    if (mVideoEncoderConfig) {
        mVideoEncoderConfig->removeRef();
        mVideoEncoderConfig = NULL;
    }
    if (mAudioEncoderConfig) {
        mAudioEncoderConfig->removeRef();
        mAudioEncoderConfig = NULL;
    }
}

void AuthorDriver::handleQuit(author_command *ac)
{
    LOGV("handleQuit");
    OsclExecScheduler *sched = OsclExecScheduler::Current();
    sched->StopScheduler();
}

/*static*/ int AuthorDriver::startAuthorThread(void *cookie)
{
    prctl(PR_SET_NAME, (unsigned long) "PV author", 0, 0, 0);
    AuthorDriver *ed = (AuthorDriver *)cookie;
    return ed->authorThread();
}

void AuthorDriver::doCleanUp()
{
    if (ifpOutput) {
    fclose(ifpOutput);
    ifpOutput = NULL;
    }

    if (mCamera != NULL) {
        mCamera.clear();
    }

    if (mVideoNode) {
        PvmfMediaInputNodeFactory::Delete(mVideoNode);
        mVideoNode = NULL;
        delete mVideoInputMIO;
        mVideoInputMIO = NULL;
    }

    if (mAudioNode) {
        PvmfMediaInputNodeFactory::Delete(mAudioNode);
        mAudioNode = NULL;
        mAudioInputMIO.clear();
    }
}

int AuthorDriver::authorThread()
{
    int error;

    LOGV("InitializeForThread");
    if (!InitializeForThread()) {
        LOGE("InitializeForThread failed");
        mAuthor = NULL;
        mSyncSem->Signal();
        return -1;
    }

    LOGV("OMX_MasterInit");
    OMX_MasterInit();

    OsclScheduler::Init("AndroidAuthorDriver");
    LOGV("Create author ...");
    OSCL_TRY(error, mAuthor = PVAuthorEngineFactory::CreateAuthor(this, this, this));
    if (error) {
        // Just crash the first time someone tries to use it for now?
        LOGE("authorThread init error");
        mAuthor = NULL;
        mSyncSem->Signal();
        return -1;
    }

    AddToScheduler();
    PendForExec();

    OsclExecScheduler *sched = OsclExecScheduler::Current();
    error = OsclErrNone;
    OSCL_TRY(error, sched->StartScheduler(mSyncSem));
    OSCL_FIRST_CATCH_ANY(error,
             // Some AO did a leave, log it
             LOGE("Author Engine AO did a leave, error=%d", error)
            );

    LOGV("Delete Author");
    PVAuthorEngineFactory::DeleteAuthor(mAuthor);
    mAuthor = NULL;


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
    OsclScheduler::Cleanup();
    OMX_MasterDeinit();
    UninitializeForThread();
    return 0;
}

/*static*/ void AuthorDriver::syncCompletion(status_t s, void *cookie)
{
    AuthorDriver *ed = (AuthorDriver *)cookie;
    ed->mSyncStatus = s;
    ed->mSyncSem->Signal();
}

// Backward compatible hardcoded video bit rate setting
// These bit rate settings are from the original work with
// QCOM's hardware encoders. Originally, anything above
// 420000 bps is not stable, and default low quality bit
// rate it set to 192000 bps. For those devices with
// media capabilities specified as system properties, these
// bit rate settings will not be used.
static int setVideoBitrateHeuristically(int videoWidth)
{
    int bitrate_setting = 192000;
    if (videoWidth >= 480) {
        bitrate_setting = 420000;
    } else if (videoWidth >= 352) {
        bitrate_setting = 360000;
    } else if (videoWidth >= 320) {
        bitrate_setting = 320000;
    }
    return bitrate_setting;
}


// Returns true on success
static bool getMinAndMaxValuesOfProperty(const char*propertyKey, int64& minValue, int64& maxValue)
{
    char value[PROPERTY_VALUE_MAX];
    int rc = property_get(propertyKey, value, 0);
    LOGV("property_get(): rc = %d, value=%s", rc, value);
    if (rc > 0) {
        char* b = strchr(value, ',');
        if (b == 0) {  // A pair of values separated by ","?
            return false;
        } else {
            String8 key(value, b - value);
            if (!safe_strtoi64(key.string(), &minValue) || !safe_strtoi64(b + 1, &maxValue)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

// Maps the given encoder to a system property key
// Returns true on success
static bool getPropertyKeyForVideoEncoder(video_encoder encoder, char* name, size_t len)
{
    switch(encoder) {
        case VIDEO_ENCODER_MPEG_4_SP:
            strncpy(name, "ro.media.enc.vid.m4v.", len);
            return true;
        case VIDEO_ENCODER_H264:
            strncpy(name, "ro.media.enc.vid.h264.", len);
            return true;
        case VIDEO_ENCODER_H263:
            strncpy(name, "ro.media.enc.vid.h263.", len);
            return true;
        default:
            LOGE("Failed to get system property key for video encoder(%d)", encoder);
            return false;
    }
}

// Retrieves the advertised video property range from system properties for the given encoder.
// If the encoder is not found, or the video property is not listed as a system property,
// default hardcoded min and max values will be used.
static void getSupportedPropertyRange(video_encoder encoder, const char* property, int64& min, int64& max)
{
    char videoEncoderName[PROPERTY_KEY_MAX];
    bool propertyKeyExists = getPropertyKeyForVideoEncoder(encoder, videoEncoderName, PROPERTY_KEY_MAX - 1);
    if (propertyKeyExists) {
        if ((strlen(videoEncoderName) + strlen(property) + 1) < PROPERTY_KEY_MAX) {  // Valid key length
            strcat(videoEncoderName, property);
        } else {
            propertyKeyExists = false;
        }
    }
    if (!propertyKeyExists || !getMinAndMaxValuesOfProperty(videoEncoderName, min, max)) {
        if (strcmp(property, "bps") == 0) {
            min = MIN_VIDEO_BITRATE_SETTING;
            max = MAX_VIDEO_BITRATE_SETTING;
        } else if (strcmp(property, "fps") == 0) {
            min = ANDROID_MIN_FRAME_RATE_FPS;
            max = ANDROID_MAX_FRAME_RATE_FPS;
        } else if (strcmp(property, "width") == 0) {
            min = ANDROID_MIN_ENCODED_FRAME_WIDTH;
            max = ANDROID_MAX_ENCODED_FRAME_WIDTH;
        } else if (strcmp(property, "height") == 0) {
            min = ANDROID_MIN_ENCODED_FRAME_HEIGHT;
            max = ANDROID_MAX_ENCODED_FRAME_HEIGHT;
        } else {
            LOGE("Unknown video property: %s", property);
            min = max = 0;
        }
        LOGW("Use default video %s range [%lld %lld]", property, min, max);
    }
}

static void getSupportedVideoBitRateRange(video_encoder encoder, int64& minBitRateBps, int64& maxBitRateBps)
{
    getSupportedPropertyRange(encoder, "bps", minBitRateBps, maxBitRateBps);
}

static void getSupportedVideoFrameRateRange(video_encoder encoder, int64& minFrameRateFps, int64& maxFrameRateFps)
{
    getSupportedPropertyRange(encoder, "fps", minFrameRateFps, maxFrameRateFps);
}

static void getSupportedVideoFrameWidthRange(video_encoder encoder, int64& minWidth, int64& maxWidth)
{
    getSupportedPropertyRange(encoder, "width", minWidth, maxWidth);
}

static void getSupportedVideoFrameHeightRange(video_encoder encoder, int64& minHeight, int64& maxHeight)
{
    getSupportedPropertyRange(encoder, "height", minHeight, maxHeight);
}

// Clips the intented video encoding rate so that it is
// within the advertised support range. Logs a warning if
// the intended bit rate is out of the range.
void AuthorDriver::clipVideoBitrate()
{
    int64 minBitrate, maxBitrate;
    getSupportedVideoBitRateRange(mVideoEncoder, minBitrate, maxBitrate);
    if (mVideo_bitrate_setting < minBitrate) {
        LOGW("Intended video encoding bit rate (%d bps) is too small and will be set to (%lld bps)", mVideo_bitrate_setting, minBitrate);
        mVideo_bitrate_setting = minBitrate;
    } else if (mVideo_bitrate_setting > maxBitrate) {
        LOGW("Intended video encoding bit rate (%d bps) is too large and will be set to (%lld bps)", mVideo_bitrate_setting, maxBitrate);
        mVideo_bitrate_setting = maxBitrate;
    }
}

void AuthorDriver::clipVideoFrameRate()
{
    int64 minFrameRate, maxFrameRate;
    getSupportedVideoFrameRateRange(mVideoEncoder, minFrameRate, maxFrameRate);
    if (mVideoFrameRate < minFrameRate) {
        LOGW("Intended video encoding frame rate (%d fps) is too small and will be set to (%lld fps)", mVideoFrameRate, minFrameRate);
        mVideoFrameRate = minFrameRate;
    } else if (mVideoFrameRate > maxFrameRate) {
        LOGW("Intended video encoding frame rate (%d fps) is too large and will be set to (%lld fps)", mVideoFrameRate, maxFrameRate);
        mVideoFrameRate = maxFrameRate;
    }
}

void AuthorDriver::clipVideoFrameWidth()
{
    int64 minFrameWidth, maxFrameWidth;
    getSupportedVideoFrameWidthRange(mVideoEncoder, minFrameWidth, maxFrameWidth);
    if (mVideoWidth < minFrameWidth) {
        LOGW("Intended video encoding frame width (%d) is too small and will be set to (%lld)", mVideoWidth, minFrameWidth);
        mVideoWidth = minFrameWidth;
    } else if (mVideoWidth > maxFrameWidth) {
        LOGW("Intended video encoding frame width (%d) is too large and will be set to (%lld)", mVideoWidth, maxFrameWidth);
        mVideoWidth = maxFrameWidth;
    }
}

void AuthorDriver::clipVideoFrameHeight()
{
    int64 minFrameHeight, maxFrameHeight;
    getSupportedVideoFrameHeightRange(mVideoEncoder, minFrameHeight, maxFrameHeight);
    if (mVideoHeight < minFrameHeight) {
        LOGW("Intended video encoding frame height (%d) is too small and will be set to (%lld)", mVideoHeight, minFrameHeight);
        mVideoHeight = minFrameHeight;
    } else if (mVideoHeight > maxFrameHeight) {
        LOGW("Intended video encoding frame height (%d) is too large and will be set to (%lld)", mVideoHeight, maxFrameHeight);
        mVideoHeight = maxFrameHeight;
    }
}

void AuthorDriver::clipVideoFrameSize()
{
    clipVideoFrameWidth();
    clipVideoFrameHeight();
}

void AuthorDriver::clipAACAudioBitrate()
{
    /*  ISO-IEC-13818-7 "Information technology.  Generic coding of moving
     *   pictures and associated audio information.  Part 7: Advanced Audio
     *   Coding (AAC)" section 8.2.2.3 defines a formula for the max audio
     *   bitrate based on the audio sampling rate.
     *   6144 (bit/block) / 1024 (samples/block) * sampling_freq * number_of_channels
     *
     *  This method is to calculate the max audio bitrate and clip the desired audio
     *   bitrate if it exceeds its max.
     */

    int32 calculated_audio_bitrate = 6 * mSamplingRate * mNumberOfChannels;
    if ((calculated_audio_bitrate > 0) &&
        (mAudio_bitrate_setting > calculated_audio_bitrate))
    {
        // Clip the bitrate setting
        LOGW("Intended audio bitrate (%d) exceeds max bitrate for sampling rate (%d).  Setting audio bitrate to its calculated max (%d)", mAudio_bitrate_setting, mSamplingRate, calculated_audio_bitrate);
        mAudio_bitrate_setting = calculated_audio_bitrate;
    }
}

void AuthorDriver::CommandCompleted(const PVCmdResponse& aResponse)
{
    author_command *ac = (author_command *)aResponse.GetContext();
    status_t s = aResponse.GetCmdStatus();
    LOGV("Command (%d) completed with status(%d)", ac? ac->which: -1, s);
    if (ac == NULL) {
        LOGE("CommandCompleted: Error - null author command!");
        return;
    }

    if (ac->which == AUTHOR_SET_OUTPUT_FORMAT) {
        mSelectedComposer = aResponse.GetResponseData();
    }

    if (ac->which == AUTHOR_SET_VIDEO_ENCODER) {
        switch(mVideoEncoder) {
        case VIDEO_ENCODER_H263:
        case VIDEO_ENCODER_MPEG_4_SP:
        case VIDEO_ENCODER_H264: {
            PVMp4H263EncExtensionInterface *config = OSCL_STATIC_CAST(PVMp4H263EncExtensionInterface*,
                                                                      mVideoEncoderConfig);
            if (config) {
                if (mVideo_bitrate_setting == 0) {
                    mVideo_bitrate_setting = setVideoBitrateHeuristically(mVideoWidth);
                    LOGW("Video encoding bit rate is set to %d bps", mVideo_bitrate_setting);
                }
                clipVideoBitrate();
                config->SetNumLayers(1);
                config->SetOutputBitRate(0, mVideo_bitrate_setting);
                config->SetOutputFrameSize(0, mVideoWidth, mVideoHeight);
                config->SetOutputFrameRate(0, mVideoFrameRate);
                config->SetIFrameInterval(ANDROID_DEFAULT_I_FRAME_INTERVAL);
            }
        } break;

        default:
            break;
        }
    }

    if (ac->which == AUTHOR_SET_AUDIO_ENCODER) {
        // Perform the cast to get the audio config interface
        PVAudioEncExtensionInterface *config = OSCL_STATIC_CAST(PVAudioEncExtensionInterface*,
                                                                mAudioEncoderConfig);

        if (config)
        {
            switch(mAudioEncoder) {
                case AUDIO_ENCODER_AMR_NB:
                case AUDIO_ENCODER_AMR_WB:
                    {
                        // Map the audio bitrate to an AMR discreet bitrate
                        PVMF_GSMAMR_Rate mAMRBitrate;
                        if(!MapAMRBitrate(mAudio_bitrate_setting, mAMRBitrate)) {
                            LOGE("Failed to map the audio bitrate to an AMR bitrate!  Using the defaults.");
                            if (mAudioEncoder == AUDIO_ENCODER_AMR_NB) {
                                mAMRBitrate = DEFAULT_AMR_NARROW_BAND_BITRATE_SETTING;
                            }
                            else { // Else use the default wideband setting
                               mAMRBitrate = DEFAULT_AMR_WIDE_BAND_BITRATE_SETTING;
                            }
                        }
                        config->SetOutputBitRate(mAMRBitrate);
                        config->SetMaxNumOutputFramesPerBuffer(10);
                    }
                    break;

                case AUDIO_ENCODER_AAC:
                    {
                        if (mAudio_bitrate_setting == 0) {
                            // Audio bitrate wasnt set, use the default
                            mAudio_bitrate_setting = DEFAULT_AUDIO_BITRATE_SETTING;
                        }
                        clipAACAudioBitrate();
                        config->SetOutputBitRate(mAudio_bitrate_setting);
                    }
                    break;

                case AUDIO_ENCODER_AAC_PLUS:
                case AUDIO_ENCODER_EAAC_PLUS:
                   LOGE("AAC_PLUS and EAAC_PLUS audio formats are currently not supported");
                   // We shouldn't get here.  The setAudioEncoder function should have rejected these.
                   //  These are currently not supported by pvauthor.
                   // NO BREAK!  Fall through from the unsupported AAC_PLUS and EAAC_PLUS cases into default case
                default:
                    break;
            } // End switch(mAudioEncoder)
        } // End if (config)
    } // End if (ac->which == AUTHOR_SET_AUDIO_ENCODER)

    // delete video and/or audio nodes to prevent memory leakage
    // when an authroing session is reused
    if (ac->which == AUTHOR_CLOSE) {
        LOGV("doCleanUp");
        doCleanUp();
    }

    // Translate the PVMF error codes into Android ones
    switch(s) {
        case PVMFSuccess: s = android::OK; break;
        case PVMFPending: *(char *)0 = 0; break; /* XXX assert */
        default:
            LOGE("Command %d completed with error %d",ac->which, s);
            // s = android::UNKNOWN_ERROR;
            // FIXME: Similar to mediaplayer, set the return status to
            //        something android specific. For now, use PVMF
            //        return codes as is.
    }

    // Call the user's requested completion function
    ac->comp(s, ac->cookie);

    delete ac;
}

bool AuthorDriver::MapAMRBitrate(int32 aAudioBitrate, PVMF_GSMAMR_Rate &anAMRBitrate)
{
    if ((mAudioEncoder != AUDIO_ENCODER_AMR_NB) &&
            (mAudioEncoder != AUDIO_ENCODER_AMR_WB)) {
        LOGE("AuthorDriver::MapAMRBitrate() encoder type is not AMR.");
        return false;
    }

    // Default to AMR_NB
    uint32 AMR_Index = 0;

    // Is this ARM_WB?
    if (mAudioEncoder == AUDIO_ENCODER_AMR_WB)
    {
        // Use the other side of the array
        AMR_Index = 1;
    }

    uint32 jj;
    for (jj = 0; jj < AMR_BITRATE_MAX_NUMBER_OF_ROWS; jj++)
    {
        if (aAudioBitrate < AMR_BITRATE_MAPPING_ARRAY[jj][AMR_Index].bitrate)
        {
            // Found a match!
            anAMRBitrate = AMR_BITRATE_MAPPING_ARRAY[jj][AMR_Index].actual;
            return true;
        }
    }

    // Failed to map the bitrate.  Return false and use the defaults.
    return false;
}

void AuthorDriver::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    LOGE("HandleErrorEvent(%d)", aEvent.GetEventType());

    if (mListener != NULL) {
    mListener->notify(
        MEDIA_RECORDER_EVENT_ERROR, MEDIA_RECORDER_ERROR_UNKNOWN,
        aEvent.GetEventType());
    }
}

static int GetMediaRecorderInfoCode(const PVAsyncInformationalEvent& aEvent) {
    switch (aEvent.GetEventType()) {
        case PVMF_COMPOSER_MAXDURATION_REACHED:
            return MEDIA_RECORDER_INFO_MAX_DURATION_REACHED;

        case PVMF_COMPOSER_MAXFILESIZE_REACHED:
            return MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED;

        default:
            return MEDIA_RECORDER_INFO_UNKNOWN;
    }
}

void AuthorDriver::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    const PVEventType event_type = aEvent.GetEventType();
    assert(!IsPVMFErrCode(event_type));
    if (IsPVMFInfoCode(event_type)) {
        LOGV("HandleInformationalEvent(%d:%s)",
             event_type, PVMFStatusToString(event_type));
    } else {
        LOGV("HandleInformationalEvent(%d)", event_type);
    }

    mListener->notify(
            MEDIA_RECORDER_EVENT_INFO,
            GetMediaRecorderInfoCode(aEvent),
            aEvent.GetEventType());
}

status_t AuthorDriver::setListener(const sp<IMediaPlayerClient>& listener) {
    mListener = listener;

    return android::OK;
}

status_t AuthorDriver::getMaxAmplitude(int *max)
{
    if (mAudioInputMIO == NULL) {
        return android::UNKNOWN_ERROR;
    }
    *max = mAudioInputMIO->maxAmplitude();
    return android::OK;
}

PVAEState AuthorDriver::getAuthorEngineState()
{
    if (mAuthor) {
        return mAuthor->GetPVAuthorState();
    }
    return PVAE_STATE_IDLE;
}

