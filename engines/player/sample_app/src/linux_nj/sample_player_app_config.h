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
#ifndef SAMPLE_PLAYER_APP_CONFIG_H_INCLUDED
#define SAMPLE_PLAYER_APP_CONFIG_H_INCLUDED

#ifndef PVMF_FORMAT_TYPE_H_INCLUDED
#include "pvmf_format_type.h"
#endif

// audio sink output type
#define AUDIOSINK_FORMAT_TYPE PVMF_AMR_IETF
// video sink output type
#define VIDEOSINK_FORMAT_TYPE PVMF_M4V

// The default test file to use
#define DEFAULTSOURCEFILENAME "test.mp4"
#define DEFAULTSOURCEFORMATTYPE PVMF_MPEG4FF


// Set to 1 to use the scheduler native to the system instead of PV scheduler
#define USE_NATIVE_SCHEDULER 0

// The string to prepend to source filenames
#define SOURCENAME_PREPEND_STRING ""
#define SOURCENAME_PREPEND_WSTRING _STRLIT_WCHAR("")

// The string to prepend to output filenames
#define OUTPUTNAME_PREPEND_STRING ""
#define OUTPUTNAME_PREPEND_WSTRING _STRLIT_WCHAR("")

#endif

