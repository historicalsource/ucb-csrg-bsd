/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid:.asciz	"@(#)wait.s	5.1 (Berkeley) 7/2/86"
#endif SYSLIBC_SCCS

#include "SYS.h"

SYSCALL(wait)
	tstl	4(fp)
	jeql	1f
	movl	r1,*4(fp)
1:
	ret
