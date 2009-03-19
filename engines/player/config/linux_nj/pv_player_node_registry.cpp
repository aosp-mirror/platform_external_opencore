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

#include "pvmf_omx_videodec_factory.h"

#include "pvmf_omx_audiodec_factory.h"

#include "pvmf_amrffparser_factory.h"

#include "pvmf_aacffparser_factory.h"

#include "pvmf_wavffparser_factory.h"

#include "pvmf_mp3ffparser_factory.h"

#include "pv_player_node_registry.h"

// For recognizer registry
#include "pvmf_recognizer_registry.h"

#include "pvmi_datastreamsyncinterface_ref_factory.h"
#include "pvmf_recognizer_plugin.h"

#include "pvmp3ffrec_factory.h"

#include "pvwavffrec_factory.h"

#include "pvamrffrec_factory.h"

#ifdef HAS_OSCL_LIB_SUPPORT
#include "oscl_shared_library.h"

#include "pvmf_node_shared_lib_interface.h"

#else

#include "pvmf_sm_node_factory.h"

#endif

PVPlayerNodeRegistry::PVPlayerNodeRegistry()
{
    PVPlayerNodeInfo nodeinfo;
    iType.reserve(20);

    //For PVMFOMXVideoDecNode
    nodeinfo.iInputTypes.clear();
    nodeinfo.iInputTypes.push_back(PVMF_H263);
    nodeinfo.iInputTypes.push_back(PVMF_M4V);
    nodeinfo.iInputTypes.push_back(PVMF_H264);
    nodeinfo.iInputTypes.push_back(PVMF_H264_RAW);
    nodeinfo.iInputTypes.push_back(PVMF_H264_MP4);
    nodeinfo.iInputTypes.push_back(PVMF_WMV);
    nodeinfo.iNodeUUID = KPVMFOMXVideoDecNodeUuid;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_YUV420);
    nodeinfo.iNodeCreateFunc = PVMFOMXVideoDecNodeFactory::CreatePVMFOMXVideoDecNode;
    nodeinfo.iNodeReleaseFunc = PVMFOMXVideoDecNodeFactory::DeletePVMFOMXVideoDecNode;
    iType.push_back(nodeinfo);

    nodeinfo.iInputTypes.clear();
    // AAC FORMATS
    nodeinfo.iInputTypes.push_back(PVMF_MPEG4_AUDIO);
    nodeinfo.iInputTypes.push_back(PVMF_ADIF);
    nodeinfo.iInputTypes.push_back(PVMF_ADTS);
    nodeinfo.iInputTypes.push_back(PVMF_LATM);
    nodeinfo.iInputTypes.push_back(PVMF_ASF_MPEG4_AUDIO);
    // AMR FORMATS
    nodeinfo.iInputTypes.push_back(PVMF_AMR_IETF);
    nodeinfo.iInputTypes.push_back(PVMF_AMR_IETF_COMBINED);
    nodeinfo.iInputTypes.push_back(PVMF_AMRWB_IETF);
    nodeinfo.iInputTypes.push_back(PVMF_AMRWB_IETF_PAYLOAD);
    nodeinfo.iInputTypes.push_back(PVMF_AMR_IF2);

    // MP3 FORMAT
    nodeinfo.iInputTypes.push_back(PVMF_MP3);

    nodeinfo.iNodeUUID = KPVMFOMXAudioDecNodeUuid;;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_PCM16);
    nodeinfo.iNodeCreateFunc = PVMFOMXAudioDecNodeFactory::CreatePVMFOMXAudioDecNode;
    nodeinfo.iNodeReleaseFunc = PVMFOMXAudioDecNodeFactory::DeletePVMFOMXAudioDecNode;
    iType.push_back(nodeinfo);

    //For PVMFMP3FFParserNode
    nodeinfo.iInputTypes.clear();
    nodeinfo.iInputTypes.push_back(PVMF_MP3FF);
    nodeinfo.iNodeUUID = KPVMFMP3FFParserNodeUuid;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_FORMAT_UNKNOWN);
    nodeinfo.iNodeCreateFunc = PVMFMP3FFParserNodeFactory::CreatePVMFMP3FFParserNode;
    nodeinfo.iNodeReleaseFunc = PVMFMP3FFParserNodeFactory::DeletePVMFMP3FFParserNode;
    iType.push_back(nodeinfo);

    //For PVMFAMRFFParserNode
    nodeinfo.iInputTypes.clear();
    nodeinfo.iInputTypes.push_back(PVMF_AMRFF);
    nodeinfo.iNodeUUID = KPVMFAmrFFParserNodeUuid;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_FORMAT_UNKNOWN);
    nodeinfo.iNodeCreateFunc = PVMFAMRFFParserNodeFactory::CreatePVMFAMRFFParserNode;
    nodeinfo.iNodeReleaseFunc = PVMFAMRFFParserNodeFactory::DeletePVMFAMRFFParserNode;
    iType.push_back(nodeinfo);

    //For PVMFAACFFParserNode
    nodeinfo.iInputTypes.clear();
    nodeinfo.iInputTypes.push_back(PVMF_AACFF);
    nodeinfo.iNodeUUID = KPVMFAacFFParserNodeUuid;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_FORMAT_UNKNOWN);
    nodeinfo.iNodeCreateFunc = PVMFAACFFParserNodeFactory::CreatePVMFAACFFParserNode;
    nodeinfo.iNodeReleaseFunc = PVMFAACFFParserNodeFactory::DeletePVMFAACFFParserNode;
    iType.push_back(nodeinfo);

    //For PVMFWAVFFParserNode
    nodeinfo.iInputTypes.clear();
    nodeinfo.iInputTypes.push_back(PVMF_WAVFF);
    nodeinfo.iNodeUUID = KPVMFWavFFParserNodeUuid;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_FORMAT_UNKNOWN);
    nodeinfo.iNodeCreateFunc = PVMFWAVFFParserNodeFactory::CreatePVMFWAVFFParserNode;
    nodeinfo.iNodeReleaseFunc = PVMFWAVFFParserNodeFactory::DeletePVMFWAVFFParserNode;
    iType.push_back(nodeinfo);

