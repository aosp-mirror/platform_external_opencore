LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_jitter_buffer_extension_interface.cpp \
 	src/pvmf_jitter_buffer_port.cpp \
 	src/pvmf_jitter_buffer_impl.cpp \
 	src/pvmf_rtcp_timer.cpp \
 	src/pvmf_jitter_buffer_node.cpp \
 	src/pvmf_jb_inactivity_timer.cpp


LOCAL_MODULE := libpvjitterbuffernode

LOCAL_CFLAGS :=  $(PV_CFLAGS)



LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/nodes/streaming/jitterbuffernode/src \
 	$(PV_TOP)/nodes/streaming/jitterbuffernode/include \
 	$(PV_TOP)/nodes/streaming/common/include \
 	$(PV_TOP)/nodes/streaming/streamingmanager/plugins/mshttp/config \
 	$(PV_TOP)/nodes/streaming/streamingmanager/include \
 	$(PV_TOP)/protocols/rtp/src \
 	$(PV_TOP)/nodes/streaming/streamingmanager/config/3gpp \
 	$(PV_INCLUDES)

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \
	include/pvmf_jitter_buffer_ext_interface.h \
 	include/pvmf_jitter_buffer_internal.h \
 	include/pvmf_jitter_buffer_port.h \
 	include/pvmf_jitter_buffer.h \
 	include/pvmf_jitter_buffer_node.h \
 	include/pvmf_rtcp_timer.h \
 	include/pvmf_jb_inactivity_timer.h

include $(BUILD_STATIC_LIBRARY)
