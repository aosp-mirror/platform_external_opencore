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
/**
 * @file pvmf_mp3ffparser_outport.cpp
 */

#ifndef PVMF_MP3FFPARSER_OUTPORT_H_INCLUDED
#include "pvmf_mp3ffparser_outport.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif
#ifndef PVMF_MP3FFPARSER_DEFS_H_INCLUDED
#include "pvmf_mp3ffparser_defs.h"
#endif

////////////////////////////////////////////////////////////////////////////
PVMFMP3FFParserPort::PVMFMP3FFParserPort(int32 aTag, PVMFNodeInterface* aNode)
        : PvmfPortBaseImpl(aTag, aNode, "Mp3ParOut(Audio)")
{
    Construct();
}

////////////////////////////////////////////////////////////////////////////
PVMFMP3FFParserPort::PVMFMP3FFParserPort(int32 aTag, PVMFNodeInterface* aNode,
        uint32 aInCapacity,
        uint32 aInReserve,
        uint32 aInThreshold,
        uint32 aOutCapacity,
        uint32 aOutReserve,
        uint32 aOutThreshold)
        : PvmfPortBaseImpl(aTag, aNode,
                           aInCapacity,
                           aInReserve,
                           aInThreshold,
                           aOutCapacity,
                           aOutReserve,
                           aOutThreshold,
                           "Mp3ParOut(Audio)")
{
    Construct();
}

////////////////////////////////////////////////////////////////////////////
void PVMFMP3FFParserPort::Construct()
{
    iLogger = PVLogger::GetLoggerObject("PVMFMP3FFParserPort");
    oscl_memset(&iStats, 0, sizeof(PvmfPortBaseImplStats));
    iNumFramesGenerated = 0;
    PvmiCapabilityAndConfigPortFormatImpl::Construct(PVMF_MP3FFPARSER_OUTPORT_FORMATS
            , PVMF_MP3FFPARSER_OUTPORT_FORMATS_VALTYPE);
}

////////////////////////////////////////////////////////////////////////////
PVMFMP3FFParserPort::~PVMFMP3FFParserPort()
{
    // Disconnect the port
    Disconnect();
    // Clear the queued messages
    ClearMsgQueues();
}

////////////////////////////////////////////////////////////////////////////
bool PVMFMP3FFParserPort::IsFormatSupported(PVMFFormatType aFmt)
{
    return (aFmt == PVMF_MP3);
}

////////////////////////////////////////////////////////////////////////////
void PVMFMP3FFParserPort::FormatUpdated()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_MLDBG, iLogger, PVLOGMSG_INFO,
                    (0, "PVMFMP3FFParserPort::FormatUpdated %d", iFormat));
}


