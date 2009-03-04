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
/**
 * @file test_pv_author_engine_config.h
 * @brief Default settings and type definitions for PVAuthorEngine unit test input
 */

#ifndef TEST_PV_AUTHOR_ENGINE_CONFIG_H_INCLUDED
#define TEST_PV_AUTHOR_ENGINE_CONFIG_H_INCLUDED



// Set to 1 to use the scheduler native to the system instead of PV scheduler
#define USE_NATIVE_SCHEDULER 0

#define SOURCENAME_AUTHOR_PREPEND_STRING _STRLIT("")

#define KLogFile _STRLIT("pvauthor.log")

// Input files
#define KYUVTestInput               SOURCENAME_AUTHOR_PREPEND_STRING"yuvtestinput.yuv"
#define KH263TestInput              SOURCENAME_AUTHOR_PREPEND_STRING"h263testinput.h263"
#define KM4VTestInput               SOURCENAME_AUTHOR_PREPEND_STRING"m4vtestinput.m4v"
#define KAMRTestInput               SOURCENAME_AUTHOR_PREPEND_STRING"amrtestinput.amr"
#define KAACADIFTestInput           SOURCENAME_AUTHOR_PREPEND_STRING"aac_adif.aacadif"
#define KAACADTSTestInput           SOURCENAME_AUTHOR_PREPEND_STRING"aac_adts.aacadts"
#define KPCM16TestInput             SOURCENAME_AUTHOR_PREPEND_STRING"pcm16testinput.pcm"
#define KH264TestInput              SOURCENAME_AUTHOR_PREPEND_STRING"h264testinput.h264"
#define KTEXTTestInput              SOURCENAME_AUTHOR_PREPEND_STRING"MOL004.txt"
#define KTEXTLogTestInput           SOURCENAME_AUTHOR_PREPEND_STRING"MOL004_text0.log"
#define KTEXTTxtFileTestInput       SOURCENAME_AUTHOR_PREPEND_STRING"MOL004_sd_txt1.txt"

