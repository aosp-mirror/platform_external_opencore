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
#ifndef PVMF_AVCENC_NODE_H_INCLUDED
#include "pvmf_avcenc_node.h"
#endif

#ifndef PVMF_AVCENC_PORT_H_INCLUDED
#include "pvmf_avcenc_port.h"
#endif

#ifndef OSCL_MIME_STRING_UTILS_H
#include "pv_mime_string_utils.h"
#endif

#ifndef PVMI_KVP_UTIL_H_INCLUDED
#include "pvmi_kvp_util.h"
#endif


// Structure to hold the key string info for
// AVCEncnode's capability-and-config
struct PVAVCEncNodeKeyStringData
{
    char iString[64];
    PvmiKvpType iType;
    PvmiKvpValueType iValueType;
};

// The number of characters to allocate for the key string
#define PVAVCENCNODECONFIG_KEYSTRING_SIZE 128

// Key string info at the base level ("x-pvmf/avc/encoder")
#define PVAVCENCNODECONFIG_BASE_NUMKEYS 3

const PVAVCEncNodeKeyStringData PVAVCEncNodeConfigBaseKeys[PVAVCENCNODECONFIG_BASE_NUMKEYS] =
{
    {"encoding_mode", PVMI_KVPTYPE_VALUE, PVMI_KVPVALTYPE_UINT32},
};

enum BaseKeys_IndexMapType
{
    ENCODING_MODE = 0
};

PVMFStatus PVMFAvcEncNode::GetConfigParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr aReqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::GetConfigParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (NULL == aParameters)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::GetConfigParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));
    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVAVCENCNODECONFIG_KEYSTRING_SIZE * sizeof(char));
    if (NULL == memblock)
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::GetConfigParameter() Memory allocation for key string failed"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, PVAVCENCNODECONFIG_KEYSTRING_SIZE * sizeof(char));
    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/avc/encoder/"), 20);
    oscl_strncat(aParameters[0].key, PVAVCEncNodeConfigBaseKeys[aIndex].iString, oscl_strlen(PVAVCEncNodeConfigBaseKeys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype="), 20);
    switch (PVAVCEncNodeConfigBaseKeys[aIndex].iValueType)
    {
        case PVMI_KVPVALTYPE_BITARRAY32:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BITARRAY32_STRING), oscl_strlen(PVMI_KVPVALTYPE_BITARRAY32_STRING));
            break;

        case PVMI_KVPVALTYPE_KSV:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_KSV_STRING), oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
            break;

        case PVMI_KVPVALTYPE_BOOL:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
            break;

        case PVMI_KVPVALTYPE_INT32:
            if (PVMI_KVPATTR_CUR == aReqattr)
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_INT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            break;
        case PVMI_KVPVALTYPE_UINT32:
        default:
            if (PVMI_KVPATTR_CAP == aReqattr)
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            else
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
            }
            break;
    }
    aParameters[0].key[PVAVCENCNODECONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case ENCODING_MODE:	// "encoding_mode"
            if (PVMI_KVPATTR_CUR == aReqattr)
            {
                // Return current value
                aParameters[0].value.uint32_value = iEncodeParam.iEncMode;
            }
            else if (PVMI_KVPATTR_DEF == aReqattr)
            {
                // Return default
                aParameters[0].value.uint32_value = EAVCEI_ENCMODE_RECORDER;//default setting
            }
            else
            {
                // Return capability
            }
            break;

        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::GetConfigParameter() Invalid index to AVC enc node parameter"));
            return PVMFErrNotSupported;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::GetConfigParameter() Out"));
    return PVMFSuccess;
}

