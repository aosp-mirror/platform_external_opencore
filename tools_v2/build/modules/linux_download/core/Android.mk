LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/pvmfdownloadnodes.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := libpvdownloadmanagernode

LOCAL_SHARED_LIBRARIES := libopencorenet_support libopencoreplayer libopencorecommon

LOCAL_MODULE := libopencoredownload

LOCAL_C_INCLUDES := \
	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)
