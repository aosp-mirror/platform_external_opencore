# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = osclutil

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.  # See notes above regarding overriding project defaults.
#
#XCXXFLAGS += -DUSE_UNICODE  -D_POSIX_PTHREAD_SEMANTICS   #-D_POSIX_SOURCE
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
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared
#XINCDIRS += -I../../../osclmemory/src

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
INCSRCDIR = ../../src


SRCS =  oscl_bin_stream.cpp \
	oscl_string.cpp \
	oscl_string_containers.cpp \
	oscl_str_ptr_len.cpp \
	oscl_string_utf8.cpp \
	oscl_tickcount.cpp \
	oscl_utf8conv.cpp \
	oscl_snprintf.cpp \
	oscl_clock.cpp \
	oscl_math.cpp \
	oscl_rand.cpp \
	oscl_string_uri.cpp \
	oscl_string_xml.cpp \
	oscl_priqueue.cpp


HDRS =  oscl_bin_stream.h \
	oscl_bin_stream.inl \
	oscl_media_data.h \
	oscl_media_status.h \
	oscl_priqueue.h \
	oscl_rand.h \
	oscl_rand.inl \
	oscl_snprintf.h \
	oscl_string.h \
	oscl_string_containers.h \
	oscl_string_rep.h \
	oscl_string_utf8.h \
	oscl_str_ptr_len.h \
	oscl_tickcount.h \
	oscl_tickcount.inl \
	oscl_utf8conv.h \
	oscl_clock.h \
	oscl_math.h \
	oscl_math.inl \
	oscl_string_uri.h \
	oscl_string_xml.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template


