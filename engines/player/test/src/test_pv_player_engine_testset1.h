/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
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
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET1_H_INCLUDED
#define TEST_PV_PLAYER_ENGINE_TESTSET1_H_INCLUDED

/**
 *  @file test_pv_player_engine_testset1.h
 *  @brief This file contains the class definitions for the first set of
 *         test cases for PVPlayerEngine
 *
 */

#ifndef TEST_PV_PLAYER_ENGINE_H_INCLUDED
#include "test_pv_player_engine.h"
#endif

#ifndef PV_PLAYER_DATASOURCEURL_H_INCLUDED
#include "pv_player_datasourceurl.h"
#endif

#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif

#ifndef PV_ENGINE_TYPES_H_INCLUDED
#include "pv_engine_types.h"
#endif

#ifndef TEST_PV_PLAYER_ENGINE_CONFIG_H_INCLUDED
#include "test_pv_player_engine_config.h"
#endif

#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif

#ifndef PVMI_CONFIG_AND_CAPABILITY_OBSERVER_H_INCLUDED
#include "pvmi_config_and_capability_observer.h"
#endif

#ifndef PVMF_CPMPLUGIN_FACTORY_REGISTRY_H_INCLUDED
#include "pvmf_cpmplugin_factory_registry.h"
#endif

#define  USE_REF_FILEOUTPUT_FOR_VIDEO


class PVPlayerDataSink;
class PVPlayerDataSinkFilename;
class PvmfFileOutputNodeConfigInterface;
class PvmiCapabilityAndConfig;
class PVRefFileOutput;


/*!
 *  A test case to instantiate and destroy the player engine object via the factory class
 *  - Data Source: N/A
 *  - Data Sink(s): N/A
 *  - Sequence:
 *             -# CreatePlayer()/DeletePlayer()
 *
 */
class pvplayer_async_test_newdelete : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_newdelete(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
        {
            iTestCaseName = _STRLIT_CHAR("New-Delete");
        }

        ~pvplayer_async_test_newdelete() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);
};


