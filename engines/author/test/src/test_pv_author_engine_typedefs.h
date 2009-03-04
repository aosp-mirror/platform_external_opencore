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
#ifndef TEST_PV_AUTHOR_ENGINE_TYPEDEFS_H
#define TEST_PV_AUTHOR_ENGINE_TYPEDEFS_H

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif
#ifndef OSCL_ERROR_PANIC_H_INCLUDED
#include "oscl_error_panic.h"
#endif
#ifndef OSCL_ERROR_CODES_H_INCLUDED
#include "oscl_error_codes.h"
#endif
#ifndef OSCL_CONFIG_IO_H_INCLUDED
#include "osclconfig_io.h"
#endif
#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif
#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVLOGGER_STDERR_APPENDER_H_INCLUDED
#include "pvlogger_stderr_appender.h"
#endif
#ifndef PVLOGGER_FILE_APPENDER_H_INCLUDED
#include "pvlogger_file_appender.h"
#endif
#ifndef PVLOGGER_TIME_AND_ID_LAYOUT_H_INCLUDED
#include "pvlogger_time_and_id_layout.h"
#endif
#ifndef TEST_CASE_H_INCLUDED
#include "test_case.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifndef PV_ENGINE_TYPES_H_INCLUDED
#include "pv_engine_types.h"
#endif
#ifndef PV_ENGINE_OBSERVER_H_INCLUDED
#include "pv_engine_observer.h"
#endif
#ifndef PVAUTHORENGINEFACTORY_H_INCLUDED
#include "pvauthorenginefactory.h"
#endif
#ifndef PVAUTHORENGINEINTERFACE_H_INCLUDED
#include "pvauthorengineinterface.h"
#endif
#ifndef PVAETESTINPUT_H_INCLUDED
#include "pvaetestinput.h"
#endif
#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif
#ifndef OSCL_SCHEDULER_H_INCLUDED
#include "oscl_scheduler.h"
#endif
#ifndef TEXT_TEST_INTERPRETER_H_INCLUDED
#include "text_test_interpreter.h"
#endif
#ifndef PV_MP4_H263_ENC_EXTENSION_H_INCLUDED
#include "pvmp4h263encextension.h"
#endif
#ifndef PVMP4FFCN_CLIPCONFIG_H_INCLUDED
#include "pvmp4ffcn_clipconfig.h"
#endif
#ifndef PVMF_FILEOUTPUT_CONFIG_H_INCLUDED
#include "pvmf_fileoutput_config.h"
#endif
#ifndef PVMF_COMPOSER_SIZE_AND_DURATION_H_INCLUDED
#include "pvmf_composer_size_and_duration.h"
#endif
#ifndef PVAETEST_NODE_CONFIG_H_INCLUDED
#include "pvaetest_node_config.h"
#endif
#ifndef TEST_PV_AUTHOR_ENGINE_CONFIG_H_INCLUDED
#include "test_pv_author_engine_config.h"
#endif
#ifndef UNIT_TEST_ARGS_H_INCLUDED
#include "unit_test_args.h"
#endif
#ifndef PVLOGGER_FILE_APPENDER_H_INCLUDED
#include "pvlogger_file_appender.h"
#endif
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif
#ifndef OSCL_MEM_AUDIT_H_INCLUDED
#include "oscl_mem_audit.h"
#endif
#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif
#ifndef OSCL_STRING_UTILS_H
#include "oscl_string_utils.h"
#endif
#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_stdstring.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_OBSERVER_H_INCLUDED
#include "pvmi_config_and_capability_observer.h"
#endif

#ifndef OSCLCONFIG_H_INCLUDED
#include "osclconfig.h"
#endif

#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif

//composer mime type
#define KMp4ComposerMimeType "/x-pvmf/ff-mux/mp4"
#define K3gpComposerMimeType "/x-pvmf/ff-mux/3gp"
#define KAMRNbComposerMimeType		"/x-pvmf/ff-mux/amr-nb"
#define KAACADIFComposerMimeType	"/x-pvmf/ff-mux/adif"
#define KAACADTSComposerMimeType	"/x-pvmf/ff-mux/adts"

//encoder mime type
#define KAMRNbEncMimeType "/x-pvmf/audio/encode/amr-nb"
#define KH263EncMimeType "/x-pvmf/video/encode/h263"
#define KH264EncMimeType "/x-pvmf/video/encode/h264"
#define KMp4EncMimeType "/x-pvmf/video/encode/mp4"
#define KAACADIFEncMimeType			"/x-pvmf/audio/encode/aac/adif"
#define KAACADTSEncMimeType			"/x-pvmf/audio/encode/aac/adts"
#define KTextEncMimeType "/x-pvmf/text/encode/txt"

