LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_amrenc_node.cpp \
	src/pvmf_amrenc_port.cpp \
	src/pvmf_amrenc_data_processor.cpp \
	src/pvmf_amrenc_media_buffer.cpp \
	src/pvmf_amrenc_node_cap_config.cpp



LOCAL_MODULE := libpvamrencnode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvamrencnode/include \
	$(PV_TOP)//nodes/pvamrencnode/src \
	$(PV_TOP)//nodes/pvamrencnode/src \
	$(PV_TOP)//nodes/pvamrencnode/../../codecs_v2/audio/gsm_amr/amr_nb/enc/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmfamrencnode_extension.h \
	include/pvmf_amrenc_node_factory.h \
	include/pvmf_amrenc_node_types.h

include $(BUILD_STATIC_LIBRARY)

