/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)abort.c	5.2 (Berkeley) 7/26/85";
#endif not lint

/* C library -- abort */

#include "signal.h"

abort()
{
	sigblock(~0);
	signal(SIGILL, SIG_DFL);
	sigsetmask(~sigmask(SIGILL));
	kill(getpid(), SIGILL);
}
