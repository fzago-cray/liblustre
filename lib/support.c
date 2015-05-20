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
#include <ctype.h>

#include <lustre/lustre.h>

#include "internal.h"

/**
 * Parse a user given string containing a size with or without a
 * specifier, such as "1024", "128K" or "72P". Valid specifier are 'b'
 * for block (if b_is_bytes is false) or bytes (if b_is_bytes is
 * true); 'K' or 'k' for kilobytes; 'M' or 'm' for megabytes, 'G' or
 * 'g' for gigabytes, 'T' or 't' for terabytes; 'P' or 'p' for
 * petabytes; and 'E' or 'e' for exabytes.
 *
 * \param  string[in]    the input string
 * \param  size[out]     size found, always in bytes
 * \param  size_units[in][out]    unit found, if one was specified in the
 *                       string, otherwise this value will not change,
 *                       unless the caller set it to 0, in which case
 *                       it will be set to 1.
 * \param  b_is_bytes    if b is present in the string, indicate
 *                       whether it means 'bytes' or 'block' of 512 bytes
 */
/* TODO: Does an application even care about size_units? Can that be removed? */
int llapi_parse_size(const char *string, unsigned long long *size,
		     unsigned long long *size_units, bool b_is_bytes)
{
	char *endptr;
	const char *specifiers = "bbKkMmGgTtPpEe"; /* b is doubled */
	unsigned long long val;
	unsigned int unit;
	int i;
	const char *p;

	/* Make sure the number is not negative. Unfortunately
	 * strtoull will silently convert such number to a
	 * positive. */
	p = string;
	while(isspace(*p))
		p++;
	if (*p == '-')
		return -1;

	errno = 0;
	val = strtoull(string, &endptr, 0);

	/* Check for various possible errors */
	if ((errno == ERANGE && val == ULLONG_MAX)
	    || (errno != 0 && val == 0))
		return -1;

	/* Empty string? */
	if (endptr == string)
		return -1;

	/* Number is valid. Look for specifier. */
	if (*endptr == '\0') {
		/* No specifier. */
		if (*size_units == 0)
			*size_units = 1;
		*size = val * *size_units;
		return 0;
	}

	if (*(endptr+1) !=  '\0') {
		/* extra garbage. Specifier is only 1 char. Don't
		 * bother continuing. */
		return -1;
	}

	/* TODO: b can mean block or bytes! Analyse usage and make
	 * b=block only if possible. */
	if (*endptr == 'b' && b_is_bytes == false) {
		/* Requested value in block size */
		/* Check for overflow. ie. is val*512>2^64 ? */
		if (val > ULLONG_MAX / 512)
			return -1;

		*size = val * 512;
		*size_units = 512;

		return 0;
	}

	unit = 0;
	for (i=0; i<strlen(specifiers); i += 2) {
		/* Test both lower and upper letters. */
		if (*endptr == specifiers[i] ||
		    *endptr == specifiers[i+1]) {
			/* Check for overflow. ie. is val*unit>2^64 */
			if (val > (ULLONG_MAX >> unit))
				return -1;

			*size = val * (1ULL << unit);
			*size_units = (1ULL << unit);

			return 0;
		}

		unit += 10;	/* 2^10 */
	}

	return -1;
}

#ifdef UNIT_TEST
#include "../tests/test_support.c"
#endif
