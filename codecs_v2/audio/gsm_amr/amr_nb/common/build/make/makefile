#Include the project-wide makefile to get
# appropriate definitions

include $(PV_TOP)/makefile.pv


# BE CAREFUL NOT TO HAVE ANY TRAILING WHITESPACE FOR THE NEXT 3 LINES BELOW OR IT WILL NOT WORK!
WHAT = library
TARGET = pv_amr_nb_common_lib
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
SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  add.cpp \
	az_lsp.cpp \
	bitno_tab.cpp \
	bitreorder_tab.cpp \
	bits2prm.cpp \
	bytesused.cpp \
	c2_9pf_tab.cpp \
	copy.cpp \
	div_32.cpp \
	div_s.cpp \
	extract_h.cpp \
	extract_l.cpp \
	gains_tbl.cpp \
	gc_pred.cpp \
	gmed_n.cpp \
	grid_tbl.cpp \
	gray_tbl.cpp \
	int_lpc.cpp \
	inv_sqrt.cpp \
	inv_sqrt_tbl.cpp \
	l_abs.cpp \
	l_deposit_h.cpp \
	l_deposit_l.cpp \
	l_shr_r.cpp \
	log2.cpp \
	log2_norm.cpp \
	log2_tbl.cpp \
	lsfwt.cpp \
	lsp.cpp \
	lsp_az.cpp \
	lsp_lsf.cpp \
	lsp_lsf_tbl.cpp \
	lsp_tab.cpp \
	mult_r.cpp \
	norm_l.cpp \
	norm_s.cpp \
	overflow_tbl.cpp \
	ph_disp_tab.cpp \
	pow2.cpp \
	pow2_tbl.cpp \
	pred_lt.cpp \
	q_plsf.cpp \
	q_plsf_3.cpp \
	q_plsf_3_tbl.cpp \
	q_plsf_5.cpp \
	q_plsf_5_tbl.cpp \
	qua_gain_tbl.cpp \
	reorder.cpp \
	residu.cpp \
	round.cpp \
	set_zero.cpp \
	shr.cpp \
	shr_r.cpp \
	sqrt_l.cpp \
	sqrt_l_tbl.cpp \
	sub.cpp \
	syn_filt.cpp \
	vad1.cpp \
	weight_a.cpp \
	window_tab.cpp

LIBS =

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

