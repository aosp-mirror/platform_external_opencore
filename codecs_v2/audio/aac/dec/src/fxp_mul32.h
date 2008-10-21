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
/*
------------------------------------------------------------------------------



 Pathname: ./c/include/fxp_mul32.h

     Date: 03/14/2004

------------------------------------------------------------------------------
 REVISION HISTORY

 Description:
------------------------------------------------------------------------------
 INCLUDE DESCRIPTION

------------------------------------------------------------------------------
*/

#ifndef FXP_MUL32
#define FXP_MUL32


#if defined(_ARM)

#include "fxp_mul32_arm_v5.h"

#endif

#if defined(C_EQUIVALENT)

#include "fxp_mul32_c_equivalent.h"

#endif

#if defined(_ARM_V4)

#include "fxp_mul32_arm_v4.h"

#endif

#if defined(_MSC_EVC)

#include "fxp_mul32_c_msc_evc.h"

#endif


#if defined(_ARM_GCC)

#include "fxp_mul32_arm_gcc.h"

#endif

#if defined(_ARM_V4_GCC)

#include "fxp_mul32_arm_v4_gcc.h"

#endif

#endif   /*  FXP_MUL32  */

