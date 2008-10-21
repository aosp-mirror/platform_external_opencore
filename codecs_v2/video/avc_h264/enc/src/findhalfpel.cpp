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
#include "avcenc_lib.h"
#include "oscl_base_macros.h"
/* 3/29/01 fast half-pel search based on neighboring guess */
/* value ranging from 0 to 4, high complexity (more accurate) to
   low complexity (less accurate) */
#define HP_DISTANCE_TH		5 // 2  /* half-pel distance threshold */

#define PREF_16_VEC 129		/* 1MV bias versus 4MVs*/

const static int distance_tab[9][9] =   /* [hp_guess][k] */
{
    {0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 1, 2, 3, 4, 3, 2, 1},
    {1, 0, 0, 0, 1, 2, 3, 2, 1},
    {1, 2, 1, 0, 1, 2, 3, 4, 3},
    {1, 2, 1, 0, 0, 0, 1, 2, 3},
    {1, 4, 3, 2, 1, 0, 1, 2, 3},
    {1, 2, 3, 2, 1, 0, 0, 0, 1},
    {1, 2, 3, 4, 3, 2, 1, 0, 1},
    {1, 0, 1, 2, 3, 2, 1, 0, 0}
};


/*=====================================================================
	Function:	AVCFindHalfPelMB
	Date:		10/31/2007
	Purpose:	Find half pel resolution MV surrounding the full-pel MV
=====================================================================*/