PVMFStatus PVMFAvcEncNode::VerifyAndSetConfigParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::VerifyAndSetConfigParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (PVMI_KVPVALTYPE_UNKNOWN == keyvaltype)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::VerifyAndSetConfigParameter() Valtype in key string unknown"));
        return PVMFErrNotSupported;
    }
    // Retrieve the fourth component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(3, aParameter.key, compstr);

    int32 avcenccomp4ind = 0;
    for (avcenccomp4ind = 0; avcenccomp4ind < PVAVCENCNODECONFIG_BASE_NUMKEYS; ++avcenccomp4ind)
    {
        // Go through each component string at 4th level
        if (pv_mime_strcmp(compstr, (char*)(PVAVCEncNodeConfigBaseKeys[avcenccomp4ind].iString)) >= 0)
        {
            // Break out of the for loop
            break;
        }
    }

    if (PVAVCENCNODECONFIG_BASE_NUMKEYS <= avcenccomp4ind)
    {
        // Match couldn't be found or non-leaf node specified
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::VerifyAndSetConfigParameter() Unsupported key or non-leaf node"));
        return PVMFErrNotSupported;
    }

    // Verify the valtype
    if (keyvaltype != PVAVCEncNodeConfigBaseKeys[avcenccomp4ind].iValueType)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::VerifyAndSetConfigParameter() Valtype does not match for key"));
        return PVMFErrNotSupported;
    }

    switch (avcenccomp4ind)
    {
        case ENCODING_MODE: // "encoding_mode"
            // change the parameter
            if (aSetParam)
            {
                iEncodeParam.iEncMode = (TAVCEIEncodingMode)aParameter.value.uint32_value;
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::VerifyAndSetConfigParameter() Invalid index for AVC enc node parameter"));
            return PVMFErrNotSupported;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::VerifyAndSetConfigParameter() Out"));
    return PVMFSuccess;
}

void PVMFAvcEncNode::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp*& aRetKVP)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::setParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    if (NULL == aParameters || aNumElements < 1)
    {
        if (aParameters)
        {
            aRetKVP = aParameters;
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::setParametersSync() Passed in parameter invalid"));
        return;
    }

    // Go through each parameter
    for (int32 paramind = 0; paramind < aNumElements; ++paramind)
    {
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(aParameters[paramind].key);
        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/avc/encoder")) < 0) || compcount < 3)
        {
            // First 3 components should be "x-pvmf/avc/encoder" and there must
            // be at least 3 components
            aRetKVP = &aParameters[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::setParametersSync() Unsupported key"));
            return;
        }

        if (4 == compcount)
        {
            // Verify and set the passed-in AVC enc node setting
            PVMFStatus retval = VerifyAndSetConfigParameter(aParameters[paramind], true);
            if (PVMFSuccess != retval)
            {
                aRetKVP = &aParameters[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::setParametersSync() Setting parameter %d failed", paramind));
                return;
            }
        }

        else
        {
            // Do not support more than 4 components right now
            aRetKVP = &aParameters[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::setParametersSync() Unsupported key"));
            return;
        }
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::setParametersSync() Out"));
}


PVMFStatus PVMFAvcEncNode::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::getParametersSync()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);

    // Initialize the output parameters
    aNumParamElements = 0;
    aParameters = NULL;

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aIdentifier);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aIdentifier, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/avc/encoder")) < 0) || compcount < 4)
    {
        // First 3 components should be "x-pvmf/avc/encoder" and there must
        // be at least 3 components
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::getParametersSync() Invalid key string"));
        return PVMFErrNotSupported;
    }

    // Retrieve the fourth component from the key string
    pv_mime_string_extract_type(3, aIdentifier, compstr);

    for (int32 avcenccomp4ind = 0; avcenccomp4ind < PVAVCENCNODECONFIG_BASE_NUMKEYS; ++avcenccomp4ind)
    {
        // Go through each AVC enc component string at 4th level
        if (pv_mime_strcmp(compstr, (char*)(PVAVCEncNodeConfigBaseKeys[avcenccomp4ind].iString)) >= 0)
        {
            if (4 == compcount)
            {
                // Determine what is requested
                PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
                if (PVMI_KVPATTR_UNKNOWN == reqattr)
                {
                    reqattr = PVMI_KVPATTR_CUR;
                }

                // Return the requested info
                PVMFStatus retval = GetConfigParameter(aParameters, aNumParamElements, avcenccomp4ind, reqattr);
                if (PVMFSuccess != retval)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::getParametersSync() Retrieving AVC enc node parameter failed"));
                    return retval;
                }
            }
            else
            {
                // Right now avcenc node doesn't support more than 4 components
                // for this sub-key string so error out
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::getParametersSync() Unsupported key"));
                return PVMFErrNotSupported;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::getParametersSync() Out"));
    if (aNumParamElements == 0)
    {
        // If no one could get the parameter, return error
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::getParametersSync() Unsupported key"));
        return PVMFFailure;
    }
    else
    {
        return PVMFSuccess;
    }
}

PVMFStatus PVMFAvcEncNode::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::releaseParameters()"));
    OSCL_UNUSED_ARG(aSession);

    if (aParameters == NULL || aNumElements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::releaseParameters() KVP list is NULL or number of elements is 0"));
        return PVMFErrArgument;
    }

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aParameters[0].key);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aParameters[0].key, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/avc/encoder")) < 0) || compcount < 3)
    {
        // First 3 component should be "x-pvmf/avc/encoder" and there must
        // be at least three components
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::releaseParameters() Unsupported key"));
        return PVMFErrNotSupported;
    }

    // Retrieve the third component from the key string
    pv_mime_string_extract_type(2, aParameters[0].key, compstr);

    // Go through each KVP and release memory for value if allocated from heap
    for (int32 ii = 0; ii < aNumElements; ++ii)
    {
        // Next check if it is a value type that allocated memory
        PvmiKvpType kvptype = GetTypeFromKeyString(aParameters[ii].key);
        if (PVMI_KVPTYPE_VALUE == kvptype || PVMI_KVPTYPE_UNKNOWN == kvptype)
        {
            PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameters[ii].key);
            if (PVMI_KVPVALTYPE_UNKNOWN == keyvaltype)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::releaseParameters() Valtype not specified in key string"));
                return PVMFErrNotSupported;
            }

            if (PVMI_KVPVALTYPE_CHARPTR == keyvaltype && NULL != aParameters[ii].value.pChar_value)
            {
                oscl_free(aParameters[ii].value.pChar_value);
                aParameters[ii].value.pChar_value = NULL;
            }
            else if (keyvaltype == PVMI_KVPVALTYPE_KSV && NULL != aParameters[ii].value.key_specific_value)
            {
                oscl_free(aParameters[ii].value.key_specific_value);
                aParameters[ii].value.key_specific_value = NULL;
            }
            else if (PVMI_KVPVALTYPE_RANGE_UINT32 == keyvaltype && NULL != aParameters[ii].value.key_specific_value)
            {
                range_uint32* rui32 = (range_uint32*)aParameters[ii].value.key_specific_value;
                aParameters[ii].value.key_specific_value = NULL;
                oscl_free(rui32);
            }
            // TODO Add more types if AVC enc node starts returning more types
        }
    }

    // AVC enc node allocated its key strings in one chunk so just free the first key string ptr
    oscl_free(aParameters[0].key);

    // Free memory for the parameter list
    oscl_free(aParameters);
    aParameters = NULL;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::releaseParameters() Out"));
    return PVMFSuccess;
}


