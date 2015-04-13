/*
 * An alternate Lustre user library.
 *
 * Copyright Cray 2015, All rights reserved.
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
 * Retrieve the parent FID from a FID 
 * 
 * Note: this won't work on a symbolic link.
 */
int fid2parent(const struct lustre_fs_h *lfsh,
	       const lustre_fid *fid,
	       unsigned int linkno,
	       lustre_fid *parent_fid,
	       char *name, size_t name_size)
{
	/* Avoid allocating gp by allocating MAX_PATH after it. */
	struct {
		struct getparent gp;
		char filler[PATH_MAX];
	} x;
	int fd;
	int rc;

	/* Even if the user doesn't want the path, we still have to
	 * get it. Make sure it's not going to break the request. */
	if (name) {
		if (name_size > sizeof(x.filler))
			return -EOVERFLOW;
		x.gp.gp_name_size = name_size;
	} else {
		x.gp.gp_name_size = sizeof(x.filler);
	}

        x.gp.gp_linkno = linkno;

	fd = llapi_open_by_fid(lfsh, fid, O_RDONLY | O_NONBLOCK | O_NOFOLLOW);
	if (fd == -1) 
		return -errno;

        rc = ioctl(fd, LL_IOC_GETPARENT, &x.gp);
        if (rc == 0) {
		if (parent_fid)
			*parent_fid = x.gp.gp_fid;
		if (name) {
			/* We know the data fits. */
			strcpy(name, x.gp.gp_name);
		}
	} else {
		rc = -errno;
	}

	close(fd);

        return rc;
}

#ifdef UNIT_TEST
#include "../tests/liblustre_file2_tests.c"
#endif
