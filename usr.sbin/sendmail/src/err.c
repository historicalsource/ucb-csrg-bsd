# include "sendmail.h"
# ifdef LOG
# include <syslog.h>
# endif LOG

SCCSID(@(#)err.c	3.24		7/25/82);

/*
**  SYSERR -- Print error message.
**
**	Prints an error message via printf to the diagnostic
**	output.  If LOG is defined, it logs it also.
**
**	Parameters:
**		f -- the format string
**		a, b, c, d, e -- parameters
**
**	Returns:
**		none
**
**	Side Effects:
**		increments Errors.
**		sets ExitStat.
*/

# ifdef lint
int	sys_nerr;
char	*sys_errlist[];
# endif lint
static char	MsgBuf[BUFSIZ*2];	/* text of most recent message */

/*VARARGS1*/
syserr(fmt, a, b, c, d, e)
	char *fmt;
{
	extern char Arpa_Syserr[];
	char *saveto = CurEnv->e_to;

	/* format and output the error message */
	CurEnv->e_to = NULL;
	message(Arpa_Syserr, fmt, a, b, c, d, e);
	CurEnv->e_to = saveto;

	/* mark the error as having occured */
	Errors++;
	FatalErrors = TRUE;

	/* determine exit status if not already set */
	if (ExitStat == EX_OK)
	{
		if (errno == 0)
			ExitStat = EX_SOFTWARE;
		else
			ExitStat = EX_OSERR;
	}

# ifdef LOG
	syslog(LOG_ERR, "%s->%s: %s", CurEnv->e_from.q_paddr, CurEnv->e_to, &MsgBuf[4]);
# endif LOG
	errno = 0;
}
/*
**  USRERR -- Signal user error.
**
**	This is much like syserr except it is for user errors.
**
**	Parameters:
**		fmt, a, b, c, d -- printf strings
**
**	Returns:
**		none
**
**	Side Effects:
**		increments Errors.
*/

/*VARARGS1*/
usrerr(fmt, a, b, c, d, e)
	char *fmt;
{
	extern char SuprErrs;
	extern char Arpa_Usrerr[];

	if (SuprErrs)
		return;
	Errors++;
	FatalErrors = TRUE;

	message(Arpa_Usrerr, fmt, a, b, c, d, e);
}
/*
**  MESSAGE -- print message (not necessarily an error)
**
**	Parameters:
**		num -- the default ARPANET error number (in ascii)
**		msg -- the message (printf fmt) -- if it begins
**			with a digit, this number overrides num.
**		a, b, c, d, e -- printf arguments
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
*/

/*VARARGS2*/
message(num, msg, a, b, c, d, e)
	register char *num;
	register char *msg;
{
	errno = 0;
	fmtmsg(MsgBuf, CurEnv->e_to, num, msg, a, b, c, d, e);

	/* output to transcript */
	fprintf(Xscript, "%s\n", Smtp ? MsgBuf : &MsgBuf[4]);

	/* output to channel if appropriate */
	if (!HoldErrs && (Verbose || MsgBuf[0] != '0'))
	{
		(void) fflush(stdout);
		if (ArpaMode)
			fprintf(OutChannel, "%s\r\n", MsgBuf);
		else
			fprintf(OutChannel, "%s\n", &MsgBuf[4]);
		(void) fflush(OutChannel);
	}
}
/*
**  FMTMSG -- format a message into buffer.
**
**	Parameters:
**		eb -- error buffer to get result.
**		to -- the recipient tag for this message.
**		num -- arpanet error number.
**		fmt -- format of string.
**		a, b, c, d, e -- arguments.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

/*VARARGS4*/
static
fmtmsg(eb, to, num, fmt, a, b, c, d, e)
	register char *eb;
	char *to;
	char *num;
	char *fmt;
{
	char del;

	/* output the reply code */
	if (isdigit(*fmt))
	{
		num = fmt;
		fmt += 4;
	}
	if (num[3] == '-')
		del = '-';
	else
		del = ' ';
	(void) sprintf(eb, "%3.3s%c", num, del);
	eb += 4;

	/* output the "to" person */
	if (to != NULL && to[0] != '\0')
	{
		(void) sprintf(eb, "%s... ", to);
		while (*eb != '\0')
			*eb++ &= 0177;
	}

	/* output the message */
	(void) sprintf(eb, fmt, a, b, c, d, e);
	while (*eb != '\0')
		*eb++ &= 0177;

	/* output the error code, if any */
	if (errno != 0)
	{
		extern int sys_nerr;
		extern char *sys_errlist[];
		if (errno < sys_nerr && errno > 0)
			(void) sprintf(eb, ": %s", sys_errlist[errno]);
		else
			(void) sprintf(eb, ": error %d", errno);
		eb += strlen(eb);
	}
}
