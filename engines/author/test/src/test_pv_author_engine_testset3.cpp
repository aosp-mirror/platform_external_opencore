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
#ifndef TEST_PV_AUTHOR_ENGINE_TESTSET3_H_INCLUDED
#include "test_pv_author_engine_testset3.h"
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

void pvauthor_async_test_capconfig::StartTest()
{
    AddToScheduler();
    iState = PVAE_CMD_CREATE;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                    (0, "******pvauthor_async_test_capconfig::iTestCaseNum:%d******", iTestCaseNum));
    RunIfNotReady();
}



////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_capconfig::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    OSCL_UNUSED_ARG(aEvent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR, (0, "pvauthor_async_test_capconfig::HandleErrorEvent"));
    PVPATB_TEST_IS_TRUE(false);
    iObserver->CompleteTest(*iTestCase);
}

////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_capconfig::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    OSCL_UNUSED_ARG(aEvent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::HandleInformationalEvent"));
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
int pvauthor_async_test_capconfig::CreateAudioInput()
{
    int status = 0;
    PVAETestInput testInput;

    if (iAudioInputType == INVALID_INPUT_TYPE)	// check condition when -audio tag is not specified
    {
        return -1;
    }
    else
    {
        status = testInput.CreateInputNode(iAudioInputType, iInputFileNameAudio, iAVTConfig);
    }

    if (!status)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::CreateAudioTestInput: Error - Create input node failed"));
        return status;
    }

    return AddDataSource(testInput);
}

////////////////////////////////////////////////////////////////////////////
int pvauthor_async_test_capconfig::CreateVideoInput()
{
    int status = 0;
    PVAETestInput testInput;

    if (iVideoInputType == INVALID_INPUT_TYPE)	// check condition when -videoo tag is not specified
    {
        return -1;
    }
    else
    {
        testInput = PVAETestInput();
        status = testInput.CreateInputNode(iVideoInputType, iInputFileNameVideo, iAVTConfig);
    }

    if (!status)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::CreateTestInputs: Error - CreateInputNode failed"));
        return status;
    }

    return AddDataSource(testInput);
}

