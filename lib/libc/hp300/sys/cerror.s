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
	.asciz "@(#)cerror.s	5.1 (Berkeley) 5/12/90"
#endif /* LIBC_SCCS and not lint */

#include "SYS.h"

	.globl	_errno
cerror:
	movl	d0,_errno
	movl	#-1,d0
	rts
