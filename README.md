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

    int llapi_hsm_copytool_register(const struct lustre_fs_h *lfsh,
    struct hsm_copytool_private **priv, int archive_count, int
    *archives)

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

-   change the name of the prefix. llapi_ will conflict with
    liblustreapi. What about lus_ ?
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
-   llapi\_open\_by\_fid and some layout function should return -errno
    on error, not -1 with errno set.
-   remove recno from lus\_fid2path since it's unused
-   lus_fid2path and lus_fd2parent should use a newer ioctl that
    optionally copies the path directly into an application buffer to
    save a memory copy.
-   rework llapi_hsm_hai_first and llapi_hsm_hai_next to be nicer. Some
    of the work done by the caller could be done inside, like bound
    checking. Return the hai or NULL if no more hais are available in
    that hal.
-   inspect strcpy/strncpy/strcat/... and replace with strscpy/strscat
    if possible.
-   move the copytool to a different repository. It's in this library
    for convenience only.
-   LOV_MAXPOOLNAME doesn't include the ending NUL. That's a trap for
    userspace. Should it be 16 instead of 15?

Changes from liblustreapi
-------------------------

Some functions have disappeared and other have been renamed. This
section should help porting.

-   LPU64 is gone. Use "%llu". Cast if necessary.
-   LPX64 is gone. Use "%\#llx". Cast if necessary.
-   LPX64i is gone. Use "%llx". Cast if necessary.
-   llapi\_file\_open\_pool and llapi\_file\_open\_param are gone. Use
    llapi\_layout\_file\_open or llapi\_layout\_file\_openat. Careful
    with the return code. The former function returned negative errno,
    while the new one returns -1 and sets errno.
-   llapi\_create\_volatile\_idx is gone, and is replaced with
    llapi\_create\_volatile\_by\_fid and layout parameters.
-   llapi\_chomp\_string is not exported anymore.
-   llapi\_stripe\_limit\_check is gone. Use
    llapi\_layout\_stripe\_\*\_is\_valid.
-   llapi\_get\_mdt\_index\_by\_fid directly returns the MDT index.
-   dot\_lustre\_name is gone.
-   lus\_fid2path will return an empty string instead of / if the
    mountpoint is given. recno and linkno can now be NULL.
-   reading an HSM event is non blocking. llapi_hsm_copytool_register()
    can return -EWOULDBLOCK so the caller should handle that
    condition.
-   llapi_get_data_version is now called lus_data_version_by_fd and
    the last two parameters are swapped, to put the result last.
-   the gid given to llapi_group_lock / llapi_group_unlock is now an
    uint64_t instead of an int.
-   hai_first and hai_next are now functions called llapi_hsm_hai_first
    and llapi_hsm_hai_next
-   llapi_hsm_import takes its striping information in a layout
    struture. It now returns a file descriptor which can be used by
    the caller to set the extended attributes. The fid parameter is no
    longer needed since it can be retrieved by the caller through the
    fd.
-   llapi_hsm_user_request_alloc is gone. The caller can replace
      hur = llapi_hsm_user_request_alloc(x, y)
    with
      hur = malloc(llapi_hsm_user_request_len(x, y))
    This is more flexible as the caller can reuse the hur, resetting
    it between calls to llapi_hsm_request.

### logging

liblustre doesn't provide an API to log messages, but will send log
messages if the application requests it. It is the job of the
application to emit these logs. By default no logs are evaluated. To
retrieve the log messages, the application has to set a callback with
llapi\_msg\_callback\_set() and set a log level with
llapi\_msg\_set\_level(). When the callback is called, the application
can then emit the messages if it wishes. The posix copytool, renamed
posixct.c, has been modified to use that interface and will output all
its messages on stdout.

Package dependencies
--------------------

On Debian/Ubunto, the following packages are needed to build:

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
