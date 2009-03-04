LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvrtsp_client_engine_node.cpp\
	src/pvrtsp_client_engine_utils.cpp\
	src/pvrtsp_client_engine_factory.cpp\
	src/pvrtsp_client_engine_port.cpp



LOCAL_MODULE := libpvrtsp_cli_eng_node_3gpp

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//protocols/rtsp_client_engine/inc \
	$(PV_TOP)//protocols/rtsp_client_engine/src \
	$(PV_TOP)//protocols/rtsp_client_engine/../../nodes/streaming/config/opencore \
	$(PV_TOP)//protocols/rtsp_client_engine/../rtsp_parcom/src \
	$(PV_TOP)//protocols/rtsp_client_engine/../rtp/src \
	$(PV_TOP)//protocols/rtsp_client_engine/../../baselibs/gen_data_structures/src \
	$(PV_TOP)//protocols/rtsp_client_engine/../sdp/common/include \
	$(PV_TOP)//protocols/rtsp_client_engine/../../nodes/streaming/streamingmanager/include \
	$(PV_TOP)//protocols/rtsp_client_engine/../../baselibs/pvcrypto/src \
	$(PV_TOP)//protocols/rtsp_client_engine/../rdt_parser/include \
	$(PV_TOP)//protocols/rtsp_client_engine/../../nodes/streaming/common/include \
	$(PV_TOP)//protocols/rtsp_client_engine/../../engines/player/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	inc/pvrtsp_client_engine_node.h\
	inc/pvrtspenginenodeextensioninterface.h\
	inc/pvrtsp_client_engine_utils.h\
	inc/pvrtsp_client_engine_factory.h\
	inc/pvrtsp_client_engine_error_code.h

include $(BUILD_STATIC_LIBRARY)

