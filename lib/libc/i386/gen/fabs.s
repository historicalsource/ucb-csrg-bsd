/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)fabs.s	5.2 (Berkeley) 12/17/90"
#endif /* LIBC_SCCS and not lint */

#include "DEFS.h"

ENTRY(fabs)
	fldl	4(%esp)
	fabs
	ret
