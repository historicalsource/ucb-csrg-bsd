#ifndef lint
static char sccsid[] = "@(#)3.test.c	4.1	(Berkeley)	2/11/83";
#endif not lint

#include <stdio.h>
#
/* for testing only */
#include "def.h"

testreach()
	{
	VERT v;
	for (v = 0; v < nodenum; ++v)
		fprintf(stderr,"REACH(%d) = %d\n",v,REACH(v));
	}
