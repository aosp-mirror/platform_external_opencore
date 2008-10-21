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
#include "pv_omxdefs.h"
#include "oscl_types.h"
#include "omx_avc_component.h"


extern OMX_U32 g_ComponentIndex; // this is determined outside the component

#if PROXY_INTERFACE

#include "omx_proxy_interface.h"
extern ProxyApplication_OMX* pProxyTerm[];
#endif

// This function is called by OMX_GetHandle and it creates an instance of the avc component AO
OMX_ERRORTYPE AvcOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData)
{
    OpenmaxAvcAO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitAvcOmxComponentFields content to actual constructor

    pOpenmaxAOType = (OpenmaxAvcAO*) OSCL_NEW(OpenmaxAvcAO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
    ///////////////////////////////////////////////////////////////////////////////////////
}

// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OMX_ERRORTYPE AvcOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle)
{
    // get pointer to component AO
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE OpenmaxAvcAO::ConstructComponent(OMX_PTR pAppData)
{
    AvcComponentPortType* pInPort, *pOutPort;
    OMX_U32 ii;

    iNumPorts = 2;
    iOmxComponent.nSize = sizeof(OMX_COMPONENTTYPE);
    iOmxComponent.pComponentPrivate = (OMX_PTR) this;  // pComponentPrivate points to THIS component AO class
    iOmxComponent.pApplicationPrivate = pAppData; // init the App data


#if PROXY_INTERFACE
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;

    iOmxComponent.SendCommand = WrapperSendCommand;
    iOmxComponent.GetParameter = WrapperGetParameter;
    iOmxComponent.SetParameter = WrapperSetParameter;
    iOmxComponent.GetConfig = WrapperGetConfig;
    iOmxComponent.SetConfig = WrapperSetConfig;
    iOmxComponent.GetExtensionIndex = WrapperGetExtensionIndex;
    iOmxComponent.GetState = WrapperGetState;
    iOmxComponent.UseBuffer = WrapperUseBuffer;
    iOmxComponent.AllocateBuffer = WrapperAllocateBuffer;
    iOmxComponent.FreeBuffer = WrapperFreeBuffer;
    iOmxComponent.EmptyThisBuffer = WrapperEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = WrapperFillThisBuffer;

#else
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_FALSE;

    iOmxComponent.SendCommand = OpenmaxAvcAO::BaseComponentSendCommand;
    iOmxComponent.GetParameter = OpenmaxAvcAO::BaseComponentGetParameter;
    iOmxComponent.SetParameter = OpenmaxAvcAO::BaseComponentSetParameter;
    iOmxComponent.GetConfig = OpenmaxAvcAO::BaseComponentGetConfig;
    iOmxComponent.SetConfig = OpenmaxAvcAO::BaseComponentSetConfig;
    iOmxComponent.GetExtensionIndex = OpenmaxAvcAO::BaseComponentGetExtensionIndex;
    iOmxComponent.GetState = OpenmaxAvcAO::BaseComponentGetState;
    iOmxComponent.UseBuffer = OpenmaxAvcAO::BaseComponentUseBuffer;
    iOmxComponent.AllocateBuffer = OpenmaxAvcAO::BaseComponentAllocateBuffer;
    iOmxComponent.FreeBuffer = OpenmaxAvcAO::BaseComponentFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OpenmaxAvcAO::BaseComponentEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OpenmaxAvcAO::BaseComponentFillThisBuffer;
#endif

    iOmxComponent.SetCallbacks = OpenmaxAvcAO::BaseComponentSetCallbacks;
    iOmxComponent.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    iOmxComponent.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    iOmxComponent.nVersion.s.nRevision = SPECREVISION;
    iOmxComponent.nVersion.s.nStep = SPECSTEP;

    // PV capability
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;

    if (ipAppPriv)
    {
        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    ipAppPriv = (AvcPrivateType*) oscl_malloc(sizeof(AvcPrivateType));
    if (NULL == ipAppPriv)
    {
        return OMX_ErrorInsufficientResources;
    }

    if (iNumPorts)
    {
        if (ipPorts)
        {
            oscl_free(ipPorts);
            ipPorts = NULL;
        }

        ipPorts = (AvcComponentPortType**) oscl_calloc(iNumPorts, sizeof(AvcComponentPortType*));
        if (!ipPorts)
        {
            return OMX_ErrorInsufficientResources;
        }

        for (ii = 0; ii < iNumPorts; ii++)
        {
            ipPorts[ii] = (AvcComponentPortType*) oscl_calloc(1, sizeof(AvcComponentPortType));
            if (!ipPorts[ii])
            {
                return OMX_ErrorInsufficientResources;
            }

            ipPorts[ii]->TransientState = OMX_StateMax;
            SetHeader(&ipPorts[ii]->PortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            ipPorts[ii]->PortParam.nPortIndex = ii;

            /** Allocate and initialize buffer Queue */
            ipPorts[ii]->pBufferQueue = (QueueType*) oscl_malloc(sizeof(QueueType));

            if (NULL == ipPorts[ii]->pBufferQueue)
            {
                return OMX_ErrorInsufficientResources;
            }

            QueueInit(ipPorts[ii]->pBufferQueue);

            ipPorts[ii]->LoadedToIdleFlag = OMX_FALSE;
            ipPorts[ii]->IdleToLoadedFlag = OMX_FALSE;

        }

        AvcComponentSetPortFlushFlag(iNumPorts, -1, OMX_FALSE);
        AvcComponentSetNumBufferFlush(iNumPorts, -1, OMX_FALSE);
    }

    /** Domain specific section for the ports. */
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.cMIMEType = "video/Avc";
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDir = OMX_DirInput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_INPUT_BUFFER_AVC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize = INPUT_BUFFER_SIZE_AVC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;


    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainVideo;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.cMIMEType = "raw";
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.pNativeRender = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth = 176;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight = 144;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nBitrate = 64000;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.xFramerate = (15 << 16);
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDir = OMX_DirOutput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_OUTPUT_BUFFER_AVC
            ;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = 176 * 144 * 3 / 2; //Don't use OUTPUT_BUFFER_SIZE_AVC, just use QCIF (as default)
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;

    //Default values for Avc video param port
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoAvc.eProfile = OMX_VIDEO_AVCProfileBaseline;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->VideoAvc.eLevel = OMX_VIDEO_AVCLevel1;

    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.nProfileIndex = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eProfile = OMX_VIDEO_AVCProfileBaseline;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->ProfileLevel.eLevel = OMX_VIDEO_AVCLevel1;

    iPortTypesParam.nPorts = 2;
    iPortTypesParam.nStartPortNumber = 0;

    pInPort = (AvcComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    pOutPort = (AvcComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    SetHeader(&pInPort->VideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pInPort->VideoParam.nPortIndex = 0;
    pInPort->VideoParam.nIndex = 0;
    pInPort->VideoParam.eCompressionFormat = OMX_VIDEO_CodingAVC;
    pInPort->VideoParam.eColorFormat = OMX_COLOR_FormatUnused;

    SetHeader(&pOutPort->VideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    pOutPort->VideoParam.nPortIndex = 1;
    pOutPort->VideoParam.nIndex = 0;
    pOutPort->VideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pOutPort->VideoParam.eColorFormat = OMX_COLOR_FormatYUV420Planar;

    iCodecReady = OMX_FALSE;
    ipCallbacks = NULL;
    iCallbackData = NULL;
    iState = OMX_StateLoaded;
    ipTempInputBuffer = NULL;
    iTempInputBufferLength = 0;
    iNumInputBuffer = 0;
    iPartialFrameAssembly = OMX_FALSE;
    iEndofStream = OMX_FALSE;
    iIsInputBufferEnded = OMX_TRUE;
    iNewInBufferRequired = OMX_TRUE;
    iNewOutBufRequired = OMX_TRUE;

    /* Initialize the asynchronous command Queue  */
    if (ipCoreDescriptor)
    {
        oscl_free(ipCoreDescriptor);
        ipCoreDescriptor = NULL;
    }

    ipCoreDescriptor = (CoreDescriptorType*) oscl_malloc(sizeof(CoreDescriptorType));
    if (NULL == ipCoreDescriptor)
    {
        return OMX_ErrorInsufficientResources;
    }

    ipCoreDescriptor->pMessageQueue = NULL;
    ipCoreDescriptor->pMessageQueue = (QueueType*) oscl_malloc(sizeof(QueueType));
    if (NULL == ipCoreDescriptor->pMessageQueue)
    {
        return OMX_ErrorInsufficientResources;
    }

    QueueInit(ipCoreDescriptor->pMessageQueue);

    /** Default parameters setting */
    iIsInit = OMX_FALSE;
    iGroupPriority = 0;
    iGroupID = 0;
    ipMark = NULL;

    SetHeader(&iPortTypesParam, sizeof(OMX_PORT_PARAM_TYPE));

    iOutBufferCount = 0;
    iStateTransitionFlag = OMX_FALSE;
    iFlushPortFlag = OMX_FALSE;
    iEndOfFrameFlag = OMX_FALSE;
    iFirstFragment = OMX_FALSE;
    iMarkPropagate = OMX_FALSE;

    //Will be used in case of partial frame assembly
    ipInputCurrBuffer = NULL;
    iInputCurrBufferSize = 0;
    ipAppPriv->AvcHandle = &iOmxComponent;

    if (ipAvcDec)
    {
        OSCL_DELETE(ipAvcDec);
        ipAvcDec = NULL;
    }

    ipAvcDec = OSCL_NEW(AvcDecoder_OMX, ());
    if (ipAvcDec == NULL)
    {
        return OMX_ErrorInsufficientResources;
    }

    oscl_memset(ipAvcDec, 0, sizeof(AvcDecoder_OMX));

#if PROXY_INTERFACE

    pProxyTerm[g_ComponentIndex]->ComponentSendCommand = BaseComponentSendCommand;
    pProxyTerm[g_ComponentIndex]->ComponentGetParameter = BaseComponentGetParameter;
    pProxyTerm[g_ComponentIndex]->ComponentSetParameter = BaseComponentSetParameter;
    pProxyTerm[g_ComponentIndex]->ComponentGetConfig = BaseComponentGetConfig;
    pProxyTerm[g_ComponentIndex]->ComponentSetConfig = BaseComponentSetConfig;
    pProxyTerm[g_ComponentIndex]->ComponentGetExtensionIndex = BaseComponentGetExtensionIndex;
    pProxyTerm[g_ComponentIndex]->ComponentGetState = BaseComponentGetState;
    pProxyTerm[g_ComponentIndex]->ComponentUseBuffer = BaseComponentUseBuffer;
    pProxyTerm[g_ComponentIndex]->ComponentAllocateBuffer = BaseComponentAllocateBuffer;
    pProxyTerm[g_ComponentIndex]->ComponentFreeBuffer = BaseComponentFreeBuffer;
    pProxyTerm[g_ComponentIndex]->ComponentEmptyThisBuffer = BaseComponentEmptyThisBuffer;
    pProxyTerm[g_ComponentIndex]->ComponentFillThisBuffer = BaseComponentFillThisBuffer;

#endif

    return OMX_ErrorNone;
}


/*********************
 *
 * Component verfication routines
 *
 **********************/

void OpenmaxAvcAO::SetHeader(OMX_PTR aHeader, OMX_U32 aSize)
{
    OMX_VERSIONTYPE* pVersion = (OMX_VERSIONTYPE*)((OMX_STRING) aHeader + sizeof(OMX_U32));
    *((OMX_U32*) aHeader) = aSize;

    pVersion->s.nVersionMajor = SPECVERSIONMAJOR;
    pVersion->s.nVersionMinor = SPECVERSIONMINOR;
    pVersion->s.nRevision = SPECREVISION;
    pVersion->s.nStep = SPECSTEP;
}


OMX_ERRORTYPE OpenmaxAvcAO::CheckHeader(OMX_PTR aHeader, OMX_U32 aSize)
{
    OMX_VERSIONTYPE* pVersion = (OMX_VERSIONTYPE*)((OMX_STRING) aHeader + sizeof(OMX_U32));

    if (NULL == aHeader)
    {
        return OMX_ErrorBadParameter;
    }

    if (*((OMX_U32*) aHeader) != aSize)
    {
        return OMX_ErrorBadParameter;
    }

    if (pVersion->s.nVersionMajor != SPECVERSIONMAJOR ||
            pVersion->s.nVersionMinor != SPECVERSIONMINOR ||
            pVersion->s.nRevision != SPECREVISION ||
            pVersion->s.nStep != SPECSTEP)
    {
        return OMX_ErrorVersionMismatch;
    }

    return OMX_ErrorNone;
}


/**
 * This function verify component state and structure header
 */
OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentParameterSanityCheck(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_PTR pStructure,
    OMX_IN  size_t size)
{

    OSCL_UNUSED_ARG(hComponent);
    if (iState != OMX_StateLoaded &&
            iState != OMX_StateWaitForResources)
    {
        return OMX_ErrorIncorrectStateOperation;
    }

    if (nPortIndex >= iNumPorts)
    {
        return OMX_ErrorBadPortIndex;
    }

    return CheckHeader(pStructure, size);
}

/**
 * Set/Reset Port Flush Flag
 */
void OpenmaxAvcAO::AvcComponentSetPortFlushFlag(OMX_S32 NumPorts, OMX_S32 index, OMX_BOOL value)
{
    OMX_S32 ii;

    if (-1 == index)
    {
        for (ii = 0; ii < NumPorts; ii++)
        {
            ipPorts[ii]->IsPortFlushed = value;
        }
    }
    else
    {
        ipPorts[index]->IsPortFlushed = value;
    }

}

/**
 * Set Number of Buffer Flushed with the value Specified
 */
void OpenmaxAvcAO::AvcComponentSetNumBufferFlush(OMX_S32 NumPorts, OMX_S32 index, OMX_S32 value)
{
    OMX_S32 ii;

    if (-1 == index)
    { // For all ComponentPort
        for (ii = 0; ii < NumPorts; ii++)
        {
            ipPorts[ii]->NumBufferFlushed = value;
        }
    }
    else
    {
        ipPorts[index]->NumBufferFlushed = value;
    }
}


/** This function assembles multiple input buffers into
	* one frame with the marker flag OMX_BUFFERFLAG_ENDOFFRAME set
	*/

OMX_BOOL OpenmaxAvcAO::AvcComponentAssemblePartialFrames(OMX_BUFFERHEADERTYPE* aInputBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentAssemblePartialFrames IN"));

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;

    AvcComponentPortType* pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ipAvcInputBuffer = aInputBuffer;
    OMX_U32 BytesToCopy = 0;

    if (!iPartialFrameAssembly)
    {
        if (iNumInputBuffer > 0)
        {
            if (ipAvcInputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
            {
                iInputCurrLength = ipAvcInputBuffer->nFilledLen;
                ipFrameDecodeBuffer = ipAvcInputBuffer->pBuffer + ipAvcInputBuffer->nOffset;
                //capture the timestamp to be send to the corresponding output buffer
                iFrameTimestamp = ipAvcInputBuffer->nTimeStamp;
            }
            else
            {
                iInputCurrLength = 0;
                iPartialFrameAssembly = OMX_TRUE;
                iFirstFragment = OMX_TRUE;
                iFrameTimestamp = ipAvcInputBuffer->nTimeStamp;
                ipFrameDecodeBuffer = ipInputCurrBuffer;
            }

        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentAssemblePartialFrames ERROR"));
            return OMX_FALSE;
        }

    }

    //Assembling of partial frame will be done based on OMX_BUFFERFLAG_ENDOFFRAME flag marked
    if (iPartialFrameAssembly)
    {
        while (iNumInputBuffer > 0)
        {
            if (OMX_FALSE == iFirstFragment)
            {
                /* If the timestamp of curr fragment doesn't match with previous,
                 * discard the previous fragments & start reconstructing from new
                 */
                if (iFrameTimestamp != ipAvcInputBuffer->nTimeStamp)
                {
                    iInputCurrLength = 0;
                    iPartialFrameAssembly = OMX_TRUE;
                    iFirstFragment = OMX_TRUE;
                    iFrameTimestamp = ipAvcInputBuffer->nTimeStamp;
                    ipFrameDecodeBuffer = ipInputCurrBuffer;
                }
            }

            if ((ipAvcInputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) != 0)
            {
                break;
            }

            // check if the buffer size can take the new piece, or it needs to expand
            BytesToCopy = ipAvcInputBuffer->nFilledLen;

            if (iInputCurrBufferSize < (iInputCurrLength + BytesToCopy))
            {


                // allocate new partial frame buffer
                OMX_U8 *temp_new_buffer = NULL;
                temp_new_buffer = (OMX_U8 *) oscl_malloc(sizeof(OMX_U8) * (iInputCurrLength + BytesToCopy));

                // in the event that new buffer cannot be allocated
                if (temp_new_buffer == NULL)
                {
                    // copy into what space is available, and let the decoder complain
                    BytesToCopy = iInputCurrLength - iInputCurrBufferSize;
                }
                else
                {

                    // copy contents of the old buffer into the new one
                    oscl_memcpy(temp_new_buffer, ipInputCurrBuffer, iInputCurrBufferSize);
                    // free the old buffer
                    if (ipInputCurrBuffer)
                        oscl_free(ipInputCurrBuffer);
                    // assign new one
                    ipInputCurrBuffer = temp_new_buffer;
                    iInputCurrBufferSize = (iInputCurrLength + BytesToCopy);
                    ipFrameDecodeBuffer = ipInputCurrBuffer + iInputCurrLength;
                }
            }


            iInputCurrLength += BytesToCopy;
            oscl_memcpy(ipFrameDecodeBuffer, (ipAvcInputBuffer->pBuffer + ipAvcInputBuffer->nOffset), BytesToCopy); // copy buffer data
            ipFrameDecodeBuffer += BytesToCopy; // move the ptr

            ipAvcInputBuffer->nFilledLen = 0;

            AvcComponentReturnInputBuffer(ipAvcInputBuffer, pInPort);

            iFirstFragment = OMX_FALSE;

            if (iNumInputBuffer > 0)
            {
                ipAvcInputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);
                if (ipAvcInputBuffer->nFlags & OMX_BUFFERFLAG_EOS)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentAssemblePartialFrames EndOfStream arrived"));
                    iEndofStream = OMX_TRUE;
                }
            }
        }

        // if we broke out of the while loop because of lack of buffers, then return and wait for more input buffers
        if (0 == iNumInputBuffer)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentAssemblePartialFrames OUT"));
            return OMX_FALSE;
        }
        else
        {
            // we have found the buffer that is the last piece of the frame.
            // Copy the buffer, but do not release it yet (this will be done after decoding for consistency)

            BytesToCopy = ipAvcInputBuffer->nFilledLen;
            // check if the buffer size can take the new piece, or it needs to expand
            if (iInputCurrBufferSize < (iInputCurrLength + BytesToCopy))
            {
                // allocate new partial frame buffer
                OMX_U8 *temp_new_buffer = NULL;
                temp_new_buffer = (OMX_U8 *) oscl_malloc(sizeof(OMX_U8) * (iInputCurrLength + BytesToCopy));

                // if you cannot allocate new buffer, just copy what data you can
                if (temp_new_buffer == NULL)
                {
                    BytesToCopy = iInputCurrBufferSize - iInputCurrLength;
                }
                else
                {

                    // copy contents of the old one into new one
                    oscl_memcpy(temp_new_buffer, ipInputCurrBuffer, iInputCurrBufferSize);
                    // free the old buffer
                    if (ipInputCurrBuffer)
                        oscl_free(ipInputCurrBuffer);
                    // assign new one
                    ipInputCurrBuffer = temp_new_buffer;
                    iInputCurrBufferSize = (iInputCurrLength + BytesToCopy);
                    ipFrameDecodeBuffer = ipInputCurrBuffer + iInputCurrLength;
                }
            }

            iInputCurrLength += BytesToCopy;
            oscl_memcpy(ipFrameDecodeBuffer, (ipAvcInputBuffer->pBuffer + ipAvcInputBuffer->nOffset), BytesToCopy); // copy buffer data
            ipFrameDecodeBuffer += BytesToCopy; // move the ptr

            ipFrameDecodeBuffer = ipInputCurrBuffer; // reset the pointer back to beginning of assembly buffer
            iPartialFrameAssembly = OMX_FALSE; // we have finished with assembling the frame, so this is not needed any more
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentAssemblePartialFrames OUT"));
    return OMX_TRUE;
}


/** This is the central function for buffers processing and decoding.
	* It is called through the Run() of active object when the component is in executing state
	* and is signalled each time a new buffer is available on the given ports
	* This function will process the input buffers & return output buffers
	*/

void OpenmaxAvcAO::AvcComponentBufferMgmtFunction()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentBufferMgmtFunction IN"));

    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    AvcComponentPortType* pInPort = (AvcComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];

    OMX_BOOL PartialFrameReturn;

    /* Don't dequeue any further buffer after endofstream buffer has been dequeued
     * till we send the callback and reset the flag back to false
     */
    if (OMX_FALSE == iEndofStream)
    {
        //More than one frame can't be dequeued in case of outbut blocked
        if ((OMX_TRUE == iNewInBufferRequired) && (GetQueueNumElem(pInputQueue) > 0))
        {
            ipAvcInputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);

            if (ipAvcInputBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentBufferMgmtFunction EndOfStream arrived"));
                iEndofStream = OMX_TRUE;
            }

            if (0 != ipAvcInputBuffer->nFilledLen)
            {
                if (0 == iFrameCount)
                {
                    //Set the marker flag (iEndOfFrameFlag) if first frame has the EnfOfFrame flag marked.
                    if (ipAvcInputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentBufferMgmtFunction EndOfFrame flag present"));
                        iEndOfFrameFlag = OMX_TRUE;
                    }
                }

                /* This condition will be true if OMX_BUFFERFLAG_ENDOFFRAME flag is
                 *  not marked in all the input buffers
                 */
                if (!iEndOfFrameFlag)
                {
                    AvcBufferMgmtWithoutMarker(ipAvcInputBuffer);

                }
                //If OMX_BUFFERFLAG_ENDOFFRAME flag is marked, come here
                else
                {
                    PartialFrameReturn = AvcComponentAssemblePartialFrames(ipAvcInputBuffer);
                    if (OMX_FALSE == PartialFrameReturn)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentBufferMgmtFunction OUT"));
                        return;
                    }
                    iIsInputBufferEnded = OMX_FALSE;

                    ipTargetComponent = (OMX_COMPONENTTYPE*) ipAvcInputBuffer->hMarkTargetComponent;

                    iTargetMarkData = ipAvcInputBuffer->pMarkData;
                    if (ipTargetComponent == (OMX_COMPONENTTYPE*) pHandle)
                    {
                        (*(ipCallbacks->EventHandler))
                        (pHandle,
                         iCallbackData,
                         OMX_EventMark,
                         1,
                         0,
                         ipAvcInputBuffer->pMarkData);
                    }
                }

            }	//end braces for if (ipAvcInputBuffer->nFilledLen != 0)
            else
            {
                AvcComponentReturnInputBuffer(ipAvcInputBuffer, pInPort);
            }

        }	//end braces for if ((OMX_TRUE == iNewInBufferRequired) && (GetQueueNumElem(pInputQueue) > 0))
    }	//if (OMX_FALSE == iEndofStream)

    if (!iEndOfFrameFlag)
    {
        AvcDecodeWithoutMarker();
    }
    else
    {
        AvcDecodeWithMarker(ipAvcInputBuffer);
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentBufferMgmtFunction OUT"));
    return;
}

/* Assemble the partial frames in case of buffers without end of frame flag marked */

void OpenmaxAvcAO::AvcBufferMgmtWithoutMarker(OMX_BUFFERHEADERTYPE* pAvcInputBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcBufferMgmtWithoutMarker IN"));

    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;
    AvcComponentPortType*	pInPort = (AvcComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;

    OMX_U32 TempInputBufferSize = (2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));


    /* Assembling of partial frame will be done based on max input buf size
     * If Flushport flag is true, that means its not a partial frame
     * but an unconsumed frame, process it independently
     * Same is true for endofstream condition, process the buffer independently
     */
    if ((pAvcInputBuffer->nFilledLen < pAvcInputBuffer->nAllocLen)
            && (OMX_TRUE != iEndofStream) && (OMX_FALSE == iFlushPortFlag))
    {
        if (!iPartialFrameAssembly)
        {
            iInputCurrLength = 0;
            ipFrameDecodeBuffer = ipInputCurrBuffer;
        }

        while (iNumInputBuffer > 0)
        {
            oscl_memcpy(ipFrameDecodeBuffer, (pAvcInputBuffer->pBuffer + pAvcInputBuffer->nOffset), pAvcInputBuffer->nFilledLen);
            ipFrameDecodeBuffer += pAvcInputBuffer->nFilledLen; // move the ptr

            iFrameTimestamp = pAvcInputBuffer->nTimeStamp;

            if (((iInputCurrLength += pAvcInputBuffer->nFilledLen) >= pAvcInputBuffer->nAllocLen)
                    || OMX_TRUE == iEndofStream)
            {
                break;
            }

            //Set the filled len to zero to indiacte buffer is fully consumed
            pAvcInputBuffer->nFilledLen = 0;
            AvcComponentReturnInputBuffer(pAvcInputBuffer, pInPort);

            if (iNumInputBuffer > 0)
            {
                pAvcInputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);
                if (pAvcInputBuffer->nFlags & OMX_BUFFERFLAG_EOS)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcBufferMgmtWithoutMarker EndOfStream arrived"));
                    iEndofStream = OMX_TRUE;
                }
            }
        }

        if (((iInputCurrLength < pAvcInputBuffer->nAllocLen)) && OMX_TRUE != iEndofStream)
        {
            iPartialFrameAssembly = OMX_TRUE;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcBufferMgmtWithoutMarker OUT"));
            return;
        }
        else
        {
            ipFrameDecodeBuffer = ipInputCurrBuffer;
            iPartialFrameAssembly = OMX_FALSE;
        }
    }
    else
    {
        if (iNumInputBuffer > 0)
        {
            iInputCurrLength = pAvcInputBuffer->nFilledLen;
            ipFrameDecodeBuffer = pAvcInputBuffer->pBuffer + pAvcInputBuffer->nOffset;
            iFrameTimestamp = pAvcInputBuffer->nTimeStamp;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcBufferMgmtWithoutMarker OUT"));
            return; // nothing to decode
        }
    }

    if (iTempInputBufferLength < (TempInputBufferSize >> 1))
    {
        oscl_memmove(ipTempInputBuffer, &ipTempInputBuffer[iTempConsumedLength], iTempInputBufferLength);
        iIsInputBufferEnded = OMX_TRUE;
        iTempConsumedLength = 0;
    }

    if ((iTempInputBufferLength + iTempConsumedLength + iInputCurrLength)
            <= TempInputBufferSize)
    {
        oscl_memcpy(&ipTempInputBuffer[iTempInputBufferLength + iTempConsumedLength], ipFrameDecodeBuffer, iInputCurrLength);
        iTempInputBufferLength += iInputCurrLength;

        if (iTempInputBufferLength + (TempInputBufferSize >> 1) <= TempInputBufferSize)
        {
            iNewInBufferRequired = OMX_TRUE;
        }
        else
        {
            iNewInBufferRequired = OMX_FALSE;
        }

        ipTargetComponent = (OMX_COMPONENTTYPE*) pAvcInputBuffer->hMarkTargetComponent;

        iTargetMarkData = pAvcInputBuffer->pMarkData;
        if (ipTargetComponent == (OMX_COMPONENTTYPE*) pHandle)
        {
            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventMark,
             1,
             0,
             pAvcInputBuffer->pMarkData);
        }
        pAvcInputBuffer->nFilledLen = 0;
        AvcComponentReturnInputBuffer(pAvcInputBuffer, pInPort);

    }

    if (iTempInputBufferLength >= (TempInputBufferSize >> 1))
    {
        iIsInputBufferEnded = OMX_FALSE;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcBufferMgmtWithoutMarker OUT"));
    return;

}


/* Decoding function for input buffers without end of frame flag marked */
void OpenmaxAvcAO::AvcDecodeWithoutMarker()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithoutMarker IN"));

    QueueType*				pInputQueue  = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType*				pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    AvcComponentPortType*	pOutPort =     ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_COMPONENTTYPE *		pHandle =      &iOmxComponent;

    OMX_U8*					pOutBuffer;
    OMX_U32					OutputLength;
    OMX_U8*					pTempInBuffer;
    OMX_U32					TempInLength;
    OMX_BOOL				DecodeReturn;
    OMX_BOOL				MarkerFlag = OMX_FALSE;
    OMX_TICKS				TempTimestamp;
    OMX_BOOL				ResizeNeeded = OMX_FALSE;

    OMX_U32 TempInputBufferSize = (2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));
    OMX_U32 CurrWidth =  ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth;
    OMX_U32 CurrHeight = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight;


    if ((!iIsInputBufferEnded) || ((iEndofStream) || (0 != iTempInputBufferLength)))
    {
        //Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not
            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithoutMarker OUT output buffer unavailable"));
                //Store the mark data for output buffer, as it will be overwritten next time
                if (NULL != ipTargetComponent)
                {
                    ipTempTargetComponent = ipTargetComponent;
                    iTempTargetMarkData = iTargetMarkData;
                    iMarkPropagate = OMX_TRUE;
                }
                return;
            }

            ipAvcOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
            //Do not proceed if the output buffer can't fit the YUV data
            if (ipAvcOutputBuffer->nAllocLen < (OMX_U32)(((CurrWidth + 15)&(~15)) * ((CurrHeight + 15)&(~15)) * 3 / 2))
            {
                ipAvcOutputBuffer->nFilledLen = 0;
                AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);
                return;
            }
            ipAvcOutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;
        }


        /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
         * command and hMarkTargetComponent as given by the specifications
         */
        if (NULL != ipMark)
        {
            ipAvcOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
            ipAvcOutputBuffer->pMarkData = ipMark->pMarkData;
            ipMark = NULL;
        }

        if ((OMX_TRUE == iMarkPropagate) && (ipTempTargetComponent != ipTargetComponent))
        {
            ipAvcOutputBuffer->hMarkTargetComponent = ipTempTargetComponent;
            ipAvcOutputBuffer->pMarkData = iTempTargetMarkData;
            ipTempTargetComponent = NULL;
            iMarkPropagate = OMX_FALSE;
        }
        else if (ipTargetComponent != NULL)
        {
            ipAvcOutputBuffer->hMarkTargetComponent = ipTargetComponent;
            ipAvcOutputBuffer->pMarkData = iTargetMarkData;
            ipTargetComponent = NULL;
            iMarkPropagate = OMX_FALSE;
        }
        //Mark buffer code ends here

        pOutBuffer = ipAvcOutputBuffer->pBuffer;
        OutputLength = 0;

        pTempInBuffer = ipTempInputBuffer + iTempConsumedLength;
        TempInLength = iTempInputBufferLength;

        //Output buffer is passed as a short pointer
        DecodeReturn = ipAvcDec->AvcDecodeVideo_OMX(pOutBuffer, (OMX_U32*) & OutputLength,
                       &(pTempInBuffer),
                       &TempInLength,
                       &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam),
                       &iFrameCount,
                       MarkerFlag, &TempTimestamp, &ResizeNeeded);

        ipAvcOutputBuffer->nFilledLen = OutputLength;

        //offset not required in our case, set it to zero
        ipAvcOutputBuffer->nOffset = 0;

        //Set the timestamp equal to the input buffer timestamp
        ipAvcOutputBuffer->nTimeStamp = iFrameTimestamp;

        iTempConsumedLength += (iTempInputBufferLength - TempInLength);
        iTempInputBufferLength = TempInLength;

        //If decoder returned error, report it to the client via a callback
        if (!DecodeReturn && OMX_FALSE == iEndofStream)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithoutMarker ErrorStreamCorrupt callback send"));

            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventError,
             OMX_ErrorStreamCorrupt,
             0,
             NULL);
        }


        //Do not decode if big buffer is less than half the size
        if (TempInLength < (TempInputBufferSize >> 1))
        {
            iIsInputBufferEnded = OMX_TRUE;
            iNewInBufferRequired = OMX_TRUE;
        }

        if (ResizeNeeded == OMX_TRUE)
        {
            OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->AvcHandle;

            iResizePending = OMX_TRUE;
            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventPortSettingsChanged, //The command was completed
             OMX_PORT_OUTPUTPORT_INDEX,
             0,
             NULL);
        }

        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         */
        if (OMX_TRUE == iEndofStream)
        {
            if ((0 == iTempInputBufferLength) && !DecodeReturn)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithoutMarker EOS callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventBufferFlag,
                 1,
                 OMX_BUFFERFLAG_EOS,
                 NULL);

                iNewInBufferRequired = OMX_TRUE;
                iEndofStream = OMX_FALSE;

                ipAvcOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;

                AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithoutMarker OUT"));

                return;
            }
        }

        //Send the output buffer back when it has some data in it
        if (ipAvcOutputBuffer->nFilledLen > 0)
        {
            AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);
        }

        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if ((ResizeNeeded == OMX_FALSE) && ((TempInLength != 0) || (GetQueueNumElem(pInputQueue) > 0))
                && ((GetQueueNumElem(pOutputQueue) > 0) || (OMX_FALSE == iNewOutBufRequired)))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithoutMarker OUT"));
    return;
}


