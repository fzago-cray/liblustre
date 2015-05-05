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
 * Copyright (c) 2003, 2004, 2007, 2010, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2010, 2011, 2014, Intel Corporation.
 */

/* This file has definitions from upstream lustre_user.h, lustre_idl.h
 * and lustreapi.h. */

#ifndef _LIBLUSTRE_INTERNAL_H_
#error Do not include directly - include liblustre_internal.h
#endif

#ifndef _LIBLUSTRE_INTERNAL_GPL_H_
#define _LIBLUSTRE_INTERNAL_GPL_H_

/*
 * LOV
 */
#define LOV_MAGIC_MAGIC	0x0BD0
#define LOV_MAGIC_V1	(0x0BD10000 | LOV_MAGIC_MAGIC)
#define LOV_MAGIC_V3	(0x0BD30000 | LOV_MAGIC_MAGIC)

#define LOV_PATTERN_RAID0	0x001
#define LOV_PATTERN_RAID1	0x002
#define LOV_PATTERN_FIRST	0x100
#define LOV_PATTERN_CMOBD	0x200

#define LOV_PATTERN_F_MASK	0xffff0000
#define LOV_PATTERN_F_HOLE	0x40000000
#define LOV_PATTERN_F_RELEASED  0x80000000

#define LOV_MAXPOOLNAME 15

struct lov_user_md_v3 {
        __u32 lmm_magic;
        __u32 lmm_pattern;
        struct ost_id lmm_oi;
        __u32 lmm_stripe_size;
        __u16 lmm_stripe_count;
        union {
                __u16 lmm_stripe_offset;
                __u16 lmm_layout_gen;
        };
        char  lmm_pool_name[LOV_MAXPOOLNAME + 1];
        struct lov_user_ost_data_v1 lmm_objects[0];
} __attribute__((packed));

static inline __u32 lov_user_md_size(__u16 stripes, __u32 lmm_magic)
{
	if (lmm_magic == LOV_USER_MAGIC_V1)
		return sizeof(struct lov_user_md_v1) +
			stripes * sizeof(struct lov_user_ost_data_v1);
	else
		return sizeof(struct lov_user_md_v3) +
			stripes * sizeof(struct lov_user_ost_data_v1);
}

/*
 * Layouts
 */
#define LLAPI_LAYOUT_INVALID    0x1000000000000001ULL
#define LLAPI_LAYOUT_DEFAULT    (LLAPI_LAYOUT_INVALID + 1)
#define LLAPI_LAYOUT_WIDE       (LLAPI_LAYOUT_INVALID + 2)

#define LLAPI_LAYOUT_RAID0    0

#define LAYOUT_GET_EXPECTED 0x1

/* struct stat is used. Make sure we're not on a 32 bits system, where
 * the size of stat is different. The drivers expect a stat64. */
#if __WORDSIZE != 64
#error The library only compiles on 64 bits systems
#endif

#define lov_user_mds_data lov_user_mds_data_v1
struct lov_user_mds_data_v1 {
        struct stat lmd_st;
        struct lov_user_md_v1 lmd_lmm;
} __attribute__((packed));


#define KUC_MAGIC  0x191C /*Lustre9etLinC */

#define XATTR_NAME_LMA          "trusted.lma"

#define LUSTRE_VOLATILE_HDR ".\x0c\x13\x14\x12:VOLATILE"
#define LUSTRE_VOLATILE_HDR_LEN     14

int get_param_lmv(int fd, const char *param, char **value);

/*
 * FID
 */
struct getinfo_fid2path {
	struct lu_fid   gf_fid;
	__u64		gf_recno;
	__u32		gf_linkno;
	__u32		gf_pathlen;
	char		gf_path[0];
} __attribute__((packed));

struct getparent {
        struct lu_fid   gp_fid;
        __u32           gp_linkno;
        __u32           gp_name_size;
        char            gp_name[0];
} __attribute__((packed));

/*
 * HSM
 */
struct hsm_copy {
	__u64			hc_data_version;
	__u16			hc_flags;
	__u16			hc_errval;
	__u32			padding;
	struct hsm_action_item	hc_hai;
};

enum hss_valid {
	HSS_SETMASK	= 0x01,
	HSS_CLEARMASK   = 0x02,
	HSS_ARCHIVE_ID  = 0x04,
};

