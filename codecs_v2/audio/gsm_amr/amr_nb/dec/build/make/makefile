#Include the project-wide makefile to get
# appropriate definitions

include $(PV_TOP)/makefile.pv


# BE CAREFUL NOT TO HAVE ANY TRAILING WHITESPACE FOR THE NEXT 3 LINES BELOW OR IT WILL NOT WORK!
WHAT = library
TARGET = pvdecoder_gsmamr
# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = "
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS = 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += -Wall -g -DUSE_UNICODE -D_POSIX_PTHREAD_SEMANTICS
XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if
# possible.
# See notes above regarding overriding project defaults.
#
#XCPPFLAGS += -DORIGINAL_TYPEDEF_H -DOSCL_BASE_H_INCLUDED -DOSCL_TYPES_H_INCLUDE -DOSCL_ASSERT_H_INCLUDED -DOSCL_MEM_BASIC_FUNCTIONS_H -DOSCL_MEM_IMP_H
XCPPFLAGS += 

# Use XINCDIRS for extra include directories. These should be proceeded
# with "-I" just as they would be when passing to the compiler.
#
# XINCDIRS +=
XINCDIRS += -I ../../../common/include

# Use XLIBDIRS for extra library directories. These should be proceeded
# with "-L" just as they would be when passing to the linker.
# Used for building executables.
#
XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS +=
SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  decoder_gsm_amr.cpp \
	a_refl.cpp \
	agc.cpp \
	amrdecode.cpp \
	b_cn_cod.cpp \
	bgnscd.cpp \
	c_g_aver.cpp \
	d1035pf.cpp \
	d2_11pf.cpp \
	d2_9pf.cpp \
	d3_14pf.cpp \
	d4_17pf.cpp \
	d8_31pf.cpp \
	d_gain_c.cpp \
	d_gain_p.cpp \
	d_plsf.cpp \
	d_plsf_3.cpp \
	d_plsf_5.cpp \
	dec_amr.cpp \
	dec_gain.cpp \
	dec_input_format_tab.cpp \
	dec_lag3.cpp \
	dec_lag6.cpp \
	dtx_dec.cpp \
	ec_gains.cpp \
	ex_ctrl.cpp \
	if2_to_ets.cpp \
	int_lsf.cpp \
	lsp_avg.cpp \
	ph_disp.cpp \
	post_pro.cpp \
	preemph.cpp \
	pstfilt.cpp \
	qgain475_tab.cpp \
	sp_dec.cpp \
	wmf_to_ets.cpp

LIBS =

HDRS = decoder_gsm_amr.h pvamrnbdecoder_api.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

