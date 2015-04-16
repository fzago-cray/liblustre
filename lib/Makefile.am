AM_CFLAGS= -I${top_srcdir}/include -I.

lib_LTLIBRARIES = liblustre.la liblustre_unittest.la

# LGPL
liblustre_la_SOURCES = \
	liblustreapi_json.c \
	liblustreapi_hsm.c \
	liblustreapi_layout.c

# Not LGPL yet
liblustre_la_SOURCES += \
	kernel_user_comm.c \
	liblustre.c \
	liblustre_file2.c \
	liblustre_internal.h \
	liblustre_ioctls.c \
	liblustre_logging.c \
	liblustre_misc.c \
	liblustre_misc2.c \
	liblustre_osts.c \
	liblustre_params.c

liblustre_la_LDFLAGS = -Wl,--version-script=liblustre.map

# file for pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblustre.pc
EXTRA_DIST = liblustre.pc.in
CLEANFILES = liblustre.pc


liblustre_unittest_la_SOURCES = $(liblustre_la_SOURCES)
liblustre_unittest_la_CFLAGS = -DUNIT_TEST
liblustre_unittest_la_LDFLAGS = -Wl,--version-script=liblustre.map
