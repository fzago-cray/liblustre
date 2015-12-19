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
 * Tests for the FID related functions.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <check.h>
#include "check_extra.h"

#include <lustre/lustre.h>

/* Name of file/directory. Will be set once and will not change. */
static char mainpath[PATH_MAX];
static const char *maindir = "llapi_fid_test_name_9585766";

static char *lustre_dir;		/* Test directory inside Lustre */

struct lustre_fs_h *lfsh;

/* Cleanup our test directory. */
static void cleanup(void)
{
	char cmd[PATH_MAX];
	int rc;

	rc = snprintf(cmd, sizeof(cmd), "rm -rf -- '%s'", mainpath);
	ck_assert(rc > 0 && rc < sizeof(cmd));

	rc = system(cmd);
	ck_assert_int_ne(rc, -1);
	ck_assert_int_eq(WEXITSTATUS(rc), 0);
}

static void setup(void)
{
	cleanup();
}

static void teardown(void)
{
	cleanup();
}

/* Helper - call path2fid, fd2fid and fid2path against an existing
 * file/directory */
static void helper_fid2path(const char *filename, int fd)
{
	lustre_fid fid;
	lustre_fid fid2;
	lustre_fid fid3;
	char path[PATH_MAX];
	char path3[PATH_MAX];
	long long recno;
	unsigned int linkno;
	int rc;

	rc = llapi_path2fid(filename, &fid);
	ck_assert_int_eq(rc, 0);

	recno = -1;
	linkno = 0;
	rc = llapi_fid2path(lfsh, &fid, path, sizeof(path), &recno, &linkno);
	ck_assert_int_eq(rc, 0);

	/* Try fd2fid and check that the result is still the same. */
	if (fd != -1) {
		rc = llapi_fd2fid(fd, &fid3);
		ck_assert_int_eq(rc, 0);

		ck_assert_int_eq(memcmp(&fid, &fid3, sizeof(fid)), 0);
	}

	/* Pass the result back to fid2path and ensure the fid stays
	 * the same. */
	rc = snprintf(path3, sizeof(path3), "%s/%s",
		      lus_get_mountpoint(lfsh), path);
	ck_assert(rc > 0 && rc < sizeof(path3));
	rc = llapi_path2fid(path3, &fid2);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(memcmp(&fid, &fid2, sizeof(fid)), 0);
}

/* Test fid2path with helper */
START_TEST(test10a)
{
	/* Against Lustre root */
	helper_fid2path(lustre_dir, -1);
}
END_TEST

START_TEST(test10b)
{
	int rc;
	int fd;

	/* Against a regular file */
	fd = creat(mainpath, 0);
	ck_assert_int_ge(fd, 0);
	helper_fid2path(mainpath, fd);
	close(fd);
	rc = unlink(mainpath);
	ck_assert_int_eq(rc, 0);
}
END_TEST

START_TEST(test10c)
{
	int rc;

	/* Against a pipe */
	rc = mkfifo(mainpath, 0);
	ck_assert_int_eq(rc, 0);
	helper_fid2path(mainpath, -1);
	rc = unlink(mainpath);
	ck_assert_int_eq(rc, 0);

}
END_TEST

START_TEST(test10d)
{
	int rc;

	/* Against a directory */
	rc = mkdir(mainpath, 0);
	ck_assert_int_eq(rc, 0);
	helper_fid2path(mainpath, -1);
	rc = rmdir(mainpath);
	ck_assert_int_eq(rc, 0);
}
END_TEST

START_TEST(test10e)
{
	int rc;
	struct stat statbuf;

	/* Against a char device. Use same as /dev/null in case things
	 * go wrong. */
	rc = stat("/dev/null", &statbuf);
	ck_assert_int_eq(rc, 0);
	rc = mknod(mainpath, S_IFCHR, statbuf.st_rdev);
	ck_assert_int_eq(rc, 0);
	helper_fid2path(mainpath, -1);
	rc = unlink(mainpath);
	ck_assert_int_eq(rc, 0);
}
END_TEST

START_TEST(test10f)
{
	int rc;
	struct stat statbuf;

	/* Against a block device device. Reuse same dev. */
	rc = stat("/dev/null", &statbuf);
	ck_assert_int_eq(rc, 0);
	rc = mknod(mainpath, S_IFBLK, statbuf.st_rdev);
	ck_assert_int_eq(rc, 0);
	helper_fid2path(mainpath, -1);
	rc = unlink(mainpath);
	ck_assert_int_eq(rc, 0);
}
END_TEST

