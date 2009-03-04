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
#ifndef PVMI_MIO_AVIFILE_H_INCLUDED
#define PVMI_MIO_AVIFILE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

#ifndef OSCLCONFIG_IO_H_INCLUDED
#include "osclconfig_io.h"
#endif

#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif

#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif

#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif

#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif

#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif

#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef PVMI_MIO_CONTROL_H_INCLUDED
#include "pvmi_mio_control.h"
#endif

#ifndef PVMI_MEDIA_TRANSFER_H_INCLUDED
#include "pvmi_media_transfer.h"
#endif

#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif

#ifndef PVMF_SIMPLE_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_simple_media_buffer.h"
#endif

#ifndef PVMI_MIO_AVIFILE_FACTORY_H_INCLUDED
#include "pvmi_mio_avi_wav_file_factory.h"
#endif

#ifndef PVWAVFILEPARSER_H_INCLUDED
#include "pvwavfileparser.h"
#endif

#ifndef PVLOGGER_FILE_APPENDER_H_INCLUDED
#include "pvlogger_file_appender.h"
#endif



// Forward declaration
class PvmiMIOAviWavFileSettings;

/**
 * Enumerated list of asychronous commands for PvmiMIOAviWavFile
 */
typedef enum
{
    CMD_QUERY_UUID,
    CMD_QUERY_INTERFACE,
    CMD_INIT,
    CMD_START,
    CMD_PAUSE,
    CMD_FLUSH,
    CMD_STOP,
    CMD_CANCEL_ALL_COMMANDS,
    CMD_CANCEL_COMMAND,
    CMD_RESET,
    DATA_EVENT,
    INVALID_CMD
} PvmiMIOAviWavFileCmdType;


const uint32 BITS_PER_SAMPLE8		= 8;
const uint32 BITS_PER_SAMPLE12		= 12;
const uint32 BITS_PER_SAMPLE16		= 16;
const uint32 BITS_PER_SAMPLE24		= 24;
const uint32 PVWAV_MSEC_PER_BUFFER	= 100;
const uint32 BYTE_COUNT				= 8;
const uint32 DATA_BUFF_THRESHOLD	= 1;

#define PROFILING_ON (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_PROF)

////////////////////////////////////////////////////////////////////////////
/**
 * Class containing information for a command or data event
 */
class PvmiMIOAviWavFileCmd
{
    public:
        PvmiMIOAviWavFileCmd()
        {
            iId = 0;
            iType = INVALID_CMD;
            iContext = NULL;
            iData1 = NULL;
        }

        PvmiMIOAviWavFileCmd(const PvmiMIOAviWavFileCmd& aCmd)
        {
            Copy(aCmd);
        }

        ~PvmiMIOAviWavFileCmd() {}

        PvmiMIOAviWavFileCmd& operator=(const PvmiMIOAviWavFileCmd& aCmd)
        {
            Copy(aCmd);
            return (*this);
        }

        PVMFCommandId iId;  /** ID assigned to this command */
        int32 iType;       /** PvmiMIOAviWavFileCmdType value */
        OsclAny* iContext; /** Other data associated with this command */
        OsclAny* iData1;  /** Other data associated with this command */

    private:

        void Copy(const PvmiMIOAviWavFileCmd& aCmd)
        {
            iId = aCmd.iId;
            iType = aCmd.iType;
            iContext = aCmd.iContext;
            iData1 = aCmd.iData1;
        }
};

class PvmiMIOAviWavFileMediaData
{
    public:
        PvmiMIOAviWavFileMediaData()
        {
            iId = 0;
            iData = NULL;
        }

        PvmiMIOAviWavFileMediaData(const PvmiMIOAviWavFileMediaData& aData)
        {
            iId = aData.iId;
            iData = aData.iData;
        }

        PVMFCommandId iId;
        OsclAny* iData;
};

