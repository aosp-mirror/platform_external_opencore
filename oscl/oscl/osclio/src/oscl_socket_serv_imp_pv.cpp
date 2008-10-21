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

#include "osclconfig_io.h"
#include "oscl_socket_tuneables.h"

#if(PV_SOCKET_SERVER)

#include "oscl_scheduler_ao.h"
#include "oscl_socket_method.h"
#include "oscl_socket_types.h"
#include "oscl_socket_serv_imp_pv.h"
#include "oscl_error.h"
#include "oscl_socket_imp.h"
#include "oscl_assert.h"

//Logger macro for socket server logging.
#if(PV_OSCL_SOCKET_SERVER_LOGGER_OUTPUT)
#include "pvlogger.h"
#define LOGSERV(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG,iLogger,PVLOGMSG_DEBUG,m);
#else
#define LOGSERV(m)
#endif
/*
//disable this if it's too much
#undef LOGSERV
#define LOGSERV
*/

//Macros for server stats logging for use with OsclSocketServI
#if (PV_OSCL_SOCKET_STATS_LOGGING)
#define ADD_STATS(x) iServStats.Add(x)
#define ADD_STATSP(x,y) iServStats.Add(x,y)
#define CONSTRUCT_STATS(x) iServStats.Construct(x)
#define DUMP_STATS iServStats.LogAndDump();
#else
#define ADD_STATS(x)
#define ADD_STATSP(x,y)
#define CONSTRUCT_STATS(x)
#define DUMP_STATS
#endif//PV_OSCL_SOCKET_STATS_LOGGING


//
//OsclSocketServI::LoopbackSocket
//

//Macros for server stats logging for use with OsclSocketServI::LoopbackSocket
#if (PV_OSCL_SOCKET_STATS_LOGGING)
#define ADD_LOOPSTATS(a,b) iStats.Add((TPVSocketFxn)0,a,b);
#define LOG_LOOPSTATS iStats.Log();
#define DUMP_LOOPSTATS iStats.Log();iStats.Clear((TPVSocketFxn)0);
#else
#define ADD_LOOPSTATS(a,b)
#define LOG_LOOPSTATS
#define DUMP_LOOPSTATS
#endif//PV_OSCL_SOCKET_STATS_LOGGING

void OsclSocketServI::LoopbackSocket::Init(OsclSocketServI* aContainer)
//This will intialize the loopback socket that is used
//to wakeup a blocking select call.
{
    iContainer = aContainer;
#if PV_OSCL_SOCKET_STATS_LOGGING
    iStats.Construct(NULL, this);
#endif

    //create the socket
    bool ok;
    int err;
    OsclSocket(iSocket, OSCL_AF_INET, OSCL_SOCK_DATAGRAM, OSCL_IPPROTO_UDP, ok, err);
    if (!ok)
    {
        ADD_LOOPSTATS(EOsclSocketServ_LoopsockError, err);
        return ;
    }

    //set it to non-blocking mode
    OsclSetNonBlocking(iSocket, ok, err);
    if (!ok)
    {//set non-blocking mode failed
        ADD_LOOPSTATS(EOsclSocketServ_LoopsockError, err);
        OsclCloseSocket(iSocket, ok, err);
        return ;
    }

    //bind to any available port between 5000 and 10000.
    OsclNetworkAddress myAddr("127.0.0.1", 5000);
    while (myAddr.port < 10000)
    {
        OsclSocketI::MakeAddr(myAddr, iAddr);
        OsclBind(iSocket, iAddr, ok, err);
        if (!ok)
        {
            myAddr.port++;
        }
        else
        {
            break;//bind success!
        }
    }

    if (!ok)
    {//bind failed
        ADD_LOOPSTATS(EOsclSocketServ_LoopsockError, err);
        OsclCloseSocket(iSocket, ok, err);
        return ;
    }

    //let's test a send & recv here just to be extra sure
    //the loopsock is usable.
    char tmpBuf[2] = {0, 0};
    int nbytes;
    bool wouldblock;
    wouldblock = false;
    ok = false;
    nbytes = 0;
    OsclSendTo(iSocket, tmpBuf, 1, iAddr, ok, err, nbytes, wouldblock);
    if (!ok)
    {
        if (wouldblock)
        {
            //just skip the test
            ok = true;
        }
        else
        {
            //send failed.
            ADD_LOOPSTATS(EOsclSocketServ_LoopsockError, err);
            OsclCloseSocket(iSocket, ok, err);
            return ;
        }
    }
    else
    {
        //send ok-- now try a recv
        TOsclSockAddr sourceaddr;
        TOsclSockAddrLen sourceaddrlen = sizeof(sourceaddr);
        ok = false;
        nbytes = 0;
        OsclRecvFrom(iSocket, tmpBuf, sizeof(tmpBuf),
                     &sourceaddr,
                     &sourceaddrlen,
                     ok,
                     err,
                     nbytes,
                     wouldblock);
        if (!ok)
        {
            if (wouldblock)
            {
                //just skip the test
                ok = true;
            }
            else
            {//recv failed.
                ADD_LOOPSTATS(EOsclSocketServ_LoopsockError, err);
                OsclCloseSocket(iSocket, ok, err);
                return ;
            }
        }
    }

    //loopsock is ready to use
    ADD_LOOPSTATS(EOsclSocketServ_LoopsockOk, myAddr.port);
#if PV_OSCL_SOCKET_STATS_LOGGING
    iStats.Construct((OsclAny*)iSocket, this);
#endif
    iEnable = true;
}

