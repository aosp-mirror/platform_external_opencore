LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_avcenc_node.cpp \
	src/pvmf_avcenc_port.cpp \
	src/pvmf_avcenc_node_cap_config.cpp



LOCAL_MODULE := libpvavcencnode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvavcencnode/include \
	$(PV_TOP)//nodes/pvavcencnode/src \
	$(PV_TOP)/$(SDK_LOCAL)/installed_include \
	$(PV_TOP)//nodes/pvavcencnode/src \
	$(PV_TOP)//nodes/pvavcencnode/../../pvmi/pvmf/include \
	$(PV_TOP)//nodes/pvavcencnode/include \
	$(PV_TOP)//nodes/pvavcencnode/../../baselibs/pv_mime_utils/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_avcenc_node_factory.h \
	include/pvmf_avcenc_node_types.h

include $(BUILD_STATIC_LIBRARY)

