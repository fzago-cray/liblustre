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

/* Tests OSTs functions.
 *
 * Assumptions: lustre is mounted, pool mypool exists and has at least
 * one OST in it.
 */

#include <limits.h>
#include <stdlib.h>

#include <check.h>
#include "check_extra.h"

#include "../lib/osts.c"
#include "lib_test.h"

/* Test find_poolpath */
void unittest_ost1(void)
{
	struct lustre_fs_h *lfsh;
	char poolpath[PATH_MAX];
	int rc;

	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	/* valid pool */
	rc = find_poolpath(lfsh, "mypool", poolpath, sizeof(poolpath));
	ck_assert_int_eq(rc, 0);

	/* Expect something like
	 * /proc/fs/lustre/lov/lustre-clilov-ffff88003803c000/pools/mypool */
	ck_assert(strlen(poolpath) > 20);

	/* valid pool but not enough space for result */
	rc = find_poolpath(lfsh, "mypool", poolpath, 10);
	ck_assert_int_eq(rc, -EINVAL);

	/* non existent pool */
	rc = find_poolpath(lfsh, "somepool", poolpath, sizeof(poolpath));
	ck_assert_int_eq(rc, -EINVAL);

	lus_close_fs(lfsh);
}

/* Test for open_pool_info */
void unittest_ost2(void)
{
	struct lustre_fs_h *lfsh;
	int rc;
	struct lustre_ost_info *info;
	char ostuuid[1000];

	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	/* valid, non empty pool */
	rc = open_pool_info(lfsh, "mypool", &info);
	ck_assert_int_eq(rc, 0);
	ck_assert_ptr_ne(info, NULL);
	ck_assert_int_gt(info->count, 0);

	rc = snprintf(ostuuid, sizeof(ostuuid), "%s-OST", lus_get_fsname(lfsh));
	ck_assert_msg(rc > 0 && rc < sizeof(ostuuid), "snprintf failed: %d", rc);
	ck_assert_int_eq(strncmp(info->osts[0], ostuuid, strlen(ostuuid)), 0);

	free_ost_info(&info);
	ck_assert_ptr_eq(info, NULL);

	/* Ensure this doesn't crash */
	free_ost_info(&info);

	lus_close_fs(lfsh);
}
