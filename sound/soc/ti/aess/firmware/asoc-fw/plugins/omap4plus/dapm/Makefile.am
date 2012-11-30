

AM_CFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/plugins \
	-I$(top_builddir)/plugins/omap4plus

AM_LDFLAGS = $(NODELETE_LDFLAGS)

MODULE_LDFLAGS = $(AM_LDFLAGS) -module -disable-static -avoid-version $(LDFLAGS_NOUNDEFINED)

modlibexecdir = $(libdir)/plugins
OMAP4CFLAGS = -D __EXPORTED_HEADERS__ -I $(LINUX_DIR)/include/uapi -I $(LINUX_DIR)/include -I $(LINUX_DIR)/sound/soc/omap/aess -I $(LINUX_DIR)/sound/soc/omap/


if HAVE_OMAP
modlibexec_LTLIBRARIES = \
		abe-dl1dl2-twl6040.la

abe_dl1dl2_twl6040_la_SOURCES = abe-dl1dl2-twl6040.c
abe_dl1dl2_twl6040_la_LDFLAGS = $(MODULE_LDFLAGS)
abe_dl1dl2_twl6040_la_LIBADD = $(MODULE_LIBADD)
abe_dl1dl2_twl6040_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

endif
