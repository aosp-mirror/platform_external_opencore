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
/*	File: evrc_payload_info.h													                        */
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

#ifndef EVRC_PAYLOAD_INFO_H
#define EVRC_PAYLOAD_INFO_H

#include "payload_info.h"

#define PVMF_SDP_DEFAULT_EVRC_SAMPLE_RATE 8000

class EvrcPayloadSpecificInfoType : public PayloadSpecificInfoTypeBase
{
    public:
        EvrcPayloadSpecificInfoType(int payload)
        {
            payloadNumber = payload;

            evrc_maximumBundle = -1;
            evrc_maximumFrames = -1;
            evrc_packetTime = -1;
            frameWidth = -1;
            frameHeight = -1;
        };

        inline void setMaximumFrames(int mFrames)
        {
            evrc_maximumFrames = mFrames;
        };

        inline void setMaximumBundle(int mBundle)
        {
            evrc_maximumBundle = mBundle;
        };

        inline void setPacketTime(int pTime)
        {
            evrc_packetTime = pTime;
        };

        inline void setFrameWidth(int fWidth)
        {
            frameWidth = fWidth;
        };
        inline void setFrameHeight(int fHeight)
        {
            frameHeight = fHeight;
        };

        inline int getFrameWidth()
        {
            return frameWidth;
        }

        inline int getFrameHeight()
        {
            return frameHeight;
        }


    private:
        int evrc_maximumFrames;
        int evrc_maximumBundle;
        int evrc_packetTime;
        int frameWidth;
        int frameHeight;
};

#endif
