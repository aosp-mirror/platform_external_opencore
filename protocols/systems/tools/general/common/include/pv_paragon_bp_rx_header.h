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

#ifndef __PV_PARAGON_BP_RX_HEADER_H
#define __PV_PARAGON_BP_RX_HEADER_H


#include "oscl_base.h"

#define PARAGON_BP_INFO_FIELD_TX_QUEUE_SIZE_BIT 0x01
#define PARAGON_BP_TX_QUEUE_SIZE_FIELD 2

#define PARAGON_BP_PAYLOAD_SIZE_FIELD 2

typedef struct
{
    uint8 seqNum;
    uint8 infoField;
} Paragon_BP_Rx_Base_Hdr;

typedef struct
{
    uint8 seqNum;
    uint8 infoField;
    uint16 payloadSize;
    uint8 payload[2];  //Use address of this to access payload data (align structure to 16 bit boudaries).
} Paragon_BP_Rx_Normal_Hdr;

typedef struct
{
    uint8 seqNum;
    uint8 infoField;
    uint16 txQueueSize;
    uint16 payloadSize;
    uint8 payload[2];  //Use address of this to access payload data (align structure to 16 bit boudaries).
} Paragon_BP_Rx_Hdr_Opt_Tx_Q_Size;


#define PARAGON_BP_RX_BASE_HDR_SIZE sizeof(Paragon_BP_Rx_Base_Hdr)
//Always subtract payload from size
#define PARAGON_BP_RX_NORMAL_HDR_SIZE (sizeof(Paragon_BP_Rx_Normal_Hdr) - sizeof(uint16))
#define PARAGON_BP_RX_OPT_TX_Q_SIZE_HDR_SIZE (sizeof(Paragon_BP_Rx_Hdr_Opt_Tx_Q_Size) - sizeof(uint16))

#endif // __PV_PARAGON_BP_RX_HEADER_H
