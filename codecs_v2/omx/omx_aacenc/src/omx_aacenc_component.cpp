/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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

#include "omx_aacenc_component.h"

#if PROXY_INTERFACE
#include "omx_proxy_interface.h"
#endif



// This function is called by OMX_GetHandle and it creates an instance of the aac component AO
OMX_ERRORTYPE AacEncOmxComponentFactory(OMX_OUT OMX_HANDLETYPE* pHandle, OMX_IN  OMX_PTR pAppData, OMX_PTR pProxy, OMX_STRING aOmxLibName, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLibName);
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    OmxComponentAacEncoderAO* pOpenmaxAOType;
    OMX_ERRORTYPE Status;

    // move InitAacOmxComponentFields content to actual constructor

    pOpenmaxAOType = (OmxComponentAacEncoderAO*) OSCL_NEW(OmxComponentAacEncoderAO, ());

    if (NULL == pOpenmaxAOType)
    {
        return OMX_ErrorInsufficientResources;
    }

    //Call the construct component to initialize OMX types
    Status = pOpenmaxAOType->ConstructComponent(pAppData, pProxy);

    *pHandle = pOpenmaxAOType->GetOmxHandle();

    return Status;
    ///////////////////////////////////////////////////////////////////////////////////////
}

// This function is called by OMX_FreeHandle when component AO needs to be destroyed
OMX_ERRORTYPE AacEncOmxComponentDestructor(OMX_IN OMX_HANDLETYPE pHandle, OMX_PTR &aOmxLib, OMX_PTR aOsclUuid, OMX_U32 &aRefCount)
{
    OSCL_UNUSED_ARG(aOmxLib);
    OSCL_UNUSED_ARG(aOsclUuid);
    OSCL_UNUSED_ARG(aRefCount);

    // get pointer to component AO
    OmxComponentAacEncoderAO* pOpenmaxAOType = (OmxComponentAacEncoderAO*)((OMX_COMPONENTTYPE*)pHandle)->pComponentPrivate;

    // clean up encoder, OMX component stuff
    pOpenmaxAOType->DestroyComponent();

    // destroy the AO class
    OSCL_DELETE(pOpenmaxAOType);

    return OMX_ErrorNone;
}

#if DYNAMIC_LOAD_OMX_AACENC_COMPONENT
class AacEncOmxSharedLibraryInterface:  public OsclSharedLibraryInterface,
            public OmxSharedLibraryInterface

{
    public:
        static AacEncOmxSharedLibraryInterface *Instance()
        {
            static AacEncOmxSharedLibraryInterface omxinterface;
            return &omxinterface;
        };

        OsclAny *QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId)
        {
            if (PV_OMX_AACENC_UUID == aOmxTypeId)
            {
                if (PV_OMX_CREATE_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&AacEncOmxComponentFactory));
                }
                else if (PV_OMX_DESTROY_INTERFACE == aInterfaceId)
                {
                    return ((OsclAny*)(&AacEncOmxComponentDestructor));
                }
            }
            return NULL;
        };

        OsclAny *SharedLibraryLookup(const OsclUuid& aInterfaceId)
        {
            if (aInterfaceId == PV_OMX_SHARED_INTERFACE)
            {
                return OSCL_STATIC_CAST(OmxSharedLibraryInterface*, this);
            }
            return NULL;
        };

    private:
        AacEncOmxSharedLibraryInterface() {};
};

// function to obtain the interface object from the shared library
extern "C"
{
    OSCL_EXPORT_REF OsclAny* PVGetInterface()
    {
        return AacEncOmxSharedLibraryInterface::Instance();
    }
}

#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////