// Default input settings
extern const uint32 KVideoBitrate ;
extern const uint32 KVideoFrameWidth ;
extern const uint32 KVideoFrameHeight;
extern const uint32 KVideoTimescale ;
extern const uint32 KNumLayers;

extern const uint32 KVideoFrameRate;
extern const uint32 KNum20msFramesPerChunk;
extern const uint32 KAudioBitsPerSample;

extern const uint16 KVideoIFrameInterval;
extern const uint8 KH263VideoProfile;
extern const uint8 KH263VideoLevel;
extern const uint32 KAudioBitrate;
extern const uint32 KAudioTimescale;
extern const uint32 KAudioNumChannels;

extern const uint32 KMaxFileSize;
extern const uint32 KMaxDuration;
extern const uint32 KFileSizeProgressFreq;
extern const uint32 KDurationProgressFreq;
extern const uint32 KTestDuration;
extern const uint32 KTextTimescale;
extern const uint32 KTextFrameWidth;
extern const uint32 KTextFrameHeight;



// it's for setting Authoring Time Unit for selecting counter loop
// this time unit is used as default authoring time for longetivity tests
//const uint32 KAuthoringSessionUnit = 1800; //in seconds
extern const uint32 KAuthoringSessionUnit;
extern const uint32 KPauseDuration;

// The string to prepend to source filenames
#define SOURCENAME_PREPEND_STRING ""
#define SOURCENAME_PREPEND_WSTRING _STRLIT_WCHAR("")


#define ARRAY_SIZE	512

