# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = mpeg4filelib

# Use XCFLAGS to add extra compiler directives for the C compiler
# if you add any please use "XCFLAGS += ..." not "XCFLAGS = " 
# unless you're sure you want to override the project defaults.
# Please include a comment if you do this.
#
# XCFLAGS += 

# Use XCXXFLAGS to add extra compiler directives for the C++ compiler.
# See notes above regarding overriding project defaults.
#
# XCXXFLAGS += -DUSE_UNICODE  -D_POSIX_PTHREAD_SEMANTICS   #-D_POSIX_SOURCE

# we have the define by default
XCXXFLAGS += -DUNICODE -D_UNICODE

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
# XINCDIRS += -I../include -I$(OSCL_TOP)/installed_include -I../../../oscl/osclBase/src -I../../../oscl/osclIO/src -I../../../oscl/osclMemory/src -I../../../oscl/osclUtil/src
XINCDIRS = -I../include \
-I $(VOB_BASE_DIR)/oscl/oscl/pvscheduler/base/inc \
-I $(VOB_BASE_DIR)/oscl/oscl/osclmemory/src \
-I $(VOB_BASE_DIR)/oscl/oscl/osclbase/src \
-I $(VOB_BASE_DIR)/oscl/oscl/osclutil/src \
-I $(VOB_BASE_DIR)/oscl/oscl/osclio/src \
-I $(VOB_BASE_DIR)/baselibs/media_data_structures/src \


# Use XLIBDIRS for extra library directories. These should be proceeded 
# with "-L" just as they would be when passing to the linker. 
# Used for building executables.
#
# XLIBDIRS += 

# Use XLDFLAGS for extra linker directives.
# Used for building executables.
#
# XLDFLAGS += 


SRCS = amrdecoderspecificinfo.cpp \
amrdecoderspecificinfo3gpp.cpp \
amrsampleentry.cpp \
atom.cpp \
atomutils.cpp \
audiohintsample.cpp \
audiosampleentry.cpp \
audiotrackinfo.cpp \
basecommand.cpp \
basedescriptor.cpp \
chunkoffsetatom.cpp \
contentversionatom.cpp \
dataentryatom.cpp \
dataentryurlatom.cpp \
datainformationatom.cpp \
datareferenceatom.cpp \
decoderconfigdescriptor.cpp \
decoderspecificinfo.cpp \
downloadatom.cpp \
editatom.cpp \
editlistatom.cpp \
esdatom.cpp \
esdescriptor.cpp \
es_id_inc.cpp \
es_id_ref.cpp \
evrcdecoderspecificinfo.cpp \
expandablebaseclass.cpp \
filetypeatom.cpp \
fullatom.cpp \
h263sampleentry.cpp \
h263decoderspecificinfo.cpp \
h263decoderspecificinfo3gpp.cpp \
handleratom.cpp \
hintmediaheaderatom.cpp \
hintsample.cpp \
hintsampleentry.cpp \
impeg4file.cpp \
initialobjectdescriptor.cpp \
mediaatom.cpp \
mediabuffer.cpp \
mediabuffermanager.cpp \
mediadataatom.cpp \
mediaheaderatom.cpp \
mediainformationatom.cpp \
mediainformationheaderatom.cpp \
movieatom.cpp \
movieheaderatom.cpp \
mpeg4file.cpp \
mpeg4mediaheaderatom.cpp \
mpegsampleentry.cpp \
objectdescriptoratom.cpp \
objectdescriptor.cpp \
objectdescriptorupdate.cpp \
pvuserdataatom.cpp \
randomaccessatom.cpp \
requirementsatom.cpp \
sampledescriptionatom.cpp \
sampleentry.cpp \
samplesizeatom.cpp \
sampletableatom.cpp \
sampletochunkatom.cpp \
slconfigdescriptor.cpp \
soundmediaheaderatom.cpp \
stillimagehintsample.cpp \
syncsampleatom.cpp \
timetosampleatom.cpp \
trackatom.cpp \
trackreferencetypeatom.cpp \
trackheaderatom.cpp \
trackreferenceatom.cpp \
trackinfo.cpp \
trackinfoatom.cpp \
userdataatom.cpp \
videohintsample.cpp \
videoinfoatom.cpp \
videomediaheaderatom.cpp \
videoutils.cpp \
visualsampleentry.cpp \
videotrackinfo.cpp \
wmfsetmediaatom.cpp \
wmfsetsessionatom.cpp \
ztl.cpp

INCSRCDIR = ../include

HDRS = a_impeg4file.h \
amrdecoderspecificinfo.h  \
amrdecoderspecificinfo3gpp.h  \
amrsampleentry.h \
atomdefs.h \
atom.h \
atomutils.h \
audiohintsample.h \
audiosampleentry.h \
audiotrackinfo.h \
basecommand.h \
bifs.h \
basedescriptor.h \
chunklargeoffsetatom.h \
chunkoffsetatom.h \
compositionoffsetatom.h \
contentversionatom.h \
copyrightatom.h \
dataentryatom.h \
dataentryurlatom.h \
dataentryurnatom.h \
datainformationatom.h \
datareferenceatom.h \
decoderconfigdescriptor.h \
decoderspecificinfo.h \
degradationpriorityatom.h \
downloadatom.h \
editatom.h \
editlistatom.h \
esdatom.h \
esdescriptor.h \
es_id_inc.h \
es_id_ref.h \
evrcdecoderspecificinfo.h \
expandablebaseclass.h \
filetypeatom.h \
freespaceatom.h \
fullatom.h \
h263sampleentry.h \
h263decoderspecificinfo.h \
h263decoderspecificinfo3gpp.h \
handleratom.h \
hintmediaheaderatom.h \
hintsampleentry.h \
hintsample.h \
imediabuffer.h \
initialobjectdescriptor.h \
isucceedfail.h \
mediaatom.h \
mediabuffer.h \
mediabuffermanager.h \
mediadataatom.h \
mediaheaderatom.h \
mediainformationatom.h \
mediainformationheaderatom.h \
movieatom.h \
movieheaderatom.h \
mpeg4file.h \
mpeg4mediaheaderatom.h \
mpegsampleentry.h \
objectdescriptoratom.h \
objectdescriptor.h \
objectdescriptorupdate.h \
parentable.h \
pvuserdataatom.h \
pvstring.h \
randomaccessatom.h \
renderable.h \
requirementsatom.h \
sampledescriptionatom.h \
sampleentry.h \
samplesizeatom.h \
sampletableatom.h \
sampletochunkatom.h \
scalabilityhintsample.h \
shadowsyncsampleatom.h \
slconfigdescriptor.h \
soundmediaheaderatom.h \
stillimagehintsample.h \
syncsampleatom.h \
timetosampleatom.h \
trackatom.h \
trackheaderatom.h \
trackinfo.h \
trackinfoatom.h \
trackreferenceatom.h \
trackreferencetypeatom.h \
userdataatom.h \
videohintsample.h \
videoinfoatom.h \
videotrackinfo.h \
videomediaheaderatom.h \
videoutils.h \
visualsampleentry.h \
wmfsetmediaatom.h \
wmfsetsessionatom.h \
ztl.h
 
  
DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
                      
include $(MK)/gnumake.template
