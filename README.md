liblustre: An LGPL'ed lustre API
================================

This is work in progress.

See
[https://jira.hpdd.intel.com/browse/LU-5969](https://jira.hpdd.intel.com/browse/LU-5969)
for the rationale.

Organization
------------

The directory structure is as follow:

    * include/lustre/: exported headers API. They will be installed in
      /usr/include/lustre

    * lib/: the library source code.

    * man/: the man pages. These are written in ReST format and converted
      to nroff by rst2man from python-docutils.

    * tests: test programs for the library. They are run with make
      check.

The new header to include by applications is \<lustre/lustre.h\>. No
other file need to be included.

The library itself includes \<lustre/lustre.h\>, but also "internal.h"
which contains various definitions that are needed for the library but
need not be exported, such as private structures, ioctls definitions,
...

No application should communicate directly with lustre, through ioctls
for instance. Instead simple functions must be provided.

Only the functions that are part of the API must appear in
liblustre.map.

An updated API
--------------

The current liblustreapi has some shortcomings. For instance many
functions will search for the mountpoint of a lustre filesystem, open
it, issue an ioctl, then close it. This is not always necessary. For
instance when using the copytool, we already know everything we need
about the filesystem, thus we should be able to open it once.

### struct lustre\_fs\_h

This is a new structure meant to keep the information of an existing
filesystem (mountpoint, name, ...) so the information doesn't have to be
retrieved more than once. Thus we open and close a filesystem with:

    int lus_open_fs(const char *mount_path, struct lustre_fs_h **lfsh)
    void lus_close_fs(struct lustre_fs_h *lfsh)

lfsh is an opaque handle that can then be passed to various functions:

    int lus_fid2path(const struct lustre_fs_h *lfsh, const struct
    lu_fid *fid, char *path, int path_len, long long *recno, int
    *linkno)

    int lus_open_by_fid(const struct lustre_fs_h *lfsh, const
    lustre_fid *fid, int open_flags)

    int lus_hsm_copytool_register(const struct lustre_fs_h *lfsh, int
    archive_count, int *archives, struct lus_hsm_ct_handle **priv)

### posix copytool

As an example, and to validate this solution, the posix copytool has
been ported to this new API, in the tests/ directory.

Here's an strace difference for an archiving operation. Similar traces
are not shown.

copytool based on liblustreapi:

    ....
    [pid 14694] open("/etc/mtab", O_RDONLY) = 8
    [pid 14694] fstat(8, {st_mode=S_IFREG|0644, st_size=627, ...}) = 0
    [pid 14694] mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7fa1bbe14000
    [pid 14694] read(8, <unfinished ...>
    [pid 14694] <... read resumed> "/dev/mapper/vg_c65l251intel-lv_r"..., 4096) = 627
    [pid 14694] read(8, "", 4096) = 0
    [pid 14694] close(8) = 0
    [pid 14694] munmap(0x7fa1bbe14000, 4096) = 0
    [pid 14694] open("/mnt/lustre", O_RDONLY|O_NONBLOCK|O_DIRECTORY) = 8
    [pid 14694] ioctl(8, 0xffffffffc0086696, 0x7fa1b40008c0) = 0
    [pid 14694] close(8) = 0
    ....

copytool under liblustre:

    ...
    [pid 14756] ioctl(4, 0xc0086696, 0x7ffdf80008c0) = 0
    ...

The restore operation shows similar savings.

### Future direction

Robinhood should benefit a lot too from this updated API, since a lot of
its interaction with Lustre is similar to the copytool.

TODO
----

-   All exported API functions should be documented in their sources,
    not in the headers. Format should be the same as lustre (doxygen)
    and conformity enforced during build.
-   All exported API functions should have at least a small test.
-   Fix all the "TODO"s
-   Add a man page for every API.
-   check each mention of lustreapi and fix
-   llapi\_path2parent should probably have an lfsh arg and discriminate
    between full path and relative paths. lfsh would be used only if a
    relative path is passed. Same with path2fid.
-   remove recno from lus\_fid2path since it's unused
-   lus_fid2path and lus_fd2parent should use a newer ioctl that
    optionally copies the path directly into an application buffer to
    save a memory copy.
-   rework lus_hsm_hai_first and lus_hsm_hai_next to be nicer. Some
    of the work done by the caller could be done inside, like bound
    checking. Return the hai or NULL if no more hais are available in
    that hal.
-   move the copytool to a different repository. It's in this library
    for convenience only.
-   find a better name for llapi_hsm_state_get_fd, as it doesn't return
    a file descriptor. llapi_hsm_state_get_from_fd?
-   pool name given to lus_layout_set_pool_name should not have to strip
    the prefix.
-   strscpy and strscat should return a negative errno on failure, not
    just -1.
-   add API for IOC_MDC_GETFILEINFO (hsm) and
    IOC_MDC_GETFILESTRIPE (robinhood).

Changes from liblustreapi
-------------------------

Some functions have disappeared and other have been renamed. See
README.convert.rst to help porting.

### logging

liblustre doesn't provide an API to log messages, but will send log
messages if the application requests it. It is the job of the
application to emit these logs. By default no logs are evaluated. To
retrieve the log messages, the application has to register a callback
with lus\_log\_set\_callback() and set a log level with
lus\_log\_set\_level(). When the callback is called, the application
can then emit the messages if it wishes. The posix copytool, renamed
posixct.c, has been modified to use that interface and will output all
its messages on stdout.

Package dependencies
--------------------

On Debian/Ubuntu, the following packages are needed to build:

    check libattr1-dev python-docutils

On RHEL/CentOS, the following packages are needed to build:

    check-devel libattr-devel python-docutils

Building liblustre
------------------

To build the library:

    autoreconf -i
    ./configure
    make

### RPMS

RPMS can be built with:

    make rpms

This will result in these 4 rpms:

    rpms/SRPMS/liblustre-0.1.0-g01bb122.src.rpm
    rpms/RPMS/x86_64/liblustre-0.1.0-g01bb122.x86_64.rpm
    rpms/RPMS/x86_64/liblustre-devel-0.1.0-g01bb122.x86_64.rpm
    rpms/RPMS/x86_64/liblustre-debuginfo-0.1.0-g01bb122.x86_64.rpm

Only the liblustre rpm is need to run an application. liblustre-devel
contains the man pages and the headers.

Testing liblustre
-----------------

The automake test tool is used. First setup a lustre test filesystem
with some pools. Right now the path to lustre is harcoded in the
testsuite to /mnt/lustre.

    llmount.sh
    lctl pool_new lustre.mypool
    lctl pool_add lustre.mypool OST0000 OST0001
    echo enabled > /proc/fs/lustre/mdt/lustre-MDT0000/hsm_control
    echo 1000 > /proc/fs/lustre/mdt/lustre-MDT0000/hsm/max_requests

    make check

### Check

The tests are written with the "check" framework (see
http://check.sourceforge.net/). check is available in CentOS
in the rpm "check-devel", and in Debian/Ubuntu in the deb package
"check".

Some arguments, such as no forking can be passed on the command line
while testing:

    CK_FORK=no make check

See the documentation for more information:

    http://check.sourceforge.net/


### Valgrind

The testsuite can also be run under valgrind. Use valgrind 3.11 or
later since it has support for a few Lustre ioctls. As of writing,
this version hasn't been released; its sources can be retrieved from
subversion (see http://valgrind.org/downloads/repository.html).

For that purpose, under Centos 6, the following packages need to be
updated: autoconf, automake, libtool. The packages from CentOS 7 can
be used. The dependency on m4 can be ignored (use --nodeps when
installing the rpms).

    make check-valgrind


### Posix Copytool

Execute posixct once, so that the lt-posixct binary is created. Then
run the Lustre test suite like this:

    HSMTOOL=/PATH/TO/liblustre/tests/.libs/lt-posixct ONLY=12a PDSH=ssh AGTCOUNT=1 agt1_HOST=localhost NAME=local ./sanity-hsm.sh
