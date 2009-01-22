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
#ifndef _PVDECDEF_H_
#define _PVDECDEF_H_

/********** platform dependent in-line assembly	*****************************/

/*************** Intel *****************/

/*************** ARM *****************/
/* for general ARM instruction. #define __ARM has to be defined in compiler set up.*/
/* for DSP MUL */
#ifdef __TARGET_FEATURE_DSPMUL
#define _ARM_DSP_MUL
#endif

/* for Count Leading Zero instruction */
#ifdef __TARGET_ARCH_5T
#define _ARM_CLZ
#endif
#ifdef __TARGET_ARCH_5TE
#define _ARM_CLZ
#endif
/****************************************************************************/

#ifndef _PV_TYPES_
#define _PV_TYPES_
typedef unsigned char UChar;
typedef char Char;
typedef unsigned int UInt;
typedef int Int;
typedef unsigned short UShort;
typedef short Short;
typedef short int SInt;
typedef unsigned int Bool;
typedef unsigned long	ULong;
typedef void Void;

#define PV_CODEC_INIT		0
#define PV_CODEC_STOP		1
#define PV_CODEC_RUNNING	2
#define PV_CODEC_RESET		3
#endif

typedef enum
{
    PV_SUCCESS,
    PV_FAIL,
    PV_EOS,				/* hit End_Of_Sequence     */
    PV_MB_STUFFING,		/* hit Macroblock_Stuffing */
    PV_END_OF_VOP,		/* hit End_of_Video_Object_Plane */
    PV_END_OF_MB,		/* hit End_of_Macroblock */
    PV_END_OF_BUF		/* hit End_of_Bitstream_Buffer */
} PV_STATUS;

typedef UChar PIXEL;
//typedef Int MOT;   /* : "int" type runs faster on RISC machine */

#define HTFM			/*  3/2/01, Hypothesis Test Fast Matching for early drop-out*/
//#define _MOVE_INTERFACE

//#define RANDOM_REFSELCODE

/* handle the case of devision by zero in RC */
#define MAD_MIN 1

/* 4/11/01, if SSE or MMX, no HTFM, no SAD_HP_FLY */

/* Code size reduction related Macros */
#ifdef H263_ONLY
#ifndef NO_RVLC
#define NO_RVLC
#endif
#ifndef NO_MPEG_QUANT
#define NO_MPEG_QUANT
#endif
#ifndef NO_INTER4V
#define NO_INTER4V
#endif
#endif
/**************************************/

#define TRUE	1
#define FALSE	0

#define PV_ABS(x)		(((x)<0)? -(x) : (x))
#define PV_SIGN(x)		(((x)<0)? -1 : 1)
#define PV_SIGN0(a)		(((a)<0)? -1 : (((a)>0) ? 1 : 0))
#define PV_MAX(a,b)		((a)>(b)? (a):(b))
#define PV_MIN(a,b)		((a)<(b)? (a):(b))

#define MODE_INTRA		0
#define MODE_INTER		1
#define MODE_INTRA_Q	2
#define MODE_INTER_Q	3
#define MODE_INTER4V	4
#define MODE_SKIPPED	6

#define I_VOP		0
#define P_VOP		1
#define B_VOP		2

/*09/04/00 Add MB height and width */
#define MB_WIDTH 16
#define MB_HEIGHT 16

#define VOP_BRIGHT_WHITEENC 255


#define LUMINANCE_DC_TYPE	1
#define CHROMINANCE_DC_TYPE	2

#define EOB_CODE                        1
#define EOB_CODE_LENGTH                32

/* 11/30/98 */
#define FoundRM		1	/* Resync Marker */
#define FoundVSC	2	/* VOP_START_CODE. */
#define FoundGSC	3	/* GROUP_START_CODE */
#define FoundEOB	4	/* EOB_CODE */


/* 05/08/2000, the error code returned from BitstreamShowBits() */
#define BITSTREAM_ERROR_CODE 0xFFFFFFFF

/* PacketVideo "absolution timestamp" object.  06/13/2000 */
#define PVTS_START_CODE 		0x01C4
#define PVTS_START_CODE_LENGTH	32

/* session layer and vop layer start codes */

#define SESSION_START_CODE 	0x01B0
#define SESSION_END_CODE 	0x01B1
#define VISUAL_OBJECT_START_CODE 0x01B5

#define VO_START_CODE 		    0x8
#define VO_HEADER_LENGTH        32		/* lengtho of VO header: VO_START_CODE +  VO_ID */

#define SOL_START_CODE          0x01BE
#define SOL_START_CODE_LENGTH   32

#define VOL_START_CODE 0x12
#define VOL_START_CODE_LENGTH 28

#define VOP_START_CODE 0x1B6
#define VOP_START_CODE_LENGTH	32

#define GROUP_START_CODE	0x01B3
#define GROUP_START_CODE_LENGTH  32

#define VOP_ID_CODE_LENGTH		5
#define VOP_TEMP_REF_CODE_LENGTH	16

#define USER_DATA_START_CODE	    0x01B2
#define USER_DATA_START_CODE_LENGTH 32

#define START_CODE_PREFIX	    0x01
#define START_CODE_PREFIX_LENGTH    24

#define SHORT_VIDEO_START_MARKER         0x20
#define SHORT_VIDEO_START_MARKER_LENGTH  22
#define SHORT_VIDEO_END_MARKER            0x3F
#define GOB_RESYNC_MARKER         0x01
#define GOB_RESYNC_MARKER_LENGTH  17

/* motion and resync markers used in error resilient mode  */

#define DC_MARKER                      438273
#define DC_MARKER_LENGTH                19

