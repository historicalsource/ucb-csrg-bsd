/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * %sccs.include.redist.c%
 *
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	7.3 (Berkeley) 4/19/92
 */

#include "param.h"
#include "kernel.h"

#include "../include/machConst.h"
#include "clockreg.h"

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardare clock lost state.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * Start the real-time clock.
 */
startrtclock()
{
	register volatile struct chiptime *c;
	extern int tickadj;

	tick = 15625;		/* number of micro-seconds between interrupts */
	hz = 1000000 / 15625;	/* 64 Hz */
	tickadj = 240000 / (60000000 / 15625);
	c = (volatile struct chiptime *)MACH_CLOCK_ADDR;
	c->rega = REGA_TIME_BASE | SELECTED_RATE;
	c->regb = REGB_PER_INT_ENA | REGB_DATA_MODE | REGB_HOURS_FORMAT;
}

/*
 * This code is defunct after 2099.
 * Will Unix still be here then??
 */
static short dayyr[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/*
 * Initialze the time of day register, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock (if any) provides the rest.
 */
void
inittodr(base)
	time_t base;
{
	register volatile struct chiptime *c;
	register int days, yr;
	int sec, min, hour, day, mon, year;
	long deltat;
	int badbase, s;

	if (base < 5*SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = 6*SECYR + 186*SECDAY + SECDAY/2;
		badbase = 1;
	} else
		badbase = 0;

	c = (volatile struct chiptime *)MACH_CLOCK_ADDR;
	/* don't read clock registers while they are being updated */
	s = splclock();
	while ((c->rega & REGA_UIP) == 1)
		;
	sec = c->sec;
	min = c->min;
	hour = c->hour;
	day = c->day;
	mon = c->mon;
	year = c->year + 20; /* must be multiple of 4 because chip knows leap */
	splx(s);

	/* simple sanity checks */
	if (year < 70 || mon < 1 || mon > 12 || day < 1 || day > 31 ||
	    hour > 23 || min > 59 || sec > 59) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		time.tv_sec = base;
		if (!badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		goto bad;
	}
	days = 0;
	for (yr = 70; yr < year; yr++)
		days += LEAPYEAR(yr) ? 366 : 365;
	days += dayyr[mon - 1] + day - 1;
	if (LEAPYEAR(yr) && mon > 2)
		days++;
	/* now have days since Jan 1, 1970; the rest is easy... */
	time.tv_sec = days * SECDAY + hour * 3600 + min * 60 + sec;

	if (!badbase) {
		/*
		 * See if we gained/lost two or more days;
		 * if so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;
		printf("WARNING: clock %s %d days",
		    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
bad:
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
resettodr()
{
	register volatile struct chiptime *c;
	register int t, t2;
	int sec, min, hour, day, mon, year;
	int s;

	/* compute the year */
	t2 = time.tv_sec / SECDAY;
	year = 69;
	while (t2 >= 0) {	/* whittle off years */
		t = t2;
		year++;
		t2 -= LEAPYEAR(year) ? 366 : 365;
	}

	/* t = month + day; separate */
	t2 = LEAPYEAR(year);
	for (mon = 1; mon < 12; mon++)
		if (t < dayyr[mon] + (t2 && mon > 1))
			break;

	day = t - dayyr[mon - 1] + 1;
	if (t2 && mon > 2)
		day--;

	/* the rest is easy */
	t = time.tv_sec % SECDAY;
	hour = t / 3600;
	t %= 3600;
	min = t / 60;
	sec = t % 60;

	c = (volatile struct chiptime *)MACH_CLOCK_ADDR;
	s = splclock();
	t = c->regb;
	c->regb = t | REGB_SET_TIME;
	MachEmptyWriteBuffer();
	c->sec = sec;
	c->min = min;
	c->hour = hour;
	c->day = day;
	c->mon = mon;
	c->year = year - 20; /* must be multiple of 4 because chip knows leap */
	c->regb = t;
	MachEmptyWriteBuffer();
	splx(s);
}
