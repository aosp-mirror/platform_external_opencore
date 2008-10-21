/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "AudioMIO"
#include <utils/Log.h>

#include "android_audio_mio.h"
#include "pvlogger.h"
#include "pv_mime_string_utils.h"
#include "oscl_snprintf.h"
#include "oscl_mem.h"
#include "oscl_dll.h"
#include "oscl_mem.h"

#include <sys/prctl.h>

#include <utils/Timers.h>
#include <sys/resource.h>
#include <limits.h>

#include <utils/threads.h>

#include <media/AudioTrack.h>

using namespace android;

// depth of buffer/command queues in MIO
static const int kCommandQueueDepth = 10;

// Define entry point for this DLL
OSCL_DLL_ENTRY_POINT_DEFAULT()

// Audio Media IO component base class implementation
OSCL_EXPORT_REF AndroidAudioMIO::AndroidAudioMIO(const char* name)
    : OsclTimerObject(OsclActiveObject::EPriorityNominal, name),
    iWriteCompleteAO(NULL)
{
    initData();
}

OSCL_EXPORT_REF AndroidAudioMIO::~AndroidAudioMIO()
{
    LOGV("destructor");
    Cleanup();
}

void AndroidAudioMIO::initData()
{
    ResetData();

    iCommandCounter = 0;
    iLogger = NULL;
    iCommandResponseQueue.reserve(kCommandQueueDepth);
    iWriteResponseQueueLock.Create();
    iWriteResponseQueue.reserve(kCommandQueueDepth);
    iObserver = NULL;
    iLogger = NULL;
    iPeer = NULL;
    iState = STATE_IDLE;
    iWriteBusy = false;
    iWriteBusySeqNum = 0;
    iFlushPending = false;
    iDataQueued = 0;
}

void AndroidAudioMIO::ResetData()
//reset all data from this session.
{
    // reset all the received media parameters.
    iAudioFormat = PVMF_FORMAT_UNKNOWN;
    iAudioNumChannelsValid = false;
    iAudioSamplingRateValid = false;
}

//cleanup all allocated memory and release resources.
void AndroidAudioMIO::Cleanup()
{
    LOGV("Cleanup");
    while (!iCommandResponseQueue.empty()) {
        if (iObserver) {
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId,
                        iCommandResponseQueue[0].iContext, iCommandResponseQueue[0].iStatus));
        }
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }

    // return empty buffers
    returnAllBuffers();

    // delete the request active object
    if (iWriteCompleteAO) {
        OSCL_DELETE(iWriteCompleteAO);
        iWriteCompleteAO = NULL;
    }
    iWriteResponseQueueLock.Close();
}

PVMFStatus AndroidAudioMIO::connect(PvmiMIOSession& aSession, PvmiMIOObserver* aObserver)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::connect() called"));

    // currently supports only one session
    if (iObserver) return PVMFFailure;
    iObserver = aObserver;
    return PVMFSuccess;
}

PVMFStatus AndroidAudioMIO::disconnect(PvmiMIOSession aSession)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::disconnect() called"));
    iObserver = NULL;
    return PVMFSuccess;
}


PvmiMediaTransfer* AndroidAudioMIO::createMediaTransfer(PvmiMIOSession& aSession,
                            PvmiKvp* read_formats, int32 read_flags,
                            PvmiKvp* write_formats, int32 write_flags)
{
    // create the request active object
    // such when audio output thread is done with a buffer
    // it can put the buffer on the write response queue
    // and schedule this MIO to run, to return the buffer
    // to the engine
    iWriteCompleteAO = OSCL_NEW(AndroidAudioOutputThreadSafeCallbackAO,(this, 5));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::createMediaTransfer() called"));
    return (PvmiMediaTransfer*)this;
}

PVMFCommandId AndroidAudioMIO::QueueCmdResponse(PVMFStatus status, const OsclAny* aContext)
{
    PVMFCommandId cmdId = iCommandCounter++;
    CommandResponse resp(status, cmdId, aContext);
    iCommandResponseQueue.push_back(resp);

    // cancel any timer delay so the command response will happen ASAP.
    if (IsBusy()) Cancel();
    RunIfNotReady();
    return cmdId;
}

