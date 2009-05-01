/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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

#include "pvmf_node_interface.h"

// TODO: Both the info and error handling look very similar, we should
// try to refactor them at some point (but beware of the info/error
// context difference).

OSCL_EXPORT_REF void PVMFNodeInterface::SetState(TPVMFNodeInterfaceState s)
{
    iInterfaceState = s;
    ReportInfoEvent(PVMFInfoStateChanged, (OsclAny*)s);
}

OSCL_EXPORT_REF void PVMFNodeInterface::ReportCmdCompleteEvent(
        PVMFSessionId session, const PVMFCmdResp& resp)
{
    const unsigned int size = iSessions.size();

    for (unsigned int i = 0; i < size; ++i)
    {
        if (iSessions[i].iId == session)
        {
            PVMFNodeCmdStatusObserver *observer = iSessions[i].iInfo.iCmdStatusObserver;

            if (NULL != observer)
            {
                observer->NodeCommandCompleted(resp);
            }
            return;
        }
    }
}

OSCL_EXPORT_REF void PVMFNodeInterface::ReportErrorEvent(
        PVMFEventType aEventType, OsclAny* aEventData, PVInterface*aExtMsg)
{
    ReportErrorEvent(PVMFAsyncEvent(PVMFErrorEvent,
                                    aEventType,
                                    NULL,  // Error Context will be set below.
                                    aExtMsg,
                                    aEventData));
}

OSCL_EXPORT_REF void PVMFNodeInterface::ReportErrorEvent(
        const PVMFAsyncEvent& aEvent)
{
    if (aEvent.IsA() != PVMFErrorEvent)
    {
        // TODO: Should log loudly an error here.
        return;
    }

    const unsigned int size = iSessions.size();

    for (unsigned int i = 0; i < size; ++i)
    {
        PVMFNodeErrorEventObserver *observer = iSessions[i].iInfo.iErrorObserver;

        if (NULL != observer)
        {
            PVMFAsyncEvent event(PVMFErrorEvent,
                                 aEvent.GetEventType(),
                                 iSessions[i].iInfo.iErrorContext,
                                 aEvent.GetEventExtensionInterface(),
                                 aEvent.GetEventData(),
                                 aEvent.GetLocalBuffer(),
                                 aEvent.GetLocalBufferSize());
            observer->HandleNodeErrorEvent(event);
        }
    }
}

OSCL_EXPORT_REF void PVMFNodeInterface::ReportInfoEvent(
        PVMFEventType aEventType, OsclAny* aEventData, PVInterface*aExtMsg)
{
    ReportInfoEvent(PVMFAsyncEvent(PVMFInfoEvent,
                                   aEventType,
                                   NULL,  // Info Context will be set below.
                                   aExtMsg,
                                   aEventData));
}

OSCL_EXPORT_REF void PVMFNodeInterface::ReportInfoEvent(
        const PVMFAsyncEvent& aEvent)
{
    if (aEvent.IsA() != PVMFInfoEvent)
    {
        // TODO: Should log loudly an error here.
        return;
    }

    const unsigned int size = iSessions.size();

    for (unsigned int i = 0; i < size; ++i)
    {
        PVMFNodeInfoEventObserver *observer = iSessions[i].iInfo.iInfoObserver;

        if (NULL != observer)
        {
            PVMFAsyncEvent event(PVMFInfoEvent,
                                 aEvent.GetEventType(),
                                 iSessions[i].iInfo.iInfoContext,
                                 aEvent.GetEventExtensionInterface(),
                                 aEvent.GetEventData(),
                                 aEvent.GetLocalBuffer(),
                                 aEvent.GetLocalBufferSize());

            observer->HandleNodeInformationalEvent(event);
        }
    }
}
