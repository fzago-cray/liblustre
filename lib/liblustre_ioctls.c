/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.sun.com/software/products/lustre/docs/GPLv2.pdf
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2003, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, 2014, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 *
 * lustre/utils/liblustreapi.c
 *
 * Author: Peter J. Braam <braam@clusterfs.com>
 * Author: Phil Schwan <phil@clusterfs.com>
 * Author: Robert Read <rread@clusterfs.com>
 */

/* Simple functions that acts as an interface with the drivers through
 * ioctls. */

#include <stdlib.h>
#include <errno.h>
#include <endian.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/* Return a path given a FID. */
int llapi_fid2path(const struct lustre_fs_h *lfsh, const struct lu_fid *fid,
		   char *path, int path_len, long long *recno, int *linkno)
{
	struct getinfo_fid2path *gf;
	int rc;

	gf = malloc(sizeof(*gf) + path_len);
	if (gf == NULL)
		return -ENOMEM;
	gf->gf_fid = *fid;
	gf->gf_recno = recno ? *recno : -1;
	gf->gf_linkno = linkno ? *linkno : 0;
	gf->gf_pathlen = path_len;

	rc = ioctl(lfsh->mount_fd, OBD_IOC_FID2PATH, gf);
	if (rc == -1) {
		rc = -errno;
		if (rc != -ENOENT)
			log_msg(LLAPI_MSG_ERROR, rc, "ioctl err %d", rc);
	} else {
		memcpy(path, gf->gf_path, gf->gf_pathlen);
		if (path[0] == '\0') { /* ROOT path */
			path[0] = '/';
			path[1] = '\0';
		}
		if (recno)
			*recno = gf->gf_recno;
		if (linkno)
			*linkno = gf->gf_linkno;
	}

	free(gf);

	return rc;
}
