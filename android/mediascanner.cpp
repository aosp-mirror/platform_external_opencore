/* mediascanner.cpp
 * 
 * Android wrapper for media scanner
 */

#include <media/mediascanner.h>
#include <stdio.h>


#include "pvlogger.h"
#include "pv_id3_parcom.h"
#include "oscl_string_containers.h"
#include "oscl_file_io.h"
#include "oscl_assert.h"
#include "oscl_lock_base.h"
#include "oscl_snprintf.h"
#include "pvmf_return_codes.h"
#include "pv_mime_string_utils.h"
#include "pv_id3_parcom_constants.h"
#include "oscl_utf8conv.h"
#include "imp3ff.h"
#include "impeg4file.h"

// Ogg Vorbis includes
#include "ivorbiscodec.h"
#include "ivorbisfile.h"

// Sonivox includes
#include <libsonivox/eas.h>

// used for WMA support
#include "media/mediametadataretriever.h"

#include <media/thread_init.h>

#define MAX_BUFF_SIZE   1024

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#undef LOG_TAG
#define LOG_TAG "MediaScanner"
#include "utils/Log.h"

#define MAX_STR_LEN    1000


namespace android {


MediaScanner::MediaScanner()
{
}

MediaScanner::~MediaScanner()
{
}

static PVMFStatus parseMP3(const char *filename, MediaScannerClient& client)
{
    PVID3ParCom pvId3Param;
    PVFile fileHandle;
    Oscl_FileServer iFs;
    uint32 duration;

    if(iFs.Connect() != 0)
    {
        LOGE("iFs.Connect failed\n");
        return PVMFFailure;
    }

    oscl_wchar output[MAX_BUFF_SIZE];
    oscl_UTF8ToUnicode((const char *)filename, oscl_strlen((const char *)filename), (oscl_wchar *)output, MAX_BUFF_SIZE);
    if( 0 != fileHandle.Open((oscl_wchar *)output, Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs) ) 
    {
        LOGE("Could not open the input file for reading(Test: parse id3).\n");
        return PVMFFailure;
    }

    fileHandle.Seek(0, Oscl_File::SEEKSET);
    pvId3Param.ParseID3Tag(&fileHandle);
    fileHandle.Close();
    iFs.Close();
    
    //Get the frames information from ID3 library
    PvmiKvpSharedPtrVector framevector;
    pvId3Param.GetID3Frames(framevector);
        
    uint32 num_frames = framevector.size();

    for (uint32 i = 0; i < num_frames;i++)
    {
        const char* key = framevector[i]->key;
        
        // type should follow first semicolon
        const char* type = strchr(key, ';') + 1;
        if (type == 0) continue;
        
        // KVP_VALTYPE_UTF8_CHAR check must be first, since KVP_VALTYPE_ISO88591_CHAR is a substring of KVP_VALTYPE_UTF8_CHAR
        // similarly, KVP_VALTYPE_UTF16BE_WCHAR must be checked before KVP_VALTYPE_UTF16_WCHAR
        if (oscl_strncmp(type, KVP_VALTYPE_UTF8_CHAR, KVP_VALTYPE_UTF8_CHAR_LEN) == 0) {
            // utf8
            // pass through directly
            if (!client.handleStringTag(key, framevector[i]->value.pChar_value)) goto failure;           
        } else if (oscl_strncmp(type, KVP_VALTYPE_ISO88591_CHAR, KVP_VALTYPE_ISO88591_CHAR_LEN) == 0) {
            // iso-8859-1
            // convert to utf8
            // worse case is 2x inflation
            const unsigned char* src = (const unsigned char *)framevector[i]->value.pChar_value;
            char* temp = (char *)alloca(strlen(framevector[i]->value.pChar_value) * 2 + 1);
            if (temp) {
                char* dest = temp;
                unsigned int uch;
                while ((uch = *src++) != 0) {
                    if (uch & 0x80) {
                        *dest++ = (uch >> 6) | 0xc0;
                        *dest++ = (uch & 0x3f) | 0x80;
                    } else *dest++ = uch;
                }
                *dest = 0;
                if (!client.handleStringTag(key, temp)) goto failure;           
            }
        } else if (oscl_strncmp(type, KVP_VALTYPE_UTF16BE_WCHAR, KVP_VALTYPE_UTF16BE_WCHAR_LEN) == 0 ||
                oscl_strncmp(type, KVP_VALTYPE_UTF16_WCHAR, KVP_VALTYPE_UTF16_WCHAR_LEN) == 0) {
            // convert wchar to utf8
            // the id3parcom library has already taken care of byteswapping
            const oscl_wchar*  src = framevector[i]->value.pWChar_value;
            int srcLen = oscl_strlen(src);
            // worse case is 3 bytes per character, plus zero termination
            int destLen = srcLen * 3 + 1;
            char* dest = (char *)alloca(destLen);

            if (oscl_UnicodeToUTF8(src, oscl_strlen(src), dest, destLen) > 0) {
                if (!client.handleStringTag(key, dest)) goto failure;           
            }                 
        } else if (oscl_strncmp(type, KVP_VALTYPE_UINT32, KVP_VALTYPE_UINT32_LEN) == 0) {
            char temp[20];
            snprintf(temp, sizeof(temp), "%d", (int)framevector[i]->value.uint32_value);
            if (!client.handleStringTag(key, temp)) goto failure;
        } else {
            //LOGE("unknown tag type %s for key %s\n", type, key);
        }
    }
    
    // extract non-ID3 properties below
    {
        OSCL_wHeapString<OsclMemAllocator> mp3filename(output);
        MP3ErrorType    err;
        IMpeg3File mp3File(mp3filename, err);
        if (err != MP3_SUCCESS) {
            LOGE("IMpeg3File constructor returned %d.\n", err);
            return err;
        }
        err = mp3File.ParseMp3File();
        if (err != MP3_SUCCESS) {
            LOGE("IMpeg3File::ParseMp3File returned %d.\n", err);
            return err;
        }
    
        char buffer[20];
        duration = mp3File.GetDuration();
        sprintf(buffer, "%d", duration);
        if (!client.handleStringTag("duration", buffer)) goto failure;
    }

    return PVMFSuccess;

failure:
    return PVMFFailure;
}

static PVMFStatus reportM4ATags(IMpeg4File *mp4Input, MediaScannerClient& client)
{

    OSCL_wHeapString<OsclMemAllocator> valuestring=NULL;
    MP4FFParserOriginalCharEnc charType = ORIGINAL_CHAR_TYPE_UNKNOWN;
    uint16 iLangCode=0;
    uint64 duration;
    uint32 timeScale;
    uint16 trackNum;
    uint16 totalTracks;
    uint32 val;

    char buffer[MAX_STR_LEN];

    // Title
    uint32 i=0;
    for(i=0; i<mp4Input->getNumTitle(); i++)
    {
        mp4Input->getTitle(i,valuestring,iLangCode,charType);
        if (oscl_UnicodeToUTF8(valuestring.get_cstr(),valuestring.get_size(),
            buffer,sizeof(buffer)) > 0)
        {
            if (!client.handleStringTag("title", buffer)) goto failure;
            break;
        }
    }

    // Artist
    for(i=0; i<mp4Input->getNumArtist(); i++)
    {
        mp4Input->getArtist(i,valuestring,iLangCode,charType);
        if (oscl_UnicodeToUTF8(valuestring.get_cstr(),valuestring.get_size(),
            buffer,sizeof(buffer)) > 0)
        {
            if (!client.handleStringTag("artist", buffer)) goto failure; 
            break;
        }
    }

    // Album
    for(i=0; i<mp4Input->getNumAlbum(); i++)
    {
        mp4Input->getAlbum(i,valuestring,iLangCode,charType);
        if (oscl_UnicodeToUTF8(valuestring.get_cstr(),valuestring.get_size(),
            buffer,sizeof(buffer)) > 0)
        {
            if (!client.handleStringTag("album", buffer)) goto failure;
            break;
        }
    }

    // Year
    val = 0;
    for(i=0; i<mp4Input->getNumYear(); i++)
    {
        mp4Input->getYear(i,val);
        sprintf(buffer, "%d", val);
        if (buffer[0])
        {
            if (!client.handleStringTag("year", buffer)) goto failure;
            break;
        }
    }
    
    // Writer/Composer
    if (oscl_UnicodeToUTF8(mp4Input->getITunesWriter().get_cstr(),
        mp4Input->getITunesWriter().get_size(),buffer,sizeof(buffer)) > 0) 
        if (!client.handleStringTag("composer", buffer)) goto failure;

    // Track Data
    trackNum = mp4Input->getITunesThisTrackNo();
    totalTracks = mp4Input->getITunesTotalTracks();
    sprintf(buffer, "%d/%d", trackNum, totalTracks);
    if (!client.handleStringTag("tracknumber", buffer)) goto failure;
    
    // Duration
    duration = mp4Input->getMovieDuration();
    timeScale =  mp4Input->getMovieTimescale();
    // adjust duration to milliseconds if necessary
    if (timeScale != 1000)
        duration = (duration * 1000) / timeScale;
    sprintf(buffer, "%lld", duration);
    if (!client.handleStringTag("duration", buffer)) goto failure;
    
    // Genre
    buffer[0] = 0;
    for(i=0; i<mp4Input->getNumGenre(); i++)
    {
        mp4Input->getGenre(i,valuestring,iLangCode,charType);
        if (oscl_UnicodeToUTF8(valuestring.get_cstr(),valuestring.get_size(), buffer,sizeof(buffer)) > 0)
            break;
    }
    if (buffer[0]) {
        if (!client.handleStringTag("genre", buffer)) goto failure;
    } else {
        uint16 id = mp4Input->getITunesGnreID();
        if (id > 0) {
            sprintf(buffer, "(%d)", id - 1);
            if (!client.handleStringTag("genre", buffer)) goto failure;
        }
    }

    return PVMFSuccess;

failure:
    return PVMFFailure;
}

static PVMFStatus parseMP4(const char *filename, MediaScannerClient& client)
{
    PVFile fileHandle;
    Oscl_FileServer iFs;
        
    if(iFs.Connect() != 0)
    {
        LOGE("Connection with the file server for the parse id3 test failed.\n");
        return PVMFFailure;
    }

    oscl_wchar output[MAX_BUFF_SIZE];
    oscl_UTF8ToUnicode((const char *)filename, oscl_strlen((const char *)filename), (oscl_wchar *)output, MAX_BUFF_SIZE);
    OSCL_wHeapString<OsclMemAllocator> mpegfilename(output);

    IMpeg4File *mp4Input = IMpeg4File::readMP4File(mpegfilename, 1 /* parsing_mode */, &iFs);
    if (mp4Input)
    {
        // check to see if the file contains video
        int32 count = mp4Input->getNumTracks();
        uint32* tracks = new uint32[count];
        bool hasAudio = false;
        bool hasVideo = false;
        if (tracks) {
            mp4Input->getTrackIDList(tracks, count);
            for (int i = 0; i < count; i++) {
                uint32 trackType = mp4Input->getTrackMediaType(tracks[i]);
                OSCL_wHeapString<OsclMemAllocator> streamtype = mp4Input->getTrackMIMEType(tracks[i]);
                char streamtypeutf8[128];
                if (oscl_UnicodeToUTF8(streamtype.get_cstr(),streamtype.get_size(), streamtypeutf8,
                                                                            sizeof(streamtypeutf8)) > 0) {
                    if (strcmp(streamtypeutf8,"UNKNOWN") != 0) {
                        if (trackType ==  MEDIA_TYPE_AUDIO) {
                            hasAudio = true;
                        }else if (trackType ==  MEDIA_TYPE_VISUAL) {
                            hasVideo = true;
                        }
                    } else {
                        //LOGI("@@@@@@@@ %100s: %s\n", filename, streamtypeutf8);
                    }
                }
            }
            
            delete[] tracks;
        }

        if (hasVideo) {
            if (!client.setMimeType("video/mp4")) return PVMFFailure;
        } else if (hasAudio) {
            if (!client.setMimeType("audio/mp4")) return PVMFFailure;
        } else {
            iFs.Close();
            IMpeg4File::DestroyMP4FileObject(mp4Input);
            return PVMFFailure;
        }
        
        PVMFStatus result = reportM4ATags(mp4Input, client); 
        iFs.Close();
        IMpeg4File::DestroyMP4FileObject(mp4Input);
        return result;
    }
    
    return PVMFSuccess;
}

static PVMFStatus parseOgg(const char *filename, MediaScannerClient& client)
{
    int duration;

    FILE *file = fopen(filename,"r");
    if (!file)
        return PVMFFailure;

    OggVorbis_File vf;
    if(ov_open(file, &vf, NULL, 0) < 0) {
        return PVMFFailure;
    }

    char **ptr=ov_comment(&vf,-1)->user_comments;
    while(*ptr){
        char *val = strstr(*ptr, "=");
        if (val) {
            int keylen = val++ - *ptr;
            char key[keylen + 1];
            strncpy(key, *ptr, keylen);
            key[keylen] = 0;
            if (!client.handleStringTag(key, val)) goto failure;
        }
        ++ptr;
    }
    
    // Duration
    duration = ov_time_total(&vf, -1);
    if (duration > 0) {
        char buffer[20];
        sprintf(buffer, "%d", duration);
        if (!client.handleStringTag("duration", buffer)) goto failure;
    }

    ov_clear(&vf); // this also closes the FILE
    return PVMFSuccess;

failure:
    ov_clear(&vf); // this also closes the FILE
    return PVMFFailure;
}

static PVMFStatus parseMidi(const char *filename, MediaScannerClient& client) {
    
    // get the library configuration and do sanity check
    const S_EAS_LIB_CONFIG* pLibConfig = EAS_Config();
    if ((pLibConfig == NULL) || (LIB_VERSION != pLibConfig->libVersion)) {
        LOGE("EAS library/header mismatch\n");
        return PVMFFailure;
    }
    EAS_I32 temp;

    // spin up a new EAS engine
    EAS_DATA_HANDLE easData = NULL;
    EAS_HANDLE easHandle = NULL;
    EAS_RESULT result = EAS_Init(&easData);
    if (result == EAS_SUCCESS) {
        EAS_FILE file;
        file.path = filename;
        file.fd = 0;
        file.offset = 0;
        file.length = 0;
        result = EAS_OpenFile(easData, &file, &easHandle, NULL);
    }
    if (result == EAS_SUCCESS) {
        result = EAS_Prepare(easData, easHandle);
    }
    if (result == EAS_SUCCESS) {
        result = EAS_ParseMetaData(easData, easHandle, &temp);
    }
    if (easHandle) {
        EAS_CloseFile(easData, easHandle);
    }
    if (easData) {
        EAS_Shutdown(easData);
    }

    if (result != EAS_SUCCESS) {
        return PVMFFailure;
    }

    char buffer[20];
    sprintf(buffer, "%ld", temp);
    if (!client.handleStringTag("duration", buffer)) return PVMFFailure;
    return PVMFSuccess;
}

static PVMFStatus parseWMA(const char *filename, MediaScannerClient& client)
{    
    MediaMetadataRetriever::create();
    MediaMetadataRetriever::setMode( 1 /*MediaMetadataRetriever.MODE_GET_METADATA_ONLY*/);
    status_t status = MediaMetadataRetriever::setDataSource(filename);
    if (status) {
        LOGD("parseWMA setDataSource returned %d\n", status);
        MediaMetadataRetriever::release();
        return PVMFFailure;
    }
    
    const char* value;

    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_IS_DRM_CRIPPLED);
    if (value && strcmp(value, "true") == 0) {
        // we don't support WMDRM currently
        // setting this invalid mimetype will make the java side ignore this file
        client.setMimeType("audio/x-wma-drm");
    }
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_CODEC);
    if (value && strcmp(value, "Windows Media Audio 10 Professional") == 0) {
        // we don't support WM 10 Professional currently
        // setting this invalid mimetype will make the java side ignore this file
        client.setMimeType("audio/x-wma-10-professional");
    }

    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_ALBUM);
    if (value)
        client.handleStringTag("album", value);
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_ARTIST);
    if (value)
        client.handleStringTag("artist", value);
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_COMPOSER);
    if (value)
        client.handleStringTag("composer", value);
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_GENRE);
    if (value)
        client.handleStringTag("genre", value);
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_TITLE);
    if (value)
        client.handleStringTag("title", value);
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_YEAR);
    if (value)
        client.handleStringTag("year", value);
    value = MediaMetadataRetriever::extractMetadata(METADATA_KEY_CD_TRACK_NUMBER);
    if (value)
        client.handleStringTag("tracknumber", value);
   
    MediaMetadataRetriever::release();
    return PVMFSuccess;
}

