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

#ifndef __PVCOMMON_H__
#define __PVCOMMON_H__

#include <e32base.h>

enum TPVURNType
{
    EURNTypeError,
    EUrlFile,
    ELocalFile,
    EPvxFile
};

enum TPVClipSource
{
    EClipSourceError	= 0x000,
    ELocal				= 0x001,
    EStreaming			= 0x002,
    EDownload			= 0x004,
};

enum TPVCodecType
{
    ENoCodec		= 0x0000,
    EAacDec			= 0x0001,
    EGsmamrDec		= 0x0002,
    EEvrcDec		= 0x0004,
    ELastAudioCodec	= 0x00FF,

    EM4vDec			= 0x0100,
    EH263Dec		= 0x0200
};

enum TPVClipType
{
    EInvalidType,
    EAudioOnly,
    EVideoOnly,
    EAudioVideo,
    EStillVideo,
    EAudioStillVideo
};

enum TPVEndOfClipAction
{
    ENoAction	= 0,
    ECloseApp,
    ENextUrl
};

enum TPVPlaybackControl		// For Download only
{
    ENoPlayback		= 0,
    EAfterDownload,
    EAsap,
    EReserve
};

enum TPVPlayerEngineState
{
    EIdle,
    EClipSelected,
    ESelectingTrack,
    EInitializingPlay,
    EPlaying,
    EBuffering,
    EStopped,
    EPaused,
    EDownloading,
    EDownloadingPlaying,
    EDownloadSuspended,
    EUnknownEngineState
};

enum TPVStatusCode
{
    EPVError = 0,	// API return value only
    EPVSuccess,		// API return value only

    EOpenURNSuccess,
    EOpenURNFailure,

    EPlaySuccess,
    EPlayFailure,

    EStopSuccess,
    EStopEndOfClip,
    EStopTimeReached,
    EStopFailure,

    EPauseSuccess,
    EPauseFailure,

    EInvalidStartTime,
    EInvalidStopTime,
    EInvalidFilename,

    ERepositionNotAllowed,
    EAudioOnlyNotAllowed,
    EVideoOnlyNotAllowed,

    EUnsupportedCodec,
    EVideoDimensionTooLarge,

    EDownloadSuccess,
    EDownloadFailure,
    EDownloadNotEnoughMemorySpace,

    EResumeDownloadSuccess,
    EResumeDownloadFailure,

    EStopDownloadSuccess,
    EStopDownloadFailure,

    EAudioError,
    EVideoError,
    EDownloadError,

    EInvalidURL,
    EConnectionError,
    EServerError,
    EServerErrorMessage,

    EDownloadStatus,	// 0-100 for download status
    EPlayStatus,		// 0-100 for play status, -1 for buffering/waiting mode
    EStreamBuffering,	// 0-100 how much of buffer filled (100 for buffer==preroll buffer time)
    ERepositionBuffering, // how much more time in ms till start time

    EVideoWidth,
    EVideoHeight,
    EDisplayVideoFrame,

    EMemoryAllocationError,
    ELastEventId
};


//
// Defines
//
#define MAX_URL_LEN			256
#define MAX_FILENAME_LEN	256
#define MAX_TITLE_LEN		256
#define MAX_AUTHOR_LEN		256
#define MAX_COPYRIGHT_LEN	256
#define MAX_RATING_LEN		256
#define MAX_DESCRIPTION_LEN	256
#define MAX_VERSION_LEN		256
#define MAX_OWNER_LEN		256
#define MAX_DATE_LEN		256

#define MAX_VERSIONSTRING_LEN		12
#define MAX_OBSERVERPARAM2_LEN		100


//
// Structures
//
struct TPVAppParams
{
public:
    TBool iSecured;
    TBool iUserPlaybackAllowed;
    TBool iRepositioningAllowed;
    TPVPlaybackControl iControl;
    TPVEndOfClipAction iAction;
    TBufC<MAX_URL_LEN> iNextUrl;
};

struct TPVClipInfo
{
public:
    TBool iLive;
    TBool iRepositioningAllowed;
    TBool iAllowAudioOnly;					// NOT USED CURRENTLY
    TBool iAllowVideoOnly;
    TPVClipType iType;						// Clip type
    TInt iWidth;							// Width of video
    TInt iHeight;							// Height of video
    TInt iDuration;							// Length of clip in milliseconds
    TInt iSize;								// Download file size in bytes
    TBufC<MAX_TITLE_LEN> iTitle;
    TBufC<MAX_AUTHOR_LEN> iAuthor;
    TBufC<MAX_COPYRIGHT_LEN> iCopyright;
    TBufC<MAX_RATING_LEN> iRating;
    TBufC<MAX_DESCRIPTION_LEN> iDescription;
    TBufC<MAX_OWNER_LEN> iOwner;
    TBufC<MAX_VERSION_LEN> iVersion;
    TBufC<MAX_DATE_LEN> iCreationDate;
};

struct TPVDownloadInfo
{
public:
    TBool iDownloadComplete;
    TInt iInterruptedTime;
    TBufC<MAX_FILENAME_LEN> iFilename;
};


// PVEngine event observer - for communicating events back to UI
class MPVEngineObserver
{
    public:
        IMPORT_C virtual void HandlePVEngineEvent(TPVStatusCode aEventId,
                TInt aParam1,
                const TDesC& aParam2) = 0;
        IMPORT_C virtual void HandlePVVideoEvent(const unsigned char* aData,
                TInt aSize) = 0;
};

#endif // __PVCOMMON_H__