////////////////////////////////////////////////////////////////////////////
int pvauthor_async_test_capconfig::CreateTextInput()
{
    // Text track support will be developed in future
    return -1;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::AddDataSource(PVAETestInput& aInput)
{
    int32 err = 0;

    OSCL_TRY(err, iTestInputs.push_back(aInput););
    if (err != OSCL_ERR_NONE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::AddDataSource: Error - iTestInputs.push_back failed. err=0x%x", err));
        aInput.DeleteInputNode();
        return false;
    }


    OSCL_TRY(err, iAuthor->AddDataSource(*(aInput.iNode), (OsclAny*)iAuthor););
    if (err != OSCL_ERR_NONE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::AddDataSource: Error - iAuthor->AddDataSource failed. err=0x%x", err));
        aInput.DeleteInputNode();
        return false;
    }

    return true;
}

void pvauthor_async_test_capconfig::SelectComposer()
{
    iAuthor->SelectComposer(iComposerMimeType, iComposerConfig, (OsclAny*)iAuthor);
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::ConfigComposer()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::ConfigComposer"));
    if ((iTestCaseNum >= GenericTestBegin) && (iTestCaseNum < GenericTestEnd))
    {
        uint32 aLenFileName;
        aLenFileName = oscl_strlen(iTempOutputFileName) - oscl_strlen(oscl_strchr(iTempOutputFileName, '.'));
        oscl_wchar temp[ARRAY_SIZE];
        oscl_memset(temp, 0, ARRAY_SIZE);
        oscl_strncpy(temp, iTempOutputFileName, aLenFileName);
        oscl_snprintf(temp + aLenFileName, ARRAY_SIZE, _STRLIT("_CapConfig_TestNum_%d"), iTestCaseNum);
        oscl_strcat(temp, oscl_strchr(iTempOutputFileName, '.'));
        iOutputFileName = temp;
    }

    if (!ConfigAmrAacComposer())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::ConfigComposer: Error - ConfigAmrAacComposer failed"));
        return false;
    }

    if (!ConfigMp43gpComposer())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::ConfigComposer: Error - ConfigMp43gpComposer failed"));
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::ConfigAmrAacComposer()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::ConfigAmrAacComposer"));

    if (!((iComposerMimeType == KAMRNbComposerMimeType) || (iComposerMimeType == KAACADTSComposerMimeType) || (iComposerMimeType == KAACADIFComposerMimeType)))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "pvauthor_async_test_capconfig::ConfigAmrAacComposer: AMR-AAC Composer not used in this test case"));
        return true;
    }


    PvmfFileOutputNodeConfigInterface* clipConfig = OSCL_STATIC_CAST(PvmfFileOutputNodeConfigInterface*, iComposerConfig);
    if (!clipConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::ConfigAmrAacComposer: Error - Invalid iComposerConfig"));
        return false;
    }

    if (clipConfig->SetOutputFileName(iOutputFileName) != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::ConfigAmrAacComposer: Error - SetOutputFileName failed"));
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::ConfigMp43gpComposer()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "pvauthor_async_test_capconfig::ConfigMp43gpComposer"));

    if (!(iComposerMimeType == KAMRNbComposerMimeType) && !(iComposerMimeType == KAACADTSComposerMimeType) && !(iComposerMimeType == KAACADIFComposerMimeType))
    {
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "pvauthor_async_test_capconfig::ConfigMp43gpComposer: Mp4-3GPP composer not used in this test case"));
        return true;
    }

    PVMp4FFCNClipConfigInterface* clipConfig;
    clipConfig = OSCL_STATIC_CAST(PVMp4FFCNClipConfigInterface*, iComposerConfig);
    if (!clipConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::ConfigMp43gpComposer: Error - iComposerConfig==NULL"));
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

    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::ConfigComposerOutput()
{
    PvmfComposerSizeAndDurationInterface* config =
        OSCL_REINTERPRET_CAST(PvmfComposerSizeAndDurationInterface*, iOutputSizeAndDurationConfig);
    if (!config)
    {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::QueryComposerOutputInterface()
{
    switch (iTestCaseNum)
    {
        case Generic_QueryInterface_Reset_Test:
            iAuthor->QueryInterface(PvmfComposerSizeAndDurationUuid,
                                    iOutputSizeAndDurationConfig, (OsclAny*)iAuthor);
            return true;

        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::AddAudioMediaTrack()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::AddAudioMediaTrack"));

    if (iAudioInputType == INVALID_INPUT_TYPE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::AddAudioMediaTrackL: Error - Invalid audio input type"));
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
                        (0, "pvauthor_async_test_capconfig::AddAudioMediaTrack: Error - Test input not found"));
        return false;
    }

    iAuthor->AddMediaTrack(*(testInput.iNode), iAudioEncoderMimeType, iComposer,
                           iAudioEncoderConfig, (OsclAny*)iAuthor);
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::AddVideoMediaTrack()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::AddVideoMediaTrack"));

    if (iVideoInputType == INVALID_INPUT_TYPE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "pvauthor_async_test_capconfig::AddVideoMediaTrack: Error - Invalid input type"));
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
                        (0, "pvauthor_async_test_capconfig::AddVideoMediaTrack: Error - Test input not found"));
        return false;
    }

    iAuthor->AddMediaTrack(*(testInput.iNode), iVideoEncoderMimeType, iComposer,
                           iVideoEncoderConfig, (OsclAny*)iAuthor);
    return true;
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::ConfigureVideoEncoder()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::ConfigureVideoEncoder"));

    PVMp4H263EncExtensionInterface* config;
    config = OSCL_STATIC_CAST(PVMp4H263EncExtensionInterface*, iVideoEncoderConfig);
    if (!config)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "pvauthor_async_test_capconfig::ConfigureVideoEncoder: No configuration needed"));
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
bool pvauthor_async_test_capconfig::ConfigureAudioEncoder()
{
    //Single core AMR encoder node support
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::ConfigureAudioEncoder"));

    if (!iAudioEncoderConfig)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                        (0, "pvauthor_async_test_capconfig::ConfigureAudioEncoder: No configuration needed"));
        return true;
    }

    return PVAETestNodeConfig::ConfigureAudioEncoder(iAudioEncoderConfig);
}

////////////////////////////////////////////////////////////////////////////
bool pvauthor_async_test_capconfig::DeleteTestInputs()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::DeleteTestInputs"));

    for (uint32 ii = 0; ii < iTestInputs.size(); ii++)
        iTestInputs[ii].DeleteInputNode();

    iTestInputs.clear();
    return true;
}

