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
/**
This file contains application function interfaces to the AVC encoder library
and necessary type defitionitions and enumerations.
@publishedAll
*/

#ifndef AVCENC_INT_H_INCLUDED
#define AVCENC_INT_H_INCLUDED

#ifndef AVCINT_COMMON_H_INCLUDED
#include "avcint_common.h"
#endif
#ifndef AVCENC_API_H_INCLUDED
#include "avcenc_api.h"
#endif

/* Definition for the structures below */
#define DEFAULT_ATTR	0 /* default memory attribute */
#define MAX_INPUT_FRAME 30 /* some arbitrary number, it can be much higher than this. */
#define MAX_REF_FRAME  16 /* max size of the RefPicList0 and RefPicList1 */
#define MAX_REF_PIC_LIST 33

#define MIN_QP          0
#define MAX_QP          51
#define SHIFT_QP        12
#define  LAMBDA_ACCURACY_BITS         16
#define  LAMBDA_FACTOR(lambda)        ((int)((double)(1<<LAMBDA_ACCURACY_BITS)*lambda+0.5))


#define DISABLE_THRESHOLDING  0
// for better R-D performance
#define _LUMA_COEFF_COST_       4 //!< threshold for luma coeffs
#define _CHROMA_COEFF_COST_     4 //!< threshold for chroma coeffs, used to be 7
#define _LUMA_MB_COEFF_COST_    5 //!< threshold for luma coeffs of inter Macroblocks
#define _LUMA_8x8_COEFF_COST_   5 //!< threshold for luma coeffs of 8x8 Inter Partition
#define MAX_VALUE       999999   //!< used for start value for some variables

#define  WEIGHTED_COST(factor,bits)   (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
#define  MV_COST(f,s,cx,cy,px,py)     (WEIGHTED_COST(f,mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))
#define  MV_COST_S(f,cx,cy,px,py)     (WEIGHTED_COST(f,mvbits[cx-px]+mvbits[cy-py]))

/* for sub-pel search and interpolation */
#define SUBPEL_PRED_BLK_SIZE 576 // 24x24
#define REF_CENTER 75
#define V3Q_H0Q 1
#define V3Q_H1Q 2
#define V0Q_H1Q 3
#define V1Q_H1Q 4
#define V1Q_H0Q 5
#define V1Q_H3Q 6
#define V0Q_H3Q 7
#define V3Q_H3Q 8
#define V2Q_H3Q 9
#define V2Q_H0Q 10
#define V2Q_H1Q 11
#define V2Q_H2Q 12
#define V3Q_H2Q 13
#define V0Q_H2Q 14
#define V1Q_H2Q 15

// associated with the above cost model
const uint8 COEFF_COST[2][16] =
{
    {3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9}
};



//! convert from H.263 QP to H.264 quant given by: quant=pow(2,QP/6)
const int QP2QUANT[40] =
{
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 4, 4, 4, 5, 6,
    6, 7, 8, 9, 10, 11, 13, 14,
    16, 18, 20, 23, 25, 29, 32, 36,
    40, 45, 51, 57, 64, 72, 81, 91
};


/**
This enumeration keeps track of the internal status of the encoder whether it is doing
something. The encoding flow follows the order in which these states are.
@publishedAll
*/
typedef enum
{
    AVCEnc_Initializing = 0,
    AVCEnc_Encoding_SPS,
    AVCEnc_Encoding_PPS,
    AVCEnc_Analyzing_Frame,
    AVCEnc_WaitingForBuffer,  // pending state
    AVCEnc_Encoding_Frame,
} AVCEnc_State ;

/**
Bitstream structure contains bitstream related parameters such as the pointer
to the buffer, the current byte position and bit position. The content of the
bitstreamBuffer will be in EBSP format as the emulation prevention codes are
automatically inserted as the RBSP is recorded.
@publishedAll
*/
typedef struct tagEncBitstream
{
    uint8 *bitstreamBuffer; /* pointer to buffer memory   */
    int buf_size;		/* size of the buffer memory */
    int write_pos;		/* next position to write to bitstreamBuffer  */
    int count_zeros;   /* count number of consecutive zero */
    uint current_word;	/* byte-swapped (MSB left) current word to write to buffer */
    int bit_left;      /* number of bit left in current_word */
} AVCEncBitstream;