void AVCFindHalfPelMB(AVCEncObject *encvid, uint8 *cur, AVCMV *mot, uint8 *ncand,
                      int xpos, int ypos, int hp_guess, int cmvx, int cmvy)
{
    AVCPictureData *currPic = encvid->common->currPic;
    int lx = currPic->pitch;
    int d, dmin;
    uint8* cand;
    int lambda_motion = encvid->lambda_motion;
    uint8 *mvbits = encvid->mvbits;
    int mvcost;
    /* list of candidate to go through for half-pel search*/
    uint8* subpel_pred = (uint8*) encvid->subpel_pred; // all 16 sub-pel positions
    uint8** hpel_cand = (uint8**) encvid->hpel_cand; /* half-pel position */
    uint8**	qpel_cand;
    int xh[9] = {0, 0, 2, 2, 2, 0, -2, -2, -2};
    int yh[9] = {0, -2, -2, 0, 2, 2, 2, 0, -2};
    int xq[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int yq[8] = { -1, -1, 0, 1, 1, 1, 0, -1};
    int h, hmin, q, qmin;

    OSCL_UNUSED_ARG(xpos);
    OSCL_UNUSED_ARG(ypos);
    OSCL_UNUSED_ARG(hp_guess);

    GenerateSubPelPred(subpel_pred, ncand, lx);

    cur = encvid->currYMB; // pre-load current original MB

    cand = hpel_cand[0];

    // find cost for the current full-pel position
    dmin = SATD_MB(cand, cur, 65535); // get Hadamaard transform SAD
    mvcost = MV_COST_S(lambda_motion, mot->x, mot->y, cmvx, cmvy);
    dmin += mvcost;
    hmin = 0;

    /* find half-pel */
    for (h = 1; h < 9; h++)
    {
        d = SATD_MB(hpel_cand[h], cur, dmin);
        mvcost = MV_COST_S(lambda_motion, mot->x + xh[h], mot->y + yh[h], cmvx, cmvy);
        d += mvcost;

        if (d < dmin)
        {
            dmin = d;
            hmin = h;
        }
    }

    mot->sad = dmin;
    mot->x += xh[hmin];
    mot->y += yh[hmin];
    encvid->best_hpel_pos = hmin;

    /*** search for quarter-pel ****/
    qpel_cand = encvid->qpel_cand[hmin];
    encvid->best_qpel_pos = qmin = -1;

    for (q = 0; q < 8; q++)
    {
        d = SATD_MB(qpel_cand[q], cur, dmin);
        mvcost = MV_COST_S(lambda_motion, mot->x + xq[q], mot->y + yq[q], cmvx, cmvy);
        d += mvcost;
        if (d < dmin)
        {
            dmin = d;
            qmin = q;
        }
    }

    if (qmin != -1)
    {
        mot->sad = dmin;
        mot->x += xq[qmin];
        mot->y += yq[qmin];
        encvid->best_qpel_pos = qmin;
    }

    return ;
}

#if 0
void FindHalfPelMB(AVCEncObject *encvid, uint8 *cur, AVCMV *mot, uint8 *ncand,
                   int xpos, int ypos, int hp_guess, int cmvx, int cmvy)
{
//	hp_mem = ULong *vertArray; /* 20x17 */
//			 ULong *horzArray; /* 20x16 */
//			 ULong *diagArray; /* 20x17 */
    int dmin, d;

    int xh, yh;
    int k, kmin = 0;
    int imin, jmin, ilow, jlow;
    int in_range[9] = {0, 1, 1, 1, 1, 1, 1, 1, 1}; /*  3/29/01 */
    int range = encvid->rateCtrl->mvRange;
    AVCPictureData *currPic = encvid->common->currPic;
    int lx = currPic->pitch;
    int width = currPic->width; /*  padding */
    int height = currPic->height;
    int (**SAD_MB_HalfPel)(uint8*, uint8*, int, void*) =
        encvid->functionPointer->SAD_MB_HalfPel;
    void *extra_info = encvid->sad_extra_info;

    int next_hp_pos[9][2] = {{0, 0}, {2, 0}, {1, 1}, {0, 2}, { -1, 1}, { -2, 0}, { -1, -1}, {0, -2}, {0, -1}};
    int next_ncand[9] = {0, 1 , lx, lx, 0, -1, -1, -lx, -lx};
    int xhmin, yhmin;
    int lambda_motion = encvid->lambda_motion;
    uint8 *mvbits = encvid->mvbits;
    int mvcost;

    cur = encvid->currYMB; // pre-load current original MB

    /**************** check range ***************************/
    /*  3/29/01 */
    imin = xpos + (mot[0].x >> 2);
    jmin = ypos + (mot[0].y >> 2);
    ilow = xpos - range;
    jlow = ypos - range;

    if (imin <= -15 || imin == ilow)
        in_range[1] = in_range[7] = in_range[8] = 0;
    else if (imin >= width - 1)
        in_range[3] = in_range[4] = in_range[5] = 0;
    if (jmin <= -15 || jmin == jlow)
        in_range[1] = in_range[2] = in_range[3] = 0;
    else if (jmin >= height - 1)
        in_range[5] = in_range[6] = in_range[7] = 0;

    xhmin = 0;
    yhmin = 0;
    dmin = mot->sad;

    xh = 0;
    yh = -1;
    ncand -= lx; /* initial position */

    for (k = 2; k <= 8; k += 2)
    {
        if (distance_tab[hp_guess][k] < HP_DISTANCE_TH)
        {
            if (in_range[k])
            {
                d = (*(SAD_MB_HalfPel[((yh&1)<<1)+(xh&1)]))(ncand, cur, (dmin << 16) | lx, extra_info);
                mvcost = MV_COST_S(lambda_motion, mot[0].x + (xh << 1), mot[0].y + (yh << 1), cmvx, cmvy);
                d += mvcost;

                if (d < dmin)
                {
                    dmin = d;
                    xhmin = xh;
                    yhmin = yh;
                    kmin = k;
                }
            }
        }
        xh += next_hp_pos[k][0];
        yh += next_hp_pos[k][1];
        ncand += next_ncand[k];

        if (k == 8)
        {
            if (xhmin != 0 || yhmin != 0)
            {
                k = -1;
                hp_guess = kmin;
            }
        }
    }

    mot->sad = dmin;
    mot->x += (xhmin << 1);
    mot->y += (yhmin << 1);

    return ;
}
#endif

/** This function generates sub-pel prediction around the full-pel candidate.
Each sub-pel position array is 20 pixel wide (for word-alignment) and 17 pixel tall. */
/** The sub-pel position is labeled in spiral manner from the center. */

void GenerateSubPelPred(uint8* subpel_pred, uint8 *ncand, int lx)
{
    /* let's do straightforward way first */
    uint8 *ref;
    uint8 *dst, *dst2, *dst3, *dst4, *dst5, *src;
    uint8 tmp8;
    int32 tmp32;
    int16 tmp_horz[18*22], *dst_16, *src_16;
    int i, j;

    /* first copy full-pel to the first array */
    ref = ncand - 3 - lx - (lx << 1); /* move back (-3,-3) */
    dst = subpel_pred;

    dst -= 4; /* offset */
    for (j = 0; j < 22; j++) /* 24x22 */
    {
        i = 6;
        while (i > 0)
        {
            tmp32 = *ref++;
            tmp8 = *ref++;
            tmp32 |= (tmp8 << 8);
            tmp8 = *ref++;
            tmp32 |= (tmp8 << 16);
            tmp8 = *ref++;
            tmp32 |= (tmp8 << 24);
            *((uint32*)(dst += 4)) = tmp32;
            i--;
        }
        ref += (lx - 24);
    }

    /* from the first array, we do horizontal interp */
    ref = subpel_pred + 2;
    dst_16 = tmp_horz; /* 17 x 22 */

    for (j = -2; j < 0; j++)
    {
        for (i = 0; i < 16; i++)
        {
            *dst_16++ =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
            ref++;
        }
        /* do the 17th column here */
        *dst_16 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
        dst_16 += 2; /* stride for tmp_horz is 18 */
        ref += 8;  /* stride for ref is 24 */
    }

    dst = subpel_pred + V0Q_H2Q * SUBPEL_PRED_BLK_SIZE; /* go to the 14th array 17x18*/

    for (i = 0; i < 16; i++)
    {
        tmp32 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
        *dst_16++ = tmp32;
        ref++;
        tmp32 = (tmp32 + 16) >> 5;
        *dst++ = AVC_CLIP(tmp32);
    }
    /* do the 17th column here */
    tmp32 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
    *dst_16 = tmp32;
    tmp32 = (tmp32 + 16) >> 5;
    *dst = AVC_CLIP(tmp32);

    dst += 8;  /* stride for dst is 24 */
    dst_16 += 2; /* stride for tmp_horz is 18 */
    ref += 8;  /* stride for ref is 24 */

    dst3 = subpel_pred + V0Q_H1Q * SUBPEL_PRED_BLK_SIZE; /* 3rd array 17x16 */
    dst4 = subpel_pred + V0Q_H3Q * SUBPEL_PRED_BLK_SIZE; /* 7th array 17x16 */

    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 16; i++)
        {
            tmp32 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
            *dst_16++ = tmp32;
            ref++;
            tmp32 = (tmp32 + 16) >> 5;
            tmp32 = AVC_CLIP(tmp32);
            *dst++ = tmp32;
            *dst3++ = (tmp32 + ref[-1] + 1) >> 1;
            *dst4++ = (tmp32 + ref[0] + 1) >> 1;
        }
        /* do the 17th column here */
        tmp32 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
        *dst_16 = tmp32;
        tmp32 = (tmp32 + 16) >> 5;
        tmp32 = AVC_CLIP(tmp32);
        *dst = tmp32;
        *dst3 = (tmp32 + ref[0] + 1) >> 1;
        *dst4 = (tmp32 + ref[1] + 1) >> 1;

        dst += 8;  /* stride for dst is 24 */
        dst3 += 8;
        dst4 += 8;
        dst_16 += 2; /* stride for tmp_horz is 18 */
        ref += 8;  /* stride for ref is 24 */
    }

    for (i = 0; i < 16; i++)
    {
        tmp32 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
        *dst_16++ = tmp32;
        ref++;
        tmp32 = (tmp32 + 16) >> 5;
        *dst++ = AVC_CLIP(tmp32);
    }
    /* do the 17th column here */
    tmp32 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
    *dst_16 = tmp32;
    tmp32 = (tmp32 + 16) >> 5;
    *dst = AVC_CLIP(tmp32);

    dst += 8;  /* stride for dst is 24 */
    dst_16 += 2; /* stride for tmp_horz is 18 */
    ref += 8;  /* stride for ref is 24 */

    for (j = 17; j < 19; j++)
    {
        for (i = 0; i < 16; i++)
        {
            *dst_16++ =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
            ref++;
        }
        /* do the 17th column here */
        *dst_16 =  ref[-2] + ref[3] - 5 * (ref[-1] + ref[2]) + 20 * (ref[0] + ref[1]);
        dst_16 += 2; /* stride for tmp_horz is 18 */
        ref += 8;  /* stride for ref is 24 */
    }

    /* Do vertical filtering and vertical cross */
    src_16 = tmp_horz; /* 17 x 22 */
    src = subpel_pred + V0Q_H2Q * SUBPEL_PRED_BLK_SIZE; /* 14th array 17x18 */
    dst = subpel_pred + V2Q_H2Q * SUBPEL_PRED_BLK_SIZE; /* 12th array 17x17*/
    dst3 = subpel_pred + V1Q_H2Q * SUBPEL_PRED_BLK_SIZE; /* 15th array 17x17 */
    dst4 = subpel_pred + V3Q_H2Q * SUBPEL_PRED_BLK_SIZE; /* 13th array 17x17 */

    dst -= 24; // offset
    dst3 -= 24;
    dst4 -= 24;
    for (i = 0; i < 17; i++)
    {
        for (j = 0; j < 17; j++)
        {
            tmp32 = src_16[0] + src_16[18*5] - 5 * (src_16[18] + src_16[18*4]) + 20 * (src_16[18*2] + src_16[18*3]);
            tmp32 = (tmp32 + 512) >> 10;
            tmp32 = AVC_CLIP(tmp32);
            *(dst += 24) = tmp32;
            *(dst3 += 24) = (tmp32 + *src + 1) >> 1;
            *(dst4 += 24) = (tmp32 + *(src += 24) + 1) >> 1;
            src_16 += 18;
        }
        src_16 -= ((18 * 17) - 1);
        dst -= ((24 * 17) - 1);
        dst3 -= ((24 * 17) - 1);
        dst4 -= ((24 * 17) - 1);
        src -= ((24 * 17) - 1);
    }

    /* do vertical interpolation */
    ref = subpel_pred + 2;
    dst = subpel_pred + V2Q_H0Q * SUBPEL_PRED_BLK_SIZE; /* 10th array 18x17 */
    dst -= 24; // offset
    src = subpel_pred + V2Q_H2Q * SUBPEL_PRED_BLK_SIZE; /* 12th array 17x17 */
    src -= 24; // offset
    dst4 = subpel_pred + V2Q_H1Q * SUBPEL_PRED_BLK_SIZE; /* 11th array 17x17 */
    dst4 -= 24; // offset

    for (j = 0; j < 17; j++)
    {
        tmp32 = ref[0] + ref[24*5] - 5 * (ref[24] + ref[24*4]) + 20 * (ref[24*2] + ref[24*3]);
        ref += 24;
        tmp32 = (tmp32 + 16) >> 5;
        tmp32 = AVC_CLIP(tmp32);
        *(dst += 24) = tmp32;
        *(dst4 += 24) = (tmp32 + *(src += 24) + 1) >> 1;
    }
    dst -= ((24 * 17) - 1);
    dst4 -= ((24 * 17) - 1);
    ref -= ((24 * 17) - 1);
    src -= ((24 * 17) - 1); // 12th

    dst2 = subpel_pred + V1Q_H0Q * SUBPEL_PRED_BLK_SIZE; /* 5th array 16x17 */
    dst2 -= 24; //offset
    dst3 = subpel_pred + V3Q_H0Q * SUBPEL_PRED_BLK_SIZE; /* 1st array 16x17 */
    dst3 -= 24; //offset
    dst5 = subpel_pred + V2Q_H3Q * SUBPEL_PRED_BLK_SIZE; /* 9th array 17x17 */
    dst5 -= 24; //offset

    for (i = 0; i < 16; i++)
    {
        for (j = 0; j < 17; j++)
        {
            tmp32 = ref[0] + ref[24*5] - 5 * (ref[24] + ref[24*4]) + 20 * (ref[24*2] + ref[24*3]);
            ref += 24;
            tmp32 = (tmp32 + 16) >> 5;
            tmp32 = AVC_CLIP(tmp32);
            *(dst += 24) = tmp32;  // 10th
            *(dst2 += 24) = (tmp32 + ref[24] + 1) >> 1;  // 5th
            *(dst3 += 24) = (tmp32 + ref[24*2] + 1) >> 1; // 1st
            *(dst4 += 24) = (tmp32 + *(src += 24) + 1) >> 1; // 11th
            *(dst5 += 24) = (tmp32 + src[-1] + 1) >> 1;  // 9th
        }

        dst -= ((24 * 17) - 1);
        dst2 -= ((24 * 17) - 1);
        dst3 -= ((24 * 17) - 1);
        dst4 -= ((24 * 17) - 1);
        dst5 -= ((24 * 17) - 1);
        ref -= ((24 * 17) - 1);
        src -= ((24 * 17) - 1);
    }

    src--;
    for (j = 0; j < 17; j++)
    {
        tmp32 = ref[0] + ref[24*5] - 5 * (ref[24] + ref[24*4]) + 20 * (ref[24*2] + ref[24*3]);
        ref += 24;
        tmp32 = (tmp32 + 16) >> 5;
        tmp32 = AVC_CLIP(tmp32);
        *(dst += 24) = tmp32;
        *(dst5 += 24) = (tmp32 + *(src += 24) + 1) >> 1;
    }

    /* now diagonal direction */
    ref = subpel_pred + V0Q_H2Q * SUBPEL_PRED_BLK_SIZE;  // 14th
    src = subpel_pred + V2Q_H0Q * SUBPEL_PRED_BLK_SIZE;  // 10th
    dst = subpel_pred + V1Q_H1Q * SUBPEL_PRED_BLK_SIZE;  // 4th
    dst2 = subpel_pred + V1Q_H3Q * SUBPEL_PRED_BLK_SIZE; // 6th
    dst3 = subpel_pred + V3Q_H1Q * SUBPEL_PRED_BLK_SIZE; // 2th
    dst4 = subpel_pred + V3Q_H3Q * SUBPEL_PRED_BLK_SIZE; // 8th

    for (j = 0; j < 17; j++)
    {
        for (i = 0; i < 17; i++)
        {
            *dst3++ = (ref[24] + *src + 1) >> 1;
            *dst2++ = (*ref + src[1] + 1) >> 1;
            *dst4++ = (ref[24] + src[1] + 1) >> 1;
            *dst++ = (*ref++ + *src++ + 1) >> 1;
        }
        dst += 7;
        dst2 += 7;
        dst3 += 7;
        dst4 += 7;
        ref += 7;
        src += 7;
    }

    return ;
}

