include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvomx_proxy_lib
XCPPFLAGS +=

XINCDIRS += \
	-I ../../../../../extern_libs_v2/khronos/openmax/include 

SRCDIR = ../../src
INCSRCDIR = ../../src

SRCS = 	omx_proxy_interface.cpp \
	pv_omx_interface_proxy.cpp \
	pv_omx_interface_proxy_handler.cpp \
	pv_omx_interface_proxy_notifier.cpp 


HDRS =	omx_proxy_interface.h \
	omx_proxy_vector.h \
	pv_omx_interface_proxy.h \
	pv_omx_interface_proxy_handler.h \
	pv_omx_interface_proxy_notifier.h \
	pv_omx_proxied_interface.h



DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