/**
This structure is used for rate control purpose and other performance related control
variables such as, RD cost, statistics, motion search stuffs, etc.
should be in this structure.
@publishedAll
*/
typedef struct tagAVCRateControl
{

    /* these parameters are initialized by the users AVCEncParams */
    /* bitrate-robustness tradeoff */
    uint scdEnable; /* enable scene change detection */
    int idrPeriod;  /* IDR period in number of frames */
    int	intraMBRate;   /* intra MB refresh rate per frame */
    uint dpEnable;	/* enable data partitioning */

    /* quality-complexity tradeoff */
    uint subPelEnable;  /* enable quarter pel search */
    int	mvRange;	/* motion vector search range in +/- pixel */
    uint subMBEnable;  /* enable sub MB prediction mode (4x4, 4x8, 8x4) */
    uint rdOptEnable;  /* enable RD-opt mode selection */
    uint twoPass; /* flag for 2 pass encoding ( for future )*/
    uint bidirPred; /* bi-directional prediction for B-frame. */

    uint rcEnable;  /* enable rate control, '1' on, '0' const QP */
    int initQP;	/* initial QP */

    /* note the following 3 params are for HRD, these triplets can be a series
    of triplets as the generalized HRD allows. SEI message must be generated in this case. */
    /* We no longer have to differentiate between CBR and VBR. The users to the
    AVC encoder lib will do the mapping from CBR/VBR to these parameters. */
    int32 bitRate;	/* target bit rate for the overall clip in bits/second*/
    int32 cpbSize;	/* coded picture buffer size in bytes */
    int32 initDelayOffset; /* initial CBP removal delay in bits */

    OsclFloat frame_rate; /* frame rate */
    int srcInterval; /* source frame rate in msec */
    int basicUnit;  /* number of macroblocks per BU */

    /* Then internal parameters for the operation */
    uint first_frame; /* a flag for the first frame */
    int lambda_mf; /* for example */
    int totalSAD;	 /* SAD of current frame */

    /* rate control variables according to JVT document K-049 */
    /* just take the variable names first. after it's working, cleanup names later */

    int NumberofCodedPFrame;
    int NumberofGOP;
    int TotalQpforPPicture;
    int NumberofPPicture;
    int NumberofCodedMacroBlocks;
    int NumberofHeaderBits;
    int NumberofTextureBits;
    int NumberofBasicUnitHeaderBits;
    int NumberofBasicUnitTextureBits;
    double TotalMADBasicUnit;
    double *MADofMB;

    int numFrameBits; /* keep track of number of bits of the current frame */
    int numMBHeaderBits;
    int	numMBTextureBits;

    /* rate control model parameters */
    int R;
    int Np;
    int32 T;
    double DeltaP;
    int32 bitsPerFrame;
    int32 CurrentBufferFullness;
    int32 TargetBufferLevel;

    //HRD consideration
    int   curr_skip;
    int32 UpperBound1;
    int32 LowerBound;

    /*quadratic rate-distortion model*/
    int32 Wp;
    double PMADPictureC1;
    double PMADPictureC2;
    double PPictureMAD[21];
    double ReferenceMAD[21];
    double m_rgQp[20];
    double m_rgRp[20];
    double m_X1;
    double m_X2;
    int m_Qc;
    int	Pm_Qp;
    int PPreHeader;

    int TotalFrameQP;
    int NumberofBasicUnit;
    int PAveHeaderBits1;
    int PAveHeaderBits2;
    int PAveHeaderBits3;
    int PAveFrameQP;
    int TotalNumberofBasicUnit;

    double CurrentFrameMAD;
    double PreviousFrameMAD;
    double PreviousWholeFrameMAD;

    int m_windowSize;
    int MADm_windowSize;
    int DDquant;

    int QPLastPFrame;
    int QPLastGOP;
    bool GOPOverdue;

    double *BUPFMAD;
    double *BUCFMAD;

    /* RC3 additional variables */
    int RCISliceBits;
    int RCPSliceBits;
    double RCISliceBitRatio;

} AVCRateControl;


/**
This structure is for the motion vector information. */
typedef struct tagMV
{
    int x;
    int y;
    uint sad;
} AVCMV;

/**
This structure contains function pointers for different platform dependent implementation of
functions. */
typedef struct tagAVCEncFuncPtr
{

    int (*SAD_MB_HalfPel[4])(uint8*, uint8*, int, void *);
    int (*SAD_Macroblock)(uint8 *ref, uint8 *blk, int dmin_lx, void *extra_info);

} AVCEncFuncPtr;

/**
This structure contains information necessary for correct padding.
*/
typedef struct tagPadInfo
{
    int i;
    int width;
    int j;
    int height;
} AVCPadInfo;


