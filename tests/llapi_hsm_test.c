/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.gnu.org/licenses/gpl-2.0.html
 */

/*
 * Copyright 2014, 2015 Cray Inc. All rights reserved.
 * Copyright (c) 2015, Intel Corporation.
 */

/* The purpose of this test is to check some HSM functions. HSM must
 * be enabled before running it:
 *   echo enabled > /proc/fs/lustre/mdt/lustre-MDT0000/hsm_control
 */

/* All tests return 0 on success and non zero on error. The program will
 * exit as soon a non zero error is returned. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>

#include <check.h>
#include "check_extra.h"

#include <lustre/lustre.h>

static char *lustre_dir;               /* Test directory inside Lustre */

struct lustre_fs_h *lfsh;

/* Register and unregister 2000 times. Ensures there is no fd leak
 * since there is usually 1024 fd per process. */
START_TEST(test1)
{
	int i;
	int rc;
	struct lus_hsm_ct_handle *ctdata;

	for (i = 0; i < 2000; i++) {
		rc = lus_hsm_copytool_register(lfsh, 0, NULL, &ctdata);
		ck_assert_msg(rc == 0,
			"lus_hsm_copytool_register failed: %s, loop=%d",
			strerror(-rc), i);

		rc = lus_hsm_copytool_unregister(&ctdata);
		ck_assert_msg(rc == 0,
			"lus_hsm_copytool_unregister failed: %s, loop=%d",
			strerror(-rc), i);
	}
}
END_TEST

/* Re-register */
START_TEST(test2)
{
	int rc;
	struct lus_hsm_ct_handle *ctdata1;
	struct lus_hsm_ct_handle *ctdata2;

	rc = lus_hsm_copytool_register(lfsh, 0, NULL, &ctdata1);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_register failed: %s",
		strerror(-rc));

	rc = lus_hsm_copytool_register(lfsh, 0, NULL, &ctdata2);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_register failed: %s",
		strerror(-rc));

	rc = lus_hsm_copytool_unregister(&ctdata2);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_unregister failed: %s",
		strerror(-rc));

	rc = lus_hsm_copytool_unregister(&ctdata1);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_unregister failed: %s",
		strerror(-rc));
}
END_TEST

/* Bad parameters to lus_hsm_copytool_register(). */
START_TEST(test3)
{
	int rc;
	struct lus_hsm_ct_handle *ctdata;
	int archives[33];

	rc = lus_hsm_copytool_register(lfsh, 1, NULL, &ctdata);
	ck_assert_msg(rc == -EINVAL, "lus_hsm_copytool_register error: %s",
		strerror(-rc));

	rc = lus_hsm_copytool_register(lfsh, 33, NULL, &ctdata);
	ck_assert_msg(rc == -EINVAL, "lus_hsm_copytool_register error: %s",
		strerror(-rc));

	memset(archives, 1, sizeof(archives));
	rc = lus_hsm_copytool_register(lfsh, 34, archives, &ctdata);
	ck_assert_msg(rc == -EINVAL, "lus_hsm_copytool_register error: %s",
		strerror(-rc));

#if 0
	/* BUG? Should that fail or not? */
	rc = lus_hsm_copytool_register(lfsh, -1, NULL, &ctdata);
	ck_assert_msg(rc == -EINVAL, "lus_hsm_copytool_register error: %s",
		strerror(-rc));
#endif

	memset(archives, -1, sizeof(archives));
	rc = lus_hsm_copytool_register(lfsh, 1, archives, &ctdata);
	ck_assert_msg(rc == -EINVAL, "lus_hsm_copytool_register error: %s",
		strerror(-rc));
}
END_TEST

/* Bad parameters to lus_hsm_copytool_unregister(). */
START_TEST(test4)
{
	int rc;

	rc = lus_hsm_copytool_unregister(NULL);
	ck_assert_msg(rc == -EINVAL, "lus_hsm_copytool_unregister error: %s",
		strerror(-rc));
}
END_TEST

