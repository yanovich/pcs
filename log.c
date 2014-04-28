/* log.c -- log formatted messages of varying severity
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 *
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"

static int log_level = LOG_NOTICE;
static int log_on_stderr = 1;
static char *argv0;

int
log_get_level(void)
{
	return log_level;
}

/* Report fatal message, never return */
void
fatal(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_CRIT, fmt, args);
	va_end(args);
	_exit(255);
}

/* Error messages that should be logged. */
void
error(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_ERR, fmt, args);
	va_end(args);
}

/* Warning messages that should be logged. */
void
warn(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_WARNING, fmt, args);
	va_end(args);
}

/* Log this message (information that usually should go to the log). */
void
logit(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_NOTICE, fmt, args);
	va_end(args);
}

/* More detailed messages (information that does not need to go to the log). */
void
verbose(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_INFO, fmt, args);
	va_end(args);
}

/* Debugging messages that should not be logged during normal operation. */
void
debug(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_DEBUG, fmt, args);
	va_end(args);
}

void
debug2(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_DEBUG + 1, fmt, args);
	va_end(args);
}

void
debug3(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(LOG_DEBUG + 2, fmt, args);
	va_end(args);
}

/*
 * Initialize the log.
 */
void
log_init(char *av0, int level, int facility, int on_stderr)
{
	log_level = level;
	log_on_stderr = on_stderr;

	if (log_on_stderr)
		return;

	openlog(argv0, 0, facility);
}

#define MSGBUFSIZ 1024

void
do_log2(int level, const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(level, fmt, args);
	va_end(args);
}

void
do_log(int level, const char *fmt, va_list args)
{
	char fmtbuf[MSGBUFSIZ];
	char *txt = NULL;
	int saved_errno = errno;

	if (level > log_level)
		return;

	if (level > LOG_DEBUG)
		level = LOG_DEBUG;

	if (level == LOG_DEBUG)
		txt = "debug";
	else if (level <= LOG_CRIT)
		txt = "fatal";

	if (txt != NULL) {
		snprintf(fmtbuf, sizeof(fmtbuf), "%s: %s", txt, fmt);
		fmt = fmtbuf;
	}

	if (log_on_stderr) {
		vfprintf(stderr, fmt, args);
	} else {
		vsyslog(level, fmt, args);
	}

	errno = saved_errno;
}
