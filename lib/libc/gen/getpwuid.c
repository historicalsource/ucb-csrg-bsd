#ifndef lint
static char sccsid[] = "@(#)getpwuid.c	5.1 (Berkeley) 6/5/85";
#endif not lint

#include <pwd.h>

struct passwd *
getpwuid(uid)
register uid;
{
	register struct passwd *p;
	struct passwd *getpwent();

	setpwent();
	while( (p = getpwent()) && p->pw_uid != uid );
	endpwent();
	return(p);
}
