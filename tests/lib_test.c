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

#include <stdlib.h>
#include <limits.h>
#include <check.h>

#include <lustre/lustre.h>

#include "../lib/liblustre_internal.h"

START_TEST(ost1) { unittest_ost1(); } END_TEST
START_TEST(ost2) { unittest_ost2(); } END_TEST
START_TEST(fid1) { unittest_fid1(); } END_TEST
START_TEST(fid2) { unittest_fid2(); } END_TEST
START_TEST(chomp) { unittest_chomp(); } END_TEST

static Suite *ost_suite(void)
{
	TCase *tc;

	Suite *s = suite_create("LIBLUSTRE");

	tc = tcase_create("OSTS");
	tcase_add_test(tc, ost1);
	tcase_add_test(tc, ost2);
	suite_add_tcase(s, tc);

	tc = tcase_create("FID");
	tcase_add_test(tc, fid1);
	tcase_add_test(tc, fid2);
	suite_add_tcase(s, tc);

	tc = tcase_create("CHOMP");
	tcase_add_test(tc, chomp);
	suite_add_tcase(s, tc);

	return s;
}

int main(void)
{
	int number_failed;
	Suite *s = ost_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
