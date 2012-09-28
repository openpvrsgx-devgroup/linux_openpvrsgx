

AM_CFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/coeffs

AM_LDFLAGS = $(NODELETE_LDFLAGS)

MODULE_LDFLAGS = $(AM_LDFLAGS) -module -disable-static -avoid-version $(LDFLAGS_NOUNDEFINED)

OMAP4CFLAGS = -I $(LINUX_DIR)/include/sound -I $(LINUX_DIR)/sound/soc/omap

if HAVE_OMAP

dl1dir = $(libdir)/abe/coeffs
dl1_LTLIBRARIES = dl1.la
dl1_la_SOURCES = dl1.c
dl1_la_LDFLAGS = $(MODULE_LDFLAGS)
dl1_la_LIBADD = $(MODULE_LIBADD)
dl1_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

dl2ldir = $(libdir)/abe/coeffs
dl2l_LTLIBRARIES = dl2l.la
dl2l_la_SOURCES = dl2l.c
dl2l_la_LDFLAGS = $(MODULE_LDFLAGS)
dl2l_la_LIBADD = $(MODULE_LIBADD)
dl2l_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

dl2rdir = $(libdir)/abe/coeffs
dl2r_LTLIBRARIES = dl2r.la
dl2r_la_SOURCES = dl2r.c
dl2r_la_LDFLAGS = $(MODULE_LDFLAGS)
dl2r_la_LIBADD = $(MODULE_LIBADD)
dl2r_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

dmicdir = $(libdir)/abe/coeffs
dmic_LTLIBRARIES = dmic.la
dmic_la_SOURCES = dmic.c
dmic_la_LDFLAGS = $(MODULE_LDFLAGS)
dmic_la_LIBADD = $(MODULE_LIBADD)
dmic_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

amicdir = $(libdir)/abe/coeffs
amic_LTLIBRARIES = amic.la
amic_la_SOURCES = amic.c
amic_la_LDFLAGS = $(MODULE_LDFLAGS)
amic_la_LIBADD = $(MODULE_LIBADD)
amic_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

sdtdir = $(libdir)/abe/coeffs
sdt_LTLIBRARIES = sdt.la
sdt_la_SOURCES = sdt.c
sdt_la_LDFLAGS = $(MODULE_LDFLAGS)
sdt_la_LIBADD = $(MODULE_LIBADD)
sdt_la_CFLAGS = $(AM_CFLAGS) $(OMAP4CFLAGS)

endif
