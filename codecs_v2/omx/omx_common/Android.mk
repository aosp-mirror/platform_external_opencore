LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/\
	src/pv_omxcore.cpp \
	src/pv_omxregistry.cpp \
	src/pv_omxmastercore.cpp \
	src/qc_omxcore.cpp



LOCAL_MODULE := libomx_common_lib

LOCAL_CFLAGS :=   $(PV_CFLAGS)

# board-specific configuration
LOCAL_CFLAGS += $(BOARD_OPENCORE_FLAGS)

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(PV_TOP)//codecs_v2/omx/omx_common/include \
	$(PV_TOP)//codecs_v2/omx/omx_common/src \
	$(PV_TOP)//codecs_v2/omx/omx_common/../../../extern_libs_v2/khronos/openmax/include \
	$(PV_INCLUDES) 

ifeq ($(ARCHITECTURE), linux_nj)
   LOCAL_C_INCLUDES += $(PV_TOP)//codecs_v2/omx/omx_common/config/linux_nj
else
   ifeq ($(FORMAT), 3gpp)
      LOCAL_C_INCLUDES += $(PV_TOP)//codecs_v2/omx/omx_common/config/linux_3gpp
   else
      ifeq ($(FORMAT), nj)
         LOCAL_C_INCLUDES += $(PV_TOP)//codecs_v2/omx/omx_common/config/linux_nj
      else
         LOCAL_C_INCLUDES += $(PV_TOP)//codecs_v2/omx/omx_common/config/default
      endif
   endif
endif

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pv_omxmastercore.h \
	include/pv_omxcore.h \
	include/pv_omxdefs.h \
	include/pv_omx_shared_lib_interface.h \
	include/qc_omxcore.h \
	include/pv_omxwrapperbase.h

include $(BUILD_STATIC_LIBRARY)

