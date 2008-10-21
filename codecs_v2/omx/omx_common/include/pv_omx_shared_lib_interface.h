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
#ifndef PV_OMX_SHARED_LIB_INTERFACE_H_INCLUDED
#define PV_OMX_SHARED_LIB_INTERFACE_H_INCLUDED

#define PV_OMX_SHARED_INTERFACE OsclUuid(0x1d4769f0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x67)
#define PV_OMX_CREATE_INTERFACE OsclUuid(0x1d4769f0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x68)
#define PV_OMX_DESTROY_INTERFACE OsclUuid(0x1d4769f0,0xca0c,0x11dc,0x95,0xff,0x08,0x00,0x20,0x0c,0x9a,0x69)

class OmxSharedLibraryInterface
{
    public:
        virtual OsclAny* QueryOmxComponentInterface(const OsclUuid& aOmxTypeId, const OsclUuid& aInterfaceId) = 0;
};

#endif // PV_OMX_SHARED_LIB_INTERFACE_H_INCLUDED
