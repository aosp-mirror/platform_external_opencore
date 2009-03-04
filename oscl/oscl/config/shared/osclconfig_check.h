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

#ifndef OSCLCONFIG_CHECK_H_INCLUDED
#define OSCLCONFIG_CHECK_H_INCLUDED

/*! \addtogroup osclconfig OSCL config
 *
 * @{
 */

/**
\def Make sure the basic types are defined,
either in osclconfig_limits_typedefs.h or elsewhere.
*/
typedef int8 __int8__check__;
typedef uint8 __uint8__check__;
typedef int16 __int16__check__;
typedef uint16 __uint16__check__;
typedef int32 __int32__check__;
typedef uint32 __uint32__check__;

/**
\def OSCL_ASSERT_ALWAYS macro should be set to 0 or 1.
When set to 1, OSCL_ASSERT will be compiled in release mode as well
as debug mode.
*/
#ifndef OSCL_ASSERT_ALWAYS
#error "ERROR: OSCL_ASSERT_ALWAYS has to be defined to either 1 or 0."
#endif


/**
\def OSCL_DISABLE_INLINES macro should be set to 1 if
the target compiler supports 'inline' function definitions.
Otherwise it should be set to 0.
*/
#ifndef OSCL_DISABLE_INLINES
#error "ERROR: OSCL_DISABLE_INLINES has to be defined to either 1 or 0."
#endif





/**
\def _STRLIT macro should be set to an expression to convert
a constant character string into a string literal type
appropriate for the platform.
Otherwise it should be set to 0.
*/
#ifndef _STRLIT
#error "ERROR: _STRLIT has to be defined."
#endif

/**
\def _STRLIT_CHAR macro should be set to an expression to convert
a constant character string into a char string literal type
appropriate for the platform.
Otherwise it should be set to 0.
*/
#ifndef _STRLIT_CHAR
#error "ERROR: _STRLIT_CHAR has to be defined."
#endif

/**
When OSCL_HAS_UNICODE_SUPPORT==1,
\def _STRLIT_WCHAR macro should be set to an expression to convert
a constant character string into a wchar string literal type
appropriate for the platform.
Otherwise it should be set to 0.
*/
#if !defined(_STRLIT_WCHAR)
#error "ERROR: _STRLIT_WCHAR has to be defined"
#endif

/**
When OSCL_HAS_UNICODE_SUPPORT==1,
\def OSCL_NATIVE_WCHAR_TYPE macro should be set to
the native wide character type for the platform.
Otherwise it should be set to 0.
*/
#if !defined(OSCL_NATIVE_WCHAR_TYPE)
#error "ERROR: OSCL_NATIVE_WCHAR_TYPE has to be defined."
#endif




/**
\def OSCL_INTEGERS_WORD_ALIGNED macro should be set to 1 if
the target platform requires integers to be word-aligned in memory.
Otherwise it should be set to 0.
*/
#ifndef OSCL_INTEGERS_WORD_ALIGNED
#error "ERROR: OSCL_INTEGERS_WORD_ALIGNED has to be defined to either 1 or 0."
#endif

/**
\def OSCL_BYTE_ORDER_BIG_ENDIAN macro should be set to 1 if
the target platform uses big-endian byte order in memory.
Otherwise it should be set to 0.
*/
#ifndef OSCL_BYTE_ORDER_BIG_ENDIAN
#error "ERROR: OSCL_BYTE_ORDER_BIG_ENDIAN has to be defined to either 1 or 0."
#endif

/**
\def OSCL_BYTE_ORDER_LITTLE_ENDIAN macro should be set to 1 if
the target platform uses little-endian byte order in memory.
Otherwise it should be set to 0.
*/
#ifndef OSCL_BYTE_ORDER_LITTLE_ENDIAN
#error "ERROR: OSCL_BYTE_ORDER_LITTLE_ENDIAN has to be defined to either 1 or 0."
#endif

