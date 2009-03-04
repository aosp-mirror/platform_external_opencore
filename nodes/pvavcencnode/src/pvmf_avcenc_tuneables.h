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
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

#ifndef PVMF_AVCENC_TUNEABLES_H_INCLUDED
#define PVMF_AVCENC_TUNEABLES_H_INCLUDED


// Default port queue reserve size
#define PVMF_AVCENC_NODE_PORT_VECTOR_RESERVE 2
#define PVMF_AVCENC_NODE_MAX_INPUT_PORT 1
#define PVMF_AVCENC_NODE_MAX_OUTPUT_PORT 1
#define PVMF_AVCENC_NODE_PORT_ACTIVITY_RESERVE 10

// Default command queue reserve size
#define PVMF_AVCENC_NODE_CMD_QUEUE_RESERVE 10

// Starting value for command IDs
#define PVMF_AVCENC_NODE_CMD_ID_START 10000

#define DEFAULT_BITRATE 32000
#define DEFAULT_FRAME_WIDTH 176
#define DEFAULT_FRAME_HEIGHT 144
#define DEFAULT_FRAME_RATE 5
#define MAX_OUTBUF_SIZE 8192
#define MAX_NUM_INPUTDATA	2
#define MAX_NUM_OUTPUTDATA	2

#define PVAVCENC_MEDIADATA_POOLNUM 16
#define PVAVCENC_MEDIADATA_CHUNKSIZE 128
#define PVAVCENC_MEDIABUFFER_CHUNKSIZE (MAX_OUTBUF_SIZE+128)

// Port queue settings
#define PVMF_AVCENC_PORT_CAPACITY 10
#define PVMF_AVCENC_PORT_RESERVE 10
#define PVMF_AVCENC_PORT_THRESHOLD 50

// Encoder settings
#define MAX_AVC_LAYER 1
#define PVMF_AVCENC_NODE_SPS_VECTOR_RESERVE 16  // max allowed by standard
#define PVMF_AVCENC_NODE_PPS_VECTOR_RESERVE 256 // max allowed by standard


#endif // PVMF_AVCENC_TUNEABLES_H_INCLUDED
