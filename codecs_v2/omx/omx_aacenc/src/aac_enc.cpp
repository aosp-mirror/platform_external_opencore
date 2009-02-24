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

/*----------------------------------------------------------------------------
; INCLUDES
----------------------------------------------------------------------------*/

#include "aac_enc.h"

#ifndef USE_HEAAC
#define BITRATE_DEFAULT 128000
#else
#define BITRATE_DEFAULT  56000
#endif

#define BANDWIDTH_DEFAULT      0


OmxAacEncoder::OmxAacEncoder()
{
    iAacEncoder = NULL;
    iAacInputBuffer = NULL;
    iAacOutputBuffer = NULL;
    iAscSize = 12;
    iAscFlag = OMX_FALSE;

    oscl_memset(&iAacEncConfig, 0, sizeof(ENC_CONFIGURATION));
}

OMX_S32 OmxAacEncoder::AacEncConfigDefaultSettings(ENC_CONFIGURATION* pConfig)
{
    pConfig->bitrate     = BITRATE_DEFAULT;     /* bitstream bitrate */
    pConfig->bitrateMode = 0;                   /* constant bitrate */
    pConfig->bandwidth   = BANDWIDTH_DEFAULT;
    pConfig->dualMono    = 0;                   /* no dualMono by default */

#ifndef USE_HEAAC
    pConfig->useAOT      = AOT_AAC_LC;          /* AAC-LC by default */
#else
    pConfig->useAOT      = AOT_SBR;             /* otherwise AAC/SBR */
#endif

    pConfig->useTransmux = TT_MP4_ADTS;         /* ADTS header by default */
    pConfig->useCRC      = 0;                   /* no CRC by default */
    pConfig->useTns      = 1;                   /* TNS activated by default */
    pConfig->usePns      = 1;                   /* PNS activated by default.
                                                * Depending on channelBitrate this might be set to 0 later */
#ifdef AFTER_BURNER
    pConfig->useRequant  = 1;                   /* Afterburner activated by default */
#endif
    pConfig->ancFlag     = 0;                   /* no ancillary data by default */

    pConfig->downmix = 0;

    return 0;
}


/* Decoder Initialization function */
OMX_BOOL OmxAacEncoder::AacEncInit(OMX_AUDIO_PARAM_PCMMODETYPE aPcmMode,
                                   OMX_AUDIO_PARAM_AACPROFILETYPE aAacParam,
                                   OMX_U32* aInputFrameLength)
{
    OMX_S32 Status;
    AAC_ENCODER_ERROR ErrorStatus;


    // Configure Encoder
    Status = aacEncConfigure(&iAacEncoder, &iAacEncConfig);
    if (Status)
    {
        // Failure occurred during initialization so return right away
        return OMX_FALSE;
    }

    // Get default settings
    AacEncConfigDefaultSettings(&iAacEncConfig);

    //Now Apply the parameter settings from the client
    iAacEncConfig.sampleRate = aAacParam.nSampleRate;
    iAacEncConfig.nChannelsIn = aPcmMode.nChannels;
    iAacEncConfig.nChannelsOut = aAacParam.nChannels;


    if (0 != aAacParam.nAudioBandWidth)
    {
        iAacEncConfig.bandwidth = aAacParam.nAudioBandWidth;
    }

    /* DV: comment out for now
        if (OMX_AUDIO_ChannelModeDual == aAacParam.eChannelMode)
        {
            iAacEncConfig.dualMono = 1;
        }

        if (OMX_AUDIO_ChannelModeMono == aAacParam.eChannelMode)
        {
            iAacEncConfig.nChannelsOut = 1;
        }
    */

    if (OMX_AUDIO_AACObjectMain == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_AAC_MAIN;
    }
    else if (OMX_AUDIO_AACObjectLC == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_AAC_LC;
    }
    else if (OMX_AUDIO_AACObjectSSR == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_AAC_SSR;
    }
    else if (OMX_AUDIO_AACObjectLTP == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_AAC_LTP;
    }
    else if (OMX_AUDIO_AACObjectHE == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_SBR;
    }
    else if (OMX_AUDIO_AACObjectScalable == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_AAC_SCAL;
    }
    else if (OMX_AUDIO_AACObjectERLC == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_ER_AAC_LC;
    }
    else if (OMX_AUDIO_AACObjectLD == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_ER_AAC_LD;
    }
    else if (OMX_AUDIO_AACObjectHE_PS == aAacParam.eAACProfile)
    {
        iAacEncConfig.useAOT = AOT_PS;
    }
    else
    {
        return OMX_FALSE;
    }

    iAacEncConfig.bitrate = aAacParam.nBitRate;
    // it is possible that desired bitrate is not set by the client - if so,
    // use a default value based on appropriate sampling rate and type of AAC profile
    if (iAacEncConfig.bitrate == 0)
    {
        switch (iAacEncConfig.sampleRate)
        {
            case 48000:
            case 44100:
                iAacEncConfig.bitrate = 96000;
                break;
            case 32000:
                iAacEncConfig.bitrate = 84000;
                break;
            case 24000:
            case 22050:
                iAacEncConfig.bitrate = 72000;
                break;
            case 16000:
                iAacEncConfig.bitrate = 48000;
                break;
            case 11025:
                iAacEncConfig.bitrate = 36000;
                break;
            case 8000:
                iAacEncConfig.bitrate = 24000;
                break;
            default:
                iAacEncConfig.bitrate = 24000;
                break;
        }
    }


    if ((OMX_AUDIO_AACStreamFormatMP2ADTS == aAacParam.eAACStreamFormat) ||
            (OMX_AUDIO_AACStreamFormatMP4ADTS == aAacParam.eAACStreamFormat))
    {
        iAacEncConfig.useTransmux = TT_MP4_ADTS;
    }
    else if (OMX_AUDIO_AACStreamFormatMP4LOAS == aAacParam.eAACStreamFormat)
    {
        iAacEncConfig.useTransmux = TT_MP4_LOAS;
    }
    else if (OMX_AUDIO_AACStreamFormatMP4LATM == aAacParam.eAACStreamFormat)
    {
        iAacEncConfig.useTransmux = TT_MP4_RAWLATM;
    }
    else if (OMX_AUDIO_AACStreamFormatADIF == aAacParam.eAACStreamFormat)
    {
        iAacEncConfig.useTransmux = TT_MP4_ADIF;
    }
    else if (OMX_AUDIO_AACStreamFormatMP4FF == aAacParam.eAACStreamFormat)
    {
        //iAacEncConfig.useTransmux = TT_MP4_MP4F;
        // DV: use temporarily to bypass an error:
        iAacEncConfig.useTransmux = TT_MP4_RAWPACKETS;

    }
    else if (OMX_AUDIO_AACStreamFormatRAW == aAacParam.eAACStreamFormat)
    {
        iAacEncConfig.useTransmux = TT_MP4_RAWPACKETS;
    }
    else
    {
        return OMX_FALSE;
    }


    if (0 == (OMX_AUDIO_AACToolTNS & aAacParam.nAACtools))
    {
        iAacEncConfig.useTns = 0;
    }

    if (0 == (OMX_AUDIO_AACToolPNS & aAacParam.nAACtools))
    {
        iAacEncConfig.usePns = 0;
    }


    //Allocating memory for input and output buffers
    iAacInputBuffer = (OMX_U8*) oscl_malloc(INPUTBUFFER_SIZE * 2 * sizeof(OMX_U8));
    iAacOutputBuffer = (OMX_U8*) oscl_malloc(OUTPUTBUFFER_SIZE * sizeof(OMX_U8));

    // Initialize and open the encoder
    Status = aacEncOpen(iAacEncoder, &iAacEncConfig, (INT_PCM*) iAacInputBuffer, iAacOutputBuffer);
    if (Status)
    {
        // Failure occurred during initialization so return right away
        return OMX_FALSE;
    }

    /* RTP access unit simulation: write access unit length (4 byte) before audio specific config */
    if (TT_MP4_RAWPACKETS == iAacEncConfig.useTransmux)
    {
        ErrorStatus = aacEncGetAsc(iAacEncoder, (UINT*) & iAscSize, iAscBuf);
        if (AAC_ENC_OK != ErrorStatus)
        {
            // Failure occurred during initialization so return right away
            return OMX_FALSE;
        }
    }

    *aInputFrameLength = iAacEncConfig.nSamplesRead << 1;

    return OMX_TRUE;
}


