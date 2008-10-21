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
#include "pv_player_datapath.h"

#include "pvlogger.h"

#include "pvmi_config_and_capability_utils.h"

// Temporary until port tag query is ready
#define DEFAULT_INPUT_PORTTAG 0
#define DEFAULT_OUTPUT_PORTTAG 1
#define PORT_CONFIG_INPUT_FORMATS_VALTYPE "x-pvmf/port/formattype;valtype=int32"

//
// PVPlayerDatapath Section
//
PVPlayerDatapath::PVPlayerDatapath() :
        OsclTimerObject(OsclActiveObject::EPriorityNominal, "PVPlayerDatapath"),
        iSourceNode(NULL), iSourceSessionId(0),
        iDecNode(NULL), iDecSessionId(0),
        iSinkNode(NULL), iSinkSessionId(0),
        iSourceOutPort(NULL), iDecInPort(NULL), iDecOutPort(NULL), iSinkInPort(NULL),
        iObserver(NULL),
        iErrorObserver(NULL),
        iInfoObserver(NULL),
        iContext(NULL),
        iSourceDecFormatType(PVMF_FORMAT_UNKNOWN),
        iDecSinkFormatType(PVMF_FORMAT_UNKNOWN),
        iSourceSinkFormatType(PVMF_FORMAT_UNKNOWN),
        iSourceTrackInfo(NULL),
        iState(PVPDP_IDLE),
        iDatapathConfig(CONFIG_NONE),
        iErrorCondition(false),
        iErrorOccurredDuringErrorCondition(false)
{
    AddToScheduler();

    // Retrieve the logger object
    iLogger = PVLogger::GetLoggerObject("PVPlayerEngine");
}


PVPlayerDatapath::~PVPlayerDatapath()
{
    if (IsBusy())
    {
        Cancel();
    }
}


