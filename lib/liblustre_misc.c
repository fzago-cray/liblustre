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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/**
 * size_units is to be initialized (or zeroed) by caller.
 */
/* This is more of a convenience function. That doesn't belong in this
 * library. */
int llapi_parse_size(const char *arg, unsigned long long *size,
		     unsigned long long *size_units, int bytes_spec)
{
        char *end;

        if (strncmp(arg, "-", 1) == 0)
                return -1;

        if (*size_units == 0)
                *size_units = 1;

        *size = strtoull(arg, &end, 0);

        if (*end != '\0') {
                if ((*end == 'b') && *(end + 1) == '\0' &&
                    (*size & (~0ULL << (64 - 9))) == 0 &&
                    !bytes_spec) {
                        *size_units = 1 << 9;
                } else if ((*end == 'b') &&
                           *(end + 1) == '\0' &&
                           bytes_spec) {
                        *size_units = 1;
                } else if ((*end == 'k' || *end == 'K') &&
                           *(end + 1) == '\0' &&
                           (*size & (~0ULL << (64 - 10))) == 0) {
                        *size_units = 1 << 10;
                } else if ((*end == 'm' || *end == 'M') &&
                           *(end + 1) == '\0' &&
                           (*size & (~0ULL << (64 - 20))) == 0) {
                        *size_units = 1 << 20;
                } else if ((*end == 'g' || *end == 'G') &&
                           *(end + 1) == '\0' &&
                           (*size & (~0ULL << (64 - 30))) == 0) {
                        *size_units = 1 << 30;
                } else if ((*end == 't' || *end == 'T') &&
                           *(end + 1) == '\0' &&
                           (*size & (~0ULL << (64 - 40))) == 0) {
                        *size_units = 1ULL << 40;
                } else if ((*end == 'p' || *end == 'P') &&
                           *(end + 1) == '\0' &&
                           (*size & (~0ULL << (64 - 50))) == 0) {
                        *size_units = 1ULL << 50;
                } else if ((*end == 'e' || *end == 'E') &&
                           *(end + 1) == '\0' &&
                           (*size & (~0ULL << (64 - 60))) == 0) {
                        *size_units = 1ULL << 60;
                } else {
                        return -1;
                }
        }
        *size *= *size_units;
        return 0;
}
