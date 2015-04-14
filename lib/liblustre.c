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

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <mntent.h>
#include <limits.h>
#include <sys/vfs.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/**
 * Removes trailing newlines from a string
 * \param buf  the string to modify
 */
void llapi_chomp_string(char *buf)
{
	if (buf == NULL)
		return;

	while(*buf && *buf != '\n')
		buf ++;

	if (*buf)
		*buf = '\0';
}

/**
 * Closes a Lustre filesystem opened with lustre_open_fs()
 *
 * \param lfsh	An opaque handle returned by lustre_open_fs()
 */
void lustre_close_fs(struct lustre_fs_h *lfsh)
{
	if (lfsh == NULL)
		return;

	free(lfsh->mount_path);

	if (lfsh->mount_fd != -1)
		close(lfsh->mount_fd);

	if (lfsh->fid_fd != -1)
		close(lfsh->fid_fd);
	free(lfsh);
}

/**
 * Register a new Lustre filesystem
 *
 * \param path   Lustre filesystem mountpoint
 *
 * \retval       An opaque handle, or NULL if an error occurred.
 */
int lustre_open_fs(const char *mount_path, struct lustre_fs_h **lfsh)
{
	struct lustre_fs_h *mylfsh;
	int rc;
	FILE *f = NULL;
	struct mntent *ent;
	char *p;
	struct statfs stfsbuf;

	*lfsh = NULL;

	mylfsh = calloc(1, sizeof(*mylfsh));
	if (mylfsh == NULL) {
		rc = -errno;
		goto fail;
	}

	mylfsh->mount_fd = -1;
	mylfsh->fid_fd = -1;

	mylfsh->mount_path = strdup(mount_path);
	if (mylfsh->mount_path == NULL) {
		rc = -errno;
		goto fail;
	}

	/* Remove extra slashes at the end of the mountpoint.
	 * /mnt/lustre/ --> /mnt/lustre */
	p = &mylfsh->mount_path[strlen(mylfsh->mount_path)-1];
	while(p != mylfsh->mount_path && *p == '/') {
		*p = '\0';
		p --;
	}

	/* Retrieve the lustre filesystem name from /etc/mtab. */
	f = setmntent(_PATH_MOUNTED, "r");
	if (f == NULL) {
		rc = -EINVAL;
		goto fail;
	}

	while ((ent = getmntent(f))) {
		size_t len;

		if ((strcmp(ent->mnt_dir, mylfsh->mount_path) != 0) ||
		    (strcmp(ent->mnt_type, "lustre") != 0))
			continue;

		/* Found it. The Lustre fsname is part of the fsname
		 * (ie. nodename@tcp:/lustre) so extract it. */
		p = strstr(ent->mnt_fsname, ":/");
		if (p == NULL)
			break;

		/* Skip :/ */
		p += 2;

		len = strlen(p);
		if (len >= 1 && len <= 8)
			strcpy(mylfsh->fs_name, p);

		break;
	}

	endmntent(f);

	if (mylfsh->fs_name[0] == '\0') {
		rc = -ENOENT;
		goto fail;
	}

	/* Open the mount point */
	mylfsh->mount_fd = open(mylfsh->mount_path, O_RDONLY | O_DIRECTORY);
	if (mylfsh->mount_fd == -1) {
		rc = -errno;
		goto fail;
	}

	/* Check it's indeed on Lustre */
	rc = fstatfs(mylfsh->mount_fd, &stfsbuf);
	if (rc == -1) {
		rc = -errno;
		goto fail;
	}
	if (stfsbuf.f_type != 0xbd00bd0) {
		rc = -EINVAL;
		goto fail;
	}

	/* Open the fid directory of the Lustre filesystem */
	mylfsh->fid_fd = openat(mylfsh->mount_fd, ".lustre/fid",
				O_RDONLY | O_DIRECTORY);
	if (mylfsh->fid_fd == -1) {
		rc = -errno;
		goto fail;
	}

	*lfsh = mylfsh;

	return 0;

fail:
	lustre_close_fs(mylfsh);
	return rc;
}

/* Accessor to return the Lustre filesystem name of an opened Lustre handle. */
const char *llapi_get_fsname(const struct lustre_fs_h *lfsh)
{
	return lfsh->fs_name;
}

/**
 * returns the HSM agent uuid
 */
/* TODO - do we even need this in the library? */
int llapi_get_agent_uuid(const struct lustre_fs_h *lfsh,
			 char *buf, size_t bufsize)
{
	return 0; /* TODO */
}

/********
 * TODO - The following has to be re-implemented - Nothing should
 * remain below that line
 ********/

bool fid_is_norm(const struct lu_fid *fid) { return false; }
bool fid_is_igif(const struct lu_fid *fid) { return false; }

/* TODO: doesn't layout have that? */
int llapi_stripe_limit_check(unsigned long long stripe_size, int stripe_offset,
				int stripe_count, int stripe_pattern)
{
	int page_size, rc;

	/* 64 KB is the largest common page size I'm aware of (on ia64), but
	 * check the local page size just in case. */
	page_size = LOV_MIN_STRIPE_SIZE;
	if (getpagesize() > page_size) {
		page_size = getpagesize();
		log_msg(LLAPI_MSG_WARN, 0,
			"warning: your page size (%u) is "
			"larger than expected (%u)", page_size,
			LOV_MIN_STRIPE_SIZE);
	}
	if (!llapi_stripe_size_is_aligned(stripe_size)) {
		rc = -EINVAL;
		log_msg(LLAPI_MSG_ERROR, rc, "error: bad stripe_size %llu, "
				"must be an even multiple of %d bytes",
				stripe_size, page_size);
		return rc;
	}
	if (!llapi_stripe_offset_is_valid(stripe_offset)) {
		rc = -EINVAL;
		log_msg(LLAPI_MSG_ERROR, rc, "error: bad stripe offset %d",
				stripe_offset);
		return rc;
	}
	if (!llapi_stripe_count_is_valid(stripe_count)) {
		rc = -EINVAL;
		log_msg(LLAPI_MSG_ERROR, rc, "error: bad stripe count %d",
				stripe_count);
		return rc;
	}
	if (llapi_stripe_size_is_too_big(stripe_size)) {
		rc = -EINVAL;
		log_msg(LLAPI_MSG_ERROR, rc,
				"warning: stripe size 4G or larger "
				"is not currently supported and would wrap");
		return rc;
	}
	return 0;
}
