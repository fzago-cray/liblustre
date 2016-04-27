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

/**
 * @file
 * @brief Interfaces to manage a Lustre filesystem
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <lustre/lustre.h>

#include "internal.h"

/**
 * Closes a Lustre filesystem opened with lus_open_fs()
 *
 * \param lfsh	An opaque handle returned by lus_open_fs()
 */
void lus_close_fs(struct lus_fs_handle *lfsh)
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

/* Retrieve the Lustre client version from /proc/fs/lustre/version,
 * and store the result in the handle. */
static int get_client_version(struct lus_fs_handle *lfsh)
{
	FILE *fp = NULL;
	char *line = NULL;
	int rc;
	size_t line_len;
	unsigned int major;
	unsigned int minor;
	unsigned int build;

	fp = fopen("/proc/fs/lustre/version", "r");
	if (fp == NULL) {
		rc = -ENOENT;
		goto out;
	}

	line_len = getline(&line, &line_len, fp);
	if (line_len == -1) {
		rc = -EINVAL;
		goto out;
	}

	rc = sscanf(line, "lustre: %u.%u.%u", &major, &minor, &build);
	if (rc != 3) {
		rc = -EINVAL;
		goto out;
	}

	lfsh->client_version = major * 10000 + minor * 100 + build;

	rc = 0;

out:
	free(line);

	if (fp != NULL)
		fclose(fp);

	return rc;
}

/**
 * Register a new Lustre filesystem
 *
 * \param[in]  mount_path   Lustre filesystem mountpoint
 * \param[out] lfsh	    An opaque handle
 *
 * \retval     An opaque handle, or NULL if an error occurred.
 */
int lus_open_fs(const char *mount_path, struct lus_fs_handle **lfsh)
{
	struct lus_fs_handle *mylfsh;
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

	rc = get_client_version(mylfsh);
	if (rc)
		goto fail;

	/* Refuse to run on old versions of Lustre. Must be at least
	 * 2.5. */
	if (mylfsh->client_version < 20500) {
		rc = -EINVAL;
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
	while (p != mylfsh->mount_path && *p == '/') {
		*p = '\0';
		p--;
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
	lus_close_fs(mylfsh);
	return rc;
}

/**
 * Accessor to return the Lustre filesystem name of an opened Lustre
 * handle.
 *
 * \param lfsh	An opaque handle returned by lus_open_fs()
 *
 * \retval      The Lustre filesystem name. Cannot be NULL.
 */
const char *lus_get_fsname(const struct lus_fs_handle *lfsh)
{
	return lfsh->fs_name;
}

/**
 * Accessor to return the Lustre filesystem mountpoint.
 * The returned mountpoint doesn't have trailing slashes.
 *
 * \param[in]  lfsh    an opened Lustre fs opaque handle
 *
 * \retval    the mountpoint
 */
const char *lus_get_mountpoint(const struct lus_fs_handle *lfsh)
{
	return lfsh->mount_path;
}

/**
 * Accessor to return the Lustre client version running locally.
 *
 * The number returned is major * 10000 + minor * 100 + patch For
 * instance Lustre 2.5.3 will return 20503.
 *
 * \param lfsh	An opaque handle returned by lus_open_fs()
 */
unsigned int lus_get_client_version(const struct lus_fs_handle *lfsh)
{
	return lfsh->client_version;
}
