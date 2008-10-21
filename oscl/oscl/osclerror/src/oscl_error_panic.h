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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//               O S C L _ E R R O R _ P A N I C

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclerror OSCL Error
 *
 * @{
 */

/** \file oscl_error_panic.h
    \brief Defines a way to cause a panic (i.e., terminate execution)
*/


#ifndef OSCL_ERROR_PANIC_H_INCLUDED
#define OSCL_ERROR_PANIC_H_INCLUDED

#ifndef OSCL_NAMESTRING_H_INCLUDED
#include "oscl_namestring.h"
#endif

#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif

//! Use this macro to cause a Panic. It terminates the execution of the current active function
/*!
   \param oscl_panic_status is an integer panic code
   \param oscl_panic_category is a const char [] parameter that tells
   the panic category.  The panic category should be no longer than PVPANICCATLEN or else
   it may be truncated.
*/
#define OSCL_PANIC(oscl_panic_status,oscl_panic_category) OsclError::Panic(oscl_panic_status,oscl_panic_category)

//Panic Error Class

#define PVPANICCATLEN 16

class TPVErrorPanic
{
    public:
        TPVErrorPanic(): iReason(0)
        {}
        int32 iReason;
        OsclNameString<PVPANICCATLEN> iCategory;
};

#include "oscl_error_imp.h"

//! Use this macro to trap a thread Panic.
//!	It will also trap any leave.
//! A panic is a fatal error and does not provide
//! any memory cleanup.  The application should terminate
//! any panicked thread.
/*!
   \param _leave_code is an integer variable that will receive the
   leave code, or zero if no leave occurs.
   \param _panic_code is a TPVErrorPanic variable that will receive the
   panic.  If no panic occurs, the iReason member will be zero.
   \param _statements is a statement or block of statements to execute
   under the trap handler.
*/
#define OSCL_PANIC_TRAP(_leave_code,_panic,_statements) _PV_TRAP_ALL(_leave_code,_panic,_statements)



#endif

/*! @} */

