/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)adddf3.s	5.1 (Berkeley) 6/7/90"
#endif /* LIBC_SCCS and not lint */

#include "DEFS.h"

/* double + double */
ENTRY(__adddf3)
	fmoved	sp@(4),fp0
	faddd	sp@(12),fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	rts