status_t MediaScanner::processFile(const char *path, const char* mimeType, MediaScannerClient& client)
{
    status_t result;
    InitializeForThread();

    //LOGD("processFile %s mimeType: %s\n", path, mimeType);
    const char* extension = strrchr(path, '.');

    if (extension && strcasecmp(extension, ".mp3") == 0) {
        result = parseMP3(path, client);
    } else if (extension && 
        (strcasecmp(extension, ".mp4") == 0 || strcasecmp(extension, ".m4a") == 0 ||
         strcasecmp(extension, ".3gp") == 0 || strcasecmp(extension, ".3gpp") == 0 ||
         strcasecmp(extension, ".3g2") == 0 || strcasecmp(extension, ".3gpp2") == 0)) {
        result = parseMP4(path, client);
    } else if (extension && strcasecmp(extension, ".ogg") == 0) {
        result = parseOgg(path, client);
    } else if (extension &&
        ( strcasecmp(extension, ".mid") == 0 || strcasecmp(extension, ".smf") == 0
        || strcasecmp(extension, ".imy") == 0)) {
        result = parseMidi(path, client);
    } else if (extension && strcasecmp(extension, ".wma") == 0) {
        result = parseWMA(path, client);
    } else {
        result = PVMFFailure;
    }

    return result;
}

