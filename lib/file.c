/*
 * An alternate Lustre user library.
 * Copyright 2015 Cray Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <attr/xattr.h>

#include <lustre/lustre.h>

#include "internal.h"

/**
 * Open a file given its FID.
 *
 * \param[in]  lfsh        an opened Lustre fs opaque handle
 * \param[in]  fid         the request fid, contained in the above Lustre fs
 * \param[in]  open_flags  the open flags. See open(2)
 *
 * \retval   a 0 or positive file descriptor on success
 * \retval   a negative errno on error
 */
int lus_open_by_fid(const struct lustre_fs_h *lfsh,
		    const lustre_fid *fid, int open_flags)
{
	char fidstr[FID_NOBRACE_LEN + 1];
	int rc;

	snprintf(fidstr, sizeof(fidstr), DFID_NOBRACE, PFID(fid));

	rc = openat(lfsh->fid_fd, fidstr, open_flags);
	return rc == -1 ? -errno : rc;
}

/**
 * Stat a file given its FID.
 *
 * \param[in]   lfsh   an opened Lustre fs opaque handle
 * \param[in]   fid    the request fid, contained in the above Lustre fs
 * \param[out]  stbuf  the stat result. See stat(2)
 *
 * \retval   a 0 or positive file descriptor on success
 * \retval   a negative errno on error
 */
int lus_stat_by_fid(const struct lustre_fs_h *lfsh,
		    const lustre_fid *fid, struct stat *stbuf)
{
	char fidstr[FID_NOBRACE_LEN + 1];
	int rc;

	snprintf(fidstr, sizeof(fidstr), DFID_NOBRACE, PFID(fid));
	rc = fstatat(lfsh->fid_fd, fidstr, stbuf, AT_SYMLINK_NOFOLLOW);
	return rc == -1 ? -errno : rc;
}

/**
 * Returns the parent FID of an opened file
 *
 * \param[in]   fd                  an opened file descriptor for a file
 *                                  in the above Lustre filesystem
 * \param[in]   linkno              which parent name to return
 * \param[out]  parent_fid          the parent fid
 *                                  Can be NULL.
 * \param[out]  parent_name         the full filename of the parent
 *                                  Can be NULL.
 * \param[in]   parent_name_len     the size of parent_name
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int lus_fd2parent(int fd, unsigned int linkno, lustre_fid *parent_fid,
		  char *parent_name, size_t parent_name_len)
{
	/* Avoid allocating gp by adding PATH_MAX after it. */
	struct {
		struct getparent gp;
		char filler[PATH_MAX];
	} x;
	int rc;

	x.gp.gp_name_size = sizeof(x.filler);
	x.gp.gp_linkno = linkno;

	rc = ioctl(fd, LL_IOC_GETPARENT, &x.gp);
	if (rc == 0) {
		if (parent_fid)
			*parent_fid = x.gp.gp_fid;
		if (parent_name) {
			rc = strscpy(parent_name, x.gp.gp_name,
				     parent_name_len);
			if (rc == -1)
				rc = -ENOSPC;
			else
				rc = 0;
		}
	} else {
		rc = -errno;
	}

	return rc;
}

