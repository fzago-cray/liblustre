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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <glob.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/* TODO -- assume the path is in lustre, and the fsname is 'lustre'. */
int llapi_search_fsname(const char *pathname, char *fsname)
{
	strcpy(fsname, "lustre");
	return 0;
}

static int find_target_obdpath(const char *fsname, char *path)
{
        glob_t glob_info;
        char pattern[PATH_MAX];
        int rc;

        snprintf(pattern, PATH_MAX,
                 "/proc/fs/lustre/lov/%s-*/target_obd",
                 fsname);
        rc = glob(pattern, GLOB_BRACE, NULL, &glob_info);
        if (rc == GLOB_NOMATCH)
                return -ENODEV;
        else if (rc)
                return -EINVAL;

        strcpy(path, glob_info.gl_pathv[0]);
        globfree(&glob_info);
        return 0;
}

static int find_poolpath_org(const char *fsname, const char *poolname,
			 char *poolpath)
{
        glob_t glob_info;
        char pattern[PATH_MAX];
        int rc;

        snprintf(pattern, PATH_MAX,
                 "/proc/fs/lustre/lov/%s-*/pools/%s",
                 fsname, poolname);
        rc = glob(pattern, GLOB_BRACE, NULL, &glob_info);
        /* If no pools, make sure the lov is available */
        if ((rc == GLOB_NOMATCH) &&
            (find_target_obdpath(fsname, poolpath) == -ENODEV))
                return -ENODEV;
        if (rc)
                return -EINVAL;

        strcpy(poolpath, glob_info.gl_pathv[0]);
        globfree(&glob_info);
        return 0;
}

/*
 * if pool is NULL, search ostname in target_obd
 * if pool is not NULL:
 *  if pool not found returns errno < 0
 *  if ostname is NULL, returns 1 if pool is not empty and 0 if pool empty
 *  if ostname is not NULL, returns 1 if OST is in pool and 0 if not
 */
static int llapi_search_ost(const char *fsname, const char *poolname,
			    const char *ostname)
{
        FILE *fd;
        char buffer[PATH_MAX];
        int len = 0, rc;

        if (ostname != NULL)
                len = strlen(ostname);

	rc = find_poolpath_org(fsname, poolname, buffer);
        if (rc)
                return rc;

        fd = fopen(buffer, "r");
        if (fd == NULL)
                return -errno;

        while (fgets(buffer, sizeof(buffer), fd) != NULL) {
                if (poolname == NULL) {
                        char *ptr;
                        /* Search for an ostname in the list of OSTs
                         Line format is IDX: fsname-OSTxxxx_UUID STATUS */
                        ptr = strchr(buffer, ' ');
                        if ((ptr != NULL) &&
                            (strncmp(ptr + 1, ostname, len) == 0)) {
                                fclose(fd);
                                return 1;
                        }
                } else {
                        /* Search for an ostname in a pool,
                         (or an existing non-empty pool if no ostname) */
                        if ((ostname == NULL) ||
                            (strncmp(buffer, ostname, len) == 0)) {
                                fclose(fd);
                                return 1;
                        }
                }
        }
        fclose(fd);
        return 0;
}

/**
 * Open a Lustre file.
 *
 * \param name     the name of the file to be opened
 * \param flags    access mode, see flags in open(2)
 * \param mode     permission of the file if it is created, see mode in open(2)
 * \param param    stripe pattern of the newly created file
 *
 * \retval         file descriptor of opened file
 * \retval         negative errno on failure
 */
