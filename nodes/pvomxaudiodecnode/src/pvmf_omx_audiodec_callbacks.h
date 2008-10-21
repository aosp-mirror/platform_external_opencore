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
#ifndef PVMF_OMX_AUDIODEC_CALLBACKS_H_INCLUDED
#define PVMF_OMX_AUDIODEC_CALLBACKS_H_INCLUDED


/* Audio: 3 OMX callback related active object definitions */


#ifndef THREADSAFE_CALLBACK_AO_H_INCLUDED
#include "threadsafe_callback_ao.h"
#endif

#ifndef THREADSAFE_MEMPOOL_H_INCLUDED
#include "threadsafe_mempool.h"
#endif


#ifndef OMX_Types_h
#include "omx_types.h"
#endif

#ifndef OMX_Core_h
#include "omx_core.h"
#endif

#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif
// structure that contains EventHandler callback type parameters:
typedef struct EventHandlerSpecificData_Audio
{
    OMX_HANDLETYPE hComponent;
    OMX_PTR pAppData;
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    OMX_PTR pEventData;
} EventHandlerSpecificData_Audio;


// structure that contains EmptyBufferDone callback type parameters:
typedef struct EmptyBufferDoneSpecificData_Audio
{
    OMX_HANDLETYPE hComponent;
    OMX_PTR pAppData;
    OMX_BUFFERHEADERTYPE* pBuffer;
} EmptyBufferDoneSpecificData_Audio;


// structure that contains FillBufferDone callback type parameters:
typedef struct FillBufferDoneSpecificData_Audio
{
    OMX_HANDLETYPE hComponent;
    OMX_PTR pAppData;
    OMX_BUFFERHEADERTYPE* pBuffer;
} FillBufferDoneSpecificData_Audio;


// This class defines the callback AO that handles the callbacks from the remote thread in a thread-safe way.
// The callback events arriving from the remote thread are queued and processed later in PV thread context.
// The class is DERIVED from the "ThreadSafeCallbackAO" class defined in "threadsafe_callback_ao.h/cpp"
// OVERLOAD THE METHOD : "ProcessEvent" so that it does something meaningful


// Test AO receives the remote thread specific API callback and then calls the generic API "ReceiveEvent"
const char EventHandlerAOName_Audio[] = "EventHandlerCallbackAO_Audio";
const char EmptyBufferDoneAOName_Audio[] = "EventHandlerCallbackAO_Audio";
const char FillBufferDoneAOName_Audio[] = "EventHandlerCallbackAO_Audio";


/**************** CLASS FOR EVENT HANDLER *************/
class EventHandlerThreadSafeCallbackAO_Audio : public ThreadSafeCallbackAO
{
    public:
        // Constructor
        EventHandlerThreadSafeCallbackAO_Audio(
            void* aObserver = NULL,
            uint32 aDepth = DEFAULT_QUEUE_DEPTH,
            const char* aAOname = EventHandlerAOName_Audio,
            int32 aPriority = OsclActiveObject::EPriorityNominal);


        // OVERLOADED ProcessEvent
        // overloaded Run and DeQueue to optimize performance (and process more than 1 event per Run)
        virtual void Run();
        virtual OsclAny* DeQueue(OsclReturnCode &stat);

        OsclReturnCode ProcessEvent(OsclAny* EventData);
        virtual ~EventHandlerThreadSafeCallbackAO_Audio();
        ThreadSafeMemPoolFixedChunkAllocator *iMemoryPool;

};


/**************** CLASS FOR EVENT HANDLER *************/
class EmptyBufferDoneThreadSafeCallbackAO_Audio : public ThreadSafeCallbackAO
{
    public:
        // Constructor
        EmptyBufferDoneThreadSafeCallbackAO_Audio(
            void* aObserver = NULL,
            uint32 aDepth = DEFAULT_QUEUE_DEPTH,
            const char* aAOname = EmptyBufferDoneAOName_Audio,
            int32 aPriority = OsclActiveObject::EPriorityNominal);

        // OVERLOADED ProcessEvent
        OsclReturnCode ProcessEvent(OsclAny* EventData);

        // overloaded Run and DeQueue to optimize performance (and process more than 1 event per Run)
        virtual void Run();
        virtual OsclAny* DeQueue(OsclReturnCode &stat);

        virtual ~EmptyBufferDoneThreadSafeCallbackAO_Audio();
        ThreadSafeMemPoolFixedChunkAllocator *iMemoryPool;
};




/**************** CLASS FOR EVENT HANDLER *************/
class FillBufferDoneThreadSafeCallbackAO_Audio : public ThreadSafeCallbackAO
{
    public:
        // Constructor
        FillBufferDoneThreadSafeCallbackAO_Audio(void* aObserver = NULL,
                uint32 aDepth = DEFAULT_QUEUE_DEPTH,
                const char* aAOname = FillBufferDoneAOName_Audio,
                int32 aPriority = OsclActiveObject::EPriorityNominal);

        // OVERLOADED ProcessEvent
        OsclReturnCode ProcessEvent(OsclAny* EventData);
        // overloaded Run and DeQueue to optimize performance (and process more than 1 event per Run)
        virtual void Run();
        virtual OsclAny* DeQueue(OsclReturnCode &stat);

        virtual ~FillBufferDoneThreadSafeCallbackAO_Audio();
        ThreadSafeMemPoolFixedChunkAllocator *iMemoryPool;
};

#endif	//#ifndef PVMF_OMX_AUDIODEC_CALLBACKS_H_INLCUDED
