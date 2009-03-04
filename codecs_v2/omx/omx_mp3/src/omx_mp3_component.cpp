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
#include <string.h>
#include "omx_mp3_component.h"

extern OMX_U32 g_ComponentIndex; // this is determined outside the component

#if PROXY_INTERFACE

#include "omx_proxy_interface.h"
extern ProxyApplication_OMX* pProxyTerm[];

#endif


#define OMX_HALFRANGE_THRESHOLD 0x7FFFFFFF

// This function is called by OMX_GetHandle and it creates an instance of the mp3 component AO
OMX_ERRORTYPE Mp3OmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData)
{
    OpenmaxMp3AO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitMp3OmxComponentFields content to actual constructor

    pOpenmaxAOType = (OpenmaxMp3AO*) OSCL_NEW(OpenmaxMp3AO, ());

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
OMX_ERRORTYPE Mp3OmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle)
{
    // get pointer to component AO
    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up decoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

OMX_ERRORTYPE OpenmaxMp3AO::ConstructComponent(OMX_PTR pAppData)
{
    Mp3ComponentPortType* pInPort, *pOutPort;
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

    iOmxComponent.SendCommand = OpenmaxMp3AO::BaseComponentSendCommand;
    iOmxComponent.GetParameter = OpenmaxMp3AO::BaseComponentGetParameter;
    iOmxComponent.SetParameter = OpenmaxMp3AO::BaseComponentSetParameter;
    iOmxComponent.GetConfig = OpenmaxMp3AO::BaseComponentGetConfig;
    iOmxComponent.SetConfig = OpenmaxMp3AO::BaseComponentSetConfig;
    iOmxComponent.GetExtensionIndex = OpenmaxMp3AO::BaseComponentGetExtensionIndex;
    iOmxComponent.GetState = OpenmaxMp3AO::BaseComponentGetState;
    iOmxComponent.UseBuffer = OpenmaxMp3AO::BaseComponentUseBuffer;
    iOmxComponent.AllocateBuffer = OpenmaxMp3AO::BaseComponentAllocateBuffer;
    iOmxComponent.FreeBuffer = OpenmaxMp3AO::BaseComponentFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OpenmaxMp3AO::BaseComponentEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OpenmaxMp3AO::BaseComponentFillThisBuffer;
#endif

    iOmxComponent.SetCallbacks = OpenmaxMp3AO::BaseComponentSetCallbacks;
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

    ipAppPriv = (Mp3PrivateType*) oscl_malloc(sizeof(Mp3PrivateType));
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

        ipPorts = (Mp3ComponentPortType**) oscl_calloc(iNumPorts, sizeof(Mp3ComponentPortType*));
        if (!ipPorts)
        {
            return OMX_ErrorInsufficientResources;
        }

        for (ii = 0; ii < iNumPorts; ii++)
        {
            ipPorts[ii] = (Mp3ComponentPortType*) oscl_calloc(1, sizeof(Mp3ComponentPortType));
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

        Mp3ComponentSetPortFlushFlag(iNumPorts, -1, OMX_FALSE);
        Mp3ComponentSetNumBufferFlush(iNumPorts, -1, OMX_FALSE);
    }

    /** Domain specific section for the ports. */
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainAudio;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.cMIMEType = "audio/mpeg";
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.pNativeRender = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingMP3;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDir = OMX_DirInput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_INPUT_BUFFER_MP3;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize = INPUT_BUFFER_SIZE_MP3;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;


    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainAudio;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.cMIMEType = "raw";
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.pNativeRender = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDir = OMX_DirOutput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_OUTPUT_BUFFER_MP3;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = OUTPUT_BUFFER_SIZE_MP3 * 6;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;

    //Default values for Mp3 audio param port
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioMp3Param.nChannels = 2;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioMp3Param.nBitRate = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioMp3Param.nSampleRate = 44100;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioMp3Param.nAudioBandWidth = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioMp3Param.eChannelMode = OMX_AUDIO_ChannelModeStereo;

    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioEqualizerType.sBandIndex.nMin = 0;
    //Taken these value from from mp3 decoder component
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioEqualizerType.sBandIndex.nValue = (e_equalization) flat;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioEqualizerType.sBandIndex.nMax = (e_equalization) flat_;


    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.nChannels = 2;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.eNumData = OMX_NumericalDataSigned;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.bInterleaved = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.nBitPerSample = 16;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.nSamplingRate = 44100;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    iPortTypesParam.nPorts = 2;
    iPortTypesParam.nStartPortNumber = 0;

    pInPort = (Mp3ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    pOutPort = (Mp3ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    SetHeader(&pInPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    pInPort->AudioParam.nPortIndex = 0;
    pInPort->AudioParam.nIndex = 0;
    pInPort->AudioParam.eEncoding = OMX_AUDIO_CodingMP3;

    SetHeader(&pOutPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    pOutPort->AudioParam.nPortIndex = 1;
    pOutPort->AudioParam.nIndex = 0;
    pOutPort->AudioParam.eEncoding = OMX_AUDIO_CodingPCM;

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
    iNewOutBufRequired = OMX_TRUE;
    iOutputFrameLength = OUTPUT_BUFFER_SIZE_MP3;
    iRepositionFlag = OMX_FALSE;

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

    //Will be used in case of partial frame assembly
    ipInputCurrBuffer = NULL;
    ipAppPriv->Mp3Handle = &iOmxComponent;

    if (ipMp3Dec)
    {
        OSCL_DELETE(ipMp3Dec);
        ipMp3Dec = NULL;
    }

    ipMp3Dec = OSCL_NEW(Mp3Decoder, ());
    if (ipMp3Dec == NULL)
    {
        return OMX_ErrorInsufficientResources;
    }

    oscl_memset(ipMp3Dec, 0, sizeof(Mp3Decoder));

    iSamplesPerFrame = DEFAULT_SAMPLES_PER_FRAME_MP3;
    iOutputMilliSecPerFrame = iCurrentFrameTS.GetFrameDuration();

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

void OpenmaxMp3AO::SetHeader(OMX_PTR aHeader, OMX_U32 aSize)
{
    OMX_VERSIONTYPE* pVersion = (OMX_VERSIONTYPE*)((OMX_STRING) aHeader + sizeof(OMX_U32));
    *((OMX_U32*) aHeader) = aSize;

    pVersion->s.nVersionMajor = SPECVERSIONMAJOR;
    pVersion->s.nVersionMinor = SPECVERSIONMINOR;
    pVersion->s.nRevision = SPECREVISION;
    pVersion->s.nStep = SPECSTEP;
}


OMX_ERRORTYPE OpenmaxMp3AO::CheckHeader(OMX_PTR aHeader, OMX_U32 aSize)
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
OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentParameterSanityCheck(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_PTR pStructure,
    OMX_IN  size_t size)
{
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
void OpenmaxMp3AO::Mp3ComponentSetPortFlushFlag(OMX_S32 NumPorts, OMX_S32 index, OMX_BOOL value)
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
void OpenmaxMp3AO::Mp3ComponentSetNumBufferFlush(OMX_S32 NumPorts, OMX_S32 index, OMX_S32 value)
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

OMX_BOOL OpenmaxMp3AO::Mp3ComponentAssemblePartialFrames(OMX_BUFFERHEADERTYPE* aInputBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentAssemblePartialFrames IN"));

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;

    Mp3ComponentPortType* pInPort = ipPorts[OMX_PORT_INPUTPORT_INDEX];


    ipMp3InputBuffer = aInputBuffer;

    if (!iPartialFrameAssembly)
    {
        if (iNumInputBuffer > 0)
        {
            if (ipMp3InputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
            {
                iInputCurrLength = ipMp3InputBuffer->nFilledLen;
                ipFrameDecodeBuffer = ipMp3InputBuffer->pBuffer + ipMp3InputBuffer->nOffset;
                //capture the timestamp to be send to the corresponding output buffer
                iFrameTimestamp = ipMp3InputBuffer->nTimeStamp;
            }
            else
            {
                iInputCurrLength = 0;
                iPartialFrameAssembly = OMX_TRUE;
                iFirstFragment = OMX_TRUE;
                iFrameTimestamp = ipMp3InputBuffer->nTimeStamp;
                ipFrameDecodeBuffer = ipInputCurrBuffer;
            }

        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentAssemblePartialFrames ERROR"));
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
                if (iFrameTimestamp != ipMp3InputBuffer->nTimeStamp)
                {
                    iInputCurrLength = 0;
                    iPartialFrameAssembly = OMX_TRUE;
                    iFirstFragment = OMX_TRUE;
                    iFrameTimestamp = ipMp3InputBuffer->nTimeStamp;
                    ipFrameDecodeBuffer = ipInputCurrBuffer;
                }
            }

            if ((ipMp3InputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) != 0)
            {
                break;
            }

            iInputCurrLength += ipMp3InputBuffer->nFilledLen;
            oscl_memcpy(ipFrameDecodeBuffer, (ipMp3InputBuffer->pBuffer + ipMp3InputBuffer->nOffset), ipMp3InputBuffer->nFilledLen); // copy buffer data
            ipFrameDecodeBuffer += ipMp3InputBuffer->nFilledLen; // move the ptr

            ipMp3InputBuffer->nFilledLen = 0;

            Mp3ComponentReturnInputBuffer(ipMp3InputBuffer, pInPort);

            iFirstFragment = OMX_FALSE;

            if (iNumInputBuffer > 0)
            {
                ipMp3InputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);
                if (ipMp3InputBuffer->nFlags & OMX_BUFFERFLAG_EOS)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentAssemblePartialFrames EndOfStream arrived"));
                    iEndofStream = OMX_TRUE;
                }
            }
        }

        // if we broke out of the while loop because of lack of buffers, then return and wait for more input buffers
        if (0 == iNumInputBuffer)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentAssemblePartialFrames OUT"));
            return OMX_FALSE;
        }
        else
        {
            // we have found the buffer that is the last piece of the frame.
            // Copy the buffer, but do not release it yet (this will be done after decoding for consistency)

            iInputCurrLength += ipMp3InputBuffer->nFilledLen;
            oscl_memcpy(ipFrameDecodeBuffer, (ipMp3InputBuffer->pBuffer + ipMp3InputBuffer->nOffset), ipMp3InputBuffer->nFilledLen); // copy buffer data
            ipFrameDecodeBuffer += ipMp3InputBuffer->nFilledLen; // move the ptr

            ipFrameDecodeBuffer = ipInputCurrBuffer; // reset the pointer back to beginning of assembly buffer
            iPartialFrameAssembly = OMX_FALSE; // we have finished with assembling the frame, so this is not needed any more
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentAssemblePartialFrames OUT"));
    return OMX_TRUE;
}


/** This is the central function for buffers processing and decoding.
	* It is called through the Run() of active object when the component is in executing state
	* and is signalled each time a new buffer is available on the given ports
	* This function will process the input buffers & return output buffers
	*/

void OpenmaxMp3AO::Mp3ComponentBufferMgmtFunction()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentBufferMgmtFunction IN"));

    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    Mp3ComponentPortType* pInPort = (Mp3ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];

    OMX_BOOL PartialFrameReturn, Status;

    /* Don't dequeue any further buffer after endofstream buffer has been dequeued
     * till we send the callback and reset the flag back to false
     */
    if (OMX_FALSE == iEndofStream)
    {
        //More than one frame can't be dequeued in case of outbut blocked
        if ((OMX_TRUE == iIsInputBufferEnded) && (GetQueueNumElem(pInputQueue) > 0))
        {
            ipMp3InputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);

            if (ipMp3InputBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentBufferMgmtFunction EndOfStream arrived"));
                iEndofStream = OMX_TRUE;
            }

            if (ipMp3InputBuffer->nFilledLen != 0)
            {
                if (0 == iFrameCount)
                {
                    //Set the marker flag (iEndOfFrameFlag) if first frame has the EnfOfFrame flag marked.
                    if (ipMp3InputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentBufferMgmtFunction EndOfFrame flag present"));
                        iEndOfFrameFlag = OMX_TRUE;
                    }

                    //Get the initial default value for the milli seconds per frame
                    iOutputMilliSecPerFrame = iCurrentFrameTS.GetFrameDuration();
                }

                /* This condition will be true if OMX_BUFFERFLAG_ENDOFFRAME flag is
                 *  not marked in all the input buffers
                 */
                if (!iEndOfFrameFlag)
                {
                    Status = Mp3BufferMgmtWithoutMarker(ipMp3InputBuffer);
                    if (OMX_FALSE == Status)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentBufferMgmtFunction OUT"));
                        return;
                    }

                }
                //If OMX_BUFFERFLAG_ENDOFFRAME flag is marked, come here
                else
                {
                    PartialFrameReturn = Mp3ComponentAssemblePartialFrames(ipMp3InputBuffer);
                    if (OMX_FALSE == PartialFrameReturn)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentBufferMgmtFunction OUT"));
                        return;
                    }
                    iIsInputBufferEnded = OMX_FALSE;
                }

                ipTargetComponent = (OMX_COMPONENTTYPE*) ipMp3InputBuffer->hMarkTargetComponent;

                iTargetMarkData = ipMp3InputBuffer->pMarkData;
                if (ipTargetComponent == (OMX_COMPONENTTYPE*) pHandle)
                {
                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventMark,
                     1,
                     0,
                     ipMp3InputBuffer->pMarkData);
                }


                //Do not check for silence insertion if the clip is repositioned
                if (OMX_FALSE == iRepositionFlag)
                {
                    CheckForSilenceInsertion();
                }

                /* Set the current timestamp equal to input buffer timestamp in case of
                 * a) All input frames
                 * b) First frame after repositioning */
                if (OMX_FALSE == iSilenceInsertionInProgress || OMX_TRUE == iRepositionFlag)
                {
                    // Set the current timestamp equal to input buffer timestamp
                    iCurrentFrameTS.SetFromInputTimestamp(iFrameTimestamp);

                    //Reset the flag back to false, once timestamp has been updated from input frame
                    if (OMX_TRUE == iRepositionFlag)
                    {
                        iRepositionFlag = OMX_FALSE;
                    }
                }


            }	//end braces for if (ipMp3InputBuffer->nFilledLen != 0)
            else
            {
                Mp3ComponentReturnInputBuffer(ipMp3InputBuffer, pInPort);
            }

        }	//end braces for if ((OMX_TRUE == iIsInputBufferEnded) && (GetQueueNumElem(pInputQueue) > 0))
    }	//if (OMX_FALSE == iEndofStream)

    Mp3Decode();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentBufferMgmtFunction OUT"));
    return;
}


OMX_BOOL OpenmaxMp3AO::Mp3BufferMgmtWithoutMarker(OMX_BUFFERHEADERTYPE* pMp3InputBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3BufferMgmtWithoutMarker IN"));


    Mp3ComponentPortType*	pInPort = (Mp3ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    QueueType* pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;

    ipMp3InputBuffer = pMp3InputBuffer;

    /* Assembling of partial frame will be done based on max input buf size
     * If Flushport flag is true, that means its not a partial frame
     * but an unconsumed frame, process it independently
     * Same is true for endofstream condition, process the buffer independently
     */
    if ((ipMp3InputBuffer->nFilledLen < ipMp3InputBuffer->nAllocLen)
            && (iEndofStream != OMX_TRUE) && (OMX_FALSE == iFlushPortFlag))
    {
        if (!iPartialFrameAssembly)
        {
            iInputCurrLength = 0;
            ipFrameDecodeBuffer = ipInputCurrBuffer;
        }

        while (iNumInputBuffer > 0)
        {
            oscl_memcpy(ipFrameDecodeBuffer, (ipMp3InputBuffer->pBuffer + ipMp3InputBuffer->nOffset), ipMp3InputBuffer->nFilledLen);
            ipFrameDecodeBuffer += ipMp3InputBuffer->nFilledLen; // move the ptr

            iFrameTimestamp = ipMp3InputBuffer->nTimeStamp;

            if (((iInputCurrLength += ipMp3InputBuffer->nFilledLen) >= ipMp3InputBuffer->nAllocLen)
                    || OMX_TRUE == iEndofStream)
            {
                break;
            }

            //Set the filled len to zero to indiacte buffer is fully consumed
            ipMp3InputBuffer->nFilledLen = 0;
            Mp3ComponentReturnInputBuffer(ipMp3InputBuffer, pInPort);

            if (iNumInputBuffer > 0)
            {
                ipMp3InputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pInputQueue);
                if (ipMp3InputBuffer->nFlags & OMX_BUFFERFLAG_EOS)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3BufferMgmtWithoutMarker EndOfStream arrived"));
                    iEndofStream = OMX_TRUE;
                }
            }
        }

        if (((iInputCurrLength < ipMp3InputBuffer->nAllocLen)) && OMX_TRUE != iEndofStream)
        {
            iPartialFrameAssembly = OMX_TRUE;
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3BufferMgmtWithoutMarker OUT"));
            return OMX_FALSE;
        }
        else
        {
            ipFrameDecodeBuffer = ipInputCurrBuffer;
            iPartialFrameAssembly = OMX_FALSE;
            iIsInputBufferEnded = OMX_FALSE;
        }
    }
    else
    {
        if (iNumInputBuffer > 0)
        {
            iInputCurrLength = ipMp3InputBuffer->nFilledLen;
            ipFrameDecodeBuffer = ipMp3InputBuffer->pBuffer + ipMp3InputBuffer->nOffset;
            iFrameTimestamp = ipMp3InputBuffer->nTimeStamp;
            iIsInputBufferEnded = OMX_FALSE;
        }
        else
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3BufferMgmtWithoutMarker OUT"));
            return OMX_FALSE; // nothing to decode
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3BufferMgmtWithoutMarker OUT"));
    return OMX_TRUE;

}


void OpenmaxMp3AO::Mp3Decode()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3DecodeWithMarker IN"));

    QueueType* pInputQueue  = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    Mp3ComponentPortType* pInPort = (Mp3ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    Mp3ComponentPortType* pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    OMX_U8*	pOutBuffer;
    OMX_U32	OutputLength;
    OMX_S32 DecodeReturn;
    OMX_BOOL ResizeNeeded = OMX_FALSE;

    OMX_U32 TempInputBufferSize = (2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));

    if ((!iIsInputBufferEnded) || iEndofStream)
    {

        if (OMX_TRUE == iSilenceInsertionInProgress)
        {
            DoSilenceInsertion();
            //If the flag is still true, come back to this routine again
            if (OMX_TRUE == iSilenceInsertionInProgress)
            {
                return;
            }
        }
        //Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {

            //Check whether a new output buffer is available or not
            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3DecodeWithMarker OUT output buffer unavailable"));
                return;
            }

            ipMp3OutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
            ipMp3OutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;
            //Set the current timestamp to the output buffer timestamp
            ipMp3OutputBuffer->nTimeStamp = iCurrentFrameTS.GetConvertedTs();
        }

        /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
         * command and hMarkTargetComponent as given by the specifications
         */
        if (ipMark != NULL)
        {
            ipMp3OutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
            ipMp3OutputBuffer->pMarkData = ipMark->pMarkData;
            ipMark = NULL;
        }

        if (ipTargetComponent != NULL)
        {
            ipMp3OutputBuffer->hMarkTargetComponent = ipTargetComponent;
            ipMp3OutputBuffer->pMarkData = iTargetMarkData;
            ipTargetComponent = NULL;

        }
        //Mark buffer code ends here

        pOutBuffer = &ipMp3OutputBuffer->pBuffer[ipMp3OutputBuffer->nFilledLen];
        OutputLength = 0;

        /* Copy the left-over data from last input buffer that is stored in temporary
         * buffer to the next incoming buffer.
         */
        if (iTempInputBufferLength > 0 &&
                ((iInputCurrLength + iTempInputBufferLength) < TempInputBufferSize))
        {
            oscl_memcpy(&ipTempInputBuffer[iTempInputBufferLength], ipFrameDecodeBuffer, iInputCurrLength);
            iInputCurrLength += iTempInputBufferLength;
            iTempInputBufferLength = 0;
            ipFrameDecodeBuffer = ipTempInputBuffer;
        }

        //Output buffer is passed as a short pointer
        DecodeReturn = ipMp3Dec->Mp3DecodeAudio((OMX_S16*) pOutBuffer,
                                                (OMX_U32*) & OutputLength,
                                                &(ipFrameDecodeBuffer),
                                                &iInputCurrLength,
                                                &iFrameCount,
                                                &(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode),
                                                &(ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioMp3Param),
                                                iEndOfFrameFlag,
                                                &ResizeNeeded);


        if (ResizeNeeded == OMX_TRUE)
        {
            if (0 != OutputLength)
            {
                iOutputFrameLength = OutputLength * 2;

                //Update the timestamp
                iSamplesPerFrame = OutputLength / ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.nChannels;

                iCurrentFrameTS.SetParameters(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioPcmMode.nSamplingRate, iSamplesPerFrame);
                iOutputMilliSecPerFrame = iCurrentFrameTS.GetFrameDuration();

                //Set the current timestamp to the output buffer timestamp for the first output frame
                //Later it will be done at the time of dequeue
                ipMp3OutputBuffer->nTimeStamp = iCurrentFrameTS.GetConvertedTs();
            }

            // send port settings changed event
            OMX_COMPONENTTYPE* pHandle = (OMX_COMPONENTTYPE*) ipAppPriv->Mp3Handle;

            iResizePending = OMX_TRUE;

            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventPortSettingsChanged, //The command was completed
             OMX_PORT_OUTPUTPORT_INDEX,
             0,
             NULL);
        }

        ipMp3OutputBuffer->nFilledLen += OutputLength * 2;
        //offset not required in our case, set it to zero
        ipMp3OutputBuffer->nOffset = 0;

        if (OutputLength > 0)
        {
            iCurrentFrameTS.UpdateTimestamp(iSamplesPerFrame);
        }

        /* If EOS flag has come from the client & there are no more
         * input buffers to decode, send the callback to the client
         */
        if (OMX_TRUE == iEndofStream)
        {
            /* Changed the condition here. If EOS has come and decoder can't decode
             * the buffer, do not wait for buffer length to become zero and return
             * the callback as the current buffer may contain partial frame and
             * no other data is going to come now
             */
            if (MP3DEC_SUCCESS != DecodeReturn)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3DecodeWithMarker EOS callback send"));

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventBufferFlag,
                 1,
                 OMX_BUFFERFLAG_EOS,
                 NULL);

                iEndofStream = OMX_FALSE;

                ipMp3OutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;

                Mp3ComponentReturnOutputBuffer(ipMp3OutputBuffer, pOutPort);
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3DecodeWithMarker OUT"));

                return;
            }
        }


        if (MP3DEC_SUCCESS == DecodeReturn)
        {
            ipMp3InputBuffer->nFilledLen = iInputCurrLength;
        }
        else if (MP3DEC_INCOMPLETE_FRAME == DecodeReturn)
        {
            /* If decoder returns MP4AUDEC_INCOMPLETE_FRAME,
             * this indicates the input buffer contains less than a frame data
             * Copy it to a temp buffer to be used in next decode call
             */
            oscl_memcpy(ipTempInputBuffer, ipFrameDecodeBuffer, iInputCurrLength);
            iTempInputBufferLength = iInputCurrLength;
            ipMp3InputBuffer->nFilledLen = 0;
            iInputCurrLength = 0;
        }
        else
        {
            //bitstream error, discard the current data as it can't be decoded further
            ipMp3InputBuffer->nFilledLen = 0;

            //Report it to the client via a callback
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3Decode ErrorStreamCorrupt callback send"));

            (*(ipCallbacks->EventHandler))
            (pHandle,
             iCallbackData,
             OMX_EventError,
             OMX_ErrorStreamCorrupt,
             0,
             NULL);

        }

        //Return the input buffer if it has been consumed fully by the decoder
        if (0 == ipMp3InputBuffer->nFilledLen)
        {
            Mp3ComponentReturnInputBuffer(ipMp3InputBuffer, pInPort);
            iIsInputBufferEnded = OMX_TRUE;
            iInputCurrLength = 0; //make sure to reset to prevent eos issue
        }

        //Send the output buffer back when it has become full
        if (((ipMp3OutputBuffer->nAllocLen - ipMp3OutputBuffer->nFilledLen) < (iOutputFrameLength))
                || (OMX_TRUE == ResizeNeeded))
        {
            Mp3ComponentReturnOutputBuffer(ipMp3OutputBuffer, pOutPort);
        }

        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if (((iInputCurrLength != 0 || GetQueueNumElem(pInputQueue) > 0)
                && (GetQueueNumElem(pOutputQueue) > 0) && (ResizeNeeded == OMX_FALSE))
                || (OMX_TRUE == iEndofStream))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3DecodeWithMarker OUT"));
    return;
}


