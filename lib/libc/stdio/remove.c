/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)remove.c	5.2 (Berkeley) 2/5/91";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>

remove(file)
	const char *file;
{
	return (unlink(file));
}
