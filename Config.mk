ifndef EXTERNAL_OPENCORE_CONFIG_ONCE
  # This is the first attempt to include this file.
  EXTERNAL_OPENCORE_CONFIG_ONCE := true

  PV_TOP := $(my-dir)
  PV_CFLAGS := -Wno-non-virtual-dtor -DENABLE_SHAREDFD_PLAYBACK
  FORMAT := nj
  PV_COPY_HEADERS_TO := libpv

  alternate_config := $(if $(wildcard vendor/pv/pvplayer.conf),true)
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
	$(PV_TOP)/engines/player/config/linux_nj \
	$(PV_TOP)/engines/player/include \
	$(PV_TOP)/nodes/pvmediaoutputnode/include \
	$(PV_TOP)/nodes/pvdownloadmanagernode/config/opencore \
	$(PV_TOP)/pvmi/pvmf/include \
	$(PV_TOP)/fileformats/mp4/parser/config/opencore \
	$(PV_TOP)/oscl/oscl/config/linux_nj \
	$(PV_TOP)/oscl/oscl/config/shared \
	$(PV_TOP)/engines/author/include \
	$(PV_TOP)/android/drm/oma1/src \
	$(TARGET_OUT_HEADERS)/$(PV_COPY_HEADERS_TO) 

  # Stash these values for the next includer of this file.
  OPENCORE.PV_TOP := $(PV_TOP)
  OPENCORE.PV_CFLAGS := $(PV_CFLAGS)
  OPENCORE.FORMAT := $(FORMAT)
  OPENCORE.PV_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)
  OPENCORE.VALUE_ADD := $(VALUE_ADD)
  OPENCORE.PV_INCLUDES := $(PV_INCLUDES)
else
  # This file has already been included by someone, so we can
  # use the precomputed values.
  PV_TOP := $(OPENCORE.PV_TOP)
  PV_CFLAGS := $(OPENCORE.PV_CFLAGS)
  FORMAT := $(OPENCORE.FORMAT)
  PV_COPY_HEADERS_TO := $(OPENCORE.PV_COPY_HEADERS_TO)
  VALUE_ADD := $(OPENCORE.VALUE_ADD)
  PV_INCLUDES := $(OPENCORE.PV_INCLUDES)
endif
