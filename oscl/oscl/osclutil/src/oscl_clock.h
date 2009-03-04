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
//				 O S C L _ C L O C K	C L A S S
//	  This file contains a general clock class and utils
//	  needed for multimedia softwares
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclutil OSCL Util
 *
 * @{
 */


/*!
 * \file oscl_clock.h
 * \brief Provides a time clock that can be paused and resumed,
 *	  set the start time, adjusted based on outside source feedback,
 *    and accepts user specified source for the free running clock.
 */

#ifndef OSCL_CLOCK_H_INCLUDED
#define OSCL_CLOCK_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif

#ifndef OSCL_TICKCOUNT_H_INCLUDED
#include "oscl_tickcount.h"
#endif

#ifndef OSCL_INT64_UTILS_H_INCLUDED
#include "oscl_int64_utils.h"
#endif

#ifndef OSCL_VECTOR_H_INCLUDED
#include "oscl_vector.h"
#endif

#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

/*
 * Enum for the time units used in OSCL Clock
 */
enum OsclClock_TimeUnits
{
    OSCLCLOCK_USEC	= 0,
    OSCLCLOCK_MSEC	= 1,
    OSCLCLOCK_SEC	= 2,
    OSCLCLOCK_MIN	= 3,
    OSCLCLOCK_HOUR	= 4,
    OSCLCLOCK_DAY	= 5
};

/**
	OsclClockObserver is an observer class for OsclClock.  Modules
	can optionally register themselves as clock observers.  There
	can be multiple observers for a single clock.
*/
class OsclClockObserver
{
    public:
        /*
        ** This event happens when the timebase for this clock
        ** has been updated
        */
        virtual void ClockTimebaseUpdated() = 0;
        /*
        ** This event happens for counting timebases only, when the
        ** count has been updated.
        */
        virtual void ClockCountUpdated() = 0;
        /*
        ** This event happens when the clock has been adjusted
        */
        virtual void ClockAdjusted() = 0;

        virtual ~OsclClockObserver() {}
};

/**
	OsclClockStateObserver is an observer class for OsclClock.  Modules
	can optionally register themselves as clock state observers.  There
	can be multiple observers for a single clock.
*/
class OsclClockStateObserver
{
    public:
        /*
        ** This event happens when the clock state has changed.
        */
        virtual void ClockStateUpdated() = 0;

        virtual ~OsclClockStateObserver() {}
};

/**
	OsclCountTimebase is an extension to the standard timebase to allow
	controlled stepping rather than continuous flow timebase.
*/
class OsclCountTimebase
{
    public:
        /*
        ** Set a new value for the count.  Will trigger a ClockCountUpdated callback
        ** to all observers of the clock in which this timebase resides.
        ** @param aCount (input): the new count
        */
        virtual void SetCount(int64 aCount) = 0;
        /*
        ** Read current value of the count.
        ** @param aCount (output): the current count
        */
        virtual void GetCount(int64& aCount) = 0;

        virtual ~OsclCountTimebase() {}
    private:
        /*
        ** Each OsclCountTimebase will be contained within an OsclClock
        ** class.  That OsclClock instance will set itself as the observer of
        ** the OsclCountTimebase.  To get notices from the timebase, modules
        ** can register through the SetClockObserver method of the OsclClock.
        */
        friend class OsclClock;
        virtual void SetClockObserver(OsclClockObserver* aObserver) = 0;
};

/**
	OsclTimebase is a base class to obtain the timebase clock time.
	Common source of the timebase clock is the system tickcount which is implemented
	as OsclTimebase_Tickcount further below. OsclTimebase is expected to return the time
	in units of microseconds even if the timebase itself does not have the resolution of microseconds.
*/
class OsclTimebase
{
    public:
        /**
            Virtual destructor for OsclTimebase
        */
        virtual ~OsclTimebase() {}

        /**
        	Returns the timebase clock's smallest time resolution in microseconds
        	@param aResolution: unsigned 32-bit value for the clock resolution
        */
        virtual void GetTimebaseResolution(uint32& aResolution) = 0;

        /**
        	Returns the current clock time as unsigned 64-bit integer object in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to return the current time
        	@param aUnits: the requested time units for aTime
        */
        virtual void GetCurrentTime64(uint64& aTime, OsclClock_TimeUnits aUnits)
        {
            // Call the version that returns the timebase time as well to get current time
            uint64 tmp64 = 0;
            GetCurrentTime64(aTime, aUnits, tmp64);
        }

        /**
        	Returns the current timebase time as unsigned 64-bit integer object in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to return the current time
        	@param aUnits: the requested time units for aTime
        	@param aTimebaseTime: a reference to an unsigned 64-bit integer to return the timebase time
        */
        virtual void GetCurrentTime64(uint64& aTime, OsclClock_TimeUnits aUnits, uint64& aTimebaseTime) = 0;

        /**
        	Returns the current timebase time as unsigned 32-bit integer in the specified time units
        	@param aTime: a reference to an unsigned 32-bit integer to return the current time
        	@param aOverflow: a reference to a flag which is set if time value cannot fit in unsigned 32-bit integer
        	@param aUnits: the requested time units for aTime
        */
        virtual void GetCurrentTime32(uint32& aTime, bool& aOverflow, OsclClock_TimeUnits aUnits)
        {
            // Use the 64-bit version to get the current time
            // Check for value overflow as well
            uint64 time64 = 0;
            GetCurrentTime64(time64, aUnits);
            aTime = Oscl_Int64_Utils::get_uint64_lower32(time64);
            aOverflow = (Oscl_Int64_Utils::get_uint64_upper32(time64) > 0 ? true : false);
        }

        /**
        	Returns a pointer to the OsclCountTimebase class instance, if
        	this timebase supports OsclCountTimebase.
        	@return pointer to OsclCountTimebase implementation, or NULL if not supported.
        */
        virtual OsclCountTimebase* GetCountTimebase() = 0;

};



class OsclClock : public OsclTimebase, public OsclClockObserver
{
    public:
        /**
        	The default constructor initializes the clock to 0 and goes to STOPPED state
        */
        OSCL_IMPORT_REF OsclClock();

        /**
        	The default destructor
        */
        OSCL_IMPORT_REF ~OsclClock();

        /**
        	Sets the timebase to use for this clock.
        	Will trigger an ClockTimebaseUpdated notice to all observers of this clock.
        	The clock timebase can only be set while in STOPPED and PAUSED states.
        	Returns true if the new clock timebase has been accepted, false otherwise
        	@param aTimebase: a reference to an OsclTimebase-derived object
        */
        OSCL_IMPORT_REF bool SetClockTimebase(OsclTimebase& aTimebase);


        /**
        	Starts the clock from the start time or
        	resumes the clock from the last paused time. Clock goes to RUNNING state.
        	Returns true if the clock is resumed or started, false otherwise
        */
        OSCL_IMPORT_REF bool Start();

        /**
        	Pauses the running clock. Saves the clock time when pausing as the
        	paused time. Clock goes to PAUSED state.
        	Returns true if the clock is paused, false otherwise
        */
        OSCL_IMPORT_REF bool Pause();

        /**
        	Stops the running or paused clock and start time is reset to 0. Clock goes to STOPPED state.
        	Returns true if the clock is stopped, false otherwise
        */
        OSCL_IMPORT_REF bool Stop();


        /**
            Sets the starting clock time with unsigned 32-bit integer in the specified time
        	units while in STOPPED state.
        	Returns true if the start time has been set, false otherwise
        	@param aTime: a reference to an unsigned 32-bit integer to set the start time
        	@param aUnits: the time units of aTime
        */
        OSCL_IMPORT_REF bool SetStartTime32(uint32& aTime, OsclClock_TimeUnits aUnits);

        /**
            Gets the starting clock time as an unsigned 32-bit integer in the specified time units
        	@param aTime: a reference to an unsigned 32-bit integer to copy the start time
        	@param aOverflow: a reference to a flag which is set if time value cannot fit in unsigned 32-bit integer
        	@param aUnits: the requested time units for aTime
        */
        OSCL_IMPORT_REF void GetStartTime32(uint32& aTime, bool& aOverflow, OsclClock_TimeUnits aUnits);

        /**
            Sets the starting clock time with unsigned 64-bit integer in the specified time
        	units while in STOPPED state.
        	Returns true if the start time has been set, false otherwise
        	@param aTime: a reference to an unsigned 64-bit integer to set the start time
        	@param aUnits: the time units of aTime
        */
        OSCL_IMPORT_REF bool SetStartTime64(uint64& aTime, OsclClock_TimeUnits aUnits);

        /**
            Gets the starting clock time as an unsigned 64-bit integer in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to copy the start time
        	@param aUnits: the requested time units for aTime
        */
        OSCL_IMPORT_REF void GetStartTime64(uint64& aTime, OsclClock_TimeUnits aUnits);

        /**
            Adjusts the clock time with unsigned 32-bit integer in the specified time
        	units while in RUNNING state.
        	Returns true if the clock time has been adjusted, false otherwise
        	@param aClockTime: a reference to an unsigned 32-bit integer to the observation clock time
        	@param aTimebaseTime: a reference to an unsigned 64-bit integer to the observation timebase time
        	@param aAdjustedTime: a reference to an unsigned 32-bit integer to the adjusted clock time
        	@param aUnits: the time units of aClockTime and aAdjustedTime
        */
        OSCL_IMPORT_REF bool AdjustClockTime32(uint32& aClockTime, uint64& aTimebaseTime, uint32& aAdjustedTime, OsclClock_TimeUnits aUnits);

        /**
            Adjusts the clock time with unsigned 64-bit integer in the specified time
        	units while in RUNNING state.
        	Returns true if the clock time has been adjusted, false otherwise
        	@param aClockTime: a reference to an unsigned 64-bit integer to the observation clock time
        	@param aTimebaseTime: a reference to an unsigned 64-bit integer to the observation timebase time
        	@param aAdjustedTime: a reference to an unsigned 64-bit integer to the adjusted clock time
        	@param aUnits: the time units of aClockTime and aAdjustedTime
        */
        OSCL_IMPORT_REF bool AdjustClockTime64(uint64& aClockTime, uint64& aTimebaseTime, uint64& aAdjustedTime, OsclClock_TimeUnits aUnits);

        // From OsclTimebase
        /** Returns the clock's smallest time resolution in microseconds
        	@param aResolution: unsigned 32-bit value for the clock resolution
        */
        OSCL_IMPORT_REF void GetTimebaseResolution(uint32& aResolution);

        /**
        	Returns the current clock time as unsigned 64-bit integer object in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to return the current time
        	@param aUnits: the requested time units for aTime
        */
        OSCL_IMPORT_REF void GetCurrentTime64(uint64& aClockTime, OsclClock_TimeUnits aUnits);

        /**
        	Returns the current clock time as unsigned 64-bit integer object in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to return the current time
        	@param aUnits: the requested time units for aTime
        	@param aTimebaseTime: a reference to an unsigned 64-bit integer to return the timebase time
        */
        OSCL_IMPORT_REF void GetCurrentTime64(uint64& aClockTime, OsclClock_TimeUnits aUnits, uint64& aTimebaseTime);

        /**
        	Sets an observer for this clock.  May leave if memory allocation fails.
        	@param aObserver: the observer implemenation
        */
        OSCL_IMPORT_REF void SetClockObserver(OsclClockObserver& aObserver);

        /**
        	Removes an observer for this clock.  If the observer is not registered, this
        	call does nothing.
        	@param aObserver: the observer implemenation
        */
        OSCL_IMPORT_REF void RemoveClockObserver(OsclClockObserver& aObserver);

        /**
        	Sets an observer for this clock.  May leave if memory allocation fails.
        	@param aObserver: the observer implemenation
        */
        OSCL_IMPORT_REF void SetClockStateObserver(OsclClockStateObserver& aObserver);
        /**
        	Removes an observer for this clock.  If the observer is not registered, this
        	call does nothing.
        	@param aObserver: the observer implemenation
        */
        OSCL_IMPORT_REF void RemoveClockStateObserver(OsclClockStateObserver& aObserver);

        /*
         * Enum for OsclClock's internal states
         */
        enum OsclClockState
        {
            STOPPED,
            RUNNING,
            PAUSED
        };

        OsclClockState GetState()
        {
            return iState;
        }

        /**
        	Retreive the OsclCountTimebase implementation from this clock's timebase,
        	or NULL if the timebase is not a counted timebase.
        	@return the OsclCountTimebase, or NULL.
        */
        OsclCountTimebase* GetCountTimebase()
        {
            if (iClockTimebase)
                return iClockTimebase->GetCountTimebase();
            return NULL;
        }


    protected:

        /**
            Changes the clock's state to the specified state
        	@param aState: the new state to change to
        */
        void SetClockState(OsclClockState aState);
        /**
            Updates the iLatestTime and iLatestSourceVal to specified values
        	@param aTime: the new iLatestTime value to change to
        	@param aSourceVal: the new iLatestSourceVal value to change to
        */
        void UpdateLatestTimes(uint64& aTime, uint64& aSourceVal);
        /**
            Converts a time value in the specified time units to microseconds
        	@param aSrcVal: unsigned 64-bit time value in units specified by aSrcUnits
        	@param aSrcUnits: time units of aSrcVal
        	@param aUSecVal: reference to unsigned 64-bit integer to store the microsecond time value
        */
        void ToUSec(uint64& aSrcVal, OsclClock_TimeUnits aSrcUnits, uint64& aUSecVal);
        /**
            Converts a microsecond time value to the specified time units
        	@param aUSecVal: unsigned 64-bit integer in microsecond time value
        	@param aDstVal: reference to unsigned 64-bit integer which will contain aUSecVal in the
        	specified aDstUnits time units
        	@param aDstUnits: requested time units for aDstVal
        */
        void FromUSec(uint64& aUSecVal, uint64& aDstVal, OsclClock_TimeUnits aDstUnits);

        /**
        	Updates the internal clock parameters based on the adjustment information provided.
        	This function can be overridden in the derived classes to allow variety in the adjustment algorithm
        	@param aObsTime: unsigned 64-bit integer in microsecond for the observed clock time
        	@param aObsTimebase: unsigned 64-bit integer in microsecond for the observed timebase time
        	@param aAdjTime: unsigned 64-bit integer in microsecond for the adjusted clock time
        	@param aCurrentTime: unsigned 64-bit integer in microsecond for the current clock time
        	@param aCurrentTimebase: unsigned 64-bit integer in microsecond for the current timebase time
        */
        virtual bool AdjustClock(uint64& aObsTime, uint64& aObsTimebase, uint64& aAdjTime,
                                 uint64& aCurrentTime, uint64& aCurrentTimebase);

        /**
        	Returns the adjusted current clock time when the clock is running
        	This function can be overridden in the derived classes to allow variety in the adjustment algorithm
        	@param aDstTime: unsigned 64-bit integer in microseconds to output the adjusted current clock time
        	@param aTimebaseVal: unsigned 64-bit integer in microseconds of the current timebase time
        */
        virtual void GetAdjustedRunningClockTime(uint64& aDstTime, uint64& aTimebaseVal);

        // Clock time is stored as unsigned 64-bit value in microseconds
        // Timebase time is stored as unsigned 64-bit value in microseconds
        uint64 iLatestRunningClockTime;     // Last reference clock time due to starting/resuming, pausing, and adjustment
        uint64 iLatestRunningTimebaseTime;  // Timebase time corresponding to the latest running clock time
        uint64 iStartClockTime;				// Starting clock time. Set by the SetStartTime...() APIs
        uint64 iPauseClockTime;				// Clock time when Pause() API is called.
        uint64 iLastAdjustTimebaseTime;		// The observed timebase time corresponding to the adjusted time passed in
        uint64 iAdjustmentTimebaseTime;		// The timebase time of the last successful AdjustClockTime...() call

        OsclClockState iState;					// Internal state of the clock

        OsclTimebase* iClockTimebase;			// Pointer to this clock's timebase

        //vector of clock observers.
        Oscl_Vector<OsclClockObserver*, OsclMemAllocator> iClockObservers;
        Oscl_Vector<OsclClockStateObserver*, OsclMemAllocator> iClockStateObservers;

        //from OsclClockObserver, for callbacks from an OsclCountTimebase.
        void ClockCountUpdated();
        void ClockTimebaseUpdated();
        void ClockAdjusted();
};


#define OSCL_DISABLE_WARNING_CONV_POSSIBLE_LOSS_OF_DATA
#include "osclconfig_compiler_warnings.h"

/**
	OsclTimebase_Tickcount is OsclTimebase-derived class which uses
	the OSCL's system tickcount as the timebase. This class is provided
	as the default OsclTimebase that is available on any platform with OSCL support.
*/
class OsclTimebase_Tickcount : public OsclTimebase
{
    public:
        /**
        	Constructor. Retrieves the constant to convert OSCL tickcount value to microseconds
        */
        OsclTimebase_Tickcount()
        {
            Oscl_Int64_Utils::set_uint64(iMicrosecPerTick, 0, OsclTickCount::TickCountPeriod());
            iTickcountHCounter = 0;
            iPrevTickcount = 0;
        }

        /**
        	Destructor
        */
        ~OsclTimebase_Tickcount()
        {
        }

        // From OsclTimebase
        /**
        	Returns the OSCL tickcount's time resolution in microseconds
            Implementation of virtual function from OsclTimebase
        	@param aResolution: On function completion, contains OSCL tickcount resolution
        */
        void GetTimebaseResolution(uint32& aResolution)
        {
            aResolution = Oscl_Int64_Utils::get_uint64_lower32(iMicrosecPerTick);
        }

        /**
        	Returns the current clock time as unsigned 64-bit integer object in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to return the current time
        	@param aUnits: the requested time units for aTime
        	@param aTimebaseTime: a reference to an unsigned 64-bit integer to return the timebase time
        */
        void GetCurrentTime64(uint64& aTime, OsclClock_TimeUnits aUnits, uint64& aTimebaseTime)
        {
            // Check to see if the tickcount wrapped around
            uint32 currenttickcount = OsclTickCount::TickCount();
            if (iPrevTickcount > currenttickcount)
            {
                ++iTickcountHCounter;
            }

            // Set the time and convert to microseconds
            Oscl_Int64_Utils::set_uint64(aTime, iTickcountHCounter, currenttickcount);
            aTimebaseTime = aTime;
            aTime *= iMicrosecPerTick;

            // Determine the divider constant for the specified units
            uint64 divconst = UINT64_HILO(0, 1);
            switch (aUnits)
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
                    divconst = UINT64_HILO(0, OSCL_UNSIGNED_CONST(0xD693A400));
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
                aTime /= divconst;
            }

            // Save the current tickcount for next comparison
            iPrevTickcount = currenttickcount;
        }

        OsclCountTimebase* GetCountTimebase()
        {
            return NULL;
        }

    protected:
        uint64 iMicrosecPerTick;
        uint32 iTickcountHCounter;
        uint32 iPrevTickcount;

};

