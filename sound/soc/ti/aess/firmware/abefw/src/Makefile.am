
AM_CFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/tasks

bin_PROGRAMS = abegen

abegen_SOURCES = abegen.c abegen.h abe_asrc.c

abegen_CFLAGS = $(AM_CFLAGS) -I $(LINUX_DIR) -I $(HAL_DIR) -Werror -Wall