OMX_ERRORTYPE OmxComponentAacEncoderAO::ConstructComponent(OMX_PTR pAppData, OMX_PTR pProxy)
{
    ComponentPortType* pInPort, *pOutPort;
    OMX_ERRORTYPE Status;

    iNumPorts = 2;
    iCompressedFormatPortNum = OMX_PORT_OUTPUTPORT_INDEX;
    iOmxComponent.nSize = sizeof(OMX_COMPONENTTYPE);
    iOmxComponent.pComponentPrivate = (OMX_PTR) this;  // pComponentPrivate points to THIS component AO class
    ipComponentProxy = pProxy;
    iOmxComponent.pApplicationPrivate = pAppData; // init the App data


#if PROXY_INTERFACE
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_TRUE;

    iOmxComponent.SendCommand = OmxComponentAacEncoderAO::BaseComponentProxySendCommand;
    iOmxComponent.GetParameter = OmxComponentAacEncoderAO::BaseComponentProxyGetParameter;
    iOmxComponent.SetParameter = OmxComponentAacEncoderAO::BaseComponentProxySetParameter;
    iOmxComponent.GetConfig = OmxComponentAacEncoderAO::BaseComponentProxyGetConfig;
    iOmxComponent.SetConfig = OmxComponentAacEncoderAO::BaseComponentProxySetConfig;
    iOmxComponent.GetExtensionIndex = OmxComponentAacEncoderAO::BaseComponentProxyGetExtensionIndex;
    iOmxComponent.GetState = OmxComponentAacEncoderAO::BaseComponentProxyGetState;
    iOmxComponent.UseBuffer = OmxComponentAacEncoderAO::BaseComponentProxyUseBuffer;
    iOmxComponent.AllocateBuffer = OmxComponentAacEncoderAO::BaseComponentProxyAllocateBuffer;
    iOmxComponent.FreeBuffer = OmxComponentAacEncoderAO::BaseComponentProxyFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OmxComponentAacEncoderAO::BaseComponentProxyEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OmxComponentAacEncoderAO::BaseComponentProxyFillThisBuffer;

#else
    iPVCapabilityFlags.iIsOMXComponentMultiThreaded = OMX_FALSE;

    iOmxComponent.SendCommand = OmxComponentAacEncoderAO::BaseComponentSendCommand;
    iOmxComponent.GetParameter = OmxComponentAacEncoderAO::BaseComponentGetParameter;
    iOmxComponent.SetParameter = OmxComponentAacEncoderAO::BaseComponentSetParameter;
    iOmxComponent.GetConfig = OmxComponentAacEncoderAO::BaseComponentGetConfig;
    iOmxComponent.SetConfig = OmxComponentAacEncoderAO::BaseComponentSetConfig;
    iOmxComponent.GetExtensionIndex = OmxComponentAacEncoderAO::BaseComponentGetExtensionIndex;
    iOmxComponent.GetState = OmxComponentAacEncoderAO::BaseComponentGetState;
    iOmxComponent.UseBuffer = OmxComponentAacEncoderAO::BaseComponentUseBuffer;
    iOmxComponent.AllocateBuffer = OmxComponentAacEncoderAO::BaseComponentAllocateBuffer;
    iOmxComponent.FreeBuffer = OmxComponentAacEncoderAO::BaseComponentFreeBuffer;
    iOmxComponent.EmptyThisBuffer = OmxComponentAacEncoderAO::BaseComponentEmptyThisBuffer;
    iOmxComponent.FillThisBuffer = OmxComponentAacEncoderAO::BaseComponentFillThisBuffer;
#endif

    iOmxComponent.SetCallbacks = OmxComponentAacEncoderAO::BaseComponentSetCallbacks;
    iOmxComponent.nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    iOmxComponent.nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    iOmxComponent.nVersion.s.nRevision = SPECREVISION;
    iOmxComponent.nVersion.s.nStep = SPECSTEP;

    // PV capability
    iPVCapabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentSupportsPartialFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentNeedsNALStartCode = OMX_FALSE;
    iPVCapabilityFlags.iOMXComponentCanHandleIncompleteFrames = OMX_TRUE;
    iPVCapabilityFlags.iOMXComponentNeedsFullAVCFrames = OMX_FALSE;

    if (ipAppPriv)
    {
        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    ipAppPriv = (ComponentPrivateType*) oscl_malloc(sizeof(ComponentPrivateType));
    if (NULL == ipAppPriv)
    {
        return OMX_ErrorInsufficientResources;
    }

    //Construct base class now
    Status = ConstructBaseComponent(pAppData);

    if (OMX_ErrorNone != Status)
    {
        return Status;
    }

    /** Domain specific section for the ports */
    /* Input port is raw/pcm for AAC encoder */
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainAudio;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.cMIMEType = (OMX_STRING)"raw";
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.pNativeRender = 0;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.eDir = OMX_DirInput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_INPUT_BUFFER_AAC_ENC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize = INPUT_BUFFER_SIZE_AAC_ENC;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;


    /* Output port is aac format for AAC encoder */
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDomain = OMX_PortDomainAudio;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.cMIMEType = (OMX_STRING)"audio/mpeg";
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.pNativeRender = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.eDir = OMX_DirOutput;
    //Set to a default value, will change later during setparameter call
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountActual = NUMBER_OUTPUT_BUFFER_AAC_ENC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferCountMin = 1;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.nBufferSize = OUTPUT_BUFFER_SIZE_AAC_ENC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bEnabled = OMX_TRUE;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->PortParam.bPopulated = OMX_FALSE;

    //Default values for PCM input audio param port
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.nPortIndex = OMX_PORT_INPUTPORT_INDEX;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.nChannels = 2;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.eNumData = OMX_NumericalDataSigned;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.bInterleaved = OMX_TRUE;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.nBitPerSample = 16;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.nSamplingRate = 44100;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    //Default values for AAC output audio param port
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nPortIndex = OMX_PORT_OUTPUTPORT_INDEX;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nChannels = 2;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nSampleRate = 48000;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nBitRate = 128000;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nAudioBandWidth = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nFrameLength = 0;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nAACtools = OMX_AUDIO_AACToolAll;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nAACERtools = OMX_AUDIO_AACERNone;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.eAACProfile = OMX_AUDIO_AACObjectLC;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;
    ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.eChannelMode = OMX_AUDIO_ChannelModeStereo;


    iPortTypesParam.nPorts = 2;
    iPortTypesParam.nStartPortNumber = 0;

    pInPort = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    pOutPort = (ComponentPortType*) ipPorts[OMX_PORT_OUTPUTPORT_INDEX];

    SetHeader(&pInPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    pInPort->AudioParam.nPortIndex = 0;
    pInPort->AudioParam.nIndex = 0;
    pInPort->AudioParam.eEncoding = OMX_AUDIO_CodingPCM;

    SetHeader(&pOutPort->AudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    pOutPort->AudioParam.nPortIndex = 1;
    pOutPort->AudioParam.nIndex = 0;
    pOutPort->AudioParam.eEncoding = OMX_AUDIO_CodingAAC;

    if (ipAacEnc)
    {
        OSCL_DELETE(ipAacEnc);
        ipAacEnc = NULL;
    }

    ipAacEnc = OSCL_NEW(OmxAacEncoder, ());
    if (NULL == ipAacEnc)
    {
        return OMX_ErrorInsufficientResources;
    }

    iOutputFrameLength = OUTPUTBUFFER_SIZE;


#if PROXY_INTERFACE

    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSendCommand = BaseComponentSendCommand;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetParameter = BaseComponentGetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetParameter = BaseComponentSetParameter;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetConfig = BaseComponentGetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentSetConfig = BaseComponentSetConfig;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetExtensionIndex = BaseComponentGetExtensionIndex;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentGetState = BaseComponentGetState;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentUseBuffer = BaseComponentUseBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentAllocateBuffer = BaseComponentAllocateBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFreeBuffer = BaseComponentFreeBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentEmptyThisBuffer = BaseComponentEmptyThisBuffer;
    ((ProxyApplication_OMX*)ipComponentProxy)->ComponentFillThisBuffer = BaseComponentFillThisBuffer;

#endif
    return OMX_ErrorNone;
}


/** This function is called by the omx core when the component
	* is disposed by the IL client with a call to FreeHandle().
	* \param Component, the component to be disposed
	*/

OMX_ERRORTYPE OmxComponentAacEncoderAO::DestroyComponent()
{
    if (iIsInit != OMX_FALSE)
    {
        ComponentDeInit();
    }

    //Destroy the base class now
    DestroyBaseComponent();

    if (ipAacEnc)
    {
        OSCL_DELETE(ipAacEnc);
        ipAacEnc = NULL;
    }

    if (ipAppPriv)
    {
        ipAppPriv->CompHandle = NULL;

        oscl_free(ipAppPriv);
        ipAppPriv = NULL;
    }

    return OMX_ErrorNone;
}



/* This routine will extract the input timestamp from the input buffer */
void OmxComponentAacEncoderAO::SyncWithInputTimestamp()
{
    // DV: SKIP DOING THIS FOR NOW
    //iCurrentFrameTS.SetFromInputTimestamp(iFrameTimestamp);
}


void OmxComponentAacEncoderAO::ProcessData()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ProcessData IN"));

    QueueType* pInputQueue  = ipPorts[OMX_PORT_INPUTPORT_INDEX]->pBufferQueue;
    QueueType* pOutputQueue = ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->pBufferQueue;

    ComponentPortType* pInPort  = (ComponentPortType*) ipPorts[OMX_PORT_INPUTPORT_INDEX];
    ComponentPortType* pOutPort = ipPorts[OMX_PORT_OUTPUTPORT_INDEX];
    OMX_COMPONENTTYPE* pHandle  = &iOmxComponent;

    OMX_U8*	 pOutBuffer;
    OMX_U32	 OutputLength;
    OMX_S32  EncodeReturn;

    OMX_U32 TempInputBufferSize = (2 * sizeof(uint8) * (ipPorts[OMX_PORT_INPUTPORT_INDEX]->PortParam.nBufferSize));

    if ((!iIsInputBufferEnded) || iEndofStream)
    {
        //Check whether prev output bufer has been released or not
        if (OMX_TRUE == iNewOutBufRequired)
        {
            //Check whether a new output buffer is available or not
            if (0 == (GetQueueNumElem(pOutputQueue)))
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ProcessData OUT output buffer unavailable"));
                return;
            }

            ipOutputBuffer = (OMX_BUFFERHEADERTYPE*) DeQueue(pOutputQueue);

            OSCL_ASSERT(NULL != ipOutputBuffer);
            if (NULL == ipOutputBuffer)
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "OmxComponentAacEncoderAO : ProcessData OUT ERR output buffer cannot be dequeued"));
                return;
            }
            ipOutputBuffer->nFilledLen = 0;
            iNewOutBufRequired = OMX_FALSE;

            //Set the current timestamp to the output buffer timestamp
            ipOutputBuffer->nTimeStamp = iCurrentFrameTS.GetConvertedTs();
        }

        /* Code for the marking buffer. Takes care of the OMX_CommandMarkBuffer
         * command and hMarkTargetComponent as given by the specifications
         */
        if (ipMark != NULL)
        {
            ipOutputBuffer->hMarkTargetComponent = ipMark->hMarkTargetComponent;
            ipOutputBuffer->pMarkData = ipMark->pMarkData;
            ipMark = NULL;
        }

        if (ipTargetComponent != NULL)
        {
            ipOutputBuffer->hMarkTargetComponent = ipTargetComponent;
            ipOutputBuffer->pMarkData = iTargetMarkData;
            ipTargetComponent = NULL;

        }
        //Mark buffer code ends here


        if ((iTempInputBufferLength > 0) &&
                ((iInputCurrLength + iTempInputBufferLength) <= TempInputBufferSize))
        {
            oscl_memcpy(&ipTempInputBuffer[iTempInputBufferLength], ipFrameDecodeBuffer, iInputCurrLength);
            iInputCurrLength += iTempInputBufferLength;
            iTempInputBufferLength = 0;
            ipFrameDecodeBuffer = ipTempInputBuffer;
        }

        if ((iInputCurrLength >= iInputFrameLength) || (OMX_TRUE == iEndofStream))
        {
            pOutBuffer = &ipOutputBuffer->pBuffer[ipOutputBuffer->nFilledLen];
            OutputLength = 0;

            EncodeReturn = ipAacEnc->AacEncodeFrame(pOutBuffer,
                                                    &OutputLength,
                                                    ipFrameDecodeBuffer,
                                                    iInputCurrLength);


            ipOutputBuffer->nFilledLen += OutputLength;
            ipOutputBuffer->nOffset = 0;

            if ((iFrameCount > 0) && (OutputLength > 0))
            {
                iCurrentFrameTS.UpdateTimestamp((iInputFrameLength >> 1));
            }

            iFrameCount++;

            /* If EOS flag has come from the client & there are no more
             * input buffers to decode, send the callback to the client
             */
            if (OMX_TRUE == iEndofStream)
            {
                if ((0 == OutputLength) || (OMX_TRUE != EncodeReturn))
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ProcessData EOS callback send"));

                    (*(ipCallbacks->EventHandler))
                    (pHandle,
                     iCallbackData,
                     OMX_EventBufferFlag,
                     1,
                     OMX_BUFFERFLAG_EOS,
                     NULL);

                    iEndofStream = OMX_FALSE;

                    ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;

                    ReturnOutputBuffer(ipOutputBuffer, pOutPort);
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ProcessData OUT"));

                    return;
                }
            }


            if (OMX_TRUE == EncodeReturn)
            {
                if (iInputCurrLength >= iInputFrameLength)
                {
                    // in case of "Raw" format - the first output frame contains the config data - but
                    // actual input data has not been touched.
                    if ((iFrameCount > 1) || (ipAacEnc->IsRawAACFormatUsed() != OMX_TRUE))
                        iInputCurrLength -= iInputFrameLength;
                }
                else
                {
                    iInputCurrLength = 0;
                }

                if (0 == iInputCurrLength)
                {
                    if (ipInputBuffer)
                    {
                        //Input bytes consumed now, return the buffer
                        ipInputBuffer->nFilledLen = 0;
                        ReturnInputBuffer(ipInputBuffer, pInPort);
                        ipInputBuffer = NULL;
                    }
                    iIsInputBufferEnded = OMX_TRUE;
                }
                else if (iInputCurrLength >= iInputFrameLength)
                {
                    //Do not return the input buffer in case it has more than one frame data to encode
                    if ((iFrameCount > 1) || (ipAacEnc->IsRawAACFormatUsed() != OMX_TRUE))
                        ipFrameDecodeBuffer += iInputFrameLength;
                }
                else
                {
                    /* If there are some remainder bytes out of the last buffer, copy into a temp buffer
                     * to be used in next decode cycle and return the existing input buffer*/
                    oscl_memmove(ipTempInputBuffer, &ipFrameDecodeBuffer[iInputFrameLength], iInputCurrLength);
                    iTempInputBufferLength = iInputCurrLength;

                    if (ipInputBuffer)
                    {
                        ipInputBuffer->nFilledLen = 0;
                        ReturnInputBuffer(ipInputBuffer, pInPort);
                        ipInputBuffer = NULL;
                    }
                    iIsInputBufferEnded = OMX_TRUE;
                    iInputCurrLength = 0;
                }
            }
            //In case of error, discard the bitstream and report data corruption error via callback
            else
            {
                PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ProcessData ErrorStreamCorrupt callback send"));

                if (ipInputBuffer)
                {
                    ipInputBuffer->nFilledLen = 0;
                    ReturnInputBuffer(ipInputBuffer, pInPort);
                    ipInputBuffer = NULL;
                }
                iIsInputBufferEnded = OMX_TRUE;
                iInputCurrLength = 0;

                (*(ipCallbacks->EventHandler))
                (pHandle,
                 iCallbackData,
                 OMX_EventError,
                 OMX_ErrorStreamCorrupt,
                 0,
                 NULL);
            }
        }
        else
        {
            // if we ended up here - the data in the incoming input buffer combined with temp data still
            // wasn't enough in quantity to send for processing - so it wasn't even processed - but we did
            // the necessary memcopies  to save the data
            //oscl_memmove(ipTempInputBuffer, &ipFrameDecodeBuffer[iInputFrameLength], iInputCurrLength);
            iTempInputBufferLength = iInputCurrLength;
            ipTempInputBuffer = ipFrameDecodeBuffer;

            if (ipInputBuffer)
            {
                ipInputBuffer->nFilledLen = 0;
                ReturnInputBuffer(ipInputBuffer, pInPort);
                ipInputBuffer = NULL;
            }
            iIsInputBufferEnded = OMX_TRUE;
            iInputCurrLength = 0;
        }


        /* Send the output buffer back when it has become full*/
        if ((ipOutputBuffer->nAllocLen - ipOutputBuffer->nFilledLen) < (iOutputFrameLength))
        {
            //Attach the end of frame flag while sending out the output buffer
            ipOutputBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            ReturnOutputBuffer(ipOutputBuffer, pOutPort);
        }


        /* If there is some more processing left with current buffers, re-schedule the AO
         * Do not go for more than one round of processing at a time.
         * This may block the AO longer than required.
         */
        if (((iInputCurrLength != 0) || (GetQueueNumElem(pInputQueue) > 0))
                && ((GetQueueNumElem(pOutputQueue) > 0) || (OMX_FALSE == iNewOutBufRequired)))
        {
            RunIfNotReady();
        }
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ProcessData OUT"));
    return;
}


