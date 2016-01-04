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
 * Tests FIDs functions.
 *
 * Assumption: lustre is mounted.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <check.h>
#include "check_extra.h"

#include "../lib/file.c"
#include "lib_test.h"

/* Test FID functions. */
void unittest_fid1(void)
{
	struct lus_fs_handle *lfsh;
	int fd;
	lustre_fid fid;
	int rc;
	struct stat stbuf;
	ssize_t sret;
	char buf[10] = { 0, };
	char fname[PATH_MAX];

	rc = snprintf(fname, sizeof(fname), "%s/unittest_fid", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(fname), "snprintf failed: %d", rc);

	/* Create a small file, get its FID, and close it. */
	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	sret = write(fd, buf, sizeof(buf));
	ck_assert_int_eq(sret, sizeof(buf));

	rc = lus_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);

	/* Stat the file by FID and ensure it worked. */
	memset(&stbuf, 0x55, sizeof(stbuf));
	rc = lus_stat_by_fid(lfsh, &fid, &stbuf);
	ck_assert_int_eq(rc, 0);
	ck_assert(S_ISREG(stbuf.st_mode));
	ck_assert_int_eq(stbuf.st_size, sizeof(buf));

	/* Open it agin by FID. */
	fd = lus_open_by_fid(lfsh, &fid, O_RDONLY);
	ck_assert_int_gt(fd, 0);
	close(fd);

	/* Fake file */
	fid.f_seq += 100;
	fd = lus_open_by_fid(lfsh, &fid, O_RDONLY);
	ck_assert_int_eq(fd, -ENOENT);

	unlink(fname);

	lus_close_fs(lfsh);
}

/* Test lus_fid2parent */
void unittest_fid2(void)
{
	struct lus_fs_handle *lfsh;
	int fd;
	lustre_fid mnt_fid;
	lustre_fid fid;
	lustre_fid parent_fid;
	int rc;
	char name[PATH_MAX];
	char fname[PATH_MAX];

	rc = snprintf(fname, sizeof(fname), "%s/unittest_fid", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(fname), "snprintf failed: %d", rc);

	/*
	 * On the mountpoint
	 */
	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	rc = lus_path2fid(lus_get_mountpoint(lfsh), &mnt_fid);
	ck_assert_int_eq(rc, 0);

	/* Doesn't work on mountpoint. */
	rc = lus_fid2parent(lfsh, &mnt_fid, 0, &parent_fid, NULL, 0);
	ck_assert_int_eq(rc, -ENODATA);

	/*
	 * On a regular file, under the mountpoint
	 */

	/* On a file */
	fd = open(fname, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = lus_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);

	rc = lus_fid2parent(lfsh, &fid, 0, NULL, NULL, 0);
	ck_assert_int_eq(rc, 0);

	rc = lus_fid2parent(lfsh, &fid, 0, NULL, NULL, sizeof(name));
	ck_assert_int_eq(rc, 0);

	memset(&parent_fid, 0xaa, sizeof(parent_fid));
	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, NULL, 0);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(memcmp(&parent_fid, &mnt_fid, sizeof(lustre_fid)), 0);

	memset(name, 0x5a, sizeof(name));
	memset(&parent_fid, 0xaa, sizeof(parent_fid));
	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, name, sizeof(name));
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(name, "unittest_fid");
	ck_assert_int_eq(memcmp(&parent_fid, &mnt_fid, sizeof(lustre_fid)), 0);

	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, name, 0);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, name, 5);
	ck_assert_int_eq(rc, -ENOSPC);

	memset(name, 0x5a, sizeof(name));
	memset(&parent_fid, 0xaa, sizeof(parent_fid));
	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, name, 50);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(name, "unittest_fid");
	ck_assert_int_eq(memcmp(&parent_fid, &mnt_fid, sizeof(lustre_fid)), 0);

	memset(name, 0x5a, sizeof(name));
	memset(&parent_fid, 0xaa, sizeof(parent_fid));
	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, name, PATH_MAX);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(name, "unittest_fid");
	ck_assert_int_eq(memcmp(&parent_fid, &mnt_fid, sizeof(lustre_fid)), 0);

	memset(name, 0x5a, sizeof(name));
	memset(&parent_fid, 0xaa, sizeof(parent_fid));
	rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid, name, PATH_MAX+1);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(name, "unittest_fid");
	ck_assert_int_eq(memcmp(&parent_fid, &mnt_fid, sizeof(lustre_fid)), 0);

	rc = unlink(fname);
	ck_assert(rc == 0);

	if (0) {
		/* Try with a symlink */
		/* TODO? this is not supported. */
		char fname_other[PATH_MAX];

		rc = snprintf(fname_other, sizeof(fname_other),
			      "%s/unittest_fid_other", lustre_dir);
		ck_assert_msg(rc > 0 && rc < sizeof(fname_other),
			      "snprintf failed: %d", rc);

		rc = unlink(fname);
		ck_assert(rc == 0 || errno == ENOENT);

		rc = unlink(fname_other);
		ck_assert(rc == 0 || errno == ENOENT);

		rc = symlink(fname_other, fname);
		ck_assert_int_eq(rc, 0);

		rc = lus_fid2parent(lfsh, &fid, 0, &parent_fid,
				    name, sizeof(name));
		ck_assert_int_eq(rc, 0);
	}

	lus_close_fs(lfsh);
}

