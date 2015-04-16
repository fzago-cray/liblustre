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

/* Read values from procfs. */

#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <errno.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/* Read a single line from a file in procfs. Returns 0 on success, or
 * a negative errno on failure. The output parameter must be large
 * enough.
 * A success guarantees:
 *  - no truncation occurred
 *  - the string is NUL terminated
 *  - the value doesn't have a carriage return.
 */
static int read_procfs_value(const char *type, const char *inst,
			     const char *param, char *buf, size_t buf_size)
{
	FILE *fp = NULL;
	char *line = NULL;
	size_t line_len;
	char path[PATH_MAX];
	int rc;
	ssize_t n;

	rc = snprintf(path, sizeof(path),
		      "/proc/fs/lustre/%s/%s/%s", type, inst, param);
	if (rc < 0 || rc >= sizeof(path))
		return -ENAMETOOLONG;

	fp = fopen(path, "r");
	if (fp == NULL) {
		return -errno;
		goto out;
	}

	n = getline(&line, &line_len, fp);
	if (n == -1) {
		rc = -errno;
		goto out;
	}

	chomp_string(line);

	if (n > buf_size) {
		rc = -EOVERFLOW;
		goto out;
	}

	strcpy(buf, line);

	rc = 0;

out:
	if (fp != NULL)
		fclose(fp);
	free(line);

	return rc;
}

int get_param_lmv(int fd, const char *param, char *buf, size_t buf_size)
{
	struct obd_uuid uuid;
	int rc;

	/* TODO: can we cache this value? in lfsh? */
	rc = ioctl(fd, OBD_IOC_GETMDNAME, &uuid);
	if (rc != 0)
		return -errno;

	return read_procfs_value("lmv", uuid.uuid, param, buf, buf_size);
}

#ifdef UNIT_TEST
#include "../tests/liblustre_params_tests.c"
#endif


