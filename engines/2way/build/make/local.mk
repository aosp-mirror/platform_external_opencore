# Get the current local path as the first operation
LOCAL_PATH := $(call get_makefile_dir)

# Clear out the variables used in the local makefiles
include $(MK)/clear.mk

TARGET := pv2wayengine 



XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)
XCPPFLAGS +=  -DPV_DISABLE_VIDRECNODE -DPV_DISABLE_DEVSOUNDNODES -DPV_DISABLE_DEVVIDEOPLAYNODE

ifdef NO2WAYSIP
XCPPFLAGS += -DNO_2WAY_SIP
else
XCPPFLAGS += -DSIP_VOIP_PROJECT=1
endif

ifdef NO2WAY324
XCPPFLAGS += -DNO_2WAY_324
endif

ifeq ($(USING_OMX),1)
XCPPFLAGS += -DPV2WAY_USE_OMX_AMR_DECODER -DPV2WAY_USE_OMX_AMR_ENCODER \
-DPV2WAY_USE_OMX_H263_DECODER -DPV2WAY_USE_OMX_H263_ENCODER\
-DPV2WAY_USE_OMX_MPEG4_DECODER -DPV2WAY_USE_OMX_MPEG4_ENCODER
else
XCPPFLAGS += -DPV_USE_AMR_CODECS
endif 


XINCDIRS +=  ../../src  ../../include  ../../../common/include  \
../../../../common/pvdebug/src \
 ../../../../protocols/systems/3g-324m_pvterminal/h324/tsc/include \
 ../../../../protocols/systems/3g-324m_pvterminal/common/include \
 ../../../../protocols/systems/3g-324m_pvterminal/h245/cmn/include \
 ../../../../protocols/systems/3g-324m_pvterminal/h245/per/include \
 ../../../../protocols/systems/3g-324m_pvterminal/h245/se/include \
 ../../../../protocols/systems/3g-324m_pvterminal/h324/srp/include  \
../../../../protocols/systems/3g-324m_pvterminal/h324/tsc/include  \
../../../../protocols/systems/3g-324m_pvterminal/h223/include  \
../../../../protocols/systems/common/include  \
../../../../protocols/systems/tools/general/common/include  \
../../../../nodes/streaming/common/include \
../../../../nodes/pvmediainputnode/include 

ifeq ($(USING_OMX),1)
XINCDIRS +=  ../../../../extern_libs_v2/khronos/openmax/include \
  ../../../../nodes/pvomxvideodecnode/include \
  ../../../../nodes/pvomxbasedecnode/include \
  ../../../../nodes/pvomxaudiodecnode/include \
  ../../../../nodes/pvomxencnode/include
endif


SRCDIR := ../../src
INCSRCDIR := ../../include

SRCS := pv_2way_datapath.cpp \
	pv_2way_engine.cpp \
	pv_2way_data_channel_datapath.cpp \
	pv_2way_cmd_control_datapath.cpp \
	pv_2way_dec_data_channel_datapath.cpp \
	pv_2way_enc_data_channel_datapath.cpp \
	pv_2way_mux_datapath.cpp \
	pv_2way_preview_datapath.cpp \
	pv_2way_rec_datapath.cpp \
	pv_2way_engine_factory.cpp \
	pv_2way_proxy_adapter.cpp \
	pv_2way_proxy_factory.cpp

HDRS := pv_2way_interface.h \
	pv_2way_engine_factory.h \
	pv_2way_proxy_factory.h 



include $(MK)/library.mk

doc:
	cp -f $(VOB_BASE_DIR)/protocols/systems/3g-324m_pvterminal/h324/tsc/include/tsc_h324m_config_interface.h $(VOB_BASE_DIR)/engines/2way/include
	cp -f $(VOB_BASE_DIR)/nodes/common/include/pvmp4h263encextension.h $(VOB_BASE_DIR)/engines/2way/include
	perl $(VOB_BASE_DIR)/tools_v2/build/document/bin/doc_build.bat --doctype pv2way_engine --title "PV 2Way Engine" --path "$(VOB_BASE_DIR)/engines/2way/include $(VOB_BASE_DIR)/engines/common/include $(VOB_BASE_DIR)/protocols/systems/common/include" -filetype "mainpage *.h readme.txt" --exclude_pattern "*/test/* */obsolete/* */doxydir/* */cpp/* *symbian* *pv_2way_datapath* *pv_2way_engine_* *pv_2way_interface_* *pv_2way_plugin_interfaces* *pv_2way_sdk_types*" -ver $(PV2WAY_ENGINE_VERSION)
	rm -f $(VOB_BASE_DIR)/engines/2way/include/tsc_h324m_config_interface.h
	rm -f $(VOB_BASE_DIR)/engines/2way/include/pvmp4h263encextension.h


sdkinfo_target := $(LOCAL_SRCDIR)/pv_2way_engine.cpp
sdkinfo_header_filename := $(LOCAL_SRCDIR)/pv_2way_sdkinfo.h
sdkinfo_header_macro := PV_2WAY_SDKINFO
sdkinfo_label_macro := PV2WAY_ENGINE_SDKINFO_LABEL
sdkinfo_date_macro := PV2WAY_ENGINE_SDKINFO_DATE

include $(MK)/sdkinfo.mk 
