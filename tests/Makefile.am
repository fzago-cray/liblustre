check_PROGRAMS=lib_test llapi_fid_test group_lock_test llapi_hsm_test \
	llapi_layout_test

noinst_PROGRAMS=posixct
noinst_LTLIBRARIES = liblustre_support.la

TESTS = \
	lib_test.sh llapi_fid_test.sh group_lock_test.sh \
	llapi_hsm_test.sh llapi_layout_test.sh

testsdir=@datarootdir@/@PACKAGE_NAME@/tests
tests_PROGRAMS=$(check_PROGRAMS)
tests_SCRIPTS=$(check_SCRIPTS)

# Support library - Not exported
liblustre_support_la_SOURCES = support.c support.h
liblustre_support_la_CFLAGS = -I${top_srcdir}/include -I.

posixct_CFLAGS = -I${top_srcdir}/include
posixct_SOURCES = posixct.c ../lib/strings.c
posixct_LDADD = \
	liblustre_support.la \
	${top_builddir}/lib/liblustre.la -lpthread

lib_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
lib_test_LDADD = \
	liblustre_support.la \
	liblustre_unittest.la ${CHECK_LIBS}

llapi_fid_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
llapi_fid_test_SOURCE = llapi_fid_test.c check_extra.h
llapi_fid_test_LDADD = \
	${top_builddir}/lib/liblustre.la \
	liblustre_support.la ${CHECK_LIBS}

llapi_hsm_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
llapi_hsm_test_SOURCE = llapi_hsm_test.c check_extra.h
llapi_hsm_test_LDADD = \
	${top_builddir}/lib/liblustre.la \
	liblustre_support.la ${CHECK_LIBS}

llapi_layout_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
llapi_layout_test_SOURCE = llapi_layout_test.c check_extra.h
llapi_layout_test_LDADD = \
	${top_builddir}/lib/liblustre.la \
	liblustre_support.la ${CHECK_LIBS}

group_lock_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
group_lock_test_SOURCE = group_lock_test.c check_extra.h
group_lock_test_LDADD = \
	${top_builddir}/lib/liblustre.la \
	liblustre_support.la ${CHECK_LIBS}

# liblustre_unittest is built from the same sources as liblustre.la +
# liblustre_support.la. A lot of the liblustre code is static and/or
# not exported, so for some files we create a wrapper and compile it
# instead of the original file. The wrapper contains the tests. If the
# library file is called xxx.c, the wrapper will be test_xxx.c and
# will include xxx.c.  That way we can test whatever function we want.
noinst_LTLIBRARIES += liblustre_unittest.la

liblustre_unittest_la_SOURCES = \
	test_file.c \
	test_misc.c \
	test_osts.c \
	test_params.c \
	test_support.c \
	check_extra.h \
	lib_test.h \
	$(top_srcdir)/lib/liblustre.c \
	$(top_srcdir)/lib/internal.h \
	$(top_srcdir)/lib/liblustreapi_hsm.c \
	$(top_srcdir)/lib/liblustreapi_layout.c \
	$(top_srcdir)/lib/logging.c \
	$(top_srcdir)/lib/strings.c

liblustre_unittest_la_CFLAGS = -I${top_srcdir}/include -I.
liblustre_unittest_la_LDFLAGS = -Wl,--version-script=$(top_srcdir)/lib/liblustre.map

# See m4/ax_valgrind_check.m4 for documentation
@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = valgrind.supp
VALGRIND_memcheck_FLAGS= --malloc-fill=0x9f --free-fill=0xe2 --track-fds=yes
EXTRA_DIST = valgrind.supp
