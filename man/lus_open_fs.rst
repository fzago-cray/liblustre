===========
lus_open_fs
===========

------------------------------
Lustre API copytool management
------------------------------

:Author: Frank Zago
:Date:   2016-01-07
:Manual section: 3
:Manual group: liblustre


SYNOPSIS
========

**#include <lustre/lustre.h>**

**int lus_open_fs(const char \***\ path\ **, struct lus_fs_handle \*\***\ lfsh\ **)**

**int lus_close_fs(const char \***\ path\ **, struct lus_fs_handle \*\***\ lfsh\ **)**

**const char \*\ lus_get_fsname(const struct lus_fs_handle \***\ lfsh\ **)**

**const char \*\ lus_get_mountpoint(const struct lus_fs_handle \***\ lfsh\ **)**

DESCRIPTION
===========

**lus_open_fs** opens a Lustre filesystem given a *path* where one is
mounted. It also opens its *.lustre/fid* directory. The filesystem
cannot be unmounted until the handle is released with
**lus_close_fs**. The opaque handle *lfsh* can be passed to other
operations provided by the liblustre library.

Once the filesystem has been opened, **lus_get_fsname** can be used to
retrieve the filesystem name, and **lus_get_mountpoint** can get its
mountpoint. Neither function can fail, and a read-only valid string is
returned.

**lus_close_fs** can be called safely several times. The opaque handle
*lfsh* will be set to NULL the first time.

RETURN VALUE
============

**lus_open_fs** returns 0 on success, and a negative errno on
failure. The *lfsh* handle will be set on error, or NULL on failure.

**lus_close_fs**,

SEE ALSO
========

**liblustre**\ (7)
