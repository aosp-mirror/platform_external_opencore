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
#ifndef SAMPLE_PLAYER_APP_H_INCLUDED
#define SAMPLE_PLAYER_APP_H_INCLUDED

#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif

#ifndef OSCL_EXCEPTION_H_INCLUDE
#include "oscl_exception.h"
#endif

#ifndef PV_PLAYER_FACTORY_H_INCLUDED
#include "pv_player_factory.h"
#endif

#ifndef PV_PLAYER_INTERFACE_H_INCLUDE
#include "pv_player_interface.h"
#endif

#ifndef PV_ENGINE_OBSERVER_H_INCLUDED
#include "pv_engine_observer.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef PVLOGGER_STDERR_APPENDER_H_INCLUDED
#include "pvlogger_stderr_appender.h"
#endif

#ifndef PVLOGGER_TIME_AND_ID_LAYOUT_H_INCLUDED
#include "pvlogger_time_and_id_layout.h"
#endif

#ifndef PVMI_MEDIA_IO_FILEOUTPUT_H_INCLUDED
#include "pvmi_media_io_fileoutput.h"
#endif

#ifndef PV_PLAYER_DATASOURCEURL_H_INCLUDED
#include "pv_player_datasourceurl.h"
#endif

#ifndef PV_PLAYER_DATASINKFILENAME_H_INCLUDED
#include "pv_player_datasinkfilename.h"
#endif

#ifndef PVMF_ERRORINFOMESSAGE_EXTENSION_H_INCLUDED
#include "pvmf_errorinfomessage_extension.h"
#endif


// for PVLogger
template<class DestructClass>
class LogAppenderDestructDealloc : public OsclDestructDealloc
{
    public:
        virtual void destruct_and_dealloc(OsclAny *ptr)
        {
            delete((DestructClass*)ptr);
        }
};


class pvplayer_engine_interface : public OsclTimerObject,
            public PVCommandStatusObserver,
            public PVInformationalEventObserver,
            public PVErrorEventObserver
{
    public:
        pvplayer_engine_interface(char *aFileName, PVMFFormatType aFileType);
        ~pvplayer_engine_interface();

        // needed for OsclTimerObject
        void Run();

        void CommandCompleted(const PVCmdResponse& aResponse);
        void HandleErrorEvent(const PVAsyncErrorEvent& aEvent);
        void HandleInformationalEvent(const PVAsyncInformationalEvent& aEvent);

        enum PVPlayerState
        {
            STATE_UNKNOWN,
            STATE_CREATE,
            STATE_ADDDATASOURCE,
            STATE_INIT,
            STATE_ADDDATASINK_VIDEO,
            STATE_ADDDATASINK_AUDIO,
            STATE_ADDDATASINK_TEXT,
            STATE_GETMETADATAVALUES,
            STATE_PREPARE,
            STATE_START,
            STATE_EOSNOTREACHED,
            STATE_STOP,
            STATE_REMOVEDATASINK_VIDEO,
            STATE_REMOVEDATASINK_AUDIO,
            STATE_REMOVEDATASINK_TEXT,
            STATE_RESET,
            STATE_REMOVEDATASOURCE,
            STATE_CLEANUPANDCOMPLETE
        };

        PVPlayerState iState;
        PVPlayerInterface* iPlayer;

        PVPlayerDataSourceURL* iDataSource;
        PVPlayerDataSink* iDataSinkVideo;
        PVPlayerDataSink* iDataSinkAudio;
        PVPlayerDataSink* iDataSinkText;

        PVMFNodeInterface* iIONodeVideo;
        PVMFNodeInterface* iIONodeAudio;
        PVMFNodeInterface* iIONodeText;
        PvmiMIOControl* iMIOFileOutVideo;
        PvmiMIOControl* iMIOFileOutAudio;
        PvmiMIOControl* iMIOFileOutText;

        PVCommandId iCurrentCmdId;

        // Utility function to retrieve the filename from string and replace ',' with '_'
        void RetrieveFilename(const oscl_wchar* aSource, OSCL_wHeapString<OsclMemAllocator>& aFilename)
        {
            if (aSource == NULL)
            {
                return;
            }

            // Find the last '\' or '/' in the string
            oscl_wchar* lastslash = (oscl_wchar*)aSource;
            bool foundlastslash = false;
            while (!foundlastslash)
            {
                oscl_wchar* tmp1 = oscl_strstr(lastslash, _STRLIT_WCHAR("\\"));
                oscl_wchar* tmp2 = oscl_strstr(lastslash, _STRLIT_WCHAR("/"));
                if (tmp1 != NULL)
                {
                    lastslash = tmp1 + 1;
                }
                else if (tmp2 != NULL)
                {
                    lastslash = tmp2 + 1;
                }
                else
                {
                    foundlastslash = true;
                }
            }

            // Now copy the filename
            if (lastslash)
            {
                aFilename = lastslash;
            }

            // Replace each '.' in filename with '_'
            bool finishedreplace = false;
            while (!finishedreplace)
            {
                oscl_wchar* tmp = oscl_strstr(aFilename.get_cstr(), _STRLIT_WCHAR("."));
                if (tmp != NULL)
                {
                    oscl_strncpy(tmp, _STRLIT_WCHAR("_"), 1);
                }
                else
                {
                    finishedreplace = true;
                }
            }
        }

        void StartPlayback();



    private:
        char *iFileName;
        PVMFFormatType iFileType;

        //for context data testing
        uint32 iContextObjectRefValue;
        uint32 iContextObject;

        // for metadata retrieval
        PVPMetadataList iMetadataKeyList;
        Oscl_Vector<PvmiKvp, OsclMemAllocator> iMetadataValueList;
        int32 iNumValues;
        uint32 iClipDuration;

        OSCL_wHeapString<OsclMemAllocator> wFileName;
        oscl_wchar output[512];
};


#endif


