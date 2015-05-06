=================
llapi_stat_by_fid
=================

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

**int llapi_stat_by_fid(const struct lustre_fs_h \***\ lfsh\ **,
const lustre_fid \***\ fid\ **, struct stat \***\ stbuf\ **)**


DESCRIPTION
===========

This function calls fstatat(2) on a file residing on Lustre, given its
FID.

*lfsh* is an opaque Lustre fs handle returned by **llapi_open_fs(3)**.

*fid* is the FID of the file to be stat'ed.

*stbuf* is the returned value.


RETURN VALUE
============

Since **llapi_stat_by_fid** is a passthrough for fstatat(2), it
returns 0 on success, or -1 with errno set.


ERRORS
======

See the man page for fstatat(2) for the possible errors.


SEE ALSO
========

**liblustre**\ (7), **fstatat**\ (2)
