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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//               O S C L _ C L O C K   C L A S S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include "oscl_clock.h"

OSCL_EXPORT_REF OsclClock::OsclClock() :
        iState(STOPPED),
        iClockTimebase(NULL)
{
    iLatestRunningClockTime = 0;
    iLatestRunningTimebaseTime = 0;
    iStartClockTime = 0;
    iPauseClockTime = 0;
    iLastAdjustTimebaseTime = 0;
    iAdjustmentTimebaseTime = 0;
}


OSCL_EXPORT_REF OsclClock::~OsclClock()
{
}


OSCL_EXPORT_REF bool OsclClock::SetClockTimebase(OsclTimebase& aTimebase)
{
    // Clock timebase can only be set during stopped or paused states
    if (iState == RUNNING)
    {
        return false;
    }

    // Save the clock timebase object pointer
    iClockTimebase = &aTimebase;

    //If this is a counting timebase, then set the timebase
    //observer to this clock, so that we'll get the count update
    //notices.
    if (aTimebase.GetCountTimebase())
        aTimebase.GetCountTimebase()->SetClockObserver(this);

    //Notify observers that the timebase is updated.
    ClockTimebaseUpdated();

    return true;
}


OSCL_EXPORT_REF bool OsclClock::Start()
{
    // Can only start from stopped or paused states
    if (iState == RUNNING)
    {
        return false;
    }

    // Retrieve the current time value from the clock timebase
    uint64 tbval = 0;
    if (iClockTimebase != NULL)
    {
        iClockTimebase->GetCurrentTime64(tbval, OSCLCLOCK_USEC);
    }

    // Save the clock timebase value to the appropriate
    // variable and update the iLatest... values.
    if (iState == STOPPED)
    {
        // Starting from stop
        UpdateLatestTimes(iStartClockTime, tbval);
    }
    else
    {
        // Resuming from pause
        UpdateLatestTimes(iPauseClockTime, tbval);
    }

    // Change to running state
    SetClockState(RUNNING);
    return true;
}


OSCL_EXPORT_REF bool OsclClock::Pause()
{
    // Can only pause during running state
    if (iState != RUNNING)
    {
        return false;
    }

    // Save the current time
    uint64 tbval = 0;
    GetCurrentTime64(iPauseClockTime, OSCLCLOCK_USEC, tbval);
    UpdateLatestTimes(iPauseClockTime, tbval);

    // Change to paused state
    SetClockState(PAUSED);
    return true;
}


OSCL_EXPORT_REF bool OsclClock::Stop()
{
    // Can only stop when running or paused
    if (iState == STOPPED)
    {
        return false;
    }

    // Reset the time values
    uint64 tmp = 0;
    UpdateLatestTimes(tmp, tmp);
    iStartClockTime = tmp;
    iPauseClockTime = tmp;
    iLastAdjustTimebaseTime = tmp;
    iAdjustmentTimebaseTime = tmp;

    // Change to stopped state
    SetClockState(STOPPED);
    return true;
}


OSCL_EXPORT_REF bool OsclClock::SetStartTime32(uint32& aTime, OsclClock_TimeUnits aUnits)
{
    // Use the 64bit version to set the start time
    uint64 time64 = 0;
    Oscl_Int64_Utils::set_uint64(time64, 0, aTime);
    return SetStartTime64(time64, aUnits);
}


OSCL_EXPORT_REF void OsclClock::GetStartTime32(uint32& aTime, bool& aOverflow, OsclClock_TimeUnits aUnits)
{
    // Use the 64-bit version to get the start time
    // Check for value overflow as well
    uint64 time64 = 0;
    GetStartTime64(time64, aUnits);
    aTime = Oscl_Int64_Utils::get_uint64_lower32(time64);
    aOverflow = (Oscl_Int64_Utils::get_uint64_upper32(time64) > 0 ? true : false);
}


OSCL_EXPORT_REF bool OsclClock::SetStartTime64(uint64& aTime, OsclClock_TimeUnits aUnits)
{
    // Can only set start time while stopped
    if (iState != STOPPED)
    {
        return false;
    }

    // Convert to usec and set the start time
    ToUSec(aTime, aUnits, iStartClockTime);

    return true;
}


OSCL_EXPORT_REF void OsclClock::GetStartTime64(uint64& aTime, OsclClock_TimeUnits aUnits)
{
    // Convert to the requested time units and return the start time
    FromUSec(iStartClockTime, aTime, aUnits);
}


OSCL_EXPORT_REF bool OsclClock::AdjustClockTime32(uint32& aClockTime, uint64& aTimebaseTime, uint32& aAdjustedTime, OsclClock_TimeUnits aUnits)
{
    // Use the 64bit version to adjust the clock
    uint64 clocktime64 = 0;
    Oscl_Int64_Utils::set_uint64(clocktime64, 0, aClockTime);
    uint64 adjusttime64 = 0;
    Oscl_Int64_Utils::set_uint64(adjusttime64, 0, aAdjustedTime);
    return AdjustClockTime64(clocktime64, aTimebaseTime, adjusttime64, aUnits);
}