// return any held buffers to the engine
void AndroidAudioMIO::ProcessWriteResponseQueue()
{
    PVMFStatus status = 0;
    PVMFCommandId cmdId = 0;
    const OsclAny* context = 0;

    iWriteResponseQueueLock.Lock();
    while (!iWriteResponseQueue.empty()) {
        status = iWriteResponseQueue[0].iStatus;
        cmdId = iWriteResponseQueue[0].iCmdId;
        context = (OsclAny*)iWriteResponseQueue[0].iContext;
        iWriteResponseQueue.erase(&iWriteResponseQueue[0]);
        iWriteResponseQueueLock.Unlock();
        if (iPeer) {
            LOGV("Return buffer (%d)", cmdId);
            iPeer->writeComplete(status, cmdId, (OsclAny*)context);
        }
        iWriteResponseQueueLock.Lock();
    }
    iWriteResponseQueueLock.Unlock();
}

PVMFCommandId AndroidAudioMIO::QueryUUID(const PvmfMimeString& aMimeType,
                                        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                        bool aExactUuidsOnly, const OsclAny* aContext)
{
    LOGV("QueryUUID");
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::QueryUUID() called"));
    return QueueCmdResponse(PVMFFailure, aContext);
}

PVMFCommandId AndroidAudioMIO::QueryInterface(const PVUuid& aUuid, PVInterface*& aInterfacePtr, const OsclAny* aContext)
{
    LOGV("QueryInterface");
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::QueryInterface() called"));
    PVMFStatus status=PVMFFailure;
    if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID) {
        PvmiCapabilityAndConfig* myInterface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        aInterfacePtr = OSCL_STATIC_CAST(PVInterface*, myInterface);
        status = PVMFSuccess;
    }
    return QueueCmdResponse(status, aContext);
}

PVMFCommandId AndroidAudioMIO::Init(const OsclAny* aContext)
{
    LOGV("Init (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::Init() called"));
    iState=STATE_INITIALIZED;
    return QueueCmdResponse(PVMFSuccess, aContext);
}

PVMFCommandId AndroidAudioMIO::Reset(const OsclAny* aContext)
{
    // Do nothing for now
    LOGV("Reset (%p)", aContext);
    return PVMFSuccess;
}

PVMFCommandId AndroidAudioMIO::Start(const OsclAny* aContext)
{
    LOGV("Start (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::Start() called"));
    iState = STATE_STARTED;
    return QueueCmdResponse(PVMFSuccess, aContext);
}

PVMFCommandId AndroidAudioMIO::Pause(const OsclAny* aContext)
{
    LOGV("Pause (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::Pause() called"));
    iState = STATE_PAUSED;
    return QueueCmdResponse(PVMFSuccess, aContext);
}


PVMFCommandId AndroidAudioMIO::Flush(const OsclAny* aContext)
{
    LOGV("Flush (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::Flush() called"));
    iState = STATE_INITIALIZED;
    return QueueCmdResponse(PVMFSuccess, aContext);
}

PVMFCommandId AndroidAudioMIO::DiscardData(const OsclAny* aContext)
{
    LOGV("DiscardData (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::DiscardData() called"));
    return DiscardData(UINT_MAX, aContext);
}

PVMFCommandId AndroidAudioMIO::DiscardData(PVMFTimestamp aTimestamp, const OsclAny* aContext)
{
    LOGV("DiscardData (%u, %p)", aTimestamp, aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::DiscardData() called"));
    return QueueCmdResponse(PVMFSuccess, aContext);
}

PVMFCommandId AndroidAudioMIO::Stop(const OsclAny* aContext)
{
    LOGV("Stop (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::Stop() called"));
    iState = STATE_INITIALIZED;
    return QueueCmdResponse(PVMFSuccess, aContext);
}

PVMFCommandId AndroidAudioMIO::CancelAllCommands(const OsclAny* aContext)
{
    LOGV("CancelAllCommands (%p)", aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::CancelAllCommands() called"));

    //commands are executed immediately upon being received, so
    //it isn't really possible to cancel them.
    return QueueCmdResponse(PVMFSuccess, aContext);
}

PVMFCommandId AndroidAudioMIO::CancelCommand(PVMFCommandId aCmdId, const OsclAny* aContext)
{
    LOGV("CancelCommand (%u, %p)", aCmdId, aContext);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::CancelCommand() called"));

    //see if the response is still queued.
    PVMFStatus status = PVMFFailure;
    for (uint32 i = 0; i < iCommandResponseQueue.size(); i++) {
        if (iCommandResponseQueue[i].iCmdId == aCmdId) {
            status = PVMFSuccess;
            break;
        }
    }
    return QueueCmdResponse(PVMFSuccess, aContext);
}

void AndroidAudioMIO::ThreadLogon()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::ThreadLogon() called"));
    if (iState == STATE_IDLE) {
        iLogger = PVLogger::GetLoggerObject("AndroidAudioMIO\n");
        AddToScheduler();
        iState=STATE_LOGGED_ON;
    }
}