void OsclSocketServI::LoopbackSocket::Cleanup()
//This will intialize the loopback socket that is used
//to wakeup a blocking select call.
{
    if (iEnable)
    {
        //close the socket
        bool ok;
        int sockerr;
        OsclCloseSocket(iSocket, ok, sockerr);
        iEnable = false;
        DUMP_LOOPSTATS;
    }
}

void OsclSocketServI::LoopbackSocket::Read()
{
    if (!iEnable)
        return;

    //read all queued data on the socket
    if (FD_ISSET(iSocket, &iContainer->iReadset))
    {
        char tmpBuf[2] = {0, 0};
        int nbytes, err;
        bool ok, wouldblock;
        TOsclSockAddr sourceaddr;
        TOsclSockAddrLen sourceaddrlen = sizeof(sourceaddr);

        bool recv = true;
        while (recv)
        {
            OsclRecvFrom(iSocket, tmpBuf, sizeof(tmpBuf),
                         &sourceaddr,
                         &sourceaddrlen,
                         ok,
                         err,
                         nbytes,
                         wouldblock);
            recv = (ok && nbytes > 0);
        }
    }
}

void OsclSocketServI::LoopbackSocket::ProcessSelect(bool aSelect, TOsclSocket& maxsocket)
//Do the necessary select loop processing to keep
//the loopback socket going.
{
    if (!iEnable)
        return;

    //Monitor this socket whenever we will be doing a select.
    if (aSelect)
    {
        FD_SET(iSocket, &iContainer->iReadset);
        if (iSocket > maxsocket)
            maxsocket = iSocket;
    }
}

void OsclSocketServI::LoopbackSocket::Write()
//Write to the loopback socket
{
    if (!iEnable)
        return;

    char tmpBuf[2] = {0, 0};
    int nbytes, err;
    bool wouldblock, ok;
    OsclSendTo(iSocket, tmpBuf, 1, iAddr, ok, err, nbytes, wouldblock);

    if (!ok)
    {//the select call will hang forever, so just go ahead and panic now.
        OsclError::Panic("OSCLSOCK", 1);
    }
}

//
//OsclSocketServI-- PV implementation
//

#if(PV_SOCKET_SERVER_IS_THREAD)
OsclSocketServI::OsclSocketServI(Oscl_DefAlloc &a)
        : OsclSocketServIBase(a)
{
}
#else
OsclSocketServI::OsclSocketServI(Oscl_DefAlloc &a)
        : OsclTimerObject(PV_SOCKET_SERVER_AO_PRIORITY, "OsclSocketServI")
        , OsclSocketServIBase(a)
{
}
#endif

