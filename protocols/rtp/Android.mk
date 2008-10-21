LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/rtcp.cpp \
	src/rtcp_decoder.cpp \
	src/rtcp_encoder.cpp \
	src/rtp_decode.cpp \
	src/rtp_encode.cpp \
	src/rtp_packet.cpp \
	src/rtp_packet_impl.cpp \
	src/rtp_payload.cpp



LOCAL_MODULE := librtprtcp

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//protocols/rtp/src \
	$(PV_TOP)//protocols/rtp/src \
	$(PV_TOP)//protocols/rtp/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	src/rtprtcp.h \
	src/rtcp.h \
	src/rtcp_decoder.h \
	src/rtcp_encoder.h \
	src/rtcp_constants.h \
	src/rtp_decode.h \
	src/rtp_encode.h \
	src/rtp.h \
	src/rtp_packet.h \
	src/rtp_packet_impl.h \
	src/rtp_payload.h

include $(BUILD_STATIC_LIBRARY)

