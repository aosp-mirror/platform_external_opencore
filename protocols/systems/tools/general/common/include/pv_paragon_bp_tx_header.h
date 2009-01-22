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

#ifndef __PV_PARAGON_BP_TX_HEADER_H
#define __PV_PARAGON_BP_TX_HEADER_H


#include "oscl_base.h"

#define PARAGON_BP_INFO_FIELD_TX_SYNC_FLAG_BIT 0x01
#define PARAGON_BP_TX_SYNC_FLAG_SIZE_FIELD 1

#define PARAGON_BP_TX_PAYLOAD_SIZE_FIELD 2
#define PARAGON_BP_TX_MAX_SYNC_FLAG_SIZE 8

typedef struct
{
    uint8 seqNum;
    uint8 infoField;
    uint8 payload[2];  // To align the struct to 32 bit boundaries
} Paragon_BP_Tx_Base_Hdr;

typedef struct
{
    uint8 seqNum;
    uint8 infoField;
    uint16 payloadSize;
    uint8 payload[4];  //Use this address to access payload data (also aligns structure to 32 bit boudaries).
} Paragon_BP_Tx_Normal_Hdr;

typedef struct
{
    uint8 seqNum;
    uint8 infoField;
    uint8 syncFlagSize;
    uint8 syncFlag[1];
} Paragon_BP_Tx_Hdr_Opt_Sync_Flag;


#define PARAGON_BP_TX_BASE_HDR_SIZE (sizeof(Paragon_BP_Tx_Base_Hdr)-(2*sizeof(uint8)))
//Always subtract payload from size
#define PARAGON_BP_TX_NORMAL_HDR_SIZE (sizeof(Paragon_BP_Rx_Normal_Hdr) - (4*sizeof(uint8)))

#endif // __PV_PARAGON_BP_RX_HEADER_H
