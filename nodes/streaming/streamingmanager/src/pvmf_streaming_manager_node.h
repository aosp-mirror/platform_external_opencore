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
#define PVMF_STREAMING_MANAGER_NODE_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_STRING_CONTAINERS_H_INCLUDED
#include "oscl_string_containers.h"
#endif
#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif
#ifndef OSCL_MEM_MEMPOOL_H_INCLUDED
#include "oscl_mem_mempool.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
#endif
#ifndef PVMF_NODE_UTILS_H_INCLUDED
#include "pvmf_node_utils.h"
#endif
#ifndef PVMF_MEDIA_DATA_H_INCLUDED
#include "pvmf_media_data.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifndef PVMF_META_DATA_H_INCLUDED
#include "pvmf_meta_data_types.h"
#endif
#ifndef PVMI_KVP_INCLUDED
#include "pvmi_kvp.h"
#endif
#ifndef PVMI_CONFIG_AND_CAPABILITY_H_INCLUDED
#include "pvmi_config_and_capability.h"
#endif
#ifndef PVMF_MEDIA_PRESENTATION_INFO_H_INCLUDED
#include "pvmf_media_presentation_info.h"
#endif
#ifndef PVMF_STREAMING_REAL_INTERFACES_INCLUDED
#include "pvmf_streaming_real_interfaces.h"
#endif
#ifndef MEDIAINFO_H
#include "media_info.h"
#endif
#ifndef PAYLOAD_PARSER_H_INCLUDED
#include "payload_parser.h"
#endif
#ifndef CPM_H_INCLUDED
#include "cpm.h"
#endif
#ifndef PVMF_CPMPLUGIN_ACCESS_INTERFACE_H_INCLUDED
#include "pvmf_cpmplugin_access_interface.h"
#endif
#ifndef PVMF_LOCAL_DATA_SOURCE_H_INCLUDED
#include "pvmf_streaming_data_source.h"
#endif
#ifndef PVMF_SOURCE_CONTEXT_DATA_H_INCLUDED
#include "pvmf_source_context_data.h"
#endif
#ifndef PVMF_META_DATA_EXTENSION_H_INCLUDED
#include "pvmf_meta_data_extension.h"
#endif
#ifndef PVMF_CPMPLUGIN_LICENSE_INTERFACE_H_INCLUDED
#include "pvmf_cpmplugin_license_interface.h"
#endif
#ifndef PVMF_SM_CONFIG_H_INCLUDED
#include "pvmf_sm_config.h"
#endif

class PVLogger;
class PVMFStreamingManagerExtensionInterfaceImpl;
class PVMFSMSessionSourceInfo;
class rm_mediaInfo;
class PVMFDataSourcePositionParams;

/* memory allocator type for this node */
typedef OsclMemAllocator PVMFStreamingManagerNodeAllocator;

/*
 * Node command type & command proc related data structures
 */
#define PVMF_STREAMING_MANAGER_INTERNAL_CMDQ_SIZE 40

