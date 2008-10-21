
MK=$(VOB_BASE_DIR)/tools_v2/build/make

PROJECT=$(PV_TOP)
STAGING=
BASELINE=

# Add SDK to search path to find core libraries.  Note that the local SDK 
# is searched first so that local builds can override the builds in $(VOB_BASE_DIR)/corelibs
#

POST_INCDIRS += -I$(SDK_LOCAL)/include \
		-I$(SDK_LOCAL)/installed_include \
		-I$(VOB_BASE_DIR)/oscl/oscl/config/shared \
		-I$(VOB_BASE_DIR)/oscl/oscl/config/$(BUILD_ARCH)


POST_LIBDIRS += -L$(SDK_LOCAL)/installed_lib/$(BUILD_ARCH) \
		-L$(VOB_BASE_DIR)/oscl/SDK/lib/$(BUILD_ARCH) \



# Set the default toolset
#

ifneq (,$(findstring wince,$(BUILD_ARCH)))
 ifeq ($(strip $(WINCE_VER)),)
        _error_no_wince_ver:
		@echo Error: WINCE_VER (200, 201, 210, 211, 212 or 300) must be defined, make stopped
		@/bin/false
 endif
endif

$ARCHITECTURE = $(shell $(MK)/../bin/archtype)

ifeq ($(ARCHITECTURE),hpux)
  TOOLSET=aCC
else
  ifeq ($(ARCHITECTURE),win32)
    ifneq (,$(findstring wince,$(BUILD_ARCH)))
      TOOLSET=ms_ecpp
    else
      ifneq (,$(findstring _arm,$(BUILD_ARCH)))
        TOOLSET=armcpp
      else 
        TOOLSET=cl
      endif
    endif
  else 
    TOOLSET=g++
  endif
endif     



# Set compilation flags based on the toolset and release/debug build type
#
#
#   Adding _HPUX_SOURCE and _XOPEN_SOURCE_EXTENDED to get socket functions 
#   with socklen_t parameters.
#

ifeq ($(TOOLSET),aCC)
    CPPFLAGS += -D_HPUX_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_PTHREADS -D_RWSTD_MULTI_THREAD -lpthread
else
  ifeq ($(TOOLSET),g++)
    CPPFLAGS += -D_GNU_SOURCE -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_PTHREADS
  else
    ifeq ($(TOOLSET),armcpp)
       # Try using these defines for now
       CPPFLAGS += -D_DUMMY_ARMCPP_DEFINE
    else
      ifeq ($(TOOLSET),cl)
        CPPFLAGS += /D "WIN32" /D "_MBCS" /D "_LIB"
      else
        ifeq ($(TOOLSET),ms_ecpp)
          CPPFLAGS += /D _WIN32_WCE=$(WINCE_VER) /D UNDER_CE=$(WINCE_VER) /D "UNICODE" /D "_UNICODE" /D "_LIB"
        else
          _error_default_bad_toolset:
		@echo Error: unknown toolset [$(TOOLSET)], make stopped
		@/bin/false
        endif
      endif
    endif
  endif
endif


ifeq ($(RELEASE),1)
  ifeq ($(TOOLSET),aCC)
    COMPFLAGS += +O2 -Aa -AA -ext +A +DD32 +DAportable
    CPPFLAGS += -DNDEBUG 
    # -AA flag needed in linker command to get new std library
    LDFLAGS += -AA -s 
  else
    ifeq ($(TOOLSET),cl)
      COMPFLAGS +=  /nologo /MTd /W3 /Z7 /FD /GZ
    else
      ifeq ($(TOOLSET),ms_ecpp)
        COMPFLAGS +=  /nologo /W3 /Z7 /Od /MC
      else
        COMPFLAGS += -Wall -O3
        CPPFLAGS += -DNDEBUG 
        # The next line is used to create a stripped binary
        LDFLAGS += -Xlinker -s
      endif
    endif
  endif
else
  ifeq ($(TOOLSET),aCC)
    COMPFLAGS += -g -Aa -AA -ext +A +DD32 +DAportable
    LDFLAGS += -D_RWSTD_MULTI_THREAD -D_REENTRANT -lpthread
    # -AA flag needed in linker command to get new std library
    LDFLAGS += -AA
  else
    ifeq ($(TOOLSET),cl)
      COMPFLAGS +=  /nologo /MTd /W3 /Z7 /FD /GZ /D "_DEBUG"
    else
      ifeq ($(TOOLSET),ms_ecpp)
        COMPFLAGS +=  /nologo /W3 /Z7 /Od /MC /D "DEBUG"
      else
        COMPFLAGS += -Wall -g 
      endif
    endif
  endif
endif

COMPFLAGS += $(DEBUG)
