/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)callout.h	8.1 (Berkeley) 6/2/93
 */

struct callout {
	struct	callout *c_next;		/* next callout in queue */
	void	*c_arg;				/* function argument */
	void	(*c_func) __P((void *));	/* function to call */
	int	c_time;				/* ticks to the event */
};

#ifdef KERNEL
struct	callout *callfree, *callout, calltodo;
int	ncallout;
#endif
