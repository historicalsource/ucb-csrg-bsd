/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)sethostname.c	5.1 (Berkeley) 4/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#if __STDC__
long
sethostname(const char *name, int namelen)
#else
long
sethostname(name, namelen)
	char *name;
	int namelen;
#endif
{
	int mib[2];

	mib[0] = CTL_KERN;
	mib[1] = KERN_HOSTNAME;
	return (sysctl(mib, 2, NULL, NULL, (void *)name, namelen));
}
