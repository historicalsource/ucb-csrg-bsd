/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)htons.s	5.6 (Berkeley) 6/1/90"
#endif /* LIBC_SCCS and not lint */

/* hostorder = htons(netorder) */

#include "DEFS.h"

ENTRY(htons, 0)
	rotl	$8,4(ap),r0
	movb	5(ap),r0
	movzwl	r0,r0
	ret