void OpenmaxMp3AO::Mp3ComponentReturnInputBuffer(OMX_BUFFERHEADERTYPE* pMp3InputBuffer, Mp3ComponentPortType *pPort)
{
    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    if (iNumInputBuffer)
    {
        iNumInputBuffer--;
    }

    //Callback for releasing the input buffer
    (*(ipCallbacks->EmptyBufferDone))
    (pHandle, iCallbackData, pMp3InputBuffer);

    pMp3InputBuffer = NULL;

}

/**
 * Returns Output Buffer back to the IL client
 */
void OpenmaxMp3AO::Mp3ComponentReturnOutputBuffer(OMX_BUFFERHEADERTYPE* pMp3OutputBuffer,
        Mp3ComponentPortType *pPort)
{
    OMX_COMPONENTTYPE* pHandle = &iOmxComponent;

    //Callback for sending back the output buffer
    (*(ipCallbacks->FillBufferDone))
    (pHandle, iCallbackData, pMp3OutputBuffer);

    if (iOutBufferCount)
    {
        iOutBufferCount--;
    }

    pPort->NumBufferFlushed++;
    iNewOutBufRequired = OMX_TRUE;
}

/** The panic function that exits from the application.
 */
OMX_S32 OpenmaxMp3AO::Mp3ComponentPanic()
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

OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentFlushPort(OMX_S32 PortIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentFlushPort IN"));

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

        if (iNumInputBuffer > 0 && ipMp3InputBuffer && (OMX_FALSE == iIsInputBufferEnded))
        {
            ipMp3InputBuffer->nFilledLen = 0;
            (*(ipCallbacks->EmptyBufferDone))
            (pHandle, iCallbackData, ipMp3InputBuffer);
            iNumInputBuffer--;

            iIsInputBufferEnded = OMX_TRUE;
            iInputCurrLength = 0;
            ipMp3Dec->iInputUsedLength = 0;
        }

    }

    if (OMX_PORT_OUTPUTPORT_INDEX == PortIndex || OMX_PORT_ALLPORT_INDEX == PortIndex)
    {
        if ((OMX_FALSE == iNewOutBufRequired) && (iOutBufferCount > 0))
        {
            if (ipMp3OutputBuffer)
            {
                (*(ipCallbacks->FillBufferDone))
                (pHandle, iCallbackData, ipMp3OutputBuffer);
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

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentFlushPort OUT"));
    return OMX_ErrorNone;
}


/** This function is called by the omx core when the component
	* is disposed by the IL client with a call to FreeHandle().
	* \param Component, the component to be disposed
	*/

OMX_ERRORTYPE OpenmaxMp3AO::DestroyComponent()
{
    OMX_U32 ii;

    if (iIsInit != OMX_FALSE)
    {
        Mp3ComponentDeInit();
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
    }

    if (ipTempInputBuffer)
    {
        oscl_free(ipTempInputBuffer);
        ipTempInputBuffer = NULL;
    }

    if (ipMp3Dec)
    {
        OSCL_DELETE(ipMp3Dec);
        ipMp3Dec = NULL;
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
        ipAppPriv->Mp3Handle = NULL;

        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    return OMX_ErrorNone;
}


/**
 * Disable Single Port
 */
void OpenmaxMp3AO::Mp3ComponentDisableSinglePort(OMX_U32 PortIndex)
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
OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentDisablePort(OMX_S32 PortIndex)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDisablePort IN"));
    OMX_U32 ii;

    if (-1 == PortIndex)
    {
        for (ii = 0; ii < iNumPorts; ii++)
        {
            ipPorts[ii]->IsPortFlushed = OMX_TRUE;
        }

        /*Flush all ports*/
        Mp3ComponentFlushPort(PortIndex);

        for (ii = 0; ii < iNumPorts; ii++)
        {
            ipPorts[ii]->IsPortFlushed = OMX_FALSE;
        }
    }
    else
    {
        /*Flush the port specified*/
        ipPorts[PortIndex]->IsPortFlushed = OMX_TRUE;
        Mp3ComponentFlushPort(PortIndex);
        ipPorts[PortIndex]->IsPortFlushed = OMX_FALSE;
    }

    /*Disable ports*/
    if (PortIndex != -1)
    {
        Mp3ComponentDisableSinglePort(PortIndex);
    }
    else
    {
        for (ii = 0; ii < iNumPorts; ii++)
        {
            Mp3ComponentDisableSinglePort(ii);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDisablePort OUT"));

    return OMX_ErrorNone;
}

/**
 * Enable Single Port
 */
void OpenmaxMp3AO::Mp3ComponentEnableSinglePort(OMX_U32 PortIndex)
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
OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentEnablePort(OMX_S32 PortIndex)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentEnablePort IN"));

    OMX_U32 ii;

    /*Enable port/s*/
    if (PortIndex != -1)
    {
        Mp3ComponentEnableSinglePort(PortIndex);
    }
    else
    {
        for (ii = 0; ii < iNumPorts; ii++)
        {
            Mp3ComponentEnableSinglePort(ii);
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentEnablePort OUT"));
    return OMX_ErrorNone;
}

//Not implemented & supported in case of base profile components

void OpenmaxMp3AO::Mp3ComponentGetRolesOfComponent(OMX_STRING* aRoleString)
{
    *aRoleString = "audio_decoder.mp3";
}



OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN  OMX_U32 nPort,
    OMX_IN  OMX_HANDLETYPE hTunneledComp,
    OMX_IN  OMX_U32 nTunneledPort,
    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentGetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentSetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentGetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

    pOpenmaxAOType->GetState(pState);

    return OMX_ErrorNone;
}

void OpenmaxMp3AO::GetState(OMX_OUT OMX_STATETYPE* pState)
{
    *pState = iState;
}



//Active object constructor
OpenmaxMp3AO::OpenmaxMp3AO() :
        OsclActiveObject(OsclActiveObject::EPriorityNominal, "OMXMp3Dec")
{

    iLogger = PVLogger::GetLoggerObject("PVMFOMXAudioDecNode");
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : constructed"));

    //Flag to call BufferMgmtFunction in the Run() when the component state is executing
    iBufferExecuteFlag = OMX_FALSE;
    ipAppPriv = NULL;

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
    iNewOutBufRequired = OMX_TRUE;

    iTempConsumedLength = 0;
    iOutBufferCount = 0;
    iCodecReady = OMX_FALSE;
    ipInputCurrBuffer = NULL;
    iInputCurrLength = 0;
    iFrameCount = 0;
    iStateTransitionFlag = OMX_FALSE;
    iFlushPortFlag = OMX_FALSE;
    iEndOfFrameFlag = OMX_FALSE;
    ipMp3InputBuffer = NULL;
    ipMp3OutputBuffer = NULL;


    iFirstFragment = OMX_FALSE;
    iResizePending = OMX_FALSE;
    iFrameTimestamp = 0;
    iSilenceInsertionInProgress = OMX_FALSE;

    iNumPorts = 0;
    ipPorts = NULL;

    //Indicate whether component has been already initialized */
    iIsInit = OMX_FALSE;

    iGroupPriority = 0;
    iGroupID = 0;

    ipMark = NULL;

    ipMp3Dec = NULL;

    if (!IsAdded())
    {
        AddToScheduler();
    }
}


//Active object destructor
OpenmaxMp3AO::~OpenmaxMp3AO()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : destructed"));
}


/** The Initialization function
 */
OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentInit IN"));

    OMX_BOOL Status;

    if (OMX_TRUE == iIsInit)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentInit error incorrect operation"));
        return OMX_ErrorIncorrectStateOperation;
    }
    iIsInit = OMX_TRUE;

    //Mp3 lib init
    if (!iCodecReady)
    {
        Status = ipMp3Dec->Mp3DecInit(&(ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioEqualizerType));
        iCodecReady = OMX_TRUE;
    }

    iInputCurrLength = 0;
    //Used in dynamic port reconfiguration
    iFrameCount = 0;

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentInit OUT"));

    if (OMX_TRUE == Status)
    {
        return OMX_ErrorNone;
    }
    else
    {
        return OMX_ErrorInvalidComponent;
    }
}

/** This function is called upon a transition to the idle or invalid state.
 *  Also it is called by the Mp3ComponentDestructor() function
 */
OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentDeInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDeInit IN"));

    iIsInit = OMX_FALSE;

    if (iCodecReady)
    {
        ipMp3Dec->Mp3DecDeinit();
        iCodecReady = OMX_FALSE;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDeInit OUT"));

    return OMX_ErrorNone;

}



OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentGetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)
{

    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
    return Status;

}


