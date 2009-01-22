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
#if 0
#include "invalid_command_test.h"


void invalid_command_test::test()
{
    fprintf(fileoutput, "Start invalid command test.\n");

    scheduler = OsclExecScheduler::Current();

    this->AddToScheduler();

    for (int i = PVT_COMMAND_INIT; i < PVT_LAST_COMMAND; i++)
    {
        test_is_true(test_command((TPV2WayCommandType) i));
    }

    this->RemoveFromScheduler();
}

void invalid_command_test::Run()
{
}

void invalid_command_test::DoCancel()
{
}


void invalid_command_test::HandleInformationalEventL(const CPVCmnAsyncInfoEvent& aEvent)
{
    OSCL_UNUSED_ARG(aEvent);
}

void invalid_command_test::CommandCompletedL(const CPVCmnCmdResp& aResponse)
{
    OSCL_UNUSED_ARG(aResponse);
}

bool invalid_command_test::test_command(TPV2WayCommandType cmd)
{
    CPV2WayInterface *temp = NULL;
    int error = 0;
    bool status = true;

    OSCL_TRY(error, temp = CPV2WayEngineFactory::CreateTerminalL(PV_324M,
                           (MPVCmnCmdStatusObserver *) this,
                           (MPVCmnInfoEventObserver *) this,
                           (MPVCmnErrorEventObserver *) this));

    if (error)
    {
        return status;
    }

    create_sink_source();

    switch (cmd)
    {
        case PVT_COMMAND_ADD_DATA_SOURCE:
        {
            OSCL_TRY(error, temp->AddDataSourceL(*iVideoSourceYUV));
        }
        break;

        case PVT_COMMAND_REMOVE_DATA_SOURCE:
            OSCL_TRY(error, temp->RemoveDataSourceL(*iVideoSourceYUV));
            break;

        case PVT_COMMAND_ADD_DATA_SINK:
        {
            TPVChannelId id;
            OSCL_TRY(error, temp->AddDataSinkL(*iVideoSinkYUV, id));
        }
        break;

        case PVT_COMMAND_REMOVE_DATA_SINK:
            OSCL_TRY(error, temp->RemoveDataSinkL(*iVideoSinkYUV));
            break;

        case PVT_COMMAND_CONNECT:
            OSCL_TRY(error, temp->ConnectL(iConnectOptions));
            break;

        case PVT_COMMAND_DISCONNECT:
        {
            TPVPostDisconnectOption option = EDisconnectLine;
            OSCL_TRY(error, temp->DisconnectL(option));
        }
        break;

        case PVT_COMMAND_SET_TRADEOFF:
            OSCL_TRY(error, temp->SetLatencyQualityTradeoffL(*iVideoSourceYUV, 1));
            break;

        case PVT_COMMAND_PAUSE:
            OSCL_TRY(error, temp->PauseL(*iVideoSourceYUV));
            break;

        case PVT_COMMAND_RESUME:
            OSCL_TRY(error, temp->ResumeL(*iVideoSourceYUV));
            break;

        case PVT_COMMAND_SEND_USER_INPUT:
        {
#ifndef NO_2WAY_324
            CPVUserInputAlphanumeric uii(NULL, 0);
            OSCL_TRY(error, temp->SendUserInputL(uii));
#else
            error = PVMFErrNotSupported;
#endif
        }
        break;

        case PVT_COMMAND_GET_CALL_STATISTICS:
            CPVCmn2WayStatistics stats;
            OSCL_TRY(error, temp->GetCallStatistics(stats));
            break;

        case PVT_COMMAND_QUERY_UUID:
        {
            TPVCmnMIMEType type = NULL;
            Oscl_Vector<PVUuid, BasicAlloc> uuid;
            OSCL_TRY(error, temp->QueryUUID(type, uuid));
        }
        break;

        case PVT_COMMAND_CANCEL_ALL_COMMANDS:
            OSCL_TRY(error, temp->CancelAllCommands());
            break;

        case PVT_COMMAND_INIT_RECORD_FILE:
        {
            OSCL_wHeapString<OsclMemAllocator> filename(RECORDED_CALL_FILENAME);
            OSCL_TRY(error, temp->InitRecordFileL(filename));
        }
        break;

        case PVT_COMMAND_RESET_RECORD_FILE:
            OSCL_TRY(error, temp->ResetRecordFileL());
            break;

        case PVT_COMMAND_START_RECORD:
            OSCL_TRY(error, temp->StartRecordL(*iVideoSinkYUV));
            break;

        case PVT_COMMAND_STOP_RECORD:
            OSCL_TRY(error, temp->StartRecordL(*iVideoSinkYUV));
            break;

        case PVT_COMMAND_PAUSE_RECORD:
            OSCL_TRY(error, temp->StartRecordL(*iVideoSinkYUV));
            break;

        case PVT_COMMAND_RESUME_RECORD:
            OSCL_TRY(error, temp->StartRecordL(*iVideoSinkYUV));
            break;

        case PVT_COMMAND_ADD_PREVIEW_SINK:
            OSCL_TRY(error, temp->AddPreviewSinkL(*iVideoSourceYUV, *iVideoPreview));
            break;

        case PVT_COMMAND_REMOVE_PREVIEW_SINK:
            OSCL_TRY(error, temp->RemovePreviewSinkL(*iVideoPreview));
            break;

        case PVT_COMMAND_PAUSE_PREVIEW_SINK:
            OSCL_TRY(error, temp->PausePreviewSinkL(*iVideoPreview));
            break;

        case PVT_COMMAND_RESUME_PREVIEW_SINK:
            OSCL_TRY(error, temp->ResumePreviewSinkL(*iVideoPreview));
            break;

        case PVT_COMMAND_INIT_PLAY_FILE:
        {
            OSCL_wHeapString<OsclMemAllocator> filename(AUDIO_H263_PLAY_FILENAME);
            OSCL_TRY(error, temp->InitPlayFileL(filename));
        }
        break;

        case PVT_COMMAND_RESET_PLAY_FILE:
            OSCL_TRY(error, temp->ResetPlayFileL());
            break;

        case PVT_COMMAND_USE_PLAY_FILE:
            OSCL_TRY(error, temp->UsePlayFileAsSourceL(true));
            break;

        case PVT_COMMAND_START_PLAY:
            OSCL_TRY(error, temp->StartPlayL());
            break;

        case PVT_COMMAND_STOP_PLAY:
            OSCL_TRY(error, temp->StopPlayL());
            break;

        case PVT_COMMAND_PAUSE_PLAY:
            OSCL_TRY(error, temp->PausePlayL());
            break;

        case PVT_COMMAND_RESUME_PLAY:
            OSCL_TRY(error, temp->ResumePlayL());
            break;

        case PVT_COMMAND_INIT:
        case PVT_COMMAND_GET_SDK_INFO:
        case PVT_COMMAND_GET_SDK_MODULE_INFO:
        case PVT_COMMAND_GET_PV2WAY_STATE:
        case PVT_COMMAND_RESET:
        case PVT_COMMAND_QUERY_INTERFACE:
        case PVT_COMMAND_SET_LOG_APPENDER:
        case PVT_COMMAND_REMOVE_LOG_APPENDER:
        case PVT_COMMAND_SET_LOG_LEVEL:
        case PVT_COMMAND_GET_LOG_LEVEL:
        default:
            error = -1;
            break;
    }

    destroy_sink_source();

    if (!error)
    {
        status = false;
    }

    CPV2WayEngineFactory::DeleteTerminal(temp);
    return status;
}
#endif






