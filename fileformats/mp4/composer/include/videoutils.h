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
/*********************************************************************************/
/*
	This PVA_FF_VideoUtils Class contains sime useful methods for operating on
	Video frames represented as IMediaSamples.
*/

#ifndef __VideoUtils_H__
#define __VideoUtils_H__

#include "oscl_types.h"
#include "oscl_base.h"

class PVA_FF_VideoUtils
{

    public:
        static uint8 getFrameModuloTimeBase(uint8 *psample);
        static uint16 getFrameVOPTimeIncrement(uint8 *psample);
        static uint32 getNumberBits(uint8 *psample, uint32 size);

};



#endif

