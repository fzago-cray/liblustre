/*
 * LGPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * (C) Copyright 2012 Commissariat a l'energie atomique et aux energies
 *     alternatives
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 or (at your discretion) any later version.
 * (LGPL) version 2.1 accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * LGPL HEADER END
 */
/*
 *
 * lustre/utils/lustreapi_internal.h
 *
 */
/*
 *
 * Author: Aurelien Degremont <aurelien.degremont@cea.fr>
 * Author: JC Lafoucriere <jacques-charles.lafoucriere@cea.fr>
 * Author: Thomas Leibovici <thomas.leibovici@cea.fr>
 */

#ifndef _LUSTREAPI_INTERNAL_H_
#define _LUSTREAPI_INTERNAL_H_

#include <stdio.h>
#include <sys/ioctl.h>

/* 
 * LOV
 */
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

/* TODO: isn't everything stat64 these days? Remove and use stat
 * instead? */
#if defined(__x86_64__) || defined(__ia64__) || defined(__ppc64__) || \
    defined(__craynv) || defined(__mips64__) || defined(__powerpc64__)
typedef struct stat     lstat_t;
# define lstat_f        lstat
#elif defined(__USE_LARGEFILE64) || defined(__KERNEL__)
typedef struct stat64   lstat_t;
# define lstat_f        lstat64
#endif

#define lov_user_mds_data lov_user_mds_data_v1
struct lov_user_mds_data_v1 {
        lstat_t lmd_st;                 /* MDS stat struct */
        struct lov_user_md_v1 lmd_lmm;  /* LOV EA V1 user data */
} __attribute__((packed));

struct lov_user_mds_data_v3 {
        lstat_t lmd_st;                 /* MDS stat struct */
        struct lov_user_md_v3 lmd_lmm;  /* LOV EA V3 user data */
} __attribute__((packed));

/* 
 * Misc
 */

struct lustre_fs_h {
	/* Lustre mountpoint, as given to lustre_open_fs. */
	char *mount_path;
	char fs_name[8 + 1];	/* filesystem name */
	int mount_fd;
	int fid_fd;
};

void llapi_chomp_string(char *buf);

#define LLAPI_LAYOUT_MAGIC 0x11AD1107

#define HAL_MAXSIZE (1 << 20)	/* 1 MB, LNET_MTU */

#define KUC_MAGIC  0x191C /*Lustre9etLinC */

#define XATTR_NAME_LMA          "trusted.lma"

#define LUSTRE_VOLATILE_HDR ".\x0c\x13\x14\x12:VOLATILE"
#define LUSTRE_VOLATILE_HDR_LEN     14

int llapi_search_fsname(const char *pathname, char *fsname);

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

/* Helper functions for testing validity of stripe attributes. */
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

static inline bool llapi_stripe_index_is_valid(int64_t index)
{
	return index >= -1 && index <= LOV_V1_INSANE_STRIPE_COUNT;
}

#define llapi_stripe_offset_is_valid(os) llapi_stripe_index_is_valid(os)

int llapi_stripe_limit_check(unsigned long long stripe_size, int stripe_offset,
			     int stripe_count, int stripe_pattern);


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

int llapi_json_init_list(struct llapi_json_item_list **item_list);
int llapi_json_destroy_list(struct llapi_json_item_list **item_list);
int llapi_json_add_item(struct llapi_json_item_list **item_list,
			const char *key, __u32 type, const void *val);
int llapi_json_write_list(struct llapi_json_item_list **item_list, FILE *fp);


int llapi_get_agent_uuid(const struct lustre_fs_h *lfsh,
			 char *buf, size_t bufsize);

/*
 * Communication with kernel.
 */
typedef struct lustre_kernelcomm {
	__u32 lk_wfd;
	__u32 lk_rfd;
	__u32 lk_uid;
	__u32 lk_group;
	__u32 lk_data;
	__u32 lk_flags;
} __attribute__((packed)) lustre_kernelcomm;

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
#define LK_NOFD -1U		

int libcfs_ukuc_start(lustre_kernelcomm *l, int groups, int rfd_flags);
int libcfs_ukuc_stop(lustre_kernelcomm *l);
int libcfs_ukuc_get_rfd(lustre_kernelcomm *link);
int libcfs_ukuc_msg_get(lustre_kernelcomm *l, char *buf, int maxsize,
			int transport);

/*
 * Logging 
 */
/* TODO: rewrite or move to lib/liblustreapi_hsm.c */
static inline const char *llapi_msg_level2str(enum llapi_message_level level)
{
	static const char *levels[LLAPI_MSG_MAX] = {"OFF", "FATAL", "ERROR",
						    "WARNING", "NORMAL",
						    "INFO", "DEBUG"};

	if (level >= LLAPI_MSG_MAX)
		return NULL;

	return levels[level];
}

/* 
 * IOCTLs
 */
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

#define IOC_MDC_TYPE            'i'
#define IOC_MDC_GETFILEINFO     _IOWR(IOC_MDC_TYPE, 22, struct lov_user_mds_data *)

#endif /* _LUSTREAPI_INTERNAL_H_ */
