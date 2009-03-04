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


#include "oscl_scheduler.h"


#include "oscl_error.h"
#include "oscl_tickcount.h"
#include "pvlogger.h"
#include "oscl_error_trapcleanup.h"
#include "pvlogger.h"
#include "oscl_tls.h"
#include "oscl_int64_utils.h"

#define OSCL_DISABLE_WARNING_CONDITIONAL_IS_CONSTANT
#include "osclconfig_compiler_warnings.h"

#include "oscl_scheduler_tuneables.h"

#if(PV_SCHED_LOG_Q)
#define LOGQ(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iLogger,PVLOGMSG_DEBUG,m);
#else
#define LOGQ(m)
#endif

//LOGERROR is for scheduler errors.
//This logging also goes to stderr on platforms with ANSI stdio.
#define LOGERROR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

//LOGNOTICE is for scheduler start/stop, install/uninstall notices.
#define LOGNOTICE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG,iLogger,PVLOGMSG_NOTICE,m);

#if(PV_SCHED_ENABLE_PERF_LOGGING)
//LOGSTATS is for logging the AO summary statistics.  These are loggged either
//when the AO is deleted, or when the scheduling ends.
#define LOGSTATS(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF,iLogger,PVLOGMSG_INFO,m);
//LOGPERF is for detailed performance logging.
#define LOGPERF_LEVEL PVLOGMSG_INFO
#define LOGPERF(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF,iLogger,LOGPERF_LEVEL,m);
//LOGPERF2 is for highest detail on scheduler activity.
#define LOGPERF2_LEVEL PVLOGMSG_INFO+1
#define LOGPERF2(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF,iLogger,LOGPERF2_LEVEL,m);
//max perf log string length
#define LOGPERFMAXSTR 64
//runtime check for whether perf logging level is enabled.
#define DO_LOGPERF (iLogger->GetLogLevel()<0||iLogger->GetLogLevel()>=LOGPERF_LEVEL)
#else
#define LOGSTATS(m)
#define LOGPERF(m)
#define LOGPERF2(m)
#define LOGPERFMAXSTR 64
#define DO_LOGPERF (0)
#endif//(PV_SCHED_ENABLE_PERF_LOGGING)

#ifndef OSCL_COMBINED_DLL
#include "oscl_dll.h"
OSCL_DLL_ENTRY_POINT_DEFAULT()
#endif

//
// OsclScheduler
//


OSCL_EXPORT_REF void OsclScheduler::Init(const char *name, Oscl_DefAlloc *alloc, int nreserve)
//Init the scheduler for this thread.
{
    int32 err;
    OSCL_TRY(err,
             OsclExecScheduler *sched = OsclExecScheduler::NewL(name, alloc, nreserve);
             sched->InstallScheduler(););
    if (err != OsclErrNone)
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);
}


OSCL_EXPORT_REF void OsclScheduler::Cleanup()
//Cleanup the scheduler for this thread.
{
    OsclExecSchedulerCommonBase *sched = OsclExecSchedulerCommonBase::GetScheduler();
    if (!sched)
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);
    sched->UninstallScheduler();
    Oscl_DefAlloc *alloc = sched->iAlloc;
    sched->~OsclExecSchedulerCommonBase();
    alloc->deallocate(sched);
}

//
// OsclExecSchedulerCommonBase
//

//
// OsclExecScheduler
//

//use the TLS registry, or the singleton registry if no TLS.
#include "oscl_error.h"
#define PVSCHEDULER_REGISTRY OsclTLSRegistryEx
#define PVSCHEDULER_REGISTRY_ID OSCL_TLS_ID_PVSCHEDULER

//For AO statistics.
#if !(PV_SCHED_ENABLE_AO_STATS)
//no stats
#define PVTICK uint32
#define SET_TICK(tick)
#define GET_TICKFREQ(tick)
#define TICK_INT(tick1)
#define TICKSTR ""
#else
//else use the oscl timer.
#define PVTICK uint32
#define SET_TICK(tick) tick=OsclTickCount::TickCount()
#define GET_TICKFREQ(tick) tick=OsclTickCount::TickCountFrequency()
#define TICK_INT(tick1) tick1
#define TICKSTR "Ticks"
#endif

#if !(PV_SCHED_ENABLE_AO_STATS)
#define DIFF_TICK(tick1,diff)
#define UPDATE_RUNERROR_TIME(x,y)
#define UPDATE_RUNL_TIME(x,y)
#define UPDATE_LEAVE_CODE(x,err)
#else
#define DIFF_TICK(tick1,diff) PVTICK _now;SET_TICK(_now);diff=TICK_INT(_now)-TICK_INT(tick1)
#define UPDATE_RUNERROR_TIME(stats,delta)\
		if(stats->i64Valid) stats->i64TotalTicksInRunL+=delta;\
		else stats->iTotalTicksInRunL+=delta;\
		stats->iNumRunError++
#define UPDATE_RUNL_TIME(stats,delta)\
		if(stats->i64Valid) stats->i64TotalTicksInRunL+=delta;\
		else stats->iTotalTicksInRunL+=delta;\
		stats->iNumRun++;
#define UPDATE_LEAVE_CODE(stats,err)if (err!=OsclErrNone)stats->iLeave=err
#endif

#if (PV_SCHED_ENABLE_LOOP_STATS)
#define DECLARE_LOOP_STATS\
		int64 loopdelta = 0;\
		PVTICK looptime;

#define START_LOOP_STATS(stats)SET_TICK(looptime);

#define START_WAIT_LOOP_STATS(rc,stats)\
		if (rc<1)\
			SET_TICK(looptime);

#define END_LOOP_STATS(stats)\
		{\
			DIFF_TICK(looptime,loopdelta);\
			if(stats->i64Valid) stats->i64TotalTicksInRunL+=loopdelta;\
			else stats->iTotalTicksInRunL+=loopdelta;\
			stats->iNumRun++;\
			LOGPERF((0,"PVSCHED: Run %d %s AO %s",(int32)loopdelta,TICKSTR,stats->iAOName.get_cstr()));\
		}

#define END_WAIT_LOOP_STATS(rc,stats)\
		if (rc<1)\
		{\
			DIFF_TICK(looptime,loopdelta);\
			if(stats->i64Valid) stats->i64TotalTicksInRunL+=loopdelta;\
			else stats->iTotalTicksInRunL+=loopdelta;\
			stats->iNumRun++;\
			LOGPERF((0,"PVSCHED: Run %d %s AO %s",(int32)loopdelta,TICKSTR,stats->iAOName.get_cstr()));\
		}
#else
#define DECLARE_LOOP_STATS
#define START_LOOP_STATS(stats)
#define START_WAIT_LOOP_STATS(rc,stats)
#define END_LOOP_STATS(stats)
#define END_WAIT_LOOP_STATS(rc,stats)
#endif

#if(PV_SCHED_ENABLE_PERF_LOGGING)
void OsclExecSchedulerCommonBase::ResetLogPerf()
{
    //print total time spend in prior interval of continuous Run calls.
    if (iLogPerfTotal > 0)
    {
        LOGPERF((0, "PVSCHED: Prior Interval %d %s", iLogPerfTotal, TICKSTR));
    }
    //reset interval.
    iLogPerfTotal = 0;
    //reset indentation to zero.
    iLogPerfIndentStrLen = 0;
    if (iLogPerfIndentStr)
        iLogPerfIndentStr[iLogPerfIndentStrLen] = '\0';
}