#define MOTION_MARKER_COMB             126977
#define MOTION_MARKER_COMB_LENGTH       17

#define MOTION_MARKER_SEP              81921
#define MOTION_MARKER_SEP_LENGTH        17

#define RESYNC_MARKER           1
#define RESYNC_MARKER_LENGTH    17

#define SPRITE_NOT_USED		0
#define STATIC_SPRITE		1
#define ONLINE_SPRITE		2
#define GMC_SPRITE		3

/* macroblock and block size */
#define MB_SIZE 16
#define NCOEFF_MB (MB_SIZE*MB_SIZE)
#define B_SIZE 8
#define NCOEFF_BLOCK (B_SIZE*B_SIZE)
#define NCOEFF_Y NCOEFF_MB
#define NCOEFF_U NCOEFF_BLOCK
#define NCOEFF_V NCOEFF_BLOCK

/* overrun buffer size  */
#define DEFAULT_OVERRUN_BUFFER_SIZE 1000


#if 0
const static Int MBtype_mode[] =
{
    MODE_INTER,
    MODE_INTER_Q,
    MODE_INTER4V,
    MODE_INTRA,
    MODE_INTRA_Q,
    MODE_SKIPPED
};

const static Int mode_MBtype[] =
{
    3,
    0,
    4,
    1,
    2,
};


const static Int  DQ_tab[4] = { -1, -2, 1, 2};

/* Normal zigzag */
const static Int zigzag[NCOEFF_BLOCK] =
{
    0, 1, 5, 6, 14, 15, 27, 28,
    2, 4, 7, 13, 16, 26, 29, 42,
    3, 8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

const static Int zigzag_inv[NCOEFF_BLOCK] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* Horizontal zigzag */
const static Int zigzag_h[NCOEFF_BLOCK] =
{
    0, 1, 2, 3, 10, 11, 12, 13,
    4, 5, 8, 9, 17, 16, 15, 14,
    6, 7, 19, 18, 26, 27, 28, 29,
    20, 21, 24, 25, 30, 31, 32, 33,
    22, 23, 34, 35, 42, 43, 44, 45,
    36, 37, 40, 41, 46, 47, 48, 49,
    38, 39, 50, 51, 56, 57, 58, 59,
    52, 53, 54, 55, 60, 61, 62, 63
};

/* Vertical zigzag */
const static Int zigzag_v[NCOEFF_BLOCK] =
{
    0, 4, 6, 20, 22, 36, 38, 52,
    1, 5, 7, 21, 23, 37, 39, 53,
    2, 8, 19, 24, 34, 40, 50, 54,
    3, 9, 18, 25, 35, 41, 51, 55,
    10, 17, 26, 30, 42, 46, 56, 60,
    11, 16, 27, 31, 43, 47, 57, 61,
    12, 15, 28, 32, 44, 48, 58, 62,
    13, 14, 29, 33, 45, 49, 59, 63
};

/* Horizontal zigzag inverse */
const static Int zigzag_h_inv[NCOEFF_BLOCK] =
{
    0, 1, 2, 3, 8, 9, 16, 17,
    10, 11, 4, 5, 6, 7, 15, 14,
    13, 12, 19, 18, 24, 25, 32, 33,
    26, 27, 20, 21, 22, 23, 28, 29,
    30, 31, 34, 35, 40, 41, 48, 49,
    42, 43, 36, 37, 38, 39, 44, 45,
    46, 47, 50, 51, 56, 57, 58, 59,
    52, 53, 54, 55, 60, 61, 62, 63
};

/* Vertical zigzag inverse */
const static Int zigzag_v_inv[NCOEFF_BLOCK] =
{
    0, 8, 16, 24, 1, 9, 2, 10,
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18, 3, 11, 4, 12,
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28, 5, 13, 6, 14,
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30, 7, 15, 23, 31,
    38, 46, 54, 62, 39, 47, 55, 63
};

/* Inverse normal zigzag */
const static Int zigzag_i[NCOEFF_BLOCK] =
{
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* INTRA */
const static Int mpeg_iqmat_def[NCOEFF_BLOCK] =
    {  8, 17, 18, 19, 21, 23, 25, 27,
       17, 18, 19, 21, 23, 25, 27, 28,
       20, 21, 22, 23, 24, 26, 28, 30,
       21, 22, 23, 24, 26, 28, 30, 32,
       22, 23, 24, 26, 28, 30, 32, 35,
       23, 24, 26, 28, 30, 32, 35, 38,
       25, 26, 28, 30, 32, 35, 38, 41,
       27, 28, 30, 32, 35, 38, 41, 45
    };

/* INTER */
const static Int mpeg_nqmat_def[64]  =
    { 16, 17, 18, 19, 20, 21, 22, 23,
      17, 18, 19, 20, 21, 22, 23, 24,
      18, 19, 20, 21, 22, 23, 24, 25,
      19, 20, 21, 22, 23, 24, 26, 27,
      20, 21, 22, 23, 25, 26, 27, 28,
      21, 22, 23, 24, 26, 27, 28, 30,
      22, 23, 24, 26, 27, 28, 30, 31,
      23, 24, 25, 27, 28, 30, 31, 33
    };

const static Int roundtab4[] = {0, 1, 1, 1};
const static Int roundtab8[] = {0, 0, 1, 1, 1, 1, 1, 2};
/*** 10/30 for TPS */
const static Int roundtab12[] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2};
/* 10/30 for TPS ***/
const static Int roundtab16[] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
#endif
/* VLC decoding related definitions */
#define VLC_ERROR	(-1)
#define VLC_ESCAPE  7167

#endif /* _PVDECDEF_H_ */
