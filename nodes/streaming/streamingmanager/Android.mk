LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_sm_node_factory.cpp \
	src/pvmf_streaming_manager_extension_interface.cpp \
	src/pvmf_streaming_manager_node.cpp \
	src/pvmf_streaming_manager_asf_nosupport.cpp \
	src/pvmf_streaming_manager_real_nosupport.cpp \
	src/pvmf_streaming_manager_cpm_support.cpp \
	src/../config/3gpp/streamingmanager_payloadparser_registry.cpp



LOCAL_MODULE := libpvstreamingmanagernode_3gpp

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/streaming/streamingmanager/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/src \
	$(PV_TOP)//nodes/streaming/streamingmanager/../common/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../config/opencore \
	$(PV_TOP)//nodes/streaming/streamingmanager/../jitterbuffernode/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../jitterbuffernode/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../medialayernode/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../../../protocols/rtp/src \
	$(PV_TOP)//nodes/streaming/streamingmanager/../../common/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../../../protocols/sdp/common/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../../../pvmi/content_policy_manager/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../../../pvmi/content_policy_manager/plugins/common/include \
	$(PV_TOP)//nodes/streaming/streamingmanager/../../../protocols/rtp_payload_parser/rfc_3640/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_sm_node_events.h \
	include/pvmf_sm_node_factory.h

include $(BUILD_STATIC_LIBRARY)

