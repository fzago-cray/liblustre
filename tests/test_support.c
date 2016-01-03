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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <check.h>
#include "check_extra.h"

#include "support.h"
#include "support.c"

/* Test llapi_parse_size */
void unittest_lus_parse_size(void)
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
	rc = lus_parse_size("1453", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 1453);
	ck_assert_int_eq(size_units, true);

	size_units = 0;
	rc = lus_parse_size("145", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 145);
	ck_assert_int_eq(size_units, true);

	/* b is block, not byte.*/
	size_units = 0;
	rc = lus_parse_size("54353b", &size, &size_units, false);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 27828736);
	ck_assert_int_eq(size_units, 512);

	/* b is block, not byte, and number is too large.*/
	size_units = 0;
	rc = lus_parse_size("145353452354354353b", &size, &size_units, false);
	ck_assert_int_eq(rc, -1);

	/* Others */
	expect_unit = 1024;
	overflow_size = 1125899906842624; /* 1024^5 */
	for (i=0; i<strlen(ext); i++) {

		/* Test uppercase letter, with bytes_spec=0 */
		sprintf(args, "12%c", ext[i]);
		size_units = 0;
		rc = lus_parse_size(args, &size, &size_units, false);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 12*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test uppercase letter, with bytes_spec=1 */
		sprintf(args, "15%c", ext[i]);
		size_units = 0;
		rc = lus_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 15*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test uppercase letter, with overflow */
		sprintf(args, "%llu%c", 16*overflow_size, ext[i]);
		size_units = 0;
		rc = lus_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, -1);

		i++;

		/* Test lowercase letter, with bytes_spec=0 */
		sprintf(args, "11%c", ext[i]);
		size_units = 0;
		rc = lus_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 11*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test lowercase letter, with bytes_spec=1 */
		sprintf(args, "15%c", ext[i]);
		size_units = 0;
		rc = lus_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(size, 15*expect_unit);
		ck_assert_int_eq(size_units, expect_unit);

		/* Test lowercase letter, with overflow */
		sprintf(args, "%llu%c", 16*overflow_size, ext[i]);
		size_units = 0;
		rc = lus_parse_size(args, &size, &size_units, true);
		ck_assert_int_eq(rc, -1);

		expect_unit *= 1024;
		overflow_size /= 1024;
	}

	/* With leading spaces */
	size_units = 0;
	rc = lus_parse_size("    87", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 87);
	ck_assert_int_eq(size_units, 1);

	/* Tests with no specifier. */
	size_units = 300;
	rc = lus_parse_size("17", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 17*300);
	ck_assert_int_eq(size_units, 300);

	size_units = 2;
	rc = lus_parse_size("17", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 17*2);
	ck_assert_int_eq(size_units, 2);

	/* Invalid strings or negative numbers */
	size_units = 0;
	rc = lus_parse_size("hello", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = lus_parse_size("17 17", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = lus_parse_size("17hello", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = lus_parse_size("17 b", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = lus_parse_size("-19", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = lus_parse_size(" -19", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	size_units = 0;
	rc = lus_parse_size("", &size, &size_units, true);
	ck_assert_int_eq(rc, -1);

	/* Hex and octal */
	size_units = 0;
	rc = lus_parse_size("0x17", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 0x17);
	ck_assert_int_eq(size_units, 1);

	size_units = 0;
	rc = lus_parse_size("017", &size, &size_units, true);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(size, 017);
	ck_assert_int_eq(size_units, 1);
}

/* Test strscpy */
void unittest_strscpy(void)
{
	char dst[100];
	size_t rc;

	rc = strscpy(dst, "", 0);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = strscpy(dst, "", 1);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(dst, "");

	rc = strscpy(dst, "", 2);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(dst, "");

	rc = strscpy(dst, "", sizeof(dst));
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(dst, "");

	rc = strscpy(dst, "hello", sizeof(dst));
	ck_assert_int_eq(rc, 5);
	ck_assert_str_eq(dst, "hello");

	rc = strscpy(dst, "hello", 4);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = strscpy(dst, "hello", 5);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = strscpy(dst, "hello", 6);
	ck_assert_int_eq(rc, 5);
	ck_assert_str_eq(dst, "hello");
}

/* Test strscat */
void unittest_strscat(void)
{
	char dst[100];
	size_t rc;

	dst[0] = 0;

	rc = strscat(dst, "", sizeof(dst));
	ck_assert_int_eq(rc, 0);

	rc = strscat(dst, "a", sizeof(dst));
	ck_assert_int_eq(rc, 1);
	ck_assert_str_eq(dst, "a");

	rc = strscat(dst, "b", sizeof(dst));
	ck_assert_int_eq(rc, 2);
	ck_assert_str_eq(dst, "ab");

	rc = strscat(dst, "c", sizeof(dst));
	ck_assert_int_eq(rc, 3);
	ck_assert_str_eq(dst, "abc");

	strcpy(dst, "hello");

	rc = strscat(dst, "", sizeof(dst));
	ck_assert_int_eq(rc, 5);
	ck_assert_str_eq(dst, "hello");

	rc = strscat(dst, " me", sizeof(dst));
	ck_assert_int_eq(rc, 8);
	ck_assert_str_eq(dst, "hello me");
}
