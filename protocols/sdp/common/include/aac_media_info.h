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
/*	File: aac_mediaInfo.h													                        */
/*	Description:															                            */
/*																			                                  */
/*																			                                  */
/*	Rev:																	                                */
/*	Created: 05/24/01														                          */
/*	=====================================================================	*/
/*																			                                  */
/*	Revision History:														                          */
/*																			                                  */
/*	Rev:																	                                */
/*	Date:																	                                */
/*	Description:															                            */
/*																			                                  */
/* /////////////////////////////////////////////////////////////////////// */

#ifndef AAC_MEDIAINFO_H
#define AAC_MEDIAINFO_H

#include "sdp_memory.h"
#include "media_info.h"


// NOTE.... this information really needs to exist for only one media object...
// i.e., there is no reason to have arrays of these fields, since we have one of these
// mediaInfo elements for each media object... the trick is how to get these from the SDP
// file into each of the media elements...


// this is a structure i need to hold information used in demultiplexing
typedef struct _streamMuxConfig
{
    bool cpresent;
    unsigned int audioMuxVersion;
    unsigned int allStreamsSameTimeFraming;
    unsigned int numSubFrames;
    unsigned int frameLengthType;  // pretty sure this has to be 0 for LC or LTP
    unsigned int bufferFullness;
    bool otherDataPresent;
    unsigned int otherDataLenBits;
    bool crcCheckPresent;
    unsigned char crcCheckSum;

    unsigned int audioObjectType;
    unsigned int samplingFrequency;
    unsigned int channelConfiguration;

    // include this because the payload parser may need to deal with inline
    // audioSpecificConfigs and if it is the same as the current one, we dont
    // want to reset the decoder to the same settings..
    unsigned char * audioSpecificConfigPtr;
    unsigned int audioSpecificConfigSize;

    unsigned char ** audioSpecificConfigPtrPtr;
    unsigned int * audioSpecificConfigSizePtr;

    unsigned int parseResult;
}streamMuxConfig;


class aac_mediaInfo : public mediaInfo
{
    private:
        int profileLevelID;
        OSCL_HeapString<SDPParserAlloc> lang;
        unsigned char *audioSpecificConfigPtr;
        int audioSpecificConfigSize;
        streamMuxConfig * sMC;
        int numSampleEntries;

    public:
        aac_mediaInfo()
        {
            lang = NULL;
            audioSpecificConfigSize = 0;
            audioSpecificConfigPtr = NULL;
            sMC = (streamMuxConfig *) oscl_calloc(1, sizeof(streamMuxConfig));
            if (sMC != NULL)
            {
                sMC->audioSpecificConfigSize = 0;
                sMC->audioSpecificConfigPtr = NULL;
                sMC->audioSpecificConfigPtrPtr = &(sMC->audioSpecificConfigPtr);
                sMC->audioSpecificConfigSizePtr = &(sMC->audioSpecificConfigSize);
            }
            profileLevelID = -1;
        };
        aac_mediaInfo(const aac_mediaInfo & pSource) : mediaInfo(pSource)
        {
            setLang(pSource.lang);
            setProfileLevelID(pSource.profileLevelID);
            sMC = (streamMuxConfig *) oscl_calloc(1, sizeof(streamMuxConfig));

            if (sMC != NULL)
            {

                sMC->audioMuxVersion = pSource.sMC->audioMuxVersion;
                sMC->allStreamsSameTimeFraming = pSource.sMC->allStreamsSameTimeFraming;
                sMC->numSubFrames = pSource.sMC->numSubFrames;
                sMC->frameLengthType = pSource.sMC->frameLengthType;
                sMC->bufferFullness = pSource.sMC->bufferFullness;
                sMC->otherDataPresent = pSource.sMC->otherDataPresent;
                sMC->otherDataLenBits = pSource.sMC->otherDataLenBits;
                sMC->crcCheckPresent = pSource.sMC->crcCheckPresent;
                sMC->crcCheckSum = pSource.sMC->crcCheckSum;
                sMC->audioSpecificConfigSize = pSource.sMC->audioSpecificConfigSize;
                if (sMC->audioSpecificConfigSize != 0)
                {
                    sMC->audioSpecificConfigPtr = (unsigned char*) oscl_calloc(pSource.sMC->audioSpecificConfigSize, sizeof(unsigned char));
                    if (sMC->audioSpecificConfigPtr != NULL)
                    {
                        oscl_memcpy(sMC->audioSpecificConfigPtr, pSource.sMC->audioSpecificConfigPtr, pSource.sMC->audioSpecificConfigSize);
                    }
                }

                sMC->audioSpecificConfigPtrPtr = &(sMC->audioSpecificConfigPtr);
                sMC->audioSpecificConfigSizePtr = &(sMC->audioSpecificConfigSize);
                audioSpecificConfigSize = pSource.audioSpecificConfigSize;
                audioSpecificConfigPtr = sMC->audioSpecificConfigPtr;

            }
        }

        ~aac_mediaInfo()
        {
            if (sMC != NULL)
            {
                if (sMC->audioSpecificConfigPtr != NULL)
                {
                    oscl_free(sMC->audioSpecificConfigPtr);
                    sMC->audioSpecificConfigPtr = audioSpecificConfigPtr = NULL;
                }
                oscl_free(sMC);
                sMC = NULL;
            }
            if (audioSpecificConfigPtr != NULL)
            {
                oscl_free(audioSpecificConfigPtr);
                audioSpecificConfigPtr = NULL;
            }
        };
        inline void setLang(char* language)
        {
            lang = language;
        };
        inline void setLang(const OSCL_HeapString<SDPParserAlloc>& language)
        {
            lang = language;
        };
        inline void setLang(const OsclMemoryFragment memFrag)
        {
            lang.set((const char*)(memFrag.ptr), memFrag.len);
        };
        inline void setProfileLevelID(int pID)
        {
            profileLevelID = pID;
        };

        inline void setAudioSpecificConfig(unsigned char* ASCPtr, int ASCLen)
        {
            audioSpecificConfigPtr = ASCPtr;
            audioSpecificConfigSize = ASCLen;
            sMC->audioSpecificConfigPtr = audioSpecificConfigPtr;
            sMC->audioSpecificConfigSize = audioSpecificConfigSize;
        };
        inline void setNumSampleEntries(int inNumSampleEntries)
        {
            numSampleEntries = inNumSampleEntries;
        };


        inline const char *getLang()
        {
            return lang.get_cstr();
        };
        inline int getProfileLevelID()
        {
            return profileLevelID;
        };

        inline const unsigned char *getAudioSpecificConfig(int*size)
        {
            *size = *(sMC->audioSpecificConfigSizePtr);
            return *(sMC->audioSpecificConfigPtrPtr);
        };
        inline const void *getStreamMuxConfig()
        {
            return sMC;
        };
        inline int getNumSampleEntries()
        {
            return numSampleEntries;
        };

};
#endif
