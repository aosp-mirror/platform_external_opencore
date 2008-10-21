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

//#define LOG_NDEBUG 0
#define LOG_TAG "mediarecorder"
#include <utils/Log.h>

#include <media/mediarecorder.h>
#include "authordriver.h"

namespace android {

MediaRecorder::MediaRecorder()
{
    mAuthorDriverWrapper = new AuthorDriverWrapper();
    mCurrentState = MEDIA_RECORDER_IDLE;
}

MediaRecorder::~MediaRecorder()
{
    delete mAuthorDriverWrapper;
}

status_t MediaRecorder::init()
{
    LOGV("init");
    // this doesn't work async for now
    if (NULL == mAuthorDriverWrapper) {
        return UNKNOWN_ERROR;
    }

    if (!(mCurrentState & MEDIA_RECORDER_IDLE)) {
        LOGV("invalid operation");
        return INVALID_OPERATION;
    }

    author_command *ac = new author_command(AUTHOR_INIT);
    status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, NULL, NULL);
    if (OK != ret) {
        LOGV("init failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return UNKNOWN_ERROR;
    } else {
        mCurrentState = MEDIA_RECORDER_INITIALIZED;
    }
    return ret;
}

status_t MediaRecorder::setAudioSource(audio_source as)
{
    LOGV("setAudioSource(%d)", as);
    if (mCurrentState & MEDIA_RECORDER_INITIALIZED) {
        set_audio_source_command *ac = new set_audio_source_command();
        ac->as = as;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setAudioSource failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setVideoSource(video_source vs)
{
    LOGV("setVideoSource(%d)", vs);
    if (mCurrentState & MEDIA_RECORDER_INITIALIZED) {
        set_video_source_command *ac = new set_video_source_command();
        ac->vs = vs;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setVideoSource failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setOutputFormat(output_format of)
{
    LOGV("setOutputFormat(%d)", of);
    if (mCurrentState & MEDIA_RECORDER_INITIALIZED) {
        set_output_format_command *ac = new set_output_format_command();
        ac->of = of;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setOutputFormat failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        } else {
            mCurrentState = MEDIA_RECORDER_PREPARING;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setAudioEncoder(audio_encoder ae)
{
    LOGV("setAudioEncoder(%d)", ae);
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        set_audio_encoder_command *ac = new set_audio_encoder_command();
        ac->ae = ae;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setAudioEncoder failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setVideoEncoder(video_encoder ve)
{
    LOGV("setVideoEncoder(%d)", ve);
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        set_video_encoder_command *ac = new set_video_encoder_command();
        ac->ve = ve;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setVideoEncoder failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}


status_t MediaRecorder::setOutputFile(const char *path)
{
    LOGV("setOutputFile(%s)", path);
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        set_output_file_command *c = new set_output_file_command();
        c->path = strdup(path);
        status_t ret = mAuthorDriverWrapper->enqueueCommand(c, 0, 0);
        if (OK != ret) {
            LOGV("setOutputFile failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setVideoSize(int width, int height)
{
    LOGV("setVideoSize(%d, %d)", width, height);
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        set_video_size_command *ac = new set_video_size_command();
        ac->width = width;
        ac->height = height;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setVideoSize failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setVideoFrameRate(int frames_per_second)
{
    LOGV("setVideoFrameRate(%d)", frames_per_second);
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        set_video_frame_rate_command *ac = new set_video_frame_rate_command();
        ac->rate = frames_per_second;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("setVideoFrameRate failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::setPreviewSurface(const sp<Surface>& surface)
{
    LOGV("setPreviewSurface(%p)", surface.get());
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        set_preview_surface_command *pc = new set_preview_surface_command();
        pc->surface = surface;
        status_t ret = mAuthorDriverWrapper->enqueueCommand(pc, 0, 0);
        if (OK != ret) {
            LOGV("setPreviewSurface failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::prepare()
{
    LOGV("prepare");
    if (mCurrentState & MEDIA_RECORDER_PREPARING) {
        author_command *ac = new author_command(AUTHOR_PREPARE);
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("prepare failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        } else {
            mCurrentState = MEDIA_RECORDER_PREPARED;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::start()
{
    LOGV("start");
    if (mCurrentState & MEDIA_RECORDER_PREPARED) {
        author_command *ac = new author_command(AUTHOR_START);
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("start failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        } else {
            mCurrentState = MEDIA_RECORDER_RECORDING;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::stop()
{
    LOGV("stop");
    if (mCurrentState & MEDIA_RECORDER_RECORDING) {
        author_command *ac = new author_command(AUTHOR_STOP);
        status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
        if (OK != ret) {
            LOGV("stop failed: %d", ret);
            mCurrentState = MEDIA_RECORDER_ERROR;
            return UNKNOWN_ERROR;
        } else {
            mCurrentState = MEDIA_RECORDER_INITIALIZED;
        }
        return ret;
    }
    return INVALID_OPERATION;
}

status_t MediaRecorder::reset()
{
    LOGV("reset");
    author_command *ac = new author_command(AUTHOR_RESET);
    status_t ret = mAuthorDriverWrapper->enqueueCommand(ac, 0, 0);
    if (OK != ret) {
            LOGV("reset failed: %d", ret);
        mCurrentState = MEDIA_RECORDER_ERROR;
        return UNKNOWN_ERROR;
    }
    mCurrentState = MEDIA_RECORDER_IDLE;
    return init();
}

// This should be called after setAudioSource() is called
status_t MediaRecorder::getMaxAmplitude(int *max)
{
    if (mCurrentState & (MEDIA_RECORDER_IDLE | MEDIA_RECORDER_ERROR)) {
        return INVALID_OPERATION;
    }
    status_t ret = mAuthorDriverWrapper->getMaxAmplitude(max);
    if (OK != ret) {
        mCurrentState = MEDIA_RECORDER_ERROR;
        return UNKNOWN_ERROR;
    }
    return ret;
}

}; // namespace android