int llapi_file_open(const char *name, int flags, mode_t mode,
		    const struct llapi_stripe_param *param)
{
	char fsname[MAX_OBD_NAME + 1] = { 0 };
	const char *pool_name = param->lsp_pool;
	struct lov_user_md *lum = NULL;
	size_t lum_size = sizeof(*lum);
	int fd, rc;

	/* Make sure we are on a Lustre file system */
	rc = llapi_search_fsname(name, fsname);
	if (rc) {
		log_msg(LLAPI_MSG_ERROR, rc,
			    "'%s' is not on a Lustre filesystem",
			    name);
		return rc;
	}

	/* Check if the stripe pattern is sane. */
	rc = llapi_stripe_limit_check(param->lsp_stripe_size,
				      param->lsp_stripe_offset,
				      param->lsp_stripe_count,
				      param->lsp_stripe_pattern);
	if (rc != 0)
		return rc;

	/* Make sure we have a good pool */
	if (pool_name != NULL) {
		/* in case user gives the full pool name <fsname>.<poolname>,
		 * strip the fsname */
		char *ptr = strchr(pool_name, '.');
		if (ptr != NULL) {
			*ptr = '\0';
			if (strcmp(pool_name, fsname) != 0) {
				*ptr = '.';
				log_msg(LLAPI_MSG_ERROR, 0,
					"Pool '%s' is not on filesystem '%s'",
					pool_name, fsname);
				return -EINVAL;
			}
			pool_name = ptr + 1;
		}

		/* Make sure the pool exists and is non-empty */
		rc = llapi_search_ost(fsname, pool_name, NULL);
		if (rc < 1) {
			char *err = rc == 0 ? "has no OSTs" : "does not exist";

			log_msg(LLAPI_MSG_ERROR, 0, "pool '%s.%s' %s",
				fsname, pool_name, err);
			return -EINVAL;
		}

		lum_size = sizeof(struct lov_user_md_v3);
	}

	/* sanity check of target list */
	if (param->lsp_is_specific) {
		char ostname[MAX_OBD_NAME + 1];
		bool found = false;
		int i;

		for (i = 0; i < param->lsp_stripe_count; i++) {
			snprintf(ostname, sizeof(ostname), "%s-OST%04x_UUID",
				 fsname, param->lsp_osts[i]);
			rc = llapi_search_ost(fsname, pool_name, ostname);
			if (rc <= 0) {
				if (rc == 0)
					rc = -ENODEV;

				log_msg(LLAPI_MSG_ERROR, rc,
					    "%s: cannot find OST %s in %s",
					    __func__, ostname,
					    pool_name != NULL ?
					    "pool" : "system");
				return rc;
			}

			/* Make sure stripe offset is in OST list. */
			if (param->lsp_osts[i] == param->lsp_stripe_offset)
				found = true;
		}
		if (!found) {
			log_msg(LLAPI_MSG_ERROR, -EINVAL,
				    "%s: stripe offset '%d' is not in the "
				    "target list",
				    __func__, param->lsp_stripe_offset);
			return -EINVAL;
		}

		lum_size = lov_user_md_size(param->lsp_stripe_count,
					    LOV_USER_MAGIC_SPECIFIC);
	}

	lum = calloc(1, lum_size);
	if (lum == NULL)
		return -ENOMEM;

retry_open:
	fd = open(name, flags | O_LOV_DELAY_CREATE, mode);
	if (fd < 0) {
		if (errno == EISDIR && !(flags & O_DIRECTORY)) {
			flags = O_DIRECTORY | O_RDONLY;
			goto retry_open;
		}
	}

	if (fd < 0) {
		rc = -errno;
		log_msg(LLAPI_MSG_ERROR, rc, "unable to open '%s'", name);
		free(lum);
		return rc;
	}

	/*  Initialize IOCTL striping pattern structure */
	lum->lmm_magic = LOV_USER_MAGIC_V1;
	lum->lmm_pattern = param->lsp_stripe_pattern;
	lum->lmm_stripe_size = param->lsp_stripe_size;
	lum->lmm_stripe_count = param->lsp_stripe_count;
	lum->lmm_stripe_offset = param->lsp_stripe_offset;
	if (pool_name != NULL) {
		struct lov_user_md_v3 *lumv3 = (void *)lum;

		lumv3->lmm_magic = LOV_USER_MAGIC_V3;
		strncpy(lumv3->lmm_pool_name, pool_name, LOV_MAXPOOLNAME);
	}
	if (param->lsp_is_specific) {
		struct lov_user_md_v3 *lumv3 = (void *)lum;
		int i;

		lumv3->lmm_magic = LOV_USER_MAGIC_SPECIFIC;
		if (pool_name == NULL) {
			/* LOV_USER_MAGIC_SPECIFIC uses v3 format plus specified
			 * OST list, therefore if pool is not specified we have
			 * to pack a null pool name for placeholder. */
			memset(lumv3->lmm_pool_name, 0, LOV_MAXPOOLNAME);
		}

		for (i = 0; i < param->lsp_stripe_count; i++)
			lumv3->lmm_objects[i].l_ost_idx = param->lsp_osts[i];
	}

	if (ioctl(fd, LL_IOC_LOV_SETSTRIPE, lum) != 0) {
		char *errmsg = "stripe already set";

		rc = -errno;
		if (errno != EEXIST && errno != EALREADY)
			errmsg = strerror(errno);

		log_msg(LLAPI_MSG_ERROR, 0,
			"error on ioctl %#x for '%s' (%d): %s",
			LL_IOC_LOV_SETSTRIPE, name, fd, errmsg);

		close(fd);
		fd = rc;
	}

	free(lum);

	return fd;
}
