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
#ifndef SAMPLE_PLAYER_APP_H_INCLUDED
#include "sample_player_app.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OSCL_MEM_AUDIT_H_INCLUDED
#include "oscl_mem_audit.h"
#endif

#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif

#ifndef OSCL_ERROR_PANIC_H_INCLUDED
#include "oscl_error_panic.h"
#endif

#ifndef OSCL_SCHEDULER_H_INCLUDED
#include "oscl_scheduler.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef PVLOGGER_FILE_APPENDER_H_INCLUDED
#include "pvlogger_file_appender.h"
#endif

#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif

#ifndef SAMPLE_PLAYER_APP_CONFIG_H_INCLUDED
#include "sample_player_app_config.h"
#endif

#ifndef OSCL_STRING_UTILS_H_INCLUDED
#include "oscl_string_utils.h"
#endif

#ifndef PV_PLAYER_TEST_MIO_FACTORY_H_INCLUDED
#include "pv_player_test_mio_factory.h"
#endif

#ifndef PV_MEDIA_OUTPUT_NODE_FACTORY_H_INCLUDED
#include "pv_media_output_node_factory.h"
#endif

#ifndef PV_PLAYER_DATASINKPVMFNODE_H_INCLUDED
#include "pv_player_datasinkpvmfnode.h"
#endif

#ifndef DEFAULTSOURCEFILENAME
#error // The default source file needs to be defined in config file
#endif

#ifndef DEFAULTSOURCEFORMATTYPE
#error // The format type for default source file needs to be defined in config file
#endif

int _local_main();

// printing to console
FILE* file = stdout;

char* input_file_name = NULL;

/*!
 *  A sample application to demostrate the normal engine sequence of playing a specified source till end of clip
 *  - Data Source: defaults to "testqvga30fps.mp4"
 *  - Data Sink(s): Video[FileOutputNode-sample_player_app_[SRCFILENAME]_video.dat]
 *                  Audio[FileOutputNode-sample_player_app__[SRCFILENAME]_audio.dat]
 *                  Text[FileOutputNode-sample_player_app__[SRCFILENAME]_text.dat]
 *
 *  - Sequence
 *             -# CreatePlayer()
 *             -# AddDataSource()
 *             -# Init()
 *             -# AddDataSink() (video)
 *             -# AddDataSink() (audio)
 *             -# AddDataSink() (text)
 *             -# GetMetadataValues()
 *             -# Prepare()
 *             -# Start()
 *             -# Play till EOS
 *             -# Stop()
 *             -# RemoveDataSink() (video)
 *             -# RemoveDataSink() (audio)
 *             -# RemoveDataSink() (text)
 *             -# Reset()
 *             -# RemoveDataSource()
 *             -# DeletePlayer()
 *
 */


// Entry point for sample program
int main(int argc, char **argv)
{
    if (argc > 1)
    {
        input_file_name = argv[1];
    }

    fprintf(file, "Sample application for pvPlayer engine:\n");

    //Init Oscl
    OsclBase::Init();
    OsclErrorTrap::Init();
    OsclMem::Init();

    //Run the test under a trap
    int result = -1;
    int32 err;
    TPVErrorPanic panic;

    OSCL_PANIC_TRAP(err, panic, result = _local_main(););

    //Show any exception.
    if (err != 0)
    {
        fprintf(file, "Error!  Leave %d\n", err);
    }
    if (panic.iReason != 0)
    {
        fprintf(file, "Error!  Panic %s %d\n", panic.iCategory.Str(), panic.iReason);
    }

    //Cleanup
#if !(OSCL_BYPASS_MEMMGT)
    //Check for memory leaks before cleaning up OsclMem.
    OsclAuditCB auditCB;
    OsclMemInit(auditCB);
    if (auditCB.pAudit)
    {
        MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
        if (stats)
        {
            fprintf(file, "\nAfter Playback Memory Stats:\n");
            fprintf(file, "  peakNumAllocs %d\n", stats->peakNumAllocs);
            fprintf(file, "  peakNumBytes %d\n", stats->peakNumBytes);
            fprintf(file, "  totalNumAllocs %d\n", stats->totalNumAllocs);
            fprintf(file, "  totalNumBytes %d\n", stats->totalNumBytes);
            fprintf(file, "  numAllocFails %d\n", stats->numAllocFails);
            if (stats->numAllocs)
            {
                fprintf(file, "  ERROR: Memory Leaks! numAllocs %d, numBytes %d\n", stats->numAllocs, stats->numBytes);
            }
        }
        uint32 leaks = auditCB.pAudit->MM_GetNumAllocNodes();
        if (leaks != 0)
        {
            fprintf(file, "ERROR: %d Memory leaks detected!\n", leaks);
            MM_AllocQueryInfo*info = auditCB.pAudit->MM_CreateAllocNodeInfo(leaks);
            uint32 leakinfo = auditCB.pAudit->MM_GetAllocNodeInfo(info, leaks, 0);
            if (leakinfo != leaks)
            {
                fprintf(file, "ERROR: Leak info is incomplete.\n");
            }
            for (uint32 i = 0;i < leakinfo;i++)
            {
                fprintf(file, "Leak Info:\n");
                fprintf(file, "  allocNum %d\n", info[i].allocNum);
                fprintf(file, "  fileName %s\n", info[i].fileName);
                fprintf(file, "  lineNo %d\n", info[i].lineNo);
                fprintf(file, "  size %d\n", info[i].size);
                fprintf(file, "  pMemBlock 0x%x\n", info[i].pMemBlock);
                fprintf(file, "  tag %s\n", info[i].tag);
            }
            auditCB.pAudit->MM_ReleaseAllocNodeInfo(info);
        }
    }
#endif

    OsclMem::Cleanup();
    OsclErrorTrap::Cleanup();
    OsclBase::Cleanup();

    return result;
}

