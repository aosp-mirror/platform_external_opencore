# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvrtspinterface

SRCDIR = .

# compose final src list for actual build
SRCS =	pvmfrtspnodes.cpp

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

