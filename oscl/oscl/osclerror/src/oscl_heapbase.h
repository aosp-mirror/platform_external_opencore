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

//                 O S C L _ H E A P B A S E

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclerror OSCL Error
 *
 * @{
 */


/** \file oscl_heapbase.h
    \brief OSCL Heap Base include file
*/
#ifndef OSCL_HEAPBASE_H_INCLUDED
#define OSCL_HEAPBASE_H_INCLUDED

#ifndef OSCLCONFIG_ERROR_H_INCLUDED
#include "osclconfig_error.h"
#endif

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

/**
//OsclHeapBase is used as the base for cleanup stack
//items with virtual destructor.
*/


class OsclHeapBase
{
    public:
        virtual ~OsclHeapBase() {}

    protected:
        OsclHeapBase() {}
        OsclHeapBase(const OsclHeapBase&) {}
    private:
        OsclHeapBase& operator=(const OsclHeapBase&);
        friend class PVCleanupStack;
};

/**
//OsclTrapItem may be used in the cleanup stack when
//a custom cleanup operation is needed.
*/

typedef void (*OsclTrapOperation)(OsclAny*);

class OsclTrapItem
{
    public:
        OSCL_INLINE OsclTrapItem(OsclTrapOperation anOperation);
        OSCL_INLINE OsclTrapItem(OsclTrapOperation anOperation, OsclAny* aPtr);
    private:
        OsclTrapOperation iOperation;
        OsclAny* iPtr;
        friend class OsclTrapStackItem;
        friend class OsclTrapStack;
};



#if !(OSCL_DISABLE_INLINES)
#include "oscl_heapbase.inl"
#endif

#endif //


/*! @} */
