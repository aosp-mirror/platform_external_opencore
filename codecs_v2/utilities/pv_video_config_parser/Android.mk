LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pv_video_config_parser.cpp



LOCAL_MODULE := libpv_video_config_parser

LOCAL_CFLAGS :=   $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/utilities/pv_video_config_parser/include \
	$(PV_TOP)//codecs_v2/utilities/pv_video_config_parser/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pv_video_config_parser.h

include $(BUILD_STATIC_LIBRARY)