void OsclExecSchedulerCommonBase::IncLogPerf(uint32 delta)
{
    //add to total interval time.
    iLogPerfTotal += delta;
    //add a space to the indent string up to the max.
    if (iLogPerfIndentStr
            && iLogPerfIndentStrLen < LOGPERFMAXSTR)
    {
        iLogPerfIndentStr[iLogPerfIndentStrLen++] = ' ';
        iLogPerfIndentStr[iLogPerfIndentStrLen] = '\0';
    }
}
//end perf logging.
#endif//(PV_SCHED_ENABLE_PERF_LOGGING)

OsclExecSchedulerCommonBase* OsclExecSchedulerCommonBase::GetScheduler()
//static function to get currently installed scheduler
//for this thread.
{
    OsclExecSchedulerCommonBase *current = (OsclExecSchedulerCommonBase*)PVSCHEDULER_REGISTRY::getInstance(PVSCHEDULER_REGISTRY_ID);
    return current;
}

OsclExecSchedulerCommonBase* OsclExecSchedulerCommonBase::SetScheduler(OsclExecSchedulerCommonBase *a)
//static function to set currently installed scheduler
//for this thread. return previous scheduler, if any.
{
    OsclExecSchedulerCommonBase* temp = GetScheduler();
    PVSCHEDULER_REGISTRY::registerInstance(a, PVSCHEDULER_REGISTRY_ID);
    return temp;
}

OSCL_EXPORT_REF OsclNameString<PVSCHEDNAMELEN> *OsclExecSchedulerCommonBase::GetName()
//static function to get scheduler name for this thread.
{
    OsclExecSchedulerCommonBase *sched = GetScheduler();
    if (sched)
        return &sched->iName;
    else
        return NULL;
}

OSCL_EXPORT_REF uint32 OsclExecSchedulerCommonBase::GetId()
{
    return PVThreadContext::Id();
}

OsclExecScheduler * OsclExecScheduler::NewL(const char *name, Oscl_DefAlloc *alloc, int nreserve)
{
    OsclExecScheduler *self;
    OsclMemAllocator defalloc;
    OsclAny* ptr = (alloc) ? alloc->ALLOCATE(sizeof(OsclExecScheduler))
                   : defalloc.ALLOCATE(sizeof(OsclExecScheduler));
    OsclError::LeaveIfNull(ptr);
    self = OSCL_PLACEMENT_NEW(ptr, OsclExecScheduler(alloc));
    OsclError::PushL(self);
    self->ConstructL(name, nreserve);
    OsclError::Pop();
    return self;
}

OsclExecSchedulerCommonBase::~OsclExecSchedulerCommonBase()
{
    //make sure scheduler is not currently installed in
    //any thread.
    if (IsInstalled())
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotStopped);

    if (iStopper)
    {
        iStopper->~PVSchedulerStopper();
        iAlloc->deallocate(iStopper);
    }
#if(PV_SCHED_ENABLE_PERF_LOGGING)
    if (iLogPerfIndentStr)
        _oscl_free(iLogPerfIndentStr);
#endif
}

OsclExecScheduler::~OsclExecScheduler()
{
}

OsclExecSchedulerCommonBase::OsclExecSchedulerCommonBase(Oscl_DefAlloc *alloc)
{
    iAlloc = (alloc) ? alloc : &iDefAlloc;
#if(PV_SCHED_ENABLE_PERF_LOGGING)
    iLogPerfIndentStr = NULL;
    iLogPerfTotal = 0;
#endif
    iCallback = NULL;
}

OsclExecScheduler::OsclExecScheduler(Oscl_DefAlloc *alloc)
        : OsclExecSchedulerCommonBase(alloc)
{
}

void OsclExecSchedulerCommonBase::ConstructL(const char *name, int nreserve)
{
    iNumAOAdded = 1;

    OsclAny* ptr = iAlloc->ALLOCATE(sizeof(PVSchedulerStopper));
    OsclError::LeaveIfNull(ptr);
    iStopper = new(ptr) PVSchedulerStopper;

#if(PV_SCHED_ENABLE_AO_STATS)
    //create a placeholder for summary stats for
    //all AOs that are not PVActiveBase.
    for (uint32 i = 0;i < EOtherExecStats_Last;i++)
        iOtherExecStats[i] = NULL;

    ptr = iAlloc->ALLOCATE(sizeof(PVActiveStats));
    OsclError::LeaveIfNull(ptr);
    iOtherExecStats[EOtherExecStats_NativeOS] = OSCL_PLACEMENT_NEW(ptr, PVActiveStats(this, "Sched_TotalNativeOS", NULL));
    //init the stat queue offset.
    {
        int offset = (int) & (iOtherExecStats[EOtherExecStats_NativeOS])->iPVStatQLink - (int)(iOtherExecStats[EOtherExecStats_NativeOS]);
        iPVStatQ.SetOffset(offset);
    }

#if(PV_SCHED_ENABLE_LOOP_STATS)
    //create nodes for summary stats for scheduler loop time

    ptr = iAlloc->ALLOCATE(sizeof(PVActiveStats));
    OsclError::LeaveIfNull(ptr);
    iOtherExecStats[EOtherExecStats_LockTime] = OSCL_PLACEMENT_NEW(ptr, PVActiveStats(this, "Sched_LockTime", NULL));

    ptr = iAlloc->ALLOCATE(sizeof(PVActiveStats));
    OsclError::LeaveIfNull(ptr);
    iOtherExecStats[EOtherExecStats_QueueTime] = OSCL_PLACEMENT_NEW(ptr, PVActiveStats(this, "Sched_QueueTime", NULL));

    ptr = iAlloc->ALLOCATE(sizeof(PVActiveStats));
    OsclError::LeaveIfNull(ptr);
    iOtherExecStats[EOtherExecStats_WaitTime] = OSCL_PLACEMENT_NEW(ptr, PVActiveStats(this, "Sched_WaitTime", NULL));
#endif
    //add the non-AO stats nodes to the stat Q.
    {
        for (uint32 i = 0;i < EOtherExecStats_Last;i++)
        {
            if (iOtherExecStats[i])
                iPVStatQ.InsertTail(*iOtherExecStats[i]);
        }
    }

#endif //PV_SCHED_ENABLE_AO_STATS

    InitExecQ(nreserve);

    iBlockingMode = false;
    iNativeMode = false;
    iName.Set(name);
    iLogger = PVLogger::GetLoggerObject("pvscheduler");
#if (PV_SCHED_ENABLE_PERF_LOGGING)
    if (DO_LOGPERF)
    {
        iLogPerfIndentStr = (char*)_oscl_malloc(LOGPERFMAXSTR + 1);
        OsclError::LeaveIfNull(iLogPerfIndentStr);
        ResetLogPerf();
    }
#endif
}

void OsclExecScheduler::ConstructL(const char *name, int nreserve)
{
    OsclExecSchedulerCommonBase::ConstructL(name, nreserve);
}

