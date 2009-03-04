include $(PV_TOP)/makefile.pv

WHAT = module
TARGET = pvcommon

export FORMAT=nj
LIBDIRS = \
    $(VOB_BASE_DIR)/oscl/oscl/osclbase/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclerror/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclmemory/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclutil/build/make \
    $(VOB_BASE_DIR)/oscl/pvlogger/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclproc/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclio/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclregcli/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/osclregserv/build/make \
    $(VOB_BASE_DIR)/oscl/unit_test/build/make \
    $(VOB_BASE_DIR)/oscl/oscl/oscllib/build/make \
    $(VOB_BASE_DIR)/pvmi/pvmf/build/make \
    $(VOB_BASE_DIR)/baselibs/pv_mime_utils/build/make \
    $(VOB_BASE_DIR)/nodes/pvfileoutputnode/build/make \
    $(VOB_BASE_DIR)/baselibs/media_data_structures/build/make \
    $(VOB_BASE_DIR)/baselibs/threadsafe_callback_ao/build/make \
    $(VOB_BASE_DIR)/codecs_v2/utilities/colorconvert/build/make \
    $(VOB_BASE_DIR)/codecs_v2/audio/gsm_amr/amr_nb/common/build/make \
    $(VOB_BASE_DIR)/codecs_v2/video/avc_h264/common/build/make \


LIBS = -losclbase -losclerror -losclmemory -losclutil -lpvlogger -losclproc \
       -losclio -losclregcli -losclregserv -loscllib -lpvmf -lpvmimeutils \
       -lthreadsafe_callback_ao \
       -lpvfileoutputnode -lpvmediadatastruct -lcolorconvert \
       -lpv_amr_nb_common_lib -lpv_avc_common_lib

# use --allow-multiple-definition to get around aac decoder and
# getactualaacconfig building the same code
XLDFLAGS += -Wl,--allow-multiple-definition

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
