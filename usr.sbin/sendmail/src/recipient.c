/*
 * Copyright (c) 1983, 1995 Eric P. Allman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)recipient.c	8.90 (Berkeley) 5/27/95";
#endif /* not lint */

# include "sendmail.h"

/*
**  SENDTOLIST -- Designate a send list.
**
**	The parameter is a comma-separated list of people to send to.
**	This routine arranges to send to all of them.
**
**	Parameters:
**		list -- the send list.
**		ctladdr -- the address template for the person to
**			send to -- effective uid/gid are important.
**			This is typically the alias that caused this
**			expansion.
**		sendq -- a pointer to the head of a queue to put
**			these people into.
**		aliaslevel -- the current alias nesting depth -- to
**			diagnose loops.
**		e -- the envelope in which to add these recipients.
**
**	Returns:
**		The number of addresses actually on the list.
**
**	Side Effects:
**		none.
*/

#define MAXRCRSN	10	/* maximum levels of alias recursion */

/* q_flags bits inherited from ctladdr */
#define QINHERITEDBITS	(QPINGONSUCCESS|QPINGONFAILURE|QPINGONDELAY|QHASNOTIFY)

int
sendtolist(list, ctladdr, sendq, aliaslevel, e)
	char *list;
	ADDRESS *ctladdr;
	ADDRESS **sendq;
	int aliaslevel;
	register ENVELOPE *e;
{
	register char *p;
	register ADDRESS *al;	/* list of addresses to send to */
	bool firstone;		/* set on first address sent */
	char delimiter;		/* the address delimiter */
	int naddrs;
	int i;
	char *oldto = e->e_to;
	char *bufp;
	char buf[MAXNAME + 1];

	if (list == NULL)
	{
		syserr("sendtolist: null list");
		return 0;
	}

	if (tTd(25, 1))
	{
		printf("sendto: %s\n   ctladdr=", list);
		printaddr(ctladdr, FALSE);
	}

	/* heuristic to determine old versus new style addresses */
	if (ctladdr == NULL &&
	    (strchr(list, ',') != NULL || strchr(list, ';') != NULL ||
	     strchr(list, '<') != NULL || strchr(list, '(') != NULL))
		e->e_flags &= ~EF_OLDSTYLE;
	delimiter = ' ';
	if (!bitset(EF_OLDSTYLE, e->e_flags) || ctladdr != NULL)
		delimiter = ',';

	firstone = TRUE;
	al = NULL;
	naddrs = 0;

	/* make sure we have enough space to copy the string */
	i = strlen(list) + 1;
	if (i <= sizeof buf)
		bufp = buf;
	else
		bufp = xalloc(i);
	strcpy(bufp, denlstring(list, FALSE, TRUE));

	for (p = bufp; *p != '\0'; )
	{
		auto char *delimptr;
		register ADDRESS *a;

		/* parse the address */
		while ((isascii(*p) && isspace(*p)) || *p == ',')
			p++;
		a = parseaddr(p, NULLADDR, RF_COPYALL, delimiter, &delimptr, e);
		p = delimptr;
		if (a == NULL)
			continue;
		a->q_next = al;
		a->q_alias = ctladdr;

		/* arrange to inherit attributes from parent */
		if (ctladdr != NULL)
		{
			ADDRESS *b;
			extern ADDRESS *self_reference();

			/* self reference test */
			if (sameaddr(ctladdr, a))
			{
				if (tTd(27, 5))
				{
					printf("sendtolist: QSELFREF ");
					printaddr(ctladdr, FALSE);
				}
				ctladdr->q_flags |= QSELFREF;
			}

			/* check for address loops */
			b = self_reference(a, e);
			if (b != NULL)
			{
				b->q_flags |= QSELFREF;
				if (tTd(27, 5))
				{
					printf("sendtolist: QSELFREF ");
					printaddr(b, FALSE);
				}
				if (a != b)
				{
					if (tTd(27, 5))
					{
						printf("sendtolist: QDONTSEND ");
						printaddr(a, FALSE);
					}
					a->q_flags |= QDONTSEND;
					continue;
				}
			}

			/* full name */
			if (a->q_fullname == NULL)
				a->q_fullname = ctladdr->q_fullname;

			/* various flag bits */
			a->q_flags &= ~QINHERITEDBITS;
			a->q_flags |= ctladdr->q_flags & QINHERITEDBITS;

			/* original recipient information */
			a->q_orcpt = ctladdr->q_orcpt;
		}

		al = a;
		firstone = FALSE;
	}

	/* arrange to send to everyone on the local send list */
	while (al != NULL)
	{
		register ADDRESS *a = al;

		al = a->q_next;
		a = recipient(a, sendq, aliaslevel, e);
		naddrs++;
	}

	e->e_to = oldto;
	if (bufp != buf)
		free(bufp);
	return (naddrs);
}
/*
**  RECIPIENT -- Designate a message recipient
**
**	Saves the named person for future mailing.
**
**	Parameters:
**		a -- the (preparsed) address header for the recipient.
**		sendq -- a pointer to the head of a queue to put the
**			recipient in.  Duplicate supression is done
**			in this queue.
**		aliaslevel -- the current alias nesting depth.
**		e -- the current envelope.
**
**	Returns:
**		The actual address in the queue.  This will be "a" if
**		the address is not a duplicate, else the original address.
**
**	Side Effects:
**		none.
*/

