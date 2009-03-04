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
/*
 *
 * Release: 050926
 *
 ***************************************************************************/
/* //////////////////////////////////////////////////////////////// */
/*	Program	: m4v_io.cpp											*/
/*	Date	: Dec. 22, 2000											*/
/* ---------------------------------------------------------------- */
/*	The MPEG-4 raw video bitstream I/O routines.					*/
/* //////////////////////////////////////////////////////////////// */
#include "m4v_io.h"
#include "oscl_base_macros.h"

static int32 nLayer;

static uint8 VOSH_START_CODE[] = { 0x00, 0x00, 0x01, 0xB0 };
static uint8 VO_START_CODE[] = { 0x00, 0x00, 0x01, 0x00 };
static uint8 VOP_START_CODE[] = { 0x00, 0x00, 0x01, 0xB6 };
static uint8 H263_START_CODE[] = { 0x00, 0x00, 0x80};

static int short_video_header = FALSE;


int32 m4v_getVideoLayerNumber()
{
    return nLayer;
}

int32 m4v_getVideoHeader(int32 layer, uint8 *buf, int32 max_size)
{
    OSCL_UNUSED_ARG(layer);

    int32 count = 0;
    char my_sc[4];

    uint8 *tmp_bs = buf;

    memcpy(my_sc, tmp_bs, 4);
    my_sc[3] &= 0xf0;

    if (max_size >= 4)
    {
        if (memcmp(my_sc, VOSH_START_CODE, 4) && memcmp(my_sc, VO_START_CODE, 4))
        {
            count = 0;
            short_video_header = TRUE;
        }
        else
        {
            count = 0;
            short_video_header = FALSE;
            while (memcmp(tmp_bs + count, VOP_START_CODE, 4))
            {
                count++;
                if (count > 1000)
                {
                    short_video_header = TRUE;
                    break;
                }
            }
            if (short_video_header == TRUE)
            {
                count = 0;
                while (memcmp(tmp_bs + count, H263_START_CODE, 3))
                {
                    count++;
                }
            }
        }
    }
    return count;
}


