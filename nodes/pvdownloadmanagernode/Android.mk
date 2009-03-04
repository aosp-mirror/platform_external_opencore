LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/\
	src/pvmf_downloadmanager_factory.cpp \
	src/pvmf_downloadmanager_node.cpp \
	src/pvmf_filebufferdatastream_factory.cpp \
	src/pvmf_memorybufferdatastream_factory.cpp



LOCAL_MODULE := libpvdownloadmanagernode

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//nodes/pvdownloadmanagernode/include \
	$(PV_TOP)//nodes/pvdownloadmanagernode/src \
	$(PV_TOP)//nodes/pvdownloadmanagernode/../common/include \
	$(PV_TOP)//nodes/pvdownloadmanagernode/../../pvmi/pvmf/include \
	$(PV_TOP)//nodes/pvdownloadmanagernode/../streaming/common/include \
	$(PV_INCLUDES) 

ifeq ($(ARCHITECTURE),linux_nj)
	LOCAL_C_INCLUDES += $(PV_TOP)//nodes/pvdownloadmanagernode/config/opencore
else
	ifeq ($(FORMAT), nj)
                LOCAL_C_INCLUDES += $(PV_TOP)//nodes/pvdownloadmanagernode/config/opencore
        else
                LOCAL_C_INCLUDES += $(PV_TOP)//nodes/pvdownloadmanagernode/config/default
        endif
endif

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_downloadmanager_defs.h \
	include/pvmf_downloadmanager_factory.h \
	include/pvmf_filebufferdatastream_factory.h \
	include/pvmf_memorybufferdatastream_factory.h

include $(BUILD_STATIC_LIBRARY)

