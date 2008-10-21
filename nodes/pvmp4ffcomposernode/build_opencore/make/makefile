# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = pvmp4ffcomposernode

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
# Added this path so that correct headers should be picked up for author
XINCDIRS += -I$(SDK_LOCAL)/installed_include -I../../src -I../../../../nodes/common/include -I../../../../pvmi/pvmf/include -I ../../include -I ../../../../baselibs/pv_mime_utils/src -I ../../../../engines/author/include -I ../../../../baselibs/media_data_structures/src -I ../../../../oscl/oscl/osclio/src -I ../../../../fileformats/mp4/composer/config/opencore

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

# compose final src list for actual build
SRCS =	pvmp4ffcn_port.cpp pvmp4ffcn_node.cpp pvmp4ffcn_node_cap_config.cpp

HDRS =  pvmp4ffcn_clipconfig.h pvmp4ffcn_factory.h pvmp4ffcn_trackconfig.h pvmp4ffcn_types.h 


DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
