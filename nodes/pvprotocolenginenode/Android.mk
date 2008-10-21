LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvdl_config_file.cpp \
	src/pvmf_protocol_engine_common.cpp \
	src/pvmf_protocol_engine_download_common.cpp \
	src/pvmf_protocol_engine_progressive_download.cpp \
	src/pvmf_protocol_engine_node_common.cpp \
	src/pvmf_protocol_engine_node_download_common.cpp \
	src/pvmf_protocol_engine_node_progressive_download.cpp \
	src/pvmf_protocol_engine_node_progressive_streaming.cpp \
	src/pvmf_protocol_engine_factory.cpp \
	src/pvmf_protocol_engine_port.cpp



LOCAL_MODULE := libprotocolenginenode

LOCAL_CFLAGS := -DPV_PROTOCOL_ENGINE_NODE_PROGRESSIVE_DOWNLOAD_ENABLED -DPV_PROTOCOL_ENGINE_NODE_PROGRESSIVE_STREAMING_ENABLED  $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvprotocolenginenode/include \
	$(PV_TOP)//nodes/pvprotocolenginenode/src \
	$(PV_TOP)/protocols/http_paracom/include \
	$(PV_TOP)/nodes/common/include \
	$(PV_TOP)/baselibs/pv_mime_utils/src \
	$(PV_TOP)/protocols/sdp/common/include \
	$(PV_TOP)/protocols/sdp/parser/include \
	$(PV_TOP)/baselibs/gen_data_structures/src \
	$(PV_TOP)/baselibs/media_data_structures/src \
	$(PV_TOP)/fileformats/pvx/parser/include \
	$(PV_TOP)//nodes/pvprotocolenginenode/config/$(BUILD_ARCH) \
	$(PV_TOP)//nodes/pvprotocolenginenode/config/linux \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_protocol_engine_factory.h \
	include/pvmf_protocol_engine_defs.h \
	include/pvmf_protocol_engine_node_extension.h \
	include/pvmf_protocol_engine_command_format_ids.h \
	include/pvmf_protocol_engine_node_events.h

include $(BUILD_STATIC_LIBRARY)

