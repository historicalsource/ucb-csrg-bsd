/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)sendmail.h	6.44 (Berkeley) 4/4/93
 */

/*
**  SENDMAIL.H -- Global definitions for sendmail.
*/

# ifdef _DEFINE
# define EXTERN
# ifndef lint
static char SmailSccsId[] =	"@(#)sendmail.h	6.44		4/4/93";
# endif
# else /*  _DEFINE */
# define EXTERN extern
# endif /* _DEFINE */

# include <unistd.h>
# include <stddef.h>
# include <stdlib.h>
# include <stdio.h>
# include <ctype.h>
# include <setjmp.h>
# include <sysexits.h>
# include <string.h>
# include <time.h>
# include <errno.h>
# include <sys/types.h>

# include "conf.h"
# include "useful.h"

# ifdef LOG
# include <syslog.h>
# endif /* LOG */

# ifdef DAEMON
# include <sys/socket.h>
# endif
# ifdef NETINET
# include <netinet/in.h>
# endif
# ifdef NETISO
# include <netiso/iso.h>
# endif
# ifdef NETNS
# include <netns/ns.h>
# endif
# ifdef NETX25
# include <netccitt/x25.h>
# endif




/*
**  Data structure for bit maps.
**
**	Each bit in this map can be referenced by an ascii character.
**	This is 128 possible bits, or 12 8-bit bytes.
*/

#define BITMAPBYTES	16	/* number of bytes in a bit map */
#define BYTEBITS	8	/* number of bits in a byte */

/* internal macros */
#define _BITWORD(bit)	(bit / (BYTEBITS * sizeof (int)))
#define _BITBIT(bit)	(1 << (bit % (BYTEBITS * sizeof (int))))

typedef int	BITMAP[BITMAPBYTES / sizeof (int)];

/* test bit number N */
#define bitnset(bit, map)	((map)[_BITWORD(bit)] & _BITBIT(bit))

/* set bit number N */
#define setbitn(bit, map)	(map)[_BITWORD(bit)] |= _BITBIT(bit)

/* clear bit number N */
#define clrbitn(bit, map)	(map)[_BITWORD(bit)] &= ~_BITBIT(bit)

/* clear an entire bit map */
#define clrbitmap(map)		bzero((char *) map, BITMAPBYTES)
/*
**  Address structure.
**	Addresses are stored internally in this structure.
*/

struct address
{
	char		*q_paddr;	/* the printname for the address */
	char		*q_user;	/* user name */
	char		*q_ruser;	/* real user name, or NULL if q_user */
	char		*q_host;	/* host name */
	struct mailer	*q_mailer;	/* mailer to use */
	u_short		q_flags;	/* status flags, see below */
	uid_t		q_uid;		/* user-id of receiver (if known) */
	gid_t		q_gid;		/* group-id of receiver (if known) */
	char		*q_home;	/* home dir (local mailer only) */
	char		*q_fullname;	/* full name if known */
	struct address	*q_next;	/* chain */
	struct address	*q_alias;	/* address this results from */
	char		*q_owner;	/* owner of q_alias */
	struct address	*q_tchain;	/* temporary use chain */
	time_t		q_timeout;	/* timeout for this address */
};

typedef struct address ADDRESS;

# define QDONTSEND	000001	/* don't send to this address */
# define QBADADDR	000002	/* this address is verified bad */
# define QGOODUID	000004	/* the q_uid q_gid fields are good */
# define QPRIMARY	000010	/* set from argv */
# define QQUEUEUP	000020	/* queue for later transmission */
# define QSENT		000040	/* has been successfully delivered */
# define QNOTREMOTE	000100	/* not an address for remote forwarding */
# define QSELFREF	000200	/* this address references itself */
# define QVERIFIED	000400	/* verified, but not expanded */
/*
**  Mailer definition structure.
**	Every mailer known to the system is declared in this
**	structure.  It defines the pathname of the mailer, some
**	flags associated with it, and the argument vector to
**	pass to it.  The flags are defined in conf.c
**
**	The argument vector is expanded before actual use.  All
**	words except the first are passed through the macro
**	processor.
*/