/* Test lus_hsm_copytool_recv in non blocking mode */
START_TEST(test5)
{
	int rc;
	int i;
	struct lus_hsm_ct_handle *ctdata;
	const struct hsm_action_list *hal;
	size_t msgsize;

	rc = lus_hsm_copytool_register(lfsh, 0, NULL, &ctdata);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_unregister failed: %s",
		strerror(-rc));

	/* Hopefully there is nothing lingering */
	for (i = 0; i < 1000; i++) {
		rc = lus_hsm_copytool_recv(ctdata, &hal, &msgsize);
		ck_assert_msg(rc == -EWOULDBLOCK,
			      "lus_hsm_copytool_recv error: %s",
			strerror(-rc));
	}

	rc = lus_hsm_copytool_unregister(&ctdata);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_unregister failed: %s",
		strerror(-rc));
}
END_TEST

/* Test polling (without actual traffic) */
START_TEST(test7)
{
	int rc;
	struct lus_hsm_ct_handle *ctdata;
	const struct hsm_action_list *hal;
	size_t msgsize;
	int fd;
	struct pollfd fds[1];

	rc = lus_hsm_copytool_register(lfsh, 0, NULL, &ctdata);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_register failed: %s",
		strerror(-rc));

	fd = lus_hsm_copytool_get_fd(ctdata);
	ck_assert_msg(fd >= 0, "lus_hsm_copytool_get_fd failed: %s",
		strerror(-rc));

	/* Ensure it's read-only */
	rc = write(fd, &rc, 1);
	ck_assert_msg(rc == -1 && errno == EBADF, "write error: %d, %s",
		rc, strerror(errno));

	rc = lus_hsm_copytool_recv(ctdata, &hal, &msgsize);
	ck_assert_msg(rc == -EWOULDBLOCK, "lus_hsm_copytool_recv error: %s",
		strerror(-rc));

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	rc = poll(fds, 1, 10);
	ck_assert_msg(rc == 0, "poll failed: %d, %s",
		rc, strerror(errno)); /* no event */

	rc = lus_hsm_copytool_unregister(&ctdata);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_unregister failed: %s",
		strerror(-rc));
}
END_TEST

/* Create the testfile of a given length. It returns a valid file
 * descriptor. */
static char testfile[PATH_MAX];
static int create_testfile(size_t length)
{
	int rc;
	int fd;

	rc = snprintf(testfile, sizeof(testfile), "%s/hsm_check_test",
		      lustre_dir);
	ck_assert_msg((rc > 0 && rc < sizeof(testfile)), "invalid name for testfile");

	/* Remove old test file, if any. */
	unlink(testfile);

	/* Use truncate so we can create a file (almost) as big as we
	 * want, while taking 0 bytes of data. */
	fd = creat(testfile, S_IRWXU);
	ck_assert_msg(fd >= 0, "create failed for '%s': %s",
		testfile, strerror(errno));

	rc = ftruncate(fd, length);
	ck_assert_msg(rc == 0, "ftruncate failed for '%s': %s",
		testfile, strerror(errno));

	return fd;
}

/* Test llapi_hsm_state_get. */
START_TEST(test50)
{
	struct hsm_user_state hus;
	int rc;
	int fd;

	fd = create_testfile(100);

	/* With fd variant */
	rc = llapi_hsm_state_get_fd(fd, &hus);
	ck_assert_msg(rc == 0, "llapi_hsm_state_get_fd failed: %s", strerror(-rc));
	ck_assert_msg(hus.hus_states == 0, "state=%u", hus.hus_states);

	rc = close(fd);
	ck_assert_msg(rc == 0, "close failed: %s", strerror(errno));

	/* Without fd */
	rc = llapi_hsm_state_get(testfile, &hus);
	ck_assert_msg(rc == 0, "llapi_hsm_state_get failed: %s", strerror(-rc));
	ck_assert_msg(hus.hus_states == 0, "state=%u", hus.hus_states);

	memset(&hus, 0xaa, sizeof(hus));
	rc = llapi_hsm_state_get(testfile, &hus);
	ck_assert_msg(rc == 0, "llapi_hsm_state_get failed: %s", strerror(-rc));
	ck_assert_msg(hus.hus_states == 0, "state=%u", hus.hus_states);
	ck_assert_msg(hus.hus_archive_id == 0, "archive_id=%u", hus.hus_archive_id);
	ck_assert_msg(hus.hus_in_progress_state == 0, "hus_in_progress_state=%u",
		hus.hus_in_progress_state);
	ck_assert_msg(hus.hus_in_progress_action == 0, "hus_in_progress_action=%u",
		hus.hus_in_progress_action);
}
END_TEST