void OsclExecSchedulerCommonBase::InstallScheduler()
{
    //make sure this scheduler is not installed in
    //any thread.
    if (IsInstalled())
        PV_SCHEDULERPANIC(EPVPanicSchedulerAlreadyInstalled);
    //make sure no scheduler is installed in this thread.
    if (GetScheduler())
        PV_SCHEDULERPANIC(EPVPanicSchedulerAlreadyInstalled);

    SetScheduler(this);

    iThreadContext.EnterThreadContext();

    iErrorTrapImp = OsclErrorTrap::GetErrorTrapImp();
    if (!iErrorTrapImp)
        PV_SCHEDULERPANIC(EPVPanicErrorTrapNotInstalled);

    if (iStopperCrit.Create() != OsclProcStatus::SUCCESS_ERROR)
        PV_SCHEDULERPANIC(EPVPanicMutexError);

    iResumeSem.Create();
    iDoStop = iDoSuspend = iSuspended = false;

    iReadyQ.Open();

    LOGNOTICE((0, "PVSCHED:Scheduler '%s', Thread 0x%x: Installed", iName.Str(), PVThreadContext::Id()));

#if (PV_SCHED_ENABLE_PERF_LOGGING)
    if (DO_LOGPERF)
    {//print tick frequencies that will show up in the perf log.
        PVTICK f;
        GET_TICKFREQ(f);
        LOGPERF((0, "PVSCHED: %s frequency %u", TICKSTR, TICK_INT(f)));
        uint32 freq = OsclTickCount::TickCountFrequency();
        LOGPERF((0, "PVSCHED: %s frequency %u", "Ticks", freq));
        OSCL_UNUSED_ARG(freq);
    }
#endif
}

void OsclExecSchedulerCommonBase::UninstallScheduler()
{
    //make sure this scheduler is currently installed in
    //this thread.
    if (!IsInstalled() || GetScheduler() != this)
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);


    if (iBlockingMode)
    {
        //in case a thread panic happened, go ahead and end
        //scheduling.
        OsclErrorTrapImp *trap = OsclErrorTrapImp::GetErrorTrap();
        if (trap
                && trap->iPanic.iReason != OsclErrNone)
            EndScheduling();
        //make sure scheduler is stopped.  If not, panic instead.
        if (IsStarted())
            PV_SCHEDULERPANIC(EPVPanicSchedulerNotStopped);
    }
    else if (IsStarted())
    {
        //end non-blocking scheduling
        EndScheduling();
    }

    SetScheduler(NULL);

    iThreadContext.ExitThreadContext();

    CleanupExecQ();

    //Cleanup the stat queue.
#if(PV_SCHED_ENABLE_AO_STATS)
    CleanupStatQ();
#endif

    if (iStopperCrit.Close() != OsclProcStatus::SUCCESS_ERROR)
        PV_SCHEDULERPANIC(EPVPanicMutexError);

    iReadyQ.Close();
    iResumeSem.Close();

    LOGNOTICE((0, "PVSCHED:Scheduler '%s', Thread 0x%x: Uninstalled", iName.Str(), PVThreadContext::Id()));
}

OSCL_EXPORT_REF OsclExecScheduler* OsclExecScheduler::Current()
//static routine to get current scheduler.
{
    return (OsclExecScheduler*)GetScheduler();
}

bool OsclExecSchedulerCommonBase::IsStarted()
{
    iStopperCrit.Lock();
    bool val = (iStopper->IsAdded()) ? true : false;
    iStopperCrit.Unlock();
    return val;
}

inline bool OsclExecSchedulerCommonBase::IsInstalled()
{
    return iThreadContext.iOpen;
}

void OsclExecSchedulerCommonBase::BeginScheduling(bool blocking, bool native)
//called before entering scheduling loop.
{
    //make sure scheduler is installed...
    if (!IsInstalled() || GetScheduler() != this)
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);

    //make sure scheduler is idle...
    if (IsStarted())
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotStopped);

    iBlockingMode = blocking;
    iNativeMode = native;

    //Add stopper AO to scheduler.
    iStopperCrit.Lock();
    {
        iStopper->AddToScheduler();
        iStopper->PendForExec();
    }
    iStopperCrit.Unlock();

#if(PV_SCHED_ENABLE_PERF_LOGGING)
    ResetLogPerf();
#endif
#if(PV_SCHED_ENABLE_AO_STATS)
    BeginStats();
#endif
}


void OsclExecSchedulerCommonBase::EndScheduling()
//called after exiting scheduling loop.
{
    //see if it's already stopped..
    if (!IsStarted())
        return;

    //remove stopper AO.
    iStopperCrit.Lock();
    iStopper->RemoveFromScheduler();
    iStopperCrit.Unlock();

#if(PV_SCHED_ENABLE_AO_STATS)
    EndStats();
#endif
}

#if(PV_SCHED_ENABLE_AO_STATS)
void OsclExecSchedulerCommonBase::ShowStats(PVActiveStats *active)
//static routine to print stats for a PV AO.
{
    if (!active)
        return;

    //don't print any AOs that never ran.
    if ((active->iNumRun + active->iNumCancel) == 0)
        return;

    PVLogger* iLogger = PVLogger::GetLoggerObject("pvscheduler");
    if (active->i64Valid)
    {
        int64 avgTicksPerRunL = (active->iNumRun == 0) ? 0 : active->i64TotalTicksInRunL / (int64)active->iNumRun;
        LOGSTATS((0, "PVSCHED:Scheduler '%s', AO '%s': NumRunL %d, AvgTicksPerRunL (hi,lo) (0x%x,0x%08x) Units %s, NumCancel %d, NumError %d, LeaveCode %d"
                  , active->iScheduler->iName.Str()
                  , active->iAOName.get_cstr()
                  , active->iNumRun
                  , Oscl_Int64_Utils::get_int64_upper32(avgTicksPerRunL)
                  , Oscl_Int64_Utils::get_int64_lower32(avgTicksPerRunL)
                  , TICKSTR
                  , active->iNumCancel
                  , active->iNumRunError
                  , active->iLeave
                 ));
    }
    else
    {
        uint32 avgTicksPerRunL = (active->iNumRun == 0) ? 0 : active->iTotalTicksInRunL / active->iNumRun;
        LOGSTATS((0, "PVSCHED:Scheduler '%s', AO '%s': NumRunL %d, AvgTicksPerRunL %d Units %s, NumCancel %d, NumError %d, LeaveCode %d"
                  , active->iScheduler->iName.Str()
                  , active->iAOName.get_cstr()
                  , active->iNumRun
                  , avgTicksPerRunL
                  , TICKSTR
                  , active->iNumCancel
                  , active->iNumRunError
                  , active->iLeave
                 ));
    }
}

void OsclExecSchedulerCommonBase::ShowSummaryStats(PVActiveStats *active, PVLogger*logger, int64 total, int64& aGrandTotal, float& aTotalPercent)
//static routine to print stats for a PV AO.
{
    if (total == (int64)0)
        return;//to avoid divide by zero

    if (!active)
        return;

    //don't print any AO's that never ran.
    if ((active->iNumRun + active->iNumCancel) == 0)
        return;

    //calculate percent of the total time that was spent in this AO.
    if (active->i64Valid)
    {
        active->iPercent = 100.0 * active->i64TotalTicksInRunL / total;
        aGrandTotal += active->i64TotalTicksInRunL;
    }
    else
    {
        active->iPercent = 100.0 * active->iTotalTicksInRunL / total;
        aGrandTotal += active->iTotalTicksInRunL;
    }
    aTotalPercent += active->iPercent;

    int32 fraction = (int32)active->iPercent;
    float decimal = active->iPercent - fraction;
    decimal *= 100.0;

    //print results
    if (active->i64Valid)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO
                        , (0, "  TIME PERCENT %d.%02d, AO '%s', NumRunL %d, TotalTicksInRunL Hi,Lo (0x%x,0x%08x), NumCancel %d, NumError %d, LeaveCode %d, NumInstance %d"
                           , (int32)active->iPercent, (int32)decimal
                           , active->iAOName.get_cstr()
                           , active->iNumRun
                           , Oscl_Int64_Utils::get_int64_upper32(active->i64TotalTicksInRunL)
                           , Oscl_Int64_Utils::get_int64_lower32(active->i64TotalTicksInRunL)
                           , active->iNumCancel
                           , active->iNumRunError
                           , active->iLeave
                           , active->iNumInstances
                          ));
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO
                        , (0, "  TIME PERCENT %d.%02d, AO '%s', NumRunL %d, TotalTicksInRunL 0x%x, NumCancel %d, NumError %d, LeaveCode %d, NumInstance %d"
                           , (int32)active->iPercent, (int32)decimal
                           , active->iAOName.get_cstr()
                           , active->iNumRun
                           , (int32)active->iTotalTicksInRunL
                           , active->iNumCancel
                           , active->iNumRunError
                           , active->iLeave
                           , active->iNumInstances
                          ));
    }
}

