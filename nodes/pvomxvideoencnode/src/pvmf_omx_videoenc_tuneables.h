/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
 * Copyright (C) 2008 HTC Inc.
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
#ifndef PVMF_OMX_VIDEOENC_TUNEABLES_H_INCLUDED
#define PVMF_OMX_VIDEOENC_TUNEABLES_H_INCLUDED

// Turn on statistics
#define PROFILING_ON (PVLOGGER_INST_LEVEL >= PVLOGMSG_INST_PROF)

// Default port queue reserve size
#define PVMF_OMX_VIDEOENC_NODE_PORT_VECTOR_RESERVE 2
#define PVMF_OMX_VIDEOENC_NODE_MAX_INPUT_PORT 1
#define PVMF_OMX_VIDEOENC_NODE_MAX_OUTPUT_PORT 1
#define PVMF_OMX_VIDEOENC_NODE_PORT_ACTIVITY_RESERVE 10

// Default command queue reserve size
#define PVMF_OMX_VIDEOENC_NODE_CMD_QUEUE_RESERVE 10

// Starting value for command IDs
#define PVMF_OMX_VIDEOENC_NODE_CMD_ID_START 10000

#define DEFAULT_BITRATE 32000
#define DEFAULT_FRAME_WIDTH 176
#define DEFAULT_FRAME_HEIGHT 144
#define DEFAULT_FRAME_RATE 5
#define MAX_OUTBUF_SIZE 8192
#define MAX_NUM_INPUTDATA	2
#define MAX_NUM_OUTPUTDATA	2

// This defines the minimum frame rate that will be always used in compute
// the required buffer size by Qualcomm rate control so as to reduce the
// chance of buffer overflow (Qualcomm encoder should prevent this from 
// happening in the first place). This is different from DEFAULT_FRAME_RATE.
#define MIN_FRAME_RATE_IN_FPS 5.0

#define PVVIDENC_MEDIADATA_POOLNUM 8
#define PVVIDENC_MEDIADATA_CHUNKSIZE 128
#define PVVIDENC_MEDIABUFFER_CHUNKSIZE (MAX_OUTBUF_SIZE+128)

// Port queue settings
#define PVMF_OMX_VIDEOENC_PORT_CAPACITY 10
#define PVMF_OMX_VIDEOENC_PORT_RESERVE 10
#define PVMF_OMX_VIDEOENC_PORT_THRESHOLD 50


#endif // PVMF_OMX_VIDEOENC_TUNEABLES_H_INCLUDED