/**
\def Either OSCL_BYTE_ORDER_BIG_ENDIAN must be set to 1
or else OSCL_BYTE_ORDER_LITTLE_ENDIAN must be set to 1.
*/
#if !(OSCL_BYTE_ORDER_BIG_ENDIAN) && !(OSCL_BYTE_ORDER_LITTLE_ENDIAN)
#error "ERROR: either OSCL_BYTE_ORDER_LITTLE_ENDIAN or else OSCL_BYTE_ORDER_BIG_ENDIAN must be 1."
#endif
#if (OSCL_BYTE_ORDER_BIG_ENDIAN) && (OSCL_BYTE_ORDER_LITTLE_ENDIAN)
#error "ERROR: either OSCL_BYTE_ORDER_LITTLE_ENDIAN or else OSCL_BYTE_ORDER_BIG_ENDIAN must be 1."
#endif


/**
Note: only one byte order mode can be defined per platform.
*/
#if (OSCL_BYTE_ORDER_LITTLE_ENDIAN) && (OSCL_BYTE_ORDER_BIG_ENDIAN)
#error "ERROR: Multiple selection for OSCL_BYTE_ORDER."
#endif




/**
\def When OSCL_HAS_NATIVE_INT64_TYPE is 1,
OSCL_NATIVE_INT64_TYPE has to be defined to the native
signed 64-bit integer type.
*/
#ifndef OSCL_NATIVE_INT64_TYPE
#error "ERROR: OSCL_NATIVE_INT64_TYPE has to be defined."
#endif

/**
\def When OSCL_HAS_NATIVE_UINT64_TYPE is 1,
OSCL_NATIVE_UINT64_TYPE has to be defined to the native
unsigned 64-bit integer type.
*/
#ifndef OSCL_NATIVE_UINT64_TYPE
#error "ERROR: OSCL_NATIVE_UINT64_TYPE has to be defined."
#endif

/**
\def When OSCL_HAS_NATIVE_INT64_TYPE is 1,
INT64(x) has to be defined to the expression for a signed
64-bit literal.
*/
#ifndef INT64
#error "ERROR: INT64(x) has to be defined."
#endif

/**
\def When OSCL_HAS_NATIVE_UINT64_TYPE is 1,
INT64(x) has to be defined to the expression for a signed
64-bit literal.
*/
#ifndef UINT64
#error "ERROR: UINT64(x) has to be defined."
#endif

/**
\def When OSCL_HAS_NATIVE_INT64_TYPE is 1,
INT64_HILO(high,low) has to be defined to an expression
to create a signed 64-bit integer from 2 32-bit integers.
*/
#ifndef INT64_HILO
#error "ERROR: INT64_HILO(high,low) has to be defined."
#endif

/**
\def When OSCL_HAS_NATIVE_UINT64_TYPE is 1,
UINT64_HILO(high,low) has to be defined to an expression
to create an unsigned 64-bit integer from 2 32-bit integers.
*/
#ifndef UINT64_HILO
#error "ERROR: UINT64_HILO(high,low) has to be defined."
#endif

#ifndef OSCL_MEMFRAG_PTR_BEFORE_LEN
/**
\def OSCL_MEMFRAG_PTR_BEFORE_LEN macro should be set to 1 if
memory fragements data structures, such as used by sendmsg
(i.e., the iovec data structures), should use ptr before length.
Otherwise it should be set to 0.
*/
#error "ERROR: OSCL_MEMFRAG_PTR_BEFORE_LEN has to be defined to either 0 or 1"
#endif


/**
\def OSCL_TLS_IS_KEYED macro should be set to 1 if
the target platform's thread local storage function requires an
input key value to uniquely identify the TLS.
If the thread local storage function does not require any key,
or thread local storage is not supported, it should be set to 0.
*/
#ifndef OSCL_TLS_IS_KEYED
#error "ERROR: OSCL_TLS_IS_KEYED has to be defined to either 1 or 0"
#endif


/**
When OSCL_TLS_IS_KEYED==1,
\def OSCL_TLS_STORE_FUNC macro must be set to an expression that will
set the TLS value and evalutes to true on success, false on failure.
The macro takes 2 input parameters (key, ptr).
*/
#if (OSCL_TLS_IS_KEYED) && !defined(OSCL_TLS_STORE_FUNC)
#error "ERROR: OSCL_TLS_STORE_FUNC has to be defined"
#endif

