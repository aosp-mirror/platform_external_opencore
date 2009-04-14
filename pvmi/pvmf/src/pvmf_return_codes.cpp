/*
**
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "pvmf_return_codes.h"

#ifdef CONSIDER
#error "CONSIDER already defined!"
#endif
#define CONSIDER(val) case val: return #val

const char *PVMFStatusToString(const PVMFStatus status) {
    switch (status) {
        CONSIDER(PVMFSuccess);
        CONSIDER(PVMFPending);
        CONSIDER(PVMFNotSet);
        CONSIDER(PVMFFailure);
        CONSIDER(PVMFErrCancelled);
        CONSIDER(PVMFErrNoMemory);
        CONSIDER(PVMFErrNotSupported);
        CONSIDER(PVMFErrArgument);
        CONSIDER(PVMFErrBadHandle);
        CONSIDER(PVMFErrAlreadyExists);
        CONSIDER(PVMFErrBusy);
        CONSIDER(PVMFErrNotReady);
        CONSIDER(PVMFErrCorrupt);
        CONSIDER(PVMFErrTimeout);
        CONSIDER(PVMFErrOverflow);
        CONSIDER(PVMFErrUnderflow);
        CONSIDER(PVMFErrInvalidState);
        CONSIDER(PVMFErrNoResources);
        CONSIDER(PVMFErrResourceConfiguration);
        CONSIDER(PVMFErrResource);
        CONSIDER(PVMFErrProcessing);
        CONSIDER(PVMFErrPortProcessing);
        CONSIDER(PVMFErrAccessDenied);
        CONSIDER(PVMFErrLicenseRequired);
        CONSIDER(PVMFErrLicenseRequiredPreviewAvailable);
        CONSIDER(PVMFErrContentTooLarge);
        CONSIDER(PVMFErrMaxReached);
        CONSIDER(PVMFLowDiskSpace);
        CONSIDER(PVMFErrHTTPAuthenticationRequired);
        CONSIDER(PVMFInfoPortCreated);
        CONSIDER(PVMFInfoPortDeleted);
        CONSIDER(PVMFInfoPortConnected);
        CONSIDER(PVMFInfoPortDisconnected);
        CONSIDER(PVMFInfoOverflow);
        CONSIDER(PVMFInfoUnderflow);
        CONSIDER(PVMFInfoProcessingFailure);
        CONSIDER(PVMFInfoEndOfData);
        CONSIDER(PVMFInfoBufferCreated);
        CONSIDER(PVMFInfoBufferingStart);
        CONSIDER(PVMFInfoBufferingStatus);
        CONSIDER(PVMFInfoBufferingComplete);
        CONSIDER(PVMFInfoDataReady);
        CONSIDER(PVMFInfoPositionStatus);
        CONSIDER(PVMFInfoStateChanged);
        CONSIDER(PVMFInfoDataDiscarded);
        CONSIDER(PVMFInfoErrorHandlingStart);
        CONSIDER(PVMFInfoErrorHandlingComplete);
        CONSIDER(PVMFInfoRemoteSourceNotification);
        CONSIDER(PVMFInfoLicenseAcquisitionStarted);
        CONSIDER(PVMFInfoContentLength);
        CONSIDER(PVMFInfoContentTruncated);
        CONSIDER(PVMFInfoSourceFormatNotSupported);
        CONSIDER(PVMFInfoPlayListClipTransition);
        CONSIDER(PVMFInfoContentType);
        CONSIDER(PVMFInfoTrackDisable);
        CONSIDER(PVMFInfoUnexpectedData);
        CONSIDER(PVMFInfoSessionDisconnect);
        CONSIDER(PVMFInfoStartOfData);
        CONSIDER(PVMFInfoReportObserverRecieved);
        CONSIDER(PVMFInfoMetadataAvailable);
        CONSIDER(PVMFInfoDurationAvailable);
        CONSIDER(PVMFInfoChangePlaybackPositionNotSupported);
        CONSIDER(PVMFInfoPoorlyInterleavedContent);
        CONSIDER(PVMFInfoVideoTrackFallingBehind);
        default: return "UNKNOWN PVMFStatus";
    }
}

#undef CONSIDER
