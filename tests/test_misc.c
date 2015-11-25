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
#include <stdlib.h>

#include <check.h>
#include "check_extra.h"

#include "../lib/misc.c"

/* Test chomp_string */
void unittest_chomp(void)
{
	char buf[100];

	chomp_string(NULL);

	strcpy(buf, "hello\nbye\nbye");
	chomp_string(buf);
	ck_assert_str_eq(buf, "hello");

	strcpy(buf, "\n");
	chomp_string(buf);
	ck_assert_str_eq(buf, "");

	strcpy(buf, "\nhello");
	chomp_string(buf);
	ck_assert_str_eq(buf, "");

	strcpy(buf, "\nhello\n");
	chomp_string(buf);
	ck_assert_str_eq(buf, "");

	strcpy(buf, "hello\n\n\n");
	chomp_string(buf);
	ck_assert_str_eq(buf, "hello");
}