void OsclExecSchedulerCommonBase::BeginStats()
//Begin stats for all AOs.
{
    iTotalTicksTemp = (uint8*)OSCL_MALLOC(sizeof(PVTICK));
    SET_TICK(*((PVTICK*)iTotalTicksTemp));
}

void OsclExecSchedulerCommonBase::EndStats()
//End stats for all AOs.
{
    //get the end time for the scheduler run.
    int64 total;
    DIFF_TICK((*((PVTICK*)iTotalTicksTemp)), total);
    OSCL_FREE(iTotalTicksTemp);

    //there may be multiple entries per AO in the stats table, so combine them now.
    if (!iPVStatQ.IsEmpty())
    {
        OsclDoubleRunner<PVActiveStats> iter(iPVStatQ);
        PVActiveStats *item;
        for (iter.SetToHead(); ;iter++)
        {
            item = iter;
            //find all subsequent entries in the list that have
            //the same AO name as this entry.
            if (item->iNumInstances > 0
                    && !iPVStatQ.IsTail(item))
            {
                OsclDoubleRunner<PVActiveStats> iter2(iPVStatQ);
                PVActiveStats* item2;
                for (iter2 = iter, iter2++; ;iter2++)
                {
                    item2 = iter2;
                    if (item2->iAOName == item->iAOName)
                    {
                        item->Combine(*item2);
                        //mark this entry to ignore in further processing.
                        item2->iNumInstances = 0;
                    }
                    if (iPVStatQ.IsTail(item2))
                        break;
                }
            }
            if (iPVStatQ.IsTail(item))
                break;
        }
    }
    //end of multiple-instance combine.

    QUE_ITER_BEGIN(PVActiveStats, iPVStatQ)
    {
        if (item
                && item->iNumInstances > 0)
            OsclExecScheduler::ShowStats(item);
    }
    QUE_ITER_END(iPVStatQ)

    //Show summary stats

    PVLogger* logger = PVLogger::GetLoggerObject("OsclSchedulerPerfStats");

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO,
                    (0, "OSCL SCHEDULER SUMMARY STATISTICS FOR SCHEDULER '%s'", iName.Str())
                   )

    iGrandTotalTicks = 0;
    iTotalPercent = 0.0;
    QUE_ITER_BEGIN(PVActiveStats, iPVStatQ)
    {
        if (item
                && item->iNumInstances > 0)
            OsclExecScheduler::ShowSummaryStats(item, logger, total, iGrandTotalTicks, iTotalPercent);
    }
    QUE_ITER_END(iPVStatQ)

    //split total percent into whole & decimal parts.
    int32 fraction = (int32)iTotalPercent;
    float decimal = iTotalPercent - fraction;
    decimal *= 100.0;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO,
                    (0, "   Total Time (hi,lo): (0x%x,0x%08x) Units: %s", Oscl_Int64_Utils::get_int64_upper32(total), Oscl_Int64_Utils::get_int64_lower32(total), TICKSTR)
                   )
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO,
                    (0, "   Total Time Accounted (hi,lo): (0x%x,0x%08x) Units: %s", Oscl_Int64_Utils::get_int64_upper32(iGrandTotalTicks), Oscl_Int64_Utils::get_int64_lower32(iGrandTotalTicks), TICKSTR)
                   )
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO,
                    (0, "   Total Percent Accounted: %d.%02d", (int32)iTotalPercent, (int32)decimal)
                   )

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, logger, PVLOGMSG_INFO,
                    (0, "END OSCL SCHEDULER SUMMARY STATISTICS FOR SCHEDULER '%s'", iName.Str())
                   )

}
#endif //PV_SCHED_ENABLE_AO_STATS




void OsclExecSchedulerCommonBase::Error(int32 anError) const
//call this when any AO leaves and its error handler does not handle the error.
{
    LOGERROR((0, "PVSCHED:Scheduler '%s', Thread 0x%x: Error! Reason %d", iName.Str(), PVThreadContext::Id(), anError));
    fprintf(stderr, "PVSCHED:Scheduler '%s', Thread 0x%x: Error! Reason %d\n", iName.Str(), PVThreadContext::Id(), anError);

    //propagate the leave
    OsclError::Leave(anError);
}


OSCL_EXPORT_REF void OsclExecSchedulerCommonBase::StartScheduler(OsclSemaphore *aSignal)
//blocking call to start PV scheduler.
//Will leave if any AO leaves.
{

    BeginScheduling(true, false);//blocking, non-native
    if (aSignal)
        aSignal->Signal();


    LOGNOTICE((0, "PVSCHED:Scheduler '%s', Thread 0x%x: Starting PV Scheduling Loop", iName.Str(), PVThreadContext::Id()));

    int32 err;
    OSCL_TRY(err, BlockingLoopL(););

    LOGNOTICE((0, "PVSCHED:Scheduler '%s', Thread 0x%x: Exited PV Scheduling Loop", iName.Str(), PVThreadContext::Id()));

    EndScheduling();

    if (err)
        OsclError::Leave(err);

}


OSCL_EXPORT_REF void OsclExecSchedulerCommonBase::StartNativeScheduler()
//blocking call to start native scheduler.
//Will leave if any AO leaves.
{
    PV_SCHEDULERPANIC(EPVPanicNativeSchedulerError);
}

//scheduler stopper request status codes.
#define STOPPER_REQUEST_STOP_NATIVE 0
#define STOPPER_REQUEST_STOP_PV 1
#define STOPPER_REQUEST_SUSPEND 2

OSCL_EXPORT_REF void OsclExecSchedulerCommonBase::StopScheduler()
//any thread can use this to stop the blocking scheduler.
{
    if (!IsInstalled())
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);

    if (!iBlockingMode)
        OsclError::Leave(OsclErrNotReady);
    if (!IsStarted())
        return ;

    if (iStopper->iStatus != OSCL_REQUEST_PENDING)
        OsclError::Leave(OsclErrNotReady);

    //in case scheduler is in the suspend loop...
    if (iDoSuspend || iSuspended)
        iResumeSem.Signal();

    if (iNativeMode)
        iStopper->PendComplete(STOPPER_REQUEST_STOP_NATIVE);
    else
        iStopper->PendComplete(STOPPER_REQUEST_STOP_PV);

}

OSCL_EXPORT_REF void OsclExecSchedulerCommonBase::SuspendScheduler()
//any thread can use this to suspend the blocking scheduler.
{
    if (!IsInstalled())
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);

    if (iNativeMode)
        OsclError::Leave(OsclErrNotSupported);

    if (!iBlockingMode)
        OsclError::Leave(OsclErrNotSupported);

    if (!IsStarted())
        return;

    if (iStopper->iStatus != OSCL_REQUEST_PENDING)
        OsclError::Leave(OsclErrNotReady);

    iStopper->PendComplete(STOPPER_REQUEST_SUSPEND);

}

