/*
**  Vacation
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983, 1987 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/

#ifndef lint
static char	SccsId[] = "@(#)vacation.c	5.4 (Berkeley) 7/2/87";
#endif not lint

#include <sys/param.h>
#include <sys/file.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>

/*
**  VACATION -- return a message to the sender when on vacation.
**
**	This program could be invoked as a message receiver when someone is
**	on vacation.  It returns a message specified by the user to whoever
**	sent the mail, taking care not to return a message too often to
**	prevent "I am on vacation" loops.
*/

#define	NO	0			/* no/false */
#define	YES	1			/* yes/true */
#define	EOS	'\0'			/* end of string */
#define	MAXLINE	500			/* max line from mail header */
#define	PERIOD	(60L*60L*24L*7L)	/* week between notifications */
#define	VACAT	".vacation"		/* dbm's database prefix */
#define	VDIR	".vacation.dir"		/* dbm's DB prefix, part 1 */
#define	VIGN	".vacation.ignore"	/* addresses never replied to */
#define	VMSG	".vacation.msg"		/* vacation message */
#define	VPAG	".vacation.pag"		/* dbm's DB prefix, part 2 */

typedef struct {
	char	*dptr;
	int	dsize;
} DATUM;

typedef struct {
	time_t	sentdate;
} DBREC;

static int	debug = NO;		/* debugging flag */

main(argc, argv)
	int	argc;
	char	**argv;
{
	extern int	optind;
	struct passwd	*pw;
	int	ch, iflag = NO;
	char	*from,
		*getfrom();

	while ((ch = getopt(argc, argv, "Idi")) != EOF)
		switch((char)ch) {
		case 'd':			/* debug */
			debug = YES;
			break;
		case 'i': case 'I':		/* re-init the database */
			iflag = YES;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc != 1)
		usage();

	/* find and move to user's home directory */
	if (!(pw = getpwnam(*argv))) {
		fprintf(stderr, "vacation: unknown user %s.\n", *argv);
		exit(1);
	}
	if (chdir(pw->pw_dir)) {
		perror("vacation: chdir");
		exit(1);
	}

	/* iflag cleans out the database */
	if (iflag) {
		initialize();
		exit(0);
	}

	/*
	 * if database missing, we create it and do a dbminit;
	 * otherwise, just do the dbminit.
	 */
	if (access(VDIR, F_OK))
		initialize();
	else
		dbminit(VACAT);

	/* find out who sent us mail */
	from = getfrom();

	/* ignore if junk mail */
	if (junkmail(from))
		exit(0);

	/*
	 * ignore if recently replied to this address,
	 * else note the time and send a reply
	 */
	if (!knows(from)) {
		setknows(from, NO);
		sendmessage(from, *argv);
	}
	exit(0);
}

/*
 * getfrom --
 *	read mail "From" line and return sender's address
 */
static char *
getfrom()
{
	register char	*p;
	char	buf[MAXLINE],
		*malloc(), *strcpy();

	/* read the from line */
	if (!gets(buf) || strncmp(buf, "From ", 5)) {
		fputs("vacation: no initial From line.\n", stderr);
		exit(1);
	}

	/* find the end of the sender address and terminate it */
	for (p = &buf[5]; *p && *p != ' '; ++p);
	if (!*p) {
		fprintf(stderr, "vacation: address terminated From line '%s'", buf);
		exit(1);
	}
	*p = EOS;
	if (!(p = malloc((u_int)(strlen(&buf[5]) + 1)))) {
		fputs("vacation: out of space.\n", stderr);
		exit(1);
	}
	/* return the sender address */
	return(strcpy(p, &buf[5]));
}

/*
 * junkmail --
 *	read the header and return if automagic/junk/bulk mail
 */