/* assuming cand always has a pitch of 24 */
int SATD_MB(uint8 *cand, uint8 *cur, int dmin)
{
    int cost;
    int j, k;
    int16 res[256], *pres; // residue
    int m0, m1, m2, m3;

    // calculate SATD
    pres = res;
    // horizontal transform
    for (j = 0; j < 16; j++)
    {
        k = 4;
        while (k > 0)
        {
            m0 = cur[0] - cand[0];
            m3 = cur[3] - cand[3];
            m0 += m3;
            m3 = m0 - (m3 << 1);
            m1 = cur[1] - cand[1];
            m2 = cur[2] - cand[2];
            m1 += m2;
            m2 = m1 - (m2 << 1);
            pres[0] = m0 + m1;
            pres[2] = m0 - m1;
            pres[1] = m2 + m3;
            pres[3] = m3 - m2;

            cur += 4;
            pres += 4;
            cand += 4;
            k--;
        }
        cand += 8;
    }
    /* vertical transform */
    cost = 0;
    for (j = 0; j < 4; j++)
    {
        pres = res + (j << 6);
        k = 16;
        while (k > 0)
        {
            m0 = pres[0];
            m3 = pres[3<<4];
            m0 += m3;
            m3 = m0 - (m3 << 1);
            m1 = pres[1<<4];
            m2 = pres[2<<4];
            m1 += m2;
            m2 = m1 - (m2 << 1);

            pres[0] = m0 = m0 + m1;
            cost += ((m0 > 0) ? m0 : -m0);
            m1 = m0 - (m1 << 1);
            cost += ((m1 > 0) ? m1 : -m1);
            m3 = m2 + m3;
            cost += ((m3 > 0) ? m3 : -m3);
            m2 = m3 - (m2 << 1);
            cost += ((m2 > 0) ? m2 : -m2);

            pres++;
            k--;
        }
        if ((cost >> 1) > dmin) /* early drop out */
        {
            return (cost >> 1);
        }
    }

    return (cost >> 1);
}