/**
When OSCL_TLS_IS_KEYED==1,
\def OSCL_TLS_GET_FUNC macro should be set to an expression that
returns the TLS value.
The macro takes 1 input parameter (key).
*/
#if (OSCL_TLS_IS_KEYED) && !defined(OSCL_TLS_GET_FUNC)
#error "ERROR: OSCL_TLS_GET_FUNC has to be defined"
#endif

/**
When OSCL_TLS_IS_KEYED==1,
\def OSCL_TLS_GET_FUNC macro should be set to an expression that
creates a TLS entry and evalutes to true on success, false on failure.
The macro takes 1 input parameter (key).
*/
#if (OSCL_TLS_IS_KEYED) && !defined(OSCL_TLS_KEY_CREATE_FUNC)
#error "ERROR: OSCL_TLS_KEY_CREATE_FUNC has to be defined"
#endif

/**
When OSCL_TLS_IS_KEYED==1,
\def OSCL_TLS_GET_FUNC macro should be set to an expression that
deletes a TLS entry.
The macro takes 1 input parameter (key).
*/
#if (OSCL_TLS_IS_KEYED) && !defined(OSCL_TLS_KEY_DELETE_FUNC)
#error "ERROR: OSCL_TLS_KEY_DELETE_FUNC has to be defined"
#endif

/**
When OSCL_TLS_IS_KEYED==1,
\def OSCL_TLS_THREAD_ID_FUNC macro should be set to an expression that
returns a unique thread ID.
Alternately,
\def OSCL_TLS_THREAD_ID_FUNC_EXPR macro should be set to an expression that
returns a unique thread ID and evaluates to true if successful.
The macro takes one output parameter of type TOsclTlsThreadId.
*/
#if (OSCL_TLS_IS_KEYED) && !defined(OSCL_TLS_THREAD_ID_FUNC) && !defined(OSCL_TLS_THREAD_ID_FUNC_EXPR)
#error "ERROR: Either OSCL_TLS_THREAD_ID_FUNC or OSCL_TLS_THREAD_ID_FUNC_EXPR has to be defined"
#endif

/**
When OSCL_TLS_IS_KEYED==1,
\def OSCL_TLS_THREAD_ID_EQUAL macro should be set to an expression that
compares 2 thread IDs and returns true if they are equal, false if not equal.
The macro takes 2 input paramers (id1, id2)
*/
#if (OSCL_TLS_IS_KEYED) && !defined(OSCL_TLS_THREAD_ID_EQUAL)
#error "ERROR: OSCL_TLS_THREAD_ID_EQUAL has to be defined"
#endif


/**
When OSCL_TLS_IS_KEYED==0,
\def OSCL_TLS_STORE_FUNC macro must be set to an expression that will
set the TLS value and evalutes to true on success, false on failure.
The macro takes 1 input parameter (ptr).
*/
#if ! OSCL_TLS_IS_KEYED && !defined(OSCL_TLS_STORE_FUNC)
#error "ERROR: OSCL_TLS_STORE_FUNC has to be defined"
#endif

/**
When OSCL_TLS_IS_KEYED==0,
\def OSCL_TLS_GET_FUNC macro should be set to an expression that
returns the TLS value.
*/
#if ! OSCL_TLS_IS_KEYED && !defined(OSCL_TLS_GET_FUNC)
#error "ERROR: OSCL_TLS_GET_FUNC has to be defined"
#endif

/**
type TOsclBasicLockObject should be defined as the type used as
a mutex object or handle on the target platform.  It can
be either typedef'd as a C-compilable type or can be #defined.
Examples:
typedef pthread_mutex_t TOsclBasicLockObject;
#define TOsclBasicLockObject RMutex
*/
#if !defined(TOsclBasicLockObject)
typedef TOsclBasicLockObject __verify__TOsclBasicLockObject__defined__;
#endif

/*! @} */

#endif // OSCLCONFIG_CHECK_H_INCLUDED