OSCL_EXPORT_REF void OsclExecSchedulerCommonBase::ResumeScheduler()
//any thread can use this to resume the blocking scheduler.
{
    if (!IsInstalled())
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);

    if (iDoSuspend || iSuspended)
    {
        iResumeSem.Signal();
        return ;
    }
    else
        OsclError::Leave(OsclErrNotReady); //not suspended.
}

OSCL_EXPORT_REF void OsclExecScheduler::RunSchedulerNonBlocking(int32 aCount, int32 &aReady, uint32 &aShortestDelay)
//run scheduler in non-blocking mode.
//Will leave if any AO leaves.
{


    aReady = 0;
    aShortestDelay = 0;

    //make sure this scheduler is installed.
    if (!IsInstalled())
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);

#if !(OSCL_RELEASE_BUILD)  && !defined(NDEBUG)
    //make sure this scheduler is really installed in this
    //thread.
    if (GetScheduler() != this)
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotInstalled);
#endif

    //start scheduling if needed.
    if (!IsStarted())
    {
        BeginScheduling(false, false);//nonblocking, non-native
    }
    else if (iBlockingMode || iNativeMode)
        PV_SCHEDULERPANIC(EPVPanicSchedulerNotStopped);

    //Process timers.  All ready timers will get
    //moved to the ready queue.
    UpdateTimersMsec(aShortestDelay);

    //Run until the requested count is reached, or there
    //aren't any AOs ready.
    for (int32 count = 0;count < aCount;)
    {
        //find highest pri ready AO.
        iReadyQ.Lock();
        PVActiveBase* pvactive = iReadyQ.PopTop();
        iReadyQ.Unlock();
        if (pvactive)
        {
            //run it
            count++;
            CallRunExec(pvactive);

            //re-evaluate timers
            UpdateTimersMsec(aShortestDelay);
        }
        else
            break;//nothing ready
    }

    //at this point, either nothing else is ready or the target count was reached.

    aReady = iReadyQ.Depth();

}

OSCL_EXPORT_REF void OsclExecScheduler::RegisterForCallback(OsclSchedulerObserver* aCallback, OsclAny* aCallbackContext)
{
    //Update the callback pointers.

    //Use the ready Q lock to avoid thread contention over
    //callback pointer.
    iReadyQ.Lock();
    int32 depth = iReadyQ.Depth();
    if (depth && aCallback)
    {
        //need to callback now!
        iCallback = NULL;
        aCallback->OsclSchedulerReadyCallback(aCallbackContext);
    }
    else
    {
        //save the new pointers.  Callback will happen when timer Q or ready Q is
        //updated.
        iCallback = aCallback;
        iCallbackContext = aCallbackContext;
    }
    iReadyQ.Unlock();
}

////////////////////////////////////////
// Queue management
////////////////////////////////////////


PVActiveBase * OsclExecSchedulerCommonBase::FindPVBase(PVActiveBase *active, OsclDoubleList<PVActiveBase> &q)
//Search a PVActiveBase queue, given a PVActiveBase ptr.
{
    QUE_ITER_BEGIN(PVActiveBase, q)
    {
        if (item == active)
            return item;
    }
    QUE_ITER_END(q)
    return NULL;
}

#if(PV_SCHED_ENABLE_AO_STATS)
void OsclExecSchedulerCommonBase::CleanupStatQ()
{
    while (!iPVStatQ.IsEmpty())
    {
        PVActiveStats* first = iPVStatQ.Head();
        first->iPVStatQLink.Remove();
        first->~PVActiveStats();
        first->iScheduler->iAlloc->deallocate(first);
    }
}
#endif

void OsclExecSchedulerCommonBase::CleanupExecQ()
{
    //Cleanup timers.
    {
        PVActiveBase *top;
        while ((top = iExecTimerQ.PopTop()))
            top->RemoveFromScheduler();
    }
    //Cleanup ready AOs.
    iReadyQ.Lock();
    PVActiveBase* top = iReadyQ.Top();
    iReadyQ.Unlock();
    while (top)
    {
        top->RemoveFromScheduler();
        iReadyQ.Lock();
        top = iReadyQ.Top();
        iReadyQ.Unlock();
    }
}

void OsclExecSchedulerCommonBase::InitExecQ(int nreserve)
//init the pvactive queues.
{
    iExecTimerQ.Init(nreserve, "ExecTimerQ");
    iReadyQ.Init(nreserve, "ReadyQ");
}

////////////////////////////////////////
// Ready queue management.
////////////////////////////////////////


void OsclExecSchedulerCommonBase::AddToExecTimerQ(PVActiveBase* anActive, uint32 aDelayMicrosec)
//timer implementation.
//Add an AO to the pending timer queue.
{
    if (!anActive)
        PV_EXECPANIC(EExecNull);

    //make sure this AO is not already added.
    if (OsclReadyQ::IsInAny(anActive))
        PV_EXECPANIC(EExecAlreadyAdded);

    //Set time in ticks when AO should run.

    uint32 tickperiod = OsclTickCount::TickCountPeriod();
    OSCL_ASSERT(tickperiod != 0);

    //Round to the nearest integer with the computation:
    //floor((2*Interval_usec/ticks_per_usec +  1)/2)
    //
    //The computed time may rollover the 32-bit value-- that's OK, because
    //the tick count will also rollover.
    uint32 timenow = OsclTickCount::TickCount();
    anActive->iPVReadyQLink.iTimeToRunTicks = timenow + (aDelayMicrosec * 2 / tickperiod + 1) / 2;

    if (aDelayMicrosec > 0)
    {
        LOGPERF2((0, "PVSCHED:%s AO %s Timer delay %d TimeToRunTicks %d Timenow %d"
                  , iLogPerfIndentStr, anActive->iName.Str()
                  , aDelayMicrosec, anActive->iPVReadyQLink.iTimeToRunTicks
                  , timenow));
    }

    //queue with timer sort
    iExecTimerQ.Add(anActive, true);

    //if this AO is in the front of the queue now, we need to do a
    //callback, because the shortest delay interval has changed.
    if (iCallback
            && anActive == iExecTimerQ.Top())
    {
        //must use the lock when updating callback pointer.
        iReadyQ.Lock();
        OsclSchedulerObserver* callback = iCallback;
        iCallback = NULL;
        iReadyQ.Unlock();
        //callback needs to happen outside the lock, to allow code
        //to register for additional callback.
        if (callback)
            callback->OsclSchedulerTimerCallback(iCallbackContext, aDelayMicrosec / 1000);
    }
}

//For 32-bit time comparisons with rollover handling.
//This value is (2^31)-1
const uint32 OsclExecSchedulerCommonBase::iTimeCompareThreshold = 0x7fffffff;

PVActiveBase* OsclExecSchedulerCommonBase::UpdateTimers(uint32 &aShortestDelay)
//timer processing.
//Complete requests for all timers that are ready now,
//then return the pending timer with the shortest delay if any.
//If any pending timer is returned it's the top of the queue so
//it can be discarded later with Pop.
{
    aShortestDelay = 0;

    PVActiveBase *top = iExecTimerQ.Top();

    if (!top)
        return NULL;

    uint32 timenow = OsclTickCount::TickCount();

    //Find all timers that are ready, and the first
    //timer that isn't.  The list is sorted by
    //time then priority.
    for (;top;top = iExecTimerQ.Top())
    {
        //calculate time to run <= timenow, taking possible rollover into account
        uint32 deltaTicks = timenow - top->iPVReadyQLink.iTimeToRunTicks;
        if (deltaTicks <= iTimeCompareThreshold)
        {
            //this timer is ready
            iExecTimerQ.Pop(top);

            PendComplete(top, OSCL_REQUEST_ERR_NONE, EPVThreadContext_InThread);
        }
        else
        {
            //we found the pending timer with the shortest delay.
            //get the delay value
            int32 delayTicks = deltaTicks;
            if (delayTicks < 0)
                delayTicks = (-delayTicks);
            aShortestDelay = delayTicks;
            return top;
        }
    }

    return NULL;//no pending timers.
}

