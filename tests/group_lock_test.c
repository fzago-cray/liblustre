/*
 * An alternate Lustre user library.
 *
 * Copyright 2014, 2015 Cray Inc. All rights reserved.
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
 * Author: Frank Zago.
 *
 * The purpose of this test is to exert the group locks.
 *
 * The program will exit as soon as a non zero error code is returned.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <limits.h>
#include <check.h>

#include <lustre/lustre.h>

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
#define ck_assert_int_ge(X, Y) _ck_assert_int(X, >=, Y)
#define ck_assert_int_lt(X, Y) _ck_assert_int(X, <, Y)
#endif

/* Name of file/directory. Will be set once and will not change. */
static char mainpath[PATH_MAX];
static const char *maindir = "group_lock_test_name_9585766";
static char *lustre_dir;	/* Test directory inside Lustre */
struct lustre_fs_h *lfsh;

/* Cleanup our test file. */
static void cleanup(void)
{
	unlink(mainpath);
	rmdir(mainpath);
}

static void setup(void)
{
	cleanup();
}

static void teardown(void)
{
	cleanup();
}

/* Test lock / unlock */
START_TEST(test10)
{
	int rc;
	int fd;
	int gid;
	int i;

	cleanup();

	/* Create the test file, and open it. */
	fd = creat(mainpath, 0);
	ck_assert_int_ge(fd, 0);

	/* Valid command first. */
	gid = 1234;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd, gid);
	ck_assert_int_eq(rc, 0);

	/* Again */
	gid = 768;
	for (i = 0; i < 1000; i++) {
		rc = llapi_group_lock(fd, gid);
		ck_assert_int_eq(rc, 0);
		rc = llapi_group_unlock(fd, gid);
		ck_assert_int_eq(rc, 0);
	}

	/* Lock twice. */
	gid = 97486;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, -EINVAL);
	rc = llapi_group_unlock(fd, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd, gid);
	ck_assert_int_eq(rc, -EINVAL);

	/* 0 is an invalid gid */
	gid = 0;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, -EINVAL);

	/* Lock/unlock with a different gid */
	gid = 3543;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, 0);
	for (gid = -10; gid < 10; gid++) {
		rc = llapi_group_unlock(fd, gid);
		ck_assert_int_eq(rc, -EINVAL);
	}
	gid = 3543;
	rc = llapi_group_unlock(fd, gid);
	ck_assert_int_eq(rc, 0);

	close(fd);
}
END_TEST

/* Test open/lock/close without unlocking */
START_TEST(test11)
{
	int rc;
	int fd;
	int gid;
	char buf[10000];

	cleanup();

	/* Create the test file. */
	fd = creat(mainpath, 0);
	ck_assert_int_ge(fd, 0);
	memset(buf, 0x5a, sizeof(buf));
	rc = write(fd, buf, sizeof(buf));
	ck_assert_int_eq(rc, sizeof(buf));
	close(fd);

	/* Open/lock and close many times. Open with different
	 * flags. */
	for (gid = 1; gid < 10000; gid++) {
		int oflags = O_RDONLY;

		switch (gid % 10) {
		case 0:
			oflags = O_RDONLY;
			break;
		case 1:
			oflags = O_WRONLY;
			break;
		case 2:
			oflags = O_WRONLY | O_APPEND;
			break;
		case 3:
			oflags = O_WRONLY | O_CLOEXEC;
			break;
		case 4:
			oflags = O_WRONLY | O_DIRECT;
			break;
		case 5:
			oflags = O_WRONLY | O_NOATIME;
			break;
		case 6:
			oflags = O_WRONLY | O_SYNC;
			break;
		case 7:
			oflags = O_RDONLY | O_DIRECT;
			break;
		case 8:
			oflags = O_RDWR;
			break;
		case 9:
			oflags = O_RDONLY | O_LOV_DELAY_CREATE;
			break;
		}

		fd = open(mainpath, oflags);
		ck_assert_int_ge(fd, 0);

		rc = llapi_group_lock(fd, gid);
		ck_assert_int_eq(rc, 0);

		close(fd);
	}

	cleanup();
}
END_TEST

static void helper_test20(int fd)
{
	int gid;
	int rc;

	gid = 1234;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, -ENOTTY);

	gid = 0;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, -ENOTTY);

	gid = 1;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, -ENOTTY);

	gid = -1;
	rc = llapi_group_lock(fd, gid);
	ck_assert_int_eq(rc, -ENOTTY);
}