OMX_ERRORTYPE OpenmaxMp3AO::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR ComponentParameterStructure)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter IN"));

    OMX_PRIORITYMGMTTYPE* pPrioMgmt;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pBufSupply;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_PORT_PARAM_TYPE* pPortDomains;
    OMX_U32 PortIndex;

    OMX_AUDIO_PARAM_PORTFORMATTYPE* pAudioPortFormat;
    OMX_AUDIO_PARAM_PCMMODETYPE* pAudioPcmMode;
    OMX_AUDIO_PARAM_MP3TYPE* pAudioMp3;
    OMX_AUDIO_CONFIG_EQUALIZERTYPE* pAudioEqualizer;

    Mp3ComponentPortType* pComponentPort;

    if (NULL == ComponentParameterStructure)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error bad parameter"));
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

        case OMX_IndexParamAudioInit:
        {
            SetHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
            oscl_memcpy(ComponentParameterStructure, &iPortTypesParam, sizeof(OMX_PORT_PARAM_TYPE));
        }
        break;


        //Following 3 cases have a single common piece of code to be executed
        case OMX_IndexParamVideoInit:
        case OMX_IndexParamImageInit:
        case OMX_IndexParamOtherInit:
        {
            pPortDomains = (OMX_PORT_PARAM_TYPE*) ComponentParameterStructure;
            SetHeader(pPortDomains, sizeof(OMX_PORT_PARAM_TYPE));
            pPortDomains->nPorts = 0;
            pPortDomains->nStartPortNumber = 0;
        }
        break;

        case OMX_IndexParamAudioPortFormat:
        {
            pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*) ComponentParameterStructure;
            //Added to pass parameter test
            if (pAudioPortFormat->nIndex > ipPorts[pAudioPortFormat->nPortIndex]->AudioParam.nIndex)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error index out of range"));
                return OMX_ErrorNoMore;
            }
            SetHeader(pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            if (pAudioPortFormat->nPortIndex <= 1)
            {
                pComponentPort = (Mp3ComponentPortType*) ipPorts[pAudioPortFormat->nPortIndex];
                oscl_memcpy(pAudioPortFormat, &pComponentPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamAudioPcm:
        {
            pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*) ComponentParameterStructure;
            if (pAudioPcmMode->nPortIndex > 1)
            {
                return OMX_ErrorBadPortIndex;
            }
            PortIndex = pAudioPcmMode->nPortIndex;
            oscl_memcpy(pAudioPcmMode, &ipPorts[PortIndex]->AudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
            SetHeader(pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        break;

        case OMX_IndexParamAudioMp3:
        {
            pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE*) ComponentParameterStructure;
            if (pAudioMp3->nPortIndex != 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            PortIndex = pAudioMp3->nPortIndex;
            oscl_memcpy(pAudioMp3, &ipPorts[PortIndex]->AudioMp3Param, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
            SetHeader(pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
        }
        break;

        case OMX_IndexConfigAudioEqualizer:
        {
            pAudioEqualizer = (OMX_AUDIO_CONFIG_EQUALIZERTYPE*) ComponentParameterStructure;
            if (pAudioEqualizer->nPortIndex != 0)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            PortIndex = pAudioEqualizer->nPortIndex;
            oscl_memcpy(pAudioEqualizer, &ipPorts[PortIndex]->AudioEqualizerType, sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
            SetHeader(pAudioEqualizer, sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
        }
        break;

        case OMX_IndexParamPortDefinition:
        {
            pPortDef  = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
            PortIndex = pPortDef->nPortIndex;
            if (PortIndex >= iNumPorts)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error bad port index"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error bad port index"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error pCap_flags NULL"));
                return OMX_ErrorBadParameter;
            }
            oscl_memcpy(pCap_flags, &iPVCapabilityFlags, sizeof(iPVCapabilityFlags));

        }
        break;

        default:
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter error Unsupported Index"));
            return OMX_ErrorUnsupportedIndex;
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : GetParameter OUT"));

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentSetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)
{

    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->SetParameter(hComponent, nParamIndex, ComponentParameterStructure);

    return Status;
}


OMX_ERRORTYPE OpenmaxMp3AO::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR ComponentParameterStructure)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter IN"));

    OMX_PRIORITYMGMTTYPE* pPrioMgmt;
    OMX_AUDIO_PARAM_PORTFORMATTYPE* pAudioPortFormat;
    OMX_AUDIO_PARAM_PCMMODETYPE* pAudioPcmMode;
    OMX_AUDIO_PARAM_MP3TYPE* pAudioMp3;
    OMX_PARAM_BUFFERSUPPLIERTYPE* pBufSupply;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef ;
    OMX_PARAM_COMPONENTROLETYPE* pCompRole;
    Mp3ComponentPortType* pComponentPort;

    OMX_U32 PortIndex;
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;
    OMX_AUDIO_CONFIG_EQUALIZERTYPE* pAudioEqualizer;


    if (NULL == ComponentParameterStructure)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error bad parameter"));
        return OMX_ErrorBadParameter;
    }

    switch (nParamIndex)
    {
        case OMX_IndexParamAudioInit:
        {
            /*Check Structure Header*/
            CheckHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error audio init failed"));
                return ErrorType;
            }
            oscl_memcpy(&iPortTypesParam, ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
        }
        break;

        case OMX_IndexParamAudioPortFormat:
        {
            pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*) ComponentParameterStructure;
            PortIndex = pAudioPortFormat->nPortIndex;
            /*Check Structure Header and verify component state*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }
            if (PortIndex <= 1)
            {
                pComponentPort = (Mp3ComponentPortType*) ipPorts[PortIndex];
                oscl_memcpy(&pComponentPort->AudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamAudioPcm:
        {
            pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*) ComponentParameterStructure;
            PortIndex = pAudioPcmMode->nPortIndex;
            /*Check Structure Header and verify component State*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
            oscl_memcpy(&ipPorts[PortIndex]->AudioPcmMode, pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        }
        break;

        case OMX_IndexParamAudioMp3:
        {
            pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE*) ComponentParameterStructure;
            PortIndex = pAudioMp3->nPortIndex;
            /*Check Structure Header and verify component state*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }
            oscl_memcpy(&ipPorts[PortIndex]->AudioMp3Param, pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
        }
        break;

        case OMX_IndexConfigAudioEqualizer:
        {
            pAudioEqualizer = (OMX_AUDIO_CONFIG_EQUALIZERTYPE*) ComponentParameterStructure;
            PortIndex = pAudioEqualizer->nPortIndex;
            /*Check Structure Header and verify component state*/
            ErrorType = BaseComponentParameterSanityCheck(hComponent, PortIndex, pAudioEqualizer, sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
            if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }
            oscl_memcpy(&ipPorts[PortIndex]->AudioEqualizerType, pAudioEqualizer, sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
        }
        break;

        case OMX_IndexParamPriorityMgmt:
        {
            if (iState != OMX_StateLoaded && iState != OMX_StateWaitForResources)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error incorrect state error"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error parameter sanity check error"));
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
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error incorrect state error"));
                    return OMX_ErrorIncorrectStateOperation;
                }
            }
            else if (ErrorType != OMX_ErrorNone)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error parameter sanity check error"));
                return ErrorType;
            }

            if (pBufSupply->eBufferSupplier == OMX_BufferSupplyUnspecified)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter OUT"));
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
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter error bad parameter"));
            return OMX_ErrorBadParameter;
        }
        break;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetParameter OUT"));
    return ErrorType;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->UseBuffer(hComponent, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);

    return Status;
}


