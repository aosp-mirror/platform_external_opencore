LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/\
	src/imp3ff.cpp \
	src/mp3fileio.cpp \
	src/mp3parser.cpp \
	src/mp3utils.cpp



LOCAL_MODULE := libpvmp3ff

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//fileformats/mp3/parser/include \
	$(PV_TOP)//fileformats/mp3/parser/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/imp3ff.h \
	include/mp3fileio.h \
	include/mp3parser.h \
	include/mp3utils.h

include $(BUILD_STATIC_LIBRARY)

