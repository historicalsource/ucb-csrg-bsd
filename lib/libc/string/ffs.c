/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)ffs.c	5.3 (Berkeley) 5/15/90";
#endif /* LIBC_SCCS and not lint */

/*
 * ffs -- vax ffs instruction
 */
ffs(mask)
	register int mask;
{
	register int bit;

	if (mask == 0)
		return(0);
	for (bit = 1; !(mask & 1); bit++)
		mask >>= 1;
	return(bit);
}