OMX_ERRORTYPE OpenmaxMp3AO::UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : UseBuffer IN"));
    Mp3ComponentPortType* pBaseComponentPort;
    OMX_U32 ii;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : UseBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : UseBuffer error incorrect state"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : UseBuffer error insufficient resources"));
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
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : UseBuffer OUT"));
            return OMX_ErrorNone;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : UseBuffer OUT"));
    return OMX_ErrorNone;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->AllocateBuffer(hComponent, pBuffer, nPortIndex, pAppPrivate, nSizeBytes);

    return Status;
}


OMX_ERRORTYPE OpenmaxMp3AO::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer IN"));

    Mp3ComponentPortType* pBaseComponentPort;
    OMX_U32 ii;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pBaseComponentPort = ipPorts[nPortIndex];

    if (pBaseComponentPort->TransientState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer error incorrect state"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }
            SetHeader(pBaseComponentPort->pBuffer[ii], sizeof(OMX_BUFFERHEADERTYPE));
            /* allocate the buffer */
            pBaseComponentPort->pBuffer[ii]->pBuffer = (OMX_BYTE) oscl_malloc(nSizeBytes);
            if (NULL == pBaseComponentPort->pBuffer[ii]->pBuffer)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer error insufficient resources"));
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

            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer OUT"));
            return OMX_ErrorNone;
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : AllocateBuffer OUT"));
    return OMX_ErrorInsufficientResources;
}

OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FreeBuffer(hComponent, nPortIndex, pBuffer);

    return Status;
}


OMX_ERRORTYPE OpenmaxMp3AO::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FreeBuffer IN"));

    Mp3ComponentPortType* pBaseComponentPort;

    OMX_U32 ii;
    OMX_BOOL FoundBuffer;

    if (nPortIndex >= iNumPorts)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FreeBuffer error bad port index"));
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

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FreeBuffer OUT"));
    return OMX_ErrorNone;
}


/** Set Callbacks. It stores in the component private structure the pointers to the user application callbacs
	* @param hComponent the handle of the component
	* @param ipCallbacks the OpenMAX standard structure that holds the callback pointers
	* @param pAppData a pointer to a private structure, not covered by OpenMAX standard, in needed
    */

OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentSetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->SetCallbacks(hComponent, pCallbacks, pAppData);

    return Status;
}

OMX_ERRORTYPE OpenmaxMp3AO::SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SetCallbacks"));
    ipCallbacks = pCallbacks;
    iCallbackData = pAppData;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentSendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam,
    OMX_IN  OMX_PTR pCmdData)
{

    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->SendCommand(hComponent, Cmd, nParam, pCmdData);

    return Status;
}

OMX_ERRORTYPE OpenmaxMp3AO::SendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_S32 nParam,
    OMX_IN  OMX_PTR pCmdData)
{

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand IN"));

    OMX_U32 ii;
    OMX_ERRORTYPE ErrMsgHandler = OMX_ErrorNone;
    QueueType* pMessageQueue;
    CoreMessage* Message;

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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }

            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            Message->MessageType = SENDCOMMAND_MSG_TYPE;
            Message->MessageParam1 = OMX_CommandStateSet;
            Message->MessageParam2 = nParam;
            Message->pCmdData = pCmdData;

            if ((OMX_StateIdle == nParam) && (OMX_StateLoaded == iState))
            {
                ErrMsgHandler = Mp3ComponentInit();

                if (ErrMsgHandler != OMX_ErrorNone)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error component init"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error insufficient resources"));
                return OMX_ErrorInsufficientResources;
            }

            Message->pComponent = (OMX_COMPONENTTYPE *) hComponent;
            Message->MessageType = SENDCOMMAND_MSG_TYPE;
            Message->MessageParam1 = OMX_CommandFlush;
            Message->MessageParam2 = nParam;
            Message->pCmdData = pCmdData;

            if ((iState != OMX_StateExecuting) && (iState != OMX_StatePause))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error incorrect state"));
                ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                break;

            }
            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }
            //Assume that reposition command has come
            iRepositionFlag = OMX_TRUE;
            //Reset the silence insertion logic also
            iSilenceInsertionInProgress = OMX_FALSE;
            // reset decoder
            if (ipMp3Dec)
            {
                ipMp3Dec->ResetDecoder();
            }

            Mp3ComponentSetPortFlushFlag(iNumPorts, nParam, OMX_TRUE);
            Mp3ComponentSetNumBufferFlush(iNumPorts, -1, 0);
        }
        break;

        case OMX_CommandPortDisable:
        {
            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }

            iResizePending = OMX_FALSE;// disable the flag to enable further processing

            if (-1 == nParam)
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (!PORT_IS_ENABLED(ipPorts[ii]))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error incorrect state"));
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
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error incorrect state"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error insufficient resources"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }

            if (-1 == nParam)
            {
                for (ii = 0; ii < iNumPorts; ii++)
                {
                    if (PORT_IS_ENABLED(ipPorts[ii]))
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error incorrect state"));
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
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error incorrect state"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error insufficient resources"));
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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error incorrect state"));
                ErrMsgHandler = OMX_ErrorIncorrectStateOperation;
                break;
            }

            if ((nParam != -1) && ((OMX_U32) nParam >= iNumPorts))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error bad port index"));
                return OMX_ErrorBadPortIndex;
            }

            Message = (CoreMessage*) oscl_malloc(sizeof(CoreMessage));
            if (NULL == Message)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error insufficient resources"));
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
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand error unsupported index"));
            ErrMsgHandler = OMX_ErrorUnsupportedIndex;
        }
        break;
    }

    Queue(pMessageQueue, Message);
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : SendCommand OUT"));
    return ErrMsgHandler;
}


