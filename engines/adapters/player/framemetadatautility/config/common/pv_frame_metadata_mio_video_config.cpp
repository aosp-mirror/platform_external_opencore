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
#include "pv_frame_metadata_mio_video.h"
#include "pvlogger.h"

// Add/remove header file and modify function below for different CC support
#if 0
#include "cczoomrotation12.h"
#endif
#include "cczoomrotation16.h"
#if 0
#include "cczoomrotation24.h"
#endif


PVMFStatus PVFMVideoMIO::CreateYUVToRGBColorConverter(ColorConvertBase*& aCC, PVMFFormatType aRGBFormatType)
{
    int32 leavecode = 0;
#if 0
    if (aRGBFormatType == PVMF_MIME_RGB12)
    {
        OSCL_TRY(leavecode, aCC = ColorConvert12::NewL());
    }
#endif
    if (aRGBFormatType == PVMF_MIME_RGB16)
    {
        OSCL_TRY(leavecode, aCC = ColorConvert16::NewL());
    }
#if 0
    if (aRGBFormatType == PVMF_MIME_RGB24)
    {
        OSCL_TRY(leavecode, aCC = ColorConvert24::NewL());
    }
#endif
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVFMVideoMIO::CreateYUVToRGBColorConverter() Unsupported RGB mode for color converter. Asserting"));
        return PVMFErrNotSupported;
    }

    OSCL_FIRST_CATCH_ANY(leavecode,
                         PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVFMVideoMIO::CreateYUVToRGBColorConverter() Color converter instantiation did a leave"));
                         return PVMFErrNoResources;
                        );

    return PVMFSuccess;
}


PVMFStatus PVFMVideoMIO::DestroyYUVToRGBColorConverter(ColorConvertBase*& aCC, PVMFFormatType aRGBFormatType)
{
    OSCL_ASSERT(aCC != NULL);

#if 0
    if (aRGBFormatType == PVMF_MIME_RGB12)
    {
        OSCL_DELETE(((ColorConvert12*)aCC));
        aCC = NULL;
    }
#endif
    if (aRGBFormatType == PVMF_MIME_RGB16)
    {
        OSCL_DELETE(((ColorConvert16*)aCC));
        aCC = NULL;
    }
#if 0
    if (aRGBFormatType == PVMF_MIME_RGB24)
    {
        OSCL_DELETE(((ColorConvert24*)aCC));
        aCC = NULL;
    }
#endif
    else
    {
        PVLOGGER_LOGMSG(PVLOGMSG_INST_HLDBG, iLogger, PVLOGMSG_ERR, (0, "PVFMVideoMIO::CreateYUVToRGBColorConverter() Unsupported RGB mode for color converter. Asserting"));
        return PVMFErrNotSupported;
    }

    return PVMFSuccess;
}