//Not implemented & supported in case of base profile components

void OmxComponentAacEncoderAO::ComponentGetRolesOfComponent(OMX_STRING* aRoleString)
{
    *aRoleString = (OMX_STRING)"audio_encoder.aac";
}


//Component constructor
OmxComponentAacEncoderAO::OmxComponentAacEncoderAO()
{
    ipAacEnc = NULL;
    iInputFrameLength = 0;
    iOutputFrameLength = 0;

    if (!IsAdded())
    {
        AddToScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : constructed"));
}


//Active object destructor
OmxComponentAacEncoderAO::~OmxComponentAacEncoderAO()
{
    if (IsAdded())
    {
        RemoveFromScheduler();
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : destructed"));
}


/** The Initialization function
 */
OMX_ERRORTYPE OmxComponentAacEncoderAO::ComponentInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ComponentInit IN"));

    OMX_BOOL Status = OMX_TRUE;

    if (OMX_TRUE == iIsInit)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ComponentInit error incorrect operation"));
        return OMX_ErrorIncorrectStateOperation;
    }
    iIsInit = OMX_TRUE;

    //aac encoder lib init
    if (!iCodecReady)
    {
        Status = ipAacEnc->AacEncInit(ipPorts[OMX_PORT_INPUTPORT_INDEX]->AudioPcmMode,
                                      ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam,
                                      &iInputFrameLength);

        iCodecReady = OMX_TRUE;

        iCurrentFrameTS.SetParameters(ipPorts[OMX_PORT_OUTPUTPORT_INDEX]->AudioAacParam.nSampleRate, (iInputFrameLength >> 1));
    }

    iInputCurrLength = 0;

    if (OMX_TRUE == Status)
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ComponentInit OUT"));
        return OMX_ErrorNone;
    }
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : Error ComponentInit, OUT"));
        return OMX_ErrorInvalidComponent;
    }
}



/** This function is called upon a transition to the idle or invalid state.
 *  Also it is called by the ComponentDestructor() function
 */
OMX_ERRORTYPE OmxComponentAacEncoderAO::ComponentDeInit()
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ComponentDeInit IN"));

    iIsInit = OMX_FALSE;

    if (iCodecReady)
    {
        ipAacEnc->AacEncDeinit();
        iCodecReady = OMX_FALSE;
    }

    PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_NOTICE, (0, "OmxComponentAacEncoderAO : ComponentDeInit OUT"));

    return OMX_ErrorNone;

}


/* A component specific routine called from BufferMgmtWithoutMarker */
void OmxComponentAacEncoderAO::ProcessInBufferFlag()
{
    iIsInputBufferEnded = OMX_FALSE;
}
