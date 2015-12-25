/* Public API for liblustre. */

#ifndef _LUSTRE_H_
#define _LUSTRE_H_

#include <asm/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

#define LOV_MAXPOOLNAME 15

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

struct lustre_mdt_attrs {
	__u32   lma_compat;
	__u32   lma_incompat;
	struct lu_fid  lma_self_fid;
};

#define UUID_MAX 40
struct obd_uuid {
	char uuid[UUID_MAX];
};

/* Lustre extended attributes */
#define XATTR_LUSTRE_PREFIX	"lustre."
#define XATTR_LUSTRE_LOV	XATTR_LUSTRE_PREFIX"lov"

/* Data version flushing options */
#define LL_DV_RD_FLUSH (1 << 0)
#define LL_DV_WR_FLUSH (1 << 1)

/* Layout swap flags */
#define SWAP_LAYOUTS_CHECK_DV1  (1 << 0)
#define SWAP_LAYOUTS_CHECK_DV2  (1 << 1)
#define SWAP_LAYOUTS_KEEP_MTIME (1 << 2)
#define SWAP_LAYOUTS_KEEP_ATIME (1 << 3)
#define SWAP_LAYOUTS_MDS_HSM    (1 << 31)

/*
 * Layouts
 */
#define LLAPI_LAYOUT_INVALID    0x1000000000000001ULL
#define LLAPI_LAYOUT_DEFAULT    (LLAPI_LAYOUT_INVALID + 1)
#define LLAPI_LAYOUT_WIDE       (LLAPI_LAYOUT_INVALID + 2)

#define LLAPI_LAYOUT_RAID0    0

#define LLAPI_LAYOUT_RELEASED 0x0100

#define LAYOUT_GET_EXPECTED 0x1

/*
 * Logging
 */

/* Logging levels. The default is LUS_LOG_OFF. */
enum lus_log_level {
	LUS_LOG_OFF = 0,
	LUS_LOG_FATAL,
	LUS_LOG_ERROR,
	LUS_LOG_WARN,
	LUS_LOG_NORMAL,
	LUS_LOG_INFO,
	LUS_LOG_DEBUG
};

void lus_log_set_level(enum lus_log_level level);
enum lus_log_level lus_log_get_level(void);
typedef void (*lus_log_callback_t)(enum lus_log_level level, int err,
				   const char *fmt, va_list args);
void lus_log_set_callback(lus_log_callback_t cb);

/*
 * Open / close a filesystem
 */
struct lustre_fs_h;
void lus_close_fs(struct lustre_fs_h *lfsh);
int lus_open_fs(const char *mount_path, struct lustre_fs_h **lfsh);
const char *lus_get_fsname(const struct lustre_fs_h *lfsh);
const char *lus_get_mountpoint(const struct lustre_fs_h *lfsh);

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
 * Layouts
 */
struct llapi_layout;
struct llapi_layout *llapi_layout_get_by_path(const char *path, uint32_t flags);
struct llapi_layout *llapi_layout_get_by_fd(int fd, uint32_t flags);
struct llapi_layout *llapi_layout_get_by_fid(const struct lustre_fs_h *lfsh,
					     const lustre_fid *fid,
					     uint32_t flags);
struct llapi_layout *llapi_layout_alloc(unsigned int num_stripes);
void llapi_layout_free(struct llapi_layout *layout);
int llapi_layout_stripe_count_get(const struct llapi_layout *layout,
				  uint64_t *count);
int llapi_layout_stripe_count_set(struct llapi_layout *layout, uint64_t count);
int llapi_layout_stripe_size_get(const struct llapi_layout *layout,
				 uint64_t *size);
int llapi_layout_stripe_size_set(struct llapi_layout *layout, uint64_t size);
int llapi_layout_pattern_get(const struct llapi_layout *layout,
			     uint64_t *pattern);
int llapi_layout_pattern_set(struct llapi_layout *layout, uint64_t pattern);
int llapi_layout_pattern_flags_set(struct llapi_layout *layout,
				   uint64_t pattern_flags);
int llapi_layout_ost_index_get(const struct llapi_layout *layout,
			       uint64_t stripe_number, uint64_t *index);
int llapi_layout_ost_index_set(struct llapi_layout *layout, int stripe_number,
			       uint64_t index);
int llapi_layout_pool_name_get(const struct llapi_layout *layout,
			       char *pool_name, size_t pool_name_len);
int llapi_layout_pool_name_set(struct llapi_layout *layout,
			       const char *pool_name);
int llapi_layout_file_open(const char *path, int open_flags, mode_t mode,
			   const struct llapi_layout *layout);
int llapi_layout_file_openat(int dir_fd, const char *path, int open_flags,
			     mode_t mode, const struct llapi_layout *layout);
int llapi_layout_file_create(const char *path, int open_flags, int mode,
			     const struct llapi_layout *layout);
int lus_fswap_layouts(int fd1, int fd2, uint64_t dv1, uint64_t dv2,
		      uint64_t flags);
int lus_lovxattr_to_layout(struct lov_user_md *lum, size_t lum_len,
			   struct llapi_layout **layout);

/*
 * Misc
 */
int lus_fid2path(const struct lustre_fs_h *lfsh, const struct lu_fid *fid,
		 char *path, size_t path_len, long long *recno,
		 unsigned int *linkno);
