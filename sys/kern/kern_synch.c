/*-
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)kern_synch.c	7.25 (Berkeley) 10/11/92
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/vmmeter.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <machine/cpu.h>

u_char	curpri;			/* usrpri of curproc */
int	lbolt;			/* once a second sleep address */

/*
 * Force switch among equal priority processes every 100ms.
 */
/* ARGSUSED */
void
roundrobin(arg)
	void *arg;
{

	need_resched();
	timeout(roundrobin, (void *)0, hz / 10);
}

/*
 * constants for digital decay and forget
 *	90% of (p_cpu) usage in 5*loadav time
 *	95% of (p_pctcpu) usage in 60 seconds (load insensitive)
 *          Note that, as ps(1) mentions, this can let percentages
 *          total over 100% (I've seen 137.9% for 3 processes).
 *
 * Note that hardclock updates p_cpu and p_cpticks independently.
 *
 * We wish to decay away 90% of p_cpu in (5 * loadavg) seconds.
 * That is, the system wants to compute a value of decay such
 * that the following for loop:
 * 	for (i = 0; i < (5 * loadavg); i++)
 * 		p_cpu *= decay;
 * will compute
 * 	p_cpu *= 0.1;
 * for all values of loadavg:
 *
 * Mathematically this loop can be expressed by saying:
 * 	decay ** (5 * loadavg) ~= .1
 *
 * The system computes decay as:
 * 	decay = (2 * loadavg) / (2 * loadavg + 1)
 *
 * We wish to prove that the system's computation of decay
 * will always fulfill the equation:
 * 	decay ** (5 * loadavg) ~= .1
 *
 * If we compute b as:
 * 	b = 2 * loadavg
 * then
 * 	decay = b / (b + 1)
 *
 * We now need to prove two things:
 *	1) Given factor ** (5 * loadavg) ~= .1, prove factor == b/(b+1)
 *	2) Given b/(b+1) ** power ~= .1, prove power == (5 * loadavg)
 *	
 * Facts:
 *         For x close to zero, exp(x) =~ 1 + x, since
 *              exp(x) = 0! + x**1/1! + x**2/2! + ... .
 *              therefore exp(-1/b) =~ 1 - (1/b) = (b-1)/b.
 *         For x close to zero, ln(1+x) =~ x, since
 *              ln(1+x) = x - x**2/2 + x**3/3 - ...     -1 < x < 1
 *              therefore ln(b/(b+1)) = ln(1 - 1/(b+1)) =~ -1/(b+1).
 *         ln(.1) =~ -2.30
 *
 * Proof of (1):
 *    Solve (factor)**(power) =~ .1 given power (5*loadav):
 *	solving for factor,
 *      ln(factor) =~ (-2.30/5*loadav), or
 *      factor =~ exp(-1/((5/2.30)*loadav)) =~ exp(-1/(2*loadav)) =
 *          exp(-1/b) =~ (b-1)/b =~ b/(b+1).                    QED
 *
 * Proof of (2):
 *    Solve (factor)**(power) =~ .1 given factor == (b/(b+1)):
 *	solving for power,
 *      power*ln(b/(b+1)) =~ -2.30, or
 *      power =~ 2.3 * (b + 1) = 4.6*loadav + 2.3 =~ 5*loadav.  QED
 *
 * Actual power values for the implemented algorithm are as follows:
 *      loadav: 1       2       3       4
 *      power:  5.68    10.32   14.94   19.55
 */

/* calculations for digital decay to forget 90% of usage in 5*loadav sec */
#define	loadfactor(loadav)	(2 * (loadav))
#define	decay_cpu(loadfac, cpu)	(((loadfac) * (cpu)) / ((loadfac) + FSCALE))

/* decay 95% of `p_pctcpu' in 60 seconds; see CCPU_SHIFT before changing */
fixpt_t	ccpu = 0.95122942450071400909 * FSCALE;		/* exp(-1/20) */

