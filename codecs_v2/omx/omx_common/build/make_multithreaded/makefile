include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = omx_common_lib

XINCDIRS += \
	-I ../../../../../extern_libs_v2/khronos/openmax/include 

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS = 	\
	pv_omxcore.cpp \
	pv_omxregistry.cpp \
	pv_omxmastercore.cpp \
	qc_omxcore.cpp

ifeq ($(ARCHITECTURE), linux_nj)
   XINCDIRS += -I ../../config/linux_nj
else
   ifeq ($(FORMAT), 3gpp)
      XINCDIRS += -I ../../config/linux_3gpp
   else
      ifeq ($(FORMAT), nj)
         XINCDIRS += -I ../../config/linux_nj
      else
         XINCDIRS += -I ../../config/default
      endif
   endif
endif

HDRS =	\
        pv_omxmastercore.h \
	pv_omxcore.h \
	pv_omxdefs.h \
	pv_omx_shared_lib_interface.h \
	qc_omxcore.h \
 	pv_omxwrapperbase.h
	

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

