# Libraries to build. liblustre_unittest is built from the same
# sources as liblustre.la + liblustre_support.la. A lot of the
# liblustre code is static and/or not exported, so for every function
# we create a passthrough with the unittest prefix. That way we can
# test whatever function we want. All the unittest_* functions are
# stored in the tests/ directory, and the C files in this directory
# include it, protected by a UNIT_TEST define.
lib_LTLIBRARIES = liblustre.la
noinst_LTLIBRARIES = liblustre_unittest.la liblustre_support.la

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
	liblustre_logging.c \
	liblustre_misc.c \
	liblustre_osts.c \
	liblustre_params.c

liblustre_la_LDFLAGS = -Wl,--version-script=$(top_srcdir)/lib/liblustre.map
liblustre_la_CFLAGS = -I${top_srcdir}/include -I.

# Support library - Not exported
liblustre_support_la_SOURCES = \
	liblustre_support.c

# file for pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblustre.pc
EXTRA_DIST = liblustre.pc.in
CLEANFILES = liblustre.pc

liblustre_unittest_la_SOURCES = $(liblustre_la_SOURCES) $(liblustre_support_la_SOURCES)
liblustre_unittest_la_CFLAGS = -I${top_srcdir}/include -I. -DUNIT_TEST
liblustre_unittest_la_LDFLAGS = -Wl,--version-script=$(top_srcdir)/lib/liblustre.map

EXTRA_DIST += liblustre.map