struct mailer
{
	char	*m_name;	/* symbolic name of this mailer */
	char	*m_mailer;	/* pathname of the mailer to use */
	BITMAP	m_flags;	/* status flags, see below */
	short	m_mno;		/* mailer number internally */
	char	**m_argv;	/* template argument vector */
	short	m_sh_rwset;	/* rewrite set: sender header addresses */
	short	m_se_rwset;	/* rewrite set: sender envelope addresses */
	short	m_rh_rwset;	/* rewrite set: recipient header addresses */
	short	m_re_rwset;	/* rewrite set: recipient envelope addresses */
	char	*m_eol;		/* end of line string */
	long	m_maxsize;	/* size limit on message to this mailer */
	int	m_linelimit;	/* max # characters per line */
	char	*m_execdir;	/* directory to chdir to before execv */
};

typedef struct mailer	MAILER;

/* bits for m_flags */
# define M_NOCOMMENT	'c'	/* don't include comment part of address */
# define M_CANONICAL	'C'	/* make addresses canonical "u@dom" */
		/*	'D'	/* CF: include Date: */
# define M_EXPENSIVE	'e'	/* it costs to use this mailer.... */
# define M_ESCFROM	'E'	/* escape From lines to >From */
# define M_FOPT		'f'	/* mailer takes picky -f flag */
		/*	'F'	/* CF: include From: or Resent-From: */
# define M_HST_UPPER	'h'	/* preserve host case distinction */
		/*	'H'	/* UIUC: MAIL11V3: preview headers */
# define M_INTERNAL	'I'	/* SMTP to another sendmail site */
# define M_LOCALMAILER	'l'	/* delivery is to this host */
# define M_LIMITS	'L'	/* must enforce SMTP line limits */
# define M_MUSER	'm'	/* can handle multiple users at once */
		/*	'M'	/* CF: include Message-Id: */
# define M_NHDR		'n'	/* don't insert From line */
		/*	'N'	/* UIUC: MAIL11V3: DATA returns multi-status */
# define M_FROMPATH	'p'	/* use reverse-path in MAIL FROM: */
		/*	'P'	/* CF: include Return-Path: */
# define M_ROPT		'r'	/* mailer takes picky -r flag */
# define M_SECURE_PORT	'R'	/* try to send on a reserved TCP port */
# define M_STRIPQ	's'	/* strip quote chars from user/host */
# define M_RESTR	'S'	/* must be daemon to execute */
# define M_USR_UPPER	'u'	/* preserve user case distinction */
# define M_UGLYUUCP	'U'	/* this wants an ugly UUCP from line */
		/*	'V'	/* UIUC: !-relativize all addresses */
# define M_XDOT		'X'	/* use hidden-dot algorithm */
# define M_7BITS	'7'	/* use 7-bit path */

EXTERN MAILER	*Mailer[MAXMAILERS+1];

EXTERN MAILER	*LocalMailer;		/* ptr to local mailer */
EXTERN MAILER	*ProgMailer;		/* ptr to program mailer */
EXTERN MAILER	*FileMailer;		/* ptr to *file* mailer */
EXTERN MAILER	*InclMailer;		/* ptr to *include* mailer */
/*
**  Header structure.
**	This structure is used internally to store header items.
*/

struct header
{
	char		*h_field;	/* the name of the field */
	char		*h_value;	/* the value of that field */
	struct header	*h_link;	/* the next header */
	u_short		h_flags;	/* status bits, see below */
	BITMAP		h_mflags;	/* m_flags bits needed */
};

typedef struct header	HDR;

/*
**  Header information structure.
**	Defined in conf.c, this struct declares the header fields
**	that have some magic meaning.
*/

struct hdrinfo
{
	char	*hi_field;	/* the name of the field */
	u_short	hi_flags;	/* status bits, see below */
};

extern struct hdrinfo	HdrInfo[];

