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
/*********************************************************************************/
/*
    This PVA_FF_HintSampleEntry is used for hint tracks.
*/


#ifndef __HintSampleEntry_H__
#define __HintSampleEntry_H__

#include "sampleentry.h"
#include "hintsample.h"

class PVA_FF_HintSampleEntry : public PVA_FF_SampleEntry
{

    public:
        PVA_FF_HintSampleEntry(uint32 protocol); // Constructor

        virtual ~PVA_FF_HintSampleEntry();

        // Member gets and sets
        uint32 getEntryCount() const
        {
            return _entryCount;
        }

        //void nextHintSample(uint8 *psample, uint32 size, uint32 ts, BOOLEAN isIFrame, BOOLEAN isBaseLayer);
        void nextHintSample(uint8 *psample, uint32 size, uint32 ts, uint8 flags); // For video hint samples
        void nextHintSample(uint8 *psample, uint32 size, uint32 ts); // For audio hint samples
        void nextHintSample(uint8 *psample, uint32 size, uint32 ts, uint32 duration); // For still image hint samples

        PVA_FF_HintSample* getSampleAt(int32 index) const;

        Oscl_Vector<PVA_FF_HintSample*, OsclMemAllocator>& getSampleVec() const
        {
            return *_psampleVec;
        }

        int32 seek(int32 baseLayerSampleNum) const; // Seeks to the base-layer sample in the hint track

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        virtual uint32 getHintOffset()
        {
            return (_HintOffset);
        }


    private:
        void addHintSample(PVA_FF_HintSample *sample);
        void insertHintEntry(int32 index, PVA_FF_HintSample *sample);
        void replaceHintEntry(int32 index, PVA_FF_HintSample *sample);

        void init();
        virtual void recomputeSize();

        uint32 _entryCount;
        Oscl_Vector<PVA_FF_HintSample*, OsclMemAllocator>*_psampleVec;
        uint32  _mediaOffset;
        uint32 _HintOffset;

};


#endif

