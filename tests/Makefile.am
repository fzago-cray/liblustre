check_PROGRAMS=lib_test posixct llapi_fid_test group_lock_test

noinst_PROGRAMS=$(check_PROGRAMS)

TESTS = lib_test llapi_fid_test group_lock_test

posixct_CFLAGS = -I${top_srcdir}/include
posixct_LDADD = \
	${top_builddir}/lib/liblustre_support.la \
	${top_builddir}/lib/liblustre.la -lpthread

lib_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
lib_test_LDADD = \
	${top_builddir}/lib/liblustre_support.la \
	${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS}

llapi_fid_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
llapi_fid_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS}

group_lock_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
group_lock_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS}

EXTRA_DIST = \
	lib_test.c \
	llapi_fid_test.c \
	test_file.c \
	test_misc.c \
	test_osts.c \
	test_params.c \
	test_support.c

# See m4/ax_valgrind_check.m4 for documentation
@VALGRIND_CHECK_RULES@
VALGRIND_SUPPRESSIONS_FILES = valgrind.supp
EXTRA_DIST += valgrind.supp
