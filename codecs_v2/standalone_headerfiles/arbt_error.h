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
#ifndef ARBT_ERROR_H_INCLUDED
#define ARBT_ERROR_H_INCLUDED

#if 0

//! Use this macro to cause a Leave. It terminates the execution of the current active function
/*!
	It also tries to cleanup the items on the cleanup stack.
   \param arbt_leave_status tells the cause for the Leave
*/
#define ARBT_LEAVE(_leave_status) ArbtError::Leave(_leave_status)


//! This macro will be used to set up a try block
/*!
   The try block identifies a block of code that might throw exceptions
   (or leave)
   \param arbt_leave_status arbt_leave_status will receive the result of any
   ARBT_LEAVE (which will get called from a ARBT_THROW) on systems that do not
   support exception handling.This is unused on systems that do support
   exception handling
   \param statements is a statement or block of statements that could throw
   exceptions and will be executed in the try block
*/
#define ARBT_TRY(_leave_status,_statements) _PV_TRAP(_leave_status,_statements)

//! This section defines the macros to be used in the catch block following the try block

//! Use this macro to call a function that handles all exception types thrown in the preceding try block
/*!
   \param _leave_status
   \param _statements is a statement or block of statements that will
   catch all the exception types thrown by the preceding try block
   This is a standalone macro and should not be used with any of the macros above
*/
#define ARBT_FIRST_CATCH_ANY(_leave_status, _statements) \
   if (_leave_status!=ArbtErrNone) { _statements; }

//! Use this macro to define a block of code that catches the first exception type thrown in the preceding try block
/*!
   \param arbt_leave_status is the leave code that was returned by ARBT_THROW
   \param exceptiontype is the exception handled by this catch block
   This macro MUST be used in conjunction with either ARBT_LAST_CATCH or ARBT_CATCH_ANY
*/
#define ARBT_FIRST_CATCH( _leave_status, _catch_value, _statements) \
	if (_leave_status!=ArbtErrNone && _leave_status == _catch_value){_statements;}

//! Use this macro to define a block of code for catching additional exception types
/*!
   ARBT_FIRST_CATCH can be used to catch one exception type. Then the
   ARBT_CATCH macro can be used to catch each subsequent type. The catch
   block must end with ARBT_LAST_CATCH or ARBT_CATCH_ANY
   \param arbt_leave_status is the result of any ARBT_THROW
   \param exceptiontype is the exception handled by this catch block
*/
#define ARBT_CATCH( _leave_status, _catch_value, _statements) \
	else if (_leave_status!=ArbtErrNone && _leave_status == _catch_value){_statements;}

//! Use this macro to call a function that will catch all remaining exception types
/*!
   \param _leave_status
   \param _statements is a statement or block of statements to
   handle all remaining exception types.
   This macro ends the try block.
*/
#define ARBT_CATCH_ANY(_leave_status,_statements) \
	else if (_leave_status!=ArbtErrNone){ _statements;}

//! Use this macro if ARBT_CATCH_ANY has not been used. It will mark the end of the catch block
/*!
   \param _leave_status will be propagated up the call stack
   This macro will do an ARBT_LEAVE if the leave has not been handled by the calls above.
   This macro ends the try block.
*/

#define ARBT_LAST_CATCH(_leave_status) \
	else if (_leave_status!=ArbtErrNone){ARBT_LEAVE(_leave_status);}



#else

#define ARBT_LEAVE(x)

#endif // if 0

#endif //ARBT_ERROR_H_INCLUDED

