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
#ifndef PVMF_EVENT_HANDLING_H_INCLUDED
#define PVMF_EVENT_HANDLING_H_INCLUDED


#include "oscl_base.h"
#include "oscl_mem_auto_ptr.h"
#include "oscl_mem_basic_functions.h"
#include "pvmf_return_codes.h"
#include "pv_interface.h"


/**
   Identifies the specific observer session
**/
typedef int32 PVMFSessionId;

/**
   Identifies the specific API/command that was invoked
**/
typedef int32 PVMFCommandType;

/**
   A unique command id identifying an invocation of any command
**/
typedef int32 PVMFCommandId;

typedef enum
{
    PVMFCmdRespEvent,
    PVMFErrorEvent,
    PVMFInfoEvent,
    PVMFEventLast
} PVMFEventCategory;

/**
   Identifies the type of event (error & informational)
**/
typedef int32 PVMFEventType;


/**
   The base class for PVMF callback events
**/
class PVMFEventBase
{
    public:
        PVMFEventBase() {}

        virtual ~PVMFEventBase() {};

        /**
         * @return the event's category.
         */
        OSCL_IMPORT_REF virtual PVMFEventCategory IsA() const = 0;
};

/**
 * PVMFCmdResp Class
 *
 * PVMFCmdResp class is used to pass completion status on previously issued
 * commands
 **/
