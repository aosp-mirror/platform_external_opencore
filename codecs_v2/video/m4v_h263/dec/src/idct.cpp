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
------------------------------------------------------------------------------
 MODULE DESCRIPTION

 This file contains the functions that transform an 8x8 image block from
 dequantized DCT coefficients to spatial domain pixel values by calculating
 inverse discrete cosine transform (IDCT).

------------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/
#include "mp4dec_lib.h"
#include "idct.h"
#include "motion_comp.h"
#ifndef FAST_IDCT

/*
------------------------------------------------------------------------------
 FUNCTION NAME: idct
------------------------------------------------------------------------------
 INPUT AND OUTPUT DEFINITIONS FOR idct

 Inputs:
 	blk = pointer to the buffer containing the dequantized DCT
	      coefficients of type int for an 8x8 image block;
	      values range from (-2048, 2047) which defined as standard.

 Local Stores/Buffers/Pointers Needed:
	None

 Global Stores/Buffers/Pointers Needed:
	None

 Outputs:
	None

 Pointers and Buffers Modified:
	blk points to the found IDCT values for an 8x8 image block.

 Local Stores Modified:
	None

 Global Stores Modified:
	None

------------------------------------------------------------------------------
 FUNCTION DESCRIPTION FOR idct

 This function transforms an 8x8 image block from dequantized DCT coefficients
 (F(u,v)) to spatial domain pixel values (f(x,y)) by performing the two
 dimensional inverse discrete cosine transform (IDCT).

		 _7_ _7_      C(u) C(v)
	f(x,y) = \   \	F(u,v)---- ----cos[(2x+1)*u*pi/16]cos[(2y+1)*v*pi/16]
		 /__ /__	2    2
		 u=0 v=0

	where	C(i) = 1/sqrt(2)	if i=0
		C(i) = 1		otherwise

 2-D IDCT can be separated as horizontal(row-wise) and vertical(column-wise)
 1-D IDCTs. Therefore, 2-D IDCT values are found by the following two steps:
 1. Find horizontal 1-D IDCT values for each row from 8x8 dequantized DCT
    coefficients by row IDCT operation.

 		  _7_        C(u)
	g(x,v) =  \   F(u,v) ---- cos[(2x+1)*u*pi/16]
		  /__ 	      2
		  u=0

 2. Find vertical 1-D IDCT values for each column from the results of 1
    by column IDCT operation.

    		  _7_ 	     C(v)
	f(x,y) =  \   g(x,v) ---- cos[(2y+1)*v*pi/16]
		  /__ 	      2
		  v=0

------------------------------------------------------------------------------
 REQUIREMENTS FOR idct

 None

------------------------------------------------------------------------------
*/
/*  REFERENCES FOR idct	*/
/* idct.c, inverse fast discrete cosine transform
 inverse two dimensional DCT, Chen-Wang algorithm
 (cf. IEEE ASSP-32, pp. 803-816, Aug. 1984)
 32-bit integer arithmetic (8 bit coefficients)
 11 mults, 29 adds per DCT
 sE, 18.8.91

 coefficients extended to 12 bit for IEEE1180-1990
 compliance                           sE,  2.1.94
*/