/*!
 *  A test case to test the normal engine sequence of playing a specified source
 *  - Data Source: Passed in parameter
 *  - Data Sink(s): Video[FileOutputNode-test_player_openplaystop_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_openplaystop_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_openplaystop_[SRCFILENAME]_text.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# AddDataSink() (text)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 15 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# RemoveDataSink() (text)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_openplaystopreset : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_openplaystopreset(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeText(NULL)
                , iMIOFileOutText(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Open-Play-Stop-Reset");
        }

        ~pvplayer_async_test_openplaystopreset() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_ADDDATASINK_TEXT,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_REMOVEDATASINK_TEXT,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_WAIT_FOR_ERROR_HANDLING,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVPlayerDataSink* iDataSinkText;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutText;
        PVCommandId iCurrentCmdId;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


class PVMFLocalDataSource;
class PVMFOma1PassthruPluginFactory;

/*!
 *  A test case to test the normal engine sequence of playing a specified source with pass-through CPM plug-in
 *  - Data Source: Passed in parameter
 *  - Data Sink(s): Video[FileOutputNode-test_player_cpmopenplaystop_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_cpmopenplaystop_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_cpmopenplaystop_[SRCFILENAME]_text.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource() with pass-through CPM plug-in
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# AddDataSink() (text)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 15 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# RemoveDataSink() (text)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_cpmopenplaystopreset : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_cpmopenplaystopreset(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeText(NULL)
                , iMIOFileOutText(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("CPM Open-Play-Stop-Reset");
            iLocalDataSource = NULL;
            iPluginFactory = NULL;
        }

        ~pvplayer_async_test_cpmopenplaystopreset() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_ADDDATASINK_TEXT,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_REMOVEDATASINK_TEXT,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_WAIT_FOR_ERROR_HANDLING,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVPlayerDataSink* iDataSinkText;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutText;
        PVCommandId iCurrentCmdId;

        PVMFLocalDataSource* iLocalDataSource;
        PVMFCPMPluginFactoryRegistryClient iPluginRegistryClient;
        PVMFOma1PassthruPluginFactory* iPluginFactory;
        OSCL_HeapString<OsclMemAllocator> iPluginMimeType;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test metadata retrieval APIs on a local MP4 file
 *  - Data Source: test_metadata.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_metadata_video.dat]\n
 *                  Audio[FileOutputNode-test_player_metadata_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             =# GetMetadataKeys()
 *             -# GetMetadataValues() Check the metadata values
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# GetMetadataKeys() with key query string
 *             -# GetMetadataValues() Check the metadata values
 *             -# GetMetadataKeys() full list
 *             -# GetMetadataValues() full list
 *             -# GetMetadataValues() segment 1 (0-5)
 *             -# GetMetadataValues() segment 2 (6-end) Check that total of two segments is same as full list
 *             -# Start()
 *             -# WAIT 3 sec.
 *             -# Stop()
 *             -# GetMetadataKeys() Check that list of keys shrunk
 *             -# GetMetadataValues() Check the metadata values
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_metadata : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_metadata(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Metadata");
        }

        ~pvplayer_async_test_metadata() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_GETMETADATAKEYS1,
            STATE_GETMETADATAVALUES1,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_GETMETADATAKEYS2,
            STATE_GETMETADATAVALUES2,
            STATE_GETMETADATAKEYSSEG,
            STATE_GETMETADATAVALUESSEG1,
            STATE_GETMETADATAVALUESSEG2,
            STATE_GETMETADATAVALUESSEG3,
            STATE_START,
            STATE_STOP,
            STATE_GETMETADATAKEYS3,
            STATE_GETMETADATAVALUES3,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_WAIT_FOR_ERROR_HANDLING,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;

        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;

        PVCommandId iCurrentCmdId;

    private:
        PVPMetadataList iKeyList;
        int32 iNumAvailableValues;
        Oscl_Vector<PvmiKvp, OsclMemAllocator> iValueList;
        Oscl_Vector<PvmiKvp, OsclMemAllocator> iValueListSeg1;
        Oscl_Vector<PvmiKvp, OsclMemAllocator> iValueListSeg2;
        OSCL_HeapString<OsclMemAllocator> iKeyQueryString;

        int32 CheckMetadataValue(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList);
};


/*!
 *  A test case to start a normal playback of local MP4 file, check the playback position periodically, and stop.
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_timing_video.dat]\n
 *                  Audio[FileOutputNode-test_player_timing_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# Check playback position every 100 millisec and stop after 4 sec
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_timing : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_timing(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
                , iStartTime(0)
        {
            iTestCaseName = _STRLIT_CHAR("Timing");
        }

        ~pvplayer_async_test_timing() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE,
            STATE_WAIT
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
        PVPPlaybackPosition aPos;
        uint32 iStartTime;
};



/*!
 *  A test case to test the invalid states of the player engine
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_invalid_video.dat]\n
 *                  Audio[FileOutputNode-test_player_invalid_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# Start() (Invalid State)
 *             -# Stop() (Invalid State)
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSource() (Invalid State)
 *             -# Start() (Invalid State)
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Init() (Invalid State)
 *             -# Pause() (Invalid State)
 *             -# Start()
 *             -# Resume() (Invalid State)
 *             -# Prepare() (Invalid State)
 *             -# Pause()
 *             -# Init() (Invalid State)
 *             -# Prepare() (Invalid State)
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_invalidstate : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_invalidstate(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Invalid State");
        }

        ~pvplayer_async_test_invalidstate() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_IDLE_INVALID_START,
            STATE_IDLE_INVALID_STOP,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_INITIALIZED_INVALID_ADDDATASOURCE,
            STATE_INITIALIZED_INVALID_START,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_PREPARED_INVALID_INIT,
            STATE_PREPARED_INVALID_PAUSE,
            STATE_START,
            STATE_STARTED_INVALID_RESUME,
            STATE_STARTED_INVALID_PREPARE,
            STATE_PAUSE,
            STATE_PAUSED_INVALID_INIT,
            STATE_PAUSED_INVALID_PREPARE,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the normal engine sequence of calling stop immediately after prepare
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_preparedstop_video.dat]\n
 *                  Audio[FileOutputNode-test_player_preparedstop_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_preparedstop : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_preparedstop(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Stop When Prepared");
        }

        ~pvplayer_async_test_preparedstop() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the video only play back for 7 seconds
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_videoonly7s_video.dat]\n
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 7 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_videoonlyplay7seconds: public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_videoonlyplay7seconds(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Video-only Play 7 Sec");
        }

        ~pvplayer_async_test_videoonlyplay7seconds() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test playback of 5 sec, stopping, and then playing for 10 sec from beginning again
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_playstop2times_video.dat]\n
 *                  Audio[FileOutputNode-test_player_playstop2times_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 5 sec.
 *             -# Stop()
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 10 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_play5stopplay10stopreset: public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_play5stopplay10stopreset(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play 5 Sec-Stop-Play 10 Sec-Stop-Reset");
        }

        ~pvplayer_async_test_play5stopplay10stopreset() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE_FIRST,
            STATE_START_FIRST,
            STATE_STOP_FIRST,
            STATE_PREPARE_SECOND,
            STATE_START_SECOND,
            STATE_STOP_SECOND,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test pausing and resuming during normal playback
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_pauseresume_video.dat]\n
 *                  Audio[FileOutputNode-test_player_pauseresume_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 10 sec.
 *             -# Pause()
 *             -# WAIT 5 sec
 *             -# Resume()
 *             -# WAIT 10 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_pauseresume : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_pauseresume(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Pause-Resume");
        }

        ~pvplayer_async_test_pauseresume() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_PAUSE,
            STATE_RESUME,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test stop when playback is paused
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_playpausestop_vidoe.dat]\n
 *                  Audio[FileOutputNode-test_player_playpausestop_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 20 sec.
 *             -# Pause()
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_playpausestop : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_playpausestop(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutVideo(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play-Pause-Stop");
        }

        ~pvplayer_async_test_playpausestop() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_PAUSE,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test whether engine can accept outside node for video sink
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_outsidevideosink_video.dat]\n
 *                  Audio[FileOutputNode-test_player_outsidevideosink_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)Where we will pass the outside node
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 10 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_outsidenodeforvideosink : public pvplayer_async_test_base,
            public PVMFNodeCmdStatusObserver,
            public PVMFNodeInfoEventObserver,
            public PVMFNodeErrorEventObserver
{
    public:
        pvplayer_async_test_outsidenodeforvideosink(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iMOutVideo(NULL)
                , iMOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Outside Node for Video Sink");
        }

        ~pvplayer_async_test_outsidenodeforvideosink() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        void NodeCommandCompleted(const PVMFCmdResp& aResponse);
        void HandleNodeInformationalEvent(const PVMFAsyncEvent& /*aEvent*/) {}
        void HandleNodeErrorEvent(const PVMFAsyncEvent& /*aEvent*/) {}

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMOutVideo;
        PvmiMIOControl* iMOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the whether the engine is in correct state or not
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_getplayerstate_video.dat]\n
 *                  Audio[FileOutputNode-test_player_getplayerstate_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# GetPVPlayerStateSync()
 *             -# Init()
 *             -# GetPVPlayerStateSync()
 *             -# AddDataSink() (video)
 *             -# GetPVPlayerStateSync()
 *             -# AddDataSink() (audio)
 *             -# GetPVPlayerStateSync()
 *             -# Prepare()
 *             -# GetPVPlayerStateSync()
 *             -# Start()
 *             -# GetPVPlayerStateSync()
 *             -# WAIT 20 sec.
 *             -# Stop()
 *             -# GetPVPlayerState()
 *             -# RemoveDataSink() (video)
 *             -# GetPVPlayerState()
 *             -# RemoveDataSink() (audio)
 *             -# GetPVPlayerState()
 *             -# Reset()
 *             -# GetPVPlayerState()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_getplayerstate : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_getplayerstate(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("GetPlayerState");
        }

        ~pvplayer_async_test_getplayerstate() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_PLAYER_STATE_FIRST,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_PLAYER_STATE_SECOND,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_PLAYER_STATE_THIRD,
            STATE_RESET,
            STATE_PLAYER_STATE_FOURTH,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;
        PVPlayerState aState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test current position of the play back
 *  - Data Source: test.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_getcurrentposition_video.dat]\n
 *                  Audio[FileOutputNode-test_player_getcurrentposition_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# GetCurrentPositionSync()(Call this synchronous API after every 1 sec)
 *             -# GetCurrentPosition()(Call this asynchronous API after every 1 sec)
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_getcurrentposition : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_getcurrentposition(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
                , iCount(0)
        {
            iTestCaseName = _STRLIT_CHAR("GetCurrentPosition");
        }

        ~pvplayer_async_test_getcurrentposition() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_GETCURRENT_POSITION_SYNC,
            STATE_GETCURRENT_POSITION,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
        PVMFStatus Status;
        PVPPlaybackPosition Position ;
        uint32 iCount;
};


/*!
 *  A test case to test the play and stops the play at specified position
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_playsetstopposition_video.dat]\n
 *                  Audio[FileOutputNode-test_player_playsetstopposition_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# SetPlaybackRange(blank,stop at 15 sec)
 *             -# WAIT 20 sec. for end time reached event
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_playsetstopposition : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_playsetstopposition(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play-Set End Time");
        }

        ~pvplayer_async_test_playsetstopposition() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_SETPLAYBACKRANGE,
            STATE_STOPTIMENOTREACHED,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the play and stops the play at specified video frame number
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_playsetstoppositionvidframenum_video.dat]\n
 *                  Audio[FileOutputNode-test_player_playsetstoppositionvidframenum_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# SetPlaybackRange(blank,stop at 150 frame=~15 sec)
 *             -# WAIT 20 sec. for end time reached event
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_playsetstoppositionvidframenum : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_playsetstoppositionvidframenum(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play-Set End Time by VidFrameNum");
        }

        ~pvplayer_async_test_playsetstoppositionvidframenum() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_SETPLAYBACKRANGE,
            STATE_STOPTIMENOTREACHED,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the start of play at specified position and then stop
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_setstartpositionplaystop_video.dat]\n
 *                  Audio[FileOutputNode-test_player_setstartpositionplaystop_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# SetPlaybackRange(start at 10 sec, blank)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 20 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_setstartpositionplaystop : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_setstartpositionplaystop(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Set Begin Position-Play-Stop");
        }

        ~pvplayer_async_test_setstartpositionplaystop() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_SETPLAYBACKRANGE,
            STATE_PREPARE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test the start and stop of play at specified position using the play range
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_setplayrange_video.dat]\n
 *                  Audio[FileOutputNode-test_player_setplayrange_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             =# SetPlaybackRange(start at 10 sec, stop at 25 sec)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 20 sec. for end time reached event
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_setplayrangeplay : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_setplayrangeplay(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("SetPlaybackRange-Play");
        }

        ~pvplayer_async_test_setplayrangeplay() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_SETPLAYBACKRANGE,
            STATE_PREPARE,
            STATE_START,
            STATE_STOPTIMENOTREACHED,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the start and stop of play at specified position using the play range
 *  with start position in video frame number
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_setplayrangevidframenum_video.dat]\n
 *                  Audio[FileOutputNode-test_player_setplayrangevidframenum_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             =# SetPlaybackRange(start at frame 100=~10 sec, stop at 20sec)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 20 sec. for end time reached event
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_setplayrangevidframenumplay : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_setplayrangevidframenumplay(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("SetPlaybackRange by VidFrameNum-Play");
        }

        ~pvplayer_async_test_setplayrangevidframenumplay() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_SETPLAYBACKRANGE,
            STATE_PREPARE,
            STATE_START,
            STATE_STOPTIMENOTREACHED,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the repositioning during playback
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_playsetplayrangestop_video.dat]\n
 *                  Audio[FileOutputNode-test_player_playsetplayrangestop_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 10 sec.
 *             -# SetPlaybackRange(start at 20 sec, blank)
 *             -# WAIT 10 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_playsetplayrangestop : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_playsetplayrangestop(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play-SetPlaybackRange-Stop");
        }

        ~pvplayer_async_test_playsetplayrangestop() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_SETPLAYBACKRANGE,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the repositioning during playback using video frame number
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_playsetplayrangevidframenumstop_video.dat]\n
 *                  Audio[FileOutputNode-test_player_playsetplayrangevidframenumstop_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 10 sec.
 *             -# SetPlaybackRange(start at video frame 100=~10sec, blank)
 *             -# WAIT 10 sec.
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_playsetplayrangevidframenumstop : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_playsetplayrangevidframenumstop(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play-SetPlaybackRange by VidFrameNum-Stop");
        }

        ~pvplayer_async_test_playsetplayrangevidframenumstop() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_SETPLAYBACKRANGE,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the track level information extension interface
 *  - Data Source: test_trackinfo.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_tracklevelinfo_video.dat]\n
 *                  Audio[FileOutputNode-test_player_tracklevelinfo_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# QueryInterface() (track level info extension IF) THIS SHOULD FAIL
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# QueryInterface() (track level info extension IF)
 *             -# Call track level info APIs
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# QueryInterface() (track level info extension IF) THIS SHOULD FAIL
 *             -# DeletePlayer()
 *
 */
class PVMFTrackLevelInfoExtensionInterface;

class pvplayer_async_test_tracklevelinfo : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_tracklevelinfo(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iTrackLevelInfoIF(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Track-Level Info");
        }

        ~pvplayer_async_test_tracklevelinfo() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_INVALIDQUERYINTERFACE1,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_QUERYINTERFACE,
            STATE_TRACKLEVELINFOTEST,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_INVALIDQUERYINTERFACE2,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVMFTrackLevelInfoExtensionInterface* iTrackLevelInfoIF;
        PVCommandId iCurrentCmdId;
    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test playback at a rate faster than "real-time"
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_setplaybackrate2X_video.dat]\n
 *                  Audio[FileOutputNode-test_player_setplaybackrate2X_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             =# SetPlaybackRate(200000)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 15 sec
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_setplaybackrate2X : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_setplaybackrate2X(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("SetPlaybackRate 2X");
        }

        ~pvplayer_async_test_setplaybackrate2X() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_SETPLAYBACKRATE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test playback at a rate slower than "real-time"
 *  - Data Source: test_reposition.mp4
 *  - Data Sink(s): Video[FileOutputNode-test_player_setplaybackratefifth_video.dat]\n
 *                  Audio[FileOutputNode-test_player_setplaybackratefifth_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             =# SetPlaybackRate(20000)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT 15 sec
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_setplaybackratefifth : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_setplaybackratefifth(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("SetPlaybackRate 1/5X");
        }

        ~pvplayer_async_test_setplaybackratefifth() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_SETPLAYBACKRATE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;
};


/*!
 *  A test case to test the normal engine sequence of playing a specified source when queueing commands together
 *  - Data Source: Specified source
 *  - Data Sink(s): Video[FileOutputNode-test_player_queuedcmd_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_queuedcmd_[SRCFILENAME]_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()/Init()/AddDataSink() (video)/AddDataSink() (audio)
 *             -# Prepare()/Start()
 *             -# WAIT 10 sec.
 *             -# Stop()/RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)/Reset()/RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_queuedcommands : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_queuedcommands(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Queued Commands");
        }

        ~pvplayer_async_test_queuedcommands() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_QUEUEDCMD1,
            STATE_QUEUEDCMD2,
            STATE_QUEUEDCMD3,
            STATE_QUEUEDCMD4,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        Oscl_Vector<PVCommandId, OsclMemAllocator> iCmdIds;
        PVCommandId iCurrentCmdId;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test looping feature by calling SetPlaybackRange() when engine automatically pauses due to end time reached
 *  - Data Source: Specified source
 *  - Data Sink(s): Video[FileOutputNode-test_player_looping_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_looping_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_looping_[SRCFILENAME]_text.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()/Init()/AddDataSink() (video)/AddDataSink() (audio)/AddDataSink() (text)/Prepare()/Start()
 *             -# SetPlaybackRange(blank, end at 10 sec)
 *             -# WAIT 15 sec. for end time reached event
 *             -# SetPlaybackRange(start at 0 sec, end at 10 sec)
 *             -# Resume()
 *             -# WAIT 15 sec. for end time reached event
 *             -# Stop()/RemoveDataSink() (video)/RemoveDataSink() (audio)/RemoveDataSink() (text)/Reset()/RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_looping : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_looping(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeText(NULL)
                , iMIOFileOutText(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Looping");
        }

        ~pvplayer_async_test_looping() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_STARTPLAYBACK,
            STATE_SETPLAYBACKRANGE1,
            STATE_ENDTIMENOTREACHED1,
            STATE_SETPLAYBACKRANGE2,
            STATE_RESUME,
            STATE_ENDTIMENOTREACHED2,
            STATE_SHUTDOWN,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVPlayerDataSink* iDataSinkText;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutText;
        Oscl_Vector<PVCommandId, OsclMemAllocator> iCmdIds;
        PVCommandId iCurrentCmdId;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test the normal engine sequence of playing a specified source till end of clip
 *  - Data Source: Passed in parameter
 *  - Data Sink(s): Video[FileOutputNode-test_player_waitforeos_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_waitforeos_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_waitforeos_[SRCFILENAME]_text.dat]
 *  - Sequence
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# AddDataSink() (text)
 *             -# Prepare()
 *             -# Start()
 *             -# WAIT FOR EOS OR 180 SEC TIMEOUT
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# RemoveDataSink() (text)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_waitforeos : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_waitforeos(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeText(NULL)
                , iMIOFileOutText(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Play-Wait For EOS");
        }

        ~pvplayer_async_test_waitforeos() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_ADDDATASINK_TEXT,
            STATE_PREPARE,
            STATE_START,
            STATE_EOSNOTREACHED,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_REMOVEDATASINK_TEXT,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVPlayerDataSink* iDataSinkText;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutText;
        PVCommandId iCurrentCmdId;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};



/*!
 *  A test case to test if the player engine can handle multiple pause-resume requests
 *  - Data Source: Specified source
 *  - Data Sink(s): Video[FileOutputNode-test_player_multipauseresume_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_multipauseresume_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_multipauseresume_[SRCFILENAME]_text.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()/Init()/AddDataSink() (video)/AddDataSink() (audio)/AddDataSink() (text)/Prepare()/Start()
 *             -# WAIT 5 sec.
 *             -# Pause()/Resume() X 5
 *             -# WAIT 2 sec.
 *             -# Pause()
 *             -# WAIT 5 sec.
 *             -# Resume()
 *             -# WAIT 3 sec.
 *             -# Pause()/Resume() X 3
 *             -# Wait 5 sec.
 *             -# Stop()/RemoveDataSink() (video)/RemoveDataSink() (audio)/RemoveDataSink() (text)/Reset()/RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_multipauseresume : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_multipauseresume(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeText(NULL)
                , iMIOFileOutText(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Multiple Pause-Resume");
        }

        ~pvplayer_async_test_multipauseresume() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_STARTPLAYBACK,
            STATE_PAUSERESUME1,
            STATE_PAUSE,
            STATE_RESUME,
            STATE_PAUSERESUME2,
            STATE_SHUTDOWN,
            STATE_WAIT_FOR_ERROR_HANDLING,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVPlayerDataSink* iDataSinkText;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutText;
        Oscl_Vector<PVCommandId, OsclMemAllocator> iCmdIds;
        PVCommandId iCurrentCmdId;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test if the player engine can handle multiple repositioning during playback
 *  - Data Source: Specified source
 *  - Data Sink(s): Video[FileOutputNode-test_player_multireposition_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_multireposition_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_multireposition_[SRCFILENAME]_text.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()/Init()/AddDataSink() (video)/AddDataSink() (audio)/AddDataSink() (text)/Prepare()/Start()
 *             -# WAIT 5 sec.
 *             -# SetPlaybackRange(20 sec, indeterminate)
 *             -# WAIT 5 sec.
 *             -# SetPlaybackRange(5 sec, indeterminate)
 *             -# WAIT 5 sec.
 *             -# SetPlaybackRange(25 sec, indeterminate)
 *             -# WAIT 5 sec.
 *             -# SetPlaybackRange(10 sec, indeterminate)
 *             -# WAIT 5 sec.
 *             -# SetPlaybackRange(20 sec, indeterminate)
 *             -# SetPlaybackRange(0 sec, indeterminate)
 *             -# WAIT 5 sec.
 *             -# Stop()/RemoveDataSink() (video)/RemoveDataSink() (audio)/RemoveDataSink() (text)/Reset()/RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_multireposition : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_multireposition(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeText(NULL)
                , iMIOFileOutText(NULL)
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("Multiple SetPlaybackRange");
        }

        ~pvplayer_async_test_multireposition() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_STARTPLAYBACK,
            STATE_SETPLAYBACKRANGE1,
            STATE_SETPLAYBACKRANGE2,
            STATE_SETPLAYBACKRANGE3,
            STATE_SETPLAYBACKRANGE4,
            STATE_SETPLAYBACKRANGE5,
            STATE_SETPLAYBACKRANGE6,
            STATE_SHUTDOWN,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVPlayerDataSink* iDataSinkText;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutText;
        PVCommandId iCurrentCmdId;
        Oscl_Vector<PVCommandId, OsclMemAllocator> iCmdIds;

        Oscl_FileServer iFS;
        Oscl_File iTimeLogFile;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test capability-and-configuration interface of player engine
 *  - Data Source: Passed in parameter
 *  - Data Sink(s): Video[FileOutputNode-test_player_capconfigif_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_capconfigif_[SRCFILENAME]_audio.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# QueryInterface() (capability-and-config interface)
 *             -# CALL CAPCONFIG METHODS WHILE IDLE
 *             -# AddDataSource()
 *             -# Init()
 *             -# CALL CAPCONFIG METHODS WHILE INITIALIZED
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# Prepare()
 *             -# Start()
 *             -# CALL CAPCONFIG METHODS WHILE PLAYING
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_capconfigiftest : public pvplayer_async_test_base, public PvmiConfigAndCapabilityCmdObserver
{
    public:
        pvplayer_async_test_capconfigiftest(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iPlayerCapConfigIF(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iIONodeVideo(NULL)
                , iMIOFileOutVideo(NULL)
                , iDataSinkAudio(NULL)
                , iIONodeAudio(NULL)
                , iMIOFileOutAudio(NULL)
                , iCurrentCmdId(0)
                , iErrorKVP(NULL)
        {
            iTestCaseName = _STRLIT_CHAR("Capability&Config Interface");
        }

        ~pvplayer_async_test_capconfigiftest() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        // From PvmiConfigAndCapabilityCmdObserver
        void SignalEvent(int32 req_id);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_QUERYINTERFACE,
            STATE_CAPCONFIG1,
            STATE_CAPCONFIG2,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_CAPCONFIG3,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_PREPARE,
            STATE_START,
            STATE_CAPCONFIG4,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_WAIT_FOR_ERROR_HANDLING,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PvmiCapabilityAndConfig* iPlayerCapConfigIF;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVMFNodeInterface* iIONodeVideo;
        PvmiMIOControl* iMIOFileOutVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVMFNodeInterface* iIONodeAudio;
        PvmiMIOControl* iMIOFileOutAudio;
        PVCommandId iCurrentCmdId;

        PvmiKvp* iErrorKVP;
        PvmiKvp iKVPSetAsync;
        OSCL_StackString<64> iKeyStringSetAsync;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


/*!
 *  A test case to test the working of Start - Stop - Prepare - SetPlaybackRange - Start
 *  - Data Source: Specified source
 *  - Data Sink(s): Video[FileOutputNode-test_player_multireposition_[SRCFILENAME]_video.dat]\n
 *                  Audio[FileOutputNode-test_player_multireposition_[SRCFILENAME]_audio.dat]\n
 *                  Text[FileOutputNode-test_player_multireposition_[SRCFILENAME]_text.dat]
 *  - Sequence:
 *             -# CreatePlayer()
 *             -# AddDataSource()/Init()/AddDataSink() (video)/AddDataSink() (audio)/AddDataSink() (text)
 *			   -# Prepare()/ SetPlaybackRange() / Start()
 *			   -# Wait for 5secs.
 *			   -# Stop()/RemoveDataSink() (video)/RemoveDataSink() (audio)/RemoveDataSink() (text)/Reset()/RemoveDataSource()
 *             -# DeletePlayer()
 *
 */
class pvplayer_async_test_setplaybackafterprepare : public pvplayer_async_test_base
{
    public:
        pvplayer_async_test_setplaybackafterprepare(PVPlayerAsyncTestParam aTestParam):
                pvplayer_async_test_base(aTestParam)
                , iPlayer(NULL)
                , iDataSource(NULL)
                , iDataSinkVideo(NULL)
                , iDataSinkAudio(NULL)
                , iDataSinkText(NULL)
                , iIONodeVideo(NULL)
                , iIONodeAudio(NULL)
                , iIONodeText(NULL)
#ifdef USE_REF_FILEOUTPUT_FOR_VIDEO
                , iMIOFileOutVideo(NULL)
#else
                , iMIOOutVideo(NULL)
#endif
                , iMIOOutAudio(NULL)
                , iMIOFileOutText(NULL)
#if USE_AUDIO_EXTN_INTERFACE
                , iAudioOutputController(NULL)
#endif // USE_AUDIO_EXTN_INTERFACE
                , iCurrentCmdId(0)
        {
            iTestCaseName = _STRLIT_CHAR("SetPlaybackRange After Prepare");
        }

        ~pvplayer_async_test_setplaybackafterprepare() {}

        void StartTest();
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVTestState
        {
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_ADDDATASINK_TEXT,
            STATE_PREPARE,
            STATE_SETPLAYBACKRANGE,
            STATE_START,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_REMOVEDATASINK_TEXT,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_WAIT_FOR_ERROR_HANDLING,
            STATE_CLEANUPANDCOMPLETE
        };

        PVTestState iState;

        PVPlayerInterface* iPlayer;
        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVPlayerDataSink* iDataSinkText;

        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PVMFNodeInterface* iIONodeText;
#ifdef USE_REF_FILEOUTPUT_FOR_VIDEO
        PVRefFileOutput* iMIOFileOutVideo;
#else
        PvmiMIOControl* iMIOOutVideo;
#endif

        PvmiMIOControl* iMIOOutAudio;

        PVRefFileOutput* iMIOFileOutText;

#if USE_AUDIO_EXTN_INTERFACE
        PVMioAudioOutputControl* iAudioOutputController;
#endif // USE_AUDIO_EXTN_INTERFACE
        PVCommandId iCurrentCmdId;
        Oscl_Vector<PVCommandId, OsclMemAllocator> iCmdIds;

        Oscl_FileServer iFS;
        Oscl_File iTimeLogFile;

    private:
        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


#endif // TEST_PV_PLAYER_ENGINE_TESTSET1_H_INCLUDED

