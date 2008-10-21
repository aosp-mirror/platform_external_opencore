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

#ifndef OSCLCONFIG_PROC_CHECK_H_INCLUDED
#define OSCLCONFIG_PROC_CHECK_H_INCLUDED





/**
type TOsclThreadId should be defined as the type used as
a thread ID
on the target platform.
Example:
typedef DWORD TOsclThreadId;
*/
typedef TOsclThreadId __verify__TOsclThreadId__defined__;

/**
type TOsclThreadFuncRet should be defined as the type used as
a thread function return value
on the target platform.
Example:
typedef DWORD TOsclThreadFuncRet;
*/
typedef TOsclThreadFuncRet __verify__TOsclThreadFuncRet__defined__;

/**
type TOsclThreadFuncArg should be defined as the type used as
a thread function argument
on the target platform.
Example:
typedef LPVOID TOsclThreadFuncArg;
*/
typedef TOsclThreadFuncArg __verify__TOsclThreadFuncArg__defined__;

/**
OSCL_THREAD_DECL macro should be defined to the
necessary function declaration modifiers for thread routines,
or a null macro if no modifiers are needed.
Example:
#define OSCL_THREAD_DECL WINAPI
*/
#ifndef OSCL_THREAD_DECL
#error "ERROR: OSCL_THREAD_DECL has to be defined."
#endif

/**
Example of a declaration of a thread routine called MyThreadMain using
the Oscl definitions:

static TOsclThreadFuncRet OSCL_THREAD_DECL MyThreadMain(TOsclThreadFuncArg arg);
*/

/**
type TOsclThreadObject should be defined as the type used as
a thread object or handle
on the target platform.
Example:
typedef pthread_t TOsclThreadObject;
*/
typedef TOsclThreadObject __verify__TOsclThreadObject__defined__;

/**
type TOsclMutexObject should be defined as the type used as
a mutex object or handle
on the target platform.
Example:
typedef pthread_mutex_t TOsclMutexObject;
*/
typedef TOsclMutexObject __verify__TOsclMutexObject__defined__;

/**
type TOsclSemaphoreObject should be defined as the type used as
a mutex object or handle
on the target platform.
Example:
typedef sem_t TOsclSemaphoreObject;
*/
typedef TOsclSemaphoreObject __verify__TOsclSemaphoreObject__defined__;

/**
type TOsclConditionObject should be defined as the type used as
a condition variable
on the target platform.
Example:
typedef pthread_cond_t TOsclConditionObject;

Note: Condition variables are only used with certain semaphore implementations.
If the semaphore implementation does not require a condition variable,
then this type can be defined as 'int' as follows:
typedef int TOsclConditionObject; //not used
*/
typedef TOsclConditionObject __verify__TOsclConditionObject__defined__;


#endif //OSCLCONFIG_PROC_CHECK_H_INCLUDED


