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
/**
 * @file pvmf_jb_inactivity_timer.cpp
 * @brief Inactivity timer to Jitter Buffer Node
 */
#ifndef PVMF_JB_INACTIVITY_TIMER_H_INCLUDED
#include "pvmf_jb_inactivity_timer.h"
#endif
#ifndef PVMF_JITTER_BUFFER_INTERNAL_H_INCLUDED
#include "pvmf_jitter_buffer_internal.h"
#endif

////////////////////////////////////////////////////////////////////////////
PvmfJBInactivityTimer::PvmfJBInactivityTimer(PvmfJBInactivityTimerObserver* aObserver)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmfJBInactivityTimer"),
        iInactivityDurationInMS(DEFAULT_MAX_INACTIVITY_DURATION_IN_MS),
        iObserver(aObserver),
        iStarted(false)
{
    iLogger = PVLogger::GetLoggerObject("PvmfJBInactivityTimer");
    AddToScheduler();
}

////////////////////////////////////////////////////////////////////////////
PvmfJBInactivityTimer::~PvmfJBInactivityTimer()
{
    Stop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBInactivityTimer::Start()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBInactivityTimer::Start"));
    if (iInactivityDurationInMS > 0)
    {
        RunIfNotReady(iInactivityDurationInMS*1000);
        iStarted = true;
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBInactivityTimer::setMaxInactivityDurationInMS(uint32 duration)
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBInactivityTimer::setMaxInactivityDurationInMS"));
    iInactivityDurationInMS = duration;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBInactivityTimer::Stop()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBInactivityTimer::Stop"));
    Cancel();
    iStarted = false;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmfJBInactivityTimer::Run()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBInactivityTimer::Run"));

    if (!iStarted)
        return;

    if (!iObserver)
    {
        PVMF_JBNODE_LOGERROR((0, "PvmfJBInactivityTimer::Run: Error - Observer not set"));
        return;
    }

    iObserver->PVMFJBInactivityTimerEvent();
    /*
     * Do not reschudule the AO here. Observer would reschedule this AO
     * once it is done processing the timer event.
     */
}

////////////////////////////////////////////////////////////////////////////
PvmfJBSessionDurationTimer::PvmfJBSessionDurationTimer(PvmfJBSessionDurationTimerObserver* aObserver)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmfJBSessionDurationTimer"),
        iCurrentMonitoringIntervalInMS(0),
        iSessionDurationInMS(0),
        iElapsedSessionDurationInMS(0),
        iObserver(aObserver),
        iStarted(false),
        iTimerStartTimeInMS(0),
        iMonitoringIntervalElapsed(0),
        iEstimatedServerClock(NULL),
        iEstimatedServClockValAtLastCancel(0),
        iExpectedEstimatedServClockValAtSessionEnd(0)
{
    iLogger = PVLogger::GetLoggerObject("PvmfJBSessionDurationTimer");
    iClockLoggerSessionDuration = PVLogger::GetLoggerObject("clock.streaming_manager.sessionduration");
    AddToScheduler();
    iClock.SetClockTimebase(iClockTimeBase);
}


////////////////////////////////////////////////////////////////////////////
PvmfJBSessionDurationTimer::~PvmfJBSessionDurationTimer()
{
    Stop();
    iEstimatedServerClock = NULL;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBSessionDurationTimer::Start()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBSessionDurationTimer::Start"));
    if ((iSessionDurationInMS > 0) && (iCurrentMonitoringIntervalInMS > 0))
    {
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PvmfJBSessionDurationTimer::Start - SessionDurationInMS = %d", iSessionDurationInMS));
        iClock.Start();
        uint64 timebase64 = 0;
        iTimerStartTimeInMS = 0;
        iMonitoringIntervalElapsed = 0;
        iClock.GetCurrentTime64(iTimerStartTimeInMS, OSCLCLOCK_MSEC, timebase64);
        /* Compute expected estimated serv clock value when duration expires */
        if (iEstimatedServerClock != NULL)
        {
            iExpectedEstimatedServClockValAtSessionEnd = iEstimatedServClockValAtLastCancel;
            uint32 currEstServClk32 = Oscl_Int64_Utils::get_uint64_lower32(iExpectedEstimatedServClockValAtSessionEnd);
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PvmfJBSessionDurationTimer::Start - CurrEstServClock  = %d", currEstServClk32));
            uint64 remainingSessionDuration64 = 0;
            Oscl_Int64_Utils::set_uint64(remainingSessionDuration64, 0, (iSessionDurationInMS - iElapsedSessionDurationInMS));
            iExpectedEstimatedServClockValAtSessionEnd += remainingSessionDuration64;
            uint32 eVal32 = Oscl_Int64_Utils::get_uint64_lower32(iExpectedEstimatedServClockValAtSessionEnd);
            PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PvmfJBSessionDurationTimer::Start - ExpectedEstimatedServClockValAtSessionEnd = %d", eVal32));
        }
        RunIfNotReady(iCurrentMonitoringIntervalInMS*1000);
        iStarted = true;
        return PVMFSuccess;
    }
    return PVMFFailure;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBSessionDurationTimer::setSessionDurationInMS(uint32 duration)
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBSessionDurationTimer::setMaxInactivityDurationInMS"));
    iSessionDurationInMS = duration;
    iElapsedSessionDurationInMS = 0;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBSessionDurationTimer::Stop()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBSessionDurationTimer::Stop"));
    OsclTimerObject::Cancel();
    iStarted = false;
    iSessionDurationInMS = 0;
    iClock.Stop();
    iTimerStartTimeInMS = 0;
    iMonitoringIntervalElapsed = 0;
    iExpectedEstimatedServClockValAtSessionEnd = 0;
    iEstimatedServClockValAtLastCancel = 0;
    iElapsedSessionDurationInMS = 0;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBSessionDurationTimer::Cancel()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBSessionDurationTimer::Cancel"));
    iStarted = false;
    uint64 timebase64 = 0;
    uint64 cancelTime = 0;
    iClock.GetCurrentTime64(cancelTime, OSCLCLOCK_MSEC, timebase64);
    iMonitoringIntervalElapsed = (cancelTime - iTimerStartTimeInMS);
    iEstimatedServClockValAtLastCancel = 0;
    if (iEstimatedServerClock != NULL)
    {
        uint64 timebase64 = 0;
        iEstimatedServerClock->GetCurrentTime64(iEstimatedServClockValAtLastCancel, OSCLCLOCK_MSEC, timebase64);
    }
    uint32 eVal32 = Oscl_Int64_Utils::get_uint64_lower32(iEstimatedServClockValAtLastCancel);
    PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PvmfJBSessionDurationTimer::Cancel - EstimatedServClockValAtLastCancel = %d", eVal32));
    iClock.Stop();
    iTimerStartTimeInMS = 0;
    OsclTimerObject::Cancel();
    return PVMFSuccess;
}

void PvmfJBSessionDurationTimer::EstimatedServerClockUpdated()
{
    /*
     * Check if the estimated server clock has reached the expected value,
     * if so, cancel the timer and report session duration complete
     */
    if (iEstimatedServerClock != NULL)
    {
        uint64 timebase64 = 0;
        uint64 estServClock = 0;
        iEstimatedServerClock->GetCurrentTime64(estServClock, OSCLCLOCK_MSEC, timebase64);
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PvmfJBSessionDurationTimer::EstimatedServerClockUpdated - CurrEstServClock = %2d", estServClock));
        PVMF_JBNODE_LOGCLOCK_SESSION_DURATION((0, "PvmfJBSessionDurationTimer::EstimatedServerClockUpdated - ExpectedEstServClock = %2d", iExpectedEstimatedServClockValAtSessionEnd));
        if (estServClock >= iExpectedEstimatedServClockValAtSessionEnd)
        {
            this->Cancel();
            iObserver->PVMFJBSessionDurationTimerEvent();
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void PvmfJBSessionDurationTimer::Run()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBSessionDurationTimer::Run"));

    if (!iStarted)
        return;

    if (!iObserver)
    {
        PVMF_JBNODE_LOGERROR((0, "PvmfJBSessionDurationTimer::Run: Error - Observer not set"));
        return;
    }

    uint64 timebase64 = 0;
    uint64 cancelTime = 0;
    iClock.GetCurrentTime64(cancelTime, OSCLCLOCK_MSEC, timebase64);
    iMonitoringIntervalElapsed = (cancelTime - iTimerStartTimeInMS);
    iClock.Stop();
    iTimerStartTimeInMS = 0;
    iObserver->PVMFJBSessionDurationTimerEvent();
    /*
     * Do not reschudule the AO here. Observer would reschedule this AO
     * once it is done processing the timer event.
     */
}

////////////////////////////////////////////////////////////////////////////
PvmfJBJitterBufferDurationTimer::PvmfJBJitterBufferDurationTimer(PvmfJBJitterBufferDurationTimerObserver* aObserver)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmfJBJitterBufferDurationTimer"),
        iJitterBufferDurationInMS(0),
        iObserver(aObserver),
        iStarted(false)
{
    iLogger = PVLogger::GetLoggerObject("PvmfJBJitterBufferDurationTimer");
    AddToScheduler();
}

////////////////////////////////////////////////////////////////////////////
PvmfJBJitterBufferDurationTimer::~PvmfJBJitterBufferDurationTimer()
{
    Stop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBJitterBufferDurationTimer::Start()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBJitterBufferDurationTimer::Start"));
    if (iJitterBufferDurationInMS > 0)
    {
        RunIfNotReady(iJitterBufferDurationInMS*1000);
        uint32 startTime = 0;

        // setup timer
        iRunClock.Stop();
        bool result = iRunClock.SetStartTime32(startTime, OSCLCLOCK_USEC);
        OSCL_ASSERT(result);
        result = iRunClock.Start();
        OSCL_ASSERT(result);

        iStarted = true;
        return PVMFSuccess;
    }
    else
    {
        return PVMFFailure;
    }
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBJitterBufferDurationTimer::setJitterBufferDurationInMS(uint32 duration)
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBSessionDurationTimer::setJitterBufferDurationInMS"));

    if (iStarted)
    {
        // we're running... we can only DECREASE the jitter buffer value
        if (duration > iJitterBufferDurationInMS)
        {
            PVMF_JBNODE_LOGINFO(
                (0, "PvmfJBSessionDurationTimer::setJitterBufferDurationInMS: "
                 "Attempting to increase timer value while running. %d to %d ms.",
                 iJitterBufferDurationInMS, duration));

            return PVMFFailure;
        }

        // get the current time in us
        uint32 currtime;
        bool overflow = false;
        iRunClock.GetCurrentTime32(currtime, overflow, OSCLCLOCK_USEC);
        OSCL_ASSERT(!overflow);  // if the time elapsed is > 32bits, something is wrong

        // compare against the new duration
        uint32 durationUs = duration * 1000; // save a multiply
        if (durationUs > currtime)
        {
            // set a new Run for the outstanding balance
            Cancel();
            RunIfNotReady(durationUs - currtime);
        }
        else
        {
            // the new duration has already elapsed - schedule a Run immediately
            RunIfNotReady();
        }
    }

    // save the new duration
    iJitterBufferDurationInMS = duration;

    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfJBJitterBufferDurationTimer::Stop()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBJitterBufferDurationTimer::Stop"));
    Cancel();

    if (iStarted)
    {
        bool result = iRunClock.Stop();
        OSCL_UNUSED_ARG(result);
        OSCL_ASSERT(result);
    }

    iStarted = false;
    iJitterBufferDurationInMS = 0;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmfJBJitterBufferDurationTimer::Run()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfJBJitterBufferDurationTimer::Run"));

    if (!iStarted)
        return;

    if (!iObserver)
    {
        PVMF_JBNODE_LOGERROR((0, "PvmfJBJitterBufferDurationTimer::Run: Error - Observer not set"));
        return;
    }

    bool result;
    result = iRunClock.Stop();
    OSCL_ASSERT(result);
    iStarted = false;

    iObserver->PVMFJBJitterBufferDurationTimerEvent();
    /*
     * Do not reschudule the AO here. Observer would reschedule this AO
     * once it is done processing the timer event.
     */
}

////////////////////////////////////////////////////////////////////////////
PvmfFirewallPacketTimer::PvmfFirewallPacketTimer(PvmfFirewallPacketTimerObserver* aObserver)
        : OsclTimerObject(OsclActiveObject::EPriorityNominal, "PvmfFirewallPacketTimer"),
        iFirewallPacketRecvTimeOutInMS(0),
        iObserver(aObserver),
        iStarted(false)
{
    iLogger = PVLogger::GetLoggerObject("PvmfFirewallPacketTimer");
    AddToScheduler();
}

////////////////////////////////////////////////////////////////////////////
PvmfFirewallPacketTimer::~PvmfFirewallPacketTimer()
{
    Stop();
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfFirewallPacketTimer::Start()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfFirewallPacketTimer::Start"));
    iStarted = true;
    if (iFirewallPacketRecvTimeOutInMS > 0)
    {
        RunIfNotReady(iFirewallPacketRecvTimeOutInMS*1000);
    }
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfFirewallPacketTimer::setFirewallPacketRecvTimeOutInMS(uint32 aRecvTimeOut)
{
    PVMF_JBNODE_LOGINFO((0, "PvmfFirewallPacketTimer::setFirewallPacketRecvTimeOutInMS"));
    iFirewallPacketRecvTimeOutInMS = aRecvTimeOut;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
PVMFStatus PvmfFirewallPacketTimer::Stop()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfFirewallPacketTimer::Stop"));
    Cancel();
    iStarted = false;
    iFirewallPacketRecvTimeOutInMS = 0;
    return PVMFSuccess;
}

////////////////////////////////////////////////////////////////////////////
void PvmfFirewallPacketTimer::Run()
{
    PVMF_JBNODE_LOGINFO((0, "PvmfFirewallPacketTimer::Run"));

    if (!iStarted)
        return;

    if (!iObserver)
    {
        PVMF_JBNODE_LOGERROR((0, "PvmfFirewallPacketTimer::Run: Error - Observer not set"));
        return;
    }

    iObserver->PvmfFirewallPacketTimerEvent();
    /*
     * Do not reschudule the AO here. Observer would reschedule this AO
     * once it is done processing the timer event.
     */
}

