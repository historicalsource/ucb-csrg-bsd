/*-
 * Copyright (c) 1979, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)blkclr.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

blkclr(at, siz)
	register char	*at;
	long		siz;
{
	register int	size = siz;

	while(size-- > 0)
		*at++ = 0;
}
