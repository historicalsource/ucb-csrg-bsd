/*-
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)putsi.c	4.2 (Berkeley) 4/22/91";
#endif /* not lint */

#include <stdio.h>
putsi(a){
	putc((char)a,stdout);
	putc((char)(a>>8),stdout);
}
