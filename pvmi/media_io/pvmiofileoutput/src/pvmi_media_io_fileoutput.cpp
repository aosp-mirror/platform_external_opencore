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
#include "pvmi_media_io_fileoutput.h"
#include "pvlogger.h"
#include "pv_mime_string_utils.h"
#include "oscl_snprintf.h"
#include "pvmf_timedtext.h"
#include "oscl_file_io.h"

#include "oscl_dll.h"

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

//The factory functions.
#include "pvmi_media_io_fileoutput_registry_factory.h"
#include "oscl_mem.h"

OSCL_EXPORT_REF PvmiMIOControl* PVMFMediaFileOutputRegistryFactory::CreateMediaIO(OsclAny* aParam)
{
    PVRefFileOutput* ptr = OSCL_NEW

                           (PVRefFileOutput, ((oscl_wchar*)aParam));
    return ptr;
}

OSCL_EXPORT_REF void PVMFMediaFileOutputRegistryFactory::ReleaseMediaIO(PvmiMIOControl* aNode)
{
    OSCL_DELETE(aNode);
}

// This class implements the reference media IO for file output
// This class constitutes the Media IO component


OSCL_EXPORT_REF PVRefFileOutput::PVRefFileOutput(const OSCL_wString& aFileName, bool logStrings)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "pvreffileoutput")
        , iOutputFileName(aFileName)
{
    initData();
    iLogStrings = logStrings;
}


OSCL_EXPORT_REF PVRefFileOutput::PVRefFileOutput(const OSCL_wString& aFileName
        , PVRefFileOutputTestObserver*aTestObs
        , bool aSimTiming, uint32 aQueueLimit
        , bool aSimFlowControl
        , bool logStrings)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "pvreffileoutput")
        , iOutputFileName(aFileName)
{
    initData();
    //test features...
    iSimFlowControl = aSimFlowControl;
    iTestObserver = aTestObs;
    iActiveTiming = NULL;
    if (aSimTiming)
    {
        OsclMemAllocator alloc;
        OsclAny*ptr = alloc.allocate(sizeof(PVRefFileOutputActiveTimingSupport));
        if (ptr)
        {
            iActiveTiming = OSCL_PLACEMENT_NEW(ptr, PVRefFileOutputActiveTimingSupport(aQueueLimit));
        }
    }
    iLogStrings = logStrings;
    iParametersLogged = false;
}

OSCL_EXPORT_REF PVRefFileOutput::PVRefFileOutput(const oscl_wchar* aFileName,
        bool aActiveTiming)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "pvreffileoutput")
        , iOutputFileName(aFileName)
{
    initData();
    iActiveTiming = NULL;
    if (aActiveTiming)
    {
        OsclMemAllocator alloc;
        OsclAny*ptr = alloc.allocate(sizeof(PVRefFileOutputActiveTimingSupport));
        if (ptr)
        {
            iActiveTiming = OSCL_PLACEMENT_NEW(ptr, PVRefFileOutputActiveTimingSupport(10));
        }
    }
}

void PVRefFileOutput::initData()
{
    iAudioFormat = PVMF_FORMAT_UNKNOWN;
    iAudioNumChannelsValid = false;
    iAudioSamplingRateValid = false;

    iVideoFormat = PVMF_FORMAT_UNKNOWN;
    iVideoHeightValid = false;
    iVideoWidthValid = false;
    iVideoDisplayHeightValid = false;
    iVideoDisplayWidthValid = false;

    iCommandCounter = 0;
    iLogger = NULL;
    iFileOpened = false;
    iFsConnected = false;
    iCommandResponseQueue.reserve(5);
    iWriteResponseQueue.reserve(5);
    iObserver = NULL;
    iLogger = NULL;
    iPeer = NULL;
    iState = STATE_IDLE;
    iWriteBusy = false;
    iWriteBusySeqNum = 0;

    //test features...
    iSimFlowControl = false;
    iTestObserver = NULL;
    iActiveTiming = NULL;
    iLogStrings = false;
    iParametersLogged = false;
    iFormatMask = 0;
    iTextFormat = PVMF_FORMAT_UNKNOWN;
#if PVFILEOUTPUT_CLOCK_EXTN_SUPPORTED
    iUseClockExtension = true;
#else
    iUseClockExtension = false;
#endif
    iRIFFChunk.chunkID = FOURCC_RIFF;//0x46464952;   //"RIFF" in ASCII form, big-endian form
    iRIFFChunk.chunkSize = 0;
    iRIFFChunk.format  = FOURCC_WAVE;//0x45564157;   //"WAVE" in ASCII form, big-endian form

    iFmtSubchunk.subchunk1ID = FOURCC_fmt;//0x20746d66;   //"fmt " in ASCII form, big-endian form
    iFmtSubchunk.subchunk1Size = 16;         //for PCM16
    iFmtSubchunk.audioFormat   = 1;          //PCM = 1
    iFmtSubchunk.numChannels   = 0;
    iFmtSubchunk.sampleRate    = 0;
    iFmtSubchunk.byteRate      = 0;
    iFmtSubchunk.blockAlign    = 0;
    iFmtSubchunk.bitsPerSample = 16;

    iDataSubchunk.subchunk2ID  = FOURCC_data;//0x61746164;  //"data" in ASCII form, big-endian form
    iDataSubchunk.subchunk2Size = 0;

    iHeaderWritten = false;
    iAudioFormatType = 0;
    iVideoFormatType = 0;
    iInitializeAVIDone = false;
    iAVIChunkSize = 0;
    iVideoLastTimeStamp = 0;
    iVideoCount = 0;
}

void PVRefFileOutput::ResetData()
//reset all data from this session.
{
    if (iAudioFormatType == PVMF_PCM16 || iAudioFormatType == PVMF_PCM8)
    {
        UpdateWaveChunkSize();
    }

    if (iVideoFormatType == PVMF_YUV420)
    {
        UpdateVideoChunkHeaderIdx();
    }
    Cleanup();

    //reset all the received media parameters.

    iAudioFormatString = "";
    iAudioFormat = PVMF_FORMAT_UNKNOWN;
    iAudioNumChannelsValid = false;
    iAudioSamplingRateValid = false;

    iVideoFormatString = "";
    iVideoFormat = PVMF_FORMAT_UNKNOWN;
    iVideoHeightValid = false;
    iVideoWidthValid = false;
    iVideoDisplayHeightValid = false;
    iVideoDisplayWidthValid = false;

    iTextFormatString = "";
    iTextFormat = PVMF_FORMAT_UNKNOWN;

    iParametersLogged = false;
}

void PVRefFileOutput::Cleanup()
//cleanup all allocated memory and release resources.
{
    if (iFileOpened)
    {
        iOutputFile.Flush();
        iOutputFile.Close();
    }
    iFileOpened = false;
    if (iFsConnected)
    {
        iFs.Close();
    }
    iFsConnected = false;

    while (!iCommandResponseQueue.empty())
    {
        if (iObserver)
        {
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId, iCommandResponseQueue[0].iContext, iCommandResponseQueue[0].iStatus));
        }
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }
    while (!iWriteResponseQueue.empty())
    {
        if (iPeer)
        {
            iPeer->writeComplete(iWriteResponseQueue[0].iStatus, iWriteResponseQueue[0].iCmdId, (OsclAny*)iWriteResponseQueue[0].iContext);
        }
        iWriteResponseQueue.erase(&iWriteResponseQueue[0]);
    }
}

PVRefFileOutput::~PVRefFileOutput()
{
    Cleanup();

    if (iActiveTiming)
    {
        iActiveTiming->~PVRefFileOutputActiveTimingSupport();
        OsclMemAllocator alloc;
        alloc.deallocate(iActiveTiming);
        iActiveTiming = NULL;
    }
}


PVMFStatus PVRefFileOutput::connect(PvmiMIOSession& aSession, PvmiMIOObserver* aObserver)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::connect() called"));
    // Each Session could have its own set of Comfiguration parametres
    //in an array of structures and the session ID could be an index to that array.

    OSCL_UNUSED_ARG(aSession);
    //currently supports only one session
    if (iObserver)
    {
        return PVMFFailure;
    }

    iObserver = aObserver;
    return PVMFSuccess;
}


