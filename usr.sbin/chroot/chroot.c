/*
 * Copyright (c) 1988 The Regents of the University of California.
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)chroot.c	5.6 (Berkeley) 2/21/90";
#endif /* not lint */

#include <stdio.h>
#include <paths.h>

main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno;
	char *shell, *getenv(), *strerror();

	if (argc < 2) {
		(void)fprintf(stderr, "usage: chroot newroot [command]\n");
		exit(1);
	}
	if (chdir(argv[1]) || chroot(argv[1]))
		fatal(argv[1]);
	if (argv[2]) {
		execvp(argv[2], &argv[2]);
		fatal(argv[2]);
	} else {
		if (!(shell = getenv("SHELL")))
			shell = _PATH_BSHELL;
		execlp(shell, shell, "-i", (char *)NULL);
		fatal(shell);
	}
	/* NOTREACHED */
}

fatal(msg)
	char *msg;
{
	extern int errno;

	(void)fprintf(stderr, "chroot: %s: %s\n", msg, strerror(errno));
	exit(1);
}
