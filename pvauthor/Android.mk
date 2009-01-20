ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
        libpvauthorengine \
        libpvm4vencoder \
        libpvencoder_gsmamr \
        libpvavch264enc \
        libpvmp4ffcomposer \
        libpvamrencnode \
        libpvmp4ffcomposernode \
        libpvomxvideoencnode \
        libpvavcencnode \
        libpvmediainputnode \
        libandroidpvauthor

LOCAL_LDLIBS := -lpthread

LOCAL_SHARED_LIBRARIES := libopencoreplayer libutils libcutils libui libhardware_legacy libandroid_runtime libdrm1 libmedia libsgl libopencorecommon 

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif

# Include board-specific extensions
LOCAL_SHARED_LIBRARIES += $(BOARD_OPENCORE_LIBRARIES)

LOCAL_MODULE := libopencoreauthor

include $(BUILD_SHARED_LIBRARY)
include $(PV_TOP)//engines/author/Android.mk
include $(PV_TOP)//codecs_v2/video/m4v_h263/enc/Android.mk
include $(PV_TOP)//codecs_v2/audio/gsm_amr/amr_nb/enc/Android.mk
include $(PV_TOP)//codecs_v2/video/avc_h264/enc/Android.mk
include $(PV_TOP)//fileformats/mp4/composer/Android.mk
include $(PV_TOP)//nodes/pvamrencnode/Android.mk
include $(PV_TOP)//nodes/pvmp4ffcomposernode/Android.mk
include $(PV_TOP)//nodes/pvvideoencnode/Android.mk
include $(PV_TOP)//nodes/pvomxvideoencnode/Android.mk
include $(PV_TOP)//nodes/pvavcencnode/Android.mk
include $(PV_TOP)//nodes/pvmediainputnode/Android.mk
#include $(PV_TOP)//pvmi/media_io/pvmi_mio_fileinput/Android.mk
#include $(PV_TOP)//pvmi/media_io/pvmi_mio_avi_wav_fileinput/Android.mk
#include $(PV_TOP)//fileformats/avi/parser/Android.mk
include $(PV_TOP)//android/author/Android.mk
endif