PVMFStatus PVRefFileOutput::disconnect(PvmiMIOSession aSession)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::disconnect() called"));
    OSCL_UNUSED_ARG(aSession);
    //currently supports only one session

    while (!iCommandResponseQueue.empty())
    {
        if (iObserver)
        {
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId, iCommandResponseQueue[0].iContext, iCommandResponseQueue[0].iStatus));
        }
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }
    while (!iWriteResponseQueue.empty())
    {
        if (iPeer)
        {
            iPeer->writeComplete(iWriteResponseQueue[0].iStatus, iWriteResponseQueue[0].iCmdId, (OsclAny*)iWriteResponseQueue[0].iContext);
        }
        iWriteResponseQueue.erase(&iWriteResponseQueue[0]);
    }

    iObserver = NULL;
    return PVMFSuccess;
}


PvmiMediaTransfer* PVRefFileOutput::createMediaTransfer(PvmiMIOSession& aSession,
        PvmiKvp* read_formats, int32 read_flags,
        PvmiKvp* write_formats, int32 write_flags)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::createMediaTransfer() called"));

    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(read_formats);
    OSCL_UNUSED_ARG(read_flags);
    OSCL_UNUSED_ARG(write_formats);
    OSCL_UNUSED_ARG(write_flags);

    return (PvmiMediaTransfer*)this;
}

void PVRefFileOutput::QueueCommandResponse(CommandResponse& aResp)
{
    //queue a command response and schedule processing.

    iCommandResponseQueue.push_back(aResp);

    //cancel any timer delay so the command response will happen ASAP.
    if (IsBusy())
    {
        Cancel();
    }

    RunIfNotReady();
}

