#
# This makefile template is used for programs whose main routine is
# defined in C++ code.  See prog.mk for more information.
#

BUILD_ARCH = $(ARCHITECTURE)

REALTARGET = $(BUILD_ARCH)/$(TARGET)

STAT_LIB_EXT = a
STAT_OBJS_EXT = o

CLEAN+=$(REALTARGET) $(OBJS)


include $(MK)/prog.mk



#
# Build the actual target using the C++ compiler.  This rule uses
# gnu make's extension for supporting library dependencies with the
# -llib syntax.
#

STATIC_BINDING = -Bstatic

$(REALTARGET):: $(OBJS) $(ALL_LIBS)
	$(build_target:_LINK_=$(CXX))
