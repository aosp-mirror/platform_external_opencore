/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#ifndef MPEG4BITRATE_H_INCLUDED
#define MPEG4BITRATE_H_INCLUDED

#include "atom.h"
#include "atomutils.h"
#include "oscl_file_io.h"

class PVA_FF_Mpeg4Bitrate : public PVA_FF_Atom
{

    public:
        PVA_FF_Mpeg4Bitrate(uint32 BufferSizeDB, uint32 MaxBitRate, uint32 AvgBitRate);
        virtual ~PVA_FF_Mpeg4Bitrate() {};  // Destructor
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);
        virtual void recomputeSize();
        virtual uint32 getSize();

    private:
        uint32 _bufferSizeDB;
        uint32 _maxBitRate;
        uint32 _avgBitRate;
};

#endif // MPEG4BITRATE_H_INCLUDED