PVActiveBase* OsclExecSchedulerCommonBase::UpdateTimersMsec(uint32 &aShortestDelay)
//Identical to UpdateTimers except the delay returned is milliseconds instead
//of ticks.
{
    aShortestDelay = 0;

    PVActiveBase *top = iExecTimerQ.Top();

    if (!top)
        return NULL;

    uint32 timenow = OsclTickCount::TickCount();

    //Find all timers that are ready, and the first
    //timer that isn't.  The list is sorted by
    //time then priority.
    for (;top;top = iExecTimerQ.Top())
    {
        //calculate time to run <= timenow, taking possible rollover into account
        uint32 deltaTicks = timenow - top->iPVReadyQLink.iTimeToRunTicks;
        if (deltaTicks <= iTimeCompareThreshold)
        {
            //this timer is ready
            iExecTimerQ.Pop(top);

            PendComplete(top, OSCL_REQUEST_ERR_NONE, EPVThreadContext_InThread);
        }
        else
        {
            //we found the pending timer with the shortest delay.
            //get the delay value
            int32 delayTicks = deltaTicks;
            if (delayTicks < 0)
                delayTicks = (-delayTicks);
            aShortestDelay = OsclTickCount::TicksToMsec(delayTicks);

            //if delay became zero after the conversion from ticks to msec,
            //then just consider this timer to be ready now.
            if (aShortestDelay == 0)
            {
                //this timer is ready
                iExecTimerQ.Pop(top);

                PendComplete(top, OSCL_REQUEST_ERR_NONE, EPVThreadContext_InThread);
            }
            else
            {
                return top;
            }
        }
    }

    return NULL;//no pending timers.
}

//scheduler has an allocator but there's no way to use
//it here since this allocator must be a template argument
//to oscl priority queue.
OsclAny* OsclReadyAlloc::allocate_fl(const uint32 size, const char * file_name, const int line_num)
{
    OsclAny*p = iBasicAlloc.allocate_fl(size, file_name, line_num);
    OsclError::LeaveIfNull(p);
    return p;
}
OsclAny* OsclReadyAlloc::allocate(const uint32 size)
{
    OsclAny*p = iBasicAlloc.ALLOCATE(size);
    OsclError::LeaveIfNull(p);
    return p;
}
void OsclReadyAlloc::deallocate(OsclAny* p)
{
    iBasicAlloc.deallocate(p);
}

//evalute "priority of a is less than priority of b"
int OsclReadyCompare::compare(TOsclReady& a, TOsclReady& b) const
{
    if (a->iPVReadyQLink.iTimerSort)
    {
        //first sort: by time to run.  Earlier "time to run" has precedence.
        if (a->iPVReadyQLink.iTimeToRunTicks != b->iPVReadyQLink.iTimeToRunTicks)
        {
            //calculate a>b, taking possible rollover into account.
            uint32 delta = b->iPVReadyQLink.iTimeToRunTicks - a->iPVReadyQLink.iTimeToRunTicks;
            return (delta > OsclExecScheduler::iTimeCompareThreshold);
        }
    }

    //second sort: by AO priority.  Higher priority has precedence.
    if (a->iPVReadyQLink.iAOPriority != b->iPVReadyQLink.iAOPriority)
        return (a->iPVReadyQLink.iAOPriority < b->iPVReadyQLink.iAOPriority);

    //if there was a priority tie, impose a FIFO order.

//This section allows switching between "fair scheduling" and linear
//behavior.  We always use fair scheduling, but for testing it can be helpful to
//swap in the linear behavior.
#if PV_SCHED_FAIR_SCHEDULING
    //third sort: by FIFO order, to create fair scheduling.
    //AOs that have been queued the longest have precedence.
    return (a->iPVReadyQLink.iSeqNum >= b->iPVReadyQLink.iSeqNum);
#else
    //third sort: by the order when AO was added to scheduler, to simulate
    //Symbian native ActiveScheduler behavior.
    //AOs that were added earlier have precedence.
    return (a->iAddedNum > b->iAddedNum);
#endif
//End fair scheduling option.
}


void OsclReadyQ::Init(int nreserve, const char* name)
{
    iSeqNumCounter = 0;
    if (nreserve > 0)
        c.reserve(nreserve);
    iName = name;
    iLogger = PVLogger::GetLoggerObject(name);
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Init", iName.get_cstr()));
}

void OsclReadyQ::Print()
{
#if(PV_SCHED_LOG_Q)
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Size %d", iName.get_cstr(), size()));
    for (uint32 i = 0;i < size();i++)
    {
        PVActiveBase* elem = vec()[i];
        LOGQ((0, " '%s' addr 0x%x pri %d active %d status %d TimeQueued %d"
              , elem->iName.Str()
              , elem
              , elem->iPVReadyQLink.iAOPriority
              , elem->iBusy
              , elem->iStatus
              , elem->iPVReadyQLink.iTimeQueuedTicks
             ));
        LOGQ((0, "    TimeToRunTicks %d TimerSort %d SeqNum %d IsIn 0x%x"
              , elem->iPVReadyQLink.iTimeToRunTicks
              , elem->iPVReadyQLink.iTimerSort
              , elem->iPVReadyQLink.iSeqNum
              , elem->iPVReadyQLink.iIsIn
             ));
        if (elem->iStats.iEnable)
        {
            LOGQ((0, "    NumRunL %d Leave %d NumCancel %d"
                  , elem->iStats.iNumRun
                  , elem->iStats.iLeave
                  , elem->iStats.iNumCancel
                 ));
        }
    }
#endif
}

void OsclReadyQ::Open()
{
    iSem.Create();
    iCrit.Create();
}

void OsclReadyQ::Close()
{
    iSem.Close();
    iCrit.Close();
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Close", iName.get_cstr()));
    iLogger = NULL;
}

void OsclReadyQ::Lock()
{
    iCrit.Lock();
}

void OsclReadyQ::Unlock()
{
    iCrit.Unlock();
}

void OsclReadyQ::Wait()
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Wait IN", iName.get_cstr()));
    iSem.Wait();
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Wait OUT", iName.get_cstr()));
}

bool OsclReadyQ::Wait(uint32 timeout)
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Wait %d IN", iName.get_cstr(), timeout));
    bool ok = (iSem.Wait(timeout) == OsclProcStatus::WAIT_TIMEOUT_ERROR);
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Wait %d OUT returning ok=%d", iName.get_cstr(), timeout, ok));
    return ok;
}

void OsclReadyQ::Signal(uint32 count)
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Signal %d IN", iName.get_cstr(), count));
    for (uint32 i = 0;i < count;i++)
        iSem.Signal();
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Signal %d OUT", iName.get_cstr(), count));
}

void OsclReadyQ::Clear()
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Clear IN", iName.get_cstr()));
    for (PVActiveBase *elem = PopTop();elem;elem = PopTop())
        ;
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Clear OUT", iName.get_cstr()));
}

