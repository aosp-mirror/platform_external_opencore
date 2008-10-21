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
    This PVA_FF_SampleDescriptionAtom Class gives detailed information about the codeing
    type used, and any initialization information needed for coding.
*/


#define IMPLEMENT_SampleDescriptionAtom

//#define AMR_3GPP_CONTENT
//#define H263_3GPP_CONTENT

#include "sampledescriptionatom.h"

#include "visualsampleentry.h"
#include "audiosampleentry.h"
#include "hintsampleentry.h"
#include "mpegsampleentry.h"

#include "objectdescriptorupdate.h"

#include "atomutils.h"
#include "a_atomdefs.h"
#include "objectdescriptor.h"
#include "decoderspecificinfo.h"
#include "amrdecoderspecificinfo.h"
#include "evrcdecoderspecificinfo.h"
#include "h263decoderspecificinfo.h"

#include "amrsampleentry.h"
#include "h263sampleentry.h"
#include"avcsampleentry.h"

#ifndef TEXTSAMPLEENTRY_H_INCLUDED
#include "textsampleentry.h"
#endif
const int32 AMRModeSetMask[16] =
{
    0x0001, 0x0002, 0x0004, 0x0008,
    0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800,
    0x1000, 0x2000, 0x4000, 0x8000
};

const int32 AMRBitRates[9] =
{
    4750, 5150,  5900,
    6700, 7400,  7950,
    10200, 12200, 2400
};

typedef Oscl_Vector<PVA_FF_SampleEntry*, OsclMemAllocator> PVA_FF_SampleEntryVecType;
typedef Oscl_Vector<uint32, OsclMemAllocator> PVA_FF_SampleEntryTypeVecType;

// Constructor
PVA_FF_SampleDescriptionAtom::PVA_FF_SampleDescriptionAtom(uint32 mediaType, int32 codecType,
        bool o3GPPCompliant, uint32 protocol, uint8 profile,
        uint8 profileComp, uint8 level)
        : PVA_FF_FullAtom(SAMPLE_DESCRIPTION_ATOM, (uint8)0, (uint32)0),
        _mediaType(mediaType)
{
    _codecType = codecType;

    _o3GPPCompliant = o3GPPCompliant;

    init(mediaType, protocol, profile, profileComp, level);
    recomputeSize();
}

