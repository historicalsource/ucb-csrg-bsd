/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)strlen.s	5.6 (Berkeley) 6/1/90"
#endif /* LIBC_SCCS and not lint */

/*
 * Return the length of cp (not counting '\0').
 *
 * strlen(cp)
 *	char *cp;
 */
#include "DEFS.h"

ENTRY(strlen, 0)
	movl	4(ap),r1
1:
	locc	$0,$65535,(r1)	# look for '\0'
	beql	1b
	subl3	4(ap),r1,r0	# len = cp - base
	ret
