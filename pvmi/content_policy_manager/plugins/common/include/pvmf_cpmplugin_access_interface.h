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
#ifndef PVMF_CPMPLUGIN_ACCESS_INTERFACE_H_INCLUDED
#define PVMF_CPMPLUGIN_ACCESS_INTERFACE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_TYPES_H_INCLUDED
#include "oscl_types.h"
#endif
#ifndef PV_UUID_H_INCLUDED
#include "pv_uuid.h"
#endif
#ifndef PV_INTERFACE_H_INCLUDED
#include "pv_interface.h"
#endif
#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif
#ifndef PVMF_RETURN_CODES_H_INCLUDED
#include "pvmf_return_codes.h"
#endif
#ifndef PVMI_DATA_STREAM_INTERFACE_H_INCLUDED
#include "pvmi_data_stream_interface.h"
#endif

#define PVMF_CPMPLUGIN_ACCESS_INTERFACE_MIMETYPE "pvxxx/pvmf/cpm/plugin/access_interface"
#define PVMFCPMPluginAccessInterfaceUuid PVUuid(0xc5f05532,0x7bcb,0x4186,0x85,0xa6,0xfa,0x9e,0x42,0x2d,0xb7,0x6d)

/**
 * Base Content Access Interface for all Content Policy Manager Plugins
 */
class PVMFCPMPluginAccessInterface : public PVInterface
{
    public:
        virtual void Init(void) = 0;
        virtual void Reset(void) = 0;
};


#define PVMF_CPMPLUGIN_LOCAL_SYNC_ACCESS_INTERFACE_MIMETYPE "pvxxx/pvmf/cpm/plugin/local_sync_access_interface"
#define PVMFCPMPluginLocalSyncAccessInterfaceUuid PVUuid(0xa3aa0c20,0xab74,0x4b52,0xaa,0xae,0x76,0x05,0xe8,0x31,0x3c,0x11)

/**
 * Local Synchronous Content Access Interface for Content Policy Manager Plugins
 */
class PVMFCPMPluginLocalSyncAccessInterface : public PVMFCPMPluginAccessInterface
{
    public:
        /**
         * Opens the registered content.
         *
         *
         * @return returns 0 if successful and a non-zero value otherwise
         */
        virtual int32 OpenContent() = 0;

        /**
         * The File Read & Decrypt operation
         * Reads from the file into the buffer a maximum of 'numelements'
         * of size 'size'.
         *
         * @param buffer pointer to buffer of type void
         * @param size   element size in bytes
         * @param numelements
         *               max number of elements to read
         *
         * @return returns the number of full elements actually read, which
         *         may be less than count if an error occurs or if the end
         *         of the file is encountered before reaching count. Use the
         *         CheckEndOfFile or GetError function to distinguish a read
         *         error from an end-of-file condition.
         */
        virtual uint32 ReadAndUnlockContent(OsclAny *buffer,
                                            uint32 size,
                                            uint32 numelements) = 0;

        /**
         * The File Seek operation
         * Sets the position for file pointer
         *
         * @param offset offset from the specified origin.
         * @param origin starting point
         *
         * @return returns 0 on success, and a non-zero value otherwise
         */
        virtual int32 SeekContent(int32 offset,
                                  Oscl_File::seek_type origin) = 0;

        /**
         * The File Tell operation
         * Returns the current file position for file specified by fp
         */
        virtual int32 GetCurrentContentPosition() = 0;

        /**
         * The File Size operation
         * Returns the file size
         */
        virtual int32 GetContentSize() = 0;

        /**
         * The File Close operation
         * Closes the file after flushing any remaining data in the
         * buffers.
         *
         * @return returns 0 if successful, and a non-zero value otherwise
         */
        virtual int32 CloseContent() = 0;

        /**
         * The File Flush operation
         * On an output stream OSCL_FileFlush causes any buffered
         * but unwritten data to be written to the file.
         *
         * @return returns 0 if successful, and a non-zero value otherwise
         */
        virtual int32 Flush() = 0;

        /**
         * The File Error operation
         * If no error has occurred on stream, returns 0. Otherwise,
         * it returns a nonzero value
         *
         * @return
         */
        virtual int32 GetContentAccessError() = 0;
        /**
         * Determines if the content is drm protected or not.
         *
         * @param aProtected set to true if protected, false otherwise.
         * Value undefined in case of error
         *
         * @return returns PVMFSuccess if successful and an appropriate errcode
         * otherwise
         */
        virtual PVMFStatus IsContentProtected(bool& aProtected) = 0;
};

#define PVMF_CPMPLUGIN_DECRYPTION_INTERFACE_MIMETYPE "pvxxx/pvmf/cpm/plugin/remote_sync_access_interface"
#define PVMFCPMPluginDecryptionInterfaceUuid PVUuid(0x1e14b2fe,0x947b,0x49c8,0x8b,0x11,0xe9,0xec,0x4c,0x11,0xa2,0x8f)

/**
 * Access Unit Decryption Interface for Content Policy Manager Plugins
 */
class PVMFCPMPluginAccessUnitDecryptionInterface : public PVMFCPMPluginAccessInterface
{
    public:
        /**
         * DecryptAccessUnit operation
         * Decrypts input buffer of size 'aInputSizeInBytes'
         *
         * @param aInputBuffer pointer to input buffer containing
         *                     encrypted data of type uint8*
         *
         * @param aInputBufferSizeInBytes  input buffer size in bytes,
         *
         * @param aOutputBuffer pointer to output buffer containing
         *                      decrypted data of type uint8*,
            *                      memory for the buffer allocated by the caller
         *
         * @param aOutputBufferSizeInBytes   size of allocated output buffer
         *                                   in bytes
         *
         * @return returns true on successful decryption, false otherwise
         */
        virtual bool DecryptAccessUnit(uint8*& aInputBuffer,
                                       uint32  aInputBufferSizeInBytes,
                                       uint8*& aOutputBuffer,
                                       uint32& aOutputBufferSizeInBytes,
                                       uint32  aTrackID = 0,
                                       uint32  aAccesUnitTimeStamp = 0) = 0;
        virtual int32 GetDecryptError() = 0;

        /**
        * For in-place decryption
        */
        virtual bool CanDecryptInPlace() = 0;
        virtual bool DecryptAccessUnit(uint8*& aInputBuffer,
                                       uint32  aInputBufferSizeInBytes,
                                       uint32  aTrackID = 0,
                                       uint32  aAccesUnitTimeStamp = 0) = 0;

        /**
        * Optional version of DecryptAccessUnit with a decrypt context
        */
        virtual bool DecryptAccessUnit(uint8*& aInputBuffer,
                                       uint32  aInputBufferSizeInBytes,
                                       uint32  aTrackID ,
                                       uint32  aAccesUnitTimeStamp,
                                       PVInterface* aContext = NULL)
        {
            OSCL_UNUSED_ARG(aInputBuffer);
            OSCL_UNUSED_ARG(aInputBufferSizeInBytes);
            OSCL_UNUSED_ARG(aTrackID);
            OSCL_UNUSED_ARG(aAccesUnitTimeStamp);
            OSCL_UNUSED_ARG(aContext);
            return false;
        }

};


#endif //PVMF_CPMPLUGIN_ACCESS_INTERFACE_H_INCLUDED

