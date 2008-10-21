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

//               O S C L _ E R R O R _ I M P _ J U M P S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclerror OSCL Error
 *
 * @{
 */


/** \file oscl_error_imp_jumps.h
    \brief Implemenation of Leave and Panic using Setjmp / Longjmp.
*/

#ifndef OSCL_ERROR_IMP_JUMPS_H_INCLUDED
#define OSCL_ERROR_IMP_JUMPS_H_INCLUDED

#ifndef OSCL_ERROR_TRAPCLEANUP_H_INCLUDED
#include "oscl_error_trapcleanup.h"
#endif
#ifndef OSCL_ASSERT_H_INCLUDED
#include "oscl_assert.h"
#endif

// Implemenation of Leave and Panic using Setjmp / Longjmp.

//ANSI setjmp/longjmp implementation.  This is needed on any OS
//that does not support C++ exceptions.  This is a complete implementation.

#ifndef OSCLCONFIG_ERROR_H_INCLUDED
#include "osclconfig_error.h"
#endif

#ifndef OSCL_ERROR_TRAPCLEANUP_H_INCLUDED
#include "oscl_error_trapcleanup.h"
#endif
#ifndef OSCL_DEFALLOC_H_INCLUDED
#include "oscl_defalloc.h"
#endif
#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif
#ifndef OSCL_EXECPANIC_H_INCLUDED
#include "oscl_execpanic.h"
#endif

class Oscl_DefAlloc;

//this defines the maximum depth of the jump mark stack.
#define OSCL_JUMP_MAX_JUMP_MARKS OSCL_MAX_TRAP_LEVELS


//OsclJump class
class OsclJump
{
    public:
        //for use in macros only.

        OSCL_IMPORT_REF static void StaticJump(int a);

        void Jump(int a)
        {
            if (!Top())
            {
                //Note: you can't panic here, since panic would
                //invoke this routine again.  It is not safe to return
                //either, because calling code is expecting an execution
                //end.
                OSCL_ASSERT(false);
                _OSCL_Abort();
            }
            longjmp(*Top(), a);
        }

        jmp_buf *Top()
        {
            OSCL_ASSERT(iJumpIndex >= 0);
            return &iJumpArray[iJumpIndex];
        }

        ~OsclJump()
        {
            //jump mark stack should be empty at this point.
            OSCL_ASSERT(iJumpIndex == (-1));
        }

    private:
        OsclJump(): iJumpIndex(-1) {}

        void PushMark()
        {
            if (iJumpIndex == (OSCL_JUMP_MAX_JUMP_MARKS - 1))
                PV_EXECPANIC(ETrapNoFreeSlotItem);//jump stack is full!
            else
                iJumpIndex++;
        }

        void PopMark()
        {
            if (iJumpIndex < 0)
                PV_EXECPANIC(ETrapPopUnderflow);
            else
                iJumpIndex--;
        }

        jmp_buf iJumpArray[OSCL_JUMP_MAX_JUMP_MARKS];

        //index to top of stack, or (-1) when stack is empty
        int32 iJumpIndex;

        friend class OsclErrorTrapImp;
};


//internal jump type codes.
#define internalLeave (-1)
#define internalPanic (-2)

//Leave and Panic use the OsclJump methods
#define PVError_DoLeave() OsclJump::StaticJump(internalLeave)
#define PVError_DoPanic() OsclJump::StaticJump(internalPanic)

//_PV_TRAP macro catches leaves but allows panics to bubble
//_r is leave code, _s is statements to execute.
#define _PV_TRAP(__r,__s)\
	__r=OsclErrNone;\
	{\
		OsclErrorTrapImp* __trap=OsclErrorTrapImp::Trap();\
		if(!__trap){__s;}else{\
		int __tr=setjmp(*(__trap->iJumpData->Top()));\
		if (__tr==0)\
		{__s;}\
		else if (__tr==internalLeave)\
		{__r=__trap->iLeave;}\
		else if (__tr==internalPanic)\
		{__trap->UnTrap();__trap->iJumpData->Jump(__tr);}\
		__trap->UnTrap();}\
	}

//Same as _PV_TRAP but avoids a TLS lookup.
// __trapimp is the OsclErrorTrapImp* for the calling thread.
#define _PV_TRAP_NO_TLS(__trapimp,__r,__s)\
	__r=OsclErrNone;\
	{\
		OsclErrorTrapImp* __trap=OsclErrorTrapImp::TrapNoTls(__trapimp);\
		if(!__trap){__s;}else{\
		int __tr=setjmp(*(__trap->iJumpData->Top()));\
		if (__tr==0)\
		{__s;}\
		else if (__tr==internalLeave)\
		{__r=__trap->iLeave;}\
		else if (__tr==internalPanic)\
		{__trap->UnTrap();__trap->iJumpData->Jump(__tr);}\
		__trap->UnTrap();}\
	}

//_PV_TRAP_ALL macro catches leaves and panics.
//_r is leave code, _p is TPVErrorPanic,
//_s is statements to execute.
#define _PV_TRAP_ALL(__r,__p,__s)\
	__r=OsclErrNone;\
	__p.iReason=OsclErrNone;\
	{\
		OsclErrorTrapImp* __trap=OsclErrorTrapImp::Trap();\
		if(!__trap){__s;}else{\
		int __tr=setjmp(*(__trap->iJumpData->Top()));\
		if (__tr==0)\
		{__s;}\
		else if (__tr==internalLeave)\
		{__r=__trap->iLeave;}\
		else if (__tr==internalPanic)\
		{__p.iReason=__trap->iPanic.iReason;__p.iCategory.Set(__trap->iPanic.iCategory.Str());}\
		__trap->UnTrap();}\
	}

#endif // OSCL_ERROR_IMP_JUMPS_H_INCLUDED

/*! @} */
