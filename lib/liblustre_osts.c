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

#include <string.h>
#include <stdlib.h>
#include <glob.h>
#include <errno.h>
#include <limits.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/**
 * Returns the path to the pools for a filesystem.
 * eg: /proc/fs/lustre/lov/lustre-MDT0000-mdtlov/pools/
 *
 * \param path[out]    the path to the pool
 * \param pathlen[in]  length of path
 *
 * \retval 0           on success
 * \retval -EINVAL     if the path doesn't exist
 */
static int find_poolpath(const struct lustre_fs_h *lfsh, const char *poolname,
		  char *path, size_t pathlen)
{
	glob_t globbuf;
	char pattern[200];
	int rc;

        rc = snprintf(pattern, sizeof(pattern), "/proc/fs/lustre/lov/%s-*/pools/%s",
		      lfsh->fs_name, poolname);
	if (rc < 0 || rc >= sizeof(pattern))
		return -EINVAL;

        rc = glob(pattern, GLOB_NOSORT | GLOB_ONLYDIR, NULL, &globbuf);
	if (rc == GLOB_NOMATCH) {
		/* TODO: the original also used
		 * find_target_obdpath. Is that really necessary? */
		return -EINVAL;
	}

        rc = snprintf(path, pathlen, "%s", globbuf.gl_pathv[0]);
        globfree(&globbuf);

	if (rc < 0 || rc >= pathlen)
		return -EINVAL;

        return 0;
}

/**
 * Free a lustre_ost_info structure
 */
void free_ost_info(struct lustre_ost_info *info)
{
	int i;

	if (info == NULL)
		return;

	for(i=0; i<info->count; i++)
		free(info->osts[i]);

	free(info);
}

/**
 * Retrieve the list of OSTs in a pool from /proc
 */
int open_pool_info(const struct lustre_fs_h *lfsh, const char *poolname,
		   struct lustre_ost_info **info)
{
	char poolpath[PATH_MAX+1];
	struct lustre_ost_info *myinfo;
	FILE *f = NULL;
	int alloc_count;
	int rc;
	char *line;
	ssize_t sret;
	size_t len;

	rc = find_poolpath(lfsh, poolname, poolpath, sizeof(poolpath));
	if (rc != 0)
		return rc;

	alloc_count = 10;
	myinfo = malloc(sizeof(*info) + alloc_count * sizeof (char *));
	if (myinfo == NULL)
		return -ENOMEM;

	myinfo->count = 0;

	f = fopen(poolpath, "r");
	if (f == NULL) {
		rc = -errno;
		goto done;
	}

	alloc_count = 0;
	line = NULL;
	while((sret = getline(&line, &len, f)) != -1) {
		/* Realloc buffers if needed. */
		if (alloc_count == myinfo->count) {
			struct lustre_ost_info *newinfo;
			alloc_count += 10;
			newinfo = realloc(myinfo, sizeof(*info) + 
					  alloc_count * sizeof (char *));
			if (newinfo == NULL) {
				rc = -ENOMEM;
				goto done;
			}
		}

		/* Remove trailing \n if present. */
		if (len > 0 && line[len-1] == '\n')
			line[len-1] = '\0';

		myinfo->osts[myinfo->count] = line;
		myinfo->count ++;

		line = NULL;
	} 

	rc = 0;

done:
	if (f)
		fclose(f);

	if (rc == 0) {
		*info = myinfo;
	} else {
		free_ost_info(myinfo);
	}

	return rc;
}

#ifdef UNIT_TEST
#include "../tests/liblustre_osts_tests.c"
#endif
