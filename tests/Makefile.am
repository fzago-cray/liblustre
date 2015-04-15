check_PROGRAMS=lib_test lhsmtool_posix llapi_fid_test

noinst_PROGRAMS=$(check_PROGRAMS)

TESTS = lib_test llapi_fid_test

lhsmtool_posix_LDADD=${top_builddir}/lib/liblustre.la -lbsd -lpthread

lib_test_CFLAGS = ${CHECK_CFLAGS}
lib_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS} -lbsd

llapi_fid_test_CFLAGS = ${CHECK_CFLAGS}
llapi_fid_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS} -lbsd