/* bits for h_flags and hi_flags */
# define H_EOH		00001	/* this field terminates header */
# define H_RCPT		00002	/* contains recipient addresses */
# define H_DEFAULT	00004	/* if another value is found, drop this */
# define H_RESENT	00010	/* this address is a "Resent-..." address */
# define H_CHECK	00020	/* check h_mflags against m_flags */
# define H_ACHECK	00040	/* ditto, but always (not just default) */
# define H_FORCE	00100	/* force this field, even if default */
# define H_TRACE	00200	/* this field contains trace information */
# define H_FROM		00400	/* this is a from-type field */
# define H_VALID	01000	/* this field has a validated value */
# define H_RECEIPTTO	02000	/* this field has return receipt info */
# define H_ERRORSTO	04000	/* this field has error address info */
/*
**  Envelope structure.
**	This structure defines the message itself.  There is usually
**	only one of these -- for the message that we originally read
**	and which is our primary interest -- but other envelopes can
**	be generated during processing.  For example, error messages
**	will have their own envelope.
*/

struct envelope
{
	HDR		*e_header;	/* head of header list */
	long		e_msgpriority;	/* adjusted priority of this message */
	time_t		e_ctime;	/* time message appeared in the queue */
	char		*e_to;		/* the target person */
	char		*e_receiptto;	/* return receipt address */
	ADDRESS		e_from;		/* the person it is from */
	char		*e_sender;	/* e_from.q_paddr w comments stripped */
	char		**e_fromdomain;	/* the domain part of the sender */
	ADDRESS		*e_sendqueue;	/* list of message recipients */
	ADDRESS		*e_errorqueue;	/* the queue for error responses */
	long		e_msgsize;	/* size of the message in bytes */
	int		e_nrcpts;	/* number of recipients */
	short		e_class;	/* msg class (priority, junk, etc.) */
	short		e_flags;	/* flags, see below */
	short		e_hopcount;	/* number of times processed */
	short		e_nsent;	/* number of sends since checkpoint */
	short		e_sendmode;	/* message send mode */
	short		e_errormode;	/* error return mode */
	int		(*e_puthdr)();	/* function to put header of message */
	int		(*e_putbody)();	/* function to put body of message */
	struct envelope	*e_parent;	/* the message this one encloses */
	struct envelope *e_sibling;	/* the next envelope of interest */
	char		*e_df;		/* location of temp file */
	FILE		*e_dfp;		/* temporary file */
	char		*e_id;		/* code for this entry in queue */
	FILE		*e_xfp;		/* transcript file */
	FILE		*e_lockfp;	/* the lock file for this message */
	char		*e_message;	/* error message */
	char		*e_statmsg;	/* stat msg (changes per delivery) */
	char		*e_macro[128];	/* macro definitions */
};

typedef struct envelope	ENVELOPE;

/* values for e_flags */
#define EF_OLDSTYLE	000001		/* use spaces (not commas) in hdrs */
#define EF_INQUEUE	000002		/* this message is fully queued */
#define EF_TIMEOUT	000004		/* this message is too old */
#define EF_CLRQUEUE	000010		/* disk copy is no longer needed */
#define EF_SENDRECEIPT	000020		/* send a return receipt */
#define EF_FATALERRS	000040		/* fatal errors occured */
#define EF_KEEPQUEUE	000100		/* keep queue files always */
#define EF_RESPONSE	000200		/* this is an error or return receipt */
#define EF_RESENT	000400		/* this message is being forwarded */
#define EF_VRFYONLY	001000		/* verify only (don't expand aliases) */
#define EF_WARNING	002000		/* warning message has been sent */
#define EF_QUEUERUN	004000		/* this envelope is from queue */

