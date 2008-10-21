LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)




LOCAL_CFLAGS :=   $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/audio/gsm_amr/common/dec/include \
	$(PV_TOP)//codecs_v2/audio/gsm_amr/common/dec/ \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvgsmamrdecoderinterface.h

include $(BUILD_COPY_HEADERS)