/* Decoding function for input buffers with end of frame flag marked */
void OpenmaxAvcAO::AvcDecodeWithMarker(OMX_BUFFERHEADERTYPE* pAvcInputBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker IN"));

    OMX_COMPONENTTYPE  *pHandle = &iOmxComponent;
    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    AvcComponentPortType*	pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX];
    AvcComponentPortType*	pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    OMX_U8*					pOutBuffer;
    OMX_U32					OutputLength;
    static OMX_BOOL			DecodeReturn = OMX_FALSE;
    OMX_BOOL				MarkerFlag = OMX_TRUE;
    OMX_BOOL				Status;
    OMX_BOOL				ResizeNeeded = OMX_FALSE;

    OMX_U32 CurrWidth =  ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameWidth;
    OMX_U32 CurrHeight = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.video.nFrameHeight;

    if ((!iIsInputBufferEnded) || (iEndofStream))
    {
        //Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not
            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                iNewInBufferRequired = OMX_FALSE;
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker OUT output buffer unavailable"));
                return;
            }

            ipAvcOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);

            //Do not proceed if the output buffer can't fit the YUV data
            if (ipAvcOutputBuffer->nAllocLen < (OMX_U32)(((CurrWidth + 15)&(~15)) * ((CurrHeight + 15)&(~15)) * 3 / 2))
            {
                ipAvcOutputBuffer->nFilledLen = 0;
                AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);
                return;
            }
            ipAvcOutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;
        }

        /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
         * command and hMarkTargetComponent as given by the specifications
         */
        if (NULL != ipMark)
        {
            ipAvcOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
            ipAvcOutputBuffer->pMarkData = ipMark->pMarkData;
            ipMark = NULL;
        }

        if (NULL != ipTargetComponent)
        {
            ipAvcOutputBuffer->hMarkTargetComponent = ipTargetComponent;
            ipAvcOutputBuffer->pMarkData = iTargetMarkData;
            ipTargetComponent = NULL;

        }
        //Mark buffer code ends here

        pOutBuffer = ipAvcOutputBuffer->pBuffer;
        OutputLength = 0;

        if (iInputCurrLength > 0)
        {
            //Store the input timestamp into a temp variable
            ipAvcDec->CurrInputTimestamp = iFrameTimestamp;

            //Output buffer is passed as a short pointer
            DecodeReturn = ipAvcDec->AvcDecodeVideo_OMX(pOutBuffer, (OMX_U32*) & OutputLength,
                           &(ipFrameDecodeBuffer),
                           &(iInputCurrLength),
                           &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam),
                           &iFrameCount,
                           MarkerFlag,
                           &(ipAvcOutputBuffer->nTimeStamp), &ResizeNeeded);

            ipAvcOutputBuffer->nFilledLen = OutputLength;
            //offset not required in our case, set it to zero
            ipAvcOutputBuffer->nOffset = 0;

            if (ResizeNeeded == OMX_TRUE)
            {
                OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->AvcHandle;

                // set the flag. Do not process any more frames until
                // IL Client sends PortDisable event (thus starting the procedure for
                // dynamic port reconfiguration)
                iResizePending = OMX_TRUE;

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventPortSettingsChanged, //The command was completed
                 OMX_PORT_OUTPUTPORT_INDEX,
                 0,
                 NULL);
            }


            //If decoder returned error, report it to the client via a callback
            if (!DecodeReturn && OMX_FALSE == iEndofStream)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker ErrorStreamCorrupt callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventError,
                 OMX_ErrorStreamCorrupt,
                 0,
                 NULL);
            }


            if (0 == iInputCurrLength)
            {
                pAvcInputBuffer->nFilledLen = 0;
                AvcComponentReturnInputBuffer(pAvcInputBuffer, pInPort);
                iNewInBufferRequired = OMX_TRUE;
                iIsInputBufferEnded = OMX_TRUE;
            }
            else
            {
                iNewInBufferRequired = OMX_FALSE;
            }
        }


        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client*/
        if (OMX_TRUE == iEndofStream)
        {
            if (!DecodeReturn)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker EOS callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventBufferFlag,
                 1,
                 OMX_BUFFERFLAG_EOS,
                 NULL);

                iNewInBufferRequired = OMX_TRUE;
                iEndofStream = OMX_FALSE;

                ipAvcOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);

                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker OUT"));

                return;
            }
            else if (DecodeReturn)
            {
                Status = ipAvcDec->FlushOutput_OMX(pOutBuffer, &OutputLength, &(ipAvcOutputBuffer->nTimeStamp), pOutPort->PortParam.format.video.nFrameWidth, pOutPort->PortParam.format.video.nFrameHeight);
                ipAvcOutputBuffer->nFilledLen = OutputLength;

                ipAvcOutputBuffer->nOffset = 0;

                if (OMX_FALSE != Status)
                {
                    AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker OUT"));
                    return;
                }
                else
                {
                    DecodeReturn = OMX_FALSE;
                    RunIfNotReady();
                }
            }
        }

        //Send the output buffer back when it has some data in it
        if (ipAvcOutputBuffer->nFilledLen > 0)
        {
            AvcComponentReturnOutputBuffer(ipAvcOutputBuffer, pOutPort);
        }


        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if ((ResizeNeeded == OMX_FALSE) && ((iInputCurrLength != 0) || (GetQueueNumElem(pInputQueue) > 0))
                && ((GetQueueNumElem(pOutputQueue) > 0) || (OMX_FALSE == iNewOutBufRequired)))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcDecodeWithMarker OUT"));
    return;
}