EXTERN ENVELOPE	*CurEnv;	/* envelope currently being processed */
/*
**  Message priority classes.
**
**	The message class is read directly from the Priority: header
**	field in the message.
**
**	CurEnv->e_msgpriority is the number of bytes in the message plus
**	the creation time (so that jobs ``tend'' to be ordered correctly),
**	adjusted by the message class, the number of recipients, and the
**	amount of time the message has been sitting around.  This number
**	is used to order the queue.  Higher values mean LOWER priority.
**
**	Each priority class point is worth WkClassFact priority points;
**	each recipient is worth WkRecipFact priority points.  Each time
**	we reprocess a message the priority is adjusted by WkTimeFact.
**	WkTimeFact should normally decrease the priority so that jobs
**	that have historically failed will be run later; thanks go to
**	Jay Lepreau at Utah for pointing out the error in my thinking.
**
**	The "class" is this number, unadjusted by the age or size of
**	this message.  Classes with negative representations will have
**	error messages thrown away if they are not local.
*/

struct priority
{
	char	*pri_name;	/* external name of priority */
	int	pri_val;	/* internal value for same */
};

EXTERN struct priority	Priorities[MAXPRIORITIES];
EXTERN int		NumPriorities;	/* pointer into Priorities */
/*
**  Rewrite rules.
*/

struct rewrite
{
	char	**r_lhs;	/* pattern match */
	char	**r_rhs;	/* substitution value */
	struct rewrite	*r_next;/* next in chain */
};

EXTERN struct rewrite	*RewriteRules[MAXRWSETS];

/*
**  Special characters in rewriting rules.
**	These are used internally only.
**	The COND* rules are actually used in macros rather than in
**		rewriting rules, but are given here because they
**		cannot conflict.
*/

/* left hand side items */
# define MATCHZANY	0220	/* match zero or more tokens */
# define MATCHANY	0221	/* match one or more tokens */
# define MATCHONE	0222	/* match exactly one token */
# define MATCHCLASS	0223	/* match one token in a class */
# define MATCHNCLASS	0224	/* match anything not in class */
# define MATCHREPL	0225	/* replacement on RHS for above */

/* right hand side items */
# define CANONNET	0226	/* canonical net, next token */
# define CANONHOST	0227	/* canonical host, next token */
# define CANONUSER	0230	/* canonical user, next N tokens */
# define CALLSUBR	0231	/* call another rewriting set */

/* conditionals in macros */
# define CONDIF		0232	/* conditional if-then */
# define CONDELSE	0233	/* conditional else */
# define CONDFI		0234	/* conditional fi */

/* bracket characters for host name lookup */
# define HOSTBEGIN	0235	/* hostname lookup begin */
# define HOSTEND	0236	/* hostname lookup end */

/* bracket characters for generalized lookup */
# define LOOKUPBEGIN	0205	/* generalized lookup begin */
# define LOOKUPEND	0206	/* generalized lookup end */

/* macro substitution character */
# define MACROEXPAND	0201	/* macro expansion */

/* to make the code clearer */
# define MATCHZERO	CANONHOST

/* external <==> internal mapping table */
struct metamac
{
	char	metaname;	/* external code (after $) */
	char	metaval;	/* internal code (as above) */
};
/*
**  Information about currently open connections to mailers, or to
**  hosts that we have looked up recently.
*/

# define MCI	struct mailer_con_info

MCI
{
	short		mci_flags;	/* flag bits, see below */
	short		mci_errno;	/* error number on last connection */
	short		mci_exitstat;	/* exit status from last connection */
	short		mci_state;	/* SMTP state */
	FILE		*mci_in;	/* input side of connection */
	FILE		*mci_out;	/* output side of connection */
	int		mci_pid;	/* process id of subordinate proc */
	char		*mci_phase;	/* SMTP phase string */
	struct mailer	*mci_mailer;	/* ptr to the mailer for this conn */
	char		*mci_host;	/* host name */
	time_t		mci_lastuse;	/* last usage time */
};


/* flag bits */
#define MCIF_VALID	00001		/* this entry is valid */
#define MCIF_TEMP	00002		/* don't cache this connection */
#define MCIF_CACHED	00004		/* currently in open cache */

