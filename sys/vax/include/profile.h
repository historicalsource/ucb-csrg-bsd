/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)profile.h	7.1 (Berkeley) 7/16/92
 */

#define	_MCOUNT_DECL static void _mcount

#define	MCOUNT \
asm(".text; .globl mcount; mcount: pushl 16(fp); calls $1,__mcount; rsb");
