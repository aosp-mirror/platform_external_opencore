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
#ifndef TEST_PV_AUTHOR_ENGINE_TESTSET1_H_INCLUDED
#include "test_pv_author_engine_testset1.h"
#endif

#ifndef PVMF_COMPOSER_SIZE_AND_DURATION_H_INCLUDED
#include "pvmf_composer_size_and_duration.h"
#endif

#ifndef PVMF_FILEOUTPUT_CONFIG_H_INCLUDED
#include "pvmf_fileoutput_config.h"
#endif

#ifndef PVMP4FFCN_CLIPCONFIG_H_INCLUDED
#include "pvmp4ffcn_clipconfig.h"
#endif

#ifndef PV_MP4_H263_ENC_EXTENSION_H_INCLUDED
#include "pvmp4h263encextension.h"
#endif

#ifndef PVAETEST_NODE_CONFIG_H_INCLUDED
#include "pvaetest_node_config.h"
#endif

void pvauthor_async_test_miscellaneous::StartTest()
{
    AddToScheduler();
    iState = PVAE_CMD_CREATE;
    RunIfNotReady();
}



////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_miscellaneous::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    OSCL_UNUSED_ARG(aEvent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR, (0, "pvauthor_async_test_miscellaneous::HandleErrorEvent"));
    iState = PVAE_CMD_RESET;
    RunIfNotReady();
}

////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_miscellaneous::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::HandleInformationalEvent"));

    OsclAny* eventData = NULL;
    switch (aEvent.GetEventType())
    {
        case PVMF_COMPOSER_MAXFILESIZE_REACHED:
        case PVMF_COMPOSER_MAXDURATION_REACHED:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::HandleNodeInformationalEvent: Max file size reached"));
            Cancel();
            PVPATB_TEST_IS_TRUE(true);
            iObserver->CompleteTest(*iTestCase);
            break;

        case PVMF_COMPOSER_DURATION_PROGRESS:
            aEvent.GetEventData(eventData);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::HandleNodeInformationalEvent: Duration progress: %d ms",
                             (int32)eventData));
            fprintf(iStdOut, "Duration: %d ms\n", (int32)eventData);
            break;

        case PVMF_COMPOSER_FILESIZE_PROGRESS:
            aEvent.GetEventData(eventData);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::HandleNodeInformationalEvent: File size progress: %d bytes",
                             (int32)eventData));
            fprintf(iStdOut, "File size: %d bytes\n", (int32)eventData);
            break;

        case PVMF_COMPOSER_EOS_REACHED:
            //Engine already stopped at EOS so send reset command.
            iState = PVAE_CMD_RESET;
            //cancel recording timeout scheduled for timer object.
            Cancel();
            RunIfNotReady();
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
int pvauthor_async_test_miscellaneous::CreateAudioInput()
{
    int status = 0;
    PVAETestInput testInput;

    switch (iTestCaseNum)
    {
        case AMR_FOutput_Test:
        case AMR_Input_AOnly_3gpTest:
        case AMR_Input_AOnly_Mp4Test:
        case AMR_YUV_Input_AV_3gpTest:
        case AMR_H263_Input_AV_3gpTest:
        case AMR_YUV_Input_AV_Mp4Test:
        case Pause_Resume_Test:
        case KMaxFileSizeTest:
        case KMaxDurationTest:
        case KFileSizeProgressTest:
        case KDurationProgressTest:
        case KFastTrackContentModeTest:
        case K3GPPDownloadModeTest:
        case K3GPPProgressiveDownloadModeTest:
        case KMovieFragmentModeTest:
        case H264_AMR_Input_AV_Mp4Test:
        case H264_AMR_Input_AV_Mp4_LongetivityTest:
        case AMR_FOutput_LongetivityTest:
        case AMR_Input_AOnly_3gp_LongetivityTest:
        case AMR_Input_AOnly_Mp4_LongetivityTest:
        case AMR_YUV_Input_AV_3gp_LongetivityTest:
        case AMR_H263_Input_AV_3gp_LongetivityTest:
        case AMR_YUV_Input_AV_Mp4_LongetivityTest:
        case AMR_TEXT_Input_AT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:

        case KMaxFileSizeLongetivityTest:
        case KMaxDurationLongetivityTest:
        case KFileSizeProgressLongetivityTest:
        case KDurationProgressLongetivityTest:
        case KFastTrackContentModeLongetivityTest:
        case K3GPPDownloadModeLongetivityTest:
        case K3GPPProgressiveDownloadModeLongetivityTest:
        case AMR_TEXT_Input_AT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateAudioTestInput: AMR input"));
            if (testInput.IsTestInputTypeSupported(SYMBIAN_DEV_SOUND))
            {
                iAudioInputType = SYMBIAN_DEV_SOUND;
                status = testInput.CreateInputNode(SYMBIAN_DEV_SOUND, iInputFileNameAudio, iAVTConfig);
            }
            else if (testInput.IsTestInputTypeSupported(AMR_IETF_FILE))
            {
                iAudioInputType = AMR_IETF_FILE;
                status = testInput.CreateInputNode(AMR_IETF_FILE, iInputFileNameAudio, iAVTConfig);
            }
            break;

        case AACADIF_FOutput_Test:
        case AACADIF_FOutput_LongetivityTest:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateAudioTestInput: AAC-ADIF input"));
            if (testInput.IsTestInputTypeSupported(AAC_ADIF_FILE))
            {
                iAudioInputType = AAC_ADIF_FILE;
                status = testInput.CreateInputNode(AAC_ADIF_FILE, iInputFileNameAudio, iAVTConfig);
            }
            break;


        case AACADTS_FOutput_Test:
        case AACADTS_FOutput_LongetivityTest:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateAudioTestInput: AAC-ADTS input"));
            if (testInput.IsTestInputTypeSupported(AAC_ADTS_FILE))
            {
                iAudioInputType = AAC_ADTS_FILE;
                status = testInput.CreateInputNode(AAC_ADTS_FILE, iInputFileNameAudio, iAVTConfig);
            }
            break;

        case PCM16In_AMROut_Test:
        case PCM16_Input_AOnly_3gpTest:
        case PCM16_Input_AOnly_Mp4Test:
        case PCM16_YUV_Input_AV_3gpTest:
        case PCM16_YUV_Input_AV_Mp4Test:
        case PCM16In_AMROut_LongetivityTest:
        case PCM16_Input_AOnly_3gp_LongetivityTest:
        case PCM16_Input_AOnly_Mp4_LongetivityTest:
        case PCM16_YUV_Input_AV_3gp_LongetivityTest:
        case PCM16_YUV_Input_AV_Mp4_LongetivityTest:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateAudioTestInput: PCM16 input"));
            if (testInput.IsTestInputTypeSupported(PCM16_FILE))
            {
                iAudioInputType = PCM16_FILE;
                status = testInput.CreateInputNode(PCM16_FILE, iInputFileNameAudio, iAVTConfig);
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateAudioTestInput: Audio input node not needed for this test case"));
            return -1;
            break;
    }

    if (!status)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::CreateAudioTestInput: Error - Create input node failed"));
        return status;
    }

    return AddDataSource(testInput);
}