void OpenmaxAvcAO::AvcComponentReturnInputBuffer(OMX_BUFFERHEADERTYPE* pAvcInputBuffer, AvcComponentPortType *pPort)
{
    OSCL_UNUSED_ARG(pPort);
    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    if (iNumInputBuffer)
    {
        iNumInputBuffer--;
    }

    //Callback for releasing the input buffer
    (*(ipCallbacks->EmptyBufferDone))
    (pHandle, iCallbackData, pAvcInputBuffer);

    pAvcInputBuffer = NULL;

}


/**
 * Returns Output Buffer back to the IL client
 */
void OpenmaxAvcAO::AvcComponentReturnOutputBuffer(OMX_BUFFERHEADERTYPE* pAvcOutputBuffer,
        AvcComponentPortType *pPort)
{
    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    //Callback for sending back the output buffer
    (*(ipCallbacks->FillBufferDone))
    (pHandle, iCallbackData, pAvcOutputBuffer);

    if (iOutBufferCount)
    {
        iOutBufferCount--;
    }

    pPort->NumBufferFlushed++;
    iNewOutBufRequired = OMX_TRUE;
}

/** The panic function that exits from the application.
 */
OMX_S32 OpenmaxAvcAO::AvcComponentPanic()
{
    OSCL_ASSERT(false);
    OsclError::Panic("PVERROR", OsclErrGeneral);
    return 0;
}