START_TEST(test10g)
{
	int rc;

	/* Against a socket. */
	rc = mknod(mainpath, S_IFSOCK, (dev_t)0);
	ck_assert_int_eq(rc, 0);
	helper_fid2path(mainpath, -1);
	rc = unlink(mainpath);
	ck_assert_int_eq(rc, 0);
}
END_TEST

/* Test against deleted files. */
START_TEST(test11)
{
	int rc;
	int fd;
	lustre_fid fid;
	char path[PATH_MAX];
	long long recno;
	unsigned int linkno;

	/* Against a regular file */
	fd = creat(mainpath, 0);
	ck_assert_int_ge(fd, 0);
	close(fd);

	rc = llapi_path2fid(mainpath, &fid);
	ck_assert_int_eq(rc, 0);

	rc = unlink(mainpath);
	ck_assert_int_eq(rc, 0);

	recno = -1;
	linkno = 0;
	rc = llapi_fid2path(lfsh, &fid, path,
			    sizeof(path), &recno, &linkno);
	ck_assert_int_eq(rc, -ENOENT);
}
END_TEST

/* Test volatile file. */
START_TEST(test12)
{
	int rc;
	int fd;
	int fd2;
	int fd3;
	lustre_fid fid;
	lustre_fid maindir_fid;

	rc = mkdir(mainpath, 0);
	ck_assert_int_eq(rc, 0);

	/* Get the fid of maindir, to create the volatile. */
	rc = llapi_path2fid(mainpath, &maindir_fid);
	ck_assert_int_eq(rc, 0);

	/* Against a volatile file */
	fd = llapi_create_volatile_by_fid(lfsh, &maindir_fid, -1, 0, 0600, NULL);
	ck_assert_int_ge(fd, 0);

	rc = llapi_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	/* Not many ways to test, except to open by fid. */
	fd2 = lus_open_by_fid(lfsh, &fid, 0600);
	ck_assert_int_ge(fd2, 0);

	close(fd);

	/* Check the file can still be opened, since fd2 is not
	 * closed. */
	fd3 = lus_open_by_fid(lfsh, &fid, 0600);
	ck_assert_int_ge(fd3, 0);

	close(fd2);
	close(fd3);

	/* The volatile file is gone now. */
	fd = lus_open_by_fid(lfsh, &fid, 0600);
	ck_assert_int_eq(fd, -ENOENT);
}
END_TEST

/* Test with sub directories */
START_TEST(test20)
{
	char testpath[PATH_MAX];
	size_t len;
	int dir_created = 0;
	int rc;

	rc = snprintf(testpath, sizeof(testpath), "%s", mainpath);
	ck_assert(rc > 0 && rc < sizeof(testpath));

	rc = mkdir(testpath, S_IRWXU);
	ck_assert_int_eq(rc, 0);

	len = strlen(testpath);

	/* Create subdirectories as long as we can. Each new subdir is
	 * "/x", so we need at least 3 characters left in testpath. */
	while (len <= sizeof(testpath) - 3) {
		strncat(testpath, "/x", 2);

		len += 2;

		rc = mkdir(testpath, S_IRWXU);
		ck_assert_int_eq(rc, 0);

		dir_created++;

		helper_fid2path(testpath, -1);
	}

	/* And test the last one. */
	helper_fid2path(testpath, -1);

	/* Make sure we have created enough directories. Even with a
	 * reasonably long mountpath, we should have created at least
	 * 2000. */
	ck_assert_int_ge(dir_created, 2000);
}
END_TEST