typedef PVMFGenericNodeCommand < PVMFStreamingManagerNodeAllocator > PVMFStreamingManagerNodeCommandBase;
class PVMFStreamingManagerNodeCommand : public PVMFStreamingManagerNodeCommandBase
{
    public:
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       int32 arg1,
                       int32 arg2,
                       int32& arg3,
                       const OsclAny*aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)arg1;
            iParam2 = (OsclAny*)arg2;
            iParam3 = (OsclAny*) & arg3;
        };

        void Parse(int32&arg1, int32&arg2, int32*&arg3)
        {
            arg1 = (int32)iParam1;
            arg2 = (int32)iParam2;
            arg3 = (int32*)iParam3;
        };

        /* Constructor and parser for SetDataSourcePosition */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       PVMFTimestamp aTargetNPT,
                       PVMFTimestamp* aActualNPT,
                       PVMFTimestamp* aActualMediaDataTS,
                       bool aSeekToSyncPoint,
                       uint32 aStreamID,
                       const OsclAny*aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aTargetNPT;
            iParam2 = (OsclAny*)aActualNPT;
            iParam3 = (OsclAny*)aActualMediaDataTS;
            iParam4 = (OsclAny*)aSeekToSyncPoint;
            iParam5 = (OsclAny*)aStreamID;
        };

        void Parse(PVMFTimestamp& aTargetNPT,
                   PVMFTimestamp* &aActualNPT,
                   PVMFTimestamp* &aActualMediaDataTS,
                   bool& aSeekToSyncPoint,
                   uint32& aStreamID)
        {
            aTargetNPT = (PVMFTimestamp)iParam1;
            aActualNPT = (PVMFTimestamp*)iParam2;
            aActualMediaDataTS = (PVMFTimestamp*)iParam3;
            aSeekToSyncPoint = (iParam4 ? true : false);
            aStreamID = (uint32)iParam5;
        };

        /* Constructor and parser for SetDataSourcePosition - Playlist */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       PVMFDataSourcePositionParams* aParams,
                       const OsclAny*aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aParams;
            iParam2 = NULL;
            iParam3 = NULL;
            iParam4 = NULL;
            iParam5 = NULL;
        };

        void Parse(PVMFDataSourcePositionParams*& aParams)
        {
            aParams = (PVMFDataSourcePositionParams*)iParam1;
        };


        /* Constructor and parser for QueryDataSourcePosition */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       PVMFTimestamp aTargetNPT,
                       PVMFTimestamp* aActualNPT,
                       bool aSeekToSyncPoint,
                       const OsclAny*aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aTargetNPT;
            iParam2 = (OsclAny*)aActualNPT;
            iParam3 = (OsclAny*)aSeekToSyncPoint;
            iParam4 = NULL;
            iParam5 = NULL;
        };

        void Construct(PVMFSessionId s,
                       int32 cmd,
                       PVMFTimestamp aTargetNPT,
                       PVMFTimestamp* aSeekPointBeforeTargetNPT,
                       PVMFTimestamp* aSeekPointAfterTargetNPT,
                       const OsclAny*aContext,
                       bool aSeekToSyncPoint
                      )
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aTargetNPT;
            iParam3 = (OsclAny*)aSeekToSyncPoint;
            iParam4 = aSeekPointBeforeTargetNPT;
            iParam5 = aSeekPointAfterTargetNPT;
        };

        void Parse(PVMFTimestamp& aTargetNPT,
                   PVMFTimestamp* &aActualNPT,
                   bool& aSeekToSyncPoint)
        {
            aTargetNPT = (PVMFTimestamp)iParam1;
            aActualNPT = (PVMFTimestamp*)iParam2;
            aSeekToSyncPoint = (iParam3 ? true : false);
        };

        void Parse(PVMFTimestamp& aTargetNPT,
                   PVMFTimestamp*& aSeekPointBeforeTargetNPT,
                   bool& aSeekToSyncPoint,
                   PVMFTimestamp*& aSeekPointAfterTargetNPT)

        {
            aTargetNPT = (PVMFTimestamp)iParam1;
            aSeekToSyncPoint = (iParam3) ? true : false;
            aSeekPointBeforeTargetNPT = (PVMFTimestamp*)iParam4;
            aSeekPointAfterTargetNPT = (PVMFTimestamp*)iParam5;
        }


        /* Constructor and parser for SetDataSourceRate */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       int32 aRate,
                       OsclTimebase* aTimebase,
                       const OsclAny* aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aRate;
            iParam2 = (OsclAny*)aTimebase;
            iParam3 = NULL;
            iParam4 = NULL;
            iParam5 = NULL;
        };

        void Parse(int32& aRate, OsclTimebase*& aTimebase)
        {
            aRate = (int32)iParam1;
            aTimebase = (OsclTimebase*)iParam2;
        }

        /* Constructor and parser for GetNodeMetadataKeys */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       PVMFMetadataList& aKeyList,
                       int32 aStartingIndex,
                       int32 aMaxEntries,
                       char* aQueryKey,
                       const OsclAny* aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*) & aKeyList;
            iParam2 = (OsclAny*)aStartingIndex;
            iParam3 = (OsclAny*)aMaxEntries;
            if (aQueryKey)
            {
                /*allocate a copy of the query key string */
                Oscl_TAlloc<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> str;
                iParam4 = str.ALLOC_AND_CONSTRUCT(aQueryKey);
            }
        }

        void Parse(PVMFMetadataList*& MetaDataListPtr,
                   uint32 &aStartingIndex,
                   int32 &aMaxEntries,
                   char*& aQueryKey)
        {
            MetaDataListPtr = (PVMFMetadataList*)iParam1;
            aStartingIndex = (uint32)iParam2;
            aMaxEntries = (int32)iParam3;
            aQueryKey = NULL;
            if (iParam4)
            {
                OSCL_HeapString<OsclMemAllocator>* keystring =
                    (OSCL_HeapString<OsclMemAllocator>*)iParam4;
                aQueryKey = keystring->get_str();
            }
        }

        /* Constructor and parser for GetNodeMetadataValue */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       PVMFMetadataList& aKeyList,
                       Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
                       uint32 aStartIndex,
                       int32 aMaxEntries,
                       const OsclAny* aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*) & aKeyList;
            iParam2 = (OsclAny*) & aValueList;
            iParam3 = (OsclAny*)aStartIndex;
            iParam4 = (OsclAny*)aMaxEntries;

        }

        void Parse(PVMFMetadataList* &aKeyList,
                   Oscl_Vector<PvmiKvp, OsclMemAllocator>* &aValueList,
                   uint32 &aStartingIndex,
                   int32 &aMaxEntries)
        {
            aKeyList = (PVMFMetadataList*)iParam1;
            aValueList = (Oscl_Vector<PvmiKvp, OsclMemAllocator>*)iParam2;
            aStartingIndex = (uint32)iParam3;
            aMaxEntries = (int32)iParam4;
        }

        /* Constructor and parser for GetLicenseW */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       OSCL_wString& aContentName,
                       OsclAny* aLicenseData,
                       uint32 aDataSize,
                       int32 aTimeoutMsec,
                       const OsclAny* aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*) & aContentName;
            iParam2 = (OsclAny*)aLicenseData;
            iParam3 = (OsclAny*)aDataSize;
            iParam4 = (OsclAny*)aTimeoutMsec;
            iParam5 = NULL;
        }

        void Parse(OSCL_wString*& aContentName,
                   OsclAny*& aLicenseData,
                   uint32& aDataSize,
                   int32& aTimeoutMsec)
        {
            aContentName = (OSCL_wString*)iParam1;
            aLicenseData = (PVMFTimestamp*)iParam2;
            aDataSize = (uint32)iParam3;
            aTimeoutMsec = (int32)iParam4;
        }

        /* Constructor and parser for GetLicense */
        void Construct(PVMFSessionId s,
                       int32 cmd,
                       OSCL_String& aContentName,
                       OsclAny* aLicenseData,
                       uint32 aDataSize,
                       int32 aTimeoutMsec,
                       const OsclAny*aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*) & aContentName;
            iParam2 = (OsclAny*)aLicenseData;
            iParam3 = (OsclAny*)aDataSize;
            iParam4 = (OsclAny*)aTimeoutMsec;
            iParam5 = NULL;
        };
        void Parse(OSCL_String*& aContentName,
                   OsclAny*& aLicenseData,
                   uint32& aDataSize,
                   int32& aTimeoutMsec)
        {
            aContentName = (OSCL_String*)iParam1;
            aLicenseData = (PVMFTimestamp*)iParam2;
            aDataSize = (uint32)iParam3;
            aTimeoutMsec = (int32)iParam4;
        }

        /* Constructor and parser for setParametersAsync */
        void Construct(PVMFSessionId s, int32 cmd, PvmiMIOSession aSession,
                       PvmiKvp* aParameters, int num_elements,
                       PvmiKvp*& aRet_kvp, OsclAny* aContext)
        {
            PVMFStreamingManagerNodeCommandBase::Construct(s, cmd, aContext);
            iParam1 = (OsclAny*)aSession;
            iParam2 = (OsclAny*)aParameters;
            iParam3 = (OsclAny*)num_elements;
            iParam4 = (OsclAny*) & aRet_kvp;
        }
        void Parse(PvmiMIOSession& aSession, PvmiKvp*& aParameters,
                   int &num_elements, PvmiKvp** &ppRet_kvp)
        {
            aSession = (PvmiMIOSession)iParam1;
            aParameters = (PvmiKvp*)iParam2;
            num_elements = (int)iParam3;
            ppRet_kvp = (PvmiKvp**)iParam4;
        }

        /* need to overlaod the base Copy routine to copy metadata key */
        void Copy(const PVMFGenericNodeCommand<OsclMemAllocator>& aCmd);

        /* need to overlaod the base Destroy routine to cleanup metadata key */
        void Destroy();
};

typedef PVMFNodeCommandQueue<PVMFStreamingManagerNodeCommand, PVMFStreamingManagerNodeAllocator> PVMFStreamingManagerNodeCmdQ;

typedef struct tagPVMFSMPortContext
{
    uint32 trackID;
    uint32 portTag;
} PVMFSMPortContext;

class PVMFSMCommandContext
{
    public:
        PVMFSMCommandContext()
        {
            parentCmd = 0;
            cmd = 0;
            oFree = true;
            portContext.trackID = 0;
            portContext.portTag = 0;
            oInternalCmd = false;
        };

        bool oInternalCmd;
        int32 parentCmd;
        int32 cmd;
        bool  oFree;
        PVMFSMPortContext portContext;
};

class PVMFSMTrackMetaDataInfo
{
    public:
        PVMFSMTrackMetaDataInfo()
        {
            iTrackDurationAvailable = false;
            iTrackDuration = 0;
            iTrackDurationTimeScale = 0;
            iTrackBitRate = 0;
            iTrackMaxBitRate = 0;
            iTrackSelected = false;
            iTrackID = iTrackID;
            iCodecName = NULL;
            iCodecDescription = NULL;
            iTrackWidth = 0;
            iTrackHeight = 0;
            iVideoFrameRate = 0;
            iAudioSampleRate = 0;
            iAudioNumChannels = 0;
            iAudioBitsPerSample = 0;
        };

        PVMFSMTrackMetaDataInfo(const PVMFSMTrackMetaDataInfo& a)
        {
            iTrackDurationAvailable = a.iTrackDurationAvailable;
            iTrackDuration = a.iTrackDuration;
            iTrackDurationTimeScale = a.iTrackDurationTimeScale;
            iMimeType = a.iMimeType;
            iTrackBitRate = a.iTrackBitRate;
            iTrackMaxBitRate = a.iTrackMaxBitRate;
            iTrackSelected = a.iTrackSelected;
            iTrackID = a.iTrackID;
            iCodecName = a.iCodecName;
            iCodecDescription = a.iCodecDescription;
            iCodecSpecificInfo = a.iCodecSpecificInfo;
            iTrackWidth = a.iTrackWidth;
            iTrackHeight = a.iTrackHeight;
            iVideoFrameRate = a.iVideoFrameRate;
            iAudioSampleRate = a.iAudioSampleRate;
            iAudioNumChannels = a.iAudioNumChannels;
            iAudioBitsPerSample = a.iAudioBitsPerSample;
        };

        PVMFSMTrackMetaDataInfo& operator=(const PVMFSMTrackMetaDataInfo& a)
        {
            if (&a != this)
            {
                iTrackDurationAvailable = a.iTrackDurationAvailable;
                iTrackDuration = a.iTrackDuration;
                iTrackDurationTimeScale = a.iTrackDurationTimeScale;
                iMimeType = a.iMimeType;
                iTrackBitRate = a.iTrackBitRate;
                iTrackMaxBitRate = a.iTrackMaxBitRate;
                iTrackSelected = a.iTrackSelected;
                iTrackID = a.iTrackID;
                iCodecName = a.iCodecName;
                iCodecDescription = a.iCodecDescription;
                iCodecSpecificInfo = a.iCodecSpecificInfo;
                iTrackWidth = a.iTrackWidth;
                iTrackHeight = a.iTrackHeight;
                iVideoFrameRate = a.iVideoFrameRate;
                iAudioSampleRate = a.iAudioSampleRate;
                iAudioNumChannels = a.iAudioNumChannels;
                iAudioBitsPerSample = a.iAudioBitsPerSample;
            }
            return (*this);
        };

        bool   iTrackDurationAvailable;
        uint64 iTrackDuration;
        uint64 iTrackDurationTimeScale;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iMimeType;
        uint32 iTrackBitRate;
        uint32 iTrackMaxBitRate;
        bool   iTrackSelected;
        uint32 iTrackID;
        OSCL_wHeapString<OsclMemAllocator> iCodecName;
        OSCL_wHeapString<OsclMemAllocator> iCodecDescription;
        uint32 iTrackWidth;
        uint32 iTrackHeight;
        uint32 iVideoFrameRate;
        uint32 iAudioSampleRate;
        uint32 iAudioNumChannels;
        uint32 iAudioBitsPerSample;
        OsclRefCounterMemFrag iCodecSpecificInfo;
};

class PVMFSMSessionMetaDataInfo
{
    public:
        PVMFSMSessionMetaDataInfo()
        {
            iTitlePresent = false;
            iDescriptionPresent = false;
            iCopyRightPresent = false;
            iPerformerPresent = false;
            iAuthorPresent = false;
            iGenrePresent = false;
            iRatingPresent = false;
            iClassificationPresent = false;
            iKeyWordsPresent = false;
            iLocationPresent = false;
            iLyricsPresent = false;
            iWMPicturePresent = false;

            iIsTitleUnicode = false;
            iIsDescriptionUnicode = false;
            iIsCopyRightUnicode = false;
            iIsPerformerUnicode = false;
            iIsAuthorUnicode = false;
            iIsGenreUnicode = false;
            iIsRatingUnicode = false;
            iIsClassificationUnicode = false;
            iIsKeyWordsUnicode = false;
            iIsLocationUnicode = false;
            iIsLyricsUnicode = false;

            iSessionDuration = 0;
            iSessionDurationTimeScale = 0;
            iNumTracks = 0;
            iSessionDurationAvailable = false;
            iSessionDuration = 0;
            iSessionDurationTimeScale = 0;
            iNumTracks = 0;
            iRandomAccessDenied = false;
            iNumWMPicture = 0;

            iExtendedMetaDataDescriptorCount = 0;
        };

        void Reset()
        {
            iTitlePresent = false;
            iDescriptionPresent = false;
            iCopyRightPresent = false;
            iPerformerPresent = false;
            iAuthorPresent = false;
            iGenrePresent = false;
            iRatingPresent = false;
            iClassificationPresent = false;
            iKeyWordsPresent = false;
            iLocationPresent = false;
            iLyricsPresent = false;
            iWMPicturePresent = false;

            iIsTitleUnicode = false;
            iIsDescriptionUnicode = false;
            iIsCopyRightUnicode = false;
            iIsPerformerUnicode = false;
            iIsAuthorUnicode = false;
            iIsGenreUnicode = false;
            iIsRatingUnicode = false;
            iIsClassificationUnicode = false;
            iIsKeyWordsUnicode = false;
            iIsLocationUnicode = false;
            iIsLyricsUnicode = false;

            iSessionDurationAvailable = false;
            iSessionDuration = 0;
            iSessionDurationTimeScale = 0;
            iNumTracks = 0;
            iRandomAccessDenied = false;
            iNumWMPicture = 0;

            iExtendedMetaDataNameVec.clear();
            iExtendedMetaDataValueTypeVec.clear();
            iExtendedMetaDataValueLenVec.clear();
            iExtendedMetaDataIndexVec.clear();

            iTrackMetaDataInfoVec.clear();
        };

        PVMFSMTrackMetaDataInfo* getTrackMetaDataInfo(uint32 aId)
        {
            Oscl_Vector<PVMFSMTrackMetaDataInfo, PVMFStreamingManagerNodeAllocator>::iterator it;
            for (it = iTrackMetaDataInfoVec.begin(); it != iTrackMetaDataInfoVec.end(); it++)
            {
                if (it->iTrackID == aId)
                {
                    return (it);
                }
            }
            return NULL;
        };

        bool iTitlePresent;
        bool iDescriptionPresent;
        bool iCopyRightPresent;
        bool iPerformerPresent;
        bool iAuthorPresent;
        bool iGenrePresent;
        bool iRatingPresent;
        bool iClassificationPresent;
        bool iKeyWordsPresent;
        bool iLocationPresent;
        bool iLyricsPresent;
        bool iWMPicturePresent;

        bool iIsTitleUnicode;
        bool iIsDescriptionUnicode;
        bool iIsCopyRightUnicode;
        bool iIsPerformerUnicode;
        bool iIsAuthorUnicode;
        bool iIsGenreUnicode;
        bool iIsRatingUnicode;
        bool iIsClassificationUnicode;
        bool iIsKeyWordsUnicode;
        bool iIsLocationUnicode;
        bool iIsLyricsUnicode;
        uint32 iNumWMPicture;

        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iTitle;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iDescription;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iCopyright;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iPerformer;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iAuthor;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iGenre;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iRating;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iClassification;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iKeyWords;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iLocation;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iLyrics;

        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iTitleUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iDescriptionUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iCopyrightUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iPerformerUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iAuthorUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iGenreUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iRatingUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iClassificationUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iKeyWordUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iLocationUnicode;
        OSCL_wHeapString<PVMFStreamingManagerNodeAllocator> iLyricsUnicode;

        bool   iSessionDurationAvailable;
        uint64 iSessionDuration;
        uint64 iSessionDurationTimeScale;
        uint32 iNumTracks;
        bool   iRandomAccessDenied;

        Oscl_Vector<PVMFSMTrackMetaDataInfo, PVMFStreamingManagerNodeAllocator> iTrackMetaDataInfoVec;

        uint32 iExtendedMetaDataDescriptorCount;
        Oscl_Vector<OSCL_HeapString<PVMFStreamingManagerNodeAllocator>, PVMFStreamingManagerNodeAllocator> iExtendedMetaDataNameVec;
        Oscl_Vector<uint16, PVMFStreamingManagerNodeAllocator> iExtendedMetaDataValueTypeVec;
        Oscl_Vector<uint32, PVMFStreamingManagerNodeAllocator> iExtendedMetaDataValueLenVec;
        Oscl_Vector<uint32, PVMFStreamingManagerNodeAllocator> iExtendedMetaDataIndexVec;
        Oscl_Vector<uint32, PVMFStreamingManagerNodeAllocator> iWMPictureIndexVec;
};


/*
 * Structure to contain a node and all ports and extensions associated to it
 */
enum PVMFSMNodeCmdState
{
    PVMFSM_NODE_CMD_PENDING,
    PVMFSM_NODE_CMD_COMPLETE,
    PVMFSM_NODE_CMD_NO_PENDING,
    PVMFSM_NODE_CMD_CANCEL_PENDING,
    PVMFSM_NODE_CMD_CANCEL_COMPLETE
};

class PVMFSMNodeContainer
{
    public:
        PVMFSMNodeContainer()
        {
            iAutoPaused = false;
            iNodeCmdState = PVMFSM_NODE_CMD_NO_PENDING;
        };

        PVMFSMNodeContainer(const PVMFSMNodeContainer& inContainer)
        {
            commandStartOffset = inContainer.commandStartOffset;
            iNode = inContainer.iNode;
            iSessionId = inContainer.iSessionId;
            iNodeTag = inContainer.iNodeTag;
            iInputPorts = inContainer.iInputPorts;
            iOutputPorts = inContainer.iOutputPorts;
            iFeedBackPorts = inContainer.iFeedBackPorts;
            iExtensions = inContainer.iExtensions;
            iExtensionUuids = inContainer.iExtensionUuids;
            iAutoPaused = inContainer.iAutoPaused;
            iNodeCmdState = inContainer.iNodeCmdState;
        }

        PVMFSMNodeContainer& operator=(const PVMFSMNodeContainer& inContainer)
        {
            if (&inContainer != this)
            {
                commandStartOffset = inContainer.commandStartOffset;
                iNode = inContainer.iNode;
                iSessionId = inContainer.iSessionId;
                iNodeTag = inContainer.iNodeTag;
                iInputPorts = inContainer.iInputPorts;
                iOutputPorts = inContainer.iOutputPorts;
                iFeedBackPorts = inContainer.iFeedBackPorts;
                iExtensions = inContainer.iExtensions;
                iExtensionUuids = inContainer.iExtensionUuids;
                iAutoPaused = inContainer.iAutoPaused;
                iNodeCmdState = inContainer.iNodeCmdState;
            }
            return *this;
        }

        virtual ~PVMFSMNodeContainer() {};

        void Reset()
        {
            iInputPorts.clear();
            iOutputPorts.clear();
            iFeedBackPorts.clear();
            for (uint32 i = 0; i < iExtensions.size(); i++)
            {
                iExtensions[i]->removeRef();
            }
            iExtensions.clear();
            iExtensionUuids.clear();
            iAutoPaused = false;
        }

        int32         commandStartOffset;
        PVMFNodeInterface* iNode;
        PVMFSessionId iSessionId;
        int32         iNodeTag;
        Oscl_Vector<PVMFPortInterface*, PVMFStreamingManagerNodeAllocator> iInputPorts;
        Oscl_Vector<PVMFPortInterface*, PVMFStreamingManagerNodeAllocator> iOutputPorts;
        Oscl_Vector<PVMFPortInterface*, PVMFStreamingManagerNodeAllocator> iFeedBackPorts;
        Oscl_Vector<PVInterface*, PVMFStreamingManagerNodeAllocator> iExtensions;
        Oscl_Vector<PVUuid, PVMFStreamingManagerNodeAllocator> iExtensionUuids;
        bool iAutoPaused;

        PVMFSMNodeCmdState iNodeCmdState;

};
typedef Oscl_Vector<PVMFSMNodeContainer, PVMFStreamingManagerNodeAllocator> PVMFSMNodeContainerVector;

/*
 * Track related structures
 */
class PVMFSMTrackInfo
{
    public:
        PVMFSMTrackInfo()
        {
            trackID = 0;
            rdtStreamID = 0;
            portTag = 0;
            bitRate = 0;
            trackTimeScale = 1;
            iNetworkNodePort = NULL;
            iJitterBufferInputPort = NULL;
            iJitterBufferOutputPort = NULL;
            iMediaLayerInputPort = NULL;
            iMediaLayerOutputPort = NULL;
            iJitterBufferRTCPPort = NULL;
            iNetworkNodeRTCPPort = NULL;
            iSessionControllerOutputPort = NULL;
            iSessionControllerFeedbackPort = NULL;
            iRTPSocketID = 0;
            iRTCPSocketID = 0;
            iRateAdaptation = false;
            iRateAdaptationFeedBackFrequency = 0;
            iFormatType = PVMF_FORMAT_UNKNOWN;
            iBitStreamSwitchingInProgress = false;
            iNewTrackID = 0;
            iRTCPBwSpecified = false;
            iTrackDisable = false;
        };

        PVMFSMTrackInfo(const PVMFSMTrackInfo& a)
        {
            trackID = a.trackID;
            rdtStreamID = a.rdtStreamID;
            portTag = a.portTag;
            bitRate = a.bitRate;
            trackTimeScale = a.trackTimeScale;
            iTrackConfig = a.iTrackConfig;
            iTransportType = a.iTransportType;
            iFormatType = a.iFormatType;
            iMimeType = a.iMimeType;
            iNetworkNodePort = a.iNetworkNodePort;
            iJitterBufferInputPort = a.iJitterBufferInputPort;
            iJitterBufferOutputPort = a.iJitterBufferOutputPort;
            iMediaLayerInputPort = a.iMediaLayerInputPort;
            iMediaLayerOutputPort = a.iMediaLayerOutputPort;
            iJitterBufferRTCPPort = a.iJitterBufferRTCPPort;
            iNetworkNodeRTCPPort = a.iNetworkNodeRTCPPort;
            iSessionControllerOutputPort = a.iSessionControllerOutputPort;
            iSessionControllerFeedbackPort = a.iSessionControllerFeedbackPort;
            iRTPSocketID = a.iRTPSocketID;
            iRTCPSocketID = a.iRTCPSocketID;
            iRateAdaptation = a.iRateAdaptation;
            iRateAdaptationFeedBackFrequency = a.iRateAdaptationFeedBackFrequency;
            iBitStreamSwitchingInProgress = a.iBitStreamSwitchingInProgress;
            iNewTrackID = a.iNewTrackID;
            iRTCPBwSpecified = a.iRTCPBwSpecified;
            iTrackDisable = a.iTrackDisable;
            iRR = a.iRR;
            iRS = a.iRS;

        };

        PVMFSMTrackInfo& operator=(const PVMFSMTrackInfo& a)
        {
            if (&a != this)
            {
                trackID = a.trackID;
                rdtStreamID = a.rdtStreamID;
                portTag = a.portTag;
                bitRate = a.bitRate;
                trackTimeScale = a.trackTimeScale;
                iTrackConfig = a.iTrackConfig;
                iTransportType = a.iTransportType;
                iFormatType = a.iFormatType;
                iMimeType = a.iMimeType;
                iNetworkNodePort = a.iNetworkNodePort;
                iJitterBufferInputPort = a.iJitterBufferInputPort;
                iJitterBufferOutputPort = a.iJitterBufferOutputPort;
                iMediaLayerInputPort = a.iMediaLayerInputPort;
                iMediaLayerOutputPort = a.iMediaLayerOutputPort;
                iJitterBufferRTCPPort = a.iJitterBufferRTCPPort;
                iNetworkNodeRTCPPort = a.iNetworkNodeRTCPPort;
                iSessionControllerOutputPort = a.iSessionControllerOutputPort;
                iSessionControllerFeedbackPort = a.iSessionControllerFeedbackPort;
                iRTPSocketID = a.iRTPSocketID;
                iRTCPSocketID = a.iRTCPSocketID;
                iRateAdaptation = a.iRateAdaptation;
                iRateAdaptationFeedBackFrequency = a.iRateAdaptationFeedBackFrequency;
                iBitStreamSwitchingInProgress = a.iBitStreamSwitchingInProgress;
                iNewTrackID = a.iNewTrackID;
                iRTCPBwSpecified = a.iRTCPBwSpecified;
                iTrackDisable = a.iTrackDisable;
                iRR = a.iRR;
                iRS = a.iRS;

            }
            return *this;
        };

        virtual ~PVMFSMTrackInfo() {};

