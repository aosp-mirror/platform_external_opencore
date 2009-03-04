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
 * @file pvmf_jitter_buffer_port.h
 */
#ifndef PVMF_JITTER_BUFFER_PORT_H_INCLUDED
#define PVMF_JITTER_BUFFER_PORT_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_PORT_BASE_IMPL_H_INCLUDED
#include "pvmf_port_base_impl.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif
#ifndef PVMF_JITTER_BUFFER_H_INCLUDED
#include "pvmf_jitter_buffer.h"
#endif
#ifndef __MEDIA_CLOCK_CONVERTER_H
#include "media_clock_converter.h"
#endif
#ifndef PVMF_STREAMING_MANAGER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif
#ifndef PVMF_STREAMING_BUFFER_ALLOCATORS_H_INCLUDED
#include "pvmf_streaming_buffer_allocators.h"
#endif
#ifndef PVMF_RTCP_TIMER_H_INCLUDED
#include "pvmf_rtcp_timer.h"
#endif
#ifndef PVMI_PORT_CONFIG_KVP_H_INCLUDED
#include "pvmi_port_config_kvp.h"
#endif

//Default vector reserve size
#define PVMF_JITTER_BUFFER_NODE_PORT_VECTOR_RESERVE 10

// Capability mime strings
#define PVMF_JITTER_BUFFER_PORT_SPECIFIC_ALLOCATOR "x-pvmf/pvmfstreaming/socketmemallocator"
#define PVMF_JITTER_BUFFER_PORT_SPECIFIC_ALLOCATOR_VALTYPE "x-pvmf/pvmfstreaming/socketmemallocator;valtype=ksv"

/** Enumerated list of port tags supported by this port */
typedef enum
{
    PVMF_JITTER_BUFFER_PORT_TYPE_UNKNOWN = -1,
    PVMF_JITTER_BUFFER_PORT_TYPE_INPUT = 0,
    PVMF_JITTER_BUFFER_PORT_TYPE_OUTPUT = 1,
    PVMF_JITTER_BUFFER_PORT_TYPE_FEEDBACK = 2
} PVMFJitterBufferNodePortTag;

typedef enum
{
    PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_UNKNOWN = -1,
    PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RTP = 0,
    PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_ASF = 1,
    PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_RDT = 2
} PVMFJitterBufferNodePortTransportType;

class PVMFJitterBufferPortParams
{
    public:
        PVMFJitterBufferPortParams()
        {
            id = -1;
            tag = PVMF_JITTER_BUFFER_PORT_TYPE_UNKNOWN;
            iPort = NULL;
            iJitterBuffer = NULL;
            timeScale = 0;
            iSocketAlloc = 0;
            iStartTimeInMS = 0;
            iStopTimeInMS = 0;
            oUpStreamEOSRecvd = false;
            oEOSReached = false;
            iNumMediaMsgsRecvd = 0;
            iNumMediaMsgsSent = 0;
            oJitterBufferEmpty = false;
            oRateAdaptation = false;
            iRateAdaptationFeedBackFrequency = 0;
            iRateAdaptationRTCPRRCount = 0;
            iRateAdaptationFreeBufferSpaceInBytes = 0;
            oProcessIncomingMessages = true;
            oProcessOutgoingMessages = true;
            oInPlaceProcessing = false;
            iMediaDataAlloc = NULL;
            iMediaDataImplAlloc = NULL;
            iMediaMsgAlloc = NULL;
            oFireWallPacketRecvd = false;
            iFireWallPacketCount = 0;
            SSRC = 0;
            oMonitorForRemoteActivity = true;
            bTransportHeaderPreParsed = false;
            iInitialRtcp = true;
            iLastMsgTimeStamp = 0;

            iRTCPIntervalInMicroSeconds = 0;
            iRTCPTimer = NULL;
            avg_rtcp_size = 0.0;
            numSenders = RR = RS = 0;
            iInitialRtcp = true;

            RtcpBwConfigured = false;
            eTransportType = PVMF_JITTER_BUFFER_PORT_TRANSPORT_TYPE_UNKNOWN;
        };

        ~PVMFJitterBufferPortParams()
        {
        }

        void ResetParams()
        {
            oUpStreamEOSRecvd = false;
            oEOSReached = false;
            oJitterBufferEmpty = false;
            oProcessIncomingMessages = true;
            oProcessOutgoingMessages = true;
            oMonitorForRemoteActivity = true;
            bTransportHeaderPreParsed = false;
            RtcpBwConfigured = false;
            avg_rtcp_size = 0.0;
        };

