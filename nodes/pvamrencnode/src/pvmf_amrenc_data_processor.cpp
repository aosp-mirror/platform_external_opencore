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
 * @file pvmf_amrenc_data_processor.cpp
 * @brief Data processor to encode PCM16 data to AMR-IETF and AMR-IF2 using PV AMR encoder library
 */
#ifndef PVMF_AMRENC_DATA_PROCESSOR_H_INCLUDED
#include "pvmf_amrenc_data_processor.h"
#endif
#ifndef PVMF_AMRENC_TUNEABLES_INCLUDED
#include "pvmf_amrenc_tuneables.h"
#endif
#ifndef GSMAMR_ENCODER_H_INCLUDED
#include "gsmamr_encoder_wrapper.h"
#endif

#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif

////////////////////////////////////////////////////////////////////////////
PvmfAmrEncDataProcessor::PvmfAmrEncDataProcessor()
        : iMediaOutMemoryPool(OUTPUT_BITSTREAMBUFFER_POOLNUM),
        iMediaDataMemPool(OUTPUT_MEDIADATA_POOL_SIZE)
{
    int32 err = 0;
    Construct();
    OSCL_TRY(err,
             // Reserve for data queue
             iIncomingDataQueue.reserve(PVMF_AMRENC_NODE_PORT_ACTIVITY_RESERVE);

             // Create media data allocator
             iMediaOutAlloc = OSCL_NEW(PvmfAmrEncBufferAlloc, (&iMediaOutMemoryPool));
             if (!iMediaOutAlloc)
             OSCL_LEAVE(OsclErrNoMemory);
            );

    OSCL_FIRST_CATCH_ANY(err,
                         // If a leave happened, cleanup and re-throw the error
                         iIncomingDataQueue.clear();
                         OSCL_LEAVE(err);
                        );

    iLogger = PVLogger::GetLoggerObject("PvmfAmrEncDataProcessor");
    iDiagnosticsLogger = PVLogger::GetLoggerObject("pvauthordiagnostics.encnode.amrencnode");

#if PROFILING_ON
    total_ticks = 0;
    total_frames = 0;
    iFinalTimeStamp = 0;
    iMinEncDuration = 0;
    iMaxEncDuration = 0;
    iAverageEncDuration = 0;
    oDiagnosticsLogged = false;
#endif

}

