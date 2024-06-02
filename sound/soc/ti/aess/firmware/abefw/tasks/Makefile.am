

AM_CFLAGS = \
	-I$(top_srcdir)/src

AM_LDFLAGS = $(NODELETE_LDFLAGS)

MODULE_LDFLAGS = $(AM_LDFLAGS) -module -disable-static -avoid-version $(LDFLAGS_NOUNDEFINED)

OMAP4CFLAGS = -I $(LINUX_DIR) -Werror -Wall

abe009590dir = $(libdir)/abe/
abe009590_LTLIBRARIES = abe009590.la
abe009590_la_SOURCES = abe.c 009590/abe_asrc.c
abe009590_la_LDFLAGS = $(MODULE_LDFLAGS)
abe009590_la_LIBADD = $(MODULE_LIBADD)
abe009590_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS) -I 009590 -I$(top_srcdir)/include/009590

