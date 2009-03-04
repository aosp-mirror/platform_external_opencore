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


#ifndef PV_OMX_INTERFACE_PROXY_H_INCLUDED
#include "pv_omx_interface_proxy.h"
#endif

#ifndef PV_OMX_INTERFACE_PROXY_HANDLER_H_INCLUDED
#include "pv_omx_interface_proxy_handler.h"
#endif

#ifndef PV_OMX_INTERFACE_PROXY_NOTIFIER_H_INCLUDED
#include "pv_omx_interface_proxy_notifier.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif


OSCL_DLL_ENTRY_POINT_DEFAULT()

//
//CPVInterfaceProxy_OMX
//

OSCL_EXPORT_REF CPVInterfaceProxy_OMX * CPVInterfaceProxy_OMX::NewL(
    PVProxiedEngine_OMX& app
    , OsclLockBase *lock
    , Oscl_DefAlloc *alloc
    , int32 stacksize
    , uint32 nreserve1
    , uint32 nreserve2
    , int32 handlerPri
    , int32 notifierPri)
//called under app thread context
{
    // custom allocator not using TRY/LEAVE/TLS
    oscl_allocator defallocL;
    OsclAny *ptr = NULL;
    if (alloc)
    {
        ptr = alloc->ALLOCATE(sizeof(CPVInterfaceProxy_OMX));
        //Commented all the oscl tls statements
        //OsclError::LeaveIfNull(ptr);
        if (ptr == NULL)
        {
            return NULL;
        }
    }
    else
    {
        ptr = defallocL.ALLOCATE(sizeof(CPVInterfaceProxy_OMX));
    }
    CPVInterfaceProxy_OMX *self = OSCL_PLACEMENT_NEW(ptr, CPVInterfaceProxy_OMX(app, lock, alloc, stacksize));
    bool err;
    //Commented OSCL_TRY to remove oscl initialization dependency
    err = self->ConstructL(nreserve1, nreserve2, handlerPri, notifierPri);
    if (err == false)
    {
        self->Delete();
        return NULL;
    }
    return self;
}

CPVInterfaceProxy_OMX::CPVInterfaceProxy_OMX(PVProxiedEngine_OMX& app, OsclLockBase *lock, Oscl_DefAlloc*alloc, int32 stacksize)
        : iPVApp(app)
        , iLock(lock)
//called under app thread context
{
    iCommandIdCounter = 0;
    iProxyIdCounter = 0;
    iHandler = NULL;
    iPVScheduler = NULL;
    iNotifier = NULL;
    iStacksize = 0x8000;
    OSCL_UNUSED_ARG(stacksize);
    iStopped = true;
    iAlloc = (alloc) ? alloc : &iDefAlloc;
    iLogger = NULL;
}

//Commented all the OsclError::Leave statements to remove oscl tls dependency
//Changed return type of this function from void to bool