OsclSocketServI::~OsclSocketServI()
{
    Close(false);
    CleanupServImp();
}

OsclSocketServI* OsclSocketServI::NewL(Oscl_DefAlloc &a)
{
    OsclAny*p = a.ALLOCATE(sizeof(OsclSocketServI));
    OsclError::LeaveIfNull(p);
    OsclSocketServI *self = OSCL_PLACEMENT_NEW(p, OsclSocketServI(a));
    OsclError::LeaveIfNull(self);
    OsclError::PushL(self);
    self->ConstructL();
    OsclError::Pop();
    return self;
}

void OsclSocketServI::ConstructL()
{
    iSockServRequestList.iActiveRequests.reserve(6);
    ConstructServImp();
    CONSTRUCT_STATS(this);
}

int32 OsclSocketServI::Connect(uint32 aMessageSlots)
{
    CONSTRUCT_STATS(this);

    //Connect to Oscl socket server

    OSCL_UNUSED_ARG(aMessageSlots);

    //should only connect once
    if (iServState == ESocketServ_Connected)
    {
        return OsclErrGeneral;
    }

#ifdef OsclSocketStartup
    //startup the socket system.
    bool ok;
    OsclSocketStartup(ok);
    if (!ok)
    {
        return OsclErrGeneral;
    }
#endif//OsclSocketStartup

    iServState = ESocketServ_Idle;

    //Start the server thread or AO
    int32 err = StartServImp();
    if (err != OsclErrNone)
    {
        return err;
    }

    //check state.
    if (iServState != ESocketServ_Connected)
    {
        //cleanup after a failure.
        Close(false);
        return OsclErrGeneral;
    }
    return OsclErrNone;
}

void OsclSocketServI::Close(bool aCleanup)
{
    //Close oscl socket server

    if (iServState == ESocketServ_Connected)
    {
        StopServImp();
    }

    iLoopbackSocket.Cleanup();

#ifdef OsclSocketCleanup
    //close the socket system
    if (aCleanup)
    {
        bool ok;
        OsclSocketCleanup(ok);
    }
#endif//OsclSocketCleanup

    DUMP_STATS;
}

bool OsclSocketServI::IsServerThread()
{
#if(PV_SOCKET_SERVER_IS_THREAD)
    TOsclThreadId tid;
    OsclThread::GetId(tid);
    return tid == iThreadId;
#else
    return true;
#endif
}

#ifdef OsclSocketSelect
/**
 * Process all active socket requests.
 *
 * This is called under the server thread or server AO.
 *
 * @param aSelect(output): True if we need to do another Select call.
 * @param aNfds(output): Value = 1 + maximum socket handle for all sockets
 *    that we are monitoring with the select call.
 */
