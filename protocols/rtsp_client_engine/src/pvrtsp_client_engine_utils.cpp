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
#ifndef PVRTSP_CLIENT_ENGINE_UTILS_H
#include "pvrtsp_client_engine_utils.h"
#endif
#include "oscl_mem.h"
#include "oscl_mem_basic_functions.h"

bool composeURL(const char *baseURL, const char *relativeURL, char *completeURL, unsigned int &completeURLLen)
{
    char* copyOfBaseURL = OSCL_ARRAY_NEW(char, 8 * MAX_LONG_TEXT_LEN);
    if (NULL == copyOfBaseURL)
    {
        return false;
    }

    URLType urlType = findRelativeURLType(relativeURL);
    switch (urlType)
    {
        case CONCATENATE:
        {
            int len = oscl_strlen(baseURL);
            oscl_strncpy(copyOfBaseURL, baseURL, (len + 1));
            //JJ 12/04/06
            //dropTextAfterLastSlash(copyOfBaseURL);
            if ((copyOfBaseURL[len-1] != '/') && (len > 0))
            {
                copyOfBaseURL[len] = '/';
                copyOfBaseURL[len+1] = 0;
                len++;
            }
            if (completeURLLen <= (len + oscl_strlen(relativeURL)))
            {
                OSCL_ARRAY_DELETE(copyOfBaseURL);
                return false;
            }
            oscl_strncpy(completeURL, copyOfBaseURL, (oscl_strlen(copyOfBaseURL) + 1));
            oscl_strcat(completeURL, relativeURL);
            completeURLLen = oscl_strlen(completeURL);
            break;
        }
        case REPLACE_PATH:
        {
            oscl_strncpy(copyOfBaseURL, baseURL, (oscl_strlen(baseURL) + 1));
            //JJ 12/04/06
            //dropTextAfterFirstSlash(copyOfBaseURL);
            if (completeURLLen <= (oscl_strlen(copyOfBaseURL) + oscl_strlen(relativeURL)))
            {
                OSCL_ARRAY_DELETE(copyOfBaseURL);
                return false;
            }
            oscl_strncpy(completeURL, copyOfBaseURL, (oscl_strlen(copyOfBaseURL) + 1));
            oscl_strcat(completeURL, relativeURL);
            completeURLLen = oscl_strlen(completeURL);
            break;
        }
        case REPLACE_HOST:
        {
            const char RTSP[] = "rtsp:";
            if (completeURLLen > (oscl_strlen(RTSP) + oscl_strlen(relativeURL)))
            {
                oscl_strncpy(completeURL, RTSP, (oscl_strlen(RTSP) + 1));
                oscl_strcat(completeURL, relativeURL);
                completeURLLen = oscl_strlen(completeURL);
            }
            else
            {
                OSCL_ARRAY_DELETE(copyOfBaseURL);
                return false;
            }
            break;
        }
        case UNKNOWN:
        {
            OSCL_ARRAY_DELETE(copyOfBaseURL);
            return false;
            // break;	This statement was removed to avoid compiler warning for Unreachable Code

        }
    }

    if (copyOfBaseURL)
        OSCL_ARRAY_DELETE(copyOfBaseURL);

    return true;
}

const char* findRelativeURL(const char *aURL)
{
    int i = 0;
    while (aURL[i] != '\0')
    {
        if (aURL[i] == FWD_SLASH)
        {
            if (aURL[i+1] != FWD_SLASH)
            {
                return &(aURL[i]);
            }
            else
            {//	"//"
                i++;
            }
        }
        i++;
    }
    return NULL;
}

URLType findRelativeURLType(const char *relativeURL)
{
    if ((FWD_SLASH == relativeURL[0]) && (FWD_SLASH == relativeURL[1]))
    {
        return REPLACE_HOST;
    }
    else if ((FWD_SLASH == relativeURL[0]) && (FWD_SLASH != relativeURL[1]))
    {
        return REPLACE_PATH;
    }
    else if ((oscl_strstr(relativeURL, &COLON) != NULL) || (DOT == relativeURL[0]))
    {
        return UNKNOWN;
    }
    else
    {
        return CONCATENATE;
    }
}

void dropTextAfterLastSlash(char *copyOfBaseURL)
{
    int textLen = oscl_strlen(copyOfBaseURL) - 1;

    for (int ii = textLen; ii > 0; ii--)
    {
        if ((FWD_SLASH == copyOfBaseURL[ii]) && (ii == textLen))
        {
            return;
        }
        else if ((FWD_SLASH != copyOfBaseURL[ii-1]) &&
                 (FWD_SLASH == copyOfBaseURL[ii]) &&
                 (FWD_SLASH != copyOfBaseURL[ii+1]))
        {
            copyOfBaseURL[ii+1] = '\0';//NULL;
            return;
        }
    }
    //We reach this point only if we a base URL that is not terminated with a '/'.
    //So, we need to append '/' to the string.
    if (8*MAX_LONG_TEXT_LEN >= (textLen + 2))
    {
        copyOfBaseURL[textLen+1] = FWD_SLASH;
        copyOfBaseURL[textLen+2] = '\0';//NULL;
    }
    return;
}

void dropTextAfterFirstSlash(char *copyOfBaseURL)
{
    int textLen = oscl_strlen(copyOfBaseURL);
    int ii;
    for (ii = 1; ii < textLen - 1; ii++)
    {
        if ((FWD_SLASH != copyOfBaseURL[ii-1]) &&
                (FWD_SLASH == copyOfBaseURL[ii]) &&
                (FWD_SLASH != copyOfBaseURL[ii+1]))
        {
            copyOfBaseURL[ii] = '\0';//NULL;
            return;
        }
    }
    //We reach this point if we were not able to find a '/' in the end of the clip.
    if (FWD_SLASH == copyOfBaseURL[ii])
    {
        copyOfBaseURL[ii] = '\0';//NULL;
    }

    return;
}

#ifndef OSCL_TIME_H_INCLUDED
#include "oscl_time.h"
#endif
#if (!defined(MD5_H) && defined(SDK_HAS_REAL_HTTP_CLOAKING_SUPPORT))
#include "md5.h"
#endif
#ifndef OSCL_SNPRINTF_H_INCLUDED
#include "oscl_snprintf.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif

bool generatePseudoUUID(OSCL_String& aUUID)
{
#ifdef SDK_HAS_REAL_HTTP_CLOAKING_SUPPORT
    //6e31c837-b458-4c27-b290-73805cb08da1
    TimeValue current_time;
    current_time.set_to_current_time();

    uint32 seed_1 = current_time.get_sec();
    uint32 seed_2 = current_time.get_usec();

    MD5 myMD5;
    myMD5.add((const  uint8 *)(&seed_1), sizeof(seed_1));
    myMD5.add((const  uint8 *)(&seed_2), sizeof(seed_2));

    MD5OctetHashValue md5output;
    myMD5.compute_hash(md5output);

    const int MAX_UUID_BUFSIZE = 64;
    char buffer[MAX_UUID_BUFSIZE+1];

    {
        const char hex[] = "0123456789abcdef";
        char *ptr = buffer;

        for (uint32 i = 0, j = 0; i < 16; i++)
        {
            ptr[j++] = hex[md5output.hash_array[i] >> 4];
            ptr[j++] = hex[md5output.hash_array[i] & 0x0f];

            if ((j == 8) || (j == 13) || (j == 18) || (j == 23))
                ptr[j++] = '-';

            ptr[j] = '\0';
        }
    }
    aUUID = buffer;
#else
    aUUID = NULL;
#endif
    return true;
}

