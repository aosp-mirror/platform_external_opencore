LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/sample_player_app.cpp \
	src/pv_player_engine_testset_mio_file.cpp

LOCAL_STATIC_LIBRARIES := libunit_test

LOCAL_SHARED_LIBRARIES := libc libopencorecommon libopencoreplayer

LOCAL_LDLIBS += -lpthread



LOCAL_MODULE := pv_sample_player

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//engines/player/sample_app/src \
	$(PV_TOP)//engines/player/sample_app/src \
	$(PV_TOP)//engines/player/sample_app/src/linux_nj \
	$(PV_TOP)//engines/player/sample_app/../../common/include \
	$(PV_TOP)//engines/player/sample_app/../../../pvmi/media_io/pvmiofileoutput/include \
	$(PV_TOP)//engines/player/sample_app/../../../nodes/pvmediaoutputnode/include \
	$(PV_TOP)//engines/player/sample_app/../include \
	$(PV_TOP)//engines/player/sample_app/../../../nodes/common/include \
	$(PV_INCLUDES) 

ifeq ($(ARCHITECTURE),linux_nj)
   LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/sample_app/config/linux_nj
else
   LOCAL_C_INCLUDES += $(PV_TOP)//engines/player/sample_app/config/linux_3gpp
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