ADDRESS *
recipient(a, sendq, aliaslevel, e)
	register ADDRESS *a;
	register ADDRESS **sendq;
	int aliaslevel;
	register ENVELOPE *e;
{
	register ADDRESS *q;
	ADDRESS **pq;
	register struct mailer *m;
	register char *p;
	bool quoted = FALSE;		/* set if the addr has a quote bit */
	int findusercount = 0;
	bool initialdontsend = bitset(QDONTSEND, a->q_flags);
	int i;
	char *buf;
	char buf0[MAXNAME + 1];		/* unquoted image of the user name */
	extern int safefile();

	e->e_to = a->q_paddr;
	m = a->q_mailer;
	errno = 0;
	if (aliaslevel == 0)
		a->q_flags |= QPRIMARY;
	if (tTd(26, 1))
	{
		printf("\nrecipient (%d): ", aliaslevel);
		printaddr(a, FALSE);
	}

	/* if this is primary, add it to the original recipient list */
	if (a->q_alias == NULL)
	{
		if (e->e_origrcpt == NULL)
			e->e_origrcpt = a->q_paddr;
		else if (e->e_origrcpt != a->q_paddr)
			e->e_origrcpt = "";
	}

	/* break aliasing loops */
	if (aliaslevel > MAXRCRSN)
	{
		a->q_status = "5.4.6";
		usrerr("554 aliasing/forwarding loop broken (%d aliases deep; %d max",
			aliaslevel, MAXRCRSN);
		return (a);
	}

	/*
	**  Finish setting up address structure.
	*/

	/* get unquoted user for file, program or user.name check */
	i = strlen(a->q_user);
	if (i >= sizeof buf0)
		buf = xalloc(i + 1);
	else
		buf = buf0;
	(void) strcpy(buf, a->q_user);
	for (p = buf; *p != '\0' && !quoted; p++)
	{
		if (*p == '\\')
			quoted = TRUE;
	}
	stripquotes(buf);

	/* check for direct mailing to restricted mailers */
	if (m == ProgMailer)
	{
		if (a->q_alias == NULL)
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 Cannot mail directly to programs");
		}
		else if (bitset(QBOGUSSHELL, a->q_alias->q_flags))
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 User %s@%s doesn't have a valid shell for mailing to programs",
				a->q_alias->q_ruser, MyHostName);
		}
		else if (bitset(QUNSAFEADDR, a->q_alias->q_flags))
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 Address %s is unsafe for mailing to programs",
				a->q_alias->q_paddr);
		}
	}

	/*
	**  Look up this person in the recipient list.
	**	If they are there already, return, otherwise continue.
	**	If the list is empty, just add it.  Notice the cute
	**	hack to make from addresses suppress things correctly:
	**	the QDONTSEND bit will be set in the send list.
	**	[Please note: the emphasis is on "hack."]
	*/

	for (pq = sendq; (q = *pq) != NULL; pq = &q->q_next)
	{
		if (sameaddr(q, a))
		{
			if (tTd(26, 1))
			{
				printf("%s in sendq: ", a->q_paddr);
				printaddr(q, FALSE);
			}
			if (!bitset(QPRIMARY, q->q_flags))
			{
				if (!bitset(QDONTSEND, a->q_flags))
					message("duplicate suppressed");
				q->q_flags |= a->q_flags;
			}
			else if (bitset(QSELFREF, q->q_flags))
				q->q_flags |= a->q_flags & ~QDONTSEND;
			a = q;
			goto done;
		}
	}

	/* add address on list */
	*pq = a;
	a->q_next = NULL;

	/*
	**  Alias the name and handle special mailer types.
	*/

  trylocaluser:
	if (tTd(29, 7))
		printf("at trylocaluser %s\n", a->q_user);

	if (bitset(QDONTSEND|QBADADDR|QVERIFIED, a->q_flags))
		goto testselfdestruct;

	if (m == InclMailer)
	{
		a->q_flags |= QDONTSEND;
		if (a->q_alias == NULL)
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 Cannot mail directly to :include:s");
		}
		else
		{
			int ret;

			message("including file %s", a->q_user);
			ret = include(a->q_user, FALSE, a, sendq, aliaslevel, e);
			if (transienterror(ret))
			{
#ifdef LOG
				if (LogLevel > 2)
					syslog(LOG_ERR, "%s: include %s: transient error: %s",
						e->e_id == NULL ? "NOQUEUE" : e->e_id,
						a->q_user, errstring(ret));
#endif
				a->q_flags |= QQUEUEUP;
				a->q_flags &= ~QDONTSEND;
				usrerr("451 Cannot open %s: %s",
					a->q_user, errstring(ret));
			}
			else if (ret != 0)
			{
				a->q_flags |= QBADADDR;
				a->q_status = "5.2.4";
				usrerr("550 Cannot open %s: %s",
					a->q_user, errstring(ret));
			}
		}
	}
	else if (m == FileMailer)
	{
		extern bool writable();

		/* check if writable or creatable */
		if (a->q_alias == NULL)
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 Cannot mail directly to files");
		}
		else if (bitset(QBOGUSSHELL, a->q_alias->q_flags))
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 User %s@%s doesn't have a valid shell for mailing to files",
				a->q_alias->q_ruser, MyHostName);
		}
		else if (bitset(QUNSAFEADDR, a->q_alias->q_flags))
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.7.1";
			usrerr("550 Address %s is unsafe for mailing to files",
				a->q_alias->q_paddr);
		}
		else if (!writable(buf, getctladdr(a), SFF_CREAT))
		{
			a->q_flags |= QBADADDR;
			giveresponse(EX_CANTCREAT, m, NULL, a->q_alias,
				     (time_t) 0, e);
		}
	}

	/* try aliasing */
	if (!bitset(QDONTSEND, a->q_flags) && bitnset(M_ALIASABLE, m->m_flags))
		alias(a, sendq, aliaslevel, e);

