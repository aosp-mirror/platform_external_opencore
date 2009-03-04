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

#include "m4v_config_parser.h"
#include "oscl_mem.h"
#include "oscl_dll.h"
OSCL_DLL_ENTRY_POINT_DEFAULT()

#define PV_CLZ(A,B) while (((B) & 0x8000) == 0) {(B) <<=1; A++;}

static const uint32 mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

int32 LocateFrameHeader(uint8 *ptr, int32 size)
{
    int count = 0;
    int32 i = size;

    if (size < 1)
    {
        return 0;
    }
    while (i--)
    {
        if ((count > 1) && (*ptr == 0x01))
        {
            i += 2;
            break;
        }

        if (*ptr++)
            count = 0;
        else
            count++;
    }
    return (size - (i + 1));
}

void movePointerTo(mp4StreamType *psBits, int32 pos)
{
    uint32 byte_pos;
    if (pos < 0)
    {
        pos = 0;
    }

    byte_pos = pos >> 3;

    if (byte_pos > (psBits->numBytes - psBits->bytePos))
    {
        byte_pos = (psBits->numBytes - psBits->bytePos);
    }

    psBits->bytePos = byte_pos & -4;
    psBits->dataBitPos = psBits->bytePos << 3;
    FlushBits(psBits, ((pos & 0x7) + ((byte_pos & 0x3) << 3)));
}

int16 SearchNextM4VFrame(mp4StreamType *psBits)
{
    int16 status = 0;
    uint8 *ptr;
    int32 i;
    uint32 initial_byte_aligned_position = (psBits->dataBitPos + 7) >> 3;

    ptr = psBits->data + initial_byte_aligned_position;

    i = LocateFrameHeader(ptr, psBits->numBytes - initial_byte_aligned_position);
    if (psBits->numBytes <= initial_byte_aligned_position + i)
    {
        status = -1;
    }
    (void)movePointerTo(psBits, ((i + initial_byte_aligned_position) << 3)); /* ptr + i */
    return status;
}

OSCL_EXPORT_REF int16 iGetM4VConfigInfo(uint8 *buffer, int length, int *width, int *height, int *display_width, int *display_height)
{
    int16 status;
    mp4StreamType psBits;
    psBits.data = buffer;
    psBits.numBytes = length;
    psBits.bitBuf = 0;
    psBits.bitPos = 32;
    psBits.bytePos = 0;
    psBits.dataBitPos = 0;
    *width = *height = *display_height = *display_width = 0;
    status = iDecodeVOLHeader(&psBits, width, height, display_width, display_height);
    return status;
}