static bool fileMatchesExtension(const char* path, const char* extensions) {
    char* extension = strrchr(path, '.');
    if (!extension) return false;
    ++extension;    // skip the dot
    if (extension[0] == 0) return false;
    
    while (extensions[0]) {
        char* comma = strchr(extensions, ',');
        int length = (comma ? comma - extensions : strlen(extensions));
        if (length == strlen(extension) && strncasecmp(extension, extensions, length) == 0) return true;
        extensions += length;
        if (extensions[0] == ',') ++extensions;
    }
    
    return false;
}

status_t MediaScanner::doProcessDirectory(char *path, int pathRemaining, const char* extensions, 
        MediaScannerClient& client, ExceptionCheck exceptionCheck, void* exceptionEnv)
{
    // place to copy file or directory name
    char* fileSpot = path + strlen(path);
    struct dirent* entry;

    // ignore directories that contain a  ".nomedia" file
    if (pathRemaining >= 8 /* strlen(".nomedia") */ ) {
        strcpy(fileSpot, ".nomedia");
        if (access(path, F_OK) == 0) {
            LOGD("found .nomedia, skipping directory\n");
            return OK;
        }
        
        // restore path
        fileSpot[0] = 0;
    }
    
    DIR* dir = opendir(path);
    if (!dir) {
        LOGD("opendir %s failed, errno: %d", path, errno);
        return PVMFFailure;
    }
    
    while ((entry = readdir(dir))) {
        const char* name = entry->d_name;

        // ignore "." and ".."
        if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0))) {
            continue;
        }
        
        int type = entry->d_type;
        if (type == DT_REG || type == DT_DIR) {
            int nameLength = strlen(name);
            bool isDirectory = (type == DT_DIR);
            
            if (nameLength > pathRemaining || (isDirectory && nameLength + 1 > pathRemaining)) {
                // path too long!
                continue;
            }
            
            strcpy(fileSpot, name);
            if (isDirectory) {
                // ignore directories with a name that starts with '.'
                // for example, the Mac ".Trashes" directory
                if (name[0] == '.') continue;
                
                strcat(fileSpot, "/");
                int err = doProcessDirectory(path, pathRemaining - nameLength - 1, extensions, client, exceptionCheck, exceptionEnv);
                if (err) goto failure;
            } else if (fileMatchesExtension(path, extensions)) {
                struct stat statbuf;
                stat(path, &statbuf);
                if (statbuf.st_size > 0) {
                    client.scanFile(path, statbuf.st_mtime, statbuf.st_size);
                }
                if (exceptionCheck && exceptionCheck(exceptionEnv)) goto failure;
            }
        }       
    }
    
    closedir(dir);
    return OK;
