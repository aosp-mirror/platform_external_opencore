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
//                                                                              //
//  File: media_clock_converter.cpp                                             //
//                                                                              //
//////////////////////////////////////////////////////////////////////////////////

#include "media_clock_converter.h"

// Use default DLL entry point for Symbian
#include "oscl_dll.h"
OSCL_DLL_ENTRY_POINT_DEFAULT()

//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void MediaClockConverter::set_value(const MediaClockConverter& src)
{
    uint64 value = (uint64(src.get_wrap_count())) << 32;

    value += src.get_current_timestamp();

    value = (uint64(value) * timescale) / src.get_timescale();

    wrap_count = ((uint32)(value >> 32)) % timescale;

    current_ts = (uint32)(value & 0xFFFFFFFF);
}

//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void MediaClockConverter::set_timescale(uint32 new_timescale)
{
    uint64 value = ((uint64)wrap_count) << 32;
    value += current_ts;

    value = (value * new_timescale) / timescale;

    timescale = new_timescale;

    wrap_count = ((uint32)(value >> 32)) % timescale;

    current_ts = (uint32)(value & 0xFFFFFFFF);
}

//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF void MediaClockConverter::set_clock_other_timescale(uint32 value,
        uint32 in_timescale)
{
    uint64 new_value = (uint64)value * timescale;
    uint64 in_timescale64Comp = (uint64)(in_timescale - 1);
    new_value = new_value + in_timescale64Comp ;
    new_value /= in_timescale;

    wrap_count = ((uint32)(new_value >> 32)) % timescale;

    current_ts = (uint32)(new_value & 0xFFFFFFFF);
}

//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 MediaClockConverter::get_timediff_and_update_clock(uint32 value,
        uint32 in_timescale,
        uint32 output_timescale)
{
    // convert to native timescale
    uint64 new_value = (uint64)value * timescale;
    new_value /= in_timescale;

    uint32 new_timevalue = ((uint32)(new_value & 0xFFFFFFFF));

    return get_timediff_and_update_clock(new_timevalue, output_timescale);

}

//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 MediaClockConverter::get_timediff_and_update_clock(uint32 value,
        uint32 output_timescale)
{
    uint32 diff = value - current_ts;

    // convert to output timescale
    uint64 new_value = (uint64)diff * output_timescale;
    new_value /= timescale;

    diff = ((uint32)(new_value & 0xFFFFFFFF));

    if (update_clock(value))
    {
        return diff;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool MediaClockConverter::update_clock(uint32 new_ts)
{
    uint32 diff = new_ts - current_ts;
    if (new_ts < current_ts)
    {
        if (diff < WRAP_THRESHOLD)
        {
            if (++wrap_count >= timescale)
            {
                wrap_count = 0;
            }
            current_ts = new_ts;
            return true;
        }
        // otherwise this an earlier value so ignore it.
    }
    else
    {
        if (diff < MISORDER_THRESHOLD)
        {
            current_ts = new_ts;
            return true;
        }
        // otherwise this an earlier value so ignore it.
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF uint32 MediaClockConverter::get_converted_ts(uint32 new_timescale) const
{

    uint64 value = ((uint64)wrap_count) << 32;
    //Accounting for a roundingoff error
    value += (uint64(current_ts) * uint64(new_timescale) + uint64(timescale - 1)) / uint64(timescale);

    return ((uint32) value);

}
