/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)besyn_.c	5.2 (Berkeley) 4/12/91";
#endif /* not lint */

double yn();

float besyn_(n, x)
long *n; float *x;
{
	return((float)yn((int)*n, (double)*x));
}
