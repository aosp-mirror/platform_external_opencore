ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libosclbase \
	libosclerror \
	libosclmemory \
	libosclutil \
	libpvlogger \
	libosclproc \
	libosclio \
	libosclregcli \
	libosclregserv \
	liboscllib \
	libpvmf \
	libpvmimeutils \
	libpvfileoutputnode \
	libpvmediadatastruct \
	libthreadsafe_callback_ao \
	libcolorconvert \
	libpv_amr_nb_common_lib \
	libpv_avc_common_lib 

LOCAL_LDLIBS := -lpthread

LOCAL_SHARED_LIBRARIES := libutils libcutils libhardware libandroid_runtime
ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

# Include board-specific extensions
LOCAL_SHARED_LIBRARIES += $(BOARD_OPENCORE_LIBRARIES)


LOCAL_MODULE := libopencorecommon

include $(BUILD_SHARED_LIBRARY)
include $(PV_TOP)//oscl/oscl/osclbase/Android.mk
include $(PV_TOP)//oscl/oscl/osclerror/Android.mk
include $(PV_TOP)//oscl/oscl/osclmemory/Android.mk
include $(PV_TOP)//oscl/oscl/osclutil/Android.mk
include $(PV_TOP)//oscl/pvlogger/Android.mk
include $(PV_TOP)//oscl/oscl/osclproc/Android.mk
include $(PV_TOP)//oscl/oscl/osclio/Android.mk
include $(PV_TOP)//oscl/oscl/osclregcli/Android.mk
include $(PV_TOP)//oscl/oscl/osclregserv/Android.mk
include $(PV_TOP)//oscl/unit_test/Android.mk
include $(PV_TOP)//oscl/oscl/oscllib/Android.mk
include $(PV_TOP)//pvmi/pvmf/Android.mk
include $(PV_TOP)//baselibs/pv_mime_utils/Android.mk
include $(PV_TOP)//nodes/pvfileoutputnode/Android.mk
include $(PV_TOP)//baselibs/media_data_structures/Android.mk
include $(PV_TOP)//baselibs/threadsafe_callback_ao/Android.mk
include $(PV_TOP)//codecs_v2/utilities/colorconvert/Android.mk
include $(PV_TOP)//codecs_v2/audio/gsm_amr/amr_nb/common/Android.mk
include $(PV_TOP)//codecs_v2/video/avc_h264/common/Android.mk
endif
