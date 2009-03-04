LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvlogger.cpp \
	src/pvlogger_c.cpp



LOCAL_MODULE := libpvlogger

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//oscl/pvlogger/src \
	$(PV_TOP)//oscl/pvlogger/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	src/pvlogger.h \
	src/pvlogger_c.h \
	src/pvlogger_accessories.h \
	src/pvlogger_stderr_appender.h \
	src/pvlogger_file_appender.h \
	src/pvlogger_mem_appender.h \
	src/pvlogger_time_and_id_layout.h

include $(BUILD_STATIC_LIBRARY)