/*
 * If `ccpu' is not equal to `exp(-1/20)' and you still want to use the
 * faster/more-accurate formula, you'll have to estimate CCPU_SHIFT below
 * and possibly adjust FSHIFT in "param.h" so that (FSHIFT >= CCPU_SHIFT).
 *
 * To estimate CCPU_SHIFT for exp(-1/20), the following formula was used:
 *	1 - exp(-1/20) ~= 0.0487 ~= 0.0488 == 1 (fixed pt, *11* bits).
 *
 * If you dont want to bother with the faster/more-accurate formula, you
 * can set CCPU_SHIFT to (FSHIFT + 1) which will use a slower/less-accurate
 * (more general) method of calculating the %age of CPU used by a process.
 */
#define	CCPU_SHIFT	11

/*
 * Recompute process priorities, once a second
 */
/* ARGSUSED */
void
schedcpu(arg)
	void *arg;
{
	register fixpt_t loadfac = loadfactor(averunnable.ldavg[0]);
	register struct proc *p;
	register int s;
	register unsigned int newcpu;

	wakeup((caddr_t)&lbolt);
	for (p = (struct proc *)allproc; p != NULL; p = p->p_nxt) {
		/*
		 * Increment time in/out of memory and sleep time
		 * (if sleeping).  We ignore overflow; with 16-bit int's
		 * (remember them?) overflow takes 45 days.
		 */
		p->p_time++;
		if (p->p_stat == SSLEEP || p->p_stat == SSTOP)
			p->p_slptime++;
		p->p_pctcpu = (p->p_pctcpu * ccpu) >> FSHIFT;
		/*
		 * If the process has slept the entire second,
		 * stop recalculating its priority until it wakes up.
		 */
		if (p->p_slptime > 1)
			continue;
		/*
		 * p_pctcpu is only for ps.
		 */
#if	(FSHIFT >= CCPU_SHIFT)
		p->p_pctcpu += (hz == 100)?
			((fixpt_t) p->p_cpticks) << (FSHIFT - CCPU_SHIFT):
                	100 * (((fixpt_t) p->p_cpticks)
				<< (FSHIFT - CCPU_SHIFT)) / hz;
#else
		p->p_pctcpu += ((FSCALE - ccpu) *
			(p->p_cpticks * FSCALE / hz)) >> FSHIFT;
#endif
		p->p_cpticks = 0;
		newcpu = (u_int) decay_cpu(loadfac, p->p_cpu) + p->p_nice;
		p->p_cpu = min(newcpu, UCHAR_MAX);
		setpri(p);
		s = splhigh();	/* prevent state changes */
		if (p->p_pri >= PUSER) {
#define	PPQ	(128 / NQS)		/* priorities per queue */
			if ((p != curproc) &&
			    p->p_stat == SRUN &&
			    (p->p_flag & SLOAD) &&
			    (p->p_pri / PPQ) != (p->p_usrpri / PPQ)) {
				remrq(p);
				p->p_pri = p->p_usrpri;
				setrq(p);
			} else
				p->p_pri = p->p_usrpri;
		}
		splx(s);
	}
	vmmeter();
	if (bclnlist != NULL)
		wakeup((caddr_t)pageproc);
	timeout(schedcpu, (void *)0, hz);
}

/*
 * Recalculate the priority of a process after it has slept for a while.
 * For all load averages >= 1 and max p_cpu of 255, sleeping for at least
 * six times the loadfactor will decay p_cpu to zero.
 */
void
updatepri(p)
	register struct proc *p;
{
	register unsigned int newcpu = p->p_cpu;
	register fixpt_t loadfac = loadfactor(averunnable.ldavg[0]);

	if (p->p_slptime > 5 * loadfac)
		p->p_cpu = 0;
	else {
		p->p_slptime--;	/* the first time was done in schedcpu */
		while (newcpu && --p->p_slptime)
			newcpu = (int) decay_cpu(loadfac, newcpu);
		p->p_cpu = min(newcpu, UCHAR_MAX);
	}
	setpri(p);
}

