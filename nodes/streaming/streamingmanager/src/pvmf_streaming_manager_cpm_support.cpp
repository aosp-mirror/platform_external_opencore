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
#ifndef PVMFPROTOCOLENGINENODE_EXTENSION_H_INCLUDED
#include "pvmf_protocol_engine_node_extension.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif
#ifndef PVMF_STREAMING_MANAGER_NODE_H_INCLUDED
#include "pvmf_streaming_manager_node.h"
#endif
#ifndef PVMF_STREAMING_MANAGER_INTERNAL_H_INCLUDED
#include "pvmf_streaming_manager_internal.h"
#endif
#ifndef PVMF_SOCKET_NODE_H_INCLUDED
#include "pvmf_socket_node.h"
#endif
#ifndef PVMF_JITTER_BUFFER_NODE_H_INCLUDED
#include "pvmf_jitter_buffer_node.h"
#endif
#ifndef PVMF_MEDIALAYER_NODE_H_INCLUDED
#include "pvmf_medialayer_node.h"
#endif
#ifndef PVMF_MEDIA_PRESENTATION_INFO_H_INCLUDED
#include "pvmf_media_presentation_info.h"
#endif
#ifndef PVMF_PROTOCOLENGINE_FACTORY_H_INCLUDED
#include "pvmf_protocol_engine_factory.h"
#endif
#ifndef PVMF_PROTOCOLENGINE_DEFS_H_INCLUDED
#include "pvmf_protocol_engine_defs.h"
#endif
#ifndef PVMFPROTOCOLENGINENODE_EXTENSION_H_INCLUDED
#include "pvmf_protocol_engine_node_extension.h"
#endif
#ifndef PVMF_STREAMING_DATA_SOURCE_H_INCLUDED
#include "pvmf_streaming_data_source.h"
#endif


void PVMFStreamingManagerNode::InitCPM()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::InitCPM() In"));

    iCPMInitCmdId = iCPM->Init();
}

void PVMFStreamingManagerNode::OpenCPMSession()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::OpenCPMSession() In"));

    iCPMOpenSessionCmdId = iCPM->OpenSession(iCPMSessionID);
}

void PVMFStreamingManagerNode::CPMRegisterContent()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CPMRegisterContent() In"));

    if (iSourceContextDataValid)
    {
        iCPMRegisterContentCmdId = iCPM->RegisterContent(iCPMSessionID,
                                   iSessionSourceInfo->_sessionURL,
                                   iSessionSourceInfo->_sessionType,
                                   (OsclAny*) & iSourceContextData);
    }
    else
    {
        iCPMRegisterContentCmdId = iCPM->RegisterContent(iCPMSessionID,
                                   iSessionSourceInfo->_sessionURL,
                                   iSessionSourceInfo->_sessionType,
                                   (OsclAny*) & iCPMSourceData);
    }
}

void PVMFStreamingManagerNode::GetCPMLicenseInterface()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetCPMLicenseInterface() In"));

    iCPMGetLicenseInterfaceCmdId =
        iCPM->QueryInterface(iCPMSessionID,
                             PVMFCPMPluginLicenseInterfaceUuid,
                             OSCL_STATIC_CAST(PVInterface*&, iCPMLicenseInterface));
}

void PVMFStreamingManagerNode::GetCPMCapConfigInterface()
{
    iCPMGetCapConfigCmdId =
        iCPM->QueryInterface(iCPMSessionID,
                             PVMI_CAPABILITY_AND_CONFIG_PVUUID,
                             OSCL_STATIC_CAST(PVInterface*&, iCPMCapConfigInterface));
}


bool PVMFStreamingManagerNode::GetCPMContentAccessFactory()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetCPMContentAccessFactory() In"));

    PVMFStatus status = iCPM->GetContentAccessFactory(iCPMSessionID,
                        iCPMContentAccessFactory);
    if (status != PVMFSuccess)
    {
        return false;
    }
    return true;
}

bool PVMFStreamingManagerNode::GetCPMMetaDataExtensionInterface()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetCPMMetaDataExtensionInterface() In"));

    bool retVal =
        iCPM->queryInterface(KPVMFMetadataExtensionUuid,
                             OSCL_STATIC_CAST(PVInterface*&, iCPMMetaDataExtensionInterface));
    return retVal;
}

