# Include the project-wide makefile to get 
# appropriate definitions
include $(PV_TOP)/makefile.pv

WHAT = library
TARGET = osclio

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
XCXXFLAGS += $(FLAG_COMPILE_WARNINGS_AS_ERRORS)

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
# Added this to for osclconfig.h
#XINCDIRS += -I../../../config/$(BUILD_ARCH)  -I../../../config/shared


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
INCSRCDIR = ../../src

# compose final src list for actual build
SRCS =	oscl_dns.cpp \
	oscl_dns_gethostbyname.cpp \
	oscl_dns_imp_base.cpp \
	oscl_dns_imp_pv.cpp \
	oscl_dns_method.cpp \
	oscl_dns_param.cpp \
	oscl_dns_request.cpp \
	oscl_file_async_read.cpp \
	oscl_file_cache.cpp \
	oscl_file_dir_utils.cpp \
	oscl_file_find.cpp \
	oscl_file_io.cpp \
	oscl_file_native.cpp \
	oscl_file_server.cpp \
	oscl_file_stats.cpp \
	oscl_ip_socket.cpp \
	oscl_socket.cpp \
	oscl_socket_accept.cpp \
	oscl_socket_bind.cpp \
	oscl_socket_connect.cpp \
	oscl_socket_imp_base.cpp \
	oscl_socket_imp_pv.cpp \
	oscl_socket_imp_serv.cpp \
	oscl_socket_listen.cpp \
	oscl_socket_method.cpp \
	oscl_socket_recv.cpp \
	oscl_socket_recv_from.cpp \
	oscl_socket_request.cpp \
	oscl_socket_send.cpp \
	oscl_socket_send_to.cpp \
	oscl_socket_serv_imp_pv.cpp \
	oscl_socket_serv_imp_reqlist.cpp \
	oscl_socket_shutdown.cpp \
	oscl_socket_stats.cpp \
	oscl_tcp_socket.cpp \
	oscl_udp_socket.cpp


HDRS =  oscl_file_dir_utils.h \
	oscl_file_find.h \
	oscl_file_handle.h \
	oscl_file_io.h \
	oscl_file_server.h \
	oscl_socket.h \
	oscl_socket_types.h \
    oscl_dns.h

DESTDIR = $(SDK_LOCAL)/installed_lib/$(BUILD_ARCH)
INCDESTDIR = $(SDK_LOCAL)/installed_include
include $(MK)/gnumake.template
