# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = osclbase

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
XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

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
INCSRCDIR = ../../src

# compose final src list for actual build
SRCS =	oscl_time.cpp \
	oscl_assert.cpp \
	oscl_byte_order.cpp \
	oscl_tree.cpp \
	oscl_tagtree.cpp \
	oscl_stdstring.cpp \
	oscl_string_utils.cpp \
	oscl_int64_utils.cpp \
	oscl_base.cpp \
	oscl_tls.cpp \
	oscl_mem_basic_functions.cpp \
	oscl_singleton.cpp \
	oscl_vector.cpp \
	oscl_queue.cpp \
	oscl_linked_list.cpp
#\
#oscl_mem_basic_functions.cpp


HDRS =	oscl_assert.h \
	oscl_base.h \
	oscl_base_alloc.h \
	oscl_base_macros.h \
	oscl_byte_order.h \
	oscl_defalloc.h \
	oscl_dll.h \
	oscl_exclusive_ptr.h \
	oscl_int64_utils.h \
	oscl_mem_inst.h \
	oscl_mem_basic_functions.h \
	oscl_mem_basic_functions.inl \
	oscl_linked_list.h \
	oscl_lock_base.h \
	oscl_map.h \
	oscl_opaque_type.h \
	oscl_queue.h \
	oscl_refcounter.h \
	oscl_refcounter_memfrag.h \
	oscl_shared_ptr.h \
	oscl_stdstring.h \
	oscl_string_utils.h \
	oscl_tagtree.h \
	oscl_time.h \
	oscl_tls.h \
	oscl_tree.h \
	oscl_types.h \
	oscl_uuid.h \
	oscl_vector.h \
	oscl_singleton.h \
	oscl_assert.inl \
	oscl_byte_order.inl \
	oscl_time.inl 

#\
#	oscl_mem_basic_functions.h
#\
#	oscl_mem_basic_functions.inl


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
