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
	@file omx_mpeg4_component.h
	OpenMax decoder_component component.

*/

#ifndef OMX_MPEG4_COMPONENT_H_INCLUDED
#define OMX_MPEG4_COMPONENT_H_INCLUDED

#ifndef OMX_Component_h
#include "omx_component.h"
#endif

#ifndef MPEG4_DEC_H_INCLUDED
#include "mpeg4_dec.h"
#endif


#ifndef OSCL_SCHEDULER_H_INCLUDED
#include "oscl_scheduler.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef PV_OMX_QUEUE_H_INCLUDED
#include "pv_omx_queue.h"
#endif

#ifndef PV_OMXCORE_H_INCLUDED
#include "pv_omxcore.h"
#endif


#define OMX_PORT_INPUTPORT_INDEX OMX_DirInput
#define OMX_PORT_OUTPUTPORT_INDEX OMX_DirOutput
#define OMX_PORT_ALLPORT_INDEX -1


#define INPUT_BUFFER_SIZE_MP4 64000
#define OUTPUT_BUFFER_SIZE_MP4 152064

#define NUMBER_INPUT_BUFFER_MP4  10
#define NUMBER_OUTPUT_BUFFER_MP4  2


/* Application's private data */
typedef struct Mpeg4PrivateType
{
    OMX_HANDLETYPE Mpeg4Handle;

}Mpeg4PrivateType;


/**
 * This is the Component template from which all
 * other Component instances are factored by the core.
 */

/**
 * The structure for port Type.
 */
enum
{
    MODE_H263 = 0,
    MODE_MPEG4
};

typedef struct Mpeg4ComponentPortType
{
    /** @param pBuffer An array of pointers to buffer headers. */
    OMX_BUFFERHEADERTYPE** pBuffer;
    /** @param BufferState The State of the Buffer whether assigned or allocated */
    OMX_U32* BufferState;
    /** @param NumAssignedBuffers Number of buffer assigned on each port */
    OMX_U32 NumAssignedBuffers;
    /** @param pBufferQueue queue for buffer to be processed by the port */
    QueueType* pBufferQueue;
    OMX_STATETYPE TransientState;
    /** @param BufferUnderProcess  Boolean variables indicate whether the port is processing any buffer */
    OMX_BOOL BufferUnderProcess;
    OMX_PARAM_PORTDEFINITIONTYPE PortParam;
    /** @param NumBufferFlushed Number of buffer Flushed */
    OMX_U32 NumBufferFlushed;
    /** @param IsPortFlushed Boolean variables indicate port is being flushed at the moment */
    OMX_BOOL IsPortFlushed;

    OMX_VIDEO_PARAM_PORTFORMATTYPE VideoParam;
    OMX_VIDEO_PARAM_MPEG4TYPE VideoMpeg4;
    OMX_VIDEO_PARAM_H263TYPE VideoH263;
    OMX_VIDEO_PARAM_PROFILELEVELTYPE ProfileLevel;

    //Added these flags as a replacement of semaphores on win32 platform
    OMX_BOOL LoadedToIdleFlag ;
    OMX_BOOL IdleToLoadedFlag ;

} Mpeg4ComponentPortType;


class OpenmaxMpeg4AO : public OsclActiveObject
{
    public:

        OpenmaxMpeg4AO();
        ~OpenmaxMpeg4AO();

        /** Component entry points declarations */
        static OMX_ERRORTYPE BaseComponentGetComponentVersion(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STRING pComponentName,
            OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
            OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
            OMX_OUT OMX_UUIDTYPE* pComponentUUID);