// name: iDecodeVOLHeader
// Purpose: decode VOL header
// return:  error code
int16 iDecodeVOLHeader(mp4StreamType *psBits, int *width, int *height, int *display_width, int *display_height)
{

    int16 iErrorStat;
    uint32 codeword;
    int time_increment_resolution, nbits_time_increment;
    int i, j;

    ShowBits(psBits, 32, &codeword);

    if (codeword == VISUAL_OBJECT_SEQUENCE_START_CODE)
    {
        psBits->dataBitPos += 32;

        ReadBits(psBits, 8, &codeword);

        ShowBits(psBits, 32, &codeword);
        if (codeword == USER_DATA_START_CODE)
        {
            iErrorStat = DecodeUserData(psBits);
            if (iErrorStat) return MP4_INVALID_VOL_PARAM;
        }


        ReadBits(psBits, 32, &codeword);
        if (codeword != VISUAL_OBJECT_START_CODE) return MP4_INVALID_VOL_PARAM;

        /*  is_visual_object_identifier            */
        ReadBits(psBits, 1, &codeword);

        if (codeword)
        {
            /* visual_object_verid                            */
            ReadBits(psBits, 4, &codeword);
            /* visual_object_priority                         */
            ReadBits(psBits, 3, &codeword);
        }
        /* visual_object_type                                 */
        ReadBits(psBits, 4, &codeword);

        if (codeword == 1)
        { /* video_signal_type */
            ReadBits(psBits, 1, &codeword);
            if (codeword == 1)
            {
                /* video_format */
                ReadBits(psBits, 3, &codeword);
                /* video_range  */
                ReadBits(psBits, 1, &codeword);
                /* color_description */
                ReadBits(psBits, 1, &codeword);
                if (codeword == 1)
                {
                    /* color_primaries */
                    ReadBits(psBits, 8, &codeword);;
                    /* transfer_characteristics */
                    ReadBits(psBits, 8, &codeword);
                    /* matrix_coefficients */
                    ReadBits(psBits, 8, &codeword);
                }
            }
        }
        else
        {
            int16 status = 0;
            do
            {
                /* Search for VOL_HEADER */
                status = SearchNextM4VFrame(psBits); /* search 0x00 0x00 0x01 */
                if (status != 0)
                    return MP4_INVALID_VOL_PARAM;

                status = ReadBits(psBits, VOL_START_CODE_LENGTH, &codeword);
            }
            while ((codeword != VOL_START_CODE) && (status == 0));
            goto decode_vol;
        }
        /* next_start_code() */
        ByteAlign(psBits);

        ShowBits(psBits, 32, &codeword);
        if (codeword == USER_DATA_START_CODE)
        {
            iErrorStat = DecodeUserData(psBits);
            if (iErrorStat) return MP4_INVALID_VOL_PARAM;
        }
        ShowBits(psBits, 27, &codeword);
    }
    else
    {
        ShowBits(psBits, 27, &codeword);
    }

    if (codeword == VO_START_CODE)
    {

        ReadBits(psBits, 32, &codeword);

        /* video_object_layer_start_code                   */
        ReadBits(psBits, 28, &codeword);
        if (codeword != VOL_START_CODE)
        {
            if (psBits->dataBitPos >= (psBits->numBytes << 3))
            {
                return SHORT_HEADER_MODE; /* SH */
            }
            else
            {
                int16 status = 0;
                do
                {
                    /* Search for VOL_HEADER */
                    status = SearchNextM4VFrame(psBits); /* search 0x00 0x00 0x01 */
                    if (status != 0)
                        return MP4_INVALID_VOL_PARAM;

                    status = ReadBits(psBits, VOL_START_CODE_LENGTH, &codeword);
                }
                while ((codeword != VOL_START_CODE) && (status == 0));
                goto decode_vol;
            }
        }
decode_vol:
        /* vol_id (4 bits) */
        ReadBits(psBits, 4, & codeword);


        // RandomAccessibleVOLFlag
        ReadBits(psBits, 1, &codeword);


        //Video Object Type Indication
        ReadBits(psBits, 8, &codeword);
        if (codeword > 2)
        {
            return MP4_INVALID_VOL_PARAM;
        }

        // is_object_layer_identifier
        ReadBits(psBits, 1, &codeword);

        if (codeword)
        {
            ReadBits(psBits, 4, &codeword);
            ReadBits(psBits, 3, &codeword);
        }

        // aspect ratio
        ReadBits(psBits, 4, &codeword);

        if (codeword == 0xF)
        {
            // Extended Parameter
            /* width */
            ReadBits(psBits, 8, &codeword);
            /* height */
            ReadBits(psBits, 8, &codeword);
        }

        ReadBits(psBits, 1, &codeword);

        if (codeword)
        {
            ReadBits(psBits, 2, &codeword);
            if (codeword != 1)
            {
                return MP4_INVALID_VOL_PARAM;
            }

            ReadBits(psBits, 1, &codeword);

            if (!codeword)
            {
                return MP4_INVALID_VOL_PARAM;
            }

            ReadBits(psBits, 1, &codeword);
            if (codeword)   /* if (vbv_parameters) {}, page 36 */
            {
                ReadBits(psBits, 15, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;

                ReadBits(psBits, 15, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;


                ReadBits(psBits, 19, &codeword);
                if (!(codeword & 0x8))
                    return MP4_INVALID_VOL_PARAM;

                ReadBits(psBits, 11, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;

                ReadBits(psBits, 15, &codeword);
                ReadBits(psBits, 1, &codeword);
                if (codeword != 1)
                    return MP4_INVALID_VOL_PARAM;
            }

        }

        ReadBits(psBits, 2, &codeword);

        if (codeword != 0)
        {
            return MP4_INVALID_VOL_PARAM;
        }

        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;

        ReadBits(psBits, 16, &codeword);
        time_increment_resolution = codeword;


        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;



        ReadBits(psBits, 1, &codeword);

        if (codeword && time_increment_resolution > 2)
        {
            i = time_increment_resolution - 1;
            j = 1;
            while (i >>= 1)
            {
                j++;
            }
            nbits_time_increment = j;

            ReadBits(psBits, nbits_time_increment, &codeword);
        }

        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;

        /* this should be 176 for QCIF */
        ReadBits(psBits, 13, &codeword);
        *display_width = (int)codeword;
        ReadBits(psBits, 1, &codeword);
        if (codeword != 1)
            return MP4_INVALID_VOL_PARAM;

        /* this should be 144 for QCIF */
        ReadBits(psBits, 13, &codeword);
        *display_height = (int)codeword;

        *width = (*display_width + 15) & -16;
        *height = (*display_height + 15) & -16;
    }
    else
    {
        /* SHORT_HEADER */
        ShowBits(psBits, SHORT_VIDEO_START_MARKER_LENGTH, &codeword);
        if (codeword == SHORT_VIDEO_START_MARKER)
        {
            iDecodeShortHeader(psBits, width, height, display_width, display_height);
        }
        else
        {
            int16 status = 0;
            do
            {
                /* Search for VOL_HEADER */
                status = SearchNextM4VFrame(psBits); /* search 0x00 0x00 0x01 */
                if (status != 0)
                    return MP4_INVALID_VOL_PARAM;

                status = ReadBits(psBits, VOL_START_CODE_LENGTH, &codeword);
            }
            while ((codeword != VOL_START_CODE) && (status == 0));
            goto decode_vol;
        }
    }
    return 0;
}



OSCL_EXPORT_REF
int16 iDecodeShortHeader(mp4StreamType *psBits,
                         int *width,
                         int *height,
                         int *display_width,
                         int *display_height)
{
    uint32 codeword;
    int	extended_PTYPE = 0;
    int UFEP = 0;
    int custom_PFMT = 0;

    ShowBits(psBits, 22, &codeword);

    if (codeword !=  0x20)
    {
        return MP4_INVALID_VOL_PARAM;
    }
    FlushBits(psBits, 22);
    ReadBits(psBits, 8, &codeword);

    ReadBits(psBits, 1, &codeword);
    if (codeword == 0) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    /* source format */
    ReadBits(psBits, 3, &codeword);
    switch (codeword)
    {
        case 1:
            *width = 128;
            *height = 96;
            break;

        case 2:
            *width = 176;
            *height = 144;
            break;

        case 3:
            *width = 352;
            *height = 288;
            break;

        case 4:
            *width = 704;
            *height = 576;
            break;

        case 5:
            *width = 1408;
            *height = 1152;
            break;

        case 7:
            extended_PTYPE = 1;
            break;
        default:
            /* Msg("H.263 source format not legal\n"); */
            return MP4_INVALID_VOL_PARAM;
    }

    if (extended_PTYPE == 0)
    {
        *display_width = *width;
        *display_height = *height;
        return 0;
    }
    /* source format */
    ReadBits(psBits, 3, &codeword);
    UFEP = codeword;
    if (UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        switch (codeword)
        {
            case 1:
                *width = 128;
                *height = 96;
                break;

            case 2:
                *width = 176;
                *height = 144;
                break;

            case 3:
                *width = 352;
                *height = 288;
                break;

            case 4:
                *width = 704;
                *height = 576;
                break;

            case 5:
                *width = 1408;
                *height = 1152;
                break;

            case 6:
                custom_PFMT = 1;
                break;
            default:
                /* Msg("H.263 source format not legal\n"); */
                return MP4_INVALID_VOL_PARAM;
        }
        if (custom_PFMT == 0)
        {
            *display_width = *width;
            *display_height = *height;
            return 0;
        }
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 3, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM; 			/* RPS, ISD, AIV */
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 4, &codeword);
        if (codeword != 8) return MP4_INVALID_VOL_PARAM;
    }
    if (UFEP == 0 || UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        if (codeword > 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
    }
    else
    {
        return MP4_INVALID_VOL_PARAM;
    }
    ReadBits(psBits, 1, &codeword);
    if (codeword) return MP4_INVALID_VOL_PARAM; /* CPM */
    if (custom_PFMT == 1 && UFEP == 1)
    {
        ReadBits(psBits, 4, &codeword);
        if (codeword == 0) return MP4_INVALID_VOL_PARAM;
        if (codeword == 0xf)
        {
            ReadBits(psBits, 8, &codeword);
            ReadBits(psBits, 8, &codeword);
        }
        ReadBits(psBits, 9, &codeword);
        *display_width = (codeword + 1) << 2;
        *width = (*display_width + 15) & -16;
        ReadBits(psBits, 1, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 9, &codeword);
        if (codeword == 0) return MP4_INVALID_VOL_PARAM;
        *display_height = codeword << 2;
        *height = (*display_height + 15) & -16;
    }

    return 0;
}


int16 ShowBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,          /* nr of bits to read */
    uint32 *pulOutData      /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;

    uint i;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3; /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        if (dataBytePos > pStream->numBytes - 4)
        {
            pStream->bitBuf = 0;
            for (i = 0;i < pStream->numBytes - dataBytePos;i++)
            {
                pStream->bitBuf |= pStream->data[dataBytePos+i];
                pStream->bitBuf <<= 8;
            }
            pStream->bitBuf <<= 8 * (3 - i);
        }
        else
        {
            bits = &pStream->data[dataBytePos];
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        pStream->bitPos = bitPos;
    }

    bitPos += ucNBits;

    *pulOutData = (pStream->bitBuf >> (32 - bitPos)) & mask[(uint16)ucNBits];


    return 0;
}

int16 FlushBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits                      /* number of bits to flush */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;


    if ((dataBitPos + ucNBits) > (uint32)(pStream->numBytes << 3))
        return (-2); // Buffer over run

    dataBitPos += ucNBits;
    bitPos     += ucNBits;

    if (bitPos > 32)
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    pStream->dataBitPos = dataBitPos;
    pStream->bitPos     = bitPos;

    return 0;
}

int16 ReadBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,                     /* nr of bits to read */
    uint32 *pulOutData                 /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;


    if ((dataBitPos + ucNBits) > (pStream->numBytes << 3))
    {
        *pulOutData = 0;
        return (-2); // Buffer over run
    }

    //  dataBitPos += ucNBits;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    pStream->dataBitPos += ucNBits;
    pStream->bitPos      = (unsigned char)(bitPos + ucNBits);

    *pulOutData = (pStream->bitBuf >> (32 - pStream->bitPos)) & mask[(uint16)ucNBits];

    return 0;
}