PVMFCommandId PVRefFileOutput::QueryUUID(const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::QueryUUID() called"));

    OSCL_UNUSED_ARG(aMimeType);
    OSCL_UNUSED_ARG(aExactUuidsOnly);

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;
    int32 err ;
    OSCL_TRY(err,
             aUuids.push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID);
             if (iActiveTiming)
{
    PVUuid uuid;
    iActiveTiming->queryUuid(uuid);
        aUuids.push_back(uuid);
    }
            );
    if (err == OsclErrNone)
    {
        status = PVMFSuccess;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


PVMFCommandId PVRefFileOutput::QueryInterface(const PVUuid& aUuid, PVInterface*& aInterfacePtr, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::QueryInterface() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;
    if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
        status = PVMFSuccess;
    }
    else if (aUuid == PvmiClockExtensionInterfaceUuid)
    {
        //the clock extension interface is present only when the component
        //has active timing.
        if (iActiveTiming)
        {
            PvmiClockExtensionInterface* myInterface = OSCL_STATIC_CAST(PvmiClockExtensionInterface*, iActiveTiming);
            aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
            status = PVMFSuccess;
        }
        else if (iUseClockExtension == true)
        {
            PvmiClockExtensionInterface* myInterface = OSCL_STATIC_CAST(PvmiClockExtensionInterface*, this);
            aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
            status = PVMFSuccess;
        }
        else
        {
            status = PVMFFailure;
        }
    }
    else
    {
        status = PVMFFailure;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


void PVRefFileOutput::deleteMediaTransfer(PvmiMIOSession& aSession, PvmiMediaTransfer* media_transfer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::deleteMediaTransfer() called"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(media_transfer);

    if (iWriteResponseQueue.empty() == false)
    {
        // All media transfer requests are not completed yet. Do a leave
        OSCL_LEAVE(OsclErrBusy);
        // return;	This statement was removed to avoid compiler warning for Unreachable Code
    }

    if (iPeer)
    {
        // Since media transfer is gone, peer is gone as well
        iPeer = NULL;
    }
}


PVMFCommandId PVRefFileOutput:: Init(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::Init() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;

    switch (iState)
    {
        case STATE_LOGGED_ON:
            if (!iFileOpened)
            {
                if (iOutputFile.Open(iOutputFileName.get_cstr(), Oscl_File::MODE_READWRITE | Oscl_File::MODE_BINARY, iFs) != 0)
                {
                    status = PVMFFailure;
                }
                else
                {
                    status = PVMFSuccess;
                    iFileOpened = true;
                }
            }
            else
            {
                status = PVMFSuccess;
                iFileOpened = true;
            }
            if (status == PVMFSuccess)
            {
                iState = STATE_INITIALIZED;
            }
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::Reset(const OsclAny* aContext)
{
    // Reset all data from this session
    ResetData();

    // TEMP to properly behave asynchronously
    PVMFCommandId cmdid = iCommandCounter++;
    CommandResponse resp(PVMFSuccess, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::Start(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::Start() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;

    switch (iState)
    {
        case STATE_INITIALIZED:
        case STATE_PAUSED:
            iState = STATE_STARTED;
            status = PVMFSuccess;
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


PVMFCommandId PVRefFileOutput::Pause(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::Pause() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;

    switch (iState)
    {
        case STATE_STARTED:
        case STATE_INITIALIZED:
        case STATE_PAUSED:
            iState = STATE_PAUSED;
            status = PVMFSuccess;
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}


PVMFCommandId PVRefFileOutput::Flush(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::Flush() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;

    switch (iState)
    {
        case STATE_STARTED:
            iOutputFile.Flush();
            iState = STATE_INITIALIZED;
            status = PVMFSuccess;
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::DiscardData(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::DiscardData() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    //this component doesn't buffer data, so there's nothing
    //needed here.

    PVMFStatus status = PVMFSuccess;

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::DiscardData(PVMFTimestamp aTimestamp, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::DiscardData() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    aTimestamp = 0;

    //this component doesn't buffer data, so there's nothing
    //needed here.

    PVMFStatus status = PVMFSuccess;

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::Stop(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::Stop() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    PVMFStatus status = PVMFFailure;

    switch (iState)
    {
        case STATE_STARTED:
        case STATE_PAUSED:
            // Add this case because of the following problem:
            // This mio component may delay its start until it does
            // configuration based on the first fragment it receives.
            // If the parent node is stopped before this fragment arrives
            // it will issue a stop command to this component, which
            // is still in the initialized state.  returning
            // PVMFErrInvalid state would be unnecessary here
            // when the component is already in the proper post-stop()
            // state.
        case STATE_INITIALIZED:
            iState = STATE_INITIALIZED;
            status = PVMFSuccess;
            break;

        default:
            status = PVMFErrInvalidState;
            break;
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::CancelAllCommands(const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::CancelAllCommands() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    //commands are executed immediately upon being received, so
    //it isn't really possible to cancel them.

    PVMFStatus status = PVMFSuccess;

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

PVMFCommandId PVRefFileOutput::CancelCommand(PVMFCommandId aCmdId, const OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::CancelCommand() called"));

    PVMFCommandId cmdid = iCommandCounter++;

    //commands are executed immediately upon being received, so
    //it isn't really possible to cancel them.

    //see if the response is still queued.
    PVMFStatus status = PVMFFailure;
    for (uint32 i = 0;i < iCommandResponseQueue.size();i++)
    {
        if (iCommandResponseQueue[i].iCmdId == aCmdId)
        {
            status = PVMFSuccess;
            break;
        }
    }

    CommandResponse resp(status, cmdid, aContext);
    QueueCommandResponse(resp);
    return cmdid;
}

void PVRefFileOutput::ThreadLogon()
{
    if (iState == STATE_IDLE)
    {
        iLogger = PVLogger::GetLoggerObject("PVRefFileOutput");
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::ThreadLogon() called"));
        AddToScheduler();
        iState = STATE_LOGGED_ON;
    }

    //Open the file.
    if (!iFsConnected)
    {
        if (iFs.Connect() != 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::ThreadLogon() Unable to Connect File Server"));
        }
        else
        {
            iFsConnected = true;
        }
    }
}


void PVRefFileOutput::ThreadLogoff()
{
    if (iFsConnected)
    {
        iFs.Close();
        iFsConnected = false;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::ThreadLogoff() called"));
    if (iState != STATE_IDLE)
    {
        RemoveFromScheduler();
        iLogger = NULL;
        iState = STATE_IDLE;
    }
}


void PVRefFileOutput::setPeer(PvmiMediaTransfer* aPeer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::setPeer() called"));
    // Set the observer
    iPeer = aPeer;
}


void PVRefFileOutput::useMemoryAllocators(OsclMemAllocator* write_alloc)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::useMemoryAllocators() called"));
    //not supported.
    OSCL_UNUSED_ARG(write_alloc);
}

bool PVRefFileOutput::CheckWriteBusy(uint32 aSeqNum)
//This routine will determine whether data can be accepted in a writeAsync
//call and if not, will return true;
{
    //we don't expect another data transfer when we're already busy.
    //but if it does occur, leave again.
    if (iWriteBusy)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                        (0, "PVRefFileOutput::LeaveIfBusy: Unexpected call from peer!"));
        return true;
    }

    if (iActiveTiming)
    {
        //when doing active timing, impose a limit on the number
        //of queued messages.  otherwise the queue may grow without
        //bound since the caller is sending data ASAP.
        if (iWriteResponseQueue.size() >= iActiveTiming->iQueueLimit)
        {
            return true;
        }
    }
    else
    {
        //"sim flow control" is a test feature that causes this component
        //to simulate a busy condition on every 5th input buffer.
        if (iSimFlowControl
                && aSeqNum != iWriteBusySeqNum
                && ((aSeqNum + 1) % 5 == 0))
        {
            return true;
        }
    }

    //for all other cases, accept data now.
    return false;
}

void PVRefFileOutput::LogParameters()
{
    iParametersLogged = true;
    char string[128];
    int32 len;
    if (iVideoFormatString.get_size() > 0)
    {
        len = oscl_snprintf(string, 128, "Video Format %s ", iVideoFormatString.get_str());
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iVideoHeightValid)
    {
        len = oscl_snprintf(string, 128, "Video Height %d ", iVideoHeight);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iVideoWidthValid)
    {
        len = oscl_snprintf(string, 128, "Video Width %d ", iVideoWidth);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iVideoDisplayHeightValid)
    {
        len = oscl_snprintf(string, 128, "Video Display Height %d ", iVideoDisplayHeight);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iVideoDisplayWidthValid)
    {
        len = oscl_snprintf(string, 128, "Video Display Width %d ", iVideoDisplayWidth);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iAudioFormatString.get_size() > 0)
    {
        len = oscl_snprintf(string, 128, "Audio Format %s ", iAudioFormatString.get_str());
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iAudioNumChannelsValid)
    {
        len = oscl_snprintf(string, 128, "Audio Num Channels %d ", iAudioNumChannels);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iAudioSamplingRateValid)
    {
        len = oscl_snprintf(string, 128, "Audio Sampling Rate %d ", iAudioSamplingRate);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    if (iTextFormatString.get_size() > 0)
    {
        len = oscl_snprintf(string, 128, "Text Format %s ", iTextFormatString.get_str());
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
}

void PVRefFileOutput::LogCodecHeader(uint32 aSeqNum, const PVMFTimestamp& aTimestamp, uint32 datalen)
{
    if (iLogStrings)
    {
        char string[128];
        int32 len = oscl_snprintf(string, 128, "SeqNum %d Timestamp %d Len %d Codec Header", aSeqNum, aTimestamp, datalen);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    else
    {
        if (iVideoFormat == PVMF_H264)
        {
            iOutputFile.Write(&datalen, sizeof(uint8), sizeof(uint32));
        }
    }
}

void PVRefFileOutput::LogEndOfStream(uint32 aSeqNum, const PVMFTimestamp& aTimestamp)
{
    if (iLogStrings)
    {
        char string[128];
        int32 len = oscl_snprintf(string, 128, "SeqNum %d Timestamp %d EOS", aSeqNum, aTimestamp);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
}

void PVRefFileOutput::LogFrame(uint32 aSeqNum, const PVMFTimestamp& aTimestamp, uint32 datalen)
{
    if (iLogStrings)
    {
        char string[128];
        int32 len = oscl_snprintf(string, 128, "SeqNum %d Timestamp %d Len %d Frame", aSeqNum, aTimestamp, datalen);
        iOutputFile.Write(string, sizeof(uint8), len) ;
    }
    else
    {
        if (iVideoFormat == PVMF_H264)
        {
            iOutputFile.Write(&datalen, sizeof(uint8), sizeof(uint32));
        }
    }
}

PVMFCommandId PVRefFileOutput::writeAsync(uint8 aFormatType, int32 aFormatIndex, uint8* aData, uint32 aDataLen,
        const PvmiMediaXferHeader& data_header_info, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVRefFileOutput::writeAsync() seqnum %d ts %d context %d",
                     data_header_info.seq_num, data_header_info.timestamp, aContext));

    PVMFStatus status = PVMFFailure;
    bool discard = false;

    switch (aFormatType)
    {
        case PVMI_MEDIAXFER_FMT_TYPE_COMMAND :
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::writeAsync() called with Command info."));
            //ignore
            status = PVMFSuccess;
            break;

        case PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION :
            switch (aFormatIndex)
            {
                case PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM:
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVRefFileOutput::writeAsync() called with Notification info - EOS."));
                    if (iLogStrings)
                    {
                        LogEndOfStream(data_header_info.seq_num, data_header_info.timestamp);
                    }
                    status = PVMFSuccess;
                }
                break;
                case PVMI_MEDIAXFER_FMT_INDEX_RE_CONFIG_NOTIFICATION:
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVRefFileOutput::writeAsync() called with Notification info - RECONFIG."));
                    status = HandleReConfig(data_header_info.seq_num);
                }
                break;
                default:
                    //ignore
                    status = PVMFSuccess;
                    break;
            }
            break;

        case PVMI_MEDIAXFER_FMT_TYPE_DATA :
            switch (aFormatIndex)
            {
                case PVMI_MEDIAXFER_FMT_INDEX_FMT_SPECIFIC_INFO:
                    //format-specific info contains codec headers.
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                    (0, "PVRefFileOutput::writeAsync() called with format-specific info."));

                    if (iState < STATE_INITIALIZED)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVRefFileOutput::writeAsync: Error - Invalid state"));
                        status = PVMFErrInvalidState;
                    }
                    else
                    {
                        // Just write out the passed in data to file
                        if (iLogStrings)
                        {
                            if (!iParametersLogged)
                                LogParameters();
                        }
                        if (aDataLen > 0)
                        {
                            LogCodecHeader(data_header_info.seq_num, data_header_info.timestamp, aDataLen);
                            if (iOutputFile.Write(aData, sizeof(uint8), aDataLen) != aDataLen)
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                (0, "PVRefFileOutput::writeAsync: Error - File write failed"));
                                status = PVMFFailure;
                            }
                            else
                            {
                                status = PVMFSuccess;
                            }
                        }
                        else
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO,
                                            (0, "PVRefFileOutput::writeAsync() called aDataLen==0."));
                            status = PVMFSuccess;
                        }
                    }
                    break;

                case PVMI_MEDIAXFER_FMT_INDEX_DATA:
                    //data contains the media bitstream.

                    //Verify the state
                    if (iState != STATE_STARTED)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVRefFileOutput::writeAsync: Error - Invalid state"));
                        status = PVMFErrInvalidState;
                    }
                    //Check whether we can accept data now and leave if we can't.
                    else if (CheckWriteBusy(data_header_info.seq_num))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVRefFileOutput::writeAsync: Entering busy state"));

                        //schedule an event to re-start the data flow.
                        iWriteBusy = true;
                        iWriteBusySeqNum = data_header_info.seq_num;
                        RunIfNotReady();

                        OSCL_LEAVE(OsclErrBusy);
                    }
                    else
                    {
                        // Just write out the passed in data to file
                        if (iLogStrings)
                        {
                            if (!iParametersLogged)
                            {
                                LogParameters();
                            }
                        }

                        if (aDataLen > 0)
                        {
                            //check whether the player clock is in frame-step mode.
                            //do not render audio in frame-step mode.
                            if (iAudioFormat != PVMF_FORMAT_UNKNOWN
                                    && iActiveTiming
                                    && iActiveTiming->FrameStepMode())
                            {
                                discard = true;
                            }

                            LogFrame(data_header_info.seq_num, data_header_info.timestamp, aDataLen);
                            if (iTextFormat == PVMF_3GPP_TIMEDTEXT)
                            {
                                // Guard against somebody setting this MIO component for multiple data types
                                OSCL_ASSERT(iVideoFormat == PVMF_FORMAT_UNKNOWN && iAudioFormat == PVMF_FORMAT_UNKNOWN);

                                PVMFTimedTextMediaData* textmediadata = (PVMFTimedTextMediaData*)aData;

                                // Write out the text sample entry
                                if (textmediadata->iTextSampleEntry.GetRep() != NULL)
                                {
                                    // @todo Write out the text sample entry in a better format
                                    if (iOutputFile.Write((OsclAny*)(textmediadata->iTextSampleEntry.GetRep()), sizeof(PVMFTimedTextSampleEntry), 1) != 1)
                                    {
                                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                        (0, "PVRefFileOutput::writeAsync: Error - File write failed for text sample entry"));
                                        status = PVMFFailure;
                                        break;
                                    }
                                }

                                // Write out the raw text sample
                                if (iOutputFile.Write(textmediadata->iTextSample, sizeof(uint8), textmediadata->iTextSampleLength) != textmediadata->iTextSampleLength)
                                {
                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                    (0, "PVRefFileOutput::writeAsync: Error - File write failed for text sample data"));
                                    status = PVMFFailure;
                                }
                                else
                                {
                                    status = PVMFSuccess;
                                }
                            }
                            else if (discard)
                            {
                                //do not render this frame.
                                char string[128];
                                int32 len = oscl_snprintf(string, 128, "discard-- frame-step mode");
                                if (iOutputFile.Write(string, sizeof(uint8), len) != (uint32)len)
                                {
                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                    (0, "PVRefFileOutput::writeAsync: Error - File write failed"));
                                    status = PVMFFailure;
                                }
                            }
                            else
                            {
                                if (iHeaderWritten != true && (iAudioFormatType == PVMF_PCM16 || iAudioFormatType == PVMF_PCM8))
                                {
                                    iOutputFile.Write(&iRIFFChunk, sizeof(uint8), sizeof(RIFFChunk));
                                    iOutputFile.Write(&iFmtSubchunk, sizeof(uint8), sizeof(fmtSubchunk));
                                    iOutputFile.Write(&iDataSubchunk, sizeof(uint8), sizeof(dataSubchunk));
                                    iHeaderWritten = true;
                                }
                                if (iHeaderWritten != true && (iVideoFormatType == PVMF_YUV420 || iVideoFormatType == PVMF_YUV422))
                                {
                                    WriteHeaders();
                                    iHeaderWritten = true;
                                }

                                if (iAudioFormatType == PVMF_AMR_IETF || iAudioFormatType == PVMF_AMR_IF2 || iVideoFormatType == PVMF_H263 || iVideoFormatType == PVMF_M4V)
                                {
                                    if (iOutputFile.Write(aData, sizeof(uint8), aDataLen) != aDataLen)
                                    {
                                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                        (0, "PVRefFileOutput::writeAsync: Error - File write failed"));
                                        status = PVMFFailure;
                                    }
                                    else
                                    {
                                        status = PVMFSuccess;
                                    }
                                }
                                //'render' this frame
                                if (iAudioFormatType == PVMF_PCM16 || iAudioFormatType == PVMF_PCM8)
                                {
                                    if (iOutputFile.Write(aData, sizeof(uint8), aDataLen) != aDataLen)
                                    {
                                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                        (0, "PVRefFileOutput::writeAsync: Error - File write failed"));
                                        status = PVMFFailure;
                                    }
                                    else
                                    {
                                        if (iAudioFormatType == PVMF_PCM16 || iAudioFormatType == PVMF_PCM8)
                                            iDataSubchunk.subchunk2Size += aDataLen;
                                        status = PVMFSuccess;
                                    }
                                }

                                if (iVideoFormatType == PVMF_YUV420 || iVideoFormatType == PVMF_YUV422)
                                {
#ifdef AVI_OUTPUT
#if 1
                                    unsigned char *u, *v, ch;
                                    uint32 fsize = iVideoWidth * iVideoHeight;
                                    uint32 bsize = iVideoWidth * iVideoHeight * 3 / 2;
                                    u = aData + fsize;
                                    v = aData + fsize * 5 / 4;
                                    for (int j = 0;j < fsize / 4;j++)
                                    {
                                        ch = u[j];
                                        u[j] = v[j];
                                        v[j] = ch;
                                    }
                                    AddChunk(aData, bsize, videoChunkID);
                                    iVideoLastTimeStamp = data_header_info.timestamp;
                                    iAVIChunkSize += bsize + 4 + 4;
#else
                                    uint8 pBufRGBRev[80120];
                                    yuv2rgb(pBufRGBRev, aData, iVideoWidth, iVideoHeight);
                                    uint32 bsize = iVideoWidth * iVideoHeight * 3;
                                    AddChunk(pBufRGBRev, bsize, videoChunkID);
                                    iVideoLastTimeStamp = data_header_info.timestamp;
                                    iAVIChunkSize += bsize + 4 + 4;
#endif
                                    status = PVMFSuccess;
#else
                                    uint32 size = iVideoWidth * iVideoHeight * 3 / 2;
                                    if (iOutputFile.Write(aData, sizeof(uint8), size) != size)
                                    {
                                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                                        (0, "PVRefFileOutput::writeAsync: Error - File write failed"));
                                        status = PVMFFailure;
                                    }
                                    else
                                        status = PVMFSuccess;
#endif
                                }
                            }
                        }
                        else
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO,
                                            (0, "PVRefFileOutput::writeAsync() called aDataLen==0."));
                            status = PVMFSuccess;
                        }
                    }
                    break;

                default:
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "PVRefFileOutput::writeAsync: Error - unrecognized format index"));
                    status = PVMFFailure;
                    break;
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                            (0, "PVRefFileOutput::writeAsync: Error - unrecognized format type"));
            status = PVMFFailure;
            break;
    }

    //Schedule asynchronous response
    PVMFCommandId cmdid = iCommandCounter++;
    WriteResponse resp(status, cmdid, aContext, data_header_info.timestamp, discard);
    iWriteResponseQueue.push_back(resp);
    RunIfNotReady();
    return cmdid;

}

