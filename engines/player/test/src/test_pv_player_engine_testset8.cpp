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
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET8_H_INCLUDED
#include "test_pv_player_engine_testset8.h"
#endif

#ifndef OSCL_ERROR_CODES_H_INCLUDED
#include "oscl_error_codes.h"
#endif

#ifndef OSCL_TICKCOUNT_H_INCLUDED
#include "oscl_tickcount.h"
#endif

#ifndef PV_PLAYER_DATASINKFILENAME_H_INCLUDED
#include "pv_player_datasinkfilename.h"
#endif

#ifndef PV_PLAYER_DATASINKPVMFNODE_H_INCLUDED
#include "pv_player_datasinkpvmfnode.h"
#endif

#ifndef PVMI_MEDIA_IO_FILEOUTPUT_H_INCLUDED
#include "pvmi_media_io_fileoutput.h"
#endif

#ifndef PV_MEDIA_OUTPUT_NODE_FACTORY_H_INCLUDED
#include "pv_media_output_node_factory.h"
#endif

#ifndef PVMF_ERRORINFOMESSAGE_EXTENSION_H_INCLUDED
#include "pvmf_errorinfomessage_extension.h"
#endif

#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif

#ifndef PVMI_KVP_H_INCLUDED
#include "pvmi_kvp.h"
#endif

#ifndef PVMI_KVP_UTIL_H_INCLUDED
#include "pvmi_kvp_util.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OSCL_MEM_AUDIT_H_INCLUDED
#include "oscl_mem_audit.h"
#endif

//
// pvplayer_async_test_printmetadata section
//
void pvplayer_async_test_printmetadata::StartTest()
{
    AddToScheduler();
    iState = STATE_CREATE;
    RunIfNotReady();
}