int lus_fd2fid(int fd, lustre_fid *fid);
int lus_path2fid(const char *path, lustre_fid *fid);
int lus_open_by_fid(const struct lustre_fs_h *lfsh,
		    const lustre_fid *fid, int open_flags);
int lus_stat_by_fid(const struct lustre_fs_h *lfsh,
		    const lustre_fid *fid, struct stat *stbuf);
int lus_get_mdt_index_by_fid(const struct lustre_fs_h *lfsh,
			     const struct lu_fid *fid);
int lus_create_volatile_by_fid(const struct lustre_fs_h *lfsh,
			       const lustre_fid *parent_fid,
			       int mdt_idx, int open_flags, mode_t mode,
			       const struct llapi_layout *layout);
int lus_fd2parent(int fd, unsigned int linkno, lustre_fid *parent_fid,
		  char *parent_name, size_t parent_name_len);
int lus_fid2parent(const struct lustre_fs_h *lfsh,
		   const lustre_fid *fid,
		   unsigned int linkno,
		   lustre_fid *parent_fid,
		   char *parent_name, size_t parent_name_len);
int lus_path2parent(const char *path, unsigned int linkno,
		      lustre_fid *parent_fid,
		      char *parent_name, size_t parent_name_len);
int lus_data_version_by_fd(int fd, uint64_t flags, uint64_t *dv);
int lus_group_lock(int fd, uint64_t gid);
int lus_group_unlock(int fd, uint64_t gid);

/* Library initialization */
bool lus_initialized;
void lus_init(void);

/*
 * HSM
 */

/* Maximum number of copytools. */
#define LL_HSM_MAX_ARCHIVE (sizeof(__u32) * 8)

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
	HSMA_NONE    = 10,
	HSMA_ARCHIVE = 20,
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

enum hsm_progress_states {
	HPS_NONE    = 0,
	HPS_WAITING = 1,
	HPS_RUNNING = 2,
	HPS_DONE    = 3,
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
       lustre_fid	 hui_fid;
       struct hsm_extent hui_extent;
} __attribute__((packed));

struct hsm_user_request {
	struct hsm_request	hur_request;
	struct hsm_user_item	hur_user_item[0];
} __attribute__((packed));

/* Header for HSM action items. A series of action item follow the
 * header, and each is 8 bytes aligned. */
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

int llapi_hsm_request(const struct lustre_fs_h *lfsh,
		      const struct hsm_user_request *request);
int llapi_hsm_current_action(const char *path,
			     struct hsm_current_action *hca);
int llapi_hsm_state_get_fd(int fd, struct hsm_user_state *hus);
int llapi_hsm_state_get(const char *path, struct hsm_user_state *hus);
int llapi_hsm_state_set_fd(int fd, __u64 setmask, __u64 clearmask,
			   __u32 archive_id);
int llapi_hsm_state_set(const char *path, __u64 setmask, __u64 clearmask,
			__u32 archive_id);

/**
 * Helper to return the length needed for an HSM user request.
 *
 * \param itemcount    number of items in the request
 * \param data_len     length of extra data
 *
 * \return the length of memory to allocate
 */
static inline size_t llapi_hsm_user_request_len(unsigned int itemcount,
						unsigned int data_len)
{
	return sizeof(struct hsm_user_request) +
		sizeof(struct hsm_user_item) * itemcount +
		data_len;
}

/*
 * HSM copytool interface.
 * priv is private state, managed internally by these functions
 */
struct lus_hsm_ct_handle;
struct lus_hsm_action_handle;

int lus_hsm_copytool_register(const struct lustre_fs_h *lfsh,
			      unsigned int archive_count, int *archives,
			      struct lus_hsm_ct_handle **priv);
int lus_hsm_copytool_unregister(struct lus_hsm_ct_handle **priv);
int lus_hsm_copytool_get_fd(const struct lus_hsm_ct_handle *ct);
int lus_hsm_copytool_recv(struct lus_hsm_ct_handle *priv,
			  const struct hsm_action_list **hal,
			  size_t *msgsize);
int lus_hsm_action_begin(struct lus_hsm_action_handle **phcp,
			 const struct lus_hsm_ct_handle *ct,
			 const struct hsm_action_item *hai,
			 int restore_mdt_index, int restore_open_flags,
			 bool is_error);
int lus_hsm_action_end(struct lus_hsm_action_handle **phcp,
		       const struct hsm_extent *he,
		       int hp_flags, int errval);
int lus_hsm_action_progress(const struct lus_hsm_action_handle *hcp,
			    const struct hsm_extent *he, uint64_t total,
			    unsigned int hp_flags);
int lus_hsm_action_get_dfid(const struct lus_hsm_action_handle *hcp,
			    struct lu_fid *fid);
int lus_hsm_action_get_fd(const struct lus_hsm_action_handle *hcp);
int llapi_hsm_import(const char *dst, int archive, const struct stat *st,
		     struct llapi_layout *layout);
const struct hsm_action_item *
lus_hsm_hai_first(const struct hsm_action_list *hal);
const struct hsm_action_item *
lus_hsm_hai_next(const struct hsm_action_item *hai);

#endif
