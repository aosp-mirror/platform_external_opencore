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
#ifndef TEST_PV_MEDIAINPUT_AUTHOR_ENGINE_H
#define TEST_PV_MEDIAINPUT_AUTHOR_ENGINE_H

#ifndef TEST_PV_AUTHOR_ENGINE_TYPEDEFS_H
#include "test_pv_author_engine_typedefs.h"
#endif

#ifndef PV_AVIFILE_H_INCLUDED
#include "pv_avifile.h"
#endif

#ifndef PVWAVFILEPARSER_H_INCLUDED
#include "pvwavfileparser.h"
#endif

#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif

#ifndef PVMF_MEDIA_INPUT_NODE_FACTORY_H_INCLUDED
#include "pvmf_media_input_node_factory.h"
#endif

#ifndef PVMI_MIO_AVIFILE_FACTORY_H_INCLUDED
#include "pvmi_mio_avi_wav_file_factory.h"
#endif

typedef struct _PVMediaInputTestParam
{
    FILE* iFile;
    PVMFFormatType	iInputFormat;
    OSCL_HeapString<OsclMemAllocator> iIPFileInfo;
    OSCL_HeapString<OsclMemAllocator> iOPFileInfo;
    OSCL_HeapString<OsclMemAllocator> iVideoEncInfo;
    OSCL_HeapString<OsclMemAllocator> iAudioEncInfo;
    OSCL_HeapString<OsclMemAllocator> iComposerInfo;
    uint32 iLoopTime;
    bool iRealTimeAuthoring;

}PVMediaInputTestParam;

typedef struct _PVMediaInputAuthorEngineTestParam
{
    int32 iLogLevel;
    int32 iLogNode;
    int32 iLogFile;
    int32 iFirstTest;
    int32 iLastTest;
    PVMediaInputTestParam iMediainputParam;

}PVMediaInputAuthorEngineTestParam;

class PVMIOControlComp
{

    public:
        /** constructor */
        PVMIOControlComp(PVMFFormatType aType, OsclAny* aFileParser, uint32 aLoopDuration)
        {
            switch (aType)
            {
                case PVMF_WAVFF:
                {
                    iPVWavFile = OSCL_STATIC_CAST(PV_Wav_Parser*, aFileParser);
                    iPVAviFile = NULL;
                    PVWAVFileInfo wavFileInfo;
                    iPVWavFile->RetrieveFileInfo(wavFileInfo);
                    iFileDuration = wavFileInfo.NumSamples / wavFileInfo.SampleRate; //in sec
                }
                break;

                case PVMF_AVIFF:
                {
                    iPVAviFile = OSCL_STATIC_CAST(PVAviFile*, aFileParser);
                    iPVWavFile = NULL;
                    iFileDuration = iPVAviFile->GetFileDuration(); //in sec
                }
                break;

                default :
                    break;
            }

            iLogger = PVLogger::GetLoggerObject("PVMIOControlComp");
            iLoopDuration = aLoopDuration;

        }

        /** default constructor */
        PVMIOControlComp()
        {
            iPVAviFile = NULL;
            iPVWavFile = NULL;
            iLogger = NULL;
            iFileDuration = 0;
            iLoopDuration = 0;
        };

        /** default destructor */
        ~PVMIOControlComp() {}

        /**
         * Copy constructor
         * @param arInput Source object
         */
        PVMIOControlComp(const PVMIOControlComp& arInput)
        {
            iMIONode = arInput.iMIONode;
            iMediaInput = arInput.iMediaInput;
            iPVAviFile = arInput.iPVAviFile;
            iPVWavFile = arInput.iPVWavFile;
            iLogger = arInput.iLogger;
            iFileDuration = arInput.iFileDuration;
            iLoopDuration = arInput.iLoopDuration;
        }

        /**
        * Assignment Operator
        * @param arInput Source object
        * @return Reference to this object with values copied from the source object
        */
        PVMIOControlComp& operator=(const PVMIOControlComp& arInput)
        {
            iMIONode = arInput.iMIONode;
            iMediaInput = arInput.iMediaInput;
            iPVAviFile = arInput.iPVAviFile;
            iPVWavFile = arInput.iPVWavFile;
            iLogger = arInput.iLogger;
            iFileDuration = arInput.iFileDuration;
            iLoopDuration = arInput.iLoopDuration;
            return (*this);
        }

        /**
         * Query whether the specified input type is supported
         *
         * @param aType Input type to be supported
         * @return True if input type is supported, else false
         */
        bool IsTestInputTypeSupported(PVMFFormatType aType)
        {
            switch (aType)
            {
                case PVMF_WAVFF:
                case PVMF_AVIFF:
                {
                    return true;
                }
                default:
                    return false;
            }
        }

