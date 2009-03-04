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
#ifndef TEST_PV_MEDIAINPUT_AUTHOR_ENGINE_H
#include "test_pv_mediainput_author_engine.h"
#endif

#ifndef TEST_PV_AUTHOR_ENGINE_TESTSET5_H_INCLUDED
#include "test_pv_author_engine_testset5.h"
#endif

#ifndef TEST_PV_AUTHOR_ENGINE_TESTSET6_H_INCLUDED
#include "test_pv_author_engine_testset6.h"
#endif

#ifndef TEST_PV_AUTHOR_ENGINE_TESTSET7_H_INCLUDED
#include "test_pv_author_engine_testset7.h"
#endif


void PVMediaInputAuthorEngineTest::test()
{
    iTotalSuccess = 0;
    iTotalFail = 0;
    iTotalError = 0;

    PVAECmdType resetState = PVAE_CMD_OPEN;
    while ((iNextTestCase <= iLastTest) || (iNextTestCase < Invalid_Test))
    {
        if (iCurrentTest)
        {
            delete iCurrentTest;
            iCurrentTest = NULL;

            // Shutdown PVLogger and scheduler before checking mem stats
            CleanupLoggerScheduler();
#if !(OSCL_BYPASS_MEMMGT)
            // Print out the memory usage results for this test case
            OsclAuditCB auditCB;
            OsclMemInit(auditCB);
            if (auditCB.pAudit)
            {
                MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
                if (stats)
                {
                    uint32 alloclks = stats->numAllocs - iNumAllocs;
                    fprintf(iFile, "  Mem stats: TotalAllocs(%d), TotalBytes(%d),\n AllocFailures(%d), AllocLeak(%d)\n",
                            stats->totalNumAllocs - iTotalAlloc, stats->totalNumBytes - iTotalBytes, stats->numAllocFails - iAllocFails, stats->numAllocs - iNumAllocs);
                }
                else
                {
                    fprintf(iFile, "Retrieving memory statistics after running test case failed! Memory statistics result is not available.\n");
                }
            }
            else
            {
                fprintf(iFile, "Memory audit not available! Memory statistics result is not available.\n");
            }
#endif

        }   //iCurrentTest

#if !(OSCL_BYPASS_MEMMGT)
        // Obtain the current mem stats before running the test case
        OsclAuditCB auditCB;
        OsclMemInit(auditCB);

        if (auditCB.pAudit)
        {
            MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
            if (stats)
            {
                iTotalAlloc = stats->totalNumAllocs;
                iTotalBytes = stats->totalNumBytes;
                iAllocFails = stats->numAllocFails;
                iNumAllocs = stats->numAllocs;
            }
            else
            {
                fprintf(iFile, "Retrieving memory statistics before running test case failed! Memory statistics result would be invalid.\n");
            }
        }
        else
        {
            fprintf(iFile, "Memory audit not available! Memory statistics result would be invalid.\n");
        }
#endif


        //stop at last test
        if (iNextTestCase > iLastTest)
        {
            iNextTestCase = Invalid_Test;
        }
        else
        {
            fprintf(iFile, "\nStarting Test %d: ", iNextTestCase);
            InitLoggerScheduler();
        }

        PVAuthorAsyncTestParam testparam;
        testparam.iObserver = this;
        testparam.iTestCase = this;
        testparam.iTestCaseNum = iNextTestCase;
        testparam.iStdOut = iFile;

        switch (iNextTestCase)
        {
            case PVMediaInput_Open_Compose_Stop_Test:
            {
                fprintf(iFile, "Open Compose Stop test with AVI/WAV MIO Comp:\n");
                iCurrentTest = new pv_mediainput_async_test_opencomposestop(testparam, iMediaInputParam, false);
                // Go to next test
                ++iNextTestCase;
            }
            break;
            case PVMediaInput_Open_RealTimeCompose_Stop_Test:
            {
                fprintf(iFile, "Open Real Time Compose Stop test with AVI/WAV MIO Comp:\n");
                iMediaInputParam.iRealTimeAuthoring = true;
                iCurrentTest = new pv_mediainput_async_test_opencomposestop(testparam, iMediaInputParam, false);
                // Go to next test
                ++iNextTestCase;
            }
            break;

            case PVMediaInput_Pause_Resume_Test:
            {
                fprintf(iFile, "Pause Resume test with AVI/WAV MIO Comp:\n");
                iCurrentTest = new pv_mediainput_async_test_opencomposestop(testparam, iMediaInputParam, true);
                // Go to next test
                ++iNextTestCase;
            }
            break;
            case PVMediaInput_ErrorHandling_Test_WrongFormat:
            {
                fprintf(iFile, "Error Handling Wrong Format test with AVI/WAV MIO Comp\n");

                iMediaInputParam.iIPFileInfo = FILE_NAME_ERROR_HANDLING;
                iMediaInputParam.iInputFormat = DEFAULTSOURCEFORMATTYPE;
                iCurrentTest = new pv_mediainput_async_test_errorhandling(testparam, iMediaInputParam, false, true);

                // Go to next test
                ++iNextTestCase;
            }
            break;

            case PVMediaInput_ErrorHandling_Test_WrongIPFileName:
            {
                fprintf(iFile, "Error Handling wrong IP File test with AVI/WAV MIO Comp\n");

                iMediaInputParam.iIPFileInfo = WRONGIPFILENAME_ERRORHANDLING;
                iMediaInputParam.iInputFormat = DEFAULTSOURCEFORMATTYPE;
                iCurrentTest = new pv_mediainput_async_test_errorhandling(testparam, iMediaInputParam, false, false);

                // Go to next test
                ++iNextTestCase;
            }
            break;

            case PVMediaInput_Reset_Test:
            {
                fprintf(iFile, "Reset test with AVI/WAV MIO Comp\n");
                switch (resetState)
                {
                    case PVAE_CMD_OPEN:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_OPEN\n");
                    }
                    break;
                    case PVAE_CMD_ADD_DATA_SOURCE:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_ADD_DATA_SOURCE\n");
                    }
                    break;
                    case PVAE_CMD_SELECT_COMPOSER:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_SELECT_COMPOSER\n");
                    }
                    break;
                    case PVAE_CMD_ADD_MEDIA_TRACK:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_ADD_MEDIA_TRACK\n");
                    }
                    break;
                    case PVAE_CMD_INIT:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_INIT\n");
                    }
                    break;
                    case PVAE_CMD_START:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_START\n");
                    }
                    break;
                    case PVAE_CMD_PAUSE:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_PAUSE\n");
                    }
                    break;
                    case PVAE_CMD_RECORDING:
                    {
                        fprintf(iFile, "Current reset state: PVAE_CMD_RECORDING\n");
                    }
                    break;

                    default:
                        break;


                }
                iCurrentTest = new pv_mediainput_async_test_reset(testparam, iMediaInputParam, false, resetState);
                resetState = ((pv_mediainput_async_test_reset*)iCurrentTest)->GetNextResetState();
                //run reset test for all states.
                if (PVAE_CMD_STOP == resetState)
                {
                    // Go to next test
                    ++iNextTestCase;
                }

            }
            break;
            case AVI_Input_Longetivity_Test:
            {
                fprintf(iFile, "AVI Input Longetivity test:\n");
                iCurrentTest = new pv_mediainput_async_test_opencomposestop(testparam, iMediaInputParam, false);
                // Go to next test
                ++iNextTestCase;

            }
            break;
            default:
            {
                iCurrentTest = NULL;
                break;
            }
        }

        if (iCurrentTest)
        {
            // Setup Scheduler
            OsclExecScheduler *sched = OsclExecScheduler::Current();
            if (sched)
            {
                iCurrentTest->StartTest();

#if USE_NATIVE_SCHEDULER
                // Have PV scheduler use the scheduler native to the system
                sched->StartNativeScheduler();
#else
                int32 err;
                OSCL_TRY(err, sched->StartScheduler(););
#endif
            }
            else
            {
                fprintf(iFile, "ERROR! Scheduler is not available. Test case could not run.");
                iNextTestCase++;
            }

        }
        else
        {
            iNextTestCase++;

            if (iNextTestCase < Invalid_Test)
            {
                CleanupLoggerScheduler();
            }
        }

    }//while iNextTest loop
}

void PVMediaInputAuthorEngineTest::CompleteTest(test_case& arTC)
{
    // Print out the result for this test case
    const test_result the_result = arTC.last_result();

    fprintf(iFile, "  Successes %d, Failures %d\n", the_result.success_count() - iTotalSuccess,
            the_result.failures().size() - iTotalFail);

    iTotalSuccess = the_result.success_count();
    iTotalFail = the_result.failures().size();
    iTotalError = the_result.errors().size();

    // Stop the scheduler
    OsclExecScheduler *sched = OsclExecScheduler::Current();
    if (sched)
    {
        sched->StopScheduler();
    }
}