/* states */
#define MCIS_CLOSED	0		/* no traffic on this connection */
#define MCIS_OPENING	1		/* sending initial protocol */
#define MCIS_OPEN	2		/* open, initial protocol sent */
#define MCIS_ACTIVE	3		/* message being sent */
#define MCIS_QUITING	4		/* running quit protocol */
#define MCIS_SSD	5		/* service shutting down */
#define MCIS_ERROR	6		/* I/O error on connection */
/*
**  Mapping functions
**
**	These allow arbitrary mappings in the config file.  The idea
**	(albeit not the implementation) comes from IDA sendmail.
*/


/*
**  The class of a map -- essentially the functions to call
*/

# define MAPCLASS	struct _mapclass

MAPCLASS
{
	bool	(*map_init)();		/* initialization function */
	char	*(*map_lookup)();	/* lookup function */
};


/*
**  An actual map.
*/

# define MAP		struct _map

MAP
{
	MAPCLASS	*map_class;	/* the class of this map */
	int		map_flags;	/* flags, see below */
	char		*map_file;	/* the (nominal) filename */
	void		*map_db;	/* the open database ptr */
	char		*map_app;	/* to append to successful matches */
	char		*map_domain;	/* the (nominal) NIS domain */
	char		*map_rebuild;	/* program to run to do auto-rebuild */
};

/* bit values for map_flags */
# define MF_VALID	00001		/* this entry is valid */
# define MF_INCLNULL	00002		/* include null byte in key */
# define MF_OPTIONAL	00004		/* don't complain if map not found */
# define MF_NOFOLDCASE	00010		/* don't fold case in keys */
# define MF_MATCHONLY	00020		/* don't use the map value */
/*
**  Symbol table definitions
*/

struct symtab
{
	char		*s_name;	/* name to be entered */
	char		s_type;		/* general type (see below) */
	struct symtab	*s_next;	/* pointer to next in chain */
	union
	{
		BITMAP		sv_class;	/* bit-map of word classes */
		ADDRESS		*sv_addr;	/* pointer to address header */
		MAILER		*sv_mailer;	/* pointer to mailer */
		char		*sv_alias;	/* alias */
		MAPCLASS	sv_mapclass;	/* mapping function class */
		MAP		sv_map;		/* mapping function */
		char		*sv_hostsig;	/* host signature */
		MCI		sv_mci;		/* mailer connection info */
	}	s_value;
};

typedef struct symtab	STAB;

/* symbol types */
# define ST_UNDEF	0	/* undefined type */
# define ST_CLASS	1	/* class map */
# define ST_ADDRESS	2	/* an address in parsed format */
# define ST_MAILER	3	/* a mailer header */
# define ST_ALIAS	4	/* an alias */
# define ST_MAPCLASS	5	/* mapping function class */
# define ST_MAP		6	/* mapping function */
# define ST_HOSTSIG	7	/* host signature */
# define ST_MCI		16	/* mailer connection info (offset) */

# define s_class	s_value.sv_class
# define s_address	s_value.sv_addr
# define s_mailer	s_value.sv_mailer
# define s_alias	s_value.sv_alias
# define s_mci		s_value.sv_mci
# define s_mapclass	s_value.sv_mapclass
# define s_hostsig	s_value.sv_hostsig
# define s_map		s_value.sv_map

extern STAB	*stab();

/* opcodes to stab */
# define ST_FIND	0	/* find entry */
# define ST_ENTER	1	/* enter if not there */
/*
**  STRUCT EVENT -- event queue.
**
**	Maintained in sorted order.
**
**	We store the pid of the process that set this event to insure
**	that when we fork we will not take events intended for the parent.
*/

struct event
{
	time_t		ev_time;	/* time of the function call */
	int		(*ev_func)();	/* function to call */
	int		ev_arg;		/* argument to ev_func */
	int		ev_pid;		/* pid that set this event */
	struct event	*ev_link;	/* link to next item */
};

typedef struct event	EVENT;