void PVRefFileOutput::writeComplete(PVMFStatus aStatus, PVMFCommandId  write_cmd_id, OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(write_cmd_id);
    OSCL_UNUSED_ARG(aContext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::writeComplete() called"));
    //won't be called since this component is a sink.

}


PVMFCommandId  PVRefFileOutput::readAsync(uint8* data, uint32 max_data_len, OsclAny* aContext,
        int32* formats, uint16 num_formats)
{
    OSCL_UNUSED_ARG(data);
    OSCL_UNUSED_ARG(max_data_len);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(formats);
    OSCL_UNUSED_ARG(num_formats);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::readAsync() called"));
    //read not supported.
    OsclError::Leave(OsclErrNotSupported);
    return 0;
}


void PVRefFileOutput::readComplete(PVMFStatus aStatus, PVMFCommandId  read_cmd_id, int32 format_index,
                                   const PvmiMediaXferHeader& data_header_info, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::readComplete() called"));
    //won't be called since this component is a sink.
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(read_cmd_id);
    OSCL_UNUSED_ARG(format_index);
    OSCL_UNUSED_ARG(data_header_info);
    OSCL_UNUSED_ARG(aContext);
}


void PVRefFileOutput::statusUpdate(uint32 status_flags)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::statusUpdate() called"));
    //won't be called since this component is a sink.
    OSCL_UNUSED_ARG(status_flags);
}


void PVRefFileOutput::cancelCommand(PVMFCommandId  command_id)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::cancelCommand() called"));

    //the purpose of this API is to cancel a writeAsync command and report
    //completion ASAP.

    //in this implementation, the write commands are executed immediately
    //when received so it isn't really possible to cancel.
    //just report completion immediately.

    for (uint32 i = 0;i < iWriteResponseQueue.size();i++)
    {
        if (iWriteResponseQueue[i].iCmdId == command_id)
        {
            //report completion
            if (iPeer)
            {
                iPeer->writeComplete(iWriteResponseQueue[i].iStatus, iWriteResponseQueue[i].iCmdId, (OsclAny*)iWriteResponseQueue[i].iContext);
            }
            iWriteResponseQueue.erase(&iWriteResponseQueue[i]);
            break;
        }
    }
}

void PVRefFileOutput::cancelAllCommands()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVRefFileOutput::cancelAllCommands() called"));

    //the purpose of this API is to cancel all writeAsync commands and report
    //completion ASAP.

    //in this implementaiton, the write commands are executed immediately
    //when received so it isn't really possible to cancel.
    //just report completion immediately.

    while (iWriteResponseQueue.size() > 0)
    {
        //report completion
        if (iPeer)
        {
            iPeer->writeComplete(iWriteResponseQueue[0].iStatus, iWriteResponseQueue[0].iCmdId, (OsclAny*)iWriteResponseQueue[0].iContext);
        }
        iWriteResponseQueue.erase(&iWriteResponseQueue[0]);
    }

}

