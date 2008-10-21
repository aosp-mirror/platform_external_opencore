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
 * @file pvmi_mio_avi_wavfile.cpp
 * @brief PV Media IO interface implementation using file input
 */

#ifndef PVMI_MIO_AVIFILE_H_INCLUDED
#include "pvmi_mio_avi_wav_file.h"
#endif
#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

#define PVMIOFILEIN_MEDIADATA_POOLNUM 8

// Logging macros
#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m)
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m)
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m)

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PvmiMIOControl* PvmiMIOAviWavFileFactory::Create(uint32 aNumLoops, bool aRecordingMode, uint32 aStreamNo, OsclAny* aFileParser, FileFormatType aFileType, int32& arError)
{
    PvmiMIOControl *mioFilein = (PvmiMIOControl*) OSCL_NEW(PvmiMIOAviWavFile, (aNumLoops, aRecordingMode, aStreamNo, aFileParser, aFileType, arError));

    return mioFilein;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PvmiMIOAviWavFileFactory::Delete(PvmiMIOControl* aMio)
{
    PvmiMIOAviWavFile* mioFilein = (PvmiMIOAviWavFile*)aMio;
    if (!mioFilein)
    {
        return false;
    }
    delete mioFilein;

    mioFilein = NULL;
    return true;

}

////////////////////////////////////////////////////////////////////////////
PvmiMIOAviWavFile::~PvmiMIOAviWavFile()
{
#if PROFILING_ON
    if (!oDiagnosticsLogged)
        LogDiagnostics();
#endif
    if (iMediaBufferMemPool)
    {
        OSCL_TEMPLATED_DELETE(iMediaBufferMemPool, OsclMemPoolFixedChunkAllocator, OsclMemPoolFixedChunkAllocator);
        iMediaBufferMemPool = NULL;
    }
    delete(iMioClock);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOAviWavFile::connect(PvmiMIOSession& aSession, PvmiMIOObserver* aObserver)
{
    if (!aObserver)
    {
        return PVMFFailure;
    }

    int32 err = 0;
    OSCL_TRY(err, iObservers.push_back(aObserver));
    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);
    aSession = (PvmiMIOSession)(iObservers.size() - 1); // Session ID is the index of observer in the vector
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOAviWavFile::disconnect(PvmiMIOSession aSession)
{
    uint32 index = (uint32)aSession;
    if (index >= iObservers.size())
    {
        // Invalid session ID
        return PVMFFailure;
    }

    iObservers.erase(iObservers.begin() + index);
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PvmiMediaTransfer* PvmiMIOAviWavFile::createMediaTransfer(PvmiMIOSession& aSession,
        PvmiKvp* aRead_formats,
        int32 aRead_flags,
        PvmiKvp* aWrite_formats,
        int32 aWrite_flags)
{
    OSCL_UNUSED_ARG(aRead_formats);
    OSCL_UNUSED_ARG(aRead_flags);
    OSCL_UNUSED_ARG(aWrite_formats);
    OSCL_UNUSED_ARG(aWrite_flags);

    uint32 index = (uint32)aSession;
    if (index >= iObservers.size())
    {
        // Invalid session ID
        OSCL_LEAVE(OsclErrArgument);
        return NULL;
    }

    return (PvmiMediaTransfer*)this;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::deleteMediaTransfer(PvmiMIOSession& aSession,
        PvmiMediaTransfer* aMediaTransfer)
{
    uint32 index = (uint32)aSession;
    if (!aMediaTransfer || index >= iObservers.size())
    {
        // Invalid session ID
        OSCL_LEAVE(OsclErrArgument);
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::QueryUUID(const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly,
        const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aMimeType);
    OSCL_UNUSED_ARG(aExactUuidsOnly);

    int32 err = 0;
    OSCL_TRY(err, aUuids.push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID););
    OSCL_FIRST_CATCH_ANY(err, OSCL_LEAVE(OsclErrNoMemory););

    return AddCmdToQueue(CMD_QUERY_UUID, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::QueryInterface(const PVUuid& aUuid,
        PVInterface*& aInterfacePtr,
        const OsclAny* aContext)
{
    if (PVMI_CAPABILITY_AND_CONFIG_PVUUID == aUuid)
    {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
    }
    else
    {
        aInterfacePtr = NULL;
    }

    return AddCmdToQueue(CMD_QUERY_INTERFACE, aContext, (OsclAny*)&aInterfacePtr);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::Init(const OsclAny* aContext)
{
    if (iState != STATE_IDLE && iState != STATE_INITIALIZED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_INIT, aContext);
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::Start(const OsclAny* aContext)
{
    if (iState != STATE_INITIALIZED
            && iState != STATE_PAUSED
            && iState != STATE_STARTED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_START, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::Pause(const OsclAny* aContext)
{
    if (iState != STATE_STARTED && iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_PAUSE, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::Flush(const OsclAny* aContext)
{
    if (iState != STATE_STARTED || iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_FLUSH, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::Reset(const OsclAny* aContext)
{
    if (iState != STATE_STARTED || iState != STATE_PAUSED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_RESET, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::DiscardData(PVMFTimestamp aTimestamp, const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aTimestamp);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::DiscardData(const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::Stop(const OsclAny* aContext)
{
    if (iState != STATE_STARTED
            && iState != STATE_PAUSED
            && iState != STATE_STOPPED)
    {
        OSCL_LEAVE(OsclErrInvalidState);
        return -1;
    }

    return AddCmdToQueue(CMD_STOP, aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::ThreadLogon()
{
    if (!iThreadLoggedOn)
    {
        AddToScheduler();
        iThreadLoggedOn = true;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::ThreadLogoff()
{
    if (iThreadLoggedOn)
    {
        RemoveFromScheduler();
        iLogger = NULL;
        iThreadLoggedOn = false;
    }
}



////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::CancelAllCommands(const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::CancelCommand(PVMFCommandId aCmdId, const OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::setPeer(PvmiMediaTransfer* aPeer)
{
    iPeer = aPeer;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::useMemoryAllocators(OsclMemAllocator* aWrite_alloc)
{
    OSCL_UNUSED_ARG(aWrite_alloc);
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::writeAsync(uint8 aFormatType, int32 aFormatIndex,
        uint8* aData, uint32 aDataLen,
        const PvmiMediaXferHeader& aData_header_info,
        OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aFormatType);
    OSCL_UNUSED_ARG(aFormatIndex);
    OSCL_UNUSED_ARG(aData);
    OSCL_UNUSED_ARG(aDataLen);
    OSCL_UNUSED_ARG(aData_header_info);
    OSCL_UNUSED_ARG(aContext);
    // This is an active data source. writeAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::writeComplete(PVMFStatus aStatus, PVMFCommandId aWrite_cmd_id,
        OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aContext);
    if ((aStatus != PVMFSuccess) && (aStatus != PVMFErrCancelled))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                        (0, "PvmiMIOAviWavFile::writeComplete: Error - writeAsync failed. aStatus=%d", aStatus));
        OSCL_LEAVE(OsclErrGeneral);
    }

    for (int ii = iSentMediaData.size() - 1; ii >= 0; ii--)
    {
        if (iSentMediaData[ii].iId == aWrite_cmd_id)
        {
            iMediaBufferMemPool->deallocate(iSentMediaData[ii].iData);
            iSentMediaData.erase(&iSentMediaData[ii]);
            return;
        }
    }

    // Error: unmatching ID.
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                    (0, "PvmiMIOAviWavFile::writeComplete: Error - unmatched cmdId %d failed. QSize %d", aWrite_cmd_id, iSentMediaData.size()));
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::readAsync(uint8* aData, uint32 aMax_data_len,
        OsclAny* aContext, int32* aFormats, uint16 aNum_formats)
{
    OSCL_UNUSED_ARG(aData);
    OSCL_UNUSED_ARG(aMax_data_len);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aFormats);
    OSCL_UNUSED_ARG(aNum_formats);
    // This is an active data source. readAsync is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::readComplete(PVMFStatus aStatus, PVMFCommandId aRead_cmd_id,
        int32 aFormat_index, const PvmiMediaXferHeader& aData_header_info,
        OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aStatus);
    OSCL_UNUSED_ARG(aRead_cmd_id);
    OSCL_UNUSED_ARG(aFormat_index);
    OSCL_UNUSED_ARG(aData_header_info);
    OSCL_UNUSED_ARG(aContext);
    // This is an active data source. readComplete is not supported.
    OSCL_LEAVE(OsclErrNotSupported);
    return;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::statusUpdate(uint32 aStatus_flags)
{
    OSCL_UNUSED_ARG(aStatus_flags);
    // Ideally this routine should update the status of media input component.
    // It should check then for the status. If media input buffer is consumed,
    // media input object should be resheduled.
    // Since the Media avifile component is designed with single buffer, two
    // asynchronous reads are not possible. So this function will not be required
    // and hence not been implemented.
    OSCL_LEAVE(OsclErrNotSupported);
}


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::cancelCommand(PVMFCommandId aCmdId)
{
    OSCL_UNUSED_ARG(aCmdId);
    // This cancel command ( with a small "c" in cancel ) is for the media transfer interface.
    // implementation is similar to the cancel command of the media I/O interface.
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::cancelAllCommands()
{
    OSCL_LEAVE(OsclErrNotSupported);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    OSCL_UNUSED_ARG(aObserver);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOAviWavFile::getParametersSync(PvmiMIOSession aSession,
        PvmiKeyType aIdentifier,
        PvmiKvp*& aParameters,
        int& aNum_parameter_elements,
        PvmiCapabilityContext aContext)
{
    LOG_STACK_TRACE((0, "PvmiMIOAviWavFile::getParametersSync"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);

    aParameters = NULL;
    aNum_parameter_elements = 0;
    PVMFStatus status = PVMFFailure;

    if (pv_mime_strcmp(aIdentifier, OUTPUT_FORMATS_CAP_QUERY) == 0 ||
            pv_mime_strcmp(aIdentifier, OUTPUT_FORMATS_CUR_QUERY) == 0)
    {
        aNum_parameter_elements = 1;
        status = AllocateKvp(aParameters, OUTPUT_FORMATS_VALTYPE, aNum_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
        }
        else
        {
            aParameters[0].value.uint32_value = iSettings.iMediaFormat;

        }
    }
    else if (pv_mime_strcmp(aIdentifier, VIDEO_OUTPUT_WIDTH_CUR_QUERY) == 0)
    {
        aNum_parameter_elements = 1;
        status = AllocateKvp(aParameters, VIDEO_OUTPUT_WIDTH_CUR_VALUE, aNum_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        aParameters[0].value.uint32_value = iSettings.iFrameWidth;
    }
    else if (pv_mime_strcmp(aIdentifier, VIDEO_FRAME_ORIENTATION_CUR_QUERY) == 0)
    {
        aNum_parameter_elements = 1;
        status = AllocateKvp(aParameters, VIDEO_FRAME_ORIENTATION_CUR_VALUE, aNum_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        aParameters[0].value.uint8_value = iSettings.iPicBottomUp;

    }
    else if (pv_mime_strcmp(aIdentifier, VIDEO_OUTPUT_HEIGHT_CUR_QUERY) == 0)
    {
        aNum_parameter_elements = 1;
        status = AllocateKvp(aParameters, VIDEO_OUTPUT_HEIGHT_CUR_VALUE, aNum_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        aParameters[0].value.uint32_value = iSettings.iFrameHeight;
    }
    else if (pv_mime_strcmp(aIdentifier, VIDEO_OUTPUT_FRAME_RATE_CUR_QUERY) == 0)
    {
        aNum_parameter_elements = 1;
        status = AllocateKvp(aParameters, VIDEO_OUTPUT_FRAME_RATE_CUR_VALUE, aNum_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }

        aParameters[0].value.float_value = iSettings.iFrameRate;
    }
    else if (pv_mime_strcmp(aIdentifier, OUTPUT_TIMESCALE_CUR_QUERY) == 0)
    {
        aNum_parameter_elements = 1;
        status = AllocateKvp(aParameters, OUTPUT_TIMESCALE_CUR_VALUE, aNum_parameter_elements);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PVMFVideoEncPort::GetOutputParametersSync: Error - AllocateKvp failed. status=%d", status));
            return status;
        }
        else
        {
            switch (GetMediaTypeIndex(iSettings.iMediaFormat))
            {
                case PVMF_UNCOMPRESSED_AUDIO_FORMAT:
                case PVMF_COMPRESSED_AUDIO_FORMAT:
                    aParameters[0].value.uint32_value = (uint32)iSettings.iSamplingFrequency;
                    break;
                default:
                    aParameters[0].value.uint32_value = iSettings.iTimescale;
                    break;
            }
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOAviWavFile::releaseParameters(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int aNum_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aNum_elements);

    if (aParameters)
    {
        iAlloc.deallocate((OsclAny*)aParameters);
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::setContextParameters(PvmiMIOSession aSession,
        PvmiCapabilityContext& aContext,
        PvmiKvp* aParameters, int aNum_parameter_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNum_parameter_elements);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmiMIOAviWavFile::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
        int aNum_elements, PvmiKvp*& aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);
    PVMFStatus status = PVMFSuccess;
    aRet_kvp = NULL;

    for (int32 ii = 0; ii < aNum_elements; ii++)
    {
        status = VerifyAndSetParameter(&(aParameters[ii]), true);
        if (status != PVMFSuccess)
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::setParametersSync: Error - VerifiyAndSetParameter failed on parameter #%d", ii));
            aRet_kvp = &(aParameters[ii]);
            OSCL_LEAVE(OsclErrArgument);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFCommandId PvmiMIOAviWavFile::setParametersAsync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int aNum_elements,
        PvmiKvp*& aRet_kvp,
        OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNum_elements);
    OSCL_UNUSED_ARG(aRet_kvp);
    OSCL_UNUSED_ARG(aContext);
    OSCL_LEAVE(OsclErrNotSupported);
    return -1;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 PvmiMIOAviWavFile::getCapabilityMetric(PvmiMIOSession aSession)
{
    OSCL_UNUSED_ARG(aSession);
    return 0;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmiMIOAviWavFile::verifyParametersSync(PvmiMIOSession aSession,
        PvmiKvp* aParameters, int aNum_elements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNum_elements);
    return PVMFErrNotSupported;
}

////////////////////////////////////////////////////////////////////////////
//                            Private methods
////////////////////////////////////////////////////////////////////////////
PvmiMIOAviWavFile::PvmiMIOAviWavFile(uint32 aNumLoops, bool aRecordingMode, uint32 aStreamNo, OsclAny* aFileParser,
                                     FileFormatType aFileType, int32& arError)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmiMIOAviWavFile"),
        iCmdIdCounter(0),
        iPeer(NULL),
        iThreadLoggedOn(false),
        iDataEventCounter(0),
        iTotalNumFrames(0),
        iFileHeaderSize(0),
        iMilliSecondsPerDataEvent(0),
        iMicroSecondsPerDataEvent(0),
        iMediaBufferMemPool(NULL),
        iLogger(NULL),
        iState(STATE_IDLE),
        iStreamDuration(0),
        iBaseTimeStamp(0),
        iCurrentTimeStamp(0),
        iNextTimeStamp(0),
        iWaitingOnClock(false)
{
#if PROFILING_ON
    iNumEarlyFrames = 0;
    iNumLateFrames = 0;
    iTotalFrames = 0;
    iPercFramesDropped = 0;
    iMaxDataSize = 0;
    iMinDataSize = 0;
    iMaxFileReadTime = 0;
    iMinFileReadTime = 0;
    oDiagnosticsLogged = false;
#endif
    iSettings.iNumLoops = aNumLoops;
    iSettings.iStreamNumber = aStreamNo;
    iSettings.iRecModeSyncWithClock = aRecordingMode;
    arError = InitComp(aFileParser, aFileType);
    iMioClock = new OsclClock();
    iMioClock->SetClockTimebase(iClockTimeBase);
    uint32 start = 0;
    iMioClock->SetStartTime32(start, OSCLCLOCK_MSEC);

    iLogger = PVLogger::GetLoggerObject("PvmiMIOAviWavFile");
    iDiagnosticsLogger = PVLogger::GetLoggerObject("pvauthordiagnostics.mio.aviwav");

}

////////////////////////////////////////////////////////////////////////////
int32 PvmiMIOAviWavFile::InitComp(OsclAny* aFileParser, FileFormatType aFileType)
{

    switch (aFileType)
    {
        case FILE_FORMAT_AVI:
        {
            iPVAviFile = OSCL_STATIC_CAST(PVAviFile*, aFileParser);
            iPVWavFile = NULL;
            iSettings.iFrameDuration = iPVAviFile->GetFrameDuration();
            uint32 ii = iSettings.iStreamNumber;
            iStreamDuration = iPVAviFile->GetStreamDuration(ii);
            iPVAviFile->GetFormatSpecificInfo(ii, iFormatSpecificDataFrag);

            if (oscl_strstr((iPVAviFile->GetStreamMimeType(ii)).get_cstr(), "video"))
            {
                iSettings.iFrameHeight = iPVAviFile->GetHeight(iSettings.iPicBottomUp, ii);
                iSettings.iFrameWidth = iPVAviFile->GetWidth(ii);
                iSettings.iFrameRate = iPVAviFile->GetFrameRate(ii);
                iSettings.iTimescale = iPVAviFile->GetScale(ii);
                uint8 fmtType[4] = {0};
                uint32 size = 4;

                iPVAviFile->GetVideoFormatType((uint8*)fmtType, size, ii);
                fmtType[size] = '\0';
                /*@todo: add YUV*/
                if (!oscl_strncmp((char*)fmtType, "DIB ", size))
                {
                    BitmapInfoHhr* videoHdr = OSCL_STATIC_CAST(BitmapInfoHhr*, iFormatSpecificDataFrag.getMemFragPtr());

                    if (BITS_PER_SAMPLE12 == videoHdr->BiBitCount)
                    {
                        iSettings.iMediaFormat = PVMF_RGB12;
                        iSettings.iSampleSize = videoHdr->BiBitCount;
                    }
                    else if (BITS_PER_SAMPLE24 == videoHdr->BiBitCount)
                    {
                        iSettings.iMediaFormat = PVMF_RGB24;
                        iSettings.iSampleSize = videoHdr->BiBitCount;
                    }
                    else
                    {
                        return PVMFErrNotSupported;
                    }
                }
                else
                {
                    return PVMFErrNotSupported;
                }
            }

            if (oscl_strstr((iPVAviFile->GetStreamMimeType(ii)).get_cstr(), "audio"))
            {
                iSettings.iNumChannels = iPVAviFile->GetNumAudioChannels(ii);
                iSettings.iSamplingFrequency = iPVAviFile->GetFrameRate(ii);

                WaveFormatExStruct* audioHdr = OSCL_STATIC_CAST(WaveFormatExStruct*,
                                               iFormatSpecificDataFrag.getMemFragPtr());

                iSettings.iSampleSize = audioHdr->BitsPerSample;
                iSettings.iByteRate = audioHdr->AvgBytesPerSec;

                if (WAVE_FORMAT_PCM == audioHdr->FormatTag)
                {
                    if (BITS_PER_SAMPLE8 == audioHdr->BitsPerSample)
                    {
                        iSettings.iMediaFormat = PVMF_PCM8;
                    }
                    else if (BITS_PER_SAMPLE16 == audioHdr->BitsPerSample)
                    {
                        iSettings.iMediaFormat = PVMF_PCM16;
                    }
                    else
                    {
                        return PVMFErrNotSupported;
                    }
                }
                else
                {
                    return PVMFErrNotSupported;
                }
            }
        }
        break;

        case FILE_FORMAT_WAV:
        {
            iPVAviFile = NULL;
            iPVWavFile = OSCL_STATIC_CAST(PV_Wav_Parser*, aFileParser);
            PVWAVFileInfo wavFileInfo;
            iPVWavFile->RetrieveFileInfo(wavFileInfo);

            if ((PVWAV_ITU_G711_ALAW == wavFileInfo.AudioFormat)
                    || (PVWAV_ITU_G711_ULAW == wavFileInfo.AudioFormat))
            {
                if (iPVWavFile->SetOutputToUncompressedPCM())
                {
                    wavFileInfo.AudioFormat = PVWAV_PCM_AUDIO_FORMAT;
                    wavFileInfo.BitsPerSample = BITS_PER_SAMPLE16;
                    wavFileInfo.BytesPerSample = BITS_PER_SAMPLE16 / BYTE_COUNT;
                }
            }

            iSettings.iNumChannels = wavFileInfo.NumChannels;
            iSettings.iSamplingFrequency = wavFileInfo.SampleRate;
            iSettings.iSampleSize = wavFileInfo.BitsPerSample;
            iSettings.iByteRate = wavFileInfo.ByteRate;
            iStreamDuration = (1000000 / wavFileInfo.SampleRate) * wavFileInfo.NumSamples;

            if (BITS_PER_SAMPLE16 == wavFileInfo.BitsPerSample)
            {
                iSettings.iMediaFormat = PVMF_PCM16;
            }
            else if (BITS_PER_SAMPLE8 == wavFileInfo.BitsPerSample)
            {
                iSettings.iMediaFormat = PVMF_PCM8;
            }
            else
            {
                return PVMFErrNotSupported;
            }

        }
        break;

        default:
            return PVMFErrNotSupported;

    } // end switch

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmiMIOAviWavFile::Run()
{
    if (!iCmdQueue.empty())
    {
        PvmiMIOAviWavFileCmd cmd = iCmdQueue[0];
        iCmdQueue.erase(iCmdQueue.begin());

        switch (cmd.iType)
        {

            case CMD_INIT:
                DoRequestCompleted(cmd, DoInit());
                break;

            case CMD_START:
                DoRequestCompleted(cmd, DoStart());
                break;

            case CMD_PAUSE:
                DoRequestCompleted(cmd, DoPause());
                break;

            case CMD_FLUSH:
                DoRequestCompleted(cmd, DoFlush());
                break;

            case CMD_RESET:
                DoRequestCompleted(cmd, DoReset());
                break;

            case CMD_STOP:
                DoRequestCompleted(cmd, DoStop());
                break;

            case DATA_EVENT:
                DoRead();
                break;

            case CMD_QUERY_UUID:
            case CMD_QUERY_INTERFACE:
                DoRequestCompleted(cmd, PVMFSuccess);
                break;

            case CMD_CANCEL_ALL_COMMANDS:
            case CMD_CANCEL_COMMAND:
                DoRequestCompleted(cmd, PVMFFailure);
                break;

            default:
                break;
        }
    }

    if (!iCmdQueue.empty())
    {
        // Run again if there are more things to process
        RunIfNotReady();
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFCommandId PvmiMIOAviWavFile::AddCmdToQueue(PvmiMIOAviWavFileCmdType aType,
        const OsclAny* aContext, OsclAny* aData1)
{
    if (DATA_EVENT == aType)
    {
        OSCL_LEAVE(OsclErrArgument);
    }

    PvmiMIOAviWavFileCmd cmd;
    cmd.iType = aType;
    cmd.iContext = OSCL_STATIC_CAST(OsclAny*, aContext);
    cmd.iData1 = aData1;
    cmd.iId = iCmdIdCounter;
    ++iCmdIdCounter;
    iCmdQueue.push_back(cmd);
    RunIfNotReady();
    return cmd.iId;
}

////////////////////////////////////////////////////////////////////////////
void PvmiMIOAviWavFile::AddDataEventToQueue(uint32 aMicroSecondsToEvent)
{
    PvmiMIOAviWavFileCmd cmd;
    cmd.iType = DATA_EVENT;
    iCmdQueue.push_back(cmd);
    RunIfNotReady(aMicroSecondsToEvent);
}

////////////////////////////////////////////////////////////////////////////
void PvmiMIOAviWavFile::DoRequestCompleted(const PvmiMIOAviWavFileCmd& aCmd, PVMFStatus aStatus, OsclAny* aEventData)
{
    PVMFCmdResp response(aCmd.iId, aCmd.iContext, aStatus, aEventData);

    for (uint32 ii = 0; ii < iObservers.size(); ii++)
    {
        iObservers[ii]->RequestCompleted(response);
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::DoInit()
{
    if (STATE_INITIALIZED == iState)
    {
        return PVMFSuccess;
    }
    iDataEventCounter = 0;
    // Create memory pool for the media data, using the maximum frame size found earlier
    int32 err = 0;
    OSCL_TRY(err,
             if (iMediaBufferMemPool)
{
    OSCL_TEMPLATED_DELETE(iMediaBufferMemPool, OsclMemPoolFixedChunkAllocator, OsclMemPoolFixedChunkAllocator);
        iMediaBufferMemPool = NULL;
    }
    iMediaBufferMemPool = OSCL_NEW(OsclMemPoolFixedChunkAllocator,
                                   (PVMIOFILEIN_MEDIADATA_POOLNUM));

    if (!iMediaBufferMemPool)
{
    OSCL_LEAVE(OsclErrNoMemory);
    }

            );

    OSCL_FIRST_CATCH_ANY(err, return PVMFErrNoMemory);

    //set chunk size
    uint32 dataSize = GetDataSize();
    //add bytes in 1 msec
    dataSize += (iSettings.iSampleSize / BYTE_COUNT * iSettings.iSamplingFrequency / 1000);
    iSettings.iDataBufferSize = dataSize;

    uint8* data = (uint8*)iMediaBufferMemPool->allocate(dataSize);
    iMediaBufferMemPool->deallocate(data);
    iState = STATE_INITIALIZED;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::DoStart()
{
    iState = STATE_STARTED;
    AddDataEventToQueue(0);
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::DoPause()
{
    iState = STATE_PAUSED;
    iMioClock->Pause();
    return PVMFSuccess;
}

PVMFStatus PvmiMIOAviWavFile::DoReset()
{

#if PROFILING_ON
    if (!oDiagnosticsLogged)
        LogDiagnostics();
#endif

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::DoFlush()
{
    // This method should stop capturing media data but continue to send captured
    // media data that is already in buffer and then go to stopped state.
    // However, in this case of file input we do not have such a buffer for
    // captured data, so this behaves the same way as stop.
    return DoStop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::DoStop()
{
    iDataEventCounter = 0;
    iState = STATE_STOPPED;
    iMioClock->Stop();

#if PROFILING_ON
    if (!oDiagnosticsLogged)
        LogDiagnostics();
#endif

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
uint32 PvmiMIOAviWavFile::GetDataSize()
{
    uint32 size = 0;
    if (iPVAviFile != NULL)
    {
        size = iPVAviFile->GetStreamSuggestedBufferSize(iSettings.iStreamNumber);
    }
    else if (iPVWavFile != NULL)
    {
        //number of samples in data buffer
        uint32 numSamples = (uint32)(iSettings.iSamplingFrequency * PVWAV_MSEC_PER_BUFFER) / 1000;
        size = numSamples * iSettings.iNumChannels * (iSettings.iSampleSize / BYTE_COUNT); // in bytes
    }
    else
    {
        return 0;
    }

    return size;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::GetMediaData(uint8* aData, uint32& aDataSize, uint32& aTimeStamp)
{

    // Read data from file
    if (iPVAviFile != NULL)
    {
        PV_AVI_FILE_PARSER_ERROR_TYPE error = PV_AVI_FILE_PARSER_SUCCESS;
        error = iPVAviFile->GetNextStreamMediaSample(iSettings.iStreamNumber, aData, aDataSize, aTimeStamp);

        if (error != PV_AVI_FILE_PARSER_SUCCESS)
        {
            if (PV_AVI_FILE_PARSER_EOS_REACHED == error)
            {
                // Loop or report end of data now...
                if (iSettings.iNumLoops)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                    (0, "PvmiMIOAviWavFile::LoopInputFile"));

                    iPVAviFile->Reset(iSettings.iStreamNumber);
                    iSettings.iNumLoops--;
                    iBaseTimeStamp = iNextTimeStamp;
                    //retrieve sample again
                    PV_AVI_FILE_PARSER_ERROR_TYPE error = PV_AVI_FILE_PARSER_SUCCESS;
                    error = iPVAviFile->GetNextStreamMediaSample(iSettings.iStreamNumber, aData, aDataSize, aTimeStamp);

                    if (error != PV_AVI_FILE_PARSER_SUCCESS)
                    {
                        return PVMFFailure;
                    }
                    else
                    {
                        return PVMFSuccess;
                    }
                }
                else
                {
                    return PVMFInfoEndOfData;
                }
            }
            else
            {
                return PVMFFailure;
            }
        }
    }
    else  //WAV file
    {
        uint32 samplesRead = 0;
        uint32 numSamples = (uint32)(iSettings.iSamplingFrequency * PVWAV_MSEC_PER_BUFFER) / 1000;
        PVWavParserReturnCode retcode = iPVWavFile->GetPCMData(aData, aDataSize, numSamples, samplesRead);
        if (PVWAVPARSER_OK == retcode)
        {
            if (samplesRead > 0)
            {
                aDataSize = samplesRead * iSettings.iNumChannels * (iSettings.iSampleSize / BYTE_COUNT);
            }

            aTimeStamp = iCurrentTimeStamp + PVWAV_MSEC_PER_BUFFER;
        }
        else if (PVWAVPARSER_END_OF_FILE == retcode)
        {
            if (iSettings.iNumLoops)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PvmiMIOAviWavFile::LoopInputFile Not Supported"));

                iPVWavFile->SeekPCMSample(0);
                iSettings.iNumLoops--;
                PVWavParserReturnCode retcode = iPVWavFile->GetPCMData(aData, aDataSize, numSamples, samplesRead);
                if (PVWAVPARSER_OK == retcode)
                {
                    if (samplesRead > 0)
                    {
                        aDataSize = samplesRead * iSettings.iNumChannels * (iSettings.iSampleSize / BYTE_COUNT);
                    }

                    aTimeStamp = iCurrentTimeStamp + PVWAV_MSEC_PER_BUFFER;
                    return PVMFSuccess;
                }
                else
                {
                    return PVMFFailure;
                }
            }
            else
            {
                return PVMFInfoEndOfData;
            }
        }
        else
        {
            return PVMFFailure;
        }
    }

    return PVMFSuccess;
}
////////////////////////////////////////////////////////////////////////////
void PvmiMIOAviWavFile::GetNextTimeStamp(uint32 aDataSize)
{
    if (iPVAviFile != NULL)
    {
        if (oscl_strstr((iPVAviFile->GetStreamMimeType(iSettings.iStreamNumber)).get_cstr(), "video"))
        {
            iNextTimeStamp = iCurrentTimeStamp + (iPVAviFile->GetFrameDuration() / 1000); //in msec
        }
        else if (oscl_strstr((iPVAviFile->GetStreamMimeType(iSettings.iStreamNumber)).get_cstr(), "audio"))
        {
            if (aDataSize == 0)
            {
                uint32 ii = 0;
            }
            uint32 numSamples = aDataSize * BYTE_COUNT / iSettings.iSampleSize;
            iNextTimeStamp = iCurrentTimeStamp + (numSamples * 1000) / iSettings.iSamplingFrequency;
        }
    }
    else  //WAV File
    {
        iNextTimeStamp = iCurrentTimeStamp + PVWAV_MSEC_PER_BUFFER;
    }


}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::DoRead()
{
    // Just copy from PVMFAviFileNode::HandleEventPortActivity.  The only difference
    // is that data buffer is allocated by calling iMediaBufferMemPool->allocate(bytesToRead)
    // and there's no need to wrap it in a PVMFSharedMediaDataPtr.  Also, you'll need to
    // keep track of the data pointer and the write command id received from peer->writeAsync
    // and put it in the iSentMediaData queue

    if (iState != STATE_STARTED)
    {
        return PVMFSuccess;
    }

    uint32 dataSize = 0;
    uint32 timeStamp = 0;
    uint8* data = NULL;
    uint32 writeAsyncID = 0;

    PVMFStatus error = PVMFSuccess;

    dataSize = iSettings.iDataBufferSize;

    if (dataSize <= 0)
    {
        return PVMFErrArgument;
    }

    // for realtime recording, read new media data only if not iWaitingOnClock.
    // otherwise process previously read data
    int32 err = 0;

    //process new data only if previous data has been send to MIO node.
    if (!iWaitingOnClock)
    {
        // Create new media data buffer
        OSCL_TRY(err, data = (uint8*)iMediaBufferMemPool->allocate(dataSize););

        if (err)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PvmiMIOAviWavFile::No buffer available, wait till next data event"));

            if (iSettings.iRecModeSyncWithClock)
                CalcMicroSecPerDataEvent(dataSize);
            else
                iMicroSecondsPerDataEvent = 0;

            AddDataEventToQueue(iMicroSecondsPerDataEvent);
            return PVMFSuccess;
        }

        iData = NULL;
        iDataSize = 0;
        iTimeStamp = 0;

#if PROFILING_ON
        uint32 start = OsclTickCount::TickCount();
#endif
        //read media data
        error = GetMediaData(data, dataSize, timeStamp);

#if PROFILING_ON
        uint32 stop = OsclTickCount::TickCount();
        uint32 freadTime = OsclTickCount::TicksToMsec(stop - start);

        if (error == PVMFSuccess)
        {
            if (iMaxDataSize < dataSize)
            {
                iMaxDataSize = dataSize;
            }
            if ((iMinDataSize > dataSize) || (0 == iMinDataSize))
            {
                iMinDataSize = dataSize;
            }
            if (iMaxFileReadTime < freadTime)
            {
                iMaxFileReadTime = freadTime;
            }
            if ((iMinFileReadTime > freadTime) || (0 == iMinFileReadTime))
            {
                iMinFileReadTime = freadTime;
            }
        }
#endif
        if (error != PVMFSuccess)
        {
            if (PVMFInfoEndOfData == error) //EOS Reached
            {
                //free the allocated data buffer
                iMediaBufferMemPool->deallocate(data);
                data = NULL;

                PvmiMediaXferHeader data_hdr;
                data_hdr.seq_num = iDataEventCounter - 1;
                data_hdr.timestamp = iNextTimeStamp;
                data_hdr.flags = 0;
                data_hdr.duration = 0;
                data_hdr.stream_id = iSettings.iStreamNumber;
                dataSize = 0;
                //send EOS information to MIO Node
                OSCL_TRY(error, writeAsyncID = iPeer->writeAsync(PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION, PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM,
                                               NULL, dataSize, data_hdr););

                if (error)
                {
                    if (iSettings.iRecModeSyncWithClock)
                        CalcMicroSecPerDataEvent(dataSize);
                    else
                        iMicroSecondsPerDataEvent = 0;

                    //some error occured, retry sending EOS next time.
                    AddDataEventToQueue(iMicroSecondsPerDataEvent);
                    return PVMFSuccess;
                }

                //EOS message was sent so PAUSE MIO Component.
                AddCmdToQueue(CMD_PAUSE, NULL);

                return PVMFSuccess;
            }
            else
            {
                //free the allocated data buffer
                iMediaBufferMemPool->deallocate(data);
                data = NULL;
                AddCmdToQueue(CMD_STOP, NULL);
                return error;
            }
        }

#if PROFILING_ON
        iTotalFrames++;
#endif
        timeStamp += iBaseTimeStamp;

        if ((iBaseTimeStamp > 0) && timeStamp < iBaseTimeStamp)
        {
            //timestamp rollover. reset base timestamp
            iBaseTimeStamp = 0;
        }

        iCurrentTimeStamp = timeStamp;

        if (iSettings.iRecModeSyncWithClock)
        {

            uint64 clockTime64 = 0;
            uint64 clockTimeBase64 = 0;
            uint32 clockTime32 = 0;
            uint64 adjustTime64 = 0;

            if (iMioClock->GetState() != OsclClock::RUNNING)
            {
                iMioClock->SetStartTime32(timeStamp, OSCLCLOCK_MSEC);
                iMioClock->Start();
            }


            //Compare with Clock time to pass data forward.
            iMioClock->GetCurrentTime64(clockTime64, OSCLCLOCK_MSEC, clockTimeBase64);
            clockTime32 = Oscl_Int64_Utils::get_uint64_lower32(clockTime64);

            Oscl_Int64_Utils::set_uint64(adjustTime64, 0, iCurrentTimeStamp);

            if (adjustTime64 > clockTime64)
            {
                uint32 delta = iCurrentTimeStamp - clockTime32;
                //store data info to be send forward in next callback.
                iData = data;
                iDataSize = dataSize;
                iTimeStamp = timeStamp;
                AddDataEventToQueue(delta * 1000); //delta in microseconds.
                iWaitingOnClock = true;
#if PROFILING_ON
                iNumEarlyFrames++;
#endif
                return PVMFSuccess;
            }

#if PROFILING_ON
            if (adjustTime64 < clockTime64)
            {
                iNumLateFrames++;
            }
#endif

        }	// 	if (iSettings.iRecModeSyncWithClock)

        GetNextTimeStamp(dataSize);

        if (iSettings.iNumLoops && (iCurrentTimeStamp == iNextTimeStamp))
        {
            //could be due to small data chunk in the end of the file.
            //If looping, store this data chunk and read more data from file
            uint8* newdata = data + dataSize;
            uint32 newDataSize = iSettings.iDataBufferSize - dataSize;
            uint32 numLoops = iSettings.iNumLoops;

            //read more data
            error = GetMediaData(newdata, newDataSize, timeStamp);

            if (PVMFSuccess == error)
            {
                dataSize += newDataSize;
                if (numLoops > iSettings.iNumLoops) //received EOS recalculate base timestamp
                {
                    GetNextTimeStamp(dataSize);
                    iBaseTimeStamp = iNextTimeStamp;

                    //use the timestamp of last sample (in the end of file).
                    timeStamp = iCurrentTimeStamp;
                }
            }
            else
            {
                //if read fails, do nothing here, send the original data
                // and hope that we will be able to read more data next time.
                // Set dummy timestamp for next sample.

                iNextTimeStamp += 1;
            }

        }  //if (iSettings.iNumLoops && (iCurrentTimeStamp == iNextTimeStamp))


    }	//	if (!iWaitingOnClock)

    iWaitingOnClock = false;

    if (iData != NULL)
    {
        data = iData;
        dataSize = iDataSize;
        timeStamp = iTimeStamp;
    }

    // send data to Peer & store the id
    PvmiMediaXferHeader data_hdr;
    data_hdr.seq_num = iDataEventCounter - 1;
    data_hdr.timestamp = iCurrentTimeStamp;
    data_hdr.flags = 0;
    data_hdr.duration = 0;
    data_hdr.stream_id = iSettings.iStreamNumber;


    if (!iPeer)
    {
        return PVMFSuccess;
    }

    OSCL_TRY(err, writeAsyncID = iPeer->writeAsync(0, 0, data, dataSize, data_hdr););
    if (!err)
    {
        // Save the id and data pointer on iSentMediaData queue for writeComplete call
        PvmiMIOAviWavFileMediaData sentData;
        sentData.iId = writeAsyncID;
        sentData.iData = data;
        iSentMediaData.push_back(sentData);
    }
    else
    {
        iMediaBufferMemPool->deallocate(data);
    }


    iMicroSecondsPerDataEvent = 0;
    // Queue the next data event
    AddDataEventToQueue(iMicroSecondsPerDataEvent);

    return PVMFSuccess;
}

PVMFStatus PvmiMIOAviWavFile::CalcMicroSecPerDataEvent(uint32 aDataSize)
{
    //calculate time for a buffer to fill
    switch (iSettings.iMediaFormat)
    {
            //case PVMF_YUV422:
        case PVMF_YUV420:
        {
            //calculate time for a buffer to fill
            iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);

            iMicroSecondsPerDataEvent = (int32)(1000000 / iSettings.iFrameRate);
        }
        break;

        case PVMF_RGB16:
        case PVMF_RGB24:
        {
            //calculate time for a buffer to fill
            iMilliSecondsPerDataEvent = (int32)(1000 / iSettings.iFrameRate);
            iMicroSecondsPerDataEvent = (int32)(1000000 / iSettings.iFrameRate);
        }
        break;

        case PVMF_PCM16:
        case PVMF_PCM8:
        {
            OsclFloat chunkrate = (OsclFloat)((OsclFloat)iSettings.iByteRate / (OsclFloat)aDataSize);
            iMilliSecondsPerDataEvent = (uint32)(1000 / chunkrate);
            iMicroSecondsPerDataEvent = iMilliSecondsPerDataEvent * 1000;
        }
        break;

        default:
            return PVMFErrArgument;

    } // end switch

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::AllocateKvp(PvmiKvp*& aKvp, PvmiKeyType aKey, int32 aNumParams)
{
    LOG_STACK_TRACE((0, "PvmiMIOAviWavFile::AllocateKvp"));
    uint8* buf = NULL;
    uint32 keyLen = oscl_strlen(aKey) + 1;
    int32 err = 0;

    OSCL_TRY(err,
             buf = (uint8*)iAlloc.allocate(aNumParams * (sizeof(PvmiKvp) + keyLen));
             if (!buf)
{
    OSCL_LEAVE(OsclErrNoMemory);
    }
            );

    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmiMIOAviWavFile::AllocateKvp: Error - kvp allocation failed"));
                         return PVMFErrNoMemory;
                        );

    int32 ii = 0;
    PvmiKvp* curKvp = aKvp = OSCL_PLACEMENT_NEW(buf, PvmiKvp);
    buf += sizeof(PvmiKvp);
    for (ii = 1; ii < aNumParams; ii++)
    {
        curKvp += ii;
        curKvp = OSCL_PLACEMENT_NEW(buf, PvmiKvp);
        buf += sizeof(PvmiKvp);
    }

    for (ii = 0; ii < aNumParams; ii++)
    {
        aKvp[ii].key = (char*)buf;
        oscl_strncpy(aKvp[ii].key, aKey, keyLen);
        buf += keyLen;
    }

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmiMIOAviWavFile::VerifyAndSetParameter(PvmiKvp* aKvp, bool aSetParam)
{
    LOG_STACK_TRACE((0, "PvmiMIOAviWavFile::VerifyAndSetParameter: aKvp=0x%x, aSetParam=%d", aKvp, aSetParam));

    if (!aKvp)
    {
        LOG_ERR((0, "PvmiMIOAviWavFile::VerifyAndSetParameter: Error - Invalid key-value pair"));
        return PVMFFailure;
    }

    if (pv_mime_strcmp(aKvp->key, OUTPUT_FORMATS_VALTYPE) == 0)
    {
        if (aKvp->value.uint32_value == iSettings.iMediaFormat)
        {
            return PVMFSuccess;
        }
        else
        {
            LOG_ERR((0, "PvmiMIOAviWavFile::VerifyAndSetParameter: Error - Unsupported format %d",
                     aKvp->value.uint32_value));
            return PVMFFailure;
        }
    }

    LOG_ERR((0, "PvmiMIOAviWavFile::VerifyAndSetParameter: Error - Unsupported parameter"));
    return PVMFFailure;
}

void PvmiMIOAviWavFile::LogDiagnostics()
{
#if PROFILING_ON
    oDiagnosticsLogged = true;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_DEBUG,
                    (0, "PvmiMIOAviWavFile Stats: Stream :%s\n",
                     (iPVAviFile->GetStreamMimeType(iSettings.iStreamNumber)).get_cstr()));

    uint32 framerate = 0;

    if (iCurrentTimeStamp > 0)
    {
        framerate = (iTotalFrames * 1000) / iCurrentTimeStamp;
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_DEBUG,
                    (0, "PvmiMIOAviWavFile Stats: File read duration(Max:%d, Min:%d), Data Size read(Max:%d, Min:%d), Total frames send: %d, Frame Rate: %d\n", iMaxFileReadTime, iMinFileReadTime, iMaxDataSize, iMinDataSize, iTotalFrames, framerate));

    //Log status of early and late frames for real time authoring
    if (iSettings.iRecModeSyncWithClock)
    {

        if (iTotalFrames > 0)
        {
            iPercFramesDropped = (iNumLateFrames * 100) / iTotalFrames;
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_DEBUG,
                        (0, "PvmiMIOAviWavFile Stats: Num Early frames:%d, Num late frames:%d, Total Frames:%d, percent Late frames:%d\n", iNumEarlyFrames, iNumLateFrames, iTotalFrames, iPercFramesDropped));
    }
#endif
}

