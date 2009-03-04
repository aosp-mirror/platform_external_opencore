# Warning: This is just for the header installed in the common directory
# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = 



SRCDIR = ../../src
INCSRCDIR = ../../include

# compose final src list for actual build
SRCS =


HDRS =  \
	pv_common_types.h \
	pv_config_interface.h \
	pv_engine_observer.h \
	pv_engine_observer_message.h \
	pv_engine_types.h \
	pv_interface_cmd_message.h



DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
install:: headers-install

 