bool CPVInterfaceProxy_OMX::ConstructL(uint32 nreserve1, uint32 nreserve2, int32 handlerPri, int32 notifierPri)
{
    // Create the semaphores and critical sections
    if (iInitSem.Create() != OsclProcStatus::SUCCESS_ERROR
            || iExitedSem.Create() != OsclProcStatus::SUCCESS_ERROR
            || iCounterCrit.Create() != OsclProcStatus::SUCCESS_ERROR
            || iHandlerQueueCrit.Create() != OsclProcStatus::SUCCESS_ERROR
            || iNotifierQueueCrit.Create() != OsclProcStatus::SUCCESS_ERROR
            || iProxyListCrit.Create() != OsclProcStatus::SUCCESS_ERROR)
    {
        return false;
    }
    //reserve space in vectors...
    if (nreserve1 > 0)
        iProxyList.reserve(nreserve1);
    if (nreserve2 > 0)
    {
        iCommandQueue.reserve(nreserve2);
//		iNotificationQueue.reserve(nreserve2);
    }

    //create handler
    OsclAny *ptr = iAlloc->ALLOCATE(sizeof(CPVInterfaceProxyHandler_OMX));
    if (ptr == NULL)
    {
        return false;
    }

    iHandler = OSCL_PLACEMENT_NEW(ptr, CPVInterfaceProxyHandler_OMX(this, handlerPri));

    //create notifier
    ptr = iAlloc->ALLOCATE(sizeof(CPVInterfaceProxyNotifier_OMX));
    if (ptr == NULL)
    {
        return false;
    }

    iNotifier = OSCL_PLACEMENT_NEW(ptr, CPVInterfaceProxyNotifier_OMX(this, notifierPri));
    return true;
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::Delete()
//called under app thread context
{
    this->~CPVInterfaceProxy_OMX();
    iAlloc->deallocate(this);
}

CPVInterfaceProxy_OMX::~CPVInterfaceProxy_OMX()
//called under app thread context
{
    //make sure thread was stopped.
    StopPVThread();

    CleanupAppThreadQueues();

    //delete handler and notifier
    if (iHandler)
    {
        iHandler->~CPVInterfaceProxyHandler_OMX();
        iAlloc->deallocate(iHandler);
    }
    iHandler = NULL;
    if (iNotifier)
    {
        iNotifier->~CPVInterfaceProxyNotifier_OMX();
        iAlloc->deallocate(iNotifier);
    }
    iNotifier = NULL;

    iCounterCrit.Close();
    iHandlerQueueCrit.Close();
    iNotifierQueueCrit.Close();
    iProxyListCrit.Close();
    iInitSem.Close();
    iExitedSem.Close();
}

//forward...
TOsclThreadFuncRet OSCL_THREAD_DECL pvproxythreadmain_omx(TOsclThreadFuncArg *aPtr);

OSCL_EXPORT_REF bool CPVInterfaceProxy_OMX::StartPVThread()
//called under app thread context
{
    if (!iStopped)
        return false;//thread already active

    //Notifier not required in omx component, Scheduler not initialized in testapp thread

    // Create the PV thread.
    OsclProcStatus::eOsclProcError err;
    err = iPVThread.Create((TOsclThreadFuncPtr)pvproxythreadmain_omx,
                           iStacksize,
                           (TOsclThreadFuncArg)this);
    if (err == OSCL_ERR_NONE)
    {
        iStopped = false;
        //Wait for PV thread to initialize its scheduler.
        if (iInitSem.Wait() != OsclProcStatus::SUCCESS_ERROR)
        {
            //Commented to remove oscl tls dependency
            return false;
        }
        return true;
    }
    else
    {
        //error cleanup
        iNotifier->RemoveFromScheduler();
        return false;
    }
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::StopPVThread()
//called under app thread context.
{
    //if called under the PV thread, we'll get deadlock..
    //so don't allow it.
    if (iPVThreadContext.IsSameThreadContext())
    {	//Commented to remove oscl tls dependency (OsclError::Panic)
        return;
    }

    if (iStopped)
    {
        return ;
    }

    //deque notifier AO
    iNotifierQueueCrit.Lock();
    if (iNotifier && iNotifier->IsAdded())
    {
        iNotifier->RemoveFromScheduler();
    }

    iNotifierQueueCrit.Unlock();

    //Stop the scheduler loop.
    if (iPVScheduler)
    {
        iPVScheduler->StopScheduler();
    }

    //Wait for PV thread to finish up, then it's safe
    //to delete the remaining stuff.
    if (iExitedSem.Wait() != OsclProcStatus::SUCCESS_ERROR)
    {	//Commented to remove oscl tls dependency (OsclError::Panic)
        return;
    }

    //let the destructor know it's exited.
    iStopped = true;

    //The thread will exit on its own, but go ahead and
    //forcibly terminate it now to make sure it's cleaned
    //up by the time this call returns.
    iPVThread.Terminate(0);
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::DeliverNotifications(int32 aTargetCount, int32& aNoticesPending)
//deliver notifications off the queue, from the app thread size
{
    //make sure this isn't called under PV thread...
    if (iPVThreadContext.IsSameThreadContext())
        OsclError::Panic("PVPROXY", EPVProxyPanicWrongThreadContext);

    for (int32 count = 0;count < aTargetCount;)
    {
        //get next notification or cleanup message.
        iNotifierQueueCrit.Lock();
        //CPVProxyMsg notice(0,0,NULL);
        //Added, increased one more parameter in the call
        CPVProxyMsg_OMX notice(0, 0, 0, NULL);
        if (iNotificationQueue.size() > 0)
        {
            notice = iNotificationQueue[0];
            iNotificationQueue.erase(&iNotificationQueue[0]);
        }
        iNotifierQueueCrit.Unlock();

        if (notice.iMsg)
        {
            count++;
            CPVProxyInterface_OMX *ext = FindInterface(notice.iProxyId);
            if (ext)
                ext->iClient->HandleNotification(notice.iMsgId, notice.iMsg);
            else
            {	//since messages are cleaned up when interfaces
                //get unregistered, we should not get here.
                OsclError::Panic("PVPROXYDEBUG", 0);//debug error.
            }
        }
        else
            break;//no more messages.
    }
    //return number of notices left after trying to process
    //the desired number.
    aNoticesPending = iNotificationQueue.size();
}

void CPVInterfaceProxy_OMX::CleanupAppThreadQueues()
//cleanup memory allocated in App thread.
{
    //un-sent commands...
    iHandlerQueueCrit.Lock();
    while (!iCommandQueue.empty())
    {
        CPVProxyMsg_OMX *msg = &iCommandQueue[0];
        CPVProxyInterface_OMX *proxy = FindInterface(msg->iProxyId);
        if (proxy)
            proxy->iClient->CleanupCommand(msg->iMsgId, msg->iMsg);
        iCommandQueue.erase(msg);
    }
    iCommandQueue.clear();
    iCommandQueue.destroy();
    iHandlerQueueCrit.Unlock();

    //proxy list...
    iProxyListCrit.Lock();
    iProxyList.clear();
    iProxyList.destroy();
    iProxyListCrit.Unlock();
}

void CPVInterfaceProxy_OMX::CleanupPVThreadQueues()
//cleanup memory allocated in PV thread.
{
    //un-sent notifications
    iNotifierQueueCrit.Lock();
    while (!iNotificationQueue.empty())
    {
        CPVProxyMsg_OMX *msg = &iNotificationQueue[0];
        CPVProxyInterface_OMX *proxy = FindInterface(msg->iProxyId);
        if (proxy)
            proxy->iServer->CleanupNotification(msg->iMsgId, msg->iMsg);
        iNotificationQueue.erase(msg);
    }
    iNotificationQueue.clear();
    iNotificationQueue.destroy();
    iNotifierQueueCrit.Unlock();
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::CancelCommand(TPVProxyId aProxyId, TPVProxyMsgId aMsgId)
{
    CleanupCommands(FindInterface(aProxyId), false, aMsgId);
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::CancelAllCommands(TPVProxyId aProxyId)
{
    CleanupCommands(FindInterface(aProxyId), true);
}

void CPVInterfaceProxy_OMX::CleanupCommands(CPVProxyInterface_OMX *aExt, bool aAll, TPVProxyMsgId aMsgId)
{
    if (!aExt)
        return ;
    iHandlerQueueCrit.Lock();
    for (uint32 i = 0;i < iCommandQueue.size();i++)
    {
        CPVProxyMsg_OMX *msg = &iCommandQueue[i];
        if (msg->iProxyId == aExt->iProxyId
                && (aAll || msg->iMsgId == aMsgId))
        {
            aExt->iClient->CleanupCommand(msg->iMsgId, msg->iMsg);
            iCommandQueue.erase(msg);
            i--;//move back one since vecter gets scrunched.
            if (!aAll)
            {
                iHandlerQueueCrit.Unlock();
                return ;
            }
        }
    }
    iHandlerQueueCrit.Unlock();
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::CancelNotification(TPVProxyId aProxyId, TPVProxyMsgId aMsgId)
{
    CleanupNotifications(FindInterface(aProxyId), false, aMsgId);
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::CancelAllNotifications(TPVProxyId aProxyId)
{
    CleanupNotifications(FindInterface(aProxyId), true);
}

void CPVInterfaceProxy_OMX::CleanupNotifications(CPVProxyInterface_OMX *aExt, bool aAll, TPVProxyMsgId aMsgId)
{
    if (!aExt)
        return ;
    iNotifierQueueCrit.Lock();
    for (uint i = 0;i < iNotificationQueue.size();i++)
    {
        CPVProxyMsg_OMX *msg = &iNotificationQueue[i];
        if (msg->iProxyId == aExt->iProxyId
                && (aAll || msg->iMsgId == aMsgId))
        {
            aExt->iServer->CleanupNotification(msg->iMsgId, msg->iMsg);
            iNotificationQueue.erase(msg);
            i--;//move back one since vector gets scrunched.
            if (!aAll)
            {
                iNotifierQueueCrit.Unlock();
                return ;
            }
        }
    }
    iNotifierQueueCrit.Unlock();
}

void CPVInterfaceProxy_OMX::CleanupInterfaceMessages(CPVProxyInterface_OMX *aExt)
//cleanup all extension messages for a particular interface.
{
    CleanupCommands(aExt, true);
    CleanupNotifications(aExt, true);
}

OSCL_EXPORT_REF TPVProxyId CPVInterfaceProxy_OMX::RegisterProxiedInterface(
    PVProxiedInterfaceServer_OMX& server_proxy,
    PVProxiedInterfaceClient_OMX& client_proxy)
//Proxy extensions call this to register themselves.
{
    TPVProxyId id = ++iProxyIdCounter;
    iProxyListCrit.Lock();
    CPVProxyInterface_OMX proxy(id, &server_proxy, &client_proxy);
    int32 err;
    OSCL_TRY(err, iProxyList.push_back(proxy););
    iProxyListCrit.Unlock();
    OsclError::LeaveIfError(err);
    return id;
}

OSCL_EXPORT_REF void CPVInterfaceProxy_OMX::UnregisterProxiedInterface(TPVProxyId aProxyId)
//Proxy extensions call this to unregister themselves.
{
    iProxyListCrit.Lock();
    CPVProxyInterface_OMX *ext = FindInterface(aProxyId, true);
    if (ext)
    {
        //cleanup unprocessed messages and remove.
        CleanupInterfaceMessages(ext);
        iProxyList.erase(ext);
    }
    iProxyListCrit.Unlock();
}

OSCL_EXPORT_REF TPVProxyMsgId CPVInterfaceProxy_OMX::SendCommand(TPVProxyId aProxyId, TPVCommandId cmdid, OsclAny *aCmd)
//Proxy extensions call this to send commands from app side
//to PV side.
{
    bool status;
    iCounterCrit.Lock();
    TPVProxyMsgId id = ++iCommandIdCounter;
    iCounterCrit.Unlock();
    iHandlerQueueCrit.Lock();
    //Changed the function arguments to pass command id
    CPVProxyMsg_OMX msg(aProxyId, id, cmdid, aCmd);

    status = iCommandQueue.push_back(msg);

    //if the queue was empty, signal the AO.
    if (iCommandQueue.size() == 1)
    {
        iHandler->PendComplete(OSCL_REQUEST_ERR_NONE);
    }

    iHandlerQueueCrit.Unlock();
    //propagate any allocation failure...
    if (status == false)
    {
        return false;
    }
    return id;
}

OSCL_EXPORT_REF TPVProxyMsgId CPVInterfaceProxy_OMX::SendNotification(TPVProxyId aProxyId, OsclAny *aResp)
//Proxy extensions call this to send notifications from PV
//side to app side.
{
    int32 err = OSCL_ERR_NONE;
    iCounterCrit.Lock();
    TPVProxyMsgId id = ++iCommandIdCounter;
    iCounterCrit.Unlock();
    iNotifierQueueCrit.Lock();
    CPVProxyMsg_OMX msg(aProxyId, id, 0, aResp);
    OSCL_TRY(err, iNotificationQueue.push_back(msg););
    //if the queue was empty and the notifier is scheduled,
    //signal it.
    if (iNotifier
            && iNotifier->IsAdded()
            && iNotificationQueue.size() == 1)
        iNotifier->PendComplete(OSCL_REQUEST_ERR_NONE);
    iNotifierQueueCrit.Unlock();
    //propagate any allocation failure...
    OsclError::LeaveIfError(err);
    return id;
}

CPVProxyInterface_OMX * CPVInterfaceProxy_OMX::FindInterface(TPVProxyId aId, bool locked)
//lookup a registered proxy interface
{
    if (!locked)
        iProxyListCrit.Lock();
    for (uint32 i = 0;i < iProxyList.size();i++)
    {
        if (iProxyList[i].iProxyId == aId)
        {
            if (!locked)
                iProxyListCrit.Unlock();
            return &iProxyList[i];
        }
    }
    if (!locked)
        iProxyListCrit.Unlock();
    return NULL;
}

void CPVInterfaceProxy_OMX::InThread()
//this is the guts of the proxy thread routine.
//it's a separate routine since it needs to be
//run under a trap handler in order to avoid any
//Cbase 66 panic from the cleanup stack.
{
    volatile int32 errTerm = OsclErrNone;
    volatile int32 errSched = OsclErrNone;

    //create & install scheduler
    OsclScheduler::Init("PVProxy");
    iPVScheduler = OsclExecScheduler::Current();

    iPVThreadContext.EnterThreadContext();

    //add handler to scheduler
    iHandler->AddToScheduler();
    iHandler->PendForExec();

    //App thread logon...
    int32 err;
    OSCL_TRY(err, iPVApp.PVThreadLogon(*this););
    if (err != OsclErrNone)
        errTerm = err;


    //Start scheduler.  This call blocks until scheduler is
    //either stopped or exits due to an error.
    OSCL_TRY(err, iPVScheduler->StartScheduler(&iInitSem););
    if (err != OsclErrNone)
        errSched = err;

    //Cleanup un-processed data.
    CleanupPVThreadQueues();

    //App thread logoff...
    OSCL_TRY(err, iPVApp.PVThreadLogoff(*this););
    if (err != OSCL_ERR_NONE)
        errTerm = err;

    //Deque the handler AO
    iHandlerQueueCrit.Lock();
    iHandler->RemoveFromScheduler();
    iHandlerQueueCrit.Unlock();

    iPVThreadContext.ExitThreadContext();

    //Uninstall scheduler
    OsclScheduler::Cleanup();
    iPVScheduler = NULL;

    //Generate panics if any leaves happened.
    if (errTerm != OsclErrNone)
        OsclError::Panic("PVPROXY", EPVProxyPanicEngineLeave);
    if (errSched != OsclErrNone)
        OsclError::Panic("PVPROXY", EPVProxyPanicSchedulerLeave);
}

////////////////////////////////
// OS-specific Thread routines
////////////////////////////////


#include "oscl_mem_audit.h"

TOsclThreadFuncRet OSCL_THREAD_DECL pvproxythreadmain_omx(TOsclThreadFuncArg *aPtr)
//PV Thread main routine
{

    CPVInterfaceProxy_OMX *proxy = (CPVInterfaceProxy_OMX *) aPtr;

    //Init OSCL and create logger.
    OsclBase::Init();
    OsclErrorTrap::Init();
    OsclMem::Init(proxy->iLock); // the mem lock for this thread is the same lock object as in the master thread or NULL
    PVLogger::Init();

#if defined( OSCL_SET_THREAD_NAME)
    OSCL_SET_THREAD_NAME("OMX proxy");
#endif

    //Call the proxied app routine to create its logger appenders.
    //proxy->iPVApp.CreateLoggerAppenders();
    //proxy->iLogger=PVLogger::GetLoggerObject("");

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, proxy->iLogger, PVLOGMSG_NOTICE, (0, "PVPROXY:Proxy Thread 0x%x Entry...", OsclExecScheduler::GetId()));


    int32 leave;
    TPVErrorPanic panic;
    OSCL_PANIC_TRAP(leave, panic, proxy->InThread(););

    if (panic.iReason != OsclErrNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR, (0, "PVPROXY:Proxy Thread 0x%x Exit: Panic Cat '%s' Reason %d", OsclExecScheduler::GetId(), panic.iCategory.Str(), panic.iReason));
    }
    else if (leave != OsclErrNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR, (0, "PVPROXY:Proxy Thread 0x%x Exit: Leave Reason %d", OsclExecScheduler::GetId(), leave));
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, proxy->iLogger, PVLOGMSG_NOTICE, (0, "PVPROXY:Proxy Thread 0x%x Exit: Normal", OsclExecScheduler::GetId()));
    }

#if !(OSCL_BYPASS_MEMMGT)
    //Show mem stats
    {
        OsclAuditCB auditCB;
        OsclMemInit(auditCB);
        if (auditCB.pAudit)
        {
            MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
            if (stats)
            {
                //We need to re-init logger to report these, but that will create
                //additional allocations, so save the current stats.
                int32 peakNumAllocs = stats->peakNumAllocs;
                int32 peakNumBytes = stats->peakNumBytes;
                int32 numAllocFails = stats->numAllocFails;
                int32 numAllocs = stats->numAllocs;
                int32 numBytes = stats->numBytes;

                PVLogger::Init();
                proxy->iLogger = PVLogger::GetLoggerObject("pvproxy");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_INFO
                                , (0, "PVPROXY: Memory Stats:"));
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_INFO
                                , (0, "PVPROXY:   peakNumAllocs %d", peakNumAllocs));
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_INFO
                                , (0, "PVPROXY:   peakNumBytes %d", peakNumBytes));
                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_INFO
                                , (0, "PVPROXY:   numAllocFails %d", numAllocFails));
                if (numAllocs)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                    , (0, "PVPROXY: ERROR: Memory Leaks! numAllocs %d, numBytes %d", numAllocs, numBytes));
                }
            }
        }
    }
