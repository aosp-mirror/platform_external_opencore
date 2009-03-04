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
#define TEST_PV_AUTHOR_ENGINE_TESTSET3_H_INCLUDED

#ifndef PVAETEST_H_INCLUDED
#include "pvaetest.h"
#endif

#ifndef PVMI_CONFIG_AND_CAPABILITY_OBSERVER_H_INCLUDED
#include "pvmi_config_and_capability_observer.h"
#endif
#include "pvmi_fileio_kvp.h"

#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif

#ifndef TEST_PV_AUTHOR_ENGINE_TYPEDEFS_H
#include "test_pv_author_engine_typedefs.h"
#endif

#define PVPATB_TEST_IS_TRUE( condition ) (iTestCase->test_is_true_stub( (condition), (#condition), __FILE__, __LINE__ ))

class pvauthor_async_test_capconfig: public pvauthor_async_test_base,
            public PvmiConfigAndCapabilityCmdObserver

{
    public:
        pvauthor_async_test_capconfig(PVAuthorAsyncTestParam aTestParam, const char* aInputFileNameAudio,
                                      const char* aInputFileNameVideo, const char* aOutputFileName,
                                      PVAETestInputType aAudioInputType, PVAETestInputType aVideoInputType,
                                      const char* aComposerMimeType,
                                      const char* aAudioEncoderMimeType, const char* aVideoEncoderMimeType,
                                      AVTConfig aAVTConfig, bool aPauseResumeEnable)

                : pvauthor_async_test_base(aTestParam)

                , iComposerMimeType(aComposerMimeType)
                , iAudioEncoderMimeType(aAudioEncoderMimeType)
                , iVideoEncoderMimeType(aVideoEncoderMimeType)
                , iAudioInputType(aAudioInputType)
                , iVideoInputType(aVideoInputType)
                , iAuthor(NULL)
                , iComposer(NULL)
                , iComposerConfig(NULL)
                , iAudioEncoderConfig(NULL)
                , iVideoEncoderConfig(NULL)
                , iOutputSizeAndDurationConfig(NULL)
                , iAVTConfig(aAVTConfig)
                , bAudioTrack(true)
                , iPauseResumeEnable(aPauseResumeEnable)

        {
            iLogger = PVLogger::GetLoggerObject("pvauthor_async_test_capconfig");
            oscl_UTF8ToUnicode(aOutputFileName, oscl_strlen(aOutputFileName), iTempOutputFileName, ARRAY_SIZE);

            iInputFileNameAudio = NULL;
            iInputFileNameVideo = NULL;
            iOutputFileName = NULL;

            if (oscl_strlen(aInputFileNameAudio) != 0)
            {
                oscl_wchar output1[ARRAY_SIZE];
                oscl_UTF8ToUnicode(aInputFileNameAudio, oscl_strlen(aInputFileNameAudio), output1, ARRAY_SIZE);
                iInputFileNameAudio.set(output1, oscl_strlen(output1));
            }

            if (oscl_strlen(aInputFileNameVideo) != 0)
            {
                oscl_wchar output2[ARRAY_SIZE];
                oscl_UTF8ToUnicode(aInputFileNameVideo, oscl_strlen(aInputFileNameVideo), output2, ARRAY_SIZE);
                iInputFileNameVideo.set(output2, oscl_strlen(output2));
            }

            if (oscl_strlen(aOutputFileName) != 0)
            {
                oscl_wchar output3[ARRAY_SIZE];
                oscl_UTF8ToUnicode(aOutputFileName, oscl_strlen(aOutputFileName), output3, ARRAY_SIZE);
                iOutputFileName.set(output3, oscl_strlen(output3));
            }
            iRemoveDataSourceDone = 0;
        }

        ~pvauthor_async_test_capconfig()
        {
            Cleanup();
        }
        void Run();
        void StartTest();

        // Author engine observer functions
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);
        void CommandCompleted(const PVCmdResponse& aResponse);

        PVAECmdType iState;
        // Test output
        OSCL_wHeapString<OsclMemAllocator> iOutputFileName;
        OSCL_wHeapString<OsclMemAllocator> iInputFileNameAudio;
        OSCL_wHeapString<OsclMemAllocator> iInputFileNameVideo;

        OSCL_HeapString<OsclMemAllocator> iComposerMimeType;
        OSCL_HeapString<OsclMemAllocator> iAudioEncoderMimeType;
        OSCL_HeapString<OsclMemAllocator> iVideoEncoderMimeType;
        // Test inputs
        PVAETestInputType iAudioInputType;
        PVAETestInputType iVideoInputType;

    private:
        // Methods to create test input nodes and add to author engine
        //bool CreateTestInputs();
        int CreateAudioInput();
        int CreateVideoInput();
        int CreateTextInput();
        bool AddDataSource(PVAETestInput& aInput);

        // Methods to configure composers
        void SelectComposer();
        bool ConfigComposer();
        bool ConfigAmrAacComposer();
        bool ConfigMp43gpComposer();

        // Method to configure max filesize/duration interface
        bool QueryComposerOutputInterface();
        bool ConfigComposerOutput();

        // Methods to add media tracks
        bool AddAudioMediaTrack();
        bool AddVideoMediaTrack();

        // Methods to configure encoders
        bool ConfigureVideoEncoder();
        bool ConfigureAudioEncoder();

        bool DeleteTestInputs();

        void ResetAuthorConfig();

        void Cleanup();
        //CapConfig
        bool CapConfigSync();
        bool CapConfigAsync();
        // From PvmiConfigAndCapabilityCmdObserver
        void SignalEvent(int32 req_id);
        /////////////////////////////////////////////
        // Author engine related variables
        PVAuthorEngineInterface* iAuthor;
        OsclAny* iComposer;
        PVInterface* iComposerConfig;
        PVInterface* iAudioEncoderConfig;
        PVInterface* iVideoEncoderConfig;
        PVInterface* iOutputSizeAndDurationConfig;

        // Meta data strings
        OSCL_wHeapString<OsclMemAllocator> iVersionString;
        OSCL_wHeapString<OsclMemAllocator> iTitleString;
        OSCL_wHeapString<OsclMemAllocator> iAuthorString;
        OSCL_wHeapString<OsclMemAllocator> iCopyrightString;
        OSCL_wHeapString<OsclMemAllocator> iDescriptionString;
        OSCL_wHeapString<OsclMemAllocator> iRatingString;

        AVTConfig iAVTConfig;
        Oscl_Vector<PVAETestInput, OsclMemAllocator> iTestInputs;
        oscl_wchar iTempOutputFileName[ARRAY_SIZE];

        PVLogger* iLogger;
        bool bAudioTrack; //keep track if audio track has been added
        bool iPauseResumeEnable;
        uint iRemoveDataSourceDone;//to keep track of how many removeDataSource calls done
        //For CapConfig
        PvmiCapabilityAndConfig* iAuthorCapConfigIF;
        PvmiKvp iKVPSetAsync;
        OSCL_StackString<64> iKeyStringSetAsync;
        PvmiKvp* iErrorKVP;
};

#endif
