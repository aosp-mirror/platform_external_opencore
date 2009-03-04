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
#include "pvmi_kvp_util.h"

#include "pv_mime_string_utils.h"

#include "oscl_string_containers.h"



OSCL_EXPORT_REF PvmiKvpType GetTypeFromKeyString(PvmiKeyType aKeyString)
{
    if (aKeyString == NULL)
    {
        return PVMI_KVPTYPE_UNKNOWN;
    }

    // Determine the type
    char* paramstr = NULL;
    OSCL_StackString<24> typestr;
    OSCL_StackString<10> basestr(PVMI_KVPTYPE_STRING_CONSTCHAR);

    // value
    typestr = basestr;
    typestr += PVMI_KVPTYPE_VALUE_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, typestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPTYPE_VALUE;
    }

    // pointer
    typestr = basestr;
    typestr += PVMI_KVPTYPE_POINTER_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, typestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPTYPE_POINTER;
    }

    // aggregate
    typestr = basestr;
    typestr += PVMI_KVPTYPE_AGGREGATE_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, typestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPTYPE_AGGREGATE;
    }

    return PVMI_KVPTYPE_UNKNOWN;
}


OSCL_EXPORT_REF PvmiKvpAttr GetAttrTypeFromKeyString(PvmiKeyType aKeyString)
{
    if (aKeyString == NULL)
    {
        return PVMI_KVPATTR_UNKNOWN;
    }

    // Determine the attribute type
    char* paramstr = NULL;
    OSCL_StackString<16> attrstr;
    OSCL_StackString<8> basestr(PVMI_KVPATTR_STRING_CONSTCHAR);

    // cap
    attrstr = basestr;
    attrstr += PVMI_KVPATTR_CAP_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, attrstr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPATTR_CAP;
    }
    // def
    attrstr = basestr;
    attrstr += PVMI_KVPATTR_DEF_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, attrstr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPATTR_DEF;
    }
    // cur
    attrstr = basestr;
    attrstr += PVMI_KVPATTR_CUR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, attrstr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPATTR_CUR;
    }

    return PVMI_KVPATTR_UNKNOWN;
}


OSCL_EXPORT_REF PvmiKvpValueType GetValTypeFromKeyString(PvmiKeyType aKeyString)
{
    if (aKeyString == NULL)
    {
        return PVMI_KVPVALTYPE_UNKNOWN;
    }

    // Determine the valtype
    char* paramstr = NULL;
    OSCL_StackString<64> valtypestr;
    OSCL_StackString<10> basestr(PVMI_KVPVALTYPE_STRING_CONSTCHAR);

    // bool
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_BOOL;
    }
    // float
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_FLOAT_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_FLOAT;
    }
    // double
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_DOUBLE_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_DOUBLE;
    }
    // uint8
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_UINT8_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_UINT8;
    }
    // int32
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_INT32_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_INT32;
    }
    // uint32
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_UINT32;
    }
    // int64
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_INT64_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_INT32;
    }
    // uint64
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_UINT64_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_UINT32;
    }
    // wchar*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_WCHARPTR;
    }
    // char*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_CHARPTR;
    }
    // uint8*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_UINT8PTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_UINT8PTR;
    }
    // int32*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_INT32PTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_INT32PTR;
    }
    // uint32*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_UINT32PTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_UINT32PTR;
    }
    // int64*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_INT64PTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_INT64PTR;
    }
    // uint64*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_UINT64PTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_UINT64PTR;
    }
    // float*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_FLOATPTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_FLOATPTR;
    }
    // double*
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_DOUBLEPTR_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_DOUBLEPTR;
    }
    // ksv
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_KSV_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_KSV;
    }
    // pKvp
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_PKVP_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_PKVP;
    }
    // ppKvp
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_PPKVP_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_PPKVP;
    }
    // range_float
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_FLOAT_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_FLOAT;
    }
    // range_double
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_DOUBLE_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_DOUBLE;
    }
    // range_uint8
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_UINT8_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_UINT8;
    }
    // range_int32
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_INT32_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_INT32;
    }
    // range_uint32
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_UINT32_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_UINT32;
    }
    // range_int64
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_INT64_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_INT64;
    }
    // range_uint64
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_RANGE_UINT64_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_RANGE_UINT64;
    }
    // bitarray32
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_BITARRAY32_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_BITARRAY32;
    }
    // bitarray64
    valtypestr = basestr;
    valtypestr += PVMI_KVPVALTYPE_BITARRAY64_STRING_CONSTCHAR;
    if (pv_mime_string_parse_param(aKeyString, valtypestr.get_str(), paramstr) > 0)
    {
        return PVMI_KVPVALTYPE_BITARRAY64;
    }

    return PVMI_KVPVALTYPE_UNKNOWN;
}

OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForWStringValue(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        OSCL_wString& aValString,
        char* aMiscKeyParam,
        uint32 aMaxSize,
        uint32 aTruncateFlag)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL || aValString.get_size() == 0)
    {
        return PVMFErrArgument;
    }

    aKeyVal.value.pWChar_value = NULL;
    aKeyVal.key = NULL;

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR) + 1; // for "wchar*" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }
    uint32 valuelen = aValString.get_size() + 1;

// Allocate memory for the strings
    int32 leavecode = 0, leavecode1 = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen););

    if (aTruncateFlag)
    {
        if (aMaxSize < valuelen)
        {
            valuelen = aMaxSize + 1;
        }
    }
    if (aMaxSize >= valuelen)
    {
        OSCL_TRY(leavecode,
                 aKeyVal.value.pWChar_value = OSCL_ARRAY_NEW(oscl_wchar, valuelen););
    }

    if (leavecode == 0 && leavecode1 == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMI_KVP_SEMICOLON_STRING_CONSTCHAR, oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_WCHARPTR_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = '\0';
        // Copy the value
        if (aKeyVal.value.pWChar_value != NULL)
        {
            oscl_strncpy(aKeyVal.value.pWChar_value, aValString.get_cstr(), valuelen);
            aKeyVal.value.pWChar_value[valuelen-1] = '\0';
        }
        // Set the length and capacity
        aKeyVal.length = valuelen;
        aKeyVal.capacity = valuelen;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }
        if (aKeyVal.value.pWChar_value)
        {
            OSCL_ARRAY_DELETE(aKeyVal.value.pWChar_value);
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}


OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForCharStringValue(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        const char* aValString,
        char* aMiscKeyParam,
        uint32 aMaxSize,
        uint32 aTruncateFlag)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    aKeyVal.value.pChar_value = NULL;
    aKeyVal.key = NULL;

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR) + 1; // for "char*" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    uint32 valuelen = oscl_strlen(aValString) + 1;

    // Allocate memory for the strings
    int32 leavecode = 0, leavecode1 = 0;

    OSCL_TRY(leavecode, aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);)

    if (aTruncateFlag)
    {
        if (aMaxSize < valuelen)
        {
            valuelen = aMaxSize + 1;
        }

    }
    if (aMaxSize >= valuelen)
        OSCL_TRY(leavecode1, aKeyVal.value.pChar_value = OSCL_ARRAY_NEW(char, valuelen););

    if (leavecode == 0 && leavecode1 == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMI_KVP_SEMICOLON_STRING_CONSTCHAR, oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = '\0';
        // Copy the value
        if (aKeyVal.value.pChar_value != NULL)
        {
            oscl_strncpy(aKeyVal.value.pChar_value, aValString, valuelen);
            aKeyVal.value.pChar_value[valuelen-1] = '\0';
        }
        // Set the length and capacity
        aKeyVal.length = valuelen;
        aKeyVal.capacity = valuelen;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }
        if (aKeyVal.value.pChar_value)
        {
            OSCL_ARRAY_DELETE(aKeyVal.value.pChar_value);
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}

OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForByteArrayValue(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        uint8* aValue,
        uint32 aValueLen,
        char* aMiscKeyParam,
        uint32 aMaxSize)
{
    OSCL_UNUSED_ARG(aMaxSize);
    /* Check parameters */
    if (aKeyVal.key != NULL || aKeyTypeString == NULL || aValueLen == 0)
    {
        return PVMFErrArgument;
    }

    /* Determine the length of strings */
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_UINT8PTR_STRING_CONSTCHAR) + 1; // for "char*" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    /* Allocate memory for the strings */
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
             aKeyVal.value.pUint8_value = OSCL_ARRAY_NEW(uint8, aValueLen);
            );

    if (leavecode == 0)
    {
        /* Copy the key string */
        oscl_strncpy(aKeyVal.key,
                     aKeyTypeString,
                     oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key,
                     PVMI_KVP_SEMICOLON_STRING_CONSTCHAR,
                     oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key,
                     PVMI_KVPVALTYPE_STRING_CONSTCHAR,
                     oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key,
                     PVMI_KVPVALTYPE_UINT8PTR_STRING,
                     oscl_strlen(PVMI_KVPVALTYPE_UINT8PTR_STRING));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = NULL_TERM_CHAR;
        /* Copy the value */
        oscl_memcpy(aKeyVal.value.pUint8_value, aValue, aValueLen);
        aKeyVal.length   = aValueLen;
        aKeyVal.capacity = aValueLen;
    }
    else
    {
        /* Memory allocation failed so clean up */
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }
        if (aKeyVal.value.pUint8_value)
        {
            OSCL_ARRAY_DELETE(aKeyVal.value.pUint8_value);
        }
        return PVMFErrNoMemory;
    }
    return PVMFSuccess;
}

OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForUInt32Value(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        uint32& aValueUInt32,
        char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR) + 1; // for "uint32" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMI_KVP_SEMICOLON_STRING_CONSTCHAR, oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = '\0';
        // Copy the value
        aKeyVal.value.uint32_value = aValueUInt32;
        // Set the length and capacity
        aKeyVal.length = 1;
        aKeyVal.capacity = 1;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}


OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForFloatValue(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        float& aValueFloat,
        char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_FLOAT_STRING_CONSTCHAR) + 1; // for "float" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMI_KVP_SEMICOLON_STRING_CONSTCHAR, oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_FLOAT_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_FLOAT_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = '\0';
        // Copy the value
        aKeyVal.value.float_value = aValueFloat;
        // Set length and capacity
        aKeyVal.length = 1;
        aKeyVal.capacity = 1;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}


OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForBoolValue(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        bool& aValueBool,
        char* aMiscKeyParam)
{
    // Check parameters
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    // Determine the length of strings
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR) + 1; // for "bool" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    // Allocate memory for the strings
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
            );

    if (leavecode == 0)
    {
        // Copy the key string
        oscl_strncpy(aKeyVal.key, aKeyTypeString, oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key, PVMI_KVP_SEMICOLON_STRING_CONSTCHAR, oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key, PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR, oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING_CONSTCHAR));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = '\0';
        // Copy the value
        aKeyVal.value.bool_value = aValueBool;
        // Set length and capacity
        aKeyVal.length = 1;
        aKeyVal.capacity = 1;
    }
    else
    {
        // Memory allocation failed so clean up
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }

        return PVMFErrNoMemory;
    }

    return PVMFSuccess;
}

OSCL_EXPORT_REF PVMFStatus PVMFCreateKVPUtils::CreateKVPForKSVValue(PvmiKvp& aKeyVal,
        const char* aKeyTypeString,
        OsclAny* aValue,
        char* aMiscKeyParam)
{
    /* Check parameters */
    if (aKeyVal.key != NULL || aKeyTypeString == NULL)
    {
        return PVMFErrArgument;
    }

    /* Determine the length of strings */
    uint32 keylen = oscl_strlen(aKeyTypeString) + 1; // for key string and ";"
    keylen += oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR); // for "valtype="
    keylen += oscl_strlen(PVMI_KVPVALTYPE_UINT8PTR_STRING_CONSTCHAR) + 1; // for "char*" and NULL terminator
    if (aMiscKeyParam)
    {
        keylen += oscl_strlen(aMiscKeyParam);
    }

    /* Allocate memory for the strings */
    int32 leavecode = 0;
    OSCL_TRY(leavecode,
             aKeyVal.key = OSCL_ARRAY_NEW(char, keylen);
             aKeyVal.value.key_specific_value = NULL;
            );

    if (leavecode == 0)
    {
        /* Copy the key string */
        oscl_strncpy(aKeyVal.key,
                     aKeyTypeString,
                     oscl_strlen(aKeyTypeString) + 1);
        oscl_strncat(aKeyVal.key,
                     PVMI_KVP_SEMICOLON_STRING_CONSTCHAR,
                     oscl_strlen(PVMI_KVP_SEMICOLON_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key,
                     PVMI_KVPVALTYPE_STRING_CONSTCHAR,
                     oscl_strlen(PVMI_KVPVALTYPE_STRING_CONSTCHAR));
        oscl_strncat(aKeyVal.key,
                     PVMI_KVPVALTYPE_KSV_STRING,
                     oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
        if (aMiscKeyParam)
        {
            oscl_strncat(aKeyVal.key, aMiscKeyParam, oscl_strlen(aMiscKeyParam));
        }
        aKeyVal.key[keylen-1] = '\0';
        /* Copy the value */
        aKeyVal.value.key_specific_value = aValue;
    }
    else
    {
        /* Memory allocation failed so clean up */
        if (aKeyVal.key)
        {
            OSCL_ARRAY_DELETE(aKeyVal.key);
            aKeyVal.key = NULL;
        }
        return PVMFErrNoMemory;
    }
    return PVMFSuccess;
}