# if USERDB
	/* if not aliased, look it up in the user database */
	if (!bitset(QDONTSEND|QNOTREMOTE|QVERIFIED, a->q_flags) &&
	    bitnset(M_CHECKUDB, m->m_flags))
	{
		extern int udbexpand();

		if (udbexpand(a, sendq, aliaslevel, e) == EX_TEMPFAIL)
		{
			a->q_flags |= QQUEUEUP;
			if (e->e_message == NULL)
				e->e_message = newstr("Deferred: user database error");
# ifdef LOG
			if (LogLevel > 8)
				syslog(LOG_INFO, "%s: deferred: udbexpand: %s",
					e->e_id == NULL ? "NOQUEUE" : e->e_id,
					errstring(errno));
# endif
			message("queued (user database error): %s",
				errstring(errno));
			e->e_nrcpts++;
			goto testselfdestruct;
		}
	}
# endif

	/*
	**  If we have a level two config file, then pass the name through
	**  Ruleset 5 before sending it off.  Ruleset 5 has the right
	**  to send rewrite it to another mailer.  This gives us a hook
	**  after local aliasing has been done.
	*/

	if (tTd(29, 5))
	{
		printf("recipient: testing local?  cl=%d, rr5=%x\n\t",
			ConfigLevel, RewriteRules[5]);
		printaddr(a, FALSE);
	}
	if (!bitset(QNOTREMOTE|QDONTSEND|QQUEUEUP|QVERIFIED, a->q_flags) &&
	    ConfigLevel >= 2 && RewriteRules[5] != NULL &&
	    bitnset(M_TRYRULESET5, m->m_flags))
	{
		maplocaluser(a, sendq, aliaslevel + 1, e);
	}

	/*
	**  If it didn't get rewritten to another mailer, go ahead
	**  and deliver it.
	*/

	if (!bitset(QDONTSEND|QQUEUEUP|QVERIFIED, a->q_flags) &&
	    bitnset(M_HASPWENT, m->m_flags))
	{
		auto bool fuzzy;
		register struct passwd *pw;
		extern struct passwd *finduser();

		/* warning -- finduser may trash buf */
		pw = finduser(buf, &fuzzy);
		if (pw == NULL)
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.1.1";
			giveresponse(EX_NOUSER, m, NULL, a->q_alias,
				     (time_t) 0, e);
		}
		else
		{
			char nbuf[MAXNAME + 1];

			if (fuzzy)
			{
				/* name was a fuzzy match */
				a->q_user = newstr(pw->pw_name);
				if (findusercount++ > 3)
				{
					a->q_flags |= QBADADDR;
					a->q_status = "5.4.6";
					usrerr("554 aliasing/forwarding loop for %s broken",
						pw->pw_name);
					goto done;
				}

				/* see if it aliases */
				(void) strcpy(buf, pw->pw_name);
				goto trylocaluser;
			}
			if (strcmp(pw->pw_dir, "/") == 0)
				a->q_home = "";
			else
				a->q_home = newstr(pw->pw_dir);
			a->q_uid = pw->pw_uid;
			a->q_gid = pw->pw_gid;
			a->q_ruser = newstr(pw->pw_name);
			a->q_flags |= QGOODUID;
			buildfname(pw->pw_gecos, pw->pw_name, nbuf);
			if (nbuf[0] != '\0')
				a->q_fullname = newstr(nbuf);
			if (pw->pw_shell != NULL && pw->pw_shell[0] != '\0' &&
			    !usershellok(pw->pw_shell))
			{
				a->q_flags |= QBOGUSSHELL;
			}
			if (bitset(EF_VRFYONLY, e->e_flags))
			{
				/* don't do any more now */
				a->q_flags |= QVERIFIED;
			}
			else if (!quoted)
				forward(a, sendq, aliaslevel, e);
		}
	}
	if (!bitset(QDONTSEND, a->q_flags))
		e->e_nrcpts++;

  testselfdestruct:
	a->q_flags |= QTHISPASS;
	if (tTd(26, 8))
	{
		printf("testselfdestruct: ");
		printaddr(a, FALSE);
		if (tTd(26, 10))
		{
			printf("SENDQ:\n");
			printaddr(*sendq, TRUE);
			printf("----\n");
		}
	}
	if (a->q_alias == NULL && a != &e->e_from &&
	    bitset(QDONTSEND, a->q_flags))
	{
		for (q = *sendq; q != NULL; q = q->q_next)
		{
			if (!bitset(QDONTSEND|QBADADDR, q->q_flags) &&
			    bitset(QTHISPASS, q->q_flags))
				break;
		}
		if (q == NULL)
		{
			a->q_flags |= QBADADDR;
			a->q_status = "5.4.6";
			usrerr("554 aliasing/forwarding loop broken");
		}
	}

  done:
	a->q_flags |= QTHISPASS;
	if (buf != buf0)
		free(buf);

	/*
	**  If we are at the top level, check to see if this has
	**  expanded to exactly one address.  If so, it can inherit
	**  the primaryness of the address.
	**
	**  While we're at it, clear the QTHISPASS bits.
	*/

	if (aliaslevel == 0)
	{
		int nrcpts = 0;
		ADDRESS *only;

		for (q = *sendq; q != NULL; q = q->q_next)
		{
			if (bitset(QTHISPASS, q->q_flags) &&
			    !bitset(QDONTSEND|QBADADDR, q->q_flags))
			{
				nrcpts++;
				only = q;
			}
			q->q_flags &= ~QTHISPASS;
		}
		if (nrcpts == 1)
		{
			/* check to see if this actually got a new owner */
			q = only;
			while ((q = q->q_alias) != NULL)
			{
				if (q->q_owner != NULL)
					break;
			}
			if (q == NULL)
				only->q_flags |= QPRIMARY;
		}
		else if (!initialdontsend && nrcpts > 0)
		{
			/* arrange for return receipt */
			e->e_flags |= EF_SENDRECEIPT;
			a->q_flags |= QEXPANDED;
			if (e->e_xfp != NULL)
				fprintf(e->e_xfp,
					"%s... expanded to multiple addresses\n",
					a->q_paddr);
		}
	}

	return (a);
}
/*
**  FINDUSER -- find the password entry for a user.
**
**	This looks a lot like getpwnam, except that it may want to
**	do some fancier pattern matching in /etc/passwd.
**
**	This routine contains most of the time of many sendmail runs.
**	It deserves to be optimized.
**
**	Parameters:
**		name -- the name to match against.
**		fuzzyp -- an outarg that is set to TRUE if this entry
**			was found using the fuzzy matching algorithm;
**			set to FALSE otherwise.
**
**	Returns:
**		A pointer to a pw struct.
**		NULL if name is unknown or ambiguous.
**
**	Side Effects:
**		may modify name.
*/

