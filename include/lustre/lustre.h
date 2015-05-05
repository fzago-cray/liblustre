/*
 * LGPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * (C) Copyright 2012 Commissariat a l'energie atomique et aux energies
 *     alternatives
 * Copyright 2015 Cray Inc. All rights reserved
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 or (at your discretion) any later version.
 * (LGPL) version 2.1 accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * LGPL HEADER END
 */

#ifndef _LUSTRE_H_
#define _LUSTRE_H_

#include <stdint.h>
#include <stdbool.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <lustre/liblustre_user.h>

/* Lustre extended attributes */
#define XATTR_LUSTRE_PREFIX	"lustre."
#define XATTR_LUSTRE_LOV	XATTR_LUSTRE_PREFIX"lov"

/*
 * Logging
 */

/* Logging levels. The default is LLAPI_MSG_OFF. */
enum llapi_message_level {
	LLAPI_MSG_OFF    = 0,
	LLAPI_MSG_FATAL,
	LLAPI_MSG_ERROR,
	LLAPI_MSG_WARN,
	LLAPI_MSG_NORMAL,
	LLAPI_MSG_INFO,
	LLAPI_MSG_DEBUG
};

void llapi_msg_set_level(enum llapi_message_level level);
enum llapi_message_level llapi_msg_get_level(void);
typedef void (*llapi_log_callback_t)(enum llapi_message_level level, int err,
				     const char *fmt, va_list args);
void llapi_msg_callback_set(llapi_log_callback_t cb);

/*
 * Open / close a filesystem
 */
struct lustre_fs_h;
void llapi_close_fs(struct lustre_fs_h *lfsh);
int llapi_open_fs(const char *mount_path, struct lustre_fs_h **lfsh);
const char *llapi_get_fsname(const struct lustre_fs_h *lfsh);
const char *llapi_get_mountpoint(const struct lustre_fs_h *lfsh);

/*
 * Layouts
 */
struct llapi_layout;
struct llapi_layout *llapi_layout_get_by_path(const char *path, uint32_t flags);
struct llapi_layout *llapi_layout_get_by_fd(int fd, uint32_t flags);
struct llapi_layout *llapi_layout_get_by_fid(const struct lustre_fs_h *lfsh,
					     const lustre_fid *fid,
					     uint32_t flags);
struct llapi_layout *llapi_layout_alloc(void);
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

/*
 * Misc
 */
int llapi_fid2path(const struct lustre_fs_h *lfsh, const lustre_fid *fid,
		   char *path, size_t path_len, long long *recno,
		   unsigned int *linkno);
int llapi_fd2fid(int fd, lustre_fid *fid);
int llapi_path2fid(const char *path, lustre_fid *fid);
int llapi_open_by_fid(const struct lustre_fs_h *lfsh,
		      const lustre_fid *fid, int open_flags);
int llapi_get_mdt_index_by_fid(const struct lustre_fs_h *lfsh,
			       const struct lu_fid *fid);
int llapi_create_volatile_by_fid(const struct lustre_fs_h *lfsh,
				 const lustre_fid *parent_fid,
				 int mdt_idx, int open_flags, mode_t mode,
				 const struct llapi_layout *layout);
int llapi_parse_size(const char *string, unsigned long long *size,
		     unsigned long long *size_units, bool b_is_bytes);

int llapi_fd2parent(int fd, unsigned int linkno, lustre_fid *parent_fid,
		    char *parent_name, size_t parent_name_len);
int llapi_fid2parent(const struct lustre_fs_h *lfsh,
		     const lustre_fid *fid,
		     unsigned int linkno,
		     lustre_fid *parent_fid,
		     char *parent_name, size_t parent_name_len);
int llapi_path2parent(const char *path, unsigned int linkno,
		      lustre_fid *parent_fid,
		      char *parent_name, size_t parent_name_len);

/*
 * HSM user interface
 */

/* Maximum number of copytools. */
#define LL_HSM_MAX_ARCHIVE (sizeof(__u32) * 8)

struct hsm_user_request *llapi_hsm_user_request_alloc(int itemcount,
						      int data_len);
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
int llapi_hsm_register_event_fifo(const char *path);
int llapi_hsm_unregister_event_fifo(const char *path);
void llapi_hsm_log_error(enum llapi_message_level level, int _rc,
			 const char *fmt, va_list args);

#define LLAPI_ROUNDUP8(val) (((val) + 7) & (~0x7))

static inline struct hsm_action_item *hai_first(struct hsm_action_list *hal)
{
	return (struct hsm_action_item *)
		(hal->hal_fsname + LLAPI_ROUNDUP8(strlen(hal->hal_fsname) + 1));
}

static inline struct hsm_action_item *hai_next(struct hsm_action_item *hai)
{
	return (struct hsm_action_item *)
		((char *)hai + LLAPI_ROUNDUP8(hai->hai_len));
}

#undef LLAPI_ROUNDUP8

/*
 * HSM copytool interface.
 * priv is private state, managed internally by these functions
 */
struct hsm_copytool_private;
struct hsm_copyaction_private;

int llapi_hsm_copytool_register(const struct lustre_fs_h *lfsh,
				struct hsm_copytool_private **priv,
				int archive_count, int *archives,
				int rfd_flags);
int llapi_hsm_copytool_unregister(struct hsm_copytool_private **priv);
int llapi_hsm_copytool_get_fd(struct hsm_copytool_private *ct);
int llapi_hsm_copytool_recv(struct hsm_copytool_private *priv,
			    struct hsm_action_list **hal, size_t *msgsize);
int llapi_hsm_action_begin(struct hsm_copyaction_private **phcp,
			   const struct hsm_copytool_private *ct,
			   const struct hsm_action_item *hai,
			   int restore_mdt_index, int restore_open_flags,
			   bool is_error);
int llapi_hsm_action_end(struct hsm_copyaction_private **phcp,
			 const struct hsm_extent *he,
			 int hp_flags, int errval);
int llapi_hsm_action_progress(struct hsm_copyaction_private *hcp,
			      const struct hsm_extent *he, __u64 total,
			      int hp_flags);
int llapi_hsm_action_get_dfid(const struct hsm_copyaction_private *hcp,
			      lustre_fid *fid);
int llapi_hsm_action_get_fd(const struct hsm_copyaction_private *hcp);
int llapi_hsm_import(const char *dst, int archive, const struct stat *st,
		     unsigned long long stripe_size, int stripe_offset,
		     int stripe_count, int stripe_pattern,
		     const char *pool_name, lustre_fid *newfid);

#endif
