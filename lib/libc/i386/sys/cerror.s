/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * %sccs.include.redist.c%
 */

#if defined(SYSLIBC_SCCS) && !defined(lint)
	.asciz "@(#)cerror.s	5.1 (Berkeley) 4/23/90"
#endif /* SYSLIBC_SCCS and not lint */

#include "SYS.h"

	.globl	_errno
cerror:
	movl	%eax,_errno
	movl	$-1,%eax
	ret
