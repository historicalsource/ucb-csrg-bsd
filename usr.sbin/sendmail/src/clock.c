# include "sendmail.h"

SCCSID(@(#)clock.c	3.2		8/8/82);

/*
**  TICK -- take a clock tick
**
**	Someday this will have to do more complex event scheduling.
**
**	Parameters:
**		none.
**
**	Returns:
**		non-local through TickFrame.
**
**	Side Effects:
**		none.
*/

tick()
{
# ifdef DEBUG
	if (tTd(5, 1))
		printf("tick\n");
# endif DEBUG
	longjmp(TickFrame, 1);
}
