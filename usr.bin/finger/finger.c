/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tony Nardo of the Johns Hopkins University/Applied Physics Lab.
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)finger.c	5.14 (Berkeley) 5/7/89";
#endif /* not lint */

/*
 * Finger prints out information about users.  It is not portable since
 * certain fields (e.g. the full user name, office, and phone numbers) are
 * extracted from the gecos field of the passwd file which other UNIXes
 * may not have or may use for other things.
 *
 * There are currently two output formats; the short format is one line
 * per user and displays login name, tty, login time, real name, idle time,
 * and office location/phone number.  The long format gives the same
 * information (in a more legible format) as well as home directory, shell,
 * mail info, and .plan/.project files.
 */

#include <sys/param.h>
#include <sys/file.h>
#include <stdio.h>
#include "finger.h"
#include "pathnames.h"

time_t now;
int lflag, sflag, mflag, pplan;
char tbuf[1024];

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	int ch;
	time_t time();

	while ((ch = getopt(argc, argv, "lmps")) != EOF)
		switch(ch) {
		case 'l':
			lflag = 1;		/* long format */
			break;
		case 'm':
			mflag = 1;		/* force exact match of names */
			break;
		case 'p':
			pplan = 1;		/* don't show .plan/.project */
			break;
		case 's':
			sflag = 1;		/* short format */
			break;
		case '?':
		default:
			(void)fprintf(stderr,
			    "usage: finger [-lmps] [login ...]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;

	(void)time(&now);
	setpassent(1);
	if (!*argv) {
		/*
		 * Assign explicit "small" format if no names given and -l
		 * not selected.  Force the -s BEFORE we get names so proper
		 * screening will be done.
		 */
		if (!lflag)
			sflag = 1;	/* if -l not explicit, force -s */
		loginlist();
		if (entries == 0)
			(void)printf("No one logged on.\n");
	} else {
		userlist(argv);
		/*
		 * Assign explicit "large" format if names given and -s not
		 * explicitly stated.  Force the -l AFTER we get names so any
		 * remote finger attempts specified won't be mishandled.
		 */
		if (!sflag)
			lflag = 1;	/* if -s not explicit, force -l */
	}
	if (entries != 0) {
		if (lflag)
			lflag_print();
		else
			sflag_print();
	}
	exit(0);
}

loginlist()
{
	register PERSON *pn;
	register int fd;
	struct passwd *pw;
	struct utmp user;
	char name[UT_NAMESIZE + 1];

	if ((fd = open(_PATH_UTMP, O_RDONLY, 0)) < 0) {
		(void)fprintf(stderr, "finger: can't read %s.\n", _PATH_UTMP);
		exit(2);
	}
	name[UT_NAMESIZE] = NULL;
	while (read(fd, (char *)&user, sizeof(user)) == sizeof(user)) {
		if (!user.ut_name[0])
			continue;
		if ((pn = find_person(user.ut_name)) == NULL) {
			bcopy(user.ut_name, name, UT_NAMESIZE);
			if ((pw = getpwnam(name)) == NULL)
				continue;
			pn = enter_person(pw);
		}
		enter_where(&user, pn);
	}
	(void)close(fd);
	for (pn = phead; lflag && pn != NULL; pn = pn->next)
		enter_lastlog(pn);
}

#define	ARGIGNORE	(char *)0x01
userlist(argv)
	char **argv;
{
	register char **ap;
	register PERSON *pn;
	PERSON *nethead;
	struct utmp user;
	struct passwd *pw;
	int fd, dolocal, *index();

	/* pull out all network requests */
	for (ap = argv, dolocal = 0, nethead = NULL; *ap != NULL; ap++) {
		if (!index(*ap, '@')) {
			dolocal = 1;
			continue;
		}
		pn = palloc();
		pn->next = nethead;
		nethead = pn;
		pn->name = *ap;
		*ap = ARGIGNORE;
	}

	if (!dolocal)
		goto net;

	/*
	 * traverse the list of possible login names and check the login name
	 * and real name against the name specified by the user.
	 */
	if (mflag)
		for (ap = argv; *ap != NULL; ap++) {
			if (*ap == ARGIGNORE)
				continue;
			if ((pw = getpwnam(*ap)) == NULL)
				continue;
			enter_person(pw);
			*ap = ARGIGNORE;
		}
	else while (pw = getpwent())
		for (ap = argv; *ap != NULL; ap++) {
			if (*ap == ARGIGNORE)
				continue;
			if (strcasecmp(pw->pw_name, *ap) && !match(pw, *ap))
				continue;
			enter_person(pw);
			*ap = ARGIGNORE;
			/* don't break, may be listed multiple times */
		}

	/* list errors */
	for (ap = argv; *ap != NULL; ap++)
		if (*ap != ARGIGNORE)
			(void)fprintf(stderr,
			    "finger: %s: no such user.\n", *ap);

	/* handle network requests */
net:	for (pn = nethead; pn; pn = pn->next) {
		netfinger(pn->name);
		if (pn->next || entries)
			putchar('\n');
	}

	if (entries == 0)
		return;

	/*
	 * Scan thru the list of users currently logged in, saving
	 * appropriate data whenever a match occurs.
	 */
	if ((fd = open(_PATH_UTMP, O_RDONLY, 0)) < 0) {
		(void)fprintf( stderr, "finger: can't read %s.\n", _PATH_UTMP);
		exit(1);
	}
	while (read(fd, (char *)&user, sizeof(user)) == sizeof(user)) {
		if (!user.ut_name[0])
			continue;
		if ((pn = find_person(user.ut_name)) == NULL)
			continue;
		enter_where(&user, pn);
	}
	(void)close(fd);
	for (pn = phead; pn != NULL; pn = pn->next)
		enter_lastlog(pn);
}
