/*-
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#include "SYS.h"

#if defined(LIBC_SCCS) && !defined(lint)
	ASMSTR "@(#)_exit.s	5.8 (Berkeley) 5/31/90"
#endif /* LIBC_SCCS and not lint */

PSEUDO(_exit,exit)	/* _exit(status) */
