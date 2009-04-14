ifndef EXTERNAL_OPENCORE_CONFIG_ONCE
  # This is the first attempt to include this file.
  EXTERNAL_OPENCORE_CONFIG_ONCE := true

  PV_TOP := $(my-dir)
    PV_CFLAGS := -Wno-non-virtual-dtor -DENABLE_SHAREDFD_PLAYBACK -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DUSE_CML2_CONFIG
  FORMAT := android

ifeq ($(ENABLE_PV_LOGGING),1)
 PV_CFLAGS += -DPVLOGGER_INST_LEVEL=5
endif

ifeq ($(TARGET_ARCH),arm)
  PV_CFLAGS += -DPV_ARM_GCC_V5
endif

  # HAS_OSCL_LIB_SUPPORT turns on PV's OSCL dynamic loader.
  # Set PV_OSCL_LIB to true to enable it (default in release mode).
  # However for debugging with gdb you may want to set PV_OSCL_LIB to
  # false to disable this custom loader.
  PV_OSCL_LIB := true
  ifeq ($(PV_OSCL_LIB), true)
    PV_CFLAGS += -DHAS_OSCL_LIB_SUPPORT
  endif

include $(CLEAR_VARS)

  PV_COPY_HEADERS_TO := libpv

  alternate_config := $(if $(wildcard vendor/pv/pvplayer.cfg),true)
  ifeq ($(alternate_config), true)
    VALUE_ADD := 1
    PV_CFLAGS += -DPV_USE_VALUE_ADD=1
  else
    VALUE_ADD :=
  endif
  alternate_config :=

  PV_INCLUDES := \
	$(PV_TOP)/android \
	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
	$(PV_TOP)/engines/common/include \
	$(PV_TOP)/engines/player/config/core \
	$(PV_TOP)/engines/player/include \
	$(PV_TOP)/nodes/pvmediaoutputnode/include \
	$(PV_TOP)/nodes/pvdownloadmanagernode/config/opencore \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/fileformats/mp4/parser/config/opencore \
	$(PV_TOP)/oscl/oscl/config/android \
	$(PV_TOP)/oscl/oscl/config/shared \
	$(PV_TOP)/engines/author/include \
	$(PV_TOP)/android/drm/oma1/src \
	$(PV_TOP)/build_config/opencore_dynamic \
	$(TARGET_OUT_HEADERS)/$(PV_COPY_HEADERS_TO) 

  # Stash these values for the next includer of this file.
  OPENCORE.PV_TOP := $(PV_TOP)
  OPENCORE.PV_CFLAGS := $(PV_CFLAGS)
  OPENCORE.FORMAT := $(FORMAT)
  OPENCORE.PV_OSCL_LIB := $(OPENCORE.PV_OSCL_LIB)
  OPENCORE.PV_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
  OPENCORE.VALUE_ADD := $(VALUE_ADD)
  OPENCORE.PV_INCLUDES := $(PV_INCLUDES)
else
  # This file has already been included by someone, so we can
  # use the precomputed values.
  PV_TOP := $(OPENCORE.PV_TOP)
  PV_CFLAGS := $(OPENCORE.PV_CFLAGS)
  FORMAT := $(OPENCORE.FORMAT)
  PV_OSCL_LIB := $(OPENCORE.PV_OSCL_LIB)
  PV_COPY_HEADERS_TO := $(OPENCORE.PV_COPY_HEADERS_TO)
  VALUE_ADD := $(OPENCORE.VALUE_ADD)
  PV_INCLUDES := $(OPENCORE.PV_INCLUDES)
endif