struct passwd *
finduser(name, fuzzyp)
	char *name;
	bool *fuzzyp;
{
	register struct passwd *pw;
	register char *p;

	if (tTd(29, 4))
		printf("finduser(%s): ", name);

	*fuzzyp = FALSE;

#ifdef HESIOD
	/* DEC Hesiod getpwnam accepts numeric strings -- short circuit it */
	for (p = name; *p != '\0'; p++)
		if (!isascii(*p) || !isdigit(*p))
			break;
	if (*p == '\0')
	{
		if (tTd(29, 4))
			printf("failed (numeric input)\n");
		return NULL;
	}
#endif

	/* look up this login name using fast path */
	if ((pw = sm_getpwnam(name)) != NULL)
	{
		if (tTd(29, 4))
			printf("found (non-fuzzy)\n");
		return (pw);
	}

#ifdef MATCHGECOS
	/* see if fuzzy matching allowed */
	if (!MatchGecos)
	{
		if (tTd(29, 4))
			printf("not found (fuzzy disabled)\n");
		return NULL;
	}

	/* search for a matching full name instead */
	for (p = name; *p != '\0'; p++)
	{
		if (*p == (SpaceSub & 0177) || *p == '_')
			*p = ' ';
	}
	(void) setpwent();
	while ((pw = getpwent()) != NULL)
	{
		char buf[MAXNAME + 1];

		buildfname(pw->pw_gecos, pw->pw_name, buf);
		if (strchr(buf, ' ') != NULL && !strcasecmp(buf, name))
		{
			if (tTd(29, 4))
				printf("fuzzy matches %s\n", pw->pw_name);
			message("sending to login name %s", pw->pw_name);
			*fuzzyp = TRUE;
			return (pw);
		}
	}
	if (tTd(29, 4))
		printf("no fuzzy match found\n");
#else
	if (tTd(29, 4))
		printf("not found (fuzzy disabled)\n");
#endif
	return (NULL);
}
/*
**  WRITABLE -- predicate returning if the file is writable.
**
**	This routine must duplicate the algorithm in sys/fio.c.
**	Unfortunately, we cannot use the access call since we
**	won't necessarily be the real uid when we try to
**	actually open the file.
**
**	Notice that ANY file with ANY execute bit is automatically
**	not writable.  This is also enforced by mailfile.
**
**	Parameters:
**		filename -- the file name to check.
**		ctladdr -- the controlling address for this file.
**		flags -- SFF_* flags to control the function.
**
**	Returns:
**		TRUE -- if we will be able to write this file.
**		FALSE -- if we cannot write this file.
**
**	Side Effects:
**		none.
*/

