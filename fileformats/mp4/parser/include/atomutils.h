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
/*********************************************************************************/
/*     -------------------------------------------------------------------       */
/*                            MPEG-4 AtomUtils Class                             */
/*     -------------------------------------------------------------------       */
/*********************************************************************************/
/*
    This AtomUtils Class contains sime useful methods for operating on Atoms
*/


#ifndef ATOMUTILS_H_INCLUDED
#define ATOMUTILS_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDE
#include "oscl_base.h"
#endif

#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif

#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif

#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef OSCL_MEM_BASIC_FUNCTIONS_H
#include "oscl_mem_basic_functions.h"
#endif

/* CPM Related Header Files */
#ifndef CPM_H_INCLUDED
#include "cpm.h"
#endif
#ifndef PVFILE_H_INCLUDED
#include "pvfile.h"
#endif

typedef Oscl_File* MP4_FF_FILE_REFERENCE;

#define PVMF_MP4FFPARSER_LOGDIAGNOSTICS(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iDiagnosticsLogger,PVLOGMSG_INFO,m);
#define PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG,iStateVarLogger,PVLOGMSG_INFO,m);
#define PVMF_MP4FFPARSER_LOGPARSEDINFO(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG,iParsedDataLogger,PVLOGMSG_INFO,m);
#define PVMF_MP4FFPARSER_LOGERROR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

#define PV_MP4_FF_NEW(auditCB,T,params,ptr)\
{\
ptr = OSCL_NEW(T,params);\
}


#define PV_MP4_FF_DELETE(auditCB,T,ptr)\
{\
OSCL_DELETE(ptr);\
}

#define PV_MP4_FF_TEMPLATED_DELETE(auditCB,T,Tsimple,ptr)\
{\
OSCL_TEMPLATED_DELETE(ptr, T, Tsimple);\
}


#define PV_MP4_FF_ARRAY_MALLOC(auditCB,T,count,ptr)\
{\
	ptr = (T*)OSCL_MALLOC(count);\
}


#define PV_MP4_ARRAY_FREE(auditCB,ptr)\
{\
	OSCL_FREE(ptr);\
}

#define PV_MP4_FF_ARRAY_NEW(auditCB, T, count, ptr)\
{\
	ptr = OSCL_ARRAY_NEW(T, count);\
}

#define PV_MP4_ARRAY_DELETE(auditCB, ptr)\
{\
	OSCL_ARRAY_DELETE(ptr);\
}


class MP4_FF_FILE
{
    public:
        MP4_FF_FILE()
                : _fileSize(0)
                , _fileServSession(NULL)
        {
        }
        MP4_FF_FILE(const MP4_FF_FILE& a)
                : _fileSize(a._fileSize)
                , _fileServSession(a._fileServSession)
                , _pvfile(a._pvfile)
        {
        }
        bool IsOpen()
        {
            return _pvfile.IsOpen();
        }

        int32                 _fileSize;
        Oscl_FileServer*      _fileServSession;
        PVFile                _pvfile;
};

class AtomUtils
{

    public:
        // Methods for reading in data from a file stream
        static bool read64(MP4_FF_FILE *fp, uint64 &data);
        static bool read32(MP4_FF_FILE *fp, uint32 &data);
        static bool read32read32(MP4_FF_FILE *fp, uint32 &data1, uint32 &data2);
        static bool read24(MP4_FF_FILE *fp, uint32 &data);
        static bool read16(MP4_FF_FILE *fp, uint16 &data);
        static bool read16read16(MP4_FF_FILE *fp, uint16 &data1, uint16 &data2);
        static bool read8(MP4_FF_FILE *fp, uint8 &data);
        static bool read8read8(MP4_FF_FILE *fp, uint8 &data1, uint8 &data2);
        static bool readNullTerminatedString(MP4_FF_FILE *fp, OSCL_wString& data);

        static bool readNullTerminatedUnicodeString(MP4_FF_FILE *fp, OSCL_wString& data);
        static bool readNullTerminatedAsciiString(MP4_FF_FILE *fp, OSCL_wString& data);

        static bool readByteData(MP4_FF_FILE *fp, uint32 length, uint8 *data);

        // Method to set time value in seconds since 1904
        static void setTime(uint32& ulTime);

        // This method is used to calculate the number of bytes needed to store the
        // overall size of the class - the value contentSize is the size of the class
        // NOT including the actual _sizeOfClass field (since this is a variable-length
        // field).  This is used when determining the actual _sizeOfClass value for
        // all the Descriptor and Command classes.
        static uint32 getNumberOfBytesUsedToStoreSizeOfClass(uint32 contentSize);

        // Returns the atom type from parsing the input stream
        OSCL_IMPORT_REF static void getNextAtomType(MP4_FF_FILE *fp, uint32 &size, uint32 &type);
        static int32 getNextAtomSize(MP4_FF_FILE *fp);
        static uint32 getMediaTypeFromHandlerType(uint32 handlerType);
        static uint32 getNumberOfBytesUsedToStoreContent(uint32 sizeOfClass);

        // Peeks and returns the next Nth byte from the file
        static uint32 peekNextNthBytes(MP4_FF_FILE *fp, int32 n);
        static uint8  peekNextByte(MP4_FF_FILE *fp);
        OSCL_IMPORT_REF static void   seekFromCurrPos(MP4_FF_FILE *fp, uint32 n);
        OSCL_IMPORT_REF static void   seekFromStart(MP4_FF_FILE *fp, uint32 n);
        OSCL_IMPORT_REF static void   seekToEnd(MP4_FF_FILE *fp);
        static void   rewindFilePointerByN(MP4_FF_FILE *fp, uint32 n);
        OSCL_IMPORT_REF static int32  getCurrentFilePosition(MP4_FF_FILE *fp);
        OSCL_IMPORT_REF static int32  OpenMP4File(OSCL_wString& filename, uint32 mode, MP4_FF_FILE *fp);
        OSCL_IMPORT_REF static int32  CloseMP4File(MP4_FF_FILE *fp);
        OSCL_IMPORT_REF static int32  Flush(MP4_FF_FILE *fp);
        OSCL_IMPORT_REF static bool   getCurrentFileSize(MP4_FF_FILE *fp, uint32& aCurrentSize);

        static bool read32(uint8 *&buf, uint32 &data);
        static bool read32read32(uint8 *&buf, uint32 &data1, uint32 &data2);
        static bool read16(uint8 *&buf, uint16 &data);
        static bool read8(uint8 *&buf, uint8 &data);
        static bool readByteData(uint8 *&buf, uint32 length, uint8 *data);
        static uint32 getNextAtomType(uint8 *buf);
        static int32 getNextAtomSize(uint8 *buf);

        // returns content length
        static uint32 getContentLength(MP4_FF_FILE *fp);
        // returns the data stream cache size
        static uint32 getFileBufferingCapacity(MP4_FF_FILE *fp);
        // peek to a new offset (MBDS only), read pointer does not change
        // this is used to trigger a http request
        static void skipFromStart(MP4_FF_FILE *fp, uint32 n);
        // for progressive playback seeking
        // returns the range of available bytes in MBDS
        // first + last byte offset inclusive
        static void getCurrentByteRange(MP4_FF_FILE *fp, uint32& aCurrentFirstByteOffset, uint32& aCurrentLastByteOffset);


};

#endif // ATOMUTILS_H_INCLUDED