void PVRefFileOutput::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    OSCL_UNUSED_ARG(aObserver);
    //not needed since this component only supports synchronous capability & config
    //APIs.
}

PVMFStatus PVRefFileOutput::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
        PvmiKvp*& aParameters, int& num_parameter_elements,
        PvmiCapabilityContext aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);

    if (pv_mime_strcmp(aIdentifier, INPUT_FORMATS_CAP_QUERY) == 0)
    {
        aParameters = NULL;
        num_parameter_elements = 0;

        //This is a query for the list of supported formats.

        //This component supports any audio or video format

        //Generate a list of all the PVMF audio & video formats...
        int32 count = 0;
        if (iFormatMask == 0 || (iFormatMask & PVMF_UNCOMPRESSED_AUDIO_FORMAT))
        {
            count += (1 + PVMF_LAST_UNCOMPRESSED_AUDIO - PVMF_FIRST_UNCOMPRESSED_AUDIO);
        }
        if (iFormatMask == 0 || (iFormatMask & PVMF_COMPRESSED_AUDIO_FORMAT))
        {
            count += (1 + PVMF_LAST_COMPRESSED_AUDIO - PVMF_FIRST_COMPRESSED_AUDIO);
        }
        if (iFormatMask == 0 || (iFormatMask & PVMF_UNCOMPRESSED_VIDEO_FORMAT))
        {
            count += (1 + PVMF_LAST_UNCOMPRESSED_VIDEO - PVMF_FIRST_UNCOMPRESSED_VIDEO);
        }
        if (iFormatMask == 0 || (iFormatMask & PVMF_COMPRESSED_VIDEO_FORMAT))
        {
            count += (1 + PVMF_LAST_COMPRESSED_VIDEO - PVMF_FIRST_COMPRESSED_VIDEO);
        }
        if (iFormatMask == 0 || (iFormatMask & PVMF_TEXT_FORMAT))
        {
            count += ( + 1 + PVMF_LAST_TEXT - PVMF_FIRST_TEXT);
        }

        aParameters = (PvmiKvp*)oscl_malloc(count * sizeof(PvmiKvp));

        if (aParameters)
        {
            PVMFFormatType fmt;
            if (iFormatMask == 0 || (iFormatMask & PVMF_UNCOMPRESSED_AUDIO_FORMAT))
            {
                for (fmt = PVMF_FIRST_UNCOMPRESSED_AUDIO;fmt <= PVMF_LAST_UNCOMPRESSED_AUDIO;fmt++)
                {
                    aParameters[num_parameter_elements].value.uint32_value = (uint32)fmt;
                    aParameters[num_parameter_elements].key = (PvmiKeyType)oscl_malloc(oscl_strlen(MOUT_AUDIO_FORMAT_KEY) + 1);
                    if (!aParameters[num_parameter_elements].key)
                    {
                        return PVMFErrNoMemory;
                        // (hope it's safe to leave array partially
                        //  allocated, caller will free?)
                    }
                    oscl_strncpy(aParameters[num_parameter_elements++].key, MOUT_AUDIO_FORMAT_KEY, oscl_strlen(MOUT_AUDIO_FORMAT_KEY) + 1);
                }
            }
            if (iFormatMask == 0 || (iFormatMask & PVMF_COMPRESSED_AUDIO_FORMAT))
            {
                for (fmt = PVMF_FIRST_COMPRESSED_AUDIO;fmt <= PVMF_LAST_COMPRESSED_AUDIO;fmt++)
                {
                    aParameters[num_parameter_elements].value.uint32_value = (uint32)fmt;
                    aParameters[num_parameter_elements].key = (PvmiKeyType)oscl_malloc(oscl_strlen(MOUT_AUDIO_FORMAT_KEY) + 1);
                    if (!aParameters[num_parameter_elements].key)
                    {
                        return PVMFErrNoMemory;
                    }
                    oscl_strncpy(aParameters[num_parameter_elements++].key, MOUT_AUDIO_FORMAT_KEY, oscl_strlen(MOUT_AUDIO_FORMAT_KEY) + 1);
                }
            }
            if (iFormatMask == 0 || (iFormatMask & PVMF_UNCOMPRESSED_VIDEO_FORMAT))
            {
                for (fmt = PVMF_FIRST_UNCOMPRESSED_VIDEO;fmt <= PVMF_LAST_UNCOMPRESSED_VIDEO;fmt++)
                {
                    aParameters[num_parameter_elements].value.uint32_value = (uint32)fmt;
                    aParameters[num_parameter_elements].key = (PvmiKeyType)oscl_malloc(oscl_strlen(MOUT_VIDEO_FORMAT_KEY) + 1);
                    if (!aParameters[num_parameter_elements].key)
                    {
                        return PVMFErrNoMemory;
                    }
                    oscl_strncpy(aParameters[num_parameter_elements++].key, MOUT_VIDEO_FORMAT_KEY, oscl_strlen(MOUT_VIDEO_FORMAT_KEY) + 1);
                }
            }
            if (iFormatMask == 0 || (iFormatMask & PVMF_COMPRESSED_VIDEO_FORMAT))
            {
                for (fmt = PVMF_FIRST_COMPRESSED_VIDEO;fmt <= PVMF_LAST_COMPRESSED_VIDEO;fmt++)
                {
                    aParameters[num_parameter_elements].value.uint32_value = (uint32)fmt;
                    aParameters[num_parameter_elements].key = (PvmiKeyType)oscl_malloc(oscl_strlen(MOUT_VIDEO_FORMAT_KEY) + 1);
                    if (!aParameters[num_parameter_elements].key)
                    {
                        return PVMFErrNoMemory;
                    }
                    oscl_strncpy(aParameters[num_parameter_elements++].key, MOUT_VIDEO_FORMAT_KEY, oscl_strlen(MOUT_VIDEO_FORMAT_KEY) + 1);
                }
            }
            if (iFormatMask == 0 || (iFormatMask & PVMF_TEXT_FORMAT))
            {
                for (fmt = PVMF_FIRST_TEXT;fmt <= PVMF_LAST_TEXT;fmt++)
                {
                    aParameters[num_parameter_elements].value.uint32_value = (uint32)fmt;
                    aParameters[num_parameter_elements].key = (PvmiKeyType)oscl_malloc(oscl_strlen(MOUT_TEXT_FORMAT_KEY) + 1);
                    if (!aParameters[num_parameter_elements].key)
                    {
                        return PVMFErrNoMemory;
                    }
                    oscl_strncpy(aParameters[num_parameter_elements++].key, MOUT_TEXT_FORMAT_KEY, oscl_strlen(MOUT_TEXT_FORMAT_KEY) + 1);
                }
            }
            return PVMFSuccess;
        }
        return PVMFErrNoMemory;
    }
    else if (pv_mime_strcmp(aIdentifier, PVMF_NUM_DECODED_FRAMES_CONFIG_KEY) == 0)
    {
        aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
        if (!aParameters)
        {
            return PVMFErrNoMemory;
        }

        aParameters[0].value.uint32_value = DEFAULT_NUM_DECODED_FRAMES_CAPABILITY;
        return PVMFSuccess;
    }
    //other queries are not currently supported.

    //unrecognized key.
    return PVMFFailure;
}

PVMFStatus PVRefFileOutput::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    OSCL_UNUSED_ARG(aSession);

    //release parameters that were allocated by this component.
    if (aParameters)
    {
        for (int i = 0; i < num_elements; i++)
        {
            oscl_free(aParameters[i].key);
        }
        oscl_free(aParameters);
        return PVMFSuccess;
    }
    return PVMFFailure;
}

void PVRefFileOutput::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);

    OsclError::Leave(OsclErrNotSupported);
}

void PVRefFileOutput::setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
        PvmiKvp* aParameters, int num_parameter_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_parameter_elements);

    OsclError::Leave(OsclErrNotSupported);
}

void PVRefFileOutput::DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);

    OsclError::Leave(OsclErrNotSupported);
}


