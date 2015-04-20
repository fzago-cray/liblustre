liblustre: A (not yet LGPL) lustre API
======================================

This is work in progress. NOT ALL FILES ARE UNDER LGPL YET.

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
other file need to be included. \<lustre/lustre.h\> will itself include
\<lustre/lustre\_user.h\>, but that file cannot be included directly.

The library itself includes \<lustre/lustre.h\>, but also
"lustre\_internal.h" which contains various definitions and helpers that
are needed for the library but need not be exported, such as private
structures, ioctls definitions, ...

No application should communicate directly with lustre, through ioctls
for instance. Instead simple functions must be provided.

Only the functions that are part of the API must appear in
liblustre.map.

An updated API
--------------

The current liblustreapi has some shortcomings. For instance many
function will search for the mountpoint of a lustre filesystem, open it,
issue an ioctl, then close it. This is not always necessary. For
instance when using the copytool, we already know everything we need
about the filesystem, thus we should be able to open it once.

### struct lustre\_fs\_h

This is a new structure meant to keep the information of an existing
filesystem (mountpoint, name, ...) so the information doesn't have to be
retrieved more than once. Thus we open and close a filesystem with:

    void llapi_close_fs(struct lustre_fs_h *lfsh)
    int llapi_open_fs(const char *mount_path, struct lustre_fs_h **lfsh)

lfsh is an opaque handle that can then be passed to various functions:

    int llapi_fid2path(const struct lustre_fs_h *lfsh, const struct
    lu_fid *fid, char *path, int path_len, long long *recno, int
    *linkno)

    int llapi_open_by_fid(const struct lustre_fs_h *lfsh, const
    lustre_fid *fid, int open_flags)

    int llapi_hsm_copytool_register(const struct lustre_fs_h *lfsh,
    struct hsm_copytool_private **priv, int archive_count, int
    *archives, int rfd_flags)

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
-   do something with llapi\_hsm\_log\_error; it doesn't fit.
-   llapi\_path2parent should probably have an lfsh arg and discriminate
    between full path and relative paths. lfsh would be used only if a
    relative path is passed. Same with path2fid.
-   llapi\_open\_by\_fid and some layout function should return -errno
    on error, not -1 with errno set.
-   remove recno from llapi\_fid2path since it's unused
-   llapi_fid2path and llapi_fd2parent should use a newer ioctl that
    optionally copies the path directly into an application buffer to
    save a memory copy.

Changes from liblustreapi
-------------------------

Some function have disappeared and other have been renamed. This section
should help porting.

-   LPU64 is gone. Use "%llu". Cast if necessary.
-   LPX64 is gone. Use "%\#llx". Cast if necessary.
-   LPX64i is gone. Use "%llx". Cast if necessary.
-   llapi\_file\_open\_pool and llapi\_file\_open\_param are gone. Use
    llapi\_layout\_file\_open or llapi\_layout\_file\_openat. Careful
    with the return code. The former function returned negative errno,
    while the new one return -1 and sets errno.
-   llapi\_file\_open\_param is gone. Use llapi\_file\_open.
-   llapi\_create\_volatile\_idx is gone, and is replaced with
    llapi\_create\_volatile\_by\_fid and a layout parameters.
-   llapi\_chomp\_string is not exported anymore.
-   llapi\_stripe\_limit\_check is gone. Use
    llapi\_layout\_stripe\_\*\_is\_valid.
-   llapi\_get\_mdt\_index\_by\_fid directly returns the MDT index.
-   dot\_lustre\_name is gone.
-   llapi\_fid2path will return an empty string instead of / if the
    mountpoint is given. recno and linkno can now be NULL.

### logging

liblustre doesn't provide an API to log messages. It is the job of the
application to emit these logs. By default no logs are evaluated. To
retrieve the log messages, the application has to set a callback with
llapi\_msg\_callback\_set() and set a log level with
llapi\_msg\_set\_level(). When the callback is called, the application
can then emit the messages if it wishes. lhsmtool\_posix.c has been
modified to use that interface. The posix copytool will output all its
messages on stdout.

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

Only the liblustre rpm is need to run an
application. liblustre-develcontains the man pages and the headers.

Testing liblustre
-----------------

The automake test tool is used. First setup a lustre test filesystem
with some pools. Right now the path to lustre is harcoded in the
testsuite to /mnt/lustre.

    llmount.sh
    lctl pool_new lustre.mypool
    lctl pool_add lustre.mypool OST0000

    make check

### Check

The tests are written with the "check" framework (see
http://check.sourceforge.net/). Some arguments, such as no forking can
be passed on the command line while testing:

    CK_FORK=no make check

See the documentation for more information:


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
