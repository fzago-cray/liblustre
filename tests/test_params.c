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
 * Assumptions: /mnt/lustre exists.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <check.h>

#include "../lib/params.c"

/* Not defined in check 0.9.8 - license is LGPL 2.1 or later */
#ifndef ck_assert_ptr_ne
#define _ck_assert_ptr(X, OP, Y) do { \
  const void* _ck_x = (X); \
  const void* _ck_y = (Y); \
  ck_assert_msg(_ck_x OP _ck_y, "Assertion '"#X#OP#Y"' failed: "#X"==%p, "#Y"==%p", _ck_x, _ck_y); \
} while (0)
#define ck_assert_ptr_eq(X, Y) _ck_assert_ptr(X, ==, Y)
#define ck_assert_ptr_ne(X, Y) _ck_assert_ptr(X, !=, Y)
#define ck_assert_int_gt(X, Y) _ck_assert_int(X, >, Y)
#endif

#define FNAME "/mnt/lustre/unittest_params"

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

	fd = open(FNAME, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	/* Read an uuid.
	 * uuid sample: b40310d6-d551-90d6-a220-6261f93e51d9  */

	/* Success */
	rc = get_param_lmv(fd, "uuid", &value);
	ck_assert_int_eq(rc, 0);
	ck_assert_ptr_ne(value, NULL);
	free(value);

	close(fd);
}
