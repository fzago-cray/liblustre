/*
 * An alternate Lustre user library.
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

/*
 * Miscellaneous functions.
 *
 * strscpy and strscat should not be exported, but the copytool needs
 * them. Eventually they should be rolled in the library, and copytool
 * could use its own copy, or just compile support.c too.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <lustre/lustre.h>

#include "internal.h"

/**
 * A truncation safe version of strcpy.
 *
 * \param[in]  dst         a pointer to a buffer, of size dst_len
 * \param[in]  src         a NUL terminated C string
 * \param[in]  dst_size    size in bytes of dst
 *
 * \retval  -ENOSPC if truncation would occur. dst is left untouched.
 * \retval  size of the string copied, not including the NUL
 *          terminator. The copy was successful, NUL terminated and
 *          not truncated.
 */
ssize_t strscpy(char *dst, const char *src, size_t dst_size)
{
	ssize_t src_len = strlen(src);

	if ((src_len + 1) > dst_size)
		return -ENOSPC;

	memcpy(dst, src, src_len + 1);

	return src_len;
}

/**
 * A truncation safe version of strcat.
 *
 * \param[in]  dst         a pointer to a buffer, of size dst_len
 * \param[in]  src         a NUL terminated C string
 * \param[in]  dst_size    size in bytes of dst
 *
 * \retval  -ENOSPC if truncation would occur. dst is left untouched.
 * \retval  size of the new string, not including the NUL
 *          terminator. The concatenation was successful, NUL
 *          terminated and not truncated.
 */
ssize_t strscat(char *dst, const char *src, size_t dst_size)
{
	ssize_t src_len = strlen(src);
	size_t dst_len = strlen(dst);

	if ((src_len + 1) > (dst_size - dst_len))
		return -ENOSPC;

	memcpy(dst + dst_len, src, src_len + 1);

	return src_len + dst_len;
}
