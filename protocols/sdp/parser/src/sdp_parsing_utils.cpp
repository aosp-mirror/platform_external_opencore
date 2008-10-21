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

#include "oscl_string_utils.h"
#include "common_info.h"
#include "rtsp_range_utils.h"
#include "oscl_str_ptr_len.h"
#include "oscl_stdstring.h"

bool get_next_line(const char *start_ptr, const char * end_ptr,
                   const char *& line_start,
                   const char *& line_end)
{
    // Finds the boundaries of the next non-empty line within start
    // and end ptrs

    // This initializes line_start to the first non-whitespace character
    line_start = skip_whitespace_and_line_term(start_ptr, end_ptr);

    line_end = skip_to_line_term(line_start, end_ptr);

    return (line_start < end_ptr);

}

bool parseQoEMetrics(const char *start_ptr, const char *end_ptr, QoEMetricsType &qoeMetrics)
{
    const char *sptr = start_ptr;
    const char *eptr = end_ptr;

    sptr = skip_whitespace_and_line_term(sptr, end_ptr);

    StrPtrLen rate("rate=");
    StrPtrLen range("range:");

    if (!oscl_strncmp(sptr, "{", 1))
        sptr = sptr + 1;
    else
        return false;

    sptr = skip_whitespace_and_line_term(sptr, end_ptr);
    if (sptr > eptr)
        return false;

    while (sptr < end_ptr)
    {

        if (!oscl_strncmp(sptr, "Initial_Buffering_Duration",
                          oscl_strlen("Initial_Buffering_Duration")))
        {
            qoeMetrics.name[QoEMetricsType::INITIAL_BUFFERING_DURATION] = true;
            sptr = sptr + oscl_strlen("Initial_Buffering_Duration");
        }
        else if (!oscl_strncmp(sptr, "Rebuffering_Duration",
                               oscl_strlen("Rebuffering_Duration")))
        {
            qoeMetrics.name[QoEMetricsType::REBUFFERING_DURATION] = true;
            sptr = sptr + oscl_strlen("Rebuffering_Duration");
        }
        else if (!oscl_strncmp(sptr, "Corruption_Duration",
                               oscl_strlen("Corruption_Duration")))
        {
            qoeMetrics.name[QoEMetricsType::CORRUPTION_DURATION] = true;
            sptr = sptr + oscl_strlen("Corruption_Duration");
        }
        else if (!oscl_strncmp(sptr, "Succssive_Loss",
                               oscl_strlen("Succssive_Loss")))
        {
            qoeMetrics.name[QoEMetricsType::SUCESSIVE_LOSS] = true;
            sptr = sptr + oscl_strlen("Succssive_Loss");
        }
        else if (!oscl_strncmp(sptr, "Framerate_Deviation",
                               oscl_strlen("Framerate_Deviation")))
        {
            qoeMetrics.name[QoEMetricsType::FRAMERATE_DEVIATION] = true;
            sptr = sptr + oscl_strlen("Framerate_Deviation");
        }
        else if (!oscl_strncmp(sptr, "Jitter_Duration",
                               oscl_strlen("Jitter_Duration")))
        {
            qoeMetrics.name[QoEMetricsType::JITTER_DURATION] = true;
            sptr = sptr + oscl_strlen("Jitter_Duration");
        }
        else if (!oscl_strncmp(sptr, "Decoded_Bytes",
                               oscl_strlen("Decoded_Bytes")))
        {
            qoeMetrics.name[QoEMetricsType::DECODED_BYTES] = true;
            sptr = sptr + oscl_strlen("Decoded_Bytes");
        }
        else
            return false;

        sptr = skip_whitespace_and_line_term(sptr, end_ptr);
        if (sptr > end_ptr)
            return false;

        if (!oscl_strncmp(sptr, ",", 1))
            sptr = sptr + 1;
        else if (!oscl_strncmp(sptr, "}", 1))
        {
            sptr = sptr + 1;
            break;
        }
    }

    if (sptr > end_ptr)
        return false;

    if (!oscl_strncmp(sptr, ";", 1))
        sptr = sptr + 1;
    else
        return false;

    if (!oscl_strncmp(sptr, rate.c_str(), rate.length()))
    {
        sptr = sptr + rate.length();
        if (!oscl_strncmp(sptr, "End", oscl_strlen("End")))
        {
            qoeMetrics.rateFmt = QoEMetricsType::END;
            qoeMetrics.rateEnd = 'E';
            sptr = sptr + oscl_strlen("End");

        }
        else
        {
            uint32 temp;
            eptr = sptr;		//get length of range digit
            for (; (*eptr != ';' && eptr < end_ptr); ++eptr);

            qoeMetrics.rateFmt = QoEMetricsType::VAL;

            if (PV_atoi(sptr, 'd', (int)(eptr  - sptr), temp))
            {
                qoeMetrics.rateVal = temp;
                sptr = eptr ;
            }
            else
                return false;
        }

    }
    else
        return false;

    if (sptr == end_ptr) //end of line reached.
        return true;

    if (sptr > end_ptr)
        return false;

    if (!oscl_strncmp(sptr, ";", 1))
        sptr = sptr + 1;
    else
        return false;
    if (!oscl_strncmp(sptr, range.c_str(), range.length()))
    {
        eptr = sptr + range.length();
        for (; *eptr != ';' && eptr != end_ptr ; eptr++); // get length of range.

        if (!parseRtspRange((sptr + range.length()), (eptr - sptr - range.length()),
                            qoeMetrics.range))
            return false;
    }

    sptr = eptr;
    if (sptr == end_ptr)
        return true;    // end of line reached.
    else  //Parameter_Ext
    {
        sptr = eptr + 1;
        if (!oscl_strncmp(sptr, "On", oscl_strlen("On")))
        {
            qoeMetrics.paramExtStat = true;
            qoeMetrics.paramFmt = QoEMetricsType::STATUS;
        }
        else if (!oscl_strncmp(sptr, "Off", oscl_strlen("Off")))
        {
            qoeMetrics.paramExtStat = false;
            qoeMetrics.paramFmt = QoEMetricsType::STATUS;
        }
        else
        {
            if (NULL != oscl_strstr(sptr, "."))	//if floating point number
            {
                if (!PV_atof(sptr, (int)(eptr - sptr), qoeMetrics.paramExtFdigit))
                    return false;
                qoeMetrics.paramFmt = QoEMetricsType::FDIGIT;
            }

            else	    		// hex digit
            {
                uint32 temp;
                if (PV_atoi(sptr, 'x', (int)(eptr - sptr), temp))
                {
                    qoeMetrics.paramExtIdigit = temp;
                    qoeMetrics.paramFmt = QoEMetricsType::IDIGIT;

                }
                else
                    return false;
            }

        }


    }

    return true;

}

