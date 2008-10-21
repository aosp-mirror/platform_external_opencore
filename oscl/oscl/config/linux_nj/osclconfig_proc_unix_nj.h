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

//     O S C L C O N F I G _ P R O C  ( P L A T F O R M   C O N F I G   I N F O )

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_proc.h
 *  \brief This file contains configuration information for the linux platform
 *
 */

#ifndef OSCLCONFIG_PROC_UNIX_NJ_H_INCLUDED
#define OSCLCONFIG_PROC_UNIX_NJ_H_INCLUDED



//semaphore with advanced realtime features incl. timed wait.
//#include <time.h>
//#include <semaphore.h>

//pthreads

#include <pthread.h>
#include <errno.h>
#include <signal.h>

// threads, mutex, semaphores
typedef pthread_t TOsclThreadId;
typedef void* TOsclThreadFuncArg;
typedef void* TOsclThreadFuncRet;
#define OSCL_THREAD_DECL
typedef pthread_t TOsclThreadObject;
typedef pthread_mutex_t TOsclMutexObject;
typedef int TOsclSemaphoreObject;
typedef pthread_cond_t TOsclConditionObject;

#endif