OSCL_EXPORT_REF bool OsclClock::AdjustClockTime64(uint64& aClockTime, uint64& aTimebaseTime, uint64& aAdjustedTime, OsclClock_TimeUnits aUnits)
{
    // Clock can only be adjusted when it is running
    if (iState != RUNNING)
    {
        return false;
    }

    // Check if the adjustment's observed time is later than the last one
    if (aTimebaseTime < iLastAdjustTimebaseTime)
    {
        // Old data so don't use it for adjustment
        return false;
    }

    // Convert the observed clock and adjustment time to usec so it can be compared
    uint64 adjusttime, clocktime;
    ToUSec(aClockTime, aUnits, clocktime);
    ToUSec(aAdjustedTime, aUnits, adjusttime);

    // Make sure the adjustment's clock and timebase times are before current time
    uint64 currenttime = 0;
    uint64 tbval = 0;

    // Get the current timebase time
    if (iClockTimebase)
    {
        iClockTimebase->GetCurrentTime64(tbval, OSCLCLOCK_USEC);
    }
    // Get the current clock time
    GetCurrentTime64(currenttime, OSCLCLOCK_USEC);

    if (aTimebaseTime > tbval)
    {
        // Observed timebase time cannot be later than the current timebase time
        return false;
    }

    if (clocktime > currenttime)
    {
        // Observed clock time cannot be later than the current clock time
        return false;
    }

    // Make the adjustment
    return AdjustClock(clocktime, aTimebaseTime, adjusttime, currenttime, tbval);
}


OSCL_EXPORT_REF void OsclClock::GetTimebaseResolution(uint32& aResolution)
{
    if (iClockTimebase)
    {
        // Retrieve the resolution from the timebase being used
        iClockTimebase->GetTimebaseResolution(aResolution);
    }
    else
    {
        // No timebase so set to 0
        aResolution = 0;
    }
}


OSCL_EXPORT_REF void OsclClock::GetCurrentTime64(uint64& aClockTime, OsclClock_TimeUnits aUnits)
{
    // Use the base class version
    OsclTimebase::GetCurrentTime64(aClockTime, aUnits);
}


OSCL_EXPORT_REF void OsclClock::GetCurrentTime64(uint64& aClockTime, OsclClock_TimeUnits aUnits, uint64& aTimebaseTime)
{
    // Get and return the current timebase value
    uint64 tbval = 0;
    if (iClockTimebase)
    {
        iClockTimebase->GetCurrentTime64(tbval, OSCLCLOCK_USEC);
    }
    aTimebaseTime = tbval;

    // Determine and return the current clock time
    if (iState == STOPPED)
    {
        // Return the specified start time
        FromUSec(iStartClockTime, aClockTime, aUnits);
    }
    else if (iState == PAUSED)
    {
        // Returned the paused time
        FromUSec(iPauseClockTime, aClockTime, aUnits);
    }
    else
    {
        // Running state
        uint64 currenttime;

        // Determine current clock time including any adjustment
        GetAdjustedRunningClockTime(currenttime, tbval);

        // Convert to requested time units
        FromUSec(currenttime, aClockTime, aUnits);
    }
}

OSCL_EXPORT_REF void OsclClock::SetClockObserver(OsclClockObserver& aObserver)
{
    iClockObservers.push_back(&aObserver);
}

OSCL_EXPORT_REF void OsclClock::RemoveClockObserver(OsclClockObserver& aObserver)
{
    for (uint32 i = 0;i < iClockObservers.size();i++)
    {
        if (iClockObservers[i] == &aObserver)
            iClockObservers.erase(&iClockObservers[i]);
    }
}

OSCL_EXPORT_REF void OsclClock::SetClockStateObserver(OsclClockStateObserver& aObserver)
{
    iClockStateObservers.push_back(&aObserver);
}

OSCL_EXPORT_REF void OsclClock::RemoveClockStateObserver(OsclClockStateObserver& aObserver)
{
    for (uint32 i = 0;i < iClockStateObservers.size();i++)
    {
        if (iClockStateObservers[i] == &aObserver)
            iClockStateObservers.erase(&iClockStateObservers[i]);
    }
}

void OsclClock::SetClockState(OsclClockState aState)
{
    // Change the clock state
    iState = aState;
    // Notify observers
    for (uint32 i = 0;i < iClockStateObservers.size();i++)
        iClockStateObservers[i]->ClockStateUpdated();
}

void OsclClock::UpdateLatestTimes(uint64& aTime, uint64& aTimebaseVal)
{
    // Set the latest time values
    iLatestRunningClockTime = aTime;
    iLatestRunningTimebaseTime = aTimebaseVal;
}

#define OSCL_DISABLE_WARNING_CONV_POSSIBLE_LOSS_OF_DATA
#include "osclconfig_compiler_warnings.h"

