LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_omx_audiodec_factory.cpp \
	src/pvmf_omx_audiodec_node.cpp \
	src/pvmf_omx_audiodec_port.cpp \
	src/pvmf_omx_audiodec_callbacks.cpp



LOCAL_MODULE := libpvomxaudiodecnode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvomxaudiodecnode/include \
	$(PV_TOP)//nodes/pvomxaudiodecnode/src \
	$(PV_TOP)//nodes/pvomxaudiodecnode/../../extern_libs_v2/khronos/openmax/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_omx_audiodec_defs.h \
	include/pvmf_omx_audiodec_factory.h \
	include/pvmf_omx_audiodec_port.h

include $(BUILD_STATIC_LIBRARY)