bool
writable(filename, ctladdr, flags)
	char *filename;
	ADDRESS *ctladdr;
	int flags;
{
	uid_t euid;
	gid_t egid;
	int bits;
	register char *p;
	char *uname;

	if (tTd(29, 5))
		printf("writable(%s, 0x%x)\n", filename, flags);

#ifdef SUID_ROOT_FILES_OK
	/* really ought to be passed down -- and not a good idea */
	flags |= SFF_ROOTOK;
#endif

	/*
	**  File does exist -- check that it is writable.
	*/

	if (ctladdr != NULL && geteuid() == 0)
	{
		euid = ctladdr->q_uid;
		egid = ctladdr->q_gid;
		uname = ctladdr->q_user;
	}
	else if (bitset(SFF_RUNASREALUID, flags))
	{
		extern char RealUserName[];

		euid = RealUid;
		egid = RealGid;
		uname = RealUserName;
	}
	else if (FileMailer != NULL)
	{
		euid = FileMailer->m_uid;
		egid = FileMailer->m_gid;
	}
	else
	{
		euid = egid = 0;
	}
	if (euid == 0)
	{
		euid = DefUid;
		uname = DefUser;
	}
	if (egid == 0)
		egid = DefGid;
	if (geteuid() == 0)
		flags |= SFF_SETUIDOK;

	errno = safefile(filename, euid, egid, uname, flags, S_IWRITE, NULL);
	return errno == 0;
}
/*
**  INCLUDE -- handle :include: specification.
**
**	Parameters:
**		fname -- filename to include.
**		forwarding -- if TRUE, we are reading a .forward file.
**			if FALSE, it's a :include: file.
**		ctladdr -- address template to use to fill in these
**			addresses -- effective user/group id are
**			the important things.
**		sendq -- a pointer to the head of the send queue
**			to put these addresses in.
**		aliaslevel -- the alias nesting depth.
**		e -- the current envelope.
**
**	Returns:
**		open error status
**
**	Side Effects:
**		reads the :include: file and sends to everyone
**		listed in that file.
**
**	Security Note:
**		If you have restricted chown (that is, you can't
**		give a file away), it is reasonable to allow programs
**		and files called from this :include: file to be to be
**		run as the owner of the :include: file.  This is bogus
**		if there is any chance of someone giving away a file.
**		We assume that pre-POSIX systems can give away files.
**
**		There is an additional restriction that if you
**		forward to a :include: file, it will not take on
**		the ownership of the :include: file.  This may not
**		be necessary, but shouldn't hurt.
*/

