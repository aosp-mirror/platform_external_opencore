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
 * @file pvmf_amrenc_data_processor_interface.h
 * @brief Data processing interface for PvmfAmrEncNode
 */
#ifndef PVMF_AMRENC_DATA_PROCESSOR_INTERFACE_H_INCLUDED
#define PVMF_AMRENC_DATA_PROCESSOR_INTERFACE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif
#ifndef OSCL_DEFALLOC_H_INCLUDED
#include "oscl_defalloc.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#include "pvmf_format_type.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_RETURN_CODES_H_INCLUDED
#include "pvmf_return_codes.h"
#endif

class PvmfAmrEncCapabilityConfig
{
    public:
        virtual ~PvmfAmrEncCapabilityConfig() {}
        /**
         * Sets the sampling rate of the unencoded input data
         * @param aSamplingRate, sampling rate
         * @return PVMFSuccess if successful, else see PVMF return code.
         */
        virtual PVMFStatus SetInputSamplingRate(uint32 aSamplingRate) = 0;

        /**
         * Sets the bits per sample of unencoded input data.
         * @param aBitsPerSample, number of bits per sample
         * @return PVMFSuccess if successful, else see PVMF return code.
         */
        virtual PVMFStatus SetInputBitsPerSample(uint32 aBitsPerSample) = 0;

        /**
         * Sets the frame size of unencoded input data.
         * @param aNumChannels, number of channels
         * @return PVMFSuccess if successful, else see PVMF return code.
         */
        virtual PVMFStatus SetInputNumChannels(uint32 aNumChannels) = 0;

        /**
         * Sets the format (IF2 or IETF) of encoded output.
         * @param aFormat, IF2 or IETF format, use PVMF format type, PVMF_AMR_IETF or PVMF_AMR_IF2
         * @return PVMFSuccess if successful, else see PVMF return code.
         */
        virtual PVMFStatus SetOutputFormat(PVMFFormatType aFormat) = 0;

        /**
         * Gets the format of encoded output
         * @return Format type
         */
        virtual PVMFFormatType GetOutputFormat() = 0;

        /**
         * Gets the bitrate of encoded output.
         * @return Output bitrate of encoded output.
         */
        virtual uint32 GetOutputBitRate() = 0;
};

/**
 * This enumeration is especially for the result of the following Process() interface
 * It lists a bunch of the return values(cases) for this API.
 */
typedef enum
{
    // output media data can be generated, but the data in the buffer queue is not enough (or even no data) to continue processing
    ProcessComplete_NotEnoughDataInQueue		   = 0,
    // output media data can be generated, but the data in the buffer queue is enough to continue processing
    ProcessComplete_EnoughDataToContinueProcessing = 1,

    // the data in the buffer is not enough to generate one output media data, needs to get more data
    // Note that the data in the buffer queue is not consumed any more, so the data needs to be held in the queue. The user is not
    // responsible to remove the data in the buffer queue.
    ProcessInComplete		 = 2,

    // different errors
    ProcessError			 = -1,
    ProcessNotInitialized    = -2, // initialization not done yet
    ProcessNoOutputMemory    = -3
}PvmfAmrEncDataProcessResult;

/** PVNodeDataPrecessor observer class is designed to delegate the callback from the memory pool, i.e.,
**  OsclMemPoolFixedChunkAllocatorObserver::freechunkavailable(). And the memory pool is typically used
**  inside PVNodeDataPrecessor class for memory allocation for the output (processed) data
*/
class PvmfAmrEncDataProcessorObserver
{
    public:
        virtual ~PvmfAmrEncDataProcessorObserver() {}
        virtual void OutputMemoryAvailable(void) = 0;
        virtual void ProcessComplete(PvmfAmrEncDataProcessResult aResult, PVMFSharedMediaDataPtr& aMediaDataOut) = 0;
};


