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

#include "oscl_base.h"

#include "oscl_tls.h"
#include "oscl_assert.h"


OSCL_EXPORT_REF void TLSStorageOps::save_registry(TOsclTlsKey* key, OsclAny* ptr, int32 &aError)
{
    aError = 0;
#if (OSCL_TLS_IS_KEYED)
    OSCL_ASSERT(key);
    if (!OSCL_TLS_STORE_FUNC(*key, ptr))
    {
        aError = EPVErrorBaseSystemCallFailed;
        return;
    }
#else
    OSCL_UNUSED_ARG(key);
    if (!OSCL_TLS_STORE_FUNC(ptr))
    {
        aError = EPVErrorBaseSystemCallFailed;
        return;
    }
#endif
}

OSCL_EXPORT_REF OsclAny* TLSStorageOps::get_registry(TOsclTlsKey* key)
{
#if (OSCL_TLS_IS_KEYED)
    OSCL_ASSERT(key);
    return OSCL_TLS_GET_FUNC(*key);
#else
    OSCL_UNUSED_ARG(key);
    return OSCL_TLS_GET_FUNC();
#endif
}

#if (OSCL_TLS_IS_KEYED)

//Global lookup table for Tls Keys.
//There's one entry per thread.  Thread ID is used to lookup the key.
OsclTLSRegistry::TlsKeyTable* OsclTLSRegistry::iTlsKeyTable = NULL;


void OsclTLSRegistry::GetThreadId(TOsclTlsThreadId &threadId, int32 &aError)
//Get thread ID of current thread.
{
    aError = 0;
#if defined OSCL_TLS_THREAD_ID_FUNC

    threadId = OSCL_TLS_THREAD_ID_FUNC();

#elif defined OSCL_TLS_THREAD_ID_FUNC_EXPR

    if (!OSCL_TLS_THREAD_ID_FUNC_EXPR(threadId))
        aError = EPVErrorBaseSystemCallFailed;

#else
#error No Thread ID Function!
#endif
}

TOsclTlsKey* OsclTLSRegistry::LookupTlsKey(int32 &aError)
//Lookup Tls Key for current thread.
{
    aError = 0;

    if (!iTlsKeyTable)
    {
        aError = EPVErrorBaseNotInstalled;//No table!
        return NULL;
    }

    //Get the thread ID.
    TOsclTlsThreadId threadId;
    GetThreadId(threadId, aError);
    if (aError)
        return NULL;

    //Search the Tls Key Table for this thread's entry.
    iTlsKeyTable->iLock.Lock();
    for (uint32 i = 0;i < OSCL_TLS_MAX_THREADS;i++)
    {
        if (OSCL_TLS_THREAD_ID_EQUAL(iTlsKeyTable->iKeys[i].iThreadId, threadId))
        {
            //found it!
            TOsclTlsKey* key = iTlsKeyTable->iKeys[i].iTlsKey;
            iTlsKeyTable->iLock.Unlock();
            return key;
        }
    }
    iTlsKeyTable->iLock.Unlock();

    return NULL;
}

bool OsclTLSRegistry::SaveTlsKey(TOsclTlsKey* aKey, int32 &aError)
//Save Tls key in table.
{
    OSCL_ASSERT(aKey);

    aError = 0;

    if (!iTlsKeyTable)
    {
        aError = EPVErrorBaseNotInstalled;//No table!
        return false;
    }

    bool saved = false;
    iTlsKeyTable->iLock.Lock();
    for (uint32 i = 0;i < OSCL_TLS_MAX_THREADS;i++)
    {
        if (iTlsKeyTable->iKeys[i].iTlsKey == NULL)
        {
            //found an empty entry.
            iTlsKeyTable->iKeys[i].iTlsKey = aKey;
            GetThreadId(iTlsKeyTable->iKeys[i].iThreadId, aError);
            if (aError) {
                // we don't want to store the key if the thread id cannot be found
                iTlsKeyTable->iKeys[i].iTlsKey = NULL;
                break;//can't get thread ID.
            }
            iTlsKeyTable->iNumKeys++;
            saved = true;
            break;
        }
    }
    iTlsKeyTable->iLock.Unlock();
    return saved;
}

bool OsclTLSRegistry::RemoveTlsKey(Oscl_DefAlloc& alloc, TOsclTlsKey * aKey, int32& aError)
//Remove Tls key from table
{
    OSCL_ASSERT(aKey);

    aError = 0;

    bool found = false;

    if (!iTlsKeyTable)
    {
        aError = EPVErrorBaseNotInstalled;
        return found;
    }

    iTlsKeyTable->iLock.Lock();
    for (uint32 i = 0;i < OSCL_TLS_MAX_THREADS;i++)
    {
        if (iTlsKeyTable->iKeys[i].iTlsKey == aKey)
        {
            //found it.
            found = true;
            iTlsKeyTable->iKeys[i].iTlsKey = NULL;
            iTlsKeyTable->iNumKeys--;
            break;
        }
    }

    // FIXME:
    // This is temp fix so that the iTlsKeyTable never
    // get deallocated.
    //Cleanup the table when it's empty
    if (0 && iTlsKeyTable->iNumKeys == 0)
    {
        iTlsKeyTable->iLock.Unlock();
        iTlsKeyTable->~TlsKeyTable();
        alloc.deallocate(iTlsKeyTable);
        iTlsKeyTable = NULL;
    }
    else
    {
        iTlsKeyTable->iLock.Unlock();
    }

    return found;
}

