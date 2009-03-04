LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pvmf_cpmplugin_passthru_oma1.cpp



LOCAL_MODULE := libpassthru_oma1

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/oma1/passthru/include \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/oma1/passthru/src \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/oma1/passthru/include \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/oma1/passthru/../../common/include \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/oma1/passthru/../../../include \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/oma1/passthru/../../../../../nodes/common/include \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_cpmplugin_passthru_oma1.h \
	include/pvmf_cpmplugin_passthru_oma1_types.h \
	include/pvmf_cpmplugin_passthru_oma1_factory.h

include $(BUILD_STATIC_LIBRARY)

