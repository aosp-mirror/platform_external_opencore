LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_omx_videodec_factory.cpp \
	src/pvmf_omx_videodec_node.cpp \
	src/pvmf_omx_videodec_port.cpp \
	src/pvmf_omx_videodec_callbacks.cpp



LOCAL_MODULE := libpvomxvideodecnode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvomxvideodecnode/include \
	$(PV_TOP)//nodes/pvomxvideodecnode/src \
	$(PV_TOP)//nodes/pvomxvideodecnode/../../extern_libs_v2/khronos/openmax/include \
	$(PV_TOP)//nodes/pvomxvideodecnode/../../codecs_v2/video/wmv_vc1/dec/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_omx_videodec_defs.h \
	include/pvmf_omx_videodec_factory.h \
	include/pvmf_omx_videodec_port.h

include $(BUILD_STATIC_LIBRARY)