void
PVA_FF_SampleDescriptionAtom::init(int32 mediaType, uint32 protocol, uint8 profile, uint8 profileComp, uint8 level)
{
    _entryCount = 0;
    _textflag = false;
    _text_sample_num = 0;

    PVA_FF_SampleEntry *entry = NULL;

    _psampleEntryTypeVec = NULL;
    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_SampleEntryTypeVecType, (), _psampleEntryTypeVec);

    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_SampleEntryVecType, (), _psampleEntryVec);

    _mediaTimeScale = 0;

    _h263Profile  = 0;
    _h263Level    = 10;
    _frame_width  = 0;
    _frame_height = 0;

    switch (mediaType)
    {
        case MEDIA_TYPE_TEXT:
        {
            _handlerType = MEDIA_TYPE_TEXT;
            if (_codecType == CODEC_TYPE_TIMED_TEXT)
            {
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_TextSampleEntry, (), entry);

                addSampleEntry(entry);
                entry->setParent(this);
                entry->recomputeSize();
            }

        }
        break;
        case MEDIA_TYPE_AUDIO:
        {
            _handlerType = MEDIA_TYPE_AUDIO;
            if (_codecType == CODEC_TYPE_AAC_AUDIO)
            {
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_AudioSampleEntry, (_codecType), entry);
                addSampleEntry(entry);
            }

            if (_codecType == CODEC_TYPE_AMR_AUDIO)
            {
                if (_o3GPPCompliant)
                {
                    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_AMRSampleEntry, (), entry);

                    addSampleEntry(entry);
                    entry->setParent(this);
                    entry->recomputeSize();
                }
            }
        }
        break;
        case MEDIA_TYPE_VISUAL:
        {
            _handlerType = MEDIA_TYPE_VISUAL;

            if (_codecType == CODEC_TYPE_BASELINE_H263_VIDEO)
            {
                if (_o3GPPCompliant)
                {
                    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_H263SampleEntry, (), entry);
                    addSampleEntry(entry);
                }
                else
                {
                    PV_MP4_FF_NEW(fp->auditCB, PVA_FF_VisualSampleEntry, (_codecType), entry);

                    addSampleEntry(entry);
                    addDecoderSpecificInfo(NULL);
                }
            }
            else if (_codecType == CODEC_TYPE_AVC_VIDEO)
            {
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_AVCSampleEntry, (profile, profileComp, level), entry);
                addSampleEntry(entry);
                entry->setParent(this);
                entry->recomputeSize();

            }
            else
            {
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_VisualSampleEntry, (_codecType), entry);
                addSampleEntry(entry);
            }
        }
        break;
        case MEDIA_TYPE_OBJECT_DESCRIPTOR:
        {
            _handlerType = MEDIA_TYPE_OBJECT_DESCRIPTOR;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_OBJECT_DESCRIPTOR), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_CLOCK_REFERENCE:
        {
            _handlerType = MEDIA_TYPE_CLOCK_REFERENCE;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_CLOCK_REFERENCE), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_SCENE_DESCRIPTION:
        {
            _handlerType = MEDIA_TYPE_SCENE_DESCRIPTION;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_SCENE_DESCRIPTION), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_MPEG7:
        {
            _handlerType = MEDIA_TYPE_MPEG7;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_OBJECT_CONTENT_INFO), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_OBJECT_CONTENT_INFO:
        {
            _handlerType = MEDIA_TYPE_OBJECT_CONTENT_INFO;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_OBJECT_CONTENT_INFO), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_IPMP:
        {
            _handlerType = MEDIA_TYPE_IPMP;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_IPMP), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_MPEG_J:
        {
            _handlerType = MEDIA_TYPE_MPEG_J;
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_MPEG_J), entry);
            addSampleEntry(entry);
        }
        break;
        case MEDIA_TYPE_UNKNOWN:
        default:
        {
            _handlerType = UNKNOWN_HANDLER; // Error condition!
            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_MpegSampleEntry, (MEDIA_TYPE_UNKNOWN), entry);
            addSampleEntry(entry);
        }
        break;
    }
}



// Destructor
PVA_FF_SampleDescriptionAtom::~PVA_FF_SampleDescriptionAtom()
{
    // CLEAN UP VECTOR!!

    for (uint32 i = 0; i < _psampleEntryVec->size(); i++)
    {
        PVA_FF_SampleEntry* entry = (*_psampleEntryVec)[i];

        if (_psampleEntryTypeVec != NULL)
        {
            uint32 type = entry->getType();
            if (type == AUDIO_SAMPLE_ENTRY)
            {
                PVA_FF_AudioSampleEntry* audioentry = OSCL_STATIC_CAST(PVA_FF_AudioSampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_AudioSampleEntry, audioentry);
            }
            else if (type == AMR_SAMPLE_ENTRY)
            {
                PVA_FF_AMRSampleEntry* audioentry = OSCL_STATIC_CAST(PVA_FF_AMRSampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_AMRSampleEntry, audioentry);

            }
            else if (type == H263_SAMPLE_ENTRY)
            {
                PVA_FF_H263SampleEntry* videoentry = OSCL_STATIC_CAST(PVA_FF_H263SampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_H263SampleEntry, videoentry);

            }
            else if (type == AVC_SAMPLE_ENTRY)
            {
                PVA_FF_AVCSampleEntry* videoentry = OSCL_STATIC_CAST(PVA_FF_AVCSampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_AVCSampleEntry, videoentry);

            }
            else if (type == TEXT_SAMPLE_ENTRY)//for timed text support
            {
                PVA_FF_TextSampleEntry* textentry = OSCL_STATIC_CAST(PVA_FF_TextSampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_TextSampleEntry, textentry);
            }
            else if (type == VIDEO_SAMPLE_ENTRY)
            {
                PVA_FF_VisualSampleEntry* videoentry = OSCL_STATIC_CAST(PVA_FF_VisualSampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_VisualSampleEntry, videoentry);

            }
            else if (type == MPEG_SAMPLE_ENTRY)
            {
                PVA_FF_MpegSampleEntry* videoentry = OSCL_STATIC_CAST(PVA_FF_MpegSampleEntry*, entry);
                PV_MP4_FF_DELETE(NULL, PVA_FF_MpegSampleEntry, videoentry);

            }
        }
    }
    PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_SampleEntryVecType, Oscl_Vector, _psampleEntryVec);
    if (_psampleEntryTypeVec != NULL)
    {
        PV_MP4_FF_TEMPLATED_DELETE(NULL, PVA_FF_SampleEntryTypeVecType, Oscl_Vector, _psampleEntryTypeVec);
    }

}