/** Flushes all the buffers under processing by the given port.
	* This function is called due to a state change of the component, typically
	* @param Component the component which owns the port to be flushed
	* @param PortIndex the ID of the port to be flushed
	*/

OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentFlushPort(OMX_S32 PortIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentFlushPort IN"));

    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    OMX_BUFFERHEADERTYPE* pOutputBuff;
    OMX_BUFFERHEADERTYPE* pInputBuff;

    if (OMX_PORT_INPUTPORT_INDEX == PortIndex || OMX_PORT_ALLPORT_INDEX == PortIndex)
    {
        while ((GetQueueNumElem(pInputQueue) > 0))
        {
            pInputBuff = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);
            (*(ipCallbacks->EmptyBufferDone))
            (pHandle, iCallbackData, pInputBuff);
            iNumInputBuffer--;
        }

    }

    if (OMX_PORT_OUTPUTPORT_INDEX == PortIndex || OMX_PORT_ALLPORT_INDEX == PortIndex)
    {
        if ((OMX_FALSE == iNewOutBufRequired) && (iOutBufferCount > 0))
        {
            if (ipAvcOutputBuffer)
            {
                (*(ipCallbacks->FillBufferDone))
                (pHandle, iCallbackData, ipAvcOutputBuffer);
                iOutBufferCount--;
                iNewOutBufRequired = OMX_TRUE;
            }
        }


        while ((GetQueueNumElem(pOutputQueue) > 0))
        {
            pOutputBuff = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
            pOutputBuff->nFilledLen = 0;
            (*(ipCallbacks->FillBufferDone))
            (pHandle, iCallbackData, pOutputBuff);
            iOutBufferCount--;
        }

    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentFlushPort OUT"));
    return OMX_ErrorNone;
}


/** This function is called by the omx core when the component
	* is disposed by the IL client with a call to FreeHandle().
	* \param Component, the component to be disposed
	*/

OMX_ERRORTYPE OpenmaxAvcAO::DestroyComponent()
{
    OMX_U32 ii;

    if (iIsInit != OMX_FALSE)
    {
        AvcComponentDeInit();
    }

    /*Deinitialize and free ports semaphores and Queue*/
    for (ii = 0; ii < iNumPorts; ii++)
    {
        if (ipPorts[ii]->pBufferQueue != NULL)
        {
            QueueDeinit(ipPorts[ii]->pBufferQueue);
            oscl_free(ipPorts[ii]->pBufferQueue);
            ipPorts[ii]->pBufferQueue = NULL;
        }
        /*Free port*/
        if (ipPorts[ii] != NULL)
        {
            oscl_free(ipPorts[ii]);
            ipPorts[ii] = NULL;
        }
    }

    if (ipPorts)
    {
        oscl_free(ipPorts);
        ipPorts = NULL;
    }

    iState = OMX_StateLoaded;

    if (ipInputCurrBuffer)
    {
        oscl_free(ipInputCurrBuffer);
        ipInputCurrBuffer = NULL;
        iInputCurrBufferSize = 0;
    }

    if (ipTempInputBuffer)
    {

        oscl_free(ipTempInputBuffer);
        ipTempInputBuffer = NULL;
    }

    if (ipAvcDec)
    {
        OSCL_DELETE(ipAvcDec);
        ipAvcDec = NULL;
    }

    if (ipCoreDescriptor != NULL)
    {
        if (ipCoreDescriptor->pMessageQueue != NULL)
        {
            /* De-initialize the asynchronous command queue */
            QueueDeinit(ipCoreDescriptor->pMessageQueue);
            oscl_free(ipCoreDescriptor->pMessageQueue);
            ipCoreDescriptor->pMessageQueue = NULL;
        }

        oscl_free(ipCoreDescriptor);
        ipCoreDescriptor = NULL;
    }

    if (ipAppPriv)
    {
        ipAppPriv->AvcHandle = NULL;

        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    return OMX_ErrorNone;
}


/**
 * Disable Single Port
 */
void OpenmaxAvcAO::AvcComponentDisableSinglePort(OMX_U32 PortIndex)
{
    ipPorts[PortIndex]->PortParam.bEnabled = OMX_FALSE;

    if (PORT_IS_POPULATED(ipPorts[PortIndex]) && OMX_TRUE == iIsInit)
    {
        if (OMX_FALSE == ipPorts[PortIndex]->IdleToLoadedFlag)
        {
            iStateTransitionFlag = OMX_TRUE;
            return;
        }
        else
        {
            ipPorts[PortIndex]->PortParam.bPopulated = OMX_FALSE;
        }
    }

    ipPorts[PortIndex]->NumBufferFlushed = 0;
}


/** Disables the specified port. This function is called due to a request by the IL client
	* @param Component the component which owns the port to be disabled
	* @param PortIndex the ID of the port to be disabled
	*/
OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentDisablePort(OMX_S32 PortIndex)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDisablePort IN"));
    OMX_U32 ii;

    if (-1 == PortIndex)
    {
        for (ii = 0; ii < iNumPorts; ii++)
        {
            ipPorts[ii]->IsPortFlushed = OMX_TRUE;
        }

        /*Flush all ports*/
        AvcComponentFlushPort(PortIndex);

        for (ii = 0; ii < iNumPorts; ii++)
        {
            ipPorts[ii]->IsPortFlushed = OMX_FALSE;
        }
    }
    else
    {
        /*Flush the port specified*/
        ipPorts[PortIndex]->IsPortFlushed = OMX_TRUE;
        AvcComponentFlushPort(PortIndex);
        ipPorts[PortIndex]->IsPortFlushed = OMX_FALSE;
    }

    /*Disable ports*/
    if (PortIndex != -1)
    {
        AvcComponentDisableSinglePort(PortIndex);
    }
    else
    {
        for (ii = 0; ii < iNumPorts; ii++)
        {
            AvcComponentDisableSinglePort(ii);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDisablePort OUT"));

    return OMX_ErrorNone;
}

/**
 * Enable Single Port
 */
void OpenmaxAvcAO::AvcComponentEnableSinglePort(OMX_U32 PortIndex)
{
    ipPorts[PortIndex]->PortParam.bEnabled = OMX_TRUE;

    if (!PORT_IS_POPULATED(ipPorts[PortIndex]) && OMX_TRUE == iIsInit)
    {
        if (OMX_FALSE == ipPorts[PortIndex]->LoadedToIdleFlag)
        {
            iStateTransitionFlag = OMX_TRUE;
            return;
        }
        else
        {
            ipPorts[PortIndex]->PortParam.bPopulated = OMX_TRUE;
        }
    }
}

/** Enables the specified port. This function is called due to a request by the IL client
	* @param Component the component which owns the port to be enabled
	* @param PortIndex the ID of the port to be enabled
	*/
OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentEnablePort(OMX_S32 PortIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentEnablePort IN"));

    OMX_U32 ii;

    /*Enable port/s*/
    if (PortIndex != -1)
    {
        AvcComponentEnableSinglePort(PortIndex);
    }
    else
    {
        for (ii = 0; ii < iNumPorts; ii++)
        {
            AvcComponentEnableSinglePort(ii);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentEnablePort OUT"));
    return OMX_ErrorNone;
}

//Not implemented & supported in case of base profile components

void OpenmaxAvcAO::AvcComponentGetRolesOfComponent(OMX_STRING* aRoleString)
{
    *aRoleString = "video_decoder.avc";
}



OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN  OMX_U32 nPort,
    OMX_IN  OMX_HANDLETYPE hTunneledComp,
    OMX_IN  OMX_U32 nTunneledPort,
    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OSCL_UNUSED_ARG(hComp);
    OSCL_UNUSED_ARG(nPort);
    OSCL_UNUSED_ARG(hTunneledComp);
    OSCL_UNUSED_ARG(nTunneledPort);
    OSCL_UNUSED_ARG(pTunnelSetup);
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentGetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    OSCL_UNUSED_ARG(hComponent);
    OSCL_UNUSED_ARG(nIndex);
    OSCL_UNUSED_ARG(pComponentConfigStructure);
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentSetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    OSCL_UNUSED_ARG(hComponent);
    OSCL_UNUSED_ARG(nIndex);
    OSCL_UNUSED_ARG(pComponentConfigStructure);
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OSCL_UNUSED_ARG(hComponent);
    OSCL_UNUSED_ARG(cParameterName);
    OSCL_UNUSED_ARG(pIndexType);
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentGetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    pOpenmaxAOType->GetState(pState);

    return OMX_ErrorNone;
}

void OpenmaxAvcAO::GetState(OMX_OUT OMX_STATETYPE* pState)
{
    *pState = iState;
}



//Active object constructor
OpenmaxAvcAO::OpenmaxAvcAO() :
        OsclActiveObject(OsclActiveObject::EPriorityNominal, "OMXAvcDec")
{

    iLogger = PVLogger::GetLoggerObject("PVMFOMXVideoDecNode");
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : constructed"));

    //Flag to call BufferMgmtFunction in the Run() when the component state is executing
    iBufferExecuteFlag = OMX_FALSE;
    ipAppPriv = NULL;
    //iLogger = NULL;

    ipCallbacks = NULL;
    iCallbackData = NULL;
    iState = OMX_StateLoaded;


    ipCoreDescriptor = NULL;
    iNumInputBuffer = 0;

    ipFrameDecodeBuffer = NULL;
    iPartialFrameAssembly = OMX_FALSE;
    iIsInputBufferEnded = OMX_TRUE;
    iEndofStream = OMX_FALSE;
    ipTempInputBuffer = NULL;


    ipTargetComponent = NULL;
    iTargetMarkData = NULL;
    iNewInBufferRequired = OMX_TRUE;
    iNewOutBufRequired = OMX_TRUE;

    iTempConsumedLength = 0;
    iOutBufferCount = 0;
    iCodecReady = OMX_FALSE;
    ipInputCurrBuffer = NULL;
    iInputCurrBufferSize = 0;
    iInputCurrLength = 0;
    iFrameCount = 0;
    iStateTransitionFlag = OMX_FALSE;
    iFlushPortFlag = OMX_FALSE;
    iEndOfFrameFlag = OMX_FALSE;
    ipAvcInputBuffer = NULL;
    ipAvcOutputBuffer = NULL;
    iFirstFragment = OMX_FALSE;
    iResizePending = OMX_FALSE;

    iFrameTimestamp = 0;

    iNumPorts = 0;
    ipPorts = NULL;

    //Indicate whether component has been already initialized */
    iIsInit = OMX_FALSE;

    iGroupPriority = 0;
    iGroupID = 0;

    ipMark = NULL;

    ipAvcDec = NULL;


    if (!IsAdded())
    {
        AddToScheduler();
    }
}


//Active object destructor
OpenmaxAvcAO::~OpenmaxAvcAO()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : destructed"));
}


/** The Initialization function
 */
OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    if (OMX_TRUE == iIsInit)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentInit error incorrect operation"));
        return OMX_ErrorIncorrectStateOperation;
    }
    iIsInit = OMX_TRUE;

    //avc lib init
    if (!iCodecReady)
    {
        Status = ipAvcDec->AvcDecInit_OMX();
        iCodecReady = OMX_TRUE;
    }

    iInputCurrLength = 0;
    //Used in dynamic port reconfiguration
    iFrameCount = 0;
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentInit OUT"));

    return Status;

}