        uint32 trackID;
        uint32 rdtStreamID;
        uint32 portTag;
        uint32 bitRate;
        uint32 trackTimeScale;
        OsclRefCounterMemFrag iTrackConfig;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iTransportType;
        PVMFFormatType iFormatType;
        OSCL_HeapString<PVMFStreamingManagerNodeAllocator> iMimeType;
        PVMFPortInterface* iNetworkNodePort;
        PVMFPortInterface* iJitterBufferInputPort;
        PVMFPortInterface* iJitterBufferOutputPort;
        PVMFPortInterface* iMediaLayerInputPort;
        PVMFPortInterface* iMediaLayerOutputPort;
        PVMFPortInterface* iJitterBufferRTCPPort;
        PVMFPortInterface* iNetworkNodeRTCPPort;
        PVMFPortInterface* iSessionControllerOutputPort;
        PVMFPortInterface* iSessionControllerFeedbackPort;
        uint32 iRTPSocketID;
        uint32 iRTCPSocketID;
        bool   iRateAdaptation;
        uint32 iRateAdaptationFeedBackFrequency;

        // RTCP bandwidth related
        bool   iRTCPBwSpecified;
        uint32 iRR;
        uint32 iRS;

        /* Bitstream switching related */
        bool iBitStreamSwitchingInProgress;
        uint32 iNewTrackID;

        //Check track disable or not
        bool iTrackDisable;


};
typedef Oscl_Vector<PVMFSMTrackInfo, PVMFStreamingManagerNodeAllocator> PVMFSMTrackInfoVector;

/*
*  class for saving kvps for CPM
*/
typedef Oscl_Vector<PvmiKvp, PVMFStreamingManagerNodeAllocator> PVMFKvpVector;

class PVMFSMNodeKVPStore
{
    public:
        // add kvp string with W-string value
        PVMFStatus addKVPString(const char* aKeyTypeString, OSCL_wString& aValString);
        // add kvp string with normal string value
        PVMFStatus addKVPString(const char* aKeyTypeString, const char* aValString);
        // add kvp string with normal uint32 value
        PVMFStatus addKVPuint32Value(const char* aKeyTypeString, uint32 aValue);

        // get the vector for the all constructed KVPs
        PVMFKvpVector* getKVPStore()
        {
            return &iKvpVector;
        }

        // check emptiness of the store
        bool isEmpty() const
        {
            return iKvpVector.empty();
        }

        // release memory for all the constructed KVPs and clear the vector
        void destroy()
        {
            releaseMemory();
            clear();
        }

        // constructor and destructor
        PVMFSMNodeKVPStore()
        {
            clear();
        }
        ~PVMFSMNodeKVPStore()
        {
            destroy();
        }

    private:
        enum KVPValueTypeForMemoryRelease
        {
            KVPValueTypeForMemoryRelease_NoInterest = 0,
            KVPValueTypeForMemoryRelease_String,
            KVPValueTypeForMemoryRelease_WString
        };

        void clear()
        {
            iKvpVector.clear();
            iKVPValueTypeForMemoryRelease.clear();
        }
        void releaseMemory();

    private:
        PVMFKvpVector iKvpVector;
        Oscl_Vector<uint32, PVMFStreamingManagerNodeAllocator> iKVPValueTypeForMemoryRelease;
};


