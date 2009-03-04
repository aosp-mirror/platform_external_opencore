LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)




LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/common/include \
	$(PV_TOP)//pvmi/content_policy_manager/plugins/common/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pvmf_cpmplugin_access_interface.h\
	include/pvmf_cpmplugin_access_interface_factory.h\
	include/pvmf_cpmplugin_authentication_interface.h\
	include/pvmf_cpmplugin_authorization_interface.h\
	include/pvmf_cpmplugin_license_interface.h\
	include/pvmf_cpmplugin_license_interface_types.h\
	include/pvmf_cpmplugin_interface.h

include $(BUILD_COPY_HEADERS)