void pvauthor_async_test_capconfig::ResetAuthorConfig()
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
}

void pvauthor_async_test_capconfig::Cleanup()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "pvauthor_async_test_capconfig::Cleanup"));

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
void pvauthor_async_test_capconfig::Run()
{
    switch (iState)
    {
        case PVAE_CMD_CREATE:
        {
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
                                (0, "pvauthor_async_test_capconfig::CreateTestInputs: Error - CreateAudioInput() failed"));
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
                                (0, "pvauthor_async_test_capconfig::CreateTestInputs: Error - CreateVideoInput() failed"));
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
                                (0, "pvauthor_async_test_capconfig::CreateTestInputs: Error - CreateVideoInput() failed"));
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
            bool bVideoTrack = AddVideoMediaTrack();
            if (!bVideoTrack && !bAudioTrack) //No tracks have been added
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_capconfig::CommandCompleted: Error - No track added"));
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else if (!bVideoTrack) //Audio track added but no video track
            {
                iState = PVAE_CMD_QUERY_INTERFACE2;
                RunIfNotReady();
            }
        }
        break;

        case PVAE_CMD_INIT:
            iAuthor->Init((OsclAny*)iAuthor);
            break;

        case PVAE_CMD_START:
            iAuthor->Start();
            break;
        case PVAE_CMD_STOP:
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
        case PVAE_CMD_QUERY_INTERFACE2:
            iAuthor->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (PVInterface*&)iAuthorCapConfigIF, (OsclAny*)iAuthor);
            break;
        case PVAE_CMD_CAPCONFIG_ASYNC:
            if (!CapConfigAsync())
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_capconfig::Run: Error - CapConfigAsync failed"));
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_CAPCONFIG_SYNC:
            // set configuration MIME strings Synchronously
            if (!CapConfigSync()) //CapConfig failed
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                (0, "pvauthor_async_test_capconfig::Run: Error - CapConfigSync failed"));
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else
            {
                iState = PVAE_CMD_START;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_CLEANUPANDCOMPLETE:
        {
            Cleanup();
            iObserver->CompleteTest(*iTestCase);
        }
        break;
        case PVAE_CMD_ADD_DATA_SINK:
            break;
        case PVAE_CMD_REMOVE_DATA_SINK:
            break;
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////
void pvauthor_async_test_capconfig::CommandCompleted(const PVCmdResponse& aResponse)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "pvauthor_async_test_capconfig::CommandCompleted iState:%d", iState));

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "pvauthor_async_test_capconfig::CommandCompleted iState:%d FAILED", iState));
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
                                    (0, "pvauthor_async_test_capconfig::CommandCompleted: Error - ConfigComposer failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iObserver->CompleteTest(*iTestCase);
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
                                    (0, "pvauthor_async_test_capconfig::CommandCompleted: Error - ConfigureAudioEncoder failed"));
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
                                    (0, "pvauthor_async_test_capconfig::CommandCompleted: Error - ConfigureVideoEncoder failed"));
                    PVPATB_TEST_IS_TRUE(false);
                    iObserver->CompleteTest(*iTestCase);
                }
                else
                {
                    iState = PVAE_CMD_QUERY_INTERFACE2;
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

        case PVAE_CMD_QUERY_INTERFACE2:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_INIT;
                RunIfNotReady();
            }
            else
            {
                // QueryInterface2 failed
                PVPATB_TEST_IS_TRUE(false);
                iState = PVAE_CMD_RESET;
                RunIfNotReady();
            }
            break;
        case PVAE_CMD_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = PVAE_CMD_CAPCONFIG_ASYNC;
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
                    iState = PVAE_CMD_STOP;
                    RunIfNotReady(KTestDuration*1000*1000); // Run for 10 seconds
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
                RunIfNotReady(KPauseDuration); //Run for another 5 sec
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
                                    (0, "pvauthor_async_test_capconfig::CommandCompleted: Error - DeleteTestInputs failed"));
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
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                PVPATB_TEST_IS_TRUE(true);
            }
            else
            {
                PVPATB_TEST_IS_TRUE(false);
            }
            iObserver->CompleteTest(*iTestCase);
            break;

        default:
        {
            // Testing error if this is reached
            PVPATB_TEST_IS_TRUE(false);
            iObserver->CompleteTest(*iTestCase);
        }
    }

}

