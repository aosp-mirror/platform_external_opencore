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
#include "oscl_assert.h"
#include "osclconfig.h"

#if (OSCL_DISABLE_INLINES)
#include "oscl_assert.inl"
#endif

OSCL_EXPORT_REF void OSCL_Assert(const char *expression, const char *filename, int line_number)
{
    //log the assertion...


    //default
    fprintf(stderr, "Assertion failure for: %s, at %s:%d\n", expression, filename, line_number);


    // ...then abort
    _OSCL_Abort();
}
