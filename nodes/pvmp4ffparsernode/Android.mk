LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_mp4ffparser_node.cpp \
	src/pvmf_mp4ffparser_factory.cpp \
	src/pvmf_mp4ffparser_outport.cpp \
	src/pvmf_mp4ffparser_node_metadata.cpp \
	src/pvmf_mp4ffparser_node_cap_config.cpp



LOCAL_MODULE := libpvmp4ffparsernode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)/fileformats/mp4/parser/include \
	$(PV_TOP)//nodes/pvmp4ffparsernode/include \
	$(PV_TOP)//nodes/pvmp4ffparsernode/src \
	$(PV_TOP)//nodes/pvmp4ffparsernode/src/default \
	$(PV_TOP)//nodes/pvmp4ffparsernode/../../fileformats/mp4/parser/include/ \
	$(PV_TOP)//nodes/pvmp4ffparsernode/../../fileformats/mp4/parser/config/opencore \
	$(PV_TOP)//nodes/pvmp4ffparsernode/../common/include \
	$(PV_TOP)//nodes/pvmp4ffparsernode/../../pvmi/pvmf/include \
	$(PV_TOP)//nodes/pvmp4ffparsernode/../../codecs_v2/utilities/m4v_config_parser/include \
	$(PV_TOP)//nodes/pvmp4ffparsernode/../../codecs_v2/audio/aac/dec/util/getactualaacconfig/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_mp4ffparser_factory.h \
	include/pvmf_mp4ffparser_events.h

include $(BUILD_STATIC_LIBRARY)

