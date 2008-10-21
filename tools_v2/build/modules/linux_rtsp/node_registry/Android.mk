LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/pvmfrtspnodereg.cpp

LOCAL_SHARED_LIBRARIES := libopencoreplayer libopencorecommon

LOCAL_MODULE := libopencorertspreg

LOCAL_C_INCLUDES := \
	$(PV_TOP)/nodes/common/include \
	$(PV_TOP)/engines/player/config/linux_rtsp/src \
	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