failure:
    closedir(dir);
    return -1;
}

status_t MediaScanner::processDirectory(const char *path, const char* extensions, 
        MediaScannerClient& client, ExceptionCheck exceptionCheck, void* exceptionEnv)
{
    InitializeForThread();
    
    int pathLength = strlen(path);
    if (pathLength >= PATH_MAX) {
        return PVMFFailure;
    }
    char* pathBuffer = (char *)malloc(PATH_MAX + 1);
    if (!pathBuffer) {
        return PVMFFailure;
    }
    
    int pathRemaining = PATH_MAX - pathLength;
    strcpy(pathBuffer, path);
    if (pathBuffer[pathLength - 1] != '/') {
        pathBuffer[pathLength] = '/';
        pathBuffer[pathLength + 1] = 0;
        --pathRemaining;
    }
    
    status_t result = doProcessDirectory(pathBuffer, pathRemaining, extensions, client, exceptionCheck, exceptionEnv);
    
    free(pathBuffer);
    return result;
}

static char* doExtractAlbumArt(PvmfApicStruct* aApic)
{
    char *data = (char*)malloc(aApic->iGraphicDataLen + 4);
    if (data) {
        long *len = (long*)data;
        *len = aApic->iGraphicDataLen;
        memcpy(data + 4, aApic->iGraphicData, *len);
    }
    return data;
}

