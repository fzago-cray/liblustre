#ifndef _LUSTRE_H_
#error Dot not include this file directly. Use lustre.h.
#endif

/*
 * Lustre FID
 */
typedef struct lu_fid {
	__u64 f_seq;
	__u32 f_oid;
	__u32 f_ver;
} lustre_fid;

#define DFID_NOBRACE "%#llx:0x%x:0x%x"
#define FID_NOBRACE_LEN 40
#define DFID "["DFID_NOBRACE"]"
#define FID_LEN (FID_NOBRACE_LEN + 2)

#define SFID "0x%llx:0x%x:0x%x"
#define PFID(fid) (fid)->f_seq, (fid)->f_oid, (fid)->f_ver
#define RFID(fid) &((fid)->f_seq), &((fid)->f_oid), &((fid)->f_ver)

/*
 * Misc definitions
 */

#define LOV_MIN_STRIPE_BITS 16
#define LOV_MIN_STRIPE_SIZE (1 << LOV_MIN_STRIPE_BITS)
#define LOV_MAX_STRIPE_COUNT 2000
#define LOV_V1_INSANE_STRIPE_COUNT 65532

#define UUID_MAX        40

#define O_LOV_DELAY_CREATE (0100000000 | (O_NOCTTY | FASYNC))

#define MAX_OBD_NAME 128

#define XATTR_TRUSTED_PREFIX    "trusted."

struct ost_id {
        union {
                struct {
                        __u64   oi_id;
                        __u64   oi_seq;
                } oi;
                struct lu_fid oi_fid;
        };
};

#define dot_lustre_name ".lustre" /* TODO: this should die */

struct lustre_mdt_attrs {
        __u32   lma_compat;
        __u32   lma_incompat;
        /** FID of this inode */
        struct lu_fid  lma_self_fid;
};

/*
 * LOV
 */

/* TODO: add API so posix copytool doesn't have to know that. In layout API? */
#define LOV_USER_MAGIC_V1       0x0BD10BD0
#define LOV_USER_MAGIC_V3       0x0BD30BD0
#define LOV_USER_MAGIC_SPECIFIC 0x0BD50BD0

#define lov_user_ost_data lov_user_ost_data_v1

struct lov_user_ost_data_v1 {
        struct ost_id l_ost_oi;
        __u32 l_ost_gen;
        __u32 l_ost_idx;
} __attribute__((packed));

#define lov_user_md lov_user_md_v1
struct lov_user_md_v1 {
        __u32 lmm_magic;
        __u32 lmm_pattern;
        struct ost_id lmm_oi;
        __u32 lmm_stripe_size;
        __u16 lmm_stripe_count;
        union {
                __u16 lmm_stripe_offset;
                __u16 lmm_layout_gen;
        };
        struct lov_user_ost_data_v1 lmm_objects[0];
} __attribute__((packed,  __may_alias__));

/*
 * HSM
 */
enum hsm_states {
	HS_EXISTS	= 0x00000001,
	HS_DIRTY	= 0x00000002,
	HS_RELEASED	= 0x00000004,
	HS_ARCHIVED	= 0x00000008,
	HS_NORELEASE	= 0x00000010,
	HS_NOARCHIVE	= 0x00000020,
	HS_LOST		= 0x00000040,
};

enum hsm_copytool_action {
	HSMA_NONE    = 10, /* no action */
	HSMA_ARCHIVE = 20, /* arbitrary offset */
	HSMA_RESTORE = 21,
	HSMA_REMOVE  = 22,
	HSMA_CANCEL  = 23
};

#define HP_FLAG_COMPLETED 0x01
#define HP_FLAG_RETRY     0x02

struct hsm_extent {
	__u64 offset;
	__u64 length;
} __attribute__((packed));

struct hsm_user_state {
	__u32			hus_states;
	__u32			hus_archive_id;
	__u32			hus_in_progress_state;
	__u32			hus_in_progress_action;
	struct hsm_extent	hus_in_progress_location;
	char			hus_extended_info[];
};

struct hsm_current_action {
	__u32			hca_state;
	__u32			hca_action;
	struct hsm_extent	hca_location;
};

enum hsm_user_action {
        HUA_NONE    =  1,
        HUA_ARCHIVE = 10,
        HUA_RESTORE = 11,
        HUA_RELEASE = 12,
        HUA_REMOVE  = 13,
        HUA_CANCEL  = 14,
};

struct hsm_request {
	__u32 hr_action;
	__u32 hr_archive_id;
	__u64 hr_flags;
	__u32 hr_itemcount;
	__u32 hr_data_len;
};

struct hsm_user_item {
       lustre_fid        hui_fid;
       struct hsm_extent hui_extent;
} __attribute__((packed));

struct hsm_user_request {
	struct hsm_request	hur_request;
	struct hsm_user_item	hur_user_item[0];
} __attribute__((packed));

static inline void *hur_data(struct hsm_user_request *hur)
{
	return &(hur->hur_user_item[hur->hur_request.hr_itemcount]);
}

/* TODO: should rewrite that one. */
static inline ssize_t hur_len(struct hsm_user_request *hur)
{
	__u64	size;

	size = offsetof(struct hsm_user_request, hur_user_item[0]) +
		(__u64)hur->hur_request.hr_itemcount *
		sizeof(hur->hur_user_item[0]) + hur->hur_request.hr_data_len;

	if (size != (ssize_t)size)
		return -1;

	return size;
}

struct hsm_action_list {
	__u32 hal_version;
	__u32 hal_count;
	__u64 hal_compound_id;
	__u64 hal_flags;
	__u32 hal_archive_id;
	__u32 padding1;
	char  hal_fsname[0];
} __attribute__((packed));

struct hsm_action_item {
	__u32      hai_len;
	__u32      hai_action;
	lustre_fid hai_fid;
	lustre_fid hai_dfid;
	struct hsm_extent hai_extent;
	__u64      hai_cookie;
	__u64      hai_gid;
	char       hai_data[0];
} __attribute__((packed));

#define UUID_MAX        40
struct obd_uuid {
        char uuid[UUID_MAX];
};
