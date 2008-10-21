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
#ifndef __SDP_INFO_H__
#define __SDP_INFO_H__

#include "oscl_mem.h"
#include "session_info.h"
#include "media_info.h"
#include "aac_media_info.h"
#include "amr_media_info.h"
#include "evrc_media_info.h"
#include "m4v_media_info.h"
#include "rfc3640_media_info.h"
#include "h263_media_info.h"
#include "still_image_media_info.h"
#include "bool_array.h"
#include "pcmu_media_info.h"
#include "pcma_media_info.h"

#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif



//----------------------------------------------------------------------
// Global Constant Declarations
//----------------------------------------------------------------------
#define MAX_MEDIA_OBJECTS	50
#define MAX_SEGMENTS		10

struct segmentSpecific
{
    bool segmentActive;
    bool segmentPayloadOrderPref;
};

typedef BoolArray<MAX_MEDIA_OBJECTS> SDPSelectionType;
//----------------------------------------------------------------------
// Global Type Declarations
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Global Data Declarations
//----------------------------------------------------------------------

//======================================================================
//  CLASS DEFINITIONS and FUNCTION DECLARATIONS
//======================================================================
#include "mime_registry.h"

class SDPInfo
{
    public:
        SDPInfo()
        {
            registrar_locally_allocated = false;
            reg = NULL;
            numMediaObjects = 0;
            iMediaObjectIndex = 0;
            segmentCount = 0;
            for (int ii = 0; ii < MAX_SEGMENTS; ii++)
            {
                segmentInfo[ii].segmentActive = true;
                segmentInfo[ii].segmentPayloadOrderPref = false;
            }
        }

