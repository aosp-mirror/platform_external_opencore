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
/*     -------------------------------------------------------------------       */
/*                            MPEG-4 PVA_FF_RandomAccessAtom Class                      */
/*     -------------------------------------------------------------------       */
/*********************************************************************************/

#ifndef __RandomAccessAtom_H__
#define __RandomAccessAtom_H__

#include "fullatom.h"

class PVA_FF_RandomAccessAtom : public PVA_FF_FullAtom
{

    public:
        //Methods
        PVA_FF_RandomAccessAtom(); 			// Constructor
        virtual ~PVA_FF_RandomAccessAtom() {};	// Destructor
        void setRandomAccessDenied(bool denied)
        {
            random_access_denied = denied;    // sets the random access flag
        }

        virtual void recomputeSize();

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        uint8		random_access_denied;
};



#endif

