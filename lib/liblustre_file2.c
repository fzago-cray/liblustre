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

#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <errno.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/**
 * Open a file given its FID.
 *
 * \param[in]  lfsh   an opened Lustre fs opaque handle
 * \param[in]  fid    the request fid, contained in the above Lustre fs
 * \param[in]  flags  the open flags. See open(2)
 *
 * \retval   a 0 or positive file descriptor on success
 * \retval   a negative errno on error
 */
int llapi_open_by_fid(const struct lustre_fs_h *lfsh,
		      const lustre_fid *fid, int open_flags)
{
	char fidstr[FID_NOBRACE_LEN + 1];

	snprintf(fidstr, sizeof(fidstr), DFID_NOBRACE, PFID(fid));
	return openat(lfsh->fid_fd, fidstr, open_flags);
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
int llapi_fd2parent(int fd, unsigned int linkno, lustre_fid *parent_fid,
		    char *parent_name, size_t parent_name_len)
{
	/* Avoid allocating gp by adding PATH_MAX after it. */
	struct {
		struct getparent gp;
		char filler[PATH_MAX];
	} x;
	int rc;

	/* Even if the user doesn't want the path, we still have to
	 * get it. Make sure it's not going to break the request. */
	if (parent_name)
		x.gp.gp_name_size = parent_name_len;
	else
		x.gp.gp_name_size = sizeof(x.filler);

	x.gp.gp_linkno = linkno;

	rc = ioctl(fd, LL_IOC_GETPARENT, &x.gp);
	if (rc == 0) {
		if (parent_fid)
			*parent_fid = x.gp.gp_fid;
		if (parent_name) {
			/* We know the data fits. */
			strcpy(parent_name, x.gp.gp_name);
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
int llapi_fid2parent(const struct lustre_fs_h *lfsh,
		     const lustre_fid *fid,
		     unsigned int linkno,
		     lustre_fid *parent_fid,
		     char *parent_name, size_t parent_name_len)
{
	int fd;
	int rc;

	fd = llapi_open_by_fid(lfsh, fid, O_RDONLY | O_NONBLOCK | O_NOFOLLOW);
	if (fd == -1)
		return -errno;

	rc = llapi_fd2parent(fd, linkno,
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
int llapi_path2parent(const char *path, unsigned int linkno,
		      lustre_fid *parent_fid,
		      char *parent_name, size_t parent_name_len)
{
	int fd;
	int rc;

	fd = open(path, O_RDONLY | O_NONBLOCK | O_NOFOLLOW);
	if (fd == -1)
		return -errno;

	rc = llapi_fd2parent(fd, linkno,
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
	}
	else if (rc == sizeof(struct lustre_mdt_attrs)) {
		/* FID is stored in little endian. */
		fid->f_seq = le64toh(lma.lma_self_fid.f_seq);
		fid->f_oid = le32toh(lma.lma_self_fid.f_oid);
		fid->f_ver = le32toh(lma.lma_self_fid.f_ver);

		return 0;
	} else {
		/* Attribute is shorter. API change? */
		return -EINVAL;
	}
}

/**
 * Return a FID from an opened file descriptor.
 *
 * \param[in]   fd     an opened file descriptor for a file in the above
 *                     Lustre filesystem
 * \param[out]  fid    the requested parent fid
 */
int llapi_fd2fid(int fd, lustre_fid *fid)
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
int llapi_path2fid(const char *path, lustre_fid *fid)
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
		rc = llapi_fd2fid(fd, fid);
		close(fd);
	}

	return rc;
}

/**
 * Open an anonymous file. That file will be destroyed by Lustre when
 * the last reference to it is closed.
 *
 * \param  lfsh[in]          an opaque handle returned by lustre_open_fs()
 * \param  parent_fid[in]    the FID of a directory into which the file must be
 *                           created.
 * \param  mdt_idx[in]       the MDT index onto which create the file. To use a
 *                           default MDT, set it to -1.
 * \param  open_flags[in]    open flags, see open(2)
 * \param  mode[in]          open mode, see open(2)
 * \param  layout[in]        striping information. If it is NULL, then
 *                           the default for the directory is used.
 *
 * \retval   0 on success
 * \retval   a negative errno on error
 */
int llapi_create_volatile_by_fid(const struct lustre_fs_h *lfsh,
				 const lustre_fid *parent_fid,
				 int mdt_idx, int open_flags, mode_t mode,
				 const struct llapi_layout *layout)
{
	char path[PATH_MAX];
	int fd;
	int rc;
	int rnumber;

	rnumber = random();
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
	if (fd == -1) {
		rc = -errno;
		log_msg(LLAPI_MSG_ERROR, rc,
			"Cannot create volatile file '%s' under '%s'",
			path, lfsh->mount_path);
		return rc;
	}

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
int llapi_get_mdt_index_by_fid(const struct lustre_fs_h *lfsh,
			       const struct lu_fid *fid)
{
	int rc;

	rc = ioctl(lfsh->mount_fd, LL_IOC_FID2MDTIDX, fid);
	if (rc < 0)
		return -errno;

	return rc;
}

#ifdef UNIT_TEST
#include "../tests/liblustre_file2_tests.c"
#endif
