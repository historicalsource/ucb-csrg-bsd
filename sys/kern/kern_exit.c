/*
 * Copyright (c) 1982, 1986, 1989, 1991 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)kern_exit.c	7.34 (Berkeley) 5/12/91
 */

#include "param.h"
#include "systm.h"
#include "map.h"
#include "ioctl.h"
#include "tty.h"
#include "time.h"
#include "resource.h"
#include "kernel.h"
#include "proc.h"
#include "buf.h"
#include "wait.h"
#include "file.h"
#include "vnode.h"
#include "syslog.h"
#include "malloc.h"
#include "resourcevar.h"

#include "machine/cpu.h"
#ifdef COMPAT_43
#include "machine/reg.h"
#include "machine/psl.h"
#endif

#include "vm/vm.h"
#include "vm/vm_kern.h"

/*
 * Exit system call: pass back caller's arg
 */
/* ARGSUSED */
rexit(p, uap, retval)
	struct proc *p;
	struct args {
		int	rval;
	} *uap;
	int *retval;
{

	exit(p, W_EXITCODE(uap->rval, 0));
	/* NOTREACHED */
}

/*
 * Exit: deallocate address space and other resources,
 * change proc state to zombie, and unlink proc from allproc
 * and parent's lists.  Save exit status and rusage for wait().
 * Check for child processes and orphan them.
 */
exit(p, rv)
	register struct proc *p;
	int rv;
{
	register struct proc *q, *nq;
	register struct proc **pp;
	int s;

#ifdef PGINPROF
	vmsizmon();
#endif
	MALLOC(p->p_ru, struct rusage *, sizeof(struct rusage),
		M_ZOMBIE, M_WAITOK);
	/*
	 * If parent is waiting for us to exit or exec,
	 * SPPWAIT is set; we will wakeup the parent below.
	 */
	p->p_flag &= ~(STRC|SPPWAIT);
	p->p_flag |= SWEXIT;
	p->p_sigignore = ~0;
	p->p_sig = 0;
	untimeout(realitexpire, (caddr_t)p);

	/*
	 * Close open files and release open-file table.
	 * This may block!
	 */
	fdfree(p);
#ifdef SYSVSHM
	if (p->p_vmspace->vm_shm)
		shmexit(p);
#endif

	if (p->p_pid == 1)
		panic("init died");
	if (SESS_LEADER(p)) {
		register struct session *sp = p->p_session;

		if (sp->s_ttyvp) {
			/*
			 * Controlling process.
			 * Signal foreground pgrp,
			 * drain controlling terminal
			 * and revoke access to controlling terminal.
			 */
			if (sp->s_ttyp->t_session == sp) {
				if (sp->s_ttyp->t_pgrp)
					pgsignal(sp->s_ttyp->t_pgrp, SIGHUP, 1);
				(void) ttywait(sp->s_ttyp);
				vgoneall(sp->s_ttyvp);
			}
			vrele(sp->s_ttyvp);
			sp->s_ttyvp = NULL;
			/*
			 * s_ttyp is not zero'd; we use this to indicate
			 * that the session once had a controlling terminal.
			 * (for logging and informational purposes)
			 */
		}
		sp->s_leader = NULL;
	}
	fixjobc(p, p->p_pgrp, 0);
	p->p_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	(void) acct(p);
	if (--p->p_limit->p_refcnt == 0)
		FREE(p->p_limit, M_SUBPROC);
	if (--p->p_cred->p_refcnt == 0) {
		crfree(p->p_cred->pc_ucred);
		FREE(p->p_cred, M_SUBPROC);
	}
#ifdef KTRACE
	/* 
	 * release trace file
	 */
	if (p->p_tracep)
		vrele(p->p_tracep);
#endif

	/*
	 * Remove proc from allproc queue and pidhash chain.
	 * Place onto zombproc.  Unlink from parent's child list.
	 */
	if (*p->p_prev = p->p_nxt)
		p->p_nxt->p_prev = p->p_prev;
	if (p->p_nxt = zombproc)
		p->p_nxt->p_prev = &p->p_nxt;
	p->p_prev = &zombproc;
	zombproc = p;
	p->p_stat = SZOMB;
	curproc = NULL;
	for (pp = &pidhash[PIDHASH(p->p_pid)]; *pp; pp = &(*pp)->p_hash)
		if (*pp == p) {
			*pp = p->p_hash;
			goto done;
		}
	panic("exit");
done:

	if (p->p_cptr)		/* only need this if any child is S_ZOMB */
		wakeup((caddr_t) initproc);
	for (q = p->p_cptr; q != NULL; q = nq) {
		nq = q->p_osptr;
		if (nq != NULL)
			nq->p_ysptr = NULL;
		if (initproc->p_cptr)
			initproc->p_cptr->p_ysptr = q;
		q->p_osptr = initproc->p_cptr;
		q->p_ysptr = NULL;
		initproc->p_cptr = q;

		q->p_pptr = initproc;
		/*
		 * Traced processes are killed
		 * since their existence means someone is screwing up.
		 */
		if (q->p_flag&STRC) {
			q->p_flag &= ~STRC;
			psignal(q, SIGKILL);
		}
	}
	p->p_cptr = NULL;

	/*
	 * Save exit status and final rusage info,
	 * adding in child rusage info and self times.
	 */
	p->p_xstat = rv;
	*p->p_ru = p->p_stats->p_ru;
	p->p_ru->ru_stime = p->p_stime;
	p->p_ru->ru_utime = p->p_utime;
	ruadd(p->p_ru, &p->p_stats->p_cru);

	/*
	 * Notify parent that we're gone.
	 */
	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t)p->p_pptr);