/** This is called by the OMX core in its message processing
 * thread context upon a component request. A request is made
 * by the component when some asynchronous services are needed:
 * 1) A SendCommand() is to be processed
 * 2) An error needs to be notified
 * \param Message, the message that has been passed to core
 */

OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentMessageHandler(CoreMessage* Message)
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
                ErrorType = Mp3ComponentDoStateSet(Message->MessageParam2);

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
                ErrorType = Mp3ComponentFlushPort(Message->MessageParam2);

                Mp3ComponentSetNumBufferFlush(iNumPorts, -1, 0);

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
                Mp3ComponentSetPortFlushFlag(iNumPorts, -1, OMX_FALSE);
            }
            break;

            case OMX_CommandPortDisable:
            {
                /** This condition is added to pass the tests, it is not significant for the environment */
                ErrorType = Mp3ComponentDisablePort(Message->MessageParam2);
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
                ErrorType = Mp3ComponentEnablePort(Message->MessageParam2);
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
 * the transiotion requested
 * \param Component, the component which state is to be changed
 * \param aDestinationState the requested target state.
 */

OMX_ERRORTYPE OpenmaxMp3AO::Mp3ComponentDoStateSet(OMX_U32 aDestinationState)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE,
                    (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet IN : iState (%i) aDestinationState (%i)", iState, aDestinationState));

    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;
    OMX_U32 ii;

    if (OMX_StateLoaded == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }

            case OMX_StateWaitForResources:
            {
                iState = OMX_StateLoaded;
            }
            break;

            case OMX_StateLoaded:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            break;

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
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet Waiting port to be de-populated"));
                    return OMX_ErrorNone;
                }

                iState = OMX_StateLoaded;

                iNumInputBuffer = 0;
                iOutBufferCount = 0;
                iPartialFrameAssembly = OMX_FALSE;
                iEndofStream = OMX_FALSE;
                iIsInputBufferEnded = OMX_TRUE;
                iNewOutBufRequired = OMX_TRUE;
                iFirstFragment = OMX_FALSE;
                ipMp3Dec->iInitFlag = 0;

                Mp3ComponentDeInit();
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateWaitForResources == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            break;

            case OMX_StateLoaded:
            {
                iState = OMX_StateWaitForResources;
            }
            break;

            case OMX_StateWaitForResources:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateIdle == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            break;

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
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet Waiting port to be populated"));
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
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error insufficient resources"));
                        return OMX_ErrorInsufficientResources;
                    }
                }

                if (!ipTempInputBuffer)
                {
                    ipTempInputBuffer = (OMX_U8*) oscl_malloc(2 * sizeof(uint8) * ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize);
                    if (NULL == ipTempInputBuffer)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error insufficient resources"));
                        return OMX_ErrorInsufficientResources;
                    }
                }

                iTempInputBufferLength = 0;
                iTempConsumedLength = 0;
            }
            break;

            case OMX_StateIdle:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            break;

            //Both the below cases have same body
            case OMX_StateExecuting:
            case OMX_StatePause:
            {
                Mp3ComponentSetNumBufferFlush(iNumPorts, -1, 0);
                Mp3ComponentSetPortFlushFlag(iNumPorts, -1, OMX_TRUE);

                Mp3ComponentPortType* pInPort = (Mp3ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];

                //Return all the buffers if still occupied
                QueueType *pInputQueue = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;

                while ((iNumInputBuffer > 0) && (GetQueueNumElem(pInputQueue) > 0))
                {
                    Mp3ComponentFlushPort(OMX_PORT_INPUTPORT_INDEX);
                }
                // if a buffer was previously dequeued, it wasnt freed in above loop. return it now
                if (iNumInputBuffer > 0)
                {
                    ipMp3InputBuffer->nFilledLen = 0;
                    Mp3ComponentReturnInputBuffer(ipMp3InputBuffer, pInPort);
                    iIsInputBufferEnded = OMX_TRUE;
                    iInputCurrLength = 0;
                }

                //Mark these flags as true
                iIsInputBufferEnded = OMX_TRUE;
                iEndofStream = OMX_FALSE;

                //Added these statement here
                iTempInputBufferLength = 0;
                iTempConsumedLength = 0;
                //Assume for this state transition that reposition command has come
                iRepositionFlag = OMX_TRUE;
                //Reset the silence insertion logic also
                iSilenceInsertionInProgress = OMX_FALSE;
                // reset decoder
                if (ipMp3Dec)
                {
                    ipMp3Dec->ResetDecoder();
                }

                while (iOutBufferCount > 0)
                {
                    Mp3ComponentFlushPort(OMX_PORT_OUTPUTPORT_INDEX);
                }

                Mp3ComponentSetPortFlushFlag(iNumPorts, -1, OMX_FALSE);
                Mp3ComponentSetNumBufferFlush(iNumPorts, -1, 0);

                iState = OMX_StateIdle;
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
            break;
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet OUT"));
        return ErrorType;
    }

    if (OMX_StatePause == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            break;

            case OMX_StatePause:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            break;

            //Falling through to the next case
            case OMX_StateExecuting:
            case OMX_StateIdle:
            {
                iState = OMX_StatePause;
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
            break;
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateExecuting == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            break;

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
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error same state"));
                return OMX_ErrorSameState;
            }
            break;

            default:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error incorrect state"));
                return OMX_ErrorIncorrectStateTransition;
            }
            break;
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet OUT"));
        return OMX_ErrorNone;
    }

    if (OMX_StateInvalid == aDestinationState)
    {
        switch (iState)
        {
            case OMX_StateInvalid:
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
                return OMX_ErrorInvalidState;
            }
            break;

            default:
            {
                iState = OMX_StateInvalid;
                if (iIsInit != OMX_FALSE)
                {
                    Mp3ComponentDeInit();
                }
            }
            break;
        }

        if (iIsInit != OMX_FALSE)
        {
            Mp3ComponentDeInit();
        }

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet error invalid state"));
        return OMX_ErrorInvalidState;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Mp3ComponentDoStateSet OUT"));
    return OMX_ErrorNone;
}



OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentEmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{

    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->EmptyThisBuffer(hComponent, pBuffer);

    return Status;

}