class OSCL_IMPORT_REF PVMFCmdResp : public PVMFEventBase
{
    public:
        /**
         * Constructor for PVMFCmdResp
         */
        PVMFCmdResp(PVMFCommandId aId,
                    const OsclAny* aContext,
                    PVMFStatus aStatus,
                    OsclAny* aEventData = NULL):
                iId(aId),
                iContext(aContext),
                iStatus(aStatus),
                iEventExtInterface(NULL),
                iEventData(aEventData)
        {
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        /**
         * Constructor for PVMFCmdResp
         */
        PVMFCmdResp(PVMFCommandId aId,
                    const OsclAny* aContext,
                    PVMFStatus aStatus,
                    PVInterface* aEventExtInterface):
                iId(aId),
                iContext(aContext),
                iStatus(aStatus),
                iEventExtInterface(aEventExtInterface),
                iEventData(NULL)
        {
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        /**
         * Constructor for PVMFCmdResp
         */
        PVMFCmdResp(PVMFCommandId aId,
                    const OsclAny* aContext,
                    PVMFStatus aStatus,
                    PVInterface* aEventExtInterface,
                    OsclAny* aEventData):
                iId(aId),
                iContext(aContext),
                iStatus(aStatus),
                iEventExtInterface(aEventExtInterface),
                iEventData(aEventData)
        {
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        virtual ~PVMFCmdResp() {}

        OSCL_IMPORT_REF virtual PVMFEventCategory IsA() const;

        /**
         * @return Returns the unique ID associated with a command of this type.
         */
        PVMFCommandId GetCmdId()const
        {
            return iId;
        }

        /**
         * @return Returns the opaque data that was passed in with the command.
         */
        const OsclAny* GetContext()const
        {
            return iContext;
        }

        /**
         * @return Returns the completion status of the command
         */
        PVMFStatus GetCmdStatus()const
        {
            return iStatus;
        }

        /**
         * This method is going to be deprecated soon. We intend to remove
         * the opaque event data and use PVInterface pointer if needed to
         * retrieve more information regarding command completion
         *
         *
         * @return Returns additional data asociated with the command/event.  This is to be interpreted
         based on the type and the return status
        */
        OsclAny* GetEventData()const
        {
            return iEventData;
        }

        /**
         * This method is going to be deprecated soon. We intend to remove
         * the opaque event data and use PVInterface pointer if needed to
         * retrieve more information regarding command completion.Therefore,
         * with the removal of event data, setting length of event data wont be of any significance either.
         *
         * @param1 - (uint32) length of event data in bytes.
         * @return PVMFSuccess, if length of event data can be set.
         PVMFFailure, if length of event data can't be set.
        */
        PVMFStatus SetEventDataLen(uint32 aEventDataLength)
        {
            PVMFStatus status = PVMFFailure;
            if (iEventData)
            {
                iEventDataLengthAvailable = true;
                iEventDataLength = aEventDataLength;
                status = PVMFSuccess;
            }
            return status;
        }

        /**
         * This method is going to be deprecated soon. We intend to remove
         * the opaque event data and use PVInterface pointer if needed to
         * retrieve more information regarding command completion.Therefore,
         * with the removal of event data, length if event data wont be needed either.
         *
         * @param1 - bool& aEventDataLenAvailable
         *           false - length of event data(in bytes) is not available
         *           true - length of event data(in bytes) is available
         * @param2 - uint32& aEventDataLength
         *           length of eventdata in bytes
         */
        void GetEventDataLen(bool& aEventDataLenAvailable, uint32& aEventDataLength)const
        {
            aEventDataLenAvailable = false;
            aEventDataLength = 0;
            if (iEventDataLengthAvailable)
            {
                aEventDataLenAvailable = true;
                aEventDataLength = iEventDataLength;
            }
        }

        /**
         * @return Returns the eventinfointerface
         */
        PVInterface* GetEventExtensionInterface() const
        {
            return iEventExtInterface;
        }

    protected:
        PVMFCommandId   iId;
        const OsclAny* iContext;
        PVMFStatus iStatus;
        PVInterface*  iEventExtInterface;
        /**
         * We STRONGLY DISCOURAGE use of this. This field will be deprecated
         * soon.
         */
        OsclAny* iEventData;
        bool iEventDataLengthAvailable;
        uint32 iEventDataLength;
};


/**
 * PVMFAsyncEvent Class
 *
 * PVMFAsyncEvent is the base class used to pass unsolicited error and informational
 * indications to the user. Additional information can be tagged based on the specific
 * event
 **/
#define PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE 16
class OSCL_IMPORT_REF PVMFAsyncEvent : public PVMFEventBase
{
    public:
        PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                       PVMFEventType aEventType,
                       OsclAny* aContext,
                       OsclAny* aEventData) :
                iEventCategory(aEventCategory)
                , iEventType(aEventType)
                , iEventExtInterface(NULL)
                , iLocalBufferSize(0)
                , iContext(aContext)
                , iEventData(aEventData)
        {
            oscl_memset(iLocalBuffer, 0, PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                       PVMFEventType aEventType,
                       OsclAny* aContext,
                       OsclAny* aEventData,
                       const void* aLocalBuffer,
                       const size_t aLocalBufferSize) :
                iEventCategory(aEventCategory)
                , iEventType(aEventType)
                , iEventExtInterface(NULL)
                , iLocalBufferSize(aLocalBufferSize)
                , iContext(aContext)
                , iEventData(aEventData)
        {
            if (aLocalBuffer)
            {
                OSCL_ASSERT(iLocalBufferSize <= PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
                if (iLocalBufferSize > PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE)
                {
                    iLocalBufferSize = PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE;
                }

                oscl_memcpy(iLocalBuffer, aLocalBuffer, iLocalBufferSize);
            }
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                       PVMFEventType aEventType,
                       OsclAny* aContext,
                       PVInterface* aEventExtInterface,
                       OsclAny* aEventData) :
                iEventCategory(aEventCategory)
                , iEventType(aEventType)
                , iEventExtInterface(aEventExtInterface)
                , iLocalBufferSize(0)
                , iContext(aContext)
                , iEventData(aEventData)
        {
            oscl_memset(iLocalBuffer, 0, PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        PVMFAsyncEvent(PVMFEventCategory aEventCategory,
                       PVMFEventType aEventType,
                       OsclAny* aContext,
                       PVInterface* aEventExtInterface,
                       OsclAny* aEventData,
                       const void* aLocalBuffer,
                       const size_t aLocalBufferSize) :
                iEventCategory(aEventCategory)
                , iEventType(aEventType)
                , iEventExtInterface(aEventExtInterface)
                , iLocalBufferSize(aLocalBufferSize)
                , iContext(aContext)
                , iEventData(aEventData)
        {
            if (aLocalBuffer)
            {
                OSCL_ASSERT(iLocalBufferSize <= PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE);
                if (iLocalBufferSize > PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE)
                {
                    iLocalBufferSize = PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE;
                }

                oscl_memcpy(iLocalBuffer, aLocalBuffer, iLocalBufferSize);
            }
            iEventDataLengthAvailable = false;
            iEventDataLength = 0;
        }

        virtual ~PVMFAsyncEvent() {}

        OSCL_IMPORT_REF virtual PVMFEventCategory IsA() const;

        /**
         * @return Returns the unique type identifier of the event.
         */
        PVMFEventType GetEventType()const
        {
            return iEventType;
        }

        /**
         * This method is going to be deprecated soon. We intend to remove
         * the opaque event data and use PVInterface pointer if needed to
         * retrieve more information regarding command completion
         *
         *
         * @return Returns additional data asociated with the event.
         *         This is to be interpreted
         *         based on the type and the return status
         */
        OsclAny* GetEventData() const
        {
            return iEventData;
        }

        /**
         * This method is going to be deprecated soon. We intend to remove
         * the opaque event data and use PVInterface pointer if needed to
         * retrieve more information regarding command completion.Therefore,
         * with the removal of event data, setting length of event data wont be of any significance either.
         *
         * @param1 - (uint32) length of event data in bytes.
         * @return PVMFSuccess, if length of event data can be set.
         PVMFFailure, if length of event data can't be set.
        */
        PVMFStatus SetEventDataLen(uint32 aEventDataLength)
        {
            PVMFStatus status = PVMFFailure;
            if (iEventData)
            {
                iEventDataLengthAvailable = true;
                iEventDataLength = aEventDataLength;
                status = PVMFSuccess;
            }
            return status;
        }

        /**
         * This method is going to be deprecated soon. We intend to remove
         * the opaque event data and use PVInterface pointer if needed to
         * retrieve more information regarding command completion.Therefore,
         * with the removal of event data, length if event data wont be needed either.
         *
         * @param1 - bool& aEventDataLenAvailable
         *           false - length of event data(in bytes) is not available
         *           true - length of event data(in bytes) is available
         * @param2 - uint32& aEventDataLength
         *           length of eventdata in bytes
         */
        void GetEventDataLen(bool& aEventDataLenAvailable, uint32& aEventDataLength)const
        {
            aEventDataLenAvailable = false;
            aEventDataLength = 0;
            if (iEventDataLengthAvailable)
            {
                aEventDataLenAvailable = true;
                aEventDataLength = iEventDataLength;
            }
        }

        /**
         * @return Returns the size of the local data asociated with the event.
         */
        size_t GetLocalBufferSize() const
        {
            return iLocalBufferSize;
        }

        /**
         * @return Returns the local data asociated with the event.
         * TODO: This is a const method returning a non const ref to some
         * internal array.
         */
        uint8* GetLocalBuffer() const
        {
            return (uint8*)iLocalBuffer;
        }

        /**
         * @return Returns the opaque data associated with the callback type.
         */
        const OsclAny* GetContext()const
        {
            return iContext;
        }

        /**
         * @return Returns the eventinfointerface
         */
        PVInterface* GetEventExtensionInterface() const
        {
            return iEventExtInterface;
        }

    protected:
        PVMFEventCategory iEventCategory;
        PVMFEventType iEventType;
        PVInterface*  iEventExtInterface;
        uint8 iLocalBuffer[PVMF_ASYNC_EVENT_LOCAL_BUF_SIZE];
        size_t iLocalBufferSize;
        OsclAny* iContext;
        /**
         * We STRONGLY DISCOURAGE use of this. This field will be deprecated
         * soon.
         */
        OsclAny* iEventData;
        bool iEventDataLengthAvailable;
        uint32 iEventDataLength;
};

#endif // PVMF_EVENT_HANDLING_H_INCLUDED
