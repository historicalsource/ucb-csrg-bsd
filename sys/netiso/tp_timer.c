/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)tp_timer.c	7.10 (Berkeley) 9/26/91
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/* 
 * ARGO TP
 *
 * $Header: tp_timer.c,v 5.2 88/11/18 17:29:07 nhall Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_timer.c,v $
 *
 * Contains all the timer code.  
 * There are two sources of calls to these routines:
 * the clock, and tp.trans. (ok, and tp_pcb.c calls it at init time)
 *
 * Timers come in two flavors - those that generally get
 * cancelled (tp_ctimeout, tp_cuntimeout)
 * and those that either usually expire (tp_etimeout, 
 * tp_euntimeout, tp_slowtimo) or may require more than one instance
 * of the timer active at a time.
 *
 * The C timers are stored in the tp_ref structure. Their "going off"
 * is manifested by a driver event of the TM_xxx form.
 *
 * The E timers are handled like the generic kernel callouts.
 * Their "going off" is manifested by a function call w/ 3 arguments.
 */

#include "param.h"
#include "types.h"
#include "time.h"
#include "malloc.h"
#include "socket.h"

#include "tp_param.h"
#include "tp_timer.h"
#include "tp_stat.h"
#include "tp_pcb.h"
#include "tp_tpdu.h"
#include "argo_debug.h"
#include "tp_trace.h"
#include "tp_seq.h"

struct	tp_ref *tp_ref;
int		N_TPREF = 127;
struct	tp_refinfo tp_refinfo;
struct	tp_pcb *tp_ftimeolist = (struct tp_pcb *)&tp_ftimeolist;

/*
 * CALLED FROM:
 *  at autoconfig time from tp_init() 
 * 	a combo of event, state, predicate
 * FUNCTION and ARGUMENTS:
 *  initialize data structures for the timers
 */
void
tp_timerinit()
{
	register int s;
#define GETME(x, t, n) {s = (n)*sizeof(*x); x = (t) malloc(s, M_PCB, M_NOWAIT);\
if (x == 0) panic("tp_timerinit"); bzero((caddr_t)x, s);}
	/*
	 * Initialize storage
	 */
	tp_refinfo.tpr_size = N_TPREF + 1;  /* Need to start somewhere */
	GETME(tp_ref, struct tp_ref *, tp_refinfo.tpr_size);
	tp_refinfo.tpr_base = tp_ref;
#undef GETME
}

/**********************  e timers *************************/

/*
 * CALLED FROM:
 *  tp.trans all over
 * FUNCTION and ARGUMENTS:
 * Set an E type timer.
 */
void
tp_etimeout(tpcb, fun, ticks)
	register struct tp_pcb	*tpcb;
	int 					fun; 	/* function to be called */
	int						ticks;
{

	register u_int *callp;
	IFDEBUG(D_TIMER)
		printf("etimeout pcb 0x%x state 0x%x\n", tpcb, tpcb->tp_state);
	ENDDEBUG
	IFTRACE(D_TIMER)
		tptrace(TPPTmisc, "tp_etimeout ref refstate tks Etick", tpcb->tp_lref,
		tpcb->tp_state, ticks, tp_stat.ts_Eticks);
	ENDTRACE
	if (tpcb == 0)
		return;
	IncStat(ts_Eset);
	if (ticks == 0)
		ticks = 1;
	callp = tpcb->tp_timer + fun;
	if (*callp == 0 || *callp > ticks)
		*callp = ticks;
}

/*
 * CALLED FROM:
 *  tp.trans all over
 * FUNCTION and ARGUMENTS:
 *  Cancel all occurrences of E-timer function (fun) for reference (refp)
 */
void
tp_euntimeout(tpcb, fun)
	register struct tp_pcb	*tpcb;
	int			  fun;
{
	IFTRACE(D_TIMER)
		tptrace(TPPTmisc, "tp_euntimeout ref", tpcb->tp_lref, 0, 0, 0);
	ENDTRACE

	if (tpcb)
		tpcb->tp_timer[fun] = 0;
}

