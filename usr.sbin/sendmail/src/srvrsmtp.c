# include "sendmail.h"

# ifndef SMTP
SCCSID(@(#)srvrsmtp.c	3.20.1.1		7/4/82	(no SMTP));
# else SMTP

SCCSID(@(#)srvrsmtp.c	3.20.1.1		7/4/82);

/*
**  SMTP -- run the SMTP protocol.
**
**	Parameters:
**		none.
**
**	Returns:
**		never.
**
**	Side Effects:
**		Reads commands from the input channel and processes
**			them.
*/

struct cmd
{
	char	*cmdname;	/* command name */
	int	cmdcode;	/* internal code, see below */
};

/* values for cmdcode */
# define CMDERROR	0	/* bad command */
# define CMDMAIL	1	/* mail -- designate sender */
# define CMDRCPT	2	/* rcpt -- designate recipient */
# define CMDDATA	3	/* data -- send message text */
# define CMDHOPS	4	/* hops -- specify hop count */
# define CMDRSET	5	/* rset -- reset state */
# define CMDVRFY	6	/* vrfy -- verify address */
# define CMDHELP	7	/* help -- give usage info */
# define CMDNOOP	8	/* noop -- do nothing */
# define CMDQUIT	9	/* quit -- close connection and die */
# define CMDMRSQ	10	/* mrsq -- for old mtp compat only */
# define CMDHELO	11	/* helo -- be polite */
# define CMDDBGSHOWQ	12	/* _showq -- show send queue (DEBUG) */
# define CMDDBGDEBUG	13	/* _debug -- set debug mode */
# define CMDDBGVERBOSE	14	/* _verbose -- go into verbose mode */
# define CMDDBGKILL	15	/* _kill -- kill sendmail */

static struct cmd	CmdTab[] =
{
	"mail",		CMDMAIL,
	"rcpt",		CMDRCPT,
	"mrcp",		CMDRCPT,	/* for old MTP compatability */
	"data",		CMDDATA,
	"rset",		CMDRSET,
	"vrfy",		CMDVRFY,
	"help",		CMDHELP,
	"noop",		CMDNOOP,
	"quit",		CMDQUIT,
	"mrsq",		CMDMRSQ,
	"helo",		CMDHELO,
	"hops",		CMDHOPS,
# ifdef DEBUG
	"_showq",	CMDDBGSHOWQ,
	"_debug",	CMDDBGDEBUG,
	"_verbose",	CMDDBGVERBOSE,
	"_kill",	CMDDBGKILL,
# endif DEBUG
	NULL,		CMDERROR,
};

smtp()
{
	char inp[MAXLINE];
	register char *p;
	struct cmd *c;
	char *cmd;
	extern char *skipword();
	extern bool sameword();
	bool hasmail;			/* mail command received */
	int rcps;			/* number of recipients */
	auto ADDRESS *vrfyqueue;
	extern char Version[];

	hasmail = FALSE;
	rcps = 0;
	(void) close(1);
	(void) dup(fileno(OutChannel));
	message("220", "%s Sendmail version %s at your service", HostName, Version);
	for (;;)
	{
		CurEnv->e_to = NULL;
		Errors = 0;
		(void) fflush(stdout);
		if (fgets(inp, sizeof inp, InChannel) == NULL)
		{
			/* end of file, just die */
			message("421", "%s Lost input channel", HostName);
			finis();
		}

		/* clean up end of line */
		fixcrlf(inp, TRUE);

		/* echo command to transcript */
		fprintf(Xscript, "*** %s\n", inp);

		/* break off command */
		for (p = inp; isspace(*p); p++)
			continue;
		cmd = p;
		while (*++p != '\0' && !isspace(*p))
			continue;
		if (*p != '\0')
			*p++ = '\0';

		/* decode command */
		for (c = CmdTab; c->cmdname != NULL; c++)
		{
			if (sameword(c->cmdname, cmd))
				break;
		}

		/* process command */
		switch (c->cmdcode)
		{
		  case CMDHELO:		/* hello -- introduce yourself */
			define('s', newstr(p));
			message("250", "%s Hello %s, pleased to meet you",
				HostName, p);
			break;

		  case CMDMAIL:		/* mail -- designate sender */
			if (hasmail)
			{
				message("503", "Sender already specified");
				break;
			}
			p = skipword(p, "from");
			if (p == NULL)
				break;
			if (index(p, ',') != NULL)
			{
				message("501", "Source routing not implemented");
				Errors++;
				break;
			}
			setsender(p);
			if (Errors == 0)
			{
				message("250", "Sender ok");
				hasmail = TRUE;
			}
			break;

		  case CMDRCPT:		/* rcpt -- designate recipient */
			p = skipword(p, "to");
			if (p == NULL)
				break;
			if (index(p, ',') != NULL)
			{
				message("501", "Source routing not implemented");
				Errors++;
				break;
			}
			sendto(p, 1, (ADDRESS *) NULL, &CurEnv->e_sendqueue);
			if (Errors == 0)
			{
				message("250", "%s... Recipient ok", p);
				rcps++;
			}
			break;

		  case CMDDATA:		/* data -- text of mail */
			if (!hasmail)
			{
				message("503", "Need MAIL command");
				break;
			}
			else if (rcps <= 0)
			{
				message("503", "Need RCPT (recipient)");
				break;
			}

			/* collect the text of the message */
			collect(TRUE);
			if (Errors != 0)
				break;

			/* if sending to multiple people, mail back errors */
			if (rcps != 1)
				HoldErrs = MailBack = TRUE;

			/* send to all recipients */
			sendall(CurEnv, FALSE);

			/* reset strange modes */
			HoldErrs = FALSE;
			CurEnv->e_to = NULL;

			/* issue success if appropriate */
			if (Errors == 0 || rcps != 1)
				message("250", "Sent");
			break;

		  case CMDRSET:		/* rset -- reset state */
			message("250", "Reset state");
			finis();

		  case CMDVRFY:		/* vrfy -- verify address */
			vrfyqueue = NULL;
			sendto(p, 1, (ADDRESS *) NULL, &vrfyqueue);
			while (vrfyqueue != NULL)
			{
				register ADDRESS *a = vrfyqueue->q_next;
				char *code;

				while (a != NULL && bitset(QDONTSEND, a->q_flags))
					a = a->q_next;

				if (!bitset(QDONTSEND, vrfyqueue->q_flags))
				{
					if (a != NULL)
						code = "250-";
					else
						code = "250";
					if (vrfyqueue->q_fullname == NULL)
						message(code, "<%s>", vrfyqueue->q_paddr);
					else
						message(code, "%s <%s>",
						    vrfyqueue->q_fullname, vrfyqueue->q_paddr);
				}
				else if (a == NULL)
					message("554", "Self destructive alias loop");
				vrfyqueue = a;
			}
			break;

		  case CMDHELP:		/* help -- give user info */
			if (*p == '\0')
				p = "SMTP";
			help(p);
			break;

		  case CMDNOOP:		/* noop -- do nothing */
			message("200", "OK");
			break;

		  case CMDQUIT:		/* quit -- leave mail */
			message("221", "%s closing connection", HostName);
			finis();

		  case CMDMRSQ:		/* mrsq -- negotiate protocol */
			if (*p == 'R' || *p == 'T')
			{
				/* recipients first or text first */
				message("200", "%c ok, please continue", *p);
			}
			else if (*p == '?')
			{
				/* what do I prefer?  anything, anytime */
				message("215", "R Recipients first is my choice");
			}
			else if (*p == '\0')
			{
				/* no meaningful scheme */
				message("200", "okey dokie boobie");
			}
			else
			{
				/* bad argument */
				message("504", "Scheme unknown");
			}
			break;

		  case CMDHOPS:		/* specify hop count */
			HopCount = atoi(p);
			if (++HopCount > MAXHOP)
				message("501", "Hop count exceeded");
			else
				message("200", "Hop count ok");
			break;

# ifdef DEBUG
		  case CMDDBGSHOWQ:	/* show queues */
			printf("Send Queue=");
			printaddr(CurEnv->e_sendqueue, TRUE);
			break;

		  case CMDDBGDEBUG:	/* set debug mode */
			Debug = atoi(p);
			message("200", "Debug = %d", Debug);
			break;

		  case CMDDBGVERBOSE:	/* set verbose mode */
			Verbose = TRUE;
			message("200", "Verbose mode");
			break;

		  case CMDDBGKILL:	/* kill the parent */
			if (kill(MotherPid, SIGTERM) >= 0)
				message("200", "Mother is dead");
			else
				message("500", "Can't kill Mom");
			break;
# endif DEBUG

		  case CMDERROR:	/* unknown command */
			message("500", "Command unrecognized");
			break;

		  default:
			syserr("smtp: unknown code %d", c->cmdcode);
			break;
		}
	}
}
/*
**  SKIPWORD -- skip a fixed word.
**
**	Parameters:
**		p -- place to start looking.
**		w -- word to skip.
**
**	Returns:
**		p following w.
**		NULL on error.
**
**	Side Effects:
**		clobbers the p data area.
*/

static char *
skipword(p, w)
	register char *p;
	char *w;
{
	register char *q;
	extern bool sameword();

	/* find beginning of word */
	while (isspace(*p))
		p++;
	q = p;

	/* find end of word */
	while (*p != '\0' && *p != ':' && !isspace(*p))
		p++;
	while (isspace(*p))
		*p++ = '\0';
	if (*p != ':')
	{
	  syntax:
		message("501", "Syntax error");
		Errors++;
		return (NULL);
	}
	*p++ = '\0';
	while (isspace(*p))
		p++;

	/* see if the input word matches desired word */
	if (!sameword(q, w))
		goto syntax;

	return (p);
}
/*
**  HELP -- implement the HELP command.
**
**	Parameters:
**		topic -- the topic we want help for.
**
**	Returns:
**		none.
**
**	Side Effects:
**		outputs the help file to message output.
*/

help(topic)
	char *topic;
{
	register FILE *hf;
	int len;
	char buf[MAXLINE];
	bool noinfo;
	extern char *HelpFile;

	hf = fopen(HelpFile, "r");
	if (hf == NULL)
	{
		/* no help */
		message("502", "HELP not implemented");
		return;
	}

	len = strlen(topic);
	makelower(topic);
	noinfo = TRUE;

	while (fgets(buf, sizeof buf, hf) != NULL)
	{
		if (strncmp(buf, topic, len) == 0)
		{
			register char *p;

			p = index(buf, '\t');
			if (p == NULL)
				p = buf;
			else
				p++;
			fixcrlf(p, TRUE);
			message("214-", p);
			noinfo = FALSE;
		}
	}

	if (noinfo)
		message("504", "HELP topic unknown");
	else
		message("214", "End of HELP info");
	(void) fclose(hf);
}

# endif SMTP
