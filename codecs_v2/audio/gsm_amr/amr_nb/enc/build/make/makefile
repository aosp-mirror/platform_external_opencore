#Include the project-wide makefile to get
# appropriate definitions

include $(PV_TOP)/makefile.pv


# BE CAREFUL NOT TO HAVE ANY TRAILING WHITESPACE FOR THE NEXT 3 LINES BELOW OR IT WILL NOT WORK!
WHAT = library
TARGET = pvencoder_gsmamr
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

SRCS =  amrencode.cpp \
	autocorr.cpp \
	c1035pf.cpp \
	c2_11pf.cpp \
	c2_9pf.cpp \
	c3_14pf.cpp \
	c4_17pf.cpp \
	c8_31pf.cpp \
	calc_cor.cpp \
	calc_en.cpp \
	cbsearch.cpp \
	cl_ltp.cpp \
	cod_amr.cpp \
	convolve.cpp \
	cor_h.cpp \
	cor_h_x.cpp \
	cor_h_x2.cpp \
	corrwght_tab.cpp \
	dtx_enc.cpp \
	enc_lag3.cpp \
	enc_lag6.cpp \
	enc_output_format_tab.cpp \
	ets_to_if2.cpp \
	ets_to_wmf.cpp \
	g_adapt.cpp \
	g_code.cpp \
	g_pitch.cpp \
	gain_q.cpp \
	hp_max.cpp \
	inter_36.cpp \
	inter_36_tab.cpp \
	l_comp.cpp \
	l_extract.cpp \
	l_negate.cpp \
	lag_wind.cpp \
	lag_wind_tab.cpp \
	levinson.cpp \
	lpc.cpp \
	ol_ltp.cpp \
	p_ol_wgh.cpp \
	pitch_fr.cpp \
	pitch_ol.cpp \
	pre_big.cpp \
	pre_proc.cpp \
	prm2bits.cpp \
	q_gain_c.cpp \
	q_gain_p.cpp \
	qgain475.cpp \
	qgain795.cpp \
	qua_gain.cpp \
	s10_8pf.cpp \
	set_sign.cpp \
	sid_sync.cpp \
	sp_enc.cpp \
	spreproc.cpp \
	spstproc.cpp \
	ton_stab.cpp \
	gsmamr_encoder_wrapper.cpp 

HDRS = gsmamr_encoder_wrapper.h

LIBS =

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