static char* extractMP3AlbumArt(int fd)
{
    PVID3ParCom pvId3Param;
    PVFile file;
    OsclFileHandle *filehandle;
    Oscl_FileServer iFs;
    
    if(iFs.Connect() != 0)
    {
        LOGE("Connection with the file server for the parse id3 test failed.\n");
        return NULL;
    }

    FILE *f = fdopen(fd, "r");
    filehandle = new OsclFileHandle(f);
    file.SetFileHandle(filehandle); 

    if( 0 != file.Open(NULL, Oscl_File::MODE_READ | Oscl_File::MODE_BINARY, iFs) ) 
    {
        LOGE("Could not open the input file for reading(Test: parse id3).\n");
        return NULL;
    }

    file.Seek(0, Oscl_File::SEEKSET);
    pvId3Param.ParseID3Tag(&file);
    file.Close();
    iFs.Close();
   
    //Get the frames information from ID3 library
    PvmiKvpSharedPtrVector framevector;
    pvId3Param.GetID3Frames(framevector);
        
    uint32 num_frames = framevector.size();
    for (uint32 i = 0; i < num_frames; i++)
    {
        const char* key = framevector[i]->key;
        
        // type should follow first semicolon
        const char* type = strchr(key, ';') + 1;
        if (type == 0) continue;
        const char* value = framevector[i]->value.pChar_value;
        const unsigned char* src = (const unsigned char *)value;
        
        if (oscl_strncmp(key,KVP_KEY_ALBUMART,oscl_strlen(KVP_KEY_ALBUMART)) == 0)
        {
            PvmfApicStruct* aApic = (PvmfApicStruct*)framevector[i]->value.key_specific_value;
            if (aApic) {
                char* result = doExtractAlbumArt(aApic);
                if (result)
                    return result;
            }
        } 
    }

    return NULL;
}

