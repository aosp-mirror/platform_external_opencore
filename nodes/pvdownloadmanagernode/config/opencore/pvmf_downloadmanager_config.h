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
#ifndef PVMF_DOWNLOADMANAGER_CONFIG_H_INCLUDED
#define PVMF_DOWNLOADMANAGER_CONFIG_H_INCLUDED


/*!
** This file contains a set of 0/1 flags to be used to disable/enable
** various features in download manager.
*/

/*!
** Support PVX download
*/

/*!
** Support progressive playback.
*/
#ifndef PVMF_DOWNLOADMANAGER_SUPPORT_PPB
#define PVMF_DOWNLOADMANAGER_SUPPORT_PPB   1
#endif

/*!
** Support License Acquire interface.  This allows acquiring a license
** during playback of protected content.
*/
#ifndef PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE
#define PVMF_DOWNLOADMANAGER_SUPPORT_CPM_GETLICENSE   1
#endif

/*!
** A tunable parameter setting the number of TCP buffers to
** use for progressive playback.
*/
#ifndef PVMF_DOWNLOADMANAGER_MIN_TCP_BUFFERS_FOR_PPB
#define PVMF_DOWNLOADMANAGER_MIN_TCP_BUFFERS_FOR_PPB  8
#endif

#endif // PVMF_DOWNLOADMANAGER_CONFIG_H_INCLUDED

