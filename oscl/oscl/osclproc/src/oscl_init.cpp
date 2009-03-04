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

#include "oscl_init.h"
#include "oscl_base.h"
#include "oscl_mem.h"
#include "oscl_scheduler.h"
#include "oscl_error.h"
#include "oscl_error_panic.h"
#include "pvlogger.h"

static void _OsclInit2(const OsclSelect &aSelect)
//do all init after error trap...
{
    if (aSelect.iOsclMemory)
    {
        OsclMem::Init(aSelect.iMemLock);
    }
    if (aSelect.iOsclLogger)
    {
        PVLogger::Init();
    }
    if (aSelect.iOsclScheduler)
    {
        OsclScheduler::Init(
            (aSelect.iSchedulerName) ? aSelect.iSchedulerName : "OsclScheduler"
            , aSelect.iSchedulerAlloc
            , aSelect.iSchedulerReserve);
    }
}

//need this routine to avoid a longjmp clobber warning.
static void _OsclInit2(int32 &aErr, TPVErrorPanic &aPanic, const OsclSelect &aSelect)
{
    OSCL_PANIC_TRAP(aErr, aPanic, _OsclInit2(aSelect););
}

OSCL_EXPORT_REF void OsclInit::Init(
    int32 &err
    , TPVErrorPanic &panic
    , const OsclSelect *p
)
{
    err = OsclErrNone;
    panic.iReason = OsclErrNone;

    //Use default parameters if none were input.
    OsclSelect defaultselect;
    const OsclSelect* select = (p) ? p : &defaultselect;

#if defined( OSCL_MEM_HEAPMARK)
    if (select->iHeapCheck)
    {
        OSCL_MEM_HEAPMARK;
    }
#endif

    if (select->iOsclBase)
    {
        err = OsclBase::Init();
        if (err)
            return;
    }

    bool trapit = false;
    if (select->iOsclErrorTrap)
    {
        err = OsclErrorTrap::Init(select->iErrAlloc);
        if (err)
        {
            OsclBase::Cleanup();
            return;
        }
        trapit = true;
    }

    //Do remaining init under a trap, if available.
    if (trapit)
    {
        _OsclInit2(err, panic, *select);
    }
    else
    {
        _OsclInit2(*select);
    }
}


//Note: need these routines to avoid longjmp clobber warnings from some compilers.
static void _OsclSchedulerCleanup(int32 &aErr, TPVErrorPanic& aPanic)
{
    OSCL_PANIC_TRAP(aErr, aPanic, OsclScheduler::Cleanup(););
}

static void _OsclLoggerCleanup(int32 &aErr, TPVErrorPanic& aPanic)
{
    OSCL_PANIC_TRAP(aErr, aPanic, PVLogger::Cleanup(););
}

#include "oscl_mem_audit.h"

static void _OsclMemCleanup(FILE* aFile)
{
#if (OSCL_BYPASS_MEMMGT)
    OSCL_UNUSED_ARG(aFile);
#else
    if (aFile)
    {
        //Check for memory leaks before cleaning up OsclMem.
        OsclAuditCB auditCB;
        OsclMemInit(auditCB);
        if (auditCB.pAudit)
        {
            MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
            if (stats)
            {
                fprintf(aFile, "Memory Stats:\n");
                fprintf(aFile, "  peakNumAllocs %d\n", stats->peakNumAllocs);
                fprintf(aFile, "  peakNumBytes %d\n", stats->peakNumBytes);
                fprintf(aFile, "  numAllocFails %d\n", stats->numAllocFails);
                if (stats->numAllocs)
                {
                    fprintf(aFile, "  ERROR: Memory Leaks! numAllocs %d, numBytes %d\n", stats->numAllocs, stats->numBytes);
                }
            }
            uint32 leaks = auditCB.pAudit->MM_GetNumAllocNodes();
            if (leaks != 0)
            {
                fprintf(aFile, "ERROR: %d Memory leaks detected!\n", leaks);
                MM_AllocQueryInfo*info = auditCB.pAudit->MM_CreateAllocNodeInfo(leaks);
                uint32 leakinfo = auditCB.pAudit->MM_GetAllocNodeInfo(info, leaks, 0);
                if (leakinfo != leaks)
                {
                    fprintf(aFile, "ERROR: Leak info is incomplete.\n");
                }
                for (uint32 i = 0;i < leakinfo;i++)
                {
                    fprintf(aFile, "Leak Info:\n");
                    fprintf(aFile, "  allocNum %d\n", info[i].allocNum);
                    fprintf(aFile, "  fileName %s\n", info[i].fileName);
                    fprintf(aFile, "  lineNo %d\n", info[i].lineNo);
                    fprintf(aFile, "  size %d\n", info[i].size);
                    fprintf(aFile, "  pMemBlock 0x%x\n", info[i].pMemBlock);
                    fprintf(aFile, "  tag %s\n", info[i].tag);
                }
                auditCB.pAudit->MM_ReleaseAllocNodeInfo(info);
            }
        }
    }
#endif
    OsclMem::Cleanup();
}

static void _OsclMemCleanup(int32 &aErr, TPVErrorPanic& aPanic, FILE* aFile)
{
    OSCL_PANIC_TRAP(aErr, aPanic, _OsclMemCleanup(aFile););
}

OSCL_EXPORT_REF void OsclInit::Cleanup(int32 &aErr, TPVErrorPanic& aPanic, const OsclSelect *p)
{
    int32 err = OsclErrNone;
    TPVErrorPanic panic;
    //Use default parameters if none were input.
    OsclSelect defaultselect;
    const OsclSelect* select = (p) ? p : &defaultselect;

    //Note: we continue cleanup despite errors and return the last error encoutered.

    if (select->iOsclScheduler)
    {
        _OsclSchedulerCleanup(err, panic);
        if (err)
            aErr = err;
        if (panic.iReason)
            aPanic = panic;
    }

    if (select->iOsclLogger)
    {
        _OsclLoggerCleanup(err, panic);
        if (err)
            aErr = err;
        if (panic.iReason)
            aPanic = panic;
    }

    if (select->iOsclMemory)
    {
        _OsclMemCleanup(err, panic, select->iOutputFile);
        if (err)
            aErr = err;
        if (panic.iReason)
            aPanic = panic;
    }

    if (select->iOsclErrorTrap)
    {
        err = OsclErrorTrap::Cleanup();
        if (err)
            aErr = err;
    }

    if (select->iOsclBase)
    {
        err = OsclBase::Cleanup();
        if (err)
            aErr = err;
    }

#if defined(OSCL_MEM_HEAPMARKEND)
    if (select->iHeapCheck)
    {
        OSCL_MEM_HEAPMARKEND;
    }
#endif
}

