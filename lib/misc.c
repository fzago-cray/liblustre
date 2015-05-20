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

/* Miscellaneous functions. */

#include <stdlib.h>
#include <errno.h>
#include <limits.h>

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

#ifdef UNIT_TEST
#include "../tests/test_misc.c"
#endif
