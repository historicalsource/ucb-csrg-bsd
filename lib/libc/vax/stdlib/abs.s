/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)abs.s	5.5 (Berkeley) 4/12/91"
#endif /* LIBC_SCCS and not lint */

ENTRY(abs)
	movl	4(ap),r0
	jgeq	1f
	mnegl	r0,r0
1:
	ret