OMX_ERRORTYPE OpenmaxMp3AO::EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer IN"));
    //Do not queue buffers if component is in invalid state
    if (OMX_StateInvalid == iState)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer error invalid state"));
        return OMX_ErrorInvalidState;
    }

    if ((OMX_StateIdle == iState) || (OMX_StatePause == iState) || (OMX_StateExecuting == iState))
    {
        OMX_U32 PortIndex;
        QueueType* pInputQueue;
        OMX_ERRORTYPE ErrorType = OMX_ErrorNone;

        PortIndex = pBuffer->nInputPortIndex;

        //Validate the port index & Queue the buffers available only at the input port
        if (PortIndex >= iNumPorts ||
                ipPorts[PortIndex]->PortParam.eDir != OMX_DirInput)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer error bad port index"));
            return OMX_ErrorBadPortIndex;
        }

        //Port should be in enabled state before accepting buffers
        if (!PORT_IS_ENABLED(ipPorts[PortIndex]))
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer error incorrect state"));
            return OMX_ErrorIncorrectStateOperation;
        }

        /* The number of buffers the component can queue at a time
         * depends upon the number of buffers allocated/assigned on the input port
         */
        if (iNumInputBuffer == (ipPorts[PortIndex]->NumAssignedBuffers))

        {
            RunIfNotReady();
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer error incorrect state"));
            return OMX_ErrorIncorrectStateOperation;
        }

        //Finally after passing all the conditions, queue the buffer in Input queue
        pInputQueue = ipPorts[PortIndex]->pBufferQueue;

        if ((ErrorType = CheckHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone)
        {
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer error check header failed"));
            return ErrorType;
        }

        iNumInputBuffer++;
        Queue(pInputQueue, pBuffer);

        //Signal the AO about the incoming buffer
        RunIfNotReady();
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer error incorrect state"));
        //This macro is not accepted in any other state except the three mentioned above
        return OMX_ErrorIncorrectStateOperation;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : EmptyThisBuffer OUT"));

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OpenmaxMp3AO::BaseComponentFillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{

    OpenmaxMp3AO* pOpenmaxAOType = (OpenmaxMp3AO*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    OMX_ERRORTYPE Status;

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorBadParameter;
    }

    Status = pOpenmaxAOType->FillThisBuffer(hComponent, pBuffer);

    return Status;
}

OMX_ERRORTYPE OpenmaxMp3AO::FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)

{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FillThisBuffer IN"));

    OMX_U32 PortIndex;

    QueueType* pOutputQueue;
    OMX_ERRORTYPE ErrorType = OMX_ErrorNone;

    PortIndex = pBuffer->nOutputPortIndex;
    //Validate the port index & Queue the buffers available only at the output port
    if (PortIndex >= iNumPorts ||
            ipPorts[PortIndex]->PortParam.eDir != OMX_DirOutput)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FillThisBuffer error bad port index"));
        return OMX_ErrorBadPortIndex;
    }

    pOutputQueue = ipPorts[PortIndex]->pBufferQueue;
    if (iState != OMX_StateExecuting &&
            iState != OMX_StatePause &&
            iState != OMX_StateIdle)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FillThisBuffer error invalid state"));
        return OMX_ErrorInvalidState;
    }

    //Port should be in enabled state before accepting buffers
    if (!PORT_IS_ENABLED(ipPorts[PortIndex]))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FillThisBuffer error incorrect state"));
        return OMX_ErrorIncorrectStateOperation;
    }

    if ((ErrorType = CheckHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FillThisBuffer error check header failed"));
        return ErrorType;
    }

    //Queue the buffer in output queue
    Queue(pOutputQueue, pBuffer);
    iOutBufferCount++;

    //Signal the AO about the incoming buffer
    RunIfNotReady();

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : FillThisBuffer OUT"));

    return OMX_ErrorNone;
}



void OpenmaxMp3AO::Run()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Run IN"));


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

        Mp3ComponentMessageHandler(pCoreMessage);

        /* If some allocations/deallocations are required before the state transition
         * then queue the command again to be executed later on
         */
        if (OMX_TRUE == iStateTransitionFlag)
        {
            Queue(ipCoreDescriptor->pMessageQueue, pCoreMessage);
            // Don't reschedule. Wait for arriving buffers to do it
            PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Run OUT"));
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
        Mp3ComponentBufferMgmtFunction();
    }

    //Check for any more commands in the message handler queue & schedule them for later
    if ((GetQueueNumElem(ipCoreDescriptor->pMessageQueue) > 0))
    {
        RunIfNotReady();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : Run OUT"));

    return;
}

//Check whether silence insertion is required here or not
void OpenmaxMp3AO::CheckForSilenceInsertion()
{

    OMX_TICKS CurrTimestamp, TimestampGap;
    //Set the flag to false by default
    iSilenceInsertionInProgress = OMX_FALSE;

    CurrTimestamp = iCurrentFrameTS.GetCurrentTimestamp();
    TimestampGap = iFrameTimestamp - CurrTimestamp;

    if ((TimestampGap > OMX_HALFRANGE_THRESHOLD) || (TimestampGap < iOutputMilliSecPerFrame && iFrameCount > 0))
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : CheckForSilenceInsertion OUT - No need to insert silence"));
        return;
    }

    //Silence insertion needed, mark the flag to true
    if (iFrameCount > 0)
    {
        iSilenceInsertionInProgress = OMX_TRUE;
        //Determine the number of silence frames to insert
        if (0 != iOutputMilliSecPerFrame)
        {
            iSilenceFramesNeeded = TimestampGap / iOutputMilliSecPerFrame;
        }
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : CheckForSilenceInsertion OUT - Silence Insertion required here"));
    }

    return;
}

//Currently we are doing zero frame insertion in this routine
void OpenmaxMp3AO::DoSilenceInsertion()
{
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;
    Mp3ComponentPortType* pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_U8*	pOutBuffer = NULL;


    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : DoSilenceInsertion IN"));

    while (iSilenceFramesNeeded > 0)
    {
        //Check whether prev output bufer has been consumed or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not
            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                //Resume Silence insertion next time when component will be called
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : DoSilenceInsertion OUT output buffer unavailable"));
                iSilenceInsertionInProgress = OMX_TRUE;
                return;
            }

            ipMp3OutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);
            ipMp3OutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;

            //Set the current timestamp to the output buffer timestamp
            ipMp3OutputBuffer->nTimeStamp = iCurrentFrameTS.GetConvertedTs();
        }


        pOutBuffer = &ipMp3OutputBuffer->pBuffer[ipMp3OutputBuffer->nFilledLen];
        oscl_memset(pOutBuffer, 0, iOutputFrameLength);

        ipMp3OutputBuffer->nFilledLen += iOutputFrameLength;
        ipMp3OutputBuffer->nOffset = 0;
        iCurrentFrameTS.UpdateTimestamp(iSamplesPerFrame);

        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : DoSilenceInsertion - One frame of zeros inserted"));

        //Send the output buffer back when it has become full
        if ((ipMp3OutputBuffer->nAllocLen - ipMp3OutputBuffer->nFilledLen) < iOutputFrameLength)
        {
            Mp3ComponentReturnOutputBuffer(ipMp3OutputBuffer, pOutPort);
        }

        // Decrement the silence frame counter
        --iSilenceFramesNeeded;
    }

    /* Completed Silence insertion successfully, now consider the input buffer already dequeued
     * for decoding & update the timestamps */

    iSilenceInsertionInProgress = OMX_FALSE;
    iCurrentFrameTS.SetFromInputTimestamp(iFrameTimestamp);
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OpenmaxMp3AO : DoSilenceInsertion OUT - Done successfully"));

    return;
}
