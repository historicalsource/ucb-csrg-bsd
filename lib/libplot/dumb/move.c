#ifndef lint
static char sccsid[] = "@(#)move.c	4.1 (Berkeley) 11/10/83";
#endif

#include "dumb.h"

move(x,y)
int x,y;
{
	scale(x, y);
	currentx = x;
	currenty = y;
}
