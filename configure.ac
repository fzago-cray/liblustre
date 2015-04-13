AC_PREREQ(2.59)

define([githash], esyscmd([sh -c "git rev-parse --short HEAD | tr -d '\n'"]))dnl
AC_INIT([liblustre],
		[githash],
		[],
		[liblustre],
		[https://github.com/fzago-cray/liblustre])

RELEASE="1"
AC_SUBST(RELEASE)

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Wno-portability tar-pax foreign dist-xz no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Default flags
# TODO: add -Wsign-compare
: ${CFLAGS="-ggdb -O2 -Wall -Wchar-subscripts -Wformat-security -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wshadow -Wstrict-prototypes -Wtype-limits"}

AC_GNU_SOURCE
AC_PROG_CC_STDC
AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_HEADERS([include/config.h])

LT_INIT([
    disable-static
    pic-only
])

AC_CHECK_PROGS(RST2MAN, [rst2man rst2man.py], [])
if test "x$RST2MAN" = "x"; then
  AC_MSG_ERROR(
    [rst2man is needed to build the man pages. Install python-docutils.])
fi

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

# Output files
AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile lib/liblustre.pc man/Makefile tests/Makefile])
AC_OUTPUT

AC_MSG_RESULT([
        $PACKAGE $VERSION
        =====

        prefix:                 ${prefix}
        sysconfdir:             ${sysconfdir}
        libdir:                 ${libdir}
        includedir:             ${includedir}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
])
