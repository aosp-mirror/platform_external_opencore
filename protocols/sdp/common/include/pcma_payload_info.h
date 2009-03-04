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
/*																			                                  */
/*	=====================================================================	*/
/*	File: pcma_payload_info.h													                        */
/*	Description:															                            */
/*																			                                  */
/*																			                                  */
/*	Rev:																	                                */
/*	Created: 04/04/06														                          */
/*	=====================================================================	*/
/*																			                                  */
/*	Revision History:														                          */
/*																			                                  */
/*	Rev:																	                                */
/*	Date:																	                                */
/*	Description:															                            */
/*																			                                  */
/* /////////////////////////////////////////////////////////////////////// */

#ifndef PCMA_PAYLOAD_INFO_H
#define PCMA_PAYLOAD_INFO_H

#include "payload_info.h"

class PcmaPayloadSpecificInfoType : public PayloadSpecificInfoTypeBase
{
    public:
        PcmaPayloadSpecificInfoType(int payload)
        {
            payloadNumber = payload;
        };
};


#endif
