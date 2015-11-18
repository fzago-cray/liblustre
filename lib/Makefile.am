# Libraries to build.
lib_LTLIBRARIES = liblustre.la

# LGPL
liblustre_la_SOURCES = \
	file.c \
	liblustre.c \
	internal.h \
	liblustreapi_hsm.c \
	liblustreapi_json.c \
	liblustreapi_layout.c \
	logging.c \
	misc.c \
	osts.c \
	params.c \
	strings.c

liblustre_la_LDFLAGS = -Wl,--version-script=$(top_srcdir)/lib/liblustre.map
liblustre_la_CFLAGS = -I${top_srcdir}/include -I.

# file for pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblustre.pc
EXTRA_DIST = liblustre.pc.in
CLEANFILES = liblustre.pc

EXTRA_DIST += liblustre.map