////////////////////////////////////////////////////////////////////////////
int pvauthor_async_test_miscellaneous::CreateVideoInput()
{
    int status = 0;
    PVAETestInput testInput;

    switch (iTestCaseNum)
    {
        case YUV_Input_VOnly_3gpTest:
        case YUV_Input_VOnly_Mp4Test:
        case AMR_YUV_Input_AV_3gpTest:
        case AMR_YUV_Input_AV_Mp4Test:
        case PCM16_YUV_Input_AV_3gpTest:
        case PCM16_YUV_Input_AV_Mp4Test:
        case Pause_Resume_Test:
        case KMaxFileSizeTest:
        case KMaxDurationTest:
        case KFileSizeProgressTest:
        case KDurationProgressTest:
        case KFastTrackContentModeTest:
        case K3GPPDownloadModeTest:
        case K3GPPProgressiveDownloadModeTest:
        case KMovieFragmentModeTest:
        case YUV_Input_VOnly_3gp_LongetivityTest:
        case YUV_Input_VOnly_Mp4_LongetivityTest:
        case AMR_YUV_Input_AV_3gp_LongetivityTest:
        case AMR_YUV_Input_AV_Mp4_LongetivityTest:
        case PCM16_YUV_Input_AV_3gp_LongetivityTest:
        case PCM16_YUV_Input_AV_Mp4_LongetivityTest:
        case YUV_TEXT_Input_VT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:

        case KMaxFileSizeLongetivityTest:
        case KMaxDurationLongetivityTest:
        case KFileSizeProgressLongetivityTest:
        case KDurationProgressLongetivityTest:
        case KFastTrackContentModeLongetivityTest:
        case K3GPPDownloadModeLongetivityTest:
        case K3GPPProgressiveDownloadModeLongetivityTest:
        case YUV_TEXT_Input_VT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateVideoTestInput: YUV input"));
            if (testInput.IsTestInputTypeSupported(YUV_FILE))
            {
                iVideoInputType = YUV_FILE;
                testInput = PVAETestInput();
                status = testInput.CreateInputNode(YUV_FILE, iInputFileNameVideo, iAVTConfig);
            }
            break;

        case H263_Input_VOnly_3gpTest:
        case AMR_H263_Input_AV_3gpTest:
        case H263_Input_VOnly_3gp_LongetivityTest:
        case AMR_H263_Input_AV_3gp_LongetivityTest:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateVideoTestInput: H263 input"));
            if (testInput.IsTestInputTypeSupported(H263_FILE))
            {
                iVideoInputType = H263_FILE;
                testInput = PVAETestInput();
                status = testInput.CreateInputNode(H263_FILE, iInputFileNameVideo, iAVTConfig);
            }
            break;
        case H264_Input_VOnly_Mp4Test:
        case H264_AMR_Input_AV_Mp4Test:
        case H264_Input_VOnly_Mp4_LongetivityTest:
        case H264_AMR_Input_AV_Mp4_LongetivityTest:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateVideoTestInput: YUV input"));
            if (testInput.IsTestInputTypeSupported(H264_FILE))
            {
                iVideoInputType = H264_FILE;
                testInput = PVAETestInput();
                status = testInput.CreateInputNode(H264_FILE, iInputFileNameVideo, iAVTConfig);
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateVideoTestInput: Video input node not needed for this test case"));
            return -1;
            break;
    }

    if (!status)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::CreateTestInputs: Error - CreateInputNode failed"));
        return status;
    }

    return AddDataSource(testInput);
}

////////////////////////////////////////////////////////////////////////////
int pvauthor_async_test_miscellaneous::CreateTextInput()
{
    int status = 0;
    PVAETestInput testInput;

    switch (iTestCaseNum)
    {
        case TEXT_Input_TOnly_Mp4Test:
        case AMR_TEXT_Input_AT_Mp4Test:
        case YUV_TEXT_Input_VT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
        case TEXT_Input_TOnly_3gp_LongetivityTest:
        case AMR_TEXT_Input_AT_3gp_LongetivityTest:
        case YUV_TEXT_Input_VT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateTextTestInput: Text input"));
            if (testInput.IsTestInputTypeSupported(TEXT_FILE))
            {
                iTextInputType = TEXT_FILE;
                testInput = PVAETestInput();
                status = testInput.CreateInputNode(TEXT_FILE, iInputFileNameText, iAVTConfig);
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_DEBUG,
                            (0, "pvauthor_async_test_miscellaneous::CreateTextTestInput: Text input node not needed for this test case"));
            return -1;
            break;
    }

    if (!status)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::CreateTestInputs: Error - CreateInputNode failed"));
        return status;
    }

    return AddDataSource(testInput);
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::AddDataSource(PVAETestInput& aInput)
{
    int32 err = 0;

    OSCL_TRY(err, iTestInputs.push_back(aInput););
    if (err != OSCL_ERR_NONE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddDataSource: Error - iTestInputs.push_back failed. err=0x%x", err));
        aInput.DeleteInputNode();
        return false;
    }

    OSCL_TRY(err, iAuthor->AddDataSource(*(aInput.iNode), (OsclAny*)iAuthor););
    if (err != OSCL_ERR_NONE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddDataSource: Error - iAuthor->AddDataSource failed. err=0x%x", err));
        aInput.DeleteInputNode();
        return false;
    }

    return true;
}

