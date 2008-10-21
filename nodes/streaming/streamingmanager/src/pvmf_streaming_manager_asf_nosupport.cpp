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
#ifndef PVMF_STREAMING_MANAGER_INTERNAL_H_INCLUDED
#include "pvmf_streaming_manager_internal.h"
#endif

#ifndef PVMF_STREAMING_MANAGER_NODE_H_INCLUDED
#include "pvmf_streaming_manager_node.h"
#endif

StreamAsfHeader::~StreamAsfHeader()
{
}

bool PVMFStreamingManagerNode::IsAsfStreamingSupported()
{
    return false;
}

void PVMFStreamingManagerNode::Asf_CompletePreInit()
{
}

bool PVMFStreamingManagerNode::Asf_PopulateTrackInfoVec()
{
    return false;
}

bool PVMFStreamingManagerNode::Asf_SetSocketNodePortAllocator()
{
    return false;
}

PVMFStatus PVMFStreamingManagerNode::Asf_RecognizeAndProcessHeader()
{
    return PVMFFailure;
}

PVMFStatus PVMFStreamingManagerNode::Asf_GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo)
{
    OSCL_UNUSED_ARG(aInfo);
    return PVMFFailure;
}

PVMFStatus PVMFStreamingManagerNode::Asf_SelectTracks(PVMFMediaPresentationInfo& aInfo, PVMFSessionId s)
{
    OSCL_UNUSED_ARG(aInfo);
    OSCL_UNUSED_ARG(s);
    return PVMFFailure;
}

PVMFStatus PVMFStreamingManagerNode::Asf_InitMetaData()
{
    return PVMFFailure;
}

bool PVMFStreamingManagerNode::Asf_Doprepare(PVMFStreamingManagerNodeCommand& aCmd)
{
    OSCL_UNUSED_ARG(aCmd);
    return false;
}

void PVMFStreamingManagerNode::Asf_PopulateDRMInfo()
{
}

PVMFStatus PVMFStreamingManagerNode::Asf_GetExtendedMetaData(PvmiKvp& KeyVal, uint32 aIndex)
{
    OSCL_UNUSED_ARG(KeyVal);
    OSCL_UNUSED_ARG(aIndex);
    return PVMFErrNotSupported;
}

PVMFStatus PVMFStreamingManagerNode::Asf_ConvertWMPictureToAPIC(PvmfApicStruct*& aAPICStruct,
        bool* aImageTooBig,
        uint32 aMaxSize,
        uint32* aImageSize,
        uint32 aPicIndex,
        uint32 aMetadataindex)
{
    OSCL_UNUSED_ARG(aAPICStruct);
    OSCL_UNUSED_ARG(aImageTooBig);
    OSCL_UNUSED_ARG(aMaxSize);
    OSCL_UNUSED_ARG(aImageSize);
    OSCL_UNUSED_ARG(aPicIndex);
    OSCL_UNUSED_ARG(aMetadataindex);

    return false;
}

void PVMFStreamingManagerNode::Asf_DeleteAPICStruct(PvmfApicStruct*& aAPICStruct)
{
    OSCL_UNUSED_ARG(aAPICStruct);
}


