/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)wwend.c	3.14 (Berkeley) 6/2/90";
#endif /* not lint */

#include "ww.h"
#include "tt.h"

wwend()
{
	xxend();
	(void) wwsettty(0, &wwoldtty, &wwnewtty);
}
