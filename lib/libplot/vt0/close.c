/*-
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)close.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

extern vti;
closevt(){
	close(vti);
}
closepl(){
	close(vti);
}