void PVRefFileOutput::setParametersSync(PvmiMIOSession aSession,
                                        PvmiKvp* aParameters,
                                        int num_elements,
                                        PvmiKvp * & aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);
    aRet_kvp = NULL;

    for (int32 i = 0;i < num_elements;i++)
    {
        //Check against known audio parameter keys...
        if (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_FORMAT_KEY) == 0)
        {
            if (oscl_strncmp(aParameters[i].value.pChar_value, "audio/L16", sizeof("audio/L16")) == 0)
                iAudioFormatType = PVMF_PCM16;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "audio/L8", sizeof("audio/L8")) == 0)
                iAudioFormatType = PVMF_PCM8;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "X-AMR-IF2", sizeof("X-AMR-IF2")) == 0)
                iAudioFormatType = PVMF_AMR_IF2;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "X-AMR-IETF-SEPARATE", sizeof("X-AMR-IETF-SEPARATE")) == 0)
                iAudioFormatType = PVMF_AMR_IETF;

            iAudioFormat = iAudioFormatType;
            GetFormatString(iAudioFormat, iAudioFormatString);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Audio Format Key, Value %s", iAudioFormatString.get_str()));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_SAMPLING_RATE_KEY) == 0)
        {
            iAudioSamplingRate = (int32)aParameters[i].value.uint32_value;
            iAudioSamplingRateValid = true;
            iFmtSubchunk.sampleRate = iAudioSamplingRate;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Audio Sampling Rate Key, Value %d", iAudioSamplingRate));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_NUM_CHANNELS_KEY) == 0)
        {
            iAudioNumChannels = (int32)aParameters[i].value.uint32_value;
            iAudioNumChannelsValid = true;
            iFmtSubchunk.numChannels = iAudioNumChannels;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Audio Num Channels Key, Value %d", iAudioNumChannels));
        }
        //Check against known video parameter keys...
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_FORMAT_KEY) == 0)
        {

            if (oscl_strncmp(aParameters[i].value.pChar_value, "X-YUV-420", sizeof("X-YUV-420")) == 0)
                iVideoFormatType = PVMF_YUV420;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "X-YUV-422", sizeof("X-YUV-422")) == 0)
                iVideoFormatType = PVMF_YUV422;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "video/H263-2000", sizeof("video/H263-2000")) == 0)
                iVideoFormatType = PVMF_H263;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "video/H263-1998", sizeof("video/H263-1998")) == 0)
                iVideoFormatType = PVMF_H263;
            else if (oscl_strncmp(aParameters[i].value.pChar_value, "video/MP4V-ES", sizeof("video/MP4V-ES")) == 0)
                iVideoFormatType = PVMF_M4V;

            iVideoFormat = iVideoFormatType;

            GetFormatString(iVideoFormat, iVideoFormatString);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Video Format Key, Value %s", iVideoFormatString.get_str()));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_WIDTH_KEY) == 0)
        {
            iVideoWidth = (int32)aParameters[i].value.uint32_value;
            iVideoWidthValid = true;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Video Width Key, Value %d", iVideoWidth));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_HEIGHT_KEY) == 0)
        {
            iVideoHeight = (int32)aParameters[i].value.uint32_value;
            iVideoHeightValid = true;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Video Height Key, Value %d", iVideoHeight));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_DISPLAY_HEIGHT_KEY) == 0)
        {
            iVideoDisplayHeight = (int32)aParameters[i].value.uint32_value;
            iVideoDisplayHeightValid = true;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Video Display Height Key, Value %d", iVideoDisplayHeight));
        }
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_VIDEO_DISPLAY_WIDTH_KEY) == 0)
        {
            iVideoDisplayWidth = (int32)aParameters[i].value.uint32_value;
            iVideoDisplayWidthValid = true;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Video Display Width Key, Value %d", iVideoDisplayWidth));
        }
        //Check against known text parameter keys...
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_TEXT_FORMAT_KEY) == 0)
        {
            iTextFormatString = aParameters[i].value.pChar_value;
            iTextFormat = GetFormatIndex(iTextFormatString.get_str());
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Text Format Key, Value %s", iTextFormatString.get_str()));
        }
        // Change to output rate
        else if (pv_mime_strcmp(aParameters[i].key, MOUT_MEDIAXFER_OUTPUT_RATE) == 0)
        {
            // Do nothing, this setting is meaningless for file IO
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() MediaXFER Output Rate Key, Value %d",
                             aParameters[i].value.int32_value));
        }
        else if (pv_mime_strcmp(aParameters[i].key, PVMF_FORMAT_SPECIFIC_INFO_KEY) == 0)
        {
            if (!iFileOpened)
            {
                if (iOutputFile.Open(iOutputFileName.get_cstr(), Oscl_File::MODE_READWRITE | Oscl_File::MODE_BINARY, iFs) != 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "PVRefFileOutput::setParametersSync: Error - File Open failed"));
                }
                else
                {
                    iFileOpened = true;
                    LogCodecHeader(0, 0, (int32)aParameters[i].capacity);
                    if (iOutputFile.Write(aParameters[i].value.pChar_value,
                                          sizeof(uint8),
                                          (int32)aParameters[i].capacity) != (uint32)aParameters[i].length)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "PVRefFileOutput::setParametersSync: Error - File write failed"));
                    }
                }
            }
        }
        else
        {
            //if we get here the key is unrecognized.
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVRefFileOutput::setParametersSync() Error, unrecognized key "));

            //set the return value to indicate the unrecognized key
            //and return.
            aRet_kvp = &aParameters[i];
            return;
        }
    }
    if (iAudioFormatType == PVMF_PCM16)
    {
        iFmtSubchunk.bitsPerSample = 16;
        iFmtSubchunk.byteRate = iFmtSubchunk.sampleRate * iFmtSubchunk.numChannels * iFmtSubchunk.bitsPerSample / 8;
        iFmtSubchunk.blockAlign = iFmtSubchunk.numChannels * iFmtSubchunk.bitsPerSample / 8;
    }
    else
    {
        iFmtSubchunk.bitsPerSample = 8;
        iFmtSubchunk.byteRate = iFmtSubchunk.sampleRate * iFmtSubchunk.numChannels * iFmtSubchunk.bitsPerSample / 8;
        iFmtSubchunk.blockAlign = iFmtSubchunk.numChannels * iFmtSubchunk.bitsPerSample / 8;

    }

    if ((iVideoFormatType == PVMF_YUV420 || iVideoFormatType == PVMF_YUV422) && (iVideoHeightValid == true && iVideoHeightValid == true && iInitializeAVIDone == false))
    {
        InitializeAVI(iVideoWidth, iVideoHeight);
        iInitializeAVIDone = true;
    }
}

PVMFCommandId PVRefFileOutput::setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters,
        int num_elements, PvmiKvp*& aRet_kvp, OsclAny* context)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_elements);
    OSCL_UNUSED_ARG(aRet_kvp);
    OSCL_UNUSED_ARG(context);

    OsclError::Leave(OsclErrNotSupported);
    return 0;
}

uint32 PVRefFileOutput::getCapabilityMetric(PvmiMIOSession aSession)
{
    OSCL_UNUSED_ARG(aSession);

    return 0;
}

PVMFStatus PVRefFileOutput::verifyParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int num_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(num_elements);
    /* Since we are just logging to file, we can always return success */
    return PVMFSuccess;
}

void PVRefFileOutput::setFormatMask(uint32 mask)
{
    iFormatMask = mask;
}

//
// For active timing support
//
OSCL_EXPORT_REF PVMFStatus PVRefFileOutputActiveTimingSupport::SetClock(OsclClock *clockVal)
{
    iClock = clockVal;
    return PVMFSuccess;
}

OSCL_EXPORT_REF void PVRefFileOutputActiveTimingSupport::addRef()
{
}

OSCL_EXPORT_REF void PVRefFileOutputActiveTimingSupport::removeRef()
{
}

OSCL_EXPORT_REF bool PVRefFileOutputActiveTimingSupport::queryInterface(const PVUuid& aUuid, PVInterface*& aInterface)
{
    aInterface = NULL;
    PVUuid uuid;
    queryUuid(uuid);
    if (uuid == aUuid)
    {
        PvmiClockExtensionInterface* myInterface = OSCL_STATIC_CAST(PvmiClockExtensionInterface*, this);
        aInterface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        return true;
    }
    return false;
}

void PVRefFileOutputActiveTimingSupport::queryUuid(PVUuid& uuid)
{
    uuid = PvmiClockExtensionInterfaceUuid;
}

