LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/test_pv_frame_metadata_utility.cpp \
	src/test_pv_frame_metadata_utility_testset1.cpp


LOCAL_STATIC_LIBRARIES := libunit_test

LOCAL_SHARED_LIBRARIES := libc libopencorecommon libopencoreplayer


LOCAL_MODULE := pv_fmu_test

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/test/src \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/test/src \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/test/include \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/test/config/common \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

include $(BUILD_EXECUTABLE)