/**
 * Returns the parent path and FID from a FID.
 * Note: this will fail for a symlink or a non-existent device
 *
 * \param[in]   lfsh               an opened Lustre fs opaque handle
 * \param[in]   fid                the FID of which to find the parent
 * \param[in]   linkno             which parent name to return
 * \param[out]  parent_fid         the parent fid
 *                                 Can be NULL.
 * \param[out]  parent_name        the full filename of the parent
 *                                 Can be NULL.
 * \param[in]   parent_name_len    the size of name
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int lus_fid2parent(const struct lustre_fs_h *lfsh,
		     const lustre_fid *fid,
		     unsigned int linkno,
		     lustre_fid *parent_fid,
		     char *parent_name, size_t parent_name_len)
{
	int fd;
	int rc;

	fd = lus_open_by_fid(lfsh, fid, O_RDONLY | O_NONBLOCK | O_NOFOLLOW);
	if (fd < 0)
		return fd;

	rc = lus_fd2parent(fd, linkno,
			   parent_fid, parent_name, parent_name_len);

	close(fd);

	return rc;
}

/**
 * Retrieve the parent path and FID from a path.
 *
 * Note: this will fail for a symlink or a non-existent device
 *
 * \param[in]   path          the path to get the parent FID from
 * \param[in]   linkno        which parent name to return
 * \param[out]  parent_fid    the parent fid
 * \param[out]  parent_name   the full filename of the parent
 * \param[in]   parent_name_len    the size of parent_name
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int lus_path2parent(const char *path, unsigned int linkno,
		      lustre_fid *parent_fid,
		      char *parent_name, size_t parent_name_len)
{
	int fd;
	int rc;

	fd = open(path, O_RDONLY | O_NONBLOCK | O_NOFOLLOW);
	if (fd == -1)
		return -errno;

	rc = lus_fd2parent(fd, linkno,
			   parent_fid, parent_name, parent_name_len);
	close(fd);

	return rc;
}

/* Get the FID from an extended attribute */
static int get_fid_from_xattr(const char *path, int fd, lustre_fid *fid)
{
	struct lustre_mdt_attrs lma;
	int rc;

	if (path == NULL)
		rc = fgetxattr(fd, XATTR_NAME_LMA, &lma, sizeof(lma));
	else
		rc = lgetxattr(path, XATTR_NAME_LMA, &lma, sizeof(lma));

	if (rc == -1) {
		return -errno;
	} else if (rc == sizeof(struct lustre_mdt_attrs)) {
		/* FID is stored in little endian. */
		fid->f_seq = le64toh(lma.lma_self_fid.f_seq);
		fid->f_oid = le32toh(lma.lma_self_fid.f_oid);
		fid->f_ver = le32toh(lma.lma_self_fid.f_ver);

		return 0;
	} else {
		/* Attribute is not the expected size. API change? */
		return -EINVAL;
	}
}

/**
 * Return a FID from an opened file descriptor.
 *
 * \param[in]   fd     an opened file descriptor for a file in the above
 *                     Lustre filesystem
 * \param[out]  fid    the requested parent fid
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int lus_fd2fid(int fd, lustre_fid *fid)
{
	int rc;

	rc = ioctl(fd, LL_IOC_PATH2FID, fid);
	if (rc == -1) {
		if (errno == EINVAL || errno == ENOTTY) {
			/* It's a pipe (EINVAL) or a character device
			 * (ENOTTY). Read from the extended attribute. */
			rc = get_fid_from_xattr(NULL, fd, fid);
		} else {
			rc = -errno;
		}
	}

	return rc;
}

/**
 * Return the FID of a file or directory
 *
 * \param[in]   path   a path to a file or a directory in a
 *                     Lustre filesystem
 * \param[out]  fid    the requested fid
 *
 * \retval    0 on success
 * \retval    a negative errno on error
 */
int lus_path2fid(const char *path, lustre_fid *fid)
{
	int fd;
	int rc;

	fd = open(path, O_RDONLY | O_NONBLOCK | O_NOFOLLOW);
	if (fd == -1) {
		/* It's a symbolic link (ELOOP), or a non-existent
		 * special device (ENXIO). Read from the extended
		 * attribute. */
		if (errno == ELOOP || errno == ENXIO)
			rc = get_fid_from_xattr(path, -1, fid);
		else
			rc = -errno;
	} else {
		rc = lus_fd2fid(fd, fid);
		close(fd);
	}

	return rc;
}

