/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)timeb.h	7.2 (Berkeley) 5/5/91
 */

/* The ftime(2) system call structure -- deprecated. */
struct timeb {
	time_t	time;			/* seconds since the Epoch */
	unsigned short millitm;		/* + milliseconds since the Epoch */
	short	timezone;		/* minutes west of CUT */
	short	dstflag;		/* DST == non-zero */
};