bool parseAssetInfo(const char *sptr, const char *line_end_ptr, AssetInfoType &ainfo)
{
    const char *eptr = sptr;
    int assetbox;

    while (eptr < line_end_ptr)
    {
        sptr = skip_whitespace(sptr, line_end_ptr);
        if (oscl_strncmp(sptr, "{", 1))
            return false;
        sptr = sptr + 1;
        sptr = skip_whitespace(sptr, line_end_ptr);
        if (sptr > line_end_ptr)
            return false;

        if (!oscl_strncmp(sptr, "url=", oscl_strlen("url=")))
        {
            sptr = sptr + oscl_strlen("url=");
            sptr = skip_whitespace(sptr, line_end_ptr);
            if (sptr > line_end_ptr)
                return false;

            if (oscl_strncmp(sptr, "\"", 1))
                return false;
            sptr = sptr + 1;
            sptr = skip_whitespace(sptr, line_end_ptr);
            if (sptr > line_end_ptr)
                return false;
            eptr = sptr;

            for (; *eptr != '"'; ++eptr);

            ainfo.URL.set((const char *)sptr, (eptr - sptr));

            return true;

        }


        if (!oscl_strncmp(sptr, "Title=", oscl_strlen("Title=")))
        {
            sptr = sptr + oscl_strlen("Title=");
            assetbox = (int) AssetInfoType::TITLE;
            ainfo.oTitlePresent = true;
        }
        else if (!oscl_strncmp(sptr, "Description=", oscl_strlen("Description=")))
        {
            sptr = sptr + oscl_strlen("Description=");
            assetbox = (int) AssetInfoType::DESCRIPTION;
            ainfo.oDescriptionPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Copyright=", oscl_strlen("Copyright=")))
        {
            sptr = sptr + oscl_strlen("Copyright=");
            assetbox = (int) AssetInfoType::COPYRIGHT;
            ainfo.oCopyRightPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Performer=", oscl_strlen("Performer=")))
        {
            sptr = sptr + oscl_strlen("Performer=");
            assetbox = (int) AssetInfoType::PERFORMER;
            ainfo.oPerformerPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Author=", oscl_strlen("Author=")))
        {
            sptr = sptr + oscl_strlen("Author=");
            assetbox = (int) AssetInfoType::AUTHOR;
            ainfo.oAuthorPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Genre=", oscl_strlen("Genre=")))
        {
            sptr = sptr + oscl_strlen("Genre=");
            assetbox = (int) AssetInfoType::GENRE;
            ainfo.oGenrePresent = true;
        }
        else if (!oscl_strncmp(sptr, "Rating=", oscl_strlen("Rating=")))
        {
            sptr = sptr + oscl_strlen("Rating=");
            assetbox = (int) AssetInfoType::RATING;
            ainfo.oRatingPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Classification=", oscl_strlen("Classification=")))
        {
            sptr = sptr + oscl_strlen("Classification=");
            assetbox = (int) AssetInfoType::CLASSIFICATION;
            ainfo.oClassificationPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Keywords=", oscl_strlen("Keywords=")))
        {
            sptr = sptr + oscl_strlen("Keywords=");
            assetbox = (int) AssetInfoType::KEYWORDS;
            ainfo.oKeyWordsPresent = true;
        }
        else if (!oscl_strncmp(sptr, "Location=", oscl_strlen("Location=")))
        {
            sptr = sptr + oscl_strlen("Location=");
            assetbox = (int) AssetInfoType::LOCATION;
            ainfo.oLocationPresent = true;
        }
        else// if(!oscl_strncmp(sptr, "asset-extention=", oscl_strlen("asset-extention=")))
        {//asset-extension ignore for now
            //sptr = sptr + oscl_strlen("asset-extention=");
            assetbox = (int) AssetInfoType::ASSET_EXTENTION;
            ainfo.oAssetExtensionPresent = true;
        }

        sptr = skip_whitespace(sptr, line_end_ptr);
        if (sptr > line_end_ptr)
            return false;

        for (eptr = sptr; *eptr != '}'; ++eptr)
        {
            if (eptr > line_end_ptr)
                return false;
        }

        ainfo.Box[assetbox].set((const char *)sptr, (eptr - sptr));

        eptr = eptr + 1;
        sptr = eptr + 1;
    }

    return true;
}

bool sdp_decodebase64(uint8* aInBuf, uint32 aInBufLen,
                      uint8* aOutBuf, uint32& aOutBufLen, uint32 aMaxOutBufLen)
{
    oscl_memset(aOutBuf, 0, aMaxOutBufLen);
    aOutBufLen = 0;

    int i;
    uint8 dtable[256];

    for (i = 0;i < 255;i++)
    {
        dtable[i] = 0x80;
    }
    for (i = 'A';i <= 'I';i++)
    {
        dtable[i] = 0 + (i - 'A');
    }
    for (i = 'J';i <= 'R';i++)
    {
        dtable[i] = 9 + (i - 'J');
    }
    for (i = 'S';i <= 'Z';i++)
    {
        dtable[i] = 18 + (i - 'S');
    }
    for (i = 'a';i <= 'i';i++)
    {
        dtable[i] = 26 + (i - 'a');
    }
    for (i = 'j';i <= 'r';i++)
    {
        dtable[i] = 35 + (i - 'j');
    }
    for (i = 's';i <= 'z';i++)
    {
        dtable[i] = 44 + (i - 's');
    }
    for (i = '0';i <= '9';i++)
    {
        dtable[i] = 52 + (i - '0');
    }
    dtable[(int)'+'] = 62;
    dtable[(int)'/'] = 63;
    dtable[(int)'='] = 0;

    uint32 read_count = 0;
    uint32 write_count = 0;
    while (read_count < aInBufLen)
    {
        uint8 a[4], b[4], o[3];

        for (i = 0;i < 4;i++)
        {
            uint8 c = *(aInBuf++);
            read_count++;

            if (read_count > aInBufLen)
            {
                //Input incomplete
                return false;
            }
            if (dtable[(int)c]&0x80)
            {
                //Illegal character in
                //return false;
                i--;
                continue;
            }
            a[i] = (uint8)c;
            b[i] = (uint8)dtable[(int)c];
        }
        o[0] = (b[0] << 2) | (b[1] >> 4);
        o[1] = (b[1] << 4) | (b[2] >> 2);
        o[2] = (b[2] << 6) | b[3];
        i = a[2] == '=' ? 1 : (a[3] == '=' ? 2 : 3);
        oscl_memcpy(aOutBuf, o, i);
        aOutBuf += i;
        write_count += i;
        if (write_count > aMaxOutBufLen)
        {
            return false;
        }
        if (i < 3)
        {
            break;
        }
    }
    aOutBufLen = write_count;
    return true;
}