void pvplayer_async_test_printmetadata::Run()
{
    int error = 0;

    switch (iState)
    {
        case STATE_CREATE:
        {
            iPlayer = NULL;

            OSCL_TRY(error, iPlayer = PVPlayerFactory::CreatePlayer(this, this, this));
            if (error)
            {
                PVPATB_TEST_IS_TRUE(false);
                iObserver->TestCompleted(*iTestCase);
            }
            else
            {
                iState = STATE_ADDDATASOURCE;
                RunIfNotReady();
            }
        }
        break;

        case STATE_ADDDATASOURCE:
        {
            iDataSource = new PVPlayerDataSourceURL;
            oscl_UTF8ToUnicode(iFileName, oscl_strlen(iFileName), iTmpWCharBuffer, 512);
            iTmpWCharBuffer[511] = '\0';
            iFileNameWStr = SOURCENAME_PREPEND_WSTRING;
            iFileNameWStr += iTmpWCharBuffer;
            iDataSource->SetDataSourceURL(iFileNameWStr);
            iDataSource->SetDataSourceFormatType(iFileType);
            /*
             * In case of HTTP URLs always attempt rollover,
             * since we donot know if it is a download or a streaming url
             */
            if (iFileType == PVMF_DATA_SOURCE_HTTP_URL)
            {
                iDataSource->SetAlternateSourceFormatType(PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL);
            }
            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }

        break;

        case STATE_INIT:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Init((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAKEYLIST1:
        {
            iMetadataKeyList.clear();
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataKeys(iMetadataKeyList, 0, -1, NULL, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAVALUELIST1:
        {
            iMetadataValueList.clear();
            iNumValues = 0;
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataValues(iMetadataKeyList, 0, -1, iNumValues, iMetadataValueList, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_VIDEO:
        {
            OSCL_wHeapString<OsclMemAllocator> videosinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            videosinkfilename += _STRLIT_WCHAR("test_player_printmetadata_video.dat");

            iMIOFileOutVideo = iMioFactory->CreateVideoOutput((OsclAny*) & videosinkfilename);
            iIONodeVideo = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutVideo);
            iDataSinkVideo = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkNode(iIONodeVideo);
            if (iCompressedVideo)
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(PVMF_M4V);
            }
            else
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(PVMF_YUV420);
            }

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_AUDIO:
        {
            OSCL_wHeapString<OsclMemAllocator> audiosinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            audiosinkfilename += _STRLIT_WCHAR("test_player_printmetadata_audio.dat");

            iMIOFileOutAudio = iMioFactory->CreateAudioOutput((OsclAny*) & audiosinkfilename);
            iIONodeAudio = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutAudio);
            iDataSinkAudio = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkNode(iIONodeAudio);
            if (iCompressedAudio)
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(PVMF_MPEG4_AUDIO);
            }
            else
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(AUDIOSINK_FORMAT_TYPE);
            }

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_TEXT:
        {
            OSCL_wHeapString<OsclMemAllocator> textsinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            textsinkfilename += _STRLIT_WCHAR("test_player_printmetadata_text.dat");

            iMIOFileOutText = iMioFactory->CreateTextOutput((OsclAny*) & textsinkfilename);
            iIONodeText = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutText);
            iDataSinkText = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkText)->SetDataSinkNode(iIONodeText);
            ((PVPlayerDataSinkPVMFNode*)iDataSinkText)->SetDataSinkFormatType(PVMF_3GPP_TIMEDTEXT);

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkText, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_PREPARE:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Prepare((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_START:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Start((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAKEYLIST2:
        {
            iMetadataKeyList.clear();
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataKeys(iMetadataKeyList, 0, 50, NULL, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAVALUELIST2:
        {
            iMetadataValueList.clear();
            iNumValues = 0;
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataValues(iMetadataKeyList, 0, 50, iNumValues, iMetadataValueList, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_STOP:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Stop((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAKEYLIST3:
        {
            iMetadataKeyList.clear();
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataKeys(iMetadataKeyList, 0, 50, NULL, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAVALUELIST3:
        {
            iMetadataValueList.clear();
            iNumValues = 0;
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataValues(iMetadataKeyList, 0, 50, iNumValues, iMetadataValueList, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_VIDEO:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_AUDIO:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_TEXT:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkText, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_RESET:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Reset((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASOURCE:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_WAIT_FOR_ERROR_HANDLING:
        {
            // Timed out waiting for error handling to complete
            PVPATB_TEST_IS_TRUE(false);
            // Forcibly terminate the test
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
        }
        break;

        case STATE_CLEANUPANDCOMPLETE:
        {
            PVPATB_TEST_IS_TRUE(PVPlayerFactory::DeletePlayer(iPlayer));
            iPlayer = NULL;

            delete iDataSource;
            iDataSource = NULL;

            delete iDataSinkVideo;
            iDataSinkVideo = NULL;

            delete iDataSinkAudio;
            iDataSinkAudio = NULL;

            delete iDataSinkText;
            iDataSinkText = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeVideo);
            iIONodeVideo = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeAudio);
            iIONodeAudio = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeText);
            iIONodeText = NULL;

            iMioFactory->DestroyVideoOutput(iMIOFileOutVideo);
            iMIOFileOutVideo = NULL;

            iMioFactory->DestroyAudioOutput(iMIOFileOutAudio);
            iMIOFileOutAudio = NULL;

            iMioFactory->DestroyTextOutput(iMIOFileOutText);
            iMIOFileOutText = NULL;

            iObserver->TestCompleted(*iTestCase);
        }
        break;

        default:
            break;

    }
}


void pvplayer_async_test_printmetadata::CommandCompleted(const PVCmdResponse& aResponse)
{
    if (aResponse.GetCmdId() != iCurrentCmdId)
    {
        // Wrong command ID.
        PVPATB_TEST_IS_TRUE(false);
        iState = STATE_CLEANUPANDCOMPLETE;
        RunIfNotReady();
        return;
    }

    if (aResponse.GetContext() != NULL)
    {
        if (aResponse.GetContext() == (OsclAny*)&iContextObject)
        {
            if (iContextObject != iContextObjectRefValue)
            {
                // Context data value was corrupted
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                return;
            }
        }
        else
        {
            // Context data pointer was corrupted
            PVPATB_TEST_IS_TRUE(false);
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
            return;
        }
    }

    switch (iState)
    {
        case STATE_ADDDATASOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_INIT;
                RunIfNotReady();
            }
            else
            {
                // AddDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_GETMETADATAKEYLIST1;
                // Uncomment following to skip the first metadata retrieval
                //iState=STATE_ADDDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // Init failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAKEYLIST1:
            if (aResponse.GetCmdStatus() == PVMFSuccess || aResponse.GetCmdStatus() == PVMFErrArgument)
            {
                iState = STATE_GETMETADATAVALUELIST1;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataKeys failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAVALUELIST1:
            if (aResponse.GetCmdStatus() == PVMFSuccess || aResponse.GetCmdStatus() == PVMFErrArgument)
            {
                fprintf(iTestMsgOutputFile, "After Init()\n");
                PrintMetadataInfo();
                iState = STATE_ADDDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataValue failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_ADDDATASINK_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_ADDDATASINK_TEXT;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_TEXT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_PREPARE;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_PREPARE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_START;
                RunIfNotReady();
            }
            else
            {
                // Prepare failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_START:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_GETMETADATAKEYLIST2;
                // Uncomment following to skip the second metadata retrieval
                //iState=STATE_STOP;
                RunIfNotReady();
            }
            else
            {
                // Start failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAKEYLIST2:
            if (aResponse.GetCmdStatus() == PVMFSuccess || aResponse.GetCmdStatus() == PVMFErrArgument)
            {
                iState = STATE_GETMETADATAVALUELIST2;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataKeys failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAVALUELIST2:
            if (aResponse.GetCmdStatus() == PVMFSuccess || aResponse.GetCmdStatus() == PVMFErrArgument)
            {
                fprintf(iTestMsgOutputFile, "After Start()\n");
                PrintMetadataInfo();
                iState = STATE_STOP;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataValue failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_STOP:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_GETMETADATAKEYLIST3;
                // Uncomment following to skip the third metadata retrieval
                //iState=STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // Stop failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAKEYLIST3:
            if (aResponse.GetCmdStatus() == PVMFSuccess || aResponse.GetCmdStatus() == PVMFErrArgument)
            {
                iState = STATE_GETMETADATAVALUELIST3;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataKeys failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAVALUELIST3:
            if (aResponse.GetCmdStatus() == PVMFSuccess || aResponse.GetCmdStatus() == PVMFErrArgument)
            {
                fprintf(iTestMsgOutputFile, "After Stop()\n");
                PrintMetadataInfo();
                iState = STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataValue failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASINK_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASINK_TEXT;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_TEXT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_RESET;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_RESET:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASOURCE;
                RunIfNotReady();
            }
            else
            {
                // Reset failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                PVPATB_TEST_IS_TRUE(true);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            break;

        default:
        {
            // Testing error if this is reached
            PVPATB_TEST_IS_TRUE(false);
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
        }
        break;
    }
}


void pvplayer_async_test_printmetadata::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    switch (aEvent.GetEventType())
    {
        case PVMFErrResourceConfiguration:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrResource:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrCorrupt:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrProcessing:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        default:
            // Unknown error and just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;
    }

    // Wait for engine to handle the error
    Cancel();
    iState = STATE_WAIT_FOR_ERROR_HANDLING;
    RunIfNotReady(5000000);
}


void pvplayer_async_test_printmetadata::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    // Wait for error handling to complete
    if (iState == STATE_WAIT_FOR_ERROR_HANDLING && aEvent.GetEventType() == PVMFInfoErrorHandlingComplete)
    {
        PVPlayerState pstate;
        iPlayer->GetPVPlayerStateSync(pstate);
        switch (pstate)
        {
            case PVP_STATE_INITIALIZED:
                Cancel();
                iState = STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
                break;

            case PVP_STATE_IDLE:
                Cancel();
                iState = STATE_REMOVEDATASOURCE;
                RunIfNotReady();
                break;

            default:
                // Engine should not be in any other state when
                // error handling completes
                PVPATB_TEST_IS_TRUE(false);
                Cancel();
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                break;
        }
    }
}


void pvplayer_async_test_printmetadata::PrintMetadataInfo()
{
    uint32 i = 0;

    fprintf(iTestMsgOutputFile, "Metadata key list (count=%d):\n", iMetadataKeyList.size());
    for (i = 0; i < iMetadataKeyList.size(); ++i)
    {
        fprintf(iTestMsgOutputFile, "Key %d: %s\n", (i + 1), iMetadataKeyList[i].get_cstr());
    }

    fprintf(iTestMsgOutputFile, "\nMetadata value list (count=%d):\n", iMetadataValueList.size());
    for (i = 0; i < iMetadataValueList.size(); ++i)
    {
        fprintf(iTestMsgOutputFile, "Value %d:\n", (i + 1));
        fprintf(iTestMsgOutputFile, "   Key string: %s\n", iMetadataValueList[i].key);

        switch (GetValTypeFromKeyString(iMetadataValueList[i].key))
        {
            case PVMI_KVPVALTYPE_CHARPTR:
                fprintf(iTestMsgOutputFile, "   Value:%s\n", iMetadataValueList[i].value.pChar_value);
                break;

            case PVMI_KVPVALTYPE_WCHARPTR:
            {
                // Assume string is in UCS-2 encoding so convert to UTF-8
                char tmpstr[65];
                oscl_UnicodeToUTF8(iMetadataValueList[i].value.pWChar_value,
                                   oscl_strlen(iMetadataValueList[i].value.pWChar_value), tmpstr, 65);
                tmpstr[64] = '\0';
                fprintf(iTestMsgOutputFile, "   Value(in UTF-8, first 64 chars):%s\n", tmpstr);
            }
            break;

            case PVMI_KVPVALTYPE_UINT32:
                fprintf(iTestMsgOutputFile, "   Value:%d\n", iMetadataValueList[i].value.uint32_value);
                break;

            case PVMI_KVPVALTYPE_INT32:
                fprintf(iTestMsgOutputFile, "   Value:%d\n", iMetadataValueList[i].value.int32_value);
                break;

            case PVMI_KVPVALTYPE_UINT8:
                fprintf(iTestMsgOutputFile, "   Value:%d\n", iMetadataValueList[i].value.uint8_value);
                break;

            case PVMI_KVPVALTYPE_FLOAT:
                fprintf(iTestMsgOutputFile, "   Value:%f\n", iMetadataValueList[i].value.float_value);
                break;

            case PVMI_KVPVALTYPE_DOUBLE:
                fprintf(iTestMsgOutputFile, "   Value:%f\n", iMetadataValueList[i].value.double_value);
                break;

            case PVMI_KVPVALTYPE_BOOL:
                if (iMetadataValueList[i].value.bool_value)
                {
                    fprintf(iTestMsgOutputFile, "   Value:true(1)\n");
                }
                else
                {
                    fprintf(iTestMsgOutputFile, "   Value:false(0)\n");
                }
                break;

            default:
                fprintf(iTestMsgOutputFile, "   Value: UNKNOWN VALUE TYPE\n");
                break;
        }

        fprintf(iTestMsgOutputFile, "   Length:%d  Capacity:%d\n", iMetadataValueList[i].length, iMetadataValueList[i].capacity);
    }

    fprintf(iTestMsgOutputFile, "\n\n");
}


//
// pvplayer_async_test_printmemstats section
//
void pvplayer_async_test_printmemstats::StartTest()
{
    AddToScheduler();
    iState = STATE_CREATE;
    RunIfNotReady();
}


void pvplayer_async_test_printmemstats::Run()
{
    int error = 0;

    switch (iState)
    {
        case STATE_CREATE:
        {
#if !(OSCL_BYPASS_MEMMGT)
            // Obtain the current mem stats
            OsclAuditCB auditCB;
            OsclMemInit(auditCB);
            if (auditCB.pAudit)
            {
                MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
                if (stats)
                {
                    iInitialNumBytes = stats->numBytes;
                    iInitialNumAllocs = stats->numAllocs;
                    fprintf(iTestMsgOutputFile, "Initial memstats: %d allocs, %d bytes\n", iInitialNumAllocs, iInitialNumBytes);
                }
                else
                {
                    fprintf(iTestMsgOutputFile, "Retrieving initial memory statistics failed! Memory statistics result would be invalid.\n");
                }
            }
            else
            {
                fprintf(iTestMsgOutputFile, "Memory audit not available! Memory statistics result would be invalid.\n");
            }
#else
            fprintf(iTestMsgOutputFile, "MEMORY AUDIT DISABLED\n");
#endif

            iPlayer = NULL;

            OSCL_TRY(error, iPlayer = PVPlayerFactory::CreatePlayer(this, this, this));
            if (error)
            {
                PVPATB_TEST_IS_TRUE(false);
                iObserver->TestCompleted(*iTestCase);
            }
            else
            {
                fprintf(iTestMsgOutputFile, "After player instantiation: ");
                PrintMemStats();
                iState = STATE_ADDDATASOURCE;
                RunIfNotReady();
            }
        }
        break;

        case STATE_ADDDATASOURCE:
        {
            iDataSource = new PVPlayerDataSourceURL;
            oscl_UTF8ToUnicode(iFileName, oscl_strlen(iFileName), iTmpWCharBuffer, 512);
            iTmpWCharBuffer[511] = '\0';
            iFileNameWStr = SOURCENAME_PREPEND_WSTRING;
            iFileNameWStr += iTmpWCharBuffer;
            iDataSource->SetDataSourceURL(iFileNameWStr);
            iDataSource->SetDataSourceFormatType(iFileType);
            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }

        break;

        case STATE_INIT:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Init((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAKEYLIST:
        {
            iMetadataKeyList.clear();
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataKeys(iMetadataKeyList, 0, -1, NULL, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAVALUELIST:
        {
            iMetadataValueList.clear();
            iNumValues = 0;
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataValues(iMetadataKeyList, 0, -1, iNumValues, iMetadataValueList, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_VIDEO:
        {
            OSCL_wHeapString<OsclMemAllocator> videosinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            videosinkfilename += _STRLIT_WCHAR("test_player_printmemstats_video.dat");

            iMIOFileOutVideo = iMioFactory->CreateVideoOutput((OsclAny*) & videosinkfilename);
            iIONodeVideo = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutVideo);
            iDataSinkVideo = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkNode(iIONodeVideo);
            if (iCompressedVideo)
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(PVMF_M4V);
            }
            else
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(PVMF_YUV420);
            }

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_AUDIO:
        {
            OSCL_wHeapString<OsclMemAllocator> audiosinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            audiosinkfilename += _STRLIT_WCHAR("test_player_printmemstats_audio.dat");

            iMIOFileOutAudio = iMioFactory->CreateAudioOutput((OsclAny*) & audiosinkfilename);
            iIONodeAudio = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutAudio);
            iDataSinkAudio = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkNode(iIONodeAudio);
            if (iCompressedAudio)
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(PVMF_MPEG4_AUDIO);
            }
            else
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(AUDIOSINK_FORMAT_TYPE);
            }

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_PREPARE:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Prepare((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_START:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Start((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_PRINTMEMSTATS:
        {
            PVPPlaybackPosition curpos;
            curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
            iPlayer->GetCurrentPositionSync(curpos);
            fprintf(iTestMsgOutputFile, "After playing %d ms: ", curpos.iPosValue);
            PrintMemStats();

            ++iPlayTimeCtr;
            if (iPlayTimeCtr < 20)
            {
                iState = STATE_PRINTMEMSTATS;
                RunIfNotReady(1000000);
            }
            else
            {
                iState = STATE_STOP;
                RunIfNotReady();
            }
        }
        break;

        case STATE_STOP:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Stop((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_VIDEO:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_AUDIO:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_RESET:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Reset((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASOURCE:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_WAIT_FOR_ERROR_HANDLING:
        {
            // Timed out waiting for error handling to complete
            PVPATB_TEST_IS_TRUE(false);
            // Forcibly terminate the test
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
        }
        break;

        case STATE_CLEANUPANDCOMPLETE:
        {
            PVPATB_TEST_IS_TRUE(PVPlayerFactory::DeletePlayer(iPlayer));
            iPlayer = NULL;

            fprintf(iTestMsgOutputFile, "After player destruction: ");
            PrintMemStats();

            delete iDataSource;
            iDataSource = NULL;

            delete iDataSinkVideo;
            iDataSinkVideo = NULL;

            delete iDataSinkAudio;
            iDataSinkAudio = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeVideo);
            iIONodeVideo = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeAudio);
            iIONodeAudio = NULL;

            iMioFactory->DestroyVideoOutput(iMIOFileOutVideo);
            iMIOFileOutVideo = NULL;

            iMioFactory->DestroyAudioOutput(iMIOFileOutAudio);
            iMIOFileOutAudio = NULL;

            fprintf(iTestMsgOutputFile, "After cleanup: ");
            PrintMemStats();

            iObserver->TestCompleted(*iTestCase);
        }
        break;

        default:
            break;

    }
}


void pvplayer_async_test_printmemstats::CommandCompleted(const PVCmdResponse& aResponse)
{
    if (aResponse.GetCmdId() != iCurrentCmdId)
    {
        // Wrong command ID.
        PVPATB_TEST_IS_TRUE(false);
        iState = STATE_CLEANUPANDCOMPLETE;
        RunIfNotReady();
        return;
    }

    if (aResponse.GetContext() != NULL)
    {
        if (aResponse.GetContext() == (OsclAny*)&iContextObject)
        {
            if (iContextObject != iContextObjectRefValue)
            {
                // Context data value was corrupted
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                return;
            }
        }
        else
        {
            // Context data pointer was corrupted
            PVPATB_TEST_IS_TRUE(false);
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
            return;
        }
    }

    switch (iState)
    {
        case STATE_ADDDATASOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After AddDataSource(): ");
                PrintMemStats();

                iState = STATE_INIT;
                RunIfNotReady();
            }
            else
            {
                // AddDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After Init(): ");
                PrintMemStats();

                iState = STATE_GETMETADATAKEYLIST;
                RunIfNotReady();
            }
            else
            {
                // Init failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAKEYLIST:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After GetMetadataKeys(): ");
                PrintMemStats();

                iState = STATE_GETMETADATAVALUELIST;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataKeys failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_GETMETADATAVALUELIST:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After GetMetadataValue(): ");
                PrintMemStats();

                iState = STATE_ADDDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // GetMetadataValue failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After AddDataSink() video: ");
                PrintMemStats();

                iState = STATE_ADDDATASINK_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After AddDataSink() audio: ");
                PrintMemStats();

                iState = STATE_PREPARE;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_PREPARE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After Prepare(): ");
                PrintMemStats();

                iState = STATE_START;
                RunIfNotReady();
            }
            else
            {
                // Prepare failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_START:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After Start(): ");
                PrintMemStats();

                iState = STATE_PRINTMEMSTATS;
                RunIfNotReady(1000000);
            }
            else
            {
                // Start failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_STOP:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After Stop(): ");
                PrintMemStats();

                iState = STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // Stop failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After RemoveDataSink() video: ");
                PrintMemStats();

                iState = STATE_REMOVEDATASINK_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After RemoveDataSink() audio: ");
                PrintMemStats();

                iState = STATE_RESET;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_RESET:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After Reset(): ");
                PrintMemStats();

                iState = STATE_REMOVEDATASOURCE;
                RunIfNotReady();
            }
            else
            {
                // Reset failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                fprintf(iTestMsgOutputFile, "After RemoveDataSource(): ");
                PrintMemStats();

                PVPATB_TEST_IS_TRUE(true);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            break;

        default:
        {
            // Testing error if this is reached
            PVPATB_TEST_IS_TRUE(false);
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
        }
        break;
    }
}


void pvplayer_async_test_printmemstats::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    switch (aEvent.GetEventType())
    {
        case PVMFErrResourceConfiguration:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrResource:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrCorrupt:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrProcessing:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        default:
            // Unknown error and just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;
    }

    // Wait for engine to handle the error
    Cancel();
    iState = STATE_WAIT_FOR_ERROR_HANDLING;
    RunIfNotReady(5000000);
}


void pvplayer_async_test_printmemstats::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    // Wait for error handling to complete
    if (iState == STATE_WAIT_FOR_ERROR_HANDLING && aEvent.GetEventType() == PVMFInfoErrorHandlingComplete)
    {
        PVPlayerState pstate;
        iPlayer->GetPVPlayerStateSync(pstate);
        switch (pstate)
        {
            case PVP_STATE_INITIALIZED:
                Cancel();
                iState = STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
                break;

            case PVP_STATE_IDLE:
                Cancel();
                iState = STATE_REMOVEDATASOURCE;
                RunIfNotReady();
                break;

            default:
                // Engine should not be in any other state when
                // error handling completes
                PVPATB_TEST_IS_TRUE(false);
                Cancel();
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                break;
        }
    }
}


void pvplayer_async_test_printmemstats::PrintMemStats()
{
#if !(OSCL_BYPASS_MEMMGT)
    // Obtain the current mem stats
    OsclAuditCB auditCB;
    OsclMemInit(auditCB);
    if (auditCB.pAudit)
    {
        MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
        if (stats)
        {
            fprintf(iTestMsgOutputFile, "In use %d allocs, %d bytes.\n", stats->numAllocs - iInitialNumAllocs, stats->numBytes - iInitialNumBytes);
        }
        else
        {
            fprintf(iTestMsgOutputFile, "Retrieving memory statistics failed! Memory statistics result would be invalid.\n");
        }
    }
    else
    {
        fprintf(iTestMsgOutputFile, "Memory audit not available! Memory statistics result would be invalid.\n");
    }
#else
    fprintf(iTestMsgOutputFile, "MEMORY AUDIT DISABLED\n");
#endif
}


//
// pvplayer_async_test_playuntileos section
//
void pvplayer_async_test_playuntileos::StartTest()
{
    AddToScheduler();
    iState = STATE_CREATE;
    RunIfNotReady();
}


void pvplayer_async_test_playuntileos::Run()
{
    int error = 0;

    switch (iState)
    {
        case STATE_CREATE:
        {
            iPlayer = NULL;

            OSCL_TRY(error, iPlayer = PVPlayerFactory::CreatePlayer(this, this, this));
            if (error)
            {
                PVPATB_TEST_IS_TRUE(false);
                iObserver->TestCompleted(*iTestCase);
            }
            else
            {
                iState = STATE_ADDDATASOURCE;
                RunIfNotReady();
            }
        }
        break;

        case STATE_ADDDATASOURCE:
        {
            iDataSource = new PVPlayerDataSourceURL;
            oscl_UTF8ToUnicode(iFileName, oscl_strlen(iFileName), iTmpWCharBuffer, 512);
            iFileNameWStr.set(iTmpWCharBuffer, oscl_strlen(iTmpWCharBuffer));
            iDataSource->SetDataSourceURL(iFileNameWStr);
            iDataSource->SetDataSourceFormatType(iFileType);
            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }

        break;

        case STATE_INIT:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Init((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_VIDEO:
        {
            OSCL_wHeapString<OsclMemAllocator> sinkfilename;
            sinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            sinkfilename += _STRLIT_WCHAR("test_player_playuntileos_");
            if (iCompressedVideo)
            {
                sinkfilename += _STRLIT_WCHAR("compressed_");
            }
            OSCL_wHeapString<OsclMemAllocator> inputfilename;
            RetrieveFilename(iFileNameWStr.get_str(), inputfilename);
            sinkfilename += inputfilename;
            sinkfilename += _STRLIT_WCHAR("_video.dat");

            iMIOFileOutVideo = iMioFactory->CreateVideoOutput((OsclAny*) & sinkfilename);
            iIONodeVideo = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutVideo);
            iDataSinkVideo = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkNode(iIONodeVideo);
            if (iCompressedVideo)
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(PVMF_M4V);
            }
            else
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(PVMF_YUV420);
            }

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_AUDIO:
        {
            OSCL_wHeapString<OsclMemAllocator> sinkfilename;
            sinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            sinkfilename += _STRLIT_WCHAR("test_player_playuntileos_");
            if (iCompressedAudio)
            {
                sinkfilename += _STRLIT_WCHAR("compressed_");
            }
            OSCL_wHeapString<OsclMemAllocator> inputfilename;
            RetrieveFilename(iFileNameWStr.get_str(), inputfilename);
            sinkfilename += inputfilename;
            sinkfilename += _STRLIT_WCHAR("_audio.dat");

            iMIOFileOutAudio = iMioFactory->CreateAudioOutput((OsclAny*) & sinkfilename);
            iIONodeAudio = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutAudio);
            iDataSinkAudio = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkNode(iIONodeAudio);
            if (iCompressedAudio)
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(PVMF_MPEG4_AUDIO);
            }
            else
            {
                ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(AUDIOSINK_FORMAT_TYPE);
            }

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_TEXT:
        {
            OSCL_wHeapString<OsclMemAllocator> sinkfilename;
            sinkfilename = OUTPUTNAME_PREPEND_WSTRING;
            sinkfilename += _STRLIT_WCHAR("test_player_playuntileos_");
            OSCL_wHeapString<OsclMemAllocator> inputfilename;
            RetrieveFilename(iFileNameWStr.get_str(), inputfilename);
            sinkfilename += inputfilename;
            sinkfilename += _STRLIT_WCHAR("_text.dat");

            iMIOFileOutText = iMioFactory->CreateTextOutput((OsclAny*) & sinkfilename);
            iIONodeText = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutText);
            iDataSinkText = new PVPlayerDataSinkPVMFNode;
            ((PVPlayerDataSinkPVMFNode*)iDataSinkText)->SetDataSinkNode(iIONodeText);
            ((PVPlayerDataSinkPVMFNode*)iDataSinkText)->SetDataSinkFormatType(PVMF_3GPP_TIMEDTEXT);

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkText, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_PREPARE:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Prepare((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_START:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Start((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_STOP:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Stop((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_VIDEO:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_AUDIO:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_TEXT:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkText, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_RESET:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Reset((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASOURCE:
        {
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, PVPATB_TEST_IS_TRUE(false); iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_WAIT_FOR_ERROR_HANDLING:
        {
            // Timed out waiting for error handling to complete
            PVPATB_TEST_IS_TRUE(false);
            // Forcibly terminate the test
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
        }
        break;

        case STATE_CLEANUPANDCOMPLETE:
        {
            PVPATB_TEST_IS_TRUE(PVPlayerFactory::DeletePlayer(iPlayer));
            iPlayer = NULL;

            delete iDataSource;
            iDataSource = NULL;

            delete iDataSinkVideo;
            iDataSinkVideo = NULL;

            delete iDataSinkAudio;
            iDataSinkAudio = NULL;

            delete iDataSinkText;
            iDataSinkText = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeVideo);
            iIONodeVideo = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeAudio);
            iIONodeAudio = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeText);
            iIONodeText = NULL;

            iMioFactory->DestroyVideoOutput(iMIOFileOutVideo);
            iMIOFileOutVideo = NULL;

            iMioFactory->DestroyAudioOutput(iMIOFileOutAudio);
            iMIOFileOutAudio = NULL;

            iMioFactory->DestroyTextOutput(iMIOFileOutText);
            iMIOFileOutText = NULL;

            iObserver->TestCompleted(*iTestCase);
        }
        break;

        default:
            break;

    }
}


void pvplayer_async_test_playuntileos::CommandCompleted(const PVCmdResponse& aResponse)
{
    if (aResponse.GetCmdId() != iCurrentCmdId)
    {
        // Wrong command ID.
        PVPATB_TEST_IS_TRUE(false);
        iState = STATE_CLEANUPANDCOMPLETE;
        RunIfNotReady();
        return;
    }

    if (aResponse.GetContext() != NULL)
    {
        if (aResponse.GetContext() == (OsclAny*)&iContextObject)
        {
            if (iContextObject != iContextObjectRefValue)
            {
                // Context data value was corrupted
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                return;
            }
        }
        else
        {
            // Context data pointer was corrupted
            PVPATB_TEST_IS_TRUE(false);
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
            return;
        }
    }

    switch (iState)
    {
        case STATE_ADDDATASOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_INIT;
                RunIfNotReady();
            }
            else
            {
                // AddDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_ADDDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // Init failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_ADDDATASINK_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_ADDDATASINK_TEXT;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_ADDDATASINK_TEXT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_PREPARE;
                RunIfNotReady();
            }
            else
            {
                // AddDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_PREPARE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_START;
                RunIfNotReady();
            }
            else
            {
                // Prepare failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_START:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                // Do nothing and wait for EOS event
                fprintf(iTestMsgOutputFile, "Playback started.\n");
            }
            else
            {
                // Start failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_STOP:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
            }
            else
            {
                // Stop failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_VIDEO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASINK_AUDIO;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_AUDIO:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASINK_TEXT;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASINK_TEXT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_RESET;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSink failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_RESET:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                iState = STATE_REMOVEDATASOURCE;
                RunIfNotReady();
            }
            else
            {
                // Reset failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_WAIT_FOR_ERROR_HANDLING;
                RunIfNotReady(5000000);
            }
            break;

        case STATE_REMOVEDATASOURCE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                PVPATB_TEST_IS_TRUE(true);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            else
            {
                // RemoveDataSource failed
                PVPATB_TEST_IS_TRUE(false);
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
            }
            break;

        default:
        {
            // Testing error if this is reached
            PVPATB_TEST_IS_TRUE(false);
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
        }
        break;
    }
}


void pvplayer_async_test_playuntileos::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    switch (aEvent.GetEventType())
    {
        case PVMFErrResourceConfiguration:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrResource:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrCorrupt:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        case PVMFErrProcessing:
            // Just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;

        default:
            // Unknown error and just log the error
            PVPATB_TEST_IS_TRUE(false);
            break;
    }

    // Wait for engine to handle the error
    Cancel();
    iState = STATE_WAIT_FOR_ERROR_HANDLING;
    RunIfNotReady(5000000);
}


void pvplayer_async_test_playuntileos::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    // Check for stop time reached event
    if (aEvent.GetEventType() == PVMFInfoEndOfData)
    {
        PVInterface* iface = (PVInterface*)(aEvent.GetEventExtensionInterface());
        if (iface == NULL)
        {
            return;
        }
        PVUuid infomsguuid = PVMFErrorInfoMessageInterfaceUUID;
        PVMFErrorInfoMessageInterface* infomsgiface = NULL;
        if (iface->queryInterface(infomsguuid, (PVInterface*&)infomsgiface) == true)
        {
            int32 infocode;
            PVUuid infouuid;
            infomsgiface->GetCodeUUID(infocode, infouuid);
            if ((infouuid == PVPlayerErrorInfoEventTypesUUID) && (infocode == PVPlayerInfoEndOfClipReached))
            {
                fprintf(iTestMsgOutputFile, "EOS received. Stopping playback.\n");
                iState = STATE_STOP;
                Cancel();
                RunIfNotReady();
            }
        }
    }
    // Check and print out playback position status
    else if (aEvent.GetEventType() == PVMFInfoPositionStatus)
    {
        PVInterface* iface = (PVInterface*)(aEvent.GetEventExtensionInterface());
        if (iface == NULL)
        {
            return;
        }
        PVUuid infomsguuid = PVMFErrorInfoMessageInterfaceUUID;
        PVMFErrorInfoMessageInterface* infomsgiface = NULL;
        if (iface->queryInterface(infomsguuid, (PVInterface*&)infomsgiface) == true)
        {
            int32 infocode;
            PVUuid infouuid;
            infomsgiface->GetCodeUUID(infocode, infouuid);
            if ((infouuid == PVPlayerErrorInfoEventTypesUUID) && (infocode == PVPlayerInfoPlaybackPositionStatus))
            {
                uint8* localbuf = aEvent.GetLocalBuffer();
                uint32 pbpos = 0;
                oscl_memcpy(&pbpos, &(localbuf[4]), 4);
                fprintf(iTestMsgOutputFile, "Playback status(time) %d ms\n", pbpos);
            }
        }

        PVPPlaybackPosition curpos;

        curpos.iPosValue.percent_value = 0;
        curpos.iPosUnit = PVPPBPOSUNIT_PERCENT;
        if (iPlayer->GetCurrentPositionSync(curpos) == PVMFSuccess)
        {
            fprintf(iTestMsgOutputFile, "Playback status(percentage) %d%%\n", curpos.iPosValue.percent_value);
        }


        curpos.iPosValue.samplenum_value = 0;
        curpos.iPosUnit = PVPPBPOSUNIT_SAMPLENUMBER;
        if (iPlayer->GetCurrentPositionSync(curpos) == PVMFSuccess)
        {
            fprintf(iTestMsgOutputFile, "Playback status(sample number) %d\n", curpos.iPosValue.samplenum_value);
        }

        curpos.iPosValue.datapos_value = 0;
        curpos.iPosUnit = PVPPBPOSUNIT_DATAPOSITION;
        if (iPlayer->GetCurrentPositionSync(curpos) == PVMFSuccess)
        {
            fprintf(iTestMsgOutputFile, "Playback status(data position) %d bytes\n", curpos.iPosValue.datapos_value);
        }

        fprintf(iTestMsgOutputFile, "------------------------------\n");
    }
    else if (iState == STATE_WAIT_FOR_ERROR_HANDLING && aEvent.GetEventType() == PVMFInfoErrorHandlingComplete)
    {
        PVPlayerState pstate;
        iPlayer->GetPVPlayerStateSync(pstate);
        switch (pstate)
        {
            case PVP_STATE_INITIALIZED:
                Cancel();
                iState = STATE_REMOVEDATASINK_VIDEO;
                RunIfNotReady();
                break;

            case PVP_STATE_IDLE:
                Cancel();
                iState = STATE_REMOVEDATASOURCE;
                RunIfNotReady();
                break;

            default:
                // Engine should not be in any other state when
                // error handling completes
                PVPATB_TEST_IS_TRUE(false);
                Cancel();
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                break;
        }
    }
}