/* Test lus_fid2path. */
void unittest_lus_fid2path(void)
{
	struct lus_fs_handle *lfsh;
	lustre_fid fid;
	int rc;
	char path[2*PATH_MAX];
	int fd;
	char fname[PATH_MAX];

	rc = snprintf(fname, sizeof(fname), "%s/unittest_fid", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(fname), "snprintf failed: %d", rc);

	/* On the mountpoint */
	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	rc = lus_path2fid(lus_get_mountpoint(lfsh), &fid);
	ck_assert_int_eq(rc, 0);

	rc = lus_fid2path(lfsh, &fid, path, 0, NULL, NULL);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = lus_fid2path(lfsh, &fid, path, sizeof(path), NULL, NULL);
	ck_assert_int_eq(rc, 0);

	/* On a file */
	fd = open(fname, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = lus_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);

	rc = lus_path2fid(fname, &fid);
	ck_assert_int_eq(rc, 0);

	rc = lus_fid2path(lfsh, &fid, path, 0, NULL, NULL);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = lus_fid2path(lfsh, &fid, path, 5, NULL, NULL);
	ck_assert_int_eq(rc, -ENOSPC);

	rc = lus_fid2path(lfsh, &fid, path, 50, NULL, NULL);
	ck_assert_int_eq(rc, 0);

	rc = lus_fid2path(lfsh, &fid, path, PATH_MAX, NULL, NULL);
	ck_assert_int_eq(rc, 0);

	rc = lus_fid2path(lfsh, &fid, path, PATH_MAX + 1, NULL, NULL);
	ck_assert_int_eq(rc, 0);

	rc = lus_fid2path(lfsh, &fid, path, sizeof(path), NULL, NULL);
	ck_assert_int_eq(rc, 0);

	rc = unlink(fname);
	ck_assert(rc == 0);

	lus_close_fs(lfsh);
}

/* Test lus_get_mdt_index_by_fid */
void unittest_mdt_index(void)
{
	struct lus_fs_handle *lfsh;
	lustre_fid fid;
	int rc;
	int fd;
	char fname[PATH_MAX];

	rc = snprintf(fname, sizeof(fname), "%s/unittest_fid", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(fname), "snprintf failed: %d", rc);

	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(fname, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = lus_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);
	unlink(fname);

	rc = lus_get_mdt_index_by_fid(lfsh, &fid);
	ck_assert_int_eq(rc, 0);

	lus_close_fs(lfsh);
}

/* Test lus_data_version_by_fd */
void unittest_lus_data_version_by_fd(void)
{
	struct lus_fs_handle *lfsh;
	int fd;
	int rc;
	ssize_t sret;
	char buf[10] = { 0, };
	uint64_t dv;
	uint64_t old_dv;
	char fname[PATH_MAX];

	rc = snprintf(fname, sizeof(fname), "%s/unittest_fid", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(fname), "snprintf failed: %d", rc);

	/* Create a small file, write several times and check that dv
	 * differs. */
	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = lus_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, 0);

	old_dv = dv;
	rc = lus_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(dv, old_dv);

	sret = write(fd, buf, sizeof(buf));
	ck_assert_int_eq(sret, sizeof(buf));
	sync();

	old_dv = dv;
	rc = lus_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_ne(dv, old_dv);

	close(fd);
	unlink(fname);

	lus_close_fs(lfsh);

	/* Bad fd */
	rc = lus_data_version_by_fd(876, 0, &dv);
	ck_assert_int_eq(rc, -EBADF);

	/* Non Lustre file. */
	fd = open("/dev/zero", O_RDONLY);
	rc = lus_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, -ENOTTY);
	close(fd);
}

/* Test lus_mdt_stat_by_fid */
void unittest_lus_mdt_stat_by_fid(void)
{
	struct lus_fs_handle *lfsh;
	int fd;
	int rc;
	char fname[PATH_MAX];
	struct stat buf1;
	struct stat buf2;
	lustre_fid fid;

	rc = snprintf(fname, sizeof(fname), "%s/aunittest_fid", lustre_dir);
	ck_assert_msg(rc > 0 && rc < sizeof(fname), "snprintf failed: %d", rc);

	/* Try stat and lus_mdt_stat_by_fid */
	rc = lus_open_fs(lustre_dir, &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = write(fd, fname, 100);
	ck_assert_int_eq(rc, 100);

	rc = fstat(fd, &buf1);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(buf1.st_size, 100);

	rc = lus_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	fsync(fd);
	close(fd);

	rc = lus_mdt_stat_by_fid(lfsh, &fid, &buf2);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(buf2.st_size, 100);

	lus_close_fs(lfsh);

	/* Compare both stat info */
	ck_assert_int_eq(buf1.st_dev, buf2.st_dev);
	ck_assert_int_eq(buf1.st_ino, buf2.st_ino);
	ck_assert_int_eq(buf1.st_mode, buf2.st_mode);
	ck_assert_int_eq(buf1.st_nlink, buf2.st_nlink);
	ck_assert_int_eq(buf1.st_uid, buf2.st_uid);
	ck_assert_int_eq(buf1.st_gid, buf2.st_gid);
}
