/*
 * Copyright (c) 1989 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)sleep.c	5.4 (Berkeley) 9/18/89";
#endif /* LIBC_SCCS and not lint */

#include <sys/time.h>
#include <sys/signal.h>

#define	setvec(vec, a) \
	vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

static int ringring;

sleep(seconds)
	unsigned int seconds;
{
	register struct itimerval *itp;
	struct itimerval itv, oitv;
	struct sigvec vec, ovec;
	long omask;
	void sleephandler();

	itp = &itv;
	if (!seconds)
		return;
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return;
	itp->it_value.tv_sec = seconds;
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >))
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
		else {
			itp->it_value = oitv.it_value;
			/*
			 * This is a hack, but we must have time to return
			 * from the setitimer after the alarm or else it'll
			 * be restarted.  And, anyway, sleep never did
			 * anything more than this before.
			 */
			oitv.it_value.tv_sec = 1;
			oitv.it_value.tv_usec = 0;
		}
	}
	setvec(vec, sleephandler);
	(void) sigvec(SIGALRM, &vec, &ovec);
	omask = sigblock(sigmask(SIGALRM));
	ringring = 0;
	(void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
	while (!ringring)
		sigpause(omask &~ sigmask(SIGALRM));
	(void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
	(void) sigsetmask(omask);
	(void) setitimer(ITIMER_REAL, &oitv, (struct itimerval *)0);
}

static void
sleephandler()
{
	ringring = 1;
}
