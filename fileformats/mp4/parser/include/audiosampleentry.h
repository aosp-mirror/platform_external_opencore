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
/*     -------------------------------------------------------------------       */
/*                        MPEG-4 AudioSampleEntry Class                          */
/*     -------------------------------------------------------------------       */
/*********************************************************************************/
/*
    This AudioSampleEntry Class is used for visual streams.
*/


#ifndef AUDIOSAMPLEENTRY_H_INCLUDED
#define AUDIOSAMPLEENTRY_H_INCLUDED

#ifndef OSCL_FILE_IO_H_INCLUDED
#include "oscl_file_io.h"
#endif
#ifndef SAMPLEENTRY_H_INCLUDED
#include "sampleentry.h"
#endif
#ifndef ESDATOM_H_INCLUDED
#include "esdatom.h"
#endif

class AudioSampleEntry : public SampleEntry
{

    public:
        AudioSampleEntry(MP4_FF_FILE *fp, uint32 size, uint32 type);
        virtual ~AudioSampleEntry();

        // Member gets and sets
        const ESDAtom &getESDAtom() const
        {
            return *_pes;
        }

        uint16 getTimeScale()
        {
            return _timeScale;
        }

        virtual uint32 getESID() const
        {
            if (_pes != NULL)
            {
                if (_pes->getESDescriptorPtr() != NULL)
                {
                    return _pes->getESDescriptorPtr()->getESID();
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }

        virtual const ESDescriptor *getESDescriptor() const
        {
            if (_pes != NULL)
            {
                return _pes->getESDescriptorPtr();
            }
            else
            {
                return NULL;
            }
        }

        virtual    uint8 getObjectTypeIndication() const
        {
            if (_pes != NULL)
            {
                return _pes->getObjectTypeIndication();
            }
            else
            {
                return 0xFF;
            }
        }

        virtual DecoderSpecificInfo *getDecoderSpecificInfo() const
        {
            if (_pes != NULL)
            {
                return _pes->getDecoderSpecificInfo();
            }
            else
            {
                return NULL;
            }
        }

        uint32 getAverageBitrate() const
        {
            if (_pes != NULL)
            {
                return _pes->getAverageBitrate();
            }
            else
            {
                return 0;
            }
        }

        uint32 getMaxBitrate() const
        {
            if (_pes != NULL)
            {
                return _pes->getMaxBitrate();
            }
            else
            {
                return 0;
            }
        }

        // Get the max size buffer needed to retrieve the media samples
        uint32 getMaxBufferSizeDB() const
        {
            if (_pes != NULL)
            {
                return _pes->getMaxBufferSizeDB();
            }
            else
            {
                return 0;
            }
        }


    private:
        // Reserved constants
        uint32 _reserved1[2];
        uint16 _reserved2;
        uint16 _reserved3;
        uint32 _reserved4;
        uint16 _reserved5;
        uint16 _timeScale;

        ESDAtom *_pes;
};


#endif // AUDIOSAMPLEENTRY_H_INCLUDED


