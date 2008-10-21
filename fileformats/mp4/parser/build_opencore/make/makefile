# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmp4ff

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
XCPPFLAGS += -DOPEN_FILE_ONCE_PER_TRACK  

# Use XINCDIRS for extra include directories. These should be proceeded 
# with "-I" just as they would be when passing to the compiler.
#

XINCDIRS = -I ../../config/opencore

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

SRCS =  3gpp_amrdecoderspecificinfo.cpp \
	3gpp_h263decoderspecificinfo.cpp \
	amrdecoderspecificinfo.cpp \
	amrsampleentry.cpp \
	assetinfoatoms.cpp \
	atom.cpp \
	atomutils.cpp \
	audiosampleentry.cpp \
	avcconfigurationbox.cpp \
        avcsampleentry.cpp \
	avcsampledependencytype.cpp \
	avcsampledependency.cpp \
	avcsampletogroupbox.cpp \
	avclayerdescriptionentry.cpp \
	avcsubseqdescriptionentry.cpp \
	basedescriptor.cpp \
	boxrecord.cpp \
	chunkoffsetatom.cpp \
	compositionoffsetatom.cpp \
	copyrightatom.cpp \
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
	fontrecord.cpp \
	fonttableatom.cpp \
	fullatom.cpp \
	h263decoderspecificinfo.cpp \
	h263sampleentry.cpp \
	handleratom.cpp \
	hintmediaheaderatom.cpp \
	impeg4file.cpp \
	initialobjectdescriptor.cpp \
	itunesilstatom.cpp \
	mediaatom.cpp \
	mediabuffer.cpp \
	mediabuffermanager.cpp \
	mediadataatom.cpp \
	mediaheaderatom.cpp \
	mediainformationatom.cpp \
	mediainformationheaderatom.cpp \
	metadataatom.cpp \
	movieatom.cpp \
	movieheaderatom.cpp \
	mpeg4file.cpp \
	mpeg4mediaheaderatom.cpp \
	mpeg4sampleentryextensions.cpp \
	mpegsampleentry.cpp \
	moviefragmentheaderatom.cpp \
	moviefragmentatom.cpp \
	movieextendsheaderatom.cpp \
	movieextendsatom.cpp \
	mfraoffsetatom.cpp \
	moviefragmentrandomaccess.cpp \
	objectdescriptor.cpp \
	objectdescriptoratom.cpp \
	oma2boxes.cpp \
	pvuserdataatom.cpp \
	sampledescriptionatom.cpp \
	sampleentry.cpp \
	samplesizeatom.cpp \
	sampletableatom.cpp \
	sampletochunkatom.cpp \
	slconfigdescriptor.cpp \
	soundmediaheaderatom.cpp \
	stylerecord.cpp \
	syncsampleatom.cpp \
	textsampleentry.cpp \
	textsamplemodifiers.cpp \
	timetosampleatom.cpp \
	trackatom.cpp \
	trackheaderatom.cpp \
	trackreferenceatom.cpp \
	trackreferencetypeatom.cpp \
	trackfragmentrunatom.cpp \
	trackfragmentheaderatom.cpp \
	trackfragmentatom.cpp \
	trackextendsatom.cpp \
	trackfragmentrandomaccess.cpp \
	userdataatom.cpp \
	videomediaheaderatom.cpp \
	visualsampleentry.cpp \
	paspatom.cpp \

HDRS =  impeg4file.h \
	isucceedfail.h \
        textsampleentry.h \
        sampleentry.h \
        atom.h \
        atomutils.h \
        parentable.h \
        renderable.h \
        fontrecord.h \
        atomdefs.h \
        decoderspecificinfo.h \
        avcconfigurationbox.h \
        mpeg4sampleentryextensions.h \
        basedescriptor.h \
        expandablebaseclass.h   
	

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include

include $(MK)/gnumake.template
