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
#if !defined(PVT_EVENTS_H)
#define PVT_EVENTS_H
#include "oscl_types.h"
#include "oscl_string_containers.h"
#include "pvt_common.h"
#include "pvt_params.h"

/* Base class for all PVTemrinal events */
class CPVTerminalEvent
{
    public:
        CPVTerminalEvent(OSCL_wHeapString<OsclMemAllocator>* desc = NULL)
        {
            if (desc)
                SetDescription(*desc);
        }
        virtual ~CPVTerminalEvent() {};

        /* Returns a texual description of the event */
        virtual const OSCL_wHeapString<OsclMemAllocator>* GetDescription() const
        {
            return &description;
        }
        virtual void SetDescription(OSCL_wHeapString<OsclMemAllocator>& desc)
        {
            description =  desc;
        }
    protected:
        OSCL_wHeapString<OsclMemAllocator> description;
};

/* Response to a command.  Contains the command id and the status code */
class CPVTerminalResponse : public CPVTerminalEvent
{
    public:
        CPVTerminalResponse(TPVStatusCode a_status_code):
                status_code(a_status_code) {};

        virtual ~CPVTerminalResponse() {};

        TPVStatusCode GetStatusCode() const
        {
            return status_code;
        }
    protected:
        TPVStatusCode status_code;
};

/* Unsolicited indication messages */
class CPVTerminalIndication : public CPVTerminalEvent
{
};

class CPVTerminalError : public CPVTerminalEvent
{
    public:
        CPVTerminalError(TPVSeverity s, OSCL_wHeapString<OsclMemAllocator>& desc) : CPVTerminalEvent(&desc), severity(s) {}
        virtual ~CPVTerminalError() {};
        TPVSeverity GetSeverity()
        {
            return severity;
        }
    private:
        TPVSeverity severity;
};


class ConnectResponse : public CPVTerminalResponse
{
    public:
        ConnectResponse(TPVStatusCode a_status_code): CPVTerminalResponse(a_status_code) {}
    private:
};


class DisconnectResponse : public CPVTerminalResponse
{
    public:
        DisconnectResponse(TPVStatusCode a_status_code, TPVTerminalIdentifier which):
                CPVTerminalResponse(a_status_code), iWhich(which) {};
        virtual ~DisconnectResponse() {};

        TPVTerminalIdentifier GetTerminalIdentifier()
        {
            return iWhich;
        }
    private:
        TPVTerminalIdentifier iWhich;
};

class HoldResponse : public CPVTerminalResponse
{
    public:
        HoldResponse(TPVStatusCode a_status_code): CPVTerminalResponse(a_status_code) {};
        virtual ~HoldResponse() {};
};
class ReleaseHoldResponse : public CPVTerminalResponse
{
    public:
        ReleaseHoldResponse(TPVStatusCode a_status_code): CPVTerminalResponse(a_status_code) {};
        virtual ~ReleaseHoldResponse() {};
};
class SetChannelParamsResponse : public CPVTerminalResponse
{
    public:
        SetChannelParamsResponse(TPVStatusCode a_status_code): CPVTerminalResponse(a_status_code) {};
        virtual ~SetChannelParamsResponse() {};
};
class SetMuxParamsResponse : public CPVTerminalResponse
{
    public:
        SetMuxParamsResponse(TPVStatusCode a_status_code): CPVTerminalResponse(a_status_code) {};
        virtual ~SetMuxParamsResponse() {};
};

class OpenChannelResponse : public CPVTerminalResponse
{
    public:
        OpenChannelResponse(TPVStatusCode a_status_code, PV2WayMediaType type, CPVChannelParam* param):
                CPVTerminalResponse(a_status_code), iChannelParam(param), iType(type) {};
        virtual ~OpenChannelResponse() {};
        PV2WayMediaType GetMediaType()
        {
            return iType;
        }
        CPVChannelParam* GetChannelParam()
        {
            return iChannelParam;
        }
    private:
        CPVChannelParam* iChannelParam;
        PV2WayMediaType iType;
};

class CloseChannelResponse : public CPVTerminalResponse
{
    public:
        CloseChannelResponse(TPVDirection aDirection, TPVChannelId aChannelId, TPVStatusCode a_status_code):
                CPVTerminalResponse(a_status_code), iDirection(aDirection), iChannelId(aChannelId)
        {
        }

        virtual ~CloseChannelResponse() {};

        TPVDirection GetDirection()
        {
            return iDirection;
        }

        TPVChannelId GetChannelId()
        {
            return iChannelId;
        }
    private:
        TPVDirection     iDirection;
        TPVChannelId     iChannelId;
};

class CPVChannelError: public CPVTerminalError
{
    public:
        CPVChannelError(TPVDirection dir, TPVChannelId id, TPVSeverity s, OSCL_wHeapString<OsclMemAllocator>& desc) :
                CPVTerminalError(s, desc), direction(dir), cid(id) {};
        virtual ~CPVChannelError() {};
        TPVDirection GetDirection()
        {
            return direction;
        }
        TPVChannelId GetChannelId()
        {
            return cid;
        }
    private:
        TPVDirection direction;
        TPVChannelId cid;
};

#endif