        SDPInfo(const SDPInfo &sourceSdpInfo)
        {
            registrar_locally_allocated = false;
            reg = NULL;
            numMediaObjects = 0;
            iMediaObjectIndex = 0;
            segmentCount = sourceSdpInfo.segmentCount;
            for (int jj = 0; jj < MAX_SEGMENTS; jj++)
            {
                segmentInfo[jj] = sourceSdpInfo.segmentInfo[jj];
            }

            session_info = sourceSdpInfo.session_info;
            bool alternateMedia;
            for (int ii = 0; ii < sourceSdpInfo.numMediaObjects; ii++)
            {
                for (int ss = 0; ss < (int)pMediaInfo[ii].size(); ss++)
                {
                    const char *MIMEType = (sourceSdpInfo.pMediaInfo[ii][ss])->getMIMEType();
                    if (!oscl_strncmp(MIMEType, "AAC", oscl_strlen("AAC")) || !oscl_strncmp(MIMEType, "MP4A-LATM", oscl_strlen("MP4A-LATM")))
                    {
                        aac_mediaInfo *pSourceAAC = (aac_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        //void *memory = alloc(sizeof(aac_mediaInfo) + pSourceAAC->getDecoderSpecificInfoSize());
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(aac_mediaInfo), alternateMedia);
                        aac_mediaInfo *pAAC = OSCL_PLACEMENT_NEW(memory, aac_mediaInfo());
                        //	unsigned char *decSpecificInfo = (unsigned char *)(memory) + sizeof(aac_mediaInfo);
                        //	pAAC->setDecoderSpecificInfo(decSpecificInfo, pSourceAAC->getDecoderSpecificInfoSize());
                        *pAAC = *pSourceAAC;
                    }
                    else if (!oscl_strncmp(MIMEType, "AMR", oscl_strlen("AMR")))
                    {
                        amr_mediaInfo *pSourceAMR = (amr_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(amr_mediaInfo), alternateMedia);
                        amr_mediaInfo *pAMR = OSCL_PLACEMENT_NEW(memory, amr_mediaInfo());
                        *pAMR = *pSourceAMR;
                    }
                    else if (!oscl_strncmp(MIMEType, "EVRC", oscl_strlen("EVRC")))
                    {
                        evrc_mediaInfo *pSourceEVRC = (evrc_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(evrc_mediaInfo), alternateMedia);
                        evrc_mediaInfo *pEVRC = OSCL_PLACEMENT_NEW(memory, evrc_mediaInfo());
                        *pEVRC = *pSourceEVRC;
                    }
                    else if (!oscl_strncmp(MIMEType, "MP4V-ES", oscl_strlen("MP4V-ES")))
                    {
                        m4v_mediaInfo *pSourceM4V = (m4v_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;
                        void *memory = alloc(sizeof(m4v_mediaInfo), alternateMedia);
                        m4v_mediaInfo *pM4V = OSCL_PLACEMENT_NEW(memory, m4v_mediaInfo());
                        *pM4V = *pSourceM4V;
                    }
                    else if (!oscl_strncmp(MIMEType, "H263-1998", oscl_strlen("H263-1998")))
                    {
                        h263_mediaInfo *pSourceH263 = (h263_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(h263_mediaInfo), alternateMedia);
                        h263_mediaInfo *pH263 = OSCL_PLACEMENT_NEW(memory, h263_mediaInfo());
                        *pH263 = *pSourceH263;
                    }
                    else if (!oscl_strncmp(MIMEType, "H263-2000", oscl_strlen("H263-2000")))
                    {
                        h263_mediaInfo *pSourceH263 = (h263_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(h263_mediaInfo), alternateMedia);
                        h263_mediaInfo *pH263 = OSCL_PLACEMENT_NEW(memory, h263_mediaInfo());
                        *pH263 = *pSourceH263;
                    }
                    else if (!oscl_strncmp(MIMEType, "PVMP4V-ES", oscl_strlen("PVMP4V-ES")))
                    {
                        m4v_mediaInfo *pSourceM4V = (m4v_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(m4v_mediaInfo), alternateMedia);
                        m4v_mediaInfo *pM4V = OSCL_PLACEMENT_NEW(memory, m4v_mediaInfo());
                        (mediaInfo)*pM4V = (mediaInfo) * pSourceM4V;
                        *pM4V = *pSourceM4V;
                    }
                    else if (!oscl_strncmp(MIMEType, "mpeg4-generic", oscl_strlen("mpeg4-generic")))
                    {
                        rfc3640_mediaInfo *pSourceRFC3640 = (rfc3640_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(rfc3640_mediaInfo), alternateMedia);
                        rfc3640_mediaInfo *pRFC3640 = OSCL_PLACEMENT_NEW(memory, rfc3640_mediaInfo());
                        (mediaInfo)*pRFC3640 = (mediaInfo) * pSourceRFC3640;
                        *pRFC3640 = *pSourceRFC3640;
                    }
                    else if (!oscl_strncmp(MIMEType, "X-MP4V-IMAGE", oscl_strlen("X-MP4V-IMAGE")))
                    {
                        still_image_mediaInfo *pSourceImage = (still_image_mediaInfo*)sourceSdpInfo.pMediaInfo[ii][ss];
                        if (ss == 0)
                            alternateMedia = false;
                        else
                            alternateMedia = true;

                        void *memory = alloc(sizeof(still_image_mediaInfo), alternateMedia);
                        still_image_mediaInfo *pImage = OSCL_PLACEMENT_NEW(memory, still_image_mediaInfo());
                        *pImage = *pSourceImage;
                    }

                }
            }
        }

        ~SDPInfo()
        {
            int ii = 0;
            int ss = 0;
            for (ii = 0; ii < numMediaObjects; ii++)
            {
                for (ss = 0; ss < (int)pMediaInfo[ii].size();ss++)
                {
                    pMediaInfo[ii][ss]->~mediaInfo();
                    dealloc(pMediaInfo[ii][ss]);
                }
            }
            numMediaObjects = 0;

        }

        sessionDescription *getSessionInfo()
        {
            return &session_info;
        };
        Oscl_Vector<mediaInfo *, SDPParserAlloc> getMediaInfo(int Object)
        {
            if ((Object >= 0) && (Object < MAX_MEDIA_OBJECTS))
            {
                return pMediaInfo[Object];
            }
            else
            {
                return 0;
            }
        };
        Oscl_Vector<mediaInfo *, SDPParserAlloc> *getMediaArray()
        {
            return pMediaInfo;
        };
        int getNumMediaObjects()
        {
            return numMediaObjects;
        };
        inline void dealloc(void *ptr)
        {
            oscl_free(ptr);
        };

        mediaInfo* getMediaInfoBasedOnID(uint32 trackID)
        {
            int numObjects = getNumMediaObjects();

            for (int i = 0; i < numObjects; i++)
            {
                Oscl_Vector<mediaInfo*, SDPParserAlloc> mediaInfoVec;
                mediaInfoVec = getMediaInfo(i);

                for (uint32 j = 0; j < mediaInfoVec.size(); j++)
                {
                    mediaInfo* minfo = mediaInfoVec[j];

                    if (minfo != NULL)
                    {
                        if (minfo->getMediaInfoID() == trackID)
                        {
                            return minfo;
                        }
                    }
                }
            }
            return NULL;
        }

        mediaInfo* getMediaInfoBasedOnDependsOnID(uint32 trackID)
        {
            if (trackID == 0)
            {
                return NULL;
            }

            int numObjects = getNumMediaObjects();

            for (int i = 0; i < numObjects; i++)
            {
                Oscl_Vector<mediaInfo*, SDPParserAlloc> mediaInfoVec;
                mediaInfoVec = getMediaInfo(i);

                for (uint32 j = 0; j < mediaInfoVec.size(); j++)
                {
                    mediaInfo* minfo = mediaInfoVec[j];

                    if (minfo != NULL)
                    {
                        if ((uint32)(minfo->getControlTrackID()) == trackID)
                        {
                            return minfo;
                        }
                    }
                }
            }
            return NULL;
        }


        void *alloc(const int size, bool alternateMedia)
        {
            OSCL_UNUSED_ARG(alternateMedia);

            if (numMediaObjects < MAX_MEDIA_OBJECTS)
            {
                void *mem = oscl_malloc(size * sizeof(char));
                if (mem != NULL)
                {
                    iMediaObjectIndex++;
                    pMediaInfo[numMediaObjects].push_back((mediaInfo *)mem);
                }
                return mem;
            }
            else
            {
                return NULL;
            }
        }

        void IncrementAlternateMediaInfoVectorIndex()
        {
            pMediaInfo[numMediaObjects][0]->setSegmentNumber(segmentCount);
            numMediaObjects++;
        }

        void copyFmDefMedia(mediaInfo *media)
        {
            *media = *pMediaInfo[numMediaObjects][0];
        }

        inline void reset()
        {
            session_info.resetSessionDescription();
            int ii = 0;
            int ss = 0;
            for (ii = 0; ii < numMediaObjects; ii++)
            {
                for (ss = 0; ss < (int)pMediaInfo[ii].size(); ss++)
                {
                    pMediaInfo[ii][ss]->~mediaInfo();
                    dealloc(pMediaInfo[ii][ss]);
                }
            }
            numMediaObjects = 0;
        }

        inline uint32 getMediaObjectIndex()
        {
            return iMediaObjectIndex;
        }

        bool getMediaInfoInSegment(int segment, Oscl_Vector< mediaInfo *, SDPParserAlloc>& segmentMediaInfo)
        {
            if (segmentCount == 0)
                return false;
            for (int ii = 0; ii < numMediaObjects; ii++)
            {
                if (pMediaInfo[ii][0]->getSegmentNumber() == (uint)segment)
                {
                    segmentMediaInfo.push_back(pMediaInfo[ii][0]);
                }
            }
            return true;
        }

        void setSegmentCount(int count)
        {
            segmentCount = count;
        }

        int getSegmentCount()
        {
            return segmentCount;
        }

        bool setSegmentActive(int segment, bool status)
        {
            if (segment <= segmentCount)
            {
                segmentInfo[segment].segmentActive = status;
                return true;
            }
            else
                return false;
        }

        bool isSegmentActive(int segment)
        {
            if (segment <= segmentCount)
                return segmentInfo[segment].segmentActive;
            else
                return false;
        }

        bool setSegmentPayloadOrderPref(int segment, int* payloadArray, int len)
        {
            segmentInfo[segment].segmentPayloadOrderPref = false;
            if (segment <= segmentCount)
            {
                Oscl_Vector< mediaInfo *, SDPParserAlloc> segmentMediaInfo;
                if (getMediaInfoInSegment(segment, segmentMediaInfo) == true)
                {
                    for (int ii = 0; ii < len; ii++)
                    {
                        for (uint32 jj = 0; jj < segmentMediaInfo.size(); jj++)
                        {
                            if (segmentMediaInfo[jj]->getPayloadSpecificInfoVector()[0]->getPayloadNumber() == (uint32)payloadArray[ii])
                            {
                                segmentMediaInfo[jj]->setPayloadPreference(ii);
                                break;
                            }
                        }
                    }
                }
            }
            else
                return false;
            segmentInfo[segment].segmentPayloadOrderPref = true;
            return true;
        }

        bool isPayloadOrderPreferenceSet(int segment)
        {
            if (segment <= segmentCount)
                return segmentInfo[segment].segmentPayloadOrderPref;
            else
                return false;
        }

        mediaInfo* getPreferedMediaInfo(int segment)
        {
            mediaInfo* media = NULL;
            Oscl_Vector< mediaInfo *, SDPParserAlloc> segmentMediaInfo;
            if (getMediaInfoInSegment(segment, segmentMediaInfo) == true)
            {
                for (uint32 jj = 0; jj < segmentMediaInfo.size(); jj++)
                {
                    if (segmentMediaInfo[jj]->isMatched() == true)
                    {
                        media = segmentMediaInfo[jj];
                        break;
                    }
                }
            }
            return media;
        }

        int getPreferedPayloadNumber(int segment)
        {
            int payload = -1;
            Oscl_Vector< mediaInfo *, SDPParserAlloc> segmentMediaInfo;
            if (getMediaInfoInSegment(segment, segmentMediaInfo) == true)
            {
                for (uint32 jj = 0; jj < segmentMediaInfo.size(); jj++)
                {
                    if (segmentMediaInfo[jj]->isMatched() == true)
                    {
                        payload = segmentMediaInfo[jj]->getPayloadSpecificInfoVector()[0]->getPayloadNumber();
                        break;
                    }
                }
            }
            return payload;
        }

        const oscl_wchar *getSdpFilename(uint32 &retsize)
        {
            retsize = SdpFilename.get_size();
            return SdpFilename.get_cstr();
        }

        void setSDPFilename(OSCL_wString& aURL)
        {
            SdpFilename = aURL;
        }

    private:
        sessionDescription session_info;
        Oscl_Vector< mediaInfo *, SDPParserAlloc> pMediaInfo[MAX_MEDIA_OBJECTS];
        int numMediaObjects;
        registrar *reg;
        bool registrar_locally_allocated;
        uint32 iMediaObjectIndex;
        int segmentCount;
        segmentSpecific segmentInfo[MAX_SEGMENTS];
        OSCL_wHeapString<SDPParserAlloc> SdpFilename;
};
#endif // __SDP_INFO_H__

