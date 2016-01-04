===================
lus_mdt_stat_by_fid
===================

-------------------------
liblustre file management
-------------------------

:Author: Frank Zago for Cray Inc.
:Date:   2016-01-07
:Manual section: 3
:Manual group: liblustre


SYNOPSIS
========

**#include <lustre/lustre.h>**

**int lus_mdt_stat_by_fid(const struct lus_fs_handle \***\
lfsh\ **, const lustre_fid \***\ fid\ **, struct stat \***\ st\ **)**


DESCRIPTION
===========

Query the MDT to get some file information, given a file's FID. The
data is returned in a structure similar to that of **stat**\ (2). The OSTs
are not queried so times or file sizes may not be accurate.

RETURN VALUE
============

**lus_mdt_stat_by_fid** returns 0 on success, or a negative errno on
failure.


SEE ALSO
========

**liblustre**\ (7), **stat**\ (2)
