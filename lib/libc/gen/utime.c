/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)utime.c	5.5 (Berkeley) 3/5/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/time.h>

#include <utime.h>

int
utime(path, times)
	const char *path;
	const struct utimbuf *times;
{
	struct timeval tv[2], *tvp;

	if (times) {
		tv[0].tv_sec = times->actime;
		tv[1].tv_sec = times->modtime;
		tv[0].tv_usec = tv[1].tv_usec = 0;
		tvp = tv;
	} else
		tvp = NULL;
	return (utimes(path, tvp));
}
