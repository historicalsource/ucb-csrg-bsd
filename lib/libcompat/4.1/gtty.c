/*-
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gtty.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

/*
 * Writearound to old gtty system call.
 */

#include <sgtty.h>

gtty(fd, ap)
	struct sgtty *ap;
{

	return(ioctl(fd, TIOCGETP, ap));
}