        int32                       id;
        PVMFJitterBufferNodePortTag tag;
        PVMFPortInterface* iPort;
        PVMFJitterBuffer*  iJitterBuffer;
        uint32             timeScale;
        uint32             bitrate;
        MediaClockConverter mediaClockConverter;
        PVMFTimestamp iLastMsgTimeStamp;

        bool bTransportHeaderPreParsed;

        OSCL_HeapString<PVMFJitterBufferNodeAllocator> iTransportType;
        PVMFJitterBufferNodePortTransportType eTransportType;
        OSCL_HeapString<PVMFJitterBufferNodeAllocator> iMimeType;
        PVMFSharedSocketDataBufferAlloc* iSocketAlloc;
        OsclRefCounterMemFrag iTrackConfig;
        PVMFRTCPStats iRTCPStats;
        int32 iStartTimeInMS;
        int32 iStopTimeInMS;
        bool  oUpStreamEOSRecvd;
        bool  oEOSReached;
        uint32 iNumMediaMsgsRecvd;
        uint32 iNumMediaMsgsSent;
        bool  oJitterBufferEmpty;
        bool  oRateAdaptation;
        uint32 iRateAdaptationFeedBackFrequency;
        uint32 iRateAdaptationRTCPRRCount;
        uint32 iRateAdaptationFreeBufferSpaceInBytes;
        bool oProcessIncomingMessages;
        bool oProcessOutgoingMessages;
        bool oInPlaceProcessing;
        /* Firewall packet related - valid only for RTP ports */
        /* allocator for memory fragment */
        OsclMemPoolFixedChunkAllocator* iMediaDataAlloc;
        /* allocator for media data impl */
        PVMFSimpleMediaBufferCombinedAlloc* iMediaDataImplAlloc;
        /* Memory pool for simple media data */
        OsclMemPoolFixedChunkAllocator *iMediaMsgAlloc;
        bool oFireWallPacketRecvd;
        uint32 iFireWallPacketCount;
        uint32 SSRC;
        bool oMonitorForRemoteActivity;

        // RTCP interval related
        bool RtcpBwConfigured;
        int numSenders;
        uint32 RR;
        uint32 RS;
        bool iInitialRtcp;
        float avg_rtcp_size;
        uint32 iRTCPIntervalInMicroSeconds;
        PvmfRtcpTimer* iRTCPTimer;

};

/**
 * Input (sink) ports have a simple flow control scheme.
 * Ports report "busy" when their queue is full, then when the
 * queue goes to half-empty they issue a "get data" to the connected
 * port.  The media message in the "get data" is empty and is
 * meant to be discarded.
 * Output (source) ports assume the connected port uses the
 * same flow-control scheme.
 */
