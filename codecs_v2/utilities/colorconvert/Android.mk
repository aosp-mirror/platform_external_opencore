LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/cczoomrotation12.cpp \
	src/cczoomrotation16.cpp \
	src/cczoomrotation24.cpp \
	src/cczoomrotation32.cpp \
	src/cczoomrotationbase.cpp \
	src/ccyuv422toyuv420.cpp



LOCAL_MODULE := libcolorconvert

LOCAL_CFLAGS := -DFALSE=false  $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/utilities/colorconvert/include \
	$(PV_TOP)//codecs_v2/utilities/colorconvert/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/cczoomrotation12.h \
	include/cczoomrotation16.h \
	include/cczoomrotation24.h \
	include/cczoomrotation32.h \
	include/cczoomrotationbase.h \
	include/ccyuv422toyuv420.h \
	include/colorconv_config.h

include $(BUILD_STATIC_LIBRARY)

