
AM_CFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/tasks

bin_PROGRAMS = abegen

abegen_SOURCES = abegen.c abegen.h

#todo move abe_asrc to versioned directory
abegen_CFLAGS = $(AM_CFLAGS) -I $(LINUX_DIR) -Werror -Wall