class PVMFStreamingManagerNode : public PVMFNodeInterface,
            public OsclActiveObject,
            public PVMFNodeErrorEventObserver,
            public PVMFNodeInfoEventObserver,
            public PVMFNodeCmdStatusObserver,
            public PvmiCapabilityAndConfig,
            public PVMFCPMStatusObserver
{
    public:
        OSCL_IMPORT_REF PVMFStreamingManagerNode(int32 aPriority);
        OSCL_IMPORT_REF virtual ~PVMFStreamingManagerNode();

        //from PVMFNodeInterface
        OSCL_IMPORT_REF PVMFStatus ThreadLogon();
        OSCL_IMPORT_REF PVMFStatus ThreadLogoff();
        OSCL_IMPORT_REF PVMFStatus GetCapability(PVMFNodeCapability& aNodeCapability);
        OSCL_IMPORT_REF PVMFPortIter* GetPorts(const PVMFPortFilter* aFilter = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryUUID(PVMFSessionId,
                                                const PvmfMimeString& aMimeType,
                                                Oscl_Vector< PVUuid, PVMFStreamingManagerNodeAllocator >& aUuids,
                                                bool aExactUuidsOnly = false,
                                                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId QueryInterface(PVMFSessionId, const PVUuid& aUuid,
                PVInterface*& aInterfacePtr,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId RequestPort(PVMFSessionId,
                int32 aPortTag,
                const PvmfMimeString* aPortConfig = NULL,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId ReleasePort(PVMFSessionId,
                PVMFPortInterface& aPort,
                const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Init(PVMFSessionId,
                                           const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Prepare(PVMFSessionId,
                                              const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Start(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Stop(PVMFSessionId,
                                           const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Flush(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Pause(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId Reset(PVMFSessionId,
                                            const OsclAny* aContext = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelAllCommands(PVMFSessionId,
                const OsclAny* aContextData = NULL);
        OSCL_IMPORT_REF PVMFCommandId CancelCommand(PVMFSessionId,
                PVMFCommandId aCmdId,
                const OsclAny* aContextData = NULL);

        /**
         * Handle an error event that has been generated.
         *
         * @param "aEvent" "The event to be handled."
         */
        virtual void HandleNodeErrorEvent(const PVMFAsyncEvent& aEvent);
        /**
         * Handle an informational event that has been generated.
         *
         * @param "aEvent" "The event to be handled."
         */
        virtual void HandleNodeInformationalEvent(const PVMFAsyncEvent& aEvent);
        /**
         * Handle an event that has been generated.
         *
         * @param "aResponse"	"The response to a previously issued command."
         */
        virtual void NodeCommandCompleted(const PVMFCmdResp& aResponse);

        /* from PVMFPortActivityHandler */
        void HandlePortActivity(const PVMFPortActivity& aActivity);

        PVMFSMNodeContainer* getNodeContainer(int32 tag);


        //// capability and config interface

        virtual void setObserver(PvmiConfigAndCapabilityCmdObserver* aObserver)
        {
            ciObserver = aObserver;
        }

        virtual PVMFStatus getParametersSync(PvmiMIOSession aSession,
                                             PvmiKeyType aIdentifier,
                                             PvmiKvp*& aParameters,
                                             int& aNumParamElements,
                                             PvmiCapabilityContext aContext);
        virtual PVMFStatus releaseParameters(PvmiMIOSession aSession,
                                             PvmiKvp* aParameters,
                                             int num_elements);
        virtual void createContext(PvmiMIOSession aSession, PvmiCapabilityContext& aContext);
        virtual void setContextParameters(PvmiMIOSession aSession, PvmiCapabilityContext& aContext,
                                          PvmiKvp* aParameters, int num_parameter_elements);
        virtual void DeleteContext(PvmiMIOSession aSession,
                                   PvmiCapabilityContext& aContext);
        virtual void setParametersSync(PvmiMIOSession aSession, PvmiKvp* aParameters,
                                       int num_elements, PvmiKvp * & aRet_kvp);
        virtual PVMFCommandId setParametersAsync(PvmiMIOSession aSession,
                PvmiKvp* aParameters,
                int num_elements,
                PvmiKvp*& aRet_kvp,
                OsclAny* context = NULL);
        virtual uint32 getCapabilityMetric(PvmiMIOSession aSession);
        virtual PVMFStatus verifyParametersSync(PvmiMIOSession aSession,
                                                PvmiKvp* aParameters,
                                                int num_elements);
        /* From PVMFCPMStatusObserver */
        void CPMCommandCompleted(const PVMFCmdResp& aResponse);

    private:
        //from OsclActiveObject
        void Run();
        void DoCancel();

        PVMFStreamingManagerNodeCmdQ iInputCommands;
        PVMFStreamingManagerNodeCmdQ iCurrentCommand;
        PVMFStreamingManagerNodeCmdQ iCancelCommand;
        PVMFNodeCapability iCapability;
        PVLogger *iLogger;
        PVLogger *iCmdSeqLogger;
        PVLogger *iReposLogger;

        PVInterface* iExtensionInterfacePlaceholder;
        PVMFStreamingManagerExtensionInterfaceImpl *iExtensionInterface;

        PVMFSMNodeContainerVector  iNodeContainerVec;
        bool iQueryUUIDComplete;
        bool iQueryInterfaceComplete;

        uint32 iNumChildNodesQueryUUIDPending;
        uint32 iNumChildNodesQueryUUIDComplete;
        uint32 iNumChildNodesQueryInterfacePending;
        uint32 iNumChildNodesQueryInterfaceComplete;

        int32 iNumChildNodesStartPending;
        int32 iNumChildNodesStopPending;
        bool iSendBOS;
        uint32 iStreamID;

        uint32 iNumCancelAllCommandsIssued;
        uint32 iNumCancelAllCommandsComplete;



        PVMFSMSessionSourceInfo *iSessionSourceInfo;
        PVMFStatus setSessionSourceInfo(OSCL_wString&,
                                        PVMFFormatType,
                                        OsclAny*);

        PVMFStatus GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo);
        PVMFStatus SelectTracks(PVMFMediaPresentationInfo&, PVMFSessionId);
        PVMFMediaPresentationInfo iCompleteMediaPresetationInfo;
        PVMFMediaPresentationInfo iSelectedMediaPresetationInfo;

        PVMFSMTrackInfoVector iTrackInfoVec;
        uint32 iTotalNumRequestPortsComplete;
        uint32 iNumRequestPortsPending;
        uint32 iTotalNumReleasePortsComplete;
        uint32 iNumReleasePortsPending;
        /* Init phase port requests */
        uint32 iTotalNumInitPhaseRequestPortsComplete;
        uint32 iTotalNumInitPhaseRequestPortsPending;
        bool PopulateTrackInfoVec();
        PVMFSMTrackInfo* FindTrackInfo(uint32 trackID);
        PVMFSMTrackInfo* FindTrackInfo(OSCL_String* trackMimeType);
        uint32 iJitterBufferDurationInMilliSeconds;
        void setJitterBufferDurationInMilliSeconds(uint32 duration)
        {
            iJitterBufferDurationInMilliSeconds = duration;
        }


        /* Command processing */
        PVMFSMCommandContext iInternalCmdPool[PVMF_STREAMING_MANAGER_INTERNAL_CMDQ_SIZE];

        PVMFCommandId QueueCommandL(PVMFStreamingManagerNodeCommand&);
        bool ProcessCommand(PVMFStreamingManagerNodeCommand&);
        void CommandComplete(PVMFStreamingManagerNodeCmdQ& aCmdQ,
                             PVMFStreamingManagerNodeCommand& aCmd,
                             PVMFStatus aStatus,
                             OsclAny* aEventData,
                             PVUuid* aEventUUID,
                             int32* aEventCode,
                             PVInterface* aExtMsg);
        void CommandComplete(PVMFStreamingManagerNodeCmdQ&,
                             PVMFStreamingManagerNodeCommand&,
                             PVMFStatus,
                             OsclAny* aData = NULL,
                             PVUuid* aEventUUID = NULL,
                             int32* aEventCode = NULL);
        void CommandComplete(PVMFStreamingManagerNodeCommand&,
                             PVMFStatus,
                             OsclAny* aData = NULL,
                             PVUuid* aEventUUID = NULL,
                             int32* aEventCode = NULL);
        void CommandComplete(PVMFStreamingManagerNodeCmdQ&,
                             PVMFStreamingManagerNodeCommand&,
                             PVMFStatus,
                             PVInterface* aErrorExtIntf);
        void CommandComplete(PVMFStreamingManagerNodeCommand&,
                             PVMFStatus,
                             PVInterface* aErrorExtIntf,
                             OsclAny* aData = NULL);
        void InternalCommandComplete(PVMFStreamingManagerNodeCmdQ&,
                                     PVMFStreamingManagerNodeCommand&,
                                     PVMFStatus,
                                     OsclAny* aData = NULL);
        void InternalCommandComplete(PVMFStreamingManagerNodeCommand&,
                                     PVMFStatus,
                                     OsclAny* aData = NULL);


        void DoQueryUuid(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesQueryUuid();
        void CompleteQueryUuid();

        void DoQueryInterface(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesQueryInterface();
        void CompleteQueryInterface();

        void DoRequestPort(PVMFStreamingManagerNodeCommand&);
        void DoReleasePort(PVMFStreamingManagerNodeCommand&);

        PVMFStatus DeleteUnusedSessionControllerNode();
        PVMFStatus DeleteUnusedNodes();

        PVMFStatus DoPreInit(PVMFStreamingManagerNodeCommand& aCmd);
        void CompletePreInit();
        void DoInit(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesInit();
        void CompleteInit();
        void PopulatePayloadParserRegistry();
        void destroyPayloadParserRegistry();
        PVMFStatus ProcessSDP();
        PVMFStatus RecognizeAndProcessHeader();

        void DoPrepare(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesPrepare();
        bool SendSessionControlPrepareCompleteParams();
        void CompletePrepare();

        void DoStart(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesStart();
        bool CompleteFeedBackPortsSetup();
        bool SendSessionControlStartCompleteParams();
        bool SendPacketSourceStartCompleteParams();
        void CompleteStart();

        void DoStop(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesStop();
        void CompleteStop();
        void ResetStopCompleteParams();

        void DoFlush(PVMFStreamingManagerNodeCommand&);
        bool FlushPending();
        bool CheckChildrenNodesFlush();
        void CompleteFlush();

        void DoPause(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesPause();
        void CompletePause();

        void DoReset(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesReset();
        void CompleteReset();
        void ResetNodeParams();

        void DoAutoPause(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesAutoPause();
        void CompleteAutoPause();

        void DoAutoResume(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesAutoResume();
        void CompleteAutoResume();

        void DoCancelAllCommands(PVMFStreamingManagerNodeCommand&);
        bool CheckChildrenNodesCancelAll();
        void CompleteCancelAll();

        void DoCancelCommand(PVMFStreamingManagerNodeCommand&);

        bool oGraphConstructComplete;
        bool GraphConstruct();
        bool ConstructGraphFor3GPPUDPStreaming();
        bool ConstructGraphFor3GPPTCPStreaming();
        bool SetSocketNodePortAllocator();
        bool ConstructGraphForMSHTTPStreaming();
        void CompleteGraphConstruct();
        bool ReserveSockets();
        bool RequestNetworkNodePorts(int32, uint32&);
        bool RequestRTSPNodePorts(int32, uint32&);
        bool RequestJitterBufferPorts(int32, uint32&);
        bool RequestMediaLayerPorts(int32, uint32&);
        bool RequestRtpPacketSourcePorts(int32, uint32&);
        bool oGraphConnectComplete;
        bool GraphConnectFor3GPPUDPStreaming();
        bool GraphConnectFor3GPPTCPStreaming();
        bool GraphConnectForMSHTTPStreaming();
        bool GraphConnectForRTPPacketSource();
        bool GraphConnect();
        PVMFStatus ConnectPortPairs(PVMFPortInterface*, PVMFPortInterface*);
        bool SendSessionSourceInfoToSessionController();
        void ResetNodeContainerCmdState();
        enum SMErrorEventSource
        {
            SM_NO_ERROR,
            SM_NODE_COMMAND_COMPLETION,
            SM_ERROR_EVENT
        };
        SMErrorEventSource iErrorDuringProcess;

        /* Event reporting */
        void ReportErrorEvent(PVMFEventType aEventType,
                              OsclAny* aEventData = NULL,
                              PVUuid* aEventUUID = NULL,
                              int32* aEventCode = NULL);
        void ReportInfoEvent(PVMFEventType aEventType,
                             OsclAny* aEventData = NULL,
                             PVUuid* aEventUUID = NULL,
                             int32* aEventCode = NULL);
        void SetState(TPVMFNodeInterfaceState);

        void CleanUp();
        void MoveCmdToCurrentQueue(PVMFStreamingManagerNodeCommand& aCmd);
        void MoveCmdToCancelQueue(PVMFStreamingManagerNodeCommand& aCmd);
        PVMFSMCommandContext* RequestNewInternalCmd();

        /*
         * Internal command handlers
         */
        void HandleRTSPSessionControllerCommandCompleted(const PVMFCmdResp& aResponse,
                bool& aResponseOverRide);
        void HandleSocketNodeCommandCompleted(const PVMFCmdResp& aResponse);
        void HandleJitterBufferCommandCompleted(const PVMFCmdResp& aResponse);
        void HandleMediaLayerCommandCompleted(const PVMFCmdResp& aResponse);

        void HandleRtpPacketSourceCommandCompleted(const PVMFCmdResp& aResponse);
        bool ConstructGraphForRTPPacketSource();

        /* Session start & stop times */
        uint32 iSessionStartTime;
        uint32 iSessionStopTime;
        bool   iSessionStopTimeAvailable;
        bool   iSessionSeekAvailable;

        /* Reposition related */
        bool oRepositioning;
        PVMFTimestamp iRepositionRequestedStartNPTInMS;
        PVMFTimestamp iActualRepositionStartNPTInMS;
        PVMFTimestamp* iActualRepositionStartNPTInMSPtr;
        PVMFTimestamp iActualMediaDataTS;
        PVMFTimestamp* iActualMediaDataTSPtr;
        bool iJumpToIFrame;
        PVMFDataSourcePositionParams* iPVMFDataSourcePositionParamsPtr;
        bool oPlayListRepositioning;

        PVMFCommandId SetDataSourcePosition(PVMFSessionId aSessionId,
                                            PVMFTimestamp aTargetNPT,
                                            PVMFTimestamp& aActualNPT,
                                            PVMFTimestamp& aActualMediaDataTS,
                                            bool aSeekToSyncPoint = true,
                                            uint32 aStreamID = 0,
                                            OsclAny* aContext = false);
        PVMFCommandId SetDataSourcePosition(PVMFSessionId aSessionId,
                                            PVMFDataSourcePositionParams& aPVMFDataSourcePositionParams,
                                            OsclAny* aContext = false);
        void DoSetDataSourcePosition(PVMFStreamingManagerNodeCommand&);
        void DoSetDataSourcePositionPlayList(PVMFStreamingManagerNodeCommand&);
        bool DoRepositioningPause3GPPStreaming();
        bool DoRepositioningPauseMSHTTPStreaming();
        bool DoRepositioningStart3GPPStreaming();
        bool DoRepositioningStart3GPPPlayListStreaming();
        bool DoRepositioningStart3GPPPlayListStreamingDuringPlay();
        bool DoRepositioningStartMSHTTPStreaming();
        bool DoSessionControllerSeek();
        void GetAcutalMediaTSAfterSeek();
        PVMFCommandId QueryDataSourcePosition(PVMFSessionId aSessionId,
                                              PVMFTimestamp aTargetNPT,
                                              PVMFTimestamp& aActualNPT,
                                              bool aSeekToSyncPoint = true,
                                              OsclAny* aContext = false);

        PVMFCommandId QueryDataSourcePosition(PVMFSessionId aSessionId,
                                              PVMFTimestamp aTargetNPT,
                                              PVMFTimestamp& aSeekPointBeforeTargetNPT,
                                              PVMFTimestamp& aSeekPointAfterTargetNPT,
                                              OsclAny* aContext = NULL,
                                              bool aSeekToSyncPoint = true);

        void DoQueryDataSourcePosition(PVMFStreamingManagerNodeCommand&);
        PVMFCommandId SetDataSourceRate(PVMFSessionId aSessionId,
                                        int32 aRate,
                                        OsclTimebase* aTimebase = NULL,
                                        OsclAny* aContext = NULL);
        void DoSetDataSourceRate(PVMFStreamingManagerNodeCommand&);
        /* HTTP Streaming reposition related */
        uint32 iFirstSeqNumAfterSeek;

        /*
         * Meta data related
         */
        PVMFSMSessionMetaDataInfo iMetaDataInfo;
        uint32 GetNumMetadataKeys(char* aQueryKeyString = NULL);
        uint32 GetNumMetadataValues(PVMFMetadataList& aKeyList);
        PVMFCommandId GetNodeMetadataKeys(PVMFSessionId aSessionId,
                                          PVMFMetadataList& aKeyList,
                                          uint32 aStartingKeyIndex,
                                          int32 aMaxKeyEntries,
                                          char* aQueryKeyString = NULL,
                                          const OsclAny* aContextData = NULL);
        PVMFCommandId GetNodeMetadataValues(PVMFSessionId aSessionId,
                                            PVMFMetadataList& aKeyList,
                                            Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
                                            uint32 aStartingValueIndex,
                                            int32 aMaxValueEntries,
                                            const OsclAny* aContextData = NULL);
        PVMFStatus ReleaseNodeMetadataKeys(PVMFMetadataList& aKeyList,
                                           uint32 aStartingKeyIndex,
                                           uint32 aEndKeyIndex);
        PVMFStatus ReleaseNodeMetadataValues(Oscl_Vector<PvmiKvp, OsclMemAllocator>& aValueList,
                                             uint32 aStartingValueIndex,
                                             uint32 aEndValueIndex);


        PVMFStatus DoGetMetadataKeys(PVMFStreamingManagerNodeCommand& aCmd);
        PVMFStatus DoGetMetadataValues(PVMFStreamingManagerNodeCommand& aCmd);

        PVMFStatus InitMetaData();
        PVMFStatus GetIndexParamValues(char* aString, uint32& aStartIndex, uint32& aEndIndex);
        PVMFStatus CreateKVPForCharStringValue(PvmiKvp& aKeyVal,
                                               const char* aKeyTypeString,
                                               const char* aValString,
                                               char* aMiscKeyParam = NULL);
        PVMFStatus CreateKVPForWideCharStringValue(PvmiKvp& aKeyVal,
                const char* aKeyTypeString,
                const oscl_wchar* aValString,
                char* aMiscKeyParam = NULL);
        PVMFStatus CreateKVPForUInt32Value(PvmiKvp& aKeyVal,
                                           const char* aKeyTypeString,
                                           uint32& aValueUInt32,
                                           char* aMiscKeyParam = NULL);
        PVMFStatus CreateKVPForBoolValue(PvmiKvp& aKeyVal,
                                         const char* aKeyTypeString,
                                         bool& aValueBool,
                                         char* aMiscKeyParam = NULL);

        // remove the ending ';', ',' or ' ' and calulate value length
        // for protocol extension header kvp handling
        uint32 getItemLen(char *ptrValueStart, char *ptrValueEnd);


        Oscl_Vector<OSCL_HeapString<OsclMemAllocator>, OsclMemAllocator> iAvailableMetadataKeys;
        Oscl_Vector<OSCL_HeapString<PVMFStreamingManagerNodeAllocator>, PVMFStreamingManagerNodeAllocator> iCPMMetadataKeys;

        /* Auto pause / Auto resume related */
        bool iAutoPausePending;
        int32 iAutoResumePending;
        bool iAutoPaused;

        /* Stream Thinning related */
        bool iStreamThinningInProgress;
        bool iPlaylistPlayInProgress;
        uint32 iStreamThinningRequestTime;
        uint32 iFirstSeqNumberAfterSwitch;

        PVMFStatus GetConfigParameter(PvmiKvp*& aParameters, int& aNumParamElements,
                                      int32 aIndex, PvmiKvpAttr reqattr);
        PVMFStatus VerifyAndSetConfigParameter(int index, PvmiKvp& aParameter, bool set);

        int    iNumJittBufferGrowParamsSet;
        uint32 iJitterBufferNumGrows;
        uint32 iJitterBufferGrowSize;
        bool ibCloaking;
        bool iSwitchStreamIFrameVideo;

        friend class PVMFStreamingManagerExtensionInterfaceImpl;

        bool ibRdtTransport;

        bool IsInternalCmd(PVMFCommandId aId);

        void Asf_CompletePreInit();
        bool Asf_PopulateTrackInfoVec();
        bool Asf_SetSocketNodePortAllocator();
        PVMFStatus Asf_RecognizeAndProcessHeader();
        PVMFStatus Asf_GetMediaPresentationInfo(PVMFMediaPresentationInfo& aInfo);
        PVMFStatus Asf_SelectTracks(PVMFMediaPresentationInfo&, PVMFSessionId);
        PVMFStatus Asf_InitMetaData();
        PVMFStatus Asf_GetExtendedMetaData(PvmiKvp& KeyVal, uint32 aIndex);
        bool IsAsfStreamingSupported();
        bool Asf_Doprepare(PVMFStreamingManagerNodeCommand& aCmd);
        void Asf_PopulateDRMInfo();
        PVMFStatus Asf_ConvertWMPictureToAPIC(PvmfApicStruct*& aAPICStruct,
                                              bool* aImageTooBig,
                                              uint32 aMaxSize,
                                              uint32* aImageSize,
                                              uint32 aPicIndex,
                                              uint32 aMetadataindex);
        void Asf_DeleteAPICStruct(PvmfApicStruct*& aAPICStruct);

        PVMFStatus GetMaxSizeValue(char* aString, uint32& aMaxSize);
        PVMFStatus GetTruncateFlagValue(char* aString, uint32& aTruncateFlag);

        IRealChallengeGen* ipRealChallengeGen;
        IPayloadParser*    ipRdtParser;

        bool GetRealAudioMimeType(rm_mediaInfo* pinfo, OSCL_String& mime,
                                  OsclMemoryFragment* paacConfig);
        void CreateRealStreamingObjects();
        void DestroyRealStreamingObjects();
        bool IsRealStreamingSupported();

        //// cpm related
        bool iPreviewMode;
        bool iUseCPMPluginRegistry;
        bool iDRMResetPending;
        bool iCPMInitPending;
        uint32 maxPacketSize;
        uint32 iPVMFStreamingManagerNodeMetadataValueCount;
        PVMFStreamingDataSource iCPMSourceData;
        PVMFSourceContextData iSourceContextData;
        bool iSourceContextDataValid;
        PVMFCPM* iCPM;
        PVMFSessionId iCPMSessionID;
        PVMFCPMContentType iCPMContentType;
        PVMFCPMPluginAccessInterfaceFactory* iCPMContentAccessFactory;
        PVMFCPMPluginAccessUnitDecryptionInterface* iDecryptionInterface;
        PVMFMetadataExtensionInterface* iCPMMetaDataExtensionInterface;
        PVMFCPMPluginLicenseInterface* iCPMLicenseInterface;
        PvmiCapabilityAndConfig* iCPMCapConfigInterface;
        PVMFSMNodeKVPStore iCPMKvpStore;
        PvmiKvp iRequestedUsage;
        PvmiKvp iApprovedUsage;
        PvmiKvp iAuthorizationDataKvp;
        PVMFCPMUsageID iUsageID;
        PVMFCommandId iCPMInitCmdId;
        PVMFCommandId iCPMOpenSessionCmdId;
        PVMFCommandId iCPMRegisterContentCmdId;
        PVMFCommandId iCPMRequestUsageId;
        PVMFCommandId iCPMUsageCompleteCmdId;
        PVMFCommandId iCPMCloseSessionCmdId;
        PVMFCommandId iCPMResetCmdId;
        PVMFCommandId iCPMGetMetaDataKeysCmdId;
        PVMFCommandId iCPMGetMetaDataValuesCmdId;
        PVMFCommandId iCPMGetLicenseInterfaceCmdId;
        PVMFCommandId iCPMGetLicenseCmdId;
        PVMFCommandId iCPMGetCapConfigCmdId;
        PVMFCommandId iCPMCancelGetLicenseCmdId;
        PVMFStatus iCPMRequestUsageCommandStatus;

        class V1Header
        {
            public:
                uint8* iKeyID; 	//Array of bytes that contains the KeyID from the V1 ASF header object.
                uint32 iKeyIDSize; //count of bytes in iKeyID;
                uint8* iSecretData; //Array of bytes that contains the SecretData from the V1 ASF header
                uint32 iSecretDataSize;//count of bytes in iSecretData;
                uint8* iURL; //Array of bytes that contains the license acquisition URL from the V1 ASF header
                uint32 iURLSize;//Count of bytes in iURL
        };
        V1Header iV1Header;
        void InitCPM();
        void OpenCPMSession();
        void CPMRegisterContent();
        bool GetCPMContentAccessFactory();
        bool GetCPMMetaDataExtensionInterface();
        void GetCPMLicenseInterface();
        void GetCPMCapConfigInterface();
        bool SetCPMKvps();
        void RequestUsage();
        void SendUsageComplete();
        void CloseCPMSession();
        void ResetCPM();
        void PopulateDRMInfo();
        void GetCPMMetaDataKeys();
        void GetCPMMetaDataValues();
        void CompleteGetMetaDataValues();
        void CompleteDRMInit();
        void CleanupCPMdata();
        PVMFStatus CompleteGetMetadataKeys(PVMFStreamingManagerNodeCommand& aCmd);
        PVMFStatus CheckCPMCommandCompleteStatus(PVMFCommandId aID, PVMFStatus aStatus);

        PVMFCommandId GetLicense(PVMFSessionId aSessionId,
                                 OSCL_wString& aContentName,
                                 OsclAny* aData,
                                 uint32 aDataSize,
                                 int32 aTimeoutMsec,
                                 const OsclAny* aContextData = NULL) ;

        PVMFCommandId GetLicense(PVMFSessionId aSessionId,
                                 OSCL_String&  aContentName,
                                 OsclAny* aData,
                                 uint32 aDataSize,
                                 int32 aTimeoutMsec,
                                 const OsclAny* aContextData = NULL);

        PVMFCommandId CancelGetLicense(PVMFSessionId aSessionId
                                       , PVMFCommandId aCmdId
                                       , OsclAny* aContextData = NULL);

        PVMFStatus GetLicenseStatus(PVMFCPMLicenseStatus& aStatus) ;

        PVMFStatus DoGetLicense(PVMFStreamingManagerNodeCommand& aCmd,
                                bool aWideCharVersion = false);
        void DoCancelGetLicense(PVMFStreamingManagerNodeCommand& aCmd);

        void CompleteGetLicense();

        // for HTTP extension header kvp parsing
        bool IsHttpExtensionHeaderValid(PvmiKvp &aParameter);

        //Check for RTP packet source
        bool IsRTPPacketSourcePresent();

        // for error completion while current cmd is ongoing
        PVInterface* iErrorResponseInf;
        PVMFStatus  iCmdErrStatus;
        OsclAny*	iEventData;
        bool IsFatalErrorEvent(const PVMFEventType& event);
};

#endif


