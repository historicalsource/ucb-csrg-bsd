/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This code is derived from software contributed to Berkeley by
 * Computer Consoles Inc.
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)strcat.s	1.3 (Berkeley) 6/27/88"
#endif /* LIBC_SCCS and not lint */

/* 
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * Return s1.
 * 
 * char * strcat(s1, s2)
 * register char *s1, *s2;
*/
#include "DEFS.h"
	
ENTRY(strcat, 0)
	movl	4(fp),r0
	movl	r0,r1
	cmps2			# r0 and r1 point at null at end of s1
	movl	8(fp),r0	# source string
	movs2
	movl	4(fp),r0
	ret 
