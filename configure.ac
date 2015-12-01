AC_PREREQ(2.59)

# Note: the version is coming from the RPM specfile.in. That way, the
# version is stored in only one place.
define([specversion], esyscmd([sh -c "grep 'Version:' liblustre.spec.in | cut -d ':' -f 2 | tr -d '\n'"]))dnl

# Get the git SHA-1 from git. If the build is not in a git tree, for
# instance if the rpm is being rebuilt from the SRPMS, get it from a
# file called GITHASH. If that file doesn't exists, it is an error.
#
# This allows us to generate the rpm from a git tree (with make rpms)
# or to rebuild an rpm from the srpm.
#
# The githash is used for tracability, to pinpoint what original
# commit was used to create the rpm. It is inserted in the rpm name.
define([githash], esyscmd([sh -c "( git rev-parse --short HEAD || cat GITHASH ) 2> /dev/null | tr -d '\n'"]))dnl

AC_INIT([liblustre],
		[specversion],
		[],
		[liblustre],
		[https://github.com/fzago-cray/liblustre])

RELEASE="1"
AC_SUBST(RELEASE)

AC_SUBST(GIT_HASH, [githash])

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
AX_VALGRIND_CHECK
AC_CHECK_HEADERS(attr/xattr.h, , AC_MSG_ERROR([libattr-devel / libattr1-dev must be installed]))

# Output files
AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile lib/liblustre.pc
                 man/Makefile tests/Makefile GITHASH liblustre.spec])
AC_OUTPUT

AC_MSG_RESULT([
        $PACKAGE $VERSION.g${GIT_HASH}
        =====

        prefix:                 ${prefix}
        sysconfdir:             ${sysconfdir}
        libdir:                 ${libdir}
        includedir:             ${includedir}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
])