/* Test llapi_hsm_state_set. */
START_TEST(test51)
{
	int rc;
	int fd;
	int i;
	struct hsm_user_state hus;

	fd = create_testfile(100);

	rc = llapi_hsm_state_set_fd(fd, 0, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	/* Set archive id */
	for (i = 0; i <= 32; i++) {
		rc = llapi_hsm_state_set_fd(fd, HS_EXISTS, 0, i);
		ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s",
			strerror(-rc));

		rc = llapi_hsm_state_get_fd(fd, &hus);
		ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s",
			strerror(-rc));
		ck_assert_msg(hus.hus_states == HS_EXISTS, "state=%u",
			hus.hus_states);
		ck_assert_msg(hus.hus_archive_id == i, "archive_id=%u, i=%d",
			hus.hus_archive_id, i);
	}

	/* Invalid archive numbers */
	rc = llapi_hsm_state_set_fd(fd, HS_EXISTS, 0, 33);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_EXISTS, 0, 151);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_EXISTS, 0, -1789);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd: %s", strerror(-rc));

	/* Settable flags, with respect of the HSM file state transition rules:
	 *	DIRTY without EXISTS: no dirty if no archive was created
	 *	DIRTY and RELEASED: a dirty file could not be released
	 *	RELEASED without ARCHIVED: do not release a non-archived file
	 *	LOST without ARCHIVED: cannot lost a non-archived file.
	 */
	rc = llapi_hsm_state_set_fd(fd, HS_DIRTY, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_EXISTS, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd failed: %s",
		strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_DIRTY, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_EXISTS, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_DIRTY, 0, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd failed: %s",
		strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_RELEASED, 0, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd failed: %s",
		strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_LOST, 0, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd failed: %s",
		strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_ARCHIVED, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_RELEASED, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_LOST, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_DIRTY|HS_EXISTS, 0, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd failed: %s",
		strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_RELEASED, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_DIRTY|HS_EXISTS, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_ARCHIVED, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd failed: %s",
		strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_LOST, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_ARCHIVED, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_NORELEASE, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_NORELEASE, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, HS_NOARCHIVE, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0, HS_NOARCHIVE, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_state_set_fd failed: %s", strerror(-rc));

	/* Bogus flags for good measure. */
	rc = llapi_hsm_state_set_fd(fd, 0x00080000, 0, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd: %s", strerror(-rc));

	rc = llapi_hsm_state_set_fd(fd, 0x80000000, 0, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_state_set_fd: %s", strerror(-rc));

	close(fd);
}
END_TEST

/* Test llapi_hsm_current_action */
START_TEST(test52)
{
	int rc;
	int fd;
	struct hsm_current_action hca;

	/* No fd equivalent, so close it. */
	fd = create_testfile(100);
	close(fd);

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s", strerror(-rc));
	ck_assert_msg(hca.hca_state, "hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action, "hca_state=%u", hca.hca_action);
}
END_TEST

/* Helper to simulate archiving a file. No actual data movement
 * happens. */
static void
helper_archiving(void (*progress)(struct lus_hsm_action_handle *hcp,
				  size_t length),
		 const size_t length)
{
	int rc;
	int fd;
	struct lus_hsm_ct_handle *ctdata;
	struct hsm_user_request	*hur;
	const struct hsm_action_list	*hal;
	const struct hsm_action_item	*hai;
	size_t msgsize;
	struct lus_hsm_action_handle *hcp;
	struct hsm_user_state hus;

	fd = create_testfile(length);

	rc = lus_hsm_copytool_register(lfsh, 0, NULL, &ctdata);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_register failed: %s",
		      strerror(-rc));

	/* Create and send the archive request. */
	hur = calloc(1, llapi_hsm_user_request_len(1, 0));
	ck_assert_msg(hur != NULL, "calloc returned NULL");

	hur->hur_request.hr_action = HUA_ARCHIVE;
	hur->hur_request.hr_archive_id = 1;
	hur->hur_request.hr_itemcount = 1;
	hur->hur_user_item[0].hui_extent.length = -1;

	rc = lus_fd2fid(fd, &hur->hur_user_item[0].hui_fid);
	ck_assert_msg(rc == 0, "lus_fd2fid failed: %s", strerror(-rc));

	close(fd);

	rc = llapi_hsm_request(lfsh, hur);
	ck_assert_msg(rc == 0, "llapi_hsm_request failed: %s", strerror(-rc));

	free(hur);

	/* Read the request */
	{
		struct pollfd fds[1];

		fds[0].fd = lus_hsm_copytool_get_fd(ctdata);
		ck_assert_msg(fds[0].fd >= 0,
			      "lus_hsm_copytool_get_fd failed: %s",
			      strerror(-rc));
		fds[0].events = POLLIN;

		rc = poll(fds, 1, 5 * 1000);
		ck_assert_msg(rc == 1, "poll failed: %d %s", rc, strerror(errno));
		ck_assert_msg(fds[0].revents == POLLIN,
			      "bad poll revents: %d", fds[0].revents);
	}

	rc = lus_hsm_copytool_recv(ctdata, &hal, &msgsize);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_recv failed: %s",
		      strerror(-rc));
	ck_assert_msg(hal->hal_count == 1, "hal_count=%d", hal->hal_count);

	hai = lus_hsm_hai_first(hal);
	ck_assert_msg(hai != NULL, "lus_hsm_hai_first returned NULL");
	ck_assert_msg(hai->hai_action == HSMA_ARCHIVE,
		      "hai_action=%d", hai->hai_action);

	/* "Begin" archiving */
	hcp = NULL;
	rc = lus_hsm_action_begin(&hcp, ctdata, hai, -1, 0, false);
	ck_assert_msg(rc == 0,
		      "lus_hsm_action_begin failed: %s", strerror(-rc));
	ck_assert_msg(hcp != NULL, "hcp is NULL");

	if (progress)
		progress(hcp, length);

	/* Done archiving */
	rc = llapi_hsm_action_end(&hcp, &hai->hai_extent, 0, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_end failed: %s", strerror(-rc));
	ck_assert_msg(hcp == NULL, "hcp is NULL");

	/* Close HSM client */
	rc = lus_hsm_copytool_unregister(&ctdata);
	ck_assert_msg(rc == 0, "lus_hsm_copytool_unregister failed: %s",
		      strerror(-rc));

	/* Final check */
	rc = llapi_hsm_state_get(testfile, &hus);
	ck_assert_msg(rc == 0, "llapi_hsm_state_get failed: %s", strerror(-rc));
	ck_assert_msg(hus.hus_states == (HS_EXISTS | HS_ARCHIVED),
		      "state=%u", hus.hus_states);
}

/* Simple archive. No progress. */
START_TEST(test100)
{
	const size_t length = 100;

	helper_archiving(NULL, length);
}
END_TEST

/* Archive, with a report every byte. */
static void test101_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int i;
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	/* Report progress. 1 byte at a time :) */
	for (i = 0; i < length; i++) {
		he.offset = i;
		he.length = 1;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));
	}

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == length,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test101)
{
	const size_t length = 1000;

	helper_archiving(test101_progress, length);
}
END_TEST