void OsclSocketServI::ProcessSocketRequests(bool &aSelect, int &aNfds)
{
    //process all active requests

    aSelect = false;
    TOsclSocket maxsocket = 0;
    aNfds = (int)maxsocket + 1;

    // Pick up new requests from the app thread.
    iSockServRequestList.Lock();
    {
        iSockServRequestList.GetNewRequests();

        //flush any data on the loopback socket.
        iLoopbackSocket.Read();
    }
    iSockServRequestList.Unlock();

    if (iSockServRequestList.iActiveRequests.empty())
    {
        //nothing to do!
        return;
    }

    //Make a pass through the open request list and cancel or process each request.
    uint32 i;
    for (i = 0;i < iSockServRequestList.iActiveRequests.size();i++)
    {
        OsclSocketServRequestQElem* elem = &iSockServRequestList.iActiveRequests[i];

        if (elem->iCancel)
        {
            //Request was canceled
            elem->iSocketRequest->Complete(elem, OSCL_REQUEST_ERR_CANCEL);
        }
        else if (!IsServConnected())
        {
            //Server died or was closed.
            elem->iSocketRequest->Complete(elem, OSCL_REQUEST_ERR_GENERAL
                                           , (iServError) ? iServError : PVSOCK_ERR_SERV_NOT_CONNECTED);
        }
        else
        {
            //These routines will start the request, or else process
            //the results of prior select call, and also set the select
            //flags for the next call.

            elem->iSelect = 0;

            switch (elem->iSocketRequest->Fxn())
            {
                case EPVSocketShutdown:
                    elem->iSocketRequest->iSocketI->ProcessShutdown(elem);
                    break;

                case EPVSocketConnect:
                    elem->iSocketRequest->iSocketI->ProcessConnect(elem);
                    break;

                case EPVSocketAccept:
                    elem->iSocketRequest->iSocketI->ProcessAccept(elem);
                    break;

                case EPVSocketSend:
                    elem->iSocketRequest->iSocketI->ProcessSend(elem);
                    break;

                case EPVSocketSendTo:
                    elem->iSocketRequest->iSocketI->ProcessSendTo(elem);
                    break;

                case EPVSocketRecv:
                    elem->iSocketRequest->iSocketI->ProcessRecv(elem);
                    break;

                case EPVSocketRecvFrom:
                    elem->iSocketRequest->iSocketI->ProcessRecvFrom(elem);
                    break;

                default:
                    OSCL_ASSERT(0);
                    break;
            }
        }
    }

    //Zero out any old select set
    FD_ZERO(&iReadset);
    FD_ZERO(&iWriteset);
    FD_ZERO(&iExceptset);
    LOGSERV((0, "OsclSocketServI::ProcessSocketRequests Clearing select set"));

    //Now make a pass to either delete the request or collate the select flags.
    for (i = 0;i < iSockServRequestList.iActiveRequests.size();)
    {
        OsclSocketServRequestQElem* elem = &iSockServRequestList.iActiveRequests[i];

        if (elem->iSocketRequest)
        {
            //request is still active
            i++;

            if (elem->iSelect > 0)
            {
                //Need to do a select call for this socket

                if (!aSelect)
                    aSelect = true;

                TOsclSocket osock = elem->iSocketRequest->iSocketI->Socket();

                if (osock > maxsocket)
                {
                    LOGSERV((0, "OsclSocketServI::ProcessSocketRequests Setting Maxsocket to %d", osock));
                    maxsocket = osock;
                }

                //Add the socket to the select set.  Keep in mind there can be multiple requests
                //per socket, so check whether the socket is already added before adding.

                if ((elem->iSelect & OSCL_READSET_FLAG) == OSCL_READSET_FLAG
                        && !(FD_ISSET(osock, &iReadset)))
                {
                    FD_SET(osock, &iReadset);
                    LOGSERV((0, "OsclSocketServI::ProcessSocketRequests Setting Readset for %d", osock));
                }

                if ((elem->iSelect & OSCL_WRITESET_FLAG) == OSCL_WRITESET_FLAG
                        && !(FD_ISSET(osock, &iWriteset)))
                {
                    FD_SET(osock, &iWriteset);
                    LOGSERV((0, "OsclSocketServI::ProcessSocketRequests Setting Writeset for %d", osock));
                }

                if ((elem->iSelect & OSCL_EXCEPTSET_FLAG) == OSCL_EXCEPTSET_FLAG
                        && !(FD_ISSET(osock, &iExceptset)))
                {
                    FD_SET(osock, &iExceptset);
                    LOGSERV((0, "OsclSocketServI::ProcessSocketRequests Setting Exceptset for %d", osock));
                }
            }
        }
        else
        {
            //request is complete and can be deleted.
            iSockServRequestList.iActiveRequests.erase(elem);
        }
    }

    //also monitor the loopback socket if we're going to call select.
    iLoopbackSocket.ProcessSelect(aSelect, maxsocket);

    if (aSelect)
    {
        aNfds = (int)(maxsocket + 1);
    }
    LOGSERV((0, "OsclSocketServI::ProcessSocketRequests Select %d, NFDS %d", aSelect, aNfds));
}
#endif //OsclSocketSelect

