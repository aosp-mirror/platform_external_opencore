# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvjitterbuffernode

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

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if 
# possible.
# See notes above regarding overriding project defaults.
#
# XCPPFLAGS +=  

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# Added this to for osclconfig.h
XINCDIRS += -I ../../../common/include -I ../../../config/opencore -I ../../../streamingmanager/include -I ../../../../../protocols/rtp/src



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
SRCS = pvmf_jitter_buffer_extension_interface.cpp \
	pvmf_jitter_buffer_port.cpp \
	pvmf_jitter_buffer_impl.cpp \
	pvmf_rtcp_timer.cpp \
	pvmf_jitter_buffer_node.cpp \
	pvmf_jb_inactivity_timer.cpp

HDRS = pvmf_jitter_buffer_ext_interface.h \
	pvmf_jitter_buffer_internal.h \
	pvmf_jitter_buffer_port.h \
	pvmf_jitter_buffer.h \
	pvmf_jitter_buffer_node.h \
	pvmf_rtcp_timer.h \
	pvmf_jb_inactivity_timer.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

