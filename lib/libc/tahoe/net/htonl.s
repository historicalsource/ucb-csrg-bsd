/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)htonl.s	5.1 (Berkeley) 4/12/91"
#endif /* LIBC_SCCS and not lint */

/* netorder = htonl(hostorder) */

#include "DEFS.h"

ENTRY(htonl, 0)
	movl	4(fp),r0
	ret
