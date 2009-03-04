# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmp4ffcomposer

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
#XCXXFLAGS += -DTRUE=true -DFALSE=false

# Use XCPPFLAGS to add extra preprocessor directives. Use XINCDIRS
# for other include directories, but please only do this if absolutely
# necessary -- let the makefile system find the include directories if 
# possible.
# See notes above regarding overriding project defaults.
#
# XCPPFLAGS +=  

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#

#XINCDIRS += 

# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.
#
# XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS += 

SRCDIR = ../../src
INCSRCDIR = ../../include

SRCS =  amrdecoderspecificinfo.cpp \
	amrdecoderspecificinfo3gpp.cpp \
	amrsampleentry.cpp \
    assetinfoatoms.cpp \
	atom.cpp \
	atomutils.cpp \
	audiosampleentry.cpp \
	avcconfiguration.cpp \
	avcsampleentry.cpp \
	basecommand.cpp \
	basedescriptor.cpp \
	chunkoffsetatom.cpp \
	dataentryatom.cpp \
	dataentryurlatom.cpp \
	datainformationatom.cpp \
	datareferenceatom.cpp \
	decoderconfigdescriptor.cpp \
	decoderspecificinfo.cpp \
	editatom.cpp \
	editlistatom.cpp \
	esdatom.cpp \
	esdescriptor.cpp \
	es_id_inc.cpp \
	es_id_ref.cpp \
	expandablebaseclass.cpp \
	filetypeatom.cpp \
	fullatom.cpp \
	h263decoderspecificinfo.cpp \
	h263decoderspecificinfo3gpp.cpp \
	h263sampleentry.cpp \
	handleratom.cpp \
	hintmediaheaderatom.cpp \
	impeg4file.cpp \
	initialobjectdescriptor.cpp \
	mediaatom.cpp \
	mediadataatom.cpp \
	mediaheaderatom.cpp \
	mediainformationatom.cpp \
	mediainformationheaderatom.cpp \
	movieatom.cpp \
	movieheaderatom.cpp \
	mpeg4file.cpp \
	mpeg4mediaheaderatom.cpp \
	mpegsampleentry.cpp \
	objectdescriptor.cpp \
	objectdescriptoratom.cpp \
	objectdescriptorupdate.cpp \
	sampledescriptionatom.cpp \
	sampleentry.cpp \
	samplesizeatom.cpp \
	sampletableatom.cpp \
	sampletochunkatom.cpp \
	slconfigdescriptor.cpp \
	soundmediaheaderatom.cpp \
	syncsampleatom.cpp \
	timetosampleatom.cpp \
	trackatom.cpp \
	trackheaderatom.cpp \
	trackreferenceatom.cpp \
	trackreferencetypeatom.cpp \
	userdataatom.cpp \
	videomediaheaderatom.cpp \
	visualsampleentry.cpp \
	textsampleentry.cpp \
	boxrecord.cpp \
	fontrecord.cpp \
	fonttableatom.cpp \
	stylerecord.cpp \
	interleavebuffer.cpp \
	movieextendsatom.cpp \
	movieextendsheaderatom.cpp \
	moviefragmentatom.cpp \
	moviefragmentheaderatom.cpp \
	moviefragmentrandomaccessatom.cpp \
	moviefragmentrandomaccessoffsetatom.cpp \
	trackextendsatom.cpp \
	trackfragmentatom.cpp \
	trackfragmentheaderatom.cpp \
	trackfragmentrandomaccessatom.cpp \
	trackfragmentrunatom.cpp
	
HDRS = a_impeg4file.h \
       a_isucceedfail.h \
       a_atomdefs.h \
       textsampledescinfo.h \
       ../config/opencore/pv_mp4ffcomposer_config.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