int16 ByteAlign(
    mp4StreamType *pStream           /* Input Stream */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;
    uint32 leftBits;

    leftBits =  8 - (dataBitPos & 0x7);
    if (leftBits == 8)
    {
        if ((dataBitPos + 8) > (uint32)(pStream->numBytes << 3))
            return (-2); // Buffer over run
        dataBitPos += 8;
        bitPos += 8;
    }
    else
    {
        dataBytePos = dataBitPos >> 3;
        dataBitPos += leftBits;
        bitPos += leftBits;
    }


    if (bitPos > 32)
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        bits = &pStream->data[dataBytePos];
        pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
    }

    pStream->dataBitPos = dataBitPos;
    pStream->bitPos     = bitPos;

    return 0;
}

int16 DecodeUserData(mp4StreamType *pStream)
{

    uint32 codeword;
    int16 iErrorStat;

    iErrorStat = ReadBits(pStream, 32, &codeword);
    if (iErrorStat) return iErrorStat;
    iErrorStat = ShowBits(pStream, 24, &codeword);
    if (iErrorStat) return iErrorStat;

    while (codeword != 1)
    {
        /* Discard user data for now.  CJ 04/05/2000 */
        iErrorStat = ReadBits(pStream, 8, &codeword);
        if (iErrorStat) return iErrorStat;
        iErrorStat = ShowBits(pStream, 24, &codeword);
        if (iErrorStat) return iErrorStat;
    }
    return 0;
}


