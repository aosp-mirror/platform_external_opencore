LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/omx_m4v_component_interface.cpp



LOCAL_MODULE := libomx_m4v_component_interface

LOCAL_CFLAGS :=   $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/include \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/src \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/../../../../codecs_v2/omx/omx_m4v/include \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/../../../../codecs_v2/omx/omx_m4v/src \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/../../../../extern_libs_v2/khronos/openmax/include \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/../../../../codecs_v2/video/m4v_h263/dec/src \
	$(PV_TOP)//codecs_v2/omx/factories/omx_m4v_factory/../../../../codecs_v2/video/m4v_h263/dec/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/omx_m4v_component_interface.h

include $(BUILD_STATIC_LIBRARY)

