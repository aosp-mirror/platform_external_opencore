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

//           O S C L _ E R R O R _ T R A P C L E A N U P

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclerror OSCL Error
 *
 * @{
 */


/** \file oscl_error_trapcleanup.h
    \brief OSCL Error trap and cleanup implementation include file
*/

#ifndef OSCL_ERROR_TRAPCLEANUP_H_INCLUDED
#define OSCL_ERROR_TRAPCLEANUP_H_INCLUDED

#ifndef OSCLCONFIG_ERROR_H_INCLUDED
#include "osclconfig_error.h"
#endif



#ifndef OSCL_HEAPBASE_H_INCLUDED
#include "oscl_heapbase.h"
#endif

#define OSCL_MAX_TRAP_LEVELS 20

/**
//Internal cleanup stack item type.
*/
class OsclTrapStackItem
{
    public:
        OsclTrapStackItem() {}
        OsclTrapStackItem(OsclHeapBase *aCBase)
        {
            iCBase = aCBase;
            iTAny = (OsclAny*)aCBase;
            iTrapOperation = NULL;
            iNext = NULL;
        }
        OsclTrapStackItem(OsclAny *aTAny)
        {
            iCBase = NULL;
            iTAny = aTAny;
            iTrapOperation = NULL;
            iNext = NULL;
        }
        OsclTrapStackItem(OsclTrapItem aItem)
        {
            iCBase = NULL;
            iTAny = aItem.iPtr;
            iTrapOperation = aItem.iOperation;
            iNext = NULL;
        }
        OsclHeapBase *iCBase;
        OsclAny *iTAny;
        OsclTrapOperation iTrapOperation;
        OsclTrapStackItem *iNext;
};

class OsclJump;

#ifndef OSCL_ERROR_IMP_H_INCLUDED
#include "oscl_error_imp.h"
#endif

#ifndef OSCL_DEFALLOC_H_INCLUDED
#include "oscl_defalloc.h"
#endif

#ifndef OSCL_ASSERT_H_INCLUDED
#include "oscl_assert.h"
#endif

#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif

/**
//A common type for cleanup stack and trap mark stack.
//for internal use only.
*/
class OsclTrapStack
{
    private:
        OsclTrapStack(Oscl_DefAlloc *iAlloc);
        ~OsclTrapStack();

        //Trap APIs
        inline void Trap();
        inline bool UnTrap();

        //Cleanup stack APIs
        void PushL(OsclHeapBase *aCBase);
        void PushL(OsclAny *aTAny);
        void PushL(OsclTrapItem anItem);
        void Push(OsclTrapStackItem *aItem);
        void Pop();
        void Pop(int32 aCount);
        void PopDealloc();
        void PopDealloc(int32 aCount);
        void Leaving();

        //top of cleanup stack
        OsclTrapStackItem *iTop;

        //cleanup stack allocator.
        Oscl_DefAlloc *iAlloc;

        friend class OsclError;
        friend class OsclErrorTrap;
        friend class OsclErrorTrapImp;

    private:
        //The trap mark stack is a stack used to mark the top of the cleanup stack
        //for each trap level.

        inline void PushTrapL(OsclAny *aTAny);
        inline void PopTrap();

        //top of trap mark stack
        OsclTrapStackItem *TrapTop()
        {
            if (iTrapTopIndex >= 0)
                return &iTrapTopArray[iTrapTopIndex];
            return NULL;
        }

        OsclTrapStackItem iTrapTopArray[OSCL_MAX_TRAP_LEVELS];

        //index to top of stack, or (-1) when stack is empty
        int32 iTrapTopIndex;

        void pushTrapIndex()
        {
            if (iTrapTopIndex == (OSCL_MAX_TRAP_LEVELS - 1))
                PV_EXECPANIC(ETrapNoFreeSlotItem);//stack is full.
            else
                iTrapTopIndex++;
        }

        void popTrapIndex()
        {
            if (iTrapTopIndex < 0)
                PV_EXECPANIC(ETrapPopUnderflow);
            else
                iTrapTopIndex--;
        }
};



#ifndef OSCL_ERROR_PANIC_H_INCLUDED
#include "oscl_error_panic.h"
#endif

#ifndef OSCL_BASE_ALLOC_H_INCLUDED
#include "oscl_base_alloc.h"
#endif

//For non-symbian, the error trap stack must be in a global registry.
//
//use TLS registry unless it's not available, then
//use singleton.
#include "oscl_tls.h"
#define PVERRORTRAP_REGISTRY_ID OSCL_TLS_ID_PVERRORTRAP
#define PVERRORTRAP_REGISTRY OsclTLSRegistry


/**
//A per-thread cleanup stack with nested trap support.
*/
class OsclErrorTrapImp
{
    public:
        /**
        //these are used in public macros, but
        //aren't intended as public methods or members.
        */
        OSCL_IMPORT_REF void UnTrap();
#if defined(PVERROR_IMP_JUMPS)
        OsclJump *iJumpData;
#endif

        //Global leave/panic info.
        TPVErrorPanic iPanic;
        int32 iLeave;

    public:
        /**
        //PV trap cleanup.  Public for use in macros only.
        */
        OSCL_IMPORT_REF static OsclErrorTrapImp* Trap();
        //This version of Trap is identical to the above, except it avoids the TLS lookup.
        OSCL_IMPORT_REF static OsclErrorTrapImp* TrapNoTls(OsclErrorTrapImp*);
        OsclTrapStack *iTrapStack;

    private:
        OsclErrorTrapImp(Oscl_DefAlloc *aAlloc, int32 &error);
        ~OsclErrorTrapImp();
        Oscl_DefAlloc *iAlloc;

        static OsclErrorTrapImp* GetErrorTrap(int32& aError)
        //static function to get currently installed error trap
        //for this thread.
        {
            OsclErrorTrapImp *current = (OsclErrorTrapImp*)PVERRORTRAP_REGISTRY::getInstance(PVERRORTRAP_REGISTRY_ID, aError);
            return current;
        }
        static OsclErrorTrapImp* GetErrorTrap()
        //static function to get currently installed error trap
        //for this thread.  panics on error.
        {
            int32 error;
            OsclErrorTrapImp* current = GetErrorTrap(error);
            if (error)
            {
                OSCL_ASSERT(0);
                return NULL;
            }
            return current;
        }

        static OsclErrorTrapImp* SetErrorTrap(OsclErrorTrapImp* a, int32& aError)
        //static function to set currently installed error trap
        //for this thread. return previous error trap, if any.
        {
            OsclErrorTrapImp* temp = GetErrorTrap(aError);
            PVERRORTRAP_REGISTRY::registerInstance(a, PVERRORTRAP_REGISTRY_ID, aError);
            return temp;
        }

        //Global cleanup function for OsclAny items.
        static void TrapOperation(OsclAny *ptr)
        {
            int32 error;
            OsclErrorTrapImp *trap = GetErrorTrap(error);
            if (trap && trap->iAlloc)
                trap->iAlloc->deallocate(ptr);
            else
            {
                _OsclBasicAllocator alloc;
                alloc.deallocate(ptr);
            }
        }

        //default allocators.
        _OsclBasicAllocator iDefAlloc;

        friend class OsclErrorTrap;
        friend class OsclError;
        friend class OsclExecScheduler;
        friend class OsclExecSchedulerCommonBase;
        friend class OsclJump;
        friend class OsclJumpMark;
        friend class OsclTrapStack;
        friend class CPVInterfaceProxy;
        friend class OsclScheduler;
};


#endif

/*! @} */
