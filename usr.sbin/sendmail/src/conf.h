/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)conf.h	5.27 (Berkeley) 9/23/92
 */

/*
**  CONF.H -- All user-configurable parameters for sendmail
*/

/*
**  Table sizes, etc....
**	There shouldn't be much need to change these....
*/

# define MAXLINE	1024		/* max line length */
# define MAXNAME	256		/* max length of a name */
# define MAXFIELD	4096		/* max total length of a hdr field */
# define MAXPV		40		/* max # of parms to mailers */
# define MAXHOP		17		/* max value of HopCount */
# define MAXATOM	100		/* max atoms per address */
# define MAXMAILERS	25		/* maximum mailers known to system */
# define MAXRWSETS	30		/* max # of sets of rewriting rules */
# define MAXPRIORITIES	25		/* max values for Precedence: field */
# define MAXTRUST	30		/* maximum number of trusted users */
# define MAXUSERENVIRON	40		/* max # of items in user environ */
# define QUEUESIZE	600		/* max # of jobs per queue run */
# define MAXMXHOSTS	10		/* max # of MX records */
# define SMTPLINELIM	990		/* maximum SMTP line length */
# ifndef FORK
# define FORK		vfork		/* function to call to fork mailer */
# endif

/*
**  Compilation options.
**
**	#define these if they are available; comment them out otherwise.
*/

# ifdef hpux
# define SYSTEM5	1
# endif

# define LOG		1	/* enable logging */
# define SMTP		1	/* enable user and server SMTP */
# define QUEUE		1	/* enable queueing */
# define UGLYUUCP	1	/* output ugly UUCP From lines */
# define DAEMON		1	/* include the daemon (requires IPC & SMTP) */
# define SETPROCTITLE	1	/* munge argv to display current status */
# define NAMED_BIND	1	/* use Berkeley Internet Domain Server */
# define MATCHGECOS	1	/* match user names from gecos field */

# ifdef NEWDB
# define USERDB		1	/* look in user database (requires NEWDB) */
# endif

# ifdef SYSTEM5

# define LOCKF		1	/* use System V lockf instead of flock */
# define SYS5TZ		1	/* use System V style timezones */

# define index		strchr
# define rindex		strrchr

# endif

/*
**  Older systems don't have this error code -- it should be in
**  /usr/include/sysexits.h.
*/

# ifndef EX_CONFIG
# define EX_CONFIG	78	/* configuration error */
# endif
