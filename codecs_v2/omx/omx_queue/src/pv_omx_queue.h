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
/**
	Implements a simple FIFO structure used for queueing OMX buffers.
*/

#ifndef PV_OMX_QUEUE_H_INCLUDED
#define PV_OMX_QUEUE_H_INCLUDED

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OMX_Types_h
#include "omx_types.h"
#endif

/* Maximum number of elements in a queue.
 * Note that we make no attempt to deal with buffer-full
 * situations. This is OK, since there is a limited number
 * of buffers that can be enqueued, so we will never
 * fill up the queue as long as the queue is bigger than
 * the number of buffers.
 */
#define MAX_QUEUE_ELEMENTS 32

typedef struct QueueType
{
    void *queue[MAX_QUEUE_ELEMENTS];
    int inptr;
    int outptr;
}QueueType;

OSCL_IMPORT_REF void QueueInit(QueueType* aQueue);

OSCL_IMPORT_REF void QueueDeinit(QueueType* aQueue);

OSCL_IMPORT_REF void Queue(QueueType* aQueue, void* aData);

// returns NULL if the queue is empty
OSCL_IMPORT_REF void* DeQueue(QueueType* aQueue);

OSCL_IMPORT_REF OMX_S32 GetQueueNumElem(QueueType* aQueue);

#endif		//#ifndef PV_OMX_QUEUE_H_INCLUDED


