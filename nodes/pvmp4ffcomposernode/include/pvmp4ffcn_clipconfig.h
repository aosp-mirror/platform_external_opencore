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
/**
 * @file pvmp4ffcn_clipconfig.h
 * @brief Clip level configuration of PVMp4FFComposerNode
 */

#ifndef PVMP4FFCN_CLIPCONFIG_H_INCLUDED
#define PVMP4FFCN_CLIPCONFIG_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef OSCL_STRING_H_INCLUDED
#include "oscl_string.h"
#endif
#ifndef PVMF_RETURN_CODES_H_INCLUDED
#include "pvmf_return_codes.h"
#endif
#ifndef PV_UUID_H_INCLUDED
#include "pv_uuid.h"
#endif
#ifndef PV_INTERFACE_H_INCLUDED
#include "pv_interface.h"
#endif
#ifndef PV_MP4FFCOMPOSER_CONFIG_H_INCLUDED
#include "pv_mp4ffcomposer_config.h"
#endif

#define KPVMp4FFCNClipConfigUuid PVUuid(0x2e3b479f,0x2c46,0x465c,0xba,0x41,0xb8,0x91,0x11,0xa9,0xdf,0x3a)

typedef enum
{
    /**
     * This mode authors non Progressive Downloadable output files using temp files
     * during authoring:
     * Meta data towards the end of the clip
     * Media data is not interleaved. Temp files are used.
     * Media data is authored in separate media atoms for each track
     * Temporary files are written to the same directory as the output file.
     */
    PVMP4FFCN_PV_FAST_TRACK_CONTENT_MODE = 0x00000000,

    /**
     * This mode authors 3GPP Progressive Downloadable output files:
     * Meta Data is upfront.
     * Media Data is interleaved. Temp files are used.
     * Temporary files are written to the same directory as the output file.
     */
    PVMP4FFCN_3GPP_PROGRESSIVE_DOWNLOAD_MODE = 0x00000003,

    /**
     * This mode authors 3GPP Downloadable output files:
     * Meta Data is towards the end of the clip.
     * Media Data is interleaved.
     * No temp files are used.
     */
    PVMP4FFCN_3GPP_DOWNLOAD_MODE = 0x00000009,



    /**
     * This mode authors movie fragment files:
     * Meta Data is towards the end of the clip in MOOV and MOOF.
     * Media Data is interleaved.
     * No temp files are used.
     */
    PVMP4FFCN_MOVIE_FRAGMENT_MODE = 0x00000021


} PVMp4FFCN_AuthoringMode;

#ifdef XXXX


typedef enum
{
    /**
     * This mode authors non Progressive Downloadable output files using temp files
     * during authoring:
     * Meta data towards the end of the clip
     * Media data is not interleaved. Temp files are used.
     * Media data is authored in separate media atoms for each track
     * Temporary files are written to the same directory as the output file.
     */
    PVMP4FFCN_NON_3GPP_PROGRESSIVE_DOWNLOADABLE_WITH_TEMP_FILES = 0x00000000,

    /**
     * This mode authors 3GPP Progressive Downloadable output files:
     * Meta Data is upfront.
     * Media Data is interleaved. Temp files are used.
     * Temporary files are written to the same directory as the output file.
     */
    PVMF4FFCN_3GPP_PROGRESSIVE_DOWNLOADABLE = 0x00000003,


    /**
     * This mode authors non Progressive Downloadable output files without using
     * temp files during authoring:
     * Meta data towards the end of the clip
     * Media data is interleaved. No temp files are used.
     */
    PVMP4FFCN_NON_3GPP_PROGRESSIVE_DOWNLOADABLE_NO_TEMP_FILES = 0x00000009
} PVMp4FFCN_AuthoringMode;
#endif
/**
 * PVMp4FFCNClipConfigInterface allows a client to control properties of PVMp4FFComposerNode
 */
class PVMp4FFCNClipConfigInterface : public PVInterface
{
    public:
        /**
         * Register a reference to this interface.
         */
        virtual void addRef() = 0;

        /**
         * Remove a reference to this interface.
         */
        virtual void removeRef() = 0;

        /**
         * Query for an instance of a particular interface.
         *
         * @param uuid Uuid of the requested interface
         * @param iface Output parameter where pointer to an instance of the
         * requested interface will be stored if it is supported by this object
         * @return true if the requested interface is supported, else false
         */
        virtual bool queryInterface(const PVUuid& uuid, PVInterface*& iface) = 0;