//added for timed text support
void PVA_FF_SampleDescriptionAtom::addTextDecoderSpecificInfo(PVA_FF_TextSampleDescInfo *pinfo)
{

    PVA_FF_SampleEntry *entry = NULL;
    bool flagindex = true;
    switch (_mediaType)
    {
        case MEDIA_TYPE_TEXT:
        {
            if (_codecType == CODEC_TYPE_TIMED_TEXT)
            {//storing the start, end and the sample index number
                //to which these samples belongs to.

                if (_textflag == false)
                {
                    _SDIndex.push_back(pinfo->sdindex);
                    getMutableSampleEntryAt(0)->addTextDecoderSpecificInfo(pinfo);
                    getMutableSampleEntryAt(0)->recomputeSize();
                    recomputeSize();
                    _textflag = true;
                }
                else
                {
                    for (uint32 kk = 0;kk < _SDIndex.size();kk++)
                    {
                        if (_SDIndex[kk] == pinfo->sdindex)
                        {
                            flagindex = false;
                        }
                    }
                    if (flagindex == true)
                    {
                        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_TextSampleEntry, (), entry);
                        addSampleEntry(entry);
                        entry->setParent(this);
                        entry->recomputeSize();

                        _SDIndex.push_back(pinfo->sdindex);
                        getMutableSampleEntryAt(pinfo->sdindex)->addTextDecoderSpecificInfo(pinfo);
                        getMutableSampleEntryAt(pinfo->sdindex)->recomputeSize();
                        recomputeSize();
                    }

                }

            }
        }
        break;
        default:
            return; // This fp actually an undefined situation
    }
}


void
PVA_FF_SampleDescriptionAtom::addDecoderSpecificInfo(PVA_FF_DecoderSpecificInfo *pinfo)
{
    switch (_mediaType)
    {
        case MEDIA_TYPE_VISUAL:
        {
            // ALL PARAMETERS DEFAULTED
            if (_codecType == CODEC_TYPE_BASELINE_H263_VIDEO)
            {
                PVA_FF_H263DecoderSpecificInfo *pH263info = NULL;
                PV_MP4_FF_NEW(fp->auditCB, PVA_FF_H263DecoderSpecificInfo, (), pH263info);
                pH263info->setVendorCode();
                pH263info->setEncoderVersion();
                pH263info->setCodecProfile();
                pH263info->setCodecLevel();
                pH263info->setMaxWidth();
                pH263info->setMaxHeight();
                getMutableSampleEntryAt(0)->addDecoderSpecificInfo(pH263info);
            }
            else if (_codecType == CODEC_TYPE_MPEG4_VIDEO)
            {
                getMutableSampleEntryAt(0)->addDecoderSpecificInfo(pinfo);
            }
            else if (_codecType == CODEC_TYPE_AVC_VIDEO)
            {
                getMutableSampleEntryAt(0)->addDecoderSpecificInfo(pinfo);
                getMutableSampleEntryAt(0)->recomputeSize();
                recomputeSize();

            }
            else
            {
                // BY DEFAULT SET THE DECODER SPECIFIC INFO - FOR ANY FUTURE
                // VIDEO CODECS
                getMutableSampleEntryAt(0)->addDecoderSpecificInfo(pinfo);
            }
        }
        break;

        case MEDIA_TYPE_AUDIO:
        {
            if (_codecType == CODEC_TYPE_AAC_AUDIO)
            {
                getMutableSampleEntryAt(0)->addDecoderSpecificInfo(pinfo);
            }
        }
        break;

        case MEDIA_TYPE_SCENE_DESCRIPTION:
        {
            getMutableSampleEntryAt(0)->addDecoderSpecificInfo(pinfo);
        }
        break;

        default:
            return; // This fp actually an undefined situation
    }
}


