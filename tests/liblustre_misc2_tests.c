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
