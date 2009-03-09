LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/pvmfmp4nodes.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_m4v_component_lib \
	libpvmp4decoder \
	libpvmp4ffparsernode \

LOCAL_SHARED_LIBRARIES := liblog libopencoremp4reg libopencoreplayer libopencorecommon

LOCAL_MODULE := libopencoremp4

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/video/m4v_h263/dec/include \
	$(PV_TOP)/codecs_v2/video/m4v_h263/dec/src \
	$(PV_INCLUDES)

LOCAL_CFLAGS := $(PV_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