// Stream properties
void
PVA_FF_SampleDescriptionAtom::setTargetBitrate(uint32 bitrate)
{
    switch (_mediaType)
    {
        case MEDIA_TYPE_VISUAL:
        {
            uint32 type = getSampleEntryAt(0)->getType();
            if (type == VIDEO_SAMPLE_ENTRY)
            {
                PVA_FF_VisualSampleEntry *ventry = (PVA_FF_VisualSampleEntry*) getSampleEntryAt(0);
                ventry->setTargetBitrate(bitrate);
            }
            else if (type == H263_SAMPLE_ENTRY)
            {
                PVA_FF_H263SampleEntry *hentry = (PVA_FF_H263SampleEntry*) getSampleEntryAt(0);
                PVA_FF_H263SpecficAtom *pH263info =
                    (PVA_FF_H263SpecficAtom *)(hentry->get3GPPDecoderSpecificInfo());
                pH263info->_ph263_decbitrateatom->setAvgBitrate(bitrate);
                pH263info->_ph263_decbitrateatom->setMaxBitrate(bitrate);
            }
            break;
        }
        case MEDIA_TYPE_AUDIO:
        {
            _currAudioBitrate = bitrate;
            if (_codecType == CODEC_TYPE_AAC_AUDIO)
            {
                PVA_FF_AudioSampleEntry *entry = (PVA_FF_AudioSampleEntry*) getSampleEntryAt(0);
                entry->setTargetBitrate(bitrate);
            }
            break;
        }
        default:
            break; // This fp actually an undefined situation
    }
}

void
PVA_FF_SampleDescriptionAtom::setMaxBufferSizeDB(uint32 max)
{
    // Assuming only one sample entry for video tracks
    if ((uint32) _mediaType == MEDIA_TYPE_VISUAL)
    {
        uint32 type = getSampleEntryAt(0)->getType();
        if (type == VIDEO_SAMPLE_ENTRY)
        {
            PVA_FF_VisualSampleEntry *entry =
                (PVA_FF_VisualSampleEntry*) getSampleEntryAt(0);
            entry->setMaxBufferSizeDB(max);
        }
    }
}