/* Test linkno from fid2path */
START_TEST(test30)
{
	/* Note that since the links are stored in the extended
	 * attributes, only a few of these will fit (about 150 in this
	 * test). Still, create more than that to ensure the system
	 * doesn't break. See LU-5746. */
	const int num_links = 1000;
	struct {
		char filename[PATH_MAX];
		bool seen;
	} *links;
	char buf[PATH_MAX];
	char buf2[PATH_MAX];
	lustre_fid fid;
	int rc;
	int i;
	int j;
	int fd;
	unsigned int linkno;
	bool past_link_limit = false;

	/* Create the containing directory. */
	rc = mkdir(mainpath, 0);
	ck_assert_int_eq(rc, 0);

	links = calloc(num_links, sizeof(*links));
	ck_assert_ptr_ne(links, NULL);

	/* Initializes the link array. */
	for (i = 0; i < num_links; i++) {
		rc = snprintf(links[i].filename, sizeof(links[i].filename),
			      "%s/%s/link%04d", lustre_dir, maindir, i);

		ck_assert(rc > 0 && rc < sizeof(links[i].filename));

		links[i].seen = false;
	}

	/* Create the original file. */
	fd = creat(links[0].filename, 0);
	ck_assert_int_ge(fd, 0);
	close(fd);

	rc = llapi_path2fid(links[0].filename, &fid);
	ck_assert_int_eq(rc, 0);

	/* Create the links */
	for (i = 1; i < num_links; i++) {
		rc = link(links[0].filename, links[i].filename);
		ck_assert_int_eq(rc, 0);
	}

	/* Query the links, making sure we got all of them */
	for (i = 0; i < num_links + 10; i++) {
		long long recno;
		bool found;

		/* Without braces */
		recno = -1;
		linkno = i;
		rc = llapi_fid2path(lfsh, &fid, buf,
				    sizeof(buf), &recno, &linkno);
		ck_assert_int_eq(rc, 0);

		snprintf(buf2, sizeof(buf2),
			 "%s/%s", lus_get_mountpoint(lfsh), buf);

		if (past_link_limit == false) {
			/* Find the name in the links that were created */
			found = false;
			for (j = 0; j < num_links; j++) {
				if (strcmp(buf2, links[j].filename) == 0) {
					ck_assert_int_eq(links[j].seen, false);
					links[j].seen = true;
					found = true;
					break;
				}
			}
			ck_assert_int_eq(found, true);

			if (linkno == i) {
				/* The linkno hasn't changed. This
				 * means it is the last entry
				 * stored. */
				past_link_limit = true;

				/* Also assume that some links were
				 * returned. It's hard to compute the
				 * exact value. */
				ck_assert_int_gt(i, 50);
			}
		} else {
			/* Past the number of links stored in the EA,
			 * Lustre will simply return the original
			 * file. */
			ck_assert_str_eq(buf2, links[0].filename);
		}
	}

	free(links);
}
END_TEST

/* Test llapi_fd2parent/llapi_path2parent on mainpath (whatever its
 * type). mainpath must exist. */
static void help_test40(void)
{
	lustre_fid parent_fid;
	lustre_fid fid2;
	char buf[PATH_MAX];
	int rc;
	lustre_fid maindir_fid;

	/* Get the fid of maindir, to get the parent. */
	rc = llapi_path2fid(mainpath, &maindir_fid);
	ck_assert_int_eq(rc, 0);

	/* Successful call */
	memset(buf, 0x55, sizeof(buf));
	rc = llapi_fid2parent(lfsh, &maindir_fid, 0, &parent_fid, buf, PATH_MAX);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(buf, maindir);

	/* By construction, mainpath is just under lustre_dir, so we
	 * can check that the parent fid of mainpath is indeed the one
	 * of lustre_dir. */
	rc = llapi_path2fid(lustre_dir, &fid2);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(memcmp(&parent_fid, &fid2, sizeof(fid2)), 0);

	/* Name too short */
	rc = llapi_path2parent(mainpath, 0, &parent_fid, buf, 0);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = llapi_path2parent(mainpath, 0, &parent_fid, buf, 5);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = llapi_path2parent(mainpath, 0, &parent_fid, buf, strlen(maindir));
	ck_assert_int_eq(rc, -ENOSPC);

	rc = llapi_path2parent(mainpath, 0, &parent_fid, buf,
			       strlen(maindir)+1);
	ck_assert_int_eq(rc, 0);
}

START_TEST(test40a)
{
	int rc;

	/* Against a directory. */
	rc = mkdir(mainpath, 0);
	ck_assert_int_eq(rc, 0);
	help_test40();

}
END_TEST

START_TEST(test40b)
{
	int fd;

	/* Against a regular file */
	fd = creat(mainpath, 0);
	ck_assert_int_ge(fd, 0);
	help_test40();
	close(fd);
}
END_TEST

/* Test llapi_fd2parent */
START_TEST(test41)
{
        int rc;
        int fd;
        int i;
	lustre_fid parent_fid;
	char name[PATH_MAX];

        /* Against a regular file */
        fd = creat(mainpath, 0);
        ck_assert_int_ge(fd, 0);

        /* Ask a few times */
        for (i = 0; i < 256; i++) {
                memset(name, i, sizeof(name)); /* poison */

		rc = llapi_fd2parent(fd, 0, &parent_fid, name, sizeof(name));
                ck_assert_int_eq(rc, 0);
                ck_assert_str_eq(name, maindir);
        }

        close(fd);
}
END_TEST

/* Test with linkno. Create sub directories, and put a link to the
 * original file in them. */
