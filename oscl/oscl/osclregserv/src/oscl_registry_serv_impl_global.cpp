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

#include "oscl_registry_serv_impl_global.h"

#include "osclconfig_proc.h"

//Global variable implementation.
OsclComponentRegistry* gOsclComponentRegistry;

static int calls = 0;

OsclRegistryServImpl::OsclRegistryServImpl()
{
    if (calls == 0)
    {
        gOsclComponentRegistry = new OsclComponentRegistry();
    }

    calls++;

    iOsclComponentRegistry = gOsclComponentRegistry;

    iIsOpen = false;
}

OsclRegistryServImpl::~OsclRegistryServImpl()
{
    if (iIsOpen)
    {
        Close();
    }

    calls--;

    if (calls == 0)
    {
        if (iOsclComponentRegistry)
        {
            delete iOsclComponentRegistry;
            iOsclComponentRegistry = 0;
            gOsclComponentRegistry = 0;
        }
    }
}

int32 OsclRegistryServImpl::Connect()
{
    if (iIsOpen)
    {
        return OsclErrInvalidState;
    }
    iIsOpen = true;
    iOsclComponentRegistry->OpenSession();
    return OsclErrNone;
}

void OsclRegistryServImpl::Close()
{
    //session cleanup
    if (!iIdVec.empty())
    {
        //unregister all comps that were registered by this session
        for (uint32 i = 0;i < iIdVec.size();i++)
            iOsclComponentRegistry->Unregister(iIdVec[i]);
        //clear our comp list.
        iIdVec.destroy();
    }

    if (iIsOpen)
    {
        iOsclComponentRegistry->CloseSession();
        iIsOpen = false;
    }
}

int32 OsclRegistryServImpl::Register(OSCL_String& aComp, OsclComponentFactory aFac)
{
    if (!IsOpen())
        return OsclErrInvalidState;

    int32 err;
    OSCL_TRY(err, iIdVec.reserve(iIdVec.size() + 1););
    if (err != OsclErrNone)
        return err;

    uint32 id;
    int32 result = iOsclComponentRegistry->Register(id, aComp, aFac);

    //save all comp IDs in our session data
    if (result == OsclErrNone)
        iIdVec.push_back(id);//can't leave, already reserved space.

    return result;
}

int32 OsclRegistryServImpl::UnRegister(OSCL_String& aComp)
{
    if (!IsOpen())
        return OsclErrInvalidState;

    return iOsclComponentRegistry->Unregister(aComp);
}

OsclComponentFactory OsclRegistryServImpl::GetFactory(OSCL_String& aComp)
{
    if (!IsOpen())
    {
        return NULL;
    }

    return iOsclComponentRegistry->FindExact(aComp);
}

void OsclRegistryServImpl::GetFactories(OSCL_String& aReg, Oscl_Vector<OsclRegistryAccessElement, OsclMemAllocator>& aVec)
{
    if (!IsOpen())
        return;

    iOsclComponentRegistry->FindHierarchical(aReg, aVec);
}