////////

int _local_main()
{
    // default source filename is "test.mp4"
    OSCL_HeapString<OsclMemAllocator> filenameinfo;
    filenameinfo = SOURCENAME_PREPEND_STRING;

    if (input_file_name != NULL)
    {
        filenameinfo += input_file_name;
    }
    else
    {
        filenameinfo += DEFAULTSOURCEFILENAME;
    }

    // default source file format is MP4
    PVMFFormatType inputformattype = DEFAULTSOURCEFORMATTYPE;

    fprintf(file, "  Input file name '%s'\n", filenameinfo.get_cstr());

    // instantiate a player engine
    pvplayer_engine_interface *engine_interface = new pvplayer_engine_interface(filenameinfo.get_str(), inputformattype);
    if (engine_interface)
    {
#if !(OSCL_BYPASS_MEMMGT)
        // Obtain the current mem stats before running the test case
        OsclAuditCB auditCB;
        OsclMemInit(auditCB);
        if (auditCB.pAudit)
        {
            MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
            if (stats)
            {
                fprintf(file, "\nBefore Playback Memory Stats:\n");
                fprintf(file, "  totalNumAllocs %d\n", stats->totalNumAllocs);
                fprintf(file, "  totalNumBytes %d\n", stats->totalNumBytes);
                fprintf(file, "  numAllocFails %d\n", stats->numAllocFails);
                fprintf(file, "  numAllocs %d\n", stats->numAllocs);
            }
            else
            {
                fprintf(file, "Retrieving memory statistics before running test case failed! Memory statistics result would be invalid.\n");
            }
        }
        else
        {
            fprintf(file, "Memory audit not available! Memory statistics result would be invalid.\n");
        }
#endif

        fprintf(file, "\nStarting Playback\n ");

        // Enable the following code for logging
        PVLogger::Init();
        PVLoggerAppender *appender = NULL;
        OsclRefCounter *refCounter = NULL;

        // Log all nodes, for errors only, to the stdout
        appender = new StdErrAppender<TimeAndIdLayout, 1024>();
        OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
            new OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > >(appender);
        refCounter = appenderRefCounter;

        OsclSharedPtr<PVLoggerAppender> appenderPtr(appender, refCounter);

#if 0
        // log everything at DEBUG
        PVLogger *rootnode = PVLogger::GetLoggerObject("");
        rootnode->AddAppender(appenderPtr);
        rootnode->SetLogLevel(PVLOGMSG_DEBUG);
#endif
#if 0
        // log specific components
        PVLogger *node;

        node = PVLogger::GetLoggerObject("PVPlayerEngine");
        node->AddAppender(appenderPtr);
        node->SetLogLevel(PVLOGMSG_DEBUG);

        node = PVLogger::GetLoggerObject("PVMFMP4FFParserNode");
        node->AddAppender(appenderPtr);
        node->SetLogLevel(PVLOGMSG_DEBUG);

        node = PVLogger::GetLoggerObject("PVMFFileOutputNode");
        node->AddAppender(appenderPtr);
        node->SetLogLevel(PVLOGMSG_DEBUG);

        // Log datapath only
        node = PVLogger::GetLoggerObject("datapath");
        node->AddAppender(appenderPtr);
        //info level logs ports & synchronization info.
        node->SetLogLevel(PVLOGMSG_INFO);
#endif

        // Construct and install the active scheduler
        OsclScheduler::Init("PVPlayerEngineScheduler");

        OsclExecScheduler *sched = OsclExecScheduler::Current();
        if (sched)
        {
            // Start playback
            engine_interface->StartPlayback();

            // Start the scheduler
            // control is transfered to the player engine when StartScheduler() is called
            // control is regained when StopScheduler() is called
            fprintf(file, "\nStarting Scheduler\n ");
#if USE_NATIVE_SCHEDULER
            // Have PV scheduler use the scheduler native to the system
            sched->StartNativeScheduler();
#else
            // Have PV scheduler use its own implementation of the scheduler
            sched->StartScheduler();
#endif
        }
        else
        {
            fprintf(file, "ERROR! Scheduler is not available. Test case could not run.");
        }

        // when playback completes, StopScheduler is called
        // and control is returned to this location

        // Shutdown PVLogger and scheduler before checking mem stats
        OsclScheduler::Cleanup();
        PVLogger::Cleanup();

        // done with playback, free resources
        OSCL_DELETE(engine_interface);
        engine_interface = NULL;

        return 0;
    }
    else
    {
        fprintf(file, "ERROR! pvplayer_engine_interface could not be instantiated.\n");
        return 1;
    }
}