#ifdef HTFM
typedef struct tagHTFM_Stat
{
    int abs_dif_mad_avg;
    uint countbreak;
    int offsetArray[16];
    int offsetRef[16];
} HTFM_Stat;
#endif


/**
This structure is the main object for AVC encoder library providing access to all
global variables. It is allocated at PVAVCInitEncoder and freed at PVAVCCleanUpEncoder.
@publishedAll
*/
typedef struct tagEncObject
{

    AVCCommonObj *common;

    AVCEncBitstream		*bitstream; /* for current NAL */

    /* rate control */
    AVCRateControl		*rateCtrl; /* pointer to the rate control structure */

    /* encoding operation */
    AVCEnc_State		enc_state; /* encoding state */

    AVCFrameIO			*currInput; /* pointer to the current input frame */

    int					currSliceGroup; /* currently encoded slice group id */

    int		level[24][16], run[24][16]; /* scratch memory */
    int     leveldc[16], rundc[16]; /* for DC component */
    int		levelcdc[16], runcdc[16]; /* for chroma DC component */
    int		numcoefcdc[2]; /* number of coefficient for chroma DC */
    int		numcoefdc;		/* number of coefficients for DC component */

    int     qp_const;
    int		qp_const_c;
    /********* intra prediction scratch memory **********************/
    uint8   pred_i16[AVCNumI16PredMode][256]; /* save prediction for MB */
    uint8   pred_i4[AVCNumI4PredMode][16];  /* save prediction for blk */
    uint8	pred_ic[AVCNumIChromaMode][128];  /* for 2 chroma */

    int		mostProbableI4Mode[16]; /* in raster scan order */
    /********* motion compensation related variables ****************/
    AVCMV	*mot16x16;			/* Saved motion vectors for 16x16 block*/
    AVCMV(*mot16x8)[2];		/* Saved motion vectors for 16x8 block*/
    AVCMV(*mot8x16)[2];		/* Saved motion vectors for 8x16 block*/
    AVCMV(*mot8x8)[4];		/* Saved motion vectors for 8x8 block*/

    /********* subpel position **************************************/
    uint32  subpel_pred[SUBPEL_PRED_BLK_SIZE<<2]; /* all 16 sub-pel positions  */
    uint8   *hpel_cand[9];		/* pointer to half-pel position */
    int		best_hpel_pos;			/* best position */
    uint8   *qpel_cand[9][8];		/* pointer to quarter-pel position */
    int		best_qpel_pos;

    /* need for intra refresh rate */
    uint8   *intraSearch;		/* Intra Array for MBs to be intra searched */
    uint    firstIntraRefreshMBIndx; /* keep track for intra refresh */

    int     i4_sad;				/* temporary for i4 mode SAD */
    int		*min_cost;			/* Minimum cost for the all MBs */
    int		lambda_mode;		/* Lagrange parameter for mode selection */
    int		lambda_motion;		/* Lagrange parameter for MV selection */

    uint8	*mvbits_array;		/* Table for bits spent in the cost funciton */
    uint8	*mvbits;			/* An offset to the above array. */

    /* to speedup the SAD calculation */
    void *sad_extra_info;
    uint8 currYMB[256];		/* interleaved current macroblock in HTFM order */

#ifdef HTFM
    int nrmlz_th[48];		/* Threshold for fast SAD calculation using HTFM */
    HTFM_Stat htfm_stat;    /* For statistics collection */
#endif

    /* statistics */
    int	numIntraMB;			/* keep track of number of intra MB */
    int numFalseAlarm;
    int numMisDetected;
    int numDetected;

    /* encoding complexity control */
    uint fullsearch_enable; /* flag to enable full-pel full-search */

    /* misc.*/
    bool outOfBandParamSet; /* flag to enable out-of-band param set */

    /* time control */
    uint32	prevFrameNum;	/* previous frame number starting from modTimeRef */
    uint32	modTimeRef;		/* Reference modTime update every I-Vop*/
    uint32  wrapModTime;    /* Offset to modTime Ref, rarely used */

    uint	prevProcFrameNum;  /* previously processed frame number, could be skipped */
    uint	prevCodedFrameNum;	/* previously encoded frame number */
    /* POC related variables */
    uint32	dispOrdPOCRef;		/* reference POC is displayer order unit. */

    /* Function pointers */
    AVCEncFuncPtr *functionPointer;	/* store pointers to platform specific functions */

    /* Application control data */
    AVCHandle *avcHandle;


} AVCEncObject;


#endif /*AVCENC_INT_H_INCLUDED*/

