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
#ifndef PV_PLAYER_TEST_MIO_FACTORY_H_INCLUDED
# define PV_PLAYER_TEST_MIO_FACTORY_H_INCLUDED

// for PvmiMIOControl:
#include "pvmi_mio_control.h"
// for PVRefFileOutputTestObserver:
#include "pvmi_media_io_fileoutput.h"
/*!
** PVPlayerTestMioFactory: MIO Factory functions
*/


class PVPlayerTestMioFactory
{
    public:
        static PvmiMIOControl* CreateAudioOutput(OsclAny* aParam);
        static PvmiMIOControl* CreateAudioOutput(OsclAny* aParam, PVRefFileOutputTestObserver* aObserver, bool aActiveTiming, uint32 aQueueLimit, bool aSimFlowControl, bool logStrings = true);
        static void DestroyAudioOutput(PvmiMIOControl* aMio);
        static PvmiMIOControl* CreateVideoOutput(OsclAny* aParam);
        static PvmiMIOControl* CreateVideoOutput(OsclAny* aParam, PVRefFileOutputTestObserver* aObserver, bool aActiveTiming, uint32 aQueueLimit, bool aSimFlowControl, bool logStrings = true);
        static void DestroyVideoOutput(PvmiMIOControl* aMio);
        static PvmiMIOControl* CreateTextOutput(OsclAny* aParam);
        static void DestroyTextOutput(PvmiMIOControl* aMio);

        static void DeviceInit(OsclAny* aParam);
        static void DeviceDeInit(OsclAny* aParam);
};

#endif  // include guard