////////

pvplayer_engine_interface::pvplayer_engine_interface(char *aFileName, PVMFFormatType aFileType) : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVPlayerEngine"),
        iState(STATE_UNKNOWN),
        iPlayer(NULL),
        iDataSource(NULL),
        iDataSinkVideo(NULL),
        iDataSinkAudio(NULL),
        iDataSinkText(NULL),
        iCurrentCmdId(0),
        iFileName(NULL),
        iFileType(0),
        iContextObjectRefValue(0),
        iContextObject(0),
        iNumValues(0),
        iClipDuration(0)
{
    iFileName = aFileName;
    iFileType = aFileType;

    // Initialize the variables with some random number to use for context data testing
    iContextObject = iContextObjectRefValue = 0x5C7A;

    PVPlayerTestMioFactory::DeviceInit(this);
}


////////

pvplayer_engine_interface::~pvplayer_engine_interface()
{
    PVPlayerTestMioFactory::DeviceDeInit(this);
}

////////
void pvplayer_engine_interface::StartPlayback()
{
    // add this timer object to the scheduler
    AddToScheduler();
    // use iState to drive the playback for this simple app
    // a command queuing mechanism may be desired
    // where by the end user requests, such as pause playback or
    // resume playback, are put on the queue in a separate thread
    // and are dequeued by Run()
    iState = STATE_CREATE;
    // schedule this timer object to run immediately
    RunIfNotReady();
}

////////

