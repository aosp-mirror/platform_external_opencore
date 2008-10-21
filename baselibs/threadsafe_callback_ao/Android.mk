LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/threadsafe_callback_ao.cpp \
	src/threadsafe_mempool.cpp



LOCAL_MODULE := libthreadsafe_callback_ao

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//baselibs/threadsafe_callback_ao/src \
	$(PV_TOP)//baselibs/threadsafe_callback_ao/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	src/threadsafe_callback_ao.h \
	src/threadsafe_mempool.h

include $(BUILD_STATIC_LIBRARY)

