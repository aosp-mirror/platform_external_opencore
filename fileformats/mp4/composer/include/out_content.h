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

// A custom interface to allow the applet to adjust parameters

#ifndef __ICONTENTINFO__
#define __ICONTENTINFO__

#include "pvcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*typedef struct _CONTENTINFO
    {
    	WCHAR Title[256];
    	WCHAR Author[256];
    	WCHAR Copyright[256];
    	WCHAR Description[256];
    	WCHAR Rating[256];
    } CONTENTINFO, *LPCONTENTINFO;
    */
    // {38BEEDC0-1242-11d4-9F78-0050DACFB6C8}
    DEFINE_GUID(IID_IContentInfo,
                0x38beedc0, 0x1242, 0x11d4, 0x9f, 0x78, 0x0, 0x50, 0xda, 0xcf, 0xb6, 0xc8);

    DECLARE_INTERFACE_(IContentInfo, IUnknown)
    {

        STDMETHOD(SetContentInfo)(THIS_ LPCONTENTINFO lpContentInfo) PURE;
        STDMETHOD(GetContentInfo)(THIS_ LPCONTENTINFO lpContentInfo) PURE;

        STDMETHOD(SetVersion)(THIS_ BSTR version1, BSTR version2) PURE;
        STDMETHOD(GetVersion)(THIS_ BSTR version1, BSTR version2) PURE;
    };

#ifdef __cplusplus
}
#endif

#endif // __ICONTENTINFO__