void pvauthor_async_test_miscellaneous::SelectComposer()
{
    switch (iTestCaseNum)
    {
        case AMR_Input_AOnly_3gpTest:
        case PCM16_Input_AOnly_3gpTest:
        case PCM16_YUV_Input_AV_3gpTest:
        case YUV_Input_VOnly_3gpTest:
        case H263_Input_VOnly_3gpTest:
        case AMR_YUV_Input_AV_3gpTest:
        case AMR_H263_Input_AV_3gpTest:
        case Pause_Resume_Test:
        case KMaxFileSizeTest:
        case KMaxDurationTest:
        case KFileSizeProgressTest:
        case KDurationProgressTest:
        case KFastTrackContentModeTest:
        case K3GPPDownloadModeTest:
        case K3GPPProgressiveDownloadModeTest:
        case KMovieFragmentModeTest:
        case AMR_Input_AOnly_3gp_LongetivityTest:
        case PCM16_Input_AOnly_3gp_LongetivityTest:
        case PCM16_YUV_Input_AV_3gp_LongetivityTest:
        case YUV_Input_VOnly_3gp_LongetivityTest:
        case H263_Input_VOnly_3gp_LongetivityTest:
        case AMR_YUV_Input_AV_3gp_LongetivityTest:
        case AMR_H263_Input_AV_3gp_LongetivityTest:
        case TEXT_Input_TOnly_3gp_LongetivityTest:
        case AMR_TEXT_Input_AT_3gp_LongetivityTest:
        case YUV_TEXT_Input_VT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:

        case KMaxFileSizeLongetivityTest:
        case KMaxDurationLongetivityTest:
        case KFileSizeProgressLongetivityTest:
        case KDurationProgressLongetivityTest:
        case KFastTrackContentModeLongetivityTest:
        case K3GPPDownloadModeLongetivityTest:
        case K3GPPProgressiveDownloadModeLongetivityTest:
            iComposerMimeType = K3gpComposerMimeType;
            break;

        case AMR_Input_AOnly_Mp4Test:
        case PCM16_Input_AOnly_Mp4Test:
        case PCM16_YUV_Input_AV_Mp4Test:
        case YUV_Input_VOnly_Mp4Test:
        case AMR_YUV_Input_AV_Mp4Test:
        case AMR_Input_AOnly_Mp4_LongetivityTest:
        case PCM16_Input_AOnly_Mp4_LongetivityTest:
        case PCM16_YUV_Input_AV_Mp4_LongetivityTest:
        case YUV_Input_VOnly_Mp4_LongetivityTest:
        case AMR_YUV_Input_AV_Mp4_LongetivityTest:
        case H264_Input_VOnly_Mp4Test:
        case H264_AMR_Input_AV_Mp4Test:
        case H264_Input_VOnly_Mp4_LongetivityTest:
        case H264_AMR_Input_AV_Mp4_LongetivityTest:
        case TEXT_Input_TOnly_Mp4Test:
        case AMR_TEXT_Input_AT_Mp4Test:
        case YUV_TEXT_Input_VT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
            iComposerMimeType = KMp4ComposerMimeType;
            break;

        case AMR_FOutput_Test:
        case PCM16In_AMROut_Test:
        case AMR_FOutput_LongetivityTest:
        case PCM16In_AMROut_LongetivityTest:
            iComposerMimeType = KAMRNbComposerMimeType;
            break;

        case AACADIF_FOutput_Test:
        case AACADIF_FOutput_LongetivityTest:
            iComposerMimeType = KAACADIFComposerMimeType;
            break;

        case AACADTS_FOutput_Test:
        case AACADTS_FOutput_LongetivityTest:
            iComposerMimeType = KAACADTSComposerMimeType;
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                            (0, "pvauthor_async_test_miscellaneous::SelectComposer: Error - No composer type for test case"));
            PVPATB_TEST_IS_TRUE(false);
            iObserver->CompleteTest(*iTestCase);
            return;
    }

    iAuthor->SelectComposer(iComposerMimeType, iComposerConfig, (OsclAny*)iAuthor);
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::ConfigComposer()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::ConfigComposer"));
    if (!ConfigAmrAacComposer())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::ConfigComposer: Error - ConfigAmrAacComposer failed"));
        return false;
    }

    if (!ConfigMp43gpComposer())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::ConfigComposer: Error - ConfigMp43gpComposer failed"));
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::ConfigAmrAacComposer()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::ConfigAmrAacComposer"));

    PVMFFormatType format = PVMF_FORMAT_UNKNOWN;

    switch (iTestCaseNum)
    {
        case AMR_FOutput_Test:
        case AMR_FOutput_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KFOAOnlyAMRTestOutput;
            }
            format = PVMF_AMR_IETF;
            break;

        case AACADIF_FOutput_Test:
        case AACADIF_FOutput_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KFOAOnlyAACADIFTestOutput;
            }
            format = PVMF_ADIF;
            break;


        case AACADTS_FOutput_Test:
        case AACADTS_FOutput_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KFOAOnlyAACADTSTestOutput;
            }
            format = PVMF_ADTS;
            break;

        case PCM16In_AMROut_Test:
        case PCM16In_AMROut_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KPCM16TestOutput;
            }
            format = PVMF_AMR_IETF;
            break;

        default:
            if (!((iComposerMimeType == KAMRNbComposerMimeType) || (iComposerMimeType == KAACADTSComposerMimeType) || (iComposerMimeType == KAACADIFComposerMimeType)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "pvauthor_async_test_miscellaneous::ConfigAmrAacComposer: AMR-AAC Composer not used in this test case"));
                return true;
            }
    }

    PvmfFileOutputNodeConfigInterface* clipConfig = OSCL_STATIC_CAST(PvmfFileOutputNodeConfigInterface*, iComposerConfig);
    if (!clipConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::ConfigAmrAacComposer: Error - Invalid iComposerConfig"));
        return false;
    }

    if (clipConfig->SetOutputFileName(iOutputFileName) != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::ConfigAmrAacComposer: Error - SetOutputFileName failed"));
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::ConfigMp43gpComposer()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "pvauthor_async_test_miscellaneous::ConfigMp43gpComposer"));

    switch (iTestCaseNum)
    {
        case AMR_Input_AOnly_3gpTest:
        case AMR_Input_AOnly_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KAMRInputAOnly3gpTestOutput;
            }
            break;
        case AMR_Input_AOnly_Mp4Test:
        case AMR_Input_AOnly_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KAMRInputAOnlyMp4TestOutput;
            }
            break;
        case YUV_Input_VOnly_3gpTest:
        case YUV_Input_VOnly_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KYUVInputVOnly3gpTestOutput;
            }
            break;
        case H263_Input_VOnly_3gpTest:
        case H263_Input_VOnly_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KH263InputVOnly3gpTestOutput;
            }
            break;
        case YUV_Input_VOnly_Mp4Test:
        case YUV_Input_VOnly_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KYUVInputVOnlyMp4TestOutput;
            }
            break;
        case AMR_YUV_Input_AV_3gpTest:
        case AMR_YUV_Input_AV_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KAMRYUVInputAV3gpTestOutput;
            }
            break;
        case AMR_H263_Input_AV_3gpTest:
        case AMR_H263_Input_AV_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KAMRH263InputAV3gpTestOutput;
            }
            break;
        case AMR_YUV_Input_AV_Mp4Test:
        case AMR_YUV_Input_AV_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KAMRYUVInputAVMp4TestOutput;
            }
            break;
        case KMaxFileSizeTest:
        case KMaxFileSizeLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KMaxFileSizeTestOutput;
            }
            break;
        case KMaxDurationTest:
        case KMaxDurationLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KMaxDurationTestOutput;
            }
            break;
        case KFileSizeProgressTest:
        case KFileSizeProgressLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KFileSizeProgressTestOutput;
            }
            break;
        case KDurationProgressTest:
        case KDurationProgressLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KDurationProgressTestOutput;
            }
            break;
        case PCM16_Input_AOnly_3gpTest:
        case PCM16_Input_AOnly_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KPCM16AOnly3gpTestOutput;
            }
            break;
        case PCM16_Input_AOnly_Mp4Test:
        case PCM16_Input_AOnly_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KPCM16AOnlyMp4TestOutput;
            }
            break;
        case PCM16_YUV_Input_AV_3gpTest:
        case PCM16_YUV_Input_AV_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KPCM16YUVInputAV3gpTestOutput;
            }
            break;
        case PCM16_YUV_Input_AV_Mp4Test:
        case PCM16_YUV_Input_AV_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KPCM16YUVInputAVMp4TestOutput;
            }
            break;
        case Pause_Resume_Test:

            if (iOutputFileName == NULL)
            {
                iOutputFileName = KPauseResumeTestOutput;
            }
            break;

        case KFastTrackContentModeTest:
        case KFastTrackContentModeLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KFastTrackContentModeTestOutput;
            }
            break;
        case K3GPPDownloadModeTest:
        case K3GPPDownloadModeLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = K3GPPDownloadModeTestOutput;
            }
            break;
        case K3GPPProgressiveDownloadModeTest:
        case K3GPPProgressiveDownloadModeLongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = K3GPPProgressiveDownloadModeTestOutput;
            }
            break;

        case KMovieFragmentModeTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KMovieFragmentModeTestOutput;
            }
            break;

        case H264_Input_VOnly_Mp4Test:
        case H264_Input_VOnly_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KH264InputVOnlyMp4TestOutput;
            }
            break;
        case H264_AMR_Input_AV_Mp4Test:
        case H264_AMR_Input_AV_Mp4_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KH264AMRInputAVMp4TestTestOutput;
            }
            break;
        case TEXT_Input_TOnly_Mp4Test:
        case TEXT_Input_TOnly_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KTEXTInputMp4TestOutput;
            }
            break;
        case AMR_TEXT_Input_AT_Mp4Test:
        case AMR_TEXT_Input_AT_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KAMRTEXTInputMp4TestOutput;
            }
            break;
        case YUV_TEXT_Input_VT_Mp4Test:
        case YUV_TEXT_Input_VT_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KYUVTEXTInputMp4TestOutput;
            }
            break;
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:
            if (iOutputFileName == NULL)
            {
                iOutputFileName = KYUVAMRTEXTInputAVTMp4TestOutput;
            }
            break;
        default:
            if (!(iComposerMimeType == KAMRNbComposerMimeType) && !(iComposerMimeType == KAACADTSComposerMimeType) && !(iComposerMimeType == KAACADIFComposerMimeType))
            {
                break;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "pvauthor_async_test_miscellaneous::ConfigMp43gpComposer: Mp4-3GPP composer not used in this test case"));
                return true;
            }
    }

    PVMp4FFCNClipConfigInterface* clipConfig;
    clipConfig = OSCL_STATIC_CAST(PVMp4FFCNClipConfigInterface*, iComposerConfig);
    if (!clipConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::ConfigMp43gpComposer: Error - iComposerConfig==NULL"));
        return false;
    }

    iVersionString = _STRLIT("version");
    iTitleString = _STRLIT("title");
    iAuthorString = _STRLIT("author");
    iCopyrightString = _STRLIT("copyright");
    iDescriptionString = _STRLIT("description");
    iRatingString = _STRLIT("rating");

    clipConfig->SetOutputFileName(iOutputFileName);
    clipConfig->SetPresentationTimescale(1000);
    clipConfig->SetVersion(iVersionString);
    clipConfig->SetTitle(iTitleString);
    clipConfig->SetAuthor(iAuthorString);
    clipConfig->SetCopyright(iCopyrightString);
    clipConfig->SetDescription(iDescriptionString);
    clipConfig->SetRating(iRatingString);
    if (iTestCaseNum == KFastTrackContentModeTest)
    {
        clipConfig->SetAuthoringMode(PVMP4FFCN_PV_FAST_TRACK_CONTENT_MODE);
    }
    else if (iTestCaseNum == K3GPPDownloadModeTest)
    {
        clipConfig->SetAuthoringMode(PVMP4FFCN_3GPP_DOWNLOAD_MODE);
    }
    else if (iTestCaseNum == K3GPPProgressiveDownloadModeTest)
    {
        clipConfig->SetAuthoringMode(PVMP4FFCN_3GPP_PROGRESSIVE_DOWNLOAD_MODE);
    }
    else if (iTestCaseNum == KMovieFragmentModeTest)
    {
        clipConfig->SetAuthoringMode(PVMP4FFCN_MOVIE_FRAGMENT_MODE);
    }


    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::ConfigComposerOutput()
{
    PvmfComposerSizeAndDurationInterface* config =
        OSCL_REINTERPRET_CAST(PvmfComposerSizeAndDurationInterface*, iOutputSizeAndDurationConfig);
    if (!config)
    {
        return false;
    }
    bool enabled = false;
    uint32 configData = 0;
    switch (iTestCaseNum)
    {
        case KMaxFileSizeTest:
        case KMaxFileSizeLongetivityTest:
            if (config->SetMaxFileSize(true, KMaxFileSize) != PVMFSuccess)
            {
                return false;
            }
            config->GetMaxFileSizeConfig(enabled, configData);
            if (!enabled || configData != KMaxFileSize)
            {
                return false;
            }
            break;

        case KMaxDurationLongetivityTest:
        case KMaxDurationTest:
            if (config->SetMaxDuration(true, KMaxDuration) != PVMFSuccess)
            {
                return false;
            }
            config->GetMaxDurationConfig(enabled, configData);
            if (!enabled || configData != KMaxDuration)
            {
                return false;
            }
            break;

        case KFileSizeProgressLongetivityTest:
        case KFileSizeProgressTest:
            if (config->SetFileSizeProgressReport(true, KFileSizeProgressFreq) != PVMFSuccess)
            {
                return false;
            }
            config->GetFileSizeProgressReportConfig(enabled, configData);
            if (!enabled || configData != KFileSizeProgressFreq)
            {
                return false;
            }
            break;

        case KDurationProgressTest:
        case KDurationProgressLongetivityTest:
            if (config->SetDurationProgressReport(true, KDurationProgressFreq) != PVMFSuccess)
            {
                return false;
            }
            config->GetDurationProgressReportConfig(enabled, configData);
            if (!enabled || configData != KDurationProgressFreq)
            {
                return false;
            }
            break;

        default:
            break;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::QueryComposerOutputInterface()
{
    switch (iTestCaseNum)
    {
        case KMaxFileSizeTest:
        case KMaxDurationTest:
        case KFileSizeProgressTest:
        case KDurationProgressTest:
        case KMaxFileSizeLongetivityTest:
        case KMaxDurationLongetivityTest:
        case KFileSizeProgressLongetivityTest:
        case KDurationProgressLongetivityTest:
        case Generic_QueryInterface_Reset_Test:
            iAuthor->QueryInterface(PvmfComposerSizeAndDurationUuid,
                                    iOutputSizeAndDurationConfig, (OsclAny*)iAuthor);
            return true;

        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::AddAudioMediaTrack()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::AddAudioMediaTrack"));

    switch (iTestCaseNum)
    {
        case AMR_Input_AOnly_3gpTest:
        case AMR_Input_AOnly_Mp4Test:
        case AMR_YUV_Input_AV_3gpTest:
        case AMR_H263_Input_AV_3gpTest:
        case AMR_YUV_Input_AV_Mp4Test:
        case PCM16In_AMROut_Test:
        case PCM16_Input_AOnly_3gpTest:
        case PCM16_Input_AOnly_Mp4Test:
        case PCM16_YUV_Input_AV_3gpTest:
        case PCM16_YUV_Input_AV_Mp4Test:
        case Pause_Resume_Test:
        case AMR_TEXT_Input_AT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
        case AMR_FOutput_Test:
        case KMaxFileSizeTest:
        case KMaxDurationTest:
        case KFileSizeProgressTest:
        case KDurationProgressTest:
        case KFastTrackContentModeTest:
        case K3GPPDownloadModeTest:
        case K3GPPProgressiveDownloadModeTest:
        case KMovieFragmentModeTest:
        case H264_AMR_Input_AV_Mp4Test:
        case H264_AMR_Input_AV_Mp4_LongetivityTest:
        case AMR_Input_AOnly_3gp_LongetivityTest:
        case AMR_Input_AOnly_Mp4_LongetivityTest:
        case AMR_YUV_Input_AV_3gp_LongetivityTest:
        case AMR_H263_Input_AV_3gp_LongetivityTest:
        case AMR_YUV_Input_AV_Mp4_LongetivityTest:
        case AMR_TEXT_Input_AT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:
        case PCM16In_AMROut_LongetivityTest:
        case PCM16_Input_AOnly_3gp_LongetivityTest:
        case PCM16_Input_AOnly_Mp4_LongetivityTest:
        case PCM16_YUV_Input_AV_3gp_LongetivityTest:
        case PCM16_YUV_Input_AV_Mp4_LongetivityTest:

        case AMR_FOutput_LongetivityTest:
        case KMaxFileSizeLongetivityTest:
        case KMaxDurationLongetivityTest:
        case KFileSizeProgressLongetivityTest:
        case KDurationProgressLongetivityTest:
        case KFastTrackContentModeLongetivityTest:
        case K3GPPDownloadModeLongetivityTest:
        case K3GPPProgressiveDownloadModeLongetivityTest:
            iAudioEncoderMimeType = KAMRNbEncMimeType;
            break;


        case AACADIF_FOutput_Test:
        case AACADIF_FOutput_LongetivityTest:
            iAudioEncoderMimeType = KAACADIFEncMimeType;
            break;
        case AACADTS_FOutput_Test:
        case AACADTS_FOutput_LongetivityTest:
            iAudioEncoderMimeType = KAACADTSEncMimeType;
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "pvauthor_async_test_miscellaneous::AddAudioMediaTrackL: Error - No audio for this current test case"));
            return false;
    }

    if (iAudioInputType == INVALID_INPUT_TYPE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddAudioMediaTrackL: Error - Invalid audio input type"));
        return false;
    }

    bool testInputFound = false;
    PVAETestInput testInput;
    for (uint32 ii = 0; ii < iTestInputs.size(); ii++)
    {
        if (iTestInputs[ii].iType == iAudioInputType)
        {
            testInputFound = true;
            testInput = iTestInputs[ii];
            break;
        }
    }

    if (!testInputFound)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddAudioMediaTrack: Error - Test input not found"));
        return false;
    }

    iAuthor->AddMediaTrack(*(testInput.iNode), iAudioEncoderMimeType, iComposer,
                           iAudioEncoderConfig, (OsclAny*)iAuthor);
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::AddVideoMediaTrack()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::AddVideoMediaTrack"));

    // Add video media track
    switch (iTestCaseNum)
    {
        case YUV_Input_VOnly_3gpTest:
        case AMR_YUV_Input_AV_3gpTest:
        case H263_Input_VOnly_3gpTest:
        case AMR_H263_Input_AV_3gpTest:
        case PCM16_YUV_Input_AV_3gpTest:
        case KMaxFileSizeTest:
        case KMaxDurationTest:
        case KFileSizeProgressTest:
        case KDurationProgressTest:
        case KFastTrackContentModeTest:
        case K3GPPDownloadModeTest:
        case K3GPPProgressiveDownloadModeTest:
        case KMovieFragmentModeTest:
            iVideoEncoderMimeType = KH263EncMimeType;
            break;
        case YUV_Input_VOnly_3gp_LongetivityTest:
        case AMR_YUV_Input_AV_3gp_LongetivityTest:
        case H263_Input_VOnly_3gp_LongetivityTest:
        case AMR_H263_Input_AV_3gp_LongetivityTest:
        case PCM16_YUV_Input_AV_3gp_LongetivityTest:
        case YUV_TEXT_Input_VT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:
        case KMaxFileSizeLongetivityTest:
        case KMaxDurationLongetivityTest:
        case KFileSizeProgressLongetivityTest:
        case KDurationProgressLongetivityTest:
        case KFastTrackContentModeLongetivityTest:
        case K3GPPDownloadModeLongetivityTest:
        case K3GPPProgressiveDownloadModeLongetivityTest:
            iVideoEncoderMimeType = KH263EncMimeType;
            break;


        case YUV_Input_VOnly_Mp4Test:
        case AMR_YUV_Input_AV_Mp4Test:
        case PCM16_YUV_Input_AV_Mp4Test:
        case Pause_Resume_Test:
        case YUV_TEXT_Input_VT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
        case YUV_Input_VOnly_Mp4_LongetivityTest:
        case AMR_YUV_Input_AV_Mp4_LongetivityTest:
        case PCM16_YUV_Input_AV_Mp4_LongetivityTest:
            iVideoEncoderMimeType = KMp4EncMimeType;
            break;
        case H264_Input_VOnly_Mp4Test:
        case H264_AMR_Input_AV_Mp4Test:
        case H264_Input_VOnly_Mp4_LongetivityTest:
        case H264_AMR_Input_AV_Mp4_LongetivityTest:
            iVideoEncoderMimeType = KH264EncMimeType;
            break;
        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "pvauthor_async_test_miscellaneous::AddVideoMediaTrack: Video encoder not used in this test case."));
            return false;
            break;
    }

    if (iVideoInputType == INVALID_INPUT_TYPE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddVideoMediaTrack: Error - Invalid input type"));
        return false;
    }

    bool testInputFound = false;
    PVAETestInput testInput;

    for (uint32 ii = 0; ii < iTestInputs.size(); ii++)
    {
        if (iTestInputs[ii].iType == iVideoInputType)
        {
            testInputFound = true;
            testInput = iTestInputs[ii];
            break;
        }
    }

    if (!testInputFound)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddVideoMediaTrack: Error - Test input not found"));
        return false;
    }

    iAuthor->AddMediaTrack(*(testInput.iNode), iVideoEncoderMimeType, iComposer,
                           iVideoEncoderConfig, (OsclAny*)iAuthor);
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::AddTextMediaTrack()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::AddTextMediaTrack"));

    switch (iTestCaseNum)
    {
        case TEXT_Input_TOnly_Mp4Test:
        case AMR_TEXT_Input_AT_Mp4Test:
        case YUV_TEXT_Input_VT_Mp4Test:
        case AMR_YUV_TEXT_Input_AVT_Mp4Test:
        case TEXT_Input_TOnly_3gp_LongetivityTest:
        case AMR_TEXT_Input_AT_3gp_LongetivityTest:
        case YUV_TEXT_Input_VT_3gp_LongetivityTest:
        case AMR_YUV_TEXT_Input_AVT_3gp_LongetivityTest:
            iTextEncoderMimeType = KTextEncMimeType;
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "pvauthor_async_test_miscellaneous::AddTextMediaTrackL: Error - No Text for this current test case"));
            return false;
    }

    if (iTextInputType == INVALID_INPUT_TYPE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddAudioMediaTrackL: Error - Invalid Text input type"));
        return false;
    }

    bool testInputFound = false;
    PVAETestInput testInput;
    for (uint32 ii = 0; ii < iTestInputs.size(); ii++)
    {
        if (iTestInputs[ii].iType == iTextInputType)
        {
            testInputFound = true;
            testInput = iTestInputs[ii];
            break;
        }
    }

    if (!testInputFound)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_miscellaneous::AddAudioMediaTrack: Error - Test input not found"));
        return false;
    }

    iAuthor->AddMediaTrack(*(testInput.iNode), iTextEncoderMimeType, iComposer,
                           iTextEncoderConfig, (OsclAny*)iAuthor);
    return true;
}
bool pvauthor_async_test_miscellaneous::ConfigureVideoEncoder()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::ConfigureVideoEncoder"));

    PVMp4H263EncExtensionInterface* config;
    config = OSCL_STATIC_CAST(PVMp4H263EncExtensionInterface*, iVideoEncoderConfig);
    if (!config)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "pvauthor_async_test_miscellaneous::ConfigureVideoEncoder: No configuration needed"));
        return true;
    }

    config->SetNumLayers(1);
    config->SetOutputBitRate(0, KVideoBitrate);
    config->SetOutputFrameSize(0, iAVTConfig.iWidth , iAVTConfig.iHeight);
    config->SetOutputFrameRate(0, iAVTConfig.iFps);
    config->SetIFrameInterval(iAVTConfig.iFrameInterval);

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::ConfigureAudioEncoder()
{
    //Single core AMR encoder node support
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::ConfigureAudioEncoder"));

    if (!iAudioEncoderConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "pvauthor_async_test_miscellaneous::ConfigureAudioEncoder: No configuration needed"));
        return true;
    }

    return PVAETestNodeConfig::ConfigureAudioEncoder(iAudioEncoderConfig);
}


