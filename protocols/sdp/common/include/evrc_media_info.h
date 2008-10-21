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
/*																			*/
/*	=====================================================================	*/
/*	File: evrc_mediaInfo.h													*/
/*	Description:															*/
/*																			*/
/*																			*/
/*	Rev:																	*/
/*	Created: 05/24/01														*/
/*	=====================================================================	*/
/*																			*/
/*	Revision History:														*/
/*																			*/
/*	Rev:																	*/
/*	Date:																	*/
/*	Description:															*/
/*																			*/
/* //////////////////////////////////////////////////////////////////////// */

#ifndef EVRC_MEDIAINFO_H
#define EVRC_MEDIAINFO_H

#include "sdp_memory.h"
#include "media_info.h"

struct EVRCfmtpInfoType
{
    uint32 payloadNumber;
    int maximumFrames;
    int maximumBundle;
    int packetTime;
};

class evrc_mediaInfo : public mediaInfo
{
    private:


        OSCL_HeapString<SDPParserAlloc> lang;
    public:
        evrc_mediaInfo()
        {
            mediaInfo();
            lang = NULL;
        };

        evrc_mediaInfo(const evrc_mediaInfo& pSource) : mediaInfo(pSource)
        {

            setLang(pSource.lang);
        }

        ~evrc_mediaInfo() {};

        inline void setLang(char* lan)
        {
            lang = lan;
        };
        inline void setLang(const OSCL_HeapString<SDPParserAlloc>& lan)
        {
            lang = lan;
        };
        inline void setLang(const OsclMemoryFragment memFrag)
        {
            lang.set((const char*)(memFrag.ptr), memFrag.len);
        };

        inline const char *getLang()
        {
            return lang.get_cstr();
        };

};

#endif
