/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid:.asciz	"@(#)getgroups.s	5.3 (Berkeley) 3/9/86"
#endif SYSLIBC_SCCS

#include "SYS.h"

SYSCALL(getgroups)
	ret		# ngroups = getgroups(gidsetsize, gidset)
