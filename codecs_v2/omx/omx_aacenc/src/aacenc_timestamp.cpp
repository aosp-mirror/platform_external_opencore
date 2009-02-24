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

#include "aacenc_timestamp.h"


//Initialize the parameters
void AacEncTimeStampCalc::SetParameters(uint32 aFreq, uint32 aSamples)
{
    if (0 != aFreq)
    {
        iSamplingFreq = aFreq;
    }

    iSamplesPerFrame = aSamples;
}


//Set the current timestamp equal to the input buffer timestamp
void AacEncTimeStampCalc::SetFromInputTimestamp(uint32 aValue)
{
    iCurrentTs = aValue;
    iCurrentSamples = 0;
}


void AacEncTimeStampCalc::UpdateTimestamp(uint32 aValue)
{
    iCurrentSamples += aValue;
}

//Convert current samples into the output timestamp
uint32 AacEncTimeStampCalc::GetConvertedTs()
{
    uint32 Value = iCurrentTs;

    //Formula used: TS in ms = (samples * 1000/sampling_freq)
    //Rounding added (add 0.5 to the result), extra check for divide by zero
    if (0 != iSamplingFreq)
    {
        Value = iCurrentTs + (iCurrentSamples * 1000 + (iSamplingFreq / 2)) / iSamplingFreq;
    }

    iCurrentTs = Value;
    iCurrentSamples = 0;

    return (Value);
}