void OsclSocketServI::ServerEntry()
//Server entry processing
{
    iLogger = PVLogger::GetLoggerObject("osclsocket_serv");

    iServError = 0;
    iServState = OsclSocketServI::ESocketServ_Connected;

    iSockServRequestList.Open(this);

#ifdef OsclSocketSelect
    FD_ZERO(&iReadset);
    FD_ZERO(&iWriteset);
    FD_ZERO(&iExceptset);
#endif
}

void OsclSocketServI::ServerExit()
//Server exit processing
{
    //change state if this was a normal exit.
    if (iServState == OsclSocketServI::ESocketServ_Connected)
    {
        iServState = OsclSocketServI::ESocketServ_Idle;
    }

#ifdef OsclSocketSelect
    //Go through the active requests one last time.
    //All the requests will complete with errors
    //since the server is no longer connected.
    bool doSelect;
    int nfds;
    ProcessSocketRequests(doSelect, nfds);

    iSockServRequestList.Close();

    //make sure sets are clear so resources get cleaned up.
    FD_ZERO(&iReadset);
    FD_ZERO(&iWriteset);
    FD_ZERO(&iExceptset);
#endif//OsclSocketSelect
}

void OsclSocketServI::ConstructServImp()
{
#if(PV_SOCKET_SERVER_IS_THREAD)
    iClose = false;
    iStart.Create();
    iExit.Create();
#endif
}

void OsclSocketServI::CleanupServImp()
{
#if(PV_SOCKET_SERVER_IS_THREAD)
    iStart.Close();
    iExit.Close();
#endif
}

#if(PV_SOCKET_SERVER_IS_THREAD)
//socket server thread routine
static TOsclThreadFuncRet OSCL_THREAD_DECL sockthreadmain(TOsclThreadFuncArg arg);
#endif

int32 OsclSocketServI::StartServImp()
{
#if(PV_SOCKET_SERVER_IS_THREAD)

    //setup the loopback socket and/or polling interval.
    iLoopbackSocket.iEnable = false;
    iSelectPollIntervalMsec = 0;

    //check the select timeout in the configuration.
    int32 selectTimeoutMsec = PV_SOCKET_SERVER_SELECT_TIMEOUT_MSEC;
    if (selectTimeoutMsec <= 0)
    {
        //non-polling option selected.
        //create the select cancel pipe.
        iLoopbackSocket.Init(this);

        //if loopback socket isn't available, we must poll.
        if (!iLoopbackSocket.iEnable)
            iSelectPollIntervalMsec = 10;
    }
    else
    {
        //polling option selected.
        iSelectPollIntervalMsec = selectTimeoutMsec;
#if(PV_SOCKET_SERVER_SELECT_LOOPBACK_SOCKET)
        //create the loopback socket.
        iLoopbackSocket.Init(this);
#endif
    }

    //Start the server thread.
    OsclThread thread;
    OsclProcStatus::eOsclProcError err = thread.Create((TOsclThreadFuncPtr)sockthreadmain,
                                         1024,
                                         (TOsclThreadFuncArg)this);
    if (err != OsclErrNone)
        return OsclErrGeneral;

    thread.SetPriority(PV_SOCKET_SERVER_THREAD_PRIORITY);

    //wait til thread starts
    iStart.Wait();

    return OsclErrNone;

#else//PV_SOCKET_SERVER_IS_THREAD

    //Socket server AO startup.

    iLoopbackSocket.iEnable = false;

    ServerEntry();

    if (!IsAdded())
    {
        AddToScheduler();
    }
    return OsclErrNone;

#endif //PV_SOCKET_SERVER_IS_THREAD
}

void OsclSocketServI::StopServImp()
{
#if(PV_SOCKET_SERVER_IS_THREAD)
    //stop the thread.
    iClose = true;
    iSockServRequestList.Wakeup();//wake up the thread if needed.
    WakeupBlockingSelect();
    //wait til thread exits
    iExit.Wait();

#else//PV_SOCKET_SERVER_IS_THREAD

    //Socket server AO cleanup.
    if (iServState != OsclSocketServI::ESocketServ_Connected)
    {
        return;
    }

    //cancel any active request.
    if (IsAdded())
    {
        Cancel();
    }

    ServerExit();

    //remove AO from scheduler.
    if (IsAdded())
    {
        RemoveFromScheduler();
    }
#endif//PV_SOCKET_SERVER_IS_THREAD
}


