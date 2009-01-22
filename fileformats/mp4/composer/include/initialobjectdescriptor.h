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
/*
    This PVA_FF_InitialObjectDescriptor Class
*/

#ifndef __InitialObjectDescriptor_H__
#define __InitialObjectDescriptor_H__

#include "objectdescriptor.h"
#include "es_id_inc.h"


class PVA_FF_InitialObjectDescriptor : public PVA_FF_ObjectDescriptor
{

    public:
        PVA_FF_InitialObjectDescriptor(); // Constructor

        virtual ~PVA_FF_InitialObjectDescriptor();
        void init();

        // Rendering the PVA_FF_Atom in proper format (bitlengths, etc.) to an ostream
        virtual bool renderToFileStream(MP4_AUTHOR_FF_FILE_IO_WRAP *fp);

        // Recompute size of class
        virtual void recomputeSize();

        // Member gets and sets
        bool getIncludeInlineProfilesFlag() const
        {
            return _includeInlineProfilesFlag;
        }
        void setIncludeInlineProfilesFlag(bool flag)
        {
            _includeInlineProfilesFlag = flag;
        }

        uint8 getODProfileLevelIndication()
        {
            return _ODProfileLevelIndication;
        }
        void setODProfileLevelIndication(uint8 odpli)
        {
            _ODProfileLevelIndication = odpli;
        }

        uint8 getSceneProfileLevelIndication()
        {
            return _sceneProfileLevelIndication;
        }
        void setSceneProfileLevelIndication(uint8 spli)
        {
            _sceneProfileLevelIndication = spli;
        }

        uint8 getAudioProfileLevelIndication()
        {
            return _audioProfileLevelIndication;
        }
        void setAudioProfileLevelIndication(uint8 apli)
        {
            _audioProfileLevelIndication = apli;
        }

        uint8 getVisualProfileLevelIndication()
        {
            return _visualProfileLevelIndication;
        }
        void setVisualProfileLevelIndication(uint8 vpli)
        {
            _visualProfileLevelIndication = vpli;
        }

        uint8 getGraphicsProfileLevelIndication()
        {
            return _graphicsProfileLevelIndication;
        }
        void setGraphicsProfileLevelIndication(uint8 gpli)
        {
            _graphicsProfileLevelIndication = gpli;
        }

        Oscl_Vector<PVA_FF_ES_ID_Inc*, OsclMemAllocator> &getESIDIncludes()
        {
            return *_pES_ID_Inc_Vec;
        }
        void addESIDInclude(PVA_FF_ES_ID_Inc *ref);
        PVA_FF_ES_ID_Inc* getESIDIncludeAt(int32 index);

    private:
        // ODID from base clase PVA_FF_ObjectDescriptor (10)
        // urlFlag from base class PVA_FF_ObjectDescriptor (1)
        bool _includeInlineProfilesFlag; // (1)
        // _reserved from base class - now ONLY (4) instead of (5)

        uint8 _ODProfileLevelIndication; // (8)
        uint8 _sceneProfileLevelIndication; // (8)
        uint8 _audioProfileLevelIndication; // (8)
        uint8 _visualProfileLevelIndication; // (8)
        uint8 _graphicsProfileLevelIndication; // (8)

        Oscl_Vector<PVA_FF_ES_ID_Inc*, OsclMemAllocator> *_pES_ID_Inc_Vec; // PVA_FF_ESDescriptor[1 to 30]
};



#endif
