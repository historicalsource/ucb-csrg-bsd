/*-
 * Copyright (c) 1979 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)LLIMIT.c	1.4 (Berkeley) 4/9/90";
#endif /* not lint */

#include "h00vars.h"

LLIMIT(curfile, limit)

	register struct iorec	*curfile;
	long			limit;
{
	if (limit <= 0)
		limit = 0x7fffffff;
	curfile->llimit = limit;
	if (curfile->lcount >= curfile->llimit) {
		ERROR("%s: Line limit exceeded\n", curfile->pfname);
		return;
	}
}
