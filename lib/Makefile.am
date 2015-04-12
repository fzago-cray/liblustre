AM_CFLAGS= -I${top_srcdir}/include -I.

lib_LTLIBRARIES = liblustre.la

# LGPL
liblustre_la_SOURCES = liblustreapi_json.c liblustreapi_hsm.c

# Not LGPL yet
liblustre_la_SOURCES += \
	kernel_user_comm.c \
	liblustre.c \
	liblustre_file.c \
	liblustre_ioctls.c \
	liblustre_logging.c \
	liblustre_misc.c \
	liblustre_internal.h

liblustre_la_LDFLAGS = -Wl,--version-script=liblustre.map

# file for pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblustre.pc
EXTRA_DIST = liblustre.pc.in
CLEANFILES = liblustre.pc