#ifndef HAS_OSCL_LIB_SUPPORT
    // For PVMFStreamingManagerNode
    nodeinfo.iInputTypes.clear();
    nodeinfo.iInputTypes.push_back(PVMF_DATA_SOURCE_RTSP_URL);
    nodeinfo.iInputTypes.push_back(PVMF_DATA_SOURCE_SDP_FILE);
    nodeinfo.iNodeUUID = KPVMFStreamingManagerNodeUuid;
    nodeinfo.iOutputType.clear();
    nodeinfo.iOutputType.push_back(PVMF_FORMAT_UNKNOWN);
    nodeinfo.iNodeCreateFunc = PVMFStreamingManagerNodeFactory::CreateStreamingManagerNode;
    nodeinfo.iNodeReleaseFunc = PVMFStreamingManagerNodeFactory::DeleteStreamingManagerNode;
    iType.push_back(nodeinfo);
#endif  // !HAS_OSCL_LIB_SUPPORT
}


PVPlayerNodeRegistry::~PVPlayerNodeRegistry()
{
    iType.clear();
}


PVMFStatus PVPlayerNodeRegistry::QueryRegistry(PVMFFormatType& aInputType, PVMFFormatType& aOutputType, Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids)
{
    uint32 SearchCount = 0;
    bool matchfound = false;

    // Find all nodes that support the specified input and ouput format pair
    while (SearchCount < iType.size())
    {
        uint32 inputsearchcount = 0, outputsearchcount = 0;
        bool iInputFoundFlag = false, iOutputFoundFlag = false;

        while (inputsearchcount < iType[SearchCount].iInputTypes.size())
        {
            // Check if the input format matches
            if (iType[SearchCount].iInputTypes[inputsearchcount] == aInputType)
            {
                // Set the the input flag to true since we found the match in the search
                iInputFoundFlag = true;
                break;
            }
            inputsearchcount++;
        }

        //Check the flag of input format if it is true check for the output format, if not return failure
        if (iInputFoundFlag)
        {
            while (outputsearchcount < iType[SearchCount].iOutputType.size())
            {
                if (iType[SearchCount].iOutputType[outputsearchcount] == aOutputType)
                {
                    //set the the output flag to true since we found the match in the search
                    iOutputFoundFlag = true;
                    break;
                }

                outputsearchcount++;
            }

            if (iOutputFoundFlag)
            {
                // There's a match so add this node UUID to the list.
                aUuids.push_back(iType[SearchCount].iNodeUUID);
                matchfound = true;
            }
        }

        SearchCount++;
    }

    if (matchfound)
    {
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}


PVMFNodeInterface* PVPlayerNodeRegistry::CreateNode(PVUuid& aUuid)
{
    PVMFNodeInterface* nodeInterface = NULL;
    bool iFoundFlag = false;
    uint32 NodeSearchCount = 0;

    while (NodeSearchCount < iType.size())
    {
        //Search if the UUID's will match
        if (iType[NodeSearchCount].iNodeUUID == aUuid)
        {
            //Since the UUID's match set the flag to true
            iFoundFlag = true;
            break;
        }

        NodeSearchCount++;

    }

    if (iFoundFlag)
    {
        OsclActiveObject::OsclActivePriority priority = OsclActiveObject::EPriorityNominal;
        PVPlayerNodeInfo* nodeInfo = &iType[NodeSearchCount];

        //Call the appropriate Node creation function & return Node pointer
        if (KPVMFOMXVideoDecNodeUuid == aUuid)
        {
            priority = OsclActiveObject::EPriorityLow;
        }
        else
        {
            priority = OsclActiveObject::EPriorityNominal;
        }

#ifdef HAS_OSCL_LIB_SUPPORT
        // If the node has a corresponding shared library, load that
        // first.  If not, the node is assumed to be already completely
        // linked.
        if (NULL != nodeInfo->iSharedLibrary
                && (OsclLibSuccess == (nodeInfo->iSharedLibrary)->LoadLib()))
        {
            // Need to load the library for the node and query for the
            // create and release functions. If the library is already
            // loaded, the system should avoid opening the same library
            // again on its own.
            OsclAny* interfacePtr = NULL;
            (nodeInfo->iSharedLibrary)->QueryInterface(PV_NODE_INTERFACE,
                    (OsclAny*&)interfacePtr);
            NodeSharedLibraryInterface* nodeIntPtr =
                OSCL_DYNAMIC_CAST(NodeSharedLibraryInterface*, interfacePtr);

            OsclAny* createFuncTemp =
                nodeIntPtr->QueryNodeInterface(nodeInfo->iNodeUUID, PV_CREATE_NODE_INTERFACE);
            OsclAny* releaseFuncTemp =
                nodeIntPtr->QueryNodeInterface(nodeInfo->iNodeUUID, PV_RELEASE_NODE_INTERFACE);

            nodeInfo->iNodeCreateFunc =
                OSCL_DYNAMIC_CAST(PVMFNodeInterface * (*)(int32), createFuncTemp);
            nodeInfo->iNodeReleaseFunc =
                OSCL_DYNAMIC_CAST(bool (*)(PVMFNodeInterface*), releaseFuncTemp);
        }
#endif

        if (NULL != nodeInfo->iNodeCreateFunc)
        {
            nodeInterface = (*(iType[NodeSearchCount].iNodeCreateFunc))(priority);
        }
    }

    return nodeInterface;
}

bool PVPlayerNodeRegistry::ReleaseNode(PVUuid& aUuid, PVMFNodeInterface* aNode)
{
    bool iFoundFlag = false;
    uint32 NodeSearchCount = 0;

    while (NodeSearchCount < iType.size())
    {
        //Search if the UUID's will match
        if (iType[NodeSearchCount].iNodeUUID == aUuid)
        {
            //Since the UUID's match set the flag to true
            iFoundFlag = true;
            break;
        }

        NodeSearchCount++;

    }

    if (iFoundFlag)
    {
        //Call the appropriate Node delete function
        bool del_stat = (*(iType[NodeSearchCount].iNodeReleaseFunc))(aNode);

#ifdef HAS_OSCL_LIB_SUPPORT
        // If a library needs to be loaded to use this node, close the library
        if (NULL != iType[NodeSearchCount].iSharedLibrary)
        {
            (iType[NodeSearchCount].iSharedLibrary)->Close();
        }
#endif

        return del_stat;
    }
    return false;
}

// Player engine with recognizers for MP3, MP4, WAV, and AMR
PVPlayerRecognizerRegistry::PVPlayerRecognizerRegistry()
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVPlayerRecognizerRegistry")
{
    AddToScheduler();

    iRecSessionId = 0;
    iRecognizerResult.reserve(4);
    iFileDataStreamFactory = NULL;
    iSourceFormatType = PVMF_FORMAT_UNKNOWN;
    iObserver = NULL;
    iCmdContext = NULL;
    iCancelQuery = false;
    iCancelCmdContext = NULL;

    if (PVMFRecognizerRegistry::Init() != PVMFSuccess)
    {
        OSCL_ASSERT(false);
        return;
    }

    PVMFRecognizerPluginFactory* tmpfac = NULL;
    tmpfac = OSCL_STATIC_CAST(PVMFRecognizerPluginFactory*, OSCL_NEW(PVMP3FFRecognizerFactory, ()));
    if (PVMFRecognizerRegistry::RegisterPlugin(*tmpfac) == PVMFSuccess)
    {
        iRecognizerList.push_back(tmpfac);
    }
    else
    {
        OSCL_DELETE(((PVMP3FFRecognizerFactory*)tmpfac));
        tmpfac = NULL;
        return;
    }

    tmpfac = OSCL_STATIC_CAST(PVMFRecognizerPluginFactory*, OSCL_NEW(PVWAVFFRecognizerFactory, ()));
    if (PVMFRecognizerRegistry::RegisterPlugin(*tmpfac) == PVMFSuccess)
    {
        iRecognizerList.push_back(tmpfac);
    }
    else
    {
        OSCL_DELETE(((PVWAVFFRecognizerFactory*)tmpfac));
        tmpfac = NULL;
        return;
    }

    tmpfac = OSCL_STATIC_CAST(PVMFRecognizerPluginFactory*, OSCL_NEW(PVAMRFFRecognizerFactory, ()));
    if (PVMFRecognizerRegistry::RegisterPlugin(*tmpfac) == PVMFSuccess)
    {
        iRecognizerList.push_back(tmpfac);
    }
    else
    {
        OSCL_DELETE(((PVAMRFFRecognizerFactory*)tmpfac));
        tmpfac = NULL;
        return;
    }

}



PVPlayerRecognizerRegistry::~PVPlayerRecognizerRegistry()
{
    if (iFileDataStreamFactory)
    {
        OSCL_DELETE(OSCL_STATIC_CAST(PVMIDataStreamSyncInterfaceRefFactory*, iFileDataStreamFactory));
        iFileDataStreamFactory = NULL;
    }
    PVMFRecognizerPluginFactory* tmpfac = NULL;

    if (iRecognizerList.empty() == false)
    {
        tmpfac = iRecognizerList[0];
        iRecognizerList.erase(iRecognizerList.begin());
        PVMFRecognizerRegistry::RemovePlugin(*tmpfac);
        OSCL_DELETE(((PVMP3FFRecognizerFactory*)tmpfac));
    }

    if (iRecognizerList.empty() == false)
    {
        tmpfac = iRecognizerList[0];
        iRecognizerList.erase(iRecognizerList.begin());
        PVMFRecognizerRegistry::RemovePlugin(*tmpfac);
        OSCL_DELETE(((PVWAVFFRecognizerFactory*)tmpfac));
    }

    if (iRecognizerList.empty() == false)
    {
        tmpfac = iRecognizerList[0];
        iRecognizerList.erase(iRecognizerList.begin());
        PVMFRecognizerRegistry::RemovePlugin(*tmpfac);
        OSCL_DELETE(((PVAMRFFRecognizerFactory*)tmpfac));
    }
    PVMFRecognizerRegistry::Cleanup();
}


PVMFStatus PVPlayerRecognizerRegistry::QueryFormatType(OSCL_wString& aSourceURL, PVPlayerRecognizerRegistryObserver& aObserver, OsclAny* aCmdContext)
{
    if (iObserver != NULL)
    {
        // Previous query still ongoing
        return PVMFErrBusy;
    }
    iObserver = &aObserver;
    iCmdContext = aCmdContext;

    // Create a datastream wrapper factory for standard file
    if (iFileDataStreamFactory)
    {
        OSCL_DELETE(OSCL_STATIC_CAST(PVMIDataStreamSyncInterfaceRefFactory*, iFileDataStreamFactory));
        iFileDataStreamFactory = NULL;
    }
    int32 leavecode = 0;
    OSCL_TRY(leavecode, iFileDataStreamFactory = OSCL_STATIC_CAST(PVMFDataStreamFactory*, OSCL_NEW(PVMIDataStreamSyncInterfaceRefFactory, (aSourceURL))));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         return PVMFErrNoMemory;
                        );

    // Open the session with recognizer
    PVMFRecognizerRegistry::OpenSession(iRecSessionId, *this);

    // Request file recognition
    iRecognizerResult.clear();
    iRecognizeCmdId = PVMFRecognizerRegistry::Recognize(iRecSessionId, *iFileDataStreamFactory, NULL, iRecognizerResult, NULL);

    return PVMFSuccess;
}


