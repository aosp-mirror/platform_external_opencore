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
#include "av_duplicate_test.h"



void av_duplicate_test::test()
{
    fprintf(fileoutput, "Start a/v duplicate test, num runs %d, proxy %d; try to add extra sources and sinks.\n", iMaxRuns, iUseProxy);
    int error = 0;

    scheduler = OsclExecScheduler::Current();

    this->AddToScheduler();

    if (start_async_test())
    {
        OSCL_TRY(error, scheduler->StartScheduler());
        if (error != 0)
        {
            OSCL_LEAVE(error);
        }
    }

    destroy_sink_source();

    this->RemoveFromScheduler();
}


void av_duplicate_test::Run()
{
    if (terminal)
    {
        if (iUseProxy)
        {
            //CPV2WayProxyFactory::DeleteTerminal(terminal);
        }
        else
        {
            CPV2WayEngineFactory::DeleteTerminal(terminal);
        }
        terminal = NULL;
    }

    if (timer)
    {
        delete timer;
        timer = NULL;
    }

    scheduler->StopScheduler();
}

void av_duplicate_test::DoCancel()
{
}

void av_duplicate_test::HandleInformationalEventL(const CPVCmnAsyncInfoEvent& aEvent)
{
    int error = 0;
    /*
    	switch (aEvent.GetEventType())
    	{
    	case PVT_INDICATION_INCOMING_TRACK:
    		TPVChannelId id;

    		if(((CPVCmnAsyncEvent&)aEvent).GetLocalBuffer()[0] == PV_VIDEO){
    			OSCL_TRY(error, iVideoAddSinkId = terminal->AddDataSinkL(*iVideoSinkYUV, id));
    			if (error)
    			{
    				test_is_true(false);
    				timer->Cancel();
    				disconnect();
    			}
    		}

    		if(((CPVCmnAsyncEvent&)aEvent).GetLocalBuffer()[0] == PV_AUDIO){
    			OSCL_TRY(error, iAudioAddSinkId = terminal->AddDataSinkL(*iAudioSink, id));
    			if (error)
    			{
    				test_is_true(false);
    				timer->Cancel();
    				disconnect();
    			}
    		}
    		break;

    	case PVT_INDICATION_DISCONNECT:
    		iAudioSourceAdded = false;
    		iVideoSourceAdded = false;
    		iAudioSinkAdded = false;
    		iVideoSinkAdded = false;
    		break;

    	case PVT_INDICATION_CLOSE_TRACK:
    		if (((CPVCmnAsyncEvent&)aEvent).GetLocalBuffer()[0] == PV_VIDEO)
    		{
    			if (((CPVCmnAsyncEvent&)aEvent).GetLocalBuffer()[1] == INCOMING)
    			{
    				iVideoSinkAdded = false;
    			}
    			else
    			{
    				iVideoSourceAdded = false;
    			}
    		}
    		else if (((CPVCmnAsyncEvent&)aEvent).GetLocalBuffer()[0] == PV_AUDIO)
    		{
    			if (((CPVCmnAsyncEvent&)aEvent).GetLocalBuffer()[1] == INCOMING)
    			{
    				iAudioSinkAdded = false;
    			}
    			else
    			{
    				iAudioSourceAdded = false;
    			}
    		}
    		break;

    	case PVT_INDICATION_INTERNAL_ERROR:
    		break;

    	default:
    		break;
    	}
    	*/
}