        static OMX_ERRORTYPE BaseComponentGetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nParamIndex,
            OMX_INOUT OMX_PTR ComponentParameterStructure);

        static OMX_ERRORTYPE BaseComponentSetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nParamIndex,
            OMX_IN  OMX_PTR ComponentParameterStructure);

        static OMX_ERRORTYPE BaseComponentGetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_INOUT OMX_PTR pComponentConfigStructure);

        static OMX_ERRORTYPE BaseComponentSetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_IN  OMX_PTR pComponentConfigStructure);

        static OMX_ERRORTYPE BaseComponentGetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType);

        static OMX_ERRORTYPE BaseComponentGetState(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STATETYPE* pState);

        static OMX_ERRORTYPE BaseComponentUseBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer);

        static OMX_ERRORTYPE BaseComponentAllocateBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes);

        static OMX_ERRORTYPE BaseComponentFreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

        static OMX_ERRORTYPE BaseComponentSendCommand(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_COMMANDTYPE Cmd,
            OMX_IN  OMX_U32 nParam,
            OMX_IN  OMX_PTR pCmdData);

        static OMX_ERRORTYPE BaseComponentComponentDeInit(
            OMX_IN  OMX_HANDLETYPE hComponent);

        static OMX_ERRORTYPE BaseComponentEmptyThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

        static OMX_ERRORTYPE BaseComponentFillThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

        static OMX_ERRORTYPE BaseComponentSetCallbacks(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
            OMX_IN  OMX_PTR pAppData);

        static void Mpeg4ComponentGetRolesOfComponent(OMX_STRING* aRoleString);

        /*NON STATIC COUNTERPARTS OF STATIC MEMBER API'S */

        OMX_ERRORTYPE GetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nParamIndex,
            OMX_INOUT OMX_PTR ComponentParameterStructure);

        OMX_ERRORTYPE SetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nParamIndex,
            OMX_IN  OMX_PTR ComponentParameterStructure);

        OMX_ERRORTYPE GetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_INOUT OMX_PTR pComponentConfigStructure);

        OMX_ERRORTYPE SetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_IN  OMX_PTR pComponentConfigStructure);

        OMX_ERRORTYPE GetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType);

        void GetState(OMX_OUT OMX_STATETYPE* pState);

        OMX_ERRORTYPE UseBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer);

        OMX_ERRORTYPE AllocateBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes);

        OMX_ERRORTYPE FreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

        OMX_ERRORTYPE SendCommand(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_COMMANDTYPE Cmd,
            OMX_IN  OMX_S32 nParam,
            OMX_IN  OMX_PTR pCmdData);

        OMX_ERRORTYPE EmptyThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

        OMX_ERRORTYPE FillThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

        OMX_ERRORTYPE SetCallbacks(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
            OMX_IN  OMX_PTR pAppData);

        OMX_ERRORTYPE ConstructComponent(OMX_PTR pAppData);
        OMX_ERRORTYPE DestroyComponent();

        OMX_PTR GetOmxHandle()
        {
            return &iOmxComponent;
        };

        void SetDecoderMode(int);
        /*OTHER PROCESSING FUNCTIONS */

        void Mpeg4ComponentBufferMgmtFunction();
        void Mpeg4BufferMgmtWithoutMarker(OMX_BUFFERHEADERTYPE*);
        void Mpeg4DecodeWithoutMarker();
        void Mpeg4DecodeWithMarker(OMX_BUFFERHEADERTYPE*);
        OMX_BOOL Mpeg4ComponentAssemblePartialFrames(OMX_BUFFERHEADERTYPE* aInputBuffer);

        OMX_ERRORTYPE Mpeg4ComponentMessageHandler(CoreMessage* Message);
        OMX_ERRORTYPE Mpeg4ComponentDoStateSet(OMX_U32);

        OMX_ERRORTYPE Mpeg4ComponentDisablePort(OMX_S32 PortIndex);
        void Mpeg4ComponentDisableSinglePort(OMX_U32 PortIndex);

        OMX_ERRORTYPE Mpeg4ComponentEnablePort(OMX_S32 PortIndex);
        void Mpeg4ComponentEnableSinglePort(OMX_U32 PortIndex);

        OMX_ERRORTYPE Mpeg4ComponentFlushPort(OMX_S32 PortIndex);

        void Mpeg4ComponentSetPortFlushFlag(OMX_S32, OMX_S32 index, OMX_BOOL value);
        void Mpeg4ComponentSetNumBufferFlush(OMX_S32, OMX_S32 index, OMX_S32 value);

        OMX_S32 Mpeg4ComponentPanic();

        void Mpeg4ComponentReturnInputBuffer(OMX_BUFFERHEADERTYPE* pInputBuffer, Mpeg4ComponentPortType *pPort);
        void Mpeg4ComponentReturnOutputBuffer(OMX_BUFFERHEADERTYPE* pOutputBuffer, Mpeg4ComponentPortType *pPort);

        OMX_ERRORTYPE Mpeg4ComponentInit();
        OMX_ERRORTYPE Mpeg4ComponentDeInit();

        OMX_ERRORTYPE Mpeg4ComponentTunnelRequest(
            OMX_IN  OMX_HANDLETYPE hComp,
            OMX_IN  OMX_U32 nPort,
            OMX_IN  OMX_HANDLETYPE hTunneledComp,
            OMX_IN  OMX_U32 nTunneledPort,
            OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);

        OMX_ERRORTYPE BaseComponentParameterSanityCheck(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_PTR pStructure,
            OMX_IN  size_t size);

        void SetHeader(OMX_PTR aheader, OMX_U32 asize);
        OMX_ERRORTYPE CheckHeader(OMX_PTR aheader, OMX_U32 asize);

        //Flag to call BufferMgmtFunction in the Run() when the component state is executing
        OMX_BOOL			iBufferExecuteFlag;
        Mpeg4PrivateType*	ipAppPriv;


    private:

        PVLogger* iLogger;

        void Run();

        OMX_CALLBACKTYPE*	ipCallbacks;
        OMX_PTR				iCallbackData;
        OMX_STATETYPE		iState;

        CoreDescriptorType* ipCoreDescriptor;
        OMX_U32				iNumInputBuffer;
        //OMX_U32				iNumComponentPorts;
        OMX_U8*				ipFrameDecodeBuffer;
        OMX_BOOL			iPartialFrameAssembly;
        OMX_BOOL			iIsInputBufferEnded;
        OMX_BOOL			iEndofStream;
        OMX_U8*				ipTempInputBuffer;
        OMX_U32				iTempInputBufferFilledLength;
        OMX_COMPONENTTYPE*	ipTargetComponent;
        OMX_PTR				iTargetMarkData;
        OMX_COMPONENTTYPE*	ipTempTargetComponent;
        OMX_PTR				iTempTargetMarkData;
        OMX_BOOL			iMarkPropagate;
        OMX_BOOL			iNewInBufferRequired;
        OMX_U32				iTempConsumedLength;
        OMX_U32				iOutBufferCount;
        OMX_BOOL			iCodecReady;
        OMX_U8*				ipInputCurrBuffer;
        OMX_U32				iInputCurrLength;
        OMX_S32				iFrameCount;
        OMX_BOOL			iStateTransitionFlag;
        OMX_BOOL			iFlushPortFlag;
        OMX_BOOL				iEndOfFrameFlag;
        OMX_BUFFERHEADERTYPE*	ipMpeg4InputBuffer;
        OMX_BUFFERHEADERTYPE*	ipMpeg4OutputBuffer;
        OMX_U32					iTempInputBufferAllocLength;

        OMX_BOOL				iFirstFragment;
        OMX_BOOL				iResizePending;

        OMX_BOOL				iUseExtTimestamp;
        OMX_TICKS				iFrameTimestamp;

        OMX_COMPONENTTYPE iOmxComponent;	// structure
        OMX_U32			iNumPorts;

        PV_OMXComponentCapabilityFlagsType iPVCapabilityFlags;

        //The ports of the component
        Mpeg4ComponentPortType** ipPorts;
        //Indicate whether component has been already initialized */
        OMX_BOOL iIsInit;
        //OpenMAX standard parameter that contains a short description of the available ports
        OMX_PORT_PARAM_TYPE iPortTypesParam;
        OMX_U32 iGroupPriority;
        //ID of a group of components that share the same logical chain
        OMX_U32 iGroupID;
        //Roles of the component
        OMX_U8 iComponentRole[OMX_MAX_STRINGNAME_SIZE];
        //This field holds the private data associated with a mark request, if any
        OMX_MARKTYPE* ipMark;

        //Mpeg4 specific parameter
        Mpeg4Decoder_OMX* ipMpegDecoderObject;
        int iDecMode;
};




#endif // OMX_MPEG4_COMPONENT_H_INCLUDED
