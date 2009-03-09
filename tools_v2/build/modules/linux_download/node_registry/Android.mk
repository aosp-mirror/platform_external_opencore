LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/pvmfdownloadnodereg.cpp

LOCAL_SHARED_LIBRARIES := libopencoreplayer libopencorecommon

LOCAL_MODULE := libopencoredownloadreg

LOCAL_CFLAGS := $(PV_CFLAGS)

LOCAL_C_INCLUDES := \
	$(PV_TOP)/nodes/common/include \
	$(PV_TOP)/engines/player/config/linux_download/src \
	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
