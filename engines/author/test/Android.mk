LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvaetest.cpp \
	src/test_pv_author_engine_testset1.cpp \
	src/test_pv_author_engine_testset2.cpp \
	src/test_pv_author_engine_testset3.cpp \
	src/test_pv_author_engine_testset4.cpp \
	src/./single_core/pvaetestinput.cpp \
	src/test_pv_author_engine_testset5.cpp \
	src/test_pv_author_engine_testset6.cpp \
	src/test_pv_author_engine_testset7.cpp \
	src/test_pv_mediainput_author_engine.cpp


LOCAL_SHARED_LIBRARIES := libopencorecommon libopencoreauthor libopencoreplayer

LOCAL_STATIC_LIBRARIES := libunit_test libpvmiofileinput libpvavifileparser libpvwav libpvmioaviwavfileinput


LOCAL_MODULE := test_pvauthorengine

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//engines/author/test/src \
	$(PV_TOP)//engines/author/test/src \
	$(PV_TOP)//engines/author/test/../../common/include \
	$(PV_TOP)//engines/author/test/config/linux_nj \
	$(PV_TOP)//engines/author/test/src/single_core \
	$(PV_TOP)//engines/author/test/../../../pvmi/pvmf/include \
	$(PV_TOP)//engines/author/test/../../../nodes/common/include \
	$(PV_TOP)//engines/author/test/../../../pvmi/media_io/pvmi_mio_fileinput/include \
	$(PV_TOP)//engines/author/test/../../../pvmi/media_io/pvmi_mio_fileinput/src \
	$(PV_TOP)//engines/author/test/../../../pvmi/media_io/pvmi_mio_avi_wav_fileinput/include \
	$(PV_TOP)//engines/author/test/../../../fileformats/avi/parser/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

include $(BUILD_EXECUTABLE)

