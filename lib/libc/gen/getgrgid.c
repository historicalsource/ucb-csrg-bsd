#ifndef lint
static char sccsid[] = "@(#)getgrgid.c	5.1 (Berkeley) 6/5/85";
#endif not lint

#include <grp.h>

struct group *
getgrgid(gid)
register gid;
{
	register struct group *p;
	struct group *getgrent();

	setgrent();
	while( (p = getgrent()) && p->gr_gid != gid );
	endgrent();
	return(p);
}