PVMFStatus PVPlayerDatapath::Prepare(OsclAny* aContext)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Prepare() In"));

    // Check that source and sink nodes are set
    if (!iSourceNode || !iSinkNode)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Prepare() Source and/or sink node not set"));
        return PVMFFailure;
    }

    // Check that source track info is set
    if (iSourceTrackInfo == NULL)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Prepare() Source track info not set"));
        return PVMFFailure;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Prepare() Track MIME type %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    // Determine the datapath configuration
    // from the nodes that have been set
    if (iDecNode)
    {
        iDatapathConfig = CONFIG_DEC;
    }
    else
    {
        iDatapathConfig = CONFIG_NONE;
    }

    iContext = aContext;

    // Connect to the nodes and get session IDs
    PVMFNodeSessionInfo sessioninfo(this, this, NULL, this, NULL);
    int32 leavecode = 0;

    iSourceNode->ThreadLogon();
    sessioninfo.iErrorContext = (OsclAny*)iSourceNode;
    sessioninfo.iInfoContext = (OsclAny*)iSourceNode;
    leavecode = 0;
    OSCL_TRY(leavecode, iSourceSessionId = iSourceNode->Connect(sessioninfo));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Prepare() Connect on iSourceNode did a leave!"));
                         return PVMFFailure);

    iSinkNode->ThreadLogon();
    sessioninfo.iErrorContext = (OsclAny*)iSinkNode;
    sessioninfo.iInfoContext = (OsclAny*)iSinkNode;
    leavecode = 0;
    OSCL_TRY(leavecode, iSinkSessionId = iSinkNode->Connect(sessioninfo));
    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Prepare() Connect on iSinkNode did a leave!"));
                         return PVMFFailure);

    if (iDatapathConfig == CONFIG_DEC)
    {
        iDecNode->ThreadLogon();
        sessioninfo.iErrorContext = (OsclAny*)iDecNode;
        sessioninfo.iInfoContext = (OsclAny*)iDecNode;
        leavecode = 0;
        OSCL_TRY(leavecode, iDecSessionId = iDecNode->Connect(sessioninfo));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Prepare() Connect on iDecNode did a leave!"));
                             return PVMFFailure);
    }

    iState = PREPARE_INIT;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Prepare() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerDatapath::Start(OsclAny* aContext)
{
    OSCL_ASSERT(iSourceTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Start() In %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    iContext = aContext;
    iState = START_START;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Start() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerDatapath::Pause(OsclAny* aContext, bool aSinkPaused)
{
    OSCL_ASSERT(iSourceTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Pause() In %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    iContext = aContext;
    iSinkPaused = aSinkPaused;
    if (iSinkPaused)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Pause() Sink is already paused"));
    }
    iState = PAUSE_PAUSE;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Pause() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerDatapath::Stop(OsclAny* aContext, bool aErrorCondition)
{
    OSCL_ASSERT(iSourceTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Stop() In %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    iContext = aContext;
    iErrorCondition = aErrorCondition;
    iErrorOccurredDuringErrorCondition = false;
    iState = STOP_STOP;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Stop() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerDatapath::Teardown(OsclAny* aContext, bool aErrorCondition)
{
    OSCL_ASSERT(iSourceTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Teardown() In %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    // Disconnect the ports
    if (!iSourceOutPort || !iSinkInPort ||
            !iSourceNode || !iSinkNode)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Teardown() Source/sink nodes and/or ports are not set!"));
        return PVMFFailure;
    }

    iSourceOutPort->Disconnect();

    if (iDatapathConfig == CONFIG_DEC)
    {
        if (!iDecInPort || !iDecOutPort || !iDecNode)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Teardown() Decoder node and/or ports are not set"));
            return PVMFFailure;
        }

        iDecOutPort->Disconnect();
    }

    iContext = aContext;
    iErrorCondition = aErrorCondition;
    iErrorOccurredDuringErrorCondition = false;
    iState = TEARDOWN_RELEASEPORT1;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Teardown() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerDatapath::Reset(OsclAny* aContext, bool aErrorCondition)
{
    OSCL_ASSERT(iSourceTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Reset() In %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    iContext = aContext;
    iErrorCondition = aErrorCondition;
    iErrorOccurredDuringErrorCondition = false;
    iState = RESET_RESET;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Reset() Out"));
    return PVMFSuccess;
}


PVMFStatus PVPlayerDatapath::CancelCommand(OsclAny* aContext)
{
    OSCL_ASSERT(iSourceTrackInfo != NULL);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::CancelCommand() In %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

    iContext = aContext;

    // Need to cancel node command
    iState = PVPDP_CANCEL;
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::CancelCommand() Out"));
    return PVMFSuccess;
}


void PVPlayerDatapath::DisconnectNodeSession(void)
{
    int32 leavecode = 0;

    // Emergency case only to disconnect from nodes
    if (iSourceNode)
    {
        leavecode = 0;
        OSCL_TRY(leavecode, iSourceNode->Disconnect(iSourceSessionId));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::DisconnectNodeSession() Disconnect on iSourceNode did a leave")));
    }

    if (iSinkNode)
    {
        leavecode = 0;
        OSCL_TRY(leavecode, iSinkNode->Disconnect(iSinkSessionId));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::DisconnectNodeSession() Disconnect on iSinkNode did a leave")));
    }

    if (iDatapathConfig == CONFIG_DEC && iDecNode)
    {
        leavecode = 0;
        OSCL_TRY(leavecode, iDecNode->Disconnect(iDecSessionId));
        OSCL_FIRST_CATCH_ANY(leavecode,
                             PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::DisconnectNodeSession() Disconnect on iDecNode did a leave")));
    }
}


void PVPlayerDatapath::Run()
{
    int32 leavecode = 0;
    PVMFCommandId cmdid = 0;

    switch (iState)
    {
        case PREPARE_INIT:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing PREPARE_INIT case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));
            iPendingCmds = 0;
            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Init() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->Init(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Init on iDecNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Init() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->Init(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Init on iSinkNode did a leave"));
                                 iState = PVPDP_ERROR; RunIfNotReady(); break);
            if (cmdid != -1)
            {
                ++iPendingCmds;
            }
            else
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }
            break;

        case PREPARE_REQPORT:
        {
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing PREPARE_REQPORT case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling RequestPort() on source node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSourceNode->RequestPort(iSourceSessionId, iSourceTrackInfo->getPortTag(),
                                        &(iSourceTrackInfo->getTrackMimeType()), (OsclAny*)iSourceNode));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() RequestPort on iSourceNode did a leave"));
                                 iState = PVPDP_ERROR; RunIfNotReady(); break);
            if (cmdid != -1)
            {
                ++iPendingCmds;
            }
            else
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling RequestPort() on dec node(input)"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->RequestPort(iDecSessionId, DEFAULT_INPUT_PORTTAG, &(iSourceTrackInfo->getTrackMimeType()),
                                            (OsclAny*) iSourceTrackInfo));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() RequestPort on iDecNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling RequestPort() on dec node(output)"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->RequestPort(iDecSessionId, DEFAULT_OUTPUT_PORTTAG, &iDecSinkFormatString,
                                            (OsclAny*) & iDecSinkFormatString));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() RequestPort on iDecNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }

                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling RequestPort() on sink node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iSinkNode->RequestPort(iSinkSessionId, DEFAULT_INPUT_PORTTAG, &iDecSinkFormatString,
                                            (OsclAny*)iSinkNode));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() RequestPort on iSinkNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling RequestPort() on sink node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iSinkNode->RequestPort(iSinkSessionId, DEFAULT_INPUT_PORTTAG, &(iSourceTrackInfo->getTrackMimeType()),
                                            (OsclAny*)iSinkNode));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() RequestPort on iSinkNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
            }
        }
        break;

        case PREPARE_CONNECT:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing PREPARE_CONNECT case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            if (iDatapathConfig == CONFIG_DEC)
            {
                PvmiCapabilityAndConfig *portconfigif = NULL;

                iSourceOutPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)portconfigif);
                if (portconfigif)
                {
                    pvmiSetPortFormatSync(portconfigif, PORT_CONFIG_INPUT_FORMATS_VALTYPE, iSourceDecFormatType);
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Port config IF for source node's outport not available"));
                    break;
                }

                portconfigif = NULL;
                iDecInPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)portconfigif);
                if (portconfigif)
                {
                    pvmiSetPortFormatSync(portconfigif, PORT_CONFIG_INPUT_FORMATS_VALTYPE, iSourceDecFormatType);
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Port config IF for dec node's inport not available"));
                    break;
                }

                if (iSourceOutPort->Connect(iDecInPort) != PVMFSuccess)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Connect on source and dec ports failed"));
                    break;
                }

                portconfigif = NULL;
                iDecOutPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)portconfigif);
                if (portconfigif)
                {
                    pvmiSetPortFormatSync(portconfigif, PORT_CONFIG_INPUT_FORMATS_VALTYPE, iDecSinkFormatType);
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Port config IF for dec node's outport not available"));
                    break;
                }

                portconfigif = NULL;
                iSinkInPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)portconfigif);
                if (portconfigif)
                {
                    pvmiSetPortFormatSync(portconfigif, PORT_CONFIG_INPUT_FORMATS_VALTYPE, iDecSinkFormatType);
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Port config IF for sink node's inport not available"));
                    break;
                }

                if (iDecOutPort->Connect(iSinkInPort) != PVMFSuccess)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Connect on dec and sink ports failed"));
                    break;
                }
            }
            else
            {
                PvmiCapabilityAndConfig *portconfigif;

                iSourceOutPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)portconfigif);
                if (portconfigif)
                {
                    pvmiSetPortFormatSync(portconfigif, PORT_CONFIG_INPUT_FORMATS_VALTYPE, iSourceSinkFormatType);
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Port config IF for source node's outport not available"));
                    break;
                }

                iSinkInPort->QueryInterface(PVMI_CAPABILITY_AND_CONFIG_PVUUID, (OsclAny*&)portconfigif);
                if (portconfigif)
                {
                    pvmiSetPortFormatSync(portconfigif, PORT_CONFIG_INPUT_FORMATS_VALTYPE, iSourceSinkFormatType);
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Port config IF for sink node's inport not available"));
                    break;
                }

                if (iSourceOutPort->Connect(iSinkInPort) != PVMFSuccess)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Connect on source and sink ports failed"));
                    break;
                }
            }

            iState = PREPARE_PREPARE;
            RunIfNotReady();
            break;

        case PREPARE_PREPARE:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing PREPARE_PREPARE case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Prepare() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->Prepare(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Prepare on iSinkNode did a leave"));
                                 iState = PVPDP_ERROR; RunIfNotReady(); break);

            if (cmdid != -1)
            {
                ++iPendingCmds;
                if (iDatapathConfig == CONFIG_DEC)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Prepare() on dec node"));
                    leavecode = 0;
                    OSCL_TRY(leavecode, cmdid = iDecNode->Prepare(iDecSessionId));
                    OSCL_FIRST_CATCH_ANY(leavecode,
                                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Prepare on iDecNode did a leave"));
                                         iState = PVPDP_ERROR; RunIfNotReady(); break);

                    if (cmdid != -1)
                    {
                        ++iPendingCmds;
                    }
                    else
                    {
                        iState = PVPDP_ERROR;
                        RunIfNotReady();
                        break;
                    }
                }
            }
            else
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }
            break;

        case PREPARED:
            break;

        case START_START:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing START_START case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Start() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->Start(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Start on iSinkNode did a leave"));
                                 iState = PVPDP_ERROR; RunIfNotReady(); break);
            if (cmdid != -1)
            {
                ++iPendingCmds;
            }
            else
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Start() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->Start(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Start on iDecNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
            }
            break;

        case STARTED:
            break;

        case PAUSE_PAUSE:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing PAUSE_PAUSE case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            if (iSinkPaused == false)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Pause() on sink node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iSinkNode->Pause(iSinkSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Pause on iSinkNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Sink already paused so skipping Pause() on sink"));
                iSinkPaused = false;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Pause() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->Pause(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Pause on iDecNode did a leave"));
                                     iState = PVPDP_ERROR; RunIfNotReady(); break);
                if (cmdid != -1)
                {
                    ++iPendingCmds;
                }
                else
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
            }

            if (iPendingCmds == 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() No pending node command in PAUSE_PAUSE so going to PAUSED"));
                iState = PAUSED;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Pause() command completed successfully"));
                iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
            }
            break;

        case PAUSED:
            break;

        case STOP_STOP:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing STOP_STOP case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Stop() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->Stop(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Stop on iSinkNode did a leave"));
                                 if (!iErrorCondition)
        {
            iState = PVPDP_ERROR;
            RunIfNotReady();
                break;
            }
                                );
            if (cmdid != -1 && leavecode == 0)
            {
                ++iPendingCmds;
            }
            else if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }
            else
            {
                iErrorOccurredDuringErrorCondition = true;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Stop() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->Stop(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Stop on iDecNode did a leave"));
                                     if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                    break;
                }
                                    );
                if (cmdid != -1 && leavecode == 0)
                {
                    ++iPendingCmds;
                }
                else if (!iErrorCondition)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
                else
                {
                    iErrorOccurredDuringErrorCondition = true;
                }
            }

            if (iPendingCmds == 0 && iErrorCondition)
            {
                iState = PVPDP_IDLE;
                if (iErrorCondition && iErrorOccurredDuringErrorCondition)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Stop() command completed with errors during error condition"));
                    iErrorCondition = false;
                    iObserver->HandlePlayerDatapathEvent(0, PVMFFailure, iContext);
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Stop() command completed successfully"));
                    iErrorCondition = false;
                    iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                }
            }
            break;

        case STOPPED:
            break;

            // Note: ReleasePort must be done in two stages with dest ports first
            // so the media data is freed in the dest port before the source port
        case TEARDOWN_RELEASEPORT1:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing TEARDOWN_RELEASEPORT1 case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling ReleasePort() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->ReleasePort(iSinkSessionId, *iSinkInPort));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() ReleasePort on iSinkNode did a leave"));
                                 if (!iErrorCondition)
        {
            iState = PVPDP_ERROR;
            RunIfNotReady();
                break;
            }
                                );
            if (cmdid != -1 && leavecode == 0)
            {
                ++iPendingCmds;
            }
            else if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }
            else
            {
                iErrorOccurredDuringErrorCondition = true;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling ReleasePort() on dec node(input)"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->ReleasePort(iDecSessionId, *iDecInPort));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() ReleasePort on iDecNode did a leave"));
                                     if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                    break;
                }
                                    );
                if (cmdid != -1 && leavecode == 0)
                {
                    ++iPendingCmds;
                }
                else if (!iErrorCondition)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
                else
                {
                    iErrorOccurredDuringErrorCondition = true;
                }
            }

            if (iPendingCmds == 0 && iErrorCondition)
            {
                iState = TEARDOWN_RELEASEPORT2;
                RunIfNotReady();
            }
            break;

        case TEARDOWN_RELEASEPORT2:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing TEARDOWN_RELEASEPORT2 case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling ReleasePort() on source node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSourceNode->ReleasePort(iSourceSessionId, *iSourceOutPort));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() ReleasePort on iSourceNode did a leave"));
                                 if (!iErrorCondition)
        {
            iState = PVPDP_ERROR;
            RunIfNotReady();
                break;
            }
                                );
            if (cmdid != -1 && leavecode == 0)
            {
                ++iPendingCmds;
            }
            else if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }
            else
            {
                iErrorOccurredDuringErrorCondition = true;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling ReleasePort() on dec node(output)"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->ReleasePort(iDecSessionId, *iDecOutPort));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() ReleasePort on iDecNode did a leave"));
                                     if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                    break;
                }
                                    );
                if (cmdid != -1 && leavecode == 0)
                {
                    ++iPendingCmds;
                }
                else if (!iErrorCondition)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
                else
                {
                    iErrorOccurredDuringErrorCondition = true;
                }
            }

            if (iPendingCmds == 0 && iErrorCondition)
            {
                iState = TEARDOWNED;
                if (iErrorCondition && iErrorOccurredDuringErrorCondition)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Teardown() command completed with errors during error condition"));
                    iErrorCondition = false;
                    iObserver->HandlePlayerDatapathEvent(0, PVMFFailure, iContext);
                }
                else
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Teardown() command completed successfully"));
                    iErrorCondition = false;
                    iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                }
            }
            break;

        case TEARDOWNED:
            break;

        case RESET_RESET:
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing RESET_RESET case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Reset() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->Reset(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Reset on iSinkNode did a leave"));
                                 if (!iErrorCondition)
        {
            iState = PVPDP_ERROR;
            RunIfNotReady();
                break;
            }
                                );
            if (cmdid != -1 && leavecode == 0)
            {
                ++iPendingCmds;
            }
            else if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                break;
            }
            else
            {
                iErrorOccurredDuringErrorCondition = true;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Reset() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->Reset(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Reset on iDecNode did a leave"));
                                     if (!iErrorCondition)
            {
                iState = PVPDP_ERROR;
                RunIfNotReady();
                    break;
                }
                                    );
                if (cmdid != -1 && leavecode == 0)
                {
                    ++iPendingCmds;
                }
                else if (!iErrorCondition)
                {
                    iState = PVPDP_ERROR;
                    RunIfNotReady();
                    break;
                }
                else
                {
                    iErrorOccurredDuringErrorCondition = true;
                }
            }

            if (iPendingCmds == 0 && iErrorCondition)
            {
                iState = RESETTED;
                RunIfNotReady();
            }
            break;

        case RESETTED:
        {
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing RESETTED case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            PVMFStatus retval;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling Disconnect() on nodes"));
            leavecode = 0;
            OSCL_TRY(leavecode, retval = iSourceNode->Disconnect(iSourceSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Disconnect on iSourceNode did a leave")));

            leavecode = 0;
            OSCL_TRY(leavecode, retval = iSinkNode->Disconnect(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Disconnect on iSinkNode did a leave")));

            if (iDatapathConfig == CONFIG_DEC)
            {
                leavecode = 0;
                OSCL_TRY(leavecode, retval = iDecNode->Disconnect(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Disconnect on iDecNode did a leave")));
            }

            iState = PVPDP_IDLE;
            if (iErrorCondition && iErrorOccurredDuringErrorCondition)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Reset() command completed with errors during error condition"));
                iErrorCondition = false;
                iObserver->HandlePlayerDatapathEvent(0, PVMFFailure, iContext);
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Reset() command completed successfully"));
                iErrorCondition = false;
                iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
            }
        }
        break;

        case PVPDP_CANCEL:
        {
            OSCL_ASSERT(iSourceTrackInfo != NULL);
            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Processing PVPDP_CANCEL case for %s", iSourceTrackInfo->getTrackMimeType().get_cstr()));

            iPendingCmds = 0;

            PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling CancelAllCommands() on sink node"));
            leavecode = 0;
            OSCL_TRY(leavecode, cmdid = iSinkNode->CancelAllCommands(iSinkSessionId));
            OSCL_FIRST_CATCH_ANY(leavecode,
                                 PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() CancelAllCommands on iSinkNode did a leave"));
                                );
            if (cmdid != -1 && leavecode == 0)
            {
                ++iPendingCmds;
            }

            if (iDatapathConfig == CONFIG_DEC)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Calling CancelAllCommands() on dec node"));
                leavecode = 0;
                OSCL_TRY(leavecode, cmdid = iDecNode->CancelAllCommands(iDecSessionId));
                OSCL_FIRST_CATCH_ANY(leavecode,
                                     PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() CancelAllCommands on iDecNode did a leave"));
                                    );
                if (cmdid != -1 && leavecode == 0)
                {
                    ++iPendingCmds;
                }
            }

            if (iPendingCmds == 0)
            {
                // If all CancelAllCommands() weren't accepted, assume everything is cancelled so report cancel complete
                iState = PVPDP_CANCELLED;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Cancel() command completed successfully"));
                iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
            }
        }
        break;

        case PVPDP_CANCELLED:
            break;

        case PVPDP_ERROR:
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::Run() Report command failed"));
            iState = PVPDP_IDLE;
            iObserver->HandlePlayerDatapathEvent(0, PVMFFailure, iContext);
            break;

        default:
            break;
    }
}


void PVPlayerDatapath::NodeCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() In"));

    switch (iState)
    {
        case PREPARE_INIT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node Init() requests completed successfully"));
                    iState = PREPARE_REQPORT;
                    RunIfNotReady();
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in PREPARE_INIT state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case PREPARE_REQPORT:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                if (aResponse.GetContext() == (OsclAny*)iSourceNode)
                {
                    iSourceOutPort = (PVMFPortInterface*)(aResponse.GetEventData());
                }
                else if (aResponse.GetContext() == (OsclAny*)iSinkNode)
                {
                    iSinkInPort = (PVMFPortInterface*)(aResponse.GetEventData());
                }
                else if (aResponse.GetContext() == (OsclAny*) iSourceTrackInfo)
                {
                    iDecInPort = (PVMFPortInterface*)(aResponse.GetEventData());
                }
                else if (aResponse.GetContext() == (OsclAny*) &iDecSinkFormatString)
                {
                    iDecOutPort = (PVMFPortInterface*)(aResponse.GetEventData());
                }

                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node RequestPort() requests completed successfully"));
                    iState = PREPARE_CONNECT;
                    RunIfNotReady();
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in PREPARE_REQPORT state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case PREPARE_PREPARE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node Prepare() requests completed successfully"));
                    iState = PREPARED;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Prepare() command completed successfully"));
                    iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in PREPARE_PREPARE state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case START_START:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node Start() requests completed successfully"));
                    iState = STARTED;
                    // Send event immediately- iContext might get changed before its next Run
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Start() command completed successfully"));
                    iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in START_START state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case PAUSE_PAUSE:
            if (aResponse.GetCmdStatus() == PVMFSuccess)
            {
                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node Pause() requests completed successfully"));
                    iState = PAUSED;
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Pause() command completed successfully"));
                    iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in PAUSE_PAUSE state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case STOP_STOP:
            if (aResponse.GetCmdStatus() == PVMFSuccess || iErrorCondition)
            {
                if (aResponse.GetCmdStatus() != PVMFSuccess && iErrorCondition)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node reported error in Stop() during error condition"));
                    iErrorOccurredDuringErrorCondition = true;
                }

                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node Stop() requests completed"));
                    iState = PVPDP_IDLE;
                    if (iErrorCondition && iErrorOccurredDuringErrorCondition)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Stop() command completed with errors during error condition"));
                        iErrorCondition = false;
                        iObserver->HandlePlayerDatapathEvent(0, PVMFFailure, iContext);
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Stop() command completed successfully"));
                        iErrorCondition = false;
                        iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                    }
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in STOP_STOP state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case RESET_RESET:
            if (aResponse.GetCmdStatus() == PVMFSuccess || iErrorCondition)
            {
                if (aResponse.GetCmdStatus() != PVMFSuccess && iErrorCondition)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node reported error in Reset() during error condition"));
                    iErrorOccurredDuringErrorCondition = true;
                }

                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node Reset() requests completed"));
                    iState = RESETTED;
                    RunIfNotReady();
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in RESET_RESET state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case TEARDOWN_RELEASEPORT1:
            if (aResponse.GetCmdStatus() == PVMFSuccess || iErrorCondition)
            {
                if (aResponse.GetCmdStatus() != PVMFSuccess && iErrorCondition)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node reported error in ReleasePort() during error condition(1st set)"));
                    iErrorOccurredDuringErrorCondition = true;
                }

                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node ReleasePort() requests completed(1st set)"));
                    iState = TEARDOWN_RELEASEPORT2;
                    RunIfNotReady();
                    break;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in TEARDOWN_RELEASEPORT1 state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case TEARDOWN_RELEASEPORT2:
            if (aResponse.GetCmdStatus() == PVMFSuccess || iErrorCondition)
            {
                if (aResponse.GetCmdStatus() != PVMFSuccess && iErrorCondition)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node reported error in ReleasePort() during error condition(2nd set)"));
                    iErrorOccurredDuringErrorCondition = true;
                }

                --iPendingCmds;
                if (iPendingCmds == 0)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node ReleasePort() requests completed(2nd set)"));
                    iState = TEARDOWNED;
                    if (iErrorCondition && iErrorOccurredDuringErrorCondition)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Teardown() command completed with errors during error condition"));
                        iErrorCondition = false;
                        iObserver->HandlePlayerDatapathEvent(0, PVMFFailure, iContext);
                    }
                    else
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::Run() Report Teardown() command completed successfully"));
                        iErrorCondition = false;
                        iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
                    }
                    break;
                }
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVPlayerDatapath::NodeCommandCompleted() Node command failed in TEARDOWN_RELEASEPORT2 state"));
                iState = PVPDP_IDLE;
                iObserver->HandlePlayerDatapathEvent(0, aResponse.GetCmdStatus(), iContext, (PVMFCmdResp*)&aResponse);
            }
            break;

        case PVPDP_CANCEL:
            // When cancelling, don't care about the command status
            --iPendingCmds;
            if (iPendingCmds == 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() All node CancelAllCommands() requests completed"));
                iState = PVPDP_CANCELLED;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() Report Cancel() command completed successfully"));
                iObserver->HandlePlayerDatapathEvent(0, PVMFSuccess, iContext);
            }
            break;

        default:
            break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, iLogger, PVLOGMSG_STACK_TRACE, (0, "PVPlayerDatapath::NodeCommandCompleted() Out"));
}


void PVPlayerDatapath::HandleNodeInformationalEvent(const PVMFAsyncEvent& /*aEvent*/)
{
    // Ignore node info events since the engine will receive it directly from the nodes
}


void PVPlayerDatapath::HandleNodeErrorEvent(const PVMFAsyncEvent& /*aEvent*/)
{
    // Ignore node error events since the engine will receive it directly from the nodes
}


void PVPlayerDatapath::GetFormatStringFromType(PVMFFormatType &aType, OSCL_HeapString<OsclMemAllocator>& aString)
{
    GetFormatString(aType, aString);
}





