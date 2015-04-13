check_PROGRAMS=lib_test lhsmtool_posix

noinst_PROGRAMS=$(check_PROGRAMS)

TESTS = lib_test

lhsmtool_posix_LDADD=${top_builddir}/lib/liblustre.la -lbsd -lpthread

lib_test_CFLAGS = ${CHECK_CFLAGS}
lib_test_LDADD=${top_builddir}/lib/liblustre_unittest.la ${CHECK_LIBS} -lbsd
