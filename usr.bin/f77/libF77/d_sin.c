/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)d_sin.c	5.2 (Berkeley) 4/12/91";
#endif /* not lint */

double d_sin(x)
double *x;
{
double sin();
return( sin(*x) );
}
