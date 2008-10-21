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
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET_MIO_FILE_H_INCLUDED
#include "test_pv_player_engine_testset_mio_file.h"
#endif

PVPlayerTestMioFactory* PVPlayerTestMioFactory::Create()
{
    return new PVPlayerTestFileOutputMioFactory();
}

PvmiMIOControl* PVPlayerTestFileOutputMioFactory::CreateAudioOutput(OsclAny* aParam)
{
    return new PVRefFileOutput(((OSCL_wString*)aParam)->get_cstr());
}

PvmiMIOControl* PVPlayerTestFileOutputMioFactory::CreateAudioOutput(
    OsclAny* aParam,
    PVRefFileOutputTestObserver* aObserver,
    bool aActiveTiming,
    uint32 aQueueLimit,
    bool aSimFlowControl,
    bool logStrings)
{
    return new PVRefFileOutput(
               *(OSCL_wString*)aParam,
               aObserver,
               aActiveTiming,
               aQueueLimit,
               aSimFlowControl,
               logStrings);
}

void PVPlayerTestFileOutputMioFactory::DestroyAudioOutput(PvmiMIOControl* aMio)
{
    PVRefFileOutput* mio = (PVRefFileOutput*)aMio;
    delete mio;
}

PvmiMIOControl* PVPlayerTestFileOutputMioFactory::CreateVideoOutput(OsclAny* aParam)
{
    return new PVRefFileOutput(((OSCL_wString*)aParam)->get_cstr());
}

PvmiMIOControl* PVPlayerTestFileOutputMioFactory::CreateVideoOutput(
    OsclAny* aParam,
    PVRefFileOutputTestObserver* aObserver,
    bool aActiveTiming,
    uint32 aQueueLimit,
    bool aSimFlowControl,
    bool logStrings)
{
    return new PVRefFileOutput(
               *(OSCL_wString*)aParam,
               aObserver,
               aActiveTiming,
               aQueueLimit,
               aSimFlowControl,
               logStrings);
}

void PVPlayerTestFileOutputMioFactory::DestroyVideoOutput(PvmiMIOControl* aMio)
{
    PVRefFileOutput* mio = (PVRefFileOutput*)aMio;
    delete mio;
}

PvmiMIOControl* PVPlayerTestFileOutputMioFactory::CreateTextOutput(OsclAny* aParam)
{
    return new PVRefFileOutput(((OSCL_wString*)aParam)->get_cstr());
}

void PVPlayerTestFileOutputMioFactory::DestroyTextOutput(PvmiMIOControl* aMio)
{
    PVRefFileOutput* mio = (PVRefFileOutput*)aMio;
    delete mio;
}