static
junkmail(from)
	char	*from;
{
	static struct ignore {
		char	*name;
		int	len;
	} ignore[] = {
		"-REQUEST", 8, 		"Postmaster", 10,
		"uucp", 4,		"MAILER-DAEMON", 13,
		"MAILER", 6,		NULL, NULL,
	};
	register struct ignore	*I;
	register int	len;
	register char	*p;
	char	buf[MAXLINE],
		*index();

	/*
	 * This is mildly amusing, and I'm not positive it's right; what
	 * we're trying to do is find the "real" name of the sender.  I'm
	 * assuming that addresses will be some variant of:
	 *
	 * From ADDRESS
	 * From ADDRESS@seismo.css.gov
	 * From ADDRESS%site.BITNET@seismo.css.gov
	 * From site1!site2!ADDRESS@seismo.css.gov
	 *
	 * Therefore, the following search order:
	 */
	if (!(p = index(from, '%')))
		if (!(p = index(from, '@')))
			if (!(p = index(from, '!')))
				for (p = from; *p; ++p);
	len = p - from + 1;
	for (I = ignore; I->name; ++I)
		if (len >= I->len && !strcasencmp(I->name, p - I->len, I->len))
			return(YES);

	/* read the header looking for a "Precedence:" line */
	while (gets(buf) && *buf) {
		if (strcasencmp(buf, "Precedence", 10) ||
		   buf[10] != ':' && buf[10] != ' ' && buf[10] != '\t')
			continue;

		/* find the value of this field */
		if (!(p = index(buf, ':')))
			continue;
		while (*++p && isspace(*p));
		if (!*p)
			continue;

		/* see if it is "junk" or "bulk" */
		if (!strcasencmp(p, "junk", 4) || !strcasecmp(p, "bulk", 4)) {
			puts("found junk or bulk");
			return(YES);
		}
	}
	return(NO);
}

/*
 * knows --
 *	find out if user has gotten a vacation message recently.
 */
static
knows(user)
	char	*user;
{
	DATUM	k, d,
		fetch();
	time_t	now, then,
		time();

	k.dptr = user;
	k.dsize = strlen(user) + 1;
	d = fetch(k);
	if (d.dptr) {
		/*
		 * be careful on 68k's and others with alignment
		 * restrictions
		 */
		bcopy((char *)&((DBREC *)d.dptr)->sentdate, (char *)&then, sizeof(then));
		(void)time(&now);
		if (!then || then + PERIOD > now)
			return(YES);
	}
	return(NO);
}

/*
 * setknows --
 *	store that this user knows about the vacation.
 */
static
setknows(user, forever)
	char	*user;
	int	forever;
{
	DBREC	xrec;
	DATUM	k, d;
	time_t	time();

	k.dptr = user;
	k.dsize = strlen(user) + 1;
	if (forever)
		/* zero is the flag value meaning *never* reply */
		xrec.sentdate = 0;
	else
		(void)time(&xrec.sentdate);
	d.dptr = (char *)&xrec;
	d.dsize = sizeof(xrec);
	store(k, d);
}

/*
 * sendmessage --
 *	exec sendmail to send the vacation file to the user "user".
 */
static
sendmessage(user, myname)
	char	*user, *myname;
{
	if (debug)
		printf("sending {%s} to {%s}\n", VMSG, user);
	else {
		if (!freopen(VMSG, "r", stdin)) {
			fputs("vacation: no message to send.\n", stderr);
			exit(1);
		}
		execl("/usr/lib/sendmail", "sendmail", "-f", myname, user, NULL);
		fputs("vacation: cannot exec /usr/lib/sendmail\n", stderr);
		exit(1);
	}
}

/*
 * initialize --
 *	initialize the database
 */
static
initialize()
{
	FILE	*fp;
	int	fd;

	if ((fd = open(VDIR, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
		perror("vacation: .vacation.dir");
		exit(1);
	}
	(void)close(fd);
	if ((fd = open(VPAG, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
		perror("vacation: .vacation.page");
		exit(1);
	}
	(void)close(fd);
	dbminit(VACAT);
	if (fp = fopen(VIGN, "r")) {
		char	buf[MAXLINE];

		while (fgets(buf, sizeof(buf), fp)) {
			*(index(buf, '\n')) = EOS;
			setknows(buf, YES);
		}
		(void)fclose(fp);
	}
}

/*
 * usage --
 *	print out a usage message and die
 */
static
usage()
{
	fputs("vacation [-i] username\n", stderr);
	exit(1);
}