static jmp_buf	CtxIncludeTimeout;
static void	includetimeout();

int
include(fname, forwarding, ctladdr, sendq, aliaslevel, e)
	char *fname;
	bool forwarding;
	ADDRESS *ctladdr;
	ADDRESS **sendq;
	int aliaslevel;
	ENVELOPE *e;
{
	FILE *fp = NULL;
	char *oldto = e->e_to;
	char *oldfilename = FileName;
	int oldlinenumber = LineNumber;
	register EVENT *ev = NULL;
	int nincludes;
	register ADDRESS *ca;
	uid_t saveduid, uid;
	gid_t savedgid, gid;
	char *uname;
	int rval = 0;
	int sfflags = SFF_REGONLY;
	struct stat st;
	char buf[MAXLINE];
#ifdef _POSIX_CHOWN_RESTRICTED
# if _POSIX_CHOWN_RESTRICTED == -1
#  define safechown	FALSE
# else
#  define safechown	TRUE
# endif
#else
# ifdef _PC_CHOWN_RESTRICTED
	bool safechown;
# else
#  ifdef BSD
#   define safechown	TRUE
#  else
#   define safechown	FALSE
#  endif
# endif
#endif
	extern bool chownsafe();

	if (tTd(27, 2))
		printf("include(%s)\n", fname);
	if (tTd(27, 4))
		printf("   ruid=%d euid=%d\n", getuid(), geteuid());
	if (tTd(27, 14))
	{
		printf("ctladdr ");
		printaddr(ctladdr, FALSE);
	}

	if (tTd(27, 9))
		printf("include: old uid = %d/%d\n", getuid(), geteuid());

	if (forwarding)
		sfflags |= SFF_MUSTOWN|SFF_ROOTOK|SFF_NOSLINK;

	ca = getctladdr(ctladdr);
	if (ca == NULL)
	{
		uid = DefUid;
		gid = DefGid;
		uname = DefUser;
	}
	else
	{
		uid = ca->q_uid;
		gid = ca->q_gid;
		uname = ca->q_user;
	}
#if HASSETREUID || USESETEUID
	saveduid = geteuid();
	savedgid = getegid();
	if (saveduid == 0)
	{
		initgroups(uname, gid);
		if (uid != 0)
		{
# if USESETEUID
			if (seteuid(uid) < 0)
				syserr("seteuid(%d) failure (real=%d, eff=%d)",
					uid, getuid(), geteuid());
# else
			if (setreuid(0, uid) < 0)
				syserr("setreuid(0, %d) failure (real=%d, eff=%d)",
					uid, getuid(), geteuid());
# endif
			else
				sfflags |= SFF_NOPATHCHECK;
		}
	}
#endif

	if (tTd(27, 9))
		printf("include: new uid = %d/%d\n", getuid(), geteuid());

	/*
	**  If home directory is remote mounted but server is down,
	**  this can hang or give errors; use a timeout to avoid this
	*/

	if (setjmp(CtxIncludeTimeout) != 0)
	{
		ctladdr->q_flags |= QQUEUEUP;
		errno = 0;

		/* return pseudo-error code */
		rval = EOPENTIMEOUT;
		goto resetuid;
	}
	if (TimeOuts.to_fileopen > 0)
		ev = setevent(TimeOuts.to_fileopen, includetimeout, 0);
	else
		ev = NULL;

	/* the input file must be marked safe */
	rval = safefile(fname, uid, gid, uname, sfflags, S_IREAD, NULL);
	if (rval != 0)
	{
		/* don't use this :include: file */
		if (tTd(27, 4))
			printf("include: not safe (uid=%d): %s\n",
				uid, errstring(rval));
	}
	else
	{
		fp = fopen(fname, "r");
		if (fp == NULL)
		{
			rval = errno;
			if (tTd(27, 4))
				printf("include: open: %s\n", errstring(rval));
		}
	}
	if (ev != NULL)
		clrevent(ev);

resetuid:

#if HASSETREUID || USESETEUID
	if (saveduid == 0)
	{
		if (uid != 0)
		{
# if USESETEUID
			if (seteuid(0) < 0)
				syserr("seteuid(0) failure (real=%d, eff=%d)",
					getuid(), geteuid());
# else
			if (setreuid(-1, 0) < 0)
				syserr("setreuid(-1, 0) failure (real=%d, eff=%d)",
					getuid(), geteuid());
			if (setreuid(RealUid, 0) < 0)
				syserr("setreuid(%d, 0) failure (real=%d, eff=%d)",
					RealUid, getuid(), geteuid());
# endif
		}
		setgid(savedgid);
	}
#endif

	if (tTd(27, 9))
		printf("include: reset uid = %d/%d\n", getuid(), geteuid());

	if (rval == EOPENTIMEOUT)
		usrerr("451 open timeout on %s", fname);

	if (fp == NULL)
		return rval;

	if (fstat(fileno(fp), &st) < 0)
	{
		rval = errno;
		syserr("Cannot fstat %s!", fname);
		return rval;
	}

#ifndef safechown
	safechown = chownsafe(fileno(fp));
#endif
	if (ca == NULL && safechown)
	{
		ctladdr->q_uid = st.st_uid;
		ctladdr->q_gid = st.st_gid;
		ctladdr->q_flags |= QGOODUID;
	}
	if (ca != NULL && ca->q_uid == st.st_uid)
	{
		/* optimization -- avoid getpwuid if we already have info */
		ctladdr->q_flags |= ca->q_flags & QBOGUSSHELL;
		ctladdr->q_ruser = ca->q_ruser;
	}
	else
	{
		register struct passwd *pw;

		pw = sm_getpwuid(st.st_uid);
		if (pw == NULL)
			ctladdr->q_flags |= QBOGUSSHELL;
		else
		{
			char *sh;

			ctladdr->q_ruser = newstr(pw->pw_name);
			if (safechown)
				sh = pw->pw_shell;
			else
				sh = "/SENDMAIL/ANY/SHELL/";
			if (!usershellok(sh))
			{
				if (safechown)
					ctladdr->q_flags |= QBOGUSSHELL;
				else
					ctladdr->q_flags |= QUNSAFEADDR;
			}
		}
	}

	if (bitset(EF_VRFYONLY, e->e_flags))
	{
		/* don't do any more now */
		ctladdr->q_flags |= QVERIFIED;
		e->e_nrcpts++;
		xfclose(fp, "include", fname);
		return rval;
	}

	/*
	** Check to see if some bad guy can write this file
	**
	**	This should really do something clever with group
	**	permissions; currently we just view world writable
	**	as unsafe.  Also, we don't check for writable
	**	directories in the path.  We've got to leave
	**	something for the local sysad to do.
	*/

	if (bitset(S_IWOTH, st.st_mode))
		ctladdr->q_flags |= QUNSAFEADDR;

	/* read the file -- each line is a comma-separated list. */
	FileName = fname;
	LineNumber = 0;
	ctladdr->q_flags &= ~QSELFREF;
	nincludes = 0;
	while (fgets(buf, sizeof buf, fp) != NULL)
	{
		register char *p = strchr(buf, '\n');

		LineNumber++;
		if (p != NULL)
			*p = '\0';
		if (buf[0] == '#' || buf[0] == '\0')
			continue;

		/* <sp>#@# introduces a comment anywhere */
		/* for Japanese character sets */
		for (p = buf; (p = strchr(++p, '#')) != NULL; )
		{
			if (p[1] == '@' && p[2] == '#' &&
			    isascii(p[-1]) && isspace(p[-1]) &&
			    (p[3] == '\0' || (isascii(p[3]) && isspace(p[3]))))
			{
				p[-1] = '\0';
				break;
			}
		}
		if (buf[0] == '\0')
			continue;

		e->e_to = NULL;
		message("%s to %s",
			forwarding ? "forwarding" : "sending", buf);
#ifdef LOG
		if (forwarding && LogLevel > 9)
			syslog(LOG_INFO, "%s: forward %s => %s",
				e->e_id == NULL ? "NOQUEUE" : e->e_id,
				oldto, buf);
#endif

		nincludes += sendtolist(buf, ctladdr, sendq, aliaslevel + 1, e);
	}

	if (ferror(fp) && tTd(27, 3))
		printf("include: read error: %s\n", errstring(errno));
	if (nincludes > 0 && !bitset(QSELFREF, ctladdr->q_flags))
	{
		if (tTd(27, 5))
		{
			printf("include: QDONTSEND ");
			printaddr(ctladdr, FALSE);
		}
		ctladdr->q_flags |= QDONTSEND;
	}

	(void) xfclose(fp, "include", fname);
	FileName = oldfilename;
	LineNumber = oldlinenumber;
	e->e_to = oldto;
	return rval;
}

