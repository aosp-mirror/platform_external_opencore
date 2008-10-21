#include "android_audio_output_threadsafe_callbacks.h"
#include "android_audio_output.h"

AndroidAudioOutputThreadSafeCallbackAO::AndroidAudioOutputThreadSafeCallbackAO(void* aObserver,
                                                                               uint32 aDepth, 
                                                                               const char* aAOname,
                                                                               int32 aPriority)
:ThreadSafeCallbackAO(aObserver, aDepth, aAOname, aPriority)
{

}


AndroidAudioOutputThreadSafeCallbackAO::~AndroidAudioOutputThreadSafeCallbackAO()
{

}


OsclReturnCode AndroidAudioOutputThreadSafeCallbackAO::ProcessEvent(OsclAny* EventData)
{
    // In this case, ProcessEvent calls the method of the primary test AO to process the Event
    if (iObserver != NULL)
    {
        AndroidAudioOutput* ptr = (AndroidAudioOutput*) iObserver;
        // Call RunIfNotReady() for the AudioMIO
        ptr->RunIfNotReady();
    }
    return OsclSuccess;
}

#if PROCESS_MULTIPLE_EVENTS_IN_CALLBACK
void AndroidAudioOutputThreadSafeCallbackAO::Run()
{
    OsclAny *param;
    OsclReturnCode status = OsclSuccess;
    // Process multiple events in one attempt 
    do
    {
        param = Dequeue(status);
        if((status == OsclSuccess) || (status == OsclPending))
        {
            ProcessEvent(param);
        }
    }while(status == OsclSuccess);
}

OsclAny* AndroidAudioOutputThreadSafeCallbackAO::Dequeue(OsclReturnCode &status)
{
    OsclAny *param;
    OsclProcStatus::eOsclProcError sema_status;

    status = OsclSuccess;

    Mutex.Lock();
    if(Q->NumElem == 0)
    {
        status = OsclFailure;
        Mutex.Unlock();
        return NULL;
    }
    
    param = (Q->pFirst[Q->index_out]).pData;
    Q->index_out++;
    if(Q->index_out == Q->MaxNumElements)
        Q->index_out = 0;
    Q->NumElem--;
    if(Q->NumElem == 0)
    {
        // Call PendForExec() only when there are no more elements in the queue
        // Else, continue in the do-while loop
        PendForExec();
        status = OsclPending;
    }
    Mutex.Unlock();

    sema_status = RemoteThreadCtrlSema.Signal();
    if(sema_status != OsclProcStatus::SUCCESS_ERROR)
    {
        status = OsclFailure;
        return NULL;
    }
    return param;
}
#endif