#define SQSIZE 0100	/* Must be power of 2 */
#define HASH(x)	(( (int) x >> 5) & (SQSIZE-1))
struct slpque {
	struct proc *sq_head;
	struct proc **sq_tailp;
} slpque[SQSIZE];

/*
 * During autoconfiguration or after a panic, a sleep will simply
 * lower the priority briefly to allow interrupts, then return.
 * The priority to be used (safepri) is machine-dependent, thus this
 * value is initialized and maintained in the machine-dependent layers.
 * This priority will typically be 0, or the lowest priority
 * that is safe for use on the interrupt stack; it can be made
 * higher to block network software interrupts after panics.
 */
int safepri;

/*
 * General sleep call.
 * Suspends current process until a wakeup is made on chan.
 * The process will then be made runnable with priority pri.
 * Sleeps at most timo/hz seconds (0 means no timeout).
 * If pri includes PCATCH flag, signals are checked
 * before and after sleeping, else signals are not checked.
 * Returns 0 if awakened, EWOULDBLOCK if the timeout expires.
 * If PCATCH is set and a signal needs to be delivered,
 * ERESTART is returned if the current system call should be restarted
 * if possible, and EINTR is returned if the system call should
 * be interrupted by the signal (return EINTR).
 */
int
tsleep(chan, pri, wmesg, timo)
	void *chan;
	int pri;
	char *wmesg;
	int timo;
{
	register struct proc *p = curproc;
	register struct slpque *qp;
	register s;
	int sig, catch = pri & PCATCH;
	extern int cold;
	void endtsleep __P((void *));

#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p->p_tracep, 1, 0);
#endif
	s = splhigh();
	if (cold || panicstr) {
		/*
		 * After a panic, or during autoconfiguration,
		 * just give interrupts a chance, then just return;
		 * don't run any other procs or panic below,
		 * in case this is the idle process and already asleep.
		 */
		splx(safepri);
		splx(s);
		return (0);
	}
#ifdef DIAGNOSTIC
	if (chan == NULL || p->p_stat != SRUN || p->p_rlink)
		panic("tsleep");
#endif
	p->p_wchan = chan;
	p->p_wmesg = wmesg;
	p->p_slptime = 0;
	p->p_pri = pri & PRIMASK;
	qp = &slpque[HASH(chan)];
	if (qp->sq_head == 0)
		qp->sq_head = p;
	else
		*qp->sq_tailp = p;
	*(qp->sq_tailp = &p->p_link) = 0;
	if (timo)
		timeout(endtsleep, (void *)p, timo);
	/*
	 * We put ourselves on the sleep queue and start our timeout
	 * before calling CURSIG, as we could stop there, and a wakeup
	 * or a SIGCONT (or both) could occur while we were stopped.
	 * A SIGCONT would cause us to be marked as SSLEEP
	 * without resuming us, thus we must be ready for sleep
	 * when CURSIG is called.  If the wakeup happens while we're
	 * stopped, p->p_wchan will be 0 upon return from CURSIG.
	 */
	if (catch) {
		p->p_flag |= SSINTR;
		if (sig = CURSIG(p)) {
			if (p->p_wchan)
				unsleep(p);
			p->p_stat = SRUN;
			goto resume;
		}
		if (p->p_wchan == 0) {
			catch = 0;
			goto resume;
		}
	} else
		sig = 0;
	p->p_stat = SSLEEP;
	p->p_stats->p_ru.ru_nvcsw++;
	swtch();
resume:
	curpri = p->p_usrpri;
	splx(s);
	p->p_flag &= ~SSINTR;
	if (p->p_flag & STIMO) {
		p->p_flag &= ~STIMO;
		if (sig == 0) {
#ifdef KTRACE
			if (KTRPOINT(p, KTR_CSW))
				ktrcsw(p->p_tracep, 0, 0);
#endif
			return (EWOULDBLOCK);
		}
	} else if (timo)
		untimeout(endtsleep, (void *)p);
	if (catch && (sig != 0 || (sig = CURSIG(p)))) {
#ifdef KTRACE
		if (KTRPOINT(p, KTR_CSW))
			ktrcsw(p->p_tracep, 0, 0);
#endif
		if (p->p_sigacts->ps_sigintr & sigmask(sig))
			return (EINTR);
		return (ERESTART);
	}
#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p->p_tracep, 0, 0);
#endif
	return (0);
}

