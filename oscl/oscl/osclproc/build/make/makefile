# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = osclproc

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
#XINCDIRS += -I../../../osclbase/src -I../../../osclutil/src -I../../../osclmemory/src

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

SRCS =  oscl_execschedulerbase.cpp \
	oscl_scheduler.cpp \
	oscl_scheduler_ao.cpp \
	oscl_scheduler_threadcontext.cpp \
	oscl_double_list.cpp \
	oscl_timer.cpp \
	oscl_timerbase.cpp \
	oscl_mutex.cpp \
	oscl_semaphore.cpp \
	oscl_thread.cpp \
	oscl_init.cpp

HDRS = oscl_aostatus.h \
	oscl_aostatus.inl \
	oscl_scheduler.h \
	oscl_scheduler_aobase.h \
	oscl_scheduler_ao.h \
	oscl_scheduler_readyq.h \
	oscl_scheduler_threadcontext.h \
	oscl_scheduler_types.h \
	oscl_scheduler_tuneables.h \
	oscl_double_list.h \
	oscl_double_list.inl \
	oscl_timer.h \
	oscl_mutex.h \
	oscl_semaphore.h \
	oscl_thread.h \
	oscl_procstatus.h \
	oscl_init.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
