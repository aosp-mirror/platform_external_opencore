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

#include "oscl_file_stats.h"
#include "oscl_tickcount.h"
#include "pvlogger.h"

OsclFileStats::OsclFileStats(Oscl_File* c): iContainer(c)
{
    oscl_memset(iStats, 0, sizeof(iStats));
}

void OsclFileStats::Start(uint32& aTicks)
{
    aTicks = OsclTickCount::TickCount();
}

void OsclFileStats::End(TOsclFileOp aOp, uint32 aStart, uint32 aParam, uint32 aParam2)
{
    uint32 delta = OsclTickCount::TickCount() - aStart;
    if (iStats[aOp].iOpCount == 0
            || delta > iStats[aOp].iTotalTicks)
    {
        iStats[aOp].iStartTick = aStart;
        iStats[aOp].iTotalTicks = delta;
        iStats[aOp].iParam = aParam;
        iStats[aOp].iParam2 = aParam2;
    }
    iStats[aOp].iOpCount++;
}


void OsclFileStats::Log(TOsclFileOp aOp, PVLogger* aLogger, uint32 aLogLevel)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, aLogger, aLogLevel
                    , (0, "OsclFileStats(0x%x): Op %s Count %d, Max: Param %d %d StartTick %d TotalTick %d"
                       , iContainer, TOsclFileOpStr[aOp]
                       , iStats[aOp].iOpCount, iStats[aOp].iParam, iStats[aOp].iParam2, iStats[aOp].iStartTick, iStats[aOp].iTotalTicks
                      ));
    OSCL_UNUSED_ARG(aOp);
    OSCL_UNUSED_ARG(aLogLevel);
}

void OsclFileStats::LogAll(PVLogger* aLogger, uint32 aLogLevel)
{
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, aLogger, aLogLevel
                    , (0, "OsclFileStats(0x%x): +++++++++++++++++++++ Begin", iContainer));
    for (uint32 op = 0;op < EOsclFileOp_Last;op++)
    {
        if (iStats[op].iOpCount > 0)
        {
            Log((TOsclFileOp)op, aLogger, aLogLevel);
        }
    }
    //clear all stats once they've been dumped.
    oscl_memset(iStats, 0, sizeof(iStats));
    PVLOGGER_LOGMSG(PVLOGMSG_INST_LLDBG, aLogger, aLogLevel
                    , (0, "OsclFileStats(0x%x): +++++++++++++++++++++ End", iContainer));
}