EXTERN EVENT	*EventQueue;		/* head of event queue */
/*
**  Operation, send, and error modes
**
**	The operation mode describes the basic operation of sendmail.
**	This can be set from the command line, and is "send mail" by
**	default.
**
**	The send mode tells how to send mail.  It can be set in the
**	configuration file.  It's setting determines how quickly the
**	mail will be delivered versus the load on your system.  If the
**	-v (verbose) flag is given, it will be forced to SM_DELIVER
**	mode.
**
**	The error mode tells how to return errors.
*/

EXTERN char	OpMode;		/* operation mode, see below */

#define MD_DELIVER	'm'		/* be a mail sender */
#define MD_SMTP		's'		/* run SMTP on standard input */
#define MD_DAEMON	'd'		/* run as a daemon */
#define MD_VERIFY	'v'		/* verify: don't collect or deliver */
#define MD_TEST		't'		/* test mode: resolve addrs only */
#define MD_INITALIAS	'i'		/* initialize alias database */
#define MD_PRINT	'p'		/* print the queue */
#define MD_FREEZE	'z'		/* freeze the configuration file */


/* values for e_sendmode -- send modes */
#define SM_DELIVER	'i'		/* interactive delivery */
#define SM_QUICKD	'j'		/* deliver w/o queueing */
#define SM_FORK		'b'		/* deliver in background */
#define SM_QUEUE	'q'		/* queue, don't deliver */
#define SM_VERIFY	'v'		/* verify only (used internally) */

/* used only as a parameter to sendall */
#define SM_DEFAULT	'\0'		/* unspecified, use SendMode */


/* values for e_errormode -- error handling modes */
#define EM_PRINT	'p'		/* print errors */
#define EM_MAIL		'm'		/* mail back errors */
#define EM_WRITE	'w'		/* write back errors */
#define EM_BERKNET	'e'		/* special berknet processing */
#define EM_QUIET	'q'		/* don't print messages (stat only) */

/* Offset used to ensure that name server error * codes are unique */
#define	MAX_ERRNO	100

/* privacy flags */
#define PRIV_PUBLIC		0	/* what have I got to hide? */
#define PRIV_NEEDMAILHELO	00001	/* insist on HELO for MAIL, at least */
#define PRIV_NEEDEXPNHELO	00002	/* insist on HELO for EXPN */
#define PRIV_NEEDVRFYHELO	00004	/* insist on HELO for VRFY */
#define PRIV_NOEXPN		00010	/* disallow EXPN command entirely */
#define PRIV_NOVRFY		00020	/* disallow VRFY command entirely */
#define PRIV_AUTHWARNINGS	00040	/* flag possible authorization probs */
#define PRIV_RESTRMAILQ		01000	/* restrict mailq command */
#define PRIV_GOAWAY		00777	/* don't give no info, anyway, anyhow */

/* struct defining such things */
struct prival
{
	char	*pv_name;	/* name of privacy flag */
	int	pv_flag;	/* numeric level */
};

/*
**  Regular UNIX sockaddrs are too small to handle ISO addresses, so
**  we are forced to declare a supertype here.
*/

union bigsockaddr
{
	struct sockaddr		sa;	/* general version */
#ifdef NETINET
	struct sockaddr_in	sin;	/* INET family */
#endif
#ifdef NETISO
	struct sockaddr_iso	siso;	/* ISO family */
#endif
#ifdef NETNS
	struct sockaddr_ns	sns;	/* XNS family */
#endif
#ifdef NETX25
	struct sockaddr_x25	sx25;	/* X.25 family */
#endif
};

#define SOCKADDR	union bigsockaddr

/*
**  Global variables.
*/

