# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvplayer_engine

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += -DUSE_UNICODE  -D_POSIX_PTHREAD_SEMANTICS   #-D_POSIX_SOURCE

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if 
# possible.
# See notes above regarding overriding project defaults.
#
#XCPPFLAGS += 

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# Added this to for osclconfig.h
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared

XINCDIRS += -I../../../common/include -I $(VOB_BASE_DIR)/baselibs/media_data_structures/src -I $(VOB_BASE_DIR)/nodes/pvaudioplayernode/include -I $(VOB_BASE_DIR)/codecs_v2/audio/aac_mpeg4/AAC_baseline/pv_aac_dec/cpp/include/ -I $(VOB_BASE_DIR)/codecs_v2/audio/aac_mpeg4/AAC_baseline/pv_aac_dec/c/include/ -I $(VOB_BASE_DIR)/codecs_v2/audio/mp3/thompson/include -I $(VOB_BASE_DIR)/nodes/pvmediaoutputnode/src -I $(VOB_BASE_DIR)/nodes/common/include -I $(VOB_BASE_DIR)/pvmi/pvmf/include -I $(VOB_BASE_DIR)/nodes/pvwmadecnode/include -I $(VOB_BASE_DIR)/nodes/pvwmvdecnode/include -I $(VOB_BASE_DIR)/nodes/pvasfffparsernode/include -I $(VOB_BASE_DIR)/codecs_v2/wmv_decoder/include -I $(VOB_BASE_DIR)/codecs_v2/wma_decoder/include -I $(VOB_BASE_DIR)/fileformats/asf/parser/include -I $(VOB_BASE_DIR)/pvmi/recognizer/include -I $(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvasfffrecognizer/include -I $(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvmp4ffrecognizer/include -I $(VOB_BASE_DIR)/pvmi/recognizer/plugins/pvmp3ffrecognizer/include

ifeq ($(ARCHITECTURE),linux_nj)
    XINCDIRS += -I ../../config/$(BUILD_ARCH)
else
	ifeq ($(FORMAT),nj)
    	XINCDIRS += -I ../../config/linux_nj
	else
    	XINCDIRS += -I ../../config/$(BUILD_ARCH)
	endif
endif


# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.
#
# XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS += 

SRCDIR = ../../src
INCSRCDIR = ../../include

# compose final src list for actual build
SRCS =	pv_player_datapath.cpp \
	pv_player_engine.cpp \
	pv_player_factory.cpp
#pv_player_datasink_factory.cpp


ifeq ($(FORMAT),nj)
    SRCS += ../config/linux_nj/pv_player_node_registry.cpp
else
    SRCS += ../config/$(BUILD_ARCH)/pv_player_node_registry.cpp
endif

HDRS =  pv_player_datasinkfilename.h \
	pv_player_datasource.h \
        pv_player_events.h \
	pv_player_factory.h \
	pv_player_datasink.h \
	pv_player_datasourcepvmfnode.h \
	pv_player_interface.h \
	pv_player_datasinkpvmfnode.h \
	pv_player_datasourceurl.h \
	pv_player_types.h \
	pv_player_license_acquisition_interface.h \
	pv_player_track_selection_interface.h \
	pv_player_registry_interface.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

doc:
	 perl $(VOB_BASE_DIR)/tools_v2/build/document/bin/doc_build.bat --doctype pvplayer_engine --title "PV Player Engine" --path "$(VOB_BASE_DIR)/engines/common/include $(VOB_BASE_DIR)/engines/player/include" --filetype "mainpage *.h readme.txt" --exclude_pattern "*/test/* */obsolete/* */doxydir/*" -ver $(PVPLAYER_ENGINE_VERSION)