////////////////////////////////////////////////////////////////////////////
PvmfAmrEncDataProcessor::~PvmfAmrEncDataProcessor()
{
#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif
    Reset();
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncDataProcessor::LogDiagnostics()
{
#if PROFILING_ON
    oDiagnosticsLogged = true;
    uint32 enctime = OsclTickCount::TicksToMsec(total_ticks);
    if (total_frames > 0)
    {
        iAverageEncDuration = enctime / total_frames;
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iDiagnosticsLogger, PVLOGMSG_INFO, (0, "PVMFAMREncNode Diagnostics: NumFrames=%d, Final Timestamp=%d, encode time(in ms)[Total:%d, Max:%d, Min:%d, Average:%d]\n",
                    total_frames, iFinalTimeStamp, OsclTickCount::TicksToMsec(total_ticks), iMaxEncDuration, iMinEncDuration, iAverageEncDuration));
#endif

}
////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::ThreadLogon()
{
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::ThreadLogoff()
{
    iLogger = NULL;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::QueryInterface(const PVUuid& uuid, PVInterface*& aface)
{
    if (queryInterface(uuid, aface))
        return PVMFSuccess;
    else
        return PVMFFailure;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::QueryUUID(const PvmfMimeString& aMimeType,
        Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
        bool aExactUuidsOnly)
{
    OSCL_UNUSED_ARG(aMimeType);
    OSCL_UNUSED_ARG(aExactUuidsOnly);

    int32 err = 0;
    OSCL_TRY(err, aUuids.push_back(PVAMREncExtensionUUID););
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncDataProcessor::QueryUUID: Error - aUuid.push_back() failed"));
                         return PVMFErrNoMemory;
                        );
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::Initialize()
{
    // Create encoder object and encoder settings
    int32 err = 0;
    OSCL_TRY(err,
             iGsmEncoder = OSCL_NEW(CPvGsmAmrEncoder, ());
             if (!iGsmEncoder)
             OSCL_LEAVE(OsclErrNoMemory);

             // for encoder properties
             iEncProps = OSCL_NEW(TEncodeProperties, ());
             if (!iEncProps)
                 OSCL_LEAVE(OsclErrNoMemory);
                );

    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncDataProcessor::Initialize: Error - Encoder / encoder property creation failed"));
                         return PVMFErrNoMemory;
                        );

    iEncProps->iInBitsPerSample  = iInputBitsPerSample;
    iEncProps->iInSamplingRate   = iInputSamplingRate;
    iEncProps->iInClockRate      = 1000; //iEncProps->iInSamplingRate;
    iEncProps->iInNumChannels    = (uint8)iInputNumChannels;
    iEncProps->iInInterleaveMode = TEncodeProperties::EINTERLEAVE_LR;

    iEncProps->iMode			   = (int32)iOutputBitrate;
    iEncProps->iBitStreamFormatIf2 = (iOutputFormat == PVMF_AMR_IF2);
    iEncProps->iAudioObjectType    = 0; // only for AAC encoder
    iEncProps->iOutSamplingRate    = iInputSamplingRate;
    iEncProps->iOutNumChannels     = (uint8)iInputNumChannels;
    iEncProps->iOutClockRate       = iEncProps->iInClockRate ; // set the sampling rate , but not sure at this point

    // initialize the amr encoder, cause memory buffer overflow problem in the encoding
    LOG_DEBUG((0, "PvmfAmrEncDataProcessor::Initialize: Calling InitializeEncoder()"));

    // initialize the amr encoder, cause memory buffer overflow problem in the encoding
    // when I set the maximum output buffer size as KGsmBlockSize, so set it as large value
    iMaxOutputBufferSize = iMaxNumOutputFramesPerBuffer * MAX_AMR_FRAME_SIZE;
    if (iGsmEncoder->InitializeEncoder(iMaxOutputBufferSize, iEncProps) < 0) // MAX_AMR_FRAME_SIZE = 32
    {
        LOG_ERR((0, "PvmfAmrEncDataProcessor::Initialize: Error - InitializeEncoder() failed"));
        return PVMFFailure;
    }

    OSCL_TRY(err,
             iSizeArrayForOutputFrames = (int32*)iAlloc.allocate(sizeof(int32) * iMaxNumOutputFramesPerBuffer);
             if (!iSizeArrayForOutputFrames)
             OSCL_LEAVE(OsclErrNoMemory);
             oscl_memset(iSizeArrayForOutputFrames, 0, iMaxNumOutputFramesPerBuffer*sizeof(int32));

             iOneInputFrameLength = AMR_FRAME_LENGTH_IN_TIMESTAMP * iInputSamplingRate * iInputBitsPerSample / 8000;
             iMax_input_size = iMaxNumOutputFramesPerBuffer * iOneInputFrameLength;
             iInternalInputBuffer = (uint8*)iAlloc.allocate(iMax_input_size);
             if (!iInternalInputBuffer)
                 OSCL_LEAVE(OsclErrNoMemory);
                 oscl_memset(iInternalInputBuffer, 0, iMax_input_size);
                );

    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncDataProcessor::Initialize: Error - iSizeArrayForOutputFrames or iInternalInputBuffer allocation failed"));
                         return PVMFErrNoMemory;
                        );

    iInitialized = true;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::Reset()
{
    iInitialized = false;
    iIncomingDataQueue.clear();

#if PROFILING_ON
    if (!oDiagnosticsLogged)
    {
        LogDiagnostics();
    }
#endif

    if (iGsmEncoder)
    {
        iGsmEncoder->CleanupEncoder();
        OSCL_DELETE(iGsmEncoder);
    }

    if (iEncProps)
        OSCL_DELETE(iEncProps);

    if (iMediaOutAlloc)
        OSCL_DELETE(iMediaOutAlloc);

    if (iSizeArrayForOutputFrames)
        iAlloc.deallocate(iSizeArrayForOutputFrames);

    if (iInternalInputBuffer)
        iAlloc.deallocate(iInternalInputBuffer);

    Construct();
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::QueueIncomingData(PVMFSharedMediaDataPtr& aMediaDataIn)
{
    if (!iInitialized)
    {
        LOG_ERR((0, "PvmfAmrEncDataProcessor::Process: Error - Encoder not initialized"));
        return PVMFErrNotReady;
    }

    int32 err = 0;
    OSCL_TRY(err, iIncomingDataQueue.push_back(aMediaDataIn););
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncDataProcessor::Process: Error - iIncomingDataQueue.push_back() failed"));
                         return PVMFErrNoMemory;
                        );

    if (IsEnoughDataToEncode())
        Encode();

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncDataProcessor::Encode()
{
    PVMFSharedMediaDataPtr mediaDataOut;
    int32 err = 0;

    BufferInfo bufferInfo;
    if (!GetNewInputBuffer(bufferInfo))
    {
        LOG_DEBUG((0, "PvmfAmrEncDataProcessor::Encode: Not enough data"));
        return; // Not enough data
    }

    // 2. create output buffer pointer
    OsclSharedPtr<PVMFMediaDataImpl> mediaDataImpl;
    OSCL_TRY(err, mediaDataImpl = iMediaOutAlloc->allocate(MAX_AMR_FRAME_SIZE, iMaxNumOutputFramesPerBuffer));
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncDataProcessor::Encode: Error - Memory for mediaDataImpl could not be allocated"));
                         iObserver->ProcessComplete(ProcessNoOutputMemory, mediaDataOut);
                         return;
                        );

    OSCL_TRY(err, mediaDataOut = PVMFMediaData::createMediaData(mediaDataImpl, &iMediaDataMemPool));
    OSCL_FIRST_CATCH_ANY(err,
                         LOG_ERR((0, "PvmfAmrEncDataProcessor::Encode: Error - Memory for mediaData could not be allocated"));
                         iObserver->ProcessComplete(ProcessNoOutputMemory, mediaDataOut);
                         return;
                        );

    OsclRefCounterMemFrag memFragOut;
    mediaDataOut->getMediaFragment(0, memFragOut);

    TOutputAudioStream output;
    output.iBitStreamBuffer = (uint8*)memFragOut.getMemFragPtr();
    output.iNumSampleFrames = 0;
    output.iSampleFrameSize = iSizeArrayForOutputFrames;

    // We already figured out the pointer and buffer size and then do encoding
    int32 input_frame_num = bufferInfo.ptrLength / iOneInputFrameLength; //mediaDataPtr->getNumFragments();

    TInputAudioStream input;
    input.iSampleBuffer = bufferInfo.ptr;
    input.iSampleLength = (int32)bufferInfo.ptrLength;
    input.iMode			= iEncProps->iMode;
    input.iStartTime	= (iNextStartTime >= bufferInfo.tsInFirstMediaData  ? iNextStartTime : bufferInfo.tsInFirstMediaData);
    input.iStopTime		= input.iStartTime + AMR_FRAME_LENGTH_IN_TIMESTAMP * input_frame_num; // 20ms
    iNextStartTime		= input.iStopTime; // for the next encoding

    // 3. do encoding at one time for multiple frame input
#if PROFILING_ON
    uint32 start_ticks = OsclTickCount::TickCount();
#endif
    if (iGsmEncoder->Encode(input, output) < 0 || output.iNumSampleFrames != input_frame_num)
    {
        LOG_ERR((0, "PvmfAmrEncDataProcessor::Encode: Error - iGsmEncoder->Encode() failed"));
        iObserver->ProcessComplete(ProcessError, mediaDataOut);
        return;
    }
#if PROFILING_ON
    uint32 end_ticks = OsclTickCount::TickCount();
    uint32 enctime = OsclTickCount::TicksToMsec(end_ticks - start_ticks);
    if ((iMinEncDuration > enctime) || (0 == iMinEncDuration))
    {
        iMinEncDuration = enctime;
    }

    if (iMaxEncDuration < enctime)
    {
        iMaxEncDuration = enctime;
    }

    total_ticks += (end_ticks - start_ticks);
    total_frames += 1;
    iFinalTimeStamp = input.iStopTime;
#endif

    // For IETF, make a conversion from WMF
    uint8 *tmp_buff = output.iBitStreamBuffer;
    uint32 i;
    for (i = 0; i < (uint32)output.iNumSampleFrames; i++)
    {
        // for IETF format, we need to make change
        if (!iEncProps->iBitStreamFormatIf2)  // non-IF2 => IETF format, not WMF format
        {
            tmp_buff[0] = (uint8)(((tmp_buff[0] << 3) | 0x4) & 0x7C); // IETF frame header: P(1) + FT(4) + Q(1) + P(2) , Q=1 for good frame, P=padding bit, 0
            tmp_buff += output.iSampleFrameSize[i];
        }

        // Set fragment length
        mediaDataOut->setMediaFragFilledLen(i, output.iSampleFrameSize[i]);
    }

    // Set timestamp
    mediaDataOut->setTimestamp(input.iStartTime);

    // Set sequence number
    mediaDataOut->setSeqNum(iSeqNum++);

    if (bufferInfo.bNeedDequeue)
    {
        // Remove from incoming queue
        iIncomingDataQueue.erase(iIncomingDataQueue.begin());
    }

    PvmfAmrEncDataProcessResult result;
    if (bufferInfo.bContinueProcessing)
        result = ProcessComplete_EnoughDataToContinueProcessing;
    else
        result = ProcessComplete_NotEnoughDataInQueue;

    // Send frame back to the node
    iObserver->ProcessComplete(result, mediaDataOut);
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::SetInputSamplingRate(uint32 aSamplingRate)
{
    if (aSamplingRate != 8000)
        return PVMFErrNotSupported;
    iInputSamplingRate = aSamplingRate;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::SetInputBitsPerSample(uint32 aBitsPerSample)
{
    if (aBitsPerSample != 16)
        return PVMFErrNotSupported;
    iInputBitsPerSample = aBitsPerSample;
    return PVMFSuccess;
}


////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::SetInputNumChannels(uint32 aNumChannels)
{
    if (aNumChannels != 1)
        return PVMFErrNotSupported;
    iInputNumChannels = aNumChannels;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfAmrEncDataProcessor::SetOutputFormat(PVMFFormatType aFormat)
{
    if (aFormat != PVMF_AMR_IETF && aFormat != PVMF_AMR_IF2)
        return PVMFErrNotSupported;

    iOutputFormat = aFormat;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
uint32 PvmfAmrEncDataProcessor::GetOutputBitRate()
{
    switch (iOutputBitrate)
    {
        case GSM_AMR_4_75:
            return 4750;
        case GSM_AMR_5_15:
            return 5150;
        case GSM_AMR_5_90:
            return 5900;
        case GSM_AMR_6_70:
            return 6700;
        case GSM_AMR_7_40:
            return 7400;
        case GSM_AMR_7_95:
            return 7950;
        case GSM_AMR_10_2:
            return 10200;
        case GSM_AMR_12_2:
            return 12200;
        default:
            return 0;
    }
}

////////////////////////////////////////////////////////////////////////////
//         Virtual functions of PVAMREncExtensionInterface
////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncDataProcessor::addRef()
{
    ++iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void PvmfAmrEncDataProcessor::removeRef()
{
    if (iExtensionRefCount > 0) --iExtensionRefCount;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PvmfAmrEncDataProcessor::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    if (uuid == PVAMREncExtensionUUID)
    {
        PVAMREncExtensionInterface* myInterface = OSCL_STATIC_CAST(PVAMREncExtensionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, myInterface);
        ++iExtensionRefCount;
    }
    else
    {
        iface = NULL;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncDataProcessor::SetOutputBitRate(PVMF_GSMAMR_Rate aBitRate)
{
    iOutputBitrate = aBitRate;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVMFStatus PvmfAmrEncDataProcessor::SetMaxNumOutputFramesPerBuffer(uint32 aNumOutputFrames)
{
    iMaxNumOutputFramesPerBuffer = aNumOutputFrames;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
//                           Private methods
////////////////////////////////////////////////////////////////////////////
void PvmfAmrEncDataProcessor::Construct()
{
    // Codec and encoder setting structure
    iGsmEncoder = NULL;
    iEncProps   = NULL;

    // Default values
    iInputSamplingRate  = 8000;
    iInputBitsPerSample = 16;
    iInputNumChannels   = 1;
    iOutputBitrate		= GSM_AMR_12_2;
    iOutputFormat	    = PVMF_AMR_IETF;
    iMaxNumOutputFramesPerBuffer = 25;
    iInitialized		= false;

    iSizeArrayForOutputFrames	= NULL;
    iInternalInputBuffer		= NULL;
    iMax_input_size				= 0;
    iOneInputFrameLength		= 320;
    iPosForInputMediaDataBuffer = 0;
    iNextStartTime				= 0;

    // Other stuff
    iSeqNum = 0;
    iExtensionRefCount = 0;

    iMediaOutAlloc      = NULL;
    iLogger = NULL;
}

////////////////////////////////////////////////////////////////////////////
uint32 PvmfAmrEncDataProcessor::GetTotalAvailableDataSize()
{
    uint32 i = 0, total_available_data_size = 0;

    for (i = 0; i < iIncomingDataQueue.size(); i++)
    {
        PVMFSharedMediaDataPtr mediaDataPtr = iIncomingDataQueue[i];
        total_available_data_size += mediaDataPtr->getFilledSize();
    }

    total_available_data_size -= iPosForInputMediaDataBuffer;
    return total_available_data_size;
}

////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncDataProcessor::IsEnoughDataToEncode()
{
    return (GetTotalAvailableDataSize() >= iOneInputFrameLength);
}

////////////////////////////////////////////////////////////////////////////
bool PvmfAmrEncDataProcessor::GetNewInputBuffer(BufferInfo &aBufferInfo)
{
    // Get the current available data size in the input buffer queue
    uint32 total_available_data_size = 0;
    uint8 *ptrIn = NULL;
    uint32 ptrLength = 0;

    total_available_data_size = GetTotalAvailableDataSize();
    if (!IsEnoughDataToEncode())
    {
        LOG_DEBUG((0, "PvmfAmrEncDataProcessor::GetNewInputBuffer: Not enough data"));
        return false; // No enough data
    }

    // total available data size is big enough, at least for one frame encoding
    bool bNeedDeque = true, bContinueProcessing = false;
    PVMFSharedMediaDataPtr mediaDataPtr = iIncomingDataQueue[0];
    ptrLength = mediaDataPtr->getFilledSize() - iPosForInputMediaDataBuffer;
    uint32 tsInFirstMediaData = mediaDataPtr->getTimestamp();

    if (ptrLength >= iMax_input_size || /* current available media data length is big enough */
            iIncomingDataQueue.size() == 1)    /* only one media data buffer */
    {
        // (1) the first buffer in the queue is big enough, definitely able to fill up the internal buffer
        // (2) there is only one buffer in the queue (whatever the buffer size is, but the bottom line is
        //	   the buffer size should be larger than one frame size)
        // no need to copy to the internal buffer
        ptrLength = pv_min(ptrLength, iMax_input_size);
        ptrLength -= (ptrLength % iOneInputFrameLength); // make ptrLength one pcm frame length aligned

        // get the buffer pointer
        OsclRefCounterMemFrag memFrag;
        mediaDataPtr->getMediaFragment(0, memFrag);
        ptrIn = (uint8*)memFrag.getMemFragPtr();
        ptrIn += iPosForInputMediaDataBuffer;

        // update iPosForInputMediaDataBuffer
        iPosForInputMediaDataBuffer += ptrLength;
        if (iPosForInputMediaDataBuffer == mediaDataPtr->getFilledSize())
        {
            // this buffer is consumed
            iPosForInputMediaDataBuffer = 0;
            bNeedDeque = true;
        }
        else   // still has data
        {
            bNeedDeque = false;
        }
    }
    else
    {
        // buffer queue has multiple buffers, but the first buffer in the queue is not able to
        // fill up the whole internal buffer
        // Then we have to make a copy and use internal buffer instead

        // first, we need to calculate the maximum allowable filled size using total_available_data_size calculated above
        uint32 max_filled_size = pv_min(total_available_data_size, iMax_input_size);
        max_filled_size -= (max_filled_size % iOneInputFrameLength); // make max_filled_size one frame aligned

        uint32 max_copy_size = max_filled_size;
        uint8 *ptrForInternalBuffer = iInternalInputBuffer;
        do
        {
            OsclRefCounterMemFrag memFrag;
            mediaDataPtr->getMediaFragment(0, memFrag);
            ptrIn = (uint8*)memFrag.getMemFragPtr();
            ptrIn += iPosForInputMediaDataBuffer;
            ptrLength = mediaDataPtr->getFilledSize() - iPosForInputMediaDataBuffer;
            uint32 copy_size = pv_min(ptrLength, max_copy_size);
            oscl_memcpy(ptrForInternalBuffer, ptrIn, copy_size);

            if (ptrLength >= max_copy_size)
            {
                if (ptrLength == max_copy_size)
                {
                    // a whole buffer is consumed
                    iPosForInputMediaDataBuffer = 0;

                    // Remove from incoming queue
                    iIncomingDataQueue.erase(iIncomingDataQueue.begin());
                }
                else
                {
                    iPosForInputMediaDataBuffer += copy_size;
                }

                // finalize the pointer and length for the input of amr encoding
                ptrIn = iInternalInputBuffer;
                ptrLength = max_filled_size;

                bNeedDeque = false;
                break;
            }

            // the internal buffer isn't full yet, needs to be re-filled with a new media data buffer
            // Remove from incoming queue
            iIncomingDataQueue.erase(iIncomingDataQueue.begin());

            // Note that no need to check the buffer queue is empty of not, because the above break point should be reached
            // get a new media buffer
            mediaDataPtr = iIncomingDataQueue[0];

            // other updates
            iPosForInputMediaDataBuffer = 0;
            max_copy_size -= copy_size;
            ptrForInternalBuffer += copy_size;

        }
        while (max_filled_size); // max_filled_size is always larger than zero, infinite loop, remove the warning if putting "while(1)"
    }

    // for efficiency, check the data after the encoding is enough for the next round of encoding
    total_available_data_size -= ptrLength;
    if (total_available_data_size >= iOneInputFrameLength) // data is enough for one frame encoding
        bContinueProcessing = true;
    else
        bContinueProcessing = false;

    // set aBufferInfo
    aBufferInfo.ptr					= ptrIn;
    aBufferInfo.ptrLength			= ptrLength;
    aBufferInfo.bContinueProcessing = bContinueProcessing;
    aBufferInfo.bNeedDequeue		= bNeedDeque;
    aBufferInfo.tsInFirstMediaData  = tsInFirstMediaData;

    return true;
}