////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::ConfigureTextEncoder()
{
    //Single core AMR encoder node support
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::ConfigureAudioEncoder"));

    if (!iTextEncoderConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "pvauthor_async_test_miscellaneous::ConfigureAudioEncoder: No configuration needed"));
        return true;
    }

    return true;
}

uint32 pvauthor_async_test_miscellaneous::GetMin(uint32 a, uint32 b)
{
    if (!a)
    {
        return b;
    }
    else if (!b)
    {
        return a;
    }
    else
    {
        return (a < b) ? a : b;
    }
}

void pvauthor_async_test_miscellaneous::SetAuthoringDuration()
{
    uint32 videoDuration = 0;
    uint32 audioDuration = 0;
    uint32 textDuration = 0;
    uint32 minDuration = 0;
    uint32 ii = 0;

    if (iTestCaseNum >= LongetivityTestBegin && iTestCaseNum <= AAC_OUTPUT_LongetivityTestEnd)
    {
        minDuration = iTestDuration / 1000; //in msec
    }
    else if (iTestCaseNum == Input_Stream_Looping_Test)
    {
        minDuration = iTestDuration / 1000; //in msec
    }
    else
    {
        for (ii = 0; ii < iTestInputs.size(); ii++)
        {
            if (bVideoTrack && (iTestInputs[ii].iType == iVideoInputType))
            {
                PvmiMIOFileInput* mioFileIp = OSCL_STATIC_CAST(PvmiMIOFileInput*, iTestInputs[ii].iMediaInput);
                videoDuration =	mioFileIp->GetStreamDuration();
            }
            else if (bAudioTrack && (iTestInputs[ii].iType == iAudioInputType))
            {
                PvmiMIOFileInput* mioFileIp = OSCL_STATIC_CAST(PvmiMIOFileInput*, iTestInputs[ii].iMediaInput);
                audioDuration = mioFileIp->GetStreamDuration();
            }
            else if (bTextTrack && (iTestInputs[ii].iType == iTextInputType))
            {
                PvmiMIOFileInput* mioFileIp = OSCL_STATIC_CAST(PvmiMIOFileInput*, iTestInputs[ii].iMediaInput);
                textDuration = mioFileIp->GetStreamDuration();
            }
        }

        minDuration = GetMin(videoDuration, audioDuration);
        minDuration = GetMin(minDuration, textDuration);

    }

    if (minDuration)
    {
        for (ii = 0; ii < iTestInputs.size(); ii++)
        {
            PvmiMIOFileInput* mioFileIp = OSCL_STATIC_CAST(PvmiMIOFileInput*, iTestInputs[ii].iMediaInput);
            mioFileIp->SetAuthoringDuration(minDuration);
        }
    }


}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_miscellaneous::DeleteTestInputs()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::DeleteTestInputs"));

    for (uint32 ii = 0; ii < iTestInputs.size(); ii++)
        iTestInputs[ii].DeleteInputNode();

    iTestInputs.clear();
    return true;
}

