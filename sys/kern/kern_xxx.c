/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)kern_xxx.c	7.12 (Berkeley) 6/21/90
 */

#include "param.h"
#include "systm.h"
#include "syscontext.h"
#include "kernel.h"
#include "proc.h"
#include "reboot.h"

/* ARGSUSED */
gethostid(p, uap, retval)
	struct proc *p;
	void *uap;
	long *retval;
{

	*retval = hostid;
	RETURN (0);
}

sethostid(p, uap, retval)
	struct proc *p;
	struct args {
		long	hostid;
	} *uap;
	int *retval;
{
	int error;

	if (error = suser(u.u_cred, &u.u_acflag))
		RETURN (error);
	hostid = uap->hostid;
	RETURN (0);
}

/* ARGSUSED */
gethostname(p, uap, retval)
	struct proc *p;
	struct args {
		char	*hostname;
		u_int	len;
	} *uap;
	int *retval;
{

	if (uap->len > hostnamelen + 1)
		uap->len = hostnamelen + 1;
	RETURN (copyout((caddr_t)hostname, (caddr_t)uap->hostname, uap->len));
}

/* ARGSUSED */
sethostname(p, uap, retval)
	struct proc *p;
	register struct args {
		char	*hostname;
		u_int	len;
	} *uap;
	int *retval;
{
	int error;

	if (error = suser(u.u_cred, &u.u_acflag))
		RETURN (error);
	if (uap->len > sizeof (hostname) - 1)
		RETURN (EINVAL);
	hostnamelen = uap->len;
	error = copyin((caddr_t)uap->hostname, hostname, uap->len);
	hostname[hostnamelen] = 0;
	RETURN (error);
}

/* ARGSUSED */
reboot(p, uap, retval)
	struct proc *p;
	struct args {
		int	opt;
	} *uap;
	int *retval;
{
	int error;

	if (error = suser(u.u_cred, &u.u_acflag))
		RETURN (error);
	boot(uap->opt);
	RETURN (0);
}

ovhangup()
{

	RETURN (EINVAL);
}

oldquota()
{

	RETURN (EINVAL);
}