/** This function is called upon a transition to the idle or invalid state.
 *  Also it is called by the AvcComponentDestructor() function
 */
OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentDeInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDeInit IN"));

    OMX_ERRORTYPE Status = OMX_ErrorNone;

    iIsInit = OMX_FALSE;

    if (iCodecReady)
    {
        Status = ipAvcDec->AvcDecDeinit_OMX();
        iCodecReady = OMX_FALSE;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDeInit OUT"));

    return Status;

}

OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentGetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)
{

    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status = OMX_ErrorNone;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    return Status;

}

OMX_ERRORTYPE OpenmaxAvcAO::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)

{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter IN"));

    OMX_PRIORITYMGMTTYPE* pPrioMgmt;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pBufSupply;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_PORT_PARAM_TYPE* pPortDomains;
    OMX_U32 PortIndex;
    AvcComponentPortType* pComponentPort;
    OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
    OMX_VIDEO_PARAM_AVCTYPE *pVideoAvc;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE * pProfileLevel;


    if (NULL == ComponentParameterStructure)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error bad parameter"));
        return OMX_ErrorBadParameter;
    }

    switch (nParamIndex)
    {
        case OMX_IndexParamPriorityMgmt:
        {
            pPrioMgmt = (OMX_PRIORITYMGMTTYPE*) ComponentParameterStructure;
            SetHeader(pPrioMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
            pPrioMgmt->nGroupPriority = iGroupPriority;
            pPrioMgmt->nGroupID = iGroupID;
        }
        break;

        case OMX_IndexParamVideoInit:
        {
            SetHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
            oscl_memcpy(ComponentParameterStructure, &iPortTypesParam, sizeof(OMX_PORT_PARAM_TYPE));
        }
        break;


        //Following 3 cases have a single common piece of code to be executed
        case OMX_IndexParamAudioInit:
        case OMX_IndexParamImageInit:
        case OMX_IndexParamOtherInit:
        {
            pPortDomains = (OMX_PORT_PARAM_TYPE*) ComponentParameterStructure;
            SetHeader(pPortDomains, sizeof(OMX_PORT_PARAM_TYPE));
            pPortDomains->nPorts = 0;
            pPortDomains->nStartPortNumber = 0;
        }
        break;

        case OMX_IndexParamVideoPortFormat:
        {
            pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*) ComponentParameterStructure;
            //Added to pass parameter test
            if (pVideoPortFormat->nIndex > ipPorts[pVideoPortFormat->nPortIndex]->VideoParam.nIndex)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error index out of range"));
                return OMX_ErrorNoMore;
            }
            SetHeader(pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            if (pVideoPortFormat->nPortIndex <= 1)
            {
                pComponentPort = (AvcComponentPortType*) ipPorts[pVideoPortFormat->nPortIndex];
                oscl_memcpy(pVideoPortFormat, &pComponentPort->VideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
        }

        break;

        case OMX_IndexParamVideoAvc:
        {
            pVideoAvc = (OMX_VIDEO_PARAM_AVCTYPE*) ComponentParameterStructure;
            if (pVideoAvc->nPortIndex != 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            PortIndex = pVideoAvc->nPortIndex;
            oscl_memcpy(pVideoAvc, &ipPorts[PortIndex]->VideoAvc, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
            SetHeader(pVideoAvc, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        }
        break;

        case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
            pProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*) ComponentParameterStructure;
            //Added to pass parameter test
            PortIndex = pProfileLevel->nPortIndex;
            if (pProfileLevel->nProfileIndex > ipPorts[PortIndex]->ProfileLevel.nProfileIndex)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error profile not supported"));
                return OMX_ErrorNoMore;
            }

            oscl_memcpy(pProfileLevel, &ipPorts[PortIndex]->ProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
            SetHeader(pProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        }
        break;

        case OMX_IndexParamVideoProfileLevelCurrent:
        {
            pProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*) ComponentParameterStructure;
            //Added to pass parameter test
            PortIndex = pProfileLevel->nPortIndex;

            oscl_memcpy(pProfileLevel, &ipPorts[PortIndex]->ProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
            SetHeader(pProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        }
        break;

        case OMX_IndexParamPortDefinition:
        {
            pPortDef  = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
            PortIndex = pPortDef->nPortIndex;
            if (PortIndex >= iNumPorts)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            oscl_memcpy(pPortDef, &ipPorts[PortIndex]->PortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        }
        break;

        case OMX_IndexParamCompBufferSupplier:
        {
            pBufSupply = (OMX_PARAM_BUFFERSUPPLIERTYPE*) ComponentParameterStructure;
            PortIndex = pBufSupply->nPortIndex;
            if (PortIndex >= iNumPorts)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            SetHeader(pBufSupply, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));

            if (OMX_DirInput == ipPorts[PortIndex]->PortParam.eDir)
            {
                pBufSupply->eBufferSupplier = OMX_BufferSupplyUnspecified;
            }
            else
            {
                SetHeader(pBufSupply, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
                pBufSupply->eBufferSupplier = OMX_BufferSupplyUnspecified;
            }
        }
        break;

        case(OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
        {
            PV_OMXComponentCapabilityFlagsType *pCap_flags = (PV_OMXComponentCapabilityFlagsType *) ComponentParameterStructure;
            if (NULL == pCap_flags)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error pCap_flags NULL"));
                return OMX_ErrorBadParameter;
            }
            oscl_memcpy(pCap_flags, &iPVCapabilityFlags, sizeof(iPVCapabilityFlags));

        }
        break;

        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter error Unsupported Index"));
            return OMX_ErrorUnsupportedIndex;
        }
        // break;	This break statement was removed to avoid compiler warning for Unreachable Code
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : GetParameter OUT"));

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentSetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)
{

    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->SetParameter(hComponent, nParamIndex, ComponentParameterStructure);

    return Status;
}


OMX_ERRORTYPE OpenmaxAvcAO::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter IN"));

    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;
    OMX_PRIORITYMGMTTYPE* pPrioMgmt;
    OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
    OMX_VIDEO_PARAM_AVCTYPE *pVideoAvc;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE * pProfileLevel;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pBufSupply;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef ;
    OMX_U32 PortIndex;
    OMX_PARAM_COMPONENTROLETYPE* pCompRole;

    AvcComponentPortType* pComponentPort;

    if (NULL == ComponentParameterStructure)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error bad parameter"));
        return OMX_ErrorBadParameter;
    }

    switch (nParamIndex)
    {
        case OMX_IndexParamVideoInit:
        {
            /*Check Structure Header*/
            CheckHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error video init failed"));
                return ErrorType;
            }
            oscl_memcpy(&iPortTypesParam, ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
        }
        break;

        case OMX_IndexParamVideoPortFormat:
        {
            pVideoPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*) ComponentParameterStructure;
            PortIndex = pVideoPortFormat->nPortIndex;
            /*Check Structure Header and verify component state*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }
            if (PortIndex <= 1)
            {
                pComponentPort = (AvcComponentPortType*) ipPorts[PortIndex];
                oscl_memcpy(&pComponentPort->VideoParam, pVideoPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamVideoAvc:
        {
            pVideoAvc = (OMX_VIDEO_PARAM_AVCTYPE*) ComponentParameterStructure;
            PortIndex = pVideoAvc->nPortIndex;
            /*Check Structure Header and verify component state*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pVideoAvc, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }
            oscl_memcpy(&ipPorts[PortIndex]->VideoAvc, pVideoAvc, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        }
        break;

        case OMX_IndexParamVideoProfileLevelCurrent:
        {
            pProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*) ComponentParameterStructure;
            PortIndex = pProfileLevel->nPortIndex;
            /*Check Structure Header and verify component state*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }
            oscl_memcpy(&ipPorts[PortIndex]->ProfileLevel, pProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        }
        break;

        case OMX_IndexParamPriorityMgmt:
        {
            if (iState != OMX_StateLoaded && iState != OMX_StateWaitForResources)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error incorrect state error"));
                return OMX_ErrorIncorrectStateOperation;
            }
            pPrioMgmt = (OMX_PRIORITYMGMTTYPE*) ComponentParameterStructure;
            if ((ErrorType = CheckHeader(pPrioMgmt, sizeof(OMX_PRIORITYMGMTTYPE))) != OMX_ErrorNone)
            {
                break;
            }
            iGroupPriority = pPrioMgmt->nGroupPriority;
            iGroupID = pPrioMgmt->nGroupID;
        }
        break;

        case OMX_IndexParamPortDefinition:
        {
            pPortDef  = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
            PortIndex = pPortDef->nPortIndex;

            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }

            ipPorts[PortIndex]->PortParam.nBufferCountActual = pPortDef->nBufferCountActual;
            ipPorts[PortIndex]->PortParam.nBufferSize = pPortDef->nBufferSize;
        }
        break;

        case OMX_IndexParamCompBufferSupplier:
        {
            pBufSupply = (OMX_PARAM_BUFFERSUPPLIERTYPE*) ComponentParameterStructure;
            PortIndex = pBufSupply->nPortIndex;

            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pBufSupply, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
            if (OMX_ErrorIncorrectStateOperation == ErrorType)
            {
                if (PORT_IS_ENABLED(ipPorts[pBufSupply->nPortIndex]))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error incorrect state error"));
                    return OMX_ErrorIncorrectStateOperation;
                }
            }
            else if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }

            if (pBufSupply->eBufferSupplier == OMX_BufferSupplyUnspecified)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter OUT"));
                return OMX_ErrorNone;
            }

            ErrorType = OMX_ErrorNone;
        }
        break;

        case OMX_IndexParamStandardComponentRole:
        {
            pCompRole = (OMX_PARAM_COMPONENTROLETYPE*) ComponentParameterStructure;
            if ((ErrorType = CheckHeader(pCompRole, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone)
            {
                break;
            }
            strcpy((OMX_STRING)iComponentRole, (OMX_STRING)pCompRole->cRole);
        }
        break;


        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter error bad parameter"));
            return OMX_ErrorBadParameter;
        }
        // break;	This break statement was removed to avoid compiler warning for Unreachable Code
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetParameter OUT"));
    return ErrorType;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->UseBuffer(hComponent, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : UseBuffer IN"));
    AvcComponentPortType* pBaseComponentPort;
    OMX_U32 ii;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : UseBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : UseBuffer error incorrect state"));
        return OMX_ErrorIncorrectStateTransition;
    }

    if (NULL == pBaseComponentPort->pBuffer)
    {
        pBaseComponentPort->pBuffer = (OMX_BUFFERHEADERTYPE**) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
        pBaseComponentPort->BufferState = (OMX_U32*) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_U32));
    }

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if (!(pBaseComponentPort->BufferState[ii] & BUFFER_ALLOCATED) &&
                !(pBaseComponentPort->BufferState[ii] & BUFFER_ASSIGNED))
        {
            pBaseComponentPort->pBuffer[ii] = (OMX_BUFFERHEADERTYPE*) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (NULL == pBaseComponentPort->pBuffer[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : UseBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            SetHeader(pBaseComponentPort->pBuffer[ii], sizeof(OMX_BUFFERHEADERTYPE));
            pBaseComponentPort->pBuffer[ii]->pBuffer = pBuffer;
            pBaseComponentPort->pBuffer[ii]->nAllocLen = nSizeBytes;
            pBaseComponentPort->pBuffer[ii]->nFilledLen = 0;
            pBaseComponentPort->pBuffer[ii]->nOffset = 0;
            pBaseComponentPort->pBuffer[ii]->nFlags = 0;
            pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pBaseComponentPort;
            pBaseComponentPort->pBuffer[ii]->pAppPrivate = pAppPrivate;
            pBaseComponentPort->pBuffer[ii]->nTickCount = 0;
            pBaseComponentPort->pBuffer[ii]->nTimeStamp = 0;
            *ppBufferHdr = pBaseComponentPort->pBuffer[ii];
            if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
            {
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = nPortIndex;
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = iNumPorts; // here is assigned a non-valid port index
            }
            else
            {
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = nPortIndex;
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = iNumPorts; // here is assigned a non-valid port index
            }
            pBaseComponentPort->BufferState[ii] |= BUFFER_ASSIGNED;
            pBaseComponentPort->BufferState[ii] |= HEADER_ALLOCATED;
            pBaseComponentPort->NumAssignedBuffers++;
            if (pBaseComponentPort->PortParam.nBufferCountActual == pBaseComponentPort->NumAssignedBuffers)
            {
                pBaseComponentPort->PortParam.bPopulated = OMX_TRUE;
                if (OMX_TRUE == iStateTransitionFlag)
                {
                    //Reschedule the AO for a state change (Loaded->Idle) if its pending on buffer allocation
                    RunIfNotReady();
                    //Set the corresponding flags
                    pBaseComponentPort->LoadedToIdleFlag = OMX_TRUE;
                    pBaseComponentPort->IdleToLoadedFlag = OMX_FALSE;
                    iStateTransitionFlag = OMX_FALSE;
                }
            }
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : UseBuffer OUT"));
            return OMX_ErrorNone;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : UseBuffer OUT"));
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->AllocateBuffer(hComponent, pBuffer, nPortIndex, pAppPrivate, nSizeBytes);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer IN"));

    AvcComponentPortType* pBaseComponentPort;
    OMX_U32 ii;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer error incorrect state"));
        return OMX_ErrorIncorrectStateTransition;
    }

    if (NULL == pBaseComponentPort->pBuffer)
    {
        pBaseComponentPort->pBuffer = (OMX_BUFFERHEADERTYPE**) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
        pBaseComponentPort->BufferState = (OMX_U32*) oscl_calloc(pBaseComponentPort->PortParam.nBufferCountActual, sizeof(OMX_U32));
    }

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if (!(pBaseComponentPort->BufferState[ii] & BUFFER_ALLOCATED) &&
                !(pBaseComponentPort->BufferState[ii] & BUFFER_ASSIGNED))
        {
            pBaseComponentPort->pBuffer[ii] = (OMX_BUFFERHEADERTYPE*) oscl_malloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (NULL == pBaseComponentPort->pBuffer[ii])
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            SetHeader(pBaseComponentPort->pBuffer[ii], sizeof(OMX_BUFFERHEADERTYPE));
            /* allocate the buffer */
            pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);
            if (NULL == pBaseComponentPort->pBuffer[ii]->pBuffer)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            pBaseComponentPort->pBuffer[ii]->nAllocLen = nSizeBytes;
            pBaseComponentPort->pBuffer[ii]->nFlags = 0;
            pBaseComponentPort->pBuffer[ii]->pPlatformPrivate = pBaseComponentPort;
            pBaseComponentPort->pBuffer[ii]->pAppPrivate = pAppPrivate;
            *pBuffer = pBaseComponentPort->pBuffer[ii];
            pBaseComponentPort->BufferState[ii] |= BUFFER_ALLOCATED;
            pBaseComponentPort->BufferState[ii] |= HEADER_ALLOCATED;

            if (OMX_DirInput == pBaseComponentPort->PortParam.eDir)
            {
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = nPortIndex;
                // here is assigned a non-valid port index
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = iNumPorts;
            }
            else
            {
                // here is assigned a non-valid port index
                pBaseComponentPort->pBuffer[ii]->nInputPortIndex = iNumPorts;
                pBaseComponentPort->pBuffer[ii]->nOutputPortIndex = nPortIndex;
            }

            pBaseComponentPort->NumAssignedBuffers++;

            if (pBaseComponentPort->PortParam.nBufferCountActual == pBaseComponentPort->NumAssignedBuffers)
            {
                pBaseComponentPort->PortParam.bPopulated = OMX_TRUE;

                if (OMX_TRUE == iStateTransitionFlag)
                {
                    //Reschedule the AO for a state change (Loaded->Idle) if its pending on buffer allocation
                    RunIfNotReady();
                    //Set the corresponding flags
                    pBaseComponentPort->LoadedToIdleFlag = OMX_TRUE;
                    pBaseComponentPort->IdleToLoadedFlag = OMX_FALSE;
                    iStateTransitionFlag = OMX_FALSE;
                }
            }

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer OUT"));
            return OMX_ErrorNone;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AllocateBuffer OUT"));
    return OMX_ErrorInsufficientResources;
}

OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{


    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FreeBuffer(hComponent, nPortIndex, pBuffer);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FreeBuffer IN"));

    AvcComponentPortType* pBaseComponentPort;

    OMX_U32 ii;
    OMX_BOOL FoundBuffer;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FreeBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateLoaded
            && pBaseComponentPort->TransientState != OMX_StateInvalid)
    {

        (*(ipCallbacks->EventHandler))
        (hComponent,
         iCallbackData,
         OMX_EventError, /* The command was completed */
         OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
         nPortIndex, /* The State has been changed in message->MessageParam2 */
         NULL);
    }

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if ((pBaseComponentPort->BufferState[ii] & BUFFER_ALLOCATED) &&
                (pBaseComponentPort->pBuffer[ii]->pBuffer == pBuffer->pBuffer))
        {

            pBaseComponentPort->NumAssignedBuffers--;
            oscl_free(pBuffer->pBuffer);
            pBuffer->pBuffer = NULL;

            if (pBaseComponentPort->BufferState[ii] & HEADER_ALLOCATED)
            {
                oscl_free(pBuffer);
                pBuffer = NULL;
            }
            pBaseComponentPort->BufferState[ii] = BUFFER_FREE;
            break;
        }
        else if ((pBaseComponentPort->BufferState[ii] & BUFFER_ASSIGNED) &&
                 (pBaseComponentPort->pBuffer[ii] == pBuffer))
        {

            pBaseComponentPort->NumAssignedBuffers--;

            if (pBaseComponentPort->BufferState[ii] & HEADER_ALLOCATED)
            {
                oscl_free(pBuffer);
                pBuffer = NULL;
            }

            pBaseComponentPort->BufferState[ii] = BUFFER_FREE;
            break;
        }
    }

    FoundBuffer = OMX_FALSE;

    for (ii = 0; ii < pBaseComponentPort->PortParam.nBufferCountActual; ii++)
    {
        if (pBaseComponentPort->BufferState[ii] != BUFFER_FREE)
        {
            FoundBuffer = OMX_TRUE;
            break;
        }
    }
    if (!FoundBuffer)
    {
        pBaseComponentPort->PortParam.bPopulated = OMX_FALSE;
        if (OMX_TRUE == iStateTransitionFlag)
        {
            //Reschedule the AO for a state change (Idle->Loaded) if its pending on buffer de-allocation
            RunIfNotReady();
            //Set the corresponding flags
            pBaseComponentPort->IdleToLoadedFlag = OMX_TRUE;
            pBaseComponentPort->LoadedToIdleFlag = OMX_FALSE;
            iStateTransitionFlag = OMX_FALSE;
            //Reset the decoding flags while freeing buffers
            if (OMX_PORT_INPUTPORT_INDEX == nPortIndex)
            {
                iIsInputBufferEnded = OMX_TRUE;
                iTempInputBufferLength = 0;
                iTempConsumedLength = 0;
                iNewInBufferRequired = OMX_TRUE;
            }
            else if (OMX_PORT_OUTPUTPORT_INDEX == nPortIndex)
            {
                iNewOutBufRequired = OMX_TRUE;
            }
        }

        if (NULL != pBaseComponentPort->pBuffer)
        {
            oscl_free(pBaseComponentPort->pBuffer);
            pBaseComponentPort->pBuffer = NULL;
            oscl_free(pBaseComponentPort->BufferState);
            pBaseComponentPort->BufferState = NULL;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FreeBuffer OUT"));
    return OMX_ErrorNone;
}


/** Set Callbacks. It stores in the component private structure the pointers to the user application callbacs
	* @param hComponent the handle of the component
	* @param ipCallbacks the OpenMAX standard structure that holds the callback pointers
	* @param pAppData a pointer to a private structure, not covered by OpenMAX standard, in needed
    */

OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentSetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->SetCallbacks(hComponent, pCallbacks, pAppData);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SetCallbacks"));
    ipCallbacks = pCallbacks;
    iCallbackData = pAppData;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentSendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam,
    OMX_IN  OMX_PTR pCmdData)
{

    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->SendCommand(hComponent, Cmd, nParam, pCmdData);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::SendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_S32 nParam,
    OMX_IN  OMX_PTR pCmdData)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand IN"));

    OMX_U32 ii;
    OMX_ERRORTYPE ErrMsgHandler = OMX_ErrorNone;
    QueueType* pMessageQueue;
    CoreMessage* Message = NULL;

    pMessageQueue = ipCoreDescriptor->pMessageQueue;

    if (OMX_StateInvalid == iState)
    {
        ErrMsgHandler = OMX_ErrorInvalidState;
    }

    switch (Cmd)
    {
        case OMX_CommandStateSet:
        {
            Message = (CoreMessage*) oscl_malloc(sizeof(CoreMessage));

            if (NULL == Message)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }

            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            Message->MessageType = SENDCOMMAND_MSG_TYPE;
            Message->MessageParam1 = OMX_CommandStateSet;
            Message->MessageParam2 = nParam;
            Message->pCmdData = pCmdData;

            if ((OMX_StateIdle == nParam) && (OMX_StateLoaded == iState))
            {
                ErrMsgHandler = AvcComponentInit();

                if (ErrMsgHandler != OMX_ErrorNone)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error component init"));
                    return OMX_ErrorInsufficientResources;
                }
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    ipPorts[ii]->TransientState = OMX_StateIdle;
                }
            }
            else if ((OMX_StateLoaded == nParam) && (OMX_StateIdle == iState))
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (PORT_IS_ENABLED(ipPorts[ii]))
                    {
                        ipPorts[ii]->TransientState = OMX_StateLoaded;
                    }
                }
            }
            else if (OMX_StateInvalid == nParam)
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (PORT_IS_ENABLED(ipPorts[ii]))
                    {
                        ipPorts[ii]->TransientState = OMX_StateInvalid;
                    }
                }
            }
            else if (((OMX_StateIdle == nParam) || (OMX_StatePause == nParam))
                     && (OMX_StateExecuting == iState))
            {
                iBufferExecuteFlag = OMX_FALSE;
            }

        }
        break;

        case OMX_CommandFlush:
        {
            Message = (CoreMessage*) oscl_malloc(sizeof(CoreMessage));

            if (NULL == Message)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }

            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            Message->MessageType = SENDCOMMAND_MSG_TYPE;
            Message->MessageParam1 = OMX_CommandFlush;
            Message->MessageParam2 = nParam;
            Message->pCmdData = pCmdData;

            if ((iState != OMX_StateExecuting) && (iState != OMX_StatePause))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error incorrect state"));
                ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                break;

            }
            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            AvcComponentSetPortFlushFlag(iNumPorts, nParam, OMX_TRUE);
            AvcComponentSetNumBufferFlush(iNumPorts, -1, 0);
        }
        break;

        case OMX_CommandPortDisable:
        {
            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }

            iResizePending = OMX_FALSE; //this will enable processing

            if (-1 == nParam)
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (!PORT_IS_ENABLED(ipPorts[ii]))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error incorrect state"));
                        ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                        break;
                    }
                    else
                    {
                        ipPorts[ii]->TransientState = OMX_StateLoaded;
                    }
                }
            }
            else
            {
                if (!PORT_IS_ENABLED(ipPorts[nParam]))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error incorrect state"));
                    ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                    break;
                }
                else
                {
                    ipPorts[nParam]->TransientState = OMX_StateLoaded;
                }
            }

            Message = (CoreMessage*) oscl_malloc(sizeof(CoreMessage));
            if (NULL == Message)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }

            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            if (OMX_ErrorNone == ErrMsgHandler)
            {
                Message->MessageType = SENDCOMMAND_MSG_TYPE;
                Message->MessageParam2 = nParam;
            }
            else
            {
                Message->MessageType = ERROR_MSG_TYPE;
                Message->MessageParam2 = ErrMsgHandler;
            }
            Message->MessageParam1 = OMX_CommandPortDisable;
            Message->pCmdData = pCmdData;
        }
        break;


        case OMX_CommandPortEnable:
        {
            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }

            if (-1 == nParam)
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (PORT_IS_ENABLED(ipPorts[ii]))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error incorrect state"));
                        ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                        break;
                    }
                    else
                    {
                        ipPorts[ii]->TransientState = OMX_StateIdle;
                    }
                }
            }
            else
            {
                if (PORT_IS_ENABLED(ipPorts[nParam]))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error incorrect state"));
                    ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                    break;
                }
                else
                {
                    ipPorts[nParam]->TransientState = OMX_StateIdle;
                }
            }

            Message = (CoreMessage*) oscl_malloc(sizeof(CoreMessage));
            if (NULL == Message)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }

            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            if (OMX_ErrorNone == ErrMsgHandler)
            {
                Message->MessageType = SENDCOMMAND_MSG_TYPE;
            }
            else
            {
                Message->MessageType = ERROR_MSG_TYPE;
            }

            Message->MessageParam1 = OMX_CommandPortEnable;
            Message->MessageParam2 = nParam;
            Message->pCmdData = pCmdData;
        }
        break;


        case OMX_CommandMarkBuffer:
        {
            if ((iState != OMX_StateExecuting) && (iState != OMX_StatePause))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error incorrect state"));
                ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                break;
            }

            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }

            Message = (CoreMessage*) oscl_malloc(sizeof(CoreMessage));
            if (NULL == Message)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            Message->MessageType = SENDCOMMAND_MSG_TYPE;
            Message->MessageParam1 = OMX_CommandMarkBuffer;
            Message->MessageParam2 = nParam;
            Message->pCmdData = pCmdData;
        }
        break;


        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand error unsupported index"));
            ErrMsgHandler = OMX_ErrorUnsupportedIndex;
        }
        break;
    }

    Queue(pMessageQueue, Message);
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : SendCommand OUT"));
    return ErrMsgHandler;
}


