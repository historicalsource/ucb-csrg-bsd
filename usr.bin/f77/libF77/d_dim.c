/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)d_dim.c	5.2 (Berkeley) 4/12/91";
#endif /* not lint */

double d_dim(a,b)
double *a, *b;
{
return( *a > *b ? *a - *b : 0);
}