//enum types for test cases
typedef enum
{
    //3gp output file
    AMR_Input_AOnly_3gpTest = 0, //.amr
    YUV_Input_VOnly_3gpTest, //.yuv
    H263_Input_VOnly_3gpTest, //.h263
    AMR_YUV_Input_AV_3gpTest, //.amr+.yuv
    AMR_H263_Input_AV_3gpTest, //.amr+.h263
    PCM16_Input_AOnly_3gpTest,//5 //.pcm
    PCM16_YUV_Input_AV_3gpTest, //.pcm+.yuv
    KMaxFileSizeTest, //.amr+.yuv
    KMaxDurationTest, //.amr+.yuv
    KFileSizeProgressTest, //.amr+.yuv
    KDurationProgressTest, //10 //.amr+.yuv
    KFastTrackContentModeTest, //.amr+.yuv
    K3GPPDownloadModeTest, //.amr+.yuv
    K3GPPProgressiveDownloadModeTest,//.amr+.yuv
    KMovieFragmentModeTest, // 16 //.amr+.yuv

    K3GP_OUTPUT_TestEnd, //17 placeholder
    //mp4 output file
    //H264 Encoding, no need for 3gp file test, the composerMimeType is mapped to same uuid
    H264_Input_VOnly_Mp4Test, //18 //.yuv
    H264_AMR_Input_AV_Mp4Test, //19 //.yuv
    AMR_Input_AOnly_Mp4Test, //20 //.amr
    YUV_Input_VOnly_Mp4Test, //.yuv
    AMR_YUV_Input_AV_Mp4Test, //.amr+.yuv

    TEXT_Input_TOnly_Mp4Test,//23
    AMR_TEXT_Input_AT_Mp4Test,
    YUV_TEXT_Input_VT_Mp4Test,
    AMR_YUV_TEXT_Input_AVT_Mp4Test,

    PCM16_Input_AOnly_Mp4Test, //27 //.pcm
    PCM16_YUV_Input_AV_Mp4Test, //.pcm+.yuv

    MP4_OUTPUT_TestEnd, //29 placeholder

    //amr output file
    AMR_FOutput_Test, //30 //.amr
    PCM16In_AMROut_Test,  //.pcm

    AMR_OUTPUT_TestEnd, //32 // placeholder

    //aac output file
    AACADIF_FOutput_Test,  //.aacadif
    AACADTS_FOutput_Test,  //.aacadts

    AAC_OUTPUT_TestEnd, //35 // placeholder

    Pause_Resume_Test, //36
    //To Test Error Handling scenarios
    //The incorrect paths are hardcoded in test
    ErrorHandling_WrongVideoInputFileNameTest, //37 //Incorrect name of Video input file
    ErrorHandling_WrongAudioInputFileNameTest,//Incorrect name of Audio input file
    ErrorHandling_WrongTextInputFileNameTest,//Incorrect name of Text input file
    ErrorHandling_WrongOutputPathTest, //Incorrect path of output file
    ErrorHandling_WrongInputFormatTest, //Unsupported input format say RGB16

    //m4v type file not supported currently due to non availability of VOL Hdr size
    //M4V_Input_VOnly_Mp4Test, //.m4v
    //AMR_M4V_Input_AV_Mp4Test, //.amr+.m4v
    NormalTestEnd, //placeholder

    GenericTestBegin = 60,	// generic tests start here
    Generic_Open_Reset_Test,
    Generic_AddDataSource_Audio_Reset_Test,
    Generic_AddDataSource_Video_Reset_Test,
    Generic_AddDataSource_Text_Reset_Test,
    Generic_SelectComposer_Reset_Test,//65
    Generic_QueryInterface_Reset_Test,
    Generic_Add_Audio_Media_Track_Reset_Test,
    Generic_Add_Video_Media_Track_Reset_Test,
    Generic_Add_Text_Media_Track_Reset_Test,
    Generic_Init_Reset_Test, //70
    Generic_Start_Reset_Test,
    Generic_Pause_Reset_Test,
    Generic_Resume_Reset_Test,
    Input_Stream_Looping_Test,
    CapConfigTest,
    GenericTestEnd, //76 Generic tests end here placeholder


    LongetivityTestBegin = 80, // longetivity tests start here //placeholder
    //3gp output file
    AMR_Input_AOnly_3gp_LongetivityTest,  //.amr
    YUV_Input_VOnly_3gp_LongetivityTest, //.yuv
    H263_Input_VOnly_3gp_LongetivityTest, //.h263
    TEXT_Input_TOnly_3gp_LongetivityTest,  //.txt
    AMR_YUV_Input_AV_3gp_LongetivityTest, //85 s//.amr+.yuv
    AMR_H263_Input_AV_3gp_LongetivityTest,  //.amr+.h263
    AMR_TEXT_Input_AT_3gp_LongetivityTest,  //.amr+.txt
    PCM16_Input_AOnly_3gp_LongetivityTest, //.pcm
    PCM16_YUV_Input_AV_3gp_LongetivityTest, //.pcm+.yuv
    YUV_TEXT_Input_VT_3gp_LongetivityTest, //90 //.txt+.yuv
    AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest, //.txt+.yuv +.amr
    KMaxFileSizeLongetivityTest, //.amr+.yuv
    KMaxDurationLongetivityTest, //.amr+.yuv
    KFileSizeProgressLongetivityTest,  //.amr+.yuv
    KDurationProgressLongetivityTest, //95  //.amr+.yuv
    KFastTrackContentModeLongetivityTest, //.amr+.yuv
    K3GPPDownloadModeLongetivityTest, //.amr+.yuv
    K3GPPProgressiveDownloadModeLongetivityTest,  //.amr+.yuv

    K3GP_OUTPUT_LongetivityTestEnd, //101 placeholder

    //mp4 output file
    //H264 Encoding, no need for 3gp file test, the composerMimeType is mapped to same uuid
    H264_Input_VOnly_Mp4_LongetivityTest, //102 //.yuv
    H264_AMR_Input_AV_Mp4_LongetivityTest,  //.yuv
    AMR_Input_AOnly_Mp4_LongetivityTest,  //.amr
    YUV_Input_VOnly_Mp4_LongetivityTest, //105 //.yuv
    AMR_YUV_Input_AV_Mp4_LongetivityTest, //.amr+.yuv
    PCM16_Input_AOnly_Mp4_LongetivityTest, //.pcm
    PCM16_YUV_Input_AV_Mp4_LongetivityTest, //.pcm+.yuv

    MP4_OUTPUT_LongetivityTestEnd,  //109 placeholder

    //amr output file
    AMR_FOutput_LongetivityTest, //110 //.amr
    PCM16In_AMROut_LongetivityTest,  //.pcm

    AMR_OUTPUT_LongetivityTestEnd, // placeholder

    //aac output file
    AACADIF_FOutput_LongetivityTest,  //.aacadif
    AACADTS_FOutput_LongetivityTest,  //.aacadts

    AAC_OUTPUT_LongetivityTestEnd, // longetivity tests end here //placeholder
    // AVI Test Starts
    // AVI file must have RGB24, RGB12 or PCM mono 8KHz data only
    PVMediaInput_Open_Compose_Stop_Test, //116 //Use testinput.avi from vob test_content\avi
    PVMediaInput_Open_RealTimeCompose_Stop_Test, //Real Time authoring
    PVMediaInput_Pause_Resume_Test, //avi //Use testinput.avi from vob test_content\avi
    PVMediaInput_ErrorHandling_Test_WrongFormat, //Use testinput_rgb16.avi from test_content\avi
    PVMediaInput_ErrorHandling_Test_WrongIPFileName,
    PVMediaInput_Reset_Test,
    AVI_Input_Longetivity_Test,
    //AVI Test Ends

    LastInteractiveTest = 1000,
    Invalid_Test

} PVAETestCase;

