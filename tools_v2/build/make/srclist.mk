
ifndef SRCDIR
  SRCDIR := .
endif

ifneq  ($(strip $(MAKE)),clearmake)
  ifeq ($(HOST_ARCH),win32)
    REMOTE_SRCS := $(shell $(MK)/../bin/filter_remote.bat "$(SRCS)")
  else
    REMOTE_SRCS := $(shell $(MK)/../bin/filter_remote $(SRCS))
  endif
LOCAL_SRCS := $(filter-out $(REMOTE_SRCS), $(SRCS))
else
REMOTE_SRCS := ""
LOCAL_SRCS := $(SRCS)
endif


FAMILY_DIR := $(SRCDIR)/$(ARCHITECTURE)
FAMILY_SRCS_WANTED := $(LOCAL_SRCS:%=$(FAMILY_DIR)/%)
FAMILY_SRCS_FOUND := $(wildcard $(FAMILY_SRCS_WANTED))
FAMILY_SRCS_MISSING := $(filter-out $(FAMILY_SRCS_FOUND),$(FAMILY_SRCS_WANTED))

COMMON_SRCS_WANTED := $(FAMILY_SRCS_MISSING:$(FAMILY_DIR)/%=$(SRCDIR)/%)
COMMON_SRCS_FOUND := $(wildcard $(COMMON_SRCS_WANTED))
COMMON_SRCS_MISSING := $(filter-out $(COMMON_SRCS_FOUND),$(COMMON_SRCS_WANTED))

ALL_WANTED := $(COMMON_SRCS_WANTED)
ALL_FOUND := $(COMMON_SRCS_FOUND)

ifneq ($(ALL_WANTED),$(ALL_FOUND))

default:
	@echo "missing srcs..."
	@echo $(COMMON_SRCS_MISSING)
	@echo ""
	@/bin/false

endif


# Note: CXX will be used to represent 
# C++ files/variables throughout

COMBINED_SRCS := $(FAMILY_SRCS_FOUND) $(COMMON_SRCS_FOUND) $(REMOTE_SRCS)

ifneq  ($(strip $(MAKE)),clearmake)
FINAL_SRCS := $(notdir $(COMBINED_SRCS))
else
FINAL_SRCS := $(COMBINED_SRCS)
endif

CSRCS := $(filter %.c,$(FINAL_SRCS))
CCSRCS := $(filter %.cc,$(FINAL_SRCS))
CPPSRCS := $(filter %.cpp,$(FINAL_SRCS))
CXXSRCS := $(filter %.C,$(FINAL_SRCS))
ASMSRCS := $(filter %.s,$(FINAL_SRCS))
COBJS  := $(CSRCS:%.c=%.$(STAT_OBJS_EXT))
CPPOBJS := $(CPPSRCS:%.cpp=%.$(STAT_OBJS_EXT))
CXXOBJS := $(CXXSRCS:%.C=%.$(STAT_OBJS_EXT))
CCOBJS := $(CCSRCS:%.cc=%.$(STAT_OBJS_EXT))
ASMOBJS := $(ASMSRCS:%.s=%.$(STAT_OBJS_EXT))
COMPILED_OBJS := $(COBJS) $(CXXOBJS) $(CCOBJS) $(CPPOBJS) $(ASMOBJS)