/* Test lock / unlock on a directory */
START_TEST(test20)
{
	int fd;
	int rc;
	char dname[PATH_MAX];

	cleanup();

	/* Try the mountpoint. Should fail. */
	fd = open(llapi_get_mountpoint(lfsh), O_RDONLY | O_DIRECTORY);
	ck_assert_int_ge(fd, 0);
	helper_test20(fd);
	close(fd);

	/* Try .lustre/ . Should fail. */
	rc = snprintf(dname, sizeof(dname), "%s/.lustre", llapi_get_mountpoint(lfsh));
	ck_assert_int_lt(rc, sizeof(dname));

	fd = open(llapi_get_mountpoint(lfsh), O_RDONLY | O_DIRECTORY);
	ck_assert_int_ge(fd, 0);
	helper_test20(fd);
	close(fd);

	/* A regular directory. */
	rc = mkdir(mainpath, 0600);
	ck_assert_int_eq(rc, 0);

	fd = open(mainpath, O_RDONLY | O_DIRECTORY);
	ck_assert_int_ge(fd, 0);
	helper_test20(fd);
	close(fd);
}
END_TEST

/* Test locking between several fds. */
START_TEST(test30)
{
	int fd1;
	int fd2;
	int gid;
	int gid2;
	int rc;

	cleanup();

	/* Create the test file, and open it. */
	fd1 = creat(mainpath, 0);
	ck_assert_int_ge(fd1, 0);

	/* Open a second time in non blocking mode. */
	fd2 = open(mainpath, O_RDWR | O_NONBLOCK);
	ck_assert_int_ge(fd2, 0);

	/* Valid command first. */
	gid = 1234;
	rc = llapi_group_lock(fd1, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd1, gid);
	ck_assert_int_eq(rc, 0);

	/* Lock on one fd, unlock on the other */
	gid = 6947556;
	rc = llapi_group_lock(fd1, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd2, gid);
	ck_assert_int_eq(rc, -EINVAL);
	rc = llapi_group_unlock(fd1, gid);
	ck_assert_int_eq(rc, 0);

	/* Lock from both */
	gid = 89489665;
	rc = llapi_group_lock(fd1, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_lock(fd2, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd2, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd1, gid);
	ck_assert_int_eq(rc, 0);

	/* Lock from both. Unlock in reverse order. */
	gid = 89489665;
	rc = llapi_group_lock(fd1, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_lock(fd2, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd1, gid);
	ck_assert_int_eq(rc, 0);
	rc = llapi_group_unlock(fd2, gid);
	ck_assert_int_eq(rc, 0);

	/* Try to lock with different gids */
	gid = 89489665;
	rc = llapi_group_lock(fd1, gid);
	ck_assert_int_eq(rc, 0);

	for (gid2 = -50; gid2 < 50; gid2++) {
		/* TODO: Ignore -1. Will succeed but shouldn't. Can't
		 * reproduce with upstream. */
		if (gid2 == -1)
			continue;

		rc = llapi_group_lock(fd2, gid2);
		if (gid2 == 0)
			ck_assert_int_eq(rc, -EINVAL);
		else
			ck_assert_int_eq(rc, -EAGAIN);
	}
	rc = llapi_group_unlock(fd1, gid);
	ck_assert_int_eq(rc, 0);

	close(fd1);
	close(fd2);
}
END_TEST

static Suite *gl_suite(void)
{
	TCase *tc;

	Suite *s = suite_create("GROUPLOCKS");

	tc = tcase_create("GROUPLOCKS");
	tcase_set_timeout(tc, 120);
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, test10);
	tcase_add_test(tc, test11);
	tcase_add_test(tc, test20);
	tcase_add_test(tc, test30);
	suite_add_tcase(s, tc);

	return s;
}

int main(int argc, char *argv[])
{
	int rc;
	int opt;
	int number_failed;
	Suite *s;
	SRunner *sr;

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

	rc = llapi_open_fs(lustre_dir, &lfsh);
	if (rc != 0) {
		fprintf(stderr, "Error: '%s' is not a Lustre filesystem\n",
			lustre_dir);
		return EXIT_FAILURE;
	}

	/* Create a test filename and reuse it. */
	rc = snprintf(mainpath, sizeof(mainpath), "%s/%s", lustre_dir, maindir);
	if (rc < 0 && rc >= sizeof(mainpath)) {
		fprintf(stderr, "Error: invalid name for mainpath\n");
		return EXIT_FAILURE;
	}

	s = gl_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}