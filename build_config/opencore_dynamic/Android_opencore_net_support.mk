LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libpvsocketnode \
 	libpv_http_parcom

LOCAL_MODULE := libopencore_net_support

-include $(PV_TOP)/Android_platform_extras.mk

-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libopencore_common

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/nodes/pvsocketnode/Android.mk
include   $(PV_TOP)/protocols/http_parcom/Android.mk

