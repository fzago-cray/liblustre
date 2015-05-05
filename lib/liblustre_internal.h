/*
 * An alternate Lustre user library.
 * Copyright (c) 2003, 2010, Oracle and/or its affiliates. All rights reserved.
 *   Use is subject to license terms.
 * Copyright (c) 2010, 2014, Intel Corporation.
 * (C) Copyright 2012 Commissariat a l'energie atomique et aux energies
 *     alternatives
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

#ifndef _LIBLUSTRE_INTERNAL_H_
#define _LIBLUSTRE_INTERNAL_H_

#include <stdio.h>
#include <sys/ioctl.h>

#include "liblustre_internal_gpl.h"

/*
 * Logging
 */
unsigned int log_level;
llapi_log_callback_t log_msg_callback;

void
log_msg_internal(enum llapi_message_level level, int err, const char *fmt, ...);

/* We don't want to evaluate arguments if the output is not used */
#define log_msg(level, err, fmt, ...)					\
	do {								\
		if (level <= log_level && log_msg_callback != NULL)	\
			log_msg_internal(level, err, fmt, ## __VA_ARGS__); \
	} while(0)

/*
 * Layouts
 */
#define LLAPI_LAYOUT_INVALID    0x1000000000000001ULL
#define LLAPI_LAYOUT_DEFAULT    (LLAPI_LAYOUT_INVALID + 1)
#define LLAPI_LAYOUT_WIDE       (LLAPI_LAYOUT_INVALID + 2)

#define LLAPI_LAYOUT_RAID0    0

#define LAYOUT_GET_EXPECTED 0x1

#define LLAPI_LAYOUT_MAGIC 0x11AD1107

/*
 * OSTs lists
 */
struct lustre_ost_info {
	/* How many OSTs are stored */
	size_t count;

	/* Array of OST names */
	char *osts[0];
};

void free_ost_info(struct lustre_ost_info *info);
int open_pool_info(const struct lustre_fs_h *lfsh, const char *poolname,
		   struct lustre_ost_info **info);

/*
 * Misc
 */

struct lustre_fs_h {
	/* Lustre mountpoint, as given to llapi_open_fs. */
	char *mount_path;
	char fs_name[8 + 1];	/* filesystem name */
	int mount_fd;
	int fid_fd;
};

void chomp_string(char *buf);

/* Helper functions for testing the validity of stripe attributes. */
static inline bool llapi_stripe_size_is_aligned(uint64_t size)
{
	return (size & (LOV_MIN_STRIPE_SIZE - 1)) == 0;
}

static inline bool llapi_stripe_size_is_too_big(uint64_t size)
{
	return size >= (1ULL << 32);
}

static inline bool llapi_stripe_count_is_valid(int64_t count)
{
	return count >= -1 && count <= LOV_MAX_STRIPE_COUNT;
}

static inline bool llapi_stripe_index_is_valid(int64_t idx)
{
	return idx >= -1 && idx <= LOV_V1_INSANE_STRIPE_COUNT;
}

/* JSON */
int llapi_json_init_list(struct llapi_json_item_list **item_list);
int llapi_json_destroy_list(struct llapi_json_item_list **item_list);
int llapi_json_add_item(struct llapi_json_item_list **item_list,
			const char *key, __u32 type, const void *val);
int llapi_json_write_list(struct llapi_json_item_list **item_list, FILE *fp);

/*
 * Unit testing
 */
void unittest_ost1(void);
void unittest_ost2(void);
void unittest_fid1(void);
void unittest_fid2(void);
void unittest_chomp(void);
void unittest_mdt_index(void);
void unittest_param_lmv(void);
void unittest_read_procfs_value(void);
void unittest_llapi_parse_size(void);
void unittest_llapi_fid2path(void);

#endif /* _LIBLUSTRE_INTERNAL_H_ */
