# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET =pvdownloadmanagernode

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
# Added this so that correct headers for player get included

XINCDIRS += \
	-I ../../../common/include \
	-I ../../../../pvmi/pvmf/include \
	-I ../../../streaming/common/include

#Select the config based on architecture.
ifeq ($(ARCHITECTURE),linux_nj)
	XINCDIRS += -I../../config/opencore
else
	ifeq ($(FORMAT), nj)
                XINCDIRS += -I../../config/opencore
        else
                XINCDIRS += -I../../config/default
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
SRCS = \
	pvmf_downloadmanager_factory.cpp \
	pvmf_downloadmanager_node.cpp \
	pvmf_filebufferdatastream_factory.cpp \
	pvmf_memorybufferdatastream_factory.cpp

HDRS = \
	pvmf_downloadmanager_defs.h \
	pvmf_downloadmanager_factory.h \
	pvmf_filebufferdatastream_factory.h \
	pvmf_memorybufferdatastream_factory.h


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
