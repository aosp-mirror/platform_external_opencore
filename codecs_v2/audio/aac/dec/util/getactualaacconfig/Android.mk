LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/getactualaacconfig.cpp



LOCAL_MODULE := libgetactualaacconfig

LOCAL_CFLAGS := -DAAC_PLUS -DHQ_SBR -DPARAMETRICSTEREO -DC_EQUIVALENT -DAAC_UTILITIES  $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/audio/aac/dec/util/getactualaacconfig/include \
	$(PV_TOP)//codecs_v2/audio/aac/dec/util/getactualaacconfig/src \
	$(PV_TOP)//codecs_v2/audio/aac/dec/util/getactualaacconfig/../../include \
	$(PV_TOP)//codecs_v2/audio/aac/dec/util/getactualaacconfig/../../src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/getactualaacconfig.h

include $(BUILD_STATIC_LIBRARY)

