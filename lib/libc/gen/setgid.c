/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)setgid.c	5.1 (Berkeley) 6/5/85";
#endif not lint

/*
 * Backwards compatible setgid.
 */
setgid(gid)
	int gid;
{

	return (setregid(gid, gid));
}
