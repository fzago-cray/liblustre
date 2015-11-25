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

#include <stdlib.h>
#include <limits.h>
#include <check.h>

#include <lustre/lustre.h>

#include "../lib/internal.h"
#include "support.h"

char *lustre_dir;               /* Lustre mountpoint */

START_TEST(ost1) { unittest_ost1(); } END_TEST
START_TEST(ost2) { unittest_ost2(); } END_TEST
START_TEST(fid1) { unittest_fid1(); } END_TEST
START_TEST(fid2) { unittest_fid2(); } END_TEST
START_TEST(chomp) { unittest_chomp(); } END_TEST
START_TEST(mdt_index) { unittest_mdt_index(); } END_TEST
START_TEST(param_lmv) { unittest_param_lmv(); } END_TEST
START_TEST(read_procfs_value) { unittest_read_procfs_value(); } END_TEST
START_TEST(parse_size) { unittest_llapi_parse_size(); } END_TEST
START_TEST(t_strscpy) { unittest_strscpy(); } END_TEST
START_TEST(t_strscat) { unittest_strscat(); } END_TEST
START_TEST(fid2path) { unittest_llapi_fid2path(); } END_TEST
START_TEST(data_version_by_fd) { unittest_llapi_data_version_by_fd(); } END_TEST

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
	tcase_add_test(tc, fid2path);
	suite_add_tcase(s, tc);

	tc = tcase_create("MISC");
	tcase_add_test(tc, chomp);
	tcase_add_test(tc, t_strscpy);
	tcase_add_test(tc, t_strscat);
	tcase_add_test(tc, mdt_index);
	tcase_add_test(tc, param_lmv);
	tcase_add_test(tc, read_procfs_value);
	tcase_add_test(tc, parse_size);
	tcase_add_test(tc, data_version_by_fd);
	suite_add_tcase(s, tc);

	return s;
}

int main(int argc, char *argv[])
{
	int number_failed;
	Suite *s = ost_suite();
	SRunner *sr = srunner_create(s);
	int opt;

	while ((opt = getopt(argc, argv, "d:")) != -1) {
		switch (opt) {
		case 'd':
			lustre_dir = optarg;
			break;
		case '?':
		default:
			fprintf(stderr, "Unknown option '%c'\n", optopt);
			fprintf(stderr, "Usage: %s [-d lustre_dir]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (lustre_dir == NULL)
		lustre_dir = "/mnt/lustre";

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
