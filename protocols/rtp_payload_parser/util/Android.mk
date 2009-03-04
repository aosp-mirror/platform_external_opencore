LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/latmpayloadparser.cpp



LOCAL_MODULE := libpvlatmpayloadparser

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//protocols/rtp_payload_parser/util/include \
	$(PV_TOP)//protocols/rtp_payload_parser/util/src \
	$(PV_TOP)//protocols/rtp_payload_parser/util/../../../codecs_v2/audio/aac/dec/include \
	$(PV_TOP)//protocols/rtp_payload_parser/util/../../../codecs_v2/audio/aac/dec/src \
	$(PV_TOP)//protocols/rtp_payload_parser/util/../../../baselibs/gen_data_structures/src \
	$(PV_TOP)//protocols/rtp_payload_parser/util/../../sdp/common/include \
	$(PV_TOP)//protocols/rtp_payload_parser/util/../../../pvmi/pvmf/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/latmpayloadparser.h

include $(BUILD_STATIC_LIBRARY)

