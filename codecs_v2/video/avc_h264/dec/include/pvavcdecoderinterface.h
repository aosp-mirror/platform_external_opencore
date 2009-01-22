/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#ifndef PVAVCDECODERINTERFACE_H_INCLUDED
#define PVAVCDECODERINTERFACE_H_INCLUDED

// includes
#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

typedef void (*FunctionType_Unbind)(void *, int);
typedef int (*FunctionType_Alloc)(void *, int, uint8 **);
typedef int (*FunctionType_SPS)(void *, uint, uint);
typedef int (*FunctionType_Malloc)(void *, int32, int);
typedef void(*FunctionType_Free)(void *, int);

#if 0
typedef enum
{
    AVC_NALTYPE_SLICE = 1,	/* non-IDR non-data partition */
    AVC_NALTYPE_DPA = 2,	/* data partition A */
    AVC_NALTYPE_DPB = 3,	/* data partition B */
    AVC_NALTYPE_DPC = 4,	/* data partition C */
    AVC_NALTYPE_IDR = 5,	/* IDR NAL */
    AVC_NALTYPE_SEI = 6,	/* supplemental enhancement info */
    AVC_NALTYPE_SPS = 7,	/* sequence parameter set */
    AVC_NALTYPE_PPS = 8,	/* picture parameter set */
    AVC_NALTYPE_AUD = 9,	/* access unit delimiter */
    AVC_NALTYPE_EOSEQ = 10,	/* end of sequence */
    AVC_NALTYPE_EOSTREAM = 11, /* end of stream */
    AVC_NALTYPE_FILL = 12	/* filler data */
} AVCNalUnitType;


typedef enum
{
    /**
    The followings are fail with details. Their values are negative.
    */
    AVCDEC_NO_DATA = -4,
    AVCDEC_PACKET_LOSS = -3,
    /**
    Fail information
    */
    AVCDEC_NO_BUFFER = -2, /* no output picture buffer available */
    AVCDEC_MEMORY_FAIL = -1, /* memory allocation failed */
    AVCDEC_FAIL = 0,
    /**
    Generic success value
    */
    AVCDEC_SUCCESS = 1,
    AVCDEC_PICTURE_OUTPUT_READY = 2,
    AVCDEC_PICTURE_READY = 3,

    /**
    The followings are success with warnings. Their values are positive integers.
    */
    AVCDEC_NO_NEXT_SC = 4,
    AVCDEC_REDUNDANT_FRAME = 5,
    AVCDEC_CONCEALED_FRAME = 6	/* detect and conceal the error */
} AVCDec_Status;
#endif

// PVAVCDecoderInterface pure virtual interface class
class PVAVCDecoderInterface
{
    public:
        virtual ~PVAVCDecoderInterface() {};
        virtual bool	InitAVCDecoder(FunctionType_SPS, FunctionType_Alloc, FunctionType_Unbind,
                                    FunctionType_Malloc, FunctionType_Free, void *) = 0;
        virtual void	CleanUpAVCDecoder(void) = 0;
        virtual void    ResetAVCDecoder(void) = 0;
        virtual int32	DecodeSPS(uint8 *bitstream, int32 buffer_size) = 0;
        virtual int32	DecodePPS(uint8 *bitstream, int32 buffer_size) = 0;
        virtual int32	DecodeAVCSlice(uint8 *bitstream, int32 *buffer_size) = 0;
        virtual bool	GetDecOutput(int *indx, int *release) = 0;
        virtual void	GetVideoDimensions(int32 *width, int32 *height, int32 *top, int32 *left, int32 *bottom, int32 *right) = 0;
//	virtual int		AVC_Malloc(int32 size, int attribute);
//	virtual void	AVC_Free(int mem);
};

#endif // PVAVCDECODERINTERFACE_H_INCLUDED