static void
includetimeout()
{
	longjmp(CtxIncludeTimeout, 1);
}
/*
**  SENDTOARGV -- send to an argument vector.
**
**	Parameters:
**		argv -- argument vector to send to.
**		e -- the current envelope.
**
**	Returns:
**		none.
**
**	Side Effects:
**		puts all addresses on the argument vector onto the
**			send queue.
*/

sendtoargv(argv, e)
	register char **argv;
	register ENVELOPE *e;
{
	register char *p;

	while ((p = *argv++) != NULL)
	{
		(void) sendtolist(p, NULLADDR, &e->e_sendqueue, 0, e);
	}
}
/*
**  GETCTLADDR -- get controlling address from an address header.
**
**	If none, get one corresponding to the effective userid.
**
**	Parameters:
**		a -- the address to find the controller of.
**
**	Returns:
**		the controlling address.
**
**	Side Effects:
**		none.
*/

ADDRESS *
getctladdr(a)
	register ADDRESS *a;
{
	while (a != NULL && !bitset(QGOODUID, a->q_flags))
		a = a->q_alias;
	return (a);
}
/*
**  SELF_REFERENCE -- check to see if an address references itself
**
**	The check is done through a chain of aliases.  If it is part of
**	a loop, break the loop at the "best" address, that is, the one
**	that exists as a real user.
**
**	This is to handle the case of:
**		awc:		Andrew.Chang
**		Andrew.Chang:	awc@mail.server
**	which is a problem only on mail.server.
**
**	Parameters:
**		a -- the address to check.
**		e -- the current envelope.
**
**	Returns:
**		The address that should be retained.
*/

