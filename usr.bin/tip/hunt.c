/*	hunt.c	4.3	81/10/02	*/
#include "tip.h"

#define RD	04

extern char *getremote();
extern char *rindex();

int deadfl;

dead()
{
	deadfl = 1;
}

hunt(name)
char *name;
{
	register char *cp;

	deadfl = 0;
	signal(SIGALRM, dead);
	while(cp = getremote(name)){
		if (access(cp, RD))
			continue;
		uucplock = rindex(cp, '/')+1;
		if (mlock(uucplock) < 0) {
			delock(uucplock);
			continue;
		}
		/*
		 * Straight through call units, such as the BIZCOMP
		 *  and the DF, must indicate they're hardwired in
		 *  order to get an open file descriptor placed in FD.
		 * Otherwise, as for a DN-11, the open will have to
		 *  be done in the "open" routine.
		 */
		if (!HW)
			break;
		alarm(10);
		if((FD = open(cp, 2)) >= 0){
			alarm(0);
			if(!deadfl) {
				ioctl(FD, TIOCEXCL, 0);
				signal(SIGALRM, SIG_DFL);
				return((int)cp);
			}
		}
		alarm(0);
		signal(SIGALRM, dead);
		delock(uucplock);
	}
	signal(SIGALRM, SIG_DFL);
	return(deadfl ? -1 : (int)cp);
}