/*
 * Implement timeout for tsleep.
 * If process hasn't been awakened (wchan non-zero),
 * set timeout flag and undo the sleep.  If proc
 * is stopped, just unsleep so it will remain stopped.
 */
void
endtsleep(arg)
	void *arg;
{
	register struct proc *p;
	int s;

	p = (struct proc *)arg;
	s = splhigh();
	if (p->p_wchan) {
		if (p->p_stat == SSLEEP)
			setrun(p);
		else
			unsleep(p);
		p->p_flag |= STIMO;
	}
	splx(s);
}

/*
 * Short-term, non-interruptable sleep.
 */
void
sleep(chan, pri)
	void *chan;
	int pri;
{
	register struct proc *p = curproc;
	register struct slpque *qp;
	register s;
	extern int cold;

#ifdef DIAGNOSTIC
	if (pri > PZERO) {
		printf("sleep called with pri %d > PZERO, wchan: %x\n",
		    pri, chan);
		panic("old sleep");
	}
#endif
	s = splhigh();
	if (cold || panicstr) {
		/*
		 * After a panic, or during autoconfiguration,
		 * just give interrupts a chance, then just return;
		 * don't run any other procs or panic below,
		 * in case this is the idle process and already asleep.
		 */
		splx(safepri);
		splx(s);
		return;
	}
#ifdef DIAGNOSTIC
	if (chan == NULL || p->p_stat != SRUN || p->p_rlink)
		panic("sleep");
#endif
	p->p_wchan = chan;
	p->p_wmesg = NULL;
	p->p_slptime = 0;
	p->p_pri = pri;
	qp = &slpque[HASH(chan)];
	if (qp->sq_head == 0)
		qp->sq_head = p;
	else
		*qp->sq_tailp = p;
	*(qp->sq_tailp = &p->p_link) = 0;
	p->p_stat = SSLEEP;
	p->p_stats->p_ru.ru_nvcsw++;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p->p_tracep, 1, 0);
#endif
	swtch();
#ifdef KTRACE
	if (KTRPOINT(p, KTR_CSW))
		ktrcsw(p->p_tracep, 0, 0);
#endif
	curpri = p->p_usrpri;
	splx(s);
}

/*
 * Remove a process from its wait queue
 */
void
unsleep(p)
	register struct proc *p;
{
	register struct slpque *qp;
	register struct proc **hp;
	int s;

	s = splhigh();
	if (p->p_wchan) {
		hp = &(qp = &slpque[HASH(p->p_wchan)])->sq_head;
		while (*hp != p)
			hp = &(*hp)->p_link;
		*hp = p->p_link;
		if (qp->sq_tailp == &p->p_link)
			qp->sq_tailp = hp;
		p->p_wchan = 0;
	}
	splx(s);
}

/*
 * Wakeup on "chan"; set all processes
 * sleeping on chan to run state.
 */