ADDRESS *
self_reference(a, e)
	ADDRESS *a;
	ENVELOPE *e;
{
	ADDRESS *b;		/* top entry in self ref loop */
	ADDRESS *c;		/* entry that point to a real mail box */

	if (tTd(27, 1))
		printf("self_reference(%s)\n", a->q_paddr);

	for (b = a->q_alias; b != NULL; b = b->q_alias)
	{
		if (sameaddr(a, b))
			break;
	}

	if (b == NULL)
	{
		if (tTd(27, 1))
			printf("\t... no self ref\n");
		return NULL;
	}

	/*
	**  Pick the first address that resolved to a real mail box
	**  i.e has a pw entry.  The returned value will be marked
	**  QSELFREF in recipient(), which in turn will disable alias()
	**  from marking it QDONTSEND, which mean it will be used
	**  as a deliverable address.
	**
	**  The 2 key thing to note here are:
	**	1) we are in a recursive call sequence:
	**		alias->sentolist->recipient->alias
	**	2) normally, when we return back to alias(), the address
	**	   will be marked QDONTSEND, since alias() assumes the
	**	   expanded form will be used instead of the current address.
	**	   This behaviour is turned off if the address is marked
	**	   QSELFREF We set QSELFREF when we return to recipient().
	*/

	c = a;
	while (c != NULL)
	{
		if (bitnset(M_HASPWENT, c->q_mailer->m_flags))
		{
			if (tTd(27, 2))
				printf("\t... getpwnam(%s)... ", c->q_user);
			if (sm_getpwnam(c->q_user) != NULL)
			{
				if (tTd(27, 2))
					printf("found\n");

				/* ought to cache results here */
				if (sameaddr(b, c))
					return b;
				else
					return c;
			}
			if (tTd(27, 2))
				printf("failed\n");
		}
		c = c->q_alias;
	}

	if (tTd(27, 1))
		printf("\t... cannot break loop for \"%s\"\n", a->q_paddr);

	return NULL;
}
