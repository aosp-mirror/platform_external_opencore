/*
**
** Copyright 2008, The Android Open Source Project
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

#ifndef _PLAYERDRIVER_H
#define _PLAYERDRIVER_H

#include <cstring>
#include <ui/ISurface.h>
#include <utils/Errors.h>  // for android::status_t
#include <utils/RefBase.h>  // for android::sp
#include <media/MediaPlayerInterface.h>

typedef void (*media_completion_f)(android::status_t status, void *cookie, bool cancelled);

// Commands that MediaPlayer sends to the PlayerDriver
// TODO: Move this class and subclass in their own .h
// TODO: Write a class comment.
class PlayerCommand
{
  public:
    // TODO: Explain these codes.
    enum Code {
        // Stops the scheduler. Does not free any resources in the player.
        PLAYER_QUIT                     = 1,
        // Load the player capabilities.
        PLAYER_SETUP                    = 2,
        // Reset must be called before set data source.
        PLAYER_SET_DATA_SOURCE          = 3,
        // TODO: Rename to PLAYER_SET_VIDEO_SINK.
        PLAYER_SET_VIDEO_SURFACE        = 4,
        PLAYER_SET_AUDIO_SINK           = 5,
        // Must be called after a data source has been set or modified.
        PLAYER_INIT                     = 6,
        // PLAYER_PREPARE must be called after the source(s) and sink(s) have
        // been set or after a PLAYER_STOP command.
        PLAYER_PREPARE                  = 7,
        PLAYER_START                    = 8,
        // After a PLAYER_STOP you must issue a PLAYER_PREPARE to replay the
        // same source/sink combination.
        PLAYER_STOP                     = 9,
        PLAYER_PAUSE                    = 10,
        // Discard the sinks. Does not cancel all pending and running
        // commands. Does not remove the data source.
        PLAYER_RESET                    = 11,
        // TODO: What is that? Loop on the data source?
        PLAYER_SET_LOOP                 = 12,
        // TODO: When can this happen? in prepared started and pause?
        PLAYER_SEEK                     = 13,
        // TODO: When can this happend?  in prepared started and pause?
        PLAYER_GET_POSITION             = 14,
        PLAYER_GET_DURATION             = 15,
        // TODO: Can this command be issued anytime?
        PLAYER_GET_STATUS               = 16,
        // TODO: How is that different from reset? Does it leave the sink
        // untouched?
        PLAYER_REMOVE_DATA_SOURCE       = 17,
        // TODO: clarify the scope of PLAYER_CANCEL_ALL_COMMANDS, does it work
        // for asynchronous commands only or for synchronous as well?
        PLAYER_CANCEL_ALL_COMMANDS      = 18,
        PLAYER_CHECK_LIVE_STREAMING     = 19,
    };

    virtual             ~PlayerCommand() {}
    Code                code() const { return mCode; }
    media_completion_f  callback() { return mCallback; }
    void*               cookie() { return mCookie; }

    // If no callback was supplied, the command runs in synchronous mode.
    bool                hasCallback() const { return NULL != mCallback; }
    void                complete(android::status_t status, bool cancelled) { mCallback(status, mCookie, cancelled); }
    void                set(media_completion_f cbf, void* cookie) { mCallback = cbf; mCookie = cookie; }

    // @return the command code as a string.
    const char*         toString() const;
  protected:
    PlayerCommand(Code code, media_completion_f cbf, void* cookie) :
            mCode(code), mCallback(cbf), mCookie(cookie) {}
  private:
    PlayerCommand();
    Code                mCode;
    media_completion_f  mCallback;
    void*               mCookie;
};

class PlayerQuit : public PlayerCommand
{
  public:
    PlayerQuit(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_QUIT, cbf, cookie) {}
  private:
    PlayerQuit();
};

class PlayerSetup : public PlayerCommand
{
  public:
    PlayerSetup(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_SETUP, cbf, cookie) {}
  private:
    PlayerSetup();
};

class PlayerInit : public PlayerCommand
{
  public:
    PlayerInit(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_INIT, cbf, cookie) {}
  private:
    PlayerInit();
};

class PlayerPrepare: public PlayerCommand
{
  public:
    PlayerPrepare(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_PREPARE, cbf, cookie) {}
  private:
    PlayerPrepare();
};

class PlayerStart: public PlayerCommand
{
  public:
    PlayerStart(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_START, cbf, cookie) {}
  private:
    PlayerStart();
};

class PlayerStop: public PlayerCommand
{
  public:
    PlayerStop(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_STOP, cbf, cookie) {}
  private:
    PlayerStop();
};

class PlayerPause: public PlayerCommand
{
  public:
    PlayerPause(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_PAUSE, cbf, cookie) {}
  private:
    PlayerPause();
};

class PlayerReset: public PlayerCommand
{
  public:
    PlayerReset(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_RESET, cbf, cookie) {}
  private:
    PlayerReset();
};

class PlayerSetDataSource : public PlayerCommand
{
  public:
    PlayerSetDataSource(const char* url, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_SET_DATA_SOURCE, cbf, cookie), mUrl(0) {
        if (url) mUrl = strdup(url); }
    ~PlayerSetDataSource() { if (mUrl) free(mUrl); }
    const char*         url() const { return mUrl; }
  private:
    PlayerSetDataSource();
    char*               mUrl;
};

class PlayerSetVideoSurface : public PlayerCommand
{
  public:
    PlayerSetVideoSurface(const android::sp<android::ISurface>& surface, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_SET_VIDEO_SURFACE, cbf, cookie), mSurface(surface) {}
    ~PlayerSetVideoSurface() { mSurface.clear(); }
    android::sp<android::ISurface>        surface() const { return mSurface; }
  private:
    PlayerSetVideoSurface();
    android::sp<android::ISurface>        mSurface;
};

class PlayerSetAudioSink : public PlayerCommand
{
  public:
    PlayerSetAudioSink(const android::sp<android::MediaPlayerInterface::AudioSink>& audioSink,
                       media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_SET_AUDIO_SINK, cbf, cookie), mAudioSink(audioSink) {}
    ~PlayerSetAudioSink() { mAudioSink.clear(); }
    android::sp<android::MediaPlayerInterface::AudioSink> audioSink() { return mAudioSink; }
  private:
    PlayerSetAudioSink();
    android::sp<android::MediaPlayerInterface::AudioSink> mAudioSink;
};

class PlayerSetLoop: public PlayerCommand
{
  public:
    PlayerSetLoop(int loop, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_SET_LOOP, cbf, cookie), mLoop(loop) {}
    int loop() { return mLoop; }
  private:
    PlayerSetLoop();
    int                 mLoop;
};

class PlayerSeek : public PlayerCommand
{
  public:
    PlayerSeek(int msec, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_SEEK, cbf, cookie), mMsec(msec) {}
    int                 msec() { return mMsec; }
  private:
    PlayerSeek();
    int                 mMsec;
};

class PlayerGetPosition: public PlayerCommand
{
  public:
    PlayerGetPosition(int* msec, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_GET_POSITION, cbf, cookie), mMsec(msec) {}
    void set(int msecs) { if (mMsec) *mMsec = msecs; }
  private:
    PlayerGetPosition();
    int*                mMsec;
};

class PlayerGetDuration: public PlayerCommand
{
  public:
    PlayerGetDuration(int* msec, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_GET_DURATION, cbf, cookie), mMsec(msec) {}
    void set(int msecs) { if (mMsec) *mMsec = msecs; }
  private:
    PlayerGetDuration();
    int*                mMsec;
};

class PlayerCheckLiveStreaming: public PlayerCommand
{
  public:
    PlayerCheckLiveStreaming(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_CHECK_LIVE_STREAMING, cbf, cookie) {}
  private:
    PlayerCheckLiveStreaming();
};

class PlayerGetStatus: public PlayerCommand
{
  public:
    PlayerGetStatus(int *status, media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_GET_STATUS, cbf, cookie), mStatus(status) {}
    void set(int status) { *mStatus = status; }
  private:
    PlayerGetStatus();
    int*                mStatus;
};

class PlayerRemoveDataSource: public PlayerCommand
{
  public:
    PlayerRemoveDataSource(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_REMOVE_DATA_SOURCE, cbf, cookie) {}
  private:
    PlayerRemoveDataSource();
};

class PlayerCancelAllCommands: public PlayerCommand
{
  public:
    PlayerCancelAllCommands(media_completion_f cbf, void* cookie) :
            PlayerCommand(PLAYER_CANCEL_ALL_COMMANDS, cbf, cookie) {}
  private:
    PlayerCancelAllCommands();
};


#endif // _PLAYERDRIVER_H
