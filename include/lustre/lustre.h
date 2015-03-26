/*
 * LGPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * (C) Copyright 2012 Commissariat a l'energie atomique et aux energies
 *     alternatives
 * Copyright 2015 Cray Inc., All rights reserved
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

#include <lustre/lustre_user.h>

/* Lustre extended attributes */
#define XATTR_LUSTRE_PREFIX	"lustre."
#define XATTR_LUSTRE_LOV	XATTR_LUSTRE_PREFIX"lov"

/*
 * Logging 
 */

/* lustreapi message severity level */
enum llapi_message_level {
        LLAPI_MSG_OFF    = 0,
        LLAPI_MSG_FATAL  = 1,
        LLAPI_MSG_ERROR  = 2,
        LLAPI_MSG_WARN   = 3,
        LLAPI_MSG_NORMAL = 4,
        LLAPI_MSG_INFO   = 5,
        LLAPI_MSG_DEBUG  = 6,
        LLAPI_MSG_MAX
};
#define llapi_err_noerrno(level, fmt, a...)			\
	llapi_error((level) | LLAPI_MSG_NO_ERRNO, 0, fmt, ## a)

#define LLAPI_MSG_MASK          0x00000007
#define LLAPI_MSG_NO_ERRNO      0x00000010

typedef void (*llapi_log_callback_t)(enum llapi_message_level level, int err,
                                     const char *fmt, va_list ap);
llapi_log_callback_t llapi_error_callback_set(llapi_log_callback_t cb);
void llapi_msg_set_level(int level);

/* TODO: should that really be exported. */
void llapi_error(enum llapi_message_level level, int err, const char *fmt, ...);

/*
 * Open / close a filesystem
 */
struct lustre_fs_h;
void lustre_close_fs(struct lustre_fs_h *lfsh);
int lustre_open_fs(const char *mount_path, struct lustre_fs_h **lfsh);
const char *llapi_get_fsname(const struct lustre_fs_h *lfsh);

/*
 * Misc
 */
struct llapi_stripe_param {
        unsigned long long      lsp_stripe_size;
        char                    *lsp_pool;
        int                     lsp_stripe_offset;
        int                     lsp_stripe_pattern;
        int                     lsp_stripe_count;
        bool                    lsp_is_specific;
        __u32                   lsp_osts[0];
};
int llapi_fid2path(const struct lustre_fs_h *lfsh, const struct lu_fid *fid,
		   char *path, int path_len, long long *recno, int *linkno);
int llapi_fd2fid(const int fd, lustre_fid *fid);
int llapi_open_by_fid(const struct lustre_fs_h *lfsh,
		      const lustre_fid *fid, int open_flags);
int llapi_file_open_pool(const char *name, int flags, int mode,
			 unsigned long long stripe_size, int stripe_offset,
			 int stripe_count, int stripe_pattern, char *pool_name);
int llapi_get_mdt_index_by_fid(const struct lustre_fs_h *lfsh,
			       const struct lu_fid *fid, int *mdt_index);
int llapi_create_volatile_idx(char *directory, int idx, int open_flags);

int llapi_parse_size(const char *optarg, unsigned long long *size,
		     unsigned long long *size_units, int bytes_spec);

/* TODO */
bool fid_is_norm(const struct lu_fid *fid);
bool fid_is_igif(const struct lu_fid *fid);

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

/* TODO - is there a standard function to do that? */
static inline int llapi_roundup8(int val)
{
        return (val + 7) & (~0x7);
}

static inline struct hsm_action_item *hai_first(struct hsm_action_list *hal)
{
        return (struct hsm_action_item *)
		(hal->hal_fsname + llapi_roundup8(strlen(hal->hal_fsname) + 1));
}

static inline struct hsm_action_item *hai_next(struct hsm_action_item *hai)
{
        return (struct hsm_action_item *)
		((char *)hai + llapi_roundup8(hai->hai_len));
}

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
			    struct hsm_action_list **hal, int *msgsize);
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
		     char *pool_name, lustre_fid *newfid);

#endif
