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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//                     O S C L _ T L S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclbase OSCL Base
 *
 * @{
 */


/**
 *  @file oscl_TLS.h
 *  @brief This file defines the OsclTLS template class. This class
 *         provides a container which used to give access to a single instance
 *         of a class within the scope of the TLS.
 *
 */

#ifndef OSCL_TLS_H_INCLUDED
#define OSCL_TLS_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

#ifndef OSCL_DEFALLOC_H_INCLUDED
#include "oscl_defalloc.h"
#endif


#if (OSCL_TLS_IS_KEYED)

//Keyed TLS requires global variable support

//Maximum number of thread slots for keyed TLS.
#define OSCL_TLS_MAX_THREADS 128

#else

//unused value.
typedef OsclAny TOsclTlsKey;

#endif //OSCL_TLS_IS_KEYED


// list of TLS objects
const uint32 OSCL_TLS_ID_OSCLMEM        = 0;
const uint32 OSCL_TLS_ID_PVLOGGER       = 1;
const uint32 OSCL_TLS_ID_TEST           = 2;
const uint32 OSCL_TLS_ID_PVSCHEDULER    = 3;
const uint32 OSCL_TLS_ID_PVERRORTRAP    = 4;
const uint32 OSCL_TLS_ID_SDPMEDIAPARSER = 5;
const uint32 OSCL_TLS_ID_PAYLOADPARSER  = 6;
const uint32 OSCL_TLS_ID_PVMFRECOGNIZER = 7;
const uint32 OSCL_TLS_ID_WMDRM = 8;
const uint32 OSCL_TLS_ID_OSCLREGISTRY = 9;
const uint32 OSCL_TLS_ID_SQLITE3        = 10;
const uint32 OSCL_TLS_ID_BASE_LAST      = 10; // should always equal the largest ID defined here

#define OSCL_TLS_BASE_SLOTS OSCL_TLS_ID_BASE_LAST +1

//There may be additional slots defined in the osclconfig.h for the build.
#ifndef OSCL_TLS_EXTERNAL_SLOTS
#define OSCL_TLS_EXTERNAL_SLOTS 0
#endif

//#define OSCL_TLS_MAX_SLOTS ( OSCL_TLS_BASE_SLOTS + OSCL_TLS_EXTERNAL_SLOTS)
#define OSCL_TLS_MAX_SLOTS 64

class TLSStorageOps
{
    public:
        OSCL_IMPORT_REF static void save_registry(TOsclTlsKey* key, OsclAny* ptr, int32&);
        OSCL_IMPORT_REF static OsclAny* get_registry(TOsclTlsKey* key);
};

class OsclTLSRegistry
{
    public:
        /*
        ** Get an entry
        ** @param ID: identifier
        ** @param error (output) 0 for success or an error from TPVBasePanicEnum
        ** @returns: the entry value
        */
        OSCL_IMPORT_REF static OsclAny* getInstance(uint32 ID, int32 &error);
        /*
        ** Set an entry
        ** @param ID: identifier
        ** @param error (output) 0 for success or an error from TPVBasePanicEnum
        ** @returns: the entry value
        */
        OSCL_IMPORT_REF static void registerInstance(OsclAny* ptr, uint32 ID, int32 &error);

    private:
        OsclTLSRegistry()
        {}
        typedef OsclAny* registry_type;
        typedef registry_type* registry_pointer_type;

#if ( OSCL_TLS_IS_KEYED)
        class TKeyItem
        {
            public:
                TKeyItem(): iTlsKey(NULL), iThreadId(0)
                {}
                TOsclTlsKey *iTlsKey;
                TOsclTlsThreadId iThreadId;
        };
        class TlsKeyTable
        {
            public:
                TlsKeyTable(): iNumKeys(0)
                {}
                _OsclBasicLock iLock;
                uint32 iNumKeys;
                TKeyItem iKeys[OSCL_TLS_MAX_THREADS];
        };

        //The key table is a global variable.
        static TlsKeyTable* iTlsKeyTable;

        static void GetThreadId(TOsclTlsThreadId &threadId, int32&);
        static TOsclTlsKey* LookupTlsKey(int32&);
        static bool SaveTlsKey(TOsclTlsKey* key, int32&);
        static bool RemoveTlsKey(Oscl_DefAlloc& alloc, TOsclTlsKey* key, int32&);
#endif

    private:
        OSCL_IMPORT_REF static void initialize(Oscl_DefAlloc &alloc, int32 &error);
        OSCL_IMPORT_REF static void cleanup(Oscl_DefAlloc &alloc, int32 &error);
        friend class OsclBase;

};

template < class T, uint32 ID, class Registry = OsclTLSRegistry > class OsclTLS
{
    private:
        // make the copy constructor and assignment operator private
        OsclTLS& operator=(OsclTLS& _Y)
        {
            return(*this);
        }

    protected:
        T* _Ptr;
        int32 _error;

    public:
        OsclTLS(): _Ptr(OSCL_STATIC_CAST(T*, Registry::getInstance(ID, _error))) {};

        ~OsclTLS() {};

        /**
        * @brief The indirection operator (*) accesses a value indirectly,
        * through a pointer
        *
        * This operator ensures that the OsclTLS can be used like the
        * regular pointer that it was initialized with.
        */
        T& operator*() const
        {
            return(*_Ptr);
        }

        /**
        * @brief The indirection operator (->) accesses a value indirectly,
        * through a pointer
        *
        * This operator ensures that the OsclTLS can be used like the
        * regular pointer that it was initialized with.
        */
        T *operator->() const
        {
            return(_Ptr);
        }


        /**
        * @brief set() method sets ownership to the pointer, passed.
        * This method is needed when the class is created with a default
        * constructor. Returns false in case the class is non-empty.
        *
        */
        bool set()
        {
            _Ptr = OSCL_STATIC_CAST(T*, Registry::getInstance(ID, _error));
            return (_Ptr ? true : false);
        }

};

/*! @} */



#endif

