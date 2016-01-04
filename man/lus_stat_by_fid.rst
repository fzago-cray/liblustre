===============
lus_stat_by_fid
===============

-------------------------
liblustre file management
-------------------------

:Author: Frank Zago for Cray Inc.
:Date:   2015-05-05
:Manual section: 3
:Manual group: liblustre


SYNOPSIS
========

**#include <lustre/lustre.h>**

**int lus_stat_by_fid(const struct lus_fs_handle \***\ lfsh\ **,
const lustre_fid \***\ fid\ **, struct stat \***\ stbuf\ **)**


DESCRIPTION
===========

This function calls fstatat(2) on a file residing on Lustre, given its
FID.

*lfsh* is an opaque Lustre fs handle returned by **lus_open_fs(3)**.

*fid* is the FID of the file to be stat'ed.

*stbuf* is the returned value.


RETURN VALUE
============

**lus_stat_by_fid** returns 0 on success, or a negative errno on error.


ERRORS
======

See the man page for fstatat(2) for the possible errors.


SEE ALSO
========

**liblustre**\ (7), **fstatat**\ (2)
