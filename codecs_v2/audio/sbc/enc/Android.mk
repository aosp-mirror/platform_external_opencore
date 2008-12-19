LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvsbcencoder.cpp \
	src/pvsbcencoder_factory.cpp \
	src/sbc_encoder.cpp \
	src/sbcenc_allocation.cpp \
	src/sbcenc_bitstream.cpp \
	src/sbcenc_crc8.cpp \
	src/sbcenc_filter.cpp \
	src/scalefactors.cpp


LOCAL_MODULE := libsbcencoder

LOCAL_CFLAGS := $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/audio/sbc/enc/include \
	$(PV_INCLUDES) 

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvsbcencoderinterface.h \
	include/pvsbcencoder_factory.h

include $(BUILD_STATIC_LIBRARY)
