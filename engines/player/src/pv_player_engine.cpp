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
#include "pv_player_engine.h"

#include "pv_player_engine_tunables.h"

#include "pv_player_sdkinfo.h"

#include "pv_player_config.h"

#include "pvmf_node_interface.h"

#include "pvmf_ffparsernode_extension.h"

#include "pvmf_data_source_init_extension.h"

#include "pvmf_track_selection_extension.h"

#include "pvmf_data_source_playback_control.h"

#include "pvmf_data_source_direction_control.h"

#include "pvmf_track_level_info_extension.h"

#include "pvmf_fileoutput_factory.h"

#include "pvmf_fileoutput_config.h"

#include "pvmf_nodes_sync_control.h"

#include "pvlogger.h"

#include "oscl_error_codes.h"

#include "pvmf_basic_errorinfomessage.h"

#include "pvmf_duration_infomessage.h"

#include "pv_mime_string_utils.h"

#include "pvmi_kvp_util.h"

#include "oscl_string_utils.h"

#include "media_clock_converter.h"

#include "time_comparison_utils.h"

#include "pvmf_local_data_source.h"

#include "pvmf_cpmplugin_license_interface.h"

#include "oscl_registry_access_client.h"

#include "pvmf_source_context_data.h"


#ifdef HAS_OSCL_LIB_SUPPORT

#include "pv_player_engine.h"

#include "pv_player_node_registry.h"
#include "pv_player_registry_interface.h"

// For recognizer registry
#include "pvmf_recognizer_registry.h"

#include "pvmi_datastreamsyncinterface_ref_factory.h"

#include "pvmf_recognizer_plugin.h"

#include "oscl_shared_library.h"
#include "oscl_library_list.h"

#include "oscl_shared_lib_interface.h"

#include "pvmf_node_shared_lib_interface.h"

#define PVPLAYERENGINE_DEFAULT_CONFIG_PATH_SIZE 32

#ifndef PVPLAYERENGINE_NODE_REGISTRY_CONFIG_PATH
#define PVPLAYERENGINE_NODE_REGISTRY_CONFIG_PATH NULL
#endif

#endif

//


#define PVPLAYERENGINE_NUM_COMMANDS 10

#define PVPLAYERENGINE_TIMERID_ENDTIMECHECK 1
#define PVPLAYERENGINE_TIMERID_PLAY_STATUS 2




PVPlayerEngine* PVPlayerEngine::New(PVCommandStatusObserver* aCmdStatusObserver,
                                    PVErrorEventObserver *aErrorEventObserver,
                                    PVInformationalEventObserver *aInfoEventObserver)
{
    PVPlayerEngine* engine = NULL;
    engine = OSCL_NEW(PVPlayerEngine, ());
    if (engine)
    {
        engine->Construct(aCmdStatusObserver,
                          aErrorEventObserver,
                          aInfoEventObserver);
    }

    return engine;
}


PVPlayerEngine::~PVPlayerEngine()
{
    Cancel();

    // Remove Stored KVP Values
    DeleteKVPValues();

    if (!iPendingCmds.empty())
    {
        iPendingCmds.pop();
    }

    // Clean up the datapaths
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        DoEngineDatapathCleanup(iDatapathList[i]);
    }
    iDatapathList.clear();

    // Clean up the source node
    DoSourceNodeCleanup();

    // Shutdown and destroy the timer
    if (iPollingCheckTimer)
    {
        iPollingCheckTimer->Clear();
    }

    if (iWatchDogTimer)
    {
        iWatchDogTimer->Cancel();
        OSCL_DELETE(iWatchDogTimer);
    }

    OSCL_TEMPLATED_DELETE(iPollingCheckTimer, OsclTimer<OsclMemAllocator>, OsclTimer);

    // Return all engine contexts to pool
    while (!iCurrentContextList.empty())
    {
        FreeEngineContext(iCurrentContextList[0]);
    }

    iNodeUuids.clear();

#ifdef HAS_OSCL_LIB_SUPPORT
    DepopulateAllRegistries();
#endif

}


PVCommandId PVPlayerEngine::GetSDKInfo(PVSDKInfo &aSDKInfo, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetSDKInfo()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aSDKInfo;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_SDK_INFO, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::GetSDKModuleInfo(PVSDKModuleInfo &aSDKModuleInfo, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetSDKModuleInfo()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aSDKModuleInfo;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_SDK_MODULE_INFO, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::SetLogAppender(const char* aTag, OsclSharedPtr<PVLoggerAppender>& aAppender, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SetLogAppender()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(2);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pChar_value = (char*)aTag;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aAppender;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_SET_LOG_APPENDER, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::RemoveLogAppender(const char* aTag, OsclSharedPtr<PVLoggerAppender>& aAppender, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveLogAppender()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(2);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pChar_value = (char*)aTag;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aAppender;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_REMOVE_LOG_APPENDER, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::SetLogLevel(const char* aTag, int32 aLevel, bool aSetSubtree, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SetLogLevel()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pChar_value = (char*)aTag;
    paramvec.push_back(param);
    param.int32_value = aLevel;
    paramvec.push_back(param);
    param.bool_value = aSetSubtree;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_SET_LOG_LEVEL, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::GetLogLevel(const char* aTag, PVLogLevelInfo& aLogInfo, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetLogLevel()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(2);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pChar_value = (char*)aTag;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aLogInfo;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_LOG_LEVEL, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::QueryUUID(const PvmfMimeString& aMimeType, Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids,
                                      bool aExactUuidsOnly, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::QueryUUID()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aMimeType;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aUuids;
    paramvec.push_back(param);
    param.bool_value = aExactUuidsOnly;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_QUERY_UUID, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::QueryInterface(const PVUuid& aUuid, PVInterface*& aInterfacePtr, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::QueryInterface()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aInterfacePtr;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_QUERY_INTERFACE, (OsclAny*)aContextData, &paramvec, &aUuid);
}


PVCommandId PVPlayerEngine::CancelAllCommands(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CancelAllCommands()"));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::GetPVPlayerState(PVPlayerState& aState, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetPVPlayerState()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aState;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_PVPLAYER_STATE, (OsclAny*)aContextData, &paramvec);
}


PVMFStatus PVPlayerEngine::GetPVPlayerStateSync(PVPlayerState& aState)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetPVPlayerStateSync()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aState;
    paramvec.push_back(param);
    PVPlayerEngineCommand cmd(PVP_ENGINE_COMMAND_GET_PVPLAYER_STATE, -1, NULL, &paramvec);
    return DoGetPVPlayerState(cmd, true);
}


PVCommandId PVPlayerEngine::AddDataSource(PVPlayerDataSource& aDataSource, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::AddDataSource()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aDataSource;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_ADD_DATA_SOURCE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::Init(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Init()"));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_INIT, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::GetMetadataKeys(PVPMetadataList& aKeyList, int32 aStartingIndex, int32 aMaxEntries,
        char* aQueryKey, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetMetadataKeys()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(4);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;

    param.pOsclAny_value = (OsclAny*) & aKeyList;
    paramvec.push_back(param);
    param.int32_value = aStartingIndex;
    paramvec.push_back(param);
    param.int32_value = aMaxEntries;
    paramvec.push_back(param);
    param.pChar_value = aQueryKey;
    paramvec.push_back(param);

    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_METADATA_KEY, (OsclAny*)aContextData, &paramvec);
}

PVCommandId PVPlayerEngine::GetMetadataValues(PVPMetadataList& aKeyList, int32 aStartingValueIndex, int32 aMaxValueEntries, int32& aNumAvailableValueEntries,
        Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetMetadataValues()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(5);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;

    param.pOsclAny_value = (OsclAny*) & aKeyList;
    paramvec.push_back(param);
    param.int32_value = aStartingValueIndex;
    paramvec.push_back(param);
    param.int32_value = aMaxValueEntries;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aNumAvailableValueEntries;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aValueList;
    paramvec.push_back(param);

    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_METADATA_VALUE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::AddDataSink(PVPlayerDataSink& aDataSink, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::AddDataSink()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aDataSink;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_ADD_DATA_SINK, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::SetPlaybackRange(PVPPlaybackPosition aBeginPos, PVPPlaybackPosition aEndPos, bool aQueueRange, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SetPlaybackRange()"));
    PVPPlaybackPosition curpos;
    curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    iPlaybackPositionMode = aBeginPos.iMode;
    GetPlaybackClockPosition(curpos);
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.playbackpos_value = aBeginPos;
    paramvec.push_back(param);
    param.playbackpos_value = aEndPos;
    paramvec.push_back(param);
    param.bool_value = aQueueRange;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_SET_PLAYBACK_RANGE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::GetPlaybackRange(PVPPlaybackPosition &aBeginPos, PVPPlaybackPosition &aEndPos, bool aQueued, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetPlaybackRange()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pPlaybackpos_value = &aBeginPos;
    paramvec.push_back(param);
    param.pPlaybackpos_value = &aEndPos;
    paramvec.push_back(param);
    param.bool_value = aQueued;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_PLAYBACK_RANGE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::GetCurrentPosition(PVPPlaybackPosition &aPos, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetCurrentPosition()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pPlaybackpos_value = &aPos;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_CURRENT_POSITION, (OsclAny*)aContextData, &paramvec);
}


PVMFStatus PVPlayerEngine::GetCurrentPositionSync(PVPPlaybackPosition &aPos)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetCurrentPositionSync()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pPlaybackpos_value = &aPos;
    paramvec.push_back(param);
    PVPlayerEngineCommand cmd(PVP_ENGINE_COMMAND_GET_CURRENT_POSITION, -1, NULL, &paramvec);
    return DoGetCurrentPosition(cmd, true);
}


PVCommandId PVPlayerEngine::SetPlaybackRate(int32 aRate, OsclTimebase* aTimebase, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SetPlaybackRate()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(2);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.int32_value = aRate;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*)aTimebase;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_SET_PLAYBACK_RATE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::GetPlaybackRate(int32& aRate, OsclTimebase*& aTimebase, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetPlaybackRate()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(2);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pInt32_value = &aRate;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aTimebase;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_PLAYBACK_RATE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::GetPlaybackMinMaxRate(int32& aMinRate, int32& aMaxRate, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::GetPlaybackMinMaxRate()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(2);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pInt32_value = &aMinRate;
    paramvec.push_back(param);
    param.pInt32_value = &aMaxRate;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_GET_PLAYBACK_MINMAX_RATE, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::Prepare(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Prepare()"));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_PREPARE, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::Start(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Start() "));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_START, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::Pause(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Pause()"));
    if (!iSourceDurationAvailable)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Pause() - Pause not supported"));
        OSCL_LEAVE(PVMFErrNotSupported);
        return -1;
    }

    return AddCommandToQueue(PVP_ENGINE_COMMAND_PAUSE, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::Resume(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Resume()"));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_RESUME, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::Stop(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Stop()"));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_STOP, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::RemoveDataSink(PVPlayerDataSink& aDataSink, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDataSink()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aDataSink;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_REMOVE_DATA_SINK, (OsclAny*)aContextData, &paramvec);
}


PVCommandId PVPlayerEngine::Reset(const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Reset()"));
    return AddCommandToQueue(PVP_ENGINE_COMMAND_RESET, (OsclAny*)aContextData);
}


PVCommandId PVPlayerEngine::RemoveDataSource(PVPlayerDataSource& aDataSource, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDataSource()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*) & aDataSource;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_REMOVE_DATA_SOURCE, (OsclAny*)aContextData, &paramvec);
}


void PVPlayerEngine::setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::setObserver()"));
    iCfgCapCmdObserver = aObserver;
}


PVMFStatus PVPlayerEngine::getParametersSync(PvmiMIOSession aSession, PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::getParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigGetParametersSync(aIdentifier, aParameters, aNumParamElements, aContext);
}


PVMFStatus PVPlayerEngine::releaseParameters(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::releaseParameters()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigReleaseParameters(aParameters, aNumElements);
}


void PVPlayerEngine::createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::createContext()"));
    OSCL_UNUSED_ARG(aSession);
    // Context is not really supported so just return some member variable pointer
    aContext = (PvmiCapabilityContext) & iCapConfigContext;
}


void PVPlayerEngine::setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext, PvmiKvp* aParameters, int aNumParamElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::setContextParameters()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    OSCL_UNUSED_ARG(aParameters);
    OSCL_UNUSED_ARG(aNumParamElements);
    // This method is not supported so leave
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::setContextParameters() is not supported!"));
    OSCL_LEAVE(PVMFErrNotSupported);
}


void PVPlayerEngine::DeleteContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DeleteContext()"));
    OSCL_UNUSED_ARG(aSession);
    OSCL_UNUSED_ARG(aContext);
    // Do nothing since the context is just the a member variable of the engine
}


void PVPlayerEngine::setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp* &aRetKVP)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::setParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    // Save the parameters in an engine command object
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*)aParameters;
    paramvec.push_back(param);
    param.int32_value = (int32) aNumElements;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aRetKVP;
    paramvec.push_back(param);
    PVPlayerEngineCommand cmd(PVP_ENGINE_COMMAND_CAPCONFIG_SET_PARAMETERS, -1, NULL, &paramvec);

    // Complete the request synchronously
    DoCapConfigSetParameters(cmd, true);
}


PVMFCommandId PVPlayerEngine::setParametersAsync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements, PvmiKvp*& aRetKVP, OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::setParametersAsync()"));
    OSCL_UNUSED_ARG(aSession);

    // Save the parameters in an engine command object
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = (OsclAny*)aParameters;
    paramvec.push_back(param);
    param.int32_value = (int32) aNumElements;
    paramvec.push_back(param);
    param.pOsclAny_value = (OsclAny*) & aRetKVP;
    paramvec.push_back(param);

    // Push it to command queue to be processed asynchronously
    return AddCommandToQueue(PVP_ENGINE_COMMAND_CAPCONFIG_SET_PARAMETERS, (OsclAny*)aContext, &paramvec, NULL, false);
}


uint32 PVPlayerEngine::getCapabilityMetric(PvmiMIOSession aSession)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::getCapabilityMetric()"));
    OSCL_UNUSED_ARG(aSession);
    // Not supported so return 0
    return 0;
}


PVMFStatus PVPlayerEngine::verifyParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::verifyParametersSync()"));
    OSCL_UNUSED_ARG(aSession);

    return DoCapConfigVerifyParameters(aParameters, aNumElements);
}


PVMFCommandId PVPlayerEngine::AcquireLicense(OsclAny* aLicenseData, uint32 aDataSize, oscl_wchar* aContentName, int32 aTimeoutMsec, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::AcquireLicense() wchar"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = aLicenseData;
    paramvec.push_back(param);
    param.uint32_value = aDataSize;
    paramvec.push_back(param);
    param.pWChar_value = aContentName;
    paramvec.push_back(param);
    param.int32_value = aTimeoutMsec;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR, (OsclAny*)aContextData, &paramvec);
}


PVMFCommandId PVPlayerEngine::AcquireLicense(OsclAny* aLicenseData, uint32 aDataSize, char* aContentName, int32 aTimeoutMsec, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::AcquireLicense() char"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(3);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.pOsclAny_value = aLicenseData;
    paramvec.push_back(param);
    param.uint32_value = aDataSize;
    paramvec.push_back(param);
    param.pChar_value = aContentName;
    paramvec.push_back(param);
    param.int32_value = aTimeoutMsec;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR, (OsclAny*)aContextData, &paramvec);
}

PVMFCommandId PVPlayerEngine::CancelAcquireLicense(PVMFCommandId aCmdId, const OsclAny* aContextData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CancelAcquireLicense()"));
    Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator> paramvec;
    paramvec.reserve(1);
    paramvec.clear();
    PVPlayerEngineCommandParamUnion param;
    param.int32_value = aCmdId;
    paramvec.push_back(param);
    return AddCommandToQueue(PVP_ENGINE_COMMAND_CANCEL_ACQUIRE_LICENSE, (OsclAny*)aContextData, &paramvec);
}

PVMFStatus PVPlayerEngine::GetLicenseStatus(PVMFCPMLicenseStatus& aStatus)
{
    if (iSourceNodeCPMLicenseIF)
        return iSourceNodeCPMLicenseIF->GetLicenseStatus(aStatus);
    if (iCPMPluginLicenseIF)
        return iCPMPluginLicenseIF->GetLicenseStatus(aStatus);
    return PVMFFailure;
}


void PVPlayerEngine::addRef()
{
}


void PVPlayerEngine::removeRef()
{
}


bool PVPlayerEngine::queryInterface(const PVUuid& uuid, PVInterface*& iface)
{
    if (uuid == PVMI_CAPABILITY_AND_CONFIG_PVUUID)
    {
        PvmiCapabilityAndConfig* capconfigiface = OSCL_STATIC_CAST(PvmiCapabilityAndConfig*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, capconfigiface);
    }
    else if (uuid == PVPlayerLicenseAcquisitionInterfaceUuid)
    {
        PVPlayerLicenseAcquisitionInterface* licacqiface = OSCL_STATIC_CAST(PVPlayerLicenseAcquisitionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, licacqiface);
    }
    // Check if track level info IF from source node was requested
    else if (uuid == PVMF_TRACK_LEVEL_INFO_INTERFACE_UUID && iSourceNodeTrackLevelInfoIF)
    {
        iface = OSCL_STATIC_CAST(PVInterface*, iSourceNodeTrackLevelInfoIF);
    }
    //Check if track selection IF from source node was requested
    else if (uuid == PVPlayerTrackSelectionInterfaceUuid)
    {
        PVPlayerTrackSelectionInterface* tseliface = OSCL_STATIC_CAST(PVPlayerTrackSelectionInterface*, this);
        iface = OSCL_STATIC_CAST(PVInterface*, tseliface);
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::queryInterface() Unsupported interface UUID."));
        return false;
    }

    return true;
}



PVPlayerEngine::PVPlayerEngine() :
        OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVPlayerEngine"),
        iCommandId(0),
        iState(PVP_ENGINE_STATE_IDLE),
        iCmdStatusObserver(NULL),
        iErrorEventObserver(NULL),
        iInfoEventObserver(NULL),
        iCfgCapCmdObserver(NULL),
        iPollingCheckTimer(NULL),
        iCommandCompleteInEngineAOPending(false),
        iCommandCompleteInEngineAOCmdStatus(PVMFSuccess),
        iCommandCompleteInEngineAOErrMsg(NULL),
        iCapConfigContext(0),
        iNumPendingNodeCmd(0),
        iNumPendingDatapathCmd(0),
        iDataSource(NULL),
        iDataSourcePS(NULL),
        iSourceFormatType(PVMF_FORMAT_UNKNOWN),
        iSourceNode(NULL),
        iSourceNodeSessionId(0),
        iSourceNodeInitIF(NULL),
        iSourceNodeTrackSelIF(NULL),
        iSourceNodePBCtrlIF(NULL),
        iSourceNodeDirCtrlIF(NULL),
        iSourceNodeTrackLevelInfoIF(NULL),
        iSourceNodeMetadataExtIF(NULL),
        iSourceNodeCapConfigIF(NULL),
        iSourceNodeRegInitIF(NULL),
        iSourceNodeCPMLicenseIF(NULL),
        iSourceNodePacketSourceIF(NULL),
        iCPMPlugin(NULL),
        iCPMPluginFactory(NULL),
        iCPMPluginSessionId(0),
        iCPMPluginLicenseIF(NULL),
        iCPMPluginCapConfigIf(NULL),
        iCPMPluginCommand(-1),
        iCPMGetLicenseCmdId(0),
        iCurrentContextListMemPool(12),
        iPendingCancelDueToCancelRequest(0),
        iPendingStopDueToCancelRequest(0),
        iPendingResetDueToCancelRequest(0),
        iPendingCancelDueToErrorRequest(0),
        iErrorOccurredDuringErrorHandling(false),
        iLogger(NULL),
        iPerfLogger(NULL),
        iReposLogger(NULL),
        iPlaybackClockRate(100000),
        iOutsideTimebase(NULL),
        iPlaybackClockRate_New(100000),
        iOutsideTimebase_New(NULL),
        iActualPlaybackPosition(0),
        iSeekPointBeforeTargetNPT(0),
        iSeekPointAfterTargetNPT(0),
        iActualMediaDataTS(0),
        iAdjustedMediaDataTS(0),
        iWatchDogTimerInterval(0),
        iStartNPT(0),
        iStartMediaDataTS(0),
        iForwardReposFlag(false),
        iBackwardReposFlag(false),
        iPlaybackDirection(1),
        iPlaybackDirection_New(1),
        iChangePlaybackDirectionWhenResuming(false),
        iEndTimeCheckEnabled(false),
        iQueuedRangePresent(false),
        iChangePlaybackPositionWhenResuming(false),
        iDataReadySent(false),
        iPlayStatusTimerEnabled(false),
        iPlaybackPausedDueToEndOfClip(false),
        iSourceDurationAvailable(false),
        iSourceDurationInMS(0),
        iPBPosEnable(true),
        iPBPosStatusUnit(PVPLAYERENGINE_CONFIG_PBPOSSTATUSUNIT_DEF),
        iPBPosStatusInterval(PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_DEF),
        iEndTimeCheckInterval(PVPLAYERENGINE_CONFIG_ENDTIMECHECKINTERVAL_DEF),
        iSeekToSyncPoint(PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINT_DEF),
        iSkipToRequestedPosition(PVPLAYERENGINE_CONFIG_SKIPTOREQUESTEDPOS_DEF),
        iRenderSkipped(PVPLAYERENGINE_CONFIG_RENDERSKIPPED_DEF),
        iSyncPointSeekWindow(PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINTWINDOW_DEF),
        iNodeCmdTimeout(PVPLAYERENGINE_CONFIG_NODECMDTIMEOUT_DEF),
        iNodeDataQueuingTimeout(PVPLAYERENGINE_CONFIG_NODEDATAQUEUINGTIMEOUT_DEF),
        iProdInfoProdName(_STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_PRODNAME_STRING)),
        iProdInfoPartNum(_STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_PARTNUM_STRING)),
        iProdInfoHWPlatform(_STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_HWPLATFORM_STRING)),
        iProdInfoSWPlatform(_STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_SWPLATFORM_STRING)),
        iProdInfoDevice(_STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_DEVICE_STRING)),
        iRollOverState(RollOverStateIdle),
        iBackwardRepos(false),
        iAlternateSrcFormatIndex(0),
        iStreamID(0),
        iNumPendingSkipCompleteEvent(0),
        iNumPVMFInfoStartOfDataPending(0),
        iResumeAfterReposition(false),
        iTrackSelectionHelper(NULL),
        iPlaybackPositionMode(PVPPBPOS_MODE_UNKNOWN)
{
    iCurrentBeginPosition.iIndeterminate = true;
    iCurrentEndPosition.iIndeterminate = true;
    iCurrentBeginPosition.iPlayListUri = NULL;
    iQueuedBeginPosition.iIndeterminate = true;
    iQueuedEndPosition.iIndeterminate = true;
    iChangeDirectionNPT.iIndeterminate = true;

    iSyncMarginVideo.min = PVPLAYERENGINE_CONFIG_SYNCMARGIN_EARLY_DEF;
    iSyncMarginVideo.max = PVPLAYERENGINE_CONFIG_SYNCMARGIN_LATE_DEF;
    iSyncMarginAudio.min = PVPLAYERENGINE_CONFIG_SYNCMARGIN_EARLY_DEF;
    iSyncMarginAudio.max = PVPLAYERENGINE_CONFIG_SYNCMARGIN_LATE_DEF;
    iSyncMarginText.min = PVPLAYERENGINE_CONFIG_SYNCMARGIN_EARLY_DEF;
    iSyncMarginText.max = PVPLAYERENGINE_CONFIG_SYNCMARGIN_LATE_DEF;

    iNodeUuids.clear();
}


void PVPlayerEngine::Construct(PVCommandStatusObserver* aCmdStatusObserver,
                               PVErrorEventObserver *aErrorEventObserver,
                               PVInformationalEventObserver *aInfoEventObserver)
{
    iCmdStatusObserver = aCmdStatusObserver;
    iInfoEventObserver = aInfoEventObserver;
    iErrorEventObserver = aErrorEventObserver;

    // Allocate memory for vectors
    // If a leave occurs, let it bubble up
    iCurrentCmd.reserve(1);
    iCmdToCancel.reserve(1);
    iCmdToDlaCancel.reserve(1);
    iPendingCmds.reserve(PVPLAYERENGINE_NUM_COMMANDS);
    iPvmiKvpCapNConfig.reserve(20);

    iDatapathList.reserve(3);

    iCurrentContextList.reserve(12);

    iMetadataIFList.reserve(6);
    iMetadataIFList.clear();

    iMetadataReleaseList.reserve(6);
    iMetadataReleaseList.clear();

    AddToScheduler();

    // Retrieve the logger object
    iLogger = PVLogger::GetLoggerObject("PVPlayerEngine");
    iPerfLogger = PVLogger::GetLoggerObject("pvplayerdiagnostics.perf.engine");
    iReposLogger = PVLogger::GetLoggerObject("pvplayerrepos.engine");

    // Initialize the playback clock to use tickcount timebase
    iPlaybackClock.SetClockTimebase(iPlaybackTimebase);
    uint32 starttime = 0;
    iPlaybackClock.SetStartTime32(starttime, OSCLCLOCK_MSEC);

    // Initialize the OSCL timer for polling checks
    iPollingCheckTimer = OSCL_NEW(OsclTimer<OsclMemAllocator>, ("playerengine_pollingcheck"));
    iPollingCheckTimer->SetObserver(this);
    iPollingCheckTimer->SetFrequency(10);  // 100 ms resolution

    iWatchDogTimer = OSCL_NEW(PVPlayerWatchdogTimer, (this));

#ifdef HAS_OSCL_LIB_SUPPORT
    if (NULL != PVPLAYERENGINE_NODE_REGISTRY_CONFIG_PATH)
    {
        OSCL_HeapString<OsclMemAllocator> configFileName =
            PVPLAYERENGINE_NODE_REGISTRY_CONFIG_PATH;
        PopulateAllRegistries(configFileName);
    }
#endif

    return;
}


void PVPlayerEngine::Run()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() In"));
    int32 leavecode = 0;

    if (iRollOverState == RollOverStateStart)
    {
        if (iPendingCmds.top().GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Source Roll Over In Progress But CancelAllCommands is requied by App"));
            iRollOverState = RollOverStateIdle;
        }
        else if (iPendingCmds.top().GetCmdType() == PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Source Roll Over In Progress But ResetDueToError is required by Error handling"));
            iRollOverState = RollOverStateIdle;
        }
        else
        {
            if (iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_INIT)
            {
                iCommandCompleteInEngineAOPending = false;

                //implies that we are doing a source rollover
                PVMFStatus status =
                    DoSourceNodeRollOver(iCurrentCmd[0].iCmdId,
                                         iCurrentCmd[0].iContextData);

                if (status != PVMFPending)
                {
                    if (CheckForSourceRollOver())
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::RunL() DoSourceNodeRollOver Failed, alternate source node for rollover is available"));
                        RunIfNotReady();
                        return;
                    }

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() DoSourceNodeRollOver Failed"));
                    iRollOverState = RollOverStateIdle;
                    EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFFailure);
                }
                else
                {
                    iRollOverState = RollOverStateInProgress;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() Source Roll Over In Progress But Incorrect Engine Cmd - Asserting"));
                OSCL_ASSERT(false);
            }
            return;
        }
    }

    if (iRollOverState == RollOverStateInProgress)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::RunL() Source Roll Over In Progress "));
        return;
    }

    // Check if a command that needs to complete in engine's AO is pending
    if (iCommandCompleteInEngineAOPending)
    {
        iCommandCompleteInEngineAOPending = false;

        OSCL_ASSERT(iCurrentCmd.empty() == false);
        switch (iCurrentCmd[0].GetCmdType())
        {
            case PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS:
                DoCleanupDueToCancel();
                break;

            case PVP_ENGINE_COMMAND_ADD_DATA_SOURCE:
                DoAddDataSourceFailureComplete();
                break;

            case PVP_ENGINE_COMMAND_INIT:
                DoInitFailureComplete();
                break;
            case PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR:
            case PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR:
                DoCPMPluginAcquireLicenseComplete();
                break;

            case PVP_ENGINE_COMMAND_RESET:
            case PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR:
                if (iState != PVP_ENGINE_STATE_IDLE)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() Wrong state for completing Reset encountered. Asserting"));
                    OSCL_ASSERT(false); // we should not be here
                    EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFErrInvalidState);
                    return;
                }
                if (iDataSource)
                    RemoveDataSourceSync(*iDataSource);
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFSuccess);
                break;
            default:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() Command type that does not need to complete in engine AO encountered. Asserting"));
                OSCL_ASSERT(false);
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFFailure);
                break;
        }
        return;
    }

    /* Check if CancelAll()/CancelAcquireLicense request was made */
    if (!iPendingCmds.empty())
    {
        if (iPendingCmds.top().GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS)
        {
            // Process it right away
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Processing CancelAll() request"));
            PVPlayerEngineCommand cmd(iPendingCmds.top());
            iPendingCmds.pop();
            DoCancelAllCommands(cmd);
            return;
        }
        else if (iPendingCmds.top().GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_ACQUIRE_LICENSE)
        {
            // Process it right away
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Processing CancelAcquireLicesense() request"));
            PVPlayerEngineCommand cmd(iPendingCmds.top());
            iPendingCmds.pop();
            DoCancelAcquireLicense(cmd);
            return;
        }
    }

    // Handle other requests normally
    if (!iPendingCmds.empty() && iCurrentCmd.empty())
    {
        // Retrieve the first pending command from queue
        PVPlayerEngineCommand cmd(iPendingCmds.top());

        // check if Stop cmd injection is necessary during Reset
        if (iPendingCmds.top().GetCmdType() == PVP_ENGINE_COMMAND_RESET)
        {
            switch (GetPVPlayerState())
            {
                case PVP_STATE_PREPARED:
                case PVP_STATE_STARTED:
                case PVP_STATE_PAUSED:
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Reset request, injecting Stop first"));
                    RunIfNotReady(); //reschedule Reset
                    // execute Stop before doing Reset
                    cmd = PVPlayerEngineCommand(PVP_ENGINE_COMMAND_STOP, iCommandId++, NULL, NULL, false);
                    if (iCommandId == 0x7FFFFFFF)
                        iCommandId = 0;
                    break;
                default: //process normally in all other states
                    iPendingCmds.pop();
            }

        }
        else
        {
            iPendingCmds.pop();
        }

        // Put in on the current command queue
        leavecode = 0;
        OSCL_TRY(leavecode, iCurrentCmd.push_front(cmd));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() Command could not be pushed onto iCurrentCmd vector"));
                             EngineCommandCompleted(cmd.GetCmdId(), cmd.GetContext(), PVMFErrNoMemory);
                             OSCL_ASSERT(false);
                             return;);

        // Process the command according to the cmd type
        PVMFStatus cmdstatus = PVMFSuccess;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Processing command with type=%d", cmd.GetCmdType()));
        switch (cmd.GetCmdType())
        {
            case PVP_ENGINE_COMMAND_GET_SDK_INFO:
                cmdstatus = DoGetSDKInfo(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_SDK_MODULE_INFO:
                // GetSDKModuleInfo is currently not supported
                cmdstatus = PVMFErrNotSupported;
                break;

            case PVP_ENGINE_COMMAND_SET_LOG_APPENDER:
                cmdstatus = DoSetLogAppender(cmd);
                break;

            case PVP_ENGINE_COMMAND_REMOVE_LOG_APPENDER:
                cmdstatus = DoRemoveLogAppender(cmd);
                break;

            case PVP_ENGINE_COMMAND_SET_LOG_LEVEL:
                cmdstatus = DoSetLogLevel(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_LOG_LEVEL:
                cmdstatus = DoGetLogLevel(cmd);
                break;

            case PVP_ENGINE_COMMAND_QUERY_UUID:
                cmdstatus = DoQueryUUID(cmd);;
                break;

            case PVP_ENGINE_COMMAND_QUERY_INTERFACE:
                cmdstatus = DoQueryInterface(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_PVPLAYER_STATE:
                cmdstatus = DoGetPVPlayerState(cmd, false);
                break;

            case PVP_ENGINE_COMMAND_ADD_DATA_SOURCE:
                cmdstatus = DoAddDataSource(cmd);
                break;

            case PVP_ENGINE_COMMAND_INIT:
                cmdstatus = DoInit(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_METADATA_KEY:
                cmdstatus = DoGetMetadataKey(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_METADATA_VALUE:
                cmdstatus = DoGetMetadataValue(cmd);
                break;

            case PVP_ENGINE_COMMAND_ADD_DATA_SINK:
                cmdstatus = DoAddDataSink(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_CURRENT_POSITION:
                cmdstatus = DoGetCurrentPosition(cmd, false);
                break;

            case PVP_ENGINE_COMMAND_SET_PLAYBACK_RANGE:
                cmdstatus = DoSetPlaybackRange(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_PLAYBACK_RANGE:
                cmdstatus = DoGetPlaybackRange(cmd);
                break;

            case PVP_ENGINE_COMMAND_SET_PLAYBACK_RATE:
                cmdstatus = DoSetPlaybackRate(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_PLAYBACK_RATE:
                cmdstatus = DoGetPlaybackRate(cmd);
                break;

            case PVP_ENGINE_COMMAND_GET_PLAYBACK_MINMAX_RATE:
                cmdstatus = DoGetPlaybackMinMaxRate(cmd);
                break;

            case PVP_ENGINE_COMMAND_PREPARE:
                cmdstatus = DoPrepare(cmd);
                break;

            case PVP_ENGINE_COMMAND_START:
                cmdstatus = DoStart(cmd);
                break;

            case PVP_ENGINE_COMMAND_PAUSE:
            case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDOFCLIP:
            case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDTIME_REACHED:
                cmdstatus = DoPause(cmd);
                break;

            case PVP_ENGINE_COMMAND_RESUME:
                cmdstatus = DoResume(cmd);
                break;

            case PVP_ENGINE_COMMAND_STOP:
                cmdstatus = DoStop(cmd);
                break;

            case PVP_ENGINE_COMMAND_REMOVE_DATA_SINK:
                cmdstatus = DoRemoveDataSink(cmd);
                break;

            case PVP_ENGINE_COMMAND_RESET:
                cmdstatus = DoReset(cmd);
                break;

            case PVP_ENGINE_COMMAND_REMOVE_DATA_SOURCE:
                cmdstatus = DoRemoveDataSource(cmd);
                break;

            case PVP_ENGINE_COMMAND_CAPCONFIG_SET_PARAMETERS:
                cmdstatus = DoCapConfigSetParameters(cmd, false);
                break;

            case PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR:
            case PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR:
                cmdstatus = DoAcquireLicense(cmd);
                break;

            case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW:
                cmdstatus = DoSourceUnderflowAutoPause(cmd);
                break;

            case PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY:
                cmdstatus = DoSourceDataReadyAutoResume(cmd);
                break;

            case PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR:
                cmdstatus = DoStopDueToError(cmd);
                break;

            case PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR:
                cmdstatus = DoResetDueToError(cmd);
                break;

            case PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR:
                cmdstatus = DoCleanupDueToError(cmd);
                break;

            case PVP_ENGINE_COMMAND_DATAPATH_DELETE:
                cmdstatus = DoDatapathDelete(cmd);
                break;

            case PVP_ENGINE_COMMAND_CANCEL_DUE_TO_ERROR:
                // Nothing to do since this command would not
                // be processed from here
                break;

            case PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS:
                // CancelAll() should not be handled here
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() CancelAllCommands should be not handled in here. Asserting."));
                OSCL_ASSERT(false);
                // Just handle as "not supported"
            default:
                cmdstatus = PVMFErrNotSupported;
                break;
        }

        if (cmdstatus != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::Run() Command failed CmdId %d Status %d",
                            cmd.GetCmdId(), cmdstatus));
            EngineCommandCompleted(cmd.GetCmdId(), cmd.GetContext(), cmdstatus);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::Run() Out"));
}


bool PVPlayerEngine::FindNodeTypeByNode(PVMFNodeInterface* aUnknownNode, PVPlayerNodeType& aNodeType, int32& aDatapathListIndex)
{
    if (aUnknownNode == NULL)
    {
        // Cannot check with node pointer being NULL
        // Might bring up false positives
        aNodeType = PVP_NODETYPE_UNKNOWN;
        aDatapathListIndex = -1;
        return false;
    }

    // Go through each engine datapath and find whether
    // the specified node is a dec node or sink node
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iDecNode == aUnknownNode)
        {
            aNodeType = PVP_NODETYPE_DECODER;
            aDatapathListIndex = i;
            return true;
        }
        else if (iDatapathList[i].iSinkNode == aUnknownNode)
        {
            aNodeType = PVP_NODETYPE_SINK;
            aDatapathListIndex = i;
            return true;
        }
    }

    // Could not determine the types
    aNodeType = PVP_NODETYPE_UNKNOWN;
    aDatapathListIndex = -1;
    return false;
}


bool PVPlayerEngine::FindDatapathByMediaType(PVPlayerMediaType aMediaType, int32& aDatapathListIndex)
{
    // Current implementaion needs to determine which media types are available
    // and this logic can be removed if the engine is completely media type agnostic.
    // This logic assumes there is only one possible datapath with the same media type.
    if (aMediaType == PVP_MEDIATYPE_UNKNOWN)
    {
        // Cannot check with media type being unknown
        // Might bring up false positives
        aDatapathListIndex = -1;
        return false;
    }

    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iMediaType == aMediaType)
        {
            aDatapathListIndex = i;
            return true;
        }
    }

    // Could not determine the types
    aDatapathListIndex = -1;
    return false;
}


void PVPlayerEngine::NodeCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() In"));

    int32 leavecode = 0;

    // Check if a cancel command completed
    uint32* context_uint32 = (uint32*)(aResponse.GetContext());
    if (context_uint32 == &iPendingCancelDueToCancelRequest)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Cancel in node completed for cancel command. Pending %d", iPendingCancelDueToCancelRequest));
        OSCL_ASSERT(iPendingCancelDueToCancelRequest > 0);
        --iPendingCancelDueToCancelRequest;
        if (iPendingCancelDueToCancelRequest == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Cancelling of all node/datapath commands complete so continue to shutdown"));
            RemoveDatapathContextFromList(); // empty left over contexts from cancelled datapath commands
            DoShutdownDueToCancel();
        }
        return;
    }
    // Check if stop called on node due to cancel shutdown
    else if (context_uint32 == &iPendingStopDueToCancelRequest)
    {
        // Don't care about the response
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Stop in node due to cancel completed. Pending %d", iPendingStopDueToCancelRequest));
        --iPendingStopDueToCancelRequest;
        if (iPendingStopDueToCancelRequest == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Stopping of all nodes due to cancel has completed so continuing on to reset"));
            DoResetDueToCancel();
        }
        return;
    }
    // Check if reset called on node due to cancel shutdown
    else if (context_uint32 == &iPendingResetDueToCancelRequest)
    {
        // Don't care about the response
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Reset in node due to cancel completed. Pending %d", iPendingResetDueToCancelRequest));
        --iPendingResetDueToCancelRequest;
        if (iPendingResetDueToCancelRequest == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Resetting of all nodes due to cancel has completed so continuing on to cleanup"));
            // Need to cleanup in engine's AO so set the flag.
            // There shouldn't be any other command pending to complete in engine's AO
            OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
            iCommandCompleteInEngineAOPending = true;
            if (IsBusy())
            {
                Cancel();
            }
            RunIfNotReady();
        }
        return;
    }
    // Check if cancel called on node due to error
    else if (context_uint32 == &iPendingCancelDueToErrorRequest)
    {
        // Don't care about the response
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Cancel in node due to error completed. Pending %d", iPendingCancelDueToErrorRequest));
        --iPendingCancelDueToErrorRequest;
        if (iPendingCancelDueToErrorRequest == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Cancel due to error complete."));
            // Check that current cmd is the internal cancel-due-to-error command
            OSCL_ASSERT(iCurrentCmd.empty() == false);
            OSCL_ASSERT(iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_DUE_TO_ERROR);

            RemoveDatapathContextFromList(); // empty left over contexts from cancelled datapath commands
            // Call EngineCommandCompleted() so the internal cancel-due-to-error command
            // would be completed and next engine command in pending queue would be processed
            EngineCommandCompleted(-1, NULL, PVMFSuccess);
            // Clear out iCmdToCancel queue
            OSCL_ASSERT(iCmdToCancel.empty() == false);
            iCmdToCancel.clear();
        }
        return;
    }

    PVPlayerEngineContext* nodecontext = (PVPlayerEngineContext*)(aResponse.GetContext());
    OSCL_ASSERT(nodecontext);

    // Ignore other node completion if cancelling
    if (!iCmdToCancel.empty())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Node command completion ignored due to cancel process, id=%d", aResponse.GetCmdId()));
        // Remove the context from the list
        FreeEngineContext(nodecontext);
        return;
    }

    // Process according to cmd type in the engine context data, node type, or engine state
    if (nodecontext->iCmdType == PVP_CMD_SinkNodeSkipMediaDataDuringPlayback)
    {
        HandleSinkNodeSkipMediaDataDuringPlayback(*nodecontext, aResponse);
    }
    else if (nodecontext->iCmdType == PVP_CMD_SinkNodeAutoPause)
    {
        HandleSinkNodePause(*nodecontext, aResponse);
    }
    else if (nodecontext->iCmdType == PVP_CMD_SinkNodeAutoResume)
    {
        HandleSinkNodeResume(*nodecontext, aResponse);
    }
    else if (nodecontext->iCmdType == PVP_CMD_GetNodeMetadataKey)
    {
        // Ignore the command status since it does not matter and continue going through the metadata interface list

        // Determine the number of keys were added
        uint32 numkeysadded = iGetMetadataKeysParam.iKeyList->size() - iGetMetadataKeysParam.iNumKeyEntriesInList;
        if (numkeysadded > 0)
        {
            // Create an entry for the metadata key release list
            PVPlayerEngineMetadataReleaseEntry releaseentry;
            releaseentry.iMetadataIFListIndex = iGetMetadataKeysParam.iCurrentInterfaceIndex;
            // Save the start and end indices into the key list for keys that this node added
            releaseentry.iStartIndex = iGetMetadataKeysParam.iNumKeyEntriesInList;
            releaseentry.iEndIndex = iGetMetadataKeysParam.iNumKeyEntriesInList + numkeysadded - 1;

            leavecode = 0;
            OSCL_TRY(leavecode, iMetadataReleaseList.push_back(releaseentry));
            if (leavecode != 0)
            {
                // An element could not be added to the release list vector
                // so notify completion of GetMetadataKey() command with memory failure
                EngineCommandCompleted(nodecontext->iCmdId, (OsclAny*)nodecontext->iCmdContext, PVMFErrNoMemory);

                // Release the last requested keys
                PVMFMetadataExtensionInterface* mdif = iMetadataIFList[releaseentry.iMetadataIFListIndex].iInterface;
                OSCL_ASSERT(mdif != NULL);
                mdif->ReleaseNodeMetadataKeys(*(iGetMetadataKeysParam.iKeyList), releaseentry.iStartIndex, releaseentry.iEndIndex);

                // Release the memory allocated for rest of the metadata keys
                while (iMetadataReleaseList.empty() == false)
                {
                    mdif = iMetadataIFList[iMetadataReleaseList[0].iMetadataIFListIndex].iInterface;
                    OSCL_ASSERT(mdif != NULL);
                    mdif->ReleaseNodeMetadataKeys(*(iGetMetadataKeysParam.iKeyList), iMetadataReleaseList[0].iStartIndex, iMetadataReleaseList[0].iEndIndex);
                    iMetadataReleaseList.erase(iMetadataReleaseList.begin());
                }

                // Remove the context from the list
                // Need to do this since we're calling return from here
                FreeEngineContext(nodecontext);
                return;
            }

            // Update the variables tracking the key list
            if (iGetMetadataKeysParam.iNumKeyEntriesToFill != -1)
            {
                iGetMetadataKeysParam.iNumKeyEntriesToFill -= numkeysadded;
            }
            iGetMetadataKeysParam.iNumKeyEntriesInList += numkeysadded;
        }

        // Update the interface index to the next one
        ++iGetMetadataKeysParam.iCurrentInterfaceIndex;

        // Loop until GetNodeMetadataKeys() is called or command is completed
        bool endloop = false;
        while (endloop == false)
        {
            // Check if there is another metadata interface to check
            if (iGetMetadataKeysParam.iCurrentInterfaceIndex < iMetadataIFList.size())
            {
                PVMFMetadataExtensionInterface* mdif = iMetadataIFList[iGetMetadataKeysParam.iCurrentInterfaceIndex].iInterface;
                OSCL_ASSERT(mdif != NULL);
                PVMFSessionId sessionid = iMetadataIFList[iGetMetadataKeysParam.iCurrentInterfaceIndex].iSessionId;

                // Determine the number of keys available for the specified query key
                int32 numkeys = mdif->GetNumMetadataKeys(iGetMetadataKeysParam.iQueryKey);
                if (numkeys <= 0)
                {
                    // Since there is no keys from this node, go to the next one
                    ++iGetMetadataKeysParam.iCurrentInterfaceIndex;
                    continue;
                }

                // If more key entries can be added, retrieve from the node
                if (iGetMetadataKeysParam.iNumKeyEntriesToFill > 0 || iGetMetadataKeysParam.iNumKeyEntriesToFill == -1)
                {
                    int32 leavecode = 0;
                    PVMFCommandId cmdid = -1;
                    PVPlayerEngineContext* newcontext = AllocateEngineContext(NULL, NULL, NULL, nodecontext->iCmdId, nodecontext->iCmdContext, PVP_CMD_GetNodeMetadataKey);
                    OSCL_TRY(leavecode, cmdid = mdif->GetNodeMetadataKeys(sessionid,
                                                *(iGetMetadataKeysParam.iKeyList),
                                                0,
                                                iGetMetadataKeysParam.iNumKeyEntriesToFill,
                                                iGetMetadataKeysParam.iQueryKey,
                                                (OsclAny*)newcontext));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() GetNodeMetadataKeys on a node did a leave!"));
                                         FreeEngineContext(newcontext);
                                         // Go to the next metadata IF in the list and continue
                                         ++iGetMetadataKeysParam.iCurrentInterfaceIndex;
                                         continue;);

                    // End the loop since GetNodeMetadataKeys() was called
                    endloop = true;
                }
                else
                {
                    // Retrieved the requested number of keys so notify completion of GetMetadataKey() command
                    EngineCommandCompleted(nodecontext->iCmdId, (OsclAny*)nodecontext->iCmdContext, aResponse.GetCmdStatus());

                    // Release the memory allocated for the metadata keys
                    while (iMetadataReleaseList.empty() == false)
                    {
                        mdif = iMetadataIFList[iMetadataReleaseList[0].iMetadataIFListIndex].iInterface;
                        OSCL_ASSERT(mdif != NULL);
                        mdif->ReleaseNodeMetadataKeys(*(iGetMetadataKeysParam.iKeyList), iMetadataReleaseList[0].iStartIndex, iMetadataReleaseList[0].iEndIndex);
                        iMetadataReleaseList.erase(iMetadataReleaseList.begin());
                    }

                    // End the loop since finished command
                    endloop = true;
                }
            }
            else
            {
                // No more so notify completion of GetMetadataKey() command
                EngineCommandCompleted(nodecontext->iCmdId, (OsclAny*)nodecontext->iCmdContext, aResponse.GetCmdStatus());

                // Release the memory allocated for the metadata keys
                while (iMetadataReleaseList.empty() == false)
                {
                    PVMFMetadataExtensionInterface* mdif = iMetadataIFList[iMetadataReleaseList[0].iMetadataIFListIndex].iInterface;
                    OSCL_ASSERT(mdif != NULL);
                    mdif->ReleaseNodeMetadataKeys(*(iGetMetadataKeysParam.iKeyList), iMetadataReleaseList[0].iStartIndex, iMetadataReleaseList[0].iEndIndex);
                    iMetadataReleaseList.erase(iMetadataReleaseList.begin());
                }

                // End the loop since reached the end of the metadata IF list
                endloop = true;
            }
        }
    }
    else if (nodecontext->iCmdType == PVP_CMD_GetNodeMetadataValue)
    {
        // Ignore the command status since it does not matter and continue going through the metadata interface list

        // Determine the number of values were added
        uint32 numvaluesadded = iGetMetadataValuesParam.iValueList->size() - iGetMetadataValuesParam.iNumValueEntriesInList;
        if (numvaluesadded > 0)
        {
            // Create an entry for the metadata value release list
            PVPlayerEngineMetadataReleaseEntry releaseentry;
            releaseentry.iMetadataIFListIndex = iGetMetadataValuesParam.iCurrentInterfaceIndex;
            // Save the start and end indices into the value list for values that this node added
            releaseentry.iStartIndex = iGetMetadataValuesParam.iNumValueEntriesInList;
            releaseentry.iEndIndex = iGetMetadataValuesParam.iNumValueEntriesInList + numvaluesadded - 1;

            leavecode = 0;
            OSCL_TRY(leavecode, iMetadataReleaseList.push_back(releaseentry));
            if (leavecode != 0)
            {
                // An element could not be added to the release list vector
                // so notify completion of GetMetadataValue() command with memory failure
                EngineCommandCompleted(nodecontext->iCmdId, (OsclAny*)nodecontext->iCmdContext, PVMFErrNoMemory);

                // Release the last requested values
                PVMFMetadataExtensionInterface* mdif = iMetadataIFList[releaseentry.iMetadataIFListIndex].iInterface;
                OSCL_ASSERT(mdif != NULL);
                mdif->ReleaseNodeMetadataValues(*(iGetMetadataValuesParam.iValueList), releaseentry.iStartIndex, releaseentry.iEndIndex);

                // Release the memory allocated for rest of the metadata values
                while (iMetadataReleaseList.empty() == false)
                {
                    mdif = iMetadataIFList[iMetadataReleaseList[0].iMetadataIFListIndex].iInterface;
                    OSCL_ASSERT(mdif != NULL);
                    mdif->ReleaseNodeMetadataValues(*(iGetMetadataValuesParam.iValueList), iMetadataReleaseList[0].iStartIndex, iMetadataReleaseList[0].iEndIndex);
                    iMetadataReleaseList.erase(iMetadataReleaseList.begin());
                }

                // Remove the context from the list
                // Need to do this since we're calling return from here
                FreeEngineContext(nodecontext);
                return;
            }

            // Update the variables tracking the value list
            if (iGetMetadataValuesParam.iNumValueEntriesToFill != -1)
            {
                iGetMetadataValuesParam.iNumValueEntriesToFill -= numvaluesadded;
            }
            iGetMetadataValuesParam.iNumValueEntriesInList += numvaluesadded;
        }

        // Update the interface index to the next one
        ++iGetMetadataValuesParam.iCurrentInterfaceIndex;

        // Loop until GetNodeMetadataValues() is called or command is completed
        bool endloop = false;
        while (endloop == false)
        {
            // Check if there is another metadata interface to check
            if (iGetMetadataValuesParam.iCurrentInterfaceIndex < iMetadataIFList.size())
            {
                PVMFMetadataExtensionInterface* mdif = iMetadataIFList[iGetMetadataValuesParam.iCurrentInterfaceIndex].iInterface;
                OSCL_ASSERT(mdif != NULL);
                PVMFSessionId sessionid = iMetadataIFList[iGetMetadataValuesParam.iCurrentInterfaceIndex].iSessionId;

                // Determine the number of values available for the specified key list
                int32 numvalues = mdif->GetNumMetadataValues(*(iGetMetadataValuesParam.iKeyList));
                if (numvalues > 0)
                {
                    // Add it to the total available
                    *(iGetMetadataValuesParam.iNumAvailableValues) += numvalues;
                }
                else
                {
                    // Since there is no values from this node, go to the next one
                    ++iGetMetadataValuesParam.iCurrentInterfaceIndex;
                    continue;
                }

                // If more value entries can be added, retrieve from the node
                if (iGetMetadataValuesParam.iNumValueEntriesToFill > 0 || iGetMetadataValuesParam.iNumValueEntriesToFill == -1)
                {
                    int32 leavecode = 0;
                    PVMFCommandId cmdid = -1;
                    PVPlayerEngineContext* newcontext = AllocateEngineContext(NULL, NULL, NULL, nodecontext->iCmdId, nodecontext->iCmdContext, PVP_CMD_GetNodeMetadataValue);
                    OSCL_TRY(leavecode, cmdid = mdif->GetNodeMetadataValues(sessionid,
                                                *(iGetMetadataValuesParam.iKeyList),
                                                *(iGetMetadataValuesParam.iValueList),
                                                0,
                                                iGetMetadataValuesParam.iNumValueEntriesToFill,
                                                (OsclAny*)newcontext));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() GetNodeMetadataValues on a node did a leave!"));
                                         FreeEngineContext(newcontext);
                                         // Go to the next metadata IF in the list and continue
                                         ++iGetMetadataValuesParam.iCurrentInterfaceIndex;
                                         continue;);

                    // End the loop since GetNodeMetadataValues() was called
                    endloop = true;
                }
                else
                {
                    // Retrieved requested number of values so notify completion of GetMetadataValue() command
                    EngineCommandCompleted(nodecontext->iCmdId, (OsclAny*)nodecontext->iCmdContext, aResponse.GetCmdStatus());

                    // Release the memory allocated for the metadata values
                    while (iMetadataReleaseList.empty() == false)
                    {
                        mdif = iMetadataIFList[iMetadataReleaseList[0].iMetadataIFListIndex].iInterface;
                        OSCL_ASSERT(mdif != NULL);
                        mdif->ReleaseNodeMetadataValues(*(iGetMetadataValuesParam.iValueList), iMetadataReleaseList[0].iStartIndex, iMetadataReleaseList[0].iEndIndex);
                        iMetadataReleaseList.erase(iMetadataReleaseList.begin());
                    }

                    // End the loop since finished command
                    endloop = true;
                }
            }
            else
            {
                // No more so notify completion of GetMetadataValue() command
                EngineCommandCompleted(nodecontext->iCmdId, (OsclAny*)nodecontext->iCmdContext, aResponse.GetCmdStatus());

                // Release the memory allocated for the metadata values
                while (iMetadataReleaseList.empty() == false)
                {
                    PVMFMetadataExtensionInterface* mdif = iMetadataIFList[iMetadataReleaseList[0].iMetadataIFListIndex].iInterface;
                    OSCL_ASSERT(mdif != NULL);
                    mdif->ReleaseNodeMetadataValues(*(iGetMetadataValuesParam.iValueList), iMetadataReleaseList[0].iStartIndex, iMetadataReleaseList[0].iEndIndex);
                    iMetadataReleaseList.erase(iMetadataReleaseList.begin());
                }

                // End the loop since reached the end of the metadata IF list
                endloop = true;
            }
        }
    }
    else if (nodecontext->iNode == iSourceNode)
    {
        if (nodecontext->iCmdType == PVP_CMD_SourceNodeQueryDataSourcePositionDuringPlayback)
        {
            HandleSourceNodeQueryDataSourcePositionDuringPlayback(*nodecontext, aResponse);
        }
        else if (nodecontext->iCmdType == PVP_CMD_SourceNodeSetDataSourcePositionDuringPlayback)
        {
            HandleSourceNodeSetDataSourcePositionDuringPlayback(*nodecontext, aResponse);
        }
        else if (nodecontext->iCmdType == PVP_CMD_SourceNodeSetDataSourceDirection)
        {
            HandleSourceNodeSetDataSourceDirection(*nodecontext, aResponse);
        }
        else if (nodecontext->iCmdType == PVP_CMD_SourceNodeSetDataSourceRate)
        {
            HandleSourceNodeSetDataSourceRate(*nodecontext, aResponse);
        }
        else
        {
            switch (iState)
            {
                case PVP_ENGINE_STATE_IDLE:
                    switch (nodecontext->iCmdType)
                    {
                        case PVP_CMD_SourceNodeQueryInitIF:
                            HandleSourceNodeQueryInitIF(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeQueryTrackSelIF:
                            HandleSourceNodeQueryTrackSelIF(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeQueryTrackLevelInfoIF:
                        case PVP_CMD_SourceNodeQueryPBCtrlIF:
                        case PVP_CMD_SourceNodeQueryDirCtrlIF:
                        case PVP_CMD_SourceNodeQueryMetadataIF:
                        case PVP_CMD_SourceNodeQueryCapConfigIF:
                        case PVP_CMD_SourceNodeQueryCPMLicenseIF:
                        case PVP_CMD_SourceNodeQuerySrcNodeRegInitIF:
                        case PVP_CMD_SourceNodeQueryPacketSourceIF:
                            HandleSourceNodeQueryInterfaceOptional(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeGetDlaData:
                            HandleSourceNodeGetDlaData(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeGetLicense:
                            HandleSourceNodeGetLicense(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeCancelGetLicense:
                            HandleSourceNodeCancelGetLicense(*nodecontext, aResponse);
                            break;

                        default:
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid source node command type in PVP_ENGINE_STATE_IDLE. Asserting"));
                            OSCL_ASSERT(false);
                            break;
                    }
                    break;

                case PVP_ENGINE_STATE_INITIALIZED:
                    switch (nodecontext->iCmdType)
                    {
                        case PVP_CMD_SourceNodeGetLicense:
                            HandleSourceNodeGetLicense(*nodecontext, aResponse);
                            break;
                        default:
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid source node command type in PVP_ENGINE_STATE_IDLE. Asserting"));
                            OSCL_ASSERT(false);
                            break;
                    }
                    break;

                case PVP_ENGINE_STATE_INITIALIZING:
                    switch (nodecontext->iCmdType)
                    {
                        case PVP_CMD_SourceNodeInit:
                            HandleSourceNodeInit(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeGetDurationValue:
                            HandleSourceNodeGetDurationValue(*nodecontext, aResponse);
                            break;

                        default:
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid source node command type in PVP_ENGINE_STATE_INITIALIZING. Asserting"));
                            OSCL_ASSERT(false);
                            break;
                    }
                    break;

                case PVP_ENGINE_STATE_PREPARING:
                    switch (nodecontext->iCmdType)
                    {
                        case PVP_CMD_SourceNodePrepare:
                            HandleSourceNodePrepare(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeQueryDataSourcePosition:
                            HandleSourceNodeQueryDataSourcePosition(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeSetDataSourcePosition:
                            HandleSourceNodeSetDataSourcePosition(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeSetDataSourceDirection:
                            //currently not allowed
                            break;

                        case PVP_CMD_SourceNodeStart:
                            HandleSourceNodeStart(*nodecontext, aResponse);
                            break;

                        default:
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid source node command type in PVP_ENGINE_STATE_PREPARING. Asserting"));
                            OSCL_ASSERT(false);
                            break;
                    }
                    break;

                case PVP_ENGINE_STATE_PAUSING:
                    HandleSourceNodePause(*nodecontext, aResponse);
                    break;

                case PVP_ENGINE_STATE_RESUMING:
                    switch (nodecontext->iCmdType)
                    {
                        case PVP_CMD_SourceNodeQueryDataSourcePosition:
                            HandleSourceNodeQueryDataSourcePosition(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeSetDataSourcePosition:
                            HandleSourceNodeSetDataSourcePosition(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeSetDataSourceDirection:
                            HandleSourceNodeSetDataSourceDirection(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeStart:
                            HandleSourceNodeResume(*nodecontext, aResponse);
                            break;

                        default:
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid source node command type in PVP_ENGINE_STATE_RESUMING. Asserting"));
                            OSCL_ASSERT(false);
                            break;
                    }
                    break;

                case PVP_ENGINE_STATE_STOPPING:
                    HandleSourceNodeStop(*nodecontext, aResponse);
                    break;

                case PVP_ENGINE_STATE_RESETTING:
                    HandleSourceNodeReset(*nodecontext, aResponse);
                    break;

                case PVP_ENGINE_STATE_HANDLINGERROR:
                    switch (nodecontext->iCmdType)
                    {
                        case PVP_CMD_SourceNodeStop:
                            HandleSourceNodeStopDueToError(*nodecontext, aResponse);
                            break;

                        case PVP_CMD_SourceNodeReset:
                            HandleSourceNodeResetDueToError(*nodecontext, aResponse);
                            break;

                        default:
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid source node command type in PVP_ENGINE_STATE_HANDLINGERROR. Asserting"));
                            OSCL_ASSERT(false);
                            break;
                    }
                    break;

                default:
                    break;
            }
        }
    }
    else if (iState == PVP_ENGINE_STATE_PREPARING)
    {
        switch (nodecontext->iCmdType)
        {
            case PVP_CMD_SinkNodeQueryFOConfigIF:
                HandleSinkNodeQueryFileOutConfigIF(*nodecontext, aResponse);
                break;

            case PVP_CMD_SinkNodeQuerySyncCtrlIF:
            case PVP_CMD_SinkNodeQueryMetadataIF:
            case PVP_CMD_SinkNodeQueryCapConfigIF:
                HandleSinkNodeQueryInterfaceOptional(*nodecontext, aResponse);
                break;

            case PVP_CMD_DecNodeQueryMetadataIF:
            case PVP_CMD_DecNodeQueryCapConfigIF:
                HandleDecNodeQueryInterfaceOptional(*nodecontext, aResponse);
                break;

            case PVP_CMD_SinkNodeSkipMediaData:
                HandleSinkNodeSkipMediaData(*nodecontext, aResponse);
                break;

            case PVP_CMD_SinkNodeDecNodeQueryCapConfigIF:
                HandleSinkNodeDecNodeQueryCapConfigIF(*nodecontext, aResponse);
                break;

            case PVP_CMD_SinkNodeDecNodeVerifyParameter:
                HandleSinkNodeDecNodeVerifyParameter(*nodecontext, aResponse);
                break;

            case PVP_CMD_SinkNodeDecNodeReset:
                HandleSinkNodeDecNodeReset(*nodecontext, aResponse);
                break;

            default:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::NodeCommandCompleted() Invalid node command type in PVP_ENGINE_STATE_PREPARING. Asserting"));
                OSCL_ASSERT(false);
                break;
        }
    }
    else
    {
        // Unknown node command completion. Assert
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::NodeCommandCompleted() Unknown node command completion"));
        OSCL_ASSERT(false);
    }

    // Remove the context from the list
    FreeEngineContext(nodecontext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::NodeCommandCompleted() Out"));
}


void PVPlayerEngine::HandleNodeInformationalEvent(const PVMFAsyncEvent& aEvent)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleNodeInformationalEvent() In"));

    PVMFNodeInterface* nodeorigin = (PVMFNodeInterface*)(aEvent.GetContext());

    PVPlayerNodeType nodetype = PVP_NODETYPE_UNKNOWN;
    int32 datapathindex = -1;

    // Process the info event based on the node type reporting the event
    if (nodeorigin == iSourceNode)
    {
        HandleSourceNodeInfoEvent(aEvent);
    }
    else if (FindNodeTypeByNode(nodeorigin, nodetype, datapathindex) == true)
    {
        if (nodetype == PVP_NODETYPE_SINK)
        {
            HandleSinkNodeInfoEvent(aEvent, datapathindex);
        }
        else if (nodetype == PVP_NODETYPE_DECODER)
        {
            HandleDecNodeInfoEvent(aEvent, datapathindex);
        }
        else
        {
            // Event from unknown node or component. Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleNodeInformationalEvent() Info event from unknown node type Event type 0x%x Context 0x%x Data 0x%x",
                            aEvent.GetEventType(), aEvent.GetContext(), aEvent.GetEventData()));
        }
    }
    else
    {
        // Event from unknown node or component. Do nothing but log it
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleNodeInformationalEvent() Info event from unknown node Event type 0x%x Context 0x%x Data 0x%x",
                        aEvent.GetEventType(), aEvent.GetContext(), aEvent.GetEventData()));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleNodeInformationalEvent() Out"));
}


void PVPlayerEngine::HandleNodeErrorEvent(const PVMFAsyncEvent& aEvent)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleNodeErrorEvent() In"));

    PVMFNodeInterface* nodeorigin = (PVMFNodeInterface*)(aEvent.GetContext());

    PVPlayerNodeType nodetype = PVP_NODETYPE_UNKNOWN;
    int32 datapathindex = -1;

    // Process the error event based on the node type reporting the event
    if (nodeorigin == iSourceNode)
    {
        HandleSourceNodeErrorEvent(aEvent);
    }
    else if (FindNodeTypeByNode(nodeorigin, nodetype, datapathindex) == true)
    {
        if (nodetype == PVP_NODETYPE_SINK)
        {
            HandleSinkNodeErrorEvent(aEvent, datapathindex);
        }
        else if (nodetype == PVP_NODETYPE_DECODER)
        {
            HandleDecNodeErrorEvent(aEvent, datapathindex);
        }
        else
        {
            // Event from unknown node or component. Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleNodeErrorEvent() Error event from unknown node type Event type 0x%x Context 0x%x Data 0x%x",
                            aEvent.GetEventType(), aEvent.GetContext(), aEvent.GetEventData()));
        }
    }
    else
    {
        // Event from unknown node or component. Do nothing but log it
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleNodeErrorEvent() Error event from unknown node Event type 0x%x Context 0x%x Data 0x%x",
                        aEvent.GetEventType(), aEvent.GetContext(), aEvent.GetEventData()));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleNodeErrorEvent() Out"));
}

void PVPlayerEngine::RemoveDatapathContextFromList()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDatapathContextFromList(): Erasing from ContextList iCurrentContextList.size() in : %d",
                    iCurrentContextList.size()));
    for (int32 i = iCurrentContextList.size() - 1; i >= 0; --i)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDatapathContextFromList(): iCurrentContextList[i]->iCmdType %d",
                        iCurrentContextList[i]->iCmdType));

        switch (iCurrentContextList[i]->iCmdType)
        {
            case PVP_CMD_DPPrepare:
            case PVP_CMD_DPStart:
            case PVP_CMD_DPStop:
            case PVP_CMD_DPTeardown:
            case PVP_CMD_DPReset:
                FreeEngineContext(iCurrentContextList[i]);
                break;
            default:
                break;
        }
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDatapathContextFromList(): iCurrentContextList.size() out : %d",
                    iCurrentContextList.size()));
}


void PVPlayerEngine::HandlePlayerDatapathEvent(int32 /*aDatapathEvent*/, PVMFStatus aEventStatus, OsclAny* aContext, PVMFCmdResp* aCmdResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() In"));

    // Check if a cancel command completed
    uint32* context_uint32 = (uint32*)aContext;
    if (context_uint32 == &iPendingCancelDueToCancelRequest)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Cancel in datapath completed for cancel command. Pending %d", iPendingCancelDueToCancelRequest));
        OSCL_ASSERT(iPendingCancelDueToCancelRequest > 0);
        --iPendingCancelDueToCancelRequest;
        if (iPendingCancelDueToCancelRequest == 0)
        {
            RemoveDatapathContextFromList(); // empty left over contexts from cancelled datapath commands
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Cancelling of all node/datapath command complete so continuing to shutdown"));
            DoShutdownDueToCancel();
        }
        return;
    }
    // Check if cancel called on node due to error
    else if (context_uint32 == &iPendingCancelDueToErrorRequest)
    {
        // Don't care about the response
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Cancel in datapath due to error completed. Pending %d", iPendingCancelDueToErrorRequest));
        --iPendingCancelDueToErrorRequest;
        if (iPendingCancelDueToErrorRequest == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Cancel due to error complete."));
            // Check that current cmd is the internal cancel-due-to-error command
            OSCL_ASSERT(iCurrentCmd.empty() == false);
            OSCL_ASSERT(iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_DUE_TO_ERROR);
            RemoveDatapathContextFromList(); // empty left over contexts from cancelled datapath commands
            // Call EngineCommandCompleted() so the internal cancel-due-to-error command
            // would be completed and next engine command in pending queue would be processed
            EngineCommandCompleted(-1, NULL, PVMFSuccess);

            // Clear out iCmdToCancel queue
            OSCL_ASSERT(iCmdToCancel.empty() == false);
            iCmdToCancel.clear();
        }
        return;
    }

    PVPlayerEngineContext* datapathcontext = (PVPlayerEngineContext*)aContext;
    OSCL_ASSERT(datapathcontext);

    // Ignore other datapath event if cancelling
    if (!iCmdToCancel.empty())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Datapath event ignored due to cancel process"));
        // Remove the context from the list
        FreeEngineContext(datapathcontext);
        return;
    }

    // Process the datapath event based on the engine state
    if (iState == PVP_ENGINE_STATE_PREPARING)
    {
        switch (datapathcontext->iCmdType)
        {
            case PVP_CMD_DPPrepare:
                HandleDatapathPrepare(*datapathcontext, aEventStatus, aCmdResp);
                break;

            case PVP_CMD_DPStart:
                HandleDatapathStart(*datapathcontext, aEventStatus, aCmdResp);
                break;

            default:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Invalid datapath command type in PVP_ENGINE_STATE_PREPARING. Asserting"));
                OSCL_ASSERT(false);
                break;
        }
    }
    else if (iState == PVP_ENGINE_STATE_PAUSING)
    {
        HandleDatapathPause(*datapathcontext, aEventStatus, aCmdResp);
    }
    else if (iState == PVP_ENGINE_STATE_RESUMING)
    {
        HandleDatapathResume(*datapathcontext, aEventStatus, aCmdResp);
    }
    else if (iState == PVP_ENGINE_STATE_STOPPING)
    {
        switch (datapathcontext->iCmdType)
        {
            case PVP_CMD_DPStop:
                HandleDatapathStop(*datapathcontext, aEventStatus, aCmdResp);
                break;

            case PVP_CMD_DPTeardown:
                HandleDatapathTeardown(*datapathcontext, aEventStatus, aCmdResp);
                break;

            case PVP_CMD_DPReset:
                HandleDatapathReset(*datapathcontext, aEventStatus, aCmdResp);
                break;

            default:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Invalid datapath command type in PVP_ENGINE_STATE_STOPPING. Asserting"));
                OSCL_ASSERT(false);
                break;
        }
    }
    else if (iState == PVP_ENGINE_STATE_HANDLINGERROR)
    {
        switch (datapathcontext->iCmdType)
        {
            case PVP_CMD_DPStop:
                HandleDatapathStopDueToError(*datapathcontext, aEventStatus, aCmdResp);
                break;

            case PVP_CMD_DPTeardown:
                HandleDatapathTeardownDueToError(*datapathcontext, aEventStatus, aCmdResp);
                break;

            case PVP_CMD_DPReset:
                HandleDatapathResetDueToError(*datapathcontext, aEventStatus, aCmdResp);
                break;

            default:
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Invalid datapath command type in PVP_ENGINE_STATE_HANDLINGERROR. Asserting"));
                OSCL_ASSERT(false);
                break;
        }
    }
    else
    {
        // Unknown datapath. Assert
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Invalid state for datapath command completion. Asserting"));
        OSCL_ASSERT(false);
    }

    // Remove the context from the list
    FreeEngineContext(datapathcontext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandlePlayerDatapathEvent() Out"));
}


void PVPlayerEngine::TimeoutOccurred(int32 timerID, int32 /*timeoutInfo*/)
{
    if (timerID == PVPLAYERENGINE_TIMERID_ENDTIMECHECK)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::TimeoutOccurred() Timer for EndTime check triggered"));

        PVPPlaybackPosition curpos;
        curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
        GetPlaybackClockPosition(curpos);

        if (iCurrentEndPosition.iIndeterminate || iCurrentEndPosition.iPosUnit != PVPPBPOSUNIT_MILLISEC)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::TimeoutOccurred() End time unit is invalid. Disabling end time check."));
            iEndTimeCheckEnabled = false;
            iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
            OSCL_ASSERT(false);
            return;
        }

        if (curpos.iPosValue.millisec_value >= iCurrentEndPosition.iPosValue.millisec_value)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::TimeoutOccurred() Specified end time reached so issuing pause command"));

            iEndTimeCheckEnabled = false;
            iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
            // Issues end time reached command
            AddCommandToQueue(PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDTIME_REACHED, NULL, NULL, NULL, false);
        }
        else if (!iEndTimeCheckEnabled)
        {
            iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
        }
    }
    else if (timerID == PVPLAYERENGINE_TIMERID_PLAY_STATUS)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::TimeoutOccurred() Timer for PlayStatus event triggered"));
        PVPPlaybackPosition curpos;
        curpos.iPosUnit = iPBPosStatusUnit;
        GetPlaybackClockPosition(curpos);

        uint8 poslocalbuffer[8];
        oscl_memset(poslocalbuffer, 0, 8);
        poslocalbuffer[0] = 1;
        switch (iPBPosStatusUnit)
        {
            case PVPPBPOSUNIT_SEC:
                oscl_memcpy(&poslocalbuffer[4], &(curpos.iPosValue.sec_value), sizeof(uint32));
                break;

            case PVPPBPOSUNIT_MIN:
                oscl_memcpy(&poslocalbuffer[4], &(curpos.iPosValue.min_value), sizeof(uint32));
                break;

            case PVPPBPOSUNIT_MILLISEC:
            default:
                oscl_memcpy(&poslocalbuffer[4], &(curpos.iPosValue.millisec_value), sizeof(uint32));
                break;
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* infomsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoPlaybackPositionStatus, puuid, NULL));
        // EventData parameter will be deprecated, and curpos will not be sent through
        // EventData in future.
        SendInformationalEvent(PVMFInfoPositionStatus, OSCL_STATIC_CAST(PVInterface*, infomsg), (OsclAny*)&curpos, poslocalbuffer, 8);
        infomsg->removeRef();

        if (!iPlayStatusTimerEnabled)
        {
            iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_PLAY_STATUS);
        }
    }
}


void PVPlayerEngine::CPMPluginCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CPMPluginCommandCompleted() In"));

    OSCL_ASSERT(iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR ||
                iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR);

    switch (iCPMPluginCommand)
    {
        case PVP_CMD_CPMQueryLicenseIF:
            HandleCPMPluginQueryLicenseIF(aResponse);
            break;

        case PVP_CMD_CPMQueryCapConfigIF:
            HandleCPMPluginQueryCapConfigIF(aResponse);
            break;

        case PVP_CMD_CPMGetLicense:
            HandleCPMPluginGetLicense(aResponse);
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::CPMPluginCommandCompleted() Unknown CPM plug-in command. Asserting"));
            OSCL_ASSERT(false);
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CPMPluginCommandCompleted() Out"));
}


void PVPlayerEngine::RecognizeCompleted(PVMFFormatType aSourceFormatType, OsclAny* aContext)
{
    // Check if a cancel command completed
    uint32* context_uint32 = (uint32*)(aContext);
    if (context_uint32 == &iPendingCancelDueToCancelRequest)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RecognizeCompleted() Recognize request cancelled"));
        OSCL_ASSERT(iPendingCancelDueToCancelRequest > 0);
        --iPendingCancelDueToCancelRequest;
        if (iPendingCancelDueToCancelRequest == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RecognizeCompleted() Cancelling of all pending commands complete so continue to shutdown"));
            RemoveDatapathContextFromList(); // empty left over contexts from cancelled datapath commands
            DoShutdownDueToCancel();
        }
        return;
    }

    // Ignore recognize completion if cancelling
    if (!iCmdToCancel.empty())
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RecognizeCompleted() Recognize completion ignored due to cancel process"));
        // Remove the context from the list
        FreeEngineContext((PVPlayerEngineContext*)(aContext));
        return;
    }

    // Save the recognized source format
    iSourceFormatType = aSourceFormatType;

    // Free the engine context after saving the cmd id and context
    PVPlayerEngineContext* reccontext = (PVPlayerEngineContext*)(aContext);
    OSCL_ASSERT(reccontext != NULL);
    PVCommandId cmdid = reccontext->iCmdId;
    OsclAny* cmdcontext = reccontext->iCmdContext;
    FreeEngineContext(reccontext);

    // Start the source node creation and setup sequence
    PVMFStatus retval = DoSetupSourceNode(cmdid, cmdcontext);

    if (retval != PVMFSuccess)
    {
        // Do cleanup
        DoSourceNodeCleanup();
        iDataSource = NULL;

        // Complete the AddDataSource() command with failure
        EngineCommandCompleted(cmdid, cmdcontext, retval);
    }
}


PVCommandId PVPlayerEngine::AddCommandToQueue(int32 aCmdType, OsclAny* aContextData,
        Oscl_Vector<PVPlayerEngineCommandParamUnion, OsclMemAllocator>* aParamVector,
        const PVUuid* aUuid, bool aAPICommand)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::AddCommandToQueue() In CmdType %d, CmdId %d", aCmdType, iCommandId));

    PVPlayerEngineCommand cmd(aCmdType, iCommandId, aContextData, aParamVector, aAPICommand);
    if (aUuid)
    {
        cmd.SetUuid(*aUuid);
    }

    /*
     * If engine is in error or error handling state, then don't add external commands to the queue
     */
    int32 leavecode = 0;
    if (aAPICommand == true)   //External Command
    {
        if (iState == PVP_ENGINE_STATE_ERROR || iState == PVP_ENGINE_STATE_HANDLINGERROR)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::AddCommandToQueue() Invalid state due to Error Handling : CmdType %d, CmdId %d", aCmdType, iCommandId));
            OSCL_LEAVE(OsclErrInvalidState);
            return -1;       // Dummy Cmdid
        }

    }


    OSCL_TRY(leavecode, iPendingCmds.push(cmd));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::AddCommandToQueue() Adding command to pending command list did a leave!"));
                         OSCL_ASSERT(false);
                         return -1;);

    RunIfNotReady();

    ++iCommandId;
    if (iCommandId == 0x7FFFFFFF)
    {
        iCommandId = 0;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::AddCommandToQueue() Type=%d ID=%d APIcmd=%d Tick=%d",
                     aCmdType, cmd.GetCmdId(), aAPICommand, OsclTickCount::TickCount()));

    return cmd.GetCmdId();
}


void PVPlayerEngine::SetEngineState(PVPlayerEngineState aState)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SetEngineState() In Current state %d, New state %d", iState, aState));
    iState = aState;
}


PVPlayerState PVPlayerEngine::GetPVPlayerState(void)
{
    switch (iState)
    {
        case PVP_ENGINE_STATE_IDLE:
        case PVP_ENGINE_STATE_INITIALIZING:
            return PVP_STATE_IDLE;

        case PVP_ENGINE_STATE_INITIALIZED:
        case PVP_ENGINE_STATE_PREPARING:
        case PVP_ENGINE_STATE_RESETTING:
            return PVP_STATE_INITIALIZED;

        case PVP_ENGINE_STATE_PREPARED:
        case PVP_ENGINE_STATE_STARTING:
            return PVP_STATE_PREPARED;

        case PVP_ENGINE_STATE_STARTED:
        case PVP_ENGINE_STATE_AUTO_PAUSING:
        case PVP_ENGINE_STATE_AUTO_PAUSED:
        case PVP_ENGINE_STATE_AUTO_RESUMING:
        case PVP_ENGINE_STATE_PAUSING:
        case PVP_ENGINE_STATE_STOPPING:
            return PVP_STATE_STARTED;

        case PVP_ENGINE_STATE_PAUSED:
        case PVP_ENGINE_STATE_RESUMING:
            return PVP_STATE_PAUSED;


        case PVP_ENGINE_STATE_HANDLINGERROR:
        case PVP_ENGINE_STATE_ERROR:
            return PVP_STATE_ERROR;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetPVPlayerState() Unknown engine state. Asserting"));
            OSCL_ASSERT(false);
            break;
    }

    return PVP_STATE_ERROR;
}


void PVPlayerEngine::GetPlaybackClockPosition(PVPPlaybackPosition& aClockPos)
{
    bool tmpbool = false;
    uint32 clockcurpos = 0;
    aClockPos.iIndeterminate = false;

    int32 nptcurpos;

    if (!iChangeDirectionNPT.iIndeterminate)
    {
        // report the expected NPT after the direction change
        // to avoid weird transient values between the direction change
        // and the repositioning completion.
        nptcurpos = iChangeDirectionNPT.iPosValue.millisec_value;
    }
    else
    {
        // Get current playback clock position
        iPlaybackClock.GetCurrentTime32(clockcurpos, tmpbool, OSCLCLOCK_MSEC);

        nptcurpos = iStartNPT + iPlaybackDirection * (clockcurpos - iStartMediaDataTS);
    }
    if (nptcurpos < 0)
    {
        nptcurpos = 0;
    }

    if (ConvertFromMillisec((uint32)nptcurpos, aClockPos) != PVMFSuccess)
    {
        // Other position units are not supported yet
        aClockPos.iIndeterminate = true;
    }
}


PVMFStatus PVPlayerEngine::ConvertToMillisec(PVPPlaybackPosition aPBPos, uint32& aTimeMS)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::ConvertToMillisec() In"));

    if (aPBPos.iIndeterminate)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::ConvertToMillisec() Indeterminate position"));
        return PVMFErrArgument;
    }

    bool owallclockunits = false;
    switch (aPBPos.iPosUnit)
    {
        case PVPPBPOSUNIT_MILLISEC:
            aTimeMS = aPBPos.iPosValue.millisec_value;
            owallclockunits = true;
            break;

        case PVPPBPOSUNIT_SEC:
            aTimeMS = aPBPos.iPosValue.sec_value * 1000;
            owallclockunits = true;
            break;

        case PVPPBPOSUNIT_MIN:
            aTimeMS = aPBPos.iPosValue.min_value * 60000;
            owallclockunits = true;
            break;

        case PVPPBPOSUNIT_HOUR:
            aTimeMS = aPBPos.iPosValue.hour_value * 3600000;
            owallclockunits = true;
            break;

        case PVPPBPOSUNIT_SMPTE:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() SMPTE not supported yet"));
            return PVMFErrArgument;

        case PVPPBPOSUNIT_PERCENT:
        {
            if (iSourceDurationAvailable == false)
            {
                // Duration info not available from source node so can't convert
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Duration not available so can't convert"));
                return PVMFErrArgument;
            }

            if (iSourceDurationInMS == 0)
            {
                // Duration is 0 so can't convert
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Duration value is 0 so can't convert"));
                return PVMFErrArgument;
            }

            if (aPBPos.iPosValue.percent_value >= 100)
            {
                // If percentage greater than 100, cap to 100%
                aTimeMS = iSourceDurationInMS;
            }
            else
            {
                // Calculate time in millseconds based on percentage of duration
                aTimeMS = (aPBPos.iPosValue.percent_value * iSourceDurationInMS) / 100;
            }
        }
        break;

        case PVPPBPOSUNIT_SAMPLENUMBER:
        {
            if (iSourceNodeTrackLevelInfoIF == NULL)
            {
                // The source node doesn't have the query IF to convert samplenum to time
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Sample number to time conversion not available"));
                return PVMFErrArgument;
            }

            // Determine which track to use for conversion.
            // Give preference to video track, then text, and finally audio
            PVMFTrackInfo* track = NULL;
            PVPlayerMediaType selmediatype = PVP_MEDIATYPE_UNKNOWN;

            // Search from the datapath list.
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    switch (iDatapathList[i].iMediaType)
                    {
                        case PVP_MEDIATYPE_VIDEO:
                            track = iDatapathList[i].iTrackInfo;
                            selmediatype = PVP_MEDIATYPE_VIDEO;
                            // Video track found so no need to check further
                            i = iDatapathList.size();
                            break;

                        case PVP_MEDIATYPE_TEXT:
                            track = iDatapathList[i].iTrackInfo;
                            selmediatype = PVP_MEDIATYPE_TEXT;
                            break;

                        case PVP_MEDIATYPE_AUDIO:
                            if (selmediatype != PVP_MEDIATYPE_TEXT)
                            {
                                track = iDatapathList[i].iTrackInfo;
                                selmediatype = PVP_MEDIATYPE_TEXT;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }

            if (track == NULL)
            {
                // Track is not available to do the conversion
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Track not selected for conversion"));
                return PVMFErrArgument;
            }

            // Convert the sample number to time in milliseconds
            PVMFTimestamp framets = 0;
            if (iSourceNodeTrackLevelInfoIF->GetTimestampForSampleNumber(*track, aPBPos.iPosValue.samplenum_value, framets) != PVMFSuccess)
            {
                // Conversion failed
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Sample number to time conversion failed"));
                return PVMFErrArgument;
            }

            aTimeMS = framets;
        }
        break;

        case PVPPBPOSUNIT_DATAPOSITION:
        {
            if (iSourceNodeTrackLevelInfoIF == NULL)
            {
                // The source node doesn't have the ext IF to convert data position to time
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Data position to time conversion not available"));
                return PVMFErrArgument;
            }

            // Go through each active track and find the minimum time for given data position
            bool mintsvalid = false;
            PVMFTimestamp mints = 0xFFFFFFFF;
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    PVMFTimestamp curts = 0;
                    if (iSourceNodeTrackLevelInfoIF->GetTimestampForDataPosition(*(iDatapathList[i].iTrackInfo), aPBPos.iPosValue.datapos_value, curts) != PVMFSuccess)
                    {
                        // Conversion failed
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Data position to time conversion failed"));
                    }
                    else
                    {
                        // Conversion succeeded. Save only if it is the minimum encountered so far.
                        mintsvalid = true;
                        if (curts < mints)
                        {
                            mints = curts;
                        }
                    }
                }
            }

            if (mintsvalid == false)
            {
                // Conversion on all active tracks failed
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Data position to time conversion could not be done on any active track"));
                return PVMFErrArgument;
            }

            aTimeMS = mints;
        }
        break;

        case PVPPBPOSUNIT_PLAYLIST:
        {
            switch (aPBPos.iPlayListPosUnit)
            {
                case PVPPBPOSUNIT_MILLISEC:
                    aTimeMS = aPBPos.iPlayListPosValue.millisec_value;
                    break;

                case PVPPBPOSUNIT_SEC:
                    aTimeMS = aPBPos.iPlayListPosValue.sec_value * 1000;
                    break;

                case PVPPBPOSUNIT_MIN:
                    aTimeMS = aPBPos.iPlayListPosValue.min_value * 60000;
                    break;

                case PVPPBPOSUNIT_HOUR:
                    aTimeMS = aPBPos.iPlayListPosValue.hour_value * 3600000;
                    break;

                default:
                    // Don't support the other units for now
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Unsupported playlist position units"));
                    return PVMFErrArgument;
            }
        }
        break;

        default:
            // Don't support the other units for now
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertToMillisec() Unsupported position units"));
            return PVMFErrArgument;
    }

    if (owallclockunits == true)
    {
        if ((aTimeMS > iSourceDurationInMS) && (iSourceDurationAvailable == true))
        {
            //cap time to clip duration
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVPlayerEngine::ConvertToMillisec() Capping value - Acutal=%d, CappedValue=%d",
                            aTimeMS, iSourceDurationInMS));
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_DEBUG, (0, "PVPlayerEngine::ConvertToMillisec() Capping value - Acutal=%d, CappedValue=%d",
                            aTimeMS, iSourceDurationInMS));
            aTimeMS = iSourceDurationInMS;
        }
        else
        {
            // just pass the converted time even if duration is not available and let
            // source node handle the request.
        }
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::ConvertToMillisec() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::ConvertFromMillisec(uint32 aTimeMS, PVPPlaybackPosition& aPBPos)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::ConvertFromMillisec() In"));

    // Convert to specified time units
    switch (aPBPos.iPosUnit)
    {
        case PVPPBPOSUNIT_MILLISEC:
            aPBPos.iPosValue.millisec_value = aTimeMS;
            break;

        case PVPPBPOSUNIT_SEC:
            aPBPos.iPosValue.sec_value = aTimeMS / 1000;
            break;

        case PVPPBPOSUNIT_MIN:
            aPBPos.iPosValue.min_value = aTimeMS / 60000;
            break;

        case PVPPBPOSUNIT_HOUR:
            aPBPos.iPosValue.hour_value = aTimeMS / 3600000;
            break;

        case PVPPBPOSUNIT_SMPTE:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::ConvertFromMillisec() SMPTE units not supported yet"));
            return PVMFErrArgument;

        case PVPPBPOSUNIT_PERCENT:
        {
            if (iSourceDurationAvailable == false)
            {
                // Duration info not available from source node so can't convert
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Duration not available so can't convert"));
                return PVMFErrArgument;
            }

            if (iSourceDurationInMS == 0)
            {
                // Duration is 0 so can't convert
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Duration value is 0 so can't convert"));
                return PVMFErrArgument;
            }

            if (aTimeMS >= iSourceDurationInMS)
            {
                // Put a ceiling of 100%
                aPBPos.iPosValue.percent_value = 100;
            }
            else
            {
                // Calculate percentage of playback
                aPBPos.iPosValue.percent_value = (aTimeMS * 100) / iSourceDurationInMS;
            }
        }
        break;

        case PVPPBPOSUNIT_SAMPLENUMBER:
        {
            if (iSourceNodeTrackLevelInfoIF == NULL)
            {
                // The source node doesn't have the query IF to convert time to sample number
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Time to sample numberconversion not available"));
                return PVMFErrArgument;
            }

            // Determine which track to use for conversion.
            // Give preference to video track
            PVMFTrackInfo* track = NULL;
            PVPlayerMediaType selmediatype = PVP_MEDIATYPE_UNKNOWN;

            // Search from the datapath list.
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    switch (iDatapathList[i].iMediaType)
                    {
                        case PVP_MEDIATYPE_VIDEO:
                            track = iDatapathList[i].iTrackInfo;
                            selmediatype = PVP_MEDIATYPE_VIDEO;
                            // Video track found so no need to check further
                            i = iDatapathList.size();
                            break;

                        case PVP_MEDIATYPE_TEXT:
                            track = iDatapathList[i].iTrackInfo;
                            selmediatype = PVP_MEDIATYPE_TEXT;
                            break;

                        case PVP_MEDIATYPE_AUDIO:
                            if (selmediatype != PVP_MEDIATYPE_TEXT)
                            {
                                track = iDatapathList[i].iTrackInfo;
                                selmediatype = PVP_MEDIATYPE_TEXT;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }

            if (track == NULL)
            {
                // Track is not available to do the conversion
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Track not selected for conversion"));
                return PVMFErrArgument;
            }

            // Convert the time to sample number
            PVMFTimestamp ts = aTimeMS;
            uint32 samplenum = 0;
            if (iSourceNodeTrackLevelInfoIF->GetSampleNumberForTimestamp(*track, ts, samplenum) != PVMFSuccess)
            {
                // Conversion failed
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Sample number to time conversion failed"));
                return PVMFErrArgument;
            }

            aPBPos.iPosValue.samplenum_value = samplenum;
        }
        break;

        case PVPPBPOSUNIT_DATAPOSITION:
        {
            if (iSourceNodeTrackLevelInfoIF == NULL)
            {
                // The source node doesn't have the ext IF to convert time to data position
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Time to data position conversion not available in source node"));
                return PVMFErrArgument;
            }

            // Query each active track for its data position
            // Return the max data position
            PVMFTimestamp ts = aTimeMS;
            uint32 maxdatapos = 0;
            bool maxdataposvalid = false;

            // Go through each active track
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    uint32 curdatapos = 0;
                    // Convert the time to data position
                    if (iSourceNodeTrackLevelInfoIF->GetDataPositionForTimestamp(*(iDatapathList[i].iTrackInfo), ts, curdatapos) != PVMFSuccess)
                    {
                        // Conversion failed
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Time to data position conversion failed"));
                    }
                    else
                    {
                        // Save the data position if it is greater than
                        // any position encountered so far.
                        maxdataposvalid = true;
                        if (curdatapos > maxdatapos)
                        {
                            maxdatapos = curdatapos;
                        }
                    }
                }
            }

            if (maxdataposvalid == false)
            {
                // Conversion failed for all active tracks
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::ConvertFromMillisec() Track not selected for conversion"));
                return PVMFErrArgument;
            }
            // Save the data position to return
            aPBPos.iPosValue.datapos_value = maxdatapos;
        }
        break;;

        default:
            // Don't support the other units for now
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::ConvertFromMillisec() Unsupported position units"));
            return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::ConvertFromMillisec() Out"));
    return PVMFSuccess;
}


void PVPlayerEngine::EngineCommandCompleted(PVCommandId aId, OsclAny* aContext, PVMFStatus aStatus, PVInterface* aExtInterface, OsclAny* aEventData, int32 aEventDataSize)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::EngineCommandCompleted() In CmdId %d, Status %d", aId, aStatus));

    // Update the current command vector

    // Assert if the current cmd is not saved or the cmd ID does not match
    OSCL_ASSERT(iCurrentCmd.size() == 1);
    OSCL_ASSERT(iCurrentCmd[0].GetCmdId() == aId);

    // Empty out the current cmd vector and set active if there are other pending commands
    PVPlayerEngineCommand completedcmd(iCurrentCmd[0]);
    iCurrentCmd.erase(iCurrentCmd.begin());
    if (!iPendingCmds.empty())
    {
        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::EngineCommandCompleted() Type=%d ID=%d APIcmd=%d Tick=%d",
                     completedcmd.GetCmdType(), completedcmd.GetCmdId(), completedcmd.IsAPICommand(), OsclTickCount::TickCount()));

    // Send informational event or send other callback if needed
    switch (completedcmd.GetCmdType())
    {
        case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDOFCLIP:
            SendEndOfClipInfoEvent(aStatus, aExtInterface);
            break;

        case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDTIME_REACHED:
            SendEndTimeReachedInfoEvent(aStatus, aExtInterface);
            break;

        case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW:
            SendSourceUnderflowInfoEvent(aStatus, aExtInterface);
            break;

        case PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY:
            SendSourceDataReadyInfoEvent(aStatus, aExtInterface);
            break;

        case PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR:
        case PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR:
            SendErrorHandlingCompleteEvent(aStatus, aExtInterface);
            break;

        case PVP_ENGINE_COMMAND_CAPCONFIG_SET_PARAMETERS:
            // Send callback to the specified observer
            if (iCfgCapCmdObserver)
            {
                iCfgCapCmdObserver->SignalEvent(aId);
            }
            break;

        default:
            // None to be sent
            break;
    }

    // Send the command completed event
    if (iCmdStatusObserver)
    {
        if (aId != -1 && completedcmd.IsAPICommand())
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::EngineCommandCompleted() Notifying engine command as completed. CmdId %d Status %d", aId, aStatus));
            PVCmdResponse cmdcompleted(aId, aContext, aStatus, aExtInterface, aEventData, aEventDataSize);
            iCmdStatusObserver->CommandCompleted(cmdcompleted);
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::EngineCommandCompleted() aId is -1 or not an API command. CmdType %d", completedcmd.GetCmdType()));
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::EngineCommandCompleted() iCmdStatusObserver is NULL"));
    }
}


void PVPlayerEngine::SendInformationalEvent(PVMFEventType aEventType, PVInterface* aExtInterface, OsclAny* aEventData, uint8* aLocalBuffer, uint32 aLocalBufferSize)
{
    // Send the info event if observer has been specified
    if (iInfoEventObserver)
    {
        PVAsyncInformationalEvent infoevent((PVEventType)aEventType, NULL, aExtInterface, (PVExclusivePtr)aEventData, aLocalBuffer, aLocalBufferSize);
        iInfoEventObserver->HandleInformationalEvent(infoevent);
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SendInformationalEvent() iInfoEventObserver is NULL"));
    }
}


void PVPlayerEngine::SendErrorEvent(PVMFEventType aEventType, PVInterface* aExtInterface, OsclAny* aEventData, uint8* aLocalBuffer, uint32 aLocalBufferSize)
{
    // Send the error event if observer has been specified
    if (iErrorEventObserver)
    {
        PVAsyncErrorEvent errorevent((PVEventType)aEventType, NULL, aExtInterface, (PVExclusivePtr)aEventData, aLocalBuffer, aLocalBufferSize);
        iErrorEventObserver->HandleErrorEvent(errorevent);
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SendErrorEvent() iErrorEventObserver is NULL"));
    }
}


void PVPlayerEngine::DoCancelAllCommands(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelAllCommands() In"));

    // Engine cannot be processing another cancel command
    OSCL_ASSERT(iCmdToCancel.empty() == true);

    // While AcquireLicense and CancelAcquireLicense is processing, CancelAllCommands is prohibited.
    if (!iCmdToDlaCancel.empty() || iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_ACQUIRE_LICENSE ||
            iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR || iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelAllCommands() Error due to processing AcquireLicense or CancelAcquireLicense,CmdType=%d", iCurrentCmd[0].GetCmdType()));

        PVPlayerEngineCommand currentcmd(iCurrentCmd[0]);
        iCurrentCmd.erase(iCurrentCmd.begin());
        iCurrentCmd.push_front(aCmd);
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFErrArgument);
        iCurrentCmd.push_front(currentcmd);
        return;
    }
    // Cancel the current command first
    if (iCurrentCmd.size() == 1)
    {
        // First save the current command being processed
        iCmdToCancel.push_front(iCurrentCmd[0]);
        // Cancel it
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFErrCancelled);
    }

    // Cancel all the pending commands

    // Create a temporary queue for pending commands
    OsclPriorityQueue<PVPlayerEngineCommand, OsclMemAllocator, Oscl_Vector<PVPlayerEngineCommand, OsclMemAllocator>, PVPlayerEngineCommandCompareLess> iTempPendingCmds;
    // Copy the pending commands to the new queue
    iTempPendingCmds = iPendingCmds;
    while (!iTempPendingCmds.empty())
    {
        // Get the queue from the top
        PVPlayerEngineCommand cmd(iTempPendingCmds.top());
        // Check if it needs to be cancelled
        if ((aCmd.GetCmdId() > cmd.GetCmdId()) && ((aCmd.GetCmdId() - cmd.GetCmdId()) < 0x80000000))
        {
            // Remove it from the pending commands queue
            iPendingCmds.remove(cmd);
            // Save it temporary as "current command" and then cancel it
            iCurrentCmd.push_front(cmd);
            EngineCommandCompleted(cmd.GetCmdId(), cmd.GetContext(), PVMFErrCancelled);
        }
        // Pop each cmd from the temporary queue
        iTempPendingCmds.pop();
    }

    // Make the CancelAll() command the current command
    iCurrentCmd.push_front(aCmd);

    // Check if there was an ongoing command that needs to be properly cancelled
    if (!iCmdToCancel.empty())
    {
        // Properly cancel a command being currently processed
        DoCancelCommandBeingProcessed();
    }
    else
    {
        // CancelAll() command is completed so send the completion event
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelAllCommands() Out"));
}


void PVPlayerEngine::DoCancelCommandBeingProcessed(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelCommandBeingProcessed() In"));

    // There should be a command to cancel
    OSCL_ASSERT(iCmdToCancel.empty() == false);

    switch (iCmdToCancel[0].GetCmdType())
    {
        case PVP_ENGINE_COMMAND_ADD_DATA_SOURCE:
        case PVP_ENGINE_COMMAND_INIT:
        case PVP_ENGINE_COMMAND_PREPARE:
        case PVP_ENGINE_COMMAND_PAUSE:
        case PVP_ENGINE_COMMAND_RESUME:
        case PVP_ENGINE_COMMAND_SET_PLAYBACK_RANGE:
        case PVP_ENGINE_COMMAND_STOP:
            if (!iCurrentContextList.empty())
            {
                // Since there is a pending node or datapath, cancel it
                DoCancelPendingNodeDatapathCommand();
            }
            else
            {
                // No pending command so contine to shutdown
                DoShutdownDueToCancel();
            }
            break;

        case PVP_ENGINE_COMMAND_RESET:
        case PVP_ENGINE_COMMAND_SET_PLAYBACK_RATE:
        case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDTIME_REACHED:
        case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDOFCLIP:
        case PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW:
        case PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY:
        case PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR:
        case PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR:
        case PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR:
        case PVP_ENGINE_COMMAND_GET_METADATA_KEY:
        case PVP_ENGINE_COMMAND_GET_METADATA_VALUE:
        case PVP_ENGINE_COMMAND_DATAPATH_DELETE:
            // Complete the CancelAll() but allow the pending command to complete.
            // This is necessary until transition cancel is fully implemented.
            EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFSuccess);
            iCurrentCmd.push_front(iCmdToCancel[0]);
            iCmdToCancel.clear();
            break;

        case PVP_ENGINE_COMMAND_QUERY_UUID:
        case PVP_ENGINE_COMMAND_QUERY_INTERFACE:
        case PVP_ENGINE_COMMAND_GET_PLAYBACK_RANGE:
        case PVP_ENGINE_COMMAND_GET_PLAYBACK_RATE:
        case PVP_ENGINE_COMMAND_GET_PLAYBACK_MINMAX_RATE:
        case PVP_ENGINE_COMMAND_GET_SDK_INFO:
        case PVP_ENGINE_COMMAND_GET_SDK_MODULE_INFO:
        case PVP_ENGINE_COMMAND_SET_LOG_APPENDER:
        case PVP_ENGINE_COMMAND_REMOVE_LOG_APPENDER:
        case PVP_ENGINE_COMMAND_SET_LOG_LEVEL:
        case PVP_ENGINE_COMMAND_GET_LOG_LEVEL:
        case PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS:
        case PVP_ENGINE_COMMAND_GET_PVPLAYER_STATE:
        case PVP_ENGINE_COMMAND_ADD_DATA_SINK:
        case PVP_ENGINE_COMMAND_GET_CURRENT_POSITION:
        case PVP_ENGINE_COMMAND_START:
        case PVP_ENGINE_COMMAND_REMOVE_DATA_SINK:
        case PVP_ENGINE_COMMAND_REMOVE_DATA_SOURCE:
        default:
            // These commands should complete in one AO scheduling so there should be no need to cancel.
            // CancelAll() is done so complete it
            EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFSuccess);
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelCommandBeingProcessed() Out"));
}


void PVPlayerEngine::DoCancelPendingNodeDatapathCommand()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() In"));

    OSCL_ASSERT(iCurrentContextList.empty() == false);

    // Determine where the pending commands were issued to and then cancel them
    int32 leavecode = 0;
    iPendingCancelDueToCancelRequest = 0;
    for (uint32 i = 0; i < iCurrentContextList.size(); ++i)
    {
        if (iCurrentContextList[i]->iNode)
        {
            if (iCurrentContextList[i]->iNode == iSourceNode)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Calling CancelAllCommands() on source node"));
                leavecode = 0;
                OSCL_TRY(leavecode, iSourceNode->CancelAllCommands(iSourceNodeSessionId, (OsclAny*)&iPendingCancelDueToCancelRequest));
                if (leavecode == 0)
                {
                    ++iPendingCancelDueToCancelRequest;
                }
            }
            else if (iCurrentContextList[i]->iEngineDatapath != NULL)
            {
                if (iCurrentContextList[i]->iNode == iCurrentContextList[i]->iEngineDatapath->iSinkNode)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Calling CancelAllCommands() on sink node"));
                    leavecode = 0;
                    OSCL_TRY(leavecode, iCurrentContextList[i]->iNode->CancelAllCommands(iCurrentContextList[i]->iEngineDatapath->iSinkNodeSessionId, (OsclAny*)&iPendingCancelDueToCancelRequest));
                    if (leavecode == 0)
                    {
                        ++iPendingCancelDueToCancelRequest;
                    }
                }
                else if (iCurrentContextList[i]->iNode == iCurrentContextList[i]->iEngineDatapath->iDecNode)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Calling CancelAllCommands() on dec node"));
                    leavecode = 0;
                    OSCL_TRY(leavecode, iCurrentContextList[i]->iNode->CancelAllCommands(iCurrentContextList[i]->iEngineDatapath->iDecNodeSessionId, (OsclAny*)&iPendingCancelDueToCancelRequest));
                    if (leavecode == 0)
                    {
                        ++iPendingCancelDueToCancelRequest;
                    }
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Unknown node type. Asserting"));
                    OSCL_ASSERT(false);
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Unknown node. Asserting"));
                OSCL_ASSERT(false);
            }
        }
        else if (iCurrentContextList[i]->iDatapath != NULL)
        {
            if (iCurrentContextList[i]->iEngineDatapath != NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Calling CancelAllCommands() on datapath"));
                leavecode = 0;
                OSCL_TRY(leavecode, iCurrentContextList[i]->iDatapath->CancelCommand((OsclAny*)&iPendingCancelDueToCancelRequest));
                if (leavecode == 0)
                {
                    ++iPendingCancelDueToCancelRequest;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Unknown datapath. Asserting"));
                OSCL_ASSERT(false);
            }
        }
        else if (iCurrentContextList[i]->iCmdType == PVP_CMD_QUERYSOURCEFORMATTYPE)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Calling CancelAllCommands() on recognizer"));
            leavecode = 0;
            OSCL_TRY(leavecode, iPlayerRecognizerRegistry.CancelQuery((OsclAny*)&iPendingCancelDueToCancelRequest));
            if (leavecode == 0)
            {
                ++iPendingCancelDueToCancelRequest;
            }
        }
        else
        {
            // Either a node or datapath should be pending
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() No pending node or datapath. Asserting"));
            OSCL_ASSERT(false);
        }
    }

    if (iPendingCancelDueToCancelRequest == 0)
    {
        // Cancel on the node failed so go to next step
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() CancelAllCommands() on the node did a leave"));
        RemoveDatapathContextFromList(); // remove left-over datapath contexts
        DoShutdownDueToCancel();
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() %d CancelAllCommands are pending", iPendingCancelDueToCancelRequest));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelPendingNodeDatapathCommand() Out"));
}


void PVPlayerEngine::DoShutdownDueToCancel()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoShutdownDueToCancel() In"));

    // There should be a command to cancel
    OSCL_ASSERT(iCmdToCancel.empty() == false);

    switch (iCmdToCancel[0].GetCmdType())
    {
        case PVP_ENGINE_COMMAND_ADD_DATA_SOURCE:
        case PVP_ENGINE_COMMAND_SET_PLAYBACK_RANGE:
        case PVP_ENGINE_COMMAND_PREPARE:
        case PVP_ENGINE_COMMAND_PAUSE:
        case PVP_ENGINE_COMMAND_RESUME:
        case PVP_ENGINE_COMMAND_STOP:
            DoStopDueToCancel();
            break;

        case PVP_ENGINE_COMMAND_INIT:
            DoResetDueToCancel();
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoShutdownDueToCancel() Unsupported command to cancel. Asserting"));
            OSCL_ASSERT(false);
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoShutdownDueToCancel() Out"));
}


void PVPlayerEngine::DoStopDueToCancel(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToCancel() In"));

    int32 leavecode = 0;
    iPendingStopDueToCancelRequest = 0;
    if (iSourceNode)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToCancel() Calling Stop() on source node"));
        leavecode = 0;
        OSCL_TRY(leavecode, iSourceNode->Stop(iSourceNodeSessionId, &iPendingStopDueToCancelRequest));
        if (!leavecode)
        {
            ++iPendingStopDueToCancelRequest;
        }
    }

    // Go through each active datapath and stop the decoder and sink nodes
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive)
        {
            if (iDatapathList[i].iDecNode)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToCancel() Calling Stop() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, iDatapathList[i].iDecNode->Stop(iDatapathList[i].iDecNodeSessionId, &iPendingStopDueToCancelRequest));
                if (!leavecode)
                {
                    ++iPendingStopDueToCancelRequest;
                }
            }
            if (iDatapathList[i].iSinkNode)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToCancel() Calling Stop() on sink node"));
                leavecode = 0;
                OSCL_TRY(leavecode, iDatapathList[i].iSinkNode->Stop(iDatapathList[i].iSinkNodeSessionId, &iPendingStopDueToCancelRequest));
                if (!leavecode)
                {
                    ++iPendingStopDueToCancelRequest;
                }
            }
        }
    }

    if (iPendingStopDueToCancelRequest == 0)
    {
        // All Stop() on the nodes failed so go to next step
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoStopDueToCancel() All Stop() on the nodes did a leave"));
        DoResetDueToCancel();
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoStopDueToCancel() %d Stops are pending", iPendingStopDueToCancelRequest));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToCancel() Out"));
}


void PVPlayerEngine::DoResetDueToCancel(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToCancel() In"));

    int32 leavecode = 0;
    iPendingResetDueToCancelRequest = 0;
    if (iSourceNode)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToCancel() Calling Reset() on source node"));
        leavecode = 0;
        OSCL_TRY(leavecode, iSourceNode->Reset(iSourceNodeSessionId, &iPendingResetDueToCancelRequest));
        if (!leavecode)
        {
            ++iPendingResetDueToCancelRequest;
        }
    }

    // Go through each active datapath and reset the decoder and sink nodes
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive)
        {
            if (iDatapathList[i].iDecNode)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToCancel() Calling Reset() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, iDatapathList[i].iDecNode->Reset(iDatapathList[i].iDecNodeSessionId, &iPendingResetDueToCancelRequest));
                if (!leavecode)
                {
                    ++iPendingResetDueToCancelRequest;
                }
            }
            if (iDatapathList[i].iSinkNode)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToCancel() Calling Reset() on sink node"));
                leavecode = 0;
                OSCL_TRY(leavecode, iDatapathList[i].iSinkNode->Reset(iDatapathList[i].iSinkNodeSessionId, &iPendingResetDueToCancelRequest));
                if (!leavecode)
                {
                    ++iPendingResetDueToCancelRequest;
                }
            }
        }
    }

    if (iPendingResetDueToCancelRequest == 0)
    {
        // All Reset() on the nodes failed so go to next step
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoResetDueToCancel() All Reset() on the nodes did a leave"));
        DoCleanupDueToCancel();
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToCancel() %d Resets are pending", iPendingResetDueToCancelRequest));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToCancel() Out"));
}


void PVPlayerEngine::DoCleanupDueToCancel(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCleanupDueToCancel() In"));

    // Wipe everything out
    iPollingCheckTimer->Clear();

    // Clean up the datapaths
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        DoEngineDatapathCleanup(iDatapathList[i]);
    }
    iDatapathList.clear();

    // Clean up the source node
    DoSourceNodeCleanup();

    while (!iCurrentContextList.empty())
    {
        FreeEngineContext(iCurrentContextList[0]);
    }

    SetEngineState(PVP_ENGINE_STATE_IDLE);

    iCmdToCancel.clear();
    EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCleanupDueToCancel() Out"));
}


PVMFStatus PVPlayerEngine::DoGetSDKInfo(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetSDKInfo() In"));

    PVSDKInfo* sdkinfo = (PVSDKInfo*)(aCmd.GetParam(0).pOsclAny_value);
    if (sdkinfo == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetSDKInfo() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Set the SDK info to the ones defined in the header file pv_player_sdkinfo.h generated at build time
    sdkinfo->iLabel = PVPLAYER_ENGINE_SDKINFO_LABEL;
    sdkinfo->iDate = PVPLAYER_ENGINE_SDKINFO_DATE;

    EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetSDKInfo() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSetLogAppender(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetLogAppender() In"));

    char* tag = (char*)(aCmd.GetParam(0).pChar_value);
    OsclSharedPtr<PVLoggerAppender>* appender = (OsclSharedPtr<PVLoggerAppender>*)(aCmd.GetParam(1).pOsclAny_value);

    if (tag == NULL || appender == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetLogAppender() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Get the logger node based on the specified tag
    PVLogger *rootnode = PVLogger::GetLoggerObject(tag);
    if (rootnode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetLogAppender() Node specified by tag is invalid"));
        return PVMFErrBadHandle;
    }

    // Add the specified appender to this node
    rootnode->AddAppender(*appender);

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetLogAppender() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoRemoveLogAppender(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveLogAppender() In"));

    char* tag = (char*)(aCmd.GetParam(0).pChar_value);
    OsclSharedPtr<PVLoggerAppender>* appender = (OsclSharedPtr<PVLoggerAppender>*)(aCmd.GetParam(1).pOsclAny_value);

    if (tag == NULL || appender == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoRemoveLogAppender() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Get the logger node based on the specified tag
    PVLogger *lognode = PVLogger::GetLoggerObject(tag);
    if (lognode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoRemoveLogAppender() Node specified by tag is invalid"));
        return PVMFErrBadHandle;
    }

    // Remove the specified appender to this node
    lognode->RemoveAppender(*appender);

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveLogAppender() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSetLogLevel(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetLogLevel() In"));

    char* tag = (char*)(aCmd.GetParam(0).pChar_value);
    int32 level = aCmd.GetParam(1).int32_value;
    bool subtree = aCmd.GetParam(2).bool_value;

    if (tag == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetLogLevel() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Get the logger node based on the specified tag
    PVLogger *lognode = PVLogger::GetLoggerObject(tag);
    if (lognode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetLogLevel() Node specified by tag is invalid"));
        return PVMFErrBadHandle;
    }

    // Set the log level
    if (subtree)
    {
        lognode->SetLogLevelAndPropagate(level);
    }
    else
    {
        lognode->SetLogLevel(level);
    }

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetLogLevel() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetLogLevel(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetLogLevel() In"));

    char* tag = (char*)(aCmd.GetParam(0).pChar_value);
    PVLogLevelInfo* levelinfo = (PVLogLevelInfo*)(aCmd.GetParam(1).pOsclAny_value);

    if (tag == NULL || levelinfo == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetLogLevel() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Get the logger node based on the specified tag
    PVLogger *lognode = PVLogger::GetLoggerObject(tag);
    if (lognode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetLogLevel() Node specified by tag is invalid"));
        return PVMFErrBadHandle;
    }

    // Get the log level info
    *levelinfo = lognode->GetLogLevel();

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetLogLevel() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoQueryUUID(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryUUID() In"));

    PvmfMimeString* mimetype;
    Oscl_Vector<PVUuid, OsclMemAllocator> *uuidvec;
    bool exactmatch;

    mimetype = (PvmfMimeString*)(aCmd.GetParam(0).pOsclAny_value);
    uuidvec = (Oscl_Vector<PVUuid, OsclMemAllocator>*)(aCmd.GetParam(1).pOsclAny_value);
    exactmatch = aCmd.GetParam(2).bool_value;

    if (mimetype == NULL || uuidvec == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryUUID() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    int32 leavecode = 0;
    PVMFStatus cmdstatus = PVMFSuccess;

    // For now just return all available extension interface UUID
    // Capability and config interface
    OSCL_TRY(leavecode,	uuidvec->push_back(PVMI_CAPABILITY_AND_CONFIG_PVUUID));
    OSCL_FIRST_CATCH_ANY(leavecode, cmdstatus = PVMFErrNoMemory);

    // License acquisition interface
    OSCL_TRY(leavecode,	uuidvec->push_back(PVPlayerLicenseAcquisitionInterfaceUuid));
    OSCL_FIRST_CATCH_ANY(leavecode, cmdstatus = PVMFErrNoMemory);

    // Track level info interface from source node
    if (iSourceNodeTrackLevelInfoIF && leavecode == 0)
    {
        leavecode = 0;
        OSCL_TRY(leavecode, uuidvec->push_back(PVMF_TRACK_LEVEL_INFO_INTERFACE_UUID));
        OSCL_FIRST_CATCH_ANY(leavecode, cmdstatus = PVMFErrNoMemory);
    }

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), cmdstatus);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryUUID() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoQueryInterface(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryInterface() In"));

    PVInterface** ifptr = (PVInterface**)(aCmd.GetParam(0).pOsclAny_value);
    PVUuid uuid = aCmd.GetUuid();
    if (ifptr == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryInterface() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    PVMFStatus cmdstatus = PVMFSuccess;
    if (queryInterface(uuid, *ifptr) == false)
    {
        cmdstatus = PVMFErrNotSupported;
    }
    else
    {
        (*ifptr)->addRef();
    }

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), cmdstatus);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryInterface() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetPVPlayerState(PVPlayerEngineCommand& aCmd, bool aSyncCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPVPlayerState() In"));

    PVPlayerState* state = (PVPlayerState*)(aCmd.GetParam(0).pOsclAny_value);
    if (state == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPVPlayerState() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Get player state using internal function
    *state = GetPVPlayerState();

    if (!aSyncCmd)
    {
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPVPlayerState() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoAddDataSource(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoAddDataSource() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSource() In"));

    if (GetPVPlayerState() != PVP_STATE_IDLE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAddDataSource() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    if (aCmd.GetParam(0).pOsclAny_value == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAddDataSource() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Save the data source
    iDataSource = (PVPlayerDataSource*)(aCmd.GetParam(0).pOsclAny_value);
    iDataSourcePS = (PVPlayerDataSourcePacketSource*)(aCmd.GetParam(0).pOsclAny_value);
    // (mg) For rollover reset to first available alternate
    iAlternateSrcFormatIndex = 0;
    iDataReadySent = false;

    // Check the source format and do a recognize if unknown
    PVMFStatus retval = PVMFSuccess;
    iSourceFormatType = iDataSource->GetDataSourceFormatType();

    if (iSourceFormatType == PVMF_FORMAT_UNKNOWN)
    {
        retval = DoQuerySourceFormatType(aCmd.GetCmdId(), aCmd.GetContext());
    }
    else
    {
        if (iSourceFormatType == PVMF_DATA_SOURCE_UNKNOWN_URL)
        {
            retval = SetupDataSourceForUnknownURLAccess();
            if (retval != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSource() - SetupDataSourceForUnknownURLAccess Failed"));
                return retval;
            }
        }

        // Start the source node creation and setup sequence
        retval = DoSetupSourceNode(aCmd.GetCmdId(), aCmd.GetContext());

        if (retval != PVMFSuccess)
        {
            // Do cleanup
            DoSourceNodeCleanup();
            iDataSource = NULL;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSource() Out"));
    return retval;

}


PVMFStatus PVPlayerEngine::DoQuerySourceFormatType(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoQuerySourceFormatType() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQuerySourceFormatType() In"));

    // Use the recognizer if the source format type is unknown
    OSCL_ASSERT(iDataSource != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, NULL, NULL, aCmdId, aCmdContext, PVP_CMD_QUERYSOURCEFORMATTYPE);
    PVMFStatus retval = PVMFSuccess;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, retval = iPlayerRecognizerRegistry.QueryFormatType(iDataSource->GetDataSourceURL(), *this, (OsclAny*) context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         return PVMFErrNotSupported;
                        );
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQuerySourceFormatType() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoSetupSourceNode(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSetupSourceNode() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupSourceNode() In"));

    OSCL_ASSERT(iDataSource != NULL);

    if (iSourceNode == NULL)
    {
        PVMFFormatType outputformattype = PVMF_FORMAT_UNKNOWN ;

        Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;
        // Query the node registry
        if (iPlayerNodeRegistry.QueryRegistry(iSourceFormatType, outputformattype, foundUuids) == PVMFSuccess)
        {
            if (foundUuids.empty())
            {
                // No matching node found
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSourceNode() No matching source node found"));
                return PVMFErrNotSupported;
            }


            int32 leavecode = 0;
            OSCL_TRY(leavecode, iSourceNode = iPlayerNodeRegistry.CreateNode(foundUuids[0]));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() Error in creating SourceNode"));
                                 return PVMFFailure;);

            iNodeUuids.push_back(PVPlayerEngineUuidNodeMapping(foundUuids[0], iSourceNode));

            if (iSourceNode == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSourceNode() Source node create failed"));
                return PVMFErrNoMemory;
            }
        }
        else
        {
            // Registry query failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSourceNode() Registry query for source node failed"));
            return PVMFErrNotSupported;
        }
    }

    if (iSourceNode->ThreadLogon() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSourceNode() ThreadLogon() on the source node failed"));
        return PVMFFailure;
    }

    PVMFNodeSessionInfo nodesessioninfo(this, this, (OsclAny*)iSourceNode, this, (OsclAny*)iSourceNode);
    int32 leavecode = 0;
    OSCL_TRY(leavecode, iSourceNodeSessionId = iSourceNode->Connect(nodesessioninfo));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSourceNode() Connect on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryInitIF);

    PVUuid sourceinituuid = PVMF_DATA_SOURCE_INIT_INTERFACE_UUID;
    leavecode = 0;
    PVMFCommandId cmdid = -1;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, sourceinituuid, (PVInterface*&)iSourceNodeInitIF, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSourceNode() QueryInterface on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupSourceNode() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSourceNodeQueryTrackSelIF(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() In"));

    int32 leavecode = 0;

    if (iDataSource->GetDataSourceType() == PVP_DATASRCTYPE_URL)
    {
        // Setup the source node via the initialization IF
        OSCL_ASSERT(iSourceFormatType != PVMF_FORMAT_UNKNOWN);

        OSCL_wHeapString<OsclMemAllocator> sourceURL;
        // In case the URL starts with file:// skip it
        OSCL_wStackString<8> fileScheme(_STRLIT_WCHAR("file"));
        OSCL_wStackString<8> schemeDelimiter(_STRLIT_WCHAR("://"));
        oscl_wchar* actualURL = NULL;

        if (oscl_strncmp(fileScheme.get_cstr(), iDataSource->GetDataSourceURL().get_cstr(), 4) == 0)
        {
            actualURL = oscl_strstr(iDataSource->GetDataSourceURL().get_cstr(), schemeDelimiter.get_cstr());
            if (actualURL == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() Unable to skip over file://"));
                return PVMFErrArgument;
            }
            //skip over ://
            actualURL += schemeDelimiter.get_size();
            sourceURL += actualURL;
        }
        else
        {
            sourceURL += iDataSource->GetDataSourceURL().get_cstr();
        }

        PVMFStatus retval = iSourceNodeInitIF->SetSourceInitializationData(sourceURL, iSourceFormatType, iDataSource->GetDataSourceContextData());
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() SetSourceInitializationData failed"));
            return retval;
        }
        // Set Playback Clock
        retval = iSourceNodeInitIF->SetClientPlayBackClock(&iPlaybackClock);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() SetClientPlayBackClock failed!"));
            return retval;
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() Data source type not supported yet so asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryTrackSelIF);

    // Query the source node for the track selection IF
    PVUuid trackseluuid = PVMF_TRACK_SELECTION_INTERFACE_UUID;
    PVMFCommandId cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, trackseluuid, (PVInterface*&)iSourceNodeTrackSelIF, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() QueryInterface on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryTrackSelIF() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSourceNodeQueryInterfaceOptional(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() In"));

    PVPlayerEngineContext* context = NULL;
    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;

    iNumPendingNodeCmd = 0;

    // Query for Track Level Info IF
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryTrackLevelInfoIF);
    PVUuid tracklevelinfouuid = PVMF_TRACK_LEVEL_INFO_INTERFACE_UUID;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, tracklevelinfouuid, (PVInterface*&)iSourceNodeTrackLevelInfoIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    // Query for Playback Control IF
    context = NULL;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryPBCtrlIF);
    PVUuid pbctrluuid = PvmfDataSourcePlaybackControlUuid;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, pbctrluuid, (PVInterface*&)iSourceNodePBCtrlIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    // Query for direction control IF
    context = NULL;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryDirCtrlIF);
    PVUuid dirctrluuid = PvmfDataSourceDirectionControlUuid;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, dirctrluuid, (PVInterface*&)iSourceNodeDirCtrlIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    // Query for Metadata IF
    context = NULL;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryMetadataIF);
    PVUuid metadatauuid = KPVMFMetadataExtensionUuid;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, metadatauuid, (PVInterface*&)iSourceNodeMetadataExtIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    // Query for Cap-Config IF
    context = NULL;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryCapConfigIF);
    PVUuid capconfiguuid = PVMI_CAPABILITY_AND_CONFIG_PVUUID;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, capconfiguuid, (PVInterface*&)iSourceNodeCapConfigIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    // Query for CPM License interface
    context = NULL;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryCPMLicenseIF);
    PVUuid licUuid = PVMFCPMPluginLicenseInterfaceUuid;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, licUuid, (PVInterface*&)iSourceNodeCPMLicenseIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    // Query for source node registry init extension IF
    context = NULL;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQuerySrcNodeRegInitIF);
    PVUuid regInitUuid = PVMF_DATA_SOURCE_NODE_REGISRTY_INIT_INTERFACE_UUID;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->QueryInterface(iSourceNodeSessionId, regInitUuid, (PVInterface*&)iSourceNodeRegInitIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() QueryInterface on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }
    if (iNumPendingNodeCmd > 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() Out"));
        return PVMFSuccess;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryInterfaceOptional() Out No pending QueryInterface() on source node"));
        return PVMFFailure;
    }
}


PVMFStatus PVPlayerEngine::DoAddDataSourceFailureComplete(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSourceFailureComplete() In"));

    // Cleanup the source node since AddDataSource failed
    DoSourceNodeCleanup();
    iDataSource = NULL;

    // Send the command completion
    if (iCommandCompleteInEngineAOErrMsg)
    {
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), iCommandCompleteInEngineAOCmdStatus, OSCL_STATIC_CAST(PVInterface*, iCommandCompleteInEngineAOErrMsg));
        iCommandCompleteInEngineAOErrMsg->removeRef();
        iCommandCompleteInEngineAOErrMsg = NULL;
    }
    else
    {
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), iCommandCompleteInEngineAOCmdStatus);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSourceFailureComplete() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoInitFailureComplete(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoInitFailureComplete() In"));

    // Cleanup the source node since AddDataSource failed
    DoSourceNodeCleanup();
    iDataSource = NULL;

    // Send the command completion
    if (iCommandCompleteInEngineAOErrMsg)
    {
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), iCommandCompleteInEngineAOCmdStatus, OSCL_STATIC_CAST(PVInterface*, iCommandCompleteInEngineAOErrMsg));
        iCommandCompleteInEngineAOErrMsg->removeRef();
        iCommandCompleteInEngineAOErrMsg = NULL;
    }
    else
    {
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), iCommandCompleteInEngineAOCmdStatus);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoInitFailureComplete() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetMetadataKey(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoGetMetadataKey() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetMetadataKey() In"));

    if (GetPVPlayerState() == PVP_STATE_ERROR)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataKey() Wrong engine state."));
        return PVMFFailure;
    }

    iGetMetadataKeysParam.iKeyList = (PVPMetadataList*)(aCmd.GetParam(0).pOsclAny_value);
    iGetMetadataKeysParam.iStartingKeyIndex = aCmd.GetParam(1).int32_value;
    iGetMetadataKeysParam.iMaxKeyEntries = aCmd.GetParam(2).int32_value;
    iGetMetadataKeysParam.iQueryKey = aCmd.GetParam(3).pChar_value;

    if (iGetMetadataKeysParam.iKeyList == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataKey() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    if (iGetMetadataKeysParam.iMaxKeyEntries < -1 || iGetMetadataKeysParam.iMaxKeyEntries == 0 || iGetMetadataKeysParam.iStartingKeyIndex < 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataKey() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Determine which node's metadata interface to start the retrieval based on the starting index
    uint32 i = 0;
    int32 totalnumkey = 0;
    uint32 nodestartindex = 0;
    while (i < iMetadataIFList.size())
    {
        int32 numkey = iMetadataIFList[i].iInterface->GetNumMetadataKeys(iGetMetadataKeysParam.iQueryKey);
        if (iGetMetadataKeysParam.iStartingKeyIndex < (totalnumkey + numkey))
        {
            // Found the node to start the key retrieval
            // Determine the start index for this node
            nodestartindex = iGetMetadataKeysParam.iStartingKeyIndex - totalnumkey;
            break;
        }
        else
        {
            // Keep checking
            totalnumkey += numkey;
            ++i;
        }
    }

    // Check if the search succeeded
    if (i == iMetadataIFList.size() || iMetadataIFList.size() == 0)
    {
        // Starting index is too large or there is no metadata interface available
        return PVMFErrArgument;
    }

    // Retrieve the metadata key from the first node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, NULL, NULL, aCmd.GetCmdId(), aCmd.GetContext(), PVP_CMD_GetNodeMetadataKey);
    PVMFMetadataExtensionInterface* metadataif = iMetadataIFList[i].iInterface;
    PVMFSessionId sessionid = iMetadataIFList[i].iSessionId;
    int32 leavecode = 0;
    PVMFCommandId cmdid = -1;
    OSCL_TRY(leavecode, cmdid = metadataif->GetNodeMetadataKeys(sessionid,
                                *(iGetMetadataKeysParam.iKeyList),
                                nodestartindex,
                                iGetMetadataKeysParam.iMaxKeyEntries,
                                iGetMetadataKeysParam.iQueryKey,
                                (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataKey() GetNodeMetadataKeys on a node did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetMetadataKey() Out"));

    // Save the current metadata value retrieval status
    iGetMetadataKeysParam.iCurrentInterfaceIndex = i;
    iGetMetadataKeysParam.iNumKeyEntriesToFill = iGetMetadataKeysParam.iMaxKeyEntries;
    iGetMetadataKeysParam.iNumKeyEntriesInList = iGetMetadataKeysParam.iKeyList->size();

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetMetadataValue(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoGetMetadataValue() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetMetadataValue() In"));

    if (GetPVPlayerState() == PVP_STATE_ERROR)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataValue() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    iGetMetadataValuesParam.iKeyList = (PVPMetadataList*)(aCmd.GetParam(0).pOsclAny_value);
    iGetMetadataValuesParam.iStartingValueIndex = aCmd.GetParam(1).int32_value;
    iGetMetadataValuesParam.iMaxValueEntries = aCmd.GetParam(2).int32_value;
    iGetMetadataValuesParam.iNumAvailableValues = (int32*)(aCmd.GetParam(3).pOsclAny_value);
    iGetMetadataValuesParam.iValueList = (Oscl_Vector<PvmiKvp, OsclMemAllocator>*)(aCmd.GetParam(4).pOsclAny_value);

    if (iGetMetadataValuesParam.iKeyList == NULL || iGetMetadataValuesParam.iValueList == NULL || iGetMetadataValuesParam.iNumAvailableValues == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataValue() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    if (iGetMetadataValuesParam.iMaxValueEntries < -1 || iGetMetadataValuesParam.iMaxValueEntries == 0 || iGetMetadataValuesParam.iStartingValueIndex < 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataValue() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Determine which node's metadata interface to start the retrieval based on the starting index
    uint32 i = 0;
    int32 totalnumvalue = 0;
    uint32 nodestartindex = 0;
    while (i < iMetadataIFList.size())
    {
        int32 numvalue = iMetadataIFList[i].iInterface->GetNumMetadataValues(*(iGetMetadataValuesParam.iKeyList));
        if (iGetMetadataValuesParam.iStartingValueIndex < (totalnumvalue + numvalue))
        {
            // Found the node to start the value retrieval
            // Determine the start index for this node
            nodestartindex = iGetMetadataValuesParam.iStartingValueIndex - totalnumvalue;
            // Save the number of available values so far
            *(iGetMetadataValuesParam.iNumAvailableValues) = totalnumvalue + numvalue;
            break;
        }
        else
        {
            // Keep checking
            totalnumvalue += numvalue;
            ++i;
        }
    }

    // Check if the search succeeded
    if (i == iMetadataIFList.size() || iMetadataIFList.size() == 0)
    {
        // Starting index is too large or there is no metadata interface available
        return PVMFErrArgument;
    }

    // Retrieve the metadata value from the first node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, NULL, NULL, aCmd.GetCmdId(), aCmd.GetContext(), PVP_CMD_GetNodeMetadataValue);
    PVMFMetadataExtensionInterface* metadataif = iMetadataIFList[i].iInterface;
    PVMFSessionId sessionid = iMetadataIFList[i].iSessionId;
    int32 leavecode = 0;
    PVMFCommandId cmdid = -1;
    OSCL_TRY(leavecode, cmdid = metadataif->GetNodeMetadataValues(sessionid,
                                *(iGetMetadataValuesParam.iKeyList),
                                *(iGetMetadataValuesParam.iValueList),
                                nodestartindex,
                                iGetMetadataValuesParam.iMaxValueEntries,
                                (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetMetadataValue() GetNodeMetadataValues on a node did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetMetadataValue() Out"));

    // Save the current metadata value retrieval status
    iGetMetadataValuesParam.iCurrentInterfaceIndex = i;
    iGetMetadataValuesParam.iNumValueEntriesToFill = iGetMetadataValuesParam.iMaxValueEntries;
    iGetMetadataValuesParam.iNumValueEntriesInList = iGetMetadataValuesParam.iValueList->size();

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoInit(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoInit() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoInit() In"));

    iRollOverState = RollOverStateIdle;

    if ((GetPVPlayerState() != PVP_STATE_IDLE) || (iSourceNode == NULL))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoInit() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    PVMFStatus retval = DoSourceNodeInit(aCmd.GetCmdId(), aCmd.GetContext());

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoInit() Out"));

    if (retval == PVMFSuccess)
    {
        SetEngineState(PVP_ENGINE_STATE_INITIALIZING);
        return PVMFSuccess;
    }
    else
    {
        return retval;
    }
}


PVMFStatus PVPlayerEngine::DoSourceNodeInit(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSourceNodeInit() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeInit() In"));

    OSCL_ASSERT(iSourceNode != NULL);

    int32 leavecode = 0;

    // Initialize the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeInit);

    leavecode = 0;
    PVMFCommandId cmdid = -1;
    OSCL_TRY(leavecode, cmdid = iSourceNode->Init(iSourceNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeInit() Init on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeInit() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSourceNodeGetDurationValue(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeGetDurationValue() In"));


    // Create a key list with just duration key
    iSourceDurationKeyList.clear();
    OSCL_HeapString<OsclMemAllocator> tmpstr = _STRLIT_CHAR("duration");
    iSourceDurationKeyList.push_back(tmpstr);
    // Clear the value list
    iSourceDurationValueList.clear();

    if (iSourceNodeMetadataExtIF == NULL)
    {
        return PVMFErrArgument;
    }

    // Call GetNodeMetadataValues on the source node to retrieve duration
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeGetDurationValue);

    int32 leavecode = 0;
    PVMFCommandId cmdid = -1;
    OSCL_TRY(leavecode, cmdid = iSourceNodeMetadataExtIF->GetNodeMetadataValues(iSourceNodeSessionId,
                                iSourceDurationKeyList,
                                iSourceDurationValueList,
                                0 /*starting index*/, 1 /*max entries*/, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeGetDurationValue() GetNodeMetadataValues on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeGetDurationValue() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoSourceNodeRollOver(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeRollOver() In"));
    /* Clean up any exisiting source node */
    DoSourceNodeCleanup();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeRollOver() DoSourceNodeCleanup Complete"));
    if (CheckForSourceRollOver())
    {
        if (iDataSource->GetAlternateSourceFormatType(iSourceFormatType,
                iAlternateSrcFormatIndex))
        {
            uint8 localbuffer[8];
            oscl_memset(localbuffer, 0, 8);
            localbuffer[0] = 1;
            oscl_memcpy(&localbuffer[4], &iSourceFormatType, sizeof(uint32));

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* infomsg =
                OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoAttemptingSourceRollOver, puuid, NULL));
            SendInformationalEvent(PVMFInfoSourceFormatNotSupported, OSCL_STATIC_CAST(PVInterface*, infomsg), NULL, localbuffer, 8);
            infomsg->removeRef();

            iAlternateSrcFormatIndex++;
            PVMFStatus status = DoSetupSourceNode(aCmdId, aCmdContext);
            if (status == PVMFSuccess)
            {
                //roll over pending
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeRollOver() SourceNodeRollOver In Progress"));
                return PVMFPending;
            }
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeRollOver() SourceNodeRollOver Failed"));
            return status;
        }
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeRollOver() Invalid State"));
    return PVMFErrInvalidState;
}

PVMFStatus PVPlayerEngine::DoSourceNodeGetDlaData(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeGetDlaData() In"));

    // Create a key list with just duration key
    iDlaDataKeyList.clear();
    OSCL_HeapString<OsclMemAllocator> tmpstr = _STRLIT_CHAR("drm/dla-data");
    iDlaDataKeyList.push_back(tmpstr);
    // Clear the value list
    iDlaDataValueList.clear();

    if (iSourceNodeMetadataExtIF == NULL)
    {
        return PVMFErrArgument;
    }

    // Call GetNodeMetadataValues on the source node to retrieve duration
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeGetDlaData);

    int32 leavecode = 0;
    PVMFCommandId cmdid = -1;
    OSCL_TRY(leavecode, cmdid = iSourceNodeMetadataExtIF->GetNodeMetadataValues(iSourceNodeSessionId,
                                iDlaDataKeyList,
                                iDlaDataValueList,
                                0 /*starting index*/, 1 /*max entries*/, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeGetDlaData() GetNodeMetadataValues on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeGetDlaData() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoAcquireLicense(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoAcquireLicense() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAcquireLicense() In"));

    // Retrieve the command parameters and validate
    iCPMAcquireLicenseParam.iContentNameChar = NULL;
    iCPMAcquireLicenseParam.iContentNameWChar = NULL;
    iCPMAcquireLicenseParam.iTimeoutMsec = (-1);
    iCPMAcquireLicenseParam.iLicenseData = NULL;
    iCPMAcquireLicenseParam.iLicenseDataSize = 0;

    if (aCmd.GetParam(0).pOsclAny_value != NULL)
    {
        iCPMAcquireLicenseParam.iLicenseData = aCmd.GetParam(0).pOsclAny_value;
    }

    if (aCmd.GetParam(1).uint32_value != 0)
    {
        iCPMAcquireLicenseParam.iLicenseDataSize = aCmd.GetParam(1).uint32_value;
    }

    if (aCmd.GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR)
    {
        iCPMAcquireLicenseParam.iContentNameWChar = aCmd.GetParam(2).pWChar_value;
    }
    else
    {
        iCPMAcquireLicenseParam.iContentNameChar = aCmd.GetParam(2).pChar_value;
    }
    iCPMAcquireLicenseParam.iTimeoutMsec = aCmd.GetParam(3).int32_value;

    if (iCPMAcquireLicenseParam.iContentNameWChar == NULL && iCPMAcquireLicenseParam.iContentNameChar == NULL)
    {
        // Content name not specified
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Content name not specified."));
        return PVMFErrArgument;
    }

    if (iCPMAcquireLicenseParam.iTimeoutMsec < -1)
    {
        // Timeout parameter not valid
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Timeout value not valid."));
        return PVMFErrArgument;
    }

    // To acquire license, player data source and local data source need to be available
    if (iDataSource == NULL)
    {
        // Player data source not available
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Player data source not specified."));
        return PVMFErrNotReady;
    }
    if (iDataSource->GetDataSourceContextData() == NULL)
    {
        // Pointer to the local data source if not available
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Local data source in player data source not specified."));
        return PVMFErrBadHandle;
    }

    //If the license interface is available from the source node, use that.
    if (iSourceNodeCPMLicenseIF != NULL)
    {
        PVMFStatus status = DoSourceNodeGetLicense(aCmd.GetCmdId(), aCmd.GetContext());
        if (status != PVMFSuccess)
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() DoSourceNodeGetLicense failed."));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAcquireLicense() Out"));
        return PVMFSuccess;
    }

    //Before getting license we need to get the dla-data blob from the source node
    //to pass to the new plugin instance.
    PVMFStatus status = DoSourceNodeGetDlaData(aCmd.GetCmdId(), aCmd.GetContext());
    if (status != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() DoSourceNodeGetDlaData failed."));
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAcquireLicense() Out"));
    return status;
}

void PVPlayerEngine::DoCancelAcquireLicense(PVPlayerEngineCommand& aCmd)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoCancelAcquireLicense() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelAcquireLicense() In"));

    /* Engine cannot be processing another cancel command */
    OSCL_ASSERT(iCmdToDlaCancel.empty() == true);

    PVMFCommandId id = aCmd.GetParam(0).int32_value;
    PVMFStatus status = PVMFSuccess;

    if (iCurrentCmd.size() == 1)
    {
        /* First save the current command being processed */
        PVPlayerEngineCommand currentcmd(iCurrentCmd[0]);

        /* First check "current" command if any */
        if (id == iCurrentCmd[0].GetCmdId())
        {
            /* Cancel the current command first */
            if (iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR
                    || iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR)
            {
                /* Make the CancelAll() command the current command */
                iCmdToDlaCancel.push_front(aCmd);
                /* Properly cancel a command being currently processed */
                if (iSourceNodeCPMLicenseIF != NULL)
                {
                    /* Cancel the GetLicense */
                    PVPlayerEngineContext* context = NULL;
                    PVMFCommandId cmdid = -1;
                    int32 leavecode = 0;
                    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmd.GetCmdId(), aCmd.GetContext(), PVP_CMD_SourceNodeCancelGetLicense);

                    OSCL_TRY(leavecode, cmdid = iSourceNodeCPMLicenseIF->CancelGetLicense(iSourceNodeSessionId, iCPMGetLicenseCmdId, (OsclAny*)context));
                    if (leavecode)
                    {
                        FreeEngineContext(context);
                        status = PVMFErrNotSupported;
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelAcquireLicense() CancelGetLicense on iSourceNode did a leave!"));
                    }
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelAcquireLicense() CPM plug-in registry in local data source not specified."));
                    OSCL_ASSERT(false);
                    status = PVMFErrBadHandle;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelAcquireLicense() Current cmd is not AquireLicense."));
                status = PVMFErrArgument;
            }
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelAcquireLicense() Current cmd ID is not equal with App specified cmd ID."));
            status = PVMFErrArgument;
        }
        if (status != PVMFSuccess)
        {
            /* We send error completetion for CancelAcquireLicense API*/
            iCurrentCmd.erase(iCurrentCmd.begin());
            iCurrentCmd.push_front(aCmd);
            EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), status);
            iCurrentCmd.push_front(currentcmd);
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelAcquireLicense() No Current cmd"));
        iCurrentCmd.push_front(aCmd);
        status = PVMFErrArgument;
        /* If we get here the command isn't queued so the cancel fails */
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), status);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelAcquireLicense() Out"));
    return;
}

PVMFStatus PVPlayerEngine::DoGetPluginLicenseIF()
{
    //create & use a plug-in instance.
    // Retrieve the CPM plug-in instance from the registry
    if (iDataSource->GetDataSourceContextData() != NULL)
    {
        PVInterface* pvInterface =
            OSCL_STATIC_CAST(PVInterface*, iDataSource->GetDataSourceContextData());
        PVInterface* localDataSrc = NULL;
        PVUuid localDataSrcUuid(PVMF_LOCAL_DATASOURCE_UUID);
        if (pvInterface->queryInterface(localDataSrcUuid, localDataSrc))
        {
            PVMFLocalDataSource* opaqueData =
                OSCL_STATIC_CAST(PVMFLocalDataSource*, localDataSrc);
            if (opaqueData->iUseCPMPluginRegistry == false)
            {
                // CPM plug-in registry not available in the local data source
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() CPM plug-in registry in local data source not specified."));
                return PVMFErrBadHandle;
            }
        }
        else
        {
            PVInterface* sourceDataContext = NULL;
            PVInterface* commonDataContext = NULL;
            PVUuid sourceContextUuid(PVMF_SOURCE_CONTEXT_DATA_UUID);
            PVUuid commonContextUuid(PVMF_SOURCE_CONTEXT_DATA_COMMON_UUID);
            if (pvInterface->queryInterface(sourceContextUuid, sourceDataContext))
            {
                if (sourceDataContext->queryInterface(commonContextUuid, commonDataContext))
                {
                    PVMFSourceContextDataCommon* cContext =
                        OSCL_STATIC_CAST(PVMFSourceContextDataCommon*, commonDataContext);
                    if (cContext->iUseCPMPluginRegistry == false)
                    {
                        // CPM plug-in registry not available in the local data source
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() CPM plug-in registry in local data source not specified."));
                        return PVMFErrBadHandle;
                    }
                }
            }
        }
    }
    //Lookup the factory for the janus plugin
    {
        //Connect to registry
        OsclRegistryAccessClient cli;
        if (cli.Connect() != OsclErrNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Can't connect to plugin registry."));
            return PVMFFailure;
        }

        OSCL_StackString<32> janusmimestr("X-CPM-PLUGIN/PV-WMDRM-JANUS");
        iCPMPluginFactory = cli.GetFactory(janusmimestr);

        if (iCPMPluginFactory)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoAcquireLicense() Using Janus Plug-in."));
        }
        else
        {
            OSCL_StackString<34> jupitermimestr("X-CPM-PLUGIN/PV-WMDRM-JUPITER");
            iCPMPluginFactory = cli.GetFactory(jupitermimestr);
            if (iCPMPluginFactory)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoAcquireLicense() Using Jupiter Plug-in."));
            }
        }

        //Close registry session.
        cli.Close();
    }

    if (iCPMPluginFactory == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Can't find Janus plugin factory."));
        return PVMFFailure;
    }

    //Create an instance of the plugin.
    iCPMPlugin = ((PVMFCPMPluginFactory*)iCPMPluginFactory)->CreateCPMPlugin();
    if (!iCPMPlugin)
    {
        // Specified CPM plug-in is not available in the registry
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Can't create janus plugin."));
        return PVMFFailure;
    }

    int32 leavecode = 0;

    // Create a session to the CPM plug-in
    OSCL_TRY(leavecode, iCPMPluginSessionId = iCPMPlugin->Connect(*this));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() Connect to the plug-in did a leave!"));
                         ((PVMFCPMPluginFactory*)iCPMPluginFactory)->DestroyCPMPlugin(iCPMPlugin);
                         iCPMPlugin = NULL;
                         iCPMPluginFactory = NULL;
                         return PVMFErrResource;);

    // Query for the license extension IF
    leavecode = 0;
    OSCL_TRY(leavecode, iCPMPlugin->QueryInterface(iCPMPluginSessionId, PVMFCPMPluginLicenseInterfaceUuid, (PVInterface*&)iCPMPluginLicenseIF, NULL));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAcquireLicense() QueryInterface to the plug-in did a leave!"));
                         iCPMPlugin->Disconnect(iCPMPluginSessionId);
                         ((PVMFCPMPluginFactory*)iCPMPluginFactory)->DestroyCPMPlugin(iCPMPlugin);
                         iCPMPlugin = NULL;
                         iCPMPluginFactory = NULL;
                         return PVMFErrResource;);

    iCPMPluginCommand = PVP_CMD_CPMQueryLicenseIF;

    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoSourceNodeGetLicense(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSourceNodeGetLicense() Tick=%d", OsclTickCount::TickCount()));

    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(aCmdContext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeGetLicense() In"));

    if (iSourceNodeCPMLicenseIF == NULL)
    {
        OSCL_ASSERT(false);
        return PVMFErrBadHandle;
    }

    // Get the license
    PVPlayerEngineContext* context = NULL;
    int32 leavecode = 0;
    context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeGetLicense);
    if (iCPMAcquireLicenseParam.iContentNameChar)
    {
        // Use the char version
        iCPMContentNameStr = iCPMAcquireLicenseParam.iContentNameChar;
        OSCL_TRY(leavecode, iCPMGetLicenseCmdId = iSourceNodeCPMLicenseIF->GetLicense(iSourceNodeSessionId,
                 iCPMContentNameStr,
                 iCPMAcquireLicenseParam.iLicenseData,
                 iCPMAcquireLicenseParam.iLicenseDataSize,
                 iCPMAcquireLicenseParam.iTimeoutMsec,
                 (OsclAny*)context));
    }
    else if (iCPMAcquireLicenseParam.iContentNameWChar)
    {
        // Use the wchar version
        iCPMContentNameWStr = iCPMAcquireLicenseParam.iContentNameWChar;
        OSCL_TRY(leavecode, iCPMGetLicenseCmdId = iSourceNodeCPMLicenseIF->GetLicense(iSourceNodeSessionId,
                 iCPMContentNameWStr,
                 iCPMAcquireLicenseParam.iLicenseData,
                 iCPMAcquireLicenseParam.iLicenseDataSize,
                 iCPMAcquireLicenseParam.iTimeoutMsec,
                 (OsclAny*)context));
    }
    else
    {
        // This should not happen
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeGetLicense() Content name not specified. Asserting"));
        OSCL_ASSERT(false);
        return PVMFErrArgument;
    }

    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeGetLicense() GetLicense on iSourceNode did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    if (iNumPendingNodeCmd <= 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,	(0, "PVPlayerEngine::DoSourceNodeGetLicense() Out No pending QueryInterface() on source node"));
        return PVMFFailure;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,	(0, "PVPlayerEngine::DoSourceNodeGetLicense() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoCPMPluginSetUserAgentHeader(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(aCmdContext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCPMPluginSetUserAgentHeader() In"));

    if (iCPMPluginCapConfigIf == NULL)
    {
        OSCL_ASSERT(false);
        return PVMFErrBadHandle;
    }

    //set the parameter
    OSCL_StackString<64> userAgentheaderKey(_STRLIT_CHAR("x-pvmf/net/user-agent"));

    PvmiKvp *retKVP;

    /*Search for user-agent key in the vector*/
    for (uint32 i = 0;i < iPvmiKvpCapNConfig.size();i++)
    {
        if (oscl_strstr(iPvmiKvpCapNConfig[i]->key, userAgentheaderKey.get_cstr()))
        {
            iCPMPluginCapConfigIf->setParametersSync(NULL, iPvmiKvpCapNConfig[i], 1, retKVP);

            //log if error
            if (retKVP)
            {
                //log error
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCPMPluginSetUserAgentHeader() user-agent parameter could not be set in CPM"));
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCPMPluginSetUserAgentHeader() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoCPMPluginGetLicense(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoCPMPluginGetLicense() Tick=%d", OsclTickCount::TickCount()));

    OSCL_UNUSED_ARG(aCmdId);
    OSCL_UNUSED_ARG(aCmdContext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCPMPluginGetLicense() In"));

    if (iCPMPluginLicenseIF == NULL)
    {
        OSCL_ASSERT(false);
        return PVMFErrBadHandle;
    }

    // Get the license
    int32 leavecode = 0;
    if (iCPMAcquireLicenseParam.iContentNameChar)
    {
        // Use the char version
        iCPMContentNameStr = iCPMAcquireLicenseParam.iContentNameChar;
        OSCL_TRY(leavecode, iCPMPluginLicenseIF->GetLicense(iCPMPluginSessionId,
                 iCPMContentNameStr,
                 iCPMAcquireLicenseParam.iLicenseData,
                 iCPMAcquireLicenseParam.iLicenseDataSize,
                 iCPMAcquireLicenseParam.iTimeoutMsec));
    }
    else if (iCPMAcquireLicenseParam.iContentNameWChar)
    {
        // Use the wchar version
        iCPMContentNameWStr = iCPMAcquireLicenseParam.iContentNameWChar;
        OSCL_TRY(leavecode, iCPMPluginLicenseIF->GetLicense(iCPMPluginSessionId,
                 iCPMContentNameWStr,
                 iCPMAcquireLicenseParam.iLicenseData,
                 iCPMAcquireLicenseParam.iLicenseDataSize,
                 iCPMAcquireLicenseParam.iTimeoutMsec));
    }
    else
    {
        // This should not happen
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCPMPluginGetLicense() Content name not specified. Asserting"));
        OSCL_ASSERT(false);
        return PVMFErrArgument;
    }

    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCPMPluginGetLicense() GetLicense to the plug-in's license interface did a leave!"));
                         return PVMFErrResource;);
    iCPMPluginCommand = PVP_CMD_CPMGetLicense;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCPMPluginGetLicense() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoCPMPluginAcquireLicenseComplete()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoCPMPluginAcquireLicenseComplete() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCPMPluginAcquireLicenseComplete() In"));

    // Close the CPM plug-in session and reset variables
    if (iCPMPluginLicenseIF)
    {
        iCPMPluginLicenseIF->removeRef();
        iCPMPluginLicenseIF = NULL;
    }

    iCPMPluginCapConfigIf = NULL;

    // Destroy the plugin instance that we created.
    if (iCPMPlugin)
    {
        // Trap any leave but no need to handle it other than report it
        int32 leavecode = 0;
        OSCL_TRY(leavecode, iCPMPlugin->Disconnect(iCPMPluginSessionId));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCPMPluginAcquireLicenseComplete() Disconnect on CPM plugin did a leave. Asserting."));
                             OSCL_ASSERT(false);
                            );

        ((PVMFCPMPluginFactory*)iCPMPluginFactory)->DestroyCPMPlugin(iCPMPlugin);
    }

    iCPMPlugin = NULL;
    iCPMPluginFactory = NULL;
    iCPMPluginSessionId = 0;
    iCPMPluginCommand = -1;

    // Send the command completion
    if (iCommandCompleteInEngineAOErrMsg)
    {
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), iCommandCompleteInEngineAOCmdStatus, OSCL_STATIC_CAST(PVInterface*, iCommandCompleteInEngineAOErrMsg));
        iCommandCompleteInEngineAOErrMsg->removeRef();
        iCommandCompleteInEngineAOErrMsg = NULL;
    }
    else
    {
        EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), iCommandCompleteInEngineAOCmdStatus);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCPMPluginAcquireLicenseComplete() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoAddDataSink(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoAddDataSink() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSink() In"));

    if (aCmd.GetParam(0).pOsclAny_value == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAddDataSink() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    PVPlayerDataSink* datasink = (PVPlayerDataSink*)(aCmd.GetParam(0).pOsclAny_value);

    PVPlayerEngineDatapath newdatapath;
    newdatapath.iDataSink = datasink;

    // Determine the data sink's media type
    switch (GetMediaTypeIndex(datasink->GetDataSinkFormatType()))
    {
        case PVMF_UNCOMPRESSED_VIDEO_FORMAT:
        case PVMF_COMPRESSED_VIDEO_FORMAT:
            newdatapath.iMediaType = PVP_MEDIATYPE_VIDEO;
            break;

        case PVMF_UNCOMPRESSED_AUDIO_FORMAT:
        case PVMF_COMPRESSED_AUDIO_FORMAT:
            newdatapath.iMediaType = PVP_MEDIATYPE_AUDIO;
            break;

        case PVMF_TEXT_FORMAT:
            newdatapath.iMediaType = PVP_MEDIATYPE_TEXT;
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAddDataSink() Unknown format. Cannot determine sink's media type"));
            return PVMFErrNotSupported;
    }

    // Add a new engine datapath to the list for the data sink
    int32 leavecode = 0;
    OSCL_TRY(leavecode, iDatapathList.push_front(newdatapath));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoAddDataSink() New datapath could not be added to iDatapathList"));
                         return PVMFErrNoMemory);

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoAddDataSink() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSetPlaybackRange(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoSetPlaybackRange() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetPlaybackRange() In"));

    PVMFStatus retval;

    if (iWatchDogTimer != NULL)
    {
        if (iWatchDogTimer->IsBusy())
        {
            iWatchDogTimer->Cancel();
        }
    }
    iNumPendingSkipCompleteEvent = 0;
    iNumPVMFInfoStartOfDataPending = 0;
    iStreamID++;

    if (GetPVPlayerState() == PVP_STATE_ERROR)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRange() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    if (aCmd.GetParam(2).bool_value)
    {
#if 1
        // Queueing of playback range is not supported yet
        iQueuedRangePresent = false;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRange() Queued playback range is not supported yet"));
        return PVMFErrNotSupported;
#else
        iQueuedRangePresent = true;
        iQueuedStartPosition = aCmd.GetParam(0).playbackpos_value;
        iQueuedStopPosition = aCmd.GetParam(1).playbackpos_value;
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
        return PVMFSuccess;
#endif
    }

    // Change the end position
    iCurrentEndPosition = aCmd.GetParam(1).playbackpos_value;
    retval = UpdateCurrentEndPosition(iCurrentEndPosition);
    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRange() Changing end position failed"));
        return retval;
    }

    if (aCmd.GetParam(0).playbackpos_value.iIndeterminate)
    {
        // Start position not specified so return as success
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
        return PVMFSuccess;
    }

    // Reset the paused-due-to-EOS flag
    iPlaybackPausedDueToEndOfClip = false;

    // Change the begin position
    iCurrentBeginPosition = aCmd.GetParam(0).playbackpos_value;
    retval = UpdateCurrentBeginPosition(iCurrentBeginPosition, aCmd);

    if (retval == PVMFSuccess)
    {
        // Notify completion of engine command
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    }
    else if (retval == PVMFPending)
    {
        // SetPlaybackRange command is still being processed
        // so change the return status so command is not completed yet
        retval = PVMFSuccess;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetPlaybackRange() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::UpdateCurrentEndPosition(PVPPlaybackPosition& aEndPos)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentEndPosition() In"));

    if (aEndPos.iIndeterminate)
    {
        // Disable end time checking if running
        if (iEndTimeCheckEnabled)
        {
            iEndTimeCheckEnabled = false;
            iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
        }
    }
    else
    {
        // Convert the end time to milliseconds to have consistent units internally
        uint32 timems = 0;
        PVMFStatus retval = ConvertToMillisec(aEndPos, timems);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::UpdateCurrentEndPosition() Converting to millisec failed"));
            return retval;
        }
        aEndPos.iPosValue.millisec_value = timems;
        aEndPos.iPosUnit = PVPPBPOSUNIT_MILLISEC;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentEndPosition() Changing end time to %d ms", timems));

        // Enable the end time checking if not running
        if (!iEndTimeCheckEnabled)
        {
            iEndTimeCheckEnabled = true;

            if (GetPVPlayerState() == PVP_STATE_STARTED)
            {
                // Determine the check cycle based on interval setting in milliseconds
                // and timer frequency of 100 millisec
                int32 checkcycle = iEndTimeCheckInterval / 100;
                if (checkcycle == 0)
                {
                    ++checkcycle;
                }
                iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
                iPollingCheckTimer->Request(PVPLAYERENGINE_TIMERID_ENDTIMECHECK, 0, checkcycle, this, true);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentEndPosition() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::UpdateCurrentBeginPosition(PVPPlaybackPosition& aBeginPos, PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() In"));

    PVMFStatus retval = PVMFSuccess;
    uint32 timems = 0;

    switch (GetPVPlayerState())
    {
        case PVP_STATE_PREPARED:
        case PVP_STATE_STARTED:
        {
            // Change the playback position immediately
            retval = ConvertToMillisec(aBeginPos, timems);
            if (retval != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() Converting to millisec failed"));
                return retval;
            }
            if (aBeginPos.iPosUnit == PVPPBPOSUNIT_PLAYLIST)
            {
                aBeginPos.iPlayListPosValue.millisec_value = timems;
                aBeginPos.iPlayListPosUnit = PVPPBPOSUNIT_MILLISEC;
            }
            else
            {
                aBeginPos.iPosValue.millisec_value = timems;
                aBeginPos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() Requested begin position is %d ms", timems));

            retval = DoChangePlaybackPosition(aCmd.GetCmdId(), aCmd.GetContext());
        }
        break;

        case PVP_STATE_PAUSED:
        {
            if (iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_RESUME)
            {
                // Reposition occurred during the paused state so need to change the source position first
                retval = DoSourceNodeQueryDataSourcePosition(aCmd.GetCmdId(), aCmd.GetContext());
                if (retval == PVMFSuccess)
                {
                    //return Pending to indicate a node command was issued
                    return PVMFPending;
                }
                else
                {
                    //ignore failure, continue with resume sequence
                    return PVMFSuccess;
                }
            }
            else
            {
                //if there's already a direction change pending, then don't
                //allow a reposition also
                if (iChangePlaybackDirectionWhenResuming)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() Direction change already pending, fail."));
                    return PVMFErrInvalidState;
                }

                // Convert the time units but flag to change playback position when resuming
                retval = ConvertToMillisec(aBeginPos, timems);
                if (retval != PVMFSuccess)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() Converting to millisec failed in paused state"));
                    return retval;
                }
                aBeginPos.iPosValue.millisec_value = timems;
                aBeginPos.iPosUnit = PVPPBPOSUNIT_MILLISEC;

                iChangePlaybackPositionWhenResuming = true;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() Saving requested begin position(%d ms) for resume", timems));
            }
        }
        break;

        default:
            // Playback is stopped and start position is set so wait for playback to start
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentBeginPosition() Out"));
    return retval;
}

PVMFStatus PVPlayerEngine::DoChangePlaybackPosition(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoChangePlaybackPosition() In"));

    // Check if the source node has position control IF or
    // begin position is indeterminate
    if (iSourceNodePBCtrlIF == NULL ||
            iCurrentBeginPosition.iIndeterminate ||
            ((iCurrentBeginPosition.iPosUnit != PVPPBPOSUNIT_MILLISEC) &&
             (iCurrentBeginPosition.iPlayListPosUnit != PVPPBPOSUNIT_MILLISEC)))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoChangePlaybackPosition() Playback control IF on source node not available or invalid begin position"));
        return PVMFFailure;
    }

    PVMFCommandId cmdid = -1;

    if (iSeekToSyncPoint && iSyncPointSeekWindow > 0)
    {
        PVPPlaybackPosition curpos;
        curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
        GetPlaybackClockPosition(curpos);
        // Need to query the data source on where playback would actually begin from
        PVMFTimestamp startpos;
        startpos = iCurrentBeginPosition.iPosValue.millisec_value;

        PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryDataSourcePositionDuringPlayback);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoChangePlaybackPosition() Querying source position. Position %d ms, SeekToSyncPt %d", startpos, iSeekToSyncPoint));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoChangePlaybackPosition() Querying source position. Position %d ms, SeekToSyncPt %d", startpos, iSeekToSyncPoint));
        iActualPlaybackPosition = curpos.iPosValue.millisec_value;
        int32 leavecode = 0;
        iSeekPointBeforeTargetNPT = 0;
        iSeekPointAfterTargetNPT = 0;

        // As in case of MP4 file we need to call overload function of QueryDataSourcePosition which retruns
        // I frame before and after instead of actaul NPT, format type will be checked here to first find if
        // format-type is one of the MP4 varient

        PVMFNodeCapability nodeCapability;
        iSourceNode->GetCapability(nodeCapability);
        PVMFFormatType * formatType = nodeCapability.iInputFormatCapability.begin();
        bool mpeg4FormatType = false;
        if (formatType  != NULL)
        {
            switch (*formatType)
            {
                case PVMF_MPEG4FF:
                    mpeg4FormatType = true;
                    break;

                default:
                    mpeg4FormatType = false;
                    break;
            }
        }

        if (mpeg4FormatType)
        {
            OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->QueryDataSourcePosition(iSourceNodeSessionId, startpos,
                                        iSeekPointBeforeTargetNPT, iSeekPointAfterTargetNPT, (OsclAny*)context, iSeekToSyncPoint));
        }
        else
        {
            OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->QueryDataSourcePosition(iSourceNodeSessionId, startpos, iActualPlaybackPosition,
                                        iSeekToSyncPoint, (OsclAny*)context));
        }

        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoChangePlaybackPosition() QueryDataSourcePosition on iSourceNodePBCtrlIF did a leave!"));
                             if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
    {
        return leavecode;
    }
    else
    {
        return PVMFFailure;
    }
                        );
    }
    else
    {
        // Go straight to repositioning the data source
        PVMFStatus retval = DoSourceNodeSetDataSourcePositionDuringPlayback(iCurrentBeginPosition.iPosValue.millisec_value, iSeekToSyncPoint, aCmdId, aCmdContext);
        if (retval == PVMFSuccess)
        {
            return PVMFPending;
        }
        else
        {
            return retval;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoChangePlaybackPosition() Out"));

    return PVMFPending;
}

PVMFStatus PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback(PVMFTimestamp aTargetNPT, bool aSeekToSyncPoint, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() In"));

    // Check if the source node has position control IF
    if (iSourceNodePBCtrlIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() No source playback control IF"));
        return PVMFFailure;
    }
    bool clockpausedhere;
    switch (iPlaybackPositionMode)
    {
        case PVPPBPOS_MODE_END_OF_CURRENT_PLAY_ELEMENT:
        case PVPPBPOS_MODE_END_OF_CURRENT_PLAY_SESSION:
            break;
        case PVPPBPOS_MODE_NOW:
        default:
            // Pause the playback clock
            clockpausedhere = iPlaybackClock.Pause();

            // Stop the playback position status timer
            StopPlaybackStatusTimer();
            break;
    }
    // Set the new position on the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeSetDataSourcePositionDuringPlayback);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() Calling SetDataSourcePosition() on source node. TargetNPT %d ms, SeekToSyncPoint %d", aTargetNPT, aSeekToSyncPoint));

    int32 leavecode = 0;
    if (iCurrentBeginPosition.iPosUnit == PVPPBPOSUNIT_PLAYLIST)
    {
        int32 tempStreamID = 0;
        tempStreamID = iStreamID;


        iDataSourcePosParams.iActualMediaDataTS = 0;
        iDataSourcePosParams.iActualNPT = 0;
        if ((iCurrentBeginPosition.iMode == PVPPBPOS_MODE_UNKNOWN) ||
                (iCurrentBeginPosition.iMode == PVPPBPOS_MODE_NOW))
        {
            iDataSourcePosParams.iMode = PVMF_SET_DATA_SOURCE_POSITION_MODE_NOW;
        }
        else if (iCurrentBeginPosition.iMode == PVPPBPOS_MODE_END_OF_CURRENT_PLAY_ELEMENT)
        {
            iDataSourcePosParams.iMode = PVMF_SET_DATA_SOURCE_POSITION_END_OF_CURRENT_PLAY_ELEMENT;
        }
        else if (iCurrentBeginPosition.iMode == PVPPBPOS_MODE_END_OF_CURRENT_PLAY_SESSION)
        {
            iDataSourcePosParams.iMode = PVMF_SET_DATA_SOURCE_POSITION_MODE_END_OF_CURRENT_PLAY_SESSION;
        }
        iDataSourcePosParams.iPlayElementIndex = iCurrentBeginPosition.iPlayElementIndex;
        iDataSourcePosParams.iSeekToSyncPoint = aSeekToSyncPoint;
        iDataSourcePosParams.iTargetNPT = iCurrentBeginPosition.iPlayListPosValue.millisec_value;
        iDataSourcePosParams.iStreamID = iStreamID;
        iDataSourcePosParams.iPlaylistUri = iCurrentBeginPosition.iPlayListUri;

        PVMFCommandId cmdid = -1;
        leavecode = 0;
        OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->SetDataSourcePosition(iSourceNodeSessionId,
                                    iDataSourcePosParams,
                                    (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() SetDataSourcePosition on iSourceNodePBCtrlIF did a leave!"));
                             if (clockpausedhere)
    {
        // Resume the clock if paused in this function
        iPlaybackClock.Start();

            // To get regular play status events
            StartPlaybackStatusTimer();
        }

        if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
    {
        return leavecode;
    }
    else
    {
        return PVMFFailure;
    }
                        );
    }
    else
    {
        // we assume that the data is not present in the graph
        // if present we will reset this flag.
        int32 tempStreamID = 0;
        // if reposition points exists in between the actualPlaybackPosition and
        // the currentPlaybackClock position, then the reposition data exists with
        // in the graph, so we need to start a new media segment
        // Check if forward repositioning does not need source repositioning
        PVPPlaybackPosition curpos;
        curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
        GetPlaybackClockPosition(curpos);

        tempStreamID = iStreamID;


        iActualMediaDataTS = 0;
        iAdjustedMediaDataTS = 0;
        iActualPlaybackPosition = 0;
        leavecode = 0;
        PVMFCommandId cmdid = -1;

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() SetDataSourcePosition on iSourceNodePBCtrlIF - TargetNPT=%d, aSeekToSyncPoint=%d", aTargetNPT, aSeekToSyncPoint));
        OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->SetDataSourcePosition(iSourceNodeSessionId, aTargetNPT, iActualPlaybackPosition, iActualMediaDataTS, aSeekToSyncPoint, tempStreamID, (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() SetDataSourcePosition on iSourceNodePBCtrlIF did a leave!"));
                             if (clockpausedhere)
    {
        // Resume the clock if paused in this function
        iPlaybackClock.Start();

            // To get regular play status events
            StartPlaybackStatusTimer();
        }

        if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
    {
        return leavecode;
    }
    else
    {
        return PVMFFailure;
    }
                        );
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePositionDuringPlayback() Out"));

    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback(PVCommandId aCmdId,
        OsclAny* aCmdContext,
        bool aSFR)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback() In"));

    // Pause the playback clock
    bool clockpausedhere = iPlaybackClock.Pause();

    // Stop the playback position status timer
    StopPlaybackStatusTimer();

    // Tell the sink nodes to skip the unneeded media data
    iNumPendingNodeCmd = 0;
    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;

    // For all sink node with sync control IF, call SkipMediaData()
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback() Calling SkipMediaData() on sink nodes. MediadataTS to flush to %d ms, MediadataTS to skip to %d ms, RenderSkipped %d", iActualMediaDataTS, iAdjustedMediaDataTS, iRenderSkipped));

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                        (0, "PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback() Calling SkipMediaData() on sink nodes. MediadataTS to flush to %d ms, MediadataTS to skip to %d ms, RenderSkipped %d", iActualMediaDataTS, iAdjustedMediaDataTS, iRenderSkipped));

        if (iDatapathList[i].iTrackActive == true &&
                iDatapathList[i].iEndOfDataReceived == false &&
                iDatapathList[i].iSinkNodeSyncCtrlIF != NULL)
        {
            leavecode = 0;
            PVPlayerEngineContext* context = AllocateEngineContext(&(iDatapathList[i]), iDatapathList[i].iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeSkipMediaDataDuringPlayback);
            OSCL_TRY(leavecode, cmdid = iDatapathList[i].iSinkNodeSyncCtrlIF->SkipMediaData(iDatapathList[i].iSinkNodeSessionId, iActualMediaDataTS, iAdjustedMediaDataTS, iStreamID, iRenderSkipped, aSFR, (OsclAny*) context));
            if (leavecode == 0)
            {
                ++iNumPendingNodeCmd;
                ++iNumPendingSkipCompleteEvent;
                ++iNumPVMFInfoStartOfDataPending;

            }
            else
            {
                FreeEngineContext(context);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback() Out"));
    if (iNumPendingNodeCmd > 0)
    {
        return PVMFSuccess;
    }
    else
    {
        if (clockpausedhere)
        {
            // Resume the clock if paused in this function
            iPlaybackClock.Start();

            // To get regular play status events
            StartPlaybackStatusTimer();
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeSkipMediaDataDuringPlayback() Skip on sink nodes failed"));
        return PVMFFailure;
    }
}


PVMFStatus PVPlayerEngine::DoGetPlaybackRange(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlaybackRange() In"));

    if (aCmd.GetParam(0).pPlaybackpos_value == NULL ||
            aCmd.GetParam(1).pPlaybackpos_value == NULL)
    {
        // User did not pass in the reference to write the start and stop positions
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlaybackRange() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    if (aCmd.GetParam(2).bool_value)
    {
        // Return the queued playback range
        if (iQueuedRangePresent)
        {
            *(aCmd.GetParam(0).pPlaybackpos_value) = iQueuedBeginPosition;
            *(aCmd.GetParam(1).pPlaybackpos_value) = iQueuedEndPosition;
        }
        else
        {
            // Queued range has not been set
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlaybackRange() Queued range not set"));
            return PVMFErrNotReady;
        }
    }
    else
    {
        PVMFStatus retval = PVMFSuccess;

        // Return the current playback range
        if (iCurrentBeginPosition.iIndeterminate)
        {
            // Since indeterminate, just directly copy
            *(aCmd.GetParam(0).pPlaybackpos_value) = iCurrentBeginPosition;
        }
        else
        {
            retval = ConvertFromMillisec(iCurrentBeginPosition.iPosValue.millisec_value, *(aCmd.GetParam(0).pPlaybackpos_value));
        }

        if (iCurrentEndPosition.iIndeterminate)
        {
            // Since indeterminate, just directly copy
            *(aCmd.GetParam(1).pPlaybackpos_value) = iCurrentEndPosition;
        }
        else
        {
            retval = ConvertFromMillisec(iCurrentEndPosition.iPosValue.millisec_value, *(aCmd.GetParam(1).pPlaybackpos_value));
        }

        if (retval != PVMFSuccess)
        {
            // The conversion failed.
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlaybackRange() Conversion from millisec failed"));
            return retval;
        }
    }

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlaybackRange() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetCurrentPosition(PVPlayerEngineCommand& aCmd, bool aSyncCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetCurrentPosition() In"));

    PVPPlaybackPosition* pbpos = aCmd.GetParam(0).pPlaybackpos_value;

    if (GetPVPlayerState() == PVP_STATE_IDLE ||
            GetPVPlayerState() == PVP_STATE_ERROR)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetCurrentPosition() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    if (pbpos == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetCurrentPosition() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    // Query playback clock for current playback position
    GetPlaybackClockPosition(*pbpos);

    if (pbpos->iIndeterminate)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetCurrentPosition() Passed in parameter invalid."));
        return PVMFErrArgument;
    }

    if (!aSyncCmd)
    {
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetCurrentPosition() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSetPlaybackRate(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetPlaybackRate() In"));

    int32 rate = aCmd.GetParam(0).int32_value;
    OsclTimebase* timebase = (OsclTimebase*)(aCmd.GetParam(1).pOsclAny_value);

    // Split the rate into the absolute value plus the direction 1 or -1.
    int32 direction = 1;
    if (rate < 0)
    {
        direction = (-1);
        rate = (-rate);
    }

    // Check if called in valid states.
    if (GetPVPlayerState() != PVP_STATE_PREPARED
            && GetPVPlayerState() != PVP_STATE_STARTED
            && GetPVPlayerState() != PVP_STATE_PAUSED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Wrong engine state to change rate"));
        return PVMFErrInvalidState;
    }

    // Timebase can only be changed when prepared or paused.
    if (timebase != iOutsideTimebase
            && GetPVPlayerState() != PVP_STATE_PREPARED
            && GetPVPlayerState() != PVP_STATE_PAUSED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Wrong engine state to change timebase"));
        return PVMFErrInvalidState;
    }

    // Don't allow a direction change while paused, if there's already
    // a pending reposition.  Engine doesn't have logic to handle both repos and
    // direction change during the Resume.
    if (direction != iPlaybackDirection
            && GetPVPlayerState() == PVP_STATE_PAUSED
            && iChangePlaybackPositionWhenResuming)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Repos already pending-- can't change direction."));
        return PVMFErrInvalidState;
    }

    // Switching from forward to backward really only makes sense when playing or paused,
    // otherwise we'll be at the end of clip.  If we ever allow combined repositioning
    // and direction change, this restriction could be removed.
    if (direction != iPlaybackDirection
            && direction < 0
            && GetPVPlayerState() != PVP_STATE_STARTED
            && GetPVPlayerState() != PVP_STATE_PAUSED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Wrong engine state to go backward"));
        return PVMFErrInvalidState;
    }

    // Validate the playback rate parameters.

    // Rate zero is only valid with an outside timebase.
    if (rate == 0
            && timebase == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Invalid parameter-- rate 0 with no outside timbase."));
        return PVMFErrArgument;
    }

    // Rate must be within allowed range
    if (rate > 0
            && (rate < PVP_PBRATE_MIN || rate > PVP_PBRATE_MAX))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Invalid parameter-- rate outside allowed range"));
        return PVMFErrArgument;
    }

    // With an outside timebase, we can't really support rates.  If -1x is input,
    // it means backward direction, but otherwise, rate is ignored.
    // So flag an error for any rate other than zero, 1x, or -1x.
    if (timebase != NULL
            && (rate != 0 && rate != 100000))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() Invalid rate with outside timebase"));
        return PVMFErrInvalidState;
    }

    // To do any rate change, the source node must have the playback control IF.
    if (rate != iPlaybackClockRate
            && iSourceNodePBCtrlIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() iSourceNodePBCtrlIF is NULL"));
        return PVMFFailure;
    }

    // To do any direction change, the source node must have the direction control IF.
    if (direction != iPlaybackDirection
            && iSourceNodeDirCtrlIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() iSourceNodeDirCtrlIF is NULL"));
        return PVMFFailure;
    }

    // Reset the paused-due-to-EOS flag if direction changes
    if (direction != iPlaybackDirection)
    {
        iPlaybackPausedDueToEndOfClip = false;
    }

    // Save the new values.  They won't be installed until they're verified
    iOutsideTimebase_New = timebase;
    iPlaybackDirection_New = direction;
    iPlaybackClockRate_New = rate;

    // Start the sequence.

    if (iPlaybackClockRate_New != iPlaybackClockRate)
    {
        // This code starts a rate change.  Any direction and/or timebase change
        // will happen once the rate change is complete.

        // Query the source node if the new playback rate is supported
        PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmd.GetCmdId(), aCmd.GetContext(), PVP_CMD_SourceNodeSetDataSourceRate);

        PVMFCommandId cmdid = -1;
        int32 leavecode = 0;
        OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->SetDataSourceRate(iSourceNodeSessionId, iPlaybackClockRate_New, iOutsideTimebase_New, (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetPlaybackRate() SetDataSourceRate on iSourceNodePBCtrlIF did a leave!"));
                             return PVMFFailure);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetPlaybackRate() Out"));

        return PVMFSuccess;
        // wait for the source node callback, then HandleSourceNodeSetDataSourceRate
    }

    if (iPlaybackDirection_New != iPlaybackDirection)
    {
        // Do a direction change without a rate change.
        PVMFStatus status = UpdateCurrentDirection(aCmd.GetCmdId(), aCmd.GetContext());
        switch (status)
        {
            case PVMFPending:
                // If we get here, engine is Prepared or Started, and we're now
                // waiting on source node command completion followed
                // by a call to HandleSourceNodeSetDataSource.
                // Set the return status to Success, since the caller does not expect
                // PVMFPending.
                status = PVMFSuccess;
                break;
            case PVMFSuccess:
                // If we get here, engine is Paused or Stopped.  The SetPlaybackRate
                // command is done for now, but we need to set the direction when the
                // engine is resumed or prepared.
                if (iOutsideTimebase_New != iOutsideTimebase)
                {
                    UpdateTimebaseAndRate();
                }
                EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
                break;
            default:
                //failure!
                break;
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetPlaybackRate() Out"));
        return status;
    }

    //If we get here it's either a timebase change, or no change at all, so
    //the engine command is complete.
    if (iOutsideTimebase_New != iOutsideTimebase)
    {
        UpdateTimebaseAndRate();
    }

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetPlaybackRate() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::UpdateCurrentDirection(PVMFCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() In"));

    // Launch a direction change sequence.

    PVMFStatus status = PVMFFailure;

    // Check if the source node has direction control
    if (!iSourceNodeDirCtrlIF)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::UpdateCurrentDirection() Direction control IF on source node not available "));
        status = PVMFFailure;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() Out"));
        return status;
    }

    switch (GetPVPlayerState())
    {
        case PVP_STATE_PREPARED:
        case PVP_STATE_STARTED:

            // Change the playback direction immediately
            status = DoSourceNodeSetDataSourceDirection(aCmdId, aCmdContext);
            if (status == PVMFSuccess)
            {
                //return Pending to indicate there is still a node command pending.
                status = PVMFPending;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() DoSourceNodeSetDataSourceDirection failed."));
            }
            break;

        case PVP_STATE_PAUSED:
            if (iChangePlaybackPositionWhenResuming)
            {
                //if there's already a reposition pending, don't allow
                //a direction change also.
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() Reposition already pending-- can't change direction."));
                status = PVMFFailure;
            }
            else
            {
                //The command will complete now-- but the direction change
                //won't actually occur until the engine Resume command.
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() Setting iChangePlaybackDirectionWhenResuming."));
                iChangePlaybackDirectionWhenResuming = true;
                status = PVMFSuccess;
            }
            break;

        default:
            //not supported.
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() Invalid engine state"));
            status = PVMFErrInvalidState;
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateCurrentDirection() Out"));
    return status;
}

PVMFStatus PVPlayerEngine::DoGetPlaybackRate(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlaybackRate() In"));

    int32* rate = aCmd.GetParam(0).pInt32_value;
    OsclTimebase** timebase = (OsclTimebase**)(aCmd.GetParam(1).pOsclAny_value);

    if (rate == NULL || timebase == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlaybackRate() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    if (GetPVPlayerState() != PVP_STATE_PREPARED &&
            GetPVPlayerState() != PVP_STATE_STARTED &&
            GetPVPlayerState() != PVP_STATE_PAUSED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlaybackRate() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    // Fill in with current engine settings for playback rate
    *rate = iPlaybackClockRate * iPlaybackDirection;
    *timebase = iOutsideTimebase;

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlaybackRate() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetPlaybackMinMaxRate(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlaybackMinMaxRate() In"));

    int32* minrate = aCmd.GetParam(0).pInt32_value;
    int32* maxrate = aCmd.GetParam(1).pInt32_value;

    if (minrate == NULL || maxrate == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlaybackMinMaxRate() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Use hardcoded ranges for now
    *minrate = PVP_PBRATE_MIN;
    *maxrate = PVP_PBRATE_MAX;

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlaybackMinMaxRate() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoPrepare(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoPrepare() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoPrepare() In"));

    // Reset the paused-due-to-EOS flag
    iPlaybackPausedDueToEndOfClip = false;

    if (GetPVPlayerState() != PVP_STATE_INITIALIZED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoPrepare() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    if (iDatapathList.empty() == true)
    {
        // No sink added so fail
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoPrepare() Data sinks not added."));
        return PVMFErrNotReady;
    }

    // Query cap-config based on available engine datapaths
    PVMFStatus cmdstatus = DoSourceNodeTrackQuery(aCmd.GetCmdId(), aCmd.GetContext());
    if (cmdstatus != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoPrepare() DoSourceNodeTrackQuery() failed"));
        return cmdstatus;
    }

    SetEngineState(PVP_ENGINE_STATE_PREPARING);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoPrepare() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoSourceNodeTrackQuery(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeTrackQuery() In"));

    if (iSourceNodeTrackSelIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackQuery() Source node track sel IF not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    uint32 i = 0;
    int32 leavecode = 0;
    iNumPendingNodeCmd = 0;

    bool bMimeType = false;
    bool bMediaType = false;

    PVMFMediaPresentationInfo sourcepresinfo;
    PVMFStatus retval = PVMFFailure;
    OSCL_TRY(leavecode, retval = iSourceNodeTrackSelIF->GetMediaPresentationInfo(sourcepresinfo));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackQuery() GetMediaPresentationInfo on iSourceNodeTrackSelIF did a leave!"));
                         return PVMFFailure);
    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackQuery() GetMediaPresentationInfo() call on source node failed"));
        return retval;
    }

    // For each engine datapath, assign a track info from the list
    for (i = 0;i < iDatapathList.size(); ++i)
    {
        // Destroy the track info if present
        if (iDatapathList[i].iTrackInfo)
        {
            OSCL_DELETE(iDatapathList[i].iTrackInfo);
            iDatapathList[i].iTrackInfo = NULL;
        }

        switch (iDatapathList[i].iMediaType)
        {
            case PVP_MEDIATYPE_TEXT:
            {
                for (uint32 j = 0; j < sourcepresinfo.getNumTracks(); ++j)
                {
                    // Go through each track, check codec type, and save the track info
                    PVMFTrackInfo* curtrack = sourcepresinfo.getTrackInfo(j);

                    switch (GetFormatIndex(curtrack->getTrackMimeType().get_str()))
                    {
                        case PVMF_3GPP_TIMEDTEXT:
                            retval = DoQueryTrackInfo(iDatapathList[i], curtrack, aCmdId, aCmdContext);
                            if (retval != PVMFSuccess)
                                return retval;
                            // Break out of the loop
                            j = sourcepresinfo.getNumTracks();

                            bMimeType = true;
                            break;

                        default:
                            break;
                    }
                }
            }

            bMediaType = true;

            break;

            case PVP_MEDIATYPE_VIDEO:
            {
                for (uint32 j = 0; j < sourcepresinfo.getNumTracks(); ++j)
                {
                    // Go through each track, check codec type, and save the track info
                    PVMFTrackInfo* curtrack = sourcepresinfo.getTrackInfo(j);

                    PVMFFormatType srcformat = GetFormatIndex(curtrack->getTrackMimeType().get_str());
                    PVMFFormatType sinkformat = iDatapathList[i].iDataSink->GetDataSinkFormatType();
                    Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;

                    switch (srcformat)
                    {
                        case PVMF_M4V:
                        case PVMF_H263:
                        case PVMF_H264_RAW:
                        case PVMF_H264_MP4:
                        case PVMF_H264:
                        case PVMF_WMV:
                        case PVMF_RV:
                        {
                            if (IsDecNodeNeeded(srcformat, sinkformat) == true && iPlayerNodeRegistry.QueryRegistry(srcformat, sinkformat, foundUuids) != PVMFSuccess)
                            {
                                break;
                            }
                            retval = DoQueryTrackInfo(iDatapathList[i], curtrack, aCmdId, aCmdContext);
                            if (retval != PVMFSuccess)
                                return retval;
                            // Break out of the loop
                            j = sourcepresinfo.getNumTracks();

                            bMimeType = true;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            bMediaType = true;

            break;

            case PVP_MEDIATYPE_AUDIO:
            {
                for (uint32 j = 0; j < sourcepresinfo.getNumTracks(); ++j)
                {
                    // Go through each track, check codec type, and save the track info
                    PVMFTrackInfo* curtrack = sourcepresinfo.getTrackInfo(j);

                    PVMFFormatType srcformat = GetFormatIndex(curtrack->getTrackMimeType().get_str());
                    PVMFFormatType sinkformat = iDatapathList[i].iDataSink->GetDataSinkFormatType();
                    Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;

                    switch (srcformat)
                    {
                        case PVMF_AMR_IETF:
                        case PVMF_AMR_IETF_COMBINED:
                        case PVMF_AMRWB_IETF:
                        case PVMF_AMRWB_IETF_PAYLOAD:
                        case PVMF_AMR_IF2:
                        case PVMF_ADIF:
                        case PVMF_ADTS:
                        case PVMF_MPEG4_AUDIO:
                        case PVMF_LATM:
                        case PVMF_MP3:
                        case PVMF_G726:
                        case PVMF_WMA:
                        case PVMF_REAL_AUDIO:
                        case PVMF_PCM:
                        case PVMF_PCM8:
                        case PVMF_PCM16:
                        case PVMF_PCM16_BE:
                        case PVMF_PCM_ULAW:
                        case PVMF_PCM_ALAW:
                        case PVMF_ASF_MPEG4_AUDIO:
                        {
                            if (IsDecNodeNeeded(srcformat, sinkformat) == true && iPlayerNodeRegistry.QueryRegistry(srcformat, sinkformat, foundUuids) != PVMFSuccess)
                            {
                                break;
                            }

                            retval = DoQueryTrackInfo(iDatapathList[i], curtrack, aCmdId, aCmdContext);
                            if (retval != PVMFSuccess)
                                return retval;
                            // Break out of the loop
                            j = sourcepresinfo.getNumTracks();

                            bMimeType = true;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            bMediaType = true;

            break;

            default:
                // For unknown media type, do nothing
                break;
        }
    }

    if (!bMimeType || !bMediaType)
        return PVMFFailure;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeTrackQuery() Out"));
    return retval;
}

PVMFStatus PVPlayerEngine::DoQueryTrackInfo(PVPlayerEngineDatapath &aDatapath, PVMFTrackInfo* aTrack, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryTrackInfo() In %s", aTrack->getTrackMimeType().get_cstr()));

    int32 leavecode = 0;
    PVPlayerEngineContext* context = NULL;
    PVMFCommandId cmdid = -1;

    PVMFFormatType iSrcFormat = GetFormatIndex(aTrack->getTrackMimeType().get_str());
    PVMFFormatType iSinkFormat = aDatapath.iDataSink->GetDataSinkFormatType();

    if (IsDecNodeNeeded(iSrcFormat, iSinkFormat) == true)
    {
        Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;
        // Query the player node registry for the required decoder node
        if (iPlayerNodeRegistry.QueryRegistry(iSrcFormat, iSinkFormat, foundUuids) == PVMFSuccess)
        {
            if (foundUuids.empty())
            {
                // No matching node found
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() No matching decoder node found"));
                return PVMFFailure;
            }

            leavecode = 0;
            OSCL_TRY(leavecode, aDatapath.iDecNode = iPlayerNodeRegistry.CreateNode(foundUuids[0]));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Error in creating DecNode"));
                                 return PVMFFailure;);

            iNodeUuids.push_back(PVPlayerEngineUuidNodeMapping(foundUuids[0], aDatapath.iDecNode));

            if (aDatapath.iDecNode == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Dec node creation failed"));
                return PVMFErrNoMemory;
            }


            if (aDatapath.iDecNode->ThreadLogon() != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() ThreadLogon() on dec node failed"));
                return PVMFFailure;
            }

            PVMFNodeSessionInfo nodesessioninfo(this, this, (OsclAny*)aDatapath.iDecNode, this, (OsclAny*)aDatapath.iDecNode);
            leavecode = 0;
            OSCL_TRY(leavecode, aDatapath.iDecNodeSessionId = aDatapath.iDecNode->Connect(nodesessioninfo));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Connect on dec node did a leave!"));
                                 return PVMFFailure);
        }
        else
        {
            // Registry query failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Registry query for dec node failed"));
            return PVMFFailure;
        }

        // Query for Metadata IF
        context = AllocateEngineContext(&aDatapath, aDatapath.iDecNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeDecNodeQueryCapConfigIF);

        PVUuid capconfiguuid = PVMI_CAPABILITY_AND_CONFIG_PVUUID;
        cmdid = -1;
        leavecode = 0;
        OSCL_TRY(leavecode, cmdid = aDatapath.iDecNode->QueryInterface(aDatapath.iDecNodeSessionId, capconfiguuid, (PVInterface*&)aDatapath.iDecNodeCapConfigIF, (OsclAny*)context));
        if (leavecode)
        {
            FreeEngineContext(context);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() QueryInterface on dec node for cap-config IF did a leave!"));
        }
        else
        {
            ++iNumPendingNodeCmd;
        }
    }

    if (aDatapath.iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_FILENAME)
    {
        // Create file output node for sink
        leavecode = 0;
        OSCL_TRY(leavecode, aDatapath.iSinkNode = PVFileOutputNodeFactory::CreateFileOutput());
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Creation of file output node did a leave!"));
                             return PVMFErrNoMemory);
    }
    else if (aDatapath.iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_SINKNODE)
    {
        // Use the specified output node for sink node
        aDatapath.iSinkNode = aDatapath.iDataSink->GetDataSinkNodeInterface();
        if (aDatapath.iSinkNode == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Passed in sink node is NULL"));
            return PVMFFailure;
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Unsupported player data sink type"));
        return PVMFErrNotSupported;
    }


    if (aDatapath.iSinkNode->ThreadLogon() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() ThreadLogon() on passed-in sink node failed"));
        return PVMFFailure;
    }

    PVMFNodeSessionInfo nodesessioninfo(this, this, (OsclAny*)aDatapath.iSinkNode, this, (OsclAny*)aDatapath.iSinkNode);
    leavecode = 0;
    OSCL_TRY(leavecode, aDatapath.iSinkNodeSessionId = aDatapath.iSinkNode->Connect(nodesessioninfo));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() Connect() on passed-in sink node did a leave!"));
                         return PVMFFailure);

    // Query for Cap-Config IF
    context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeDecNodeQueryCapConfigIF);

    PVUuid capconfiguuid = PVMI_CAPABILITY_AND_CONFIG_PVUUID;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->QueryInterface(aDatapath.iSinkNodeSessionId, capconfiguuid, (PVInterface*&)aDatapath.iSinkNodeCapConfigIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoQueryTrackInfo() QueryInterface on sink node for cap-config IF did a leave!"));
    }
    else
    {
        ++iNumPendingNodeCmd;
    }

    if (iNumPendingNodeCmd <= 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryTrackInfo() Out No pending QueryInterface() on sink node"));
        return PVMFErrNotSupported;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoQueryTrackInfo() Out"));
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoSourceNodeTrackSelection(PVCommandId /*aCmdId*/, OsclAny* /*aCmdContext*/)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() In"));

    if (iSourceNodeTrackSelIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() Source node track sel IF not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    //populate playable list first
    PVMFStatus retval = DoTrackSelection(true, false);
    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() DoTrackSelection - Populating playable list Failed"));
        return retval;
    }

    bool usepreferencelist = false;
    if (iTrackSelectionHelper != NULL)
    {
        PVMFMediaPresentationInfo localList;
        iPreferenceList.Reset();
        localList.setPresentationType(iPlayableList.getPresentationType());
        localList.setSeekableFlag(iPlayableList.IsSeekable());
        localList.SetDurationAvailable(iPlayableList.IsDurationAvailable());
        localList.setDurationValue(iPlayableList.getDurationValue());
        localList.setDurationTimeScale(iPlayableList.getDurationTimeScale());
        //if track selection helper is present, it means that
        //user of engine wants to provide inputs
        //the reason we use a local list instead of iPreferenceList is
        //due to memory consideration. This call to "SelectTracks" goes
        //to the app and the app allocates memory to populate the local list
        //This memory needs to be released right away. So we make a copy
        //and release the memory for local list.
        PVMFStatus status =
            iTrackSelectionHelper->SelectTracks(iPlayableList, localList);
        if ((status == PVMFSuccess) &&
                (localList.getNumTracks() != 0))
        {
            usepreferencelist = true;
            iPreferenceList = localList;
        }
        //release memory now that we have made a copy
        iTrackSelectionHelper->ReleasePreferenceList(localList);
        //else user made no choice, use playable list
    }

    retval = DoTrackSelection(false, usepreferencelist);
    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() DoTrackSelection - TrackSelection Failed"));
        return retval;
    }

    uint32 i = 0;
    int32 leavecode = 0;

    // Create a selected track list
    PVMFMediaPresentationInfo selectedtracks;
    for (i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackInfo != NULL)
        {
            selectedtracks.addTrackInfo(*(iDatapathList[i].iTrackInfo));
        }
    }

    // Check that at least one track was selected
    if (selectedtracks.getNumTracks() == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() No tracks were selected"));
        return PVMFErrResourceConfiguration;
    }

    // Select in source node
    leavecode = 0;
    OSCL_TRY(leavecode, retval = iSourceNodeTrackSelIF->SelectTracks(selectedtracks));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() SelectTracks on iSourceNodeTrackSelIF did a leave!"));
                         return PVMFFailure);
    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() SelectTracks() on source node failed"));
        return retval;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeTrackSelection() Out"));
    return retval;
}

PVMFStatus PVPlayerEngine::DoTrackSelection(bool oPopulatePlayableListOnly, bool oUsePreferenceList)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoTrackSelection() In"));

    if (iSourceNodeTrackSelIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoTrackSelection() Source node track sel IF not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    uint32 i = 0;
    int32 leavecode = 0;
    PVMFStatus retval = PVMFFailure;

    PVMFMediaPresentationInfo sourcepresinfo;
    if (oPopulatePlayableListOnly)
    {
        iPlayableList.Reset();
        OSCL_TRY(leavecode, retval = iSourceNodeTrackSelIF->GetMediaPresentationInfo(sourcepresinfo));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoTrackSelection() GetMediaPresentationInfo on iSourceNodeTrackSelIF did a leave!"));
                             return PVMFFailure);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoTrackSelection() GetMediaPresentationInfo() call on source node failed"));
            return retval;
        }
        iPlayableList.setPresentationType(sourcepresinfo.getPresentationType());
        iPlayableList.setSeekableFlag(sourcepresinfo.IsSeekable());
        iPlayableList.SetDurationAvailable(sourcepresinfo.IsDurationAvailable());
        iPlayableList.setDurationValue(sourcepresinfo.getDurationValue());
        iPlayableList.setDurationTimeScale(sourcepresinfo.getDurationTimeScale());
    }
    else
    {
        if (oUsePreferenceList)
        {
            //perform track selection based on playable list
            sourcepresinfo = iPreferenceList;
        }
        else
        {
            //perform track selection based on playable list
            sourcepresinfo = iPlayableList;
        }
    }

    // For each engine datapath, assign a track info from the list
    for (i = 0;i < iDatapathList.size(); ++i)
    {
        PVMFStatus checkcodec = PVMFFailure;
        int32 trackId = -1;
        // Destroy the track info if present
        if (iDatapathList[i].iTrackInfo)
        {
            OSCL_DELETE(iDatapathList[i].iTrackInfo);
            iDatapathList[i].iTrackInfo = NULL;
        }

        switch (iDatapathList[i].iMediaType)
        {
            case PVP_MEDIATYPE_TEXT:
            {
                for (uint32 j = 0; j < sourcepresinfo.getNumTracks(); ++j)
                {
                    // Go through each track, check codec type, and save the track info
                    PVMFTrackInfo* curtrack = sourcepresinfo.getTrackInfo(j);

                    switch (GetFormatIndex(curtrack->getTrackMimeType().get_str()))
                    {
                        case PVMF_3GPP_TIMEDTEXT:
                        {
                            if (oPopulatePlayableListOnly)
                            {
                                //add it to playable list
                                iPlayableList.addTrackInfo(*curtrack);
                            }
                            else
                            {
                                leavecode = 0;
                                OSCL_TRY(leavecode, iDatapathList[i].iTrackInfo = OSCL_NEW(PVMFTrackInfo, (*curtrack)));
                                OSCL_FIRST_CATCH_ANY(leavecode,
                                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoTrackSelection() Could not allocate memory for PVMFTrackInfo"));
                                                     return PVMFErrNoMemory);
                                // Break out of the loop
                                j = sourcepresinfo.getNumTracks();
                            }
                        }
                        break;

                        default:
                            break;
                    }
                }
            }
            break;

            case PVP_MEDIATYPE_VIDEO:
            {
                for (uint32 j = 0; j < sourcepresinfo.getNumTracks(); ++j)
                {
                    // Go through each track, check codec type, and save the track info
                    PVMFTrackInfo* curtrack = sourcepresinfo.getTrackInfo(j);

                    PVMFFormatType srcformat = GetFormatIndex(curtrack->getTrackMimeType().get_str());
                    PVMFFormatType sinkformat = iDatapathList[i].iDataSink->GetDataSinkFormatType();
                    Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;

                    switch (srcformat)
                    {
                        case PVMF_M4V:
                        case PVMF_H263:
                        case PVMF_H264_RAW:
                        case PVMF_H264_MP4:
                        case PVMF_H264:
                        case PVMF_WMV:
                        case PVMF_RV:
                        {
                            trackId = curtrack->getTrackID();
                            if (IsDecNodeNeeded(srcformat, sinkformat) == true &&
                                    iPlayerNodeRegistry.QueryRegistry(srcformat, sinkformat, foundUuids) != PVMFSuccess)
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoTrackSelection: Unsupported codec trackID=%d due to NodeRegistry", trackId));
                                break;
                            }

                            retval = DoVerifyTrackInfo(iDatapathList[i], curtrack, checkcodec);
                            if (retval != PVMFSuccess)
                                return retval;

                            if (oPopulatePlayableListOnly)
                            {
                                if (checkcodec == PVMFSuccess)
                                {
                                    //add it to playable list
                                    iPlayableList.addTrackInfo(*curtrack);
                                }
                            }
                            else
                            {
                                // Only select base track in case of temporal scalability
                                if (curtrack->DoesTrackHaveDependency() == false && checkcodec == PVMFSuccess)
                                {
                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoTrackSelection: selected video trackID=%d", trackId));
                                    leavecode = 0;
                                    OSCL_TRY(leavecode, iDatapathList[i].iTrackInfo = OSCL_NEW(PVMFTrackInfo, (*curtrack)));
                                    OSCL_FIRST_CATCH_ANY(leavecode,
                                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoTrackSelection() Could not allocate memory for PVMFTrackInfo"));
                                                         return PVMFErrNoMemory);
                                    // Break out of the loop
                                    j = sourcepresinfo.getNumTracks();
                                }
                            }
                        }
                        break;

                        default:
                            break;
                    }
                }
            }
            break;

            case PVP_MEDIATYPE_AUDIO:
            {
                for (uint32 j = 0; j < sourcepresinfo.getNumTracks(); ++j)
                {
                    // Go through each track, check codec type, and save the track info
                    PVMFTrackInfo* curtrack = sourcepresinfo.getTrackInfo(j);

                    PVMFFormatType srcformat = GetFormatIndex(curtrack->getTrackMimeType().get_str());
                    PVMFFormatType sinkformat = iDatapathList[i].iDataSink->GetDataSinkFormatType();
                    Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;

                    switch (srcformat)
                    {
                        case PVMF_AMR_IETF:
                        case PVMF_AMR_IETF_COMBINED:
                        case PVMF_AMR_IF2:
                        case PVMF_ADIF:
                        case PVMF_ADTS:
                        case PVMF_MPEG4_AUDIO:
                        case PVMF_LATM:
                        case PVMF_MP3:
                        case PVMF_G726:
                        case PVMF_WMA:
                        case PVMF_REAL_AUDIO:
                        case PVMF_PCM:
                        case PVMF_PCM8:
                        case PVMF_PCM16:
                        case PVMF_PCM16_BE:
                        case PVMF_PCM_ULAW:
                        case PVMF_PCM_ALAW:
                        case PVMF_AMRWB_IETF:
                        case PVMF_ASF_MPEG4_AUDIO:
                        case PVMF_AMRWB_IETF_PAYLOAD:
                        {
                            trackId = curtrack->getTrackID();
                            if (IsDecNodeNeeded(srcformat, sinkformat) == true && iPlayerNodeRegistry.QueryRegistry(srcformat, sinkformat, foundUuids) != PVMFSuccess)
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoTrackSelection: Unsupported codec trackID=%d due to NodeRegistry", trackId));
                                break;
                            }

                            retval = DoVerifyTrackInfo(iDatapathList[i], curtrack, checkcodec);
                            if (retval != PVMFSuccess)
                                return retval;
                            if (oPopulatePlayableListOnly)
                            {
                                if (checkcodec == PVMFSuccess)
                                {
                                    //add it to playable list
                                    iPlayableList.addTrackInfo(*curtrack);
                                }
                            }
                            else
                            {
                                if (checkcodec == PVMFSuccess)
                                {
                                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoTrackSelection: selected audio trackID=%d", trackId));
                                    leavecode = 0;
                                    OSCL_TRY(leavecode, iDatapathList[i].iTrackInfo = OSCL_NEW(PVMFTrackInfo, (*curtrack)));
                                    OSCL_FIRST_CATCH_ANY(leavecode,
                                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoTrackSelection() Could not allocate memory for PVMFTrackInfo"));
                                                         return PVMFErrNoMemory);
                                    // Break out of the loop
                                    j = sourcepresinfo.getNumTracks();
                                }
                            }
                        }
                        break;

                        default:
                            break;
                    }
                }
            }
            break;

            default:
                // For unknown media type, do nothing
                break;
        }

        if (checkcodec != PVMFSuccess && trackId >= 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoTrackSelection() Bad track config for TrackId=%d", trackId));
            SendInformationalEvent(PVMFInfoTrackDisable, NULL, (OsclAny*)trackId, NULL, NULL);
        }
    }
    return retval;
}


PVMFStatus PVPlayerEngine::DoVerifyTrackInfo(PVPlayerEngineDatapath &aDatapath, PVMFTrackInfo* aTrack, PVMFStatus& aCheckcodec)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoVerifyTrackInfo() In %s", aTrack->getTrackMimeType().get_cstr()));

    PVMFStatus status = PVMFSuccess;
    OsclMemAllocator alloc;
    PvmiKvp kvp;
    kvp.key = NULL;

    char* aFormatValType = PVMF_FORMAT_SPECIFIC_INFO_KEY;
    OsclRefCounterMemFrag aConfig;

    kvp.length = oscl_strlen(aFormatValType) + 1; // +1 for \0
    kvp.key = (PvmiKeyType)alloc.ALLOCATE(kvp.length);
    if (kvp.key == NULL)
    {
        return PVMFErrNoMemory;
    }
    oscl_strncpy(kvp.key, aFormatValType, kvp.length);
    aConfig = aTrack->getTrackConfigInfo();
    kvp.value.key_specific_value = (OsclAny*)(aConfig.getMemFragPtr());
    kvp.capacity = aConfig.getMemFragSize();

    //Check if we have decoder node cap-config
    if (aDatapath.iDecNodeCapConfigIF != NULL)
    {
        PVMFFormatType DecnodeFormatType = GetFormatIndex(aTrack->getTrackMimeType().get_str());
        switch (DecnodeFormatType)
        {
            case PVMF_M4V:
            case PVMF_H263:
            case PVMF_H264:
            case PVMF_H264_MP4:
            case PVMF_H264_RAW:
            case PVMF_LATM:
            case PVMF_MPEG4_AUDIO:
            case PVMF_ASF_MPEG4_AUDIO:
            case PVMF_WMV:
            {
                PvmiKvp* iErrorKVP = NULL;
                PvmiKvp iKVPSetFormat;
                iKVPSetFormat.key = NULL;
                OSCL_StackString<64> iKeyStringSetFormat;

                if ((DecnodeFormatType == PVMF_M4V) ||
                        (DecnodeFormatType == PVMF_H263) ||
                        (DecnodeFormatType == PVMF_H264) ||
                        (DecnodeFormatType == PVMF_H264_MP4) ||
                        (DecnodeFormatType == PVMF_H264_RAW) ||
                        (DecnodeFormatType == PVMF_WMV)
                   )
                {
                    iKeyStringSetFormat = _STRLIT_CHAR("x-pvmf/video/decoder/format-type;valtype=uint32");
                }
                else
                {
                    iKeyStringSetFormat += _STRLIT_CHAR("x-pvmf/audio/decoder/format-type;valtype=uint32");
                }
                iKVPSetFormat.key = iKeyStringSetFormat.get_str();
                iKVPSetFormat.value.uint32_value = DecnodeFormatType;

                aDatapath.iDecNodeCapConfigIF->setParametersSync(NULL, &iKVPSetFormat, 1, iErrorKVP);
            }
            break;
            default:
                break;
        }

        //verify codec specific info
        int32 leavecode = 0;
        OSCL_TRY(leavecode, aCheckcodec = aDatapath.iDecNodeCapConfigIF->verifyParametersSync(NULL, &kvp, 1));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyTrackInfo() unsupported verifyParametersSync did a leave!"));
                             alloc.deallocate((OsclAny*)(kvp.key));
                             aCheckcodec = PVMFSuccess; // set it success in case track selection info is not yet available;
                             return PVMFSuccess;);

        if (aCheckcodec != PVMFSuccess)
        {
            alloc.deallocate((OsclAny*)(kvp.key));
            //In case of other error code, this is operation error.
            if (aCheckcodec != PVMFErrNotSupported)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyTrackInfo() verifyParametersSync() on decoder node failed"));
                return aCheckcodec;
            }
            return status;
        }

        int numKvp = 0;
        PvmiKvp* kvpPtr;
        // Query using get
        OSCL_StackString<64> querykey;

        switch (aDatapath.iMediaType)
        {
            case PVP_MEDIATYPE_VIDEO:
            {
                querykey = _STRLIT_CHAR("x-pvmf/video/render");
                if (aDatapath.iDecNodeCapConfigIF->getParametersSync(NULL, querykey.get_str(), kvpPtr, numKvp, NULL) == PVMFSuccess)
                {
                    //verify width/height
                    if (aDatapath.iSinkNodeCapConfigIF != NULL)
                        aCheckcodec = aDatapath.iSinkNodeCapConfigIF->verifyParametersSync(NULL, kvpPtr, numKvp);
                    status = aDatapath.iDecNodeCapConfigIF->releaseParameters(NULL, kvpPtr, numKvp);
                    if (status != PVMFSuccess || aCheckcodec != PVMFSuccess)
                        break;
                }
            }
            break;
            case PVP_MEDIATYPE_AUDIO:
            {
                querykey = _STRLIT_CHAR("x-pvmf/audio/render");
                if (aDatapath.iDecNodeCapConfigIF->getParametersSync(NULL, querykey.get_str(), kvpPtr, numKvp, NULL) == PVMFSuccess)
                {
                    //verify samplerate and channels
                    if (aDatapath.iSinkNodeCapConfigIF != NULL)
                        aCheckcodec = aDatapath.iSinkNodeCapConfigIF->verifyParametersSync(NULL, kvpPtr, numKvp);
                    status = aDatapath.iDecNodeCapConfigIF->releaseParameters(NULL, kvpPtr, numKvp);
                    if (status != PVMFSuccess || aCheckcodec != PVMFSuccess)
                        break;
                }
            }
            break;
            default:
                break;
        }
    }
    else
    {
        if (aDatapath.iSinkNodeCapConfigIF != NULL)
        {
            aCheckcodec = aDatapath.iSinkNodeCapConfigIF->verifyParametersSync(NULL, &kvp, 1);
        }
    }

    alloc.deallocate((OsclAny*)(kvp.key));
    if (aCheckcodec != PVMFSuccess)
    {
        //In case of other error code, this is operation error.
        if (aCheckcodec != PVMFErrNotSupported)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyTrackInfo() verifyParametersSync() on sink node failed"));
            return aCheckcodec;
        }
        return status;
    }

    //verify bitrate
    PvmiKvp iKVPBitRate;
    iKVPBitRate.key = NULL;

    OSCL_StackString<64> iKVPStringBitRate = _STRLIT_CHAR(PVMF_BITRATE_VALUE_KEY);
    iKVPBitRate.key = iKVPStringBitRate.get_str();
    iKVPBitRate.value.uint32_value = aTrack->getTrackBitRate();

    if (aDatapath.iSinkNodeCapConfigIF != NULL)
        aCheckcodec = aDatapath.iSinkNodeCapConfigIF->verifyParametersSync(NULL, &iKVPBitRate, 1);
    //In case of other error code, this is operation error.
    if (aCheckcodec != PVMFSuccess)
    {
        //In case of other error code, this is operation error.
        if (aCheckcodec != PVMFErrNotSupported)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyTrackInfo() verifyParametersSync() on sinknode bitrate failed"));
            return aCheckcodec;
        }
        return status;
    }

    //verify video framerate
    switch (aDatapath.iMediaType)
    {
        case PVP_MEDIATYPE_VIDEO:
            if (aTrack->getTrackFrameRate() > 0)
            {
                PvmiKvp iKVPFrameRate;
                iKVPFrameRate.key = NULL;

                OSCL_StackString<64> iKVPStringFrameRate = _STRLIT_CHAR(PVMF_FRAMERATE_VALUE_KEY);
                iKVPFrameRate.key = iKVPStringFrameRate.get_str();
                iKVPFrameRate.value.uint32_value = aTrack->getTrackFrameRate();

                if (aDatapath.iSinkNodeCapConfigIF != NULL)
                    aCheckcodec = aDatapath.iSinkNodeCapConfigIF->verifyParametersSync(NULL, &iKVPFrameRate, 1);
                //In case of other error code, this is operation error.
                if (aCheckcodec != PVMFErrNotSupported && aCheckcodec != PVMFSuccess)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyTrackInfo() verifyParametersSync() on sink node framerate failed"));
                    return aCheckcodec;
                }
            }
            break;
        default:
            break;
    }
    return status;
}

PVMFStatus PVPlayerEngine::DoSinkNodeInit(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSinkNodeInit() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeInit() In"));

    // For each engine datapath in the list, initiate the setup sequence
    iNumPendingNodeCmd = 0;
    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;

    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iSinkNode != NULL)
        {
            // Call Init() on the sink node
            PVPlayerEngineContext* context = AllocateEngineContext(&(iDatapathList[i]), iDatapathList[i].iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeDecNodeVerifyParameter);

            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iDatapathList[i].iSinkNode->Init(iDatapathList[i].iSinkNodeSessionId, (OsclAny*) context));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeInit() Init on iSinkNode did a leave")););
            if (cmdid != -1 && leavecode == 0)
            {
                ++iNumPendingNodeCmd;
            }
            else
            {
                FreeEngineContext(context);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeInit() Out"));
    if (iNumPendingNodeCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeInit() No datapath could be prepared!"));
        return PVMFFailure;
    }
    else
    {
        return PVMFSuccess;
    }
}

PVMFStatus PVPlayerEngine::DoSinkNodeReset(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSinkNodeReset() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeReset() In"));

    // For each engine datapath in the list, initiate the setup sequence
    iNumPendingNodeCmd = 0;
    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;

    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iSinkNode != NULL)
        {
            // Call Reseet() on the sink node
            PVPlayerEngineContext* context = AllocateEngineContext(&(iDatapathList[i]), iDatapathList[i].iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeDecNodeReset);

            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iDatapathList[i].iSinkNode->Reset(iDatapathList[i].iSinkNodeSessionId, (OsclAny*) context));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeReset() Reseet on iSinkNode did a leave")););
            if (cmdid != -1 && leavecode == 0)
            {
                ++iNumPendingNodeCmd;
            }
            else
            {
                FreeEngineContext(context);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeReset() Out"));
    if (iNumPendingNodeCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeReset() No datapath could be prepared!"));
        return PVMFFailure;
    }
    else
    {
        return PVMFSuccess;
    }
}

PVMFStatus PVPlayerEngine::DoSourceNodePrepare(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSourceNodePrepare() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodePrepare() In"));

    if (iSourceNode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodePrepare() Source node not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    // If source node is already in Prepared state then don't call Prepare()
    if (iSourceNode->GetState() == EPVMFNodePrepared)
    {
        // For each engine datapath in the list, initiate the setup sequence
        iNumPendingDatapathCmd = 0;
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackInfo != NULL)
            {
                PVMFStatus cmdstatus = DoSetupSinkNode(iDatapathList[i], aCmdId, aCmdContext);
                if (cmdstatus == PVMFSuccess)
                {
                    ++iNumPendingDatapathCmd;
                }
            }
        }

        if (iNumPendingDatapathCmd == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodePrepare() No datapath could be prepared!"));
            return PVMFFailure;
        }
        else
        {
            return PVMFSuccess;
        }
    }

    // Call Prepare() on the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodePrepare);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->Prepare(iSourceNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodePrepare() Prepare on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodePrepare() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSetupSinkNode(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSetupSinkNode() for %s Tick=%d",
                     aDatapath.iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupSinkNode() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    int32 leavecode = 0;

    // Check if datapath can be setup
    if (aDatapath.iTrackInfo == NULL || aDatapath.iDataSink == NULL)
    {
        // No
        OSCL_ASSERT(false);
        return PVMFErrNotSupported;
    }

    // Determine the sink node based on PVPlayerDataSink
    if (aDatapath.iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_FILENAME)
    {
        // Create file output node for sink
        leavecode = 0;
        OSCL_TRY(leavecode, aDatapath.iSinkNode = PVFileOutputNodeFactory::CreateFileOutput());
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() Creation of file output node did a leave!"));
                             return PVMFErrNoMemory);

        if (aDatapath.iSinkNode->ThreadLogon() != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() ThreadLogon() on file output sink node failed"));
            return PVMFFailure;
        }

        PVMFNodeSessionInfo nodesessioninfo(this, this, (OsclAny*)(aDatapath.iSinkNode), this, (OsclAny*)(aDatapath.iSinkNode));
        leavecode = 0;
        OSCL_TRY(leavecode, aDatapath.iSinkNodeSessionId = aDatapath.iSinkNode->Connect(nodesessioninfo));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() Connect on  file output sink node did a leave!"));
                             return PVMFFailure);

        // Request the fileoutput node config interface
        PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeQueryFOConfigIF);

        PVMFCommandId cmdid = -1;
        leavecode = 0;
        OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->QueryInterface(aDatapath.iSinkNodeSessionId, PvmfFileOutputNodeConfigUuid, (PVInterface*&)aDatapath.iSinkNodeFOConfigIF, (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() QueryInterface on file output sink node did a leave!"));
                             return PVMFFailure);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupSinkNode() Out"));

        return PVMFSuccess;
    }
    else if (aDatapath.iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_SINKNODE)
    {
        // Use the specified output node for sink node
        aDatapath.iSinkNode = aDatapath.iDataSink->GetDataSinkNodeInterface();
        if (aDatapath.iSinkNode == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() Passed in sink node is NULL"));
            return PVMFFailure;
        }

        if (aDatapath.iSinkNode->ThreadLogon() != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() ThreadLogon() on passed-in sink node failed"));
            return PVMFFailure;
        }

        PVMFNodeSessionInfo nodesessioninfo(this, this, (OsclAny*)aDatapath.iSinkNode, this, (OsclAny*)aDatapath.iSinkNode);
        leavecode = 0;
        OSCL_TRY(leavecode, aDatapath.iSinkNodeSessionId = aDatapath.iSinkNode->Connect(nodesessioninfo));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() Connect() on passed-in sink node did a leave!"));
                             return PVMFFailure);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupSinkNode() Out"));
        return DoSinkNodeQueryInterfaceOptional(aDatapath, aCmdId, aCmdContext);
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupSinkNode() Unsupported player data sink type"));
        return PVMFErrNotSupported;
    }
}


PVMFStatus PVPlayerEngine::DoSinkNodeQueryInterfaceOptional(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() Tick=%d", OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    int32 leavecode = 0;

    if (aDatapath.iSinkNode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() Sink node not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    // Request optional extension interface from the sink node
    PVPlayerEngineContext* context = NULL;
    PVMFCommandId cmdid = -1;
    aDatapath.iNumPendingCmd = 0;

    // Request the sync control interface for the sink node
    context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeQuerySyncCtrlIF);
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->QueryInterface(aDatapath.iSinkNodeSessionId, PvmfNodesSyncControlUuid, (PVInterface*&)aDatapath.iSinkNodeSyncCtrlIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() QueryInterface on sink node for sync control IF did a leave!"));
    }
    else
    {
        ++aDatapath.iNumPendingCmd;
    }

    // Query for Metadata IF
    context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeQueryMetadataIF);
    PVUuid metadatauuid = KPVMFMetadataExtensionUuid;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->QueryInterface(aDatapath.iSinkNodeSessionId, metadatauuid, (PVInterface*&)aDatapath.iSinkNodeMetadataExtIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() QueryInterface on sink node for metadata IF did a leave!"));
    }
    else
    {
        ++aDatapath.iNumPendingCmd;
    }

    // Query for Cap-Config IF
    context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeQueryCapConfigIF);
    PVUuid capconfiguuid = PVMI_CAPABILITY_AND_CONFIG_PVUUID;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->QueryInterface(aDatapath.iSinkNodeSessionId, capconfiguuid, (PVInterface*&)aDatapath.iSinkNodeCapConfigIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() QueryInterface on sink node for cap-config IF did a leave!"));
    }
    else
    {
        ++aDatapath.iNumPendingCmd;
    }

    if (aDatapath.iNumPendingCmd > 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() Out"));
        return PVMFSuccess;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeQueryInterfaceOptional() Out No pending QueryInterface() on sink node"));
        return PVMFErrNotSupported;
    }
}


PVMFStatus PVPlayerEngine::DoSetupDecNode(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSetupDecNode() for %s Tick=%d",
                     aDatapath.iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupDecNode() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));
    int32 leavecode = 0;

    // Create decoder node if necessary
    aDatapath.iSrcFormat = GetFormatIndex(aDatapath.iTrackInfo->getTrackMimeType().get_str());
    aDatapath.iSinkFormat = aDatapath.iDataSink->GetDataSinkFormatType();
    // Determine if decoder node is needed. In future, this will be replaced by querying
    // the capability of data sink to match with source format type
    if (IsDecNodeNeeded(aDatapath.iSrcFormat, aDatapath.iSinkFormat) == true)
    {
        Oscl_Vector<PVUuid, OsclMemAllocator> foundUuids;
        // Query the player node registry for the required decoder node
        if (iPlayerNodeRegistry.QueryRegistry(aDatapath.iSrcFormat, aDatapath.iSinkFormat, foundUuids) == PVMFSuccess)
        {
            if (foundUuids.empty())
            {
                // No matching node found
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() No matching decoder node found"));
                return PVMFFailure;
            }

            leavecode = 0;
            OSCL_TRY(leavecode, aDatapath.iDecNode = iPlayerNodeRegistry.CreateNode(foundUuids[0]));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() Error in creating DecNode"));
                                 return PVMFFailure;);

            iNodeUuids.push_back(PVPlayerEngineUuidNodeMapping(foundUuids[0], aDatapath.iDecNode));

            if (aDatapath.iDecNode == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() Dec node creation failed"));
                return PVMFErrNoMemory;
            }

            if (aDatapath.iDecNode->ThreadLogon() != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() ThreadLogon() on dec node failed"));
                return PVMFFailure;
            }

            PVMFNodeSessionInfo nodesessioninfo(this, this, (OsclAny*)aDatapath.iDecNode, this, (OsclAny*)aDatapath.iDecNode);
            leavecode = 0;
            OSCL_TRY(leavecode, aDatapath.iDecNodeSessionId = aDatapath.iDecNode->Connect(nodesessioninfo));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() Connect on dec node did a leave!"));
                                 return PVMFFailure);

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupDecNode() Out"));
            return DoDecNodeQueryInterfaceOptional(aDatapath, aCmdId, aCmdContext);
        }
        else
        {
            // Registry query failed
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetupDecNode() Registry query for dec node failed"));
            return PVMFFailure;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSetupDecNode() Decoder node not needed"));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetupDecNode() Out"));
    return PVMFErrNotSupported;
}


bool PVPlayerEngine::IsDecNodeNeeded(PVMFFormatType& aSrcFormat, PVMFFormatType& aSinkFormat)
{
    OSCL_ASSERT(aSrcFormat != PVMF_FORMAT_UNKNOWN);
    OSCL_ASSERT(aSinkFormat != PVMF_FORMAT_UNKNOWN);

    if ((aSinkFormat == PVMF_YUV420) || (aSinkFormat == PVMF_RGB16))
    {
        // For uncompressed video, decoder node is always needed
        return true;
    }
    else if ((aSinkFormat >= PVMF_FIRST_UNCOMPRESSED_AUDIO && aSinkFormat <= PVMF_LAST_UNCOMPRESSED_AUDIO) &&
             (!(aSrcFormat >= PVMF_FIRST_UNCOMPRESSED_AUDIO && aSrcFormat <= PVMF_LAST_UNCOMPRESSED_AUDIO)))
    {
        // For uncompressed audio, decoder node is needed if the source type is compressed
        aSinkFormat = PVMF_PCM16;
        return true;
    }
    else
    {
        // Set the sink format to be the same as the source format as default
        aSinkFormat = aSrcFormat;
        return false;
    }
}


PVMFStatus PVPlayerEngine::DoDecNodeQueryInterfaceOptional(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::DoDecNodeQueryInterfaceOptional() Tick=%d", OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDecNodeQueryInterfaceOptional() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    // Check if the dec node is present
    if (aDatapath.iDecNode == NULL)
    {
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    PVPlayerEngineContext* context = NULL;
    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;

    aDatapath.iNumPendingCmd = 0;

    // Query for Metadata IF
    context = AllocateEngineContext(&aDatapath, aDatapath.iDecNode, NULL, aCmdId, aCmdContext, PVP_CMD_DecNodeQueryMetadataIF);
    PVUuid metadatauuid = KPVMFMetadataExtensionUuid;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iDecNode->QueryInterface(aDatapath.iDecNodeSessionId, metadatauuid, (PVInterface*&)aDatapath.iDecNodeMetadataExtIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoDecNodeQueryInterfaceOptional() QueryInterface on dec node for metadata IF did a leave!"));
    }
    else
    {
        ++aDatapath.iNumPendingCmd;
    }

    // Query for Cap-Config IF
    context = AllocateEngineContext(&aDatapath, aDatapath.iDecNode, NULL, aCmdId, aCmdContext, PVP_CMD_DecNodeQueryCapConfigIF);
    PVUuid capconfiguuid = PVMI_CAPABILITY_AND_CONFIG_PVUUID;
    cmdid = -1;
    leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iDecNode->QueryInterface(aDatapath.iDecNodeSessionId, capconfiguuid, (PVInterface*&)aDatapath.iDecNodeCapConfigIF, (OsclAny*)context));
    if (leavecode)
    {
        FreeEngineContext(context);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoDecNodeQueryInterfaceOptional() QueryInterface on dec node for cap-config IF did a leave!"));
    }
    else
    {
        ++aDatapath.iNumPendingCmd;
    }

    if (aDatapath.iNumPendingCmd > 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDecNodeQueryInterfaceOptional() Out"));
        return PVMFSuccess;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDecNodeQueryInterfaceOptional() Out No pending QueryInterface() on dec node"));
        return PVMFErrNotSupported;
    }
}


PVMFStatus PVPlayerEngine::DoDatapathPrepare(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoDatapathPrepare() for %s Tick=%d",
                     aDatapath.iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathPrepare() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));
    int32 leavecode = 0;

    // Create the datapath utility object
    if (aDatapath.iDatapath == NULL)
    {
        leavecode = 0;
        OSCL_TRY(leavecode, aDatapath.iDatapath = OSCL_NEW(PVPlayerDatapath, ()));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoDatapathPrepare() Could not create datapath object"));
                             return PVMFErrNoMemory);
    }

    // Configure the datapath utility based on datapath configuration
    aDatapath.iDatapath->SetObserver(*this, *this, *this);
    aDatapath.iDatapath->SetSourceNode(iSourceNode);
    aDatapath.iDatapath->SetSinkNode(aDatapath.iSinkNode);

    if (aDatapath.iDecNode)
    {
        aDatapath.iDatapath->SetDecNode(aDatapath.iDecNode);
        aDatapath.iDatapath->SetSourceDecTrackInfo(*(aDatapath.iTrackInfo));
        aDatapath.iDatapath->SetDecSinkFormatType(aDatapath.iSinkFormat);
    }
    else
    {
        aDatapath.iDatapath->SetSourceSinkTrackInfo(*(aDatapath.iTrackInfo));
    }

    // Prepare the datapath
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPPrepare);

    aDatapath.iTrackActive = true;
    PVMFStatus retval = aDatapath.iDatapath->Prepare((OsclAny*)context);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathPrepare() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoSourceNodeQueryDataSourcePosition(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryDataSourcePosition() In"));

    // get the current playback position, which needs to be passed to parsernode in QueryDataSourcePosition
    // to check whether reposotioning done is forward or backward.
    PVPPlaybackPosition curpos;
    curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    GetPlaybackClockPosition(curpos);

    // Initialize the playback position and clock variables
    iActualMediaDataTS = 0;
    iAdjustedMediaDataTS = 0;
    iActualPlaybackPosition = 0;
    iStartNPT = 0;
    iStartMediaDataTS = 0;

    // Check if the source node has position control IF
    if (iSourceNodePBCtrlIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryDataSourcePosition() Playback control IF on source node not available"));
        return PVMFErrNotSupported;
    }

    PVMFCommandId cmdid = -1;

    uint32 timems = 0;
    if (iCurrentBeginPosition.iIndeterminate == false)
    {
        // Convert beginning position to milliseconds
        PVMFStatus retval = ConvertToMillisec(iCurrentBeginPosition, timems);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryDataSourcePosition() Converting to millisec failed"));
            return retval;
        }
    }
    iCurrentBeginPosition.iPosValue.millisec_value = timems;
    iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

    if (iSeekToSyncPoint && iSyncPointSeekWindow > 0)
    {
        // Need to query the data source on where playback would actually begin from
        PVMFTimestamp startpos = iCurrentBeginPosition.iPosValue.millisec_value;

        PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeQueryDataSourcePosition);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryDataSourcePosition() Calling QueryDataSourcePosition() Starting pos %d ms, SeekToSyncPt %d", startpos, iSeekToSyncPoint));

        iActualPlaybackPosition = curpos.iPosValue.millisec_value;
        iSeekPointBeforeTargetNPT = 0;
        iSeekPointAfterTargetNPT = 0;

        // As in case of MP4 file we need to call overload function of QueryDataSourcePosition which retruns
        // I frame before and after instead of actaul NPT, format type will be checked here to first find if
        // format-type is one of the MP4 varient

        PVMFNodeCapability nodeCapability;
        iSourceNode->GetCapability(nodeCapability);
        PVMFFormatType * formatType = nodeCapability.iInputFormatCapability.begin();
        bool mpeg4FormatType = false;
        if (formatType != NULL)
        {
            switch (*formatType)
            {
                case PVMF_MPEG4FF:
                    mpeg4FormatType = true;
                    break;

                default:
                    mpeg4FormatType = false;
                    break;
            }
        }
        int32 leavecode = 0;
        if (mpeg4FormatType)
        {
            OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->QueryDataSourcePosition(iSourceNodeSessionId, startpos,
                                        iSeekPointBeforeTargetNPT, iSeekPointAfterTargetNPT, (OsclAny*) context, iSeekToSyncPoint));
        }
        else
        {
            OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->QueryDataSourcePosition(iSourceNodeSessionId, startpos, iActualPlaybackPosition,
                                        iSeekToSyncPoint, (OsclAny*)context));
        }

        if (leavecode != 0)
        {
            FreeEngineContext(context);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeQueryDataSourcePosition() QueryDataSourcePosition on iSourceNodePBCtrlIF did a leave!"));
            if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
            {
                // Since position qquery is not supported, assume the repositioning will
                // go to the requested position
                // Determine the SetDataSourcePosition parameter based on  reposition settings
                PVMFTimestamp requesttime = iCurrentBeginPosition.iPosValue.millisec_value;
                iActualPlaybackPosition = requesttime;
                bool seektosyncpt = true;

                // Do the source positioning
                return DoSourceNodeSetDataSourcePosition(aCmdId, aCmdContext, requesttime, seektosyncpt);
            }
            else
            {
                return PVMFFailure;
            }
        }
    }
    else
    {
        // Go straight to repositioning the data source
        PVMFTimestamp startpos = iCurrentBeginPosition.iPosValue.millisec_value;
        return DoSourceNodeSetDataSourcePosition(aCmdId, aCmdContext, startpos, iSeekToSyncPoint);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeQueryDataSourcePosition() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSourceNodeSetDataSourcePosition(PVCommandId aCmdId, OsclAny* aCmdContext, PVMFTimestamp aTargetNPT, bool aSeekToSyncPoint)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePosition() In"));

    // Check if the source node has position control IF
    if (iSourceNodePBCtrlIF == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePosition() Playback control IF on source node not available"));

        // Since repositioning IF is not supported by this source node, assume the playback
        // will start from time 0
        iActualPlaybackPosition = 0;
        iActualMediaDataTS = 0;
        iAdjustedMediaDataTS = 0;
        // Then continue to handle like success case
        iStartNPT = 0;
        iStartMediaDataTS = 0;
        // Save the actual starting position for GetPlaybackRange() query
        iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
        iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

        // Repositioning so reset the EOS received flag for each active datapath
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive)
            {
                iDatapathList[i].iEndOfDataReceived = false;
            }
        }

        // Start the source node
        return DoSourceNodeStart(aCmdId, aCmdContext);
    }

    // Set the position of the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeSetDataSourcePosition);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePosition() Calling SetDataSourcePosition() TargetNPT %d ms, SeekToSyncPoint %d", aTargetNPT, aSeekToSyncPoint));

    int32 tempStreamID = 0;
    // if reposition points exists in between the actualPlaybackPosition and
    // the currentPlaybackClock position, then the reposition data exists with
    // in the graph, so we need to start a new media segment
    PVPPlaybackPosition curpos;
    curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    GetPlaybackClockPosition(curpos);

    tempStreamID = iStreamID;

    iActualMediaDataTS = 0;
    iAdjustedMediaDataTS = 0;
    iActualPlaybackPosition = 0;
    int32 leavecode = 0;
    PVMFCommandId cmdid = -1;

    OSCL_TRY(leavecode, cmdid = iSourceNodePBCtrlIF->SetDataSourcePosition(iSourceNodeSessionId, aTargetNPT, iActualPlaybackPosition, iActualMediaDataTS, aSeekToSyncPoint, tempStreamID, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePosition() SetDataSourcePosition on iSourceNodePBCtrlIF did a leave!"));
                         if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
{
    // Since this repositioning was not supported, assume the playback
    // will start from time 0
    iActualPlaybackPosition = 0;
    iActualMediaDataTS = 0;
    iAdjustedMediaDataTS = 0;
    // Then continue to handle like success case
    iStartNPT = 0;
    iStartMediaDataTS = 0;
    // Save the actual starting position for GetPlaybackRange() query
    iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
    iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

    // Repositioning so reset the EOS received flag for each active datapath
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive)
            {
                iDatapathList[i].iEndOfDataReceived = false;
            }
        }

        // Start the source node
        return DoSourceNodeStart(aCmdId, aCmdContext);
    }
    else
    {
        return PVMFFailure;
    }
                        );

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourcePosition() Out"));

    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::DoSourceNodeSetDataSourceDirection(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() In"));

    if (iChangePlaybackDirectionWhenResuming)
    {
        //Setting direction during engine Resume, due to a SetPlaybackRate that
        //occurred during engine Paused state.

        // Check if the source node has position control IF
        if (iSourceNodeDirCtrlIF == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() Direction control IF on source node not available"));

            // Since repositioning IF is not supported by this source node, assume the playback
            // will start from time 0
            iActualPlaybackPosition = 0;
            iActualMediaDataTS = 0;
            iAdjustedMediaDataTS = 0;
            // Then continue to handle like success case
            iStartNPT = 0;
            iStartMediaDataTS = 0;
            // Save the actual starting position for GetPlaybackRange() query
            iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
            iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

            // Repositioning so reset the EOS flag for each active datapath
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    iDatapathList[i].iEndOfDataReceived = false;
                }
            }

            return PVMFErrNotSupported;
        }

        // Set the position of the source node
        PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeSetDataSourceDirection);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() Calling SetDataSourcePosition() "));

        iActualMediaDataTS = 0;
        iAdjustedMediaDataTS = 0;
        iActualPlaybackPosition = 0;
        int32 leavecode = 0;
        PVMFCommandId cmdid = -1;
        OSCL_TRY(leavecode, cmdid = iSourceNodeDirCtrlIF->SetDataSourceDirection(iSourceNodeSessionId
                                    , (iPlaybackDirection_New < 0) ? PVMF_DATA_SOURCE_DIRECTION_REVERSE : PVMF_DATA_SOURCE_DIRECTION_FORWARD
                                    , iActualPlaybackPosition
                                    , iActualMediaDataTS
                                    , iOutsideTimebase
                                    , (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() SetDataSourceDirection on iSourceNodeDirCtrlIF did a leave!"));
                             if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
    {
        // Since this repositioning was not supported, assume the playback
        // will start from time 0
        iActualPlaybackPosition = 0;
        iActualMediaDataTS = 0;
        iAdjustedMediaDataTS = 0;
        // Then continue to handle like success case
        iStartNPT = 0;
        iStartMediaDataTS = 0;
        // Save the actual starting position for GetPlaybackRange() query
        iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
        iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

        // Repositioning so reset the EOS flag for each active datapath
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    iDatapathList[i].iEndOfDataReceived = false;
                }
            }

            return PVMFErrNotSupported;
        }
        else
        {
            return PVMFFailure;
        }
                            );

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() Out"));

        return PVMFSuccess;
    }
    else
    {
        //changing direction during SetPlaybackRate command

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() In"));

        // Check if the source node has direction control IF
        if (iSourceNodeDirCtrlIF == NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() No source direction control IF"));
            return PVMFFailure;
        }

        // Pause the playback clock
        bool clockpausedhere = iPlaybackClock.Pause();

        // Stop the playback position status timer
        StopPlaybackStatusTimer();

        // Set the new direction on the source node
        PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeSetDataSourceDirection);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() Calling SetDataSourceDirection() on source node."));

        iActualPlaybackPosition = 0;
        iActualMediaDataTS = 0;
        iAdjustedMediaDataTS = 0;
        int32 leavecode = 0;
        PVMFCommandId cmdid = -1;
        OSCL_TRY(leavecode, cmdid = iSourceNodeDirCtrlIF->SetDataSourceDirection(iSourceNodeSessionId
                                    , (iPlaybackDirection_New < 0) ? PVMF_DATA_SOURCE_DIRECTION_REVERSE : PVMF_DATA_SOURCE_DIRECTION_FORWARD
                                    , iActualPlaybackPosition
                                    , iActualMediaDataTS
                                    , iOutsideTimebase_New
                                    , (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() SetDataSourceDirection on iSourceNodeDirCtrlIF did a leave!"));
                             if (clockpausedhere)
    {
        // Resume the clock if paused in this function
        iPlaybackClock.Start();
            // To get regular play status events
            StartPlaybackStatusTimer();
        }

        if (leavecode == PVMFErrNotSupported || leavecode == PVMFErrArgument)
    {
        return leavecode;
    }
    else
    {
        return PVMFFailure;
    }
                        );

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeSetDataSourceDirection() Out"));

        return PVMFSuccess;
        //wait on node command complete and a call to HandleSourceNodeSetDataSourceDirection
    }
}

PVMFStatus PVPlayerEngine::DoSourceNodeStart(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSourceNodeStart() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeStart() In"));

    if (iSourceNode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeStart() Source node not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    // Start the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeStart);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->Start(iSourceNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeStart() Start on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeStart() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoDatapathStart(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoDatapathStart() for %s Tick=%d",
                     aDatapath.iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathStart() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        OSCL_ASSERT(false);
        return PVMFErrNotSupported;
    }

    // Start the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPStart);

    PVMFStatus retval = aDatapath.iDatapath->Start((OsclAny*)context);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathStart() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoSinkNodeSkipMediaData(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoSinkNodeSkipMediaData() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeSkipMediaData() In"));

    // Tell the sink nodes to skip the unneeded media data
    iNumPendingNodeCmd = 0;
    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;

    // Call SkipMediaData() for each active datapath with sink nodes that have the sync control IF
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true &&
                iDatapathList[i].iEndOfDataReceived == false &&
                iDatapathList[i].iSinkNodeSyncCtrlIF)
        {
            leavecode = 0;
            PVPlayerEngineContext* context = AllocateEngineContext(&(iDatapathList[i]), iDatapathList[i].iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeSkipMediaData);
            OSCL_TRY(leavecode, cmdid = iDatapathList[i].iSinkNodeSyncCtrlIF->SkipMediaData(iDatapathList[i].iSinkNodeSessionId, iActualMediaDataTS, iAdjustedMediaDataTS, iStreamID, iRenderSkipped, false, (OsclAny*) context));
            if (leavecode == 0)
            {
                ++iNumPendingNodeCmd;
                ++iNumPendingSkipCompleteEvent;
                ++iNumPVMFInfoStartOfDataPending;
            }
            else
            {
                FreeEngineContext(context);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeSkipMediaData() Out"));
    if (iNumPendingNodeCmd > 0)
    {
        return PVMFSuccess;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeSkipMediaData() Skip on sink nodes failed"));
        return PVMFFailure;
    }
}


PVMFStatus PVPlayerEngine::DoStart(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoStart() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStart() In, State=%d", iState));

    if (GetPVPlayerState() != PVP_STATE_PREPARED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoStart() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    if (iPlaybackClock.GetState() != OsclClock::RUNNING)
    {
        // Enable the end time check if specified
        UpdateCurrentEndPosition(iCurrentEndPosition);

        // Start the end time check if enabled
        if (iEndTimeCheckEnabled)
        {
            // Determine the check cycle based on interval setting in milliseconds
            // and timer frequency of 100 millisec
            int32 checkcycle = iEndTimeCheckInterval / 100;
            if (checkcycle == 0)
            {
                ++checkcycle;
            }
            iPollingCheckTimer->Request(PVPLAYERENGINE_TIMERID_ENDTIMECHECK, 0, checkcycle, this, true);
        }

        // Start the playback clock
        iPlaybackClock.Start();

        // Notify data sinks that clock has started
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive)
            {
                if (iDatapathList[i].iSinkNodeSyncCtrlIF)
                {
                    iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStarted();
                }
            }
        }
    }

    // To get regular play status events
    StartPlaybackStatusTimer();

    SetEngineState(PVP_ENGINE_STATE_STARTED);
    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStart() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoPause(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_NOTICE,
                    (0, "PVPlayerEngine::DoPause() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoPause() In"));

    // Check engine state
    if (GetPVPlayerState() != PVP_STATE_STARTED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoPause() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    // Send position update to app
    TimeoutOccurred(PVPLAYERENGINE_TIMERID_PLAY_STATUS, -1);
    // Stop the playback position status timer
    StopPlaybackStatusTimer();

    // Stop the end time check timer
    iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);

    // Stop the watchdog timer if active. We will Start the timer again in resume.
    // this should only be done when engine is waiting for StartofData info event
    // after reposition.
    if (iNumPVMFInfoStartOfDataPending > 0)
    {
        if (iWatchDogTimer->IsBusy())
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                            (0, "PVPlayerEngine::DoPause - Pause after setplayback, Cancelling Watchdog timer, iNumPVMFInfoStartOfDataPending=%d", iNumPVMFInfoStartOfDataPending));
            iWatchDogTimer->Cancel();
        }
    }


    // Pause the clock and notify sinks if not auto-paused
    uint32 i;
    if (iState != PVP_ENGINE_STATE_AUTO_PAUSED)
    {
        // Pause the playback clock
        iPlaybackClock.Pause();
        // Notify data sinks that clock has paused
        for (i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
            {
                iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStopped();
            }
        }
    }

    PVMFStatus retval = PVMFErrNotSupported;

    // Issue pause to all active datapaths
    iNumPendingDatapathCmd = 0;

    for (i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive)
        {
            if (iState == PVP_ENGINE_STATE_AUTO_PAUSED)
            {
                // Since sinks are already paused in auto-pause state, skip pausing the sink in the datapath
                retval = DoDatapathPause(iDatapathList[i], aCmd.GetCmdId(), aCmd.GetContext(), true);
            }
            else
            {
                // Pause all nodes in the datapath
                retval = DoDatapathPause(iDatapathList[i], aCmd.GetCmdId(), aCmd.GetContext(), false);
            }

            if (retval == PVMFSuccess)
            {
                ++iNumPendingDatapathCmd;
            }
            else
            {
                return retval;
            }
        }
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // If there are no active datapaths, continue on to pause the source node
        retval = DoSourceNodePause(aCmd.GetCmdId(), aCmd.GetContext());
    }

    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoPause() Pausing datapath and source node failed"));
        return retval;
    }

    // Reset the flag when doing a pause
    iChangePlaybackPositionWhenResuming = false;
    iChangePlaybackDirectionWhenResuming = false;

    SetEngineState(PVP_ENGINE_STATE_PAUSING);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoPause() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoDatapathPause(PVPlayerEngineDatapath& aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext, bool aSinkPaused)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::DoDatapathPause() for %s Tick=%d",
                     aDatapath.iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathPause() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        // Only active datapaths should be paused
        OSCL_ASSERT(false);
        return PVMFErrNotSupported;
    }

    // Pause the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, -1);

    PVMFStatus retval = aDatapath.iDatapath->Pause((OsclAny*)context, aSinkPaused);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathPause() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoSourceNodePause(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodePause() In"));

    if (iSourceNode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodePause() Source node not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    // Pause the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, -1);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->Pause(iSourceNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodePause() Pause on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodePause() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoResume(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResume() In"));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResume() iNumPendingSkipCompleteEvent: %d", iNumPendingSkipCompleteEvent));



    // Check engine state
    if (GetPVPlayerState() != PVP_STATE_PAUSED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoResume() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    // Disallow resume when paused due to EOS and position/direction
    // hasn't been changed
    if (iPlaybackPausedDueToEndOfClip)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoResume() Currently paused due to EOS so not allowed!"));
        return PVMFErrInvalidState;
    }

    PVMFStatus retval;
    if (iChangePlaybackPositionWhenResuming)
    {
        // Reposition occurred during the paused state so need to change the source position first
        retval = DoSourceNodeQueryDataSourcePosition(aCmd.GetCmdId(), aCmd.GetContext());
        // ignore failure, continue on to Start
        if (retval != PVMFSuccess)
        {
            retval = DoSourceNodeStart(aCmd.GetCmdId(), aCmd.GetContext());
        }
    }
    else if (iChangePlaybackDirectionWhenResuming)
    {
        // Direction change occurred during the paused state so need to change the source direction first
        retval = DoSourceNodeSetDataSourceDirection(aCmd.GetCmdId(), aCmd.GetContext());
        // ignore failure, continue on to Start
        if (retval != PVMFSuccess)
        {
            retval = DoSourceNodeStart(aCmd.GetCmdId(), aCmd.GetContext());
        }
    }
    else
    {
        retval = DoSourceNodeStart(aCmd.GetCmdId(), aCmd.GetContext());
    }

    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoResume() Resuming source node or changing position failed"));
        return retval;
    }

    SetEngineState(PVP_ENGINE_STATE_RESUMING);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResume() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::AddToMetadataInterfaceList(PVMFMetadataExtensionInterface* aMetadataIF, PVMFSessionId aSessionId)
{
    // Validate the interface ptr
    if (aMetadataIF == NULL)
    {
        return PVMFErrArgument;
    }

    // Add the specified interface ptr and session ID to the list
    PVPlayerEngineMetadataIFInfo mdifinfo;
    mdifinfo.iInterface = aMetadataIF;
    mdifinfo.iSessionId = aSessionId;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, iMetadataIFList.push_back(mdifinfo));
    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);

    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::RemoveFromMetadataInterfaceList(PVMFMetadataExtensionInterface* aMetadataIF, PVMFSessionId aSessionId)
{
    // Validate the interface ptr
    if (aMetadataIF == NULL)
    {
        return PVMFErrArgument;
    }

    // Go through the list to find the specified entry
    for (uint32 i = 0; i < iMetadataIFList.size(); ++i)
    {
        if (aMetadataIF == iMetadataIFList[i].iInterface &&
                aSessionId == iMetadataIFList[i].iSessionId)
        {
            // Found it. Now erase it from the list
            iMetadataIFList.erase(iMetadataIFList.begin() + i);
            return PVMFSuccess;
        }
    }

    // If here that means the specified entry wasn't found in the list
    return PVMFErrArgument;
}


PVMFStatus PVPlayerEngine::DoStop(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStop() In"));


    if (GetPVPlayerState() != PVP_STATE_PREPARED &&
            GetPVPlayerState() != PVP_STATE_STARTED &&
            GetPVPlayerState() != PVP_STATE_PAUSED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoStop() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    // reset the stream id and repos related variables
    iStreamID = 0;
    iNumPVMFInfoStartOfDataPending = 0;
    iNumPendingSkipCompleteEvent = 0;
    if (iWatchDogTimer->IsBusy())
    {
        iWatchDogTimer->Cancel();
    }

    // Stop the playback position status timer
    StopPlaybackStatusTimer();

    // Stop the playback clock
    iPlaybackClock.Stop();
    uint32 starttime = 0;
    iPlaybackClock.SetStartTime32(starttime, OSCLCLOCK_MSEC);
    iPlaybackDirection = 1;

    // Reset the begin/end time variables
    iCurrentBeginPosition.iIndeterminate = true;
    iCurrentEndPosition.iIndeterminate = true;
    iQueuedBeginPosition.iIndeterminate = true;
    iQueuedEndPosition.iIndeterminate = true;

    // Reset the paused-due-to-EOS flag
    iPlaybackPausedDueToEndOfClip = false;

    // Stop the end time check
    if (iEndTimeCheckEnabled)
    {
        iEndTimeCheckEnabled = false;
        iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
    }

    PVMFStatus retval = PVMFErrNotSupported;

    // Start the stopping sequence
    // First stop all the active datapaths
    iNumPendingDatapathCmd = 0;
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true)
        {
            PVMFStatus retcode = DoDatapathStop(iDatapathList[i], aCmd.GetCmdId(), aCmd.GetContext());
            if (retcode == PVMFSuccess)
            {
                ++iNumPendingDatapathCmd;
                retval = PVMFSuccess;
            }
            else
            {
                retval = retcode;
                break;
            }
        }
    }

    if (iNumPendingDatapathCmd == 0 && retval == PVMFErrNotSupported)
    {
        // If there are no active datapath, stop the source node
        retval = DoSourceNodeStop(aCmd.GetCmdId(), aCmd.GetContext());
    }

    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoStop() Stopping datapath and source node failed"));
        return retval;
    }

    SetEngineState(PVP_ENGINE_STATE_STOPPING);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStop() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoDatapathStop(PVPlayerEngineDatapath& aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathStop() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Stop the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPStop);

    PVMFStatus retval = aDatapath.iDatapath->Stop((OsclAny*)context);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathStop() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoSourceNodeStop(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeStop() In"));

    if (iSourceNode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeStop() Source node not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    // Stop the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, -1);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->Stop(iSourceNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeStop() Stop on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeStop() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoDatapathTeardown(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathTeardown() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Teardown the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);

    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPTeardown);

    PVMFStatus retval = aDatapath.iDatapath->Teardown((OsclAny*)context);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathTeardown() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoDatapathReset(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathReset() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Reset the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);

    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPReset);

    PVMFStatus retval = aDatapath.iDatapath->Reset((OsclAny*)context);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathReset() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoRemoveDataSink(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveDataSink() In"));

    // previously removed, e.g. during error handling ?
    if (iDatapathList.empty() && GetPVPlayerState() == PVP_STATE_IDLE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveDataSink() All sinks were previously deleted"));
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
        return PVMFSuccess;
    }

    if (GetPVPlayerState() != PVP_STATE_INITIALIZED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoRemoveDataSink() Wrong engine state"));
        return PVMFErrInvalidState;
    }

    PVPlayerDataSink* datasink = (PVPlayerDataSink*)(aCmd.GetParam(0).pOsclAny_value);

    if (datasink == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoRemoveDataSink() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Find the track that the passed-in sink belongs to
    PVPlayerEngineDatapath* pvpedp = NULL;
    int32 dpindex = -1;
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iDataSink == datasink)
        {
            pvpedp = &(iDatapathList[i]);
            dpindex = i;
            break;
        }
    }

    if (pvpedp == NULL || dpindex == -1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoRemoveDataSink() Passed in data sink does not match with ones in engine"));
        return PVMFFailure;
    }
    else
    {
        // Cleanup and remove the datapath associated with the data sink from the list
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoRemoveDataSink() Removing datapath"));
        DoEngineDatapathCleanup(*pvpedp);

        iDatapathList.erase(iDatapathList.begin() + dpindex);
    }

    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveDataSink() Out"));
    return PVMFSuccess;
}

void PVPlayerEngine::DoRemoveAllSinks()
{
    // Clean up the datapaths
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        DoEngineDatapathCleanup(iDatapathList[i]);
    }
    iDatapathList.clear();
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoRemoveAllSinks() all datapaths removed"));
}


PVMFStatus PVPlayerEngine::DoReset(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoReset() In"));

    // reset the stream id and repos related variables
    iStreamID = 0;
    iNumPVMFInfoStartOfDataPending = 0;
    iNumPendingSkipCompleteEvent = 0;
    if (iWatchDogTimer->IsBusy())
    {
        iWatchDogTimer->Cancel();
    }

    int32 leavecode = 0;
    PVMFStatus status = PVMFSuccess;
    switch (GetPVPlayerState())
    {
        case PVP_STATE_IDLE: // already in final state
            DoRemoveAllSinks();

            if (iDataSource)
            {
                RemoveDataSourceSync(*iDataSource);
            }
            EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
            break;
        case PVP_STATE_INITIALIZED:
            DoRemoveAllSinks();
            if (iSourceNode)
            {
                PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmd.GetCmdId(), aCmd.GetContext(), -1);

                PVMFCommandId cmdid = -1;
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iSourceNode->Reset(iSourceNodeSessionId, (OsclAny*)context));
                OSCL_FIRST_CATCH_ANY(leavecode,

                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoReset() Reset on iSourceNode did a leave!"));
                                     FreeEngineContext(context);
                                     return PVMFFailure);

                SetEngineState(PVP_ENGINE_STATE_RESETTING);
            }
            else
            {
                SetEngineState(PVP_ENGINE_STATE_IDLE);
                EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
            }
            break;
        case PVP_STATE_PREPARED:
        case PVP_STATE_STARTED:
        case PVP_STATE_PAUSED:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoReset() called in wrong engine state!"));
            status = PVMFFailure;
            break;
        case PVP_STATE_ERROR:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoReset() called during error state!"));
            status = PVMFFailure;
            break;
        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoReset() unknown engine state!"));
            status = PVMFFailure;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoReset() Out"));
    return status;
}

PVMFStatus PVPlayerEngine::RemoveDataSourceSync(PVPlayerDataSource &aSrc)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDataSourceSync() In"));

    if (GetPVPlayerState() != PVP_STATE_IDLE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::RemoveDataSourceSync() Wrong engine state"));
        // force cleanup
        DoSourceNodeCleanup();
        iDataSource = NULL;
        return PVMFErrInvalidState;
    }

    // Clean up the trackinfo now because iSourceNode might have memfrags in them
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        // Destroy the track info if present
        if (iDatapathList[i].iTrackInfo)
        {
            OSCL_DELETE(iDatapathList[i].iTrackInfo);
            iDatapathList[i].iTrackInfo = NULL;
        }
        if (iDatapathList[i].iDatapath)
            iDatapathList[i].iDatapath->SetSourceNode(NULL);
    }

    // Destroy the source node if present
    if (iSourceNode)
    {
        // Remove reference to the parser node init interface if available
        if (iSourceNodeInitIF)
        {
            iSourceNodeInitIF->removeRef();
            iSourceNodeInitIF = NULL;
        }

        // Remove reference to the parser node track sel interface if available
        if (iSourceNodeTrackSelIF)
        {
            iPlayableList.Reset();
            iPreferenceList.Reset();
            iSourceNodeTrackSelIF->removeRef();
            iSourceNodeTrackSelIF = NULL;
            iTrackSelectionHelper = NULL;
        }

        // Remove reference to the parser node track level info interface if available
        if (iSourceNodeTrackLevelInfoIF)
        {
            iSourceNodeTrackLevelInfoIF->removeRef();
            iSourceNodeTrackLevelInfoIF = NULL;
        }

        // Remove reference to the parser node position control interface if available
        if (iSourceNodePBCtrlIF)
        {
            iSourceNodePBCtrlIF->removeRef();
            iSourceNodePBCtrlIF = NULL;
        }

        // Remove reference to the parser node direction control interface if available
        if (iSourceNodeDirCtrlIF)
        {
            iSourceNodeDirCtrlIF->removeRef();
            iSourceNodeDirCtrlIF = NULL;
        }

        // Remove reference to the parser node metadata interface if available
        if (iSourceNodeMetadataExtIF)
        {
            RemoveFromMetadataInterfaceList(iSourceNodeMetadataExtIF, iSourceNodeSessionId);
            iSourceNodeMetadataExtIF->removeRef();
            iSourceNodeMetadataExtIF = NULL;
        }

        // Reset the duration value retrieved from source
        iSourceDurationAvailable = false;
        iSourceDurationInMS = 0;

        // Remove reference to the parser node cap-config interface if available
        if (iSourceNodeCapConfigIF)
        {
            iSourceNodeCapConfigIF->removeRef();
            iSourceNodeCapConfigIF = NULL;
        }

        if (iSourceNodeCPMLicenseIF)
        {
            iSourceNodeCPMLicenseIF->removeRef();
            iSourceNodeCPMLicenseIF = NULL;
        }
        iSourceNode->Disconnect(iSourceNodeSessionId);
        iSourceNode->ThreadLogoff();

        // search for matching uuid entry in list of instantiated nodes
        PVPlayerEngineUuidNodeMapping* iter = iNodeUuids.begin();
        for (; iter != iNodeUuids.end(); ++iter)
            if (iter->iNode == iSourceNode)
                break;

        if (iter != iNodeUuids.end())
        {
            bool release_status = false;

            int32 leavecode = 0;
            OSCL_TRY(leavecode, release_status = iPlayerNodeRegistry.ReleaseNode(iter->iUuid, iSourceNode));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 // force cleanup
                                 DoSourceNodeCleanup();
                                 iDataSource = NULL;
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::RemoveDataSourceSync() Error in releasing SourceNode"));
                                 return PVMFFailure;);

            if (release_status == false)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::RemoveDataSourceSync() Factory returned false while releasing the sourcenode"));
                // force cleanup
                DoSourceNodeCleanup();
                iDataSource = NULL;
                return PVMFFailure;
            }

            iNodeUuids.erase(iter);
            iSourceNode = NULL;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::RemoveDataSourceSync() unknown node"));
            // force cleanup
            DoSourceNodeCleanup();
            iDataSource = NULL;
            return PVMFFailure;
        }
    }

    // Remove Stored KVP Values
    DeleteKVPValues();

    // Remove all metadata IF from the list
    iMetadataIFList.clear();

    iDataSource = NULL;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::RemoveDataSourceSync() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoRemoveDataSource(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveDataSource() In"));

    if (GetPVPlayerState() != PVP_STATE_IDLE)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveDataSource() called when engine is not in the IDLE state"));
        return PVMFFailure;
    }

    if (iDataSource == NULL) // previously removed, e.g. during errorhandling
    {
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
        return PVMFSuccess;
    }

    PVPlayerDataSource* src = (PVPlayerDataSource*)(aCmd.GetParam(0).pOsclAny_value);

    if (iDataSource != src || src == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoRemoveDataSource() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    PVMFStatus result = RemoveDataSourceSync(*src);

    if (result == PVMFSuccess)
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoRemoveDataSource() Out"));
    return result;
}


PVMFStatus PVPlayerEngine::DoSourceUnderflowAutoPause(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceUnderflowAutoPause() In"));

    // Allow auto-pause only when playing
    if (iState != PVP_ENGINE_STATE_STARTED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceUnderflowAutoPause() Invalid state so cancel auto-pause request!"));
        return PVMFErrCancelled;
    }

    // Pause the playback clock
    iPlaybackClock.Pause();
    // Stop the playback position status timer
    StopPlaybackStatusTimer();
    uint32 i;
    // Notify data sinks that clock has paused
    for (i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
        {
            iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStopped();
        }
    }

    PVMFStatus retval = PVMFErrNotSupported;

    // Pause all active sink nodes
    iNumPendingDatapathCmd = 0;
    for (i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true)
        {
            retval = DoSinkNodePause(iDatapathList[i], aCmd.GetCmdId(), aCmd.GetContext());
            if (retval == PVMFSuccess)
            {
                ++iNumPendingDatapathCmd;
            }
            else
            {
                break;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceUnderflowAutoPause() Out"));
    if (iNumPendingDatapathCmd == 0)
    {
        return PVMFErrNotSupported;
    }
    else
    {
        SetEngineState(PVP_ENGINE_STATE_AUTO_PAUSING);
        return retval;
    }
}


PVMFStatus PVPlayerEngine::DoSourceDataReadyAutoResume(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceDataReadyAutoResume() In"));

    //Don't need to worry about transitional states(...ING). Because auto-pause/resume cmds are just regular engine cmds and won't be interrupted by normal ones
    //Underflow->Pause->Resume->DataReady. Then we start clock in here. Because clock is not started at Resume due to underflow.
    //Resume starts playback clock. Souce node pauses it again. Engine stays in STARTED state rather than AUTO_PAUSED state.
    //Underflow->Pause->DataReady->Resume. Then we CANNOT start clock in here. Because clock is paused by app.
    //After Pause done, the engine is in PAUSED state.
    if (iPlaybackClock.GetState() == OsclClock::PAUSED && iState == PVP_ENGINE_STATE_STARTED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Clock Start"));

        iPlaybackClock.Start();
        // To get regular play status events
        StartPlaybackStatusTimer();

        // Notify all sink nodes that have sync control IF that clock has started.
        // This is necessary because engine cannot assume all data sinks implement
        // the clock state observer.
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
            {
                iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStarted();
            }
        }
        //instead of return PVMFSuccess because PVMFErrNotSupported will cause the DataReady event be sent to app
        return PVMFErrNotSupported;
    }

    // Allow auto-resume only when auto-paused
    if (iState != PVP_ENGINE_STATE_AUTO_PAUSED)
    {
        //if in these two states, the engine has NOT started yet. It is very likely the app is waiting for DataReady event to call engine Start()
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceDataReadyAutoResume() Invalid state %d", iState));
        //return PVMFErrNotSupported so the the DataReady can be sent, depending on iDataReadySent flag.
        return PVMFErrNotSupported;
    }

    PVMFStatus retval = PVMFErrNotSupported;

    // Resume all active sink nodes
    iNumPendingDatapathCmd = 0;
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true)
        {
            retval = DoSinkNodeResume(iDatapathList[i], aCmd.GetCmdId(), aCmd.GetContext());
            if (retval == PVMFSuccess)
            {
                ++iNumPendingDatapathCmd;
            }
            else
            {
                break;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceDataReadyAutoResume() Out"));
    if (iNumPendingDatapathCmd == 0)
    {
        return PVMFErrNotSupported;
    }
    else
    {
        SetEngineState(PVP_ENGINE_STATE_AUTO_RESUMING);
        return retval;
    }
}


PVMFStatus PVPlayerEngine::DoSinkNodePause(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodePause() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Pause the sink node
    OSCL_ASSERT(aDatapath.iSinkNode != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeAutoPause);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->Pause(aDatapath.iSinkNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodePause() Pause on sink node did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodePause() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSinkNodeResume(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeResume() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Start the sink node to resume
    OSCL_ASSERT(aDatapath.iSinkNode != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, aDatapath.iSinkNode, NULL, aCmdId, aCmdContext, PVP_CMD_SinkNodeAutoResume);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = aDatapath.iSinkNode->Start(aDatapath.iSinkNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSinkNodeResume() Start on sink node did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeResume() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoCancelDueToError(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelDueToError() In: iCurrentContextList.size() %d",
                    iCurrentContextList.size()));

    int32 leavecode = 0;
    if (iCurrentContextList.empty() == false)
    {
        // Issue cancel commands
        // Determine where the pending commands were issued to and then cancel them
        iPendingCancelDueToErrorRequest = 0;
        for (uint32 i = 0; i < iCurrentContextList.size(); ++i)
        {
            if (iCurrentContextList[i]->iNode)
            {
                if (iCurrentContextList[i]->iNode == iSourceNode)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelDueToError() Calling CancelAllCommands() on source node"));
                    leavecode = 0;
                    OSCL_TRY(leavecode, iSourceNode->CancelAllCommands(iSourceNodeSessionId, (OsclAny*)&iPendingCancelDueToErrorRequest));
                    if (leavecode == 0)
                    {
                        ++iPendingCancelDueToErrorRequest;
                    }
                }
                else if (iCurrentContextList[i]->iEngineDatapath != NULL)
                {
                    if (iCurrentContextList[i]->iNode == iCurrentContextList[i]->iEngineDatapath->iSinkNode)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelDueToError() Calling CancelAllCommands() on sink node"));
                        leavecode = 0;
                        OSCL_TRY(leavecode, iCurrentContextList[i]->iNode->CancelAllCommands(iCurrentContextList[i]->iEngineDatapath->iSinkNodeSessionId, (OsclAny*)&iPendingCancelDueToErrorRequest));
                        if (leavecode == 0)
                        {
                            ++iPendingCancelDueToErrorRequest;
                        }
                    }
                    else if (iCurrentContextList[i]->iNode == iCurrentContextList[i]->iEngineDatapath->iDecNode)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelDueToError() Calling CancelAllCommands() on dec node"));
                        leavecode = 0;
                        OSCL_TRY(leavecode, iCurrentContextList[i]->iNode->CancelAllCommands(iCurrentContextList[i]->iEngineDatapath->iDecNodeSessionId, (OsclAny*)&iPendingCancelDueToErrorRequest));
                        if (leavecode == 0)
                        {
                            ++iPendingCancelDueToErrorRequest;
                        }
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() Unknown node type. Asserting"));
                        OSCL_ASSERT(false);
                    }
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() Unknown node. Asserting"));
                    OSCL_ASSERT(false);
                }
            }
            else if (iCurrentContextList[i]->iDatapath != NULL)
            {
                if (iCurrentContextList[i]->iEngineDatapath != NULL)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelDueToError() Calling CancelCommand() on datapath"));
                    leavecode = 0;
                    OSCL_TRY(leavecode, iCurrentContextList[i]->iDatapath->CancelCommand((OsclAny*)&iPendingCancelDueToErrorRequest));
                    if (leavecode == 0)
                    {
                        ++iPendingCancelDueToErrorRequest;
                    }
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() Unknown datapath. Asserting"));
                    OSCL_ASSERT(false);
                }
            }
            else
            {
                // Either a node or datapath should be pending
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() No pending node or datapath. Asserting"));
                OSCL_ASSERT(false);
            }
        }

        // Reset the pending node/datapath cmd counters
        iNumPendingNodeCmd = 0;
        iNumPendingDatapathCmd = 0;

        if (iPendingCancelDueToErrorRequest > 0)
        {
            // Deactivate the engine AO if active
            // to avoid processing the next engine command in queue
            if (IsBusy())
            {
                Cancel();
            }

            // Put in an internal engine command in the current command queue
            // to prevent the next command in queue from being processed
            PVPlayerEngineCommand cmd(PVP_ENGINE_COMMAND_CANCEL_DUE_TO_ERROR, -1, NULL, NULL, false);

            leavecode = 0;
            OSCL_ASSERT(iCurrentCmd.empty() == true);
            OSCL_TRY(leavecode, iCurrentCmd.push_front(cmd));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() Cancel-due-to-error command could not be pushed onto iCurrentCmd vector. Asserting"));
                                 OSCL_ASSERT(false);
                                 return PVMFErrNoMemory;);

            // Put the same command in the cancel command queue
            leavecode = 0;
            OSCL_ASSERT(iCmdToCancel.empty() == true);
            OSCL_TRY(leavecode, iCmdToCancel.push_front(cmd));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() Cancel-due-to-error command could not be pushed onto iCmdToCancel vector. Asserting"));
                                 OSCL_ASSERT(false);
                                 return PVMFErrNoMemory;);
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCancelDueToError() % pending cancels.", iPendingCancelDueToErrorRequest));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCancelDueToError() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoStopDueToError(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToError() In"));

    SetEngineState(PVP_ENGINE_STATE_HANDLINGERROR);
    iErrorOccurredDuringErrorHandling = false;

    // Stop the playback position status timer
    StopPlaybackStatusTimer();

    // Stop the playback clock
    iPlaybackClock.Stop();
    uint32 starttime = 0;
    iPlaybackClock.SetStartTime32(starttime, OSCLCLOCK_MSEC);

    // Reset the begin/end time variables
    iCurrentBeginPosition.iIndeterminate = true;
    iCurrentEndPosition.iIndeterminate = true;
    iQueuedBeginPosition.iIndeterminate = true;
    iQueuedEndPosition.iIndeterminate = true;

    // Reset the paused-due-to-EOS flag
    iPlaybackPausedDueToEndOfClip = false;

    // Stop the end time check
    if (iEndTimeCheckEnabled)
    {
        iEndTimeCheckEnabled = false;
        iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
    }

    PVMFStatus retval = PVMFFailure;

    // Start the stopping sequence
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true)
        {
            PVMFStatus retcode = DoDatapathStopDueToError(iDatapathList[i], aCmd.GetCmdId(), aCmd.GetContext());
            if (retcode == PVMFSuccess)
            {
                ++iNumPendingDatapathCmd;
            }
        }
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // No datapath to stop so stop the source node
        retval = DoSourceNodeStopDueToError(aCmd.GetCmdId(), aCmd.GetContext());

        if (retval != PVMFSuccess)
        {
            // If stop could not be initiated, complete it and issue a reset command
            EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFFailure);
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoStopDueToError() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoDatapathStopDueToError(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathStopDueToError() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Stop the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPStop);

    PVMFStatus retval = aDatapath.iDatapath->Stop((OsclAny*)context, true);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathStopDueToError() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoSourceNodeStopDueToError(PVCommandId aCmdId, OsclAny* aCmdContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeStopDueToError() In"));

    if (iSourceNode == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeStopDueToError() Source node not available. Asserting"));
        OSCL_ASSERT(false);
        return PVMFFailure;
    }

    // Stop the source node
    PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmdId, aCmdContext, PVP_CMD_SourceNodeStop);

    PVMFCommandId cmdid = -1;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, cmdid = iSourceNode->Stop(iSourceNodeSessionId, (OsclAny*)context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         FreeEngineContext(context);
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeStopDueToError() Stop on iSourceNode did a leave!"));
                         return PVMFFailure);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeStopDueToError() Out"));

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoDatapathTeardownDueToError(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathTeardownDueToError() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Teardown the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPTeardown);

    PVMFStatus retval = aDatapath.iDatapath->Teardown((OsclAny*)context, true);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathTeardownDueToError() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoDatapathResetDueToError(PVPlayerEngineDatapath &aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext)
{
    OSCL_ASSERT(aDatapath.iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathResetDueToError() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapath.iTrackActive == false)
    {
        return PVMFErrNotSupported;
    }

    // Reset the datapath
    OSCL_ASSERT(aDatapath.iDatapath != NULL);
    PVPlayerEngineContext* context = AllocateEngineContext(&aDatapath, NULL, aDatapath.iDatapath, aCmdId, aCmdContext, PVP_CMD_DPReset);

    PVMFStatus retval = aDatapath.iDatapath->Reset((OsclAny*)context, true);
    if (retval != PVMFSuccess)
    {
        FreeEngineContext(context);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathResetDueToError() Out"));
    return retval;
}


PVMFStatus PVPlayerEngine::DoResetDueToError(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToError() In"));

    SetEngineState(PVP_ENGINE_STATE_HANDLINGERROR);
    iErrorOccurredDuringErrorHandling = false;

    int32 leavecode = 0;
    PVMFCommandId cmdid = -1;


    if (iSourceNode)
    {
        // Remove reference to the parser node init interface if available
        if (iSourceNodeInitIF)
        {
            iSourceNodeInitIF->removeRef();
            iSourceNodeInitIF = NULL;
        }

        // Remove reference to the parser node track sel interface if available
        if (iSourceNodeTrackSelIF)
        {
            iPlayableList.Reset();
            iPreferenceList.Reset();
            iSourceNodeTrackSelIF->removeRef();
            iSourceNodeTrackSelIF = NULL;
            iTrackSelectionHelper = NULL;
        }

        // Remove reference to the parser node track level info interface if available
        if (iSourceNodeTrackLevelInfoIF)
        {
            iSourceNodeTrackLevelInfoIF->removeRef();
            iSourceNodeTrackLevelInfoIF = NULL;
        }

        // Remove reference to the parser node position control interface if available
        if (iSourceNodePBCtrlIF)
        {
            iSourceNodePBCtrlIF->removeRef();
            iSourceNodePBCtrlIF = NULL;
        }

        // Remove reference to the parser node direction control interface if available
        if (iSourceNodeDirCtrlIF)
        {
            iSourceNodeDirCtrlIF->removeRef();
            iSourceNodeDirCtrlIF = NULL;
        }

        // Remove reference to the parser node metadata interface if available
        if (iSourceNodeMetadataExtIF)
        {
            RemoveFromMetadataInterfaceList(iSourceNodeMetadataExtIF, iSourceNodeSessionId);
            iSourceNodeMetadataExtIF->removeRef();
            iSourceNodeMetadataExtIF = NULL;
        }

        // Reset the duration value retrieved from source
        iSourceDurationAvailable = false;
        iSourceDurationInMS = 0;

        // Remove reference to the parser node cap-config interface if available
        if (iSourceNodeCapConfigIF)
        {
            iSourceNodeCapConfigIF->removeRef();
            iSourceNodeCapConfigIF = NULL;
        }

        if (iSourceNodeCPMLicenseIF)
        {
            iSourceNodeCPMLicenseIF->removeRef();
            iSourceNodeCPMLicenseIF = NULL;
        }

        // Reset the source node
        PVPlayerEngineContext* context = AllocateEngineContext(NULL, iSourceNode, NULL, aCmd.GetCmdId(), aCmd.GetContext(), PVP_CMD_SourceNodeReset);
        cmdid = -1;
        leavecode = 0;
        OSCL_TRY(leavecode, cmdid = iSourceNode->Reset(iSourceNodeSessionId, (OsclAny*)context));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             FreeEngineContext(context);
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoResetDueToError() Reset on iSourceNode did a leave!"));
                            );

        if (leavecode != 0)
        {
            FreeEngineContext(context);
            // If reset could not be initiated, complete it and issue a cleanup command
            EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFFailure);
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR, NULL, NULL, NULL, false);
        }


    }
    else
    {
        // Ready to be in idle state so complete the reset command
        SetEngineState(PVP_ENGINE_STATE_IDLE);
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoResetDueToError() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoCleanupDueToError(PVPlayerEngineCommand& aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCleanupDueToError() In"));

    SetEngineState(PVP_ENGINE_STATE_HANDLINGERROR);
    iErrorOccurredDuringErrorHandling = false;

    // Wipe everything out
    iPollingCheckTimer->Clear();

    // Clean up the datapaths
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        DoEngineDatapathCleanup(iDatapathList[i]);
    }
    iDatapathList.clear();

    // Clean up the source node
    DoSourceNodeCleanup();

    // Return all the engine context
    while (!iCurrentContextList.empty())
    {
        OSCL_ASSERT(iCurrentContextList[0] != aCmd.GetContext());
        FreeEngineContext(iCurrentContextList[0]);
    }

    iMetadataIFList.clear();

    SetEngineState(PVP_ENGINE_STATE_IDLE);
    EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCleanupDueToError() Out"));
    return PVMFSuccess;
}


void PVPlayerEngine::DoEngineDatapathTeardown(PVPlayerEngineDatapath& aDatapath)
{
    if (aDatapath.iTrackInfo)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoEngineDatapathTeardown() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoEngineDatapathTeardown() In"));
    }

    if (aDatapath.iDatapath)
    {
        // Shutdown and reset the datapath
        aDatapath.iDatapath->DisconnectNodeSession();
        aDatapath.iDatapath->SetSinkNode(NULL);
        aDatapath.iDatapath->SetDecNode(NULL);
        aDatapath.iDatapath->SetSourceNode(NULL);
    }

    if (aDatapath.iSinkNode)
    {
        aDatapath.iSinkNode->Disconnect(aDatapath.iSinkNodeSessionId);
        aDatapath.iSinkNode->ThreadLogoff();

        // Remove sync ctrl IF if available
        if (aDatapath.iSinkNodeSyncCtrlIF)
        {
            aDatapath.iSinkNodeSyncCtrlIF->SetClock(NULL);
            aDatapath.iSinkNodeSyncCtrlIF->removeRef();
            aDatapath.iSinkNodeSyncCtrlIF = NULL;
        }

        // Remove metadata IF if available
        if (aDatapath.iSinkNodeMetadataExtIF)
        {
            RemoveFromMetadataInterfaceList(aDatapath.iSinkNodeMetadataExtIF, aDatapath.iSinkNodeSessionId);
            aDatapath.iSinkNodeMetadataExtIF->removeRef();
            aDatapath.iSinkNodeMetadataExtIF = NULL;
        }

        // Remove cap-config IF if available
        if (aDatapath.iSinkNodeCapConfigIF)
        {
            aDatapath.iSinkNodeCapConfigIF = NULL;
        }

        if (aDatapath.iDataSink)
        {
            if (aDatapath.iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_FILENAME)
            {
                // Remove file output config IF if available
                if (aDatapath.iSinkNodeFOConfigIF)
                {
                    aDatapath.iSinkNodeFOConfigIF->removeRef();
                    aDatapath.iSinkNodeFOConfigIF = NULL;
                }
                // Delete the sink node since engine created it.
                PVFileOutputNodeFactory::DeleteFileOutput(aDatapath.iSinkNode);
            }
        }
        aDatapath.iSinkNode = NULL;
    }

    if (aDatapath.iDecNode)
    {
        // Remove metadata IF if available
        if (aDatapath.iDecNodeMetadataExtIF)
        {
            RemoveFromMetadataInterfaceList(aDatapath.iDecNodeMetadataExtIF, aDatapath.iDecNodeSessionId);
            aDatapath.iDecNodeMetadataExtIF->removeRef();
            aDatapath.iDecNodeMetadataExtIF = NULL;
        }

        // Remove cap-config IF if available
        if (aDatapath.iDecNodeCapConfigIF)
        {
            aDatapath.iDecNodeCapConfigIF = NULL;
        }


        aDatapath.iDecNode->Disconnect(aDatapath.iDecNodeSessionId);
        aDatapath.iDecNode->ThreadLogoff();

        // search for matching uuid entry in list of instantiated nodes
        PVPlayerEngineUuidNodeMapping* iter = iNodeUuids.begin();
        for (; iter != iNodeUuids.end(); ++iter)
            if (iter->iNode == aDatapath.iDecNode)
                break;

        if (iter != iNodeUuids.end())
        {
            bool release_status = false;

            int32 leavecode = 0;
            OSCL_TRY(leavecode, release_status = iPlayerNodeRegistry.ReleaseNode(iter->iUuid, aDatapath.iDecNode));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoEngineDatapathTeardown() Error in releasing DecNode"));
                                 return;);

            if (release_status == false)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoEngineDatapathTeardown() Factory returned false while releasing the decnode"));
                return;
            }

            iNodeUuids.erase(iter);
            aDatapath.iDecNode = NULL;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoEngineDatapathTeardown() decnode not found"));
            return;
        }
    }

    aDatapath.iTrackActive = false;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoEngineDatapathTeardown() Out"));
}


void PVPlayerEngine::DoEngineDatapathCleanup(PVPlayerEngineDatapath& aDatapath)
{
    if (aDatapath.iTrackInfo)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoEngineDatapathCleanup() In %s", aDatapath.iTrackInfo->getTrackMimeType().get_cstr()));
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoEngineDatapathCleanup() In"));
    }

    DoEngineDatapathTeardown(aDatapath);

    // Destroy the datapath utility object instance
    if (aDatapath.iDatapath)
    {
        OSCL_DELETE(aDatapath.iDatapath);
        aDatapath.iDatapath = NULL;
    }

    // Destroy the track info
    if (aDatapath.iTrackInfo)
    {
        OSCL_DELETE(aDatapath.iTrackInfo);
        aDatapath.iTrackInfo = NULL;
    }

    aDatapath.iTrackActive = false;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoEngineDatapathCleanup() Out"));
}


void PVPlayerEngine::DoSourceNodeCleanup(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeCleanup() In"));

    if (iSourceNode)
    {
        // Remove reference to the parser node init interface if available
        if (iSourceNodeInitIF)
        {
            iSourceNodeInitIF->removeRef();
            iSourceNodeInitIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodeInitIF Released"));
        }

        // Remove reference to the parser node track sel interface if available
        if (iSourceNodeTrackSelIF)
        {
            iPlayableList.Reset();
            iPreferenceList.Reset();
            iSourceNodeTrackSelIF->removeRef();
            iSourceNodeTrackSelIF = NULL;
            iTrackSelectionHelper = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodeTrackSelIF Released"));
        }

        // Remove reference to the parser node track level info interface if available
        if (iSourceNodeTrackLevelInfoIF)
        {
            iSourceNodeTrackLevelInfoIF->removeRef();
            iSourceNodeTrackLevelInfoIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodeTrackLevelInfoIF Released"));
        }

        // Remove reference to the parser node position control interface if available
        if (iSourceNodePBCtrlIF)
        {
            iSourceNodePBCtrlIF->removeRef();
            iSourceNodePBCtrlIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodePBCtrlIF Released"));
        }

        // Remove reference to the parser node direction control interface if available
        if (iSourceNodeDirCtrlIF)
        {
            iSourceNodeDirCtrlIF->removeRef();
            iSourceNodeDirCtrlIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodeDirCtrlIF Released"));
        }

        // Remove reference to the parser node metadata interface if available
        if (iSourceNodeMetadataExtIF)
        {
            RemoveFromMetadataInterfaceList(iSourceNodeMetadataExtIF, iSourceNodeSessionId);
            iSourceNodeMetadataExtIF->removeRef();
            iSourceNodeMetadataExtIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodeMetadataExtIF Released"));
        }

        // Reset the duration value retrieved from source
        iSourceDurationAvailable = false;
        iSourceDurationInMS = 0;

        // Remove reference to the parser node cap-config interface if available
        if (iSourceNodeCapConfigIF)
        {
            iSourceNodeCapConfigIF->removeRef();
            iSourceNodeCapConfigIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNodeCapConfigIF Released"));
        }

        if (iSourceNodeCPMLicenseIF)
        {
            iSourceNodeCPMLicenseIF->removeRef();
            iSourceNodeCPMLicenseIF = NULL;
        }
        iSourceNode->Disconnect(iSourceNodeSessionId);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - DisConnect Done"));
        iSourceNode->ThreadLogoff();
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - ThreadLogoff Done"));

        // search for matching uuid entry in list of instantiated nodes
        PVPlayerEngineUuidNodeMapping* iter = iNodeUuids.begin();
        for (; iter != iNodeUuids.end(); ++iter)
            if (iter->iNode == iSourceNode)
                break;

        if (iter != iNodeUuids.end())
        {
            bool release_status = false;

            int32 leavecode = 0;
            OSCL_TRY(leavecode, release_status = iPlayerNodeRegistry.ReleaseNode(iter->iUuid, iSourceNode));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeCleanup() Error in releasing SourceNode"));
                                 return;);

            if (release_status == false)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSourceNodeCleanup() Factory returned false while releasing the sourcenode"));
                return;
            }


            iNodeUuids.erase(iter);
            iSourceNode = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNode Delete Done"));
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::DoSourceNodeCleanup() - iSourceNode not found"));
            return;
        }
    }

    // Cleanup the control varibles related to rate & direction changes.
    iPlaybackDirection_New = iPlaybackDirection;
    iOutsideTimebase_New = iOutsideTimebase;
    iPlaybackClockRate_New = iPlaybackClockRate;
    iChangeDirectionNPT.iIndeterminate = true;
    // Reset the flag that tracks pause-due-to-EOS
    iPlaybackPausedDueToEndOfClip = false;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSourceNodeCleanup() Out"));
}


PVMFStatus PVPlayerEngine::DoCapConfigGetParametersSync(PvmiKeyType aIdentifier, PvmiKvp*& aParameters, int& aNumParamElements, PvmiCapabilityContext aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() In"));
    OSCL_UNUSED_ARG(aContext);

    // Initialize the output parameters
    aNumParamElements = 0;
    aParameters = NULL;

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aIdentifier);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aIdentifier, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
    {
        // First component should be "x-pvmf" and there must
        // be at least two components to go past x-pvmf
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Invalid key string"));
        return PVMFErrArgument;
    }

    // Retrieve the second component from the key string
    pv_mime_string_extract_type(1, aIdentifier, compstr);

    // First check if it is key string for engine ("player")
    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("player")) >= 0)
    {
        // Key is for player
        if (compcount == 2)
        {
            // Since key is "x-pvmf/player" return all
            // nodes available at this level. Ignore attribute
            // since capability is only allowed

            // Allocate memory for the KVP list
            aParameters = (PvmiKvp*)oscl_malloc(PVPLAYERCONFIG_BASE_NUMKEYS * sizeof(PvmiKvp));
            if (aParameters == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Memory allocation for KVP failed"));
                return PVMFErrNoMemory;
            }
            oscl_memset(aParameters, 0, PVPLAYERCONFIG_BASE_NUMKEYS*sizeof(PvmiKvp));
            // Allocate memory for the key strings in each KVP
            PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVPLAYERCONFIG_BASE_NUMKEYS * PVPLAYERCONFIG_KEYSTRING_SIZE * sizeof(char));
            if (memblock == NULL)
            {
                oscl_free(aParameters);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Memory allocation for key string failed"));
                return PVMFErrNoMemory;
            }
            oscl_strset(memblock, 0, PVPLAYERCONFIG_BASE_NUMKEYS*PVPLAYERCONFIG_KEYSTRING_SIZE*sizeof(char));
            // Assign the key string buffer to each KVP
            int32 j;
            for (j = 0; j < PVPLAYERCONFIG_BASE_NUMKEYS; ++j)
            {
                aParameters[j].key = memblock + (j * PVPLAYERCONFIG_KEYSTRING_SIZE);
            }
            // Copy the requested info
            for (j = 0; j < PVPLAYERCONFIG_BASE_NUMKEYS; ++j)
            {
                oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/player/"), 14);
                oscl_strncat(aParameters[j].key, PVPlayerConfigBaseKeys[j].iString, oscl_strlen(PVPlayerConfigBaseKeys[j].iString));
                oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type="), 6);
                switch (PVPlayerConfigBaseKeys[j].iType)
                {
                    case PVMI_KVPTYPE_AGGREGATE:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_AGGREGATE_STRING), oscl_strlen(PVMI_KVPTYPE_AGGREGATE_STRING));
                        break;

                    case PVMI_KVPTYPE_POINTER:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_POINTER_STRING), oscl_strlen(PVMI_KVPTYPE_POINTER_STRING));
                        break;

                    case PVMI_KVPTYPE_VALUE:
                    default:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPTYPE_VALUE_STRING), oscl_strlen(PVMI_KVPTYPE_VALUE_STRING));
                        break;
                }
                oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";valtype="), 9);
                switch (PVPlayerConfigBaseKeys[j].iValueType)
                {
                    case PVMI_KVPVALTYPE_RANGE_INT32:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_INT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_INT32_STRING));
                        break;

                    case PVMI_KVPVALTYPE_KSV:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_KSV_STRING), oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
                        break;

                    case PVMI_KVPVALTYPE_CHARPTR:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_CHARPTR_STRING), oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING));
                        break;

                    case PVMI_KVPVALTYPE_BOOL:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
                        break;

                    case PVMI_KVPVALTYPE_UINT32:
                    default:
                        oscl_strncat(aParameters[j].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
                        break;
                }
                aParameters[j].key[PVPLAYERCONFIG_KEYSTRING_SIZE-1] = 0;
            }

            aNumParamElements = PVPLAYERCONFIG_BASE_NUMKEYS;
        }
        else
        {
            // Retrieve the third component from the key string
            pv_mime_string_extract_type(2, aIdentifier, compstr);

            for (int32 engcomp3ind = 0; engcomp3ind < PVPLAYERCONFIG_BASE_NUMKEYS; ++engcomp3ind)
            {
                // Go through each engine component string at 3rd level
                if (pv_mime_strcmp(compstr, (char*)(PVPlayerConfigBaseKeys[engcomp3ind].iString)) >= 0)
                {
                    if (engcomp3ind == 12)
                    {
                        // "x-pvmf/player/productinfo"
                        if (compcount == 3)
                        {
                            // Return list of product info. Ignore the
                            // attribute since capability is only allowed

                            // Allocate memory for the KVP list
                            aParameters = (PvmiKvp*)oscl_malloc(PVPLAYERCONFIG_PRODINFO_NUMKEYS * sizeof(PvmiKvp));
                            if (aParameters == NULL)
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Memory allocation for KVP failed"));
                                return PVMFErrNoMemory;
                            }
                            oscl_memset(aParameters, 0, PVPLAYERCONFIG_PRODINFO_NUMKEYS*sizeof(PvmiKvp));
                            // Allocate memory for the key strings in each KVP
                            PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVPLAYERCONFIG_PRODINFO_NUMKEYS * PVPLAYERCONFIG_KEYSTRING_SIZE * sizeof(char));
                            if (memblock == NULL)
                            {
                                oscl_free(aParameters);
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Memory allocation for key string failed"));
                                return PVMFErrNoMemory;
                            }
                            oscl_strset(memblock, 0, PVPLAYERCONFIG_PRODINFO_NUMKEYS*PVPLAYERCONFIG_KEYSTRING_SIZE*sizeof(char));
                            // Assign the key string buffer to each KVP
                            int32 j;
                            for (j = 0; j < PVPLAYERCONFIG_PRODINFO_NUMKEYS; ++j)
                            {
                                aParameters[j].key = memblock + (j * PVPLAYERCONFIG_KEYSTRING_SIZE);
                            }
                            // Copy the requested info
                            for (j = 0; j < PVPLAYERCONFIG_PRODINFO_NUMKEYS; ++j)
                            {
                                oscl_strncat(aParameters[j].key, _STRLIT_CHAR("x-pvmf/player/productinfo/"), 26);
                                oscl_strncat(aParameters[j].key, PVPlayerConfigProdInfoKeys[j].iString, oscl_strlen(PVPlayerConfigProdInfoKeys[j].iString));
                                oscl_strncat(aParameters[j].key, _STRLIT_CHAR(";type=value;valtype=char*"), 25);
                                aParameters[j].key[PVPLAYERCONFIG_KEYSTRING_SIZE-1] = 0;
                            }

                            aNumParamElements = PVPLAYERCONFIG_PRODINFO_NUMKEYS;
                        }
                        else if (compcount == 4)
                        {
                            // Retrieve the fourth component from the key string
                            pv_mime_string_extract_type(3, aIdentifier, compstr);

                            for (int32 engcomp4ind = 0; engcomp4ind < PVPLAYERCONFIG_PRODINFO_NUMKEYS; ++engcomp4ind)
                            {
                                if (pv_mime_strcmp(compstr, (char*)(PVPlayerConfigProdInfoKeys[engcomp4ind].iString)) >= 0)
                                {
                                    // Determine what is requested
                                    PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
                                    if (reqattr == PVMI_KVPATTR_UNKNOWN)
                                    {
                                        // Default is current setting
                                        reqattr = PVMI_KVPATTR_CUR;
                                    }

                                    // Return the requested info
                                    PVMFStatus retval = DoGetPlayerProductInfoParameter(aParameters, aNumParamElements, engcomp4ind, reqattr);
                                    if (retval != PVMFSuccess)
                                    {
                                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Retrieving product info failed"));
                                        return retval;
                                    }

                                    // Break out of the for(engcomp4ind) loop
                                    break;
                                }
                            }
                        }
                        else
                        {
                            // Right now engine doesn't support more than 4 components
                            // so error out
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Unsupported key"));
                            return PVMFErrArgument;
                        }
                    }
                    else
                    {
                        if (compcount == 3)
                        {
                            // Determine what is requested
                            PvmiKvpAttr reqattr = GetAttrTypeFromKeyString(aIdentifier);
                            if (reqattr == PVMI_KVPATTR_UNKNOWN)
                            {
                                reqattr = PVMI_KVPATTR_CUR;
                            }

                            // Return the requested info
                            PVMFStatus retval = DoGetPlayerParameter(aParameters, aNumParamElements, engcomp3ind, reqattr);
                            if (retval != PVMFSuccess)
                            {
                                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Retrieving player parameter failed"));
                                return retval;
                            }
                        }
                        else
                        {
                            // Right now engine doesn't support more than 3 components
                            // for this sub-key string so error out
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Unsupported key"));
                            return PVMFErrArgument;
                        }
                    }

                    // Breakout of the for(engcomp3ind) loop
                    break;
                }
            }
        }
    }
    else
    {
        // Determine which node's cap-config IF needs to be used
        Oscl_Vector<PvmiCapabilityAndConfig*, OsclMemAllocator> nodecapconfigif;
        PVMFStatus retval = DoQueryNodeCapConfig(compstr, nodecapconfigif);
        if (retval == PVMFSuccess && !(nodecapconfigif.empty()))
        {
            uint32 nodeind = 0;
            // Go through each returned node's cap-config until successful
            while (nodeind < nodecapconfigif.size() && aNumParamElements == 0)
            {
                retval = nodecapconfigif[nodeind]->getParametersSync(NULL, aIdentifier, aParameters, aNumParamElements, aContext);
                ++nodeind;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Out"));
    if (aNumParamElements == 0)
    {
        // If no one could get the parameter, return error
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigGetParametersSync() Unsupported key"));
        return PVMFFailure;
    }
    else
    {
        return PVMFSuccess;
    }
}


PVMFStatus PVPlayerEngine::DoCapConfigReleaseParameters(PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigReleaseParameters() In"));

    if (aParameters == NULL || aNumElements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigReleaseParameters() KVP list is NULL or number of elements is 0"));
        return PVMFErrArgument;
    }

    // Count the number of components and parameters in the key
    int compcount = pv_mime_string_compcnt(aParameters[0].key);
    // Retrieve the first component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(0, aParameters[0].key, compstr);

    if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
    {
        // First component should be "x-pvmf" and there must
        // be at least two components to go past x-pvmf
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigReleaseParameters() Unsupported key"));
        return PVMFErrArgument;
    }

    // Retrieve the second component from the key string
    pv_mime_string_extract_type(1, aParameters[0].key, compstr);

    // Assume all the parameters come from the same component so the base components are the same
    // First check if it is key string for engine ("player")
    if (pv_mime_strcmp(compstr, _STRLIT_CHAR("player")) >= 0)
    {
        // Go through each KVP and release memory for value if allocated from heap
        for (int32 i = 0; i < aNumElements; ++i)
        {
            // Next check if it is a value type that allocated memory
            PvmiKvpType kvptype = GetTypeFromKeyString(aParameters[i].key);
            if (kvptype == PVMI_KVPTYPE_VALUE || kvptype == PVMI_KVPTYPE_UNKNOWN)
            {
                PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameters[i].key);
                if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigReleaseParameters() Valtype not specified in key string"));
                    return PVMFErrArgument;
                }

                if (keyvaltype == PVMI_KVPVALTYPE_CHARPTR && aParameters[i].value.pChar_value != NULL)
                {
                    oscl_free(aParameters[i].value.pChar_value);
                    aParameters[i].value.pChar_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_KSV && aParameters[i].value.key_specific_value != NULL)
                {
                    oscl_free(aParameters[i].value.key_specific_value);
                    aParameters[i].value.key_specific_value = NULL;
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_RANGE_INT32 && aParameters[i].value.key_specific_value != NULL)
                {
                    range_int32* ri32 = (range_int32*)aParameters[i].value.key_specific_value;
                    aParameters[i].value.key_specific_value = NULL;
                    oscl_free(ri32);
                }
                else if (keyvaltype == PVMI_KVPVALTYPE_RANGE_UINT32 && aParameters[i].value.key_specific_value != NULL)
                {
                    range_uint32* rui32 = (range_uint32*)aParameters[i].value.key_specific_value;
                    aParameters[i].value.key_specific_value = NULL;
                    oscl_free(rui32);
                }
            }
        }

        // Player engine allocated its key strings in one chunk so just free the first key string ptr
        oscl_free(aParameters[0].key);

        // Free memory for the parameter list
        oscl_free(aParameters);
        aParameters = NULL;
    }
    else
    {
        // Determine which node's cap-config IF needs to be used
        Oscl_Vector<PvmiCapabilityAndConfig*, OsclMemAllocator> nodecapconfigif;
        PVMFStatus retval = DoQueryNodeCapConfig(compstr, nodecapconfigif);
        if (retval == PVMFSuccess && !(nodecapconfigif.empty()))
        {
            uint32 nodeind = 0;
            retval = PVMFErrArgument;
            // Go through each returned node's cap-config until successful
            while (nodeind < nodecapconfigif.size() && retval != PVMFSuccess)
            {
                retval = nodecapconfigif[nodeind]->releaseParameters(NULL, aParameters, aNumElements);
                ++nodeind;
            }

            if (retval != PVMFSuccess)
            {
                return retval;
            }
        }
        else
        {
            // Unknown key string
            return PVMFErrArgument;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigReleaseParameters() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoCapConfigSetParameters(PVPlayerEngineCommand& aCmd, bool aSyncCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigSetParameters() In"));

    PvmiKvp* paramkvp;
    int32 numparam;
    PvmiKvp** retkvp;
    paramkvp = (PvmiKvp*)(aCmd.GetParam(0).pOsclAny_value);
    numparam = aCmd.GetParam(1).int32_value;
    retkvp = (PvmiKvp**)(aCmd.GetParam(2).pOsclAny_value);

    if (paramkvp == NULL || retkvp == NULL || numparam < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigSetParameters() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Go through each parameter
    for (int32 paramind = 0; paramind < numparam; ++paramind)
    {
        if (iRollOverState == RollOverStateIdle)
        {
            PVMFStatus ret = VerifyAndSaveKVPValues(&paramkvp[paramind]);
            if (ret != PVMFSuccess)
            {
                return ret;
            };
        }
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(paramkvp[paramind].key);
        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, paramkvp[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
        {
            // First component should be "x-pvmf" and there must
            // be at least two components to go past x-pvmf
            *retkvp = &paramkvp[paramind];
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigSetParameters() Unsupported key"));
            return PVMFErrArgument;
        }

        // Retrieve the second component from the key string
        pv_mime_string_extract_type(1, paramkvp[paramind].key, compstr);

        // First check if it is key string for engine ("player")
        if (pv_mime_strcmp(compstr, _STRLIT_CHAR("player")) >= 0)
        {
            if (compcount == 3)
            {
                // Verify and set the passed-in player setting
                PVMFStatus retval = DoVerifyAndSetPlayerParameter(paramkvp[paramind], true);
                if (retval != PVMFSuccess)
                {
                    *retkvp = &paramkvp[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigSetParameters() Setting parameter %d failed", paramind));
                    return retval;
                }
            }
            else if (compcount == 4)
            {
                // Only product info keys have four components
                // Verify and set the passed-in product info setting
                PVMFStatus retval = DoVerifyAndSetPlayerProductInfoParameter(paramkvp[paramind], true);
                if (retval != PVMFSuccess)
                {
                    *retkvp = &paramkvp[paramind];
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigSetParameters() Setting parameter %d failed", paramind));
                    return retval;
                }
            }
            else
            {
                // Do not support more than 4 components right now
                *retkvp = &paramkvp[paramind];
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigSetParameters() Unsupported key"));
                return PVMFErrArgument;
            }
        }
        else
        {
            // Determine which node's cap-config IF needs to be used
            Oscl_Vector<PvmiCapabilityAndConfig*, OsclMemAllocator> nodecapconfigif;
            PVMFStatus retval = DoQueryNodeCapConfig(compstr, nodecapconfigif);
            *retkvp = &paramkvp[paramind];
            if (retval == PVMFSuccess && !(nodecapconfigif.empty()))
            {
                uint32 nodeind = 0;
                bool anysuccess = false;
                // Go through each returned node's cap-config until successful
                while (nodeind < nodecapconfigif.size())
                {
                    *retkvp = NULL;
                    nodecapconfigif[nodeind]->setParametersSync(NULL, &paramkvp[paramind], 1, *retkvp);
                    ++nodeind;
                    if (*retkvp == NULL && anysuccess == false)
                    {
                        anysuccess = true;
                    }
                }
                if (anysuccess == false)
                {   // setParametersSync was not accepted by the node(s)
                    *retkvp = &paramkvp[paramind];
                    return PVMFErrArgument;
                }

            }
            else
            {
                // Unknown key string
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigSetParameters() Unsupported key"));
                return PVMFErrArgument;
            }
        }

    }

    if (!aSyncCmd)
    {
        EngineCommandCompleted(aCmd.GetCmdId(), aCmd.GetContext(), PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigSetParameters() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoCapConfigVerifyParameters(PvmiKvp* aParameters, int aNumElements)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() In"));

    if (aParameters == NULL || aNumElements < 1)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Passed in parameter invalid"));
        return PVMFErrArgument;
    }

    // Go through each parameter and verify
    for (int32 paramind = 0; paramind < aNumElements; ++paramind)
    {
        // Count the number of components and parameters in the key
        int compcount = pv_mime_string_compcnt(aParameters[paramind].key);
        // Retrieve the first component from the key string
        char* compstr = NULL;
        pv_mime_string_extract_type(0, aParameters[paramind].key, compstr);

        if ((pv_mime_strcmp(compstr, _STRLIT_CHAR("x-pvmf")) < 0) || compcount < 2)
        {
            // First component should be "x-pvmf" and there must
            // be at least two components to go past x-pvmf
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Unsupported key"));
            return PVMFErrArgument;
        }

        // Retrieve the second component from the key string
        pv_mime_string_extract_type(1, aParameters[paramind].key, compstr);

        // First check if it is key string for engine ("player")
        if (pv_mime_strcmp(compstr, _STRLIT_CHAR("player")) >= 0)
        {
            if (compcount == 3)
            {
                // Verify the passed-in player setting
                PVMFStatus retval = DoVerifyAndSetPlayerParameter(aParameters[paramind], false);
                if (retval != PVMFSuccess)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Verifying parameter %d failed", paramind));
                    return retval;
                }
            }
            else if (compcount == 4)
            {
                // Only product info keys have four components
                // Verify the passed-in product info setting
                PVMFStatus retval = DoVerifyAndSetPlayerProductInfoParameter(aParameters[paramind], false);
                if (retval != PVMFSuccess)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Verifying parameter %d failed", paramind));
                    return retval;
                }
            }
            else
            {
                // Do not support more than 4 components right now
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Unsupported key"));
                return PVMFErrArgument;
            }
        }
        else
        {
            // Determine which node's cap-config IF needs to be used
            Oscl_Vector<PvmiCapabilityAndConfig*, OsclMemAllocator> nodecapconfigif;
            PVMFStatus retval = DoQueryNodeCapConfig(compstr, nodecapconfigif);
            if (retval == PVMFSuccess && !(nodecapconfigif.empty()))
            {
                uint32 nodeind = 0;
                retval = PVMFErrArgument;
                // Go through each returned node's cap-config until successful
                while (nodeind < nodecapconfigif.size() && retval != PVMFSuccess)
                {
                    retval = nodecapconfigif[nodeind]->verifyParametersSync(NULL, &aParameters[paramind], 1);
                    ++nodeind;
                }

                if (retval != PVMFSuccess)
                {
                    return retval;
                }
            }
            else
            {
                // Unknown key string
                return PVMFErrArgument;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoCapConfigVerifyParameters() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoQueryNodeCapConfig(char* aKeySubString, Oscl_Vector<PvmiCapabilityAndConfig*, OsclMemAllocator>& aNodeCapConfigIF)
{
    aNodeCapConfigIF.clear();

    if (aKeySubString == NULL)
    {
        return PVMFErrArgument;
    }

    int32 leavecode = 0;
    if (pv_mime_strcmp(aKeySubString, _STRLIT_CHAR("video/decoder")) >= 0)
    {
        // Video decoder setting

        // Find the video datapath in the list
        int32 vdpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_VIDEO, vdpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[vdpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iDecNodeCapConfigIF)
                {
                    leavecode = 0;
                    OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(pvpedp->iDecNodeCapConfigIF));
                    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
                }

                if (pvpedp->iSinkNodeCapConfigIF)
                {
                    leavecode = 0;
                    OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(pvpedp->iSinkNodeCapConfigIF));
                    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
                }
            }
        }
    }
    else if (pv_mime_strcmp(aKeySubString, _STRLIT_CHAR("audio/decoder")) >= 0)
    {
        // Audio decoder setting

        // Find the audio datapath in the list
        int32 adpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_AUDIO, adpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[adpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iDecNodeCapConfigIF)
                {
                    leavecode = 0;
                    OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(pvpedp->iDecNodeCapConfigIF));
                    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
                }

                if (pvpedp->iSinkNodeCapConfigIF)
                {
                    leavecode = 0;
                    OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(pvpedp->iSinkNodeCapConfigIF));
                    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
                }
            }
        }
    }
    else if (pv_mime_strcmp(aKeySubString, _STRLIT_CHAR("video/render")) >= 0)
    {
        // Video render device setting

        // Find the video datapath in the list
        int32 vdpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_VIDEO, vdpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[vdpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iSinkNodeCapConfigIF)
                {
                    leavecode = 0;
                    OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(pvpedp->iSinkNodeCapConfigIF));
                    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
                }
            }
        }
    }
    else if (pv_mime_strcmp(aKeySubString, _STRLIT_CHAR("audio/render")) >= 0)
    {
        // Audio render device setting

        // Find the audio datapath in the list
        int32 adpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_AUDIO, adpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[adpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iSinkNodeCapConfigIF)
                {
                    leavecode = 0;
                    OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(pvpedp->iSinkNodeCapConfigIF));
                    OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
                }
            }
        }
    }
    else if (pv_mime_strcmp(aKeySubString, _STRLIT_CHAR("net")) >= 0 ||
             pv_mime_strcmp(aKeySubString, _STRLIT_CHAR("parser")) >= 0)
    {
        // Source node setting
        if (iSourceNodeCapConfigIF)
        {
            leavecode = 0;
            OSCL_TRY(leavecode, aNodeCapConfigIF.push_back(iSourceNodeCapConfigIF));
            OSCL_FIRST_CATCH_ANY(leavecode, return PVMFErrNoMemory);
        }
    }
    else
    {
        // No matching node
        return PVMFFailure;
    }

    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetPlayerParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlayerParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));
    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVPLAYERCONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for key string failed"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, PVPLAYERCONFIG_KEYSTRING_SIZE*sizeof(char));
    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/player/"), 14);
    oscl_strncat(aParameters[0].key, PVPlayerConfigBaseKeys[aIndex].iString, oscl_strlen(PVPlayerConfigBaseKeys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype="), 20);
    switch (PVPlayerConfigBaseKeys[aIndex].iValueType)
    {
        case PVMI_KVPVALTYPE_RANGE_INT32:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_INT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_INT32_STRING));
            break;

        case PVMI_KVPVALTYPE_KSV:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_KSV_STRING), oscl_strlen(PVMI_KVPVALTYPE_KSV_STRING));
            break;

        case PVMI_KVPVALTYPE_CHARPTR:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_CHARPTR_STRING), oscl_strlen(PVMI_KVPVALTYPE_CHARPTR_STRING));
            break;

        case PVMI_KVPVALTYPE_BOOL:
            oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_BOOL_STRING), oscl_strlen(PVMI_KVPVALTYPE_BOOL_STRING));
            break;

        case PVMI_KVPVALTYPE_UINT32:
        default:
            if (reqattr == PVMI_KVPATTR_CAP)
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_RANGE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_RANGE_UINT32_STRING));
            }
            else
            {
                oscl_strncat(aParameters[0].key, _STRLIT_CHAR(PVMI_KVPVALTYPE_UINT32_STRING), oscl_strlen(PVMI_KVPVALTYPE_UINT32_STRING));
            }
            break;
    }
    aParameters[0].key[PVPLAYERCONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case 0: // "pbpops_units"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                // Allocate memory for the string
                char* curstr = (char*)oscl_malloc(32 * sizeof(char));
                if (curstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                oscl_strset(curstr, 0, 32);
                // Copy the appropriate string based on units being used
                switch (iPBPosStatusUnit)
                {
                    case PVPPBPOSUNIT_SEC:
                        oscl_strncpy(curstr, _STRLIT_CHAR("PVPPBPOSUNIT_SEC"), 16);
                        aParameters[0].length = 16;
                        break;

                    case PVPPBPOSUNIT_MIN:
                        oscl_strncpy(curstr, _STRLIT_CHAR("PVPPBPOSUNIT_MIN"), 16);
                        aParameters[0].length = 16;
                        break;

                    case PVPPBPOSUNIT_MILLISEC:
                    default:
                        oscl_strncpy(curstr, _STRLIT_CHAR("PVPPBPOSUNIT_MILLISEC"), 21);
                        aParameters[0].length = 21;
                        break;
                }

                aParameters[0].value.pChar_value = curstr;
                aParameters[0].capacity = 32;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                // Allocate memory for the string
                int32 defstrlen = oscl_strlen(PVPLAYERENGINE_CONFIG_PBPOSSTATUSUNIT_DEF_STRING);
                char* defstr = (char*)oscl_malloc((defstrlen + 1) * sizeof(char));
                if (defstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(defstr, _STRLIT_CHAR(PVPLAYERENGINE_CONFIG_PBPOSSTATUSUNIT_DEF_STRING), defstrlen);
                defstr[defstrlen] = 0;
                aParameters[0].value.pChar_value = defstr;
                aParameters[0].capacity = defstrlen + 1;
                aParameters[0].length = defstrlen;
            }
            else
            {
                // Return capability
                // Allocate memory for the string
                int32 capstrlen = oscl_strlen(PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_CAP_STRING);
                char* capstr = (char*)oscl_malloc((capstrlen + 1) * sizeof(char));
                if (capstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(capstr, _STRLIT_CHAR(PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_CAP_STRING), capstrlen);
                capstr[capstrlen] = 0;
                aParameters[0].value.pChar_value = capstr;
                aParameters[0].capacity = capstrlen + 1;
                aParameters[0].length = capstrlen;
            }
            break;

        case 1:	// "pbpos_interval"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iPBPosStatusInterval;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_MIN;
                rui32->max = PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 2:	// "endtimecheck_interval"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iEndTimeCheckInterval;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVPLAYERENGINE_CONFIG_ENDTIMECHECKINTERVAL_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVPLAYERENGINE_CONFIG_ENDTIMECHECKINTERVAL_MIN;
                rui32->max = PVPLAYERENGINE_CONFIG_ENDTIMECHECKINTERVAL_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 3:	// "seektosyncpoint"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.bool_value = iSeekToSyncPoint;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.bool_value = PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINT_DEF;
            }
            else
            {
                // Return capability
                // Bool so no capability
            }
            break;

        case 4:	// "skiptorequestedpos"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.bool_value = iSkipToRequestedPosition;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.bool_value = PVPLAYERENGINE_CONFIG_SKIPTOREQUESTEDPOS_DEF;
            }
            else
            {
                // Return capability
                // Bool so no capability
            }
            break;

        case 5:	// "renderskipped"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.bool_value = iRenderSkipped;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.bool_value = PVPLAYERENGINE_CONFIG_RENDERSKIPPED_DEF;
            }
            else
            {
                // Return capability
                // Bool so no capability
            }
            break;

        case 6:	// "syncpointseekwindow"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iSyncPointSeekWindow;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINTWINDOW_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINTWINDOW_MIN;
                rui32->max = PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINTWINDOW_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 7:	// "syncmargin_video"
        case 8:	// "syncmargin_audio"
        case 9:	// "syncmargin_text"
        {
            range_int32* ri32 = (range_int32*)oscl_malloc(sizeof(range_int32));
            if (ri32 == NULL)
            {
                oscl_free(aParameters[0].key);
                oscl_free(aParameters);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for range int32 failed"));
                return PVMFErrNoMemory;
            }

            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                if (aIndex == 7)
                {
                    // Video
                    ri32->min = iSyncMarginVideo.min;
                    ri32->max = iSyncMarginVideo.max;
                }
                else if (aIndex == 8)
                {
                    // Audio
                    ri32->min = iSyncMarginAudio.min;
                    ri32->max = iSyncMarginAudio.max;
                }
                else
                {
                    // Text
                    ri32->min = iSyncMarginText.min;
                    ri32->max = iSyncMarginText.max;
                }
                aParameters[0].value.key_specific_value = (void*)ri32;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                ri32->min = PVPLAYERENGINE_CONFIG_SYNCMARGIN_EARLY_DEF;
                ri32->max = PVPLAYERENGINE_CONFIG_SYNCMARGIN_LATE_DEF;
                aParameters[0].value.key_specific_value = (void*)ri32;
            }
            else
            {
                // Return capability
                ri32->min = PVPLAYERENGINE_CONFIG_SYNCMARGIN_MIN;
                ri32->max = PVPLAYERENGINE_CONFIG_SYNCMARGIN_MAX;
                aParameters[0].value.key_specific_value = (void*)ri32;
            }
        }
        break;

        case 10:	// "nodecmd_timeout"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iNodeCmdTimeout;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVPLAYERENGINE_CONFIG_NODECMDTIMEOUT_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for range uint32 failed"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVPLAYERENGINE_CONFIG_NODECMDTIMEOUT_MIN;
                rui32->max = PVPLAYERENGINE_CONFIG_NODECMDTIMEOUT_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 11:	// "nodedataqueuing_timeout"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.uint32_value = iNodeDataQueuingTimeout;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.uint32_value = PVPLAYERENGINE_CONFIG_NODEDATAQUEUINGTIMEOUT_DEF;
            }
            else
            {
                // Return capability
                range_uint32* rui32 = (range_uint32*)oscl_malloc(sizeof(range_uint32));
                if (rui32 == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Memory allocation for range uint32"));
                    return PVMFErrNoMemory;
                }
                rui32->min = PVPLAYERENGINE_CONFIG_NODEDATAQUEUINGTIMEOUT_MIN;
                rui32->max = PVPLAYERENGINE_CONFIG_NODEDATAQUEUINGTIMEOUT_MAX;
                aParameters[0].value.key_specific_value = (void*)rui32;
            }
            break;

        case 13:	// "pbpos_enable"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                aParameters[0].value.bool_value = iPBPosEnable;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                aParameters[0].value.bool_value = true;
            }
            else
            {
                // Return capability
                // Bool so no capability
            }
            break;
        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerParameter() Invalid index to player parameter"));
            return PVMFErrArgument;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlayerParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoGetPlayerProductInfoParameter(PvmiKvp*& aParameters, int& aNumParamElements, int32 aIndex, PvmiKvpAttr reqattr)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() In"));

    aNumParamElements = 0;

    // Allocate memory for the KVP
    aParameters = (PvmiKvp*)oscl_malloc(sizeof(PvmiKvp));
    if (aParameters == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for KVP failed"));
        return PVMFErrNoMemory;
    }
    oscl_memset(aParameters, 0, sizeof(PvmiKvp));
    // Allocate memory for the key string in KVP
    PvmiKeyType memblock = (PvmiKeyType)oscl_malloc(PVPLAYERCONFIG_KEYSTRING_SIZE * sizeof(char));
    if (memblock == NULL)
    {
        oscl_free(aParameters);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for key string"));
        return PVMFErrNoMemory;
    }
    oscl_strset(memblock, 0, PVPLAYERCONFIG_KEYSTRING_SIZE*sizeof(char));
    // Assign the key string buffer to KVP
    aParameters[0].key = memblock;

    // Copy the key string
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR("x-pvmf/player/productinfo/"), 26);
    oscl_strncat(aParameters[0].key, PVPlayerConfigProdInfoKeys[aIndex].iString, oscl_strlen(PVPlayerConfigProdInfoKeys[aIndex].iString));
    oscl_strncat(aParameters[0].key, _STRLIT_CHAR(";type=value;valtype=char*"), 25);
    aParameters[0].key[PVPLAYERCONFIG_KEYSTRING_SIZE-1] = 0;

    // Copy the requested info
    switch (aIndex)
    {
        case 0: // "productname"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                // Allocate memory for the string
                char* curstr = (char*)oscl_malloc(iProdInfoProdName.get_size() + 1);
                if (curstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                oscl_strset(curstr, 0, iProdInfoProdName.get_size() + 1);
                // Copy and set
                oscl_strncpy(curstr, iProdInfoProdName.get_cstr(), iProdInfoProdName.get_size());
                aParameters[0].value.pChar_value = curstr;
                aParameters[0].length = iProdInfoProdName.get_size();
                aParameters[0].capacity = iProdInfoProdName.get_size() + 1;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                // Allocate memory for the string
                int32 defstrlen = oscl_strlen(PVPLAYERENGINE_PRODINFO_PRODNAME_STRING);
                char* defstr = (char*)oscl_malloc((defstrlen + 1) * sizeof(char));
                if (defstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(defstr, _STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_PRODNAME_STRING), defstrlen);
                defstr[defstrlen] = 0;
                aParameters[0].value.pChar_value = defstr;
                aParameters[0].capacity = defstrlen + 1;
                aParameters[0].length = defstrlen;
            }
            else
            {
                // Return capability
                // Empty string
                aParameters[0].value.pChar_value = NULL;
                aParameters[0].capacity = 0;
                aParameters[0].length = 0;
            }
            break;

        case 1: // "partnumber"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                // Allocate memory for the string
                char* curstr = (char*)oscl_malloc(iProdInfoPartNum.get_size() + 1);
                if (curstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                oscl_strset(curstr, 0, iProdInfoPartNum.get_size() + 1);
                // Copy and set
                oscl_strncpy(curstr, iProdInfoPartNum.get_cstr(), iProdInfoPartNum.get_size());
                aParameters[0].value.pChar_value = curstr;
                aParameters[0].length = iProdInfoPartNum.get_size();
                aParameters[0].capacity = iProdInfoPartNum.get_size() + 1;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                // Allocate memory for the string
                int32 defstrlen = oscl_strlen(PVPLAYERENGINE_PRODINFO_PARTNUM_STRING);
                char* defstr = (char*)oscl_malloc((defstrlen + 1) * sizeof(char));
                if (defstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(defstr, _STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_PARTNUM_STRING), defstrlen);
                defstr[defstrlen] = 0;
                aParameters[0].value.pChar_value = defstr;
                aParameters[0].capacity = defstrlen + 1;
                aParameters[0].length = defstrlen;
            }
            else
            {
                // Return capability
                // Empty string
                aParameters[0].value.pChar_value = NULL;
                aParameters[0].capacity = 0;
                aParameters[0].length = 0;
            }
            break;

        case 2: // "hardwareplatform"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                // Allocate memory for the string
                char* curstr = (char*)oscl_malloc(iProdInfoHWPlatform.get_size() + 1);
                if (curstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                oscl_strset(curstr, 0, iProdInfoHWPlatform.get_size() + 1);
                // Copy and set
                oscl_strncpy(curstr, iProdInfoHWPlatform.get_cstr(), iProdInfoHWPlatform.get_size());
                aParameters[0].value.pChar_value = curstr;
                aParameters[0].length = iProdInfoHWPlatform.get_size();
                aParameters[0].capacity = iProdInfoHWPlatform.get_size() + 1;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                // Allocate memory for the string
                int32 defstrlen = oscl_strlen(PVPLAYERENGINE_PRODINFO_HWPLATFORM_STRING);
                char* defstr = (char*)oscl_malloc((defstrlen + 1) * sizeof(char));
                if (defstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(defstr, _STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_HWPLATFORM_STRING), defstrlen);
                defstr[defstrlen] = 0;
                aParameters[0].value.pChar_value = defstr;
                aParameters[0].capacity = defstrlen + 1;
                aParameters[0].length = defstrlen;
            }
            else
            {
                // Return capability
                // Empty string
                aParameters[0].value.pChar_value = NULL;
                aParameters[0].capacity = 0;
                aParameters[0].length = 0;
            }
            break;

        case 3: // "softwareplatform"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                // Allocate memory for the string
                char* curstr = (char*)oscl_malloc(iProdInfoSWPlatform.get_size() + 1);
                if (curstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                oscl_strset(curstr, 0, iProdInfoSWPlatform.get_size() + 1);
                // Copy and set
                oscl_strncpy(curstr, iProdInfoSWPlatform.get_cstr(), iProdInfoSWPlatform.get_size());
                aParameters[0].value.pChar_value = curstr;
                aParameters[0].length = iProdInfoSWPlatform.get_size();
                aParameters[0].capacity = iProdInfoSWPlatform.get_size() + 1;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                // Allocate memory for the string
                int32 defstrlen = oscl_strlen(PVPLAYERENGINE_PRODINFO_SWPLATFORM_STRING);
                char* defstr = (char*)oscl_malloc((defstrlen + 1) * sizeof(char));
                if (defstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(defstr, _STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_SWPLATFORM_STRING), defstrlen);
                defstr[defstrlen] = 0;
                aParameters[0].value.pChar_value = defstr;
                aParameters[0].capacity = defstrlen + 1;
                aParameters[0].length = defstrlen;
            }
            else
            {
                // Return capability
                // Empty string
                aParameters[0].value.pChar_value = NULL;
                aParameters[0].capacity = 0;
                aParameters[0].length = 0;
            }
            break;

        case 4: // "device"
            if (reqattr == PVMI_KVPATTR_CUR)
            {
                // Return current value
                // Allocate memory for the string
                char* curstr = (char*)oscl_malloc(iProdInfoDevice.get_size() + 1);
                if (curstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                oscl_strset(curstr, 0, iProdInfoDevice.get_size() + 1);
                // Copy and set
                oscl_strncpy(curstr, iProdInfoPartNum.get_cstr(), iProdInfoDevice.get_size());
                aParameters[0].value.pChar_value = curstr;
                aParameters[0].length = iProdInfoDevice.get_size();
                aParameters[0].capacity = iProdInfoDevice.get_size() + 1;
            }
            else if (reqattr == PVMI_KVPATTR_DEF)
            {
                // Return default
                // Allocate memory for the string
                int32 defstrlen = oscl_strlen(PVPLAYERENGINE_PRODINFO_DEVICE_STRING);
                char* defstr = (char*)oscl_malloc((defstrlen + 1) * sizeof(char));
                if (defstr == NULL)
                {
                    oscl_free(aParameters[0].key);
                    oscl_free(aParameters);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Memory allocation for char* string failed"));
                    return PVMFErrNoMemory;
                }
                // Copy and set
                oscl_strncpy(defstr, _STRLIT_CHAR(PVPLAYERENGINE_PRODINFO_DEVICE_STRING), defstrlen);
                defstr[defstrlen] = 0;
                aParameters[0].value.pChar_value = defstr;
                aParameters[0].capacity = defstrlen + 1;
                aParameters[0].length = defstrlen;
            }
            else
            {
                // Return capability
                // Empty string
                aParameters[0].value.pChar_value = NULL;
                aParameters[0].capacity = 0;
                aParameters[0].length = 0;
            }
            break;

        default:
            // Invalid index
            oscl_free(aParameters[0].key);
            oscl_free(aParameters);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Invalid index for product info"));
            return PVMFErrArgument;
    }

    aNumParamElements = 1;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoGetPlayerProductInfoParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoVerifyAndSetPlayerParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Valtype in key string unknown"));
        return PVMFErrArgument;
    }
    // Retrieve the third component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(2, aParameter.key, compstr);

    int32 engcomp3ind = 0;
    for (engcomp3ind = 0; engcomp3ind < PVPLAYERCONFIG_BASE_NUMKEYS; ++engcomp3ind)
    {
        // Go through each engine component string at 3rd level
        if (pv_mime_strcmp(compstr, (char*)(PVPlayerConfigBaseKeys[engcomp3ind].iString)) >= 0)
        {
            // Break out of the for loop
            break;
        }
    }

    if (engcomp3ind >= PVPLAYERCONFIG_BASE_NUMKEYS || engcomp3ind == 12)
    {
        // Match couldn't be found or non-leaf node ("productinfo") specified
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Unsupported key or non-leaf node"));
        return PVMFErrArgument;
    }

    // Verify the valtype
    if (keyvaltype != PVPlayerConfigBaseKeys[engcomp3ind].iValueType)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Valtype does not match for key"));
        return PVMFErrArgument;
    }

    switch (engcomp3ind)
    {
        case 0: // "pbpos_units"
        {
            // Validate
            if (aParameter.value.pChar_value == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() char* string for pbpos_units is NULL"));
                return PVMFErrArgument;
            }

            // Check the specified unit
            // Use sample number as the invalid default since it is not allowed
            PVPPlaybackPositionUnit newposunit = PVPPBPOSUNIT_UNKNOWN;
            if (oscl_strncmp(aParameter.value.pChar_value, _STRLIT_CHAR("PVPPBPOSUNIT_SEC"), 16) == 0)
            {
                newposunit = PVPPBPOSUNIT_SEC;
            }
            else if (oscl_strncmp(aParameter.value.pChar_value, _STRLIT_CHAR("PVPPBPOSUNIT_MIN"), 16) == 0)
            {
                newposunit = PVPPBPOSUNIT_MIN;
            }
            else if (oscl_strncmp(aParameter.value.pChar_value, _STRLIT_CHAR("PVPPBPOSUNIT_MILLISEC"), 21) == 0)
            {
                newposunit = PVPPBPOSUNIT_MILLISEC;
            }

            if (newposunit == PVPPBPOSUNIT_UNKNOWN)
            {
                // Couldn't determine the new units
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid units for pbpos_units"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iPBPosStatusUnit = newposunit;
            }
        }
        break;

        case 1: // "pbpos_interval"
            // Check if within range
            if (aParameter.value.uint32_value < PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_MIN ||
                    aParameter.value.uint32_value > PVPLAYERENGINE_CONFIG_PBPOSSTATUSINTERVAL_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid value for pbpos_interval"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iPBPosStatusInterval = aParameter.value.uint32_value;
            }
            break;

        case 2: // "endtimecheck_interval"
            // Check if within range
            if (aParameter.value.uint32_value < PVPLAYERENGINE_CONFIG_ENDTIMECHECKINTERVAL_MIN ||
                    aParameter.value.uint32_value > PVPLAYERENGINE_CONFIG_ENDTIMECHECKINTERVAL_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid value for endtimecheck_interval"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iEndTimeCheckInterval = aParameter.value.uint32_value;
            }
            break;

        case 3: // "seektosyncpoint"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                iSeekToSyncPoint = aParameter.value.bool_value;
            }
            break;

        case 4: // "skiptorequestedpos"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                iSkipToRequestedPosition = aParameter.value.bool_value;
            }
            break;

        case 5: // "renderskipped"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                iRenderSkipped = aParameter.value.bool_value;
            }
            break;

        case 6: // "syncpointseekwindow"
            // Check if within range
            if (aParameter.value.uint32_value < PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINTWINDOW_MIN ||
                    aParameter.value.uint32_value > PVPLAYERENGINE_CONFIG_SEEKTOSYNCPOINTWINDOW_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid value for syncpointseekwindow"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iSyncPointSeekWindow = aParameter.value.uint32_value;
            }
            break;

        case 7: // "syncmargin_video"
        case 8: // "syncmargin_audio"
        case 9: // "syncmargin_text"
        {
            range_int32* ri32 = (range_int32*)aParameter.value.key_specific_value;
            if (ri32 == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() ksv for syncmargin is NULL"));
                return PVMFErrArgument;
            }

            // Check if within range
            if (ri32->min < PVPLAYERENGINE_CONFIG_SYNCMARGIN_MIN ||
                    ri32->min > PVPLAYERENGINE_CONFIG_SYNCMARGIN_MAX ||
                    ri32->max < PVPLAYERENGINE_CONFIG_SYNCMARGIN_MIN ||
                    ri32->max > PVPLAYERENGINE_CONFIG_SYNCMARGIN_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid range for syncmargin"));
                return PVMFErrArgument;
            }

            // Change the config if to set
            if (aSetParam)
            {
                return DoSetConfigSyncMargin(ri32->min, ri32->max, engcomp3ind - 7);
            }
        }
        break;

        case 10: // "nodecmd_timeout"
            // Check if within range
            if (aParameter.value.uint32_value < PVPLAYERENGINE_CONFIG_NODECMDTIMEOUT_MIN ||
                    aParameter.value.uint32_value > PVPLAYERENGINE_CONFIG_NODECMDTIMEOUT_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid value for ndoecmd_timeout"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iNodeCmdTimeout = aParameter.value.uint32_value;
            }
            break;

        case 11: // "nodedataqueuing_timeout"
            // Check if within range
            if (aParameter.value.uint32_value < PVPLAYERENGINE_CONFIG_NODEDATAQUEUINGTIMEOUT_MIN ||
                    aParameter.value.uint32_value > PVPLAYERENGINE_CONFIG_NODEDATAQUEUINGTIMEOUT_MAX)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid value for nodedataqueuing_timeout"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iNodeDataQueuingTimeout = aParameter.value.uint32_value;
            }
            break;

        case 13: // "pbpos_enable"
            // Nothing to validate since it is boolean
            // Change the config if to set
            if (aSetParam)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_DEBUG, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() pbpos_enable set to %d", iPBPosEnable));
                bool prevPBPosEnable = iPBPosEnable;
                iPBPosEnable = aParameter.value.bool_value;
                if (prevPBPosEnable && !(aParameter.value.bool_value))
                {
                    // Stop playback position reporting
                    StopPlaybackStatusTimer();
                }
                else if (!prevPBPosEnable && (aParameter.value.bool_value))
                {
                    // Start playback position reporting only when playback clock is running
                    if (iPlaybackClock.GetState() == OsclClock::RUNNING)
                    {
                        StartPlaybackStatusTimer();
                    }
                }

            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Invalid index for player parameter"));
            return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoVerifyAndSetPlayerParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter(PvmiKvp& aParameter, bool aSetParam)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() In"));

    // Determine the valtype
    PvmiKvpValueType keyvaltype = GetValTypeFromKeyString(aParameter.key);
    if (keyvaltype == PVMI_KVPVALTYPE_UNKNOWN)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() Valtype unknown"));
        return PVMFErrArgument;
    }
    // Retrieve the 4th component from the key string
    char* compstr = NULL;
    pv_mime_string_extract_type(3, aParameter.key, compstr);

    int32 engcomp4ind = 0;
    for (engcomp4ind = 0; engcomp4ind < PVPLAYERCONFIG_PRODINFO_NUMKEYS; ++engcomp4ind)
    {
        // Go through each engine component string at 4th level
        if (pv_mime_strcmp(compstr, (char*)(PVPlayerConfigProdInfoKeys[engcomp4ind].iString)) >= 0)
        {
            // Break out of the for loop
            break;
        }
    }

    if (engcomp4ind >= PVPLAYERCONFIG_PRODINFO_NUMKEYS)
    {
        // Match couldn't be found
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() Unsupported key"));
        return PVMFErrArgument;
    }

    // Verify the valtype
    if (keyvaltype != PVPlayerConfigProdInfoKeys[engcomp4ind].iValueType)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() Valtype does not match for key"));
        return PVMFErrArgument;
    }

    switch (engcomp4ind)
    {
        case 0: // "productname"
            // Check if within range
            if (aParameter.value.pChar_value == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() char* string for productname is NULL"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iProdInfoProdName = aParameter.value.pChar_value;
            }
            break;

        case 1: // "partnumber"
            // Check if within range
            if (aParameter.value.pChar_value == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() char* string for productname is NULL"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iProdInfoPartNum = aParameter.value.pChar_value;
            }
            break;

        case 2: // "hardwareplatform"
            // Check if within range
            if (aParameter.value.pChar_value == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() char* string for productname is NULL"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iProdInfoHWPlatform = aParameter.value.pChar_value;
            }
            break;

        case 3: // "softwareplatform"
            // Check if within range
            if (aParameter.value.pChar_value == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() char* string for productname is NULL"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iProdInfoSWPlatform = aParameter.value.pChar_value;
            }
            break;

        case 4: // "device"
            // Check if within range
            if (aParameter.value.pChar_value == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() char* string for productname is NULL"));
                return PVMFErrArgument;
            }
            // Change the config if to set
            if (aSetParam)
            {
                iProdInfoDevice = aParameter.value.pChar_value;
            }
            break;

        default:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() Invalid index for product info"));
            return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoVerifyAndSetPlayerProductInfoParameter() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerEngine::DoSetConfigSyncMargin(int32 aEarlyMargin, int32 aLateMargin, int32 aMediaType)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetConfigSyncMargin() In"));

    if (aMediaType == 0)
    {
        // Video
        iSyncMarginVideo.min = aEarlyMargin;
        iSyncMarginVideo.max = aLateMargin;

        // Find the video datapath in the list
        int32 vdpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_VIDEO, vdpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[vdpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iSinkNodeSyncCtrlIF)
                {
                    pvpedp->iSinkNodeSyncCtrlIF->SetMargins((-1*iSyncMarginVideo.min), iSyncMarginVideo.max);
                }
            }
        }
    }
    else if (aMediaType == 1)
    {
        // Audio
        iSyncMarginAudio.min = aEarlyMargin;
        iSyncMarginAudio.max = aLateMargin;

        // Find the audio datapath in the list
        int32 adpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_AUDIO, adpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[adpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iSinkNodeSyncCtrlIF)
                {
                    pvpedp->iSinkNodeSyncCtrlIF->SetMargins((-1*iSyncMarginAudio.min), iSyncMarginAudio.max);
                }
            }
        }
    }
    else if (aMediaType == 2)
    {
        // Text
        iSyncMarginText.min = aEarlyMargin;
        iSyncMarginText.max = aLateMargin;

        // Find the text datapath in the list
        int32 tdpind = -1;
        if (FindDatapathByMediaType(PVP_MEDIATYPE_TEXT, tdpind) == true)
        {
            PVPlayerEngineDatapath* pvpedp = &(iDatapathList[tdpind]);
            if (pvpedp->iTrackActive == true)
            {
                if (pvpedp->iSinkNodeSyncCtrlIF)
                {
                    pvpedp->iSinkNodeSyncCtrlIF->SetMargins((-1*iSyncMarginText.min), iSyncMarginText.max);
                }
            }
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::DoSetConfigSyncMargin() Invalid media type index"));
        return PVMFErrArgument;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSetConfigSyncMargin() Out"));
    return PVMFSuccess;
}


PVPlayerEngineContext* PVPlayerEngine::AllocateEngineContext(PVPlayerEngineDatapath* aEngineDatapath, PVMFNodeInterface* aNode, PVPlayerDatapath* aDatapath, PVCommandId aCmdId, OsclAny* aCmdContext, int32 aCmdType)
{
    // Allocate memory for the context from the fixed size memory pool
    PVPlayerEngineContext* context = NULL;
    int32 leavecode = 0;
    OSCL_TRY(leavecode, context = (PVPlayerEngineContext*)(iCurrentContextListMemPool.allocate(sizeof(PVPlayerEngineContext))));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::AllocateEngineContext() allocate on iCurrentContextListMemPool did a leave!"));
                         OSCL_ASSERT(false));

    OSCL_ASSERT(context);

    // Set the context info
    context->iEngineDatapath = aEngineDatapath;
    context->iNode = aNode;
    context->iDatapath = aDatapath;
    context->iCmdId = aCmdId;
    context->iCmdContext = aCmdContext;
    context->iCmdType = aCmdType;

    // Save the context in the list
    leavecode = 0;
    OSCL_TRY(leavecode, iCurrentContextList.push_back(context));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::AllocateEngineContext() Push back on the context list did a leave!"));
                         iCurrentContextListMemPool.deallocate((OsclAny*)context);
                         OSCL_ASSERT(false);
                         return NULL;);

    return context;
}


void PVPlayerEngine::FreeEngineContext(PVPlayerEngineContext* aContext)
{
    OSCL_ASSERT(aContext);

    // Remove the context from the list
    uint32 i = 0;
    bool foundcontext = false;
    for (i = 0; i < iCurrentContextList.size(); ++i)
    {
        if (iCurrentContextList[i] == aContext)
        {
            foundcontext = true;
            break;
        }
    }

    if (foundcontext)
    {
        iCurrentContextList.erase(iCurrentContextList.begin() + i);
        // Free the memory used by context in the memory pool
        iCurrentContextListMemPool.deallocate((OsclAny*)aContext);
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::FreeEngineContext() Context not on current list (0x%x). CmdType %d", aContext, aContext->iCmdType));
        OSCL_ASSERT(false);
        // Don't return to memory pool since it could add multiple entries
        // of same address in free list
    }
}


void PVPlayerEngine::HandleSourceNodeQueryInitIF(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSourceNodeQueryInitIF() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryInitIF() In"));

    PVMFStatus cmdstatus;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
            // Query for track selection interface
            cmdstatus = DoSourceNodeQueryTrackSelIF(aNodeContext.iCmdId, aNodeContext.iCmdContext);
            if (cmdstatus != PVMFSuccess)
            {
                // There shouldn't be any other command pending to complete in engine's AO
                OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
                iCommandCompleteInEngineAOPending = true;
                if ((CheckForSourceRollOver() == true) && (iRollOverState == RollOverStateInProgress))
                {
                    iRollOverState = RollOverStateStart;
                }
                else
                {
                    iRollOverState = RollOverStateIdle;
                }
                iCommandCompleteInEngineAOCmdStatus = cmdstatus;
                iCommandCompleteInEngineAOErrMsg = NULL;
            }
            break;

        default:
        {
            cmdstatus = aNodeResp.GetCmdStatus();
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            iCommandCompleteInEngineAOErrMsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceInit, puuid, nextmsg));
            // There shouldn't be any other command pending to complete in engine's AO
            OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
            iCommandCompleteInEngineAOPending = true;
            if ((CheckForSourceRollOver() == true) && (iRollOverState == RollOverStateInProgress))
            {
                iRollOverState = RollOverStateStart;
            }
            else
            {
                iRollOverState = RollOverStateIdle;
            }
            iCommandCompleteInEngineAOCmdStatus = cmdstatus;
        }
        break;
    }

    if (iCommandCompleteInEngineAOPending)
    {
        if (IsBusy())
        {
            Cancel();
        }

        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryInitIF() Out"));
}


void PVPlayerEngine::HandleSourceNodeQueryTrackSelIF(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSourceNodeQueryTrackSelIF() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryTrackSelIF() In"));

    PVMFStatus cmdstatus;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
            // Query the source node for optional extension IFs
            cmdstatus = DoSourceNodeQueryInterfaceOptional(aNodeContext.iCmdId, aNodeContext.iCmdContext);
            if (cmdstatus != PVMFSuccess)
            {
                // If optional extension IFs are not available, just complete the AddDataSource command as success
                if ((CheckForSourceRollOver() == true) && (iRollOverState == RollOverStateInProgress))
                {
                    iRollOverState = RollOverStateStart;
                }
                else
                {
                    iRollOverState = RollOverStateIdle;
                    iCommandCompleteInEngineAOPending = false;
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
                }
            }
            break;

        default:
        {
            cmdstatus = aNodeResp.GetCmdStatus();
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            iCommandCompleteInEngineAOErrMsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceInit, puuid, nextmsg));
            // There shouldn't be any other command pending to complete in engine's AO
            OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
            iCommandCompleteInEngineAOPending = true;
            if ((CheckForSourceRollOver() == true) && (iRollOverState == RollOverStateInProgress))
            {
                iRollOverState = RollOverStateStart;
            }
            iCommandCompleteInEngineAOCmdStatus = cmdstatus;
        }
        break;
    }

    if (iCommandCompleteInEngineAOPending)
    {
        if (IsBusy())
        {
            Cancel();
        }

        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryTrackSelIF() Out"));
}


void PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() In"));

    // Determine QueryInterface() for which interface completed
    if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQueryTrackLevelInfoIF)
    {
        if (aNodeResp.GetCmdStatus() != PVMFSuccess)
        {
            // Track level info IF is not available in this data source
            iSourceNodeTrackLevelInfoIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Track level info IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQueryPBCtrlIF)
    {
        if (aNodeResp.GetCmdStatus() != PVMFSuccess)
        {
            // Playback control is not available in this data source
            iSourceNodePBCtrlIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Position control IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQueryDirCtrlIF)
    {
        if (aNodeResp.GetCmdStatus() != PVMFSuccess)
        {
            // Direction control is not available in this data source
            iSourceNodeDirCtrlIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Direction control IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQueryMetadataIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && iSourceNodeMetadataExtIF)
        {
            // Add the parser node's metadata extension IF to the list
            if (AddToMetadataInterfaceList(iSourceNodeMetadataExtIF, iSourceNodeSessionId) != PVMFSuccess)
            {
                iSourceNodeMetadataExtIF->removeRef();
                iSourceNodeMetadataExtIF = NULL;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Metadata IF could not be added to list"));
            }
        }
        else
        {
            // Metadata is not available in this data source
            iSourceNodeMetadataExtIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Metadata IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQueryCapConfigIF)
    {
        if (aNodeResp.GetCmdStatus() != PVMFSuccess)
        {
            // Cap-config is not available in this data source
            iSourceNodeCapConfigIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Cap-Config IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQueryCPMLicenseIF)
    {
        if (aNodeResp.GetCmdStatus() != PVMFSuccess)
        {
            //CPM License is not available in this data source
            iSourceNodeCPMLicenseIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() CPM License IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SourceNodeQuerySrcNodeRegInitIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && iSourceNodeRegInitIF)
        {
            // Set source node regsitry
            iSourceNodeRegInitIF->SetPlayerNodeRegistry(&iPlayerNodeRegistry);
        }
        else
        {
            //Node Registry Init Extension Interface is not available in this data source
            iSourceNodeRegInitIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Src Node Registry Init IF not available"));
        }
    }

    // Decrement the pending counter and complete the AddDataSource command if 0.
    --iNumPendingNodeCmd;
    if (iNumPendingNodeCmd == 0)
    {
        if (iRollOverState == RollOverStateInProgress)
        {
            SetRollOverKVPValues();
            PVMFStatus retval = DoSourceNodeInit(aNodeContext.iCmdId, aNodeContext.iCmdContext);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() - Source Roll Over In Progress - Doing Source Node Init"));

            if (retval == PVMFSuccess)
            {
                SetEngineState(PVP_ENGINE_STATE_INITIALIZING);
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() - Source Roll Over In Progress - DoSourceNodeInit Failed"));
                if ((CheckForSourceRollOver() == true) && (iRollOverState == RollOverStateInProgress))
                {
                    iRollOverState = RollOverStateStart;
                }
                else
                {
                    iRollOverState = RollOverStateIdle;
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFFailure);
                }
            }
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() All QueryInterface() commands complete so AddDataSource is complete"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() %d QueryInterface() commands are still pending", iNumPendingNodeCmd));
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryInterfaceOptional() Out"));
}

void PVPlayerEngine::HandleSourceNodeInit(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSourceNodeInit() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInit() In"));

    iRollOverState = RollOverStateIdle;
    iCommandCompleteInEngineAOPending = false;
    PVMFStatus cmdstatus;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeInit() Status= %s", PVMFStatusToString(aNodeResp.GetCmdStatus())));

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Try to retrieve the duration from the source node via metadata IF
            // Only if we din't got that value through PVMFInfoDurationAvailable informational event.
            if (!iSourceDurationAvailable)
            {
                if (DoSourceNodeGetDurationValue(aNodeContext.iCmdId, aNodeContext.iCmdContext) != PVMFSuccess)
                {
                    // Duration could not be retrieved.
                    // Not an error so complete so engine's Init()
                    SetEngineState(PVP_ENGINE_STATE_INITIALIZED);
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
                }
            }
            else
            {
                // Duration is already available through Info event.
                // so complete  engine's Init()
                SetEngineState(PVP_ENGINE_STATE_INITIALIZED);
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
            }
        }
        break;


        case PVMFErrLicenseRequired:
        case PVMFErrHTTPAuthenticationRequired:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInit() PVMFErrLicenseRequired/PVMFErrHTTPAuthenticationRequired"));

            HandleErrorBasedOnPlayerState();
            cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceInit, puuid, nextmsg));

            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg), aNodeResp.GetEventData());
            errmsg->removeRef();

        }
        break;
        case PVMFErrContentInvalidForProgressivePlayback:
        {
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR, NULL, NULL, NULL, false);

            cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceInit, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg), aNodeResp.GetEventData());
            errmsg->removeRef();
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            // idle state implies that init sequence, including any rollovers are complete
            // report command complete
            if (iState == PVP_ENGINE_STATE_IDLE)
            {
                if (CheckForSourceRollOver() == false)
                {
                    cmdstatus = aNodeResp.GetCmdStatus();

                    PVMFErrorInfoMessageInterface* nextmsg = NULL;
                    if (aNodeResp.GetEventExtensionInterface())
                    {
                        nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
                    }

                    PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
                    PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceInit, puuid, nextmsg));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg), aNodeResp.GetEventData());
                    errmsg->removeRef();
                }
                else
                {
                    // Initialization of source node failed so try alternates
                    //reschedule to do source node roll over
                    iRollOverState = RollOverStateStart;
                    //remove any queued up auto-pause/auto-resume commands
                    //they are no longer applicable since we are doing a change of sourcenode
                    removeCmdFromQ(iPendingCmds, PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW, true);
                    removeCmdFromQ(iPendingCmds, PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY, true);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeInit() - Rescheduling to do source roll over"));
                    RunIfNotReady();
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeInit() - Incorrect State - Asserting"));
                OSCL_ASSERT(false);
            }
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInit() Out"));
}


void PVPlayerEngine::HandleSourceNodeGetDurationValue(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() In"));

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Extract the duration and save it
            // Check that there is one KVP in value list
            if (iSourceDurationValueList.size() != 1)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Value list size is not 1 (size=%d)",
                                iSourceDurationValueList.size()));
                break;
            }

            // Check that the key in KVP is not NULL
            if (iSourceDurationValueList[0].key == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Value list key string is NULL"));
                break;
            }

            // Check that value is for duration
            int retval = pv_mime_strstr(iSourceDurationValueList[0].key, (char*)_STRLIT_CHAR("duration"));
            if (retval == -1)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Key string does not contain duration"));
                break;
            }

            // Check that duration value is uint32. If not available assume uint32.
            PvmiKvpValueType durvaltype = GetValTypeFromKeyString(iSourceDurationValueList[0].key);
            if (durvaltype != PVMI_KVPVALTYPE_UINT32 && durvaltype != PVMI_KVPVALTYPE_UNKNOWN)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Value type is not uint32 or unknown"));
                break;
            }
            iSourceDurationInMS = iSourceDurationValueList[0].value.uint32_value;

            // Check the timescale. If not available, assume millisecond (1000)
            char* retsubstr = NULL;
            uint32 retsubstrlen = 0;
            uint32 tsparamlen = oscl_strlen(_STRLIT_CHAR("timescale="));
            retsubstr = oscl_strstr(iSourceDurationValueList[0].key, _STRLIT_CHAR("timescale="));
            if (retsubstr != NULL)
            {
                retsubstrlen = oscl_strlen(retsubstr);
                if (retsubstrlen > tsparamlen)
                {
                    uint32 timescale = 0;
                    PV_atoi((char*)(retsubstr + tsparamlen), 'd', (retsubstrlen - tsparamlen), timescale);
                    if (timescale > 0 && timescale != 1000)
                    {
                        // Convert to milliseconds
                        MediaClockConverter mcc(timescale);
                        mcc.update_clock(iSourceDurationInMS);
                        iSourceDurationInMS = mcc.get_converted_ts(1000);
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Timescale for duration is %d",
                                        timescale));
                    }
                }
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Duration in millisec is %d",
                            iSourceDurationInMS));
            iSourceDurationAvailable = true;
        }
        break;

        default:
        {
            // Duration is not available
            // Do nothing
        }
        break;
    }

    // Release any metadata values back to source node
    // and then clear it
    if (iSourceDurationValueList.empty() == false)
    {
        OSCL_ASSERT(iSourceNodeMetadataExtIF != NULL);
        iSourceNodeMetadataExtIF->ReleaseNodeMetadataValues(iSourceDurationValueList, 0, iSourceDurationValueList.size());
        iSourceDurationValueList.clear();
    }

    // Engine's Init() is now complete
    SetEngineState(PVP_ENGINE_STATE_INITIALIZED);
    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeGetDurationValue() Out"));
}

void PVPlayerEngine::HandleSourceNodeGetDlaData(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() In"));

    OSCL_UNUSED_ARG(aNodeContext);

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Extract the DLA Data and save it
            // Check that there is one KVP in value list
            if (iDlaDataValueList.size() != 1)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Value list size is not 1 (size=%d)",
                                iDlaDataValueList.size()));
                break;
            }

            // Check that the key in KVP is not NULL
            if (iDlaDataValueList[0].key == NULL)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Value list key string is NULL"));
                break;
            }

            // Check that value is for DLA Data
            int retval = pv_mime_strstr(iDlaDataValueList[0].key, (char*)_STRLIT_CHAR("drm/dla-data"));
            if (retval == -1)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Key string does not contain dla data"));
                break;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Key string does contain dla data (size=%d)",
                                iDlaDataValueList[0].length));
            }

            // Check that Dla Data value is uint8ptr.
            PvmiKvpValueType durvaltype = GetValTypeFromKeyString(iDlaDataValueList[0].key);
            if (durvaltype != PVMI_KVPVALTYPE_UINT8PTR && durvaltype != PVMI_KVPVALTYPE_UNKNOWN)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Value type is not PVMI_KVPVALTYPE_UINT8PTR or unknown"));
                break;
            }

            iCPMAcquireLicenseParam.iLicenseData = iDlaDataValueList[0].value.pUint8_value;
            iCPMAcquireLicenseParam.iLicenseDataSize = iDlaDataValueList[0].length;
        }
        break;

        default:
        {
            // DLA Data is not available.
            // Most license license acquire will fail, but continue
            // anyway.
            iCPMAcquireLicenseParam.iLicenseData = NULL;
            iCPMAcquireLicenseParam.iLicenseDataSize = 0;

        }
        break;
    }

    //Create the plugin and query for the license IF

    PVMFStatus status = DoGetPluginLicenseIF();
    if (status != PVMFSuccess)
    {
        //Trigger the AO to complete the engine command

        // There shouldn't be any other command pending to complete in engine's AO
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);

        iCommandCompleteInEngineAOPending = true;
        iCommandCompleteInEngineAOCmdStatus = status;
        iCommandCompleteInEngineAOErrMsg = NULL;

        if (iCommandCompleteInEngineAOPending)
        {
            if (IsBusy())
            {
                Cancel();
            }
            RunIfNotReady();
        }
    }
    //else wait on CPMPluginCommandCompleted.

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeGetDlaData() Out"));
}

void PVPlayerEngine::HandleSourceNodeSetDataSourceRate(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceRate() In"));

    PVMFStatus cmdstatus = aNodeResp.GetCmdStatus();

    if (cmdstatus != PVMFSuccess)
    {
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
    }
    else
    {
        //Continue on to sink rate change.
        cmdstatus = DoSinkNodeChangeClockRate();
        if (cmdstatus != PVMFSuccess)
        {
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
        }
        else
        {
            //Rate Change is complete.

            //Install the updated rate and timebase.
            UpdateTimebaseAndRate();

            //Start direction change sequence if needed.
            if (iPlaybackDirection_New != iPlaybackDirection)
            {
                cmdstatus = UpdateCurrentDirection(aNodeContext.iCmdId, aNodeContext.iCmdContext);
                switch (cmdstatus)
                {
                    case PVMFPending:
                        //wait on node command completion and call to HandleSourceNodeSetDataSourceDirection
                        break;
                    case PVMFSuccess:
                        //engine command is done, but direction is not actually set on the
                        //source until the Resume or Prepare happens.
                        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
                        break;
                    default:
                        //failed!
                        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
                        break;
                }
            }
            else
            {
                //SetPlaybackRate is complete!
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceRate() Out"));
}

PVMFStatus PVPlayerEngine::DoSinkNodeChangeClockRate()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeChangeClockRate() In"));

    // Check with sink nodes
    PVMFStatus cmdstatus = PVMFSuccess;

    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
        {
            cmdstatus = iDatapathList[i].iSinkNodeSyncCtrlIF->ChangeClockRate(iPlaybackClockRate_New);

            if (cmdstatus != PVMFSuccess)
            {
                // One of the sinks reported not supported so don't allow the clock rate change.
                break;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoSinkNodeChangeClockRate() Out"));
    return cmdstatus;
}

void PVPlayerEngine::UpdateDirection(PVMFTimestamp aNPT, PVMFTimestamp aMediaTS, PVPPlaybackPosition& aPos)
{
    //First note the current observed NPT value.  We will reposition to this
    //value.
    PVPPlaybackPosition curpos;
    curpos.iIndeterminate = false;
    curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    GetPlaybackClockPosition(curpos);
    aPos = curpos;

    //Install the new value for direction.
    iPlaybackDirection = iPlaybackDirection_New;

    //Save the start NPT and TS
    iStartNPT = aNPT;
    iStartMediaDataTS = aMediaTS;

    if (iPlaybackDirection_New < 0)
    {
        if (aPos.iPosValue.millisec_value >= iSourceDurationInMS)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::UpdateDirection() Current pos %dms is more than Duration %dms", aPos.iPosValue.millisec_value, iSourceDurationInMS));
            if (ConvertFromMillisec((uint32)(iSourceDurationInMS - 1), aPos) != PVMFSuccess)
            {
                // Other position units are not supported yet
                aPos.iIndeterminate = true;
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::UpdateDirection() Direction %d New Start NPT %d Start Media Data TS %d, Repos NPT %d"
                     , iPlaybackDirection
                     , iStartNPT
                     , iStartMediaDataTS
                     , (aPos.iIndeterminate) ? -1 : aPos.iPosValue.millisec_value));
}

void PVPlayerEngine::UpdateTimebaseAndRate()
{
    if (iPlaybackClockRate_New == iPlaybackClockRate
            && iOutsideTimebase_New == iOutsideTimebase)
        return;//no update needed

    //Install the new values for rate & timebase.
    iPlaybackClockRate = iPlaybackClockRate_New;
    iOutsideTimebase = iOutsideTimebase_New;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::UpdateTimebaseAndRate() Rate %d OutsideTB 0x%x CurDir %d NewDir %d"
                     , iPlaybackClockRate, iOutsideTimebase
                     , iPlaybackDirection, iPlaybackDirection_New));

    // Pause the clock if running. If already stopped or paused, the call would fail
    bool clockpaused = iPlaybackClock.Pause();

    if (iOutsideTimebase)
    {
        //use the outside timebase & ignore the rate.
        iPlaybackClock.SetClockTimebase(*iOutsideTimebase);
    }
    else
    {
        //use the player timebase and set the rate.
        iPlaybackClock.SetClockTimebase(iPlaybackTimebase);
        iPlaybackTimebase.SetRate(iPlaybackClockRate);
    }

    // Only restart the clock if the clock was paused in this function
    if (clockpaused)
    {
        iPlaybackClock.Start();
    }
}

void PVPlayerEngine::HandleSourceNodePrepare(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSourceNodePrepare() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodePrepare() In"));

    PVMFStatus cmdstatus = PVMFErrNotSupported;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Initiate the setup sequence for each datapath
            iNumPendingDatapathCmd = 0;
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackInfo != NULL)
                {
                    PVMFStatus retcode = DoSetupSinkNode(iDatapathList[i], aNodeContext.iCmdId, aNodeContext.iCmdContext);
                    if (retcode == PVMFSuccess)
                    {
                        ++iNumPendingDatapathCmd;
                        cmdstatus = PVMFSuccess;
                    }
                    else
                    {
                        cmdstatus = retcode;
                    }
                }
            }

            if (iNumPendingDatapathCmd == 0)
            {
                if (cmdstatus == PVMFErrNotSupported)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodePrepare() No datapath could be setup. Asserting"));
                    OSCL_ASSERT(false);

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodePrepare() Report command as failed"));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFFailure);
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodePrepare() Report command as failed"));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
                }
            }
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodePrepare() Out"));
}


void PVPlayerEngine::HandleSinkNodeQueryFileOutConfigIF(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSinkNodeQueryFileOutConfigIF() Tick=%d", OsclTickCount::TickCount()));

    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeQueryFileOutConfigIF() In %s", aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    PVMFStatus cmdstatus = PVMFErrNotSupported;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
            OSCL_ASSERT(aNodeContext.iEngineDatapath->iDataSink != NULL);

            // Set the filename
            aNodeContext.iEngineDatapath->iSinkNodeFOConfigIF->SetOutputFileName(aNodeContext.iEngineDatapath->iDataSink->GetDataSinkFilename());

            // Query the sink node for optional ext IFs
            cmdstatus = DoSinkNodeQueryInterfaceOptional(*(aNodeContext.iEngineDatapath), aNodeContext.iCmdId, aNodeContext.iCmdContext);
            if (cmdstatus == PVMFErrNotSupported)
            {
                // Check if a dec node is needed
                cmdstatus = DoSetupDecNode(*(aNodeContext.iEngineDatapath), aNodeContext.iCmdId, aNodeContext.iCmdContext);
                if (cmdstatus == PVMFErrNotSupported)
                {
                    // Continue to prepare the datapath
                    cmdstatus = DoDatapathPrepare(*(aNodeContext.iEngineDatapath), aNodeContext.iCmdId, aNodeContext.iCmdContext);
                }
            }

            if (cmdstatus != PVMFSuccess)
            {
                HandleErrorBasedOnPlayerState();

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeQueryFileOutConfigIF() Report command as failed"));
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

                --iNumPendingDatapathCmd;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkInit, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeQueryFileOutConfigIF() Out"));
}


void PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Tick=%d", OsclTickCount::TickCount()));

    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() In %s", aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Determine QueryInterface() for which interface completed
    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    if (aNodeContext.iCmdType == PVP_CMD_SinkNodeQuerySyncCtrlIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF)
        {
            // Pass the playback clock to the sync control
            aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF->SetClock(&iPlaybackClock);
            // Set the sync margin based on media type
            switch (aNodeContext.iEngineDatapath->iMediaType)
            {
                case PVP_MEDIATYPE_AUDIO:
                    aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF->SetMargins((-1*iSyncMarginAudio.min), iSyncMarginAudio.max);
                    break;

                case PVP_MEDIATYPE_TEXT:
                    aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF->SetMargins((-1*iSyncMarginText.min), iSyncMarginText.max);
                    break;

                case PVP_MEDIATYPE_VIDEO:
                default:	// Use video's sync margin for unknown media type
                    aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF->SetMargins((-1*iSyncMarginVideo.min), iSyncMarginVideo.max);
                    break;
            }
        }
        else
        {
            // Metadata is not available in this sink node
            aNodeContext.iEngineDatapath->iSinkNodeMetadataExtIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Metadata IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SinkNodeQueryMetadataIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && aNodeContext.iEngineDatapath->iSinkNodeMetadataExtIF)
        {
            // Add the video sink node's metadata extension IF to the list
            if (AddToMetadataInterfaceList(aNodeContext.iEngineDatapath->iSinkNodeMetadataExtIF, aNodeContext.iEngineDatapath->iSinkNodeSessionId) != PVMFSuccess)
            {
                aNodeContext.iEngineDatapath->iSinkNodeMetadataExtIF->removeRef();
                aNodeContext.iEngineDatapath->iSinkNodeMetadataExtIF = NULL;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Metadata IF could not be added to list"));
            }
        }
        else
        {
            // Metadata is not available in this video sink node
            aNodeContext.iEngineDatapath->iSinkNodeMetadataExtIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Metadata IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_SinkNodeQueryCapConfigIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && aNodeContext.iEngineDatapath->iSinkNodeCapConfigIF)
        {
            // Nothing to do here
        }
        else
        {
            // Cap-config is not available
            aNodeContext.iEngineDatapath->iSinkNodeCapConfigIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Cap-Config IF not available"));
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Unknown cmd type. Asserting"));
        OSCL_ASSERT(false);
    }

    // Decrement the pending counter and go to next step if 0.
    --aNodeContext.iEngineDatapath->iNumPendingCmd;
    if (aNodeContext.iEngineDatapath->iNumPendingCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() All QueryInterface() commands complete"));

        // Create the decoder node if necessary
        PVMFStatus cmdstatus = DoSetupDecNode(*(aNodeContext.iEngineDatapath), aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (cmdstatus == PVMFErrNotSupported)
        {
            cmdstatus = DoDatapathPrepare(*(aNodeContext.iEngineDatapath), aNodeContext.iCmdId, aNodeContext.iCmdContext);
        }

        if (cmdstatus != PVMFSuccess)
        {
            HandleErrorBasedOnPlayerState();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeQueryInterfaceOptional() Out"));
}

void PVPlayerEngine::HandleSinkNodeDecNodeQueryCapConfigIF(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSinkNodeDecNodeQueryCapConfigIF() Tick=%d", OsclTickCount::TickCount()));

    OSCL_UNUSED_ARG(aNodeResp);

    if (aNodeContext.iCmdType != PVP_CMD_SinkNodeDecNodeQueryCapConfigIF)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeQueryCapConfigIF() Unknown cmd type. Asserting"));
        OSCL_ASSERT(false);
    }
    // Decrement the pending counter and go to next step if 0.
    OSCL_ASSERT(iNumPendingNodeCmd > 0);
    --iNumPendingNodeCmd;
    if (iNumPendingNodeCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeDecNodeQueryCapConfigIF() All QueryInterface() commands complete"));

        PVMFStatus cmdstatus = DoSinkNodeInit(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            HandleErrorBasedOnPlayerState();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeQueryCapConfigIF() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeDecNodeQueryCapConfigIF() Out"));

}

void PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter() Tick=%d", OsclTickCount::TickCount()));

    OSCL_UNUSED_ARG(aNodeResp);

    if (aNodeContext.iCmdType != PVP_CMD_SinkNodeDecNodeVerifyParameter)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter() Unknown cmd type. Asserting"));
        OSCL_ASSERT(false);
    }

    // Decrement the pending counter and go to next step if 0.
    OSCL_ASSERT(iNumPendingNodeCmd > 0);
    --iNumPendingNodeCmd;
    if (iNumPendingNodeCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter() All Sinknode Init() commands complete"));
        PVMFStatus cmdstatus = PVMFFailure;

        // Select tracks based on available engine datapaths
        cmdstatus = DoSourceNodeTrackSelection(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            HandleErrorBasedOnPlayerState();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
            return;
        }

        // Reset SinkNode and MIO on available engine datapaths
        cmdstatus = DoSinkNodeReset(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            HandleErrorBasedOnPlayerState();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeDecNodeVerifyParameter() Out"));
}

void PVPlayerEngine::HandleSinkNodeDecNodeReset(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Tick=%d", OsclTickCount::TickCount()));

    OSCL_UNUSED_ARG(aNodeResp);

    if (aNodeContext.iCmdType != PVP_CMD_SinkNodeDecNodeReset)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Unknown cmd type. Asserting"));
        OSCL_ASSERT(false);
    }

    // Decrement the pending counter and go to next step if 0.
    OSCL_ASSERT(iNumPendingNodeCmd > 0);
    --iNumPendingNodeCmd;
    if (iNumPendingNodeCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() All Sinknode Reset() commands complete"));

        //Destroy created temporal decoder node and sink node
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iDecNode != NULL)
            {
                if (iDatapathList[i].iDecNodeCapConfigIF == NULL)
                    iDatapathList[i].iDecNodeCapConfigIF = NULL;
                iDatapathList[i].iDecNode->Disconnect(iDatapathList[i].iDecNodeSessionId);
                iDatapathList[i].iDecNode->ThreadLogoff();
                PVPlayerEngineUuidNodeMapping* iter = iNodeUuids.begin();
                for (; iter != iNodeUuids.end(); ++iter)
                    if (iter->iNode == iDatapathList[i].iDecNode)
                        break;

                if (iter != iNodeUuids.end())
                {
                    bool release_status = false;

                    int32 leavecode = 0;
                    OSCL_TRY(leavecode, release_status = iPlayerNodeRegistry.ReleaseNode(iter->iUuid, iDatapathList[i].iDecNode));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Error in releasing DecNode"));
                                         return;);

                    if (release_status == false)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Factory returned false while releasing the decnode"));
                        return;
                    }

                    iNodeUuids.erase(iter);
                    iDatapathList[i].iDecNode = NULL;
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() decnode not found"));
                    return;
                }
            }
            if (iDatapathList[i].iSinkNode != NULL)
            {
                iDatapathList[i].iSinkNode->Disconnect(iDatapathList[i].iSinkNodeSessionId);
                iDatapathList[i].iSinkNode->ThreadLogoff();
                if (iDatapathList[i].iSinkNodeCapConfigIF == NULL)
                    iDatapathList[i].iSinkNodeCapConfigIF = NULL;
                if (iDatapathList[i].iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_FILENAME)
                {
                    PVFileOutputNodeFactory::DeleteFileOutput(iDatapathList[i].iSinkNode);
                    iDatapathList[i].iSinkNode = NULL;
                }
                else if (iDatapathList[i].iDataSink->GetDataSinkType() == PVP_DATASINKTYPE_SINKNODE)
                {
                    iDatapathList[i].iSinkNode = NULL;
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Unsupported player data sink type"));
                    OSCL_ASSERT(false);
                    return;
                }
            }
        }

        // Prepare the source node
        PVMFStatus cmdstatus = DoSourceNodePrepare(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            HandleErrorBasedOnPlayerState();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
            return;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeDecNodeReset() Out"));
}

void PVPlayerEngine::HandleDecNodeQueryInterfaceOptional(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Tick=%d", OsclTickCount::TickCount()));

    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() In %s", aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Determine QueryInterface() for which interface completed
    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    if (aNodeContext.iCmdType == PVP_CMD_DecNodeQueryMetadataIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && aNodeContext.iEngineDatapath->iDecNodeMetadataExtIF)
        {
            // Add the video dec node's metadata extension IF to the list
            if (AddToMetadataInterfaceList(aNodeContext.iEngineDatapath->iDecNodeMetadataExtIF, aNodeContext.iEngineDatapath->iDecNodeSessionId) != PVMFSuccess)
            {
                aNodeContext.iEngineDatapath->iDecNodeMetadataExtIF->removeRef();
                aNodeContext.iEngineDatapath->iDecNodeMetadataExtIF = NULL;

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Metadata IF could not be added to list"));
            }
        }
        else
        {
            // Metadata is not available in this dec node
            aNodeContext.iEngineDatapath->iDecNodeMetadataExtIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Metadata IF not available"));
        }
    }
    else if (aNodeContext.iCmdType == PVP_CMD_DecNodeQueryCapConfigIF)
    {
        if (aNodeResp.GetCmdStatus() == PVMFSuccess && aNodeContext.iEngineDatapath->iDecNodeCapConfigIF)
        {
            // Configure the dec node for player use
            PvmiKvp kvpparam;
            PvmiKvp* retkvp = NULL;
            OSCL_StackString<64> kvpparamkey;

            switch (aNodeContext.iEngineDatapath->iMediaType)
            {
                case PVP_MEDIATYPE_VIDEO:
                    // Disable drop frame mode
                    kvpparamkey = _STRLIT_CHAR("x-pvmf/video/decoder/dropframe_enable;valtype=bool");
                    kvpparam.value.bool_value = false;
                    break;

                case PVP_MEDIATYPE_AUDIO:
                    // Disable silence insertion
                    kvpparamkey = _STRLIT_CHAR("x-pvmf/audio/decoder/silenceinsertion_enable;valtype=bool");
                    kvpparam.value.bool_value = false;
                    break;

                default:
                    break;
            }

            if (kvpparamkey.get_size() > 0)
            {
                kvpparam.key = kvpparamkey.get_str();
                aNodeContext.iEngineDatapath->iDecNodeCapConfigIF->setParametersSync(NULL, &kvpparam, 1, retkvp);
                if (retkvp != NULL)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Configuring dec node for player use via cap-config IF failed"));
                }
            }
        }
        else
        {
            // Cap-config is not available
            aNodeContext.iEngineDatapath->iDecNodeCapConfigIF = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_NOTICE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Cap-Config IF not available"));
        }
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Unknown cmd type. Asserting"));
        OSCL_ASSERT(false);
    }

    // Decrement the pending counter and go to next step if 0.
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iNumPendingCmd > 0);
    --aNodeContext.iEngineDatapath->iNumPendingCmd;
    if (aNodeContext.iEngineDatapath->iNumPendingCmd == 0)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() All QueryInterface() commands complete"));

        // Prepare the datapath
        PVMFStatus cmdstatus = DoDatapathPrepare(*(aNodeContext.iEngineDatapath), aNodeContext.iCmdId, aNodeContext.iCmdContext);

        if (cmdstatus != PVMFSuccess)
        {
            HandleErrorBasedOnPlayerState();

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeQueryInterfaceOptional() Out"));
}


void PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() In"));

    if (aNodeResp.GetCmdStatus() == PVMFErrNotSupported || aNodeResp.GetCmdStatus() == PVMFErrArgument)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() QueryDataSourcePosition failed. Assume position goes to requested position"));
        iActualPlaybackPosition = iCurrentBeginPosition.iPosValue.millisec_value;
    }
    else if (aNodeResp.GetCmdStatus() != PVMFSuccess)
    {
        // If not unsupported error, then assume fatal error from source node
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();
        return;
    }
    else
    {
        // Every thing is OK.. Calculate the iActualPlaybackPosition depensing upon nearest before and after syncPoints.
        //  For MPEG4 files

        PVMFNodeCapability nodeCapability;
        iSourceNode->GetCapability(nodeCapability);
        PVMFFormatType * formatType = nodeCapability.iInputFormatCapability.begin();
        bool mpeg4FormatType = false;
        if (formatType != NULL)
        {
            switch (*formatType)
            {
                case PVMF_MPEG4FF:
                    mpeg4FormatType = true;
                    break;

                default:
                    mpeg4FormatType = false;
                    break;
            }
        }

        if (mpeg4FormatType)
        {
            CalculateActualPlaybackPosition();
        }
    }

    // Determine the SetDataSourcePosition parameter based on query result and reposition settings
    PVMFTimestamp requesttime = iCurrentBeginPosition.iPosValue.millisec_value;
    bool seektosyncpt = true;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,
                    "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition()"
                    "Requested NPT %d, Actual NPT %d", requesttime, iActualPlaybackPosition));

    uint32 startOfSeekWindow = 0;
    if (iCurrentBeginPosition.iPosValue.millisec_value > iSyncPointSeekWindow)
    {
        startOfSeekWindow = (iCurrentBeginPosition.iPosValue.millisec_value - iSyncPointSeekWindow);
    }
    uint32 endOfSeekWindow = iCurrentBeginPosition.iPosValue.millisec_value + iSyncPointSeekWindow;

    // 1) Check if the actual seek point falls within the window
    if ((iActualPlaybackPosition >= startOfSeekWindow) &&
            (iActualPlaybackPosition <= endOfSeekWindow))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0,
                        "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() - "
                        "RequestedNPT(%d) ActualNPT(%d) is in the window (%d, %d), Seeking To %d",
                        requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow, iActualPlaybackPosition));

        requesttime = iActualPlaybackPosition;
    }
    else
    {
        // 1) Check if the actual seek point is before the window start, then set the
        // request time to start of the window
        // 2) Check if the actual seek point is after the window end, then
        // go back to start of the seek window
        // SFR is not really an option here since we are not playing yet, therefore always
        // go to start of the window
        if ((iActualPlaybackPosition < startOfSeekWindow) ||
                (iActualPlaybackPosition > endOfSeekWindow))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0,
                            "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() - "
                            "RequestedNPT(%d) ActualNPT(%d) is outside the window (%d, %d), Seeking To %d Seek-To-SyncPt False",
                            requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow, startOfSeekWindow));

            requesttime = startOfSeekWindow;
            seektosyncpt = false;
            iActualPlaybackPosition = requesttime;
        }
        else
        {
            //error
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0,
                            "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() - "
                            "RequestedNPT(%d) ActualNPT(%d) window (%d, %d), Error Condition Asserting",
                            requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow));
            OSCL_ASSERT(false);
        }
    }

    // Do the source positioning
    PVMFStatus retval = DoSourceNodeSetDataSourcePosition(aNodeContext.iCmdId, aNodeContext.iCmdContext, requesttime, seektosyncpt);
    if (retval != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() Report command as failed"));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, retval);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePosition() Out"));
}


void PVPlayerEngine::HandleSourceNodeSetDataSourcePosition(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() In"));

    PVMFStatus cmdstatus = PVMFFailure;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFErrArgument:
        case PVMFErrNotSupported:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                            (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() Requested begin position(%d ms) is not supported so start from time 0.",
                             iCurrentBeginPosition.iPosValue.millisec_value));
            if (iChangePlaybackPositionWhenResuming)
            {
                PVPPlaybackPosition curpos;
                curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
                GetPlaybackClockPosition(curpos);
                uint32 clockcurpos = 0;
                bool tmpbool = false;
                iPlaybackClock.GetCurrentTime32(clockcurpos, tmpbool, OSCLCLOCK_MSEC);

                // since repositioning is not supported and if the playback position change request was
                // issued during paused state, then continue from paused position.
                iWatchDogTimerInterval = 0;
                iActualPlaybackPosition = curpos.iPosValue.millisec_value;
                iActualMediaDataTS = clockcurpos;
                iAdjustedMediaDataTS = clockcurpos;

                iStartNPT = iActualPlaybackPosition;
                iStartMediaDataTS = iAdjustedMediaDataTS;
            }
            else
            {
                // Since this repositioning was not supported, assume the playback
                // will start from time 0
                iWatchDogTimerInterval = 0;
                iActualPlaybackPosition = 0;
                iActualMediaDataTS = 0;
                iAdjustedMediaDataTS = 0;
                // Then continue to handle like success case
                iStartNPT = 0;
                iStartMediaDataTS = 0;
            }

            // Save the actual starting position for GetPlaybackRange() query
            iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
            iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;
        }
        break;

        case PVMFSuccess:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() SetDataSourcePosition() successful. StartMediaTS %d ms, ActualNPT %d ms, TargetNPT %d ms",
                             iActualMediaDataTS, iActualPlaybackPosition, iCurrentBeginPosition.iPosValue.millisec_value));
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                            (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() SetDataSourcePosition() successful. StartMediaTS %d ms, ActualNPT %d ms, TargetNPT %d ms",
                             iActualMediaDataTS, iActualPlaybackPosition, iCurrentBeginPosition.iPosValue.millisec_value));
            // Compute the difference between actualNPT and targetNPT before any adjustments
            if (iCurrentBeginPosition.iPosValue.millisec_value >= iActualPlaybackPosition)
            {
                iWatchDogTimerInterval = iCurrentBeginPosition.iPosValue.millisec_value - iActualPlaybackPosition;
            }

            // Determine if adjustment needed to skip to requested time
            if (iSkipToRequestedPosition && (iActualPlaybackPosition < iCurrentBeginPosition.iPosValue.millisec_value))
            {
                if (iCurrentBeginPosition.iPosValue.millisec_value - iActualPlaybackPosition > SYNC_POINT_DIFF_THRESHOLD)
                {
                    // Sync point seems to be far away in the stream
                    // Can't adjust the skip time back so use the returned values to skip to
                    iAdjustedMediaDataTS = iActualMediaDataTS;
                    iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
                    iWatchDogTimerInterval = 0;
                }
                else
                {
                    // Adjust the media data time to skip-to to correspond to the requested time
                    // Add the difference of target NPT with actual playback position in NPT to the actual media data time to get time to skip to.
                    iAdjustedMediaDataTS = iActualMediaDataTS + (iCurrentBeginPosition.iPosValue.millisec_value - iActualPlaybackPosition);
                    iActualPlaybackPosition = iCurrentBeginPosition.iPosValue.millisec_value;
                }
            }
            else
            {
                // Can't adjust the skip time back so use the returned values to skip to
                iAdjustedMediaDataTS = iActualMediaDataTS;
                iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
                iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;
                iWatchDogTimerInterval = 0;
            }

            // Save initial NTP and TS values
            iStartNPT = iActualPlaybackPosition;
            iStartMediaDataTS = iAdjustedMediaDataTS;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() After adjustment StartMediaTS %d ms, AdjustedMediaTS %d ms, ActualPBPos %d ms Start NPT %d Start TS %d",
                             iActualMediaDataTS, iAdjustedMediaDataTS, iActualPlaybackPosition, iStartNPT, iStartMediaDataTS));
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                            (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() After adjustment StartMediaTS %d ms, AdjustedMediaTS %d ms, ActualNPT %d ms StartNPT %d StartTS %d",
                             iActualMediaDataTS, iAdjustedMediaDataTS, iActualPlaybackPosition, iStartNPT, iStartMediaDataTS));
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();
            return;
        }
    }

    // Repositioning so reset the EOS flag for each active datapath
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive)
        {
            iDatapathList[i].iEndOfDataReceived = false;
        }
    }

    // Contine on and start the source node
    cmdstatus = DoSourceNodeStart(aNodeContext.iCmdId, aNodeContext.iCmdContext);
    if (cmdstatus != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() Report command as failed"));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() Out"));
}

void PVPlayerEngine::HandleSourceNodeSetDataSourceDirection(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() In"));

    if (iChangePlaybackDirectionWhenResuming)
    {
        // Continuation of Engine Resume sequence.

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Context RESUME"));
        PVMFStatus cmdstatus = PVMFFailure;

        switch (aNodeResp.GetCmdStatus())
        {
            case PVMFErrArgument:
            case PVMFErrNotSupported:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR,
                                (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Requested direction is not supported!"));
            }
            break;

            case PVMFSuccess:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                                (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() SetDataSourceDirection() successful. StartMediaTS %d ms, ActualPBPos %d ms",
                                 iActualMediaDataTS, iActualPlaybackPosition));

                //there's no adjustment to the media TS here.
                iAdjustedMediaDataTS = iActualMediaDataTS;
                iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
                iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

                //Install the new direction and get the repositioning target.
                UpdateDirection(iActualPlaybackPosition, iAdjustedMediaDataTS, iChangeDirectionNPT);

                //Reposition the source to the desired playback time
                if (!iChangeDirectionNPT.iIndeterminate)
                {
                    iChangePlaybackDirectionWhenResuming = false;
                    iChangePlaybackPositionWhenResuming = true;
                    PVPlayerEngineCommand cmd(0, aNodeContext.iCmdId, aNodeContext.iCmdContext, NULL, false);
                    iCurrentBeginPosition = iChangeDirectionNPT;
                    PVMFStatus retval = UpdateCurrentBeginPosition(iCurrentBeginPosition, cmd);
                    if (retval == PVMFPending)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Repos to %d started", iChangeDirectionNPT.iPosValue));
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Out"));
                        return;//wait on the repos sequence...
                    }
                    else if (retval != PVMFSuccess)
                    {
                        //else can't reposition, ignore failure and continue.
                        iChangeDirectionNPT.iIndeterminate = true;
                        iChangePlaybackPositionWhenResuming = false;
                        //need to leave the flag set for later in HandleDatapathResume,
                        //to trigger the skip media data.
                        iChangePlaybackDirectionWhenResuming = true;
                    }
                }
            }
            break;

            default:
            {
                HandleErrorBasedOnPlayerState();

                cmdstatus = aNodeResp.GetCmdStatus();

                PVMFErrorInfoMessageInterface* nextmsg = NULL;
                if (aNodeResp.GetEventExtensionInterface())
                {
                    nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
                }

                PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
                PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
                errmsg->removeRef();
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Out"));
                return;
            }
        }


        // Repositioning so reset the EOS flag for each active datapath
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive)
            {
                iDatapathList[i].iEndOfDataReceived = false;
            }
        }

        // Start the source node.
        cmdstatus = DoSourceNodeStart(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePosition() Report command as failed"));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
        }
    }
    else
    {
        //Continuation of SetPlaybackRate sequence.

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Context SETPLAYBACKRATE"));

        if (aNodeResp.GetCmdStatus() != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() SetDataSourceDirection failed. Playback position change has been cancelled"));

            if (aNodeResp.GetCmdStatus() == PVMFErrNotSupported || aNodeResp.GetCmdStatus() == PVMFErrArgument)
            {
                // For non-fatal error, continue playback by resuming the clock
                iPlaybackClock.Start();
                // To get regular play status events
                StartPlaybackStatusTimer();
            }
            else
            {
                // Initiate error handling
                HandleErrorBasedOnPlayerState();
            }

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSource, puuid, nextmsg));

            // Complete the SetPlaybackRate() command as failed
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));

            // Report an error event here but do nothing to the playback
            SendErrorEvent(aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));

            errmsg->removeRef();
            errmsg = NULL;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Out"));
            return;
        }

        // no adjustement here.
        iAdjustedMediaDataTS = iActualMediaDataTS;
        iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
        iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;

        //Install the new direction and get the repositioning target
        UpdateDirection(iActualPlaybackPosition, iAdjustedMediaDataTS, iChangeDirectionNPT);

        //Launch a repositioning sequence now.
        if (!iChangeDirectionNPT.iIndeterminate)
        {
            PVPlayerEngineCommand cmd(0, aNodeContext.iCmdId, aNodeContext.iCmdContext, NULL, false);
            iCurrentBeginPosition = iChangeDirectionNPT;
            PVMFStatus retval = UpdateCurrentBeginPosition(iCurrentBeginPosition, cmd);
            if (retval == PVMFPending)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Repos to %d started", iChangeDirectionNPT.iPosValue));
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Out"));
                return;//wait on the repos sequence...
            }
            else if (retval != PVMFSuccess)
            {
                //the direction is already changed, so just ignore this failure and continue
                iChangeDirectionNPT.iIndeterminate = true;
            }
        }

        // Repositioning so reset the EOS flag for each active datapath
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive)
            {
                iDatapathList[i].iEndOfDataReceived = false;
            }
        }

        // Skip to the new source node position, so that all the data that was queued
        // when the command was received will get flushed.

        PVMFStatus retval = DoSinkNodeSkipMediaDataDuringPlayback(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Skipping media data request in sink nodes failed. Repositioning did not complete."));
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSink, puuid, NULL));

            //clear the pending direction change NPT.
            iChangeDirectionNPT.iIndeterminate = true;

            // Complete the SetPlaybackRate() command as failed
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, retval, OSCL_STATIC_CAST(PVInterface*, errmsg));

            // Report an error event
            SendErrorEvent(retval, OSCL_STATIC_CAST(PVInterface*, errmsg));

            errmsg->removeRef();
            errmsg = NULL;
        }
        // else wait on HandleSinkNodeSkipMediaDataDuringPlayback.
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourceDirection() Out"));
}

void PVPlayerEngine::HandleSourceNodeStart(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSourceNodeStart() Tick=%d", OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStart() In"));

    PVMFStatus cmdstatus = PVMFErrNotSupported;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Start the available datapaths
            iNumPendingDatapathCmd = 0;
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    PVMFStatus retval = DoDatapathStart(iDatapathList[i], aNodeContext.iCmdId, aNodeContext.iCmdContext);
                    if (retval == PVMFSuccess)
                    {
                        ++iNumPendingDatapathCmd;
                        cmdstatus = PVMFSuccess;
                    }
                    else
                    {
                        cmdstatus = retval;
                        break;
                    }
                }
            }

            if (iNumPendingDatapathCmd == 0)
            {
                if (cmdstatus == PVMFErrNotSupported)
                {
                    // There are no active datapaths. Assert.
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeStart() No datapath exists. Asserting"));
                    OSCL_ASSERT(false);

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeStart() Report command as failed"));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFFailure);
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeStart() Report command as failed"));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
                }
            }
        }

        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStart() Out"));
}


void PVPlayerEngine::HandleSinkNodeSkipMediaData(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() for %s Tick=%d",
                     aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() for %s, iNumPVMFInfoStartOfDataPending=%d",
                     aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), iNumPVMFInfoStartOfDataPending));


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                    (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() In"));

    OSCL_ASSERT(iNumPendingNodeCmd > 0);
    --iNumPendingNodeCmd;

    if (aNodeResp.GetCmdStatus() != PVMFSuccess)
    {
        // Sink node report error with SkipMediaData()
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkFatal, puuid, nextmsg));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();
        return;

    }

    OSCL_ASSERT(iNumPendingSkipCompleteEvent > 0);
    --iNumPendingSkipCompleteEvent;


    if (iNumPendingNodeCmd == 0)
    {
        PVMFTimestamp targetNPT = iCurrentBeginPosition.iPosValue.millisec_value;

        // Set the clock to the specified begin time
        iPlaybackClock.Stop();
        iPlaybackClock.SetStartTime32(iAdjustedMediaDataTS, OSCLCLOCK_MSEC);

        if (!(iWatchDogTimer->IsBusy()))
        {
            PVMFTimestamp targetNPT = iCurrentBeginPosition.iPosValue.millisec_value;
            iWatchDogTimer->Cancel();
            if (iWatchDogTimerInterval > 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                                (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() Setting WatchDogTimer for %d ms, TargetNPT=%d  ActualNPT=%d",
                                 iWatchDogTimerInterval, targetNPT, iActualPlaybackPosition));
                iWatchDogTimer->setTimerDuration(iWatchDogTimerInterval);
                iWatchDogTimer->Start();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                                (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() Skipping WatchDogTimer - Starting PlayBackClock"));
                StartPlaybackClock();
            }
        }
        // Set the actual playback position to the requested time since actual media data TS was adjusted
        // This is important since the difference between the two is used to calculate the NPT to media data offset
        // This is not required here as the ActualPlaybackPosition is already adjusted before calling Skip on Sink Node.
        // iActualPlaybackPosition=iCurrentBeginPosition.iPosValue.millisec_value;

        // Save the start NPT and TS
        iStartNPT = iActualPlaybackPosition;
        iStartMediaDataTS = iAdjustedMediaDataTS;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO,
                        (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() TargetNPT %d, StartNPT %d StartTS %d",
                         targetNPT, iStartNPT, iStartMediaDataTS));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                        (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() TargetNPT %d, StartNPT %d StartTS %d",
                         targetNPT, iStartNPT, iStartMediaDataTS));

        SetEngineState(PVP_ENGINE_STATE_PREPARED);

        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
    }

    if ((iNumPendingSkipCompleteEvent == 0) && (iNumPVMFInfoStartOfDataPending == 0))
    {
        if (iWatchDogTimer->IsBusy())
        {
            iWatchDogTimer->Cancel();
        }
        // we have received all the bos event for
        // playback hasnt started yet
        StartPlaybackClock();
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() - PlayClock Started"));
    }


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeSkipMediaData() Out"));
}


void PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() In"));

    if (aNodeResp.GetCmdStatus() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() QueryDataSourcePosition failed. Assume position goes to requested position"));
        iActualPlaybackPosition = iCurrentBeginPosition.iPosValue.millisec_value;
    }
    else
    {
        // Every thing is OK.. Calculate the iActualPlaybackPosition depensing upon nearest before and after syncPoints.
        //  For MPEG4 files

        PVMFNodeCapability nodeCapability;
        iSourceNode->GetCapability(nodeCapability);
        PVMFFormatType * formatType = nodeCapability.iInputFormatCapability.begin();
        bool mpeg4FormatType = false;
        if (formatType != NULL)
        {
            switch (*formatType)
            {
                case PVMF_MPEG4FF:
                    mpeg4FormatType = true;
                    break;

                default:
                    mpeg4FormatType = false;
                    break;
            }
        }
        if (mpeg4FormatType)
        {
            CalculateActualPlaybackPosition();
        }
    }


    // Determine the SetDataSourcePosition parameter based on query result and reposition settings
    PVMFTimestamp requesttime = iCurrentBeginPosition.iPosValue.millisec_value;
    bool seektosyncpt = true;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,
                    "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback()"
                    "Requested NPT %d, Actual NPT %d", requesttime, iActualPlaybackPosition));

    uint32 startOfSeekWindow = 0;
    if (iCurrentBeginPosition.iPosValue.millisec_value > iSyncPointSeekWindow)
    {
        startOfSeekWindow = (iCurrentBeginPosition.iPosValue.millisec_value - iSyncPointSeekWindow);
    }
    uint32 endOfSeekWindow = iCurrentBeginPosition.iPosValue.millisec_value + iSyncPointSeekWindow;

    PVPPlaybackPosition curpos;
    curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    GetPlaybackClockPosition(curpos);
    bool oSFR = false;

    //depending on whether it is fwd or rwnd, the window is different
    //if doing a rwnd, the worst case window is (0, currentplaybackposition)
    //if doing a fwd, the worst case window is (currentplaybackposition, endofclip)
    if (iCurrentBeginPosition.iPosValue.millisec_value <= curpos.iPosValue.millisec_value)
    {
        //requested pos <= currpos => rwnd
        //cap end of seek window to be the current play back pos
        endOfSeekWindow = curpos.iPosValue.millisec_value;
    }
    if (iCurrentBeginPosition.iPosValue.millisec_value > curpos.iPosValue.millisec_value)
    {
        //requested pos > currpos => fwd
        //cap start of seek window to be the current play back pos
        startOfSeekWindow = curpos.iPosValue.millisec_value;
    }

    // 1) Check if the actual seek point falls within the window
    if ((iActualPlaybackPosition >= startOfSeekWindow) &&
            (iActualPlaybackPosition <= endOfSeekWindow))
    {
        // Check for SFR
        // In case if actual playback position
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0,
                        "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() - "
                        "RequestedNPT(%d) ActualNPT(%d) is in the window (%d, %d), Seeking To %d",
                        requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow, iActualPlaybackPosition));

        requesttime = iActualPlaybackPosition;
    }
    else
    {
        // Check for SFR
        // SFR means currplaybackpos < requestedpos
        if (curpos.iPosValue.millisec_value < iCurrentBeginPosition.iPosValue.millisec_value)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0,
                            "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() - "
                            "CurrNPT(%d) less than RequestedNPT(%d) Ignoring ActualNPT(%d) and the window (%d, %d), Doing SFR",
                            curpos.iPosValue.millisec_value, requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow, startOfSeekWindow));

            oSFR = true;

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* infomsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoAttemptingSFRAsPartOfSetPlayBackRange, puuid, NULL));
            SendInformationalEvent(PVMFInfoPositionStatus, OSCL_STATIC_CAST(PVInterface*, infomsg));
            infomsg->removeRef();

        }
        else
        {
            // if the actual seek point is before the window start,
            // or if the actual seek point is after the window end,
            // then go back to start of the seek window only in case of a finite window
            // in case of infinite window just go to the requested position and do normal
            // repositioning
            if (iSyncPointSeekWindow == 0x7FFFFFFF)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0,
                                "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() - "
                                "RequestedNPT(%d) ActualNPT(%d) is outside the window (%d, %d), Seeking To %d Seek-To-SyncPt True",
                                requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow, startOfSeekWindow));

                seektosyncpt = true;
                iActualPlaybackPosition = requesttime;
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO, (0,
                                "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() - "
                                "RequestedNPT(%d) ActualNPT(%d) is outside the window (%d, %d), Seeking To %d Seek-To-SyncPt False",
                                requesttime, iActualPlaybackPosition, startOfSeekWindow, endOfSeekWindow, startOfSeekWindow));

                requesttime = startOfSeekWindow;
                seektosyncpt = false;
                iActualPlaybackPosition = requesttime;
            }
        }
    }

    if (oSFR)
    {
        // No need to change source position so go to skipping at sink nodes
        // First determine to what time sink nodes should skip to
        // Get current playback clock position in media data time
        uint32 clockcurpos = 0;
        bool tmpbool = false;
        iPlaybackClock.GetCurrentTime32(clockcurpos, tmpbool, OSCLCLOCK_MSEC);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() New source reposition before current position so no need to change source position."));

        if (iSkipToRequestedPosition)
        {
            // Skip to the requested begin position
            // Add the difference of target NPT with current time in NPT to the current clock to get media data time to skip to.
            iActualMediaDataTS = clockcurpos;
            iAdjustedMediaDataTS = (iCurrentBeginPosition.iPosValue.millisec_value - curpos.iPosValue.millisec_value) + clockcurpos;
            iActualPlaybackPosition = requesttime;
            iWatchDogTimerInterval = iCurrentBeginPosition.iPosValue.millisec_value - curpos.iPosValue.millisec_value;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() Skip-to-requested position SET. ActualNPT=%d, ActualMediaTS=%d, AdjustedMediaTS=%d",
                             iActualPlaybackPosition, iActualMediaDataTS, iAdjustedMediaDataTS));
        }
        else
        {
            // Just continue playback from current position
            iActualMediaDataTS = clockcurpos;
            iAdjustedMediaDataTS = clockcurpos;
            iActualPlaybackPosition = curpos.iPosValue.millisec_value;
            iWatchDogTimerInterval = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                            (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() Skip-to-requested position NOT SET so continue playing. ActualNPT=%d, ActualMediaTS=%d, AdjustedMediaTS=%d",
                             iActualPlaybackPosition, iActualMediaDataTS, iAdjustedMediaDataTS));
        }

        PVMFStatus retval = DoSinkNodeSkipMediaDataDuringPlayback(aNodeContext.iCmdId, aNodeContext.iCmdContext, true);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() Skipping media data request in sink nodes failed. Repositioning did not complete."));
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSink, puuid, NULL));

            //clear the pending direction change NPT.
            iChangeDirectionNPT.iIndeterminate = true;

            // Complete the SetPlaybackRange() command as failed
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, retval, OSCL_STATIC_CAST(PVInterface*, errmsg));

            // Report an error event here but do nothing to the playback
            SendErrorEvent(retval, OSCL_STATIC_CAST(PVInterface*, errmsg));

            errmsg->removeRef();
            errmsg = NULL;
        }

    }
    else
    {
        // Do the source positioning
        PVMFStatus retval = DoSourceNodeSetDataSourcePositionDuringPlayback(requesttime, seektosyncpt, aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() SetDataSourcePosition failed. Playback position change has been cancelled"));
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSource, puuid, NULL));

            // Complete the SetPlaybackRange() command as failed
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, retval, OSCL_STATIC_CAST(PVInterface*, errmsg));

            // Report an error event here but do nothing to the playback
            SendErrorEvent(aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));

            errmsg->removeRef();
            errmsg = NULL;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeQueryDataSourcePositionDuringPlayback() Out"));
}

void PVPlayerEngine::CalculateActualPlaybackPosition()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CalculateActualPlaybackPosition In"));

    PVPPlaybackPosition curpos;
    curpos.iPosUnit = PVPPBPOSUNIT_MILLISEC;
    GetPlaybackClockPosition(curpos);

    PVMFTimestamp targetNPT = iCurrentBeginPosition.iPosValue.millisec_value;

    // Following code has been taken from MP4 parser node, all the vars are kept very near to the MP4 parser node.
    // Previously the calculation of before and after sync point was done in MP4 parser node.

    if (curpos.iPosValue.millisec_value > targetNPT)
    {
        // curpos.iPosValue.millisec_value was passed as iActualPlaybackPosition in QueryDataSourcePosition
        // which became aActualNPT while collection, and used to decide forward and reverse repos.
        iBackwardReposFlag = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CalculateActualPlaybackPosition In: Backward Reposition"));
    }
    else
    {
        iForwardReposFlag = true;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CalculateActualPlaybackPosition In: Forward Reposition"));
    }

    // pick the closest time to targetNPT
    uint32 delta = 0;
    uint32 diffBetSeekPointBeforeAndTarget = 0;
    if (IsEarlier(iSeekPointBeforeTargetNPT, targetNPT, delta))
    {
        // this should always be true when checking the SeekPointBefore with
        // targetNPT.
        diffBetSeekPointBeforeAndTarget = delta;
        delta = 0;
    }
    else
    {
        // this will only happen when mp4ff library returns an SeekPointBefore which
        // is after targetNPT with small delta because of some rounding off error in
        // media clock converter class.
        diffBetSeekPointBeforeAndTarget = delta;
        delta = 0;
    }

    uint32 diffBetSeekPointAfterAndTarget = 0;
    if (IsEarlier(targetNPT, iSeekPointAfterTargetNPT, delta))
    {
        // this should always be true when checking the SeekPointAfter with
        // targetNPT.
        diffBetSeekPointAfterAndTarget = delta;
        delta = 0;
    }
    else
    {
        // this should never happen.
        diffBetSeekPointAfterAndTarget = delta;
        delta = 0;
    }

    if (diffBetSeekPointAfterAndTarget < diffBetSeekPointBeforeAndTarget)
    {
        iActualPlaybackPosition = iSeekPointAfterTargetNPT;
    }
    else
    {
        if (iSeekPointBeforeTargetNPT < curpos.iPosValue.millisec_value && iForwardReposFlag)
        {
            iActualPlaybackPosition = iSeekPointAfterTargetNPT;
            iForwardReposFlag = false;
        }
        else
        {
            iActualPlaybackPosition = iSeekPointBeforeTargetNPT;
            iForwardReposFlag = false;
        }
    }
    if (iBackwardReposFlag) // To avoid backwardlooping :: A flag to remember backward repositioning
    {
        iActualPlaybackPosition = iSeekPointBeforeTargetNPT;
        iBackwardReposFlag = false;
    }


    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0,
                    "PVPlayerEngine::CalculateActualPlaybackPosition()"
                    "targetNPT %d Current NPT %d, Actual NPT %d, SeekPointBeforeTargetNPT %d, SeekPointAfterTargetNPT %d ",
                    targetNPT, curpos.iPosValue.millisec_value, iActualPlaybackPosition, iSeekPointBeforeTargetNPT, iSeekPointAfterTargetNPT));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::CalculateActualPlaybackPosition Out"));
}

void PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback() In"));

    if (aNodeResp.GetCmdStatus() != PVMFSuccess)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback() SetDataSourcePosition failed. Playback position change has been cancelled"));

        if (aNodeResp.GetCmdStatus() == PVMFErrNotSupported || aNodeResp.GetCmdStatus() == PVMFErrArgument)
        {
            // For non-fatal error, continue playback by resuming the clock
            iPlaybackClock.Start();

            // To get regular play status events
            StartPlaybackStatusTimer();
        }
        else
        {
            // Initiate error handling
            HandleErrorBasedOnPlayerState();
        }

        // Complete the SetPlaybackRange() command as notsupported / failed
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus());

        return;
    }


    if ((iCurrentBeginPosition.iMode != PVPPBPOS_MODE_END_OF_CURRENT_PLAY_ELEMENT) && (iCurrentBeginPosition.iMode != PVPPBPOS_MODE_END_OF_CURRENT_PLAY_SESSION))
    {
        uint32 targetNPT = iCurrentBeginPosition.iPosValue.millisec_value;
        if (iCurrentBeginPosition.iPosUnit == PVPPBPOSUNIT_PLAYLIST)
        {
            iActualMediaDataTS = iDataSourcePosParams.iActualMediaDataTS;
            iActualPlaybackPosition = iDataSourcePosParams.iActualNPT;
            /* Reset */
            iDataSourcePosParams.iActualMediaDataTS = 0;
            iDataSourcePosParams.iActualNPT = 0;
            iDataSourcePosParams.iMode = PVMF_SET_DATA_SOURCE_POSITION_MODE_UNKNOWN;
            iDataSourcePosParams.iPlayElementIndex = -1;
            iDataSourcePosParams.iSeekToSyncPoint = true;
            iDataSourcePosParams.iTargetNPT = 0;
            targetNPT = iCurrentBeginPosition.iPlayListPosValue.millisec_value;
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback() SetDataSourcePosition() successful. StartMediaTS %d ms, ActualPBPos %d ms",
                         iActualMediaDataTS, iActualPlaybackPosition));

        if (iCurrentBeginPosition.iPosUnit == PVPPBPOSUNIT_PLAYLIST)
        {
            if (iCurrentBeginPosition.iPlayListPosValue.millisec_value >= iActualPlaybackPosition)
            {
                iWatchDogTimerInterval = iCurrentBeginPosition.iPlayListPosValue.millisec_value - iActualPlaybackPosition;
            }
        }
        // Compute the difference between actualNPT and targetNPT before any adjustments
        else if (iCurrentBeginPosition.iPosValue.millisec_value >= iActualPlaybackPosition)
        {
            iWatchDogTimerInterval = iCurrentBeginPosition.iPosValue.millisec_value - iActualPlaybackPosition;
        }

        //iCurrentBeginPosition.iPosUnit has served its purpose, it is ok if it is overwritten
        if (iSkipToRequestedPosition && (iActualPlaybackPosition < targetNPT))
        {
            if (targetNPT - iActualPlaybackPosition >= SYNC_POINT_DIFF_THRESHOLD)
            {
                // Can't adjust the skip time back so use the returned values to skip to
                iAdjustedMediaDataTS = iActualMediaDataTS;
                iActualPlaybackPosition = targetNPT;
                iWatchDogTimerInterval = 0;
            }
            else
            {
                // Adjust the media data time to skip-to to correspond to the requested time
                // Add the difference of target NPT with actual playback position in NPT to the actual media data time to get time to skip to.
                iAdjustedMediaDataTS = iActualMediaDataTS + (targetNPT - iActualPlaybackPosition);
                // Set the actual playback position to the requested time since actual media data TS was adjusted
                // This is important since the difference between the two is used to calculate the NPT to media data offset in HandleSinkNodeskipMediaDataDuringPlayback()
                iActualPlaybackPosition = targetNPT;
            }
            iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;
        }
        else
        {
            // Can't adjust the skip time back so just use the returned values to skip to
            iAdjustedMediaDataTS = iActualMediaDataTS;
            iCurrentBeginPosition.iPosValue.millisec_value = iActualPlaybackPosition;
            iCurrentBeginPosition.iPosUnit = PVPPBPOSUNIT_MILLISEC;
            iWatchDogTimerInterval = 0;
        }

        uint32 clockcurpos = 0;
        bool tmpbool;
        // Get current playback clock position
        iPlaybackClock.GetCurrentTime32(clockcurpos, tmpbool, OSCLCLOCK_MSEC);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE,
                        (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback() After adjustment StartMediaTS %d ms, AdjustedMediaTS %d ms, ActualPBPos %d ms Clock %d ms",
                         iActualMediaDataTS, iAdjustedMediaDataTS, iActualPlaybackPosition, clockcurpos));

        // Repositioning so reset the EOS flag for each active datapath
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive)
            {
                iDatapathList[i].iEndOfDataReceived = false;
            }
        }

        PVMFStatus retval = DoSinkNodeSkipMediaDataDuringPlayback(aNodeContext.iCmdId, aNodeContext.iCmdContext);
        if (retval != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback() Skipping media data request in sink nodes failed. Repositioning did not complete."));


            // clear the pending direction change NPT.
            iChangeDirectionNPT.iIndeterminate = true;

            // Complete the SetPlaybackRange() command as failed
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, retval);
        }
    }
    else
    {
        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface() != NULL)
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }
        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSource, puuid, nextmsg));

        // Complete the SetPlaybackRange() command as failed
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        if (errmsg)
        {
            errmsg->removeRef();
            errmsg = NULL;
        }
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeSetDataSourcePositionDuringPlayback() Out"));
}

void PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() for %s Tick=%d",
                     aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() for %s, iNumPVMFInfoStartOfDataPending=%d",
                     aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), iNumPVMFInfoStartOfDataPending));

    OSCL_UNUSED_ARG(aNodeResp);

    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() In %s", aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Stop the sink that has reached the skipping end point until other sinks are ready
    if (aNodeContext.iEngineDatapath->iTrackActive && aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF && (aNodeResp.GetCmdStatus() == PVMFSuccess))
    {
        aNodeContext.iEngineDatapath->iSinkNodeSyncCtrlIF->ClockStopped();
    }
    else
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkFatal, puuid, nextmsg));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();
        return;

    }

    OSCL_ASSERT(iNumPendingSkipCompleteEvent > 0);
    --iNumPendingSkipCompleteEvent;


    OSCL_ASSERT(iNumPendingNodeCmd > 0);
    --iNumPendingNodeCmd;

    if (iNumPendingNodeCmd == 0)
    {
        PVMFTimestamp targetNPT = iCurrentBeginPosition.iPosValue.millisec_value;

        // Stop and set to the specified reposition time
        iPlaybackClock.Stop();
        iPlaybackClock.SetStartTime32(iAdjustedMediaDataTS, OSCLCLOCK_MSEC);

        if (!(iWatchDogTimer->IsBusy()))
        {
            iWatchDogTimer->Cancel();
            if (iWatchDogTimerInterval > 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                                (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() Setting WatchDogTimer for %d ms, TargetNPT=%d  ActualNPT=%d",
                                 iWatchDogTimerInterval, targetNPT, iActualPlaybackPosition));
                iWatchDogTimer->setTimerDuration(iWatchDogTimerInterval);
                iWatchDogTimer->Start();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                                (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() Skipping WatchDogTimer - Starting PlayBackClock"));
                StartPlaybackClock();
            }
        }

        // Set the actual playback position to the requested time since actual media data TS was adjusted
        // This is important since the difference between the two is used to calculate the NPT to media data offset
        // This is not required here as the ActualPlaybackPosition is already adjusted before calling Skip on Sink Node.
        // iActualPlaybackPosition=iCurrentBeginPosition.iPosValue.millisec_value;

        //clear the pending direction change NPT.
        iChangeDirectionNPT.iIndeterminate = true;
        // Save the start NPT and TS
        iStartNPT = iActualPlaybackPosition;
        iStartMediaDataTS = iAdjustedMediaDataTS;
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_INFO,
                        (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() TargetNPT %d, ActualNPT %d StartTS %d",
                         targetNPT, iStartNPT, iStartMediaDataTS));
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                        (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() TargetNPT %d, ActualNPT %d StartTS %d",
                         targetNPT, iStartNPT, iStartMediaDataTS));

        // reset the ResumeAfterRepos boolean here
        iResumeAfterReposition = false;

        // Complete the SetPlaybackRange(), SetPlaybackRange(), or Resume() command as success
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);

        // Send info event that playback from new position has started
        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* infomsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoPlaybackFromBeginTime, puuid, NULL));
        SendInformationalEvent(PVMFInfoPositionStatus, OSCL_STATIC_CAST(PVInterface*, infomsg));
        infomsg->removeRef();

        if ((iNumPendingSkipCompleteEvent == 0) && (iNumPVMFInfoStartOfDataPending == 0))
        {
            if (iWatchDogTimer->IsBusy())
            {
                iWatchDogTimer->Cancel();
            }
            // we have received all the bos event for
            // playback hasnt started yet
            StartPlaybackClock();
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() - PlayClock Started"));
        }

    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeSkipMediaDataDuringPlayback() Out"));
}


void PVPlayerEngine::HandleSourceNodePause(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodePause() In"));

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Pause command is complete
            SetEngineState(PVP_ENGINE_STATE_PAUSED);
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodePause() Out"));
}


void PVPlayerEngine::HandleSourceNodeResume(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeResume() In"));

    PVMFStatus cmdstatus = PVMFErrNotSupported;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Issue start to all active datapaths
            iNumPendingDatapathCmd = 0;
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    PVMFStatus retval = DoDatapathStart(iDatapathList[i], aNodeContext.iCmdId, aNodeContext.iCmdContext);
                    if (retval == PVMFSuccess)
                    {
                        ++iNumPendingDatapathCmd;
                        cmdstatus = PVMFSuccess;
                    }
                    else
                    {
                        cmdstatus = retval;
                        break;
                    }
                }
            }

            if (iNumPendingDatapathCmd == 0)
            {
                if (cmdstatus == PVMFErrNotSupported)
                {
                    // No datapath so error out
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeResume() No datapath exists. Asserting"));
                    OSCL_ASSERT(false);

                    HandleErrorBasedOnPlayerState();

                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeResume() Report command as failed"));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFFailure);
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeResume() Report command as failed"));
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
                }
            }
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeResume() Out"));
}


void PVPlayerEngine::HandleSourceNodeStop(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStop() In"));

    PVMFStatus cmdstatus = PVMFErrNotSupported;

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
        {
            // Issue teardown sequence to all active datapaths
            iNumPendingDatapathCmd = 0;
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive)
                {
                    PVMFStatus retval = DoDatapathTeardown(iDatapathList[i], aNodeContext.iCmdId, aNodeContext.iCmdContext);
                    if (retval == PVMFSuccess)
                    {
                        ++iNumPendingDatapathCmd;
                        cmdstatus = PVMFSuccess;
                    }
                    else
                    {
                        cmdstatus = retval;
                        break;
                    }
                }
            }
        }

        if (iNumPendingDatapathCmd == 0)
        {
            if (cmdstatus == PVMFErrNotSupported)
            {
                // No active datapath to shutdown so stop is complete but assert
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeStop() No datapath exists. Asserting"));
                OSCL_ASSERT(false);
                SetEngineState(PVP_ENGINE_STATE_INITIALIZED);
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeStop() Report command as failed"));
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus);
            }
        }
        break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStop() Out"));
}


void PVPlayerEngine::HandleSourceNodeReset(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeReset() In"));

    if (aNodeResp.GetCmdStatus() == PVMFSuccess)
    {
        SetEngineState(PVP_ENGINE_STATE_IDLE);
        // we need to delete src node & cmdComplete from engine Run while CMD_Reset is still pending:
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        RunIfNotReady();
    }
    else
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceShutdown, puuid, nextmsg));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();
        return;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeReset() Out"));
}


void PVPlayerEngine::HandleSinkNodePause(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodePause() In %s", aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aNodeResp.GetCmdStatus() != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkFatal, puuid, nextmsg));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Auto-pause is complete
        SetEngineState(PVP_ENGINE_STATE_AUTO_PAUSED);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodePause() Report command as success"));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodePause() Out"));
}


void PVPlayerEngine::HandleSinkNodeResume(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    OSCL_ASSERT(aNodeContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aNodeContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeResume() In %s", aNodeContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aNodeResp.GetCmdStatus() != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aNodeResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkFatal, puuid, nextmsg));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, aNodeResp.GetCmdStatus(), OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Auto-resume is complete
        // Resume the playback clock
        iPlaybackClock.Start();
        // To get regular play status events
        StartPlaybackStatusTimer();
        // Notify data sinks that clock has started
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
            {
                iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStarted();
            }
        }

        SetEngineState(PVP_ENGINE_STATE_STARTED);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeResume() Report command as success"));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeResume() Out"));
}


void PVPlayerEngine::HandleSourceNodeStopDueToError(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStopDueToError() In"));

    if (aNodeResp.GetCmdStatus() == PVMFErrInvalidState &&
            iSourceNode->GetState() == EPVMFNodeInitialized &&
            iErrorOccurredDuringErrorHandling == false)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStopDueToError() Source node is already stopped"));
    }
    else if (aNodeResp.GetCmdStatus() != PVMFSuccess && iErrorOccurredDuringErrorHandling == false)
    {
        // Issue a reset command
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStopDueToError() Stop on source node failed so issue reset command"));
        iErrorOccurredDuringErrorHandling = true;
    }

    // Initiate teardown or reset sequence for each active datapath
    iNumPendingDatapathCmd = 0;
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true)
        {
            PVMFStatus retcode = DoDatapathTeardownDueToError(iDatapathList[i], aNodeContext.iCmdId, aNodeContext.iCmdContext);
            if (retcode == PVMFSuccess)
            {
                ++iNumPendingDatapathCmd;
            }
            else
            {
                retcode = DoDatapathResetDueToError(iDatapathList[i], aNodeContext.iCmdId, aNodeContext.iCmdContext);
                if (retcode == PVMFSuccess)
                {
                    ++iNumPendingDatapathCmd;
                }
            }
        }
    }

    if (iNumPendingDatapathCmd == 0)
    {
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            // make copy of context data and reschedule for deletion
            PVPlayerEngineContext* newContext = AllocateEngineContext(
                                                    &(iDatapathList[i]), NULL, NULL, 0, NULL, 0);
            AddCommandToQueue(PVP_ENGINE_COMMAND_DATAPATH_DELETE, newContext, NULL, NULL, false);
            iNumPendingDatapathCmd++; // datapathdelete pending
        }

        if (iNumPendingDatapathCmd == 0)
        {
            OSCL_ASSERT(iDatapathList.empty());
            // reset source node, usually called by datapathDelete but since there are no datapaths..
            if (iErrorOccurredDuringErrorHandling)
            {
                AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
            }
        }
        // done with STOP_DUE_TO_ERROR, clear current Cmd so that either DatapathDelete or ResetDueToError get run
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext,
                               iErrorOccurredDuringErrorHandling ? PVMFFailure : PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeStopDueToError() Out"));
}


void PVPlayerEngine::HandleSourceNodeResetDueToError(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeResetDueToError() In"));

    if (aNodeResp.GetCmdStatus() == PVMFSuccess)
    {
        SetEngineState(PVP_ENGINE_STATE_IDLE);
        // we need to delete src node & cmdComplete from engine Run while CMD_Reset_DUE_TO_ERROR is still pending:
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        RunIfNotReady();
    }
    else
    {
        // Error occurred for Reset() command but complete it and issue a cleanup command
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeResetDueToError() Reset on source node failed so issue cleanup command"));
        EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFFailure);
        SetEngineState(PVP_ENGINE_STATE_ERROR);
        AddCommandToQueue(PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR, NULL, NULL, NULL, false);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeResetDueToError() Out"));
}


void PVPlayerEngine::HandleDatapathPrepare(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathPrepare() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathPrepare() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp)
        {
            if (aCmdResp->GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
            }
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathInit, puuid, nextmsg));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Reposition and/or start the source node
        PVMFStatus cmdstatus = DoSourceNodeQueryDataSourcePosition(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            // Setting position not supported so start the source node
            cmdstatus = DoSourceNodeStart(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
        }

        if (cmdstatus != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathPrepare() Report command as failed"));
            EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, cmdstatus);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathPrepare() Out"));
}


void PVPlayerEngine::HandleDatapathStart(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathStart() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStart() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp)
        {
            if (aCmdResp->GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
            }
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathFatal, puuid, nextmsg));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Skip some media data in sink node if necessary
        PVMFStatus cmdstatus = DoSinkNodeSkipMediaData(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
        if (cmdstatus != PVMFSuccess)
        {
            // No skipping so complete the commmand
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathStart() Skip of sink node failed, but report command as completed"));
            SetEngineState(PVP_ENGINE_STATE_PREPARED);
            EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, PVMFSuccess);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStart() Out"));
}


void PVPlayerEngine::HandleDatapathPause(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathPause() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathPause() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp)
        {
            if (aCmdResp->GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
            }
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathFatal, puuid, nextmsg));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Continue on by pauseing the source node
        PVMFStatus cmdstatus = DoSourceNodePause(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);

        if (cmdstatus != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathPause() Report command as failed"));
            EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, cmdstatus);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathPause() Out"));
}


void PVPlayerEngine::HandleDatapathResume(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathResume() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathResume() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp)
        {
            if (aCmdResp->GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
            }
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathFatal, puuid, nextmsg));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        if (iChangePlaybackPositionWhenResuming || iChangePlaybackDirectionWhenResuming)
        {
            iResumeAfterReposition = true;
        }

        if (!iResumeAfterReposition)
        {
            // Resume the playback clock - only if we have come out of any previous auto pause, if any
            if (iPlaybackClock.GetState() == OsclClock::PAUSED)
            {
                iPlaybackClock.Start();
                // To get regular play status events
                StartPlaybackStatusTimer();
                // Notify all sink nodes that have sync control IF that clock has started
                for (uint32 i = 0; i < iDatapathList.size(); ++i)
                {
                    if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
                    {
                        iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStarted();
                    }
                }
            }

            // Restart the watchdog timer which was cancelled in Pause command.
            // this should only be done when engine is waiting for StartofData info event
            // after reposition.
            if (iNumPVMFInfoStartOfDataPending > 0)
            {
                if (iWatchDogTimerInterval > 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO,
                                    (0, "PVPlayerEngine::HandleDatapathResume Setting WatchDogTimer for %d ms, iNumPVMFInfoStartOfDataPending=%d",
                                     iWatchDogTimerInterval, iNumPVMFInfoStartOfDataPending));
                    iWatchDogTimer->setTimerDuration(iWatchDogTimerInterval);
                    iWatchDogTimer->Start();
                }
            }


            // Restart the end time check if enabled
            if (iEndTimeCheckEnabled)
            {
                // Determine the check cycle based on interval setting in milliseconds
                // and timer frequency of 100 millisec
                int32 checkcycle = iEndTimeCheckInterval / 100;
                if (checkcycle == 0)
                {
                    ++checkcycle;
                }
                iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
                iPollingCheckTimer->Request(PVPLAYERENGINE_TIMERID_ENDTIMECHECK, 0, checkcycle, this, true);
            }
        }

        SetEngineState(PVP_ENGINE_STATE_STARTED);

        if (iChangePlaybackPositionWhenResuming || iChangePlaybackDirectionWhenResuming)
        {
            iChangePlaybackPositionWhenResuming = false;
            iChangePlaybackDirectionWhenResuming = false;
            iResumeAfterReposition = true;
            PVMFStatus retval = DoSinkNodeSkipMediaDataDuringPlayback(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
            if (retval != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathResume() Skipping media data request in sink nodes failed. Repositioning did not complete."));
                PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
                PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSink, puuid, NULL));

                //clear the pending direction change NPT.
                iChangeDirectionNPT.iIndeterminate = true;

                // Complete the Resume() command as failed
                EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, retval, OSCL_STATIC_CAST(PVInterface*, errmsg));

                // Report an error event
                SendErrorEvent(retval, OSCL_STATIC_CAST(PVInterface*, errmsg));
                errmsg->removeRef();
                errmsg = NULL;
            }
            return;
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathResume() Report command as completed"));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathResume() Out"));
}


void PVPlayerEngine::HandleDatapathStop(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathStop() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStop() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp)
        {
            if (aCmdResp->GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
            }
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathFatal, puuid, nextmsg));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Continue on by stopping the source node
        PVMFStatus cmdstatus = DoSourceNodeStop(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);

        if (cmdstatus != PVMFSuccess)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathStop() Report command as failed"));
            EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, cmdstatus);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStop() Out"));
}


void PVPlayerEngine::HandleDatapathTeardown(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathTeardown() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathTeardown() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    PVMFStatus cmdstatus;

    switch (aDatapathStatus)
    {
        case PVMFSuccess:
            // Reset this datapath next
            OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
            cmdstatus = DoDatapathReset(*(aDatapathContext.iEngineDatapath), aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
            break;

        default:
        {
            HandleErrorBasedOnPlayerState();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aCmdResp)
            {
                if (aCmdResp->GetEventExtensionInterface())
                {
                    nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
                }
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathShutdown, puuid, nextmsg));
            EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();

            // Cancel any pending node/datapath commands
            DoCancelDueToError();
            return;
        }
    }

    if (cmdstatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleDatapathTeardown() Report command as failed"));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, cmdstatus);

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleVideoDatapathTeardown() Out"));
}


void PVPlayerEngine::HandleDatapathReset(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* aCmdResp)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_PROF, iPerfLogger, PVLOGMSG_INFO,
                    (0, "PVPlayerEngine::HandleDatapathReset() for %s Tick=%d",
                     aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr(), OsclTickCount::TickCount()));

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathReset() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess)
    {
        HandleErrorBasedOnPlayerState();

        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp)
        {
            if (aCmdResp->GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aCmdResp->GetEventExtensionInterface()));
            }
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathShutdown, puuid, nextmsg));
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, aDatapathStatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
        errmsg->removeRef();

        // Cancel any pending node/datapath commands
        DoCancelDueToError();
        return;
    }
    else
    {
        // Reset for this datapath is complete
        OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
        DoEngineDatapathTeardown(*(aDatapathContext.iEngineDatapath));
    }

    if (iNumPendingDatapathCmd == 0)
    {
        // Stop is complete
        SetEngineState(PVP_ENGINE_STATE_INITIALIZED);
        EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathReset() Out"));
}


void PVPlayerEngine::HandleDatapathStopDueToError(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* /*aCmdResp*/)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStopDueToError() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // Decrement the counter for pending datapath cmds
    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    if (aDatapathStatus != PVMFSuccess && iErrorOccurredDuringErrorHandling == false)
    {
        // Issue a reset command
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStopDueToError() Stop on datapath failed so issue reset command"));
        iErrorOccurredDuringErrorHandling = true;
    }

    PVMFStatus cmdstatus = PVMFFailure;

    if (iNumPendingDatapathCmd == 0)
    {
        // Trying stopping the source node
        cmdstatus = DoSourceNodeStopDueToError(aDatapathContext.iCmdId, aDatapathContext.iCmdContext);

        if (cmdstatus != PVMFSuccess)
        {
            // Try starting teardown or reset sequence for each active datapath
            iNumPendingDatapathCmd = 0;
            for (uint32 i = 0; i < iDatapathList.size(); ++i)
            {
                if (iDatapathList[i].iTrackActive == true)
                {
                    PVMFStatus retcode = DoDatapathTeardownDueToError(iDatapathList[i], aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
                    if (retcode == PVMFSuccess)
                    {
                        ++iNumPendingDatapathCmd;
                    }
                    else
                    {
                        retcode = DoDatapathResetDueToError(iDatapathList[i], aDatapathContext.iCmdId, aDatapathContext.iCmdContext);
                        if (retcode == PVMFSuccess)
                        {
                            ++iNumPendingDatapathCmd;
                        }
                    }
                }
            }

            if (iNumPendingDatapathCmd == 0)
            {
                // Nothing initiated so just shutdown
                SetEngineState(PVP_ENGINE_STATE_INITIALIZED);

                // Clean up the datapaths
                for (uint32 i = 0; i < iDatapathList.size(); ++i)
                {
                    DoEngineDatapathCleanup(iDatapathList[i]);
                }
                iDatapathList.clear();

                if (iErrorOccurredDuringErrorHandling)
                {
                    EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, PVMFFailure);
                }
                else
                {
                    EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext, PVMFSuccess);
                }
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathStopDueToError() Out"));
}


void PVPlayerEngine::HandleDatapathTeardownDueToError(PVPlayerEngineContext& aDatapathContext, PVMFStatus aDatapathStatus, PVMFCmdResp* /*aCmdResp*/)
{
    OSCL_ASSERT(aDatapathContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aDatapathContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathTeardownDueToError() In %s", aDatapathContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    if (aDatapathStatus != PVMFSuccess && iErrorOccurredDuringErrorHandling == false)
    {
        // Issue a reset command
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathTeardownDueToError() Teardown on datapath failed so issue reset command"));
        iErrorOccurredDuringErrorHandling = true;
    }

    PVMFStatus cmdstatus = PVMFFailure;

    // Try resetting this datapath
    OSCL_ASSERT(aDatapathContext.iEngineDatapath);
    cmdstatus = DoDatapathResetDueToError(*(aDatapathContext.iEngineDatapath), aDatapathContext.iCmdId, aDatapathContext.iCmdContext);

    if (cmdstatus != PVMFSuccess)
    {
        // Couldn't continue with reset of this datapath so decrement the pending datapath cmd counter
        --iNumPendingDatapathCmd;

        if (iNumPendingDatapathCmd == 0)
        {
            // make copy of context data and reschedule for deletion
            PVPlayerEngineContext* newContext = AllocateEngineContext(
                                                    aDatapathContext.iEngineDatapath, NULL, NULL, 0, NULL, 0);
            AddCommandToQueue(PVP_ENGINE_COMMAND_DATAPATH_DELETE, newContext, NULL, NULL, false);
            iNumPendingDatapathCmd++; // datapathdelete pending

            // a delete cmd has been added to the queue, but it can only be processed once current cmd is completed:
            // cmdId is shared by multiple datapath commands. if we have not sent confirmation previously do now
            if (iCurrentCmd.size() == 1 &&  iCurrentCmd[0].GetCmdId() == aDatapathContext.iCmdId)
            {
                EngineCommandCompleted(aDatapathContext.iCmdId, aDatapathContext.iCmdContext,
                                       iErrorOccurredDuringErrorHandling ? PVMFFailure : PVMFSuccess);
            }
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathTeardownDueToError() Out"));
}


// schedule async deletion of aDatapathContext.iEngineDatapath
void PVPlayerEngine::HandleDatapathResetDueToError(PVPlayerEngineContext& aContext, PVMFStatus aDatapathStatus, PVMFCmdResp* /*aCmdResp*/)
{
    OSCL_ASSERT(aContext.iEngineDatapath != NULL);
    OSCL_ASSERT(aContext.iEngineDatapath->iTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathResetDueToError() In %s", aContext.iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));

    // make copy of context data and reschedule for deletion
    PVPlayerEngineContext* newContext = AllocateEngineContext(
                                            aContext.iEngineDatapath, NULL, NULL, 0, NULL, 0);

    AddCommandToQueue(PVP_ENGINE_COMMAND_DATAPATH_DELETE, newContext, NULL, NULL, false);

    if (aDatapathStatus != PVMFSuccess && iErrorOccurredDuringErrorHandling == false)
    {
        // Issue a reset command if not done so previously
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathResetDueToError() Reset on datapath failed so issue reset command"));
        iErrorOccurredDuringErrorHandling = true;
    }

    // a delete cmd has been added to the queue, but it can only be processed once current cmd is completed:
    // cmdId is shared by multiple datapath commands. if we have not sent confirmation previously do now
    if (iCurrentCmd.size() == 1 &&  iCurrentCmd[0].GetCmdId() == aContext.iCmdId)
    {
        EngineCommandCompleted(aContext.iCmdId, aContext.iCmdContext,
                               iErrorOccurredDuringErrorHandling ? PVMFFailure : PVMFSuccess);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDatapathResetDueToError() Out"));
}

// delete aDatapathContext.iEngineDatapath
PVMFStatus PVPlayerEngine::DoDatapathDelete(PVPlayerEngineCommand &aCmd)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathDelete() In"));

    PVPlayerEngineContext* aDatapathContext = (PVPlayerEngineContext*) aCmd.GetContext();

    OSCL_ASSERT(aDatapathContext != NULL);
    OSCL_ASSERT(aDatapathContext->iEngineDatapath != NULL);
    if (aDatapathContext->iEngineDatapath->iTrackActive)
    {
        OSCL_ASSERT(aDatapathContext->iEngineDatapath->iTrackInfo != NULL);
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathDelete() In %s", aDatapathContext->iEngineDatapath->iTrackInfo->getTrackMimeType().get_cstr()));
    }

    // Clean up objects associated with the current datapath
    DoEngineDatapathCleanup(*(aDatapathContext->iEngineDatapath));

    OSCL_ASSERT(iNumPendingDatapathCmd > 0);
    --iNumPendingDatapathCmd;

    // all datapath resets have been handled
    if (iNumPendingDatapathCmd == 0)
    {
        iDatapathList.clear();

        // reset source node
        AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
    }

    // delete request handled
    EngineCommandCompleted(aCmd.GetCmdId(), aDatapathContext, PVMFSuccess);
    FreeEngineContext(aDatapathContext);

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::DoDatapathDelete() Out"));
    return PVMFSuccess;
}


void PVPlayerEngine::HandleCPMPluginQueryCapConfigIF(const PVMFCmdResp& aCmdResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleCPMPluginQueryCapConfigIF() In"));

    if (aCmdResp.GetCmdStatus() == PVMFSuccess)
    {
        // CapConfig interface available. Proceed to set user-agent
        DoCPMPluginSetUserAgentHeader(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext());
    }
    else
    {
        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aCmdResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        iCommandCompleteInEngineAOErrMsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrCPMPlugin, puuid, nextmsg));
        // There shouldn't be any other command pending to complete in engine's AO
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        iCommandCompleteInEngineAOCmdStatus = aCmdResp.GetCmdStatus();
    }

    // Proceed to acquire licence
    // if CapConfig UUID is not supported

    PVMFStatus retval = DoCPMPluginGetLicense(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext());
    if (retval != PVMFSuccess)
    {
        // There shouldn't be any other command pending to complete in engine's AO
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        iCommandCompleteInEngineAOCmdStatus = retval;
        iCommandCompleteInEngineAOErrMsg = NULL;
    }

    if (iCommandCompleteInEngineAOPending)
    {
        if (IsBusy())
        {
            Cancel();
        }
        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleCPMPluginQueryCapConfigIF() Out"));
}
void PVPlayerEngine::HandleCPMPluginQueryLicenseIF(const PVMFCmdResp& aCmdResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleCPMPluginQueryLicenseIF() In"));

    if (aCmdResp.GetCmdStatus() == PVMFSuccess)
    {
        //
        // License interface available. Proceed to query for CapConfig interface
        iCPMPlugin->QueryInterface(iCPMPluginSessionId, PVMI_CAPABILITY_AND_CONFIG_PVUUID, (PVInterface*&)iCPMPluginCapConfigIf, NULL);

        if (NULL == iCPMPluginCapConfigIf)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleCPMPluginQueryLicenseIF queryInterface on CPMPluginCapConfigIf returned error"));
        }
        iCPMPluginCommand = PVP_CMD_CPMQueryCapConfigIF;
    }
    else
    {
        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aCmdResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        iCommandCompleteInEngineAOErrMsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrCPMPlugin, puuid, nextmsg));
        // There shouldn't be any other command pending to complete in engine's AO
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        iCommandCompleteInEngineAOCmdStatus = aCmdResp.GetCmdStatus();
    }

    if (iCommandCompleteInEngineAOPending)
    {
        if (IsBusy())
        {
            Cancel();
        }
        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleCPMPluginQueryLicenseIF() Out"));
}


void PVPlayerEngine::HandleCPMPluginGetLicense(const PVMFCmdResp& aCmdResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleCPMPluginGetLicense() In"));

    if (aCmdResp.GetCmdStatus() == PVMFSuccess)
    {
        // License acquisition is successful.
        // There shouldn't be any other command pending to complete in engine's AO
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        iCommandCompleteInEngineAOCmdStatus = PVMFSuccess;
        iCommandCompleteInEngineAOErrMsg = NULL;
    }
    else
    {
        PVMFErrorInfoMessageInterface* nextmsg = NULL;
        if (aCmdResp.GetEventExtensionInterface())
        {
            nextmsg = GetErrorInfoMessageInterface(*(aCmdResp.GetEventExtensionInterface()));
        }

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        iCommandCompleteInEngineAOErrMsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrCPMPlugin, puuid, nextmsg));
        // There shouldn't be any other command pending to complete in engine's AO
        OSCL_ASSERT(iCommandCompleteInEngineAOPending == false);
        iCommandCompleteInEngineAOPending = true;
        iCommandCompleteInEngineAOCmdStatus = aCmdResp.GetCmdStatus();
    }

    if (IsBusy())
    {
        Cancel();
    }
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleCPMPluginGetLicense() Out"));
}

void PVPlayerEngine::HandleSourceNodeGetLicense(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeGetLicense() In"));

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
            break;

        case PVMFErrCancelled:
        default:
        {
            // report command complete
            PVMFStatus cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSource, puuid, nextmsg));
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
            errmsg->removeRef();

        }
        break;
    }
    /* Set CancelAcquireLicense cmd in current cmd */
    if (!iCmdToDlaCancel.empty())
    {
        iCurrentCmd.push_front(iCmdToDlaCancel[0]);
        iCmdToDlaCancel.clear();
    }
}

void PVPlayerEngine::HandleSourceNodeCancelGetLicense(PVPlayerEngineContext& aNodeContext, const PVMFCmdResp& aNodeResp)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeCancelGetLicense() In"));

    switch (aNodeResp.GetCmdStatus())
    {
        case PVMFSuccess:
            EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, PVMFSuccess);
            break;

        default:
        {
            /* report command complete */
            PVMFStatus cmdstatus = aNodeResp.GetCmdStatus();

            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aNodeResp.GetEventExtensionInterface())
            {
                nextmsg = GetErrorInfoMessageInterface(*(aNodeResp.GetEventExtensionInterface()));
            }

            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSource, puuid, nextmsg));

            if (iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_CHAR
                    || iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_ACQUIRE_LICENSE_WCHAR)
            {
                if (!iCmdToDlaCancel.empty())
                {
                    PVPlayerEngineCommand currentcmd(iCurrentCmd[0]);
                    iCurrentCmd.erase(iCurrentCmd.begin());
                    iCurrentCmd.push_front(iCmdToDlaCancel[0]);
                    /* If we get here the command isn't queued so the cancel fails */
                    EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
                    iCurrentCmd.push_front(currentcmd);
                    iCmdToDlaCancel.erase(iCmdToDlaCancel.begin());
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeCancelGetLicense() ASSERT"));
                    OSCL_ASSERT(false);
                }
            }
            else if (iCurrentCmd[0].GetCmdType() == PVP_ENGINE_COMMAND_CANCEL_ACQUIRE_LICENSE)
            {
                EngineCommandCompleted(aNodeContext.iCmdId, aNodeContext.iCmdContext, cmdstatus, OSCL_STATIC_CAST(PVInterface*, errmsg));
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSourceNodeCancelGetLicense() ASSERT"));
                OSCL_ASSERT(false);
            }
            errmsg->removeRef();
        }
        break;
    }
}

void PVPlayerEngine::HandleSourceNodeErrorEvent(const PVMFAsyncEvent& aEvent)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeErrorEvent() In"));

    PVMFEventType event = aEvent.GetEventType();

    switch (event)
    {
        case PVMFErrCorrupt:
        case PVMFErrOverflow:
        case PVMFErrResource:
        case PVMFErrProcessing:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeErrorEvent() Sending PVPlayerErrSourceMediaData for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceMediaData, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        case PVMFErrUnderflow:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeErrorEvent() Sending PVPlayerErrSourceMediaDataUnavailable for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceMediaDataUnavailable, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        case PVMFErrNoResources:
        case PVMFErrResourceConfiguration:
        case PVMFErrTimeout:
        case PVMFErrNoMemory:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeErrorEvent() Sending PVPlayerErrSourceFatal for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSourceFatal, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        default:
            // Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeErrorEvent() Do nothing for this event %d", event));
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeErrorEvent() Out"));
}


void PVPlayerEngine::HandleDecNodeErrorEvent(const PVMFAsyncEvent& aEvent, int32 aDatapathIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeErrorEvent() In"));

    OSCL_UNUSED_ARG(aDatapathIndex);

    PVMFEventType event = aEvent.GetEventType();

    switch (event)
    {
        case PVMFErrCorrupt:
        case PVMFErrOverflow:
        case PVMFErrUnderflow:
        case PVMFErrProcessing:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeErrorEvent() Sending PVPlayerErrDatapathMediaData for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathMediaData, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        case PVMFErrTimeout:
        case PVMFErrNoResources:
        case PVMFErrResourceConfiguration:
        case PVMFErrResource:
        case PVMFErrNoMemory:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeErrorEvent() Sending PVPlayerErrDatapathFatal for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrDatapathFatal, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        default:
            // Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeErrorEvent() Do nothing for this event %d", event));
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeErrorEvent() Out"));
}


void PVPlayerEngine::HandleSinkNodeErrorEvent(const PVMFAsyncEvent& aEvent, int32 aDatapathIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeErrorEvent() In"));

    OSCL_UNUSED_ARG(aDatapathIndex);

    PVMFEventType event = aEvent.GetEventType();

    switch (event)
    {
        case PVMFErrCorrupt:
        case PVMFErrOverflow:
        case PVMFErrUnderflow:
        case PVMFErrProcessing:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeErrorEvent() Sending PVPlayerErrDatapathMediaData for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkMediaData, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        case PVMFErrTimeout:
        case PVMFErrNoResources:
        case PVMFErrResourceConfiguration:
        case PVMFErrResource:
        case PVMFErrNoMemory:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeErrorEvent() Sending PVPlayerErrSinkFatal for error event %d", event));
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
            }
            PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
            PVMFBasicErrorInfoMessage* errmsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerErrSinkFatal, puuid, nextmsg));
            if (HandleAsyncErrorBasedOnPlayerState(event, errmsg) == true)
            {
                SendErrorEvent(event, OSCL_STATIC_CAST(PVInterface*, errmsg), aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            }
            errmsg->removeRef();
            errmsg = NULL;
        }
        break;

        default:
            // Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeErrorEvent() Do nothing for this event %d", event));
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeErrorEvent() Out"));
}


void PVPlayerEngine::HandleErrorBasedOnPlayerState(void)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() In"));

    switch (iState)
    {
        case PVP_ENGINE_STATE_PREPARED:
        case PVP_ENGINE_STATE_STARTED:
        case PVP_ENGINE_STATE_PAUSED:
        case PVP_ENGINE_STATE_AUTO_PAUSED:
            // Issue emergency stop command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error while Prepared, Started, or Paused. Issue stop due to error"));
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_INITIALIZED:
            // Issue emergency reset command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error while Initialized. Issue reset due to error"));
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_INITIALIZING:
        {
            // Initialization of source node failed so go to idle state
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error while Initializing. Go back to idle state"));
            SetEngineState(PVP_ENGINE_STATE_IDLE);
        }
        break;

        case PVP_ENGINE_STATE_STARTING:
        case PVP_ENGINE_STATE_PAUSING:
        case PVP_ENGINE_STATE_RESUMING:
        case PVP_ENGINE_STATE_AUTO_PAUSING:
        case PVP_ENGINE_STATE_AUTO_RESUMING:
            // Issue emergency stop command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error while Preparing, Starting, Resuming, or Pausing. Issue stop due to error"));
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

            //call RESET when in preparing state
        case PVP_ENGINE_STATE_PREPARING:
        case PVP_ENGINE_STATE_STOPPING:
            // Issue emergency reset command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error while Preparing/Stopping. Issue stop due to error"));
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_RESETTING:
            // Issue emergency cleanup command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error while resetting. Issue cleanup due to error"));
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            AddCommandToQueue(PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_IDLE:
            // Should not receive any error events when idle
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error event received while idle. Do nothing"));
            OSCL_ASSERT(false);
            break;

        case PVP_ENGINE_STATE_HANDLINGERROR:
        case PVP_ENGINE_STATE_ERROR:
            // Do nothing
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error event received while handling error. Do nothing"));
            break;

        default:
            // Unknown state
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Error event received while in state %d. Do nothing", iState));
            OSCL_ASSERT(false);
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleErrorBasedOnPlayerState() Out"));
}


bool PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState(PVMFEventType aErrorCode, PVMFBasicErrorInfoMessage* aErrMsg)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() In"));

    if (PVP_ENGINE_COMMAND_CANCEL_ALL_COMMANDS  == iCurrentCmd[0].GetCmdType() && iPendingResetDueToCancelRequest > 0)
    {
        return false;
    }

    bool senderrorevent = true;

    switch (iState)
    {
        case PVP_ENGINE_STATE_PREPARED:
        case PVP_ENGINE_STATE_STARTED:
        case PVP_ENGINE_STATE_PAUSED:
        case PVP_ENGINE_STATE_AUTO_PAUSED:
            // Issue emergency stop command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error while Prepared, Started, or Paused. Issue stop due to error"));
            // Change to error state
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            if (iCurrentCmd.empty() == false)
            {
                // Report the error by completing the current command with error
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), aErrorCode, aErrMsg);
                senderrorevent = false;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
            AddCommandToQueue(PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_INITIALIZED:
            // Issue emergency reset command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error while Initialized. Issue reset due to error"));
            // Change to error state
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            if (iCurrentCmd.empty() == false)
            {
                // Report the error by completing the current command with error
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), aErrorCode, aErrMsg);
                senderrorevent = false;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
            AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_INITIALIZING:
            // Initialization of source node failed so go to idle state
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error while Initializing. Go back to idle state"));
            // Change to error state
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            if (iCurrentCmd.empty() == false)
            {
                // Report the error by completing the current command with error
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), aErrorCode, aErrMsg);
                senderrorevent = false;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
            AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_PREPARING:
        case PVP_ENGINE_STATE_STARTING:
        case PVP_ENGINE_STATE_PAUSING:
        case PVP_ENGINE_STATE_RESUMING:
            // Issue emergency stop command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error while Preparing, Starting, Resuming, or Pausing. Issue stop due to error"));
            // Change to error state
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            if (iCurrentCmd.empty() == false)
            {
                // Report the error by completing the current command with error
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), aErrorCode, aErrMsg);
                senderrorevent = false;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
            AddCommandToQueue(PVP_ENGINE_COMMAND_STOP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_STOPPING:
            // Issue emergency reset command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error while Stopping. Issue reset due to error"));
            // Change to error state
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            if (iCurrentCmd.empty() == false)
            {
                // Report the error by completing the current command with error
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), aErrorCode, aErrMsg);
                senderrorevent = false;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
            AddCommandToQueue(PVP_ENGINE_COMMAND_RESET_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_RESETTING:
            // Issue emergency cleanup command
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error while resetting. Issue cleanup due to error"));
            // Change to error state
            SetEngineState(PVP_ENGINE_STATE_ERROR);
            if (iCurrentCmd.empty() == false)
            {
                // Report the error by completing the current command with error
                EngineCommandCompleted(iCurrentCmd[0].GetCmdId(), iCurrentCmd[0].GetContext(), aErrorCode, aErrMsg);
                senderrorevent = false;
                // Cancel any pending node/datapath commands
                DoCancelDueToError();
            }
            AddCommandToQueue(PVP_ENGINE_COMMAND_CLEANUP_DUE_TO_ERROR, NULL, NULL, NULL, false);
            break;

        case PVP_ENGINE_STATE_IDLE:
            // Should not receive any error events when idle
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error event received while idle. Do nothing"));
            OSCL_ASSERT(false);
            break;

        case PVP_ENGINE_STATE_HANDLINGERROR:
        case PVP_ENGINE_STATE_ERROR:
            // Do nothing
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error event received while handling error. Do nothing"));
            break;

        default:
            // Unknown state
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Error event received while in state %d. Do nothing", iState));
            OSCL_ASSERT(false);
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleAsyncErrorBasedOnPlayerState() Out"));

    return senderrorevent;
}

//Return val: true means found/deleted cmd(s).
bool PVPlayerEngine::removeCmdFromQ(Oscl_Vector<PVPlayerEngineCommand, OsclMemAllocator> &aVec, const PVPlayerEngineCommandType aCmdType, bool aRemove)
{
    if (aVec.size() == 0)
    {
        return false;
    }
    // OSCL priority queue doesn't allow index access so will need to
    // go through each pending cmd one-by-one, save commands that are not auto-resume
    // and put the commands back into the pending queue.
    // Vector to hold the pending cmds temporarily
    bool ret = false;
    Oscl_Vector<PVPlayerEngineCommand, OsclMemAllocator> tmpvec;
    tmpvec.reserve(aVec.size());
    tmpvec.clear();
    // Go through each pending command

    for (int i = aVec.size() - 1; i >= 0; i--)
    {
        if (aVec[i].GetCmdType() == aCmdType)
        {
            ret = true;
            if (!aRemove)
            {
                return ret;
            }
            continue;
        }
        tmpvec.push_back(aVec[i]);
    }
    aVec.clear();
    // Put the pending commands back in the priqueue
    while (tmpvec.empty() == false)
    {
        aVec.push_front(tmpvec[0]);
        tmpvec.erase(tmpvec.begin());
    }
    return ret;
}

//Return val: true means found/deleted cmd(s).
bool PVPlayerEngine::removeCmdFromQ(OsclPriorityQueue<PVPlayerEngineCommand, OsclMemAllocator, Oscl_Vector<PVPlayerEngineCommand, OsclMemAllocator>, PVPlayerEngineCommandCompareLess> &aVec, const PVPlayerEngineCommandType aCmdType, bool aRemove)
{
    // OSCL priority queue doesn't allow index access so will need to
    // go through each pending cmd one-by-one, save commands that are not auto-resume
    // and put the commands back into the pending queue.
    // Vector to hold the pending cmds temporarily
    bool ret = false;
    Oscl_Vector<PVPlayerEngineCommand, OsclMemAllocator> tmpvec;
    tmpvec.reserve(aVec.size());
    tmpvec.clear();
    // Go through each pending command
    while (aVec.empty() == false)
    {
        if (aVec.top().GetCmdType() == aCmdType)
        {
            ret = true;
            if (!aRemove)
            {
                return ret;
            }
            aVec.pop();
            continue;
        }
        tmpvec.push_back(aVec.top());
        aVec.pop();
    }
    // Put the pending commands back in the priqueue
    while (tmpvec.empty() == false)
    {
        aVec.push(tmpvec[0]);
        tmpvec.erase(tmpvec.begin());
    }
    return ret;
}

void PVPlayerEngine::HandleSourceNodeInfoEvent(const PVMFAsyncEvent& aEvent)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() In"));

    PVMFEventType event = aEvent.GetEventType();

    switch (event)
    {
        case PVMFInfoBufferingStart:
        case PVMFInfoBufferingComplete:
        case PVMFInfoOverflow:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending buffering event %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;

        case PVMFInfoBufferingStatus:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending buffering status event %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;

        case PVMFInfoUnderflow:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Received PVMFInfoUnderflow"));
            // remove pending auto-resume if there is any
            if (removeCmdFromQ(iPendingCmds, PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY, true))
            {//cancelled the pending auto-resume
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() PVMFInfoUnderflow got cancelled"));
                break;
            }

            // if no auto-pause in the queue and no auto-pause in progress, add one
            if ((! removeCmdFromQ(iCurrentCmd, PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW, false))
                    && (! removeCmdFromQ(iPendingCmds, PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW, false)))
            {
                AddCommandToQueue(PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW, NULL, NULL, NULL, false);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Received source underflow event, issue auto-pause command"));
            }
        }
        break;

        case PVMFInfoDataReady:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Received PVMFInfoDataReady"));
            // remove pending auto-pause if there is any
            if (removeCmdFromQ(iPendingCmds, PVP_ENGINE_COMMAND_PAUSE_DUE_TO_BUFFER_UNDERFLOW, true))
            {//cancelled the pending auto-pause
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() PVMFInfoDataReady got cancelled"));
                break;
            }

            // if no resume in the queue and no resume in progress, add one
            if ((! removeCmdFromQ(iCurrentCmd, PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY, false))
                    && (! removeCmdFromQ(iPendingCmds, PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY, false)))
            {
                AddCommandToQueue(PVP_ENGINE_COMMAND_RESUME_DUE_TO_BUFFER_DATAREADY, NULL, NULL, NULL, false);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Received source data ready event, issue auto-resume command"));
            }
        }
        break;

        case PVMFInfoContentLength:
        case PVMFInfoContentType:
        case PVMFInfoContentTruncated:
        case PVMFInfoRemoteSourceNotification:
        case PVMFInfoPlayListClipTransition:
        {
            PVInterface* intf = NULL;
            PVMFBasicErrorInfoMessage* infomsg = NULL;
            PVMFErrorInfoMessageInterface* nextmsg = NULL;
            if (aEvent.GetEventExtensionInterface() != NULL)
            {
                nextmsg = GetErrorInfoMessageInterface(*(aEvent.GetEventExtensionInterface()));
                PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
                infomsg =
                    OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoSourceMediaData,
                                                         puuid,
                                                         nextmsg));
                intf = OSCL_STATIC_CAST(PVInterface*, infomsg);
            }
            SendInformationalEvent(event, intf, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            if (infomsg != NULL)
            {
                infomsg->removeRef();
            }
            infomsg = NULL;
        }
        break;

        case PVMFInfoTrackDisable:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending bad track disabled event %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;

        case PVMFInfoUnexpectedData:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending unexpected data event %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;

        case PVMFInfoSessionDisconnect:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending session disconnect %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;
        case PVMFInfoDurationAvailable:
        {
            PVUuid infomsguuid = PVMFDurationInfoMessageInterfaceUUID;
            PVMFDurationInfoMessageInterface* eventMsg = NULL;
            PVInterface* infoExtInterface = aEvent.GetEventExtensionInterface();
            if (infoExtInterface &&
                    infoExtInterface->queryInterface(infomsguuid, (PVInterface*&)eventMsg))
            {
                PVUuid eventuuid;
                int32 infoCode;
                eventMsg->GetCodeUUID(infoCode, eventuuid);
                if (eventuuid == infomsguuid)
                {
                    iSourceDurationInMS = eventMsg->GetDuration();
                    iSourceDurationAvailable = true;
                    SendInformationalEvent(aEvent.GetEventType(), infoExtInterface, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
                }
            }
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending duration available %d", event));
        }
        break;

        case PVMFInfoMetadataAvailable:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending meta data Info %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;
        case PVMFInfoPoorlyInterleavedContent:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Sending Poorly Interleaved Content Info %d", event));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
        }
        break;
        case PVMFInfoEndOfData:
        default:
            // Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Do nothing for this event %d", event));
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSourceNodeInfoEvent() Out"));
}


void PVPlayerEngine::HandleDecNodeInfoEvent(const PVMFAsyncEvent& aEvent, int32 aDatapathIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeInfoEvent() In"));

    OSCL_UNUSED_ARG(aDatapathIndex);

    PVMFEventType event = aEvent.GetEventType();

    switch (event)
    {
        case PVMFInfoProcessingFailure:

            SendInformationalEvent(PVMFInfoProcessingFailure);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeInfoEvent() Report ProcessingFailure event %d", event));
            break;

        case PVMFInfoOverflow:
        case PVMFInfoEndOfData:
        default:
            // Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeInfoEvent() Do nothing for this event %d", event));
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleDecNodeInfoEvent() Out"));
}


void PVPlayerEngine::HandleSinkNodeInfoEvent(const PVMFAsyncEvent& aEvent, int32 aDatapathIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() In"));

    PVMFEventType event = aEvent.GetEventType();

    switch (event)
    {
        case PVMFInfoStartOfData:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() PVMFInfoStartOfData event received for %s",
                            iDatapathList[aDatapathIndex].iTrackInfo->getTrackMimeType().get_cstr()));

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() PVMFInfoStartOfData event received for %s, iNumPendingSkipCompleteEvent=%d iNumPVMFInfoStartOfDataPending=%d",
                            iDatapathList[aDatapathIndex].iTrackInfo->getTrackMimeType().get_cstr(),
                            iNumPendingSkipCompleteEvent, iNumPVMFInfoStartOfDataPending));

            uint32 *data = (uint32*) aEvent.GetEventData();
            uint32 streamID = *data;

            if (streamID != iStreamID)
            {
                // recieved StartOfData for previous streamId, ignoring these events
                break;
            }

            if (iNumPVMFInfoStartOfDataPending > 0)
            {
                --iNumPVMFInfoStartOfDataPending;
            }

            if ((iNumPendingSkipCompleteEvent == 0) && (iNumPVMFInfoStartOfDataPending == 0))
            {
                if (iWatchDogTimer->IsBusy())
                {
                    iWatchDogTimer->Cancel();
                }
                // we have received all the bos event for
                // playback hasnt started yet
                StartPlaybackClock();
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() - PlayClock Started"));
            }

            //else it could mean duplicate or old PVMFInfoStartOfData, ignore
            //both
        }
        break;

        case PVMFInfoEndOfData:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() PVMFInfoEndOfData event received for %s",
                            iDatapathList[aDatapathIndex].iTrackInfo->getTrackMimeType().get_cstr()));

            uint32 *data = (uint32*) aEvent.GetEventData();
            uint32 streamID = *data;

            if (streamID != iStreamID)
            {
                // recieved EndOfData for previous streamId, ignoring these events
                break;
            }

            if (iDatapathList[aDatapathIndex].iTrackActive && iDatapathList[aDatapathIndex].iEndOfDataReceived == false)
            {
                iDatapathList[aDatapathIndex].iEndOfDataReceived = true;
                // If all datapath received EOS, initiate a pause-due-to-EOS
                if (AllDatapathReceivedEndOfData() == true)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() Issue Pause due to end of clip"));
                    AddCommandToQueue(PVP_ENGINE_COMMAND_PAUSE_DUE_TO_ENDOFCLIP, NULL, NULL, NULL, false);
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() PVMFInfoEndOfData event received for non-active or already-ended datapath. Asserting"));
                OSCL_ASSERT(false);
            }
        }
        break;

        case PVMFInfoDataDiscarded:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() PVMFInfoDataDiscarded event received"));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            break;

        case PVMFInfoVideoTrackFallingBehind:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() PVMFInfoVideoTrackFallingBehind event received"));
            SendInformationalEvent(event, NULL, aEvent.GetEventData(), aEvent.GetLocalBuffer(), aEvent.GetLocalBufferSize());
            break;

        default:
            // Do nothing but log it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() Do nothing for this event %d", event));
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::HandleSinkNodeInfoEvent() Out"));
}


bool PVPlayerEngine::AllDatapathReceivedEndOfData()
{
    // Return false if any active datapath hasn't
    // received EOS yet. Else true.
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive == true &&
                iDatapathList[i].iEndOfDataReceived == false)
        {
            return false;
        }
    }

    return true;
}


void PVPlayerEngine::SendEndOfClipInfoEvent(PVMFStatus aStatus, PVInterface* aExtInterface)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendEndOfClipInfoEvent() In"));

    // If paused succeeded or already paused
    if (aStatus == PVMFSuccess || aStatus == PVMFErrInvalidState)
    {
        // Set the flag so we can disable resume unless user stops, repositions, or changes directrion
        iPlaybackPausedDueToEndOfClip = true;

        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* infomsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoEndOfClipReached, puuid, NULL));
        SendInformationalEvent(PVMFInfoEndOfData, OSCL_STATIC_CAST(PVInterface*, infomsg));
        infomsg->removeRef();
    }
    else
    {
        SendErrorEvent(aStatus, aExtInterface);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendEndOfClipInfoEvent() Out"));
}


void PVPlayerEngine::SendEndTimeReachedInfoEvent(PVMFStatus aStatus, PVInterface* aExtInterface)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendEndTimeReachedInfoEvent() In"));

    if (aStatus == PVMFSuccess)
    {
        PVUuid puuid = PVPlayerErrorInfoEventTypesUUID;
        PVMFBasicErrorInfoMessage* infomsg = OSCL_NEW(PVMFBasicErrorInfoMessage, (PVPlayerInfoEndTimeReached, puuid, NULL));
        SendInformationalEvent(PVMFInfoEndOfData, OSCL_STATIC_CAST(PVInterface*, infomsg));
        infomsg->removeRef();
    }
    else
    {
        SendErrorEvent(aStatus, aExtInterface);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendEndTimeReachedInfoEvent() Out"));
}


void PVPlayerEngine::SendSourceUnderflowInfoEvent(PVMFStatus aStatus, PVInterface* aExtInterface)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendSourceUnderflowInfoEvent() In"));

    if (aStatus == PVMFSuccess || aStatus == PVMFErrNotSupported)
    {
        if (iDataReadySent)
        {
            iDataReadySent = false;
            SendInformationalEvent(PVMFInfoUnderflow);
        }
    }
    else if (aStatus == PVMFErrCancelled)
    {
        // Do nothing since the auto-pause was cancelled
    }
    else
    {
        SendErrorEvent(aStatus, aExtInterface);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendSourceUnderflowInfoEvent() Out"));
}


void PVPlayerEngine::SendSourceDataReadyInfoEvent(PVMFStatus aStatus, PVInterface* aExtInterface)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendSourceDataReadyInfoEvent() In"));


    if (aStatus == PVMFSuccess || aStatus == PVMFErrNotSupported)
    {
        if (!iDataReadySent)
        {
            iDataReadySent = true;
            SendInformationalEvent(PVMFInfoDataReady);
        }
    }
    else if (aStatus == PVMFErrCancelled)
    {
        // Do nothing since the auto-resume was cancelled
    }
    else
    {
        SendErrorEvent(aStatus, aExtInterface);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendSourceDataReadyInfoEvent() Out"));
}


void PVPlayerEngine::SendErrorHandlingCompleteEvent(PVMFStatus aStatus, PVInterface* /*aExtInterface*/)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendErrorHandlingCompleteEvent() In"));

    if (aStatus == PVMFSuccess)
    {
        SendInformationalEvent(PVMFInfoErrorHandlingComplete, NULL);
    }
    else
    {
        // Don't send any event since error handling is still not complete
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::SendErrorHandlingCompleteEvent() Out"));
}


PVMFErrorInfoMessageInterface* PVPlayerEngine::GetErrorInfoMessageInterface(PVInterface& aInterface)
{
    PVMFErrorInfoMessageInterface* extiface = NULL;
    if (aInterface.queryInterface(PVMFErrorInfoMessageInterfaceUUID, (PVInterface*&)extiface))
    {
        return extiface;
    }
    else
    {
        return NULL;
    }
}


void PVPlayerEngine::StartPlaybackStatusTimer(void)
{
    // Check if playback position reporting is enabled
    if (!iPBPosEnable || iPlayStatusTimerEnabled)
        return;
    // To get regular play status events
    iPlayStatusTimerEnabled = true;

    // Determine the check cycle based on interval setting in milliseconds
    // and timer frequency of 100 millisec
    int32 checkcycle = iPBPosStatusInterval / 100;
    if (checkcycle == 0)
    {
        ++checkcycle;
    }
    OSCL_ASSERT(iPollingCheckTimer);
    iPollingCheckTimer->Request(PVPLAYERENGINE_TIMERID_PLAY_STATUS, 0, checkcycle, this, true);
}


void PVPlayerEngine::StopPlaybackStatusTimer(void)
{


    // Stop the playback position status timer
    iPlayStatusTimerEnabled = false;
    OSCL_ASSERT(iPollingCheckTimer);
    iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_PLAY_STATUS);
}

bool PVPlayerEngine::CheckForSourceRollOver()
{
    uint32 alternates = iDataSource->GetNumAlternateSourceFormatTypes();
    if ((alternates > 0) && (iAlternateSrcFormatIndex < alternates))
    {
        return true;
    }
    return false;
}

PVMFStatus PVPlayerEngine::SetupDataSourceForUnknownURLAccess()
{
    if (iDataSource == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SetupDataSourceForUnknownURLAccess() - No Data Source"));
        return PVMFErrInvalidState;
    }
    else
    {
        /*
         * In case of unknown url, here is the sequence that engine would attempt:
         *  1) First Alternate source format would be PVMF_DATA_SOURCE_RTSP_URL,
         *  implying that we would attempt a RTSP streaming session
         *
         *	2) Primary source format would be set to PVMF_DATA_SOURCE_HTTP_URL,
         *  implying that we would attempt a progressive download first.
         *
         *	3) Second Alternate source format would be PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL,
         *  implying that we would attempt a real media cloaking session
         *
         *  4) Third alternate source format would be PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL,
         *  implying that we would attempt a MS HTTP streaming session
         */
        iSourceFormatType = PVMF_DATA_SOURCE_RTSP_URL; ;
        if (iDataSource->SetAlternateSourceFormatType(PVMF_DATA_SOURCE_HTTP_URL) != true)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SetupDataSourceForUnknownURLAccess() - SetAlternateSourceFormatType Failed"));
            return PVMFFailure;
        }
        if (iDataSource->SetAlternateSourceFormatType(PVMF_DATA_SOURCE_REAL_HTTP_CLOAKING_URL) != true)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SetupDataSourceForUnknownURLAccess() - SetAlternateSourceFormatType Failed"));
            return PVMFFailure;
        }
        if (iDataSource->SetAlternateSourceFormatType(PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL) != true)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SetupDataSourceForUnknownURLAccess() - SetAlternateSourceFormatType Failed"));
            return PVMFFailure;
        }
        return PVMFSuccess;
    }
}

/* scan thru aKvpValue, create a new PvmiKvp object and push
 * than onto iPvmiKvpCapNConfig.
 *  return PVMFSuccess unless allocation error
 */
PVMFStatus PVPlayerEngine:: VerifyAndSaveKVPValues(PvmiKvp *aKvpValue)
{

    PvmiKvp* KvpCapNConfig = (PvmiKvp *)OSCL_MALLOC(sizeof(PvmiKvp));
    if (! KvpCapNConfig)  return PVMFErrNoMemory;
    oscl_memcpy(KvpCapNConfig, aKvpValue, sizeof(PvmiKvp));


    KvpCapNConfig->key = (char*)OSCL_MALLOC(oscl_strlen(aKvpValue->key) + 1);
    if (! KvpCapNConfig->key)  return PVMFErrNoMemory;
    oscl_strncpy(KvpCapNConfig->key, aKvpValue->key, oscl_strlen(aKvpValue->key) + 1);


    /// all the values have copied automatically so just need to allocate memory of pointer type.

    if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=wchar*")) != NULL)
    {
        KvpCapNConfig->value.pWChar_value = (oscl_wchar*)OSCL_MALLOC((oscl_strlen(aKvpValue->value.pWChar_value) + 1) * sizeof(oscl_wchar));
        if (! KvpCapNConfig->value.pWChar_value)  return PVMFErrNoMemory;
        oscl_strncpy(KvpCapNConfig->value.pWChar_value, aKvpValue->value.pWChar_value, oscl_strlen(aKvpValue->value.pWChar_value) + 1);

    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=char*")) != NULL)
    {
        KvpCapNConfig->value.pChar_value = (char*)OSCL_MALLOC(oscl_strlen(aKvpValue->value.pChar_value) + 1);
        if (! KvpCapNConfig->value.pChar_value)  return PVMFErrNoMemory;
        oscl_strncpy(KvpCapNConfig->value.pChar_value, aKvpValue->value.pChar_value, oscl_strlen(aKvpValue->value.pChar_value) + 1);

    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=uint8*")) != NULL)
    {
        KvpCapNConfig->value.pUint8_value = (uint8*)OSCL_MALLOC(oscl_strlen((char *)aKvpValue->value.pUint8_value) + 1);
        if (! KvpCapNConfig->value.pUint8_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pUint8_value, aKvpValue->value.pUint8_value, oscl_strlen((char *)aKvpValue->value.pUint8_value) + 1);

    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=int32*")) != NULL)
    {
        KvpCapNConfig->value.pInt32_value = (int32*)OSCL_MALLOC(sizeof(int32));
        if (! KvpCapNConfig->value.pInt32_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pInt32_value, aKvpValue->value.pInt32_value, sizeof(int32));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=uint32*")) != NULL)
    {
        KvpCapNConfig->value.pUint32_value = (uint32*)OSCL_MALLOC(sizeof(uint32));
        if (! KvpCapNConfig->value.pUint32_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pUint32_value, aKvpValue->value.pUint32_value, sizeof(uint32));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=int64*")) != NULL)
    {
        KvpCapNConfig->value.pInt64_value = (int64*)OSCL_MALLOC(sizeof(int64));
        if (! KvpCapNConfig->value.pInt64_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pInt64_value, aKvpValue->value.pInt64_value, sizeof(int64));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=uint64*")) != NULL)
    {
        KvpCapNConfig->value.pUint64_value = (uint64*)OSCL_MALLOC(sizeof(uint64));
        if (! KvpCapNConfig->value.pUint64_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pUint64_value, aKvpValue->value.pUint64_value, sizeof(uint64));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=float*")) != NULL)
    {
        KvpCapNConfig->value.pFloat_value = (float*)OSCL_MALLOC(sizeof(float));
        if (! KvpCapNConfig->value.pFloat_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pFloat_value, aKvpValue->value.pFloat_value, sizeof(float));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=double*")) != NULL)
    {
        KvpCapNConfig->value.pDouble_value = (double*)OSCL_MALLOC(sizeof(double));
        if (! KvpCapNConfig->value.pDouble_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.pDouble_value, aKvpValue->value.pDouble_value, sizeof(double));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=range_int32")) != NULL)
    {
        KvpCapNConfig->value.key_specific_value = (void*)OSCL_MALLOC(sizeof(range_int32));
        if (! KvpCapNConfig->value.key_specific_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.key_specific_value, aKvpValue->value.key_specific_value, sizeof(range_int32));
        // Currently support only one element.  Future support will be based on length.
    }
    else if (oscl_strstr(aKvpValue->key, _STRLIT_CHAR("valtype=range_uint32")) != NULL)
    {
        KvpCapNConfig->value.key_specific_value = (void*)OSCL_MALLOC(sizeof(range_uint32));
        if (! KvpCapNConfig->value.key_specific_value)  return PVMFErrNoMemory;
        oscl_memcpy(KvpCapNConfig->value.key_specific_value, aKvpValue->value.key_specific_value, sizeof(range_uint32));
        // Currently support only one element.  Future support will be based on length.
    }

    iPvmiKvpCapNConfig.push_back(KvpCapNConfig);
    return PVMFSuccess;
}


void PVPlayerEngine::SetRollOverKVPValues()
{
    PvmiKvp *SaveKvp;
    PvmiKvp *ErrorKVP;

    for (uint i = 0; i < iPvmiKvpCapNConfig.size(); i++)
    {
        SaveKvp = iPvmiKvpCapNConfig[i];
        setParametersSync(NULL, SaveKvp, 1 , ErrorKVP);

        if (ErrorKVP != NULL)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::SetRollOverKVPValues() Configuring cap-config failed"));
        }
    }
}

void PVPlayerEngine::DeleteKVPValues()
{
    uint i = 0;
    while (i < iPvmiKvpCapNConfig.size())
    {
        PvmiKvp **Setkvp = iPvmiKvpCapNConfig.begin();

        if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=wchar*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pWChar_value);
            (*Setkvp)->value.pWChar_value = NULL;

        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=char*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pChar_value);
            (*Setkvp)->value.pChar_value = NULL;
        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=uint8*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pUint8_value);
            (*Setkvp)->value.pUint8_value = NULL;
        }

        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=int32*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pInt32_value);
            (*Setkvp)->value.pInt32_value = NULL;

        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=uint32*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pUint32_value);
            (*Setkvp)->value.pUint32_value = NULL;

        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=int64*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pInt64_value);
            (*Setkvp)->value.pInt64_value = NULL;

        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=uint64*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pUint64_value);
            (*Setkvp)->value.pUint64_value = NULL;
        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=float*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pFloat_value);
            (*Setkvp)->value.pFloat_value = NULL;
        }
        else if (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=double*")) != NULL)
        {
            OSCL_FREE((*Setkvp)->value.pDouble_value);
            (*Setkvp)->value.pDouble_value = NULL;
        }
        else if ((oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=range_int32")) != NULL) ||
                 (oscl_strstr((*Setkvp)->key, _STRLIT_CHAR("valtype=range_uint32")) != NULL))
        {
            OSCL_FREE((*Setkvp)->value.key_specific_value);
            (*Setkvp)->value.key_specific_value = NULL;
        }

        OSCL_FREE((*Setkvp)->key);
        (*Setkvp)->key = NULL;

        OSCL_FREE(*Setkvp);
        *Setkvp = NULL;

        iPvmiKvpCapNConfig.erase(iPvmiKvpCapNConfig.begin());
    }

}


void PVPlayerEngine::PVPlayerWatchdogTimerEvent()
{
    StartPlaybackClock();
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerEngine::PVPlayerWatchdogTimerEvent() WatchDog timer expired"));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iReposLogger, PVLOGMSG_INFO, (0, "PVPlayerEngine::PVPlayerWatchdogTimerEvent() WatchDog timer expired"));
}

void PVPlayerEngine::StartPlaybackClock()
{
    iWatchDogTimer->Cancel();
    if (GetPVPlayerState() != PVP_STATE_STARTED)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::StartPlaybackClock() wrong engine state"));
        return;
    }

    if (iPlaybackClock.GetState() == OsclClock::RUNNING)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::StartPlaybackClock() clock already started"));
        return;
    }

    iPlaybackClock.Start();
    // Notify all sink nodes that have sync control IF that clock has started
    for (uint32 i = 0; i < iDatapathList.size(); ++i)
    {
        if (iDatapathList[i].iTrackActive && iDatapathList[i].iSinkNodeSyncCtrlIF)
        {
            iDatapathList[i].iSinkNodeSyncCtrlIF->ClockStarted();
        }
    }

    // To get regular play status events
    StartPlaybackStatusTimer();

    // Restart the end time check if enabled
    if (iEndTimeCheckEnabled)
    {
        // Determine the check cycle based on interval setting in milliseconds
        // and timer frequency of 100 millisec
        int32 checkcycle = iEndTimeCheckInterval / 100;
        if (checkcycle == 0)
        {
            ++checkcycle;
        }
        iPollingCheckTimer->Cancel(PVPLAYERENGINE_TIMERID_ENDTIMECHECK);
        iPollingCheckTimer->Request(PVPLAYERENGINE_TIMERID_ENDTIMECHECK, 0, checkcycle, this, true);
    }


    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::StartPlaybackClock() StartNPT %d StartTS %d", iStartNPT, iStartMediaDataTS));
}

PVMFStatus PVPlayerEngine::GetCompleteList(PVMFMediaPresentationInfo& aList)
{
    if (iSourceNodeTrackSelIF)
    {
        PVPlayerState state = GetPVPlayerState();
        if ((state == PVP_STATE_INITIALIZED) ||
                (state == PVP_STATE_PREPARED) ||
                (state == PVP_STATE_STARTED) ||
                (state == PVP_STATE_PAUSED))
        {
            aList.Reset();
            PVMFStatus retval = PVMFFailure;
            int32 leavecode = 0;
            OSCL_TRY(leavecode, retval = iSourceNodeTrackSelIF->GetMediaPresentationInfo(aList));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetCompleteList() GetMediaPresentationInfo on iSourceNodeTrackSelIF did a leave!"));
                                 return PVMFFailure);
            if (retval != PVMFSuccess)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetCompleteList() GetMediaPresentationInfo() call on source node failed"));
            }
            return retval;
        }
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetCompleteList() iSourceNodeTrackSelIF Invalid"));
    return PVMFFailure;
}

PVMFStatus PVPlayerEngine::ReleaseCompleteList(PVMFMediaPresentationInfo& aList)
{
    aList.Reset();
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::GetPlayableList(PVMFMediaPresentationInfo& aList)
{
    PVPlayerState state = GetPVPlayerState();
    if ((state == PVP_STATE_PREPARED) ||
            (state == PVP_STATE_STARTED) ||
            (state == PVP_STATE_PAUSED))
    {
        aList = iPlayableList;
        if (aList.getNumTracks() == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetPlayableList() No tracks"));
            return PVMFFailure;
        }
        return PVMFSuccess;
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetPlayableList() Invalid Engine State"));
    return PVMFErrInvalidState;
}

PVMFStatus PVPlayerEngine::ReleasePlayableList(PVMFMediaPresentationInfo& aList)
{
    aList.Reset();
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::GetSelectedList(PVMFMediaPresentationInfo& aList)
{
    PVPlayerState state = GetPVPlayerState();
    if ((state == PVP_STATE_PREPARED) ||
            (state == PVP_STATE_STARTED) ||
            (state == PVP_STATE_PAUSED))
    {
        aList.Reset();
        aList.setPresentationType(iPlayableList.getPresentationType());
        aList.setSeekableFlag(iPlayableList.IsSeekable());
        aList.SetDurationAvailable(iPlayableList.IsDurationAvailable());
        aList.setDurationValue(iPlayableList.getDurationValue());
        aList.setDurationTimeScale(iPlayableList.getDurationTimeScale());
        for (uint32 i = 0; i < iDatapathList.size(); ++i)
        {
            if (iDatapathList[i].iTrackInfo != NULL)
            {
                aList.addTrackInfo(*(iDatapathList[i].iTrackInfo));
            }
        }
        if (aList.getNumTracks() == 0)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetSelectedList() No tracks"));
            return PVMFFailure;
        }
        return PVMFSuccess;
    }
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerEngine::GetSelectedList() Invalid Engine State"));
    return PVMFErrInvalidState;
}

PVMFStatus PVPlayerEngine::ReleaseSelectedList(PVMFMediaPresentationInfo& aList)
{
    aList.Reset();
    return PVMFSuccess;
}

PVMFStatus PVPlayerEngine::RegisterHelperObject(PVMFTrackSelectionHelper* aObject)
{
    if (aObject != NULL)
    {
        if (iTrackSelectionHelper != NULL)
        {
            return PVMFErrAlreadyExists;
        }
    }
    iTrackSelectionHelper = aObject;
    return PVMFSuccess;
}

#ifdef HAS_OSCL_LIB_SUPPORT
void PVPlayerEngine::PopulateNodeRegistry(const OSCL_String& aConfigFilePath)
{
    OsclLibraryList libList;

    // Check the current directory for a config file first. This way you can
    // override the config file set by the tunables.
    OSCL_StackString<PVPLAYERENGINE_DEFAULT_CONFIG_PATH_SIZE> defaultConfig("./pvplayer.conf");
    if (OsclLibFail == libList.Populate(PV_NODE_REGISTRY_INTERFACE, defaultConfig))
    {
        libList.Populate(PV_NODE_REGISTRY_INTERFACE, aConfigFilePath);
    }

    for (unsigned int i = 0; i < libList.Size(); i++)
    {
        OsclSharedLibrary* lib = OSCL_NEW(OsclSharedLibrary, ());
        if (lib->LoadLib(libList.GetLibraryPathAt(i)) == OsclLibSuccess)
        {
            OsclAny* interfacePtr = NULL;
            OsclLibStatus result = lib->QueryInterface(PV_NODE_REGISTRY_INTERFACE, (OsclAny*&)interfacePtr);
            if (result == OsclLibSuccess && interfacePtr != NULL)
            {
                struct PVPlayerEngineNodeSharedLibInfo *libInfo = (struct PVPlayerEngineNodeSharedLibInfo *)oscl_malloc(sizeof(struct PVPlayerEngineNodeSharedLibInfo));
                if (NULL != libInfo)
                {
                    libInfo->iLib = lib;

                    NodeRegistrySharedLibraryInterface* nodeIntPtr = OSCL_DYNAMIC_CAST(NodeRegistrySharedLibraryInterface*, interfacePtr);
                    libInfo->iNodeLibIfacePtr = nodeIntPtr;
                    nodeIntPtr->RegisterAllNodes((PVPlayerNodeRegistryInterface *)&iPlayerNodeRegistry,  libInfo->iContext);

                    // save for depopulation later
                    iNodeLibInfoList.push_front(libInfo);
                    continue;
                }
            }
        }
        lib->Close();
        OSCL_DELETE(lib);
    }
}

void PVPlayerEngine::PopulateRecognizerRegistry(const OSCL_String& aConfigFilePath)
{
    OsclLibraryList libList;

    // Check the current directory for a config file first. This way you can
    // override the config file set by the tunables.
    OSCL_StackString<PVPLAYERENGINE_DEFAULT_CONFIG_PATH_SIZE> defaultConfig("./pvplayer.conf");
    if (OsclLibFail == libList.Populate(PV_RECOGNIZER_INTERFACE, defaultConfig))
    {
        libList.Populate(PV_RECOGNIZER_INTERFACE, aConfigFilePath);
    }

    for (unsigned int i = 0; i < libList.Size(); i++)
    {
        OsclSharedLibrary* lib = OSCL_NEW(OsclSharedLibrary, ());
        if (lib->LoadLib(libList.GetLibraryPathAt(i)) == OsclLibSuccess)
        {
            OsclAny* interfacePtr = NULL;
            OsclLibStatus result = lib->QueryInterface(PV_RECOGNIZER_INTERFACE, (OsclAny*&)interfacePtr);
            if (result == OsclLibSuccess && interfacePtr != NULL)
            {
                struct PVPlayerEngineRecognizerSharedLibInfo *libInfo = (struct PVPlayerEngineRecognizerSharedLibInfo *)oscl_malloc(sizeof(struct PVPlayerEngineRecognizerSharedLibInfo));
                if (NULL != libInfo)
                {
                    libInfo->iLib = lib;

                    RecognizerSharedLibraryInterface* recognizerIntPtr = OSCL_DYNAMIC_CAST(RecognizerSharedLibraryInterface*, interfacePtr);

                    libInfo->iRecognizerLibIfacePtr = recognizerIntPtr;

                    recognizerIntPtr->RegisterAllRecognizers((PVPlayerRecognizerRegistryInterface *)&iPlayerRecognizerRegistry, libInfo->iContext);

                    // save for depopulation later
                    iRecognizerLibInfoList.push_front(libInfo);
                    continue;
                }
            }
        }
        lib->Close();
        OSCL_DELETE(lib);
    }
}

void PVPlayerEngine::PopulateAllRegistries(const OSCL_String& aConfigFilePath)
{
    PopulateNodeRegistry(aConfigFilePath);
    PopulateRecognizerRegistry(aConfigFilePath);
}

void PVPlayerEngine::DepopulateNodeRegistry()
{
    // remove all dynamic nodes now
    // unregister node one by one
    while (!iNodeLibInfoList.empty())
    {
        struct PVPlayerEngineNodeSharedLibInfo *libInfo = iNodeLibInfoList.front();
        iNodeLibInfoList.erase(iNodeLibInfoList.begin());

        OsclSharedLibrary* lib = libInfo->iLib;
        NodeRegistrySharedLibraryInterface* nodeIntPtr = libInfo->iNodeLibIfacePtr;
        OsclAny* context = libInfo->iContext;
        oscl_free(libInfo);

        nodeIntPtr->UnregisterAllNodes((PVPlayerNodeRegistryInterface *)&iPlayerNodeRegistry, context);

        lib->Close();
        OSCL_DELETE(lib);
    }
}

void PVPlayerEngine::DepopulateRecognizerRegistry()
{
    // remove all the dynamic plugins now
    // unregister the plugins one by one
    while (!iRecognizerLibInfoList.empty())
    {
        struct PVPlayerEngineRecognizerSharedLibInfo *libInfo = iRecognizerLibInfoList.front();
        iRecognizerLibInfoList.erase(iRecognizerLibInfoList.begin());

        OsclSharedLibrary* lib = libInfo->iLib;
        RecognizerSharedLibraryInterface* recognizerIntPtr = libInfo->iRecognizerLibIfacePtr;
        OsclAny* context = libInfo->iContext;
        oscl_free(libInfo);

        recognizerIntPtr->UnregisterAllRecognizers((PVPlayerRecognizerRegistryInterface *)&iPlayerRecognizerRegistry, context);

        lib->Close();
        OSCL_DELETE(lib);
    }
}

void PVPlayerEngine::DepopulateAllRegistries()
{
    DepopulateNodeRegistry();
    DepopulateRecognizerRegistry();
}
#endif