/*----------------------------------------------------------------------------
; Function Code FOR idct
----------------------------------------------------------------------------*/
void idct_intra(
    int *blk, uint8 *comp, int width
)
{
    /*----------------------------------------------------------------------------
    ; Define all local variables
    ----------------------------------------------------------------------------*/
    int	i;
    int32	tmpBLK[64];
    int32	*tmpBLK32 = &tmpBLK[0];
    int32	x0, x1, x2, x3, x4, x5, x6, x7, x8;	/* butterfly nodes */
    int32	a;
    int offset = width - 8;
    /*----------------------------------------------------------------------------
    ; Function body here
    ----------------------------------------------------------------------------*/
    /* two dimensional inverse discrete cosine transform */


    /* column (vertical) IDCT */
    for (i = B_SIZE - 1; i >= 0; i--)
    {
        /* initialize butterfly nodes at first stage */

        x1 = blk[B_SIZE * 4 + i] << 11;
        /* since row IDCT results have net left shift by 3 */
        /* this left shift by 8 gives net left shift by 11 */
        /* in order to maintain the same scale as that of  */
        /* coefficients Wi */

        x2 = blk[B_SIZE * 6 + i];
        x3 = blk[B_SIZE * 2 + i];
        x4 = blk[B_SIZE * 1 + i];
        x5 = blk[B_SIZE * 7 + i];
        x6 = blk[B_SIZE * 5 + i];
        x7 = blk[B_SIZE * 3 + i];

        if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
        {
            /* shortcut */
            /* execute if values of g(x,1) to g(x,7) in a column*/
            /* are all zeros */

            /* make output of IDCT >>3 or scaled by 1/8 and */
            /* with the proper rounding */
            a = (blk[B_SIZE * 0 + i]) << 3;
            tmpBLK32[B_SIZE * 0 + i] = a;
            tmpBLK32[B_SIZE * 1 + i] = a;
            tmpBLK32[B_SIZE * 2 + i] = a;
            tmpBLK32[B_SIZE * 3 + i] = a;
            tmpBLK32[B_SIZE * 4 + i] = a;
            tmpBLK32[B_SIZE * 5 + i] = a;
            tmpBLK32[B_SIZE * 6 + i] = a;
            tmpBLK32[B_SIZE * 7 + i] = a;
        }
        else
        {
            x0 = (blk[8 * 0 + i] << 11) + 128;

            /* first stage */

            x8 = W7 * (x4 + x5);
            x4 = (x8 + (W1 - W7) * x4);
            /* Multiplication with Wi increases the net left */
            /* shift from 11 to 14,we have to shift back by 3*/
            x5 = (x8 - (W1 + W7) * x5);
            x8 = W3 * (x6 + x7);
            x6 = (x8 - (W3 - W5) * x6);
            x7 = (x8 - (W3 + W5) * x7);

            /* second stage */
            x8 = x0 + x1;
            x0 -= x1;

            x1 = W6 * (x3 + x2);
            x2 = (x1 - (W2 + W6) * x2);
            x3 = (x1 + (W2 - W6) * x3);

            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            /* third stage */
            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (181 * (x4 + x5) + 128) >> 8;	/* rounding */
            x4 = (181 * (x4 - x5) + 128) >> 8;

            /* fourth stage */
            /* net shift of IDCT is >>3 after the following	*/
            /* shift operation, it makes output of 2-D IDCT */
            /* scaled by 1/8, that is scaled twice by       */
            /* 1/(2*sqrt(2)) for row IDCT and column IDCT.  */
            /* see detail analysis in design doc.	        */
            tmpBLK32[0 + i] = (x7 + x1) >> 8;
            tmpBLK32[(1<<3) + i] = (x3 + x2) >> 8;
            tmpBLK32[(2<<3) + i] = (x0 + x4) >> 8;
            tmpBLK32[(3<<3) + i] = (x8 + x6) >> 8;
            tmpBLK32[(4<<3) + i] = (x8 - x6) >> 8;
            tmpBLK32[(5<<3) + i] = (x0 - x4) >> 8;
            tmpBLK32[(6<<3) + i] = (x3 - x2) >> 8;
            tmpBLK32[(7<<3) + i] = (x7 - x1) >> 8;
        }
    }
    /* row (horizontal) IDCT */
    for (i = 0 ; i < B_SIZE;i++)
    {
        /* initialize butterfly nodes at the first stage */

        x1 = ((int32)tmpBLK32[4+(i<<3)]) << 8;
        /* x1 left shift by 11 is to maintain the same	*/
        /* scale as that of coefficients (W1,...W7)	*/
        /* since blk[4] won't multiply with Wi.		*/
        /* see detail diagram in design document.	*/

        x2 = tmpBLK32[6+(i<<3)];
        x3 = tmpBLK32[2+(i<<3)];
        x4 = tmpBLK32[1+(i<<3)];
        x5 = tmpBLK32[7+(i<<3)];
        x6 = tmpBLK32[5+(i<<3)];
        x7 = tmpBLK32[3+(i<<3)];

        if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
        {
            /* shortcut */
            /* execute if values of F(1,v) to F(7,v) in a row*/
            /* are all zeros */

            /* output of row IDCT scaled by 8 */
            a = (((int32)tmpBLK32[0+(i<<3)] + 32) >> 6);
            CLIP_RESULT(a)
            *comp++ = a;
            *comp++ = a;
            *comp++ = a;
            *comp++ = a;
            *comp++ = a;
            *comp++ = a;
            *comp++ = a;
            *comp++ = a;

            comp += offset;
        }

        else
        {
            /* for proper rounding in the fourth stage */
            x0 = (((int32)tmpBLK32[0+(i<<3)]) << 8) + 8192;

            /* first stage */

            x8 = W7 * (x4 + x5) + 4;
            x4 = (x8 + (W1 - W7) * x4) >> 3;
            x5 = (x8 - (W1 + W7) * x5) >> 3;

            x8 = W3 * (x6 + x7) + 4;
            x6 = (x8 - (W3 - W5) * x6) >> 3;
            x7 = (x8 - (W3 + W5) * x7) >> 3;

            /* second stage */
            x8 = x0 + x1;
            x0 -= x1;

            x1 = W6 * (x3 + x2) + 4;
            x2 = (x1 - (W2 + W6) * x2) >> 3;
            x3 = (x1 + (W2 - W6) * x3) >> 3;

            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            /* third stage */
            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (181 * (x4 + x5) + 128) >> 8;    /* rounding */
            x4 = (181 * (x4 - x5) + 128) >> 8;

            /* fourth stage */
            /* net shift of this function is <<3 after the	  */
            /* following shift operation, it makes output of  */
            /* row IDCT scaled by 8 to retain 3 bits precision*/
            a = ((x7 + x1) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x3 + x2) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x0 + x4) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x8 + x6) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x8 - x6) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x0 - x4) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x3 - x2) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;
            a = ((x7 - x1) >> 14);
            CLIP_RESULT(a)
            *comp++ = a;

            comp += offset;
        }
    }



    /*----------------------------------------------------------------------------
    ; Return nothing or data or data pointer
    ----------------------------------------------------------------------------*/
    return;
}