/* Archive, with a report every byte, backwards. */
static void test102_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int i;
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	/* Report progress. 1 byte at a time :) */
	for (i = length-1; i >= 0; i--) {
		he.offset = i;
		he.length = 1;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));
	}

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == length,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test102)
{
	const size_t length = 1000;

	helper_archiving(test102_progress, length);
}
END_TEST

/* Archive, with a single report. */
static void test103_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	he.offset = 0;
	he.length = length;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
		strerror(-rc));

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == length,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test103)
{
	const size_t length = 1000;

	helper_archiving(test103_progress, length);
}
END_TEST

/* Archive, with 2 reports. */
static void test104_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	he.offset = 0;
	he.length = length/2;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
		strerror(-rc));

	he.offset = length/2;
	he.length = length/2;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
		strerror(-rc));

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == length,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test104)
{
	const size_t length = 1000;

	helper_archiving(test104_progress, length);
}
END_TEST

/* Archive, with 1 bogus report. */
static void test105_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	he.offset = 2*length;
	he.length = 10*length;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
		strerror(-rc));

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);

	/* BUG - offset should be 2*length, or length should
	 * be 8*length */
	ck_assert_msg(hca.hca_location.length == 10*length,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test105)
{
	const size_t length = 1000;

	helper_archiving(test105_progress, length);
}
END_TEST