#if(PV_SOCKET_SERVER_IS_THREAD)

void OsclSocketServI::InThread()
//Socket server thread implementation.
{
    OsclThread::GetId(iThreadId);

    iClose = false;

    ServerEntry();

#ifndef OsclSocketSelect
    //no implementation!
    iServState = OsclSocketServI::ESocketServ_Error;
    iStart.Signal();
    return;
#else

    //Let server know thread is started and ready to
    //process requests.
    iStart.Signal();

    //create select timeout structure
    timeval timeout;

    bool doSelect, ok;
    int nfds;
    int nhandles = 0;

    while (!iClose)
    {
        //process active requests.
        ProcessSocketRequests(doSelect, nfds);

        //Make the select call if needed.
        if (doSelect)
        {
            //Set the fixed timeout.  The select call may update this value
            //so it needs to be set on each call.
            timeout.tv_sec = 0;

            if (iSelectPollIntervalMsec == 0)
            {
                //wait forever
                timeout.tv_usec = 0x1fffffff;
            }
            else
            {
                //poll
                timeout.tv_usec = iSelectPollIntervalMsec * 1000;
            }

            LOGSERV((0, "OsclSocketServI::InThread Calling select, timeout %d", iSelectPollIntervalMsec));
            OsclSocketSelect(nfds, iReadset, iWriteset, iExceptset, timeout, ok, iServError, nhandles);
            LOGSERV((0, "OsclSocketServI::InThread Select call returned"));
            if (!ok)
            {
                //select error.
                iServState = OsclSocketServI::ESocketServ_Error;
                break;
            }
            if (nhandles)
            {
                ADD_STATS(EOsclSocketServ_SelectActivity);
            }
            else
            {
                ADD_STATS(EOsclSocketServ_SelectNoActivity);
            }
        }
        else
        {
            //wait on new requests from the app side.
            LOGSERV((0, "OsclSocketServI::InThread Waiting on requests"));
            iSockServRequestList.WaitOnRequests();
            LOGSERV((0, "OsclSocketServI::InThread Done Waiting on requests"));
        }

    }//select loop

    ServerExit();

#endif //OsclSocketSelect

    //signal close complete to caller...
    if (iClose)
    {
        iClose = false;
        iExit.Signal();
    }
}

static TOsclThreadFuncRet OSCL_THREAD_DECL sockthreadmain2(TOsclThreadFuncArg arg);
static TOsclThreadFuncRet OSCL_THREAD_DECL sockthreadmain(TOsclThreadFuncArg arg)
//socket server thread.
{
    OsclBase::Init();
    OsclErrorTrap::Init();

    //once error trap is initialized, run everything else under a trap
    int32 err;
    OSCL_TRY(err,

             OsclMem::Init();
             PVLogger::Init();

             sockthreadmain2(arg);

             PVLogger::Cleanup();
             OsclMem::Cleanup();
            );

    OsclErrorTrap::Cleanup();
    OsclBase::Cleanup();

    return 0;
}


#include "pvlogger.h"

#if(PV_OSCL_SOCKET_SERVER_LOGGER_OUTPUT)
#include "pvlogger_time_and_id_layout.h"
#include "pvlogger_file_appender.h"
#include "pvlogger_stderr_appender.h"

#define LOGFILENAME _STRLIT_WCHAR("pvosclsocket_serv.log");

template<class DestructClass>
class LogAppenderDestructDealloc : public OsclDestructDealloc
{
    public:
        void destruct_and_dealloc(OsclAny *ptr)
        {
            delete((DestructClass*)ptr);
        }
};
#endif//PV_OSCL_SOCKET_SERVER_LOGGER_OUTPUT


