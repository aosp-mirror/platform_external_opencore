# Warning: This is just for the header installed in the common directory
# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = 

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
#XCXXFLAGS += -DTRUE=true -DFALSE=false
XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

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
SRCS =


HDRS =  pvmf_common_audio_decnode.h \
	pvmf_composer_size_and_duration.h \
	pvmf_data_source_init_extension.h \
	pvmf_data_source_playback_control.h \
	pvmf_data_source_direction_control.h \
	pvmf_download_data_source.h \
	pvmf_download_progress_interface.h \
	pvmf_ffparsernode_extension.h \
	pvmf_media_presentation_info.h \
	pvmf_meta_data_extension.h \
	pvmf_mp4_progdownload_support_extension.h \
	pvmf_node_shared_lib_interface.h \
	pvmf_node_registry.h \
	pvmf_nodes_sync_control.h \
	pvmf_track_level_info_extension.h \
	pvmf_track_selection_extension.h \
	pvmf_local_data_source.h \
	pvmf_source_context_data.h \
	pv_player_node_registry_interface.h \
	pvmf_streaming_data_source.h 
 
DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
install:: headers-install

 