void PVPlayerRecognizerRegistry::CancelQuery(OsclAny* aContext)
{
    if (iObserver == NULL)
    {
        // No pending recognize request
        OSCL_LEAVE(OsclErrInvalidState);
        return;
    }

    iCancelQuery = true;
    iCancelCmdContext = aContext;

    if (!IsBusy())
    {
        // The recognition pending so cancel it
        PVMFRecognizerRegistry::CancelCommand(iRecSessionId, iRecognizeCmdId);
    }
    // Else the recognition already completed so just cancel it in the Run()
}


void PVPlayerRecognizerRegistry::Run()
{
    // Close the session with recognizer
    PVMFRecognizerRegistry::CloseSession(iRecSessionId);

    // Destroy the data stream wrapper factory for standard file
    if (iFileDataStreamFactory)
    {
        OSCL_DELETE(OSCL_STATIC_CAST(PVMIDataStreamSyncInterfaceRefFactory*, iFileDataStreamFactory));
        iFileDataStreamFactory = NULL;
    }

    // Tell the engine the result
    if (iObserver)
    {
        iObserver->RecognizeCompleted(iSourceFormatType, iCmdContext);

        if (iCancelQuery)
        {
            iObserver->RecognizeCompleted(iSourceFormatType, iCancelCmdContext);
            iCancelQuery = false;
            iCancelCmdContext = NULL;
        }
    }
    iObserver = NULL;
    iCmdContext = NULL;
}

