/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * %sccs.include.redist.c%
 */

#include "SYS.h"

#if defined(LIBC_SCCS) && !defined(lint)
	ASMSTR("@(#)pipe.s	5.1 (Berkeley) 2/29/92")
#endif /* LIBC_SCCS and not lint */

LEAF(pipe)
	li	v0, SYS_pipe	# pipe(fildes) int fildes[2];
	syscall
	bne	a3, zero, 1f
	sw	v0, 0(a0)	# store the two file descriptors
	sw	v1, 4(a0)
	move	v0, zero
	j	ra
1:
	j	_cerror
END(pipe)
