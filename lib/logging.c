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

#include <errno.h>
#include <stdlib.h>

#include <lustre/lustre.h>

#include "internal.h"

unsigned int log_level = LLAPI_MSG_OFF;
llapi_log_callback_t log_msg_callback = NULL;

/**
 * Set the log level.
 * If the level is invalid, it will be clipped to a valid value.
 *
 * \param[in]  level    new logging level
 */
void llapi_msg_set_level(enum llapi_message_level level)
{
	if (level < LLAPI_MSG_OFF)
		log_level = LLAPI_MSG_OFF;
	else if (level >= LLAPI_MSG_DEBUG)
		log_level = LLAPI_MSG_DEBUG;
	else
		log_level = level;
}

/**
 * Return the currently set log level
 *
 * \retval      the current log level
 */
enum llapi_message_level llapi_msg_get_level(void)
{
	return log_level;
}

/* Send log messages to the callback. This function should only be
 * called through log_msg() as it validates the callback.
 * Preserve errno. */
void
log_msg_internal(enum llapi_message_level level, int err, const char *fmt, ...)
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
void llapi_msg_callback_set(llapi_log_callback_t cb)
{
	log_msg_callback = cb;
}