void
wakeup(chan)
	register void *chan;
{
	register struct slpque *qp;
	register struct proc *p, **q;
	int s;

	s = splhigh();
	qp = &slpque[HASH(chan)];
restart:
	for (q = &qp->sq_head; p = *q; ) {
#ifdef DIAGNOSTIC
		if (p->p_rlink || p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup");
#endif
		if (p->p_wchan == chan) {
			p->p_wchan = 0;
			*q = p->p_link;
			if (qp->sq_tailp == &p->p_link)
				qp->sq_tailp = q;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				if (p->p_slptime > 1)
					updatepri(p);
				p->p_slptime = 0;
				p->p_stat = SRUN;
				if (p->p_flag & SLOAD)
					setrq(p);
				/*
				 * Since curpri is a usrpri,
				 * p->p_pri is always better than curpri.
				 */
				if ((p->p_flag&SLOAD) == 0)
					wakeup((caddr_t)&proc0);
				else
					need_resched();
				/* END INLINE EXPANSION */
				goto restart;
			}
		} else
			q = &p->p_link;
	}
	splx(s);
}

/*
 * The machine independent parts of swtch().
 * Must be called at splstatclock() or higher.
 */
void
swtch()
{
	register struct proc *p = curproc;	/* XXX */
	register struct rlimit *rlim;
	register long s, u;
	struct timeval tv;

	/*
	 * Compute the amount of time during which the current
	 * process was running, and add that to its total so far.
	 */
	microtime(&tv);
	u = p->p_rtime.tv_usec + (tv.tv_usec - runtime.tv_usec);
	s = p->p_rtime.tv_sec + (tv.tv_sec - runtime.tv_sec);
	if (u < 0) {
		u += 1000000;
		s--;
	} else if (u >= 1000000) {
		u -= 1000000;
		s++;
	}
	p->p_rtime.tv_usec = u;
	p->p_rtime.tv_sec = s;

	/*
	 * Check if the process exceeds its cpu resource allocation.
	 * If over max, kill it.  In any case, if it has run for more
	 * than 10 minutes, reduce priority to give others a chance.
	 */
	rlim = &p->p_rlimit[RLIMIT_CPU];
	if (s >= rlim->rlim_cur) {
		if (s >= rlim->rlim_max)
			psignal(p, SIGKILL);
		else {
			psignal(p, SIGXCPU);
			if (rlim->rlim_cur < rlim->rlim_max)
				rlim->rlim_cur += 5;
		}
	}
	if (s > 10 * 60 && p->p_ucred->cr_uid && p->p_nice == NZERO) {
		p->p_nice = NZERO + 4;
		setpri(p);
	}

	/*
	 * Pick a new current process and record its start time.
	 */
	cnt.v_swtch++;
	cpu_swtch(p);
	microtime(&runtime);
}

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
rqinit()
{
	register int i;

	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
}

/*
 * Change process state to be runnable,
 * placing it on the run queue if it is in memory,
 * and awakening the swapper if it isn't in memory.
 */
void
setrun(p)
	register struct proc *p;
{
	register int s;

	s = splhigh();
	switch (p->p_stat) {

	case 0:
	case SWAIT:
	case SRUN:
	case SZOMB:
	default:
		panic("setrun");

	case SSTOP:
	case SSLEEP:
		unsleep(p);		/* e.g. when sending signals */
		break;

	case SIDL:
		break;
	}
	p->p_stat = SRUN;
	if (p->p_flag & SLOAD)
		setrq(p);
	splx(s);
	if (p->p_slptime > 1)
		updatepri(p);
	p->p_slptime = 0;
	if ((p->p_flag&SLOAD) == 0)
		wakeup((caddr_t)&proc0);
	else if (p->p_pri < curpri)
		need_resched();
}

/*
 * Compute priority of process when running in user mode.
 * Arrange to reschedule if the resulting priority
 * is better than that of the current process.
 */
void
setpri(p)
	register struct proc *p;
{
	register unsigned int newpri;

	newpri = PUSER + p->p_cpu / 4 + 2 * p->p_nice;
	newpri = min(newpri, MAXPRI);
	p->p_usrpri = newpri;
	if (newpri < curpri)
		need_resched();
}
