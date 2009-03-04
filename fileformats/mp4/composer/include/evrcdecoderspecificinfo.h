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

#ifndef __EVRCDecoderSpecificInfo_H__
#define __EVRCDecoderSpecificInfo_H__

#include "a_atomdefs.h"
#include "decoderspecificinfo.h"

//using namespace std;

class PVA_FF_EVRCDecoderSpecificInfo : public PVA_FF_DecoderSpecificInfo
{

    public:
        PVA_FF_EVRCDecoderSpecificInfo(); // Default constructor
        virtual ~PVA_FF_EVRCDecoderSpecificInfo() {} // Destructor

        void setVendorcode(int32 VendorCode = PACKETVIDEO_FOURCC)
        {
            _VendorCode = VendorCode;
        }
        void setEncoderVersion(uint8 encoder_version = 0)
        {
            _encoder_version = encoder_version;
        }
        void setFrameType(uint8 frame_type)
        {
            _frame_type = frame_type;
        }

        uint8 getFrameType()
        {
            return _frame_type;
        }

        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

    private:
        int32            _VendorCode;
        uint8            _encoder_version;
        uint8            _frame_type;
};

#endif

