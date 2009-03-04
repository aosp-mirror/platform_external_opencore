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
    This PVA_FF_Atom Class is the base class for all other Atoms in the MPEG-4 File
    Format.
*/


#ifndef __BIFS_H__
#define __BIFS_H__

#define NEW_BIFS

#ifdef NEW_BIFS

static const int32 VIDEO_BIFS_SCENE_SIZE = 11;
static const uint8 VIDEO_BIFS_SCENE[] = {0xC0, 0x10, 0x12, 0x61,
                                        0x04, 0x88, 0x50, 0x45,
                                        0x00, 0xBF, 0x00
                                        };

static const int32 AUDIO_BIFS_SCENE_SIZE = 8;
static const uint8 AUDIO_BIFS_SCENE[] = {0xC0, 0x10, 0x12, 0x81,
                                        0x30, 0x2A, 0x01, 0x7C
                                        };

static const int32 AUDIO_VIDEO_BIFS_SCENE_SIZE = 16;

static const uint8 AUDIO_VIDEO_BIFS_SCENE[] = {0xC0, 0x10, 0x12, 0x81,
        0x30, 0x2A, 0x01, 0x72,
        0x61, 0x04, 0x88, 0x50,
        0x45, 0x00, 0xFF, 0x00
                                              };
#else

static const int32 VIDEO_BIFS_SCENE_SIZE = 26;
static const uint8 VIDEO_BIFS_SCENE[] = { 0x0,  0x0,  0x0,  0x0,
                                        0x0,  0x0,  0x0,  0x12,
                                        0xc0, 0x4,  0x24, 0xc0,
                                        0x85, 0x4,  0x50, 0xa,
                                        0x57, 0xfe, 0x0,  0x0,
                                        0x0,  0x0,  0x0,  0x0,
                                        0x1f, 0x00
                                        };

static const int32 AUDIO_BIFS_SCENE_SIZE = 25;
static const uint8 AUDIO_BIFS_SCENE[] = { 0x0,  0x0,  0x0,  0x0,
                                        0x0,  0x0,  0x0,  0x11,
                                        0xc0, 0x4,  0x25, 0x2,
                                        0x60, 0x54, 0x2,  0xa5,
                                        0xff, 0x80, 0x0,  0x0,
                                        0x0,  0x0,  0x0,  0x7,
                                        0x80
                                        };

static const int32 AUDIO_VIDEO_BIFS_SCENE_SIZE = 39;
static const uint8 AUDIO_VIDEO_BIFS_SCENE[] = { 0x0,  0x0,  0x0,  0x0,
        0x0,  0x0,  0x0,  0x1f,
        0xc0, 0x4,  0x25, 0x2,
        0x60, 0x54, 0x2,  0xa5,
        0xff, 0x80, 0x0,  0x0,
        0x0,  0x0,  0x0,  0x6,
        0x4c, 0x8,  0x50, 0x45,
        0x0,  0xe5, 0x7f, 0xe0,
        0x0,  0x0,  0x0,  0x0,
        0x0,  0x1,  0xf0
                                              };
#endif

static const int32 BIFS_DECODER_INFO_SIZE = 2;
static const uint8 BIFS_DECODER_INFO[] = { 0x20, 0x20 };



#endif