void PVMFStreamingManagerNode::RequestUsage()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::RequestUsage() In"));

    PopulateDRMInfo();
    iCPMRequestUsageId = iCPM->ApproveUsage(iCPMSessionID,
                                            iRequestedUsage,
                                            iApprovedUsage,
                                            iAuthorizationDataKvp,
                                            iUsageID,
                                            iCPMContentAccessFactory);
}

void PVMFStreamingManagerNode::PopulateDRMInfo()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::PopulateDRMInfo() In"));

    OSCL_ASSERT(iSessionSourceInfo->_sessionType ==
                PVMF_DATA_SOURCE_MS_HTTP_STREAMING_URL);

    Asf_PopulateDRMInfo();
}

void PVMFStreamingManagerNode::SendUsageComplete()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::SendUsageComplete() In"));

    iCPMUsageCompleteCmdId = iCPM->UsageComplete(iCPMSessionID, iUsageID);
}

void PVMFStreamingManagerNode::CloseCPMSession()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CloseCPMSession() In"));

    iCPMCloseSessionCmdId = iCPM->CloseSession(iCPMSessionID);
}

void PVMFStreamingManagerNode::ResetCPM()
{
    iCPMResetCmdId = iCPM->Reset();
}

void PVMFStreamingManagerNode::GetCPMMetaDataKeys()
{
    if (iCPMMetaDataExtensionInterface != NULL)
    {
        iCPMMetadataKeys.clear();
        iCPMGetMetaDataKeysCmdId =
            iCPMMetaDataExtensionInterface->GetNodeMetadataKeys(iCPMSessionID,
                    iCPMMetadataKeys,
                    0,
                    PVMF_STREAMING_MANAGER_NODE_MAX_CPM_METADATA_KEYS);
    }
}

PVMFStatus
PVMFStreamingManagerNode::CheckCPMCommandCompleteStatus(PVMFCommandId aID,
        PVMFStatus aStatus)
{
    PVMFStatus status = aStatus;
    if (aID == iCPMGetLicenseInterfaceCmdId)
    {
        if (aStatus == PVMFErrNotSupported)
        {
            /* License Interface is Optional */
            status = PVMFSuccess;
        }
    }
    return status;
}