        /**
         * This method sets the output file name. This method must be called before
         * Start() is called.
         *
         * @param aFileName Output file name
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetOutputFileName(const OSCL_wString& aFileName) = 0;

        /**
         * This method sets the authoring mode. This method must be called before
         * Start() is called.  Default authoring mode is non 3GPPProgressive
         * Downloadable file using no temp files durating authoring.
         *
         * @param aAuthoringMode Authoring mode.
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetAuthoringMode(PVMp4FFCN_AuthoringMode aAuthoringMode = PVMP4FFCN_3GPP_DOWNLOAD_MODE) = 0;

        /**
         * Method to set the sample rate (i.e. timescale) for the overall Mpeg-4 presentation.
         * This is an optional configuration API that should be called before Start() is called.
         *
         * @param aTimescale Timescale of MPEG4 presentation
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetPresentationTimescale(uint32 aTimescale) = 0;

        /**
         * This method populates the version string. Version string contains
         * information about the version of the author SDK/app that is authoring
         * the clip. Currently only wide char strings are supported.
         */
        /* This is an optional configuration API that should be called before Start() is called.
         *
         * @param aVersion   version string.
         * @param aLangCode  16 bit ISO-639-2/T Language code
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetVersion(const OSCL_wString& aVersion, uint16 aLangCode = 0) = 0;

        /**
         * This method populates the title string. Title string contains
         * the title of the authored clip.Currently only wide char strings are supported.
         */


        /* This is an optional configuration API that should be called before Start() is called.
         *
         * @param aTitle   title string.
         * @param aLangCode  16 bit ISO-639-2/T Language code
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetTitle(const OSCL_wString& aTitle, uint16 aLangCode = 0) = 0;

        /**
         * This method populates the author string. Title string information about
         * the the author of the clip (actual user - like authored by grandma martha).
         * Currently only wide char strings are supported.
         */
        /* This is an optional configuration API that should be called before Start() is called.
         *
         * @param aAuthor   author string.
         * @param aLangCode  16 bit ISO-639-2/T Language code
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetAuthor(const OSCL_wString& aAuthor, uint16 aLangCode = 0) = 0;


        /**
         * This method populates the copyright string. We do not support the
         * authoring of ISO defined copyright atom yet. This info is used to populate
         * the PV proprietary user data atom.
         */

        /* This is an optional configuration API that should be called before Start() is called.
         *
         * @param aCopyright   Copyright string.
         * @param aLangCode  16 bit ISO-639-2/T Language code
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetCopyright(const OSCL_wString& aCopyright, uint16 aLangCode = 0) = 0;

        /**
         * This method populates the description string. Description string contains
         * some brief description of the clip (viz. surfing on the beach). Currently only
         * wide char strings are supported.
         */

        /* This is an optional configuration API that should be called before Start() is called.
         *
         * @param aDescription   Description string.
         * @param aLangCode  16 bit ISO-639-2/T Language code
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetDescription(const OSCL_wString& aDescription, uint16 aLangCode = 0) = 0;

        /**
         * This method populates the rating string. Rating string contains
         * some information about the clip rating (viz.PG-13). Currently only
         * wide char strings are supported.
         */
        /* This is an optional configuration API that should be called before Start() is called.
         *
         * @param aRating   Rating string.
         * @param aLangCode  16 bit ISO-639-2/T Language code
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetRating(const OSCL_wString& aRating, uint16 aLangCode = 0) = 0;

        /**
         * This method ests the creation date in ISO 8601 format
         *
         * This is an optional configuration API that should be called before Start() is called.
         *
         * @param aCreationDate Creation date in ISO 8601 format.
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetCreationDate(const OSCL_wString& aCreationDate) = 0;

        /**
         * This method sets the real time authoring mode where incoming timestamps
         * are adjusted when authoring starts and checks are done to ensure that
         * incremental timstamps do not have 0 or negative deltas. By default, this
         * authoring mode is disabled.
         *
         * This is an optional configuration API that should be called before Start() is called.
         *
         * @param aRealTime  Use real time authoring or not.
         * @return Completion status of this method.
         */
        virtual PVMFStatus SetRealTimeAuthoring(const bool aRealTime) = 0;
};

#endif // PVMP4FFCN_CLIPCONFIG_H_INCLUDED


