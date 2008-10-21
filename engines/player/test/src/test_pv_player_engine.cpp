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
#ifndef TEST_PV_PLAYER_ENGINE_H_INCLUDED
#include "test_pv_player_engine.h"
#endif

#ifndef TEST_PV_PLAYER_ENGINE_TESTSET1_H_INCLUDED
#include "test_pv_player_engine_testset1.h"
#endif

#ifndef TEST_PV_PLAYER_ENGINE_TESTSET7_H_INCLUDED
#include "test_pv_player_engine_testset7.h"
#endif

#ifndef TEST_PV_PLAYER_ENGINE_TESTSET8_H_INCLUDED
#include "test_pv_player_engine_testset8.h"
#endif

#if RUN_HTTPDOWNLOAD_TESTCASES
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET5_H_INCLUDED
#include "test_pv_player_engine_testset5.h"
#endif
#endif

#if RUN_STREAMING_TESTCASES
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET6_H_INCLUDED
#include "test_pv_player_engine_testset6.h"
#endif
#endif

#ifndef TEST_PV_PLAYER_ENGINE_TESTSET9_H_INCLUDED
#include "test_pv_player_engine_testset9.h"
#endif

#ifndef TEST_PV_PLAYER_ENGINE_TESTSET10_H_INCLUDED
#include "test_pv_player_engine_testset10.h"
#endif

#ifndef TEST_PV_PLAYER_ENGINE_TESTSET11_H_INCLUDED
#include "test_pv_player_engine_testset11.h"
#endif
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET12_H_INCLUDED
#include "test_pv_player_engine_testset12.h"
#endif
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET13_H_INCLUDED
#include "test_pv_player_engine_testset13.h"
#endif

#if RUN_APP_TRACK_SELECTION_TESTCASES
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET_APPTRACKSELECTION_H_INCLUDED
#include "test_pv_player_engine_testset_apptrackselection.h"
#endif
#endif //RUN_APP_TRACK_SELECTION_TESTCASES


#if RUN_CPMJUPITER_TESTCASES
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET_CPMJUPITER_H_INCLUDED
#include "test_pv_player_engine_testset_cpmjupiter.h"
#endif
#endif

#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET_CPMDLAPASSTHRU_H_INCLUDED
#include "test_pv_player_engine_testset_cpmdlapassthru.h"
#endif
#endif

#if RUN_CPMACCESS_TESTCASES
#ifndef TEST_PV_PLAYER_ENGINE_TESTSET_CPMACCESS_H_INCLUDED
#include "test_pv_player_engine_testset_cpmaccess.h"
#endif
#endif


#ifndef OSCL_MEM_H_INCLUDED
#include "oscl_mem.h"
#endif

#ifndef OSCL_MEM_AUDIT_H_INCLUDED
#include "oscl_mem_audit.h"
#endif

#ifndef OSCL_ERROR_H_INCLUDED
#include "oscl_error.h"
#endif

#ifndef OSCL_ERROR_PANIC_H_INCLUDED
#include "oscl_error_panic.h"
#endif

#ifndef OSCL_SCHEDULER_H_INCLUDED
#include "oscl_scheduler.h"
#endif

#ifndef PVLOGGER_H_INCLUDED
#include "pvlogger.h"
#endif

#ifndef PVLOGGER_FILE_APPENDER_H_INCLUDED
#include "pvlogger_file_appender.h"
#endif

#ifndef PVLOGGER_MEM_APPENDER_H_INCLUDED
#include "pvlogger_mem_appender.h"
#endif

#ifndef __UNIT_TEST_TEST_ARGS__
#include "unit_test_args.h"
#endif

#ifdef BUILD_N_ARM
#ifndef __OMF_MC_H__
#include "omf_mc.h"
#endif

#define CONFIG_FILE_NAME "/usr/lib/omf/omx_media_player.cfg"
#endif

#include "omx_core.h"
#include "pv_omxmastercore.h"

#ifndef OSCL_UTF8CONV_H
#include "oscl_utf8conv.h"
#endif

#ifndef DEFAULTSOURCEFILENAME
#error // The default source file needs to be defined in config file
#endif

#ifndef DEFAULTSOURCEFORMATTYPE
#error // The format type for default source file needs to be defined in config file
#endif

#ifndef OSCL_STRING_UTILS_H_INCLUDED
#include "oscl_string_utils.h"
#endif


FILE *file;

class PVLoggerConfigFile
{
        /*  To change the logging settings without the need to compile the test application
        	Let us read the logging settings from the file instead of hard coding them over here
        	The name of the config file is pvlogger.ini
        	The format of entries in it is like
        	First entry will decide if the file appender has to be used or error appender will be used.
        	0 -> ErrAppender will be used
        	1 -> File Appender will be used
        	2 -> Mem Appender will be used
        	Entries after this will decide the module whose logging has to be taken.For example, contents of one sample config file could be
        	1
        	1,PVPlayerEngine
        	8,PVSocketNode
        	(pls note that no space is allowed between loglevel and logger tag)
        	This means, we intend to have logging of level 1 for the module PVPlayerEngine
        	and of level 8 for the PVSocketNode on file.
        */
    public:

        PVLoggerConfigFile(): iLogFileRead(false)
        {
            iFileServer.Connect();
            // Full path of pvlogger.ini is: SOURCENAME_PREPEND_STRING + pvlogger.ini
            oscl_strncpy(iLogFileName, SOURCENAME_PREPEND_STRING,
                         oscl_strlen(SOURCENAME_PREPEND_STRING) + 1);
            oscl_strcat(iLogFileName, "pvlogger.ini");
            oscl_memset(ibuffer, 0, sizeof(ibuffer));
            iAppenderType = 0;

        }

        ~PVLoggerConfigFile()
        {
            iFileServer.Close();
        }

        bool IsLoggerConfigFilePresent()
        {
            if (-1 != ReadAndParseLoggerConfigFile())
                return true;
            return false;
        }

        //Read and parse the config file
        //retval = -1 if the config file doesnt exist
        int8 ReadAndParseLoggerConfigFile()
        {
            int8 retval = 1;

            if (0 != iLogFile.Open(iLogFileName, Oscl_File::MODE_READ, iFileServer))
            {
                retval = -1;
            }
            else
            {
                if (!iLogFileRead)
                {
                    int32 nCharRead = iLogFile.Read(ibuffer, 1, sizeof(ibuffer));
                    //Parse the buffer for \n chars
                    Oscl_Vector<char*, OsclMemAllocator> LogConfigStrings;

                    //Get the logger strings
                    const char* const lnFd = "\r\n";
                    const int8 lnFdLen = oscl_strlen(lnFd);
                    int16 offset = 0;
                    char* lastValidBffrAddr = ibuffer + oscl_strlen(ibuffer);
                    char* lnFdIndx = oscl_strstr(ibuffer, lnFd);
                    while (lnFdIndx != NULL && lnFdIndx < lastValidBffrAddr)
                    {
                        oscl_memset(lnFdIndx, '\0', lnFdLen);
                        LogConfigStrings.push_back(ibuffer + offset);
                        offset = (lnFdIndx + lnFdLen) - ibuffer;
                        lnFdIndx = oscl_strstr(ibuffer + offset, lnFd);
                    }
                    if (NULL == lnFdIndx && ((ibuffer + offset) < lastValidBffrAddr)) //If \r\n is skipped after the last logging str in ini file
                    {
                        LogConfigStrings.push_back(ibuffer + offset);
                    }


                    //Populate the  LoggerConfigElements vector
                    {
                        if (!LogConfigStrings.empty())
                        {
                            Oscl_Vector<char*, OsclMemAllocator>::iterator it;
                            it = LogConfigStrings.begin();
                            uint32 appenderType;
                            PV_atoi(*it, 'd', oscl_strlen(*it), appenderType);
                            iAppenderType = appenderType;
                            if (LogConfigStrings.size() > 1)
                            {
                                for (it = LogConfigStrings.begin() + 1; it != LogConfigStrings.end(); it++)
                                {
                                    char* CommaIndex = oscl_strstr(*it, ",");
                                    if (CommaIndex != NULL)
                                    {
                                        *CommaIndex = '\0';
                                        LoggerConfigElement obj;
                                        uint32 logLevel;
                                        PV_atoi(*it, 'd', oscl_strlen(*it), logLevel);
                                        obj.iLogLevel = logLevel;
                                        obj.iLoggerString = CommaIndex + 1;
                                        iLoggerConfigElements.push_back(obj);
                                    }
                                }
                            }
                            else
                            {
                                //Add the config element for complete logging fo all the modules
                                LoggerConfigElement obj;
                                obj.iLoggerString = "";
                                obj.iLogLevel = 8;
                                iLoggerConfigElements.push_back(obj);
                            }
                        }
                    }
                    iLogFile.Close();
                    iLogFileRead = true;
                }
            }
            return retval;
        }

        void SetLoggerSettings()
        {
            Oscl_Vector<LoggerConfigElement, OsclMemAllocator>::iterator it;

            PVLoggerAppender *appender = NULL;
            OsclRefCounter *refCounter = NULL;
            if (iLoggerConfigElements.empty())
            {
                return;
            }

            if (iAppenderType == 0)
            {
                appender = new StdErrAppender<TimeAndIdLayout, 1024>();
                OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
                    new OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > >(appender);
                refCounter = appenderRefCounter;
            }
            else if (iAppenderType == 1)
            {
                OSCL_wHeapString<OsclMemAllocator> logfilename(OUTPUTNAME_PREPEND_WSTRING);
                logfilename += _STRLIT_WCHAR("player.log");
                appender = (PVLoggerAppender*)TextFileAppender<TimeAndIdLayout, 1024>::CreateAppender(logfilename.get_str());
                OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
                    new OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > >(appender);
                refCounter = appenderRefCounter;
            }
            else
            {
                OSCL_wHeapString<OsclMemAllocator> logfilename(OUTPUTNAME_PREPEND_WSTRING);
                logfilename += _STRLIT_WCHAR("player.log");
                appender = (PVLoggerAppender*)MemAppender<TimeAndIdLayout, 1024>::CreateAppender(logfilename.get_str());
                OsclRefCounterSA<LogAppenderDestructDealloc<MemAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
                    new OsclRefCounterSA<LogAppenderDestructDealloc<MemAppender<TimeAndIdLayout, 1024> > >(appender);
                refCounter = appenderRefCounter;
            }

            OsclSharedPtr<PVLoggerAppender> appenderPtr(appender, refCounter);

            for (it = iLoggerConfigElements.begin(); it != iLoggerConfigElements.end(); it++)
            {
                PVLogger *node = NULL;
                node = PVLogger::GetLoggerObject(it->iLoggerString);
                node->AddAppender(appenderPtr);
                node->SetLogLevel(it->iLogLevel);
            }
        }

    private:
        class LoggerConfigElement
        {
            public:
                LoggerConfigElement()
                {
                    iLoggerString = NULL;
                    iLogLevel = 8;
                }
                char *iLoggerString;
                int8 iLogLevel;
        };
        int8 iAppenderType; //Type of appender to be used for the logging 0-> Err Appender, 1-> File Appender
        bool iLogFileRead;
        Oscl_File iLogFile;
        Oscl_FileServer iFileServer;
        char iLogFileName[255];
        char ibuffer[1024];
        Oscl_Vector<LoggerConfigElement, OsclMemAllocator> iLoggerConfigElements;
};


/** Name: ConvertToLowerCase
  * Description: Utility function to convert a string in lower case
  *				 convert upper case ASCII String to lower case.
  *				 behaviour of this function for non-ASCII characters
  * 			 is not defined.
  *
  * @param char    input string.
  * @return		   lower case String.
  */
void ConvertToLowerCase(char *aString)
{
    int32 counter = 0;
    while (aString[counter])
    {
        // convert the input character to lower case
        aString[counter] = oscl_tolower(aString[counter]);
        counter++;
    }

}

/*Structs n funcs for parsing the logger config file completes*/

// Parse source type from arguments
void FindSourceType(cmd_line* command_line, OSCL_HeapString<OsclMemAllocator> &aFileNameInfo, PVMFFormatType &aInputFileFormatType, int32 &aTemp_Flag, FILE *aFile)
{
    bool cmdline_iswchar = command_line->is_wchar();
    int count = command_line->get_count(); //Getting number of arguments

    // use mp4 as default test file
    aFileNameInfo = SOURCENAME_PREPEND_STRING;
    aFileNameInfo += DEFAULTSOURCEFILENAME;
    aInputFileFormatType = DEFAULTSOURCEFORMATTYPE;


    // Search for the "sourcetype(example --- mp3)" argument
    // Go through each argument

    for (int formatSearch = 0; formatSearch < count; formatSearch++)
    {
        char argstr[128];
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            oscl_wchar* argwstr = NULL;
            command_line->get_arg(formatSearch, argwstr); //getting the value of argument at formatSearch position
            oscl_UnicodeToUTF8(argwstr, oscl_strlen(argwstr), argstr, 128);
            argstr[127] = NULL;
        }
        else
        {
            char* tmpstr = NULL;
            command_line->get_arg(formatSearch, tmpstr);
            int32 tmpstrlen = oscl_strlen(tmpstr) + 1;
            if (tmpstrlen > 128)
            {
                tmpstrlen = 128;
            }
            oscl_strncpy(argstr, tmpstr, tmpstrlen);
            argstr[tmpstrlen-1] = NULL;
        }

        ConvertToLowerCase(argstr); //Convert the string in lower case


        // Do the string compare
        if (oscl_strcmp(argstr, "-help") == NULL)
        {
            fprintf(aFile, "Format type specification option. Default is 'mp4':\n");
            fprintf(aFile, "Specify the format type name in upper case/lower case letter for test cases which\n");
            fprintf(aFile, "   allow user-specified format type.\n");
            fprintf(aFile, "e.g. for executing MP4 specific tests: test_pv_player_engine.exe MP4)\n");
            break;
        }
        else if (oscl_strcmp(argstr, "mp3") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_MP3_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_MP3_CBR;
            aTemp_Flag = MP3_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "aac") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_AAC_ADTS;
            aTemp_Flag = AAC_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "amr") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_AMR_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_AMR_IETF;
            aTemp_Flag = AMR_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "mp4") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_MP4_M4V_AMR;
            aTemp_Flag = MP4_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "3gp") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_3GP_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_3GP;
            aTemp_Flag = THREE_GP_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "asf") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_ASF_FORMAT_TYPE ;
            aFileNameInfo = LOCAL_TEST_FILE_ASF;
            aTemp_Flag = ASF_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "rm") == NULL)
        {
            aInputFileFormatType = LOCAL_TEST_FILE_REALAUDIO_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_REALAUDIO;
            aTemp_Flag = RM_ENABLED;
            break;

        }
        else if (oscl_strcmp(argstr, "wav") == NULL)
        {
            aInputFileFormatType =  LOCAL_TEST_FILE_WAV_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_WAV;
            aTemp_Flag = WAV_ENABLED;
            break;
        }
        else if (oscl_strcmp(argstr, "wmv") == NULL)
        {
            aInputFileFormatType =  LOCAL_TEST_FILE_WMV_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_WMV;
            aTemp_Flag = WMV_ENABLED;
            break;
        }
        else if (oscl_strcmp(argstr, "wma") == NULL)
        {
            aInputFileFormatType =  LOCAL_TEST_FILE_WMA_FORMAT_TYPE;
            aFileNameInfo = LOCAL_TEST_FILE_WMA;
            aTemp_Flag = WMA_ENABLED;
            break;
        }

    }

}