/** PVNodeDataPrecessor class is designed to provide a (small) set of interfaces that seperate basic node implementation(
 ** state transition, port, command and event handling etc.) and data processing specific (i.e. codec specific) implementation.
 ** It basically has three interfaces:
 ** (1) virtual function QueryInterface(), this is for node extension interface, for example, some encoder nodes need extension interfaces to get
 **     encoding parameters set before the actual encoding. For other nodes that do not have extension interface, the base class provides the
 **     default implementation (i.e. not supported). That is why this fucntion is set as non-pure virtual function.
 ** (2) pure virtual function Initialize(), this is the initialization function. The derived class has to implement this function.
 ** (3) pure virtual function Process(), this is the actual data processing function, for codec, this is the actual encoding or decoding function
 **     The derived class has to implement it by its own. Note that the memory allocation for the output processed data usually happens inside this
 **     function.
 **
 ** Note that no clean up function is specified, since the destructor needs to take care of resource deallocation
 **
 */
class PvmfAmrEncDataProcessorInterface : public PvmfAmrEncCapabilityConfig
{
    public:
        PvmfAmrEncDataProcessorInterface() : iObserver(NULL) {}
        virtual ~PvmfAmrEncDataProcessorInterface() {};

        void SetObserver(PvmfAmrEncDataProcessorObserver* aObserver)
        {
            iObserver = aObserver;
        }
        virtual PVMFStatus ThreadLogon() = 0;
        virtual PVMFStatus ThreadLogoff() = 0;

        /**
         * Retrieve the extension interface if there is, otherwise return PVMFErrNotSupported.
         * Here provide the default implementation for those derived classes that don't need extension interface
         *
         * @param "uuid" "the uuid for the extension interface to be queried"
         * @param "aface" "the actual interface"
         * @return PVMFStatus, if the extension interface doesn't exist, return PVMFErrNotSupported
         */
        virtual PVMFStatus QueryInterface(const PVUuid& uuid, PVInterface*& aface) = 0;

        /**
        * It allows a caller to ask for all UUIDs associated with a particular MIME type.
        * If interfaces of the requested MIME type are found within the system, they are added
        * to the UUIDs array.
        *
        * Also added to the UUIDs array will be all interfaces which have the requested MIME
        * type as a base MIME type.  This functionality can be turned off.
        *
        * @param aMimeType The MIME type of the desired interfaces
        * @param aUuids A vector to hold the discovered UUIDs
        * @param aExactUuidsOnly Turns on/off the retrival of UUIDs with aMimeType as a base type
        * @returns Completion status
        */
        virtual PVMFStatus QueryUUID(const PvmfMimeString& aMimeType,
                                     Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                     bool aExactUuidsOnly = false) = 0;

        /**
         * Initialization function. For codec, this is for codec initialization. Right now
         * this is synchonized call, and doesn't
         * support asynchronized call.
         *
         * @return PVMFStatus
         */
        virtual PVMFStatus Initialize() = 0;

        /**
         * Resets the encoder
         * @return Completion status
         */
        virtual PVMFStatus Reset() = 0;

        /**
         * Data processing function. This interface is currently designed as synchronized call for data processing.
         * Basically, this function will take all the current available input data, do processing and probably generate one media data (
         * PVMFSharedMediaDataPtr type). The reason for taking all the available input data instead of one input frame is to handle arbitrary
         * input, and also hide the handling logic for arbitrary input inside this function, which I think makes things easier.
         *
         * Therefore, the caller (the node) only needs to pass the current input data queue down to this function, and is not responsible to
         * remove the input data in the data queue (except the final clean up, the node may need to check if the input data queue has data or
         * not, and may remove them all). This function will take care of it. In addition, the caller only needs to check the return value to
         * see if the output medida data is available.
         *
         * @param "aIncomingDataQueue" "current input data queue for a node"
         * @param "aMediaDataOut" "one output media data that contains the processed data"
         * @return ProcessResult, see the above description
         */

        virtual PVMFStatus QueueIncomingData(PVMFSharedMediaDataPtr& aMediaDataIn) = 0;

        virtual void Encode() = 0;

        /**
         * This API will set the observer object for issuing a callback when the output memory becomes available
         *
         * @param "aObserver" "reference of PvmfAmrEncDataProcessorObserver object to be passed down to PVNodeDataPrecessor"
         * @return PVMFStatus
         */
        //
        void NotifyMemoryAvailable(PvmfAmrEncDataProcessorObserver& aObserver)
        {
            iObserver = &aObserver;
        }

    protected:
        PvmfAmrEncDataProcessorObserver *iObserver;
};


#endif // PVMF_AMRENC_DATA_PROCESSOR_INTERFACE_H_INCLUDED

