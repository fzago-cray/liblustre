=====================
llapi_create_volatile
=====================

-------------------------
liblustre file management
-------------------------

:Author: Frank Zago for Cray Inc.
:Date:   2015-04-10
:Manual section: 3
:Manual group: liblustre


SYNOPSIS
========

**#include <lustre/lustre.h>**

**int llapi_create_volatile(const char \***\ directory\ **,
int** mdt_idx\ **, int** open_flags\ **, mode_t** mode\ **, const
struct llapi_stripe_param \***\ stripe_param\ **)**


DESCRIPTION
===========

This function creates an anonymous, temporary, volatile file on a
Lustre filesystem. The created file is not visible with
**ls(1)**. Once the file is closed, or the owning process dies, the
file is permanently removed from the filesystem.

This function will also work on a non-Lustre filesystem, where the
file is created then deleted, leaving only the file descriptor to
access the file. This is not strictly equivalent because there is a
small window during which the file is visible to users (provided they
have access to the *directory*).

The *directory* parameter indicates where to create the file on the
Lustre filesystem.

*mdt_idx* is the MDT index onto which create the file. To use a
default MDT, set it to -1.

*open_flags* and *mode* are the same as **open(2)**.

*stripe_param* describes the striping information. If it is NULL, then
the default for the directory is used.


RETURN VALUE
============

**llapi_create_volatile** returns a file descriptor on success, or a
negative errno on failure.


ERRORS
======

The negative errno can be, but is not limited to:

**-EINVAL** An invalid value was passed.

**-ENOMEM** Not enough memory to allocate a resource.


SEE ALSO
========

**liblustre**\ (7)
