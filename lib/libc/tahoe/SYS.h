/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)SYS.h	5.3 (Berkeley) 5/20/88
 */

#include <sys/syscall.h>

#ifdef PROF
#define	ENTRY(x)	.globl _/**/x; .align 2; _/**/x: .word 0; \
			.data; 1:; .long 0; .text; pushl $1b; callf $8,mcount
#else
#define	ENTRY(x)	.globl _/**/x; .align 2; _/**/x: .word 0
#endif PROF
#define	SYSCALL(x)	err: jmp cerror; ENTRY(x); kcall $SYS_/**/x; jcs err
#define	PSEUDO(x,y)	ENTRY(x); kcall $SYS_/**/y
#define	CALL(x,y)	calls $x, _/**/y

	.globl	cerror