        /**
         * Creates an input node of the specified type.  DeleteInputNode() will need to
         * be called to deallocate the input node.  The default contructor will not
         * delete the input node created by this method.
         *
         * @param aType Type of input node to create
         * @return True for success, else false
         */
        int32 CreateMIOInputNode(bool aRecMode, PVMFFormatType aType, const OSCL_wString& aFileName)
        {
            OSCL_UNUSED_ARG(aFileName);
            int32 error = PVMFFailure;
            if (!IsTestInputTypeSupported(aType))
            {
                return error;
            }

            uint32 loopCount = 0;
            if (iLoopDuration)
            {
                loopCount = iLoopDuration / iFileDuration;
            }
            switch (aType)
            {
                case PVMF_WAVFF:
                {
                    PvmiMIOControl* mediaInput = PvmiMIOAviWavFileFactory::Create(loopCount, aRecMode, 0, (OsclAny*)iPVWavFile, FILE_FORMAT_WAV, error);
                    if (!mediaInput)
                    {
                        PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                        (0, "CreateMIOInputNode::CreateMIOInputNode: Error - PvmiMIOAviWavFileFactory::Create failed"));

                        return PVMFFailure;
                    }

                    iMediaInput.push_back(mediaInput);
                }
                break;

                case PVMF_AVIFF:
                {
                    for (uint32 ii = 0; ii < iPVAviFile->GetNumStreams(); ii++)
                    {
                        PvmiMIOControl* mediaInput = PvmiMIOAviWavFileFactory::Create(loopCount, aRecMode, ii, (OsclAny*)iPVAviFile, FILE_FORMAT_AVI, error);
                        if (!mediaInput)
                        {
                            PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                            (0, "CreateMIOInputNode::CreateMIOInputNode: Error - PvmiMIOAviWavFileFactory::Create failed"));

                            return PVMFFailure;
                        }

                        iMediaInput.push_back(mediaInput);
                    }
                }
                break;
                default :
                    break;
            }

            if (error != PVMFSuccess)
            {
                return error;
            }

            for (uint32 ii = 0; ii < iMediaInput.size(); ii++)
            {
                PVMFNodeInterface *node = PvmfMediaInputNodeFactory::Create(iMediaInput[ii]);
                if (!node)
                {
                    PVLOGGER_LOGMSG(PVLOGMSG_INST_REL, iLogger, PVLOGMSG_ERR,
                                    (0, "CreateMIOInputNode::CreateMIOInputNode: Error - PvmfMediaInputNodeFactory::Create failed"));

                    return PVMFFailure;
                }
                iMIONode.push_back(node);
            }

            return error;
        }

        /**
         * Deletes the input node contained by this object
         */
        void DeleteInputNode()
        {
            uint32 ii = 0;
            for (ii = 0; ii < iMIONode.size() ; ii++)
            {
                PvmfMediaInputNodeFactory::Delete(iMIONode[ii]);
            }

            for (ii = 0; ii < iMediaInput.size(); ii++)
            {
                PvmiMIOAviWavFileFactory::Delete(iMediaInput[ii]);
            }

        }

        PVAviFile*						   iPVAviFile;
        PV_Wav_Parser*					   iPVWavFile;
        PVLogger*                          iLogger;
        Oscl_Vector < PVMFNodeInterface*,
        OsclMemAllocator >    iMIONode;
        Oscl_Vector < PvmiMIOControl*,
        OsclMemAllocator >    iMediaInput;
        uint32							   iLoopDuration;
        OsclFloat						   iFileDuration;
};


class PVMediaInputAuthorEngineTest: public test_case,
            public pvauthor_async_test_observer,
            public PVLoggerSchedulerSetup
{
    public:
        PVMediaInputAuthorEngineTest(PVMediaInputAuthorEngineTestParam aTestParam):
                iCurrentTest(NULL),

                PVLoggerSchedulerSetup(aTestParam.iLogFile, aTestParam.iLogLevel, aTestParam.iLogNode),
                iFirstTest(aTestParam.iFirstTest),
                iLastTest(aTestParam.iLastTest),
                iNextTestCase(aTestParam.iFirstTest),
                iFile(aTestParam.iMediainputParam.iFile),
                iMediaInputParam(aTestParam.iMediainputParam)
        {};

        // From test_case
        virtual void test();
        virtual void CompleteTest(test_case& arTC);
        void RunTestCases();

    private:
        pvauthor_async_test_base* iCurrentTest;
        int32                     iFirstTest;
        int32                     iLastTest;
        int32                     iNextTestCase;
        FILE*                     iFile;
        // For test results
        int                       iTotalSuccess;
        int                       iTotalError;
        int                       iTotalFail;
        // For memory statistics
        uint32                    iTotalAlloc;
        uint32                    iTotalBytes;
        uint32                    iAllocFails;
        uint32                    iNumAllocs;
        PVMediaInputTestParam     iMediaInputParam;
};

class PVMediaInputAuthorEngineTestSuite: public test_case
{
    public:
        PVMediaInputAuthorEngineTestSuite(PVMediaInputAuthorEngineTestParam aTestParam)
        {
            adopt_test_case(new PVMediaInputAuthorEngineTest(aTestParam));
        }

        ~PVMediaInputAuthorEngineTestSuite() {} ;

};

#endif //#fidef TEST_PV_MEDIAINPUT_AUTHOR_ENGINE_H