#endif //OSCL_TLS_IS_KEYED



OSCL_EXPORT_REF void OsclTLSRegistry::initialize(Oscl_DefAlloc &alloc, int32 &aError)
{
    TOsclTlsKey* pkey = NULL;
    aError = 0;

#if ( OSCL_TLS_IS_KEYED)
    //Allocate the table on the first init call.
    //Note there's some risk of thread contention here, since
    //the thread lock is not available until after this step.
    if (!iTlsKeyTable)
    {
        OsclAny* table = alloc.allocate(sizeof(TlsKeyTable));
        if (table)
        {
            iTlsKeyTable = new(table) TlsKeyTable();
        }
        else
        {
            aError = EPVErrorBaseOutOfMemory;
            return;
        }
    }

    //allocate a tls Key for this thread and add
    //to table.

    if (LookupTlsKey(aError) != NULL)
    {
        aError = EPVErrorBaseAlreadyInstalled;
        return;
    }
    if (aError)
        return;//error in looking up TLS key.

    //allocate space for key
    pkey = (TOsclTlsKey*)alloc.allocate(sizeof(TOsclTlsKey));
    if (!pkey)
    {
        aError = EPVErrorBaseOutOfMemory;
        return;
    }

    //create key for this thread.
    if (!OSCL_TLS_KEY_CREATE_FUNC(*pkey))
    {
        alloc.deallocate(pkey);
        aError = EPVErrorBaseSystemCallFailed;
        return;
    }

    //save in table.
    if (!SaveTlsKey(pkey, aError))
    {
        //failed!
        OSCL_TLS_KEY_DELETE_FUNC(*pkey);
        alloc.deallocate(pkey);
        aError = EPVErrorBaseTooManyThreads;//can't save key
        return;
    }
    if (aError)
        return;//error in SaveTlsKey

#endif

    // allocate the space and save the pointer
    registry_pointer_type registry = OSCL_STATIC_CAST(registry_pointer_type,
                                     alloc.allocate(sizeof(registry_type) * OSCL_TLS_MAX_SLOTS));
    if (registry == 0)
    {
        aError = EPVErrorBaseOutOfMemory;
        return;
    }

    // initialize all TLSs to 0
    for (uint32 ii = 0; ii < OSCL_TLS_MAX_SLOTS; ii++)
        registry[ii] = 0;

    // save it away
    TLSStorageOps::save_registry(pkey, registry, aError);
}

OSCL_EXPORT_REF void OsclTLSRegistry::cleanup(Oscl_DefAlloc &alloc, int32 &aError)
{
    TOsclTlsKey* pkey = NULL;
    aError = 0;

#if (OSCL_TLS_IS_KEYED)
    pkey = LookupTlsKey(aError);
    if (!pkey)
    {
        aError = EPVErrorBaseNotInstalled;//No key!
        return;
    }
    if (aError)
        return;//error in LookupTlsKey
#endif

    //Cleanup this thread's registry
    registry_pointer_type registry = OSCL_STATIC_CAST(registry_pointer_type , TLSStorageOps::get_registry(pkey));
    if (registry == 0)
    {
        aError = EPVErrorBaseNotInstalled;//No registry!
        return;
    }
    alloc.deallocate(registry);

    TLSStorageOps::save_registry(pkey, NULL, aError);
    if (aError)
        return;

#if (OSCL_TLS_IS_KEYED)

    //Remove Tls key for this thread.
    bool ok = RemoveTlsKey(alloc, pkey, aError);
    if (aError)
        return;

    OSCL_ASSERT(ok);//key must be in table since we just looked it up
    OSCL_UNUSED_ARG(ok);

    //Deallocate key.
    OSCL_TLS_KEY_DELETE_FUNC(*pkey);
    alloc.deallocate(pkey);

#endif
}

OSCL_EXPORT_REF OsclAny* OsclTLSRegistry::getInstance(uint32 ID, int32 &aError)
{
    OSCL_ASSERT(ID < OSCL_TLS_MAX_SLOTS);

    aError = 0;

    TOsclTlsKey* pkey = NULL;

#if (OSCL_TLS_IS_KEYED)
    pkey = LookupTlsKey(aError);
    if (!pkey)
    {
        aError = EPVErrorBaseNotInstalled;//No key!
        return NULL;
    }
    if (aError)
        return NULL;//error in LookupTlsKey
#endif

    registry_pointer_type registry = OSCL_STATIC_CAST(registry_pointer_type , TLSStorageOps::get_registry(pkey));
    if (registry == 0)
    {
        aError = EPVErrorBaseNotInstalled;//No registry!
        return NULL;
    }

    return registry[ID];
}

OSCL_EXPORT_REF void OsclTLSRegistry::registerInstance(OsclAny* ptr, uint32 ID, int32 &aError)
{
    OSCL_ASSERT(ID < OSCL_TLS_MAX_SLOTS);

    aError = 0;
    TOsclTlsKey *pkey = NULL;

#if (OSCL_TLS_IS_KEYED)
    pkey = LookupTlsKey(aError);
    if (!pkey)
    {
        aError = EPVErrorBaseNotInstalled;//No key!
        return;
    }
    if (aError)
        return;//error in LookupTlsKey
#endif

    registry_pointer_type registry = OSCL_STATIC_CAST(registry_pointer_type , TLSStorageOps::get_registry(pkey));
    if (registry == 0)
    {
        aError = EPVErrorBaseNotInstalled;//no registry!
        return;
    }

    registry[ID] = ptr;

}



