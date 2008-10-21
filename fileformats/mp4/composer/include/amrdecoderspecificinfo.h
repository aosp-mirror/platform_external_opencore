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
/*********************************************************************************/
/*
    This PVA_FF_DecoderSpecificInfo Class that holds the Mpeg4 VOL header for the
	video stream
*/

#ifndef __AMRDecoderSpecificInfo_H__
#define __AMRDecoderSpecificInfo_H__

#include "oscl_types.h"

#include "a_atomdefs.h"
#include "decoderspecificinfo.h"

//using namespace std;

class PVA_FF_AMRDecoderSpecificInfo : public PVA_FF_DecoderSpecificInfo
{

    public:
        PVA_FF_AMRDecoderSpecificInfo(); // Default constructor
        virtual ~PVA_FF_AMRDecoderSpecificInfo() {}; // Destructor

        void setVendorcode(int32 VendorCode = PACKETVIDEO_FOURCC)
        {
            _VendorCode = VendorCode;
        }
        void setEncoderVersion(uint8 encoder_version = 0)
        {
            _encoder_version = encoder_version;
        }
        void setBandMode(uint8 band_mode = 0)
        {
            _band_mode = band_mode;
        }
        void setFrameType(uint8 frame_type)
        {
            _frame_type = frame_type;
        }
        void setModeSet(uint16 mode_set)
        {
            _mode_set = mode_set;
        }
        void setModeChangePeriod(uint8 mode_change_period = 0)
        {
            _mode_change_period = mode_change_period;
        }
        void setModeChangeNeighbour(uint8 mode_change_neighbour = 0)
        {
            _mode_change_neighbour = mode_change_neighbour;
        }

        uint8  getFrameType()
        {
            return _frame_type;
        }
        uint16 getModeSet()
        {
            return _mode_set;
        }

        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

    private:
        int32           _VendorCode;
        uint8           _encoder_version;
        uint8           _band_mode;
        uint8           _frame_type;
        uint16          _mode_set;
        uint8           _mode_change_period;
        uint8           _mode_change_neighbour;
        int32           _reserved;
};

#endif

