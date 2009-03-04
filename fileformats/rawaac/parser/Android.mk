LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/aacfileparser.cpp



LOCAL_MODULE := libpvaacparser

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//fileformats/rawaac/parser/include \
	$(PV_TOP)//fileformats/rawaac/parser/src \
	$(PV_TOP)/codecs_v2/audio/aac/dec/util/getactualaacconfig/include \
	$(PV_TOP)/codecs_v2/audio/aac/dec/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/aacfileparser.h

include $(BUILD_STATIC_LIBRARY)

