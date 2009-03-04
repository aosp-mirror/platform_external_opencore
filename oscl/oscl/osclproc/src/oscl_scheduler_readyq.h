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



/** \file oscl_scheduler_readyq.h
    \brief ready q types for oscl scheduler
*/


#ifndef OSCL_SCHEDULER_READYQ_H_INCLUDED
#define OSCL_SCHEDULER_READYQ_H_INCLUDED

#ifndef OSCL_SCHEDULER_TYPES_H_INCLUDED
#include "oscl_scheduler_types.h"
#endif
#ifndef OSCL_SCHEDULER_TUNEABLES_H_INCLUDED
#include "oscl_scheduler_tuneables.h"
#endif


#ifndef OSCL_PRIQUEUE_H_INCLUDED
#include "oscl_priqueue.h"
#endif
#ifndef OSCL_BASE_ALLOC_H_INCLUDED
#include "oscl_base_alloc.h"
#endif
#ifndef OSCL_SEMAPHORE_H_INCLUDED
#include "oscl_semaphore.h"
#endif
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif
#ifndef OSCL_MUTEX_H_INCLUDED
#include "oscl_mutex.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif

class PVActiveBase;

class OsclReadyAlloc: public Oscl_DefAlloc
{
    public:
        OsclAny* allocate(const uint32 size) ;
        OsclAny* allocate_fl(const uint32 size, const char * file_name, const int line_num);
        void deallocate(OsclAny* p) ;
    private:
        OsclMemAllocator iBasicAlloc;
};

typedef PVActiveBase* TOsclReady;

class OsclReadyCompare
{
    public:
        int compare(TOsclReady& a, TOsclReady& b) const ;
};

/**This is a thread-safe priority queue for holding the
    active objects that are ready to run.
*/
class PVLogger;
class OsclReadyQ
            : public OsclPriorityQueue<TOsclReady, OsclReadyAlloc, Oscl_Vector<TOsclReady, OsclReadyAlloc>, OsclReadyCompare>
{
    public:
        void Init(int, const char*);
        void Clear();
        void Add(TOsclReady, bool);
        void Remove(TOsclReady);
        TOsclReady PopTop();
        TOsclReady Top();
        void Pop(TOsclReady);
        bool IsIn(TOsclReady);
        bool IsInMT(TOsclReady);
        static bool IsInAny(TOsclReady);
        uint32 Depth();
        void Open();
        void Close();
        void Lock();
        void Unlock();
        void Wait();
        bool Wait(uint32);
        void Signal(uint32 = 1);
        void Print();
    private:
        //mutex for thread protection
        OsclMutex iCrit;

        //this semaphore tracks the queue size.  it is used to
        //regulate the scheduling loop when running in blocking mode.
        OsclSemaphore iSem;

        PVLogger* iLogger;
        OSCL_HeapString<OsclMemAllocator> iName;

        //a sequence number needed to maintain FIFO sorting order in oscl pri queue.
        uint32 iSeqNumCounter;

};

/** This class defines the queue link.  Each AO contains its own
     queue link object.
*/
class TReadyQueLink
{
    public:
        TReadyQueLink()
        {
            iAOPriority = 0;
            iTimeToRunTicks = 0;
            iTimerSort = false;
            iSeqNum = 0;
            iIsIn = NULL;
        }

        int32 iAOPriority;//scheduling priority
        uint32 iTimeToRunTicks;//for active timers, this
        //is the time to run in ticks.
        uint32 iTimeQueuedTicks;//the time when the AO was queued, in ticks.
        bool iTimerSort;//sort by time, then priority.
        uint32 iSeqNum;//sequence number for oscl pri queue.
        static int compare(TReadyQueLink& a, TReadyQueLink &b);//for oscl pri queue.
        OsclReadyQ* iIsIn;//pointer to the queue we're in.

};

#endif


/*! @} */
