/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)main.h	5.1 (Berkeley) 3/7/91
 */

extern int rootpid;	/* pid of main shell */
extern int rootshell;	/* true if we aren't a child of the main shell */

#ifdef __STDC__
void readcmdfile(char *);
void cmdloop(int);
#else
void readcmdfile();
void cmdloop();
#endif