bool PVRefFileOutputActiveTimingSupport::FrameStepMode()
{
    if (iClock && iClock->GetCountTimebase())
    {
        //frame-step mode detected!

        //there will be a discontinuity in the rendering-- so reset the
        //simulated delay control now.
        if (iLastTimestampValid)
        {
            iLastTimestampValid = false;
        }
        return true;
    }
    return false;
}

void PVRefFileOutputActiveTimingSupport::AdjustClock(PVMFTimestamp& aTs)
{
    //Adjust player clock to match rendering time.
    //This is called at the time of simulated rendering.
    //On a real device, we would read the rendering position from the device.
    //In this simulation, we just assume that data is rendered ASAP and there
    //is no drift in the rendering device clock.
    //Therefore we just set the player clock to match the media timestamp at
    //the time of simulated rendering.
    //However, only adjust the clock ahead when in frame step mode

    if (iClock)
    {
        uint64 clktime;
        uint64 tbtime;
        iClock->GetCurrentTime64(clktime, OSCLCLOCK_MSEC, tbtime);
        {
            // always adjust clock if not in frame step mode
            // if in framestep mode, only adjust clock if the timestamp is ahead
            bool frameStep = FrameStepMode();
            if (!frameStep || (frameStep && (aTs > (PVMFTimestamp)clktime)))
            {
                if (!iLogger)
                {
                    iLogger = PVLogger::GetLoggerObject("PVRefFileOutput");
                }
                uint64 adjtime = aTs;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG,
                                (0, "PVRefFileOutputActiveTimingSupport::AdjustClock: from %d to %d", (uint32)clktime, (uint32)adjtime));
                iClock->AdjustClockTime64(clktime, tbtime, adjtime, OSCLCLOCK_MSEC);
            }
        }
    }
}

//This routine does 2 things-- computes the delay before we can render this frame,
//based on simulated rendering time for the prior frame, and adjusts the
//player clock when it's time to render this frame.
uint32 PVRefFileOutputActiveTimingSupport::GetDelayMsec(PVMFTimestamp& aTs)
{
    //This routine can be called twice per frame.  On the first call, if
    //a non-zero delay is returned, it will be called a second time after the delay
    //has elapsed.

    if (iDelay > 0)
    {
        //already delayed this frame, so render now.
        iDelay = 0;
    }
    else //first call for this frame.
    {
        if (!iLastTimestampValid)
        {
            //this is the very first frame-- render it ASAP.
            //render first frame ASAP.
            iLastTimestampValid = true;
            iDelay = 0;
        }
        else
        {
            //not the first frame.
            //delay long enough to allow the prior data to render.
            iDelay = aTs - iLastTimestamp;
        }
        //save this timestamp for next delta computation.
        iLastTimestamp = aTs;
    }

    //if delay is zero, it's time to render now.  in this case, we
    //need to adjust the player clock with the device rendering time.
    if (iDelay == 0)
    {
        AdjustClock(aTs);
    }

    return iDelay;
}

//
// Private section
//
PVMFStatus PVRefFileOutput::HandleReConfig(uint32 aReconfigSeqNum)
{
    PVMFStatus status = PVMFFailure;
    /* Close the existing file and open a new one */
    if (iFileOpened)
    {
        iOutputFile.Close();
    }
    iFileOpened = false;

    aReconfigSeqNum++;
    oscl_wchar append[8];
    OSCL_wStackString<8> fmt(_STRLIT_WCHAR("%d"));
    oscl_snprintf(append, 8, fmt.get_cstr(), aReconfigSeqNum);
    append[7] = (oscl_wchar)'\0';
    OSCL_wStackString<32> appendString(append);
    iOutputFileName += appendString.get_cstr();
    if (iOutputFile.Open(iOutputFileName.get_cstr(), Oscl_File::MODE_READWRITE | Oscl_File::MODE_BINARY, iFs) == 0)
    {
        status = PVMFSuccess;
        iFileOpened = true;
        iParametersLogged = false;
    }
    return status;
}

void PVRefFileOutput::Run()
{
    //send async command responses
    while (!iCommandResponseQueue.empty())
    {
        if (iObserver)
        {
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId, iCommandResponseQueue[0].iContext, iCommandResponseQueue[0].iStatus));
        }
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }

    //send async write completion
    while (!iWriteResponseQueue.empty())
    {
        //for active timing mode, insert some delays to simulate
        //actual device rendering time.
        if (!iWriteResponseQueue[0].iDiscard
                && iActiveTiming)
        {
            uint32 delay = iActiveTiming->GetDelayMsec(iWriteResponseQueue[0].iTimestamp);
            if (delay > 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVRefFileOutput::Run() Re-scheduling in %d msec", delay));
                RunIfNotReady(delay*1000);
                return;
            }
        }
        //report write complete
        if (iPeer)
        {
            iPeer->writeComplete(iWriteResponseQueue[0].iStatus, iWriteResponseQueue[0].iCmdId, (OsclAny*)iWriteResponseQueue[0].iContext);
        }
        //report playback position to the test observer.
        if (iTestObserver)
        {
            iTestObserver->Pos(iWriteResponseQueue[0].iTimestamp);
        }
        iWriteResponseQueue.erase(&iWriteResponseQueue[0]);
    }

    //Re-start the data transfer if needed.
    if (iWriteBusy)
    {
        iWriteBusy = false;
        iPeer->statusUpdate(PVMI_MEDIAXFER_STATUS_WRITE);
    }
}


int32 PVRefFileOutput::yuv2rgb(uint8 * pBufRGBRev, uint8 * pBufYUV, int32 width, int32 height)
{
    int32 i, j, yi, yj;
    int32 w2 = width / 2;
    int32 rowoff, yrowoff, uvrowoff;
    uint8 *pBufY = pBufYUV;
    uint8 *pBufU = pBufYUV + width * height;
    uint8 *pBufV = pBufU + width * height / 4;

    for (i = 0, yi = 0; i < height; i++, yi++)
    {
        rowoff = ((height - 1) - i) * width * 3;
        yrowoff = yi * width;
        uvrowoff = (yi / 2) * w2;
        for (j = 0, yj = 0; j < width*3; j += 3, yj++)
        {
            pBufRGBRev[rowoff+j]   = (uint8)max(0, min(255, (1.164 * (pBufY[yrowoff+yj] - 16) +
                                                2.018 * (pBufU[uvrowoff+yj/2] - 128)))); // blue
            pBufRGBRev[rowoff+j+1] = (uint8)max(0, min(255, (1.164 * (pBufY[yrowoff+yj] - 16) -
                                                0.813 * (pBufV[uvrowoff+yj/2] - 128) -
                                                0.391 * (pBufU[uvrowoff+yj/2] - 128)))); // green
            pBufRGBRev[rowoff+j+2] = (uint8)max(0, min(255, (1.164 * (pBufY[yrowoff+yj] - 16) +
                                                1.596 * (pBufV[uvrowoff+yj/2] - 128)))); // red

        }
    }
    return 0;
}


void PVRefFileOutput::InitializeAVI(int width, int height)
{

    uint32 fsize, bsize;
    fsize = width * height;			 // avi physical frame size
    bsize = width * height * 3;  // frame buffer size

    //Init the AVIMainHeader
    iAVIMainHeader.dwMicroSecPerFrame = 200000;
    iAVIMainHeader.dwMaxBytesPerSec    = 5 * 3 * width * height;
    iAVIMainHeader.dwPaddingGranularity = 0;
    iAVIMainHeader.dwFlags = AVIF_TRUSTCKTYPE | AVIF_HASINDEX;
    iAVIMainHeader.dwTotalFrames = DEFAULT_COUNT;
    iAVIMainHeader.dwInitialFrames = 0;
    iAVIMainHeader.dwStreams = 1;
    iAVIMainHeader.dwSuggestedBufferSize = bsize;
    iAVIMainHeader.dwWidth = width;
    iAVIMainHeader.dwHeight = height;
    iAVIMainHeader.dwScale = 0;
    iAVIMainHeader.dwRate = 0;
    iAVIMainHeader.dwStart = 0;
    iAVIMainHeader.dwLength = 0;

    // creating fourCC independent parameters
    iAVIStreamHeader.fccType = streamtypeVIDEO;
    /* support only YUV for now */
    iAVIStreamHeader.fccHandler = mmioFOURCC('I', '4', '2', '0'); // BI_RGB
    iAVIStreamHeader.dwFlags = 0;			// containing AVITF_ flags
    iAVIStreamHeader.wPriority = 0;
    iAVIStreamHeader.wLanguage = 0;
    iAVIStreamHeader.dwScale = 1000;
    iAVIStreamHeader.dwInitialFrames = 0;
    iAVIStreamHeader.dwRate = 5000;
    iAVIStreamHeader.dwStart = 0;
    iAVIStreamHeader.dwLength = DEFAULT_COUNT;  /* will be changed later */
    iAVIStreamHeader.dwSuggestedBufferSize = bsize;
    iAVIStreamHeader.dwQuality = 0;
    iAVIStreamHeader.dwSampleSize = 0;
    iAVIStreamHeader.rcFrame.left = 0;
    iAVIStreamHeader.rcFrame.top = 0;
    iAVIStreamHeader.rcFrame.right = width;
    iAVIStreamHeader.rcFrame.bottom = height;

    bi_hdr.biSize = sizeof(bi_hdr);
    bi_hdr.biWidth = width;
    bi_hdr.biHeight = height;
    bi_hdr.biPlanes = 1;
    bi_hdr.biXPelsPerMeter = 0;
    bi_hdr.biYPelsPerMeter = 0;
    bi_hdr.biClrUsed = 0;
    bi_hdr.biClrImportant = 0;
    bi_hdr.biBitCount = 24;				// every WORD a pixel
    bi_hdr.biSizeImage = bsize;
    bi_hdr.biCompression = iAVIStreamHeader.fccHandler;
}


