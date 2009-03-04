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
 *
 * @file pvvideoencmdf_node.h
 * @brief Node for PV Video Encoder MDFs. This node
 * encodes raw video data (YUV/RGB) to MP4 or H263 formats.
 *
 */
#ifndef PVMF_AMRENC_DATA_PROCESSOR_H_INCLUDED
#define PVMF_AMRENC_DATA_PROCESSOR_H_INCLUDED

#ifndef PVMF_AMRENC_DATA_PROCESSOR_INTERFACE_H_INCLUDED
#include "pvmf_amrenc_data_processor_interface.h"
#endif
#ifndef PVMF_AMRENC_MEDIA_BUFFER_H_INCLUDED
#include "pvmf_amrenc_media_buffer.h"
#endif
#ifndef PVMFAMRENCNODE_EXTENSION_H_INCLUDED
#include "pvmfamrencnode_extension.h"
#endif
#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif
#ifndef OSCL_TICKCOUNT_H_INCLUDED
#include "oscl_tickcount.h"
#endif

#define PROFILING_ON (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_PROF)

#define pv_min(a, b)  ((a) <= (b) ? (a) : (b));
typedef OsclMemAllocator PvmfAmrEncDataProcessorAlloc;
typedef Oscl_Vector<PVMFSharedMediaDataPtr, PvmfAmrEncDataProcessorAlloc> PvmfAmrEncMediaDataVector;
typedef OsclMemPoolFixedChunkAllocator PvmfAmrEncDataProcessorMemPool;

struct BufferInfo
{
    uint8 *ptr;
    uint32 ptrLength;
    uint32 tsInFirstMediaData;
    bool bContinueProcessing;
    bool bNeedDequeue;

    BufferInfo()
    {
        oscl_memset(this, 0, sizeof(struct BufferInfo));
    }
};

// Forward declarations
class CPvGsmAmrEncoder;
class TEncodeProperties;
class OsclClock;
class PvmfAmrEncDataProcessor : public PvmfAmrEncDataProcessorInterface,
            public PVAMREncExtensionInterface,
            public OsclMemPoolFixedChunkAllocatorObserver
{
    public:

        PvmfAmrEncDataProcessor();
        ~PvmfAmrEncDataProcessor();

        // Virtual functions of PVNodeDataPrecessor
        PVMFStatus ThreadLogon();
        PVMFStatus ThreadLogoff();
        PVMFStatus QueryUUID(const PvmfMimeString& aMimeType,
                             Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                             bool aExactUuidsOnly = false);
        PVMFStatus QueryInterface(const PVUuid& uuid, PVInterface*& aface);
        PVMFStatus Initialize();
        PVMFStatus Reset();
        PVMFStatus QueueIncomingData(PVMFSharedMediaDataPtr& aMediaDataIn);
        void Encode();
        PVMFStatus SetInputSamplingRate(uint32 aSamplingRate);
        PVMFStatus SetInputBitsPerSample(uint32 aBitsPerSample);
        PVMFStatus SetInputNumChannels(uint32 aNumChannels);
        PVMFStatus SetOutputFormat(PVMFFormatType aFormat);
        uint32 GetOutputBitRate();
        PVMFFormatType GetOutputFormat()
        {
            return iOutputFormat;
        }

        // Virtual functions of PVAMREncExtensionInterface
        OSCL_IMPORT_REF void addRef();
        OSCL_IMPORT_REF void removeRef();
        OSCL_IMPORT_REF bool queryInterface(const PVUuid& uuid, PVInterface*& iface);
        OSCL_IMPORT_REF PVMFStatus SetOutputBitRate(PVMF_GSMAMR_Rate aBitRate);
        OSCL_IMPORT_REF PVMFStatus SetMaxNumOutputFramesPerBuffer(uint32 aNumOutputFrames);

    private:
        void Construct();
        uint32 GetTotalAvailableDataSize();
        bool IsEnoughDataToEncode();
        bool GetNewInputBuffer(BufferInfo &aBufferInfo);

        // virtual functions from OsclMemPoolFixedChunkAllocatorObserver
        void freechunkavailable(OsclAny *)
        {
            if (iObserver) iObserver->OutputMemoryAvailable();
        }

    private:
        void LogDiagnostics();

        // Codec and encoder settings
        CPvGsmAmrEncoder *iGsmEncoder;
        TEncodeProperties *iEncProps;

        // Encoding Settings
        uint32	iInputSamplingRate;
        uint32	iInputBitsPerSample;
        uint32	iInputNumChannels;
        PVMF_GSMAMR_Rate iOutputBitrate;
        PVMFFormatType	iOutputFormat;
        uint32	iMaxNumOutputFramesPerBuffer;
        uint32	iMaxOutputBufferSize;
        int32	*iSizeArrayForOutputFrames;
        uint8	*iInternalInputBuffer;
        uint32	iMax_input_size;
        uint32	iOneInputFrameLength;
        uint32	iPosForInputMediaDataBuffer;
        uint32	iNextStartTime;
        bool	iInitialized;

        // Generic memory allocator
        PvmfAmrEncDataProcessorAlloc iAlloc;

        // Output media data allocator and memory pool
        PvmfAmrEncDataProcessorMemPool iMediaOutMemoryPool;
        PvmfAmrEncBufferAlloc *iMediaOutAlloc;

        // Output media data object memory pool
        PvmfAmrEncDataProcessorMemPool iMediaDataMemPool;

        // Input data queue
        PvmfAmrEncMediaDataVector iIncomingDataQueue;

        PVLogger* iLogger;	/** Logger */
        uint32 iSeqNum;		/** Sequence number */
        uint32 iExtensionRefCount; /** Reference counter for extension interface */

        PVLogger* iDiagnosticsLogger;

#if PROFILING_ON
        uint32 total_ticks;
        uint32 total_frames;
        uint32 iFinalTimeStamp;
        uint32 iMinEncDuration;
        uint32 iMaxEncDuration;
        uint32 iAverageEncDuration;
        bool oDiagnosticsLogged;
#endif

};

#endif // PVMF_AMRENC_DATA_PROCESSOR_H_INCLUDED