// this is called by the PV scheduler
void pvplayer_engine_interface::Run()
{
    int error = 0;

    // in this simple app, state transition is used to drive the engine
    // in proper deployment, user requests may be used instead to drive engine
    // end user requests may be queued by in a separate thread and dequeued here
    switch (iState)
    {
        case STATE_CREATE:
        {
            iPlayer = NULL;

            OSCL_TRY(error, iPlayer = PVPlayerFactory::CreatePlayer(this, this, this));
            if (error)
            {
                // clean up
                fprintf(file, "\nStopping Scheduler\n");

                // Stop the scheduler
                OsclExecScheduler *sched = OsclExecScheduler::Current();
                if (sched)
                {
                    sched->StopScheduler();
                }
            }
            else
            {
                // player created successfully
                // next step is to add the data source
                iState = STATE_ADDDATASOURCE;
                // schedule to run immediately
                RunIfNotReady();
            }
        }
        break;

        case STATE_ADDDATASOURCE:
        {
            // create a datasource
            iDataSource = new PVPlayerDataSourceURL;

            // convert to wchar
            oscl_UTF8ToUnicode(iFileName, oscl_strlen(iFileName), output, 512);
            wFileName.set(output, oscl_strlen(output));

            iDataSource->SetDataSourceURL(wFileName);
            iDataSource->SetDataSourceFormatType(iFileType);

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }

        break;

        case STATE_INIT:
        {
            // initialize the player
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Init((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_VIDEO:
        {
            // create video sink
            // use a file output sink for now
            // output format is defined in config

            OSCL_wHeapString<OsclMemAllocator> SinkFileName;
            SinkFileName = OUTPUTNAME_PREPEND_WSTRING;
            SinkFileName += _STRLIT_WCHAR("sample_player_app_");
            OSCL_wHeapString<OsclMemAllocator> inputfilename;
            RetrieveFilename(wFileName.get_str(), inputfilename);
            SinkFileName += inputfilename;
            SinkFileName += _STRLIT_WCHAR("_video.dat");

            iMIOFileOutVideo = PVPlayerTestMioFactory::CreateVideoOutput
                               ((OsclAny*) & SinkFileName,
                                NULL,  // aObserver
                                true,  // aActiveTiming
                                50,    // aQueueLimit
                                false, // aSimFlowControl
                                false  // logStrings
                               );
            iIONodeVideo = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutVideo);
            iDataSinkVideo = new PVPlayerDataSinkPVMFNode;

            ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkNode(iIONodeVideo);
            ((PVPlayerDataSinkPVMFNode*)iDataSinkVideo)->SetDataSinkFormatType(VIDEOSINK_FORMAT_TYPE);

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_AUDIO:
        {
            // create audio sink
            // use a file output sink for now
            // default output format is PCM16

            OSCL_wHeapString<OsclMemAllocator> SinkFileName;
            SinkFileName = OUTPUTNAME_PREPEND_WSTRING;
            SinkFileName += _STRLIT_WCHAR("sample_player_app_");
            OSCL_wHeapString<OsclMemAllocator> inputfilename;
            RetrieveFilename(wFileName.get_str(), inputfilename);
            SinkFileName += inputfilename;
            SinkFileName += _STRLIT_WCHAR("_audio.dat");

            iMIOFileOutAudio = PVPlayerTestMioFactory::CreateAudioOutput
                               ((OsclAny*) & SinkFileName,
                                NULL,  // aObserver
                                true,  // aActiveTiming
                                50,    // aQueueLimit
                                false, // aSimFlowControl
                                false  // logStrings
                               );

            iIONodeAudio = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutAudio);
            iDataSinkAudio = new PVPlayerDataSinkPVMFNode;

            ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkNode(iIONodeAudio);

            ((PVPlayerDataSinkPVMFNode*)iDataSinkAudio)->SetDataSinkFormatType(AUDIOSINK_FORMAT_TYPE);

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_ADDDATASINK_TEXT:
        {
            // create text sink
            // use a file output sink for now
            // default output format is 3GPP timed text

            OSCL_wHeapString<OsclMemAllocator> SinkFileName;
            SinkFileName = OUTPUTNAME_PREPEND_WSTRING;
            SinkFileName += _STRLIT_WCHAR("sample_player_app_");
            OSCL_wHeapString<OsclMemAllocator> inputfilename;
            RetrieveFilename(wFileName.get_str(), inputfilename);
            SinkFileName += inputfilename;
            SinkFileName += _STRLIT_WCHAR("_text.dat");

            iMIOFileOutText = PVPlayerTestMioFactory::CreateTextOutput((OsclAny*) & SinkFileName); // simple form
            iIONodeText = PVMediaOutputNodeFactory::CreateMediaOutputNode(iMIOFileOutText);
            iDataSinkText = new PVPlayerDataSinkPVMFNode;

            ((PVPlayerDataSinkPVMFNode*)iDataSinkText)->SetDataSinkNode(iIONodeText);
            ((PVPlayerDataSinkPVMFNode*)iDataSinkText)->SetDataSinkFormatType(PVMF_3GPP_TIMEDTEXT);

            OSCL_TRY(error, iCurrentCmdId = iPlayer->AddDataSink(*iDataSinkText, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_GETMETADATAVALUES:
        {
            // find out the source clip duration
            // the duration is retrieved from the metadata
            iMetadataKeyList.push_back(OSCL_HeapString<OsclMemAllocator>("duration"));
            iMetadataValueList.clear();
            iNumValues = 0;
            OSCL_TRY(error, iCurrentCmdId = iPlayer->GetMetadataValues(iMetadataKeyList, 0,  -1, iNumValues, iMetadataValueList, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_PREPARE:
        {
            // prepare the player
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Prepare((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_START:
        {
            // start playback from the beginning of the clip
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Start((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_EOSNOTREACHED:
        {
            // stop the playback
            iState = STATE_STOP;
            RunIfNotReady();
        }
        break;

        case STATE_STOP:
        {
            // stop the player
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Stop((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_VIDEO:
        {
            // remove the video sink, which is a currently a file output sink
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkVideo, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_AUDIO:
        {
            // remove the audio sink, which is a currently a file output sink
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkAudio, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASINK_TEXT:
        {
            // remove the text sink, which is a currently a file output sink
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSink(*iDataSinkText, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_RESET:
        {
            // reset the player
            OSCL_TRY(error, iCurrentCmdId = iPlayer->Reset((OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_REMOVEDATASOURCE:
        {
            // remove the data souce
            OSCL_TRY(error, iCurrentCmdId = iPlayer->RemoveDataSource(*iDataSource, (OsclAny*) & iContextObject));
            OSCL_FIRST_CATCH_ANY(error, iState = STATE_CLEANUPANDCOMPLETE; RunIfNotReady());
        }
        break;

        case STATE_CLEANUPANDCOMPLETE:
        {
            // free up all resources
            PVPlayerFactory::DeletePlayer(iPlayer);
            iPlayer = NULL;

            OSCL_DELETE(iDataSource);
            iDataSource = NULL;

            OSCL_DELETE(iDataSinkVideo);
            iDataSinkVideo = NULL;

            OSCL_DELETE(iDataSinkAudio);
            iDataSinkAudio = NULL;

            OSCL_DELETE(iDataSinkText);
            iDataSinkText = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeVideo);
            iIONodeVideo = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeAudio);
            iIONodeAudio = NULL;

            PVMediaOutputNodeFactory::DeleteMediaOutputNode(iIONodeText);
            iIONodeText = NULL;

            PVPlayerTestMioFactory::DestroyVideoOutput(iMIOFileOutVideo);
            iMIOFileOutVideo = NULL;

            PVPlayerTestMioFactory::DestroyAudioOutput(iMIOFileOutAudio);
            iMIOFileOutAudio = NULL;

            PVPlayerTestMioFactory::DestroyTextOutput(iMIOFileOutText);
            iMIOFileOutText = NULL;

            fprintf(file, "\nStopping Scheduler\n");

            // Stop the scheduler
            OsclExecScheduler *sched = OsclExecScheduler::Current();
            if (sched)
            {
                sched->StopScheduler();
            }
        }
        break;

        default:
            break;

    }
}

////////

// callback by PV engine when a request has been completed
void pvplayer_engine_interface::CommandCompleted(const PVCmdResponse& aResponse)
{
    if (aResponse.GetCmdId() != iCurrentCmdId)
    {
        // Wrong command ID.
        fprintf(file, "\nCommandCompleted Mismatched Id\n");
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
                iState = STATE_CLEANUPANDCOMPLETE;
                RunIfNotReady();
                return;
            }
        }
        else
        {
            // Context data pointer was corrupted
            iState = STATE_CLEANUPANDCOMPLETE;
            RunIfNotReady();
            return;
        }
    }

    if (aResponse.GetCmdStatus() != PVMFSuccess)
    {
        // Treat as unrecoverable error
        iState = STATE_CLEANUPANDCOMPLETE;
        RunIfNotReady();
        return;
    }

    bool bWait = false;

    switch (iState)
    {
        case STATE_ADDDATASOURCE:
            // Data source was added
            // Initialize the player
            iState = STATE_INIT;
            break;

        case STATE_INIT:
            // Player was initialized
            // Add video sink
            iState = STATE_ADDDATASINK_VIDEO;
            break;

        case STATE_ADDDATASINK_VIDEO:
            // Video sink was added
            // Add audio sink
            iState = STATE_ADDDATASINK_AUDIO;
            break;

        case STATE_ADDDATASINK_AUDIO:
            // Audio sink was added
            // Add text sink
            iState = STATE_ADDDATASINK_TEXT;
            break;

        case STATE_ADDDATASINK_TEXT:
            // Text sink was added
            // Retrieve metadata values (clip duration in particular)
            iState = STATE_GETMETADATAVALUES;
            break;

        case STATE_GETMETADATAVALUES:
            // If duration is available, get it. Default is 10 sec
            iClipDuration = 10000;
            if (iMetadataValueList.empty() == false)
            {
                for (uint32 i = 0; i < iMetadataValueList.size(); ++i)
                {
                    // Search for timescale and duration info
                    char* substr = oscl_strstr(iMetadataValueList[i].key, _STRLIT_CHAR("duration;valtype=uint32;timescale="));
                    if (substr != NULL)
                    {
                        uint32 timescale = 1000;
                        if (PV_atoi((substr + 34), 'd', timescale) == false)
                        {
                            // No timescale info, default to 1000
                            timescale = 1000;
                        }
                        uint32 duration = iMetadataValueList[i].value.uint32_value;
                        if (duration > 0 && timescale > 0)
                        {
                            // Save the clip duration in milliseconds
                            iClipDuration = ((duration * 1000) / timescale);
                        }
                    }
                }
            }

            fprintf(file, " clip duration %d\n ", iClipDuration);

            // Prepare for playback
            iState = STATE_PREPARE;
            break;

        case STATE_PREPARE:
            // Prepare succeeded
            // Start playback
            iState = STATE_START;
            break;

        case STATE_START:
            // Wait until EOS
            iState = STATE_EOSNOTREACHED;
            bWait = true;
            break;

        case STATE_STOP:
            // Engine stopped
            // Remove the video sink
            iState = STATE_REMOVEDATASINK_VIDEO;
            break;

        case STATE_REMOVEDATASINK_VIDEO:
            // Video sink removed
            // Remove audio sink
            iState = STATE_REMOVEDATASINK_AUDIO;
            break;

        case STATE_REMOVEDATASINK_AUDIO:
            // Audio sink removed
            // Remove text sink
            iState = STATE_REMOVEDATASINK_TEXT;
            break;

        case STATE_REMOVEDATASINK_TEXT:
            // Text sink removed
            // Reset engine
            iState = STATE_RESET;
            break;

        case STATE_RESET:
            // Engine reset
            // Remove data source
            iState = STATE_REMOVEDATASOURCE;
            break;

        case STATE_REMOVEDATASOURCE:
            // Data source removed
            // Free resouces
            iState = STATE_CLEANUPANDCOMPLETE;
            break;

        default:
            // engine error if this is reached
            fprintf(file, "\nCommandCompleted unknown state %d\n ", iState);
            iState = STATE_CLEANUPANDCOMPLETE;
            break;
    } // end switch

    if (bWait == false)
    {
        // schedule this object to run immediately
        RunIfNotReady();
    }

}


////////

// callback by PV engine when an error is encountered
void pvplayer_engine_interface::HandleErrorEvent(const PVAsyncErrorEvent& aEvent)
{
    switch (aEvent.GetEventType())
    {
        case PVMFErrResourceConfiguration:
        case PVMFErrResource:
        case PVMFErrCorrupt:
        case PVMFErrProcessing:
            // Just log the error for now
            fprintf(file, "\nHandleErrorEvent event type %d\n ", aEvent.GetEventType());
            break;

        default:
            // Unknown error and just log the error
            fprintf(file, "\nHandleErrorEvent unknown event type %d\n ", aEvent.GetEventType());
            break;
    }
}


////////

// callback by PV engine when information is available
void pvplayer_engine_interface::HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent)
{
    PVInterface* iface = (PVInterface*)(aEvent.GetEventExtensionInterface());

    // Check for end of clip event and clip position event
    if ((aEvent.GetEventType() == PVMFInfoEndOfData) || (aEvent.GetEventType() == PVMFInfoPositionStatus))
    {
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

            if (infouuid == PVPlayerErrorInfoEventTypesUUID)
            {
                if (infocode == PVPlayerInfoEndOfClipReached)
                {
                    fprintf(file, "\nEnd of Clip Reached\n ");

                    // Playback has reached end of clip
                    // Stop the player
                    iState = STATE_STOP;
                    Cancel();
                    RunIfNotReady();
                }
                else if (infocode == PVPlayerInfoPlaybackPositionStatus)
                {
                    // position update
                    // retrieve and display the media time
                    uint32 aPos = 0;

                    uint8* localbuf = aEvent.GetLocalBuffer();
                    if (aEvent.GetLocalBufferSize() == 8 && localbuf[0] == 1)
                    {
                        oscl_memcpy(&aPos, &localbuf[4], sizeof(uint32));
                    }

                    fprintf(file, " media time %d\r ", aPos);
                    fflush(file);
                }
            }
        }
    }
}