void AndroidAudioMIO::ThreadLogoff()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::ThreadLogoff() called"));
    if (iState!=STATE_IDLE) {
        RemoveFromScheduler();
        iLogger = NULL;
        iState = STATE_IDLE;
        ResetData();
    }
}

void AndroidAudioMIO::setPeer(PvmiMediaTransfer* aPeer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,"AndroidAudioMIO::setPeer() called"));
    // Set the observer
    iPeer = aPeer;
}

//This routine will determine whether data can be accepted in a writeAsync
//call and if not, will return true;
bool AndroidAudioMIO::CheckWriteBusy(uint32 aSeqNum)
{
    // FIXME: this line screws up video output - why?
    // return (iOSSRequestQueue.size() >= 5);
    return false;
}

PVMFCommandId AndroidAudioMIO::writeAsync(uint8 aFormatType, int32 aFormatIndex, uint8* aData, uint32 aDataLen,
                                        const PvmiMediaXferHeader& data_header_info, OsclAny* aContext)
{
    uint32 aSeqNum = data_header_info.seq_num;
    PVMFTimestamp aTimestamp = data_header_info.timestamp;
    uint32 flags = data_header_info.flags;

    bool bWriteComplete = true;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
        (0,"AndroidAudioMIO::writeAsync() seqnum %d ts %d flags %d context %d",
         aSeqNum, aTimestamp, flags,aContext));

    PVMFStatus status=PVMFFailure;

    switch(aFormatType) {
    case PVMI_MEDIAXFER_FMT_TYPE_COMMAND :
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
            (0,"AndroidAudioMIO::writeAsync() called with Command info."));
        //ignore
        LOGV("PVMI_MEDIAXFER_FMT_TYPE_COMMAND");
        status = PVMFSuccess;
        break;

    case PVMI_MEDIAXFER_FMT_TYPE_NOTIFICATION :
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
            (0,"AndroidAudioMIO::writeAsync() called with Notification info."));
        switch(aFormatIndex) {
        case PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM:
            LOGV("PVMI_MEDIAXFER_FMT_INDEX_END_OF_STREAM");
            bWriteComplete = false; //force an empty buffer through the audio thread
            break;
        default:
            break;
        }
        //ignore
        status = PVMFSuccess;
        break;

    case PVMI_MEDIAXFER_FMT_TYPE_DATA :
        switch(aFormatIndex) {
        case PVMI_MEDIAXFER_FMT_INDEX_FMT_SPECIFIC_INFO:
            status = PVMFSuccess;
            LOGV("PVMI_MEDIAXFER_FMT_INDEX_FMT_SPECIFIC_INFO");
            break;

        case PVMI_MEDIAXFER_FMT_INDEX_DATA:
            LOGV("PVMI_MEDIAXFER_FMT_INDEX_DATA");
            //data contains the media bitstream.

            //Check whether we can accept data now and leave if we can't.
            if (CheckWriteBusy(aSeqNum)) {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                    (0,"AndroidAudioMIO::writeAsync: Entering busy state"));

                //schedule an event to re-start the data flow.
                iWriteBusy = true;
                iWriteBusySeqNum = aSeqNum;
                bWriteComplete = false;

                // Rich:: commenting in this line.
                //        Need some timeout here just in case.
                //        I have no evidence of any problems.
                RunIfNotReady(10 * 1000);
                OSCL_LEAVE(OsclErrBusy);
            } else {
                if (aDataLen > 0) {
                    // this buffer will be queued by the audio output thread to process
                    // this buffer cannot be write completed until
                    // it has been processed by the audio output thread
                    bWriteComplete = false;
                } else { PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO,
                            (0,"AndroidAudioMIO::writeAsync() called aDataLen==0."));
                }
                status = PVMFSuccess;
            }
            break;

        default:
            LOGV("aFormatIndex=%u", aFormatIndex);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                (0,"AndroidAudioMIO::writeAsync: Error - unrecognized format index"));
            status = PVMFFailure;
            break;
        }
        break;

    default:
        LOGV("aFormatType=%u", aFormatType);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
            (0,"AndroidAudioMIO::writeAsync: Error - unrecognized format type"));
        status = PVMFFailure;
        break;
    }

    //Schedule asynchronous response
    PVMFCommandId cmdid=iCommandCounter++;
    if (bWriteComplete) {
        LOGV("write complete (%d)", cmdid);
        WriteResponse resp(status, cmdid, aContext, aTimestamp);
        iWriteResponseQueueLock.Lock();
        iWriteResponseQueue.push_back(resp);
        iWriteResponseQueueLock.Unlock();
        RunIfNotReady();
    } else if (!iWriteBusy) {
        writeAudioBuffer(aData, aDataLen, cmdid, aContext, aTimestamp);
    }
    LOGV("data queued = %u", iDataQueued);

    return cmdid;
}

