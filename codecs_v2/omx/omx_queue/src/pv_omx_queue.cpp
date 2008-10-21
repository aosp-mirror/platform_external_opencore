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

#include "pv_omx_queue.h"

// Use default DLL entry point for Symbian
#ifndef OSCL_DLL_H_INCLUDED
#include "oscl_dll.h"
#endif

OSCL_DLL_ENTRY_POINT_DEFAULT()

OSCL_EXPORT_REF void QueueInit(QueueType* aQueue)
{
    aQueue->inptr = 0;
    aQueue->outptr = 0;
}

OSCL_EXPORT_REF void QueueDeinit(QueueType* aQueue)
{
    aQueue->inptr = 0;
    aQueue->outptr = 0;
}

OSCL_EXPORT_REF void Queue(QueueType* aQueue, void* aData)
{
    aQueue->queue[aQueue->inptr++] = aData;
    if (aQueue->inptr >= MAX_QUEUE_ELEMENTS)
    {
        aQueue->inptr = 0;
    }
}

OSCL_EXPORT_REF void* DeQueue(QueueType* aQueue)
{
    if (aQueue->inptr == aQueue->outptr)
    {
        return NULL;
    }

    void *ret =  aQueue->queue[aQueue->outptr++];
    if (aQueue->outptr >= MAX_QUEUE_ELEMENTS)
    {
        aQueue->outptr = 0;
    }
    return ret;
}

OSCL_EXPORT_REF OMX_S32 GetQueueNumElem(QueueType* aQueue)
{
    int num = aQueue->inptr - aQueue->outptr;
    if (num < 0)
    {
        num += MAX_QUEUE_ELEMENTS;
    }
    return num;
}

