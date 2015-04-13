/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
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
 * http://www.sun.com/software/products/lustre/docs/GPLv2.pdf
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2003, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2011, 2014, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 *
 * lustre/utils/liblustreapi.c
 *
 * Author: Peter J. Braam <braam@clusterfs.com>
 * Author: Phil Schwan <phil@clusterfs.com>
 * Author: Robert Read <rread@clusterfs.com>
 */

/*
 * Read parameters from /proc
 */

static int read_procfs_value(const char *type, const char *inst,
			     const char *param, char *buf, size_t buf_size)
{
        char param_path[PATH_MAX];
        FILE *param_file;
        int rc;

        snprintf(param_path, sizeof(param_path),
                 "/proc/fs/lustre/%s/%s/%s", type, inst, param);
	param_path[sizeof(param_path)-1] = 0;

        param_file = fopen(param_path, "r");
        if (param_file == NULL) {
                rc = -errno;
                goto out;
        }

        if (fgets(buf, buf_size, param_file) == NULL) {
                rc = -errno;
                goto out;
        }

        rc = 0;
out:
        if (param_file != NULL)
                fclose(param_file);

        return rc;
}

int llapi_file_fget_lmv_uuid(int fd, struct obd_uuid *lov_name)
{
        int rc = ioctl(fd, OBD_IOC_GETMDNAME, lov_name);
        if (rc) {
                rc = -errno;
                llapi_error(LLAPI_MSG_ERROR, rc, "error: can't get lmv name.");
        }
        return rc;
}

int get_param_lmv(int fd, const char *param, char *buf, size_t buf_size)
{
        struct obd_uuid uuid;
        int rc;

        rc = llapi_file_fget_lmv_uuid(fd, &uuid);
        if (rc != 0)
                return rc;

        return get_param_cli("lmv", uuid.uuid, param, buf, buf_size);
}


