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
 * Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2012, 2013, Intel Corporation.
 */
/*
 * This file is part of Lustre, http://www.lustre.org/
 * Lustre is a trademark of Sun Microsystems, Inc.
 *
 * Author: Nathan Rutman <nathan.rutman@sun.com>
 *
 * Kernel <-> userspace communication routines.
 * Using pipes for all arches.
 */

#define DEBUG_SUBSYSTEM S_CLASS
#define D_KUC D_OTHER

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/** Start the userspace side of a KUC pipe.
 * @param link Private descriptor for pipe/socket.
 * @param groups KUC broadcast group to listen to
 *          (can be null for unicast to this pid)
 * @param rfd_flags flags for read side of pipe (e.g. O_NONBLOCK)
 */
int libcfs_ukuc_start(lustre_kernelcomm *lnk, int group, int rfd_flags)
{
	int pfd[2];
	int rc;

	lnk->lk_rfd = lnk->lk_wfd = LK_NOFD;

	if (pipe(pfd) < 0)
		return -errno;

	if (fcntl(pfd[0], F_SETFL, rfd_flags) < 0) {
		rc = -errno;
		close(pfd[0]);
		close(pfd[1]);
		return rc;
	}

	memset(lnk, 0, sizeof(*lnk));
	lnk->lk_rfd = pfd[0];
	lnk->lk_wfd = pfd[1];
	lnk->lk_group = group;
	lnk->lk_uid = getpid();
	return 0;
}

int libcfs_ukuc_stop(lustre_kernelcomm *lnk)
{
	int rc;

	if (lnk->lk_wfd != LK_NOFD)
		close(lnk->lk_wfd);
        rc = close(lnk->lk_rfd);
	lnk->lk_rfd = lnk->lk_wfd = LK_NOFD;
	return rc;
}

/** Returns the file descriptor for the read side of the pipe,
 *  to be used with poll/select.
 * @param lnk Private descriptor for pipe/socket.
 */
int libcfs_ukuc_get_rfd(lustre_kernelcomm *lnk)
{
	return lnk->lk_rfd;
}

#define lhsz sizeof(*kuch)

/** Read a message from the lnk.
 * Allocates memory, returns handle
 *
 * @param lnk Private descriptor for pipe/socket.
 * @param buf Buffer to read into, must include size for kuc_hdr
 * @param maxsize Maximum message size allowed
 * @param transport Only listen to messages on this transport
 *      (and the generic transport)
 */
int libcfs_ukuc_msg_get(lustre_kernelcomm *lnk, char *buf, int maxsize,
                        int transport)
{
        struct kuc_hdr *kuch;
        int rc = 0;

        memset(buf, 0, maxsize);

        while (1) {
                /* Read header first to get message size */
                rc = read(lnk->lk_rfd, buf, lhsz);
                if (rc <= 0) {
                        rc = -errno;
                        break;
                }
                kuch = (struct kuc_hdr *)buf;

                if (kuch->kuc_magic != KUC_MAGIC) {
                        log_msg(LLAPI_MSG_ERROR, 0,
				"bad message magic %x != %x",
				kuch->kuc_magic, KUC_MAGIC);
                        rc = -EPROTO;
                        break;
                }

                if (kuch->kuc_msglen > maxsize) {
                        rc = -EMSGSIZE;
                        break;
                }

                /* Read payload */
                rc = read(lnk->lk_rfd, buf + lhsz, kuch->kuc_msglen - lhsz);
                if (rc < 0) {
                        rc = -errno;
                        break;
                }
                if (rc < (kuch->kuc_msglen - lhsz)) {
			log_msg(LLAPI_MSG_ERROR, 0,
				"short read: got %d of %d bytes",
				rc, kuch->kuc_msglen);
                        rc = -EPROTO;
                        break;
                }

                if (kuch->kuc_transport == transport ||
                    kuch->kuc_transport == KUC_TRANSPORT_GENERIC) {
                        return 0;
                }
                /* Drop messages for other transports */
        }
        return rc;
}