/** This is called by the OMX core in its message processing
 * thread context upon a component request. A request is made
 * by the component when some asynchronous services are needed:
 * 1) A SendCommand() is to be processed
 * 2) An error needs to be notified
 * \param Message, the message that has been passed to core
 */

OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentMessageHandler(CoreMessage* Message)
{

    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;
    OMX_U32 ii;
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;


    /** Dealing with a SendCommand call.
     * -MessageParam1 contains the command to execute
     * -MessageParam2 contains the parameter of the command
     *  (destination state in case of a state change command).
     */

    OMX_STATETYPE orig_state = iState;
    if (SENDCOMMAND_MSG_TYPE == Message->MessageType)
    {
        switch (Message->MessageParam1)
        {
            case OMX_CommandStateSet:
            {
                /* Do the actual state change */
                ErrorType = AvcComponentDoStateSet(Message->MessageParam2);

                if (OMX_TRUE == iStateTransitionFlag)
                {
                    return OMX_ErrorNone;
                }

                //Do not send the callback now till the State gets changed
                if (ErrorType != OMX_ErrorNone)
                {
                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventError, /* The command was completed */
                     ErrorType, /* The commands was a OMX_CommandStateSet */
                     0, /* The iState has been changed in Message->MessageParam2 */
                     NULL);
                }
                else
                {
                    /* And run the callback */
                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventCmdComplete, /* The command was completed */
                     OMX_CommandStateSet, /* The commands was a OMX_CommandStateSet */
                     Message->MessageParam2, /* The iState has been changed in Message->MessageParam2 */
                     NULL);
                }
            }
            break;

            case OMX_CommandFlush:
            {
                /*Flush ports*/
                ErrorType = AvcComponentFlushPort(Message->MessageParam2);

                AvcComponentSetNumBufferFlush(iNumPorts, -1, 0);

                if (ErrorType != OMX_ErrorNone)
                {
                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventError, /* The command was completed */
                     ErrorType, /* The commands was a OMX_CommandStateSet */
                     0, /* The iState has been changed in Message->MessageParam2 */
                     NULL);
                }
                else
                {
                    if (-1 == Message->MessageParam2)
                    { /*Flush all port*/
                        for (ii = 0; ii < iNumPorts; ii++)
                        {
                            (*(ipCallbacks->EventHandler))
                            (pHandle,
                             iCallbackData,
                             OMX_EventCmdComplete, /* The command was completed */
                             OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
                             ii, /* The iState has been changed in Message->MessageParam2 */
                             NULL);
                        }
                    }
                    else
                    {/*Flush input/output port*/
                        (*(ipCallbacks->EventHandler))
                        (pHandle,
                         iCallbackData,
                         OMX_EventCmdComplete, /* The command was completed */
                         OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
                         Message->MessageParam2, /* The iState has been changed in Message->MessageParam2 */
                         NULL);
                    }
                }
                AvcComponentSetPortFlushFlag(iNumPorts, -1, OMX_FALSE);
            }
            break;

            case OMX_CommandPortDisable:
            {
                /** This condition is added to pass the tests, it is not significant for the environment */
                ErrorType = AvcComponentDisablePort(Message->MessageParam2);
                if (OMX_TRUE == iStateTransitionFlag)
                {
                    return OMX_ErrorNone;
                }

                if (ErrorType != OMX_ErrorNone)
                {
                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventError, /* The command was completed */
                     ErrorType, /* The commands was a OMX_CommandStateSet */
                     0, /* The iState has been changed in Message->MessageParam2 */
                     NULL);
                }
                else
                {
                    if (-1 == Message->MessageParam2)
                    { /*Disable all ports*/
                        for (ii = 0; ii < iNumPorts; ii++)
                        {
                            (*(ipCallbacks->EventHandler))
                            (pHandle,
                             iCallbackData,
                             OMX_EventCmdComplete, /* The command was completed */
                             OMX_CommandPortDisable, /* The commands was a OMX_CommandStateSet */
                             ii, /* The iState has been changed in Message->MessageParam2 */
                             NULL);
                        }
                    }
                    else
                    {
                        (*(ipCallbacks->EventHandler))
                        (pHandle,
                         iCallbackData,
                         OMX_EventCmdComplete, /* The command was completed */
                         OMX_CommandPortDisable, /* The commands was a OMX_CommandStateSet */
                         Message->MessageParam2, /* The iState has been changed in Message->MessageParam2 */
                         NULL);
                    }
                }
            }
            break;

            case OMX_CommandPortEnable:
            {
                ErrorType = AvcComponentEnablePort(Message->MessageParam2);
                if (OMX_TRUE == iStateTransitionFlag)
                {
                    return OMX_ErrorNone;
                }

                if (ErrorType != OMX_ErrorNone)
                {
                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventError, /* The command was completed */
                     ErrorType, /* The commands was a OMX_CommandStateSet */
                     0, /* The State has been changed in Message->MessageParam2 */
                     NULL);
                }
                else
                {
                    if (Message->MessageParam2 != -1)
                    {
                        (*(ipCallbacks->EventHandler))
                        (pHandle,
                         iCallbackData,
                         OMX_EventCmdComplete, /* The command was completed */
                         OMX_CommandPortEnable, /* The commands was a OMX_CommandStateSet */
                         Message->MessageParam2, /* The State has been changed in Message->MessageParam2 */
                         NULL);
                    }
                    else
                    {
                        for (ii = 0; ii < iNumPorts; ii++)
                        {
                            (*(ipCallbacks->EventHandler))
                            (pHandle,
                             iCallbackData,
                             OMX_EventCmdComplete, /* The command was completed */
                             OMX_CommandPortEnable, /* The commands was a OMX_CommandStateSet */
                             ii, /* The State has been changed in Message->MessageParam2 */
                             NULL);
                        }
                    }
                }
            }
            break;

            case OMX_CommandMarkBuffer:
            {
                ipMark = (OMX_MARKTYPE *)Message->pCmdData;
            }
            break;

            default:
            {

            }
            break;
        }
        /* Dealing with an asynchronous error condition
         */
    }

    if (orig_state != OMX_StateInvalid)
    {
        ErrorType = OMX_ErrorNone;
    }

    return ErrorType;
}

