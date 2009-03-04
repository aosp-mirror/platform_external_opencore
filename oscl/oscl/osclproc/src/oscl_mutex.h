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

//                     O S C L _ M U T E X

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/**
 *  @file oscl_mutex.h
 *  @brief This file provides implementation of mutex
 *
 */

#ifndef OSCL_MUTEX_H_INCLUDED
#define OSCL_MUTEX_H_INCLUDED

#ifndef OSCLCONFIG_PROC_H_INCLUDED
#include "osclconfig_proc.h"
#endif
#ifndef OSCL_TYPES_H_INCLUDED
#include "oscl_types.h"
#endif
#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_THREAD_H_INCLUDED
#include "oscl_thread.h"
#endif
#ifndef OSCL_LOCK_BASE_H_INCLUDED
#include "oscl_lock_base.h"
#endif

/**
 * Class OsclMutex
 */
class OsclMutex : public OsclLockBase
{
    public:

        /**
         * Class constructor
         */
        OSCL_IMPORT_REF OsclMutex();

        /**
         * Class destructor
         */
        OSCL_IMPORT_REF virtual ~OsclMutex();

        /**
         * Creates the Mutex
         *
         * @param No input arguments
         *
         * @return Returns the Error whether it is success or failure.
         * Incase of failure it will return what is the specific error
         */
        OSCL_IMPORT_REF OsclProcStatus::eOsclProcError Create(void);


        /**
         * Locks the Mutex
         *
         * @param It wont take any parameters
         *
         * @return Returns nothing
         */
        OSCL_IMPORT_REF void Lock();

        /**
         * Try to lock the mutex,if the Mutex is already locked calling thread
         * immediately returns with out blocking
         * @param It wont take any parameters
         *
         * @return Returns SUCCESS_ERROR if the mutex was acquired,
         * MUTEX_LOCKED_ERROR if the mutex cannot be acquired without waiting,
         * or an error code if the operation failed.
         * Note: this function may not be supported on all platforms, and
         * may return NOT_IMPLEMENTED.
         */
        OSCL_IMPORT_REF OsclProcStatus::eOsclProcError TryLock();


        /**
         * Releases the Mutex
         *
         * @param It wont take any parameters
         *
         * @return Returns nothing
         */
        OSCL_IMPORT_REF void Unlock();


        /**
         * Closes the Mutex
         *
         * @param It wont take any prameters
         *
         * @return Returns the Error whether it is success or failure.
         * Incase of failure it will return what is the specific error
         */
        OSCL_IMPORT_REF OsclProcStatus::eOsclProcError Close(void);

    private:

        /**
         * Error Mapping
         *
         * @param It will take error returned by OS specific API
         *
         * @return Returns specific error
         */
        OsclProcStatus::eOsclProcError ErrorMapping(int32 Error);

        TOsclMutexObject    ObjMutex;
        bool bCreated;

};

#ifndef OSCL_LOCK_BASE_H_INCLUDED
#include "oscl_lock_base.h"
#endif

/**
** An implementation of OsclLockBase using a mutex
**/
class OsclThreadLock: public OsclLockBase
{
    public:
        OSCL_IMPORT_REF OsclThreadLock();
        OSCL_IMPORT_REF virtual ~OsclThreadLock();
        OSCL_IMPORT_REF void Lock();
        OSCL_IMPORT_REF void Unlock();
    private:
        OsclMutex iMutex;
};

#endif



