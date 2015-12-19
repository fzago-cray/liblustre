============================
llapi_create_volatile_by_fid
============================

-------------------------
liblustre file management
-------------------------

:Author: Frank Zago for Cray Inc.
:Date:   2015-04-14
:Manual section: 3
:Manual group: liblustre


SYNOPSIS
========

**#include <lustre/lustre.h>**

**int llapi_create_volatile_by_fid(const struct lustre_fs_h \***\
lfsh\ **, const lustre_fid \***\ parent_fid\ **, int** mdt_idx\ **,
int** open_flags\ **, mode_t** mode\ **, const struct llapi_layout
\***\ layout\ **)**


DESCRIPTION
===========

This function creates an anonymous, temporary, volatile file on a
Lustre filesystem. The created file is not visible with
**ls(1)**. Once the file is closed, or the owning process dies, the
file is permanently removed from the filesystem.

*lfsh* is an opaque Lustre fs handle returned by **lus_open_fs(3)**.

*parent_fid* is the FID of a directory into which the file must be
created.

*mdt_idx* is the MDT index onto which create the file. To use a
default MDT, set it to -1.

*open_flags* and *mode* are the same as **open(2)**.

*layout* describes the striping information. If it is NULL, then
the default for the directory is used.


RETURN VALUE
============

**llapi_create_volatile_by_fid** returns a file descriptor on success,
or a negative errno on failure.


ERRORS
======

The negative errno can be, but is not limited to:

**-EINVAL** An invalid value was passed.

**-ENOMEM** Not enough memory to allocate a resource.


SEE ALSO
========

**liblustre**\ (7)
