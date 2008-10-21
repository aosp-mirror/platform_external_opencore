LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/audiogetid3info.cpp \
	src/pvfile.cpp \
	src/pvmi_datastreamsyncinterface_ref_factory.cpp \
	src/pvmi_datastreamsyncinterface_ref_impl.cpp



LOCAL_MODULE := libpvfileparserutils

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//fileformats/common/parser/include \
	$(PV_TOP)//fileformats/common/parser/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvfile.h \
	include/audiometadata.h \
	include/audiogetid3info.h \
	include/pvmi_datastreamsyncinterface_ref_factory.h \
	include/virtual_buffer.h

include $(BUILD_STATIC_LIBRARY)

