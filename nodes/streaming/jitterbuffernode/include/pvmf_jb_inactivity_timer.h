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
#ifndef PVMF_JB_INACTIVITY_TIMER_H_INCLUDED
#define PVMF_JB_INACTIVITY_TIMER_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_CLOCK_H_INCLUDED
#include "oscl_clock.h"
#endif
#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif
#ifndef OSCL_SCHEDULER_AO_H_INCLUDED
#include "oscl_scheduler_ao.h"
#endif
#ifndef PVMF_RETURN_CODES_H_INCLUDED
#include "pvmf_return_codes.h"
#endif
#ifndef PVMF_SM_TUNABLES_H_INCLUDED
#include "pvmf_sm_tunables.h"
#endif

/**
 * Observer class for the inactivity timer AO
 */
class PvmfJBInactivityTimerObserver
{
    public:
        virtual ~PvmfJBInactivityTimerObserver() {}
        /**
         * A timer event, indicating that the inactivity timer has expired.
         */
        virtual void PVMFJBInactivityTimerEvent() = 0;
};

/**
 * Inactivity timer object to Jitter Buffer node. This object generates event
 * on remote inactivity (no UDP traffic from server for a certain time)
 */
class PvmfJBInactivityTimer : public OsclTimerObject
{
    public:
        PvmfJBInactivityTimer(PvmfJBInactivityTimerObserver* aObserver);

        virtual ~PvmfJBInactivityTimer();

        /** Start Timer */
        PVMFStatus Start();

        PVMFStatus setMaxInactivityDurationInMS(uint32 duration);

        uint32 getMaxInactivityDurationInMS()
        {
            return iInactivityDurationInMS;
        }

        /** Stop Timer events */
        PVMFStatus Stop();

        bool IsTimerStarted()
        {
            return iStarted;
        }

    private:
        void Run();

        uint32 iInactivityDurationInMS;
        PvmfJBInactivityTimerObserver* iObserver;
        PVLogger* iLogger;
        bool iStarted;
};


/**
 * Observer class for the session duration timer AO
 */
class PvmfJBSessionDurationTimerObserver
{
    public:
        virtual ~PvmfJBSessionDurationTimerObserver() {}
        /**
         * A timer event, indicating that the timer has expired.
         */
        virtual void PVMFJBSessionDurationTimerEvent() = 0;
};

/**
 * Sessionduration timer object to Jitter Buffer node.
 * This object generates event when the session duration expires
 */
class PvmfJBSessionDurationTimer : public OsclTimerObject
{
    public:
        PvmfJBSessionDurationTimer(PvmfJBSessionDurationTimerObserver* aObserver);

        virtual ~PvmfJBSessionDurationTimer();

        /** Start Timer */
        PVMFStatus Start();

        PVMFStatus setSessionDurationInMS(uint32 duration);

        uint32 getSessionDurationInMS()
        {
            return iSessionDurationInMS;
        }

        /** Stop Timer events */
        PVMFStatus Stop();

        virtual PVMFStatus Cancel();

        bool IsTimerStarted()
        {
            return iStarted;
        }

        void SetEstimatedServerClock(OsclClock* aEstimatedServerClock)
        {
            iEstimatedServerClock = aEstimatedServerClock;
        }

        void EstimatedServerClockUpdated();

        uint64 GetExpectedEstimatedServClockValAtSessionEnd()
        {
            return iExpectedEstimatedServClockValAtSessionEnd;
        }

        void setCurrentMonitoringIntervalInMS(uint32 aCurrentMonitoringIntervalInMS)
        {
            iCurrentMonitoringIntervalInMS = aCurrentMonitoringIntervalInMS;
        }

        uint64 GetMonitoringIntervalElapsed()
        {
            return iMonitoringIntervalElapsed;
        }

        void UpdateElapsedSessionDuration(uint32 aElapsedTime)
        {
            iElapsedSessionDurationInMS += aElapsedTime;
        }

        uint32 GetElapsedSessionDurationInMS()
        {
            return iElapsedSessionDurationInMS;
        }

