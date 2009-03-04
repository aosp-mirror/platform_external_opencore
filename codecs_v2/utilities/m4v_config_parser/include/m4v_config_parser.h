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
#ifndef PV_M4V_CONFIG_PARSER_H_INCLUDED
#define PV_M4V_CONFIG_PARSER_H_INCLUDED

#include "oscl_base.h"
#include "oscl_types.h"

#define MP4_INVALID_VOL_PARAM -1
#define SHORT_HEADER_MODE -4


#define VISUAL_OBJECT_SEQUENCE_START_CODE 	0x01B0
#define VISUAL_OBJECT_SEQUENCE_END_CODE 	0x01B1
#define VISUAL_OBJECT_START_CODE   0x01B5
#define VO_START_CODE 		    0x8
#define VO_HEADER_LENGTH        32
#define VOL_START_CODE 0x12
#define VOL_START_CODE_LENGTH 28

#define GROUP_START_CODE	0x01B3
#define GROUP_START_CODE_LENGTH  32

#define VOP_ID_CODE_LENGTH		5
#define VOP_TEMP_REF_CODE_LENGTH	16

#define USER_DATA_START_CODE	    0x01B2
#define USER_DATA_START_CODE_LENGTH 32

#define SHORT_VIDEO_START_MARKER		0x20
#define SHORT_VIDEO_START_MARKER_LENGTH  22

typedef struct
{
    uint8 *data;
    uint32 numBytes;
    uint32 bytePos;
    uint32 bitBuf;
    uint32 dataBitPos;
    uint32  bitPos;
} mp4StreamType;


int16 ShowBits(
    mp4StreamType *pStream,
    uint8 ucNBits,
    uint32 *pulOutData
);

int16 FlushBits(
    mp4StreamType *pStream,
    uint8 ucNBits
);

int16 ReadBits(
    mp4StreamType *pStream,
    uint8 ucNBits,
    uint32 *pulOutData
);



int16 ByteAlign(
    mp4StreamType *pStream
);



int16 iDecodeVOLHeader(mp4StreamType *psBits, int *width, int *height, int *, int *);
OSCL_IMPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int length, int *width, int *height, int *, int *);
int16 DecodeUserData(mp4StreamType *pStream);
OSCL_IMPORT_REF int16 iDecodeShortHeader(mp4StreamType *psBits, int *width, int *height, int *, int *);
OSCL_IMPORT_REF int16 iGetAVCConfigInfo(uint8 *buffer, int length, int *width, int *height, int *, int *);
int16 DecodeSPS(mp4StreamType *psBits, int *width, int *height, int *display_width, int *display_height);
void ue_v(mp4StreamType *psBits, uint32 *codeNum);
void se_v(mp4StreamType *psBits, int32 *value);
void Parser_EBSPtoRBSP(uint8 *nal_unit, int *size);

#endif //PV_M4V_CONFIG_PARSER_H_INCLUDED