#if defined(tahoe)
	/* move this to cpu_exit */
	p->p_addr->u_pcb.pcb_savacc.faddr = (float *)NULL;
#endif
	/*
	 * Finally, call machine-dependent code to release the remaining
	 * resources including address space, the kernel stack and pcb.
	 * The address space is released by "vmspace_free(p->p_vmspace)";
	 * This is machine-dependent, as we may have to change stacks
	 * or ensure that the current one isn't reallocated before we
	 * finish.  cpu_exit will end with a call to swtch(), finishing
	 * our execution (pun intended).
	 */
	cpu_exit(p);
	/* NOTREACHED */
}

#ifdef COMPAT_43
owait(p, uap, retval)
	struct proc *p;
	register struct args {
		int	pid;
		int	*status;
		int	options;
		struct	rusage *rusage;
		int	compat;
	} *uap;
	int *retval;
{

	if ((p->p_regs[PS] & PSL_ALLCC) != PSL_ALLCC) {
		uap->options = 0;
		uap->rusage = 0;
	} else {
		uap->options = p->p_regs[R0];
		uap->rusage = (struct rusage *)p->p_regs[R1];
	}
	uap->pid = WAIT_ANY;
	uap->status = 0;
	uap->compat = 1;
	return (wait1(p, uap, retval));
}

wait4(p, uap, retval)
	struct proc *p;
	struct args {
		int	pid;
		int	*status;
		int	options;
		struct	rusage *rusage;
		int	compat;
	} *uap;
	int *retval;
{

	uap->compat = 0;
	return (wait1(p, uap, retval));
}
#else
#define	wait1	wait4
#endif

/*
 * Wait: check child processes to see if any have exited,
 * stopped under trace, or (optionally) stopped by a signal.
 * Pass back status and deallocate exited child's proc structure.
 */
wait1(q, uap, retval)
	register struct proc *q;
	register struct args {
		int	pid;
		int	*status;
		int	options;
		struct	rusage *rusage;
#ifdef COMPAT_43
		int compat;
#endif
	} *uap;
	int retval[];
{
	register int nfound;
	register struct proc *p;
	int status, error;

	if (uap->pid == 0)
		uap->pid = -q->p_pgid;
#ifdef notyet
	if (uap->options &~ (WUNTRACED|WNOHANG))
		return (EINVAL);
#endif
loop:
	nfound = 0;
	for (p = q->p_cptr; p; p = p->p_osptr) {
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid && p->p_pgid != -uap->pid)
			continue;
		nfound++;
		if (p->p_stat == SZOMB) {
			retval[0] = p->p_pid;
#ifdef COMPAT_43
			if (uap->compat)
				retval[1] = p->p_xstat;
			else
#endif
			if (uap->status) {
				status = p->p_xstat;	/* convert to int */
				if (error = copyout((caddr_t)&status,
				    (caddr_t)uap->status, sizeof(status)))
					return (error);
			}
			if (uap->rusage && (error = copyout((caddr_t)p->p_ru,
			    (caddr_t)uap->rusage, sizeof (struct rusage))))
				return (error);
			p->p_xstat = 0;
			ruadd(&q->p_stats->p_cru, p->p_ru);
			FREE(p->p_ru, M_ZOMBIE);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(p);
			if (*p->p_prev = p->p_nxt)	/* off zombproc */
				p->p_nxt->p_prev = p->p_prev;
			if (q = p->p_ysptr)
				q->p_osptr = p->p_osptr;
			if (q = p->p_osptr)
				q->p_ysptr = p->p_ysptr;
			if ((q = p->p_pptr)->p_cptr == p)
				q->p_cptr = p->p_osptr;

#ifdef i386
			cpu_wait(p);			/* XXX */
#endif

			FREE(p, M_PROC);
			nprocs--;
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_flag & SWTED) == 0 &&
		    (p->p_flag & STRC || uap->options & WUNTRACED)) {
			p->p_flag |= SWTED;
			retval[0] = p->p_pid;
#ifdef COMPAT_43
			if (uap->compat) {
				retval[1] = W_STOPCODE(p->p_xstat);
				error = 0;
			} else
#endif
			if (uap->status) {
				status = W_STOPCODE(p->p_xstat);
				error = copyout((caddr_t)&status,
				    (caddr_t)uap->status, sizeof(status));
			} else
				error = 0;
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (uap->options & WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	if (error = tsleep((caddr_t)q, PWAIT | PCATCH, "wait", 0))
		return (error);
	goto loop;
}