void pvauthor_async_test_miscellaneous::ResetAuthorConfig()
{
    if (iComposerConfig)
    {
        iComposerConfig->removeRef();
        iComposerConfig = NULL;
    }
    if (iAudioEncoderConfig)
    {
        iAudioEncoderConfig->removeRef();
        iAudioEncoderConfig = NULL;
    }
    if (iVideoEncoderConfig)
    {
        iVideoEncoderConfig->removeRef();
        iVideoEncoderConfig = NULL;
    }
    if (iTextEncoderConfig)
    {
        iTextEncoderConfig->removeRef();
        iTextEncoderConfig = NULL;
    }
}

void pvauthor_async_test_miscellaneous::Cleanup()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "pvauthor_async_test_miscellaneous::Cleanup"));

    iComposer = NULL;

    ResetAuthorConfig();

    if (iAuthor)
    {
        PVAuthorEngineFactory::DeleteAuthor(iAuthor);
        iAuthor = NULL;
    }

    DeleteTestInputs();
    iOutputFileName = NULL;
}


////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_miscellaneous::Run()
{
    switch (iState)
    {
        case PVAE_CMD_CREATE:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                            (0, "******pvauthor_async_test_miscellaneous::iTestCaseNum:%d******", iTestCaseNum));
            iAuthor = PVAuthorEngineFactory::CreateAuthor(this, this, this);
            if (!iAuthor)
            {
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();

            }
            else
            {
                iState = PVAE_CMD_OPEN;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_OPEN:
            iAuthor->Open((OsclAny*)iAuthor);
            break;

        case PVAE_CMD_ADD_DATA_SOURCE_AUDIO:
        {
            int aStatus = -1;
            // Create audio input
            aStatus = CreateAudioInput();
            if (aStatus == 0) //Failed while creating audio input
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_miscellaneous::CreateTestInputs: Error - CreateAudioInput() failed"));
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else if (aStatus == -1) //Failed due to test being audio only or video only
            {
                iState = PVAE_CMD_ADD_DATA_SOURCE_VIDEO;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_ADD_DATA_SOURCE_VIDEO:
        {
            // Create video input
            int aStatus = -1;
            aStatus = CreateVideoInput();
            if (aStatus == 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_miscellaneous::CreateTestInputs: Error - CreateVideoInput() failed"));
                Cleanup();
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else if (aStatus == -1) //Failed due to test being audio only or video only
            {
                iState = PVAE_CMD_ADD_DATA_SOURCE_TEXT;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_ADD_DATA_SOURCE_TEXT:
        {
            // Create text input
            int aStatus = -1;
            aStatus = CreateTextInput();
            if (aStatus == 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_miscellaneous::CreateTestInputs: Error - CreateTextInput() failed"));
                Cleanup();
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else if (aStatus == -1) //Failed due to test being audio only or video only
            {
                iState = PVAE_CMD_SELECT_COMPOSER;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_SELECT_COMPOSER:
            SelectComposer();
            break;

        case PVAE_CMD_QUERY_INTERFACE:
            if (!QueryComposerOutputInterface())
            {
                iState = PVAE_CMD_ADD_AUDIO_MEDIA_TRACK;
                RunIfNotReady();
            }
            break;

        case PVAE_CMD_ADD_AUDIO_MEDIA_TRACK:
            if (!AddAudioMediaTrack())
            {
                bAudioTrack = false;
                iState = PVAE_CMD_ADD_VIDEO_MEDIA_TRACK;
                RunIfNotReady();
            }
            break;

        case PVAE_CMD_ADD_VIDEO_MEDIA_TRACK:
        {
            bVideoTrack = AddVideoMediaTrack();
            if (!bVideoTrack && !bAudioTrack) //No tracks have been added
            {
                bVideoTrack = false;
                iState = PVAE_CMD_ADD_TEXT_MEDIA_TRACK;
                RunIfNotReady();
            }
            else if (!bVideoTrack) //Audio track added but no video track
            {
                iState = PVAE_CMD_ADD_TEXT_MEDIA_TRACK;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_ADD_TEXT_MEDIA_TRACK:
        {
            bTextTrack = AddTextMediaTrack();
            if (!bTextTrack && !bAudioTrack && !bVideoTrack) //No tracks have been added
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_miscellaneous::Run: Error - No track added"));
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else if (!bTextTrack) //Audio and video track added but no text track
            {
                iState = PVAE_CMD_INIT;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_INIT:
            iAuthor->Init((OsclAny*)iAuthor);
            break;

        case PVAE_CMD_START:
        {
            SetAuthoringDuration();
            iAuthor->Start();
        }
        break;
        case PVAE_CMD_STOP:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                            (0, "pvauthor_async_test_miscellaneous::Run: Fire Stop"));
            iAuthor->Stop((OsclAny*)iAuthor);
            break;
        case PVAE_CMD_RESET:
        {
            ResetAuthorConfig();
            iAuthor->Reset((OsclAny*)iAuthor);
        }
        break;
        case PVAE_CMD_REMOVE_DATA_SOURCE:
        {
            for (uint ii = 0; ii < iTestInputs.size(); ii++)
            {
                iAuthor->RemoveDataSource(*(iTestInputs[ii].iNode), (OsclAny*)iAuthor);
            }
        }
        break;
        case PVAE_CMD_CLOSE:
            iAuthor->Close((OsclAny*)iAuthor);
            break;
        case PVAE_CMD_PAUSE:
            iAuthor->Pause((OsclAny*)iAuthor);
            break;
        case PVAE_CMD_RESUME:
            iAuthor->Resume((OsclAny*)iAuthor);
            break;
        case PVAE_CMD_CLEANUPANDCOMPLETE:
        {
            Cleanup();
            iObserver->CompleteTest(*iTestCase);
        }
        break;

        case PVAE_CMD_QUERY_INTERFACE2:
            break;
        case PVAE_CMD_ADD_DATA_SINK:
            break;
        case PVAE_CMD_REMOVE_DATA_SINK:
            break;
        case PVAE_CMD_RECORDING:
            if (!iAuthoringCounter)
            {
                iState = PVAE_CMD_STOP;
                // this is for the finshing the loop count
                if (iAuthoringTime > KAuthoringSessionUnit)
                {
                    iTestDuration = (iAuthoringTime % KAuthoringSessionUnit) * 1000 * 1000;
                }
                // otherwise iTestDuration is already set through Constructor
                RunIfNotReady(iTestDuration);
            }
            else
            {
                // carry with the loops
                iState = PVAE_CMD_RECORDING;
                iAuthoringCounter--;
                RunIfNotReady(iTestDuration);
            }
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_miscellaneous::CommandCompleted(const PVCmdResponse& aResponse)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_miscellaneous::CommandCompleted iState:%d", iState));

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "pvauthor_async_test_generic_reset::CommandCompleted iState:%d FAILED", iState));
    }
    switch (iState)
    {
        case PVAE_CMD_OPEN:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_ADD_DATA_SOURCE_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // Open failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_ADD_DATA_SOURCE_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_ADD_DATA_SOURCE_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // AddDataSourceAudio failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_ADD_DATA_SOURCE_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_ADD_DATA_SOURCE_TEXT;
                RunIfNotReady();
            }
            else
            {
                // AddDataSourceVideo failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_ADD_DATA_SOURCE_TEXT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_SELECT_COMPOSER;
                RunIfNotReady();
            }
            else
            {
                // AddDataSourceText failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_SELECT_COMPOSER:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iComposer = aResponse.GetResponseData();
                if (!ConfigComposer())
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "pvauthor_async_test_miscellaneous::CommandCompleted: Error - ConfigComposer failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iState = PVAE_CMD_RESET;
                    RunIfNotReady();
                    return;
                }
                else
                {
                    iState = PVAE_CMD_QUERY_INTERFACE;
                    RunIfNotReady();
                }
            }
            else
            {
                // SelectComposer failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_QUERY_INTERFACE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                ConfigComposerOutput();
                iState = PVAE_CMD_ADD_AUDIO_MEDIA_TRACK;
                RunIfNotReady();
            }
            else
            {
                // QueryInterface failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_ADD_AUDIO_MEDIA_TRACK:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                if (!ConfigureAudioEncoder())
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "pvauthor_async_test_miscellaneous::CommandCompleted: Error - ConfigureAudioEncoder failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iState = PVAE_CMD_CLOSE;
                    RunIfNotReady();
                }
                else
                {
                    iState = PVAE_CMD_ADD_VIDEO_MEDIA_TRACK;
                    RunIfNotReady();
                }
            }
            else
            {
                // AddAudioMediaTrack failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;

        case PVAE_CMD_ADD_VIDEO_MEDIA_TRACK:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                if (!ConfigureVideoEncoder())
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "pvauthor_async_test_miscellaneous::CommandCompleted: Error - ConfigureVideoEncoder failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iObserver->CompleteTest(*iTestCase);
                }
                else
                {
                    iState = PVAE_CMD_ADD_TEXT_MEDIA_TRACK;
                    RunIfNotReady();
                }
            }
            else
            {
                // AddVideoMediaTrack failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;

        case PVAE_CMD_ADD_TEXT_MEDIA_TRACK:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                if (!ConfigureTextEncoder())
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "pvauthor_async_test_miscellaneous::CommandCompleted: Error - ConfigureTextEncoder failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iObserver->CompleteTest(*iTestCase);
                }
                else
                {
                    iState = PVAE_CMD_INIT;
                    RunIfNotReady();
                }
            }
            else
            {
                // AddTextMediaTrack failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;

        case PVAE_CMD_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_START;
                RunIfNotReady();
            }
            else
            {
                // Init failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;

        case PVAE_CMD_START:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                if (iPauseResumeEnable)
                {
                    iState = PVAE_CMD_PAUSE;
                    RunIfNotReady(KPauseDuration); //Pause after 5 sec
                }
                else
                {
                    iState = PVAE_CMD_RECORDING;
                    RunIfNotReady();
                }
            }
            else
            {
                // Start failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_PAUSE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_RESUME;
                /* Stay paused for 10 seconds */
                RunIfNotReady(10*1000*1000);
            }
            else
            {
                //Pause failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_RESUME:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_STOP;
                RunIfNotReady(10*1000*1000); //Run for another 10 sec before stopping
            }
            else
            {
                //Resume failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_STOP:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iOutputFileName = NULL;
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            else
            {
                // Stop failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_RESET:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                if (iTestInputs.size() == 0)
                {
                    if (aResponse.GetCmdStatus() == PVMFSuccess)
                    {
                        PVPATB_TEST_IS_TRUE(true);
                    }
                    else
                    {
                        PVPATB_TEST_IS_TRUE(false);
                    }
                    //Since there are no testInputs, we end here
                    //No need to call RemoveDataSource
                    iObserver->CompleteTest(*iTestCase);
                    break;
                }
                iState = PVAE_CMD_REMOVE_DATA_SOURCE;
                RunIfNotReady();
            }
            else
            {
                // Reset failed
                PVPATB_TEST_IS_TRUE(false);
                iObserver->CompleteTest(*iTestCase);
            }
            break;

        case PVAE_CMD_REMOVE_DATA_SOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iRemoveDataSourceDone++;
                if (iRemoveDataSourceDone < iTestInputs.size())
                {
                    return;//We will wait for all RemoveDataSource calls to complete
                }
                iOutputFileName = NULL;
                if (!DeleteTestInputs())
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "pvauthor_async_test_miscellaneous::CommandCompleted: Error - DeleteTestInputs failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iObserver->CompleteTest(*iTestCase);
                    return;
                }
                else
                {
                    iState = PVAE_CMD_CLOSE;
                    RunIfNotReady();
                }
            }
            else
            {
                // RemoveDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iObserver->CompleteTest(*iTestCase);
            }
            break;
        case PVAE_CMD_CLOSE:
        {
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                PVPATB_TEST_IS_TRUE(true);
            }
            else
            {
                PVPATB_TEST_IS_TRUE(false);
            }
            iObserver->CompleteTest(*iTestCase);
        }
        break;
        default:
        {
            // Testing error if this is reached
            PVPATB_TEST_IS_TRUE(false);
            iObserver->CompleteTest(*iTestCase);
        }
    }
}