class PVMFJitterBufferPort : public PvmfPortBaseImpl,
            public PvmiCapabilityAndConfig
{
    public:
        /**
         * Default constructor. Default settings will be used for the data queues.
         * @param aId ID assigned to this port
         * @param aTag Port tag
         * @param aNode Container node
         */
        PVMFJitterBufferPort(int32 aTag, PVMFNodeInterface* aNode, const char*);

        /**
         * Constructor that allows the node to configure the data queues of this port.
         * @param aTag Port tag
         * @param aNode Container node
         * @param aSize Data queue capacity. The data queue size will not grow beyond this capacity.
         * @param aReserve Size of data queue for which memory is reserved. This must be
         * less than or equal to the capacity. If this is less than capacity, memory will be
         * allocated when the queue grows beyond the reserve size, but will stop growing at
         * capacity.
         * @param aThreshold Ready-to-receive threshold, in terms of percentage of the data queue capacity.
         * This value should be between 0 - 100.
         */
        PVMFJitterBufferPort(int32 aTag,
                             PVMFNodeInterface* aNode,
                             uint32 aInCapacity,
                             uint32 aInReserve,
                             uint32 aInThreshold,
                             uint32 aOutCapacity,
                             uint32 aOutReserve,
                             uint32 aOutThreshold, const char*);

        /** Destructor */
        ~PVMFJitterBufferPort();

        /* Over ride Connect() */
        PVMFStatus Connect(PVMFPortInterface* aPort);

        /* Over ride QueryInterface - this port supports config interface */
        void QueryInterface(const PVUuid &aUuid, OsclAny*&aPtr)
        {
            if (aUuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
                aPtr = (PvmiCapabilityAndConfig*)this;
            else
                aPtr = NULL;
        }

        // Implement pure virtuals from PvmiCapabilityAndConfig interface
        PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                                     PvmiKvp*& aParameters, int& num_parameter_elements,	PvmiCapabilityContext aContext);
        PVMFStatus releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);
        void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                               int num_elements, PvmiKvp * & aRet_kvp);
        PVMFStatus verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);

        // Unsupported PvmiCapabilityAndConfig methods
        void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
        {
            OSCL_UNUSED_ARG(aObserver);
        };
        void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
        {
            OSCL_UNUSED_ARG(aSession);
            OSCL_UNUSED_ARG(aContext);
        };
        void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                  PvmiKvp* aParameters, int num_parameter_elements)
        {
            OSCL_UNUSED_ARG(aSession);
            OSCL_UNUSED_ARG(aContext);
            OSCL_UNUSED_ARG(aParameters);
            OSCL_UNUSED_ARG(num_parameter_elements);
        };
        void DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
        {
            OSCL_UNUSED_ARG(aSession);
            OSCL_UNUSED_ARG(aContext);
        };
        PVMFCommandId setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                         int num_elements, PvmiKvp*& aRet_kvp, OsclAny* context = NULL)
        {
            OSCL_UNUSED_ARG(aSession);
            OSCL_UNUSED_ARG(aParameters);
            OSCL_UNUSED_ARG(num_elements);
            OSCL_UNUSED_ARG(aRet_kvp);
            OSCL_UNUSED_ARG(context);
            return -1;
        }
        uint32 getCapabilityMetric(PvmiMIOSession aSession)
        {
            OSCL_UNUSED_ARG(aSession);
            return 0;
        }

        void createPortAllocators(OSCL_String& aMimeType, uint32 aSizeInBytes);
        void createPortAllocators(OSCL_String& aMimeType, uint32 aSizeInBytes,
                                  uint maxNumResizes, uint resizeSize);

        OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> getPortDataAlloc()
        {
            return iPortDataAlloc;
        }

        uint32 getAvailableBufferSpace(bool aFirstParentChunkOnly)
        {
            if (iBufferAlloc != NULL)
            {
                return (iBufferAlloc->getAvailableBufferSpace(aFirstParentChunkOnly));
            }
            else if (iBufferNoResizeAlloc != NULL)
            {
                return (iBufferNoResizeAlloc->getAvailableBufferSpace(aFirstParentChunkOnly));
            }
            else
            {
                //Error
                return 0;
            }
        }

        PVMFJitterBufferPortParams* iPortParams;
        // Corresponding port paired with current port
        PVMFJitterBufferPort*       iPortCounterpart;
        // Parameters of port paired with current port (e.g. iPortCounterpart)
        PVMFJitterBufferPortParams* iCounterpartPortParams;


        //overrides from PVMFPortInterface
        PVMFStatus QueueOutgoingMsg(PVMFSharedMediaMsgPtr aMsg);
        bool IsOutgoingQueueBusy();

    private:
        void Construct();

        void pvmiSetPortAllocatorSync(PvmiCapabilityAndConfig *aPort,
                                      const char* aFormatValType);

        void createSocketDataAllocReSize(OSCL_String& aMimeType, int32 size,
                                         bool userParams = false,
                                         uint maxNumResizes = 0,
                                         uint resizeSize = 0);

        PVMFSocketBufferAllocator* iBufferNoResizeAlloc;
        PVMFSMSharedBufferAllocWithReSize* iBufferAlloc;
        OsclSharedPtr<PVMFSharedSocketDataBufferAlloc> iPortDataAlloc;

        PVLogger *iLogger;
        PVMFFormatType iFormat;
        PVMFJitterBufferNodePortTag iPortType;

        bool iInPlaceDataProcessing;

        PVMFJitterBufferNode* iJitterBufferNode;

        friend class PVMFJitterBufferNode;
        friend class Oscl_TAlloc<PVMFJitterBufferPort, PVMFJitterBufferNodeAllocator>;
        friend class PVMFJitterBufferExtensionInterfaceImpl;
};

#endif // PVMF_JITTER_BUFFER_PORT_H_INCLUDED



