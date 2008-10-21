# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvauthorengine

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
#XCPPFLAGS += -DUSE_PVVIDEODISPLAYNODE 

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#
# Added this to for osclconfig.h
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared

XINCDIRS += -I../../../common/include -I../../src/single_core

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
SRCS =	pvaenodeutility.cpp pvauthorengine.cpp

HDRS =  pvauthorenginefactory.h pvauthorengineinterface.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

doc: 
	perl $(VOB_BASE_DIR)/tools_v2/build/document/bin/doc_build.bat --doctype pvauthor_engine --title "PV Author Engine" --path "$(VOB_BASE_DIR)/engines/common/include $(VOB_BASE_DIR)/engines/author/include" --filetype "mainpage *.h readme.txt" --exclude_pattern "*/test/* */obsolete/* */doxydir/*" -ver $(PVAUTHOR_ENGINE_VERSION)