int32
PVA_FF_SampleDescriptionAtom::nextSample(uint32 size, uint8 flags)
{
    uint8 audio_frame_type;
    int32 nReturn = 0;

    switch (_mediaType)
    {
        case MEDIA_TYPE_VISUAL:
        {
            if (_codecType == CODEC_TYPE_AVC_VIDEO)
            {
                //	PVA_FF_AVCSampleEntry *entry = (PVA_FF_AVCSampleEntry*) getSampleEntryAt(0);

                //	entry->setSample(psample,size);

                //		entry->recomputeSize();
                //		recomputeSize();


            }

            else if (!_o3GPPCompliant)
            {
                PVA_FF_VisualSampleEntry *entry = (PVA_FF_VisualSampleEntry*) getSampleEntryAt(0);
                entry->nextSampleSize(size);
            }
            nReturn = 1;
            break;
        }
        case MEDIA_TYPE_AUDIO:
        {
            if (_codecType == CODEC_TYPE_AAC_AUDIO)
            {
                //HANDLE LIKE VIDEO - ONLY ONE SAMPLE ENTRY PER TRACK
                PVA_FF_AudioSampleEntry *entry = (PVA_FF_AudioSampleEntry*) getSampleEntryAt(0);
                entry->nextSampleSize(size);
                nReturn = 1;
            }
            else
            {
                unsigned short mode_set = 0;

                if (!_o3GPPCompliant)
                {
                    bool found = false;
                    uint32 i = 0;
                    int32 frameType;

                    //CHECK FOR FRAME TYPE AND SET FLAGS ACCORDINGLY
                    audio_frame_type = (uint8)(flags & 0x0f);

                    for (i = 0; i < _psampleEntryVec->size(); i++)
                    {
                        switch (_codecType)
                        {
                            case CODEC_TYPE_AMR_AUDIO:
                            {
                                PVA_FF_AMRDecoderSpecificInfo *pAMRInfo =
                                    (PVA_FF_AMRDecoderSpecificInfo *)(getSampleEntryAt(i)->getDecoderSpecificInfo());

                                frameType = pAMRInfo->getFrameType();

                                if (frameType == audio_frame_type)
                                {
                                    found = true;
                                }
                                else
                                {
                                    mode_set = pAMRInfo->getModeSet();

                                    mode_set |= AMRModeSetMask[(int)audio_frame_type];

                                    pAMRInfo->setModeSet(mode_set);
                                }
                            }
                            break;

                            default:
                                break;
                        }

                        if (found)
                        {
                            break;
                        }
                    }

                    if (!found) // Add new audio sample entry
                    {
                        PVA_FF_AudioSampleEntry *entry = NULL;
                        PV_MP4_FF_NEW(fp->auditCB, PVA_FF_AudioSampleEntry, (_codecType), entry);
                        entry->nextSampleSize(size);

                        if (_codecType == CODEC_TYPE_AMR_AUDIO)
                        {
                            PVA_FF_AMRDecoderSpecificInfo *pAMRInfo = NULL;
                            PV_MP4_FF_NEW(fp->auditCB, PVA_FF_AMRDecoderSpecificInfo, (), pAMRInfo);

                            pAMRInfo->setVendorcode(PACKETVIDEO_FOURCC);
                            pAMRInfo->setEncoderVersion(1);

                            entry->setTargetBitrate(AMRBitRates[(int)audio_frame_type]);


                            pAMRInfo->setFrameType(audio_frame_type);

                            mode_set |= AMRModeSetMask[(int)audio_frame_type];
                            pAMRInfo->setModeSet(mode_set);
                            entry->addDecoderSpecificInfo(pAMRInfo);
                        }
                        addSampleEntry(entry);

                        nReturn = _psampleEntryVec->size();
                    }
                    else
                    {
                        nReturn = i + 1;
                    }
                }
                else
                {
                    if (_codecType == CODEC_TYPE_AMR_AUDIO)
                    {
                        //CHECK FOR FRAME TYPE AND SET FLAGS ACCORDINGLY
                        //audio_frame_type = (uint8)(flags & 0x0f);

                        audio_frame_type = flags >> 3;

                        if (getSampleEntryAt(0) != NULL)
                        {
                            PVA_FF_AMRSampleEntry *entry = (PVA_FF_AMRSampleEntry *)(getSampleEntryAt(0));

                            mode_set = entry->getModeSet();

                            mode_set |= AMRModeSetMask[(int)audio_frame_type];

                            entry->setModeSet(mode_set);
                        }
                        nReturn = 1;
                    }
                }
            }
            break;
        }
        // TBA
        // Check whether or not this sample meets the criteria
        // of the existing PVA_FF_SampleEntry(s) ESDAtoms.  If it does, return.
        // Otherwise, add new entry according to this sample
        case MEDIA_TYPE_OBJECT_DESCRIPTOR:
        {
            // Actual sample fp an PVA_FF_ObjectDescriptorUpdate command
            // PVA_FF_ObjectDescriptorUpdate *pupdate = (PVA_FF_ObjectDescriptorUpdate*)psample;
            // TBA
            // Check whether or not this sample meets the criteria
            // of the existing PVA_FF_SampleEntry(s) ESDAtoms.  If it does, return.
            // Otherwise, add new entry according to this sample
            // ASSUMING ATLEAST ONE MPEG SAMPLE ENTRY WILL BE ADDED TO THE
            // OD TRACK
            PVA_FF_MpegSampleEntry *entry = (PVA_FF_MpegSampleEntry*) getSampleEntryAt(0);
            entry->nextSampleSize(size);
            nReturn = 1;
        }
        break;
        case MEDIA_TYPE_CLOCK_REFERENCE:
        case MEDIA_TYPE_SCENE_DESCRIPTION:
        case MEDIA_TYPE_MPEG7:
        case MEDIA_TYPE_OBJECT_CONTENT_INFO:
        case MEDIA_TYPE_IPMP:
        case MEDIA_TYPE_MPEG_J:
        case MEDIA_TYPE_UNKNOWN:
        default:
        {
            // ASSUMING ATLEAST ONE MPEG SAMPLE ENTRY WILL BE ADDED TO THE
            // BIFS OR OTHER TRACKS

            PVA_FF_MpegSampleEntry *entry = (PVA_FF_MpegSampleEntry*) getSampleEntryAt(0);
            entry->nextSampleSize(size);
            nReturn = 1;
        }
        break;
    }

    return (nReturn);
}
int32
PVA_FF_SampleDescriptionAtom::nextTextSample(uint32 size, uint8 flags, int32 index)
{
    OSCL_UNUSED_ARG(flags);
    int32 nReturn = 0;

    switch (_mediaType)
    {
        case MEDIA_TYPE_TEXT://added for timed text support
        {
            if (index >= 0)
            {
                nReturn = index + 1;
            }
            else
            {
                return -1;//failure
            }
        }
        break;
        default:
        {
            // ASSUMING ATLEAST ONE MPEG SAMPLE ENTRY WILL BE ADDED TO THE
            // BIFS OR OTHER TRACKS

            PVA_FF_MpegSampleEntry *entry = (PVA_FF_MpegSampleEntry*) getSampleEntryAt(0);
            entry->nextSampleSize(size);
            nReturn = 1;
        }
        break;
    }

    return (nReturn);
}

void
PVA_FF_SampleDescriptionAtom::addSampleEntry(PVA_FF_SampleEntry *entry)
{
    entry->setParent(this);

    // Do type checking
    switch (_handlerType)
    {
        case MEDIA_TYPE_TEXT:
        {
            if (entry->getType() == TEXT_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else
            {
                return;
            }
        }
        break;
        case MEDIA_TYPE_AUDIO:
            if (entry->getType() == AUDIO_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else if (entry->getType() == AMR_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else
            {
                return;
            }
            break;
        case MEDIA_TYPE_VISUAL:
            if (entry->getType() == VIDEO_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else if (entry->getType() == H263_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else if (entry->getType() == AVC_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else
            {
//          cerr << "ERROR: Trying to add PVA_FF_SampleEntry of wrong type!" << endl;
                return;
            }
            break;
        case MEDIA_TYPE_OBJECT_DESCRIPTOR:
        case MEDIA_TYPE_CLOCK_REFERENCE:
        case MEDIA_TYPE_SCENE_DESCRIPTION:
        case MEDIA_TYPE_MPEG7:
        case MEDIA_TYPE_OBJECT_CONTENT_INFO:
        case MEDIA_TYPE_IPMP:
        case MEDIA_TYPE_MPEG_J:
            if (entry->getType() == MPEG_SAMPLE_ENTRY)
            {
                _psampleEntryVec->push_back(entry);
            }
            else
            {
//          cerr << "ERROR: Trying to add PVA_FF_SampleEntry of wrong type!" << endl;
                return;
            }
            break;
        default:
            _psampleEntryVec->push_back(entry);
            break;
    }

    _entryCount += 1;
    recomputeSize();
}

void
PVA_FF_SampleDescriptionAtom::insertSampleEntryAt(int32 index, PVA_FF_SampleEntry *entry)
{
    if (index < (int32)_entryCount)  // If index in valid range
    {

        // Do type checking
        switch (_handlerType)
        {
            case MEDIA_TYPE_AUDIO:
                if (entry->getType() == AUDIO_SAMPLE_ENTRY)
                {
                    (*_psampleEntryVec)[index] = entry;
                }
                else
                {
//              cerr << "ERROR: Trying to add PVA_FF_SampleEntry of wrong type!" << endl;
                    return;
                }
                break;
            case MEDIA_TYPE_VISUAL:
                if (entry->getType() == VIDEO_SAMPLE_ENTRY)
                {
                    (*_psampleEntryVec)[index] = entry;
                }
                else
                {
//              cerr << "ERROR: Trying to add PVA_FF_SampleEntry of wrong type!" << endl;
                    return;
                }
                break;
            default:
                (*_psampleEntryVec)[index] = entry;
                break;
        }

        _entryCount += 1;
        recomputeSize();
    }

}

void
PVA_FF_SampleDescriptionAtom::replaceSampleEntryAt(int32 index, PVA_FF_SampleEntry *entry)
{

    if (index < (int32)_entryCount)  // If index in valid range
    {

        // Do type checking
        switch (_handlerType)
        {
            case MEDIA_TYPE_AUDIO:
                if (entry->getType() == AUDIO_SAMPLE_ENTRY)
                {
                    //_psampleEntryVec->erase(index); /SKK
                    (*_psampleEntryVec)[index] = entry;
                }
                else
                {
//              cerr << "ERROR: Trying to add PVA_FF_SampleEntry of wrong type!" << endl;
                    return;
                }
                break;
            case MEDIA_TYPE_VISUAL:
                if (entry->getType() == VIDEO_SAMPLE_ENTRY)
                {
                    //_psampleEntryVec->erase(index); //SKK
                    (*_psampleEntryVec)[index] = entry;
                }
                else
                {
//              cerr << "ERROR: Trying to add PVA_FF_SampleEntry of wrong type!" << endl;
                    return;
                }
                break;
            default:
            {
                //_psampleEntryVec->erase(index); //SKK
                (*_psampleEntryVec)[index] = entry;
            }
            break;
        }
    }
}

const PVA_FF_SampleEntry*
PVA_FF_SampleDescriptionAtom::getSampleEntryAt(int32 index) const
{
    if (index < (int32)_entryCount)
    {
        return (*_psampleEntryVec)[index];
    }
    else
    {
        return NULL;
    }
}

PVA_FF_SampleEntry*
PVA_FF_SampleDescriptionAtom::getMutableSampleEntryAt(int32 index)
{
    if (index < (int32)_entryCount)
    {
        return (*_psampleEntryVec)[index];
    }
    else
    {
        return NULL;
    }
}

// Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
bool
PVA_FF_SampleDescriptionAtom::renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp)
{
    int32 rendered = 0;

    if (!renderAtomBaseMembers(fp))
    {
        return false;
    }
    rendered += getDefaultSize();

    if (!PVA_FF_AtomUtils::render32(fp, getEntryCount()))
    {
        return false;
    }
    rendered += 4;

    for (int32 i = 0; i < (int32)(getEntryCount()); i++)
    {
        if ((uint32) _mediaType == MEDIA_TYPE_AUDIO)
        {
            PVA_FF_AudioSampleEntry *entry =
                (PVA_FF_AudioSampleEntry *)(*_psampleEntryVec)[i];
            if (entry != NULL)
            {
                entry->setTimeScale(_mediaTimeScale);
            }
        }
        else if ((uint32) _mediaType == MEDIA_TYPE_VISUAL)
        {
            if (_codecType == CODEC_TYPE_BASELINE_H263_VIDEO)
            {
                if (_o3GPPCompliant)
                {
                    PVA_FF_H263SampleEntry *entry =
                        (PVA_FF_H263SampleEntry *)((*_psampleEntryVec)[i]);
                    entry->setVideoParams(_frame_width, _frame_height);

                    PVA_FF_H263SpecficAtom *h263SpecificAtom =
                        entry->get3GPPDecoderSpecificInfo();

                    h263SpecificAtom->setCodecProfile(_h263Profile);
                    h263SpecificAtom->setCodecLevel(_h263Level);
                }
                else
                {
                    PVA_FF_VisualSampleEntry *entry =
                        (PVA_FF_VisualSampleEntry *)((*_psampleEntryVec)[i]);

                    PVA_FF_H263DecoderSpecificInfo *pH263info =
                        (PVA_FF_H263DecoderSpecificInfo *)(entry->getDecoderSpecificInfo());

                    pH263info->setMaxWidth(_frame_width);
                    pH263info->setMaxHeight(_frame_height);
                    pH263info->setCodecProfile(_h263Profile);
                    pH263info->setCodecLevel(_h263Level);
                }
            }
            else if (_codecType == CODEC_TYPE_AVC_VIDEO)
            {
                PVA_FF_AVCSampleEntry *entry =
                    (PVA_FF_AVCSampleEntry *)((*_psampleEntryVec)[i]);
                entry->setVideoParam(_frame_width, _frame_height);
            }
            else
            {
                PVA_FF_VisualSampleEntry *entry =
                    (PVA_FF_VisualSampleEntry *)((*_psampleEntryVec)[i]);
                entry->setVideoParams(_frame_width, _frame_height);
            }
        }

        if (!((*_psampleEntryVec)[i]->renderToFileStream(fp)))
        {
            return false;
        }
        rendered += (*_psampleEntryVec)[i]->getSize();
    }

    return true;
}

void
PVA_FF_SampleDescriptionAtom::recomputeSize()
{
    int32 size = getDefaultSize();
    size += 4; // For _entryCount

    for (int32 i = 0; i < (int32)(getEntryCount()); i++)
    {
        size += (*_psampleEntryVec)[i]->getSize();
    }

    _size = size;

    // Update the size of the parent atom
    if (_pparent != NULL)
    {
        _pparent->recomputeSize();
    }
}