static char* extractM4AAlbumArt(int fd)
{
    PVFile file;
    OsclFileHandle *filehandle;
    Oscl_FileServer iFs;
    char* result = NULL;
        
    if(iFs.Connect() != 0)
    {
         LOGE("Connection with the file server for the parse id3 test failed.\n");
        return NULL;
    }

    FILE *f = fdopen(fd, "r");
    filehandle = new OsclFileHandle(f);
    file.SetFileHandle(filehandle); 

    oscl_wchar output[MAX_BUFF_SIZE];
    oscl_UTF8ToUnicode("", 0, (oscl_wchar *)output, MAX_BUFF_SIZE);
    OSCL_wHeapString<OsclMemAllocator> mpegfilename(output);
    IMpeg4File *mp4Input = IMpeg4File::readMP4File(
            mpegfilename, /* name */
            NULL, /* plugin access interface factory */
            filehandle,
            0, /* parsing_mode */
            &iFs);

    if (!mp4Input)
        return NULL;

    PvmfApicStruct* aApic = mp4Input->getITunesImageData();
    if (aApic) {
        result = doExtractAlbumArt(aApic);
    }
    
    IMpeg4File::DestroyMP4FileObject(mp4Input);
    return result; 
}


char* MediaScanner::extractAlbumArt(int fd)
{
    InitializeForThread();

    int32 ident;
    lseek(fd, 4, SEEK_SET);
    read(fd, &ident, sizeof(ident));

    if (ident == 0x70797466) {
        // some kind of mpeg 4 stream
        lseek(fd, 0, SEEK_SET);
        return extractM4AAlbumArt(fd);
    } else {
        // might be mp3
        return extractMP3AlbumArt(fd);
    }
}

}; // namespace android