// Pull out source file name from arguments
//	-source sometestfile.mp4

void FindSourceFile(cmd_line* command_line, OSCL_HeapString<OsclMemAllocator> &aFileNameInfo, PVMFFormatType &aInputFileFormatType, FILE *aFile)
{
    int iFileArgument = 0;
    bool iFileFound = false;
    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-source" argument
    // Go through each argument
    for (int iFileSearch = 0; iFileSearch < count; iFileSearch++)
    {
        char argstr[128];
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            oscl_wchar* argwstr = NULL;
            command_line->get_arg(iFileSearch, argwstr);
            oscl_UnicodeToUTF8(argwstr, oscl_strlen(argwstr), argstr, 128);
            argstr[127] = '\0';
        }
        else
        {
            char* tmpstr = NULL;
            command_line->get_arg(iFileSearch, tmpstr);
            int32 tmpstrlen = oscl_strlen(tmpstr) + 1;
            if (tmpstrlen > 128)
            {
                tmpstrlen = 128;
            }
            oscl_strncpy(argstr, tmpstr, tmpstrlen);
            argstr[tmpstrlen-1] = '\0';
        }

        // Do the string compare
        if (!oscl_strcmp(argstr, "-help"))
        {
            fprintf(aFile, "Source specification option. Default is 'test.mp4':\n");
            fprintf(aFile, "  -source sourcename\n");
            fprintf(aFile, "   Specify the source filename or URL to use for test cases which\n");
            fprintf(aFile, "   allow user-specified source name. The unit test determines the\n");
            fprintf(aFile, "   source format type using extension or URL header.\n\n");
        }
        else if (!oscl_strcmp(argstr, "-source"))
        {
            iFileFound = true;
            iFileArgument = ++iFileSearch;
            break;
        }
    }

    if (iFileFound)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            oscl_wchar* cmd;
            command_line->get_arg(iFileArgument, cmd);
            char tmpstr[256];
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), tmpstr, 256);
            tmpstr[255] = '\0';
            aFileNameInfo = tmpstr;
        }
        else
        {
            char* cmdlinefilename = NULL;
            command_line->get_arg(iFileArgument, cmdlinefilename);
            aFileNameInfo = cmdlinefilename;
        }

        // Check the file extension to determine format type
        // AAC file
        if (oscl_strstr(aFileNameInfo.get_cstr(), ".aac") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".AAC") != NULL)
        {
            aInputFileFormatType = PVMF_AACFF;


        }
        // AMR file (IETF and IF2)
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".amr") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".AMR") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), ".cod") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".COD") != NULL)
        {
            aInputFileFormatType = PVMF_AMRFF;
        }
        // RTSP URL
        else  if ((!oscl_strncmp("rtsp", aFileNameInfo.get_cstr(), 4)) ||
                  (!oscl_strncmp("RTSP", aFileNameInfo.get_cstr(), 4)))
        {
            aInputFileFormatType = PVMF_DATA_SOURCE_RTSP_URL;
        }
        // HTTP URL
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), "http:") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), "HTTP:") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), "mms:") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), "MMS:") != NULL)
        {
            aInputFileFormatType = PVMF_DATA_SOURCE_HTTP_URL;
        }
        // MP4/3GP file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".mp4") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".MP4") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), ".3gp") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".3GP") != NULL)
        {
            aInputFileFormatType = PVMF_MPEG4FF;
        }
        // ASF file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".asf") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".ASF") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), ".wma") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".WMA") != NULL ||
                  oscl_strstr(aFileNameInfo.get_cstr(), ".wmv") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".WMV") != NULL)
        {
            aInputFileFormatType = PVMF_ASFFF;
        }
        // MP3 file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".mp3") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".MP3") != NULL)
        {
            aInputFileFormatType = PVMF_MP3FF;
        }
        // RM file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".rm") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".RM") != NULL)
        {
            aInputFileFormatType = PVMF_RMFF;
        }
        // SDP file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".sdp") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".SDP") != NULL)
        {
            aInputFileFormatType = PVMF_DATA_SOURCE_SDP_FILE;
        }
        // PVX file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".pvx") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".PVX") != NULL)
        {
            aInputFileFormatType = PVMF_DATA_SOURCE_PVX_FILE;
        }
        // WAV file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".wav") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".WAV") != NULL)
        {
            aInputFileFormatType = PVMF_WAVFF;
        }
        // MIDI file
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".mid") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".MID") != NULL)
        {
            aInputFileFormatType = PVMF_MIDIFF;
        }
        // unrecognized eny (Jupiter envelope) files go to the still image node
        else  if (oscl_strstr(aFileNameInfo.get_cstr(), ".eny") != NULL || oscl_strstr(aFileNameInfo.get_cstr(), ".ENY") != NULL)
        {
            aInputFileFormatType = PVMF_IMAGE_FORMAT;
        }
        // Unknown so set to unknown try to have the player engine recognize
        else
        {
            fprintf(file, "Source type unknown so setting to unknown and have the player engine recognize it\n");
            aInputFileFormatType = PVMF_FORMAT_UNKNOWN;
        }
    }
}

//Find test range args:
//To run a range of tests by enum ID:
//	-test 17 29
//To run all Local play tests>
//	-test L
//To run all Download tests
//	-test D
//To run all Streaming tests:
//	-test S
void FindTestRange(cmd_line *command_line,	int32 &iFirstTest, int32 &iLastTest, FILE *aFile)
{
    //default is to run all tests.
    iFirstTest = 0;
    iLastTest = 9999;

    int iTestArgument = 0;
    char *iTestArgStr1 = NULL;
    char *iTestArgStr2 = NULL;
    bool iTestFound = false;
    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-test" argument
    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "Test cases to run option. Default is ALL:\n");
            fprintf(aFile, "  -test x y\n");
            fprintf(aFile, "   Specify a range of test cases to run. To run one test case, use the\n");
            fprintf(aFile, "   same index for x and y.\n");
            fprintf(aFile, "  -test L\n");
            fprintf(aFile, "   Run local playback test cases only.\n");
            fprintf(aFile, "  -test D\n");
            fprintf(aFile, "   Run download playback test cases only.\n");
            fprintf(aFile, "  -test S\n");
            fprintf(aFile, "   Run streaming playback test cases only.\n\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-test"))
        {
            iTestFound = true;
            iTestArgument = ++iTestSearch;
            break;
        }
        else
        {
            ConvertToLowerCase(iSourceFind);
            if (oscl_strcmp(iSourceFind, "aac") == NULL || oscl_strcmp(iSourceFind, "amr") == NULL || //When argument is supplied as test.exe mp3/amr,then local playback range has to be set
                    oscl_strcmp(iSourceFind, "mp4") == NULL || oscl_strcmp(iSourceFind, "3gp") == NULL ||
                    oscl_strcmp(iSourceFind, "mp3") == NULL || oscl_strcmp(iSourceFind, "wav") == NULL ||
                    oscl_strcmp(iSourceFind, "wmv") == NULL || oscl_strcmp(iSourceFind, "wma") == NULL ||
                    oscl_strcmp(iSourceFind, "asf") == NULL || oscl_strcmp(iSourceFind, "rm") == NULL)
            {
                iFirstTest = FIRST_LOCAL_PLAYBACK;
                iLastTest = LAST_LOCAL_PLAYBACK;
            }
        }

    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }

    if (iTestFound)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            iTestArgStr1 = new char[256];
            OSCL_TCHAR* cmd;
            command_line->get_arg(iTestArgument, cmd);
            if (cmd)
            {
                oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iTestArgStr1, 256);
            }

            iTestArgStr2 = new char[256];
            command_line->get_arg(iTestArgument + 1, cmd);
            if (cmd)
            {
                oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iTestArgStr2, 256);
            }
        }
        else
        {
            command_line->get_arg(iTestArgument, iTestArgStr1);
            command_line->get_arg(iTestArgument + 1, iTestArgStr2);
        }

        //Pull out 2 integers...
        if (iTestArgStr1 && '0' <= iTestArgStr1[0] && iTestArgStr1[0] <= '9' && iTestArgStr2 && '0' <= iTestArgStr2[0] && iTestArgStr2[0] <= '9')
        {
            int len = oscl_strlen(iTestArgStr1);
            switch (len)
            {

                case 4:
                    iFirstTest = 0;
                    if ('0' <= iTestArgStr1[0] && iTestArgStr1[0] <= '9')
                    {
                        iFirstTest = iFirstTest + 1000 * (iTestArgStr1[0] - '0');
                    }

                    if ('0' <= iTestArgStr1[1] && iTestArgStr1[1] <= '9')
                    {
                        iFirstTest = iFirstTest + 100 * (iTestArgStr1[1] - '0');
                    }

                    if ('0' <= iTestArgStr1[2] && iTestArgStr1[2] <= '9')
                    {
                        iFirstTest = iFirstTest + 10 * (iTestArgStr1[2] - '0');
                    }

                    if ('0' <= iTestArgStr1[3] && iTestArgStr1[3] <= '9')
                    {
                        iFirstTest = iFirstTest + 1 * (iTestArgStr1[3] - '0');
                    }
                    break;

                case 3:
                    iFirstTest = 0;
                    if ('0' <= iTestArgStr1[0] && iTestArgStr1[0] <= '9')
                    {
                        iFirstTest = iFirstTest + 100 * (iTestArgStr1[0] - '0');
                    }

                    if ('0' <= iTestArgStr1[1] && iTestArgStr1[1] <= '9')
                    {
                        iFirstTest = iFirstTest + 10 * (iTestArgStr1[1] - '0');
                    }

                    if ('0' <= iTestArgStr1[2] && iTestArgStr1[2] <= '9')
                    {
                        iFirstTest = iFirstTest + 1 * (iTestArgStr1[2] - '0');
                    }
                    break;

                case 2:
                    iFirstTest = 0;
                    if ('0' <= iTestArgStr1[0] && iTestArgStr1[0] <= '9')
                    {
                        iFirstTest = iFirstTest + 10 * (iTestArgStr1[0] - '0');
                    }

                    if ('0' <= iTestArgStr1[1] && iTestArgStr1[1] <= '9')
                    {
                        iFirstTest = iFirstTest + 1 * (iTestArgStr1[1] - '0');
                    }
                    break;

                case 1:
                    iFirstTest = 0;
                    if ('0' <= iTestArgStr1[0] && iTestArgStr1[0] <= '9')
                    {
                        iFirstTest = iFirstTest + 1 * (iTestArgStr1[0] - '0');
                    }
                    break;

                default:
                    break;
            }

            len = oscl_strlen(iTestArgStr2);
            switch (len)
            {

                case 4:
                    iLastTest = 0;
                    if ('0' <= iTestArgStr2[0] && iTestArgStr2[0] <= '9')
                    {
                        iLastTest = iLastTest + 1000 * (iTestArgStr2[0] - '0');
                    }

                    if ('0' <= iTestArgStr2[1] && iTestArgStr2[1] <= '9')
                    {
                        iLastTest = iLastTest + 100 * (iTestArgStr2[1] - '0');
                    }

                    if ('0' <= iTestArgStr2[2] && iTestArgStr2[2] <= '9')
                    {
                        iLastTest = iLastTest + 10 * (iTestArgStr2[2] - '0');
                    }

                    if ('0' <= iTestArgStr2[3] && iTestArgStr2[3] <= '9')
                    {
                        iLastTest = iLastTest + 1 * (iTestArgStr2[3] - '0');
                    }

                    break;

                case 3:
                    iLastTest = 0;
                    if ('0' <= iTestArgStr2[0] && iTestArgStr2[0] <= '9')
                    {
                        iLastTest = iLastTest + 100 * (iTestArgStr2[0] - '0');
                    }

                    if ('0' <= iTestArgStr2[1] && iTestArgStr2[1] <= '9')
                    {
                        iLastTest = iLastTest + 10 * (iTestArgStr2[1] - '0');
                    }

                    if ('0' <= iTestArgStr2[2] && iTestArgStr2[2] <= '9')
                    {
                        iLastTest = iLastTest + 1 * (iTestArgStr2[2] - '0');
                    }
                    break;

                case 2:
                    iLastTest = 0;
                    if ('0' <= iTestArgStr2[0] && iTestArgStr2[0] <= '9')
                    {
                        iLastTest = iLastTest + 10 * (iTestArgStr2[0] - '0');
                    }

                    if ('0' <= iTestArgStr2[1] && iTestArgStr2[1] <= '9')
                    {
                        iLastTest = iLastTest + 1 * (iTestArgStr2[1] - '0');
                    }
                    break;

                case 1:
                    iLastTest = 0;
                    if ('0' <= iTestArgStr2[0] && iTestArgStr2[0] <= '9')
                    {
                        iLastTest = iLastTest + 1 * (iTestArgStr2[0] - '0');
                    }
                    break;

                default:
                    break;
            }
        }
        //look for "D", "L" or "S"
        else if (iTestArgStr1
                 && iTestArgStr1[0] == 'L')
        {
            //local playback tests.
            iFirstTest = 0;
            iLastTest = 99;
        }
        else if (iTestArgStr1
                 && iTestArgStr1[0] == 'D')
        {
            //download tests
            iFirstTest = 100;
            iLastTest = 199;
        }
        else if (iTestArgStr1
                 && iTestArgStr1[0] == 'S')
        {
            //streaming tests
            iFirstTest = 200;
            iLastTest = 299;
        }
    }

    if (cmdline_iswchar)
    {
        if (iTestArgStr1)
        {
            delete[] iTestArgStr1;
            iTestArgStr1 = NULL;
        }

        if (iTestArgStr2)
        {
            delete[] iTestArgStr2;
            iTestArgStr2 = NULL;
        }

        if (iSourceFind)
        {
            delete[] iSourceFind;
            iSourceFind = NULL;
        }
    }
}