static TOsclThreadFuncRet OSCL_THREAD_DECL sockthreadmain2(TOsclThreadFuncArg arg)
{
    OSCL_ASSERT(arg);
    OsclSocketServI *serv = (OsclSocketServI*)arg;

#if(PV_OSCL_SOCKET_SERVER_LOGGER_OUTPUT)
    //create logger appenders.
    PVLoggerAppender* appender;

#if 1
//File logger
    //find an unused filename so we don't over-write any prior logs
    Oscl_FileServer fs;
    Oscl_File file;
    fs.Connect();
    OSCL_wHeapString<OsclMemAllocator> filename;
    oscl_wchar fileid[2];
    fileid[1] = (oscl_wchar)'\0';
    for (char c = 'A';c <= 'Z';c++)
    {
        filename = LOGFILENAME;
        fileid[0] = (oscl_wchar)c;
        filename += fileid;
        filename += _STRLIT_WCHAR(".txt");
        if (file.Open((oscl_wchar*)filename.get_cstr(), Oscl_File::MODE_READ, fs) != 0)
        {
            break;//found a nonexistent file.
        }
        file.Close();
    }
    fs.Close();
    //create appender using the selected filename.
    appender = TextFileAppender<TimeAndIdLayout, 1024>::CreateAppender((OSCL_TCHAR*)filename.get_cstr());
    OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
        new OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > >(appender);
#else
//stderr logger.
    appender = new StdErrAppender<TimeAndIdLayout, 1024>();
    OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
        new OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > >(appender);
#endif
    //Set logging options.
    OsclSharedPtr<PVLoggerAppender> appenderPtr(appender, appenderRefCounter);
    PVLogger *rootnode = PVLogger::GetLoggerObject("");
    rootnode->AddAppender(appenderPtr);
    rootnode->SetLogLevel(PVLOGMSG_DEBUG);
#endif //PV_OSCL_SOCKET_SERVER_LOGGER_OUTPUT

    serv->InThread();

    return 0;
}


#else//PV_SOCKET_SERVER_IS_THREAD
//Non-threaded section


void OsclSocketServI::Run()
//Socket server AO
{
    int nhandles = 0;
    bool doSelect = false;;

#ifdef OsclSocketSelect
    int nfds;

    //process active requests.
    ProcessSocketRequests(doSelect, nfds);

    //Make the select call if needed.
    if (doSelect)
    {
        // Reset the flag
        doSelect = false;

        //use a delay of zero since we're essentially polling for socket activity.
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        bool ok;
        OsclSocketSelect(nfds, iReadset, iWriteset, iExceptset, timeout, ok, iServError, nhandles);
        if (!ok)
        {
            //a select error is fatal.
            StopServImp();//stop the AO
            iServState = OsclSocketServI::ESocketServ_Error;
            return;
        }

        if (nhandles)
        {
            ADD_STATS(EOsclSocketServ_SelectActivity);
        }
        else
        {
            ADD_STATS(EOsclSocketServ_SelectNoActivity);
        }

        if (nhandles > 0)
        {
            // Select() call reported a request was completed so complete the socket request
            ProcessSocketRequests(doSelect, nfds);
        }
    }

#else
    OsclError::Panic("OSCLSOCK", 1);//no implementation.
#endif //OsclSocketSelect

    // Re-schedule as long as there are requests and waiting for
    // select() to report a request is complete
    if (!iSockServRequestList.iActiveRequests.empty())
    {
        if (doSelect)
        {
            // Requests were processed and there are still requests waiting
            // for select() to report a request is complete so check again
            // as possible as.
            ADD_STATS(EOsclSocketServ_SelectRescheduleAsap);
            RunIfNotReady();
        }
        else
        {
            // The select() call did not report any completion so check again
            // afte the specified interval
            ADD_STATS(EOsclSocketServ_SelectReschedulePoll);
            RunIfNotReady(1000*PV_SOCKET_SERVER_AO_INTERVAL_MSEC);
        }
    }
}


#endif//PV_SOCKET_SERVER_IS_THREAD
#endif //PV_SOCKET_SERVER