START_TEST(test42)
{

	const int num_links = 100;
	struct {
		char subdir[PATH_MAX];
		lustre_fid subdir_fid;
		char filename[PATH_MAX];
		bool seen;
	} links[num_links];
	char link0[PATH_MAX];
	char buf[PATH_MAX];
	int rc;
	int i;
	int fd;
	int linkno;
	lustre_fid parent_fid;

	/* Create the containing directory. */
	rc = mkdir(mainpath, 0);
	ck_assert_int_eq(rc, 0);

	/* Initializes the link array. */
	for (i = 0; i < num_links; i++) {
		rc = snprintf(links[i].subdir, sizeof(links[i].subdir),
			      "%s/sub%04d", mainpath, i);
		ck_assert(rc > 0 && rc < sizeof(links[i].subdir));

		rc = snprintf(links[i].filename, sizeof(links[i].filename),
			      "link%04d", i);
		ck_assert(rc > 0 && rc < sizeof(links[i].filename));

		links[i].seen = false;
	}

	/* Create the subdirectories. */
	for (i = 0; i < num_links; i++) {
		rc = mkdir(links[i].subdir, S_IRWXU);
		ck_assert_int_eq(rc, 0);

		rc = llapi_path2fid(links[i].subdir, &links[i].subdir_fid);
		ck_assert_int_eq(rc, 0);
	}

	/* Create the original file. */
	rc = snprintf(link0, sizeof(link0), "%s/%s",
		      links[0].subdir, links[0].filename);
	ck_assert(rc > 0 && rc < sizeof(link0));

	fd = creat(link0, 0);
	ck_assert_int_ge(fd, 0);
	close(fd);

	/* Create the links */
	for (i = 1; i < num_links; i++) {
		rc = snprintf(buf, sizeof(buf), "%s/%s",
			      links[i].subdir, links[i].filename);
		ck_assert(rc > 0 && rc < sizeof(buf));

		rc = link(link0, buf);
		ck_assert_int_eq(rc, 0);
	}

	/* Query the links, making sure we got all of them. Do it in
	 * reverse order, just because! */
	for (linkno = num_links-1; linkno >= 0; linkno--) {
		bool found;

		rc = llapi_path2parent(link0, linkno, &parent_fid, buf,
				       sizeof(buf));
		ck_assert_int_eq(rc, 0);

		/* Find the name in the links that were created */
		found = false;
		for (i = 0; i < num_links; i++) {
			if (memcmp(&parent_fid, &links[i].subdir_fid,
				   sizeof(parent_fid)) != 0)
				continue;

			ck_assert_str_eq(links[i].filename, buf);
			ck_assert_int_eq(links[i].seen, false);

			links[i].seen = true;
			found = true;
			break;
		}
		ck_assert_int_eq(found, true);
	}

	/* check non existent n+1 link */
	rc = llapi_path2parent(link0, num_links, &parent_fid, buf, sizeof(buf));
	ck_assert_int_eq(rc, -ENODATA);
}
END_TEST

static Suite *ost_suite(void)
{
	TCase *tc;

	Suite *s = suite_create("FIDS");

	tc = tcase_create("FIDS");
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, test10a);
	tcase_add_test(tc, test10b);
	tcase_add_test(tc, test10c);
	tcase_add_test(tc, test10d);
	tcase_add_test(tc, test10e);
	tcase_add_test(tc, test10f);
	tcase_add_test(tc, test10g);
	tcase_add_test(tc, test11);
	tcase_add_test(tc, test12);
	tcase_add_test(tc, test40a);
	tcase_add_test(tc, test40b);
	tcase_add_test(tc, test41);
	tcase_add_test(tc, test42);
	suite_add_tcase(s, tc);

	tc = tcase_create("FIDS_LONG");
	tcase_set_timeout(tc, 120);
	tcase_add_checked_fixture(tc, setup, teardown);
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

	rc = lus_open_fs(lustre_dir, &lfsh);
	if (rc != 0) {
		fprintf(stderr, "Error: '%s' is not a Lustre filesystem\n",
			lustre_dir);
		return EXIT_FAILURE;
	}

	/* Create a test filename and reuse it. Remove possibly old files. */
	rc = snprintf(mainpath, sizeof(mainpath), "%s/%s", lustre_dir, maindir);
	if (rc < 0 && rc >= sizeof(mainpath)) {
		fprintf(stderr, "Error: invalid name for mainpath\n");
		return EXIT_FAILURE;
	}

	s = ost_suite();
	sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	lus_close_fs(lfsh);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
