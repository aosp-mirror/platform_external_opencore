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


#define REGISTER_OMX_AVC_COMPONENT 1

#define REGISTER_OMX_M4V_COMPONENT 1

#define REGISTER_OMX_H263_COMPONENT 1

#ifndef REGISTER_OMX_WMV_COMPONENT
#if PV_USE_VALUE_ADD
#define REGISTER_OMX_WMV_COMPONENT 1
#else
#define REGISTER_OMX_WMV_COMPONENT 0
#endif
#endif

#define REGISTER_OMX_AAC_COMPONENT 1

#define REGISTER_OMX_AMR_COMPONENT 1

#define REGISTER_OMX_MP3_COMPONENT 1
