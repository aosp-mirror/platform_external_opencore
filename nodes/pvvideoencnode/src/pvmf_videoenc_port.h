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
#ifndef PVMF_VIDEOENC_PORT_H_INCLUDED
#define PVMF_VIDEOENC_PORT_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef PVMF_PORT_BASE_IMPL_H_INCLUDED
#include "pvmf_port_base_impl.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_UTILS_H_INCLUDED
#include "pvmi_config_and_capability_utils.h"
#endif

// Forward declaration
class PVMFVideoEncNode;

class PVMFVideoEncPort : public PvmfPortBaseImpl,
            public PvmiCapabilityAndConfig,
            public PVMFPortActivityHandler,
            public OsclActiveObject
{
    public:
        PVMFVideoEncPort(int32 aTag, PVMFVideoEncNode* aNode, int32 aPriority, const char* name = NULL);
        ~PVMFVideoEncPort();

        // Overload PvmfPortBaseImpl methods
        OSCL_IMPORT_REF PVMFStatus Connect(PVMFPortInterface* aPort);
        OSCL_IMPORT_REF void QueryInterface(const PVUuid& aUuid, OsclAny*& aPtr);


        // Implement pure virtuals from PvmiCapabilityAndConfig interface
        OSCL_IMPORT_REF void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver);
        OSCL_IMPORT_REF PVMFStatus getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier,
                PvmiKvp*& aParameters, int& num_parameter_elements,
                PvmiCapabilityContext aContext);
        OSCL_IMPORT_REF PVMFStatus releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);
        OSCL_IMPORT_REF void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        OSCL_IMPORT_REF void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                PvmiKvp* aParameters, int num_parameter_elements);
        OSCL_IMPORT_REF void DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        OSCL_IMPORT_REF void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                               int num_elements, PvmiKvp * & aRet_kvp);
        OSCL_IMPORT_REF PVMFCommandId setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                int num_elements, PvmiKvp*& aRet_kvp, OsclAny* context = NULL);
        OSCL_IMPORT_REF uint32 getCapabilityMetric(PvmiMIOSession aSession);
        OSCL_IMPORT_REF PVMFStatus verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int num_elements);

        PVMFStatus SetFormat(PVMFFormatType aFormat);

        // Notification from node to resume processing incoming msg
        void ProcessIncomingMsgReady();
        void ProcessOutgoingMsgReady();

        // From PVMFPortActivityHandler
        void HandlePortActivity(const PVMFPortActivity& aActivity);

    private:
        // Implement pure virtuals from OsclActiveObject
        void Run();

        /**
         * Check if a format is supported for a specific port type
         * @return true if specified format is supported for the specified port type
         */
        bool IsFormatSupported(PVMFFormatType aFormat);

        /**
         * Synchronous query of input port parameters
         */
        PVMFStatus GetInputParametersSync(PvmiKeyType identifier, PvmiKvp*& parameters,
                                          int& num_parameter_elements);

        /**
         * Synchronous query of output port parameters
         */
        PVMFStatus GetOutputParametersSync(PvmiKeyType identifier, PvmiKvp*& parameters,
                                           int& num_parameter_elements);
        /**
         * Allocate a specified number of key-value pairs and set the keys
         *
         * @param aKvp Output parameter to hold the allocated key-value pairs
         * @param aKey Key for the allocated key-value pairs
         * @param aNumParams Number of key-value pairs to be allocated
         * @return Completion status
         */
        PVMFStatus AllocateKvp(PvmiKvp*& aKvp, PvmiKeyType aKey, int32 aNumParams);

        /**
         * Verify one key-value pair parameter against capability of the port and
         * if the aSetParam flag is set, set the value of the parameter corresponding to
         * the key.
         *
         * @param aKvp Key-value pair parameter to be verified
         * @param aSetParam If true, set the value of parameter corresponding to the key.
         * @return PVMFSuccess if parameter is supported, else PVMFFailure
         */
        PVMFStatus VerifyAndSetParameter(PvmiKvp* aKvp, bool aSetParam = false);

        /**
         * Negotiates input port settings (format, video size and frame rate) and
         * configures the peer port and the container node with these settings
         *
         * @param aConfig Capability and config object for peer port
         * @return PVMFSuccess if settings are successfully negotiated.
         */
        PVMFStatus NegotiateInputSettings(PvmiCapabilityAndConfig* aConfig);

        /**
         * Negotiates output port settings and configures the peer port using settings
         * from the container node.
         *
         * @param aConfig Capability and config object for peer port
         * @return PVMFSuccess if settings are successfully negotiated.
         */
        PVMFStatus NegotiateOutputSettings(PvmiCapabilityAndConfig* aConfig);

        PVMFFormatType iFormat;
        OsclMemAllocator iAlloc;
        PVMFVideoEncNode* iNode;
};

class PVMFVideoEncInputFormatCompareLess
{
    public:
        /**
         * The algorithm used in OsclPriorityQueue needs a compare function
         * that returns true when A's priority is less than B's
         * @return true if A's priority is less than B's, else false
         */
        int compare(PvmiKvp*& a, PvmiKvp*& b) const
        {
            return (PVMFVideoEncInputFormatCompareLess::GetPriority(a) <
                    PVMFVideoEncInputFormatCompareLess::GetPriority(b));
        }

        /**
         * Returns the priority of each command
         * @return A 0-based priority number. A lower number indicates lower priority.
         */
        static int GetPriority(PvmiKvp*& aKvp)
        {
            switch (aKvp->value.uint32_value)
            {
                case PVMF_YUV420:
                    return 3;
                case PVMF_YUV422:
                    return 2;
                case PVMF_RGB12:
                    return 1;
                case PVMF_RGB24:
                    return 0;
                default:
                    return 0;
            }
        }
};

#endif // PVMF_VIDEOENC_INPORT_H_INCLUDED