void pvauthor_async_test_capconfig::SignalEvent(int32 aReq_Id)
{
    if (aReq_Id == 0) //For PVAE_CMD_CAPCONFIG_SET_PARAMETERS
    {
        if (iErrorKVP != NULL)
        {
            // There was an error in setParameterAsync()
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                            (0, "pvauthor_async_test_capconfig::SignalEvent: setParametersAsync failed"));
            PVPATB_TEST_IS_TRUE(false);
            iObserver->CompleteTest(*iTestCase);
        }
        else
        {
            iState = PVAE_CMD_CAPCONFIG_SYNC;
            RunIfNotReady();
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// function for setting configuration MIME strings Synchronously

bool pvauthor_async_test_capconfig::CapConfigSync()
{
    // set the mime strings here
    // capabilty and configuration feature implemented here
    // set the config parameters using MIME strings here

    // set the KVP array
    PvmiKvp paramkvp1;
    // set cache size for composer node
    //	PVMI_FILEIO_PV_CACHE_SIZE (MACRO defined in "pvmi_fileio_kvp.h")
    OSCL_StackString<64> paramkey1(_STRLIT_CHAR(PVMI_FILEIO_PV_CACHE_SIZE));
    paramkey1 += _STRLIT_CHAR(";valtype=uint32");
    // set KVP values
    // for composer node
    paramkvp1.key = paramkey1.get_str();
    paramkvp1.value.uint32_value = 1024;

    iErrorKVP = NULL;

    // verifyParametersSync will give Failure for test cases not using MP4 composer("fileio") node so commented
    // Verify the new settings
    {
        // set the parameter
        iAuthorCapConfigIF->setParametersSync(NULL, &paramkvp1, 1, iErrorKVP);
        if (iErrorKVP == NULL)
        {
            // Check by calling get
            PvmiKvp* retparam = NULL;
            int retnumparam = 0;
            paramkey1 += _STRLIT_CHAR(";attr=cur");
            // pass the string
            // retrieve a MIME string back
            iAuthorCapConfigIF->getParametersSync(NULL, paramkey1.get_str(), retparam, retnumparam, NULL);

            if ((retparam != NULL) && (retnumparam == 1))
            {
                // release the parameters
                if (iAuthorCapConfigIF->releaseParameters(NULL, retparam, retnumparam) != PVMFSuccess)
                {
                    return false;
                }
            }
        }
    }

    //MIME for file output node
    // set here any parameter here
    OSCL_StackString<64> paramkey2(_STRLIT_CHAR("x-pvmf/file/output/parameter1;valtype=uint32"));

    // for file output node
    paramkvp1.key = paramkey2.get_str();
    paramkvp1.value.int32_value = 144;

    // set the value in node using SetParameterSync f(n) here
    iAuthorCapConfigIF->setParametersSync(NULL, &paramkvp1, 1, iErrorKVP);

    // MIME for media io node
    // set parameters here
    OSCL_StackString<64> paramkey3(_STRLIT_CHAR("x-pvmf/media-io/parameter1;valtype=uint32"));

    // for media io node
    paramkvp1.key = paramkey3.get_str();
    paramkvp1.value.int32_value = 144;

    // set the value in node using SetParameterSync f(n) here
    iAuthorCapConfigIF->setParametersSync(NULL, &paramkvp1, 1, iErrorKVP);

    if (iErrorKVP == NULL)
    {
        // Check by calling get
        PvmiKvp* retparam = NULL;
        int retnumparam = 0;
        paramkey3 += _STRLIT_CHAR(";attr=cur");
        // pass the string
        // retrieve a MIME string back
        iAuthorCapConfigIF->getParametersSync(NULL, paramkey3.get_str(), retparam, retnumparam, NULL);
        if ((retparam != NULL) && (retnumparam == 1))
        {
            // release the parameters
            if (iAuthorCapConfigIF->releaseParameters(NULL, retparam, retnumparam) != PVMFSuccess)
            {
                return false;
            }
        }
    }

    // MIME for amr encoder
    // set the parameter here

    // MIME string here "x-pvmf/audio/render/sampling_rate;valtype=uint32"
    OSCL_StackString<64> paramkey4(_STRLIT_CHAR(MOUT_AUDIO_SAMPLING_RATE_KEY));
    // MOUT_AUDIO_SAMPLING_RATE_KEY (MACRO defined in "pvmi_kvp.h")
    // for amr encoder node
    paramkvp1.key = paramkey4.get_str();
    paramkvp1.value.int32_value = 8000;

    // set the value in node using SetParameterSync f(n) here
    iAuthorCapConfigIF->setParametersSync(NULL, &paramkvp1, 1, iErrorKVP);

    if (iErrorKVP == NULL)
    {
        // Check by calling get
        PvmiKvp* retparam = NULL;
        int retnumparam = 0;
        paramkey1 += _STRLIT_CHAR(";attr=cur");
        // pass the string
        // retrieve a MIME string back
        iAuthorCapConfigIF->getParametersSync(NULL, paramkey4.get_str(), retparam, retnumparam, NULL);
        if ((retparam != NULL) && (retnumparam == 1))
        {
            // release the parameters
            if (iAuthorCapConfigIF->releaseParameters(NULL, retparam, retnumparam) != PVMFSuccess)
            {
                return false;
            }
        }
    }


    // MIME for video encoder
    // set the parameter frame width in the node (use combinations for width n height default is 176 by 144)

    // MIME string here "x-pvmf/video/render/output_width;valtype=uint32"
    OSCL_StackString<64> paramkey5(_STRLIT_CHAR(MOUT_VIDEO_OUTPUT_WIDTH_KEY));
    // MACRO "MOUT_VIDEO_OUTPUT_WIDTH_KEY" been defined in "pvmi_kvp.h"
    // for video encoder node
    paramkvp1.key = paramkey5.get_str();
    paramkvp1.value.uint32_value = 176;
    // set the value in node using SetParameterSync f(n) here
    iAuthorCapConfigIF->setParametersSync(NULL, &paramkvp1, 1, iErrorKVP);

    if (iErrorKVP == NULL)
    {
        // Check by calling get
        PvmiKvp* retparam = NULL;
        int retnumparam = 0;
        paramkey1 += _STRLIT_CHAR(";attr=cur");
        // pass the string
        // retrieve a MIME string back
        iAuthorCapConfigIF->getParametersSync(NULL, paramkey5.get_str(), retparam, retnumparam, NULL);
        if ((retparam != NULL) && (retnumparam == 1))
        {
            // release the parameters
            if (iAuthorCapConfigIF->releaseParameters(NULL, retparam, retnumparam) != PVMFSuccess)
            {
                return false;
            }
        }
    }

    // MIME for AVC Encoder
    // set the parameter encoding mode

    // MIME string here "x-pvmf/avc/encoder/encoding_mode;valtype=uint32"
    OSCL_StackString<64> paramkey6(_STRLIT_CHAR(PVMF_AVC_ENCODER_ENCODINGMODE_KEY));
    // MACRO "PVMF_AVC_ENCODER_ENCODINGMODE_KEY" been defined in "pvmi_kvp.h"
    // for AVC Encoder node
    paramkvp1.key = paramkey6.get_str();
    paramkvp1.value.uint32_value = 3;//EAVCEI_ENCMODE_TWOWAY;
    // set the value in node using SetParameterSync f(n) here
    iAuthorCapConfigIF->setParametersSync(NULL, &paramkvp1, 1, iErrorKVP);

    if (iErrorKVP == NULL)
    {
        // Check by calling get
        PvmiKvp* retparam = NULL;
        int retnumparam = 0;
        paramkey1 += _STRLIT_CHAR(";attr=cur");
        // pass the string
        // retrieve a MIME string back
        iAuthorCapConfigIF->getParametersSync(NULL, paramkey6.get_str(), retparam, retnumparam, NULL);
        if ((retparam != NULL) && (retnumparam == 1))
        {
            // release the parameters
            if (iAuthorCapConfigIF->releaseParameters(NULL, retparam, retnumparam) != PVMFSuccess)
            {
                return false;
            }
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
// function for setting configuration MIME strings Asynchronously

bool pvauthor_async_test_capconfig::CapConfigAsync()
{
    // Set the observer
    iAuthorCapConfigIF->setObserver(this);

    iKeyStringSetAsync = _STRLIT_CHAR("x-pvmf/author/productinfo/dummyprod1;valtype=int32");
    iKVPSetAsync.key = iKeyStringSetAsync.get_str();
    iKVPSetAsync.value.int32_value = 2;

    // set the parameter
    iErrorKVP = NULL;
    iAuthorCapConfigIF->setParametersAsync(NULL, &iKVPSetAsync, 1, iErrorKVP);

    return true;
}

