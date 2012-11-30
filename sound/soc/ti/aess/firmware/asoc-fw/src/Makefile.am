
COMPATNUM=@LIBTOOL_VERSION_INFO@

AM_CFLAGS = \
	-I$(top_srcdir)/src -Wall -Werror -D __EXPORTED_HEADERS__ -I $(LINUX_DIR)/include/uapi -I $(LINUX_DIR)/include

bin_PROGRAMS = socfw

socfw_SOURCES = socfw.c socfw.h

socfw_CFLAGS = $(AM_CFLAGS)
socfw_LDADD = libsocfw.la

include_HEADERS = \
	socfw.h

lib_LTLIBRARIES = libsocfw.la
libsocfw_la_SOURCES = \
	lib.c

libsocfw_la_LDFLAGS = \
	-version-info 0 -release $(VERSION) \
	-export-dynamic