class pvauthor_async_test_observer;

typedef struct
{
    pvauthor_async_test_observer* iObserver;
    test_case* iTestCase;
    int32 iTestCaseNum;
    FILE* iStdOut;

} PVAuthorAsyncTestParam;




/** Enumeration of types of asychronous commands that can be issued to PV Author Engine */
typedef enum
{
    PVAE_CMD_NONE = 0,
    PVAE_CMD_SET_LOG_APPENDER,
    PVAE_CMD_REMOVE_LOG_APPENDER,
    PVAE_CMD_SET_LOG_LEVEL,
    PVAE_CMD_GET_LOG_LEVEL,
    PVAE_CMD_CREATE,
    PVAE_CMD_OPEN,
    PVAE_CMD_CLOSE,
    PVAE_CMD_ADD_DATA_SOURCE,
    PVAE_CMD_ADD_DATA_SOURCE_AUDIO,
    PVAE_CMD_ADD_DATA_SOURCE_VIDEO,
    PVAE_CMD_ADD_DATA_SOURCE_TEXT,
    PVAE_CMD_REMOVE_DATA_SOURCE,
    PVAE_CMD_SELECT_COMPOSER,
    PVAE_CMD_ADD_MEDIA_TRACK,
    PVAE_CMD_ADD_AUDIO_MEDIA_TRACK,
    PVAE_CMD_ADD_VIDEO_MEDIA_TRACK,
    PVAE_CMD_ADD_TEXT_MEDIA_TRACK,
    PVAE_CMD_ADD_DATA_SINK,
    PVAE_CMD_REMOVE_DATA_SINK,
    PVAE_CMD_INIT,
    PVAE_CMD_RESET,
    PVAE_CMD_START,
    PVAE_CMD_PAUSE,
    PVAE_CMD_RESUME,
    PVAE_CMD_STOP,
    PVAE_CMD_QUERY_UUID,
    PVAE_CMD_QUERY_INTERFACE,
    PVAE_CMD_GET_SDK_INFO,
    PVAE_CMD_GET_SDK_MODULE_INFO,
    PVAE_CMD_CANCEL_ALL_COMMANDS,
    PVAE_CMD_QUERY_INTERFACE2,
    PVAE_CMD_CLEANUPANDCOMPLETE,
    PVAE_CMD_CAPCONFIG_SYNC,
    PVAE_CMD_CAPCONFIG_ASYNC,
    PVAE_CMD_RECORDING
} PVAECmdType;

////////////////////////////////////////////////////////////////////////////
template<class DestructClass>
class LogAppenderDestructDealloc : public OsclDestructDealloc
{
    public:
        virtual void destruct_and_dealloc(OsclAny *ptr)
        {
            delete((DestructClass*)ptr);
        }
};

////////////////////////////////////////////////////////////////////////////
class PVLoggerSchedulerSetup
{
    public:
        PVLoggerSchedulerSetup(int32 aLogFile, int32 aLogLevel, int32 aLogNode):
                iLogFile(aLogFile),
                iLogLevel(aLogLevel),
                iLogNode(aLogNode)	{}

        ~PVLoggerSchedulerSetup() {};