bool OsclReadyQ::IsIn(TOsclReady b)
//tell if elemement is in this q.
{
    return (b->iPVReadyQLink.iIsIn == this);
}

bool OsclReadyQ::IsInMT(TOsclReady b)
//tell if elemement is in this q, with thread lock.
{
    Lock();
    bool val = (b->iPVReadyQLink.iIsIn == this);
    Unlock();
    return val;
}

bool OsclReadyQ::IsInAny(TOsclReady b)
//tell if elemement is in any q.
{
    return (b->iPVReadyQLink.iIsIn != NULL);
}

PVActiveBase* OsclReadyQ::PopTop()
//deque and return highest pri element.
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) PopTop IN", iName.get_cstr()));

    PVActiveBase*top = Top();
    if (top)
        Pop(top);

    LOGQ((0, "PVSCHED:OsclReadyQ(%s) PopTop top= 0x%x", iName.get_cstr(), top));
    if (top)
    {
        LOGQ((0, "PVSCHED:OsclReadyQ(%s) PopTop returning AO 0x%x '%s'", iName.get_cstr(), top, top->iName.Str()));
        OSCL_ASSERT(!IsInAny(top));
    }

    return top;
}

PVActiveBase* OsclReadyQ::Top()
//return highest pri element without removing.
{
    if (size() > 0)
        return top();
    return NULL;
}

void OsclReadyQ::Pop(TOsclReady b)
//remove queue top.
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Pop '%s' IN", iName.get_cstr(), b->iName.Str()));

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(b == Top());
    OSCL_ASSERT(size() > 0);
#endif

    b->iPVReadyQLink.iIsIn = NULL;

#if(PV_SCHED_CHECK_Q)
    uint32 n = size();
#endif

    pop();

#if(PV_SCHED_LOG_Q)
    Print();
#endif

    iSem.Wait();//won't block, just decrementing the sem.

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(size() == n - 1);
#endif

    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Pop '%s' OUT", iName.get_cstr(), b->iName.Str()));
}

void OsclReadyQ::Remove(TOsclReady a)
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Remove '%s' addr 0x%x IN"
          , iName.get_cstr(), a->iName.Str(), a));

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(IsIn(a));
    OSCL_ASSERT(size() > 0);
    uint32 n = size();
#endif

    a->iPVReadyQLink.iIsIn = NULL;

    int32 nfound = remove(a);

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(nfound == 1);
#endif

#if(PV_SCHED_LOG_Q)
    Print();
#endif

    if (nfound > 0)
        iSem.Wait();//won't block, just decrementing the sem.

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(size() == n - 1);
#endif

    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Remove '%s' OUT", iName.get_cstr(), a->iName.Str()));

}

void OsclReadyQ::Add(TOsclReady b, bool timersort)
{
    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Add IN AO '%s' timer %d seqnum %d", iName.get_cstr()
          , b->iName.Str(), timersort, iSeqNum + 1));

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(!IsInAny(b));
#endif

    b->iPVReadyQLink.iTimerSort = timersort;
    b->iPVReadyQLink.iIsIn = this;
    b->iPVReadyQLink.iTimeQueuedTicks = OsclTickCount::TickCount();
    //inc the sequence num for the FIFO sort.
    b->iPVReadyQLink.iSeqNum = ++iSeqNumCounter;

#if(PV_SCHED_CHECK_Q)
    uint32 n = size();
#endif

    push(b);

#if(PV_SCHED_LOG_Q)
    Print();
#endif

    iSem.Signal();

#if(PV_SCHED_CHECK_Q)
    OSCL_ASSERT(size() == n + 1);
#endif

    LOGQ((0, "PVSCHED:OsclReadyQ(%s) Add OUT", iName.get_cstr()));
}

uint32 OsclReadyQ::Depth()
{
    return size();
}


////////////////////////////////////////
// request handling
////////////////////////////////////////


void OsclExecSchedulerCommonBase::PendComplete(PVActiveBase *pvbase, int32 aReason, TPVThreadContext aThreadContext)
//complete a request for this scheduler.
//Calling context can be any thread.
{
    if (aThreadContext == EPVThreadContext_InThread)
    {
        LOGPERF2((0, "PVSCHED: %s AO %s Request complete", iLogPerfIndentStr, pvbase->iName.Str()));

        //for timer cancellation, the AO may still be queued at
        //this point, so remove it.
        //check thread context first so we don't try to access
        //timer Q from out-of-thread.
        if (iExecTimerQ.IsIn(pvbase))
            iExecTimerQ.Remove(pvbase);
    }

    iReadyQ.Lock();

    //make sure this AO is not already queued.
    if (OsclReadyQ::IsInAny(pvbase))
    {
        iReadyQ.Unlock();
        PV_EXECPANIC(EExecAlreadyAdded);
        return;
    }

    //make sure the AO has a request active
    if (!pvbase->iBusy
            || pvbase->iStatus != OSCL_REQUEST_PENDING)
    {
        iReadyQ.Unlock();
        PV_EXECPANIC(EExecStrayEvent);
        return;
    }

    //add to ready queue with priority sort.
    iReadyQ.Add(pvbase, false);

    //update the AO status
    pvbase->iStatus = aReason;

    //make scheduler callback if needed.
    //note: this needs to happen under the lock since we're updating
    //the callback pointer.
    if (iCallback)
    {
        iCallback->OsclSchedulerReadyCallback(iCallbackContext);
        iCallback = NULL;
    }

    iReadyQ.Unlock();
}

void OsclExecSchedulerCommonBase::RequestCanceled(PVActiveBase* pvbase)
{
    LOGPERF2((0, "PVSCHED: %s AO %s Request canceled", iLogPerfIndentStr, pvbase->iName.Str()));

    //This gets called right after the AO's DoCancel was
    //called.
    //Calling context is always in-thread.

    //See if the request was completed by the DoCancel.
    bool complete = iReadyQ.IsInMT(pvbase);

    if (!complete)
    {
        //If request is still pending after DoCancel is called, it
        //means some other thread will complete the request cancellation.
        //If the AO does not have a proper DoCancel, this will hang up.

#if (PV_SCHED_PERF_LOGGING)
        uint32 time = 0;
#endif
        int32 nwait = 0;
        while (!complete)
        {
#if (PV_SCHED_PERF_LOGGING)
            if (nwait == 0)
            {
                if (DO_LOGPERF)
                {
                    //reset the perf indent when scheduler gives up CPU...
                    ResetLogPerf();
                    LOGPERF((0, "PVSCHED: Waiting on cancel... AO '%s'", pvbase->iName.Str()));
                    time = OsclTickCount::TickCount();
                }
            }
#endif
            //Wait on some request to complete.
            iReadyQ.Wait();
            nwait++;

            //Some request was complete but it might not be this one, so
            //check again.
            complete = iReadyQ.IsInMT(pvbase);
        }

        //Restore the request semaphore value since we decremented it without
        //removing anything from the ReadyQ.
        if (nwait > 0)
        {
            iReadyQ.Signal(nwait);
        }

#if (PV_SCHED_PERF_LOGGING)
        if (DO_LOGPERF)
        {
            uint32 delta = OsclTickCount::TickCount() - time;
            LOGPERF((0, "PVSCHED: ...Cancel took %d Ticks", delta));
        }
#endif
    }

    //Set request idle and remove from ready Q.
    //The AO will not run
    pvbase->iBusy = false;
    iReadyQ.Lock();
    iReadyQ.Remove(pvbase);
    iReadyQ.Unlock();
}