// Output files
#define KAMRInputAOnly3gpTestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"amr_in_a_only_test.3gp"
#define KAMRInputAOnlyMp4TestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"amr_in_a_only_test.mp4"
#define KYUVInputVOnly3gpTestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"yuv_in_v_only_test.3gp"
#define KH263InputVOnly3gpTestOutput    SOURCENAME_AUTHOR_PREPEND_STRING"h263_in_v_only_test.3gp"
#define KYUVInputVOnlyMp4TestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"yuv_in_v_only_test.mp4"
#define KM4VInputVOnlyMp4TestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"m4v_in_v_only_test.mp4"
#define KAMRYUVInputAV3gpTestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"amr_yuv_in_av_test.3gp"
#define KAMRH263InputAV3gpTestOutput    SOURCENAME_AUTHOR_PREPEND_STRING"amr_h263_in_av_test.3gp"
#define KAMRYUVInputAVMp4TestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"amr_yuv_in_av_test.mp4"
#define KAMRM4VInputAVMp4TestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"amr_m4v_in_av_test.mp4"
#define KPauseResumeTestOutput          SOURCENAME_AUTHOR_PREPEND_STRING"pause_resume_test.mp4"
#define KFOAOnlyAMRTestOutput           SOURCENAME_AUTHOR_PREPEND_STRING"amr_testoutput.amr"
#define KFOAOnlyAACADIFTestOutput       SOURCENAME_AUTHOR_PREPEND_STRING"aac_adif_output.aac"
#define KFOAOnlyAACADTSTestOutput       SOURCENAME_AUTHOR_PREPEND_STRING"aac_adts_output.aac"
#define KPCM16TestOutput                SOURCENAME_AUTHOR_PREPEND_STRING"pcm_testoutput.amr"
#define KPCM16AOnly3gpTestOutput        SOURCENAME_AUTHOR_PREPEND_STRING"pcm_in_a_only_test.3gp"
#define KPCM16AOnlyMp4TestOutput        SOURCENAME_AUTHOR_PREPEND_STRING"pcm_in_a_only_test.mp4"
#define KPCM16YUVInputAV3gpTestOutput   SOURCENAME_AUTHOR_PREPEND_STRING"pcm_in_av_test.3gp"
#define KPCM16YUVInputAVMp4TestOutput   SOURCENAME_AUTHOR_PREPEND_STRING"pcm_in_av_test.mp4"
#define KMaxFileSizeTestOutput          SOURCENAME_AUTHOR_PREPEND_STRING"maxfilesize_test.3gp"
#define KMaxDurationTestOutput          SOURCENAME_AUTHOR_PREPEND_STRING"maxduration_test.3gp"
#define KFileSizeProgressTestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"filesizeprogress_test.3gp"
#define KDurationProgressTestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"durationprogress_test.3gp"
#define KIMotionAuthoringModeTestOutput SOURCENAME_AUTHOR_PREPEND_STRING"IMotionAuthoring_test.3gp"
#define KFastTrackContentModeTestOutput SOURCENAME_AUTHOR_PREPEND_STRING"FastTrackContentAuthoring_test.3gp"
#define K3GPPDownloadModeTestOutput     SOURCENAME_AUTHOR_PREPEND_STRING"3GPPDownloadAuthoring_test.3gp"
#define K3GPPProgressiveDownloadModeTestOutput SOURCENAME_AUTHOR_PREPEND_STRING"3GPPProgressiveDownloadAuthoring_test.3gp"
#define KIMotionDownloadModeTestOutput  SOURCENAME_AUTHOR_PREPEND_STRING"IMotionDownloadAuthoring_test.3gp"
#define KH264InputVOnlyMp4TestOutput    SOURCENAME_AUTHOR_PREPEND_STRING"h264_in_v_only_test.mp4"
#define KH264AMRInputAVMp4TestTestOutput SOURCENAME_AUTHOR_PREPEND_STRING"h264_amr_in_av_test.mp4"
#define KMovieFragmentModeTestOutput    SOURCENAME_AUTHOR_PREPEND_STRING"movieFragmentFileTest.mp4"
#define KYUVAMRTEXTInputAVTMp4TestOutput SOURCENAME_AUTHOR_PREPEND_STRING"yuv_amr_text_test.mp4"
#define KYUVTEXTInputMp4TestOutput       SOURCENAME_AUTHOR_PREPEND_STRING"yuv_text_test.mp4"
#define KAMRTEXTInputMp4TestOutput       SOURCENAME_AUTHOR_PREPEND_STRING"amr_text_test.mp4"
#define KTEXTInputMp4TestOutput          SOURCENAME_AUTHOR_PREPEND_STRING"text_test.mp4"
//Error Handling TestCases
#define KYUVTestInputWrong					SOURCENAME_AUTHOR_PREPEND_STRING"yuvtestinput_wrong.yuv"
#define KTEXTTestInputWrong					SOURCENAME_AUTHOR_PREPEND_STRING"texttestinput_wrong.txt"
//specify wrong path
#define KAMRYUVInputAV3gpTestOutputWrong	SOURCENAME_AUTHOR_PREPEND_STRING"../../../test_input_wrong/amr_yuv_in_av_test.3gp"
#define KAMRTestInputWrong					SOURCENAME_AUTHOR_PREPEND_STRING"amrtestinput_wrong.amr"

#define DEFAULTSOURCEFILENAME				"testinput.avi"
#define DEFAULTSOURCEFORMATTYPE			  	PVMF_AVIFF
#define DEFAULTOUTPUTFILENAME				"testoutput.mp4"

#define FILE_NAME_ERROR_HANDLING			"testinput_rgb16.avi"
#define WRONGIPFILENAME_ERRORHANDLING		"/wrongdir/testinput.avi"
#endif // TEST_PV_AUTHOR_ENGINE_CONFIG_H_INCLUDED


