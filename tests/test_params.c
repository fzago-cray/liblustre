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

/*
 * Tests some misc functions.
 *
 * Assumptions: lustre is mounted.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <check.h>
#include "check_extra.h"

#include "../lib/params.c"
#include "lib_test.h"

/* Test read_procfs_value */
void unittest_read_procfs_value(void)
{
	char *buf = NULL;
	int rc;

	/* /proc/fs/lustre/version is multiline, but get only the
	 * first one, which should be something like:
	 *   lustre: 2.7.51 */
	rc = read_procfs_value("", "", "version", &buf);
	ck_assert_int_eq(rc, 0);
	ck_assert_ptr_ne(buf, NULL);

	ck_assert(strncmp(buf, "lustre:", 7) == 0);

	free(buf);
}

/* Test get_param_lmv(). */
void unittest_param_lmv(void)
{
	int fd;
	int rc;
	char *value;
	char path[PATH_MAX];

	rc = snprintf(path, sizeof(path), "%s/unittest_params", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(path), "snprintf failed: %d", rc);

	fd = open(path, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	/* Read an uuid.
	 * uuid sample: b40310d6-d551-90d6-a220-6261f93e51d9  */

	/* Success */
	rc = get_param_lmv(fd, "uuid", &value);
	ck_assert_int_eq(rc, 0);
	ck_assert_ptr_ne(value, NULL);
	free(value);

	close(fd);
	unlink(path);
}