void FindCompressed(cmd_line* command_line, bool& aCompV, bool& aCompA, FILE* aFile)
{
    //default is uncompressed
    aCompV = false;
    aCompA = false;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-compressed" argument
    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "Compressed video and audio output option. Default is OFF for both:\n");
            fprintf(aFile, "  -compV AND/OR -compA\n");
            fprintf(aFile, "   For test cases and sinks that support compressed media output (media\n");
            fprintf(aFile, "   I/O node test cases), the output data files would have compressed\n");
            fprintf(aFile, "   bitstreams. This also means the player engine would not use a decoder\n");
            fprintf(aFile, "   node to decode the bitstream from the source node.\n\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-compV"))
        {
            aCompV = true;
        }
        else if (!oscl_strcmp(iSourceFind, "-compA"))
        {
            aCompA = true;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

void FindPacketSource(cmd_line* command_line, bool& aFileInput, bool& aBCS, FILE* aFile)
{
    aFileInput = false;
    aBCS = false;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "Packet Source plug-in option. Default is OFF for both:\n");
            fprintf(aFile, "  -fi\n");
            fprintf(aFile, "  For file input\n");
            fprintf(aFile, "  -bcs\n");
            fprintf(aFile, "  For broadcast socket\n\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-fi"))
        {
            aFileInput = true;
        }
        else if (!oscl_strcmp(iSourceFind, "-bcs"))
        {
            aBCS = true;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

void FindMemMgmtRelatedCmdLineParams(cmd_line* command_line, bool& aPrintDetailedMemLeakInfo, FILE* aFile)
{
    aPrintDetailedMemLeakInfo = false;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-logerr"/"-logwarn" argument
    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "Printing leak info option. Default is OFF:\n");
            fprintf(aFile, "  -leakinfo\n");
            fprintf(aFile, "   If there is a memory leak, prints out the memory leak information\n");
            fprintf(aFile, "   after all specified test cases have finished running.\n\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-leakinfo"))
        {
            aPrintDetailedMemLeakInfo = true;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

void FindLoggerNode(cmd_line* command_line, int32& lognode, FILE* aFile)
{
    //default is log player engine.
    lognode = 0;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-logerr"/"-logwarn" argument
    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "Log node options. Default is player engine only:\n");
            fprintf(aFile, "  -logall\n");
            fprintf(aFile, "   Log everything (log appender at root node)\n");
            fprintf(aFile, "  -logdatapath\n");
            fprintf(aFile, "   Log datapath only\n");
            fprintf(aFile, "  -logclock\n");
            fprintf(aFile, "   Log clock only\n");
            fprintf(aFile, "  -logoscl\n");
            fprintf(aFile, "   Log OSCL only\n");
            fprintf(aFile, "  -logperf\n");
            fprintf(aFile, "   Log scheduler performance\n");
            fprintf(aFile, "  -logperfmin\n");
            fprintf(aFile, "   Log scheduler performance (minimal)\n");
            fprintf(aFile, "  -logdatapathsrc\n");
            fprintf(aFile, "   Log source node datapath only\n\n");
            fprintf(aFile, "  -logdatapathdec\n");
            fprintf(aFile, "   Log decoder node datapath only\n\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-logall"))
        {
            lognode = 1;		//log everything
        }
        else if (!oscl_strcmp(iSourceFind, "-logdatapath"))
        {
            lognode = 2;		//datapath only
        }
        else if (!oscl_strcmp(iSourceFind, "-logclock"))
        {
            lognode = 3;		//clock only
        }
        else if (!oscl_strcmp(iSourceFind, "-logoscl"))
        {
            lognode = 4;		//oscl only
        }
        else if (!oscl_strcmp(iSourceFind, "-logperf"))
        {
            lognode = 5;		//scheduler perf logging
        }
        else if (!oscl_strcmp(iSourceFind, "-logdatapathsrc"))
        {
            lognode = 6;		//source node data path only
        }
        else if (!oscl_strcmp(iSourceFind, "-logdatapathdec"))
        {
            lognode = 7;		//source node data path only
        }
        else if (!oscl_strcmp(iSourceFind, "-logsync"))
        {
            lognode = 8;		//media output node datapath only
        }
        else if (!oscl_strcmp(iSourceFind, "-logdiagnostics"))
        {
            lognode = 9;		//diagnostics log only
        }
        else if (!oscl_strcmp(iSourceFind, "-logosclfileio"))
        {
            lognode = 10; 	//hds access log only
        }
        else if (!oscl_strcmp(iSourceFind, "-loghds"))
        {
            lognode = 11; 	//oscl file-io access log only
        }
        else if (!oscl_strcmp(iSourceFind, "-loghdsandosclfileio"))
        {
            lognode = 12; 	//file-io and hds access log only
        }
        else if (!oscl_strcmp(iSourceFind, "-logjupiter"))
        {
            lognode = 13; 	//jupiter DRM only
        }
        else if (!oscl_strcmp(iSourceFind, "-logjanus"))
        {
            lognode = 14; 	//janus DRM only
        }
        else if (oscl_strcmp(iSourceFind, "-logperfmin") == NULL)
        {
            lognode = 15;   //scheduler perf logging
        }
        else if (oscl_strcmp(iSourceFind, "-logppb") == NULL)
        {
            lognode = 16;	//progressive playback log only
        }
        else if (!oscl_strcmp(iSourceFind, "-logrepos"))
        {
            lognode = 17; 	//repos related
        }
        else if (oscl_strcmp(iSourceFind, "-logsnode") == NULL)
        {
            lognode = 18; 	//socket node related
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

void FindLogLevel(cmd_line* command_line, int32& loglevel, FILE* aFile)
{
    //default is verbose
    loglevel = PVLOGMSG_DEBUG;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-logerr"/"-logwarn" argument
    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "Log level options. Default is debug level:\n");
            fprintf(aFile, "  -logerr\n");
            fprintf(aFile, "   Log at error level\n");
            fprintf(aFile, "  -logwarn\n");
            fprintf(aFile, "   Log at warning level\n\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-logerr"))
        {
            loglevel = PVLOGMSG_ERR;
        }
        else if (!oscl_strcmp(iSourceFind, "-logwarn"))
        {
            loglevel = PVLOGMSG_WARNING;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

void FindLogText(cmd_line* command_line, int32& logtext, FILE* aFile)
{
    logtext = false;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-logfile"))
        {
            logtext = 1;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

void FindLogMem(cmd_line* command_line, int32& logmem, FILE* aFile)
{
    logmem = false;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-logmem"))
        {
            logmem = 1;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}

//Find if -proxy present in cmd line params
void FindProxyEnabled(cmd_line* command_line, bool& aProxyEnabled, FILE *aFile)
{
    //default is not enabled
    aProxyEnabled = false;

    bool cmdline_iswchar = command_line->is_wchar();

    int count = command_line->get_count();

    // Search for the "-proxy" argument
    char *iSourceFind = NULL;
    if (cmdline_iswchar)
    {
        iSourceFind = new char[256];
    }

    // Go through each argument
    for (int iTestSearch = 0; iTestSearch < count; iTestSearch++)
    {
        // Convert to UTF8 if necessary
        if (cmdline_iswchar)
        {
            OSCL_TCHAR* cmd = NULL;
            command_line->get_arg(iTestSearch, cmd);
            oscl_UnicodeToUTF8(cmd, oscl_strlen(cmd), iSourceFind, 256);
        }
        else
        {
            iSourceFind = NULL;
            command_line->get_arg(iTestSearch, iSourceFind);
        }

        // Do the string compare
        if (!oscl_strcmp(iSourceFind, "-help"))
        {
            fprintf(aFile, "   proxy enabled ON or OFF, default is OFF\n");
            fprintf(aFile, "  -proxy\n");
            fprintf(aFile, "   For test cases where proxy support is reqd\n");
        }
        else if (!oscl_strcmp(iSourceFind, "-proxy"))
        {
            aProxyEnabled = true;
        }
    }

    if (cmdline_iswchar)
    {
        delete[] iSourceFind;
        iSourceFind = NULL;
    }
}
int _local_main(FILE *filehandle, cmd_line* command_line);


// Entry point for the unit test program
int local_main(FILE *filehandle, cmd_line* command_line)
{
    //Init Oscl
    OsclBase::Init();
    OsclErrorTrap::Init();
    OsclMem::Init();
    //OMX_Init();
    PV_MasterOMX_Init();

    fprintf(filehandle, "Test Program for pvPlayer engine class.\n");

    bool oPrintDetailedMemLeakInfo = false;
    FindMemMgmtRelatedCmdLineParams(command_line, oPrintDetailedMemLeakInfo, filehandle);

    //Run the test under a trap
    int result = 0;
    int32 err = 0;
    TPVErrorPanic panic;

    OSCL_PANIC_TRAP(err, panic, result = _local_main(filehandle, command_line););

    //Show any exception.
    if (err != 0)
    {
        fprintf(file, "Error!  Leave %d\n", err);
    }
    if (panic.iReason != 0)
    {
        fprintf(file, "Error!  Panic %s %d\n", panic.iCategory.Str(), panic.iReason);
    }
    //Cleanup
    //OMX_Deinit();
    PV_MasterOMX_Deinit();

#if !(OSCL_BYPASS_MEMMGT)
    //Check for memory leaks before cleaning up OsclMem.
    OsclAuditCB auditCB;
    OsclMemInit(auditCB);
    if (auditCB.pAudit)
    {
        MM_Stats_t *stats = auditCB.pAudit->MM_GetStats("");
        if (stats)
        {
            fprintf(file, "\nMemory Stats:\n");
            fprintf(file, "  peakNumAllocs %d\n", stats->peakNumAllocs);
            fprintf(file, "  peakNumBytes %d\n", stats->peakNumBytes);
            fprintf(file, "  totalNumAllocs %d\n", stats->totalNumAllocs);
            fprintf(file, "  totalNumBytes %d\n", stats->totalNumBytes);
            fprintf(file, "  numAllocFails %d\n", stats->numAllocFails);
            if (stats->numAllocs)
            {
                fprintf(file, "  ERROR: Memory Leaks! numAllocs %d, numBytes %d\n", stats->numAllocs, stats->numBytes);
            }
        }
        uint32 leaks = auditCB.pAudit->MM_GetNumAllocNodes();
        if (leaks != 0)
        {
            if (oPrintDetailedMemLeakInfo)
            {
                fprintf(file, "ERROR: %d Memory leaks detected!\n", leaks);
                MM_AllocQueryInfo*info = auditCB.pAudit->MM_CreateAllocNodeInfo(leaks);
                uint32 leakinfo = auditCB.pAudit->MM_GetAllocNodeInfo(info, leaks, 0);
                if (leakinfo != leaks)
                {
                    fprintf(file, "ERROR: Leak info is incomplete.\n");
                }
                for (uint32 i = 0;i < leakinfo;i++)
                {
                    fprintf(file, "Leak Info:\n");
                    fprintf(file, "  allocNum %d\n", info[i].allocNum);
                    fprintf(file, "  fileName %s\n", info[i].fileName);
                    fprintf(file, "  lineNo %d\n", info[i].lineNo);
                    fprintf(file, "  size %d\n", info[i].size);
                    fprintf(file, "  pMemBlock 0x%x\n", info[i].pMemBlock);
                    fprintf(file, "  tag %s\n", info[i].tag);
                }
                auditCB.pAudit->MM_ReleaseAllocNodeInfo(info);
            }
        }
    }
#endif
    OsclMem::Cleanup();
    OsclErrorTrap::Cleanup();
    OsclBase::Cleanup();


    return result;
}

int _local_main(FILE *filehandle, cmd_line *command_line)
{
    file = filehandle;

    // Print out the extension for help if no argument
    if (command_line->get_count() == 0)
    {
        fprintf(file, "  Specify '-help' first to get help information on options\n\n");
    }

    OSCL_HeapString<OsclMemAllocator> filenameinfo;
    PVMFFormatType inputformattype ;
    int32 iFileFormatType = -1; // unknown file format type

    FindSourceType(command_line, filenameinfo, inputformattype, iFileFormatType, file); //Added with an additional argument
    FindSourceFile(command_line, filenameinfo, inputformattype, file);

    int32 firsttest, lasttest;
    FindTestRange(command_line, firsttest, lasttest, file);

    bool compV;
    bool compA;
    FindCompressed(command_line, compV, compA, file);

    bool fileinput;
    bool bcs;
    FindPacketSource(command_line, fileinput, bcs, file);

    int32 loglevel;
    FindLogLevel(command_line, loglevel, file);

    int32 lognode;
    FindLoggerNode(command_line, lognode, file);

    int32 logtext;
    FindLogText(command_line, logtext, file);

    int32 logmem;
    FindLogMem(command_line, logmem, file);

    bool proxyenabled;
    FindProxyEnabled(command_line, proxyenabled, file);
    fprintf(file, "  Input file name '%s'\n", filenameinfo.get_cstr());
    fprintf(file, "  Test case range %d to %d\n", firsttest, lasttest);
    fprintf(file, "  Compressed output ");
    if (compV)
    {
        fprintf(file, "Video(Yes) ");
    }
    else
    {
        fprintf(file, "Video(No) ");
    }
    if (compA)
    {
        fprintf(file, "Audio(Yes)\n");
    }
    else
    {
        fprintf(file, "Audio(No)\n");
    }
    fprintf(file, "  Log level %d; Log node %d Log Text %d Log Mem %d\n", loglevel, lognode, logtext, logmem);

    pvplayer_engine_test_suite *engine_tests = NULL;
    engine_tests = new pvplayer_engine_test_suite(filenameinfo.get_str(), inputformattype, firsttest, lasttest, compV, compA, fileinput, bcs, loglevel, lognode, logtext, logmem, iFileFormatType , proxyenabled);
    if (engine_tests)
    {

        //Set the Initial timer

        double starttick = OsclTickCount::TickCount();
        // Run the engine test
        engine_tests->run_test();
        double endtick = OsclTickCount::TickCount();

        double t1 = OsclTickCount::TicksToMsec(starttick);
        double t2 = OsclTickCount::TicksToMsec(endtick);
        fprintf(file, "Total Execution time for file %s is : %f seconds", filenameinfo.get_cstr(), (t2 - t1) / 1000);

        // Print out the results
        text_test_interpreter interp;
        _STRING rs = interp.interpretation(engine_tests->last_result());
        fprintf(file, rs.c_str());

        const test_result the_result = engine_tests->last_result();
        delete engine_tests;
        engine_tests = NULL;

        return (the_result.success_count() != the_result.total_test_count());
    }
    else
    {
        fprintf(file, "ERROR! pvplayer_engine_test_suite could not be instantiated.\n");
        return 1;
    }
}


pvplayer_engine_test_suite::pvplayer_engine_test_suite(char *aFileName, PVMFFormatType aFileType, int32 aFirstTest, int32 aLastTest, bool aCompV, bool aCompA, bool aFileInput, bool aBCS, int32 aLogLevel, int32 aLogNode, int32 aLogText, int32 aLogMem, int32 aFileFormatType, bool aProxyEnabled): test_case()
{
    adopt_test_case(new pvplayer_engine_test(aFileName, aFileType, aFirstTest, aLastTest, aCompV, aCompA, aFileInput, aBCS, aLogLevel, aLogNode, aLogText, aLogMem, aFileFormatType, aProxyEnabled));
}



pvplayer_engine_test::pvplayer_engine_test(char *aFileName, PVMFFormatType aFileType, int32 aFirstTest, int32 aLastTest,
        bool aCompV, bool aCompA, bool aFileInput, bool aBCS, int32 aLogLevel, int32 aLogNode, int32 aLogFile, int32 aLogMem, int32 aFileFormatType, bool aProxyEnabled)
{
    iFileName = aFileName;
    iFileType = aFileType;
    iCurrentTestNumber = 0;
    iCurrentTest = NULL;
    iFirstTest = aFirstTest;
    iLastTest = aLastTest;
    iCompressedVideoOutput = aCompV;
    iCompressedAudioOutput = aCompA;
    iFileInput = aFileInput;
    iBCS = aBCS;
    iLogLevel = aLogLevel;
    iLogNode = aLogNode;
    iLogFile = aLogFile;
    iLogMem = aLogMem;
    iTotalAlloc = 0;
    iTotalBytes = 0;
    iAllocFails = 0;
    iNumAllocs = 0;
    iProxyEnabled = aProxyEnabled;
    iFileFormatType = aFileFormatType;

#ifdef BUILD_N_ARM
    OMX_Init(CONFIG_FILE_NAME);
#endif
}


pvplayer_engine_test::~pvplayer_engine_test()
{
#ifdef BUILD_N_ARM
    OMX_Deinit();
#endif
}

//Function to determine invalid test case
bool pvplayer_engine_test::ValidateTestCase(int& aCurrentTestCaseNumber)
{
    int testCaseNumber = 0;
    if (iFileFormatType == MP3_ENABLED) //For MP3
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_MP3_INVALID_TESTCASES ;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < MP3_INVALID_TEST_ARRAY[testCaseNumber])
                return true;//aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == MP3_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > MP3_INVALID_TEST_ARRAY[NO_OF_MP3_INVALID_TESTCASES -1])
            return  true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == AMR_ENABLED) 	//For AMR
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_AMR_INVALID_TESTCASES;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < AMR_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == AMR_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > AMR_INVALID_TEST_ARRAY[NO_OF_AMR_INVALID_TESTCASES -1])
            return true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == MP4_ENABLED) // MP4 Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_3GP_OR_MP4_INVALID_TESTCASES;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < FILE_3GP_OR_MP4_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == FILE_3GP_OR_MP4_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > FILE_3GP_OR_MP4_INVALID_TEST_ARRAY[NO_OF_3GP_OR_MP4_INVALID_TESTCASES -1])
            return true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == THREE_GP_ENABLED)// 3GP Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_3GP_OR_MP4_INVALID_TESTCASES;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < FILE_3GP_OR_MP4_INVALID_TEST_ARRAY[testCaseNumber])
                return true;//aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == FILE_3GP_OR_MP4_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > FILE_3GP_OR_MP4_INVALID_TEST_ARRAY[NO_OF_3GP_OR_MP4_INVALID_TESTCASES -1])
            return true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == ASF_ENABLED) 	//ASF Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_ASF_INVALID_TESTCASES ;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < ASF_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == ASF_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                {
                    return false;
                }
            }
        }
        if (aCurrentTestCaseNumber > ASF_INVALID_TEST_ARRAY[NO_OF_ASF_INVALID_TESTCASES -1])
            return true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == WMV_ENABLED)	//WMV Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_WMV_INVALID_TESTCASES;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < WMV_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == WMV_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > WMV_INVALID_TEST_ARRAY[NO_OF_WMV_INVALID_TESTCASES-1])
            return true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == WMA_ENABLED)	//WMA Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_WMA_INVALID_TESTCASES ;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < WMA_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == WMA_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > WMA_INVALID_TEST_ARRAY[NO_OF_WMA_INVALID_TESTCASES -1])
            return true; //aCurrentTestCaseNumber ; //Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == WAV_ENABLED) 	//WAV Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_WAV_INVALID_TESTCASES ;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < WAV_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == WAV_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > WAV_INVALID_TEST_ARRAY[NO_OF_WAV_INVALID_TESTCASES -1])
            return true;//Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == RM_ENABLED) //RM Enabled
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_REAL_INVALID_TESTCASES ;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < REAL_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == REAL_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > REAL_INVALID_TEST_ARRAY[NO_OF_REAL_INVALID_TESTCASES -1])
            return true;//Sending when CurrentCount is more than all the element of array
    }
    else if (iFileFormatType == AAC_ENABLED) 	//For AAC
    {
        for (testCaseNumber = 0;testCaseNumber < NO_OF_AAC_INVALID_TESTCASES;testCaseNumber++)
        {
            if (aCurrentTestCaseNumber < AAC_INVALID_TEST_ARRAY[testCaseNumber])
                return true; //aCurrentTestCaseNumber;
            else if (aCurrentTestCaseNumber == AAC_INVALID_TEST_ARRAY[testCaseNumber])
            {
                if (iLastTest > aCurrentTestCaseNumber)
                    aCurrentTestCaseNumber++;
                else
                    return false;
            }
        }
        if (aCurrentTestCaseNumber > AAC_INVALID_TEST_ARRAY[NO_OF_AAC_INVALID_TESTCASES-1])
            return true;//Sending when CurrentCount is more than all the element of array
    }
    else
    {
        return true;//aCurrentTestCaseNumber;
    }

    return false;
}