void PVPlayerRecognizerRegistry::RecognizerCommandCompleted(const PVMFCmdResp& aResponse)
{
    iSourceFormatType = PVMF_FORMAT_UNKNOWN;

    if (aResponse.GetCmdId() == iRecognizeCmdId)
    {
        // Recognize() command completed
        if (aResponse.GetCmdStatus() == PVMFSuccess)
        {
            Oscl_Vector<PVMFRecognizerResult, OsclMemAllocator>::iterator it;
            for (it = iRecognizerResult.begin(); it != iRecognizerResult.end(); it++)
            {
                if (it->iRecognitionConfidence == PVMFRecognizerConfidenceCertain)
                {
                    // Current implementation does not support the case where multiple
                    // recognizer plugins can recognize the file with certainty
                    iSourceFormatType = GetFormatIndex(it->iRecognizedFormat.get_str());
                    break;
                }
                if (it->iRecognitionConfidence == PVMFRecognizerConfidencePossible)
                {
                    iSourceFormatType = GetFormatIndex(it->iRecognizedFormat.get_str());
                }
            }
        }

        if (aResponse.GetCmdStatus() == PVMFErrCancelled)
        {
            // If cancelled, need to wait for the cancel command to complete before
            // calling Run
            return;
        }
    }

    RunIfNotReady();
}