PVMFStatus PVMFAvcEncNode::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::verifyParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    if (NULL == aParameters || aNumElements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::verifyParametersSync() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Go through each parameter
    for (int32 paramind = 0; paramind < aNumElements; ++paramind)
    {
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(aParameters[paramind].key);
        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf/avc/encoder")) < 0) || compcount < 3)
        {
            // First 3 components should be "x-pvmf/avc/encoder" and there must
            // be at least 3 components
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::verifyParametersSync() Unsupported key"));
            return PVMFErrNotSupported;
        }

        if (4 == compcount)
        {
            // Verify and set the passed-in avc enc node setting
            PVMFStatus retval = VerifyAndSetConfigParameter(aParameters[paramind], false);
            if (retval != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::verifyParametersSync() Setting parameter %d failed", paramind));
                return retval;
            }
        }
        else
        {
            // Do not support more than 4 components right now
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFAvcEncNode::verifyParametersSync() Unsupported key"));
            return PVMFErrNotSupported;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVMFAvcEncNode::verifyParametersSync() Out"));
    return PVMFSuccess;

}

uint32 PVMFAvcEncNode::getCapabilityMetric(PvmiMIOSession aSession)
{
    OSCL_UNUSED_ARG(aSession);
    return 0;
}

//PvmiCapConfigInterface
void PVMFAvcEncNode::createContext(PvmiMIOSession aSession,
                                   PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFAvcEncNode::setContextParameters(PvmiMIOSession aSession,
        PvmiCapabilityContext& aContext,
        PvmiKvp* aParameters,
        int aNumElements)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumElements);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

void PVMFAvcEncNode::DeleteContext(PvmiMIOSession aSession,
                                   PvmiCapabilityContext& aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // not supported
    OSCL_LEAVE(PVMFErrNotSupported);
}

PVMFCommandId PVMFAvcEncNode::setParametersAsync(PvmiMIOSession aSession,
        PvmiKvp* aParameters,
        int aNumElements,
        PvmiKvp*& aRet_kvp,
        OsclAny* aContext)
{
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumElements);
    OSCL_UNUSED_ARG(aRet_kvp);
//	PVMFAVCEncNodeCommand cmd;
//	cmd.PVMFMP4FFParserNodeCommand::Construct(NULL, PVMF_MP4_PARSER_NODE_CAPCONFIG_SETPARAMS, aSession, aParameters, num_elements, aRet_kvp, context);
//	return QueueCommandL(cmd);
    return 0;
}

void PVMFAvcEncNode::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    ciObserver = aObserver;
}