/**
 * Return a path given a FID. The path is relative to the filesystem
 * mountpoint. If the FID is of the mountpoint, the returned path will
 * be empty.
 *
 * \param[in]   lfsh          an opened Lustre fs opaque handle
 * \param[in]   fid           the FID of the file / directory
 * \param[out]  path          the requested path
 * \param[in]   path_len      the length of path
 * \param[in,out]   recno     may be NULL
 * \param[in,out]   linkno    which name to return; may be NULL
 *
 * \retval    0 on success
 * \retval    a negative errno on error.
 */
int lus_fid2path(const struct lustre_fs_h *lfsh, const lustre_fid *fid,
		 char *path, size_t path_len,
		 long long *recno, unsigned int *linkno)
{
	/* Avoid allocating gf by adding MAX_PATH after it. */
	struct {
		struct getinfo_fid2path gf;
		char filler[PATH_MAX];
	} x;
	int rc;

	/* The returned path is NUL terminated, so we need at least 1
	 * byte in it. */
	if (path_len == 0)
		return -ENOSPC;

	x.gf.gf_fid = *fid;
	x.gf.gf_recno = (recno != NULL) ? *recno : -1;
	x.gf.gf_linkno = (linkno != NULL) ? *linkno : 0;
	x.gf.gf_pathlen = sizeof(x.filler);

	rc = ioctl(lfsh->mount_fd, OBD_IOC_FID2PATH, &x.gf);
	if (rc == 0) {
		rc = strscpy(path, x.gf.gf_path, path_len);
		if (rc == -1) {
			rc = -ENOSPC;
		} else {
			rc = 0;
			if (recno != NULL)
				*recno = x.gf.gf_recno;
			if (linkno != NULL)
				*linkno = x.gf.gf_linkno;
		}
	} else {
		rc = -errno;
	}

	return rc;
}

/**
 * Open an anonymous file. That file will be destroyed by Lustre when
 * the last reference to it is closed.
 *
 * \param[in]  lfsh          an opaque handle returned by lus_open_fs()
 * \param[in]  parent_fid    the FID of a directory into which the file must be
 *                           created.
 * \param[in]  mdt_idx       the MDT index onto which create the file. To use a
 *                           default MDT, set it to -1.
 * \param[in]  open_flags    open flags, see open(2)
 * \param[in]  mode          open mode, see open(2)
 * \param[in]  layout        striping information. If it is NULL, then
 *                           the default for the directory is used.
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int lus_create_volatile_by_fid(const struct lustre_fs_h *lfsh,
			       const lustre_fid *parent_fid,
			       int mdt_idx, int open_flags, mode_t mode,
			       const struct llapi_layout *layout)
{
	char path[PATH_MAX];
	int fd;
	int rc;
	int rnumber;

	rnumber = random();

	if (parent_fid == NULL) {
		if (mdt_idx == -1)
			rc = snprintf(path, sizeof(path),
				      LUSTRE_VOLATILE_HDR "::%.4X", rnumber);
		else
			rc = snprintf(path, sizeof(path),
				      LUSTRE_VOLATILE_HDR ":%.4X:%.4X",
				      mdt_idx, rnumber);
		if (rc == -1 || rc >= sizeof(path))
			return -ENAMETOOLONG;

		fd = llapi_layout_file_openat(lfsh->mount_fd, path,
					      open_flags | O_RDWR | O_CREAT,
					      mode, layout);
	} else {

		if (mdt_idx == -1)
			rc = snprintf(path, sizeof(path),
				      DFID_NOBRACE "/" LUSTRE_VOLATILE_HDR "::%.4X",
				      PFID(parent_fid), rnumber);
		else
			rc = snprintf(path, sizeof(path),
				      DFID_NOBRACE "/" LUSTRE_VOLATILE_HDR ":%.4X:%.4X",
				      PFID(parent_fid), mdt_idx, rnumber);
		if (rc == -1 || rc >= sizeof(path))
			return -ENAMETOOLONG;

		fd = llapi_layout_file_openat(lfsh->fid_fd, path,
					      open_flags | O_RDWR | O_CREAT,
					      mode, layout);
	}

	if (fd == -1)
		return -errno;

	return fd;
}

/**
 * Return the MDT index for a file, given its FID.
 *
 * \param[in]   lfsh          an opened Lustre fs opaque handle
 * \param[in]   fid           the FID of which to find the parent
 *
 * \retval   0 or a positive MDT index
 * \retval   a negative errno on error
 */