void pvplayer_engine_test::TestCompleted(test_case &tc)
{
    // Print out the result for this test case
    const test_result the_result = tc.last_result();
    fprintf(file, "Results for Test Case %d:\n", iCurrentTestNumber);
    fprintf(file, "  Successes %d, Failures %d\n"
            , the_result.success_count() - iTotalSuccess, the_result.failures().size() - iTotalFail);
    fflush(file);
    iTotalSuccess = the_result.success_count();
    iTotalFail = the_result.failures().size();
    iTotalError = the_result.errors().size();

    // Go to next test
    ++iCurrentTestNumber;
    /*Skipping the test cases based on File type */

    if (!ValidateTestCase(iCurrentTestNumber))
    {
        if (iLastTest == iCurrentTestNumber) //Dealing with when -test a b ,b is invalid test case
        {
            iCurrentTestNumber = BeyondLastTest;
        }
    }
    // Stop the scheduler
    OsclExecScheduler *sched = OsclExecScheduler::Current();
    if (sched)
    {
        sched->StopScheduler();
    }
}


void pvplayer_engine_test::test()
{
    bool AtleastOneExecuted = false;
    // Specify the starting test case
    iCurrentTestNumber = iFirstTest;
    iTotalSuccess = iTotalFail = iTotalError = 0;
    if (!ValidateTestCase(iCurrentTestNumber))
    {                                           //Dealing with when -test a b ,a is invalid test case and a==b
        if (iLastTest <= iCurrentTestNumber)
        {
            iCurrentTestNumber = BeyondLastTest;
        }
    }
    else //Atleast one test case is executed
    {
        AtleastOneExecuted = true;
    }

    while (iCurrentTestNumber <= iLastTest || iCurrentTestNumber < BeyondLastTest || AtleastOneExecuted)
    {
        if (iCurrentTest)
        {
            delete iCurrentTest;
            iCurrentTest = NULL;

            // Shutdown PVLogger and scheduler before checking mem stats
            OsclScheduler::Cleanup();
            PVLogger::Cleanup();
#if !(OSCL_BYPASS_MEMMGT)
            // Print out the memory usage results for this test case
            OsclAuditCB auditCB;
            OsclMemInit(auditCB);
            if (auditCB.pAudit)
            {
                MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
                if (stats)
                {
                    uint32 alloclks = stats->numAllocs - iNumAllocs;
                    fprintf(file, "  Mem stats: TotalAllocs(%d), TotalBytes(%d),\n             AllocFailures(%d), AllocLeak(%d)\n",
                            stats->totalNumAllocs - iTotalAlloc, stats->totalNumBytes - iTotalBytes, stats->numAllocFails - iAllocFails, stats->numAllocs - iNumAllocs);
#if 0 //debug
                    if (alloclks != 0)
                    {
                        alloclks++;
                        alloclks--;
                    }
#endif
                }
                else
                {
                    fprintf(file, "Retrieving memory statistics after running test case failed! Memory statistics result is not available.\n");
                }
            }
            else
            {
                fprintf(file, "Memory audit not available! Memory statistics result is not available.\n");
            }
#endif
        }

#if !(OSCL_BYPASS_MEMMGT)
        // Obtain the current mem stats before running the test case
        OsclAuditCB auditCB;
        OsclMemInit(auditCB);
        if (auditCB.pAudit)
        {
            MM_Stats_t* stats = auditCB.pAudit->MM_GetStats("");
            if (stats)
            {
                iTotalAlloc = stats->totalNumAllocs;
                iTotalBytes = stats->totalNumBytes;
                iAllocFails = stats->numAllocFails;
                iNumAllocs = stats->numAllocs;
            }
            else
            {
                fprintf(file, "Retrieving memory statistics before running test case failed! Memory statistics result would be invalid.\n");
            }
        }
        else
        {
            fprintf(file, "Memory audit not available! Memory statistics result would be invalid.\n");
        }
#endif

        bool setupLoggerScheduler = false;

        // skip IF2 tests which are no longer supported
        if (iCurrentTestNumber == AMRIF2FileOpenPlayStopTest)
        {
            iCurrentTestNumber++;
        }
        if (iCurrentTestNumber == AMRIF2FilePlayStopPlayStopTest)
        {
            iCurrentTestNumber++;
        }
        //skip the placeholders and empty ranges.
        if (iCurrentTestNumber == LastLocalTest)
        {
            iCurrentTestNumber = FirstDownloadTest;
        }
        if (iCurrentTestNumber == FirstDownloadTest)
        {
            iCurrentTestNumber++;
        }
        if (iCurrentTestNumber == LastDownloadTest)
        {
            iCurrentTestNumber = FirstStreamingTest;
        }
        if (iCurrentTestNumber == FirstStreamingTest)
        {
            iCurrentTestNumber++;
        }
        if (iCurrentTestNumber == LastStreamingTest)
        {
            iCurrentTestNumber = BeyondLastTest;
        }

        //stop at last test of selected range.
        if (iCurrentTestNumber > iLastTest)
        {
            iCurrentTestNumber = BeyondLastTest;
        }
        else
        {
            fprintf(file, "\nStarting Test %d: ", iCurrentTestNumber);
            fflush(file);
            SetupLoggerScheduler();
            setupLoggerScheduler = true;
        }

        // Setup the standard test case parameters based on current unit test settings
        PVPlayerAsyncTestParam testparam;
        testparam.iObserver = this;
        testparam.iTestCase = this;
        testparam.iTestMsgOutputFile = file;
        testparam.iFileName = iFileName;
        testparam.iFileType = iFileType;
        testparam.iCompressedVideo = iCompressedVideoOutput;
        testparam.iCompressedAudio = iCompressedAudioOutput;
        testparam.iFileInput = iFileInput;
        testparam.iBCS = iBCS;
        testparam.iCurrentTestNumber = iCurrentTestNumber;
        testparam.iProxyEnabled = iProxyEnabled;

        switch (iCurrentTestNumber)
        {
            case NewDeleteTest:
                iCurrentTest = new pvplayer_async_test_newdelete(testparam);
                break;

            case OpenPlayStopResetTest:
                iCurrentTest = new pvplayer_async_test_openplaystopreset(testparam);
                break;

            case OpenPlayStopResetCPMTest:
                iCurrentTest = new pvplayer_async_test_cpmopenplaystopreset(testparam);
                break;

            case CPM_DLA_OMA1PASSTRHU_OpenFailAuthPlayStopResetTest:
#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
                iCurrentTest = new pvplayer_async_test_cpmdlapassthru(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        OMA1_DLA_FAIL,
                        false);
                ((pvplayer_async_test_cpmdlapassthru*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "CPM OMA1 DLA PassThru tests not enabled\n");
#endif
                break;

            case CPM_DLA_OMA1PASSTRHU_OpenPlayStopResetTest:
#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
                iCurrentTest = new pvplayer_async_test_cpmdlapassthru(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        OMA1_DLA_NORMAL,
                        false);
                ((pvplayer_async_test_cpmdlapassthru*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "CPM OMA1 DLA PassThru tests not enabled\n");
#endif
                break;

            case CPM_DLA_OMA1PASSTRHU_UnknownContentOpenPlayStopResetTest:
#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
                iCurrentTest = new pvplayer_async_test_cpmdlapassthru(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        OMA1_DLA_UNKNOWN_CPM_CONTENTTYPE,
                        false);
                ((pvplayer_async_test_cpmdlapassthru*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "CPM OMA1 DLA PassThru tests not enabled\n");
#endif
                break;

            case CPM_DLA_OMA1PASSTRHU_CancelAcquireLicenseTooLate_CancelFails:
#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
                iCurrentTest = new pvplayer_async_test_cpmdlapassthru(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        OMA1_DLA_CANCEL_ACQUIRE_LICENSE_FAILS,
                        true);
                ((pvplayer_async_test_cpmdlapassthru*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "CPM OMA1 DLA PassThru tests not enabled\n");
#endif
                break;

            case CPM_DLA_OMA1PASSTRHU_CancelAcquireLicense_CancelSucceeds:
#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
                iCurrentTest = new pvplayer_async_test_cpmdlapassthru(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        OMA1_DLA_CANCEL_ACQUIRE_LICENSE_SUCCEEDS,
                        true);
                ((pvplayer_async_test_cpmdlapassthru*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "CPM OMA1 DLA PassThru tests not enabled\n");
#endif
                break;

            case CPM_DLA_OMA1PASSTRHU_ContentNotSupported:
#if RUN_CPMOMA1_DLAPASSTHRU_TESTCASES
                iCurrentTest = new pvplayer_async_test_cpmdlapassthru(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        OMA1_DLA_CONTENT_NOTSUPPORTED,
                        false);
                ((pvplayer_async_test_cpmdlapassthru*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "CPM OMA1 DLA PassThru tests not enabled\n");
#endif
                break;


            case MetaDataTest:
                iCurrentTest = new pvplayer_async_test_metadata(testparam);
                break;

            case TimingTest:
                iCurrentTest = new pvplayer_async_test_timing(testparam);
                break;

            case InvalidStateTest:
                iCurrentTest = new pvplayer_async_test_invalidstate(testparam);
                break;

            case PreparedStopTest:
                iCurrentTest = new pvplayer_async_test_preparedstop(testparam);
                break;

            case VideoOnlyPlay7Seconds:
                iCurrentTest = new pvplayer_async_test_videoonlyplay7seconds(testparam);
                break;

            case Play5StopPlay10StopReset:
                iCurrentTest = new pvplayer_async_test_play5stopplay10stopreset(testparam);
                break;

            case PauseResume:
                iCurrentTest = new pvplayer_async_test_pauseresume(testparam);
                break;

            case PlayPauseStop:
                iCurrentTest = new pvplayer_async_test_playpausestop(testparam);
                break;

            case OutsideNodeForVideoSink:
                iCurrentTest = new pvplayer_async_test_outsidenodeforvideosink(testparam);
                break;

            case GetPlayerState:
                iCurrentTest = new pvplayer_async_test_getplayerstate(testparam);
                break;

            case GetCurrentPosition:
                iCurrentTest = new pvplayer_async_test_getcurrentposition(testparam);
                break;

            case PlaySetStopPosition:
                iCurrentTest = new pvplayer_async_test_playsetstopposition(testparam);
                break;

            case PlaySetStopPositionVidFrameNum:
                iCurrentTest = new pvplayer_async_test_playsetstoppositionvidframenum(testparam);
                break;

            case SetStartPositionPlayStop:
                iCurrentTest = new pvplayer_async_test_setstartpositionplaystop(testparam);
                break;

            case SetPlayRangePlay:
                iCurrentTest = new pvplayer_async_test_setplayrangeplay(testparam);
                break;

            case SetPlayRangeVidFrameNumPlay:
                iCurrentTest = new pvplayer_async_test_setplayrangevidframenumplay(testparam);
                break;

            case PlaySetPlayRangeStop:
                iCurrentTest = new pvplayer_async_test_playsetplayrangestop(testparam);
                break;

            case PlaySetPlayRangeVidFrameNumStop:
                iCurrentTest = new pvplayer_async_test_playsetplayrangevidframenumstop(testparam);
                break;

            case TrackLevelInfoTest:
                iCurrentTest = new pvplayer_async_test_tracklevelinfo(testparam);
                break;

            case SetPlaybackRate2X:
                iCurrentTest = new pvplayer_async_test_setplaybackrate2X(testparam);
                break;

            case SetPlaybackRateFifth:
                iCurrentTest = new pvplayer_async_test_setplaybackratefifth(testparam);
                break;

            case CapConfigInterfaceTest:
                iCurrentTest = new pvplayer_async_test_capconfigiftest(testparam);
                break;

            case QueuedCommandsTest:
                iCurrentTest = new pvplayer_async_test_queuedcommands(testparam);
                break;

            case LoopingTest:
                iCurrentTest = new pvplayer_async_test_looping(testparam);
                break;

            case WaitForEOSTest:
                iCurrentTest = new pvplayer_async_test_waitforeos(testparam);
                break;

            case MultiplePauseResumeTest:
                iCurrentTest = new pvplayer_async_test_multipauseresume(testparam);
                break;

            case MultipleRepositionTest:
                iCurrentTest = new pvplayer_async_test_multireposition(testparam);
                break;

            case MediaIONodeOpenPlayStopTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                break;

            case MediaIONodePlayStopPlayTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                break;

            case MediaIONodePauseResumeTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_pauseresume(testparam);
                break;

            case MediaIONodePlaySetPlaybackRangeTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_playsetplaybackrange(testparam);
                break;

            case MediaIONodeSetPlaybackRate3XTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_3Xplayrate(testparam);
                break;

            case MediaIONodeSetPlaybackRateHalfTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_halfplayrate(testparam);
                break;

            case MediaIONodeLoopingTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_looping(testparam);
                break;

            case MediaIONodeWaitForEOSTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_waitforeos(testparam);
                break;

            case MediaIOMultiplePauseResumeTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_multipauseresume(testparam);
                break;

            case MediaIOMultipleRepositionTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_multireposition(testparam);
                break;

            case MediaIORepositionConfigTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_repositionconfig(testparam);
                break;

            case MediaIONodeEOSLoopingTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_eoslooping(testparam);
                break;

            case MediaIONodeRepositionDuringPreparedTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_repositionduringprepared(testparam);
                break;

            case MediaIONodePlaySetPlaybackRangeStopPlayTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_playsetplaybackrangestopplay(testparam);
                break;

            case MediaIONodePlayStopSetPlaybackRangePlayStopTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopsetplaybackrangeplaystop(testparam);
                break;

            case MediaIONodeSetPlaybackRangeNearEndStartTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_setplaybackrangenearendplay(testparam);
                break;

            case MediaIONodePlayRepositionNearEndOfClipTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_playrepositionnearendofclip(testparam);
                break;

            case MediaIONodeForwardStepTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_forwardstep(testparam, false);
                break;

            case MediaIONodeForwardStepActiveAudioTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_forwardstep(testparam, true);
                break;

            case MediaIONodeForwardStepToEOSTest:
                iCurrentTest = new pvplayer_async_test_mediaionode_forwardsteptoeos(testparam);
                break;

            case MediaIONodeBackwardTest:
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_mediaionode_backwardplayback(testparam);
#else
                fprintf(file, "Backward playback tests not enabled\n");
#endif
                break;

            case MediaIONodeBackwardForwardTest:
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_mediaionode_backwardforwardplayback(testparam);
#else
                fprintf(file, "Backward/Forward playback tests not enabled\n");
#endif
                break;

            case MediaIONodePauseNearEOSBackwardResumeTest:
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_mediaionode_pauseneareosbackwardresume(testparam);
#else
                fprintf(file, "Backward/Forward playback tests not enabled\n");
#endif
                break;

            case MediaIONodeMultiplePauseSetPlaybackRateResumeTest:
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_mediaionode_multiplepausesetplaybackrateresume(testparam);
#else
                fprintf(file, "Backward/Forward playback tests not enabled\n");
#endif
                break;

            case MediaIONodBackwardNearEOSForwardNearStartTest:
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_mediaionode_backwardneareosforwardnearbos(testparam);
#else
                fprintf(file, "Backward/Forward playback tests not enabled\n");
#endif
                break;

            case MP4M4VAMRFileOpenPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_M4V_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP4 M4v/AMR ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4M4VAMRFilePlayStopPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_M4V_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP4 M4v/AMR ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4H263AMRFileOpenPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_H263_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP4 H.263/AMR ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4H263AMRFilePlayStopPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_H263_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP4 H.263/AMR ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4AVCAMRFileOpenPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_AVC_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP4 AVC/AMR ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4AVCAMRFilePlayStopPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_AVC_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP4 AVC/AMR ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4AMRFileOpenPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP4 AMR-only ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4AMRFilePlayStopPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_AMR;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP4 AMR-only ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4AACFileOpenPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_AAC;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP4 AAC-only ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4AACFilePlayStopPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_AAC;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP4 AAC-only ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4M4VAMRTextFileOpenPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_M4V_AMR_TEXT;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP4 M4v/AMR/Text ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case MP4M4VAMRTextFilePlayStopPlayStopTest:
#if RUN_MP4FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP4_M4V_AMR_TEXT;
                testparam.iFileType = LOCAL_TEST_FILE_MP4_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP4 M4v/AMR/Text ");
#else
                fprintf(file, "MP4 file tests not enabled\n");
#endif
                break;

            case AMRIETFFileOpenPlayStopTest:
#if RUN_AMRFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AMR_IETF;
                testparam.iFileType = LOCAL_TEST_FILE_AMR_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "AMR IETF ");
#else
                fprintf(file, "AMR file tests not enabled\n");
#endif
                break;

            case AMRIETFFilePlayStopPlayStopTest:
#if RUN_AMRFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AMR_IETF;
                testparam.iFileType = LOCAL_TEST_FILE_AMR_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "AMR IETF ");
#else
                fprintf(file, "AMR file tests not enabled\n");
#endif
                break;

            case AMRIF2FileOpenPlayStopTest:
#if RUN_AMRFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AMR_IF2;
                testparam.iFileType = LOCAL_TEST_FILE_AMR_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "AMR IF2 ");
#else
                fprintf(file, "AMR file tests not enabled\n");
#endif
                break;

            case AMRIF2FilePlayStopPlayStopTest:
#if RUN_AMRFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AMR_IF2;
                testparam.iFileType = LOCAL_TEST_FILE_AMR_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "AMR IF2 ");
#else
                fprintf(file, "AMR file tests not enabled\n");
#endif
                break;

            case AACADTSFileOpenPlayStopTest:
#if RUN_AACFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AAC_ADTS;
                testparam.iFileType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "AAC ADTS ");
#else
                fprintf(file, "AAC file tests not enabled\n");
#endif
                break;

            case AACADTSFilePlayStopPlayStopTest:
#if RUN_AACFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AAC_ADTS;
                testparam.iFileType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "AAC ADTS ");
#else
                fprintf(file, "AAC file tests not enabled\n");
#endif
                break;

            case AACADIFFileOpenPlayStopTest:
#if RUN_AACFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AAC_ADIF;
                testparam.iFileType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "AAC ADIF ");
#else
                fprintf(file, "AAC file tests not enabled\n");
#endif
                break;

            case AACADIFFilePlayStopPlayStopTest:
#if RUN_AACFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AAC_ADIF;
                testparam.iFileType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "AAC ADIF ");
#else
                fprintf(file, "AAC file tests not enabled\n");
#endif
                break;

            case AACRawFileOpenPlayStopTest:
#if RUN_AACFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AAC_RAW;
                testparam.iFileType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "AAC Raw ");