EXTERN bool	FromFlag;	/* if set, "From" person is explicit */
EXTERN bool	NoAlias;	/* if set, don't do any aliasing */
EXTERN bool	MeToo;		/* send to the sender also */
EXTERN bool	IgnrDot;	/* don't let dot end messages */
EXTERN bool	SaveFrom;	/* save leading "From" lines */
EXTERN bool	Verbose;	/* set if blow-by-blow desired */
EXTERN bool	GrabTo;		/* if set, get recipients from msg */
EXTERN bool	NoReturn;	/* don't return letter to sender */
EXTERN bool	SuprErrs;	/* set if we are suppressing errors */
EXTERN bool	HoldErrs;	/* only output errors to transcript */
EXTERN bool	NoConnect;	/* don't connect to non-local mailers */
EXTERN bool	SuperSafe;	/* be extra careful, even if expensive */
EXTERN bool	ForkQueueRuns;	/* fork for each job when running the queue */
EXTERN bool	AutoRebuild;	/* auto-rebuild the alias database as needed */
EXTERN bool	CheckAliases;	/* parse addresses during newaliases */
EXTERN bool	UseNameServer;	/* use internet domain name server */
EXTERN bool	EightBit;	/* try to preserve 8-bit data */
EXTERN int	SafeAlias;	/* minutes to wait until @:@ in alias file */
EXTERN FILE	*InChannel;	/* input connection */
EXTERN FILE	*OutChannel;	/* output connection */
EXTERN uid_t	RealUid;	/* when Daemon, real uid of caller */
EXTERN gid_t	RealGid;	/* when Daemon, real gid of caller */
EXTERN uid_t	DefUid;		/* default uid to run as */
EXTERN gid_t	DefGid;		/* default gid to run as */
EXTERN char	*DefUser;	/* default user to run as (from DefUid) */
EXTERN int	OldUmask;	/* umask when sendmail starts up */
EXTERN int	Errors;		/* set if errors (local to single pass) */
EXTERN int	ExitStat;	/* exit status code */
EXTERN int	AliasLevel;	/* depth of aliasing */
EXTERN int	LineNumber;	/* line number in current input */
EXTERN int	LogLevel;	/* level of logging to perform */
EXTERN int	FileMode;	/* mode on files */
EXTERN int	QueueLA;	/* load average starting forced queueing */
EXTERN int	RefuseLA;	/* load average refusing connections are */
EXTERN int	CurrentLA;	/* current load average */
EXTERN long	QueueFactor;	/* slope of queue function */
EXTERN time_t	QueueIntvl;	/* intervals between running the queue */
EXTERN char	*AliasFile;	/* location of alias file */
EXTERN char	*HelpFile;	/* location of SMTP help file */
EXTERN char	*ErrMsgFile;	/* file to prepend to all error messages */
EXTERN char	*StatFile;	/* location of statistics summary */
EXTERN char	*QueueDir;	/* location of queue directory */
EXTERN char	*FileName;	/* name to print on error messages */
EXTERN char	*SmtpPhase;	/* current phase in SMTP processing */
EXTERN char	*MyHostName;	/* name of this host for SMTP messages */
EXTERN char	*RealHostName;	/* name of host we are talking to */
EXTERN SOCKADDR RealHostAddr;	/* address of host we are talking to */
EXTERN char	*CurHostName;	/* current host we are dealing with */
EXTERN jmp_buf	TopFrame;	/* branch-to-top-of-loop-on-error frame */
EXTERN bool	QuickAbort;	/*  .... but only if we want a quick abort */
EXTERN bool	LogUsrErrs;	/* syslog user errors (e.g., SMTP RCPT cmd) */
EXTERN int	PrivacyFlags;	/* privacy flags */
extern char	*ConfFile;	/* location of configuration file [conf.c] */
extern char	*FreezeFile;	/* location of frozen memory image [conf.c] */
extern char	*PidFile;	/* location of proc id file [conf.c] */
extern ADDRESS	NullAddress;	/* a null (template) address [main.c] */
EXTERN char	SpaceSub;	/* substitution for <lwsp> */
EXTERN long	WkClassFact;	/* multiplier for message class -> priority */
EXTERN long	WkRecipFact;	/* multiplier for # of recipients -> priority */
EXTERN long	WkTimeFact;	/* priority offset each time this job is run */
EXTERN char	*PostMasterCopy;	/* address to get errs cc's */
EXTERN int	CheckpointInterval;	/* queue file checkpoint interval */
EXTERN char	*UdbSpec;	/* user database source spec [udbexpand.c] */
EXTERN int	MaxHopCount;	/* number of hops until we give an error */
EXTERN int	ConfigLevel;	/* config file level -- what does .cf expect? */
EXTERN char	*TimeZoneSpec;	/* override time zone specification */
EXTERN bool	MatchGecos;	/* look for user names in gecos field */
EXTERN bool	DontPruneRoutes;	/* don't prune source routes */
EXTERN int	MaxMciCache;	/* maximum entries in MCI cache */
EXTERN time_t	MciCacheTimeout;	/* maximum idle time on connections */
EXTERN char	*ForwardPath;	/* path to search for .forward files */
EXTERN long	MinBlocksFree;	/* minimum number of blocks free on queue fs */
EXTERN char	*QueueLimitRecipient;	/* limit queue runs to this recipient */
EXTERN char	*QueueLimitSender;	/* limit queue runs to this sender */
EXTERN char	*QueueLimitId;		/* limit queue runs to this id */
EXTERN char	*FallBackMX;	/* fall back MX host */


