check_PROGRAMS=lib_test lhsmtool_posix llapi_fid_test

noinst_PROGRAMS=$(check_PROGRAMS)

TESTS = lib_test llapi_fid_test

lhsmtool_posix_CFLAGS = -I${top_srcdir}/include
lhsmtool_posix_LDADD=${top_builddir}/lib/liblustre.la -lbsd -lpthread

lib_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
lib_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS} -lbsd

llapi_fid_test_CFLAGS = ${CHECK_CFLAGS} -I${top_srcdir}/include
llapi_fid_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS} -lbsd

EXTRA_DIST = \
	liblustre_file2_tests.c \
	liblustre_misc_tests.c \
	liblustre_misc2_tests.c \
	liblustre_osts_tests.c \
	liblustre_params_tests.c \
	lib_test.c \
	llapi_fid_test.c