#else
                fprintf(file, "AAC file tests not enabled\n");
#endif
                break;

            case AACRawFilePlayStopPlayStopTest:
#if RUN_AACFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_AAC_RAW;
                testparam.iFileType = LOCAL_TEST_FILE_AAC_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "AAC Raw ");
#else
                fprintf(file, "AAC file tests not enabled\n");
#endif
                break;

            case MP3CBRFileOpenPlayStopTest:
#if RUN_MP3FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP3_CBR;
                testparam.iFileType = LOCAL_TEST_FILE_MP3_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP3 CBR ");
#else
                fprintf(file, "MP3 file tests not enabled\n");
#endif
                break;

            case MP3CBRFilePlayStopPlayStopTest:
#if RUN_MP3FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP3_CBR;
                testparam.iFileType = LOCAL_TEST_FILE_MP3_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP3 CBR ");
#else
                fprintf(file, "MP3 file tests not enabled\n");
#endif
                break;

            case MP3VBRFileOpenPlayStopTest:
#if RUN_MP3FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP3_VBR;
                testparam.iFileType = LOCAL_TEST_FILE_MP3_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "MP3 VBR ");
#else
                fprintf(file, "MP3 file tests not enabled\n");
#endif
                break;

            case MP3VBRFilePlayStopPlayStopTest:
#if RUN_MP3FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_MP3_VBR;
                testparam.iFileType = LOCAL_TEST_FILE_MP3_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "MP3 VBR ");
#else
                fprintf(file, "MP3 file tests not enabled\n");
#endif
                break;

            case WAVFileOpenPlayStopTest:
#if RUN_WAVFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_WAV;
                testparam.iFileType = LOCAL_TEST_FILE_WAV_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "WAV ");
#else
                fprintf(file, "WAV file tests not enabled\n");
#endif
                break;

            case WAVFilePlayStopPlayStopTest:
#if RUN_WAVFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_WAV;
                testparam.iFileType = LOCAL_TEST_FILE_WAV_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "WAV ");
#else
                fprintf(file, "WAV file tests not enabled\n");
#endif
                break;

            case ASFFileOpenPlayStopTest:
#if RUN_ASFFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_ASF;
                testparam.iFileType = LOCAL_TEST_FILE_ASF_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "ASF ");
#else
                fprintf(file, "ASF file tests not enabled\n");
#endif
                break;

            case ASFFilePlayStopPlayStopTest:
#if RUN_ASFFILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_ASF;
                testparam.iFileType = LOCAL_TEST_FILE_ASF_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_playstopplay(testparam);
                fprintf(file, "ASF ");
#else
                fprintf(file, "ASF file tests not enabled\n");
#endif
                break;

            case RealAudioFileOpenPlayStopTest:
#if RUN_REALAUDIO_FILE_TESTCASES
                testparam.iFileName = LOCAL_TEST_FILE_REALAUDIO;
                testparam.iFileType = LOCAL_TEST_FILE_REALAUDIO_FORMAT_TYPE;
                iCurrentTest = new pvplayer_async_test_mediaionode_openplaystop(testparam);
                fprintf(file, "Real Audio ");
#else
                fprintf(file, "Real Audio file tests not enabled\n");
#endif
                break;

            case SetPlaybackAfterPrepare:

                iCurrentTest = new pvplayer_async_test_setplaybackafterprepare(testparam);
                break;

            case FTDownloadOpenPlayStopTest:
#if RUN_HTTPDOWNLOAD_TESTCASES
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_ftdlnormal(testparam);
#else
                fprintf(file, "PV Fast track download tests not enabled\n");
#endif
#else
                fprintf(file, "Download tests not enabled\n");
#endif
                break;

            case FTDownloadPlayStopPlayTest:
#if RUN_HTTPDOWNLOAD_TESTCASES
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_ftdlnormal(testparam);
                ((pvplayer_async_test_ftdlnormal*)iCurrentTest)->enablePlayStopPlay();

#else
                fprintf(file, "PV Fast track download tests not enabled\n");
#endif
#else
                fprintf(file, "Download tests not enabled\n");
#endif
                break;

            case ProgDownloadPlayAsapTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal(testparam);
#else
                fprintf(file, "Prog download tests not enabled\n");
#endif
                break;

            case ProgDownloadPlayStopPlayTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal(testparam);
                ((pvplayer_async_test_3gppdlnormal*)iCurrentTest)->enablePlayStopPlay();
#else
                fprintf(file, "Prog download tests not enabled\n");
#endif
                break;

            case ProgPlaybackPlayStopPlayTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppbnormal(testparam);
                ((pvplayer_async_test_ppbnormal*)iCurrentTest)->enablePlayStopPlay();
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadDownloadThenPlayTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal_dlthenplay(testparam);
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadDownloadThenPlayPauseTest:
#if RUN_HTTPDOWNLOAD_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal_dlthenplay(testparam);
                ((pvplayer_async_test_3gppdlnormal_dlthenplay*)iCurrentTest)->enablePauseAfterDownloadComplete();
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadDownloadThenPlayRepositionTest:
#if RUN_HTTPDOWNLOAD_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal_dlthenplay(testparam);
                ((pvplayer_async_test_3gppdlnormal_dlthenplay*)iCurrentTest)->enableReposAfterDownloadComplete();
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadDownloadOnlyTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal_dlonly(testparam);
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadCancelDuringInitTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlcancelduringinit(testparam);
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadCancelDuringInitDelayTest: //114
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlcancelduringinitdelay(testparam);
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;
            case ProgDownloadPauseResumeAfterUnderflowTest: //115
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_PDLPauseResumeAfterUnderFlow(testparam);
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadContentTooLarge:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlcontenttoolarge(testparam);
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadTruncated:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
#ifndef ONLY_3GPP_STREAMING_FORMAT
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlContentTruncated(testparam);
#else
                fprintf(file, "Prog Truncated download tests not enabled\n");
#endif
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadProtocolRolloverTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES && RUN_MS_HTTP_STREAMING_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal(testparam);
                ((pvplayer_async_test_3gppdlnormal*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadSetPlayBackRangeTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal(testparam);
                ((pvplayer_async_test_3gppdlnormal*)iCurrentTest)->enableReposAfterDownloadComplete();
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgDownloadPlayUtilEOSTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_3gppdlnormal(testparam);
                ((pvplayer_async_test_3gppdlnormal*)iCurrentTest)->enablePlayUntilEOS();
#else
                fprintf(file, "Prog Download tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4UntilEOSTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enablePlayUntilEOS();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Until EOS");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4ShortTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4ShortPauseResumeTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableShortPauseResume();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Short Pause Resume");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4LongPauseResumeTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableLongPauseResume();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enablePlayUntilEOS();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Long Pause Resume Until EOS");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4StartPauseSeekResumeTwiceTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableShortPauseResume();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableSeekAfterStart();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableTwice();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Start Pause Seek Resume Twice");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4SeekStartTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableSeekBeforeStart();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enablePlayUntilEOS();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Seek Start Until EOS");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4StartPauseSeekResumeLoopTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableShortPauseResume();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableSeekAfterStart();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableLoop();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Start Pause Seek Resume Loop");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;

            case ProgPlaybackMP4SeekForwardStepLoopTest:
#if RUN_HTTPDOWNLOAD_TESTCASES && RUN_MP4FILE_TESTCASES
                testparam.iFileType = PVMF_MPEG4FF;
                iCurrentTest = new pvplayer_async_test_ppb_normal(testparam);
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableShortPauseResume();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableSeekAfterStart();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableLoop();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->enableForwardStep();
                ((pvplayer_async_test_ppb_normal*)iCurrentTest)->iTestCaseName = _STRLIT_CHAR("MP4 Progressive Playback Seek Forward Step Loop");
#else
                fprintf(file, "MP4 progressive playback tests not enabled\n");
#endif
                break;


            case FTDownloadOpenPlayUntilEOSTest:
#if RUN_HTTPDOWNLOAD_TESTCASES
#ifndef ONLY_3GPP_STREAMING_FORMAT
                iCurrentTest = new pvplayer_async_test_ftdlnormal(testparam);
                ((pvplayer_async_test_ftdlnormal*)iCurrentTest)->enablePlayUntilEOS();
#else
                fprintf(file, "PV Fast track download tests not enabled\n");
#endif
#else
                fprintf(file, "Download tests not enabled\n");