/*
**  Timeouts
**
**	Indicated values are the MINIMUM per RFC 1123 section 5.3.2.
*/

EXTERN struct
{
	time_t	to_initial;	/* initial greeting timeout [5m] */
	time_t	to_mail;	/* MAIL command [5m] */
	time_t	to_rcpt;	/* RCPT command [5m] */
	time_t	to_datainit;	/* DATA initiation [2m] */
	time_t	to_datablock;	/* DATA block [3m] */
	time_t	to_datafinal;	/* DATA completion [10m] */
	time_t	to_nextcommand;	/* next command [5m] */
			/* following timeouts are not mentioned in RFC 1123 */
	time_t	to_rset;	/* RSET command */
	time_t	to_helo;	/* HELO command */
	time_t	to_quit;	/* QUIT command */
	time_t	to_miscshort;	/* misc short commands (NOOP, VERB, etc) */
			/* following are per message */
	time_t	to_q_return;	/* queue return timeout */
	time_t	to_q_warning;	/* queue warning timeout */
} TimeOuts;


/*
**  Trace information
*/

/* trace vector and macros for debugging flags */
EXTERN u_char	tTdvect[100];
# define tTd(flag, level)	(tTdvect[flag] >= level)
# define tTdlevel(flag)		(tTdvect[flag])
/*
**  Miscellaneous information.
*/



/*
**  Some in-line functions
*/

/* set exit status */
#define setstat(s)	{ \
				if (ExitStat == EX_OK || ExitStat == EX_TEMPFAIL) \
					ExitStat = s; \
			}

/* make a copy of a string */
#define newstr(s)	strcpy(xalloc(strlen(s) + 1), s)

#define STRUCTCOPY(s, d)	d = s


/*
**  Declarations of useful functions
*/

#if defined(__STDC__) && defined(_FORGIVING_CC_)
#define P(protos)	protos
#else
#define P(protos)	()
#endif

extern ADDRESS	*parseaddr P((char *, ADDRESS *, int, char, char **, ENVELOPE *));
extern char	*xalloc P((int));
extern bool	sameaddr P((ADDRESS *, ADDRESS *));
extern FILE	*dfopen P((char *, char *));
extern EVENT	*setevent P((time_t, int(*)(), int));
extern char	*sfgets P((char *, int, FILE *, time_t));
extern char	*queuename P((ENVELOPE *, char));
extern time_t	curtime P(());
extern bool	transienterror P((int));
extern char	*errstring P((int));

/* ellipsis is a different case though */
#ifdef __STDC__
extern void	auth_warning(ENVELOPE *, char *, ...);
extern void	syserr(char *, ...);
extern void	usrerr(char *, ...);
extern void	message(char *, ...);
extern void	nmessage(char *, ...);
#else
extern void	auth_warning();
extern void	syserr();
extern void	usrerr();
extern void	message();
extern void	nmessage();
#endif
