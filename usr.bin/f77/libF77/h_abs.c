/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)h_abs.c	5.2 (Berkeley) 4/12/91";
#endif /* not lint */

short h_abs(x)
short *x;
{
if(*x >= 0)
	return(*x);
return(- *x);
}
