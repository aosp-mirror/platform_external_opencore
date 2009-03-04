LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libprotocolenginenode \
	libpvsocketnode \
	libpv_http_parcom \
	libpvgendatastruct \
	libpvsdpparser

LOCAL_SHARED_LIBRARIES := libopencoreplayer libopencorecommon libutils libcutils

LOCAL_MODULE := libopencorenet_support

LOCAL_C_INCLUDES := \
	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

