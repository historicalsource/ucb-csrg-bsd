/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*	@(#)globals.h	2.1	(Berkeley)	12/10/85	*/

#include <sys/param.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

extern int errno;
extern int sock;

#define RANGE		20
#define MSGS 		5
#define TRIALS		6
#define SAMPLEINTVL	240	
#define MAXSEQ 		30000
#define MINTOUT		360
#define MAXTOUT		900

#define GOOD		1
#define UNREACHABLE	2
#define NONSTDTIME	3
#define HOSTDOWN 	0x7fffffff

#define OFF	0
#define ON	1

#define SLAVE 	1
#define MASTER	2
#define IGNORE	4
#define ALL	(SLAVE|MASTER|IGNORE)
#define SUBMASTER	(SLAVE|MASTER)

#define NHOSTS		30	/* max number of hosts controlled by timed */

struct host {
	char *name;
	struct sockaddr_in addr;
	long delta;
	u_short seq;
};

struct netinfo {
	struct netinfo *next;
	u_long net;
	u_long mask;
	struct in_addr my_addr;
	struct sockaddr_in dest_addr;	/* broadcast addr or point-point */
	long status;
};

extern struct netinfo *nettab;
extern int status;
extern int trace;
extern int sock;
extern struct sockaddr_in from;
extern FILE *fd;
extern char hostname[];
extern char tracefile[];
extern struct host hp[];
extern int backoff;
extern long delay1, delay2;
extern int slvcount;

char *strcpy(), *malloc();