enum hsm_message_type {
	HMT_ACTION_LIST = 100,
};

struct hsm_state_set {
	__u32 hss_valid;
	__u32 hss_archive_id;
	__u64 hss_setmask;
	__u64 hss_clearmask;
};

struct hsm_user_import {
	__u64 hui_size;
	__u64 hui_atime;
	__u64 hui_mtime;
	__u32 hui_atime_ns;
	__u32 hui_mtime_ns;
	__u32 hui_uid;
	__u32 hui_gid;
	__u32 hui_mode;
	__u32 hui_archive_id;
};

struct hsm_progress {
	lustre_fid	  hp_fid;
	__u64		  hp_cookie;
	struct hsm_extent hp_extent;
	__u16		  hp_flags;
	__u16		  hp_errval;
	__u32		  padding;
};

/* JSON handling */
enum llapi_json_types {
	LLAPI_JSON_INTEGER = 1,
	LLAPI_JSON_BIGNUM,
	LLAPI_JSON_REAL,
	LLAPI_JSON_STRING
};

struct llapi_json_item {
	char		*lji_key;
	__u32		 lji_type;
	union {
		int	 lji_integer;
		__u64	 lji_u64;
		double	 lji_real;
		char	*lji_string;
	};
	struct llapi_json_item	*lji_next;
};

struct llapi_json_item_list {
	int			ljil_item_count;
	struct llapi_json_item	*ljil_items;
};

/*
 * Communication with kernel.
 */
struct lustre_kernelcomm {
	__u32 lk_wfd;
	__u32 lk_rfd;
	__u32 lk_uid;
	__u32 lk_group;
	__u32 lk_data;
	__u32 lk_flags;
} __attribute__((packed));

struct kuc_hdr {
	__u16 kuc_magic;
	__u8  kuc_transport;
	__u8  kuc_flags;
	__u16 kuc_msgtype;
	__u16 kuc_msglen;
} __attribute__((aligned(sizeof(__u64))));

enum kuc_transport_type {
	KUC_TRANSPORT_GENERIC   = 1,
	KUC_TRANSPORT_HSM	= 2,
	KUC_TRANSPORT_CHANGELOG = 3,
};

enum kuc_generic_message_type {
	KUC_MSG_SHUTDOWN = 1,
};

#define KUC_GRP_HSM	   0x02

#define LK_FLG_STOP 0x01

/*
 * IOCTLs
 */
#define OBD_IOC_GETMDNAME	_IOR ('f', 131, char[MAX_OBD_NAME])
#define OBD_IOC_FID2PATH	_IOWR('f', 150, long)
#define LL_IOC_LOV_SETSTRIPE    _IOW ('f', 154, long)
#define LL_IOC_PATH2FID         _IOR ('f', 173, long)
#define LL_IOC_HSM_STATE_GET	_IOR ('f', 211, struct hsm_user_state)
#define LL_IOC_HSM_STATE_SET    _IOW ('f', 212, struct hsm_state_set)
#define LL_IOC_HSM_CT_START	_IOW ('f', 213, struct lustre_kernelcomm)
#define LL_IOC_HSM_COPY_START   _IOW ('f', 214, struct hsm_copy *)
#define LL_IOC_HSM_COPY_END	_IOW ('f', 215, struct hsm_copy *)
#define LL_IOC_HSM_PROGRESS	_IOW ('f', 216, struct hsm_user_request)
#define LL_IOC_HSM_REQUEST	_IOW ('f', 217, struct hsm_user_request)
#define LL_IOC_HSM_ACTION	_IOR ('f', 220, struct hsm_current_action)
#define LL_IOC_HSM_IMPORT	_IOWR('f', 245, struct hsm_user_import)
#define LL_IOC_FID2MDTIDX	_IOWR('f', 248, struct lu_fid)
#define LL_IOC_GETPARENT	_IOWR('f', 249, struct getparent)

#define IOC_MDC_TYPE            'i'
#define IOC_MDC_GETFILEINFO     _IOWR(IOC_MDC_TYPE, 22, struct lov_user_mds_data *)

#endif /* _LIBLUSTRE_INTERNAL_GPL_H_ */
