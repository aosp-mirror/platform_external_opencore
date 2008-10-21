LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/pv_frame_metadata_utility.cpp \
	src/pv_frame_metadata_factory.cpp \
	src/pv_frame_metadata_mio_video.cpp \
	src/pv_frame_metadata_mio_audio.cpp \
	src/../config/common/pv_frame_metadata_mio_video_config.cpp



LOCAL_MODULE := libpvframemetadatautility

LOCAL_CFLAGS :=   $(PV_CFLAGS)


LOCAL_C_INCLUDES := \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/include \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/src \
	$(PV_TOP)/engines/common/include \
	$(PV_TOP)/engines/player/include \
	$(PV_TOP)//engines/adapters/player/framemetadatautility/config/linux \
	$(PV_TOP)/nodes/common/include \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/baselibs/pv_mime_utils/src \
	$(PV_TOP)/nodes/pvmediaoutputnode/include \
	$(PV_TOP)/codecs_v2/utilities/colorconvert/include \
	$(PV_TOP)/codecs_v2/utilities/colorconvert/src \
	$(PV_INCLUDES) 


LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := \
	include/pv_frame_metadata_factory.h \
	include/pv_frame_metadata_interface.h

include $(BUILD_STATIC_LIBRARY)

