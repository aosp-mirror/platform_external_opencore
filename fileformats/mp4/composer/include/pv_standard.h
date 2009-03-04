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

#ifndef PV_STANDARD_H
#define PV_STANDARD_H


/* Type definitions */

#ifndef int8
typedef char int8;
#endif

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef int16
typedef short int16;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef int32
typedef long int32;
#endif

#ifndef uint32
typedef unsigned long uint32;
#endif

/* Architecture definitions */
/*
    Compiler information:
    PV_COMPILER_VISUAL_CPLUSPLUS_PC Visual C++ compiler for PC platform
*/


#if defined(PV_PROCESSOR_PENTIUM)
#define BYTE_ORDER_LITTLE_ENDIAN
#elif defined(PV_PROCESSOR_PENTIUM_2)
#define BYTE_ORDER_LITTLE_ENDIAN
#elif defined(PV_PROCESSOR_PENTIUM_3)
#define BYTE_ORDER_LITTLE_ENDIAN
#elif defined(PV_PROCESSOR_INTEL_80386)
#define BYTE_ORDER_LITTLE_ENDIAN
#elif defined(PV_PROCESSOR_INTEL_80486)
#define BYTE_ORDER_LITTLE_ENDIAN

#elif defined(PV_PROCESSOR_INTEL_STRONGARM)
/* CPU supports both little endian and big endian. Words must be aligned to word boundaries */
#define INTEGERS_WORD_ALIGNED
#elif defined(PV_PROCESSOR_MIPS_R4000)
/* CPU supports both little endian and big endian. Words must be aligned to word boundaries */
#define INTEGERS_WORD_ALIGNED
#elif defined(PV_PROCESSOR_HITACHI_SH3)
/* CPU supports both little endian and big endian. Words must be aligned to word boundaries */
#define INTEGERS_WORD_ALIGNED
#else
#error need to specify the target processor
#endif

#if defined(PV_OS_WINDOWS_95)
#define PV_OS_WIN32
#elif defined(PV_OS_WINDOWS_98)
#define PV_OS_WIN32
#elif defined(PV_OS_WINDOWS_2000)
#define PV_OS_WIN32
#elif defined(PV_OS_WINDOWS_NT)
#define PV_OS_WIN32
#else
#define PV_OS_WIN32
#endif

/* Use PV_OS_VERSION to specify the OS version */

#if !defined(BYTE_ORDER_LITTLE_ENDIAN) && !defined(BYTE_ORDER_BIG_ENDIAN)
#error must define either BYTE_ORDER_LITTLE_ENDIAN or BYTE_ORDER_BIG_ENDIAN
#endif


#endif