        uint64 GetEstimatedServClockValAtLastCancel()
        {
            return iEstimatedServClockValAtLastCancel;
        }

        void ResetEstimatedServClockValAtLastCancel()
        {
            iEstimatedServClockValAtLastCancel = 0;
            if (iEstimatedServerClock != NULL)
            {
                uint64 timebase64 = 0;
                iEstimatedServerClock->GetCurrentTime64(iEstimatedServClockValAtLastCancel, OSCLCLOCK_MSEC, timebase64);
            }
        }

    private:
        void Run();

        uint32 iCurrentMonitoringIntervalInMS;
        uint32 iSessionDurationInMS;
        uint32 iElapsedSessionDurationInMS;
        PvmfJBSessionDurationTimerObserver* iObserver;
        PVLogger* iLogger;
        bool iStarted;

        OsclClock iClock;
        OsclTimebase_Tickcount iClockTimeBase;
        uint64 iTimerStartTimeInMS;
        uint64 iMonitoringIntervalElapsed;

        OsclClock* iEstimatedServerClock;
        uint64 iEstimatedServClockValAtLastCancel;
        uint64 iExpectedEstimatedServClockValAtSessionEnd;

        PVLogger *iClockLoggerSessionDuration;
};

/**
 * Observer class for the jitter buffer duration timer AO
 */
class PvmfJBJitterBufferDurationTimerObserver
{
    public:
        virtual ~PvmfJBJitterBufferDurationTimerObserver() {}
        /**
         * A timer event, indicating that the timer has expired.
         */
        virtual void PVMFJBJitterBufferDurationTimerEvent() = 0;
};

/**
 * Bufferingduration timer object to Jitter Buffer node.
 * This object generates event when the jitter buffer duration expires
 */
class PvmfJBJitterBufferDurationTimer : public OsclTimerObject
{
    public:
        PvmfJBJitterBufferDurationTimer(PvmfJBJitterBufferDurationTimerObserver* aObserver);

        virtual ~PvmfJBJitterBufferDurationTimer();

        /** Start Timer */
        PVMFStatus Start();

        PVMFStatus setJitterBufferDurationInMS(uint32 duration);

        uint32 getJitterBufferDurationInMS()
        {
            return iJitterBufferDurationInMS;
        }

        /** Stop Timer events */
        PVMFStatus Stop();

        bool IsTimerStarted()
        {
            return iStarted;
        }

    private:
        void Run();

        uint32 iJitterBufferDurationInMS;
        PvmfJBJitterBufferDurationTimerObserver* iObserver;
        PVLogger* iLogger;
        bool iStarted;

        OsclClock iRunClock;
        OsclTimebase_Tickcount iRunClockTimeBase;
};

/**
 * Observer class for the firewall packet timer AO
 */
class PvmfFirewallPacketTimerObserver
{
    public:
        virtual ~PvmfFirewallPacketTimerObserver() {}
        /**
         * A timer event, indicating that the timer has expired.
         */
        virtual void PvmfFirewallPacketTimerEvent() = 0;
};

/**
 * FirewallPacketTimer object to Jitter Buffer node.
 * This object generates event when the Firewall packet recv timeout expires
 */
class PvmfFirewallPacketTimer : public OsclTimerObject
{
    public:
        PvmfFirewallPacketTimer(PvmfFirewallPacketTimerObserver* aObserver);

        virtual ~PvmfFirewallPacketTimer();

        /** Start Timer */
        PVMFStatus Start();

        PVMFStatus setFirewallPacketRecvTimeOutInMS(uint32 aRecvTimeOut);

        uint32 getFirewallPacketRecvTimeOutInMS()
        {
            return iFirewallPacketRecvTimeOutInMS;
        }

        /** Stop Timer events */
        PVMFStatus Stop();

        bool IsTimerStarted()
        {
            return iStarted;
        }

    private:
        void Run();

        uint32 iFirewallPacketRecvTimeOutInMS;
        PvmfFirewallPacketTimerObserver* iObserver;
        PVLogger* iLogger;
        bool iStarted;
};

#endif // PVMF_JB_INACTIVITY_TIMER_H_INCLUDED