/****************  c timers **********************
 *
 * These are not chained together; they sit
 * in the tp_ref structure. they are the kind that
 * are typically cancelled so it's faster not to
 * mess with the chains
 */

/*
 * CALLED FROM:
 *  the clock, every 500 ms
 * FUNCTION and ARGUMENTS:
 *  Look for open references with active timers.
 *  If they exist, call the appropriate timer routines to update
 *  the timers and possibly generate events.
 *  (The E timers are done in other procedures; the C timers are
 *  updated here, and events for them are generated here.)
 */
ProtoHook
tp_slowtimo()
{
	register u_int 	*cp, *cpbase;
	register struct tp_ref		*rp;
	struct tp_pcb		*tpcb;
	struct tp_event		E;
	int 				s = splnet(), t;

	/* check only open reference structures */
	IncStat(ts_Cticks);
	/* tp_ref[0] is never used */
	for (rp = tp_ref + tp_refinfo.tpr_maxopen; rp > tp_ref; rp--) {
		if ((tpcb = rp->tpr_pcb) == 0 || rp->tpr_state < REF_OPEN) 
			continue;
		cpbase = tpcb->tp_timer;
		t = TM_NTIMERS;
		/* check the timers */
		for (cp = cpbase + t; (--t, --cp) >= cpbase; ) {
			if (*cp && --(*cp) <= 0 ) {
				*cp = 0;
				E.ev_number = t;
				IFDEBUG(D_TIMER)
					printf("C expired! type 0x%x\n", t);
				ENDDEBUG
				IncStat(ts_Cexpired);
				tp_driver( rp->tpr_pcb, &E);
				if (t == TM_reference && tpcb->tp_state == TP_CLOSED) {
					if (tpcb->tp_notdetached) {
						IFDEBUG(D_CONN)
							printf("PRU_DETACH: not detached\n");
						ENDDEBUG
						tp_detach(tpcb);
					}
					/* XXX wart; where else to do it? */
					free((caddr_t)tpcb, M_PCB);
				}
			}
		}
	}
	splx(s);
	return 0;
}

/*
 * Called From: tp.trans from tp_slowtimo() -- retransmission timer went off.
 */
tp_data_retrans(tpcb)
register struct tp_pcb *tpcb;
{
	int rexmt, win;
	tpcb->tp_rttemit = 0;	/* cancel current round trip time */
	tpcb->tp_dupacks = 0;
	tpcb->tp_sndnxt = tpcb->tp_snduna;
	if (tpcb->tp_fcredit == 0) {
		/*
		 * We transmitted new data, started timing it and the window
		 * got shrunk under us.  This can only happen if all data
		 * that they wanted us to send got acked, so don't
		 * bother shrinking the congestion windows, et. al.
		 * The retransmission timer should have been reset in goodack()
		 */
		tpcb->tp_rxtshift = 0;
		tpcb->tp_timer[TM_data_retrans] = 0;
		tpcb->tp_timer[TM_sendack] = tpcb->tp_dt_ticks;
	}
	rexmt = tpcb->tp_dt_ticks << min(tpcb->tp_rxtshift, TP_MAXRXTSHIFT);
	win = min(tpcb->tp_fcredit, (tpcb->tp_cong_win / tpcb->tp_l_tpdusize / 2));
	win = max(win, 2);
	tpcb->tp_cong_win = tpcb->tp_l_tpdusize;	/* slow start again. */
	tpcb->tp_ssthresh = win * tpcb->tp_l_tpdusize;
	/* We're losing; our srtt estimate is probably bogus.
	 * Clobber it so we'll take the next rtt measurement as our srtt;
	 * Maintain current rxt times until then.
	 */
	if (++tpcb->tp_rxtshift > TP_NRETRANS / 4) {
		/* tpcb->tp_nlprotosw->nlp_losing(tpcb->tp_npcb) someday */
		tpcb->tp_rtt = 0;
	}
	if (rexmt > 128)
		rexmt = 128; /* XXXX value from tcp_timer.h */
	tpcb->tp_timer[TM_data_retrans] = tpcb->tp_rxtcur = rexmt;
	tp_send(tpcb);
}