int lus_get_mdt_index_by_fid(const struct lustre_fs_h *lfsh,
			     const struct lu_fid *fid)
{
	int rc;

	rc = ioctl(lfsh->mount_fd, LL_IOC_FID2MDTIDX, fid);
	if (rc < 0)
		return -errno;

	return rc;
}

/**
 * Return the data version of an opened file.
 *
 * \param[in]   fd       an opened file descriptor for a file on Lustre
 * \param[in]   flags    flush policy (0, LL_DV_RD_FLUSH or LL_DV_WR_FLUSH)
 * \param[out]  dv       resulting data version
 *
 * \retval   0 on success, with dv set
 * \retval   a negative errno on error
 */
int lus_data_version_by_fd(int fd, uint64_t flags, uint64_t *dv)
{
	struct ioc_data_version idv = { .idv_flags = flags };
	int rc;

	rc = ioctl(fd, LL_IOC_DATA_VERSION, &idv);
	if (rc == -1)
		return -errno;

	*dv = idv.idv_version;

	return 0;
}

/**
 * Swap the layout (ie. the data) of two opened files. Both files must
 * have been opened in writing mode.
 *
 * \param[in]  fd1      an opened file descriptor for a Lustre file
 * \param[in]  fd2      an opened file descriptor for a Lustre file
 * \param[in]  dv1      expected file data version for fd1. Used if
 *                        flags has SWAP_LAYOUTS_CHECK_DV1.
 * \param[in]  dv2      expected file data version for fd2. Used if
 *                        flags has SWAP_LAYOUTS_CHECK_DV2.
 * \param[in]  flags    or'ed SWAP_LAYOUTS_* flags
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int lus_fswap_layouts(int fd1, int fd2,
		      uint64_t dv1, uint64_t dv2,
		      uint64_t flags)
{
	struct lustre_swap_layouts lsl = {
		.sl_flags = flags,
		.sl_fd2 = fd2,
		.sl_gid = random(),
		.sl_dv1 = dv1,
		.sl_dv2 = dv2,
	};
	int rc;

	/* Set a non-zero group lock to flush dirty cache. */
	while (lsl.sl_gid == 0)
		lsl.sl_gid = random();

	rc = ioctl(fd1, LL_IOC_LOV_SWAP_LAYOUTS, &lsl);
	if (rc == -1)
		return -errno;

	return 0;
}

/**
 * Get a lock on the file. If the file is already locked, the gid must
 * match the lock.
 *
 * \param[in]  fd     an opened file descriptor for a file on Lustre
 * \param[in]  gid    a non-zero random number to identify the lock
 *
 * \retval  0 on success
 * \retval  a negative errno on error
 */
int lus_group_lock(int fd, uint64_t gid)
{
	int rc;

	rc = ioctl(fd, LL_IOC_GROUP_LOCK, gid);
	if (rc == -1)
		return -errno;

	return 0;
}

/**
 * Release a lock on a file, previously acquired with
 * lus_group_lock. The gid must match.
 *
 * \param[in]  fd     an opened file descriptor for a file on Lustre
 * \param[in]  gid    a non-zero random number to identify the lock
 *
 * \retval  0 on success
 * \retval  a negative errno on error
 */
int llapi_group_unlock(int fd, uint64_t gid)
{
	int rc;

	rc = ioctl(fd, LL_IOC_GROUP_UNLOCK, gid);
	if (rc == -1)
		return -errno;

	return 0;
}
