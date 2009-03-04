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
/** \file oscl_execpanic.h
    \brief Definition of ExecutionObject/Scheduler panic codes.
*/
#ifndef OSCL_EXECPANIC_H_INCLUDED
#define OSCL_EXECPANIC_H_INCLUDED

#ifndef OSCLCONFIG_ERROR_H_INCLUDED
#include "osclconfig_error.h"
#endif


/**
//ExecutionObject Panic codes for category "PVEXEC"
*/
enum OsclExecPanic
{
    EExecStillReadyOnDestruct = 40, //
    EExecAlreadyAdded = 41, //
    EExecAlreadyActive = 42, //
    //EExecSchedulerAlreadyExists=43,
    //EExecSchedulerDoesNotExist=44,
    //EExecTooManyStops=45,
    EExecStrayEvent = 46,
    EExecObjectLeave = 47,
    EExecNull = 48, //
    EExecNotAdded = 49, //

    ETrapPopAcrossLevels = 63, //
    ETrapPopUnderflow = 64, //
    ETrapLevelUnderflow = 65, //
    ETrapPushAtLevelZero = 66,
    //ETrapNoCleanupOperation=67,
    ETrapNoFreeSlotItem = 68,
    //ETrapNoHandlerInstalled=69,
    ETrapPopCountNegative = 70, //
    ETrapLevelNotEmpty = 71, //
};

#define PV_EXECPANIC(x) OsclError::Panic("PVEXEC",x)


#endif