        void InitLoggerScheduler()
        {
            // Logging by PV Logger
            PVLogger::Init();
            PVLoggerAppender *appender = NULL;
            OsclRefCounter *refCounter = NULL;

            if (iLogFile)
            {
                appender = (PVLoggerAppender*)TextFileAppender<TimeAndIdLayout, 1024>::CreateAppender((oscl_wchar*)(KLogFile));
                OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
                    new OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > >(appender);
                refCounter = appenderRefCounter;
            }
            else
            {
                appender = new StdErrAppender<TimeAndIdLayout, 1024>();
                OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
                    new OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > >(appender);
                refCounter = appenderRefCounter;

            }

            OsclSharedPtr<PVLoggerAppender> appenderPtr(appender, refCounter);

            if (iLogNode == 0)
            {
                PVLogger *rootnode = PVLogger::GetLoggerObject("PVAuthorEngine");
                rootnode->AddAppender(appenderPtr);
                rootnode->SetLogLevel(iLogLevel);
            }
            else if (iLogNode == 1)
            {
                // Log all
                PVLogger *rootnode = PVLogger::GetLoggerObject("");
                rootnode->AddAppender(appenderPtr);
                rootnode->SetLogLevel(iLogLevel);
            }
            else if (iLogNode == 2)
            {
                // Log datapath only
                PVLogger *node = PVLogger::GetLoggerObject("datapath");
                node->AddAppender(appenderPtr);
                //info level logs ports & synchronization info.
                node->SetLogLevel(PVLOGMSG_INFO);

                PVLogger* datapathsrc = PVLogger::GetLoggerObject("datapath.sourcenode");
                datapathsrc->DisableAppenderInheritance();
                PVLogger* datapathdec = PVLogger::GetLoggerObject("datapath.decnode");
                datapathdec->DisableAppenderInheritance();
            }
            else if (iLogNode == 3)
            {
                // Log clock only
                PVLogger *clocknode = PVLogger::GetLoggerObject("clock");
                clocknode->AddAppender(appenderPtr);
                clocknode->SetLogLevel(PVLOGMSG_DEBUG);
            }
            else if (iLogNode == 4)
            {
                // Log oscl only
                PVLogger *clocknode = PVLogger::GetLoggerObject("pvscheduler");
                clocknode->AddAppender(appenderPtr);
                clocknode->SetLogLevel(iLogLevel);
                clocknode = PVLogger::GetLoggerObject("osclsocket");
                clocknode->AddAppender(appenderPtr);
                clocknode->SetLogLevel(iLogLevel);
            }
            else if (iLogNode == 5)
            {
                // Log scheduler perf only.
                PVLogger *clocknode = PVLogger::GetLoggerObject("pvscheduler");
                clocknode->AddAppender(appenderPtr);
                //info level logs scheduler activity including AO calls and idle time.
                clocknode->SetLogLevel(PVLOGMSG_INFO);

            }
            else if (iLogNode == 6)
            {
                PVLogger *clocknode = PVLogger::GetLoggerObject("Oscl_File");
                clocknode->AddAppender(appenderPtr);
                clocknode->SetLogLevel(PVLOGMSG_DEBUG + 1);
            }
            else if (iLogNode == 7)
            {
                PVLogger *clocknode = PVLogger::GetLoggerObject("pvauthordiagnostics");
                clocknode->AddAppender(appenderPtr);
                clocknode->SetLogLevel(PVLOGMSG_DEBUG);
            }

            // Construct and install the active scheduler
            OsclScheduler::Init("PVAuthorEngineTestScheduler");
        }

        void CleanupLoggerScheduler()
        {
            OsclScheduler::Cleanup();
            PVLogger::Cleanup();
        }

        //For logging
        int32                     iLogFile;
        int32                     iLogLevel;
        int32                     iLogNode;

};
// Observer class for pvPlayer async test to notify completion of test
class pvauthor_async_test_observer
{
    public:
        // Signals completion of test. Test instance can be deleted after this callback completes.
        virtual void CompleteTest(test_case &) = 0;
};


////////////////////////////////////////////////////////////////////////////
class pvauthor_async_test_base : public OsclTimerObject,
            public PVCommandStatusObserver,
            public PVErrorEventObserver,
            public PVInformationalEventObserver
{
    public:
        pvauthor_async_test_base(PVAuthorAsyncTestParam aTestParam)
                : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVAuthorEngineAsyncTestBase"),
                iObserver(aTestParam.iObserver),
                iTestCase(aTestParam.iTestCase),
                iTestCaseNum(aTestParam.iTestCaseNum),
                iStdOut(aTestParam.iStdOut)
        {};

        virtual ~pvauthor_async_test_base() {}
        virtual void StartTest() = 0;
        virtual void CommandCompleted(const PVCmdResponse& /*aResponse*/) {}
        virtual void HandleErrorEvent(const PVAsyncErrorEvent& /*aEvent*/) {}
        virtual void HandleInformationalEvent(const PVAsyncInformationalEvent& /*aEvent*/) {}

        pvauthor_async_test_observer* iObserver;
        test_case* iTestCase;
        int32 iTestCaseNum;
        FILE* iStdOut;
};


#endif //#ifndef TEST_PV_AUTHOR_ENGINE_TYPEDEFS_H