PVMFStatus AndroidAudioMIO::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                                              PvmiKvp*& aParameters, int& num_parameter_elements,
                                              PvmiCapabilityContext aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    aParameters=NULL;
    num_parameter_elements=0;

    if(pv_mime_strcmp(aIdentifier, INPUT_FORMATS_CAP_QUERY) == 0) {
        //This is a query for the list of supported formats.
        aParameters=(PvmiKvp*)oscl_malloc(2 * sizeof(PvmiKvp));
        if (aParameters) {
            aParameters[num_parameter_elements++].value.uint32_value = PVMF_PCM16;
            aParameters[num_parameter_elements++].value.uint32_value = PVMF_PCM8;
            return PVMFSuccess;
        }
        return PVMFErrNoMemory;
    }
    //other queries are not currently supported.

    //unrecognized key.
    return PVMFFailure;
}

PVMFStatus AndroidAudioMIO::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements)
{
    //release parameters that were allocated by this component.
    if (aParameters) {
        oscl_free(aParameters);
        return PVMFSuccess;
    }
    return PVMFFailure;
}

void AndroidAudioMIO::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                        int num_elements, PvmiKvp * & aRet_kvp)
{
    OSCL_UNUSED_ARG(aSession);

    aRet_kvp = NULL;

    for (int32 i=0;i<num_elements;i++) {
        //Check against known audio parameter keys...
        if (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_FORMAT_KEY) == 0) {
            LOGV("Audio format: %s", aParameters[i].value.pChar_value);
            iAudioFormat=GetFormatIndex(aParameters[i].value.pChar_value);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidAudioOutput::setParametersSync() Audio Format Key, Value %s",aParameters[i].value.pChar_value));
        } else if (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_SAMPLING_RATE_KEY) == 0) {
            iAudioSamplingRate=(int32)aParameters[i].value.uint32_value;
            iAudioSamplingRateValid=true;
            // LOGD("iAudioSamplingRate=%d", iAudioSamplingRate);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidAudioMIO::setParametersSync() Audio Sampling Rate Key, Value %d",iAudioSamplingRate));
        } else if (pv_mime_strcmp(aParameters[i].key, MOUT_AUDIO_NUM_CHANNELS_KEY) == 0) {
            iAudioNumChannels=(int32)aParameters[i].value.uint32_value;
            iAudioNumChannelsValid=true;
            // LOGD("iAudioNumChannels=%d", iAudioNumChannels);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidAudioMIO::setParametersSync() Audio Num Channels Key, Value %d",iAudioNumChannels));
        } else {
            //if we get here the key is unrecognized.

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                (0,"AndroidAudioMIO::setParametersSync() Error, unrecognized key "));

            //set the return value to indicate the unrecognized key
            //and return.
            aRet_kvp = &aParameters[i];
            return;
        }
    }
}

//
// Private section
//

void AndroidAudioMIO::Run()
{
    while (!iCommandResponseQueue.empty()) {
        if (iObserver) {
            iObserver->RequestCompleted(PVMFCmdResp(iCommandResponseQueue[0].iCmdId,
                        iCommandResponseQueue[0].iContext,
                        iCommandResponseQueue[0].iStatus));
        }
        iCommandResponseQueue.erase(&iCommandResponseQueue[0]);
    }

    //send async write completion
    ProcessWriteResponseQueue();

    //Re-start the data transfer if needed.
    if (iWriteBusy) {
        iWriteBusy = false;
        iPeer->statusUpdate(PVMI_MEDIAXFER_STATUS_WRITE);
    }
}

