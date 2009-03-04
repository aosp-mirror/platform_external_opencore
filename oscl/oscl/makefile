LIBDIRS = \
	osclbase/build/make \
	osclerror/build/make \
	osclmemory/build/make \
	osclutil/build/make \
	osclproc/build/make \
	../pvlogger/build/make \
	osclio/build/make \
	osclregserv/build/make \
	osclregcli/build/make

TESTDIRS = \
	osclbase/test/build/make \
	osclerror/test/build/make \
	osclmemory/test/build/make \
	osclutil/test/build/make \
	osclproc/test/build/make \
	../pvlogger/test/build/make \
	osclio/test/build/make  \
	osclregcli/test/build/make


ifeq ($(UNITTEST),1)
LIBDIRS += \
	../unit_test/build/make 
endif

include $(MK)/recursive.mk

doc:
	 perl $(VOB_BASE_DIR)/tools_v2/build/document/bin/doc_build.bat --doctype oscl --title "OSCL API" --path "$(VOB_BASE_DIR)/oscl/oscl" --filetype "mainpage *.h readme.txt" --exclude_pattern "*/test/* */obsolete/* */doxydir/*" -ver $(OSCL_VERSION)




