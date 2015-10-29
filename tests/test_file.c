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
 * Assumptions: /mnt/lustre exists.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <check.h>

#include "../lib/file.c"

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

#define FNAME "/mnt/lustre/unittest_fid"
#define FNAME_OTHER "/mnt/lustre/unittest_fid_other"

/* Test FID functions. */
void unittest_fid1(void)
{
	struct lustre_fs_h *lfsh;
	int fd;
	lustre_fid fid;
	int rc;
	struct stat stbuf;
	ssize_t sret;
	char buf[10] = { 0, };

	/* Create a small file, get its FID, and close it. */
	rc = llapi_open_fs("/mnt/lustre", &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(FNAME, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	sret = write(fd, buf, sizeof(buf));
	ck_assert_int_eq(sret, sizeof(buf));

	rc = llapi_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);

	/* Stat the file by FID and ensure it worked. */
	memset(&stbuf, 0x55, sizeof(stbuf));
	rc = llapi_stat_by_fid(lfsh, &fid, &stbuf);
	ck_assert_int_eq(rc, 0);
	ck_assert(S_ISREG(stbuf.st_mode));
	ck_assert_int_eq(stbuf.st_size, sizeof(buf));

	/* Open it agin by FID. */
	fd = llapi_open_by_fid(lfsh, &fid, O_RDONLY);
	ck_assert_int_gt(fd, 0);
	close(fd);

	/* Fake file */
	fid.f_seq += 100;
	fd = llapi_open_by_fid(lfsh, &fid, O_RDONLY);
	ck_assert_int_eq(fd, -1);
	ck_assert_int_eq(errno, ENOENT);

	llapi_close_fs(lfsh);
}

/* Test llapi_fid2parent */
void unittest_fid2(void)
{
	struct lustre_fs_h *lfsh;
	int fd;
	lustre_fid fid;
	lustre_fid parent_fid;
	int rc;
	char name[PATH_MAX];

	rc = llapi_open_fs("/mnt/lustre", &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(FNAME, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = llapi_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);

	/* Several times the same request, with varying arguments. */
	rc = llapi_fid2parent(lfsh, &fid, 0, &parent_fid, NULL, 0);
	ck_assert_int_eq(rc, 0);

	rc = llapi_fid2parent(lfsh, &fid, 0, NULL, NULL, 0);
	ck_assert_int_eq(rc, 0);

	rc = llapi_fid2parent(lfsh, &fid, 0, NULL, name, 5);
	ck_assert_int_eq(rc, -EOVERFLOW);

	rc = llapi_fid2parent(lfsh, &fid, 0, NULL, name, sizeof(name));
	ck_assert_int_eq(rc, 0);

	rc = llapi_fid2parent(lfsh, &fid, 0, &parent_fid, name, sizeof(name));
	ck_assert_int_eq(rc, 0);

	if (0) {
		/* Try with a symlink */
		/* TODO? this is not supported. */
		rc = unlink(FNAME);
		ck_assert(rc == 0 || errno == ENOENT);

		rc = unlink(FNAME_OTHER);
		ck_assert(rc == 0 || errno == ENOENT);

		rc = symlink(FNAME_OTHER, FNAME);
		ck_assert_int_eq(rc, 0);

		rc = llapi_fid2parent(lfsh, &fid, 0, &parent_fid, name, sizeof(name));
		ck_assert_int_eq(rc, 0);
	}

	llapi_close_fs(lfsh);
}

/* Test llapi_fid2path. */
void unittest_llapi_fid2path(void)
{
	struct lustre_fs_h *lfsh;
	lustre_fid fid;
	int rc;
	char path[PATH_MAX];

	rc = llapi_open_fs("/mnt/lustre", &lfsh);
	ck_assert_int_eq(rc, 0);

	rc = llapi_path2fid(llapi_get_mountpoint(lfsh), &fid);
	ck_assert_int_eq(rc, 0);

	rc = llapi_fid2path(lfsh, &fid, path, sizeof(path), NULL, NULL);
	ck_assert_int_eq(rc, 0);

	llapi_close_fs(lfsh);
}

/* Test llapi_get_mdt_index_by_fid */
void unittest_mdt_index(void)
{
	struct lustre_fs_h *lfsh;
	lustre_fid fid;
	int rc;
	int fd;

	rc = llapi_open_fs("/mnt/lustre", &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(FNAME, O_CREAT | O_TRUNC, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = llapi_fd2fid(fd, &fid);
	ck_assert_int_eq(rc, 0);

	close(fd);

	rc = llapi_get_mdt_index_by_fid(lfsh, &fid);
	ck_assert_int_eq(rc, 0);

	llapi_close_fs(lfsh);
}

/* Test llapi_data_version_by_fd */
void unittest_llapi_data_version_by_fd(void)
{
	struct lustre_fs_h *lfsh;
	int fd;
	int rc;
	ssize_t sret;
	char buf[10] = { 0, };
	uint64_t dv;
	uint64_t old_dv;

	/* Create a small file, write several times and check that dv
	 * differs. */
	rc = llapi_open_fs("/mnt/lustre", &lfsh);
	ck_assert_int_eq(rc, 0);

	fd = open(FNAME, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	ck_assert_int_gt(fd, 0);

	rc = llapi_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, 0);

	old_dv = dv;
	rc = llapi_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_eq(dv, old_dv);

	sret = write(fd, buf, sizeof(buf));
	ck_assert_int_eq(sret, sizeof(buf));
	sync();

	old_dv = dv;
	rc = llapi_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, 0);
	ck_assert_int_ne(dv, old_dv);

	close(fd);

	llapi_close_fs(lfsh);

	/* Bad fd */
	rc = llapi_data_version_by_fd(876, 0, &dv);
	ck_assert_int_eq(rc, -EBADF);

	/* Non Lustre file. */
	fd = open("/dev/zero", O_RDONLY);
	rc = llapi_data_version_by_fd(fd, 0, &dv);
	ck_assert_int_eq(rc, -ENOTTY);
	close(fd);
}
