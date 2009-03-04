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
/*! \addtogroup osclproc OSCL Proc
 *
 * @{
 */


/** \file oscl_scheduler_types.h
    \brief Scheduler common types include file.
*/

/** This file defines the OsclExecBase, OsclTimerBase, and OsclExecSchedulerBase
    classes.  These are the base classes for PV AOs and PV Scheduler.
	We want the PV exec objects to be usable with either a PV scheduler
	or a non-PV native Symbian scheduler.  We also want the PV scheduler to be
	usable with non-PV exec objects.  Therefore, the PV scheduler and AO classes
	derived from Symbian classes on Symbian platforms. On non-Symbian platforms,
	the PV classes derive from classes with a similar API to the Symbian classes.
  */

#ifndef OSCL_SCHEDULER_TYPES_H_INCLUDED
#define OSCL_SCHEDULER_TYPES_H_INCLUDED

#ifndef OSCLCONFIG_PROC_H_INCLUDED
#include "osclconfig_proc.h"
#endif

#ifndef OSCL_EXECPANIC_H_INCLUDED
#include "oscl_execpanic.h"
#endif


//Non-Symbian

#ifndef OSCL_AOSTATUS_H_INCLUDED
#include "oscl_aostatus.h"
#endif

#ifndef OSCL_HEAPBASE_H_INCLUDED
#include "oscl_heapbase.h"
#endif

/*
** OsclExecBase is the base class for OsclActiveObject.
** On non-symbian, PVActiveBase implements the AO, so
** nothing is really needed here.
*/
typedef OsclHeapBase OsclActiveObj;

/*
** OsclTimerBase is the base class for OsclTimerObject.
** On non-symbian, PVActiveBase implements the AO, so
** nothing is really needed here.
*/
typedef OsclHeapBase OsclTimerBase;


/** OsclActiveSchedulerBase is the base for OsclExecScheduler.
  The non-Symbian OsclActiveSchedulerBase class is functionally similar to
  a subset of Symbian CActiveScheduler class.
*/
class OsclExecSchedulerBase : public OsclHeapBase
{
    private:
        virtual void Error(int32 anError) const;
        virtual void OnStarting();
        virtual void OnStopping();
        OsclExecSchedulerBase();
        ~OsclExecSchedulerBase();
        friend class OsclExecScheduler;
        friend class OsclCoeActiveScheduler;
        friend class PVActiveBase;
};




#endif //


/*! @} */
