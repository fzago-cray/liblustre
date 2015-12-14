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

/**
 * @file
 * @brief Miscellaneous functions.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include <lustre/lustre.h>

#include "internal.h"

/**
 * Removes trailing newlines from a string.
 *
 * \param buf    the string to modify in place
 */
void chomp_string(char *buf)
{
	char *p;

	if (buf == NULL)
		return;

	p = strchr(buf, '\n');
	if (p)
		*p = '\0';
}

/**
 * Indicate whether the liblustreapi_init() constructor below has run or not.
 *
 * This can be used by external programs to ensure the initialization
 * mechanism has actually worked.
 */
bool lus_initialized;

/**
 * Initializes the library. This function is automatically called on
 * application startup. However it is not called is the library is
 * loaded with dlopen(), so the init function has to be called
 * manually in that case.
 */
void llapi_init(void)  __attribute__ ((constructor));
void llapi_init(void)
{
	unsigned int	seed;
	struct timeval	tv;
	int		fd;

	/* Initializes the random number generator (random()). Get
	 * data from different places in case one of them fails. This
	 * is enough to get reasonably random numbers, but is not
	 * strong enough to be used for cryptography. */
	seed = syscall(SYS_gettid);

	if (gettimeofday(&tv, NULL) == 0) {
		seed ^= tv.tv_sec;
		seed ^= tv.tv_usec;
	}

	fd = open("/dev/urandom", O_RDONLY | O_NOFOLLOW);
	if (fd >= 0) {
		unsigned int rnumber;
		ssize_t ret;

		ret = read(fd, &rnumber, sizeof(rnumber));
		seed ^= rnumber ^ ret;
		close(fd);
	}

	srandom(seed);
	lus_initialized = true;
}
