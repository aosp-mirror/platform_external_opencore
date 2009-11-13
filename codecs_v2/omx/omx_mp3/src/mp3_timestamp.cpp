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

#include "mp3_timestamp.h"


//Initialize the parameters
void Mp3TimeStampCalc::SetParameters(uint32 aFreq, uint32 aSamples)
{
    if (0 != aFreq)
    {
        iSamplingFreq = aFreq;
    }

    iSamplesPerFrame = aSamples;
}


//Set the current timestamp equal to the input buffer timestamp
void Mp3TimeStampCalc::SetFromInputTimestamp(OMX_TICKS aValue)
{
    iCurrentTs = aValue;
    iCurrentSamples = 0;
}


void Mp3TimeStampCalc::UpdateTimestamp(uint32 aValue)
{
    iCurrentSamples += aValue;
}

//Convert current samples into the output timestamp
OMX_TICKS Mp3TimeStampCalc::GetConvertedTs()
{
    OMX_TICKS Value = iCurrentTs;

    //Formula used: TS in microsec = (samples * 1000000/sampling_freq)
    //Rounding added (add 0.5 to the result), extra check for divide by zero
    if (0 != iSamplingFreq)
    {

        Value = iCurrentTs + ((OMX_TICKS)iCurrentSamples * 1000000 + (iSamplingFreq >> 1)) / iSamplingFreq;
    }

    iCurrentTs = Value;
    iCurrentSamples = 0;

    return (Value);
}

/* Do not update iCurrentTs value, just calculate & return the current timestamp */
OMX_TICKS Mp3TimeStampCalc::GetCurrentTimestamp()
{
    OMX_TICKS Value = iCurrentTs;

    if (0 != iSamplingFreq)
    {
        Value = iCurrentTs + ((OMX_TICKS)iCurrentSamples * 1000000 + (iSamplingFreq >> 1)) / iSamplingFreq;

    }

    return (Value);
}

//Calculate the duration in microsec of single frame
OMX_TICKS Mp3TimeStampCalc::GetFrameDuration()
{
    OMX_TICKS Value = 0;

    if (0 != iSamplingFreq)
    {
        Value = ((OMX_TICKS)iSamplesPerFrame * 1000000 + (iSamplingFreq >> 1)) / iSamplingFreq;
    }

    return (Value);
}
