/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)r_sign.c	5.3 (Berkeley) 4/12/91";
#endif /* not lint */

float r_sign(a,b)
float *a, *b;
{
float x;
x = (*a >= 0 ? *a : - *a);
return( *b >= 0 ? x : -x);
}