int
tp_fasttimo()
{
	register struct tp_pcb *t;
	int s = splnet();
	struct tp_event		E;

	E.ev_number = TM_sendack;
	while ((t = tp_ftimeolist) != (struct tp_pcb *)&tp_ftimeolist) {
		if (t == 0) {
			printf("tp_fasttimeo: should panic");
			tp_ftimeolist = (struct tp_pcb *)&tp_ftimeolist;
		} else {
			if (t->tp_flags & TPF_DELACK) {
				t->tp_flags &= ~TPF_DELACK;
				IncStat(ts_Fdelack);
				tp_driver(t, &E);
				t->tp_timer[TM_sendack] = t->tp_keepalive_ticks;
			} else
				IncStat(ts_Fpruned);
			tp_ftimeolist = t->tp_fasttimeo;
			t->tp_fasttimeo = 0;
		}
	}
	splx(s);
}

/*
 * CALLED FROM:
 *  tp.trans, tp_emit()
 * FUNCTION and ARGUMENTS:
 * 	Set a C type timer of type (which) to go off after (ticks) time.
 */
void
tp_ctimeout(tpcb, which, ticks)
	register struct tp_pcb	*tpcb;
	int 					which, ticks; 
{

	IFTRACE(D_TIMER)
		tptrace(TPPTmisc, "tp_ctimeout ref which tpcb active", 
			tpcb->tp_lref, which, tpcb, tpcb->tp_timer[which]);
	ENDTRACE
	if(tpcb->tp_timer[which])
		IncStat(ts_Ccan_act);
	IncStat(ts_Cset);
	if (ticks <= 0)
		ticks = 1;
	tpcb->tp_timer[which] = ticks;
}

/*
 * CALLED FROM:
 *  tp.trans 
 * FUNCTION and ARGUMENTS:
 * 	Version of tp_ctimeout that resets the C-type time if the 
 * 	parameter (ticks) is > the current value of the timer.
 */
void
tp_ctimeout_MIN(tpcb, which, ticks)
	register struct tp_pcb	*tpcb;
	int						which, ticks; 
{
	IFTRACE(D_TIMER)
		tptrace(TPPTmisc, "tp_ctimeout_MIN ref which tpcb active", 
			tpcb->tp_lref, which, tpcb, tpcb->tp_timer[which]);
	ENDTRACE
	IncStat(ts_Cset);
	if (tpcb->tp_timer[which])  {
		tpcb->tp_timer[which] = MIN(ticks, tpcb->tp_timer[which]);
		IncStat(ts_Ccan_act);
	} else
		tpcb->tp_timer[which] = ticks;
}

/*
 * CALLED FROM:
 *  tp.trans
 * FUNCTION and ARGUMENTS:
 *  Cancel the (which) timer in the ref structure indicated by (refp).
 */
void
tp_cuntimeout(tpcb, which)
	register struct tp_pcb	*tpcb;
	int						which;
{
	IFDEBUG(D_TIMER)
		printf("tp_cuntimeout(0x%x, %d) active %d\n",
				tpcb, which, tpcb->tp_timer[which]);
	ENDDEBUG

	IFTRACE(D_TIMER)
		tptrace(TPPTmisc, "tp_cuntimeout ref which, active", refp-tp_ref, 
			which, tpcb->tp_timer[which], 0);
	ENDTRACE

	if (tpcb->tp_timer[which])
		IncStat(ts_Ccan_act);
	else
		IncStat(ts_Ccan_inact);
	tpcb->tp_timer[which] = 0;
}
