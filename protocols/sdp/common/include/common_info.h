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

#ifndef COMMONINFO_H
#define COMMONINFO_H


#include "rtsp_range_utils.h"
#include "oscl_types.h"
#include "oscl_string_containers.h"
#include "sdp_memory.h"


#define MAX_METRICS_NAME	7
#define ASSET_NAME_SIZE		11
#define MAX_ALTERNATIVES	16
#define ALT_SIZE			128

#define FIRST_STATIC_PAYLOAD 0
#define LAST_STATIC_PAYLOAD 33

#define PVMF_PCMU 0
#define PVMF_PCMA 8

#define PVMF_MIME_PCMU "PCMU"
#define PVMF_MIME_PCMA "PCMA"



struct QoEMetricsType
{
    enum
    {
        INITIAL_BUFFERING_DURATION, REBUFFERING_DURATION,
        CORRUPTION_DURATION, SUCESSIVE_LOSS, FRAMERATE_DEVIATION,
        JITTER_DURATION, DECODED_BYTES
    };

    //if a Metrics_Name is present, corresponding array component will be true.
    bool name[MAX_METRICS_NAME];
    enum { END, VAL };
    int rateFmt;
    union
    {
        char rateEnd;
        int  rateVal;
    };

    RtspRangeType range;

    enum { STATUS, IDIGIT, FDIGIT };
    int paramFmt;
    union
    {
        bool paramExtStat;  // (true = On)/(false = Off)
        int paramExtIdigit;
        OsclFloat paramExtFdigit;
    };


};

class AssetInfoType
{
    public:
        enum
        {
            TITLE = 0,
            DESCRIPTION = 1,
            COPYRIGHT = 2,
            PERFORMER = 3,
            AUTHOR = 4,
            GENRE = 5,
            RATING = 6,
            CLASSIFICATION = 7,
            KEYWORDS = 8,
            LOCATION = 9,
            ASSET_EXTENTION = 10
        };

        AssetInfoType()
        {
            oTitlePresent = false;
            oDescriptionPresent = false;
            oCopyRightPresent = false;
            oPerformerPresent = false;
            oAuthorPresent = false;
            oGenrePresent = false;
            oRatingPresent = false;
            oClassificationPresent = false;
            oKeyWordsPresent = false;
            oLocationPresent = false;
            oAssetExtensionPresent = false;
        };

        OSCL_HeapString<SDPParserAlloc> URL;
        OSCL_HeapString<SDPParserAlloc> Box[ASSET_NAME_SIZE];

        bool oTitlePresent;
        bool oDescriptionPresent;
        bool oCopyRightPresent;
        bool oPerformerPresent;
        bool oAuthorPresent;
        bool oGenrePresent;
        bool oRatingPresent;
        bool oClassificationPresent;
        bool oKeyWordsPresent;
        bool oLocationPresent;
        bool oAssetExtensionPresent;
};


struct altGpBWType
{

    uint32 group;	//4 bits represent one alt-id hence "group" can represent
    //a group of 8 alt-ids. e.g for a=alt-group=BW:AS:28=1,2,4,5
    //group represent 1,2,4,5. Hence group=0x5421
    uint32 val;

};

struct altGpLANGType
{

    uint32 group;	//4 bits represent one alt-id hence "group" can represent
    //a group of 8 alt-ids. e.g for a=alt-group=LANG:RFC3066:en-US=1,2,4,5
    //group represent 1,2,4,5. Hence group=0x5421
    OSCL_HeapString<SDPParserAlloc> val;
};

typedef struct _connectionInfo
{
    OSCL_HeapString<SDPParserAlloc> connectionNetworkType; //Add set and get methods.
    OSCL_HeapString<SDPParserAlloc> connectionAddressType;
    OSCL_HeapString<SDPParserAlloc> connectionAddress;


}connectionInfo;

#endif
