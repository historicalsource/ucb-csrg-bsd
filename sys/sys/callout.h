/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)callout.h	7.3 (Berkeley) 7/8/92
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
