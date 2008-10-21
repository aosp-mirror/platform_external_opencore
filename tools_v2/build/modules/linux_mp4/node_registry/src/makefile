# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmp4reginterface

XINCDIRS += \
	-I $(VOB_BASE_DIR)/nodes/common/include \
	-I $(VOB_BASE_DIR)/nodes/pvmp4ffparsernode/include 

SRCDIR = .

# compose final src list for actual build
SRCS =	pvmfmp4nodereg.cpp

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template

