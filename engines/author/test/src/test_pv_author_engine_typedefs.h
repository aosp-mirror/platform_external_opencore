/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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

#ifndef PVAELOGGER_H_INCLUDED
#include "test_pv_author_engine_logger.h"
#endif

//#define _W(x) _STRLIT_WCHAR(x)

//composer mime type
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
    ERROR_NOSTATE = 0,
    ERROR_VIDEO_START_INIT,
    ERROR_VIDEO_START_ENCODE,
    ERROR_VIDEO_START_ENCODE_5FRAMES,
    ERROR_COMPOSER_START_ADDMEMFRAG,
    ERROR_COMPOSER_START_ADDTRACK,
    ERROR_MEDIAINPUTNODE_ADDDATASOURCE_START,
    ERROR_MEDIAINPUTNODE_ADDDATASOURCE_STOP,
    ERROR_AVC_START_ENCODE,
    ERROR_AVC_START_ENCODE_5FRAMES
}FAIL_STATE;


typedef enum
{

    /*********** Compressed Tests Begin****************************/
    // Tests that take compressed inputs
    //3gp output file
    AMR_Input_AOnly_3gpTest = 0,					//.amr
    H263_Input_VOnly_3gpTest,						//.h263
    AMR_YUV_Input_AV_3gpTest,						//.amr+.yuv
    AMR_H263_Input_AV_3gpTest,						//.amr+.h263

    H264_AMR_Input_AV_3gpTest,						//4 //.yuv
    AMR_YUV_Input_AV_M4V_AMR_Output_3gpTest,						//.amr+.yuv
    TEXT_Input_TOnly_3gpTest,						//6
    AMR_TEXT_Input_AT_3gpTest,
    YUV_TEXT_Input_VT_3gpTest,
    AMR_YUV_TEXT_Input_AVT_Mp4Test,

    K3GP_OUTPUT_TestEnd,							//10 placeholder

    //amr output file
    AMR_FOutput_Test,								//11 //.amr

    AMR_OUTPUT_TestEnd,								//12 // placeholder

    //aac output file
    AACADIF_FOutput_Test,							//.aacadif
    AACADTS_FOutput_Test,							//.aacadts

    AAC_OUTPUT_TestEnd,								//15 // placeholder

    //ErrorHandling_WrongTextInputFileNameTest,		//Incorrect name of Text input file

    CompressedNormalTestEnd,						//placeholder

    /*********** Compressed Longetivity Tests *********************/
    // tests that takes compressed inputs and run for long duration
    // The duration is configurable through command line argument
    CompressedLongetivityTestBegin = 100,			//placeholder
    //3gp output file
    TEXT_Input_TOnly_3gp_LongetivityTest,			//.txt
    AMR_TEXT_Input_AT_3gp_LongetivityTest,			//.amr+.txt
    YUV_TEXT_Input_VT_3gp_LongetivityTest,			//103 //.txt+.yuv
    AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest,		//.txt+.yuv +.amr
    Compressed_LongetivityTestEnd,					//105 placeholder

    /*********** Compressed Tests End******************************/

    /*********** UnCompressed Tests Begin**************************/
    // Test case that takes Uncompressed input through AVI File begin

    // AVI file must have RGB24, RGB12, YUV420 planar or PCM mono 8KHz data only
    UnCompressed_NormalTestBegin = 200,				//placeholder
    PVMediaInput_Open_Compose_Stop_Test  ,			//Use testinput.avi
    PVMediaInput_Open_RealTimeCompose_Stop_Test,	//Real Time authoring

    YUV_Input_VOnly_3gpTest,						//.yuv
    PCM16_Input_AOnly_3gpTest,						//204 //.pcm
    PCM16_YUV_Input_AV_3gpTest,						//205 //.pcm+.yuv
    H264_Input_VOnly_3gpTest,						//.yuv
    PCM16In_AMROut_Test,							//.pcm//207

    KMaxFileSizeTest,								//.amr+.yuv

#ifndef _IMOTION_SPECIFIC_UT_DISABLE
    KIMotionAuthoringModeTest,						//.amr+.yuv
#endif
    K3GPPDownloadModeTest,							//.amr+.yuv
    K3GPPProgressiveDownloadModeTest,				//.amr+.yuv

#ifndef _IMOTION_SPECIFIC_UT_DISABLE
    KIMotionDownloadModeTest,						//.amr+.yuv
#endif
    KMovieFragmentModeTest,
    CapConfigTest,									//218

    PVMediaInput_Pause_Resume_Test,//219

    PVMediaInput_Reset_After_Create_Test,//220
    PVMediaInput_Reset_After_Open_Test,//221
    PVMediaInput_Reset_After_AddDataSource_Test,//222
    PVMediaInput_Reset_After_SelectComposer_Test,//223
    PVMediaInput_Reset_After_AddMediaTrack_Test,//224
    PVMediaInput_Reset_After_Init_Test,//225
    PVMediaInput_Reset_After_Start_Test,//226
    PVMediaInput_Reset_After_Pause_Test,//227
    PVMediaInput_Reset_After_Recording_Test,//228
    PVMediaInput_Reset_After_Stop_Test,//229

    PVMediaInput_Delete_After_Create_Test,//230
    PVMediaInput_Delete_After_Open_Test,//231
    PVMediaInput_Delete_After_AddDataSource_Test,//232
    PVMediaInput_Delete_After_SelectComposer_Test,//233
    PVMediaInput_Delete_After_AddMediaTrack_Test,//234
    PVMediaInput_Delete_After_Init_Test,//235
    PVMediaInput_Delete_After_Start_Test,//236
    PVMediaInput_Delete_After_Pause_Test,//237
    PVMediaInput_Delete_After_Recording_Test,//238
    PVMediaInput_Delete_After_Stop_Test,//239

    UnCompressed_NormalTestEnd,

    /********** Uncompressed Longetivity tests*********************/
    UnCompressed_LongetivityTestBegin = 300,		//placeholder
    AVI_Input_Longetivity_Test,						//301
    KMaxFileSizeLongetivityTest,					//.amr+.yuv/302
#ifndef _IMOTION_SPECIFIC_UT_DISABLE
    KIMotionAuthoringModeLongetivityTest,			//.amr+.yuv
#endif
    K3GPPDownloadModeLongetivityTest,				//.amr+.yuv
    K3GPPProgressiveDownloadModeLongetivityTest,    //.amr+.yuv
#ifndef _IMOTION_SPECIFIC_UT_DISABLE
    KIMotionDownloadModeLongetivityTest,			//307 //.amr+.yuv
#endif
    KMovieFragmentModeLongetivityTest,				//308
    UnCompressed_LongetivityTestEnd,

    /*********** UnCompressed Tests End****************************/

    /*********** Error Handling Tests Begin************************/

    // Error Handling tests. These are to test the error handling capability of Author Engine.
    // Some of the tests takes unsupported inputs like RGB16 data (PVMediaInput_ErrorHandling_Test_WrongFormat).
    // Other tests deliberately induces errors at various points in the data path. The error point is send through
    // KVP keys through the test app.

    // Error handling tests that takes compressed inputs
    KCompressed_Errorhandling_TestBegin = 400,
    ErrorHandling_WrongTextInputFileNameTest,
    ErrorHandling_MediaInputNodeStartFailed,
    KCompressed_Errorhandling_TestEnd,

    //Error handling tests that takes uncompressed inputs through avi files.
    KUnCompressed_Errorhandling_TestBegin = 500,
    PVMediaInput_ErrorHandling_Test_WrongFormat,	//Use testinput_rgb16.avi
    PVMediaInput_ErrorHandling_Test_WrongIPFileName,
    ErrorHandling_WrongOutputPathTest,
    ErrorHandling_VideoInitFailed,							// 504, //Video Encoder Init Failed
    ErrorHandling_VideoEncodeFailed,						//Video Encoder Encode Failed
    ErrorHandling_VideoEncode5FramesFailed,					//VideoEncNode Encode5Frames Failed
    ErrorHandling_ComposerAddFragFailed,					//507, Composer AddMemFrag Failed
    ErrorHandling_ComposerAddTrackFailed,					//Composer AddMemTrack Failed
    ErrorHandling_AVCVideoEncodeFailed,						//AVCEncNode Encode Failed
    ErrorHandling_AVCVideoEncode5FramesFailed,				//AVCEncNode Encode5Frames Failed
    ErrorHandling_MediaInputNodeStopFailed,
    ErrorHandling_AudioInitFailed,							//Audio Encoder(AMR) Init Failed
    ErrorHandling_AudioEncodeFailed,						//513, Audio Encoder(AMR) Encode Failed
    ErrorHandling_MediaInputNode_NoMemBuffer,				//514
    ErrorHandling_MediaInputNode_Out_Queue_busy,			//515
    ErrorHandling_MediaInputNode_large_time_stamp,						//516 MediaInputNode Error in time stamp for large value.
    ErrorHandling_MediaInputNode_wrong_time_stamp_after_duration,		//517 MediaInputNode Error in time stamp for wrong value after duration of time.
    ErrorHandling_MediaInputNode_zero_time_stamp,						//518 MediaInputNode Error in time stamp for zero value.
    ErrorHandling_MediaInputNode_StateFailure_EPause_SendMIORequest,	//MediaInputNode Error in SendMIOioRequest().
    ErrorHandling_MediaInputNode_StateFailure_CancelMIORequest,			//MediaInputNode Error in CancelMIORequest().
    ErrorHandling_MediaInputNode_Corrupt_Video_InputData,				//MediaInputNode Corrupt the video input data.
    ErrorHandling_MediaInputNode_Corrupt_Audio_InputData,				//MediaInputNode Corrupt the audio input data.
    ErrorHandling_MediaInputNode_DataPath_Stall,						//MediaInputNode Stall the data path.
    ErrorHandling_MP4Composer_AddTrack_PVMF_AMR_IETF,					//524 MP4ComposerNode Error in AddTrack() for PVMF_AMR_IETF.
    ErrorHandling_MP4Composer_AddTrack_PVMF_3GPP_TIMEDTEXT,				//MP4ComposerNode Error in AddTrack() for PVMF_3GPP_TIMEDTEXT.
    ErrorHandling_MP4Composer_AddTrack_PVMF_M4V,						//MP4ComposerNode Error in AddTrack() for PVMF_M4V.
    ErrorHandling_MP4Composer_AddTrack_PVMF_H263,						//MP4ComposerNode Error in AddTrack() for PVMF_H263.
    ErrorHandling_MP4Composer_AddTrack_PVMF_H264_MP4,					//MP4ComposerNode Error in AddTrack() for PVMF_H264_MP4.
    ErrorHandling_MP4Composer_Create_FileParser,						//MP4ComposerNode Error in the creation of mp4 file parser.
    ErrorHandling_MP4Composer_RenderToFile,								//MP4ComposerNode Error in the RenderToFile().
    ErrorHandling_MP4Composer_FailAfter_FileSize,						//MP4ComposerNode Error after a particular file size is reached.
    ErrorHandling_MP4Composer_FailAfter_Duration,						//MP4ComposerNode Error after a duration of some time.
    ErrorHandling_MP4Composer_DataPathStall,							//MP4ComposerNode Stall the data path.
    ErrorHandling_VideoEncodeNode_ConfigHeader,							//VideoEncodeNode Error in GetVolHeader().
    ErrorHandling_VideoEncodeNode_DataPathStall_Before_ProcessingData,	//535 VideoEncodeNode Stall the data path before processing starts.
    ErrorHandling_VideoEncodeNode_DataPathStall_After_ProcessingData,	//VideoEncodeNode Stall the data path post processing.
    ErrorHandling_VideoEncodeNode_FailEncode_AfterDuration,				//VideoEncodeNode Error in encode after duration of time.
    ErrorHandling_AudioEncodeNode_FailEncode_AfterDuration,				//AudioEncodeNode Error in encode operation after duration of time.
    ErrorHandling_AudioEncodeNode_DataPathStall_Before_ProcessingData,	//AudioEncodeNode Stall data path before processing starts.
    ErrorHandling_AudioEncodeNode_DataPathStall_After_ProcessingData,	//540 AudioEncodeNode Stall data path post processing.
    ErrorHandling_AVCEncodeNode_ConfigHeader,							//AVCEncodeNode Error in in getting SPS & PPS Values.
    ErrorHandling_AVCEncodeNode_DataPathStall_Before_ProcessingData,	//AVCEncodeNode Stall the data path before processing starts.
    ErrorHandling_AVCEncodeNode_DataPathStall_After_ProcessingData,		//AVCEncodeNode Stall the data path post processing.
    ErrorHandling_AVCEncodeNode_FailEncode_AfterDuration,				//AVCEncodeNode Error in encode after duration of time.

    /***** Test for Node Commands *****/
    ErrorHandling_MediaInputNode_Node_Cmd_Start,			//545 MediaInputNode Error in node command DoStart().
    ErrorHandling_MediaInputNode_Node_Cmd_Stop,				//MediaInputNode Error in node command DoStop().
    ErrorHandling_MediaInputNode_Node_Cmd_Flush,			//MediaInputNode Error in node command DoFlush().
    ErrorHandling_MediaInputNode_Node_Cmd_Pause,			//MediaInputNode Error in node command DoPause().
    ErrorHandling_MediaInputNode_Node_Cmd_ReleasePort,		//MediaInputNode Error in node command DoReleasePort().
    ErrorHandling_MP4Composer_Node_Cmd_Start,				//MP4ComposerNode Error in the node command DoStart().
    ErrorHandling_MP4Composer_Node_Cmd_Stop,				//MP4ComposerNode Error in the node command DoStop().
    ErrorHandling_MP4Composer_Node_Cmd_Flush,				//552 MP4ComposerNode Error in the node command DoFlush().
    ErrorHandling_MP4Composer_Node_Cmd_Pause,				//MP4ComposerNode Error in the node command DoPause().
    ErrorHandling_MP4Composer_Node_Cmd_ReleasePort,			//MP4ComposerNode Error in the node command DoReleasePort().
    ErrorHandling_VideoEncodeNode_Node_Cmd_Start,			//VideoEncodeNode Error in node command DoStart().
    ErrorHandling_VideoEncodeNode_Node_Cmd_Stop,			//VideoEncodeNode Error in node command DoStop().
    ErrorHandling_VideoEncodeNode_Node_Cmd_Flush,			//VideoEncodeNode Error in node command DoFlush().
    ErrorHandling_VideoEncodeNode_Node_Cmd_Pause,			//VideoEncodeNode Error in node command DoPause().
    ErrorHandling_VideoEncodeNode_Node_Cmd_ReleasePort,		//VideoEncodeNode Error in node command DoReleasePort().
    ErrorHandling_AudioEncodeNode_Node_Cmd_Start,			//560 AudioEncodeNode Error in node command DoStart().
    ErrorHandling_AudioEncodeNode_Node_Cmd_Stop,			//AudioEncodeNode Error in node command DoStop().
    ErrorHandling_AudioEncodeNode_Node_Cmd_Flush,			//AudioEncodeNode Error in node command DoFlush().
    ErrorHandling_AudioEncodeNode_Node_Cmd_Pause,			//AudioEncodeNode Error in node command DoPause().
    ErrorHandling_AudioEncodeNode_Node_Cmd_ReleasePort,		//AudioEncodeNode Error in node command DoReleasePort().
    ErrorHandling_AVCEncodeNode_Node_Cmd_Start,				//AVCEncodeNode Error in node command DoStart().
    ErrorHandling_AVCEncodeNode_Node_Cmd_Stop,				//AVCEncodeNode Error in node command DoStop().
    ErrorHandling_AVCEncodeNode_Node_Cmd_Flush,				//AVCEncodeNode Error in node command DoFlush().
    ErrorHandling_AVCEncodeNode_Node_Cmd_Pause,				//AVCEncodeNode Error in node command DoPause().
    ErrorHandling_AVCEncodeNode_Node_Cmd_ReleasePort,		//569 AVCEncodeNode Error in node command DoReleasePort().

    KUnCompressed_Errorhandling_TestEnd,

    /*********** Error Handling Tests End**************************/
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
    PVAE_CMD_RECORDING,
    PVAE_CMD_QUERY_INTERFACE1,
    PVAE_CMD_CAPCONFIG_SYNC1,
    PVAE_CMD_QUERY_INTERFACE_COMP
} PVAECmdType;


////////////////////////////////////////////////////////////////////////////
class PVLoggerSchedulerSetup
{
    public:
        PVLoggerSchedulerSetup() {};
        ~PVLoggerSchedulerSetup() {};

        void InitLoggerScheduler()
        {
            // Logging by PV Logger
            PVLogger::Init();
            //PVAELogger::ParseConfigFile(_W("uilogger.txt"));
            PVAELogger::ParseConfigFile(KPVAELoggerFile);

            // Construct and install the active scheduler
            OsclScheduler::Init("PVAuthorEngineTestScheduler");
        }

        void CleanupLoggerScheduler()
        {
            OsclScheduler::Cleanup();
            PVLogger::Cleanup();
        }

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