OSCL_EXPORT_REF int16 iGetAVCConfigInfo(uint8 *buffer, int length, int *width, int *height, int *display_width, int *display_height)
{
    int16 status;
    mp4StreamType psBits;
    uint16 sps_length;
    int size;
    int i = 0;
    uint8* nal_unit = buffer;
    uint8* temp_ptr = NULL;
    uint8* temp = (uint8 *)OSCL_MALLOC(sizeof(uint8) * length);

    if (temp)
    {
        temp_ptr = temp; // Make a copy of the original pointer to be freed later
        // Successfull allocation... copy input buffer
        oscl_memcpy(temp, buffer, length);
    }
    else
    {
        // Allocation failed
        return MP4_INVALID_VOL_PARAM;
    }

    *width = *height = *display_height = *display_width = 0;

    if (temp[0] == 0 && temp[1] == 0)
    {
        /* find SC at the beginning of the NAL */
        while (nal_unit[i++] == 0 && i < length)
        {
        }

        if (nal_unit[i-1] == 1)
        {
            temp = nal_unit + i;
        }
        else
        {
            if (temp_ptr)
            {
                OSCL_FREE(temp_ptr);
            }
            return MP4_INVALID_VOL_PARAM;
        }

        sps_length = length - i;
    }
    else
    {
        sps_length = (uint16)(temp[1] << 8) | temp[0];
        temp += 2;
    }

    if (sps_length > length)
    {
        if (temp_ptr)
        {
            OSCL_FREE(temp_ptr);
        }
        return MP4_INVALID_VOL_PARAM;
    }

    size = sps_length;

#if 0
// FLAGG
    uint32 ii = 0;
    uint32 count = size;
    printf("iGetAVCConfigInfo() before size %d\n", size);
    for (ii = 0; ii < count; ii++)
    {
        printf("%02x ", temp[ii]);
        if ((ii + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
#endif

    Parser_EBSPtoRBSP(temp, &size);

#if 0
    ii = 0;
    count = size;
    printf("iGetAVCConfigInfo() after size %d\n", size);
    for (ii = 0; ii < count; ii++)
    {
        printf("%02x ", temp[ii]);
        if ((ii + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
#endif

    psBits.data = temp;
    psBits.numBytes = size;
    psBits.bitBuf = 0;
    psBits.bitPos = 32;
    psBits.bytePos = 0;
    psBits.dataBitPos = 0;

    status = DecodeSPS(&psBits, width, height, display_width, display_height);
    if (temp_ptr)
    {
        OSCL_FREE(temp_ptr);
    }
    return status;
}

int16 DecodeSPS(mp4StreamType *psBits, int *width, int *height, int *display_width, int *display_height)
{
    uint32 temp;
    int32 temp0;
    uint left_offset, right_offset, top_offset, bottom_offset;
    uint i;

    ReadBits(psBits, 8, &temp);

    if ((temp & 0x1F) != 7) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 8, &temp);
    ReadBits(psBits, 1, &temp);
    ReadBits(psBits, 1, &temp);
    ReadBits(psBits, 1, &temp);
    ReadBits(psBits, 5, &temp);
    ReadBits(psBits, 8, &temp);
    if (temp > 51)
        return MP4_INVALID_VOL_PARAM;


    ue_v(psBits, &temp);
    ue_v(psBits, &temp);
    ue_v(psBits, &temp);

    if (temp == 0)
    {
        ue_v(psBits, &temp);
    }
    else if (temp == 1)
    {
        ReadBits(psBits, 1, &temp);
        se_v(psBits, &temp0);
        se_v(psBits, &temp0);
        ue_v(psBits, &temp);

        for (i = 0; i < temp; i++)
        {
            se_v(psBits, &temp0);
        }
    }
    ue_v(psBits, &temp);


    ReadBits(psBits, 1, &temp);
    ue_v(psBits, &temp);
    *display_width = *width = (temp + 1) << 4;
    ue_v(psBits, &temp);
    *display_height = *height = (temp + 1) << 4;



    ReadBits(psBits, 1, &temp);

    if (!temp)
    {
        ReadBits(psBits, 1, &temp);
    }


    ReadBits(psBits, 1, &temp);

    ReadBits(psBits, 1, &temp);

    if (temp)
    {
        ue_v(psBits, (uint32*)&left_offset);
        ue_v(psBits, (uint32*)&right_offset);
        ue_v(psBits, (uint32*)&top_offset);
        ue_v(psBits, (uint32*)&bottom_offset);

        *display_width = *width - 2 * (right_offset + left_offset);
        *display_height = *height - 2 * (top_offset + bottom_offset);
    }
    return 0;
}

void ue_v(mp4StreamType *psBits, uint32 *codeNum)
{
    uint32 temp;
    uint tmp_cnt;
    int leading_zeros = 0;
    ShowBits(psBits, 16, &temp);
    tmp_cnt = temp  | 0x1;

    PV_CLZ(leading_zeros, tmp_cnt)

    if (leading_zeros < 8)
    {
        *codeNum = (temp >> (15 - (leading_zeros << 1))) - 1;
        FlushBits(psBits, (leading_zeros << 1) + 1);
    }
    else
    {
        ReadBits(psBits, (leading_zeros << 1) + 1, &temp);
        *codeNum = temp - 1;
    }

}


void se_v(mp4StreamType *psBits, int32 *value)
{
    int leadingZeros = 0;
    uint32 temp;

    OSCL_UNUSED_ARG(value);

    ReadBits(psBits, 1, &temp);
    while (!temp)
    {
        leadingZeros++;
        if (ReadBits(psBits, 1, &temp))
        {
            break;
        }
    }
    ReadBits(psBits, leadingZeros, &temp);
}

void Parser_EBSPtoRBSP(uint8 *nal_unit, int *size)
{
    int i, j;
    int count = 0;


    for (i = 0; i < *size; i++)
    {
        if (count == 2 && nal_unit[i] == 0x03)
        {
            break;
        }

        if (nal_unit[i])
            count = 0;
        else
            count++;
    }

    count = 0;
    j = i++;
    for (;i < *size; i++)
    {
        if (count == 2 && nal_unit[i] == 0x03)
        {
            i++;
            count = 0;
        }
        nal_unit[j] = nal_unit[i];
        if (nal_unit[i])
            count = 0;
        else
            count++;
        j++;
    }

    *size = j;
}



