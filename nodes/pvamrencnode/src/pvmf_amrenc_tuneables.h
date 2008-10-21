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
 * @file pvvideoencmdf_tuneables.h
 * @brief Tuneable hard coded parameters for using PV video encoder MDF
 */

#ifndef PVMF_AMRENC_TUNEABLES_H_INCLUDED
#define PVMF_AMRENC_TUNEABLES_H_INCLUDED

#define LOG_STACK_TRACE(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, m);
#define LOG_DEBUG(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, m);
#define LOG_ERR(m) PVLOGGER_LOGMSG(PVLOGMSG_INST_REL,iLogger,PVLOGMSG_ERR,m);

// Constants for pvmf_amrenc_media_buffer.cpp
const uint32 PVMF_AMRENC_MEDIA_BUF_DEFAULT_NUMOFFRAGMENTS  = 4;
const uint32 PVMF_AMRENC_MEDIA_BUF_DEFAULT_FRAGMENT_LENGTH = 32;

// Constants for pvmf_amrenc_node.cpp
const uint32 MAX_AMR_FRAME_SIZE = 32;
const uint32 AMR_FRAME_LENGTH_IN_TIMESTAMP = 20;
const uint32 OUTPUT_BITSTREAMBUFFER_POOLNUM = 4;
const uint32 OUTPUT_MEDIADATA_POOL_SIZE = 10;
const uint32 OUTPUT_MEDIADATA_CHUNK_SIZE = 128;

// Default port queue reserve size
#define PVMF_AMRENC_NODE_PORT_VECTOR_RESERVE 2
#define PVMF_AMRENC_NODE_MAX_INPUT_PORT 1
#define PVMF_AMRENC_NODE_MAX_OUTPUT_PORT 1
#define PVMF_AMRENC_NODE_PORT_ACTIVITY_RESERVE 10

// Default command queue reserve size
#define PVMF_AMRENC_NODE_CMD_QUEUE_RESERVE 10

// Starting value for command IDs
#define PVMF_AMRENC_NODE_CMD_ID_START 50000

// Port queue settings
#define PVMF_AMRENC_PORT_CAPACITY 10
#define PVMF_AMRENC_PORT_RESERVE 10
#define PVMF_AMRENC_PORT_THRESHOLD 50

#define PVMF_AMRENC_DEFAULT_SAMPLING_RATE 8000
#define PVMF_AMRENC_DEFAULT_NUM_CHANNELS 1

#endif // PVMF_AMRENC_TUNEABLES_H_INCLUDED