/** Changes the state of a component taking proper actions depending on
 *  the transiotion requested
 * \param Component, the component which state is to be changed
 * \param aDestinationState the requested target state.
 */

OMX_ERRORTYPE OpenmaxAvcAO::AvcComponentDoStateSet(OMX_U32 aDestinationState)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE,
                    (0, "OpenmaxAvcAO : AvcComponentDoStateSet IN : iState (%i) aDestinationState (%i)", iState, aDestinationState));

    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;
    OMX_U32 ii;

    if (OMX_StateLoaded == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }

            case OMX_StateWaitForResources:
            {
                iState = OMX_StateLoaded;
            }
            break;

            case OMX_StateLoaded:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            case OMX_StateIdle:
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (PORT_IS_ENABLED(ipPorts[ii]) &&
                            PORT_IS_POPULATED(ipPorts[ii]))
                    {
                        if (OMX_FALSE == ipPorts[ii]->IdleToLoadedFlag)
                        {
                            iStateTransitionFlag = OMX_TRUE;
                        }

                        else
                        {
                            ipPorts[ii]->PortParam.bPopulated = OMX_FALSE;
                            ipPorts[ii]->TransientState = OMX_StateMax;
                        }
                    }
                }
                if (OMX_TRUE == iStateTransitionFlag)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet Waiting port to be de-populated"));
                    return OMX_ErrorNone;
                }

                iState = OMX_StateLoaded;

                iNumInputBuffer = 0;
                iOutBufferCount = 0;
                iPartialFrameAssembly = OMX_FALSE;
                iEndofStream = OMX_FALSE;
                iIsInputBufferEnded = OMX_TRUE;
                iNewInBufferRequired = OMX_TRUE;
                iNewOutBufRequired = OMX_TRUE;
                iFirstFragment = OMX_FALSE;

                AvcComponentDeInit();
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateWaitForResources == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            case OMX_StateLoaded:
            {
                iState = OMX_StateWaitForResources;
            }
            break;

            case OMX_StateWaitForResources:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateIdle == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            case OMX_StateWaitForResources:
            {
                iState = OMX_StateIdle;
            }
            break;

            case OMX_StateLoaded:
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (PORT_IS_ENABLED(ipPorts[ii]) &&
                            !PORT_IS_POPULATED(ipPorts[ii]))
                    {
                        if (OMX_FALSE == ipPorts[ii]->LoadedToIdleFlag)
                        {
                            iStateTransitionFlag = OMX_TRUE;
                        }
                        else
                        {
                            ipPorts[ii]->PortParam.bPopulated = OMX_TRUE;
                            ipPorts[ii]->TransientState = OMX_StateMax;
                        }
                    }
                }
                if (OMX_TRUE == iStateTransitionFlag)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet Waiting port to be populated"));
                    return OMX_ErrorNone;
                }

                iState = OMX_StateIdle;

                //Used in case of partial frame assembly
                if (!ipInputCurrBuffer)
                {
                    //Keep the size of temp buffer double to be on safer side
                    ipInputCurrBuffer = (OMX_U8*) oscl_malloc(2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));
                    if (NULL == ipInputCurrBuffer)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error insufficient resources"));
                        return OMX_ErrorInsufficientResources;
                    }
                    iInputCurrBufferSize = 2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize);
                }

                if (!ipTempInputBuffer)
                {
                    ipTempInputBuffer = (OMX_U8*) oscl_malloc(2 * sizeof(uint8) * ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize);
                    if (NULL == ipTempInputBuffer)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error insufficient resources"));
                        return OMX_ErrorInsufficientResources;
                    }
                }

                iTempInputBufferLength = 0;
                iTempConsumedLength = 0;
            }
            break;

            case OMX_StateIdle:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            //Both the below cases have same body
            case OMX_StateExecuting:
            case OMX_StatePause:
            {
                AvcComponentSetNumBufferFlush(iNumPorts, -1, 0);
                AvcComponentSetPortFlushFlag(iNumPorts, -1, OMX_TRUE);

                AvcComponentPortType* pInPort = (AvcComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];

                //Return all the buffers if still occupied
                QueueType *pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;

                while ((iNumInputBuffer > 0) && (GetQueueNumElem(pInputQueue) > 0))
                {
                    AvcComponentFlushPort(OMX_PORT_INPUTPORT_INDEX);
                }
                // if a buffer was previously dequeued, it wasnt freed in above loop. return it now
                if (iNumInputBuffer > 0)
                {
                    ipAvcInputBuffer->nFilledLen = 0;
                    AvcComponentReturnInputBuffer(ipAvcInputBuffer, pInPort);
                    iNewInBufferRequired = OMX_TRUE;
                    iIsInputBufferEnded = OMX_TRUE;
                }

                //Mark these flags as true
                iIsInputBufferEnded = OMX_TRUE;
                iEndofStream = OMX_FALSE;
                iNewInBufferRequired = OMX_TRUE;

                //Added these statement here
                iTempInputBufferLength = 0;
                iTempConsumedLength = 0;

                while (iOutBufferCount > 0)
                {
                    AvcComponentFlushPort(OMX_PORT_OUTPUTPORT_INDEX);
                }

                AvcComponentSetPortFlushFlag(iNumPorts, -1, OMX_FALSE);
                AvcComponentSetNumBufferFlush(iNumPorts, -1, 0);

                iState = OMX_StateIdle;
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet OUT"));
        return ErrorType;
    }

    if (OMX_StatePause == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            case OMX_StatePause:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            //Falling through to the next case
            case OMX_StateExecuting:
            case OMX_StateIdle:
            {
                iState = OMX_StatePause;
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateExecuting == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            case OMX_StateIdle:
            {
                iState = OMX_StateExecuting;
            }
            break;

            case OMX_StatePause:
            {
                iState = OMX_StateExecuting;
                /* A trigger to start the processing of buffers when component
                 * transitions to executing from pause, as it is already
                 * holding the required buffers
                 */
                RunIfNotReady();
            }
            break;

            case OMX_StateExecuting:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateInvalid == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            // break;	This break statement was removed to avoid compiler warning for Unreachable Code

            default:
            {
                iState = OMX_StateInvalid;
                if (iIsInit != OMX_FALSE)
                {
                    AvcComponentDeInit();
                }
            }
            break;
        }

        if (iIsInit != OMX_FALSE)
        {
            AvcComponentDeInit();
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet error invalid state"));
        return OMX_ErrorInvalidState;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : AvcComponentDoStateSet OUT"));
    return OMX_ErrorNone;
}



OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentEmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{

    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->EmptyThisBuffer(hComponent, pBuffer);

    return Status;

}


OMX_ERRORTYPE OpenmaxAvcAO::EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)