#endif
                break;

            case StreamingOpenPlayStopTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlayStopTest");
                iCurrentTest =
                    new pvplayer_async_test_streamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false,
                            false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingOpenPlayPausePlayStopTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlayPausePlayStopTest");
                iCurrentTest =
                    new pvplayer_async_test_streamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            false,
                            false,
                            false,
                            false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingOpenPlaySeekStopTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlaySeekStopTest");
                iCurrentTest =
                    new pvplayer_async_test_streamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            false,
                            false,
                            false,
                            false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingCancelDuringPrepareTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingCancelDuringPrepareTest");
                iCurrentTest =
                    new pvplayer_async_test_streamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            true,
                            false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case PrintMetadataTest:
                iCurrentTest = new pvplayer_async_test_printmetadata(testparam);
                break;

            case PrintMemStatsTest:
                iCurrentTest = new pvplayer_async_test_printmemstats(testparam);
                break;

            case PlayUntilEOSTest:
                iCurrentTest = new pvplayer_async_test_playuntileos(testparam);
                break;

            case CleanDrmData_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case LoadLicense_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case OpenPlayStop_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case PlayStopPlayStop_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case QueryEngine_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case DLA_CleanDrmData_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case DLA_OpenPlayStop_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case DLA_LicenseCapture_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case DLA_CancelAcquireLicense_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case StartupMeasurement_JanusCPMTest:
                fprintf(file, "Janus CPM tests not enabled\n");
                break;

            case DLA_QueryEngine_JupiterCPMTest_v2_WMA:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_query(testparam
                        , EJupContentType_v2_WMA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_QueryEngine_JupiterCPMTest_v24_WMA:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_query(testparam
                        , EJupContentType_v24_WMA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_QueryEngine_JupiterCPMTest_v4_Enveloped_ASF:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_query(testparam
                        , EJupContentType_v4_Enveloped_ASF);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_QueryEngine_JupiterCPMTest_v4_Enveloped_MP4:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_query(testparam
                        , EJupContentType_v4_Enveloped_MP4);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_QueryEngine_JupiterCPMTest_v4_Enveloped_Image:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_query(testparam
                        , EJupContentType_v4_Enveloped_Image);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_EnvelopeRead_JupiterCPMTest_v4_Enveloped_Image:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_envelopeimageread(testparam
                        , EJupContentType_v4_Enveloped_Image);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v2_WMA:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v2_WMA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v2_WMV:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v2_WMV);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_redirect:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_envelopeimageread(testparam
                        , EJupContentType_v4_Enveloped_Image
                        , EJupLicenseType_DLA_With_Redirect);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v4_WMA:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v4_WMA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v4_WMV:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v4_WMV);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v4_AAC:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v4_AAC);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v4_H264:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v4_H264);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v4_Enveloped_ASF:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v4_Enveloped_ASF);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v4_Enveloped_MP4:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v4_Enveloped_MP4);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_unprotected_H264:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_unprotected_H264_in_ASF);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_unprotected_AAC:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_unprotected_AAC_in_ASF);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA_fallback:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA
                        , EJupLicenseType_DLA_With_Fallback);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMV:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMV);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA_ringtone:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA
                        , EJupLicenseType_DLA_With_Ringtone);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA_domain:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA
                        , EJupLicenseType_DLA_With_Domain);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA_domain_renew:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA
                        , EJupLicenseType_DLA_With_DomainRenew);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA_domain_offline:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA
                        , EJupLicenseType_DLA_With_DomainOffline);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_OpenPlayStop_JupiterCPMTest_v24_WMA_domain_history:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_openplaystop(testparam
                        , EJupContentType_v24_WMA
                        , EJupLicenseType_DLA_With_DomainHistory);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_JoinDomain_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_joindomain(testparam);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_LeaveDomain_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_leavedomain(testparam);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_MeteringByCert_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_metering_or_sync(testparam
                        , EPVMeterTestType_byCert
                        , EJupContentType_mixed
                        , EJupLicenseType_DLA_With_Metering);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_MeteringByMID_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_metering_or_sync(testparam
                        , EPVMeterTestType_byMID
                        , EJupContentType_mixed
                        , EJupLicenseType_DLA_With_Metering);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_MeteringAll_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_metering_or_sync(testparam
                        , EPVMeterTestType_All
                        , EJupContentType_mixed
                        , EJupLicenseType_DLA_With_Metering);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_LicenseUpdateAll_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_metering_or_sync(testparam
                        , EPVMeterTestType_SyncAll
                        , EJupContentType_mixed
                        , EJupLicenseType_DLA_With_Sync);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_LicenseUpdateExpired_JupiterCPMTest:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_metering_or_sync(testparam
                        , EPVMeterTestType_SyncExpiredOnly
                        , EJupContentType_mixed
                        , EJupLicenseType_DLA_With_Sync);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;


            case DLA_DeleteLicense_JupiterCPMTest_v2_Content:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_deletelicense(testparam, EJupContentType_v2_WMA, EJupLicenseType_DLA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_DeleteLicense_JupiterCPMTest_v24_Content:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_deletelicense(testparam, EJupContentType_v24_WMA, EJupLicenseType_DLA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_CancelAcquireLicense_JupiterCPMTest_v4_Enveloped_Image:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_dla_cancelacquirelicense_envelopeimageread(testparam
                        , EJupContentType_v4_Enveloped_Image);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_CancelAcquireLicense_JupiterCPMTest_v2_Content:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_cancelacquirelicense(testparam, EJupContentType_v2_WMA, EJupLicenseType_DLA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_CancelAcquireLicense_JupiterCPMTest_v24_Content:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_cancelacquirelicense(testparam, EJupContentType_v24_WMA, EJupLicenseType_DLA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_CancelAcquireLicense_JupiterCPMTest_v4_Enveloped_MP4:
#if RUN_CPMJUPITER_TESTCASES
                iCurrentTest = new pvplayer_async_test_jupitercpm_cancelacquirelicense(testparam, EJupContentType_v4_Enveloped_MP4, EJupLicenseType_DLA);
#else
                fprintf(file, "Jupiter CPM tests not enabled\n");
#endif
                break;

            case DLA_PDL_OpenPlayUntilEOS_JanusCPMTest:
                fprintf(file, "Janus CPM + PDL tests not enabled\n");
                break;

            case StreamingOpenPlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;
            case StreamingCloakingOpenPlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
#if RUN_RTSP_CLOAKING_TESTCASES
                fprintf(file, "StreamingCloakingOpenPlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        true,
                        false,
                        false);
#else
                fprintf(file, "Streaming: cloaking tests not enabled\n");
#endif
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;
            case StreamingOpenPlayPausePlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlayPausePlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        true,
                        false,
                        true,
                        false,
                        false,
                        false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;

            case StreamingOpenPlayMultiplePausePlayUntilEOSTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlayPausePlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        true,
                        false,
                        true,
                        false,
                        false,
                        false);

                pvplayer_async_test_streamingopenplaystop* ptr =
                    (pvplayer_async_test_streamingopenplaystop*)iCurrentTest;
                ptr->setMultiplePauseMode(1);

#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;


            case StreamingOpenPlaySeekPlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlaySeekPlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        true,
                        true,
                        false,
                        false,
                        false);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;

            case StreamingOpenPlayForwardPlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingOpenPlayForwardPlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        true);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;

            case StreamingJitterBufferAdjustUntilEOSTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingJitterBufferAdjustUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingJBadjust(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;

            case StreamingPlayBitStreamSwitchPlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
#ifndef ONLY_3GPP_STREAMING_FORMAT
                fprintf(file, "StreamingPlayBitStreamSwitchPlayUntilEOSTest");
                if ((NULL == (oscl_strstr(testparam.iFileName, "rtsp://"))) &&
                        (NULL == (oscl_strstr(testparam.iFileName, ".sdp"))))
                {
                    iCurrentTest = new pvplayer_async_test_streaming_bitstream_switch(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true);
                }
                else
                {
                    fprintf(file, "\nBitStreamSwitching Can't be initiated by client for a RTSP url\n");
                }
#else
                fprintf(file, "Streaming: bitstream switching tests not enabled\n");
#endif
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;

            case StreamingMultiplePlayUntilEOSTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingMultiplePlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);

                pvplayer_async_test_streamingopenplaystop* ptr =
                    (pvplayer_async_test_streamingopenplaystop*)iCurrentTest;
                ptr->setMultiplePlayMode(2);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingMultipleCloakingPlayUntilEOSTest:
            {
#if RUN_STREAMING_TESTCASES
#if RUN_RTSP_CLOAKING_TESTCASES
                fprintf(file, "StreamingMultipleCloakingPlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        true,
                        false,
                        false);

                pvplayer_async_test_streamingopenplaystop* ptr =
                    (pvplayer_async_test_streamingopenplaystop*)iCurrentTest;
                ptr->setMultiplePlayMode(2);
#else
                fprintf(file, "Streaming: cloaking tests not enabled\n");
#endif
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingProtocolRollOverTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingProtocolRollOverTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);

                ((pvplayer_async_test_streamingopenplaystop*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingProtocolRollOverTestWithUnknownURLType:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingProtocolRollOverTestWithUnknownURLType");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);

                ((pvplayer_async_test_streamingopenplaystop*)iCurrentTest)->setProtocolRollOverModeWithUnknownURL();
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingPlayListSeekTest:
            {
#if RUN_STREAMING_TESTCASES
#ifndef ONLY_3GPP_STREAMING_FORMAT
                fprintf(file, "StreamingPlayListSeekTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        true,
                        true,
                        false,
                        false,
                        false);

                ((pvplayer_async_test_streamingopenplaystop*)iCurrentTest)->setPlayListMode();
#else
                fprintf(file, "Streaming: playlist tests not enabled\n");
#endif
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingSeekAfterEOSTest:
            {
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingSeekAfterEOSTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);

                pvplayer_async_test_streamingopenplaystop* ptr =
                    (pvplayer_async_test_streamingopenplaystop*)iCurrentTest;
                ptr->setSeekAfterEOSMode();
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
            }
            break;

            case StreamingLongPauseTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "StreamingLongPauseTest");
                iCurrentTest = new pvplayer_async_test_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        true,
                        false,
                        true,
                        false,
                        false,
                        false);

                ((pvplayer_async_test_streamingopenplaystop*)iCurrentTest)->setPauseDurationInMS(2*60*1000);
#else
                fprintf(file, "Streaming tests not enabled\n");
#endif
                break;

            case DLA_StreamingOpenPlayUntilEOST_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayPausePlayUntilEOS_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlaySeekPlayUntilEOS_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingMultiplePlayUntilEOS_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingProtocolRollOverTest_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingProtocolRollOverTestWithUnknownURLType_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAcquireLicense_JanusCPMTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case GenericReset_AddDataSource:
            {
                fprintf(file, "GenericReset_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_ADDDATASOURCE);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_Init:
            {
                fprintf(file, "GenericReset_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_INIT);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_AddDataSinkVideo:
            {
                fprintf(file, "GenericReset_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_ADDDATASINK_VIDEO);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_AddDataSinkAudio:
            {
                fprintf(file, "GenericReset_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_ADDDATASINK_AUDIO);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_Prepare:
            {
                fprintf(file, "GenericReset_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_PREPARE);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_Start:
            {
                fprintf(file, "GenericReset_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_START);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_Pause:
            {
                fprintf(file, "GenericReset_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_PAUSE);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_Resume:
            {
                fprintf(file, "GenericReset_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         true,
                                                         false,
                                                         true,
                                                         false,
                                                         false, STATE_RESUME);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericReset_Stop:
            {
                fprintf(file, "GenericReset_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         false,
                                                         false,
                                                         false,
                                                         false,
                                                         false, STATE_STOP);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericReset_SetPlaybackRange:
            {
                fprintf(file, "GenericReset_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericreset(testparam,
                                                         PVMF_YUV420,
                                                         PVMF_PCM16,
                                                         iCurrentTestNumber,
                                                         false,
                                                         true,
                                                         true,
                                                         false,
                                                         false, STATE_SETPLAYBACKRANGE);
                ((pvplayer_async_test_genericreset*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_AddDataSource:
            {
                fprintf(file, "GenericDelete_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_ADDDATASOURCE, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_Init:
            {
                fprintf(file, "GenericDelete_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_INIT, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_AddDataSinkVideo:
            {
                fprintf(file, "GenericDelete_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_ADDDATASINK_VIDEO, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_AddDataSinkAudio:
            {
                fprintf(file, "GenericDelete_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_ADDDATASINK_AUDIO, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_Prepare:
            {
                fprintf(file, "GenericDelete_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_PREPARE, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_Start:
            {
                fprintf(file, "GenericDelete_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_START, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_Pause:
            {
                fprintf(file, "GenericDelete_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_PAUSE, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_Resume:
            {
                fprintf(file, "GenericDelete_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_RESUME, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDelete_Stop:
            {
                fprintf(file, "GenericDelete_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          false, STATE_STOP, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericDelete_SetPlaybackRange:
            {
                fprintf(file, "GenericDelete_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          false,
                                                          true,
                                                          true,
                                                          false,
                                                          false, STATE_SETPLAYBACKRANGE, false);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_AddDataSource:
            {
                fprintf(file, "GenericDeleteWhileProc_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_ADDDATASOURCE, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_Init:
            {
                fprintf(file, "GenericDeleteWhileProc_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_INIT, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "GenericDeleteWhileProc_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_ADDDATASINK_VIDEO, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "GenericDeleteWhileProc_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_ADDDATASINK_AUDIO, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_Prepare:
            {
                fprintf(file, "GenericDeleteWhileProc_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_PREPARE, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_Start:
            {
                fprintf(file, "GenericDeleteWhileProc_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_START, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_Pause:
            {
                fprintf(file, "GenericDeleteWhileProc_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_PAUSE, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_Resume:
            {
                fprintf(file, "GenericDeleteWhileProc_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          true,
                                                          false,
                                                          true,
                                                          false,
                                                          false, STATE_RESUME, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_Stop:
            {
                fprintf(file, "GenericDeleteWhileProc_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          false,
                                                          false,
                                                          false,
                                                          false,
                                                          false, STATE_STOP, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericDeleteWhileProc_SetPlaybackRange:
            {
                fprintf(file, "GenericDeleteWhileProc_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericdelete(testparam,
                                                          PVMF_YUV420,
                                                          PVMF_PCM16,
                                                          iCurrentTestNumber,
                                                          false,
                                                          true,
                                                          true,
                                                          false,
                                                          false, STATE_SETPLAYBACKRANGE, true);
                ((pvplayer_async_test_genericdelete*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_AddDataSource:
            {
                fprintf(file, "GenericCancelAll_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_Init:
            {
                fprintf(file, "GenericCancelAll_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_INIT, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_AddDataSinkVideo:
            {
                fprintf(file, "GenericCancelAll_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_AddDataSinkAudio:
            {
                fprintf(file, "GenericCancelAll_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, false);

                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_Prepare:
            {
                fprintf(file, "GenericCancelAll_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_Start:
            {
                fprintf(file, "GenericCancelAll_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_START, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_Pause:
            {
                fprintf(file, "GenericCancelAll_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_Resume:
            {
                fprintf(file, "GenericCancelAll_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_Stop:
            {
                fprintf(file, "GenericCancelAll_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAll_SetPlaybackRange:
            {
                fprintf(file, "GenericCancelAll_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, false);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_AddDataSource:
            {
                fprintf(file, "GenericCancelAllWhileProc_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_Init:
            {
                fprintf(file, "GenericCancelAllWhileProc_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_INIT, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "GenericCancelAllWhileProc_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "GenericCancelAllWhileProc_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_Prepare:
            {
                fprintf(file, "GenericCancelAllWhileProc_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_Start:
            {
                fprintf(file, "GenericCancelAllWhileProc_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_START, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_Pause:
            {
                fprintf(file, "GenericCancelAllWhileProc_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_Resume:
            {
                fprintf(file, "GenericCancelAllWhileProc_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_Stop:
            {
                fprintf(file, "GenericCancelAllWhileProc_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericCancelAllWhileProc_SetPlaybackRange:
            {
                fprintf(file, "GenericCancelAllWhileProc_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelall(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, true);
                ((pvplayer_async_test_genericcancelall*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnect_AddDataSource:
            {
                fprintf(file, "GenericNetworkDisconnect_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_Init:
            {
                fprintf(file, "GenericNetworkDisconnect_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_INIT, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_AddDataSinkVideo:
            {
                fprintf(file, "GenericNetworkDisconnect_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_AddDataSinkAudio:
            {
                fprintf(file, "GenericNetworkDisconnect_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, false);

                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_Prepare:
            {
                fprintf(file, "GenericNetworkDisconnect_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_Start:
            {
                fprintf(file, "GenericNetworkDisconnect_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_START, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_Pause:
            {
                fprintf(file, "GenericNetworkDisconnect_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_Resume:
            {
                fprintf(file, "GenericNetworkDisconnect_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnect_Stop:
            {
                fprintf(file, "GenericNetworkDisconnect_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnect_SetPlaybackRange:
            {
                fprintf(file, "GenericNetworkDisconnect_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, false);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_AddDataSource:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_Init:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_INIT, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_Prepare:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_Start:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_START, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_Pause:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_Resume:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_Stop:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectWhileProc_SetPlaybackRange:
            {
                fprintf(file, "GenericNetworkDisconnectWhileProc_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, true);
                ((pvplayer_async_test_genericnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnectReconnect_AddDataSource:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_Init:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_INIT, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_AddDataSinkVideo:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_AddDataSinkAudio:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, false);

                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_Prepare:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_Start:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_START, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_Pause:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_Resume:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnect_Stop:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnectReconnect_SetPlaybackRange:
            {
                fprintf(file, "GenericNetworkDisconnectReconnect_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, false);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_AddDataSource:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_Init:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_INIT, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_Prepare:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_Start:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_START, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_Pause:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_Resume:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectReconnectWhileProc_Stop:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnectReconnectWhileProc_SetPlaybackRange:
            {
                fprintf(file, "GenericNetworkDisconnectReconnectWhileProc_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericnetworkdisconnectreconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, true);
                ((pvplayer_async_test_genericnetworkdisconnectreconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnectCancelAll_AddDataSource:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_Init:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_INIT, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_AddDataSinkVideo:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_AddDataSinkAudio:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, false);

                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_Prepare:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_Start:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_START, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_Pause:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_Resume:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_Stop:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAll_SetPlaybackRange:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAll_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, false);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_AddDataSource:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_AddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASOURCE, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_Init:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_Init");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_INIT, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_AddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_VIDEO, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_AddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_ADDDATASINK_AUDIO, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_Prepare:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_Prepare");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PREPARE, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_Start:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_Start");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_START, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_Pause:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_Pause");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_PAUSE, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_Resume:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_Resume");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false, STATE_RESUME, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_Stop:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_Stop");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false,
                            false,
                            false, STATE_STOP, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;
            case GenericNetworkDisconnectCancelAllWhileProc_SetPlaybackRange:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProc_SetPlaybackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false, STATE_SETPLAYBACKRANGE, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericNetworkDisconnectCancelAllWhileProtocolRollover:
            {
                fprintf(file, "GenericNetworkDisconnectCancelAllWhileProtocolRollover");
                iCurrentTest =
                    new pvplayer_async_test_genericcancelallnetworkdisconnect(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false, STATE_PROTOCOLROLLOVER, true);
                ((pvplayer_async_test_genericcancelallnetworkdisconnect*)iCurrentTest)->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayMultiplePauseResumeUntilEOSTest:
            {
                fprintf(file, "GenericOpenPlayMultiplePauseResumeUntilEOSTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setMultiplePauseMode(NUM_PAUSE, SEQUENTIAL_PAUSE_INTERVAL, FIRST_PAUSE_AFTER_START);
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayMultipleSeekUntilEOSTest:
            {
                fprintf(file, "GenericOpenPlayMultipleSeekUntilEOSTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
                ptr->setMultipleSeekMode(SEQUENTIAL_SEEK_INTERVAL, FIRST_SEEK_AFTER_START);
            }
            break;

            case GenericOpenPlayStop_SleepAddDataSource:
            {
                fprintf(file, "GenericOpenPlayStop_SleepAddDataSource");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_ADDDATASOURCE,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepInit:
            {
                fprintf(file, "GenericOpenPlayStop_SleepInit");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_INIT,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepAddDataSinkVideo:
            {
                fprintf(file, "GenericOpenPlayStop_SleepAddDataSinkVideo");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_ADDDATASINK_VIDEO,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepAddDataSinkAudio:
            {
                fprintf(file, "GenericOpenPlayStop_SleepAddDataSinkAudio");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_ADDDATASINK_AUDIO,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepPrepare:
            {
                fprintf(file, "GenericOpenPlayStop_SleepPrepare");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_PREPARE,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepGetMetaDataValueList:
            {
                fprintf(file, "GenericOpenPlayStop_SleepGetMetaDataValueList");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_GETMETADATAVALUELIST,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepStart:
            {
                fprintf(file, "GenericOpenPlayStop_SleepStart");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_START,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepPause:
            {
                fprintf(file, "GenericOpenPlayStop_SleepPause");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false,
                            STATE_PAUSE,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepResume:
            {
                fprintf(file, "GenericOpenPlayStop_SleepResume");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false,
                            STATE_RESUME,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepSetPlaybackRange:
            {
                fprintf(file, "GenericOpenPlayStop_SleepSetplayBackRange");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false,
                            STATE_SETPLAYBACKRANGE,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayStop_SleepStop:
            {
                fprintf(file, "GenericOpenPlayStop_SleepStop");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            STATE_STOP,
                            true);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayPauseResumeSeekStopProfiling:
            {
                fprintf(file, "GenericOpenPlayPauseResumeSeekStopProfiling");
                iCurrentTest =
                    new pvplayer_async_test_genericprofiling(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false);
                pvplayer_async_test_genericprofiling* ptr =
                    (pvplayer_async_test_genericprofiling*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayPauseRepositionResumeUntilEOSTest:
            {
                fprintf(file, "GenericOpenPlayPauseRepositionResumeUntilEOSTest");
                iCurrentTest =
                    new pvplayer_async_test_genericplaypauserepositionresumetest(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            false);
                pvplayer_async_test_genericplaypauserepositionresumetest* ptr =
                    (pvplayer_async_test_genericplaypauserepositionresumetest*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayPauseRepositionResumeNetworkDisconnectCancelAllTest:
            {
                fprintf(file, "GenericOpenPlayPauseRepositionResumeNetworkDisconnectCancelAllTest");
                iCurrentTest =
                    new pvplayer_async_test_genericplaypauserepositionresumenwdisconnectcancelalltest(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            false);
                pvplayer_async_test_genericplaypauserepositionresumenwdisconnectcancelalltest* ptr =
                    (pvplayer_async_test_genericplaypauserepositionresumenwdisconnectcancelalltest*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenSetPlaybackRangeStartPlayStopTest:
            {
                fprintf(file, "GenericOpenSetPlaybackRangeStartPlayStopTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopensetplaybackrangestartplaystoptest(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            false);
                pvplayer_async_test_genericopensetplaybackrangestartplaystoptest* ptr =
                    (pvplayer_async_test_genericopensetplaybackrangestartplaystoptest*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayRepositionToEndTest:
            {
                fprintf(file, "GenericOpenPlayRepositionToEndPlayTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplayrepositiontoendtest(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            false);
                pvplayer_async_test_genericopenplayrepositiontoendtest* ptr =
                    (pvplayer_async_test_genericopenplayrepositiontoendtest*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericPVMFErrorCorruptReNotified:
            {
                fprintf(file, "GenericPVMFErrorCorruptReNotified");
                iCurrentTest =
                    new pvplayer_async_test_genericpvmferrorcorruptrenotified(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false);
                pvplayer_async_test_genericpvmferrorcorruptrenotified* ptr =
                    (pvplayer_async_test_genericpvmferrorcorruptrenotified*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenPlayPauseGetMetaDataUntilEOSTest:
            {
                fprintf(file, "GenericOpenPlayPauseGetMetaDataUntilEOSTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaygetmetadatatest(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false);
                pvplayer_async_test_genericopenplaygetmetadatatest* ptr =
                    (pvplayer_async_test_genericopenplaygetmetadatatest*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case GenericOpenGetMetaDataPicTest:
            {
                fprintf(file, "GenericOpenGetMetaDataPicTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopengetmetadatapictest(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false);
                pvplayer_async_test_genericopengetmetadatapictest* ptr =
                    (pvplayer_async_test_genericopengetmetadatapictest*)iCurrentTest;
                ptr->setProtocolRollOverMode();
            }
            break;

            case DLA_StreamingOpenPlayUntilEOST_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingOpenPlayUntilEOST_JupiterCPMTest");
                iCurrentTest =
                    new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            false);
#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DLA_StreamingOpenPlayPausePlayUntilEOS_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingOpenPlayPausePlayUntilEOS_JupiterCPMTest");
                iCurrentTest =
                    new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            true,
                            false,
                            true,
                            false,
                            false,
                            false);
#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DLA_StreamingOpenPlaySeekPlayUntilEOS_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingOpenPlaySeekPlayUntilEOS_JupiterCPMTest");
                iCurrentTest = new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        true,
                        true,
                        false,
                        false,
                        false);
#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DLA_StreamingMultiplePlayUntilEOS_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingMultiplePlayUntilEOS_JupiterCPMTest");
                iCurrentTest =
                    new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false,
                            false);

                pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop* ptr =
                    (pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop*)iCurrentTest;
                ptr->setMultiplePlayMode(2);
                fprintf(file, "\n0x%x 0x%x setMultiplePlayMode\n", ptr, iCurrentTest);
#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DLA_StreamingProtocolRollOverTest_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingProtocolRollOverTest_JupiterCPMTest");
                iCurrentTest = new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);

                ((pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop*)iCurrentTest)->setProtocolRollOverMode();
#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DLA_StreamingProtocolRollOverTestWithUnknownURLType_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingProtocolRollOverTestWithUnknownURLType_JupiterCPMTest");
                iCurrentTest = new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        false);

                ((pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop*)iCurrentTest)->setProtocolRollOverModeWithUnknownURL();
#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DLA_StreamingCancelAll_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAll_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAll_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAll_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAll_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAll_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAll_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAll_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAll_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAll_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAll_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAllWhileProc_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAllWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAllWhileProc_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingCancelAllWhileProc_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnect_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnect_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnect_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnect_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnect_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnect_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnect_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnect_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnect_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnect_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnect_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectWhileProc_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectWhileProc_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectReconnect_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnect_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectReconnect_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;
            case DLA_StreamingNetworkDisconnectReconnectWhileProc_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectReconnectWhileProc_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAll_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAll_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAll_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAll_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAll_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_AddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_Init:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_LicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_AddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_AddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_Prepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_Start:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_Pause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_Resume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_Stop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;


            case DLA_StreamingNetworkDisconnectCancelAllWhileProc_SetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayMultiplePauseResumeUntilEOSTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayMultipleSeekUntilEOSTest:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepAddDataSource:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepInit:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }

            break;

            case DLA_StreamingOpenPlayStop_SleepLicenseAcquired:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepAddDataSinkVideo:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepAddDataSinkAudio:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepPrepare:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepStart:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepPause:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepResume:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepSetPlaybackRange:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingOpenPlayStop_SleepStop:
            {
                fprintf(file, "Streaming tests not enabled\n");
            }
            break;

            case DLA_StreamingCancelAcquireLicense_JupiterCPMTest:
            {
#if RUN_STREAMING_TESTCASES && RUN_CPMJUPITER_TESTCASES
                fprintf(file, "DLA_StreamingCancelAcquireLicense_JupiterCPMTest");
                iCurrentTest = new pvplayer_async_test_jupitercpm_DLAstreamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false,
                        true);

#else
                fprintf(file, "Jupiter Streaming tests not enabled\n");
#endif
            }
            break;

            case DVBH_StreamingOpenPlayStopTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "DVBH_StreamingOpenPlayStopTest");
                iCurrentTest = new pvplayer_async_test_dvbh_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        false,
                        false,
                        false);
#else
                fprintf(file, "DVB Streaming tests not enabled\n");
#endif
                break;

            case DVBH_StreamingOpenPlayUntilEOSTest:
#if RUN_STREAMING_TESTCASES
                fprintf(file, "DVBH_StreamingOpenPlayUntilEOSTest");
                iCurrentTest = new pvplayer_async_test_dvbh_streamingopenplaystop(testparam,
                        PVMF_YUV420,
                        PVMF_PCM16,
                        iCurrentTestNumber,
                        false,
                        false,
                        true,
                        false,
                        false);
#else
                fprintf(file, "DVB Streaming tests not enabled\n");
#endif
                break;

            case GenericOpenPlaySeekBeyondClipDurationTest:
            {
                fprintf(file, "GenericOpenPlaySeekBeyondClipDurationTest");
                iCurrentTest =
                    new pvplayer_async_test_genericopenplaystop(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            true,
                            true,
                            false,
                            false);
                pvplayer_async_test_genericopenplaystop* ptr =
                    (pvplayer_async_test_genericopenplaystop*)iCurrentTest;
                ptr->setProtocolRollOverMode();
                ptr->setSeekBeyondClipDurationMode();
            }
            break;

            /** ALP ACCESS DRM CPM TESTS */
            case QueryEngine_AccessCPMTest:
#if RUN_CPMACCESS_TESTCASES
                iCurrentTest = new pvplayer_async_test_accesscpm_query(testparam);
#else
                fprintf(file, "Access CPM tests not enabled\n");
#endif
                break;
            case OpenPlayStop_AccessCPMTest:
#if RUN_CPMACCESS_TESTCASES
                iCurrentTest = new pvplayer_async_test_accesscpm_openplaystop(testparam);
#else
                fprintf(file, "Access CPM tests not enabled\n");
#endif
                break;
            case PlayStopPlayStop_AccessCPMTest:
#if RUN_CPMACCESS_TESTCASES
                iCurrentTest = new pvplayer_async_test_accesscpm_playstopplay(testparam);
#else
                fprintf(file, "Access CPM tests not enabled\n");
#endif
                break;
            case StartupMeasurement_AccessCPMTest:
#if RUN_CPMACCESS_TESTCASES
                iCurrentTest = new pvplayer_async_test_accesscpm_startupmeasurement(testparam);
#else
                fprintf(file, "Access CPM tests not enabled\n");
#endif
                break;

            case OpenPlayStop_MultiCPMTest:
                fprintf(file, "Multi CPM tests not enabled\n");
                break;

            case ApplicationInvolvedTrackSelectionTestDefault:
            {
#if RUN_APP_TRACK_SELECTION_TESTCASES
                fprintf(file, "ApplicationInvolvedTrackSelectionTest");
                iCurrentTest =
                    new pvplayer_async_test_apptrackselection(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false);
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setProtocolRollOverMode();
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setTrackSelectionMode(PVPLAYER_TEST_TRACK_SELECTION_MODE_DEFAULT);
#else
                fprintf(file, "ApplicationInvolvedTrackSelectionTestDefault not enabled\n");
#endif
            }
            break;

            case ApplicationInvolvedTrackSelectionTestPassthru:
            {
#if RUN_APP_TRACK_SELECTION_TESTCASES
                fprintf(file, "ApplicationInvolvedTrackSelectionTestPassthru");
                iCurrentTest =
                    new pvplayer_async_test_apptrackselection(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false);
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setProtocolRollOverMode();
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setTrackSelectionMode(PVPLAYER_TEST_TRACK_SELECTION_MODE_PASSTHRU);
#else
                fprintf(file, "ApplicationInvolvedTrackSelectionTestPassthru not enabled\n");
#endif
            }
            break;

            case ApplicationInvolvedTrackSelectionTestAudioOnly:
            {
#if RUN_APP_TRACK_SELECTION_TESTCASES
                fprintf(file, "ApplicationInvolvedTrackSelectionTestAudioOnly");
                iCurrentTest =
                    new pvplayer_async_test_apptrackselection(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false);
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setProtocolRollOverMode();
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setTrackSelectionMode(PVPLAYER_TEST_TRACK_SELECTION_MODE_AUDIO_ONLY);
#else
                fprintf(file, "ApplicationInvolvedTrackSelectionTestAudioOnly not enabled\n");
#endif
            }
            break;

            case ApplicationInvolvedTrackSelectionTestVideoOnly:
            {
#if RUN_APP_TRACK_SELECTION_TESTCASES
                fprintf(file, "ApplicationInvolvedTrackSelectionTestVideoOnly");
                iCurrentTest =
                    new pvplayer_async_test_apptrackselection(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false);
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setProtocolRollOverMode();
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setTrackSelectionMode(PVPLAYER_TEST_TRACK_SELECTION_MODE_VIDEO_ONLY);
#else
                fprintf(file, "ApplicationInvolvedTrackSelectionTestVideoOnly not enabled\n");
#endif
            }
            break;

            case ApplicationInvolvedTrackSelectionTestTextOnly:
            {
#if RUN_APP_TRACK_SELECTION_TESTCASES
                fprintf(file, "ApplicationInvolvedTrackSelectionTestTextOnly");
                iCurrentTest =
                    new pvplayer_async_test_apptrackselection(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false);
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setProtocolRollOverMode();
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setTrackSelectionMode(PVPLAYER_TEST_TRACK_SELECTION_MODE_TEXT_ONLY);
#else
                fprintf(file, "ApplicationInvolvedTrackSelectionTestTextOnly not enabled\n");
#endif
            }
            break;

            case ApplicationInvolvedTrackSelectionTestNoTracks:
            {
#if RUN_APP_TRACK_SELECTION_TESTCASES
                fprintf(file, "ApplicationInvolvedTrackSelectionTestNoTracks");
                iCurrentTest =
                    new pvplayer_async_test_apptrackselection(testparam,
                            PVMF_YUV420,
                            PVMF_PCM16,
                            iCurrentTestNumber,
                            false,
                            false,
                            true,
                            false,
                            false);
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setProtocolRollOverMode();
                ((pvplayer_async_test_apptrackselection*)iCurrentTest)->setTrackSelectionMode(PVPLAYER_TEST_TRACK_SELECTION_MODE_NO_TRACKS);
#else
                fprintf(file, "ApplicationInvolvedTrackSelectionTestNoTracks not enabled\n");
#endif
            }
            break;


            case BeyondLastTest:
                AtleastOneExecuted = false; //Resetting the flag
                break;

            default:
                PVPlayerProjTestFactory projTestFactory;
                iCurrentTest = projTestFactory.GetProjTest(iCurrentTestNumber, testparam);
                if (!iCurrentTest)
                {
                    //exit the loop.
                    iCurrentTestNumber = BeyondLastTest;
                }
                break;
        }

        if (iCurrentTest)
        {
            OsclExecScheduler *sched = OsclExecScheduler::Current();
            if (sched)
            {
                // Print out the test name
                fprintf(file, "%s\n", iCurrentTest->iTestCaseName.get_cstr());
                // Start the test
                iCurrentTest->StartTest();
                // Start the scheduler so the test case would run
#if USE_NATIVE_SCHEDULER
                // Have PV scheduler use the scheduler native to the system
                sched->StartNativeScheduler();
#else
                // Have PV scheduler use its own implementation of the scheduler
                sched->StartScheduler();
#endif
            }
            else
            {
                fprintf(file, "ERROR! Scheduler is not available. Test case could not run.");
                ++iCurrentTestNumber;
            }
        }
        else //no test to run, skip to next test.
        {
            ++iCurrentTestNumber;
            if (setupLoggerScheduler)
            {
                // Shutdown PVLogger and scheduler before continuing on
                OsclScheduler::Cleanup();
                PVLogger::Cleanup();
                setupLoggerScheduler = false;
            }
        }
    }
}


void pvplayer_engine_test::SetupLoggerScheduler()
{
    // Enable the following code for logging using PV Logger
    PVLogger::Init();
#if 1
    OSCL_wHeapString<OsclMemAllocator> logfilename(OUTPUTNAME_PREPEND_WSTRING);
    logfilename += _STRLIT_WCHAR("player.log");

    PVLoggerAppender *appender = NULL;
    OsclRefCounter *refCounter = NULL;
    if (iLogMem)
    {
        appender = (PVLoggerAppender*)MemAppender<TimeAndIdLayout, 1024>::CreateAppender(logfilename.get_str(), 0x200000);
        OsclRefCounterSA<LogAppenderDestructDealloc<MemAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
            new OsclRefCounterSA<LogAppenderDestructDealloc<MemAppender<TimeAndIdLayout, 1024> > >(appender);

        refCounter = appenderRefCounter;
    }
    else if (iLogFile)
    {
        appender = (PVLoggerAppender*)TextFileAppender<TimeAndIdLayout, 1024>::CreateAppender(logfilename.get_str());
        OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
            new OsclRefCounterSA<LogAppenderDestructDealloc<TextFileAppender<TimeAndIdLayout, 1024> > >(appender);
        refCounter = appenderRefCounter;
    }
    else
    {
        appender = new StdErrAppender<TimeAndIdLayout, 1024>();
        OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > > *appenderRefCounter =
            new OsclRefCounterSA<LogAppenderDestructDealloc<StdErrAppender<TimeAndIdLayout, 1024> > >(appender);
        refCounter = appenderRefCounter;
    }

    OsclSharedPtr<PVLoggerAppender> appenderPtr(appender, refCounter);


    PVLoggerConfigFile obj;
    if (obj.IsLoggerConfigFilePresent())
    {
        obj.SetLoggerSettings();
        // Construct and install the active scheduler
        OsclScheduler::Init("PVPlayerEngineTestScheduler");
        return;
    }

    switch (iLogNode)
    {
        case 0: //default
        {
            //selective logging
            PVLogger *node;
            node = PVLogger::GetLoggerObject("PVPlayerEngine");
            node->AddAppender(appenderPtr);
            node->SetLogLevel(iLogLevel);
            /*
            		node = PVLogger::GetLoggerObject("PVMFMP4FFParserNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFAMRFFParserNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFAACFFParserNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFMP3FFParserNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFASFParserNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFVideoDecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFAVCDecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFWmvDecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFGSMAMRDecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFAACDecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFMP3DecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFWMADecNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMFFileOutputNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMediaOutputNode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PVMediaOutputNodePort");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PvmfSyncUtil");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("PvmfSyncUtilDataQueue");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
            /*
            		node = PVLogger::GetLoggerObject("datapath.asfparsernode");
            		node->AddAppender(appenderPtr);
            		node->SetLogLevel(iLogLevel);
            */
        }
        break;
        case 1: //-logall
        {
            // Log all
            PVLogger *rootnode = PVLogger::GetLoggerObject("");
            rootnode->AddAppender(appenderPtr);
            rootnode->SetLogLevel(iLogLevel);
        }
        break;
        case 2: //-logdatapath
        {
            // Log datapath only
            PVLogger *node = PVLogger::GetLoggerObject("datapath");
            node->AddAppender(appenderPtr);
            //info level logs ports & synchronization info.
            node->SetLogLevel(PVLOGMSG_INFO);
            //debug level includes additional memory pool logging.
            //node->SetLogLevel(PVLOGMSG_DEBUG);
            //diasble logs from all its child nodes
            PVLogger* datapathsrc = PVLogger::GetLoggerObject("datapath.sourcenode");
            datapathsrc->DisableAppenderInheritance();
            PVLogger* datapathdec = PVLogger::GetLoggerObject("datapath.decnode");
            datapathdec->DisableAppenderInheritance();
        }
        break;
        case 3: //-logclock
        {
            // Log clock only
            PVLogger *clocknode = PVLogger::GetLoggerObject("clock");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(PVLOGMSG_DEBUG);
        }
        break;
        case 4: //-logoscl
        {
            // Log oscl only
            PVLogger *clocknode = PVLogger::GetLoggerObject("pvscheduler");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(iLogLevel);
            clocknode = PVLogger::GetLoggerObject("osclsocket");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(iLogLevel);
        }
        break;
        case 5: //-logperf
        {
            // Log scheduler perf only.
            PVLogger *clocknode ;

            clocknode = PVLogger::GetLoggerObject("pvscheduler");
            clocknode->AddAppender(appenderPtr);
            //info level logs scheduler activity including AO calls and idle time.
            clocknode->SetLogLevel(PVLOGMSG_INFO);
            //debug level logs additional AO request status changes.
            //clocknode->SetLogLevel(PVLOGMSG_DEBUG);

            //log streaming packet loss
            clocknode = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(PVLOGMSG_DEBUG);
        }
        break;
        case 6: //-logdatapathsrc
        {
            // Log source node data path only.
            PVLogger *node = NULL;
            node = PVLogger::GetLoggerObject("datapath.sourcenode");
            node->AddAppender(appenderPtr);
            node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.mp4parsernode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.socketnode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.protocolenginenode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.in");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.jitterbuffer.out");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.medialayer");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.medialayer.in");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.medialayer.out");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.sourcenode.medialayer.portflowcontrol");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
        }
        break;
        case 7://-logdatapathdec
        {
            // Log decoder node data path only.
            PVLogger *node = NULL;
            node = PVLogger::GetLoggerObject("datapath.decnode");
            node->AddAppender(appenderPtr);
            node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.decnode.m4vdecnode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.decnode.aacdecnode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.decnode.wmadecnode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
            //node = PVLogger::GetLoggerObject("datapath.decnode.wmvdecnode");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_INFO);
        }
        break;
        case 8://-logsync
        {
            PVLogger *node = NULL;
            node = PVLogger::GetLoggerObject("datapath.sinknode");
            node->AddAppender(appenderPtr);
            //info level logs ports & synchronization info.
            node->SetLogLevel(PVLOGMSG_DEBUG);
        }
        break;
        case 9://-logdiagnostics
        {
            PVLogger *node = NULL;
            node = PVLogger::GetLoggerObject("pvplayerdiagnostics");
            node->AddAppender(appenderPtr);
            node->SetLogLevel(PVLOGMSG_DEBUG);
            //node = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");
            //node->AddAppender(appenderPtr);
            //node->SetLogLevel(PVLOGMSG_DEBUG);
        }
        break;
        case 10://-logosclfileio
        {
            PVLogger *clocknode = PVLogger::GetLoggerObject("Oscl_File");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(PVLOGMSG_DEBUG + 1);
        }
        break;
        case 11://-loghds
        {
            PVLogger *clocknode = PVLogger::GetLoggerObject("PVWmdrmHds");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(PVLOGMSG_DEBUG + 1);
        }
        break;
        case 12://-loghdsandosclfileio
        {
            PVLogger *clocknode = PVLogger::GetLoggerObject("PVWmdrmHds");
            clocknode->AddAppender(appenderPtr);

            clocknode = PVLogger::GetLoggerObject("Oscl_File");
            clocknode->AddAppender(appenderPtr);
            clocknode->SetLogLevel(PVLOGMSG_DEBUG + 1);
        }
        break;
        case 13://-logjupiter
        {
            //jupiter debugging.
            PVLogger *loggernode;
            loggernode = PVLogger::GetLoggerObject("PVDrmManagerJupiter");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("PVWMDRM");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            //loggernode->SetLogLevel(PVLOGMSG_DEBUG+1);//for ChkDR and Oem time logging
            loggernode = PVLogger::GetLoggerObject("PVMFJupiterPlugin");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            /*
            loggernode = PVLogger::GetLoggerObject("PVMFJupiterPluginLocalSyncAccessInterfaceImpl");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            */
            loggernode = PVLogger::GetLoggerObject("PVMFJupiterUtility");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("HTTPRequest");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            /*
            loggernode = PVLogger::GetLoggerObject("oscldns");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("osclsocket");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("osclsocketserv");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("osclsocket_serv");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            */
            loggernode = PVLogger::GetLoggerObject("datapath.sourcenode.asfparsernode.drm");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
        }
        break;
        case 14://-logjanus
        {
            //janus debugging.
            PVLogger *loggernode;
            loggernode = PVLogger::GetLoggerObject("PVDrmManager");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("PVWMDRM");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            //loggernode->SetLogLevel(PVLOGMSG_DEBUG+1);//for ChkDR and Oem time logging
            loggernode = PVLogger::GetLoggerObject("PVMFJanusPlugin");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            /*
            loggernode = PVLogger::GetLoggerObject("PVMFJanusPluginLocalSyncAccessInterfaceImpl");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            */
            loggernode = PVLogger::GetLoggerObject("HTTPRequest");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            /*
            loggernode = PVLogger::GetLoggerObject("oscldns");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("osclsocket");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("osclsocketserv");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            loggernode = PVLogger::GetLoggerObject("osclsocket_serv");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            */
        }
        break;
        case 15://-logperfmin
        {
            //minimal scheduler perf logging
            PVLogger *loggernode;
            loggernode = PVLogger::GetLoggerObject("OsclSchedulerPerfStats");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_INFO);
        }
        break;
        case 16://-logppb
        {
            // Log progressive playback node data path only.
            PVLogger *loggernode;
            /*
                    loggernode = PVLogger::GetLoggerObject("datapath.sourcenode");
                    loggernode->AddAppender(appenderPtr);
                    loggernode->SetLogLevel(PVLOGMSG_INFO);
            */
            loggernode = PVLogger::GetLoggerObject("datapath.sourcenode.mp4parsernode");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_INFO);
            /*
            loggernode = PVLogger::GetLoggerObject("datapath.socketnode");
                    loggernode->AddAppender(appenderPtr);
                    loggernode->SetLogLevel(PVLOGMSG_INFO);
            */
            /*
            loggernode = PVLogger::GetLoggerObject("datapath.sourcenode.protocolenginenode");
             loggernode->AddAppender(appenderPtr);
             loggernode->SetLogLevel(PVLOGMSG_INFO);
            */

            loggernode = PVLogger::GetLoggerObject("datapath.decnode");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_INFO);

            loggernode = PVLogger::GetLoggerObject("PVMediaOutputNode");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_INFO);

            loggernode = PVLogger::GetLoggerObject("PVRefFileOutput");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);

            loggernode = PVLogger::GetLoggerObject("PVMFOMXVideoDecNode");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_STACK_TRACE);

        }
        break;
        case 17://-logrepos
        {
            PVLogger *node = NULL;
            node = PVLogger::GetLoggerObject("pvplayerrepos");
            node->AddAppender(appenderPtr);
            //info level logs ports & synchronization info.
            node->SetLogLevel(PVLOGMSG_DEBUG);
        }
        break;
        case 18: //-logsnode
        {
            //minimal scheduler perf logging
            PVLogger *loggernode;
            loggernode = PVLogger::GetLoggerObject("OsclSchedulerPerfStats");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_INFO);

            //socket node debugging
            loggernode = PVLogger::GetLoggerObject("pvplayerdiagnostics.socketnode");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            /*
            loggernode=PVLogger::GetLoggerObject("datapath.socketnode");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
            */
            loggernode = PVLogger::GetLoggerObject("pvplayerdiagnostics.streamingmanager");
            loggernode->AddAppender(appenderPtr);
            loggernode->SetLogLevel(PVLOGMSG_DEBUG);
        }
        default:
            break;
    }
#endif //logging

    // Construct and install the active scheduler
    OsclScheduler::Init("PVPlayerEngineTestScheduler");
}