void av_duplicate_test::CommandCompletedL(const CPVCmnCmdResp& aResponse)
{
    int error = 0;
    TPVChannelId id;
    switch (aResponse.GetCmdType())
    {
        case PVT_COMMAND_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                OSCL_TRY(error, iCommsAddSourceId = terminal->AddDataSource(*iCommServer));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    reset();
                }
            }
            else
            {
                test_is_true(false);
                timer->Cancel();
                RunIfNotReady();
            }
            break;

        case PVT_COMMAND_RESET:
            RunIfNotReady();
            break;

        case PVT_COMMAND_ADD_DATA_SOURCE:
            if (aResponse.GetCmdId() == iCommsAddSourceId)
            {
                OSCL_TRY(error, terminal->Connect(iConnectOptions));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    reset();
                }
            }
            else if (aResponse.GetCmdId() == iAudioAddSourceId)
            {
                if (aResponse.GetCmdStatus() == PVMFSuccess)
                {
                    iAudioSourceAdded = true;
                    start_duplicates_if_ready();
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                iAudioAddSourceId = 0;
            }
            else if (aResponse.GetCmdId() == iAudioAddSource2Id)
            {
                if (aResponse.GetCmdStatus() == PVMFErrInvalidState)
                {
                    test_is_true(true);
                    // now try and add a duplicate VideoSource
                    OSCL_TRY(error, iVideoAddSource2Id = terminal->AddDataSource(*iVideoSourceM4V));
                    if (error)
                    {
                        test_is_true(false);
                        timer->Cancel();
                        disconnect();
                    }
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
            }
            else if (aResponse.GetCmdId() == iVideoAddSourceId)
            {
                if (aResponse.GetCmdStatus() == PVMFSuccess)
                {
                    iVideoSourceAdded = true;
                    start_duplicates_if_ready();
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                iVideoAddSourceId = 0;
            }
            else if (aResponse.GetCmdId() == iVideoAddSource2Id)
            {
                if (aResponse.GetCmdStatus() == PVMFErrInvalidState)
                {
                    test_is_true(true);
                    // now add a duplicate audio sink
                    OSCL_TRY(error, iAudioAddSink2Id = terminal->AddDataSink(*iAudioSink2, id));
                    if (error)
                    {
                        test_is_true(false);
                        timer->Cancel();
                        disconnect();
                    }
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
            }
            break;

        case PVT_COMMAND_REMOVE_DATA_SOURCE:
            if (aResponse.GetCmdId() == iAudioRemoveSourceId)
            {
                iAudioRemoveSourceId = 0;
                iAudioSourceAdded = false;
            }
            else if (aResponse.GetCmdId() == iVideoRemoveSourceId)
            {
                iVideoRemoveSourceId = 0;
                iVideoSourceAdded = false;
            }
            break;

        case PVT_COMMAND_ADD_DATA_SINK:
            if (aResponse.GetCmdId() == iAudioAddSinkId)
            {
                if (aResponse.GetCmdStatus() == PVMFSuccess)
                {
                    iAudioSinkAdded = true;
                    start_duplicates_if_ready();
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                iAudioAddSinkId = 0;
            }
            else if (aResponse.GetCmdId() == iVideoAddSinkId)
            {
                if (aResponse.GetCmdStatus() == PVMFSuccess)
                {
                    iVideoSinkAdded = true;
                    start_duplicates_if_ready();
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                iVideoAddSinkId = 0;
            }
            else if (aResponse.GetCmdId() == iAudioAddSink2Id)
            {
                if (aResponse.GetCmdStatus() == PVMFErrInvalidState)
                {
                    // now try and add a duplicate video sink
                    OSCL_TRY(error, iVideoAddSink2Id = terminal->AddDataSink(*iVideoSinkM4V, id));
                    if (error)
                    {
                        test_is_true(false);
                        timer->Cancel();
                        disconnect();
                    }
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
            }
            else if (aResponse.GetCmdId() == iVideoAddSink2Id)
            {
                if (aResponse.GetCmdStatus() == PVMFErrInvalidState)
                {
                    test_is_true(true);
                    disconnect();
                }
                else
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
            }

            break;

        case PVT_COMMAND_REMOVE_DATA_SINK:
            if (aResponse.GetCmdId() == iAudioRemoveSinkId)
            {
                iAudioRemoveSinkId = 0;
                iAudioSinkAdded = false;
            }
            else if (aResponse.GetCmdId() == iVideoRemoveSinkId)
            {
                iVideoRemoveSinkId  = 0;
                iVideoSinkAdded = false;
            }
            break;

        case PVT_COMMAND_CONNECT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                //OSCL_TRY(error, iGetSessionParamsId = terminal->GetSessionParamsL(sessionParams));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
            }
            else
            {
                test_is_true(false);
                timer->Cancel();
                reset();
            }
            break;
#if 0
        case PVT_COMMAND_GET_SESSION_PARAMS:
            // Don't bother checking supported formats here
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                OSCL_TRY(error, iAudioAddSourceId = terminal->AddDataSourceL(*iAudioSource));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                OSCL_TRY(error, iVideoAddSourceId = terminal->AddDataSourceL(*iVideoSourceYUV));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                OSCL_TRY(error, iVideoAddSinkId = terminal->AddDataSinkL(*iVideoSinkYUV, id));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
                OSCL_TRY(error, iAudioAddSinkId = terminal->AddDataSinkL(*iAudioSink, id));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    disconnect();
                }
            }
            else
            {
                test_is_true(false);
                timer->Cancel();
                reset();
            }
            break;
#endif
        case PVT_COMMAND_DISCONNECT:
            iCurrentRun++;
            if (check_audio_started() &&
                    check_video_started() &&
                    (iCurrentRun >= iMaxRuns))
            {
                test_is_true(true);
            }

            iAudioSourceAdded = false;
            iVideoSourceAdded = false;
            iAudioSinkAdded = false;
            iVideoSinkAdded = false;


            if (iCurrentRun < iMaxRuns)
            {
                OSCL_TRY(error, terminal->ConnectL(iConnectOptions));
                if (error)
                {
                    test_is_true(false);
                    timer->Cancel();
                    reset();
                }
            }
            else
            {
                reset();
            }
            break;
    }
}

void av_duplicate_test::TimerCallback()
{
    disconnect();
}

bool av_duplicate_test::start_async_test()
{
    int error = 0;

    timer = new engine_timer(this);
    if (timer == NULL)
    {
        test_is_true(false);
        return false;
    }
    timer->AddToScheduler();

    if (iUseProxy)
    {
        //OSCL_TRY(error, terminal = CPV2WayProxyFactory::CreateTerminalL(PV_324M,
        //(MPVCmnCmdStatusObserver *) this,
        //(MPVCmnInfoEventObserver *) this,
        //(MPVCmnErrorEventObserver *) this));
    }
    else
    {
        OSCL_TRY(error, terminal = CPV2WayEngineFactory::CreateTerminalL(PV_324M,
                                   (PVCommandStatusObserver *) this,
                                   (PVInformationalEventObserver *) this,
                                   (PVErrorEventObserver *) this));
    }

    if (error)
    {
        test_is_true(false);
        return false;
    }

    create_sink_source();

    OSCL_TRY(error, terminal->InitL(iSdkInitInfo));
    if (error)
    {
        test_is_true(false);
        if (iUseProxy)
        {
            //CPV2WayProxyFactory::DeleteTerminal(terminal);
        }
        else
        {
            CPV2WayEngineFactory::DeleteTerminal(terminal);
        }
        terminal = NULL;
        return false;
    }

    return true;
}

void av_duplicate_test::start_duplicates_if_ready()
{
    int error = 0;
    if (iAudioSinkAdded && iVideoSinkAdded &&
            iAudioSourceAdded && iVideoSourceAdded &&
            !iDuplicatesStarted)
    {
        iDuplicatesStarted = true;
        OSCL_TRY(error, iAudioAddSource2Id = terminal->AddDataSource(*iAudioSource2));
        if (error)
        {
            test_is_true(false);
            timer->Cancel();
            disconnect();
        }
    }
}
#endif