void PVMFStreamingManagerNode::CPMCommandCompleted(const PVMFCmdResp& aResponse)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CPMCommandCompleted() In"));

    PVMFCommandId id = aResponse.GetCmdId();
    PVMFStatus status =
        CheckCPMCommandCompleteStatus(id, aResponse.GetCmdStatus());

    if (id == iCPMCancelGetLicenseCmdId)
    {
        /*
         * if this command is CancelGetLicense, we will return success or fail here.
         */
        OSCL_ASSERT(!iCancelCommand.empty());
        CommandComplete(iCancelCommand,
                        iCancelCommand.front(),
                        status);
        return;
    }
    else
    {
        /*
         * if there was any pending cancel, we just ignore CPM process.
         */
        if (iCurrentCommand.empty())
        {
            return;
        }
        if (!iCancelCommand.empty())
        {
            if (iCancelCommand.front().iCmd != PVMF_STREAMING_MANAGER_NODE_CANCEL_GET_LICENSE)
                return;
        }
    }

    if (status != PVMFSuccess)
    {
        /*
         * If any command fails, the sequence fails.
         */
        if (iCurrentCommand.front().iCmd == PVMF_STREAMING_MANAGER_NODE_RESET)
        {
            iDRMResetPending = false;
            PVMF_SM_LOG_COMMAND_SEQ((0, "PVMFStreamingManagerNode:CPMCommandCompleted: - Reset Error on ID=%d", id));
            if (iErrorDuringProcess == SM_NO_ERROR)
            {
                if (iErrorResponseInf != NULL)
                {
                    PVMF_SM_LOGERROR((0, "PVMFStreamingManagerNode::CPMCommandCompleted - iErrorResponseInf is not NULL"));
                    OSCL_ASSERT(false);
                }
                if (aResponse.GetEventExtensionInterface())
                {
                    iErrorResponseInf = aResponse.GetEventExtensionInterface();
                    iErrorResponseInf->addRef();
                }
                iErrorDuringProcess = SM_NODE_COMMAND_COMPLETION;
            }
            else
            {
                PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode:CPMCommandCompleted: - CancelAllCommands already Queued"));
            }
            CompleteReset();
        }
        else
        {
            if (id == iCPMRequestUsageId)
            {
                /*
                 * Only when PVMFErrLicenseRequired is replied for license authentication,
                 * Set iCPMInitPending into true.
                 */
                if (aResponse.GetCmdStatus() == PVMFErrLicenseRequired)
                    iCPMInitPending = true;
            }
            CommandComplete(iCurrentCommand,
                            iCurrentCommand.front(),
                            aResponse.GetCmdStatus(),
                            NULL,
                            NULL,
                            NULL,
                            aResponse.GetEventExtensionInterface());
        }
    }
    else
    {
        /*
         * process the response, and issue the next command in
         * the sequence.
         */

        if (id == iCPMInitCmdId)
        {
            OpenCPMSession();
        }
        else if (id == iCPMOpenSessionCmdId)
        {
            CPMRegisterContent();
        }
        else if (id == iCPMRegisterContentCmdId)
        {
            GetCPMCapConfigInterface();
        }
        else if (id == iCPMGetCapConfigCmdId)
        {
            // got capConfig interface, set kvps for CPM plugins
            if (!SetCPMKvps()) return;
            GetCPMLicenseInterface();
        }
        else if (id == iCPMGetLicenseInterfaceCmdId)
        {
            iCPMContentType = iCPM->GetCPMContentType(iCPMSessionID);
            if (iCPMContentType == PVMF_CPM_FORMAT_ACCESS_BEFORE_AUTHORIZE)
            {
                GetCPMContentAccessFactory();
                GetCPMMetaDataExtensionInterface();
                RequestUsage();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::CPMCommandCompleted() Unknown CPM format type"));
                /* Protected but Unknown format ?? */
                OSCL_ASSERT(false);

            }
        }
        else if (id == iCPMRequestUsageId)
        {
            if (iCPMContentType == PVMF_CPM_FORMAT_ACCESS_BEFORE_AUTHORIZE)
            {
                /* End of Node Init sequence. */
                OSCL_ASSERT(!iCurrentCommand.empty());
                OSCL_ASSERT(iCurrentCommand.front().iCmd == PVMF_STREAMING_MANAGER_NODE_INIT);
                CompleteDRMInit();
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVMFStreamingManagerNode::CPMCommandCompleted() Unknown CPM format type"));
                /* Unknown format - should never get here */
                OSCL_ASSERT(false);
            }
        }
        else if (id == iCPMGetMetaDataKeysCmdId)
        {
            /* End of GetNodeMetaDataKeys */
            PVMFStatus status =
                CompleteGetMetadataKeys(iCurrentCommand.front());
            CommandComplete(iCurrentCommand,
                            iCurrentCommand.front(),
                            status,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
        }
        else if (id == iCPMUsageCompleteCmdId)
        {
            if (iDecryptionInterface != NULL)
            {
                iDecryptionInterface->Reset();
                /* Remove the decrpytion interface */
                PVUuid uuid = PVMFCPMPluginDecryptionInterfaceUuid;
                iCPMContentAccessFactory->DestroyPVMFCPMPluginAccessInterface(uuid, iDecryptionInterface);
                iDecryptionInterface = NULL;
            }
            CloseCPMSession();
        }
        else if (id == iCPMCloseSessionCmdId)
        {
            ResetCPM();
        }
        else if (id == iCPMResetCmdId)
        {
            /* End of Node Reset sequence */
            OSCL_ASSERT(!iCurrentCommand.empty());
            OSCL_ASSERT(iCurrentCommand.front().iCmd == PVMF_STREAMING_MANAGER_NODE_RESET);
            iDRMResetPending = false;
            CompleteReset();
        }
        else if (id == iCPMGetMetaDataValuesCmdId)
        {
            /* End of GetNodeMetaDataValues */
            OSCL_ASSERT(!iCurrentCommand.empty());
            OSCL_ASSERT(iCurrentCommand.front().iCmd == PVMF_STREAMING_MANAGER_NODE_GETNODEMETADATAVALUES);
            CompleteGetMetaDataValues();
        }
        else if (id == iCPMGetLicenseCmdId)
        {
            CompleteGetLicense();
        }
        else
        {
            /* Unknown cmd ?? - error */
            CommandComplete(iCurrentCommand,
                            iCurrentCommand.front(),
                            PVMFFailure);
        }
    }
}

void PVMFStreamingManagerNode::CompleteGetMetaDataValues()
{
    CommandComplete(iCurrentCommand,
                    iCurrentCommand.front(),
                    PVMFSuccess);
}