#endif


#if !(OSCL_BYPASS_MEMMGT)
    {
        //Check for memory leaks before cleaning up OsclMem.
        OsclAuditCB auditCB;
        OsclMemInit(auditCB);
        if (auditCB.pAudit)
        {
            uint32 leaks = auditCB.pAudit->MM_GetNumAllocNodes();
            if (leaks != 0)
            {
                //If we found leaks we need to re-init logger so we can report them.
                //But this will create some new allocations, so make a note of the
                //current allocation number to weed them out of the report.
                uint32 allocafterleaks = auditCB.pAudit->MM_GetAllocNo();
                PVLogger::Init();
                proxy->iLogger = PVLogger::GetLoggerObject("pvproxy");

                PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                , (0, "PVPROXY: ERROR: %d Memory leaks detected in PV Thread!", leaks));

                //Allocate space for the leak info.
                uint32 nodes = auditCB.pAudit->MM_GetNumAllocNodes();
                MM_AllocQueryInfo*info = auditCB.pAudit->MM_CreateAllocNodeInfo(nodes);
                uint32 leakinfo = auditCB.pAudit->MM_GetAllocNodeInfo(info, nodes, 0);
                if (leakinfo < nodes)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                    , (0, "PVPROXY: ERROR: Leak info is incomplete."));
                }
                //Report only the original leaks before the logger allocations.
                for (uint32 i = 0;i < leakinfo;i++)
                {
                    if (info[i].allocNum < allocafterleaks)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY: Leak Info:"));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY:   allocNum %d", info[i].allocNum));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY:   fileName %s", info[i].fileName));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY:   lineNo %d", info[i].lineNo));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY:   size %d", info[i].size));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY:   pMemBlock 0x%x", info[i].pMemBlock));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, proxy->iLogger, PVLOGMSG_ERR
                                        , (0, "PVPROXY:   tag %s", info[i].tag));
                    }
                }
                auditCB.pAudit->MM_ReleaseAllocNodeInfo(info);
            }
        }
    }
#endif
    //Cleanup logger.
    PVLogger::Cleanup();

    OsclMem::Cleanup();
    proxy->iLogger = NULL;
    OsclErrorTrap::Cleanup();
    OsclBase::Cleanup();

    //Signal & Exit
    proxy->iExitedSem.Signal();

    return 0;
}