void idct(
    int *blk, uint8 *pred, uint8 *dst, int width)
{
    /*----------------------------------------------------------------------------
    ; Define all local variables
    ----------------------------------------------------------------------------*/
    int	i;
    int32	tmpBLK[64];
    int32	*tmpBLK32 = &tmpBLK[0];
    int32	x0, x1, x2, x3, x4, x5, x6, x7, x8;	/* butterfly nodes */
    int32	a;
    int res;

    /*----------------------------------------------------------------------------
    ; Function body here
    ----------------------------------------------------------------------------*/
    /* two dimensional inverse discrete cosine transform */


    /* column (vertical) IDCT */
    for (i = B_SIZE - 1; i >= 0; i--)
    {
        /* initialize butterfly nodes at first stage */

        x1 = blk[B_SIZE * 4 + i] << 11;
        /* since row IDCT results have net left shift by 3 */
        /* this left shift by 8 gives net left shift by 11 */
        /* in order to maintain the same scale as that of  */
        /* coefficients Wi */

        x2 = blk[B_SIZE * 6 + i];
        x3 = blk[B_SIZE * 2 + i];
        x4 = blk[B_SIZE * 1 + i];
        x5 = blk[B_SIZE * 7 + i];
        x6 = blk[B_SIZE * 5 + i];
        x7 = blk[B_SIZE * 3 + i];

        if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
        {
            /* shortcut */
            /* execute if values of g(x,1) to g(x,7) in a column*/
            /* are all zeros */

            /* make output of IDCT >>3 or scaled by 1/8 and */
            /* with the proper rounding */
            a = (blk[B_SIZE * 0 + i]) << 3;
            tmpBLK32[B_SIZE * 0 + i] = a;
            tmpBLK32[B_SIZE * 1 + i] = a;
            tmpBLK32[B_SIZE * 2 + i] = a;
            tmpBLK32[B_SIZE * 3 + i] = a;
            tmpBLK32[B_SIZE * 4 + i] = a;
            tmpBLK32[B_SIZE * 5 + i] = a;
            tmpBLK32[B_SIZE * 6 + i] = a;
            tmpBLK32[B_SIZE * 7 + i] = a;
        }
        else
        {
            x0 = (blk[8 * 0 + i] << 11) + 128;

            /* first stage */

            x8 = W7 * (x4 + x5);
            x4 = (x8 + (W1 - W7) * x4);
            /* Multiplication with Wi increases the net left */
            /* shift from 11 to 14,we have to shift back by 3*/
            x5 = (x8 - (W1 + W7) * x5);
            x8 = W3 * (x6 + x7);
            x6 = (x8 - (W3 - W5) * x6);
            x7 = (x8 - (W3 + W5) * x7);

            /* second stage */
            x8 = x0 + x1;
            x0 -= x1;

            x1 = W6 * (x3 + x2);
            x2 = (x1 - (W2 + W6) * x2);
            x3 = (x1 + (W2 - W6) * x3);

            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            /* third stage */
            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (181 * (x4 + x5) + 128) >> 8;	/* rounding */
            x4 = (181 * (x4 - x5) + 128) >> 8;

            /* fourth stage */
            /* net shift of IDCT is >>3 after the following	*/
            /* shift operation, it makes output of 2-D IDCT */
            /* scaled by 1/8, that is scaled twice by       */
            /* 1/(2*sqrt(2)) for row IDCT and column IDCT.  */
            /* see detail analysis in design doc.	        */
            tmpBLK32[0 + i] = (x7 + x1) >> 8;
            tmpBLK32[(1<<3) + i] = (x3 + x2) >> 8;
            tmpBLK32[(2<<3) + i] = (x0 + x4) >> 8;
            tmpBLK32[(3<<3) + i] = (x8 + x6) >> 8;
            tmpBLK32[(4<<3) + i] = (x8 - x6) >> 8;
            tmpBLK32[(5<<3) + i] = (x0 - x4) >> 8;
            tmpBLK32[(6<<3) + i] = (x3 - x2) >> 8;
            tmpBLK32[(7<<3) + i] = (x7 - x1) >> 8;
        }
    }
    /* row (horizontal) IDCT */
    for (i = B_SIZE - 1; i >= 0; i--)
    {
        /* initialize butterfly nodes at the first stage */

        x1 = ((int32)tmpBLK32[4+(i<<3)]) << 8;
        /* x1 left shift by 11 is to maintain the same	*/
        /* scale as that of coefficients (W1,...W7)	*/
        /* since blk[4] won't multiply with Wi.		*/
        /* see detail diagram in design document.	*/

        x2 = tmpBLK32[6+(i<<3)];
        x3 = tmpBLK32[2+(i<<3)];
        x4 = tmpBLK32[1+(i<<3)];
        x5 = tmpBLK32[7+(i<<3)];
        x6 = tmpBLK32[5+(i<<3)];
        x7 = tmpBLK32[3+(i<<3)];

        if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7))
        {
            /* shortcut */
            /* execute if values of F(1,v) to F(7,v) in a row*/
            /* are all zeros */

            /* output of row IDCT scaled by 8 */
            a = (tmpBLK32[0+(i<<3)] + 32) >> 6;
            blk[0+(i<<3)] = a;
            blk[1+(i<<3)] = a;
            blk[2+(i<<3)] = a;
            blk[3+(i<<3)] = a;
            blk[4+(i<<3)] = a;
            blk[5+(i<<3)] = a;
            blk[6+(i<<3)] = a;
            blk[7+(i<<3)] = a;

        }

        else
        {
            /* for proper rounding in the fourth stage */
            x0 = (((int32)tmpBLK32[0+(i<<3)]) << 8) + 8192;

            /* first stage */

            x8 = W7 * (x4 + x5) + 4;
            x4 = (x8 + (W1 - W7) * x4) >> 3;
            x5 = (x8 - (W1 + W7) * x5) >> 3;

            x8 = W3 * (x6 + x7) + 4;
            x6 = (x8 - (W3 - W5) * x6) >> 3;
            x7 = (x8 - (W3 + W5) * x7) >> 3;

            /* second stage */
            x8 = x0 + x1;
            x0 -= x1;

            x1 = W6 * (x3 + x2) + 4;
            x2 = (x1 - (W2 + W6) * x2) >> 3;
            x3 = (x1 + (W2 - W6) * x3) >> 3;

            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            /* third stage */
            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (181 * (x4 + x5) + 128) >> 8;    /* rounding */
            x4 = (181 * (x4 - x5) + 128) >> 8;

            /* fourth stage */
            /* net shift of this function is <<3 after the	  */
            /* following shift operation, it makes output of  */
            /* row IDCT scaled by 8 to retain 3 bits precision*/
            blk[0+(i<<3)] = (x7 + x1) >> 14;
            blk[1+(i<<3)] = (x3 + x2) >> 14;
            blk[2+(i<<3)] = (x0 + x4) >> 14;
            blk[3+(i<<3)] = (x8 + x6) >> 14;
            blk[4+(i<<3)] = (x8 - x6) >> 14;
            blk[5+(i<<3)] = (x0 - x4) >> 14;
            blk[6+(i<<3)] = (x3 - x2) >> 14;
            blk[7+(i<<3)] = (x7 - x1) >> 14;
        }
        /*  add with prediction ,  08/03/05 */
        res = (*pred++ + block[0+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[1+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[2+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[3+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[4+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[5+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[6+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;
        res = (*pred++ + block[7+(i<<3)]);
        CLIP_RESULT(res);
        *dst++ = res;

        pred += 8;
        dst += (width - 8);
    }



    /*----------------------------------------------------------------------------
    ; Return nothing or data or data pointer
    ----------------------------------------------------------------------------*/
    return;
}

#endif
/*----------------------------------------------------------------------------
; End Function: idct
----------------------------------------------------------------------------*/