/* Archive, with 1 empty report. */
static void test106_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	he.offset = 0;
	he.length = 0;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
		strerror(-rc));

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == 0,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test106)
{
	const size_t length = 1000;

	helper_archiving(test106_progress, length);
}
END_TEST

/* Archive, with 1 bogus report. */
static void test107_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	he.offset = -1;
	he.length = 10;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == -EINVAL, "llapi_hsm_action_progress error: %s",
		strerror(-rc));

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == 0,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test107)
{
	const size_t length = 1000;

	helper_archiving(test107_progress, length);
}
END_TEST

/* Archive, with same report, many times. */
static void test108_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	int i;
	struct hsm_current_action hca;

	for (i = 0; i < 1000; i++) {
		he.offset = 0;
		he.length = length;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));
	}

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == length,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test108)
{
	const size_t length = 1000;

	helper_archiving(test108_progress, length);
}
END_TEST

/* Archive, 1 report, with large number. */
static void test109_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	struct hsm_extent he;
	struct hsm_current_action hca;

	he.offset = 0;
	he.length = 0xffffffffffffffffULL;
	rc = llapi_hsm_action_progress(hcp, &he, length, 0);
	ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
		strerror(-rc));

	rc = llapi_hsm_current_action(testfile, &hca);
	ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
		strerror(-rc));
	ck_assert_msg(hca.hca_state == HPS_RUNNING,
		"hca_state=%u", hca.hca_state);
	ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
		"hca_state=%u", hca.hca_action);
	ck_assert_msg(hca.hca_location.length == 0xffffffffffffffffULL,
		"length=%llu", (unsigned long long)hca.hca_location.length);
}

START_TEST(test109)
{
	const size_t length = 1000;

	helper_archiving(test109_progress, length);
}
END_TEST

/* Archive, with 10 reports, checking progress. */
static void test110_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	int i;
	struct hsm_extent he;
	struct hsm_current_action hca;

	for (i = 0; i < 10; i++) {
		he.offset = i*length/10;
		he.length = length/10;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));

		rc = llapi_hsm_current_action(testfile, &hca);
		ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
			strerror(-rc));
		ck_assert_msg(hca.hca_state == HPS_RUNNING,
			"hca_state=%u", hca.hca_state);
		ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
			"hca_state=%u", hca.hca_action);
		ck_assert_msg(hca.hca_location.length == (i+1)*length/10,
			"i=%d, length=%llu",
			i, (unsigned long long)hca.hca_location.length);
	}
}

