#Include the project-wide makefile to get
# appropriate definitions

include $(PV_TOP)/makefile.pv


# BE CAREFUL NOT TO HAVE ANY TRAILING WHITESPACE FOR THE NEXT 3 LINES BELOW OR IT WILL NOT WORK!
WHAT = library
TARGET = pvmp3
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

ifeq ($(ARCHITECTURE),linux_arm)
XCPPFLAGS += -DPV_ARM_GCC
else
XCPPFLAGS += -DC_EQUIVALENT
endif

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


SRCS =  pvmp3_normalize.cpp \
	pvmp3_alias_reduction.cpp \
	pvmp3_crc.cpp \
	pvmp3_decode_header.cpp \
	pvmp3_decode_huff_cw.cpp \
	pvmp3_decoder.cpp \
	pvmp3_getbits.cpp \
	pvmp3_dequantize_sample.cpp \
	pvmp3_framedecoder.cpp \
	pvmp3_get_main_data_size.cpp \
	pvmp3_get_side_info.cpp \
	pvmp3_get_scale_factors.cpp \
	pvmp3_mpeg2_get_scale_data.cpp \
	pvmp3_mpeg2_get_scale_factors.cpp \
	pvmp3_mpeg2_stereo_proc.cpp \
	pvmp3_huffman_decoding.cpp \
	pvmp3_huffman_parsing.cpp \
	pvmp3_tables.cpp \
	pvmp3_imdct_synth.cpp \
	pvmp3_mdct_6.cpp \
	pvmp3_dct_6.cpp \
	pvmp3_poly_phase_synthesis.cpp \
	pvmp3_equalizer.cpp \
	pvmp3_seek_synch.cpp \
	pvmp3_stereo_proc.cpp \
	pvmp3_reorder.cpp 


ifeq ($(ARCHITECTURE),linux_arm)
SRCS+=  asm/pvmp3_polyphase_filter_window_gcc.s \
        asm/pvmp3_mdct_18_gcc.s \
        asm/pvmp3_dct_9_gcc.s \
        asm/pvmp3_dct_16_gcc.s 
else
SRCS+=  pvmp3_polyphase_filter_window.cpp \
        pvmp3_mdct_18.cpp \
        pvmp3_dct_9.cpp \
        pvmp3_dct_16.cpp 
endif

HDRS = pvmp3_decoder.h pvmp3decoder_api.h pvmp3_audio_type_defs.h

LIBS =

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template

