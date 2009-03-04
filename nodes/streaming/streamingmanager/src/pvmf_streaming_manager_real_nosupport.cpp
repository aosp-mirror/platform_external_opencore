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
#ifndef PVMF_STREAMING_MANAGER_NODE_H_INCLUDED
#include "pvmf_streaming_manager_node.h"
#endif
#ifndef SDP_PARSER_H
#include "sdp_parser.h"
#endif

bool PVMFStreamingManagerNode::GetRealAudioMimeType(
    rm_mediaInfo* pinfo,
    OSCL_String& mime,
    OsclMemoryFragment* paacConfig)
{
    OSCL_UNUSED_ARG(pinfo);
    OSCL_UNUSED_ARG(mime);
    OSCL_UNUSED_ARG(paacConfig);

    return false;  // not supported
}

void PVMFStreamingManagerNode::CreateRealStreamingObjects()
{
    ipRealChallengeGen = NULL;
    ipRdtParser = NULL;
}

void PVMFStreamingManagerNode::DestroyRealStreamingObjects()
{
}

bool PVMFStreamingManagerNode::IsRealStreamingSupported()
{
    return false;
}
