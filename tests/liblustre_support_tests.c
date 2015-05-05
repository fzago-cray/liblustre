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
 * Tests some miscellaneous functions.
 */

#include <stdlib.h>
#include <limits.h>
#include <check.h>

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

/* Test llapi_parse_string */
void unittest_llapi_parse_size(void)
{
	unsigned long long size_units = 1;
	unsigned long long size;
	int rc;
	const char *ext = "KkMmGgTtPpEe";
	int i;
	unsigned long long expect_unit;
	unsigned long long overflow_size;
	char args[1000];

	/* byte */
	size_units = 0;
	rc = llapi_parse_size("1453", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 1453);
	ck_assert_int_eq(size_units, true);

	size_units = 0;
	rc = llapi_parse_size("145", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 145);
	ck_assert_int_eq(size_units, true);

	/* b is block, not byte.*/
	size_units = 0;
	rc = llapi_parse_size("54353b", &size, &size_units, false);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 27828736);
	ck_assert_int_eq(size_units, 512);

	/* b is block, not byte, and number is too large.*/
	size_units = 0;
	rc = llapi_parse_size("145353452354354353b", &size, &size_units, false);
	ck_assert_int_eq(rc, -1);

	/* Others */
	expect_unit = 1024;
	overflow_size = 1125899906842624; /* 1024^5 */
	for (i=0; i<strlen(ext); i++) {

		/* Test uppercase letter, with bytes_spec=0 */
		sprintf(args, "12%c", ext[i]);
		size_units = 0;
		rc = llapi_parse_size(args, &size, &size_units, false);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 12*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test uppercase letter, with bytes_spec=1 */
		sprintf(args, "15%c", ext[i]);
		size_units = 0;
		rc = llapi_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 15*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test uppercase letter, with overflow */
		sprintf(args, "%llu%c", 16*overflow_size, ext[i]);
		size_units = 0;
		rc = llapi_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, -1);

		i++;

		/* Test lowercase letter, with bytes_spec=0 */
		sprintf(args, "11%c", ext[i]);
		size_units = 0;
		rc = llapi_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 11*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test lowercase letter, with bytes_spec=1 */
		sprintf(args, "15%c", ext[i]);
		size_units = 0;
		rc = llapi_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 15*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test lowercase letter, with overflow */
		sprintf(args, "%llu%c", 16*overflow_size, ext[i]);
		size_units = 0;
		rc = llapi_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, -1);

		expect_unit *= 1024;
		overflow_size /= 1024;
	}

	/* With leading spaces */
	size_units = 0;
	rc = llapi_parse_size("    87", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 87);
	ck_assert_int_eq(size_units, 1);

	/* Tests with no specifier. */
	size_units = 300;
	rc = llapi_parse_size("17", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 17*300);
	ck_assert_int_eq(size_units, 300);

	size_units = 2;
	rc = llapi_parse_size("17", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 17*2);
	ck_assert_int_eq(size_units, 2);

	/* Invalid strings or negative numbers */
	size_units = 0;
	rc = llapi_parse_size("hello", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = llapi_parse_size("17 17", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = llapi_parse_size("17hello", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = llapi_parse_size("17 b", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = llapi_parse_size("-19", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = llapi_parse_size(" -19", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = llapi_parse_size("", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	/* Hex and octal */
	size_units = 0;
	rc = llapi_parse_size("0x17", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 0x17);
	ck_assert_int_eq(size_units, 1);

	size_units = 0;
	rc = llapi_parse_size("017", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 017);
	ck_assert_int_eq(size_units, 1);
}
