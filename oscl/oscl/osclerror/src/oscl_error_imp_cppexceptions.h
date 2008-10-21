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

//         O S C L _ E R R O R _ I M P _ C P P E X C E P T I O N S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclerror OSCL Error
 *
 * @{
 */


/** \file oscl_error_imp_cppexceptions.h
    \brief Implementation File for Leave and Panic using C++ exceptions.
*/

#ifndef OSCL_ERROR_IMP_CPPEXCEPTIONS_H_INCLUDED
#define OSCL_ERROR_IMP_CPPEXCEPTIONS_H_INCLUDED

#ifndef OSCL_ERROR_TRAPCLEANUP_H_INCLUDED
#include "oscl_error_trapcleanup.h"
#endif

//Implementation file for Leave and Panic using C++ exceptions.

//This is a full implementation of Leave and Panic.

class internalLeave
{
    public:
        int a;
};
class internalPanic
{
    public:
        int b;
};

//Leave and Panic throw C++ exceptions.
#define PVError_DoLeave() internalLeave __ilv;__ilv.a=0;throw(__ilv)
#define PVError_DoPanic() internalPanic __ipn;__ipn.b=0;throw(__ipn)


//_PV_TRAP catches Leaves but allows panics to bubble.
//_r is the leave code, _s are statements to execute
#define _PV_TRAP(__r,__s)\
	__r=OsclErrNone;\
	{\
		OsclErrorTrapImp* __tr=OsclErrorTrapImp::Trap();\
		if(!__tr){__s;}else{\
		try{__s;}\
		catch(internalLeave __lv)\
		{__lv.a=__r=__tr->iLeave;}\
		catch(internalPanic __pn)\
		{__tr->UnTrap();throw(__pn);}\
		__tr->UnTrap();}\
	}

//_PV_TRAP_NO_TLS catches Leaves but allows panics to bubble.
//_r is the leave code, _s are statements to execute
#define _PV_TRAP_NO_TLS(__trapimp,__r,__s)\
	__r=OsclErrNone;\
	{\
		OsclErrorTrapImp* __tr=OsclErrorTrapImp::TrapNoTls(__trapimp);\
		if(!__tr){__s;}else{\
		try{__s;}\
		catch(internalLeave __lv)\
		{__lv.a=__r=__tr->iLeave;}\
		catch(internalPanic __pn)\
		{__tr->UnTrap();throw(__pn);}\
		__tr->UnTrap();}\
	}

//_PV_TRAP_ALL catches Leaves and panics.
//_r is leave code, _p is TPVErrorPanic, _s is statements to execute.
#define _PV_TRAP_ALL(__r,__p,__s)\
	__r=OsclErrNone;\
	__p.iReason=OsclErrNone;\
	{\
		OsclErrorTrapImp* __tr=OsclErrorTrapImp::Trap();\
		if(!__tr){__s;}else{\
		try{__s;}\
		catch(internalLeave __lv)\
		{__lv.a=__r=__tr->iLeave;}\
		catch(internalPanic __pn)\
		{__pn.b=__p.iReason=__tr->iPanic.iReason;__p.iCategory.Set(__tr->iPanic.iCategory.Str());}\
		__tr->UnTrap();}\
	}


#endif // OSCL_ERROR_IMP_CPPEXCEPTIONS_H_INCLUDED

/*! @} */