{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer IN"));
    //Do not queue buffers if component is in invalid state
    if (OMX_StateInvalid == iState)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer error invalid state"));
        return OMX_ErrorInvalidState;
    }

    if ((OMX_StateIdle == iState) || (OMX_StatePause == iState) || (OMX_StateExecuting == iState))
    {
        OMX_U32 PortIndex;
        QueueType* pInputQueue;
        OMX_ERRORTYPE ErrorType = OMX_ErrorNone;

        PortIndex = pBuffer->nInputPortIndex;

        //Validate the port index & Queue the buffers available only at the input port
        if (PortIndex >= iNumPorts || ipPorts[PortIndex]->PortParam.eDir != OMX_DirInput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer error bad port index"));
            return OMX_ErrorBadPortIndex;
        }

        //Port should be in enabled state before accepting buffers
        if (!PORT_IS_ENABLED(ipPorts[PortIndex]))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer error incorrect state"));
            return OMX_ErrorIncorrectStateOperation;
        }

        /* The number of buffers the component can queue at a time
         * depends upon the number of buffers allocated/assigned on the input port
         */
        if (iNumInputBuffer == ipPorts[PortIndex]->NumAssignedBuffers)
        {
            RunIfNotReady();
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer error incorrect state"));
            return OMX_ErrorIncorrectStateOperation;
        }

        //Finally after passing all the conditions, queue the buffer in Input queue
        pInputQueue = ipPorts[PortIndex]->pBufferQueue;

        if ((ErrorType = CheckHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer error check header failed"));
            return ErrorType;
        }

        iNumInputBuffer++;
        Queue(pInputQueue, pBuffer);

        //Signal the AO about the incoming buffer
        RunIfNotReady();
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer error incorrect state"));
        //This macro is not accepted in any other state except the three mentioned above
        return OMX_ErrorIncorrectStateOperation;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : EmptyThisBuffer OUT"));

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OpenmaxAvcAO::BaseComponentFillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{

    OpenmaxAvcAO* pOpenmaxAOType = (OpenmaxAvcAO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FillThisBuffer(hComponent, pBuffer);

    return Status;
}

OMX_ERRORTYPE OpenmaxAvcAO::FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)

{
    OSCL_UNUSED_ARG(hComponent);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FillThisBuffer IN"));

    OMX_U32 PortIndex;

    QueueType* pOutputQueue;
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;

    PortIndex = pBuffer->nOutputPortIndex;
    //Validate the port index & Queue the buffers available only at the output port
    if (PortIndex >= iNumPorts ||
            ipPorts[PortIndex]->PortParam.eDir != OMX_DirOutput)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FillThisBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pOutputQueue = ipPorts[PortIndex]->pBufferQueue;
    if (iState != OMX_StateExecuting &&
            iState != OMX_StatePause &&
            iState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FillThisBuffer error invalid state"));
        return OMX_ErrorInvalidState;
    }

    //Port should be in enabled state before accepting buffers
    if (!PORT_IS_ENABLED(ipPorts[PortIndex]))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FillThisBuffer error incorrect state"));
        return OMX_ErrorIncorrectStateOperation;
    }

    if ((ErrorType = CheckHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FillThisBuffer error check header failed"));
        return ErrorType;
    }

    //Queue the buffer in output queue
    Queue(pOutputQueue, pBuffer);
    iOutBufferCount++;

    //Signal the AO about the incoming buffer
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : FillThisBuffer OUT"));

    return OMX_ErrorNone;
}

void OpenmaxAvcAO::Run()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : Run IN"));

    CoreMessage* pCoreMessage;

    //Execute the commands from the message handler queue
    if ((GetQueueNumElem(ipCoreDescriptor->pMessageQueue) > 0))
    {
        pCoreMessage = (CoreMessage*) DeQueue(ipCoreDescriptor->pMessageQueue);

        if (OMX_CommandStateSet == pCoreMessage->MessageParam1)
        {
            if (OMX_StateExecuting == pCoreMessage->MessageParam2)
            {
                iBufferExecuteFlag = OMX_TRUE;
            }
            else
            {
                iBufferExecuteFlag = OMX_FALSE;
            }
        }

        AvcComponentMessageHandler(pCoreMessage);

        /* If some allocations/deallocations are required before the state transition
         * then queue the command again to be executed later on
         */
        if (OMX_TRUE == iStateTransitionFlag)
        {
            Queue(ipCoreDescriptor->pMessageQueue, pCoreMessage);
            // Don't reschedule. Wait for arriving buffers to do it
            //RunIfNotReady();
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : Run OUT"));
            return;
        }

        else
        {
            oscl_free(pCoreMessage);
            pCoreMessage = NULL;
        }
    }

    /* If the component is in executing state, call the Buffer management function.
     * Stop calling this function as soon as state transition request is received.
     */
    if ((OMX_TRUE == iBufferExecuteFlag) && (OMX_TRUE != iResizePending))
    {
        AvcComponentBufferMgmtFunction();
    }

    //Check for any more commands in the message handler queue & schedule them for later
    if ((GetQueueNumElem(ipCoreDescriptor->pMessageQueue) > 0))
    {
        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxAvcAO : Run OUT"));

    return;
}


