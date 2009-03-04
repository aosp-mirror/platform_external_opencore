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
#ifndef PVMF_OMX_AUDIODEC_DEFS_H_INCLUDED
#define PVMF_OMX_AUDIODEC_DEFS_H_INCLUDED

// Error and info messages
//

enum PVMFOMXAudioDecNodeErrors
{
    // Video decoder node failed to initialize the video decoder. Fatal error so the node needs to be reset.
    PVOMXAUDIODECNODE_ERROR_DECODER_INIT_FAILED = PVMF_NODE_ERROR_EVENT_LAST
};

enum PVMFOMXAudioDecNodeInfo
{
    // Decoding of a frame failed. Video decoder node will continue on to decode the next frame
    PVOMXAUDIODECNODE_INFO_DECODEFRAME_FAILED = PVMF_NODE_INFO_EVENT_LAST,
    // Input bitstream buffer overflowed (frame too large or couldn't find frame marker). Data will be dropped until the next frame marker is found
    // then frame will be decoded.
    PVOMXAUDIODECNODE_INFO_INPUTBITSTREAMBUFFER_OVERFLOW
};

#endif // PVMF_OMXAUDIODEC_DEFS_H_INCLUDED


