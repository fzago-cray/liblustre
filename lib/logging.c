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
 * @brief Logging interface for liblustre error messages
 */

#include <errno.h>
#include <stdlib.h>

#include <lustre/lustre.h>

#include "internal.h"

unsigned int log_level = LUS_LOG_OFF;
lus_log_callback_t log_msg_callback;

/**
 * Set the log level.
 * If the level is invalid, it will be clipped to a valid value.
 *
 * \param[in]  level    new logging level
 */
void lus_log_set_level(enum lus_log_level level)
{
	if (level < LUS_LOG_OFF)
		log_level = LUS_LOG_OFF;
	else if (level >= LUS_LOG_DEBUG)
		log_level = LUS_LOG_DEBUG;
	else
		log_level = level;
}

/**
 * Return the currently set log level
 *
 * \retval      the current log level
 */
enum lus_log_level lus_log_get_level(void)
{
	return log_level;
}

/* Send log messages to the callback. This function should only be
 * called through log_msg() as it validates the callback.
 * Preserve errno. */
void log_msg_internal(enum lus_log_level level, int err, const char *fmt, ...)
{
	va_list args;
	int errno_org = errno;

	va_start(args, fmt);
	log_msg_callback(level, err, fmt, args);
	va_end(args);

	errno = errno_org;
}

/**
 * An application would set a callback to retrieve logging message from
 * the library. It can be set to NULL to stop receiving.
 *
 * \param[in]  cb    new logging callback
 */
void lus_log_set_callback(lus_log_callback_t cb)
{
	log_msg_callback = cb;
}
