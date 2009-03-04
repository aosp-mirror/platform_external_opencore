#Include the project-wide makefile to get
# appropriate definitions

include $(PV_TOP)/makefile.pv


# BE CAREFUL NOT TO HAVE ANY TRAILING WHITESPACE FOR THE NEXT 3 LINES BELOW OR IT WILL NOT WORK!
WHAT = library
TARGET = pvamrwbdecoder
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

# Use XLIBDIRS for extra library directories. These should be proceeded
# with "-L" just as they would be when passing to the linker.
# Used for building executables.
#
XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS +=

ifeq ($(ARCHITECTURE),linux_arm)
XCPPFLAGS += -D_ARM_GCC
else
XCPPFLAGS += -DC_EQUIVALENT
endif

SRCDIR = ../../src
INCSRCDIR = ../../include
SRCS =  agc2_amr_wb.cpp \
	band_pass_6k_7k.cpp \
	dec_acelp_2p_in_64.cpp \
	dec_acelp_4p_in_64.cpp \
	dec_alg_codebook.cpp \
	dec_gain2_amr_wb.cpp \
	decoder_amr_wb.cpp \
	deemphasis_32.cpp \
	dtx_decoder_amr_wb.cpp \
	get_amr_wb_bits.cpp \
	highpass_400hz_at_12k8.cpp \
	highpass_50hz_at_12k8.cpp \
	homing_amr_wb_dec.cpp \
	interpolate_isp.cpp \
	isf_extrapolation.cpp \
	isp_az.cpp \
	isp_isf.cpp \
	lagconceal.cpp \
	low_pass_filt_7k.cpp \
	median5.cpp \
	mime_io.cpp \
	noise_gen_amrwb.cpp \
	normalize_amr_wb.cpp \
	oversamp_12k8_to_16k.cpp \
	phase_dispersion.cpp \
	pit_shrp.cpp \
	pred_lt4.cpp \
	preemph_amrwb_dec.cpp \
	pvamrwb_math_op.cpp \
	pvamrwbdecoder.cpp \
	q_gain2_tab.cpp \
	qisf_ns.cpp \
	qisf_ns_tab.cpp \
	qpisf_2s.cpp \
	qpisf_2s_tab.cpp \
	scale_signal.cpp \
	synthesis_amr_wb.cpp \
	voice_factor.cpp \
	wb_syn_filt.cpp \
	weight_amrwb_lpc.cpp
	
HDRS = decoder_amr_wb.h pvamrwbdecoder_api.h

LIBS =

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