void PVMFStreamingManagerNode::CompleteDRMInit()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteDRMInit - In"));
    if (iApprovedUsage.value.uint32_value !=
            iRequestedUsage.value.uint32_value)
    {
        CommandComplete(iCurrentCommand,
                        iCurrentCommand.front(),
                        PVMFErrAccessDenied,
                        NULL, NULL, NULL);
        return;
    }
    /* If protected content, get the decryption interface, if the intent is play */
    if (iCPMSourceData.iIntent & BITMASK_PVMF_SOURCE_INTENT_PLAY)
    {
        PVMFSMNodeContainer* iMediaLayerNodeContainer =
            getNodeContainer(PVMF_STREAMING_MANAGER_MEDIA_LAYER_NODE);
        if (iMediaLayerNodeContainer == NULL) OSCL_LEAVE(OsclErrBadHandle);
        PVMFMediaLayerNodeExtensionInterface* mlExtIntf =
            (PVMFMediaLayerNodeExtensionInterface*)
            (iMediaLayerNodeContainer->iExtensions[0]);

        PVUuid uuid = PVMFCPMPluginDecryptionInterfaceUuid;
        PVInterface* intf =
            iCPMContentAccessFactory->CreatePVMFCPMPluginAccessInterface(uuid);
        PVMFCPMPluginAccessInterface* interimPtr =
            OSCL_STATIC_CAST(PVMFCPMPluginAccessInterface*, intf);
        iDecryptionInterface = OSCL_STATIC_CAST(PVMFCPMPluginAccessUnitDecryptionInterface*, interimPtr);
        if (iDecryptionInterface == NULL)
        {
            CommandComplete(iCurrentCommand,
                            iCurrentCommand.front(),
                            PVMFErrAccessDenied,
                            NULL, NULL, NULL);
            return;
        }
        iDecryptionInterface->Init();
        mlExtIntf->setDRMDecryptionInterface(maxPacketSize, iDecryptionInterface);
    }

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteDRMInit Success"));
    //License authentication was successfull. Init is completed at protected clip
    SetState(EPVMFNodeInitialized);
    CommandComplete(iCurrentCommand,
                    iCurrentCommand.front(),
                    PVMFSuccess);

    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteDRMInit - Out"));
    return;
}

bool PVMFStreamingManagerNode::SetCPMKvps()
{
    if (iCPMCapConfigInterface && !iCPMKvpStore.isEmpty())
    {
        PVMFKvpVector *aVector = iCPMKvpStore.getKVPStore();
        PvmiKvp *aErrorKVP = NULL;
        int32 err = 0;
        for (uint32 i = 0; i < aVector->size(); i++)
        {
            OSCL_TRY(err, iCPMCapConfigInterface->setParametersSync(NULL, &((*aVector)[i]), 1, aErrorKVP));
            if (err)
            {
                CommandComplete(iCurrentCommand,
                                iCurrentCommand.front(),
                                PVMFFailure);
                iCPMKvpStore.destroy();
                return false;
            }
        }
        iCPMKvpStore.destroy();
    }
    return true;
}
PVMFCommandId
PVMFStreamingManagerNode::GetLicense(PVMFSessionId aSessionId,
                                     OSCL_wString& aContentName,
                                     OsclAny* aData,
                                     uint32 aDataSize,
                                     int32 aTimeoutMsec,
                                     const OsclAny* aContextData)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetLicense - Wide called"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_GET_LICENSE_W,
            aContentName,
            aData,
            aDataSize,
            aTimeoutMsec,
            aContextData);
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFStreamingManagerNode::GetLicense(PVMFSessionId aSessionId,
                                     OSCL_String&  aContentName,
                                     OsclAny* aData,
                                     uint32 aDataSize,
                                     int32 aTimeoutMsec,
                                     const OsclAny* aContextData)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::GetLicense - called"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommand::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_GET_LICENSE,
            aContentName,
            aData,
            aDataSize,
            aTimeoutMsec,
            aContextData);
    return QueueCommandL(cmd);
}

PVMFCommandId
PVMFStreamingManagerNode::CancelGetLicense(PVMFSessionId aSessionId
        , PVMFCommandId aCmdId
        , OsclAny* aContextData)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CancelGetLicense - called"));
    PVMFStreamingManagerNodeCommand cmd;
    cmd.PVMFStreamingManagerNodeCommandBase::Construct(aSessionId,
            PVMF_STREAMING_MANAGER_NODE_CANCEL_GET_LICENSE,
            aCmdId,
            aContextData);
    return QueueCommandL(cmd);
}