/**
	OsclTimebase_Counter is OsclTimebase-derived class that can be used to
	implement a simple count-based timebase.
*/
class OsclTimebase_Count : public OsclTimebase, public OsclCountTimebase
{
    public:
        /**
        	Constructor.
        */
        OsclTimebase_Count()
        {
            iCurrentCount = 0;
            iObserver = NULL;
        }

        /**
        	Destructor
        */
        ~OsclTimebase_Count()
        {
        }

        // From OsclTimebase
        /**
        	Returns the OSCL tickcount's time resolution in microseconds
            Implementation of virtual function from OsclTimebase
        	@param aResolution: On function completion, contains OSCL tickcount resolution
        */
        void GetTimebaseResolution(uint32& aResolution)
        {
            aResolution = 0;//not meaningful for a count-based timebase.
        }

        /**
        	Returns the current clock time as unsigned 64-bit integer object in the specified time units
        	@param aTime: a reference to an unsigned 64-bit integer to return the current time
        	@param aUnits: the requested time units for aTime
        	@param aTimebaseTime: a reference to an unsigned 64-bit integer to return the timebase time
        */
        void GetCurrentTime64(uint64& aTime, OsclClock_TimeUnits aUnits, uint64& aTimebaseTime)
        {
            //not meaningful for a count-based timebase.
            OSCL_UNUSED_ARG(aUnits);
            aTime = 0;
            aTimebaseTime = 0;
        }

        /**
        	Returns the OsclCountTimebase implementation pointer
        */
        OsclCountTimebase* GetCountTimebase()
        {
            return this;
        }

        //From OsclCountTimebase
        /**
        	Used to adjust the current count.
        	@param aCount (input): new count value.
        */
        void SetCount(int64 aCount)
        {
            iCurrentCount = aCount;
            if (iObserver)
            {
                iObserver->ClockCountUpdated();
            }
        }

        /**
        	Used to retreive the current count.
        	@param aCount (output): new count value.
        */
        void GetCount(int64& aCount)
        {
            aCount = iCurrentCount;
        }


    protected:
        int64 iCurrentCount;
        OsclClockObserver* iObserver;

    private:
        friend class OsclClock;
        //From OsclCountTimebase
        void SetClockObserver(OsclClockObserver* aObserver)
        {
            iObserver = aObserver;
        }

};


#endif // OSCL_CLOCK_H_INCLUDED


