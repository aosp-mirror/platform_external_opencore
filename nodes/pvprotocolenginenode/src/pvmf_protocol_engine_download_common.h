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
#ifndef PVMF_PROTOCOLENGINE_DOWNLOAD_COMMON_H_INCLUDED
#define PVMF_PROTOCOLENGINE_DOWNLOAD_COMMON_H_INCLUDED

#ifndef PVMF_PROTOCOLENGINE_COMMON_H_INCLUDED
#include "pvmf_protocol_engine_common.h"
#endif

class DownloadState : public ProtocolState
{
    public:
        void setConfigInfo(OsclAny* aConfigInfo)
        {
            iCfgFile = *((OsclSharedPtr<PVDlCfgFile> *)aConfigInfo);
        }

        // The header for download should be http header, but node will take care of http header
        bool getHeader(Oscl_Vector<OsclRefCounterMemFrag, OsclMemAllocator> &aHeader)
        {
            OSCL_UNUSED_ARG(aHeader);
            return false;
        }

        // constructor
        DownloadState() : iSetContentLengthFlagtoConfigFileObject(false)
        {
            ;
        }
        // virtual destructor, let internal objects destruct automatically
        virtual ~DownloadState()
        {
            ;
        }

    protected:
        virtual int32 processMicroStateSendRequestPreCheck();
        virtual int32 processMicroStateGetResponsePreCheck();
        // To compose a request, only need to override/implement these two functions
        virtual void setRequestBasics();
        virtual bool setHeaderFields();
        virtual int32 updateDownloadStatistics();
        virtual void saveConfig();

        // shared routine for all the download protocols
        virtual int32 checkParsingStatus(int32 parsingStatus);

    protected:
        OsclSharedPtr<PVDlCfgFile> iCfgFile;
        Oscl_Vector<OsclRefCounterMemFrag, OsclMemAllocator> iOutputDataQueue;

    private:
        bool iSetContentLengthFlagtoConfigFileObject;
};

#endif // PVMF_PROTOCOLENGINE_DOWNLOAD_COMMON_H_INCLUDED