PVMFStatus PVMFStreamingManagerNode::GetLicenseStatus(
    PVMFCPMLicenseStatus& aStatus)
{
    if (iCPMLicenseInterface)
        return iCPMLicenseInterface->GetLicenseStatus(aStatus);
    return PVMFFailure;
}

PVMFStatus PVMFStreamingManagerNode::DoGetLicense(PVMFStreamingManagerNodeCommand& aCmd,
        bool aWideCharVersion)
{
    if (iCPMLicenseInterface == NULL)
    {
        return PVMFErrNotSupported;
    }

    if (aWideCharVersion == true)
    {

        OSCL_wString* contentName = NULL;
        OsclAny* data = NULL;
        uint32 dataSize = 0;
        int32 timeoutMsec = 0;
        aCmd.PVMFStreamingManagerNodeCommand::Parse(contentName,
                data,
                dataSize,
                timeoutMsec);
        iCPMGetLicenseCmdId =
            iCPMLicenseInterface->GetLicense(iCPMSessionID,
                                             *contentName,
                                             data,
                                             dataSize,
                                             timeoutMsec);
    }
    else
    {
        OSCL_String* contentName = NULL;
        OsclAny* data = NULL;
        uint32 dataSize = 0;
        int32 timeoutMsec = 0;
        aCmd.PVMFStreamingManagerNodeCommand::Parse(contentName,
                data,
                dataSize,
                timeoutMsec);
        iCPMGetLicenseCmdId =
            iCPMLicenseInterface->GetLicense(iCPMSessionID,
                                             *contentName,
                                             data,
                                             dataSize,
                                             timeoutMsec);
    }
    return PVMFPending;
}

void PVMFStreamingManagerNode::CompleteGetLicense()
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::CompleteGetLicense - called"));
    CommandComplete(iCurrentCommand,
                    iCurrentCommand.front(),
                    PVMFSuccess,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
}

void PVMFStreamingManagerNode::DoCancelGetLicense(PVMFStreamingManagerNodeCommand& aCmd)
{
    PVMF_SM_LOGSTACKTRACE((0, "PVMFStreamingManagerNode::DoCancelGetLicense() Called"));
    PVMFStatus status = PVMFErrArgument;

    if (iCPMLicenseInterface == NULL)
    {
        status = PVMFErrNotSupported;
    }
    else
    {
        /* extract the command ID from the parameters.*/
        PVMFCommandId id;
        aCmd.PVMFStreamingManagerNodeCommandBase::Parse(id);

        /* first check "current" command if any */
        PVMFStreamingManagerNodeCommand* cmd = iCurrentCommand.FindById(id);
        if (cmd)
        {
            if (cmd->iCmd == PVMF_STREAMING_MANAGER_NODE_GET_LICENSE_W || cmd->iCmd == PVMF_STREAMING_MANAGER_NODE_GET_LICENSE)
            {
                iCPMCancelGetLicenseCmdId =
                    iCPMLicenseInterface->CancelGetLicense(iCPMSessionID, iCPMGetLicenseCmdId);

                /*
                 * the queued commands are all asynchronous commands to the
                 * CPM module. CancelGetLicense can cancel only for GetLicense cmd.
                 * We need to wait CPMCommandCompleted.
                 */
                MoveCmdToCancelQueue(aCmd);
                return;
            }
        }

        /*
         * next check input queue.
         * start at element 1 since this cancel command is element 0.
         */
        cmd = iInputCommands.FindById(id, 1);
        if (cmd)
        {
            if (cmd->iCmd == PVMF_STREAMING_MANAGER_NODE_GET_LICENSE_W || cmd->iCmd == PVMF_STREAMING_MANAGER_NODE_GET_LICENSE)
            {
                /* cancel the queued command */
                CommandComplete(iInputCommands, *cmd, PVMFErrCancelled, NULL, NULL);
                /* report cancel success */
                CommandComplete(iInputCommands, aCmd, PVMFSuccess);
                return;
            }
        }
    }
    /* if we get here the command isn't queued so the cancel fails */
    CommandComplete(iInputCommands, aCmd, status);
    return;
}

