/*
 * Internal header for liblustre. None of these declarations are
 * necessary for an application.
 */

#ifndef _LIBLUSTRE_INTERNAL_H_
#define _LIBLUSTRE_INTERNAL_H_

#include <stdio.h>
#include <sys/ioctl.h>

/*
 * Logging
 */
unsigned int log_level;
lus_log_callback_t log_msg_callback;

void
log_msg_internal(enum lus_log_level level, int err, const char *fmt, ...)
	__attribute__((format(__printf__, 3, 4)));

/* We don't want to evaluate arguments if the output is not used */
#define log_msg(level, err, fmt, ...)					\
	do {								\
		if (level <= log_level && log_msg_callback != NULL)	\
			log_msg_internal(level, err, fmt, ## __VA_ARGS__); \
	} while(0)

/*
 * Layouts
 */
#define LLAPI_LAYOUT_MAGIC 0x11AD1107

/* Layout swap */
struct lustre_swap_layouts {
        __u64   sl_flags;
        __u32   sl_fd2;
        __u32   sl_gid;
        __u64   sl_dv1;
        __u64   sl_dv2;
};

/*
 * OSTs lists
 */
struct lustre_ost_info {
	/* How many OSTs are stored */
	size_t count;

	/* Array of OST names */
	char *osts[0];
};

void free_ost_info(struct lustre_ost_info **info);
int open_pool_info(const struct lus_fs_handle *lfsh, const char *poolname,
		   struct lustre_ost_info **info);

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
	char lmm_pool_name[LUS_POOL_NAME_LEN];
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
 * Misc
 */

#define XATTR_NAME_LMA          "trusted.lma"

#define LUSTRE_VOLATILE_HDR ".\x0c\x13\x14\x12:VOLATILE"
#define LUSTRE_VOLATILE_HDR_LEN     14

struct lus_fs_handle {
	/* Lustre mountpoint, as given to lus_open_fs. */
	char *mount_path;
	char fs_name[8 + 1];	/* filesystem name */
	int mount_fd;
	int fid_fd;

	/* Lustre client version, as reported by
	 * /proc/fs/lustre/version, and converted to a single
	 * number. e.g. Lustre 2.5.3 is 20503. */
	unsigned int client_version;
};

/* File data version */
struct ioc_data_version {
        __u64 idv_version;
        __u64 idv_flags;
};

void chomp_string(char *buf);
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

/*
 * Communication with kernel.
 */

#define KUC_MAGIC 0x191C
#define KUC_GRP_HSM 0x02
#define LK_FLG_STOP 0x01

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

/*
 * IOCTLs
 */
#define OBD_IOC_GETMDNAME	_IOR ('f', 131, char[MAX_OBD_NAME])
#define OBD_IOC_FID2PATH	_IOWR('f', 150, long)
#define LL_IOC_LOV_SETSTRIPE    _IOW ('f', 154, long)
#define LL_IOC_GROUP_LOCK	_IOW ('f', 158, long)
#define LL_IOC_GROUP_UNLOCK	_IOW ('f', 159, long)
#define LL_IOC_PATH2FID         _IOR ('f', 173, long)
#define LL_IOC_HSM_STATE_GET	_IOR ('f', 211, struct hsm_user_state)
#define LL_IOC_HSM_STATE_SET    _IOW ('f', 212, struct hsm_state_set)
#define LL_IOC_HSM_CT_START	_IOW ('f', 213, struct lustre_kernelcomm)
#define LL_IOC_HSM_COPY_START   _IOW ('f', 214, struct hsm_copy *)
#define LL_IOC_HSM_COPY_END	_IOW ('f', 215, struct hsm_copy *)
#define LL_IOC_HSM_PROGRESS	_IOW ('f', 216, struct hsm_user_request)
#define LL_IOC_HSM_REQUEST	_IOW ('f', 217, struct hsm_user_request)
#define LL_IOC_DATA_VERSION	_IOR ('f', 218, struct ioc_data_version)
#define LL_IOC_LOV_SWAP_LAYOUTS _IOW ('f', 219, struct lustre_swap_layouts)
#define LL_IOC_HSM_ACTION	_IOR ('f', 220, struct hsm_current_action)
#define LL_IOC_HSM_IMPORT	_IOWR('f', 245, struct hsm_user_import)
#define LL_IOC_FID2MDTIDX	_IOWR('f', 248, struct lu_fid)
#define LL_IOC_GETPARENT	_IOWR('f', 249, struct getparent)

#define IOC_MDC_GETFILEINFO     _IOWR('i', 22, struct lov_user_mds_data *)

/* String helpers. */
ssize_t strscpy(char *dst, const char *src, size_t dst_size);
ssize_t strscat(char *dst, const char *src, size_t dst_size);

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
void unittest_strscpy(void);
void unittest_strscat(void);
void unittest_lus_fid2path(void);
void unittest_lus_data_version_by_fd(void);
void unittest_lus_mdt_stat_by_fid(void);

#endif /* _LIBLUSTRE_INTERNAL_H_ */