class PvmiMIOAviWavFile : public OsclTimerObject,
            public PvmiMIOControl,
            public PvmiMediaTransfer,
            public PvmiCapabilityAndConfig
{
    public:
        PvmiMIOAviWavFile(uint32 aNumLoops, bool aRecordingMode, uint32 aStreamNo, OsclAny* aFileParser, FileFormatType aFileType, int32& arError);
        virtual ~PvmiMIOAviWavFile();

        // Pure virtuals from PvmiMIOControl
        OSCL_IMPORT_REF PVMFStatus connect(PvmiMIOSession& aSession, PvmiMIOObserver* aObserver);
        OSCL_IMPORT_REF PVMFStatus disconnect(PvmiMIOSession aSession);
        OSCL_IMPORT_REF PvmiMediaTransfer* createMediaTransfer(PvmiMIOSession& aSession,
                PvmiKvp* aRead_formats = NULL,
                int32 aRead_flags = 0,
                PvmiKvp* aWrite_formats = NULL,
                int32 aWrite_flags = 0);
        OSCL_IMPORT_REF void deleteMediaTransfer(PvmiMIOSession& aSession,
                PvmiMediaTransfer* aMedia_transfer);
        OSCL_IMPORT_REF PVMFCommandId QueryUUID(const PvmfMimeString& aMimeType,
                                                Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                                bool aExactUuidsOnly = false,
                                                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryInterface(const PVUuid& aUuid,
                PVInterface*& aInterfacePtr,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Init(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Start(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Reset(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Pause(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Flush(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId DiscardData(PVMFTimestamp aTimestamp, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId DiscardData(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Stop(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelCommand(PVMFCommandId aCmdId, const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelAllCommands(const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF void ThreadLogon();
        OSCL_IMPORT_REF void ThreadLogoff();

        // Pure virtuals from PvmiMediaTransfer
        OSCL_IMPORT_REF void setPeer(PvmiMediaTransfer* aPeer);
        OSCL_IMPORT_REF void useMemoryAllocators(OsclMemAllocator* aWrite_alloc = NULL);
        OSCL_IMPORT_REF PVMFCommandId writeAsync(uint8 aFormat_type, int32 aFormat_index,
                uint8* aData, uint32 aData_len,
                const PvmiMediaXferHeader& aData_header_info,
                OsclAny* aContext = NULL);
        OSCL_IMPORT_REF void writeComplete(PVMFStatus aStatus, PVMFCommandId aWrite_cmd_id,
                                           OsclAny* aContext);
        OSCL_IMPORT_REF PVMFCommandId readAsync(uint8* aData, uint32 aMax_data_len, OsclAny* aContext = NULL,
                                                int32* aFormats = NULL, uint16 aNum_formats = 0);
        OSCL_IMPORT_REF void readComplete(PVMFStatus aStatus, PVMFCommandId aRead_cmd_id,
                                          int32 aFormat_index,
                                          const PvmiMediaXferHeader& aData_header_info,
                                          OsclAny* aContext);
        OSCL_IMPORT_REF void statusUpdate(uint32 aStatus_flags);
        OSCL_IMPORT_REF void cancelCommand(PVMFCommandId aCmdId);
        OSCL_IMPORT_REF void cancelAllCommands();

        // Pure virtuals from PvmiCapabilityAndConfig
        OSCL_IMPORT_REF void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver);
        OSCL_IMPORT_REF PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                PvmiKvp*& aParameters, int& aNum_parameter_elements,
                PvmiCapabilityContext aContext);
        OSCL_IMPORT_REF PVMFStatus releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters,
                int aNum_elements);
        OSCL_IMPORT_REF void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        OSCL_IMPORT_REF void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                PvmiKvp* aParameters, int aNum_parameter_elements);
        OSCL_IMPORT_REF void DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        OSCL_IMPORT_REF void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                               int aNum_elements, PvmiKvp * & aRet_kvp);
        OSCL_IMPORT_REF PVMFCommandId setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                int aNum_elements, PvmiKvp*& aRet_kvp,
                OsclAny* aContext = NULL);
        OSCL_IMPORT_REF uint32 getCapabilityMetric(PvmiMIOSession aSession);
        OSCL_IMPORT_REF PVMFStatus verifyParametersSync(PvmiMIOSession aSession,
                PvmiKvp* aParameters, int aNum_elements);
        OSCL_IMPORT_REF uint32 getStreamDuration()
        {
            return iStreamDuration;
        }

    private:

        void Run();
        int32 InitComp(OsclAny* aFileParser, FileFormatType aFileType);

        PVMFCommandId AddCmdToQueue(PvmiMIOAviWavFileCmdType aType, const OsclAny* aContext, OsclAny* aData1 = NULL);
        void AddDataEventToQueue(uint32 aMicroSecondsToEvent);
        void DoRequestCompleted(const PvmiMIOAviWavFileCmd& aCmd, PVMFStatus aStatus, OsclAny* aEventData = NULL);

        PVMFStatus DoInit();
        PVMFStatus DoStart();
        PVMFStatus DoReset();
        PVMFStatus DoPause();
        PVMFStatus DoFlush();
        PVMFStatus DoStop();
        PVMFStatus DoRead();

        PVMFStatus GetMediaData(uint8* aData, uint32& aDataSize, uint32& aTimeStamp);
        uint32 GetDataSize();
        PVMFStatus CalcMicroSecPerDataEvent(uint32 aDataSize);

        /**
         * Allocate a specified number of key-value pairs and set the keys
         *
         * @param aKvp Output parameter to hold the allocated key-value pairs
         * @param aKey Key for the allocated key-value pairs
         * @param aNumParams Number of key-value pairs to be allocated
         * @return Completion status
         */
        PVMFStatus AllocateKvp(PvmiKvp*& aKvp, PvmiKeyType aKey, int32 aNumParams);

        /**
         * Verify one key-value pair parameter against capability of the port and
         * if the aSetParam flag is set, set the value of the parameter corresponding to
         * the key.
         *
         * @param aKvp Key-value pair parameter to be verified
         * @param aSetParam If true, set the value of parameter corresponding to the key.
         * @return PVMFSuccess if parameter is supported, else PVMFFailure
         */
        PVMFStatus VerifyAndSetParameter(PvmiKvp* aKvp, bool aSetParam = false);
        void GetNextTimeStamp(uint32 aDataSize);
        void LogDiagnostics();

        // Command queue
        uint32 iCmdIdCounter;
        Oscl_Vector<PvmiMIOAviWavFileCmd, OsclMemAllocator> iCmdQueue;

        // PvmiMIO sessions
        Oscl_Vector<PvmiMIOObserver*, OsclMemAllocator> iObservers;

        PvmiMediaTransfer*  iPeer;

        // Thread logon
        bool iThreadLoggedOn;

        // Input file
        PvmiMIOAviWavFileSettings iSettings;

        // Counter and size info for file read
        PVAviFile* iPVAviFile;
        PV_Wav_Parser* iPVWavFile;

        PVAviFileMainHeaderStruct    iAviFileHeader;
        PVAviFileStreamHeaderStruct  iAviFileStreamHeader;
        OsclRefCounterMemFrag        iFormatSpecificDataFrag;

        int32 iDataEventCounter;
        int32 iTotalNumFrames;
        int32 iFileHeaderSize;
        // Timing
        int32 iMilliSecondsPerDataEvent;
        int32 iMicroSecondsPerDataEvent;

        // Allocator for simple media data buffer
        OsclMemAllocator iAlloc;
        OsclMemPoolFixedChunkAllocator* iMediaBufferMemPool;

        Oscl_Vector<PvmiMIOAviWavFileMediaData, OsclMemAllocator> iSentMediaData;

        // Logger
        PVLogger* iLogger;
        PVLogger* iDiagnosticsLogger;

        // State machine
        enum PvmiMIOAviWavFileState
        {
            STATE_IDLE,
            STATE_INITIALIZED,
            STATE_STARTED,
            STATE_FLUSHING,
            STATE_PAUSED,
            STATE_STOPPED
        };

        PvmiMIOAviWavFileState iState;
        OsclClock* iMioClock;
        OsclTimebase_Tickcount iClockTimeBase;
        bool iWaitingOnClock;
        uint8* iData;
        uint32 iDataSize;
        uint32 iTimeStamp;
        uint32 iStreamDuration ;  // in microsec
        uint32 iBaseTimeStamp;
        uint32 iCurrentTimeStamp;
        uint32 iNextTimeStamp;

#if PROFILING_ON
        //for diagnostics purpose
        uint32 iNumEarlyFrames;
        uint32 iNumLateFrames;
        uint32 iTotalFrames;
        uint32 iPercFramesDropped;
        uint32 iMaxDataSize;
        uint32 iMinDataSize;
        uint32 iMaxFileReadTime;
        uint32 iMinFileReadTime;
        bool   oDiagnosticsLogged;
#endif

};

#endif // PVMI_MIO_AVIFILE_H_INCLUDED