////////////////////////////////////////
// PV Scheduling Loop Implementation
////////////////////////////////////////


void OsclExecSchedulerCommonBase::CallRunExec(PVActiveBase *pvactive)
//Run a PV AO.
{
    //Set this AO inactive.  This AO may be from the
    //ready Q or the pending Timer Q.
    //The dequeing is done by the caller.
    pvactive->iBusy = false;

    //start stats
#if (PV_SCHED_ENABLE_AO_STATS)
    PVActiveStats* pvstats = pvactive->iPVActiveStats;
    uint32 delta = 0;
    PVTICK time;
    time = 0;
#endif

    SET_TICK(time);

    //Call the Run under a trap harness.
    //Pass the ErrorTrapImp pointer to reduce overhead of the Try call.
    //We already did a null ptr check on iErrorTrapImp so it's safe to de-ref here.
    int32 err;
    OSCL_TRY_NO_TLS(iErrorTrapImp, err, pvactive->Run(););

    //end stats
    DIFF_TICK(time, delta);
    UPDATE_RUNL_TIME(pvstats, delta);
    UPDATE_LEAVE_CODE(pvstats, err);

#if(PV_SCHED_ENABLE_PERF_LOGGING)
    //show AO time.
    if (DO_LOGPERF)
    {
        IncLogPerf(delta);
        LOGPERF((0, "PVSCHED: %s Run %d %s AO %s", iLogPerfIndentStr, (int32)delta, TICKSTR, pvactive->iName.Str()));
    }
#endif

    //check for a leave in the Run...
    if (err != OsclErrNone)
    {
        //start stats
        SET_TICK(time);

        //call the AO error handler
        err = pvactive->RunError(err);

        //end stats
        DIFF_TICK(time, delta);
        UPDATE_RUNERROR_TIME(pvstats, delta);

        //If the AO did not handle the error, indicated by returning
        //ErrNone, then call the scheduler error handler.
        if (err != OsclErrNone)
        {
            LOGERROR((0, "PVSCHED:Scheduler '%s', Thread 0x%x: Error! AO %s Error %d not handled"
                      , iName.Str(), PVThreadContext::Id()
                      , pvactive->iName.Str(), err));
            fprintf(stderr, "PVSCHED:Scheduler '%s', Thread 0x%x: Error! AO %s Error %d not handled\n"
                    , iName.Str(), PVThreadContext::Id()
                    , pvactive->iName.Str(), err);

            Error(err);
        }
    }
}


void OsclExecSchedulerCommonBase::BlockingLoopL()
//Blocking scheduling loop.
//Will leave if any AO leaves.
{

    while (!iDoStop)
    {
        DECLARE_LOOP_STATS;



        //First process timers.
        //All ready timers will get moved to the run Q.
        uint32 waitTicks;

        START_LOOP_STATS(iOtherExecStats[EOtherExecStats_QueueTime]);
        PVActiveBase* pvtimer = UpdateTimers(waitTicks);
        END_LOOP_STATS(iOtherExecStats[EOtherExecStats_QueueTime]);

        //now find the highest priority ready AO.

        START_LOOP_STATS(iOtherExecStats[EOtherExecStats_LockTime]);
        iReadyQ.Lock();
        END_LOOP_STATS(iOtherExecStats[EOtherExecStats_LockTime]);

        START_LOOP_STATS(iOtherExecStats[EOtherExecStats_QueueTime]);
        PVActiveBase* pvactive = iReadyQ.PopTop();
        END_LOOP_STATS(iOtherExecStats[EOtherExecStats_QueueTime]);

        START_LOOP_STATS(iOtherExecStats[EOtherExecStats_LockTime]);
        iReadyQ.Unlock();
        END_LOOP_STATS(iOtherExecStats[EOtherExecStats_LockTime]);


        if (pvactive)
        {
            //Run this AO.
            CallRunExec(pvactive);

        }
        else //nothing is ready.
        {
            //We have to wait on either a timer interval to expire,
            //or a request to be completed by some other thread.
            if (pvtimer)
            {
#if (PV_SCHED_ENABLE_PERF_LOGGING)
                //reset the perf logging indent each time scheduler gives up CPU.
                ResetLogPerf();
                LOGPERF((0, "PVSCHED: Waiting on timer... Ticks %d AO %s", waitTicks, pvtimer->iName.Str()));
#endif

                START_LOOP_STATS(iOtherExecStats[EOtherExecStats_WaitTime]);

                //Wait on shortest timer expiration or a new request.
                if (iReadyQ.Wait(OsclTickCount::TicksToMsec(waitTicks)))
                {
                    END_LOOP_STATS(iOtherExecStats[EOtherExecStats_WaitTime]);

                    //Timeout.
                    //It's time to complete this timer's request and run it.
                    //There's no need to move it to the ReadyQ, just run it.
                    pvtimer->iStatus = OSCL_REQUEST_ERR_NONE;
                    //it's the top of the queue so we can remove it with Pop.
                    START_LOOP_STATS(iOtherExecStats[EOtherExecStats_QueueTime]);
                    iExecTimerQ.Pop(pvtimer);
                    END_LOOP_STATS(iOtherExecStats[EOtherExecStats_QueueTime]);
                    CallRunExec(pvtimer);
                }
                else //another thread completed a request during the wait.
                {
                    //restore request sem since we didn't run the AO yet.
                    iReadyQ.Signal();

                    END_LOOP_STATS(iOtherExecStats[EOtherExecStats_WaitTime]);

                }
            }
            else //nothing ready and no timers pending.
            {
#if (PV_SCHED_ENABLE_PERF_LOGGING)
                //reset the perf logging indent each time scheduler gives up CPU.
                ResetLogPerf();
                LOGPERF((0, "PVSCHED: Waiting on any request..."));
#endif

                START_LOOP_STATS(iOtherExecStats[EOtherExecStats_WaitTime]);

                //this will block until a request is completed by another thread.
                iReadyQ.Wait();
                //restore request sem since we didn't run the AO yet.
                iReadyQ.Signal();

                END_LOOP_STATS(iOtherExecStats[EOtherExecStats_WaitTime]);
            }
        }

        //check for a suspend signal..
        if (iDoSuspend)
        {
            iSuspended = true;
            iDoSuspend = false;
            iResumeSem.Wait();
            iSuspended = false;
        }

    }//while !dostop

    iDoStop = false;

}


////////////////////////////////////////
// PVSchedulerStopper Implementation
////////////////////////////////////////
PVSchedulerStopper::PVSchedulerStopper()
        : OsclActiveObject((int32)OsclActiveObject::EPriorityHighest, "Stopper")
{
}

PVSchedulerStopper::~PVSchedulerStopper()
{
}

void PVSchedulerStopper::Run()
//This AO just waits for a signal to suspend or stop the scheduler.
{
    //Stop
    switch (Status())
    {
        case STOPPER_REQUEST_STOP_NATIVE:
            break;
        case STOPPER_REQUEST_STOP_PV:
        {
            //stop my scheduling loop
            OsclExecSchedulerCommonBase* myscheduler = OsclExecScheduler::GetScheduler();
            if (myscheduler)
                myscheduler->iDoStop = true;
        }
        break;
        case STOPPER_REQUEST_SUSPEND:
        {
            //suspend my scheduling loop
            OsclExecSchedulerCommonBase* myscheduler = OsclExecScheduler::GetScheduler();
            if (myscheduler)
                myscheduler->iDoSuspend = true;

            //re-schedule ourself
            PendForExec();
        }
        break;
        default:
            break;
    }
}