void PVRefFileOutput::WriteHeaders()
{
    uint32 tmp;

#ifndef AVI_OUTPUT
    return;
#endif

    tmp = FOURCC_RIFF;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"RIFF"
    tmp = 38016;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte size til the end
    tmp = formtypeAVI;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"AVI "
    tmp = FOURCC_LIST;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"LIST"
    tmp = 4 + 4 + 4 + sizeof(AVIMainHeader) + 4 + 4 + 4 + 4 + 4 + 4 + 4 + sizeof(AVIStreamHeader) + sizeof(BitMapInfoHeader);
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte list chunk size (entire list including all streams)

    // Write AVIMainHeader
    tmp = listtypeAVIHEADER;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"hdrl"
    tmp = ckidAVIMAINHDR;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"avih"
    tmp = sizeof(AVIMainHeader);
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte AVI Header size
    iAVIMainHeaderPosition = iOutputFile.Tell();
    iOutputFile.Write(&iAVIMainHeader, sizeof(uint8), sizeof(AVIMainHeader));  //AVI Header Data

    tmp = FOURCC_LIST;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"LIST"
    tmp = 4 + 4 + 4 + 4 + 4 + sizeof(AVIStreamHeader) + sizeof(BitMapInfoHeader);
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte size of second LIST chunk (for the first stream)
    tmp = listtypeSTREAMHEADER;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"strl"
    tmp = ckidSTREAMHEADER;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"strh"
    tmp = sizeof(AVIStreamHeader);  //size of strh
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte size of strh

    iAVIStreamHeaderPosition = iOutputFile.Tell();
    iOutputFile.Write(&iAVIStreamHeader, sizeof(uint8), sizeof(AVIStreamHeader));


    tmp = ckidSTREAMFORMAT;    //same format as BITMAPINFO
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"strf"
    tmp = sizeof(BitMapInfoHeader);
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte size of  strf
    iOutputFile.Write(&bi_hdr, sizeof(uint8), sizeof(BitMapInfoHeader));  //stream format data

    tmp = FOURCC_LIST;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"LIST"
    iVideoHeaderPosition = iOutputFile.Tell();
    tmp = 0;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //4-byte size of movi chunk below
    tmp = listtypeAVIMOVIE;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"movi"
#if 0
    tmp = ckidAVIPADDING;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"movi"
    tmp = 0x6E0;
    iOutputFile.Write(&tmp, sizeof(uint8), sizeof(uint32));  //"movi"
    char junk[0x6E0];
    oscl_memset(junk, 0, 0x6E0);
    iOutputFile.Write(junk, sizeof(uint8), 0x6E0);  //"movi"
#endif

    tmp = ckidAVINEWINDEX;
    oscl_memcpy(&iIndexBuffer.indexBuffer[0], &tmp, sizeof(uint32));
    iIndexBuffer.length = 4;
    tmp = 0;
    oscl_memcpy(&iIndexBuffer.indexBuffer[iIndexBuffer.length], &tmp, sizeof(uint32));
    iIndexBuffer.length += 4;
}

void PVRefFileOutput::AddChunk(uint8* chunk, uint32 size, uint32 ckid)
{
    iOutputFile.Write(&ckid, sizeof(uint8), sizeof(uint32));
    iOutputFile.Write(&size, sizeof(uint8), sizeof(uint32));
    iOutputFile.Write(chunk, sizeof(uint8), size);

    iAVIIndex.chunkID = videoChunkID;
    iAVIIndex.flags = 0x10;
    if (iVideoCount == 0)
    {
        iAVIIndex.offset = iIndexBuffer.length - 4;
        iVideoCount++;
        iPreviousOffset = iAVIIndex.offset;
    }
    else
    {
        iAVIIndex.offset = iPreviousOffset + size + 8;
        iPreviousOffset = iAVIIndex.offset;
//		iAVIIndex.offset = 4 + (size + 8) * iVideoCount; //iIndexBuffer.length + size * iVideoCount;
        iVideoCount++;
    }
    iAVIIndex.length = size;
    oscl_memcpy(&iIndexBuffer.indexBuffer[iIndexBuffer.length], &iAVIIndex, sizeof(AVIIndex));
    iIndexBuffer.length += sizeof(AVIIndex);
}

void PVRefFileOutput::UpdateWaveChunkSize()
{
    int32 ret = iOutputFile.Tell();
    iOutputFile.Seek(4, Oscl_File::SEEKSET);
    ret = iOutputFile.Tell();
    iRIFFChunk.chunkSize = 36 + iDataSubchunk.subchunk2Size;
    iOutputFile.Write(&iRIFFChunk.chunkSize, sizeof(uint8), 4);
    iOutputFile.Seek(40, Oscl_File::SEEKSET);
    iOutputFile.Write(&iDataSubchunk.subchunk2Size, sizeof(uint8), 4);
    iOutputFile.Flush();
}

void PVRefFileOutput::UpdateVideoChunkHeaderIdx()
{
    //update the AVI Main Header
    if (iVideoCount == 0 || iVideoLastTimeStamp == 0) return;
    iAVIMainHeader.dwMicroSecPerFrame = (uint32)((float)iVideoLastTimeStamp / (float)iVideoCount * 1000);
    iAVIMainHeader.dwMaxBytesPerSec    = (uint32)((float)(iVideoCount * 3 * iVideoHeight * iVideoWidth) / (float)iVideoLastTimeStamp * 1000);
    iAVIMainHeader.dwTotalFrames = iVideoCount;

    iOutputFile.Seek(iAVIMainHeaderPosition, Oscl_File::SEEKSET);
    iOutputFile.Write(&iAVIMainHeader, sizeof(uint8), sizeof(AVIMainHeader));

    iAVIStreamHeader.dwRate = (uint32)((float)(iVideoCount * 1000000) / (float)iVideoLastTimeStamp);
    iAVIStreamHeader.dwLength = iVideoCount;

    iOutputFile.Seek(iAVIStreamHeaderPosition, Oscl_File::SEEKSET);
    iOutputFile.Write(&iAVIStreamHeader, sizeof(uint8), sizeof(AVIStreamHeader));

    iOutputFile.Seek(0, Oscl_File::SEEKEND);
    uint32 tmp = iIndexBuffer.length - 8;

    //write the indexBuffer to the file
    oscl_memcpy(&iIndexBuffer.indexBuffer[4], &tmp, sizeof(uint32));
    iOutputFile.Write(iIndexBuffer.indexBuffer, sizeof(uint8), iIndexBuffer.length);
    iOutputFile.Seek(0, Oscl_File::SEEKEND);
    uint32 iAVISize = iOutputFile.Tell() - 8;
    iOutputFile.Seek(4, Oscl_File::SEEKSET);
    iOutputFile.Write(&iAVISize, sizeof(uint8), 4);
    iOutputFile.Seek(iVideoHeaderPosition, Oscl_File::SEEKSET);
    iAVIChunkSize += 4;
    iOutputFile.Write(&iAVIChunkSize, sizeof(uint8), 4);

}