START_TEST(test110)
{
	const size_t length = 1000;

	helper_archiving(test110_progress, length);
}
END_TEST

/* Archive, with 10 reports in reverse order, checking progress. */
static void test111_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	int i;
	struct hsm_extent he;
	struct hsm_current_action hca;

	for (i = 0; i < 10; i++) {
		he.offset = (9-i)*length/10;
		he.length = length/10;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));

		rc = llapi_hsm_current_action(testfile, &hca);
		ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
			strerror(-rc));
		ck_assert_msg(hca.hca_state == HPS_RUNNING,
			"hca_state=%u", hca.hca_state);
		ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
			"hca_state=%u", hca.hca_action);
		ck_assert_msg(hca.hca_location.length == (i+1)*length/10,
			"i=%d, length=%llu",
			i, (unsigned long long)hca.hca_location.length);
	}
}

START_TEST(test111)
{
	const size_t length = 1000;

	helper_archiving(test111_progress, length);
}
END_TEST

/* Archive, with 10 reports, and duplicating them, checking
 * progress. */
static void test112_progress(struct lus_hsm_action_handle *hcp, size_t length)
{
	int rc;
	int i;
	struct hsm_extent he;
	struct hsm_current_action hca;

	for (i = 0; i < 10; i++) {
		he.offset = i*length/10;
		he.length = length/10;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));

		rc = llapi_hsm_current_action(testfile, &hca);
		ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
			strerror(-rc));
		ck_assert_msg(hca.hca_state == HPS_RUNNING,
			"hca_state=%u", hca.hca_state);
		ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
			"hca_state=%u", hca.hca_action);
		ck_assert_msg(hca.hca_location.length == (i+1)*length/10,
			"i=%d, length=%llu",
			i, (unsigned long long)hca.hca_location.length);
	}

	for (i = 0; i < 10; i++) {
		he.offset = i*length/10;
		he.length = length/10;
		rc = llapi_hsm_action_progress(hcp, &he, length, 0);
		ck_assert_msg(rc == 0, "llapi_hsm_action_progress failed: %s",
			strerror(-rc));

		rc = llapi_hsm_current_action(testfile, &hca);
		ck_assert_msg(rc == 0, "llapi_hsm_current_action failed: %s",
			strerror(-rc));
		ck_assert_msg(hca.hca_state == HPS_RUNNING,
			"hca_state=%u", hca.hca_state);
		ck_assert_msg(hca.hca_action == HUA_ARCHIVE,
			"hca_state=%u", hca.hca_action);
		ck_assert_msg(hca.hca_location.length == length,
			"i=%d, length=%llu",
			i, (unsigned long long)hca.hca_location.length);
	}
}

START_TEST(test112)
{
	const size_t length = 1000;

	helper_archiving(test112_progress, length);
}
END_TEST

#define ADD_TEST(name) {			\
	tc = tcase_create(#name);		\
	tcase_add_test(tc, name);		\
	suite_add_tcase(s, tc);			\
}

int main(int argc, char *argv[])
{
	int rc;
	int opt;
	int number_failed;
	Suite *s;
	TCase *tc;
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

	s = suite_create("HSM");
	ADD_TEST(test1);
	ADD_TEST(test2);
	ADD_TEST(test3);
	ADD_TEST(test4);
	ADD_TEST(test5);
	ADD_TEST(test7);
	ADD_TEST(test50);
	ADD_TEST(test51);
	ADD_TEST(test52);
	ADD_TEST(test100);
	ADD_TEST(test101);
	ADD_TEST(test102);
	ADD_TEST(test103);
	ADD_TEST(test104);
	ADD_TEST(test105);
	ADD_TEST(test106);
	ADD_TEST(test107);
	ADD_TEST(test108);
	ADD_TEST(test109);
	ADD_TEST(test110);
	ADD_TEST(test111);
	ADD_TEST(test112);

	sr = srunner_create(s);
	srunner_run_all(sr, CK_ENV);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	lus_close_fs(lfsh);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
