/*
 * An alternate Lustre user library.
 *
 * Copyright Cray 2015, All rights reserved.
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

#define _ATFILE_SOURCE
#include <fcntl.h>

#include <lustre/lustre.h>

#include "liblustre_internal.h"

/**
 * Open a file given its FID.
 *
 * \param[in]  lfsh   an opened Lustre fs opaque handle
 * \param[in]  fid    the request fid, contained in the above Lustre fs
 * \param[in]  flags  the open flags. See open(2)
 *
 * \retval   a 0 or positive file descriptor on success
 * \retval   a negative errno on error
 */
int llapi_open_by_fid(const struct lustre_fs_h *lfsh,
		      const lustre_fid *fid, int open_flags)
{
        char fidstr[FID_NOBRACE_LEN + 1];

        snprintf(fidstr, sizeof(fidstr), DFID_NOBRACE, PFID(fid));
        return openat(lfsh->fid_fd, fidstr, open_flags);
}
