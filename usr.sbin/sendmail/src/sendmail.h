/*
**  POSTBOX.H -- Global definitions for postbox.
**
**	@(#)sendmail.h	3.6	3/9/81
*/




# include "useful.h"

/*
**  Manifest constants.
*/

# define MAXLINE	256	/* maximum line length */
# define MAXNAME	128	/* maximum length of a name */
# define MAXFIELD	2500	/* maximum total length of a header field */
# define MAXPV		15	/* maximum # of parms to mailers */
# define MAXHOP		30	/* maximum value of HopCount */
# define MAXATOM	15	/* max atoms per address */
# define ALIASFILE	"/usr/lib/aliases"	/* location of alias file */





/*
**  Mailer definition structure.
**	Every mailer known to the system is declared in this
**	structure.  It defines the pathname of the mailer, some
**	flags associated with it, and the argument vector to
**	pass to it.  The flags are defined in conf.c
**
**	The host map is a list of lists of strings.  Within each
**	list, any host is mapped to the last host in the list.
**	This allows multiple names, as well as doing clever
**	mail grouping in point-to-point networks.  Note: this
**	is only used internally, so the apparent host is still
**	kept around.
**
**	The argument vector is expanded before actual use.  Every-
**	thing is passed through except for things starting with "$".
**	"$x" defines some interpolation, as described in conf.c
**	"$x" where x is unknown expands to "x", so use "$$" to get "$".
*/

struct mailer
{
	char	*m_mailer;	/* pathname of the mailer to use */
	char	*m_name;	/* symbolic name of this mailer */
	short	m_flags;	/* status flags, see below */
	short	m_badstat;	/* the status code to use on unknown error */
	char	*m_from;	/* pattern for From: header */
	char	**m_argv;	/* template argument vector */
};

# define M_FOPT		000001	/* mailer takes picky -f flag */
# define M_ROPT		000002	/* mailer takes picky -r flag */
# define M_QUIET	000004	/* don't print error on bad status */
# define M_RESTR	000010	/* must be daemon to execute */
# define M_HDR		000020	/* insert From line */
# define M_NOHOST	000040	/* ignore host in comparisons */
# define M_STRIPQ	000100	/* strip quote characters from user/host */
# define M_FHDR		000200	/* force good From line */
# define M_NEEDFROM	000400	/* need arpa-style From: line */
# define M_NEEDDATE	001000	/* need arpa-style Date: line */
# define M_MSGID	002000	/* need Message-Id: field */
# define M_COMMAS	004000	/* need comma-seperated address lists */
# define M_USR_UPPER	010000	/* preserve user case distinction */
# define M_HST_UPPER	020000	/* preserve host case distinction */

# define M_ARPAFMT	(M_NEEDDATE|M_NEEDFROM|M_MSGID|M_COMMAS)

extern struct mailer *Mailer[];


/*
**  Address structure.
**	Addresses are stored internally in this structure.
*/

struct address
{
	char		*q_paddr;	/* the printname for the address */
	char		*q_user;	/* user name */
	char		*q_host;	/* host name */
	short		q_mailer;	/* mailer to use */
	short		q_rmailer;	/* real mailer (before mapping) */
	struct address	*q_next;	/* chain */
	struct address	*q_prev;	/* back pointer */
};

typedef struct address ADDRESS;

/* some other primitives */
# define nxtinq(q)	((q)->q_next)
# define clearq(q)	(q)->q_next = (q)->q_prev = NULL

extern ADDRESS SendQ;		/* queue of people to send to */
extern ADDRESS AliasQ;		/* queue of people that are aliases */





/*
**  Header structure.
**	This structure is used internally to store header items.
*/

struct header
{
	char		*h_field;	/* the name of the field */
	char		*h_value;	/* the value of that field */
	struct header	*h_link;	/* the next header */
	short		h_flags;	/* status bits, see below */
};

typedef struct header	HDR;

extern HDR	*Header;	/* head of header list */

/*
**  Header information structure.
**	Defined in conf.c, this struct declares the header fields
**	that have some magic meaning.
*/

struct hdrinfo
{
	char	*hi_field;	/* the name of the field */
	short	hi_flags;	/* status bits, see below */
	char	**hi_pptr;	/* &ptr to point to this value */
};

extern struct hdrinfo	HdrInfo[];

/* bits for h_flags and hi_flags */
# define H_EOH		00001	/* this field terminates header */
# define H_DELETE	00002	/* don't send this field */
# define H_DEFAULT	00004	/* if another value is found, drop this */
# define H_USED		00010	/* indicates that this has been output */


/*
**  Rewrite rules.
*/

struct rewrite
{
	char	**r_lhs;	/* pattern match */
	char	**r_rhs;	/* substitution value */
	struct rewrite	*r_next;/* next in chain */
};

struct rewrite	*RewriteRules;

# define MATCHANY	'\020'	/* match exactly one token */
# define MATCHONE	'\021'	/* match one or more tokens */

# define CANONNET	'\025'	/* canonical net, next token */
# define CANONHOST	'\026'	/* canonical host, next token */
# define CANONUSER	'\027'	/* canonical user, next N tokens */




/*
**  Global variables.
*/

extern bool	ArpaFmt;	/* if set, message is in arpanet fmt */
extern bool	FromFlag;	/* if set, "From" person is explicit */
extern bool	Debug;		/* if set, debugging info */
extern bool	MailBack;	/* mail back response on error */
extern bool	BerkNet;	/* called from BerkNet */
extern bool	WriteBack;	/* write back response on error */
extern bool	NoAlias;	/* if set, don't do any aliasing */
extern bool	ForceMail;	/* if set, mail even if already got a copy */
extern bool	MeToo;		/* send to the sender also */
extern bool	UseMsgId;	/* put msg-id's in all msgs [conf.c] */
extern bool	IgnrDot;	/* don't let dot end messages */
extern bool	SaveFrom;	/* save leading "From" lines */
extern int	Errors;		/* set if errors */
extern int	ExitStat;	/* exit status code */
extern char	InFileName[];	/* input file name */
extern char	Transcript[];	/* the transcript file name */
extern char	*MsgId;		/* the message id for this message */
extern char	*Date;		/* origination date (UNIX format) */
extern ADDRESS	From;		/* the person it is from */
extern char	*To;		/* the target person */
extern int	HopCount;	/* hop count */


# include	<sysexits.h>

# define setstat(s)		{ if (ExitStat == EX_OK) ExitStat = s; }
