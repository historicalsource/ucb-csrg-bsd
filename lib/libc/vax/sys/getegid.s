/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz	"@(#)getegid.s	5.2 (Berkeley) 6/5/85"
#endif not lint

#include "SYS.h"

PSEUDO(getegid,getgid)
	movl	r1,r0
	ret		# egid = getegid();