// send response to MIO
void AndroidAudioMIO::sendResponse(PVMFCommandId cmdid, const OsclAny* context, PVMFTimestamp timestamp)
{
    LOGV("sendResponse :: return buffer (%d) timestamp(%d) context(%p)", cmdid, timestamp, context);
    WriteResponse resp(PVMFSuccess, cmdid, context, timestamp);
    iWriteResponseQueueLock.Lock();
    if (iWriteResponseQueue.size() < iWriteResponseQueue.capacity()) {
        iWriteResponseQueue.push_back(resp);
    } else {
        LOGE("Exceeded response queue capacity");
    }
    iWriteResponseQueueLock.Unlock();

    // Create an event for the threadsafe callback AO
    OsclAny* P = NULL;
    iWriteCompleteAO->ReceiveEvent(P);
}

void AndroidAudioMIO::setAudioSink(const sp<MediaPlayerInterface::AudioSink>& audioSink)
{
    mAudioSink = audioSink;
}

///------------------------------------------------------------------------
// Active timing support
//
OSCL_EXPORT_REF PVMFStatus AndroidAudioMIOActiveTimingSupport::SetClock(OsclClock *clockVal)
{
    iClock=clockVal;
    if (iClock)
        iClock->SetClockStateObserver(*this);
    return PVMFSuccess;
}

OSCL_EXPORT_REF bool AndroidAudioMIOActiveTimingSupport::queryInterface(const PVUuid& aUuid, PVInterface*& aInterface)
{
    aInterface = NULL;
    PVUuid uuid;
    queryUuid(uuid);
    if (uuid == aUuid) {
        PvmiClockExtensionInterface* myInterface = OSCL_STATIC_CAST(PvmiClockExtensionInterface*, this);
        aInterface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        return true;
    }
    return false;
}

void AndroidAudioMIOActiveTimingSupport::queryUuid(PVUuid& uuid)
{
    uuid = PvmiClockExtensionInterfaceUuid;
}

void AndroidAudioMIOActiveTimingSupport::ClockStateUpdated()
{
    if (iClock) {
        OsclClock::OsclClockState newClockState = iClock->GetState();
        if (newClockState != iClockState) {
            iClockState = newClockState;
            switch (iClockState) {
            case OsclClock::STOPPED:
                LOGV("A/V clock stopped");
                break;
            case OsclClock::RUNNING:
                LOGV("A/V clock running");
                // must be seeking, get new clock offset for A/V sync
                if (iUpdateClock) {
                    iClock->GetCurrentTime64(iStartTime, OSCLCLOCK_MSEC);
                    iFrameCount = 0;
                    iUpdateClock = false;
                    LOGV("update iStartTime: %llu", iStartTime);
                }
                LOGV("signal thread to start");
                if (iAudioThreadSem) iAudioThreadSem->Signal();
                break;
            case OsclClock::PAUSED:
                LOGV("A/V clock paused");
                break;
            default:
                break;
            }
        }
    }
}

void AndroidAudioMIOActiveTimingSupport::UpdateClock()
{
    uint64 clockTime64, timeBaseTime64, updateClock;
    int64 correction = 0;

    if (iClock && (iClockState == OsclClock::RUNNING)) {

        // get current time
        iClock->GetCurrentTime64(clockTime64, OSCLCLOCK_MSEC, timeBaseTime64);
        LOGV("PV clock current = %llu", clockTime64);

        // calculate sample clock
        uint64 updateClock = iFrameCount * iMsecsPerFrame;
        LOGV("sample clock = %llu", updateClock);

        // startup - force clock backwards to compensate for latency
        if (updateClock < iDriverLatency) {
            LOGV("iStartTime = %llu", iStartTime);
            LOGV("iDriverLatency = %u", iDriverLatency);
            LOGV("latency stall - forcing clock");
            correction = iStartTime - clockTime64;
            LOGV("latency correction = %lld", correction);
        }

        // normal play mode - check delta between PV engine clock and sample clock
        else {
            correction = (updateClock - iDriverLatency) - (clockTime64 - iStartTime);
        }

        // do clock correction if drift exceeds threshold
        LOGV("drift = %lld", correction);
        if (OSCL_ABS(correction) > iMinCorrection) {
            if (correction > iMaxCorrection) {
                correction = iMaxCorrection;
            } else if (correction < -iMaxCorrection) {
                correction = -iMaxCorrection;
            }
            updateClock = clockTime64 + correction;
            LOGV("drift correction = %lld, new clock = %llu", correction, updateClock);
            if (!iClock->AdjustClockTime64(clockTime64, timeBaseTime64, updateClock, OSCLCLOCK_MSEC))
                LOGE("Error adjusting clock");
        }
    }
}

