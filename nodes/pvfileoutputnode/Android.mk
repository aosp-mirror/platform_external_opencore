LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_fileoutput_inport.cpp \
	src/pvmf_fileoutput_node.cpp \
	src/pvmf_fileoutput_factory.cpp \
	src/pvmf_fileoutput_node_cap_config.cpp



LOCAL_MODULE := libpvfileoutputnode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvfileoutputnode/include \
	$(PV_TOP)//nodes/pvfileoutputnode/src \
	$(PV_TOP)//nodes/pvfileoutputnode/include \
	$(PV_TOP)//nodes/pvfileoutputnode/../../pvmi/pvmf/include \
	$(PV_TOP)//nodes/pvfileoutputnode/../common/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_fileoutput_config.h\
	include/pvmf_fileoutput_factory.h

include $(BUILD_STATIC_LIBRARY)

