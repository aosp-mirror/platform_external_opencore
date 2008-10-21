LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/pvmfmp4nodereg.cpp

LOCAL_WHOLE_STATIC_LIBRARIES := libpvmp4ffrecognizer

LOCAL_SHARED_LIBRARIES := libopencoreplayer libopencorecommon

LOCAL_MODULE := libopencoremp4reg

LOCAL_C_INCLUDES := \
	$(PV_TOP)/nodes/common/include \
	$(PV_TOP)/nodes/pvmp4ffparsernode/include \
	$(PV_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