/* Decoder De-Initialization function */
void OmxAacEncoder::AacEncDeinit()
{
    if (iAacInputBuffer)
    {
        oscl_free(iAacInputBuffer);
        iAacInputBuffer = NULL;
    }

    if (iAacOutputBuffer)
    {
        oscl_free(iAacOutputBuffer);
        iAacOutputBuffer = NULL;
    }

    if (iAacEncoder)
    {
        aacEncClose(&iAacEncoder);
        iAacEncoder = NULL;
    }

}


/* Decode function for all the input formats */
OMX_BOOL OmxAacEncoder::AacEncodeFrame(OMX_U8*    aOutputBuffer,
                                       OMX_U32*   aOutputLength,
                                       OMX_U8*    aInBuffer,
                                       OMX_U32	  aInBufSize)
{

    OMX_S32 NumImputSamples = iAacEncConfig.nSamplesRead;
    OMX_S32 Status;
    //OMX_S32 OutputOffset = 0;

    if (TT_MP4_RAWPACKETS == iAacEncConfig.useTransmux)
    {
        if (OMX_FALSE == iAscFlag)
        {
            //oscl_memcpy(aOutputBuffer, &iAscSize, 4);
            //oscl_memcpy((aOutputBuffer + 4), iAscBuf, iAscSize);
            // copy the config data
            oscl_memcpy(aOutputBuffer, iAscBuf, iAscSize);
            iAscFlag = OMX_TRUE;
            //OutputOffset = 4 + iAscSize;
            *aOutputLength = iAscSize;
            return OMX_TRUE;

        }
        //Four bytes to write the size of frame after it has been encoded
        //OutputOffset += 4;
    }

    iAacEncConfig.pOutBuffer = aOutputBuffer; // + OutputOffset;

    if (aInBufSize < NumImputSamples)
    {
        NumImputSamples = aInBufSize >> 1;
    }

    oscl_memcpy(iAacEncConfig.pInBuffer, aInBuffer, (NumImputSamples << 1));

    // Encode frame
    Status = aacEncEncode(iAacEncoder,
                          &iAacEncConfig,
                          (INT32*) aOutputLength,
                          NumImputSamples);

    // Check to find if error occurred
    if (Status)
    {
        return OMX_FALSE;
    }

    //if (TT_MP4_RAWPACKETS == iAacEncConfig.useTransmux)
    //{
    //    oscl_memcpy((aOutputBuffer + (OutputOffset - 4)), &(*aOutputLength), 4);
    //}

    return OMX_TRUE;
}



