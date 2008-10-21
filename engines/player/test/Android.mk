LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/test_pv_player_engine.cpp \
	src/test_pv_player_engine_testset_mio_file.cpp \
	src/test_pv_player_engine_testset1.cpp \
	src/test_pv_player_engine_testset5.cpp \
	src/test_pv_player_engine_testset6.cpp \
	src/test_pv_player_engine_testset7.cpp \
	src/test_pv_player_engine_testset8.cpp \
	src/test_pv_player_engine_testset9.cpp \
	src/test_pv_player_engine_testset10.cpp \
	src/test_pv_player_engine_testset11.cpp \
	src/test_pv_player_engine_testset12.cpp \
	src/test_pv_player_engine_testset13.cpp \
	src/test_pv_player_engine_testset_cpmdlapassthru.cpp


LOCAL_SHARED_LIBRARIES := libopencorecommon libopencoreplayer

LOCAL_STATIC_LIBRARIES := libunit_test


LOCAL_MODULE := pvplayer_engine_test

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//engines/player/test/src \
	$(PV_TOP)//engines/player/test/src \
	$(PV_TOP)//engines/player/test/../../common/include \
	$(PV_TOP)//engines/player/test/../../../pvmi/media_io/pvmiofileoutput/include \
	$(PV_TOP)//engines/player/test/../../../nodes/pvmediaoutputnode/include \
	$(PV_TOP)//engines/player/test/../include \
	$(PV_TOP)//engines/player/test/../../../nodes/common/include \
	$(PV_TOP)//engines/player/test/../../../extern_libs_v2/khronos/openmax/include \
	$(PV_INCLUDES) 

ifeq ($(TARGET_ARCH),arm)
   LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/test/config/linux_nj
else
   LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/test/config/linux_nj
endif

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

include $(BUILD_EXECUTABLE)