void OsclClock::ToUSec(uint64& aSrcVal, OsclClock_TimeUnits aSrcUnits, uint64& aUSecVal)
{
    uint64 multconst = UINT64_HILO(0, 1);

    // Determine the multiplier constant for the specified units
    switch (aSrcUnits)
    {
        case OSCLCLOCK_MSEC:
            multconst = UINT64_HILO(0, 1000);
            break;

        case OSCLCLOCK_SEC:
            multconst = UINT64_HILO(0, 1000000);
            break;

        case OSCLCLOCK_MIN:
            multconst = UINT64_HILO(0, 60000000);
            break;

        case OSCLCLOCK_HOUR:
            multconst = UINT64_HILO(0, OSCL_UNSIGNED_CONST(3600000000));
            break;

        case OSCLCLOCK_DAY:
            multconst = UINT64_HILO(0x14, 0x1DD76000);
            break;

        case OSCLCLOCK_USEC:
        default:
            break;
    }

    // Convert value to usec
    if (multconst != (UINT64_HILO(0, 1)))
    {
        aUSecVal = aSrcVal * multconst;
    }
    else
    {
        aUSecVal = aSrcVal;
    }
}


void OsclClock::FromUSec(uint64& aUSecVal, uint64& aDstVal, OsclClock_TimeUnits aDstUnits)
{
    uint64 divconst = UINT64_HILO(0, 1);

    // Determine the divider constant for the specified units
    switch (aDstUnits)
    {
        case OSCLCLOCK_MSEC:
            divconst = UINT64_HILO(0, 1000);
            break;

        case OSCLCLOCK_SEC:
            divconst = UINT64_HILO(0, 1000000);
            break;

        case OSCLCLOCK_MIN:
            divconst = UINT64_HILO(0, 60000000);
            break;

        case OSCLCLOCK_HOUR:
            divconst = UINT64_HILO(0, OSCL_UNSIGNED_CONST(3600000000));
            break;

        case OSCLCLOCK_DAY:
            divconst = UINT64_HILO(0x14, 0x1DD76000);
            break;

        case OSCLCLOCK_USEC:
        default:
            break;
    }

    // Convert usec val to specified units
    if (divconst != (UINT64_HILO(0, 1)))
    {
        aDstVal = aUSecVal / divconst;
    }
    else
    {
        aDstVal = aUSecVal;
    }
}


bool OsclClock::AdjustClock(uint64& aObsTime, uint64& aObsTimebase, uint64& aAdjTime,
                            uint64& aCurrentTime, uint64& aCurrentTimebase)
{
    // In this implementation, don't allow adjustments to be made with
    // data older than when the last adjustment was made
    if (aObsTimebase < iAdjustmentTimebaseTime)
    {
        return false;
    }

    // Make the adjustment
    if (aAdjTime > aObsTime)
    {
        // Adjusted time is ahead so move ahead

        // Save the observed timebase time of the adjusted time
        iLastAdjustTimebaseTime = aObsTimebase;
        UpdateLatestTimes(aAdjTime, aObsTimebase);

        // Set the latest adjustment time as the current timebase time
        iAdjustmentTimebaseTime = aCurrentTimebase;
    }
    else if (aAdjTime < aObsTime)
    {
        // Adjusted time is before the current time

        // Save the observed timebase time of the adjusted time
        iLastAdjustTimebaseTime = aObsTimebase;

        // Set the latest clock time to the current clock time
        // Set the latest timebase time to (current timebase time) + ((observed clock time) - (adjusted time))
        uint64 offsettbtime = 0;
        offsettbtime = aCurrentTimebase;
        offsettbtime += (aObsTime - aAdjTime);
        UpdateLatestTimes(aCurrentTime, offsettbtime);

        // Set the latest adjustment time to the current timebase time without offset
        // to allow the adjustment even before previous adjustment delta has elapsed.
        iAdjustmentTimebaseTime = aCurrentTimebase;
    }
    else
    {
        // Since there is no adjustment, do nothing
    }

    ClockAdjusted();
    return true;
}


void OsclClock::GetAdjustedRunningClockTime(uint64& aDstTime, uint64& aTimebaseVal)
{
    // Current time is (latest clock time)+(current timebase time - latest timebase time)
    aDstTime = iLatestRunningClockTime;

    // If backward adjustment occurs, iLatestRunningTimebaseTime might be greater than
    // the current value. To avoid negative values and clock going back, only
    // add the diff if current timebase value is greater. This makes the clock "freeze" until
    // the difference has elapsed
    if (aTimebaseVal > iLatestRunningTimebaseTime)
    {
        aDstTime += (aTimebaseVal - iLatestRunningTimebaseTime);
    }
}

void OsclClock::ClockCountUpdated()
{
    //notify all observers that the clock count was updated.
    for (uint32 i = 0;i < iClockObservers.size();i++)
        iClockObservers[i]->ClockCountUpdated();
}

void OsclClock::ClockAdjusted()
{
    //notify all observers that the clock was adjusted
    for (uint32 i = 0;i < iClockObservers.size();i++)
        iClockObservers[i]->ClockAdjusted();
}

void OsclClock::ClockTimebaseUpdated()
{
    //notify all observers that the clock timebase was updated.
    for (uint32 i = 0;i < iClockObservers.size();i++)
    {
        OsclClockObserver* obs = iClockObservers[i];
        obs->ClockTimebaseUpdated();
    }
    //reset timebase history.
    iLastAdjustTimebaseTime = 0;
    iAdjustmentTimebaseTime = 0;
}


