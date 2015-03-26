AC_PREREQ(2.59)

define([githash], esyscmd([sh -c "git rev-parse --short HEAD | tr -d '\n'"]))dnl
AC_INIT([liblustre], [githash], [])

RELEASE="1"
AC_SUBST(RELEASE)

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror tar-pax foreign dist-xz no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

: ${CFLAGS="-ggdb -O2 -Wall"}

AC_GNU_SOURCE
AC_PROG_CC
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_LIBTOOL
AC_CONFIG_HEADERS([include/config.h])

CC_OPT="-Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wshadow -Wcast-align -Wwrite-strings -Wformat=2 -std=gnu99 -Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type -D_GNU_SOURCE -include config.h"

# Output files
AC_CONFIG_FILES([Makefile include/Makefile lib/Makefile tests/Makefile])
AC_OUTPUT
