/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)machdep.c	6.16 (Berkeley) 6/8/85
 */

#include "reg.h"
#include "pte.h"
#include "psl.h"

#include "param.h"
#include "systm.h"
#include "dir.h"
#include "user.h"
#include "kernel.h"
#include "map.h"
#include "vm.h"
#include "proc.h"
#include "buf.h"
#include "reboot.h"
#include "conf.h"
#include "inode.h"
#include "file.h"
#include "text.h"
#include "clist.h"
#include "callout.h"
#include "cmap.h"
#include "mbuf.h"
#include "msgbuf.h"
#include "quota.h"

#include "frame.h"
#include "cons.h"
#include "cpu.h"
#include "mem.h"
#include "mtpr.h"
#include "rpb.h"
#include "../vaxuba/ubavar.h"
#include "../vaxuba/ubareg.h"

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif

/*
 * Machine-dependent startup code
 */
startup(firstaddr)
	int firstaddr;
{
	register int unixsize;
	register unsigned i;
	register struct pte *pte;
	int mapaddr, j;
	register caddr_t v;
	int maxbufs, base, residual;
	extern char etext;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	maxmem -= btoc(sizeof (struct msgbuf));
	pte = msgbufmap;
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		*(int *)pte++ = PG_V | PG_KW | (maxmem + i);
	mtpr(TBIA, 1);

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */
	v = (caddr_t)(0x80000000 | (firstaddr * NBPG));
#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(text, struct text, ntext, textNTEXT);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	valloc(argmap, struct map, ARGMAPSIZE);
	valloc(kernelmap, struct map, nproc);
	valloc(mbmap, struct map, nmbclusters/4);
	valloc(nch, struct nch, nchsize);
#ifdef QUOTA
	valloclim(quota, struct quota, nquota, quotaNQUOTA);
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif
	
	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory for the first 2 Meg, 5% of the remaining
	 * memory. Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		if (physmem < (2 * 1024 * 1024))
			bufpages = physmem / 10 / CLSIZE;
		else
			bufpages =
			    ((2 * 1024 * 1024) / 5 + physmem / 5) / CLSIZE;
	if (nbuf == 0) {
		nbuf = bufpages / 2;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);

	/*
	 * Now the amount of virtual memory remaining for buffers
	 * can be calculated, estimating needs for the cmap.
	 */
	ncmap = (maxmem*NBPG - ((int)v &~ 0x80000000)) /
		(CLBYTES + sizeof(struct cmap)) + 2;
	maxbufs = ((SYSPTSIZE * NBPG) -
	    ((int)(v + ncmap * sizeof(struct cmap)) - 0x80000000)) /
		(MAXBSIZE + sizeof(struct buf));
	if (maxbufs < 16)
		panic("sys pt too small");
	if (nbuf > maxbufs) {
		printf("SYSPTSIZE limits number of buffers to %d\n", maxbufs);
		nbuf = maxbufs;
	}
	if (bufpages > nbuf * (MAXBSIZE / CLBYTES))
		bufpages = nbuf * (MAXBSIZE / CLBYTES);
	valloc(buf, struct buf, nbuf);

	/*
	 * Allocate space for core map.
	 * Allow space for all of phsical memory minus the amount 
	 * dedicated to the system. The amount of physical memory
	 * dedicated to the system is the total virtual memory of
	 * the system thus far, plus core map, buffer pages,
	 * and buffer headers not yet allocated.
	 * Add 2: 1 because the 0th entry is unused, 1 for rounding.
	 */
	ncmap = (maxmem*NBPG - ((int)(v + bufpages*CLBYTES) &~ 0x80000000)) /
		(CLBYTES + sizeof(struct cmap)) + 2;
	valloclim(cmap, struct cmap, ncmap, ecmap);

	/*
	 * Clear space allocated thus far, and make r/w entries
	 * for the space in the kernel map.
	 */
	unixsize = btoc((int)v &~ 0x80000000);
	while (firstaddr < unixsize) {
		*(int *)(&Sysmap[firstaddr]) = PG_V | PG_KW | firstaddr;
		clearseg((unsigned)firstaddr);
		firstaddr++;
	}

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	v = (caddr_t) ((int)(v + PGOFSET) &~ PGOFSET);
	valloc(buffers, char, MAXBSIZE * nbuf);
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	mapaddr = firstaddr;
	for (i = 0; i < residual; i++) {
		for (j = 0; j < (base + 1) * CLSIZE; j++) {
			*(int *)(&Sysmap[mapaddr+j]) = PG_V | PG_KW | firstaddr;
			clearseg((unsigned)firstaddr);
			firstaddr++;
		}
		mapaddr += MAXBSIZE / NBPG;
	}
	for (i = residual; i < nbuf; i++) {
		for (j = 0; j < base * CLSIZE; j++) {
			*(int *)(&Sysmap[mapaddr+j]) = PG_V | PG_KW | firstaddr;
			clearseg((unsigned)firstaddr);
			firstaddr++;
		}
		mapaddr += MAXBSIZE / NBPG;
	}

	unixsize = btoc((int)v &~ 0x80000000);
	if (firstaddr >= physmem - 8*UPAGES)
		panic("no memory");
	mtpr(TBIA, 1);			/* After we just cleared it all! */

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 *
	 * THE USER PAGE TABLE MAP IS CALLED ``kernelmap''
	 * WHICH IS A VERY UNDESCRIPTIVE AND INCONSISTENT NAME.
	 */
	meminit(firstaddr, maxmem);
	maxmem = freemem;
	printf("avail mem = %d\n", ctob(maxmem));
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);
	rminit(kernelmap, (long)USRPTSIZE, (long)1,
	    "usrpt", nproc);
	rminit(mbmap, (long)((nmbclusters - 1) * CLSIZE), (long)CLSIZE,
	    "mbclusters", nmbclusters/4);

	/*
	 * Configure the system.
	 */
	configure();

	/*
	 * Clear restart inhibit flags.
	 */
	tocons(TXDB_CWSI);
	tocons(TXDB_CCSI);
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	if (mfpr(ICCS)&ICCS_INT)
		return(-1);
	else
		return(((time.tv_sec-otime)*60 + lbolt-olbolt)*16667 + mfpr(ICR)-oicr);
}
#endif

/*
 * Clear registers on exec
 */
setregs(entry)
	u_long entry;
{
#ifdef notdef
	register int *rp;

	/* should pass args to init on the stack */
	/* should also fix this code before using it, it's wrong */
	/* wanna clear the scb? */
	for (rp = &u.u_ar0[0]; rp < &u.u_ar0[16];)
		*rp++ = 0;
#endif
	u.u_ar0[PC] = entry + 2;
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by chmk
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, the frame 
 * pointer, and the argument pointer, it returns
 * to the user specified pc, psl.
 */
sendsig(p, sig, mask)
	int (*p)(), sig, mask;
{
	register struct sigcontext *scp;
	register int *regs;
	register struct sigframe {
		int	sf_signum;
		int	sf_code;
		struct	sigcontext *sf_scp;
		int	(*sf_handler)();
		int	sf_argcount;
		struct	sigcontext *sf_scpcopy;
	} *fp;
	int oonstack;

	regs = u.u_ar0;
	oonstack = u.u_onstack;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		scp = (struct sigcontext *)u.u_sigsp - 1;
		u.u_onstack = 1;
	} else
		scp = (struct sigcontext *)regs[SP] - 1;
	fp = (struct sigframe *)scp - 1;
	if ((int)fp <= USRSTACK - ctob(u.u_ssize)) 
		grow((unsigned)fp);
	if (useracc((caddr_t)fp, sizeof (*fp) + sizeof (*scp), 1) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		u.u_signal[SIGILL] = SIG_DFL;
		sig = sigmask(SIGILL);
		u.u_procp->p_sigignore &= ~sig;
		u.u_procp->p_sigcatch &= ~sig;
		u.u_procp->p_sigmask &= ~sig;
		psignal(u.u_procp, SIGILL);
		return;
	}
	/* 
	 * Build the argument list for the signal handler.
	 */
	fp->sf_signum = sig;
	if (sig == SIGILL || sig == SIGFPE) {
		fp->sf_code = u.u_code;
		u.u_code = 0;
	} else
		fp->sf_code = 0;
	fp->sf_scp = scp;
	fp->sf_handler = p;
	/*
	 * Build the calls argument frame to be used to call sigreturn
	 */
	fp->sf_argcount = 1;
	fp->sf_scpcopy = scp;
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	scp->sc_onstack = oonstack;
	scp->sc_mask = mask;
	scp->sc_sp = regs[SP];
	scp->sc_fp = regs[FP];
	scp->sc_ap = regs[AP];
	scp->sc_pc = regs[PC];
	scp->sc_ps = regs[PS];
	regs[SP] = (int)fp;
	regs[PS] &= ~(PSL_CM|PSL_FPD);
	regs[PC] = (int)u.u_pcb.pcb_sigc;
	return;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */
sigreturn()
{
	struct a {
		struct sigcontext *sigcntxp;
	};
	register struct sigcontext *scp;
	register int *regs = u.u_ar0;

	scp = ((struct a *)(u.u_ap))->sigcntxp;
	if (useracc((caddr_t)scp, sizeof (*scp), 0) == 0)
		return;
	if ((scp->sc_ps & (PSL_MBZ|PSL_IPL|PSL_IS)) != 0 ||
	    (scp->sc_ps & (PSL_PRVMOD|PSL_CURMOD)) != (PSL_PRVMOD|PSL_CURMOD) ||
	    ((scp->sc_ps & PSL_CM) &&
	     (scp->sc_ps & (PSL_FPD|PSL_DV|PSL_FU|PSL_IV)) != 0)) {
		u.u_error = EINVAL;
		return;
	}
	u.u_eosys = JUSTRETURN;
	u.u_onstack = scp->sc_onstack & 01;
	u.u_procp->p_sigmask = scp->sc_mask &~
	    (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));
	regs[FP] = scp->sc_fp;
	regs[AP] = scp->sc_ap;
	regs[SP] = scp->sc_sp;
	regs[PC] = scp->sc_pc;
	regs[PS] = scp->sc_ps;
}

/* XXX - BEGIN 4.2 COMPATIBILITY */
/*
 * Compatibility with 4.2 chmk $139 used by longjmp()
 */
osigcleanup()
{
	register struct sigcontext *scp;
	register int *regs = u.u_ar0;

	scp = (struct sigcontext *)fuword((caddr_t)regs[SP]);
	if ((int)scp == -1)
		return;
	if (useracc((caddr_t)scp, 3 * sizeof (int), 0) == 0)
		return;
	u.u_onstack = scp->sc_onstack & 01;
	u.u_procp->p_sigmask = scp->sc_mask &~
	    (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));
	regs[SP] = scp->sc_sp;
}
/* XXX - END 4.2 COMPATIBILITY */

#ifdef notdef
dorti()
{
	struct frame frame;
	register int sp;
	register int reg, mask;
	extern int ipcreg[];

	(void) copyin((caddr_t)u.u_ar0[FP], (caddr_t)&frame, sizeof (frame));
	sp = u.u_ar0[FP] + sizeof (frame);
	u.u_ar0[PC] = frame.fr_savpc;
	u.u_ar0[FP] = frame.fr_savfp;
	u.u_ar0[AP] = frame.fr_savap;
	mask = frame.fr_mask;
	for (reg = 0; reg <= 11; reg++) {
		if (mask&1) {
			u.u_ar0[ipcreg[reg]] = fuword((caddr_t)sp);
			sp += 4;
		}
		mask >>= 1;
	}
	sp += frame.fr_spa;
	u.u_ar0[PS] = (u.u_ar0[PS] & 0xffff0000) | frame.fr_psw;
	if (frame.fr_s)
		sp += 4 + 4 * (fuword((caddr_t)sp) & 0xff);
	/* phew, now the rei */
	u.u_ar0[PC] = fuword((caddr_t)sp);
	sp += 4;
	u.u_ar0[PS] = fuword((caddr_t)sp);
	sp += 4;
	u.u_ar0[PS] |= PSL_USERSET;
	u.u_ar0[PS] &= ~PSL_USERCLR;
	u.u_ar0[SP] = (int)sp;
}
#endif

/*
 * Memenable enables the memory controlle corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */
int	memintvl = MEMINTVL;

memenable()
{
	register struct mcr *mcr;
	register int m;

	for (m = 0; m < nmcr; m++) {
		mcr = mcraddr[m];
		switch (mcrtype[m]) {
#if VAX780
		case M780C:
			M780C_ENA(mcr);
			break;
		case M780EL:
			M780EL_ENA(mcr);
			break;
		case M780EU:
			M780EU_ENA(mcr);
			break;
#endif
#if VAX750
		case M750:
			M750_ENA(mcr);
			break;
#endif
#if VAX730
		case M730:
			M730_ENA(mcr);
			break;
#endif
		}
	}
	if (memintvl > 0)
		timeout(memenable, (caddr_t)0, memintvl*hz);
}

/*
 * Memerr is the interrupt routine for corrected read data
 * interrupts.  It looks to see which memory controllers have
 * unreported errors, reports them, and disables further
 * reporting for a time on those controller.
 */
memerr()
{
	register struct mcr *mcr;
	register int m;

	for (m = 0; m < nmcr; m++) {
		mcr = mcraddr[m];
		switch (mcrtype[m]) {
#if VAX780
		case M780C:
			if (M780C_ERR(mcr)) {
				printf("mcr%d: soft ecc addr %x syn %x\n",
				    m, M780C_ADDR(mcr), M780C_SYN(mcr));
#ifdef TRENDATA
				memlog(m, mcr);
#endif
				M780C_INH(mcr);
			}
			break;

		case M780EL:
			if (M780EL_ERR(mcr)) {
				printf("mcr%d: soft ecc addr %x syn %x\n",
				    m, M780EL_ADDR(mcr), M780EL_SYN(mcr));
				M780EL_INH(mcr);
			}
			break;

		case M780EU:
			if (M780EU_ERR(mcr)) {
				printf("mcr%d: soft ecc addr %x syn %x\n",
				    m, M780EU_ADDR(mcr), M780EU_SYN(mcr));
				M780EU_INH(mcr);
			}
			break;
#endif
#if VAX750
		case M750:
			if (M750_ERR(mcr)) {
				struct mcr amcr;
				amcr.mc_reg[0] = mcr->mc_reg[0];
				printf("mcr%d: %s",
				    m, (amcr.mc_reg[0] & M750_UNCORR) ?
				    "hard error" : "soft ecc");
				printf(" addr %x syn %x\n",
				    M750_ADDR(&amcr), M750_SYN(&amcr));
				M750_INH(mcr);
			}
			break;
#endif
#if VAX730
		case M730: {
			struct mcr amcr;

			/*
			 * Must be careful on the 730 not to use invalid
			 * instructions in I/O space, so make a copy;
			 */
			amcr.mc_reg[0] = mcr->mc_reg[0];
			amcr.mc_reg[1] = mcr->mc_reg[1];
			if (M730_ERR(&amcr)) {
				printf("mcr%d: %s",
				    m, (amcr.mc_reg[1] & M730_UNCORR) ?
				    "hard error" : "soft ecc");
				printf(" addr %x syn %x\n",
				    M730_ADDR(&amcr), M730_SYN(&amcr));
				M730_INH(mcr);
			}
			break;
		}
#endif
		}
	}
}

#ifdef TRENDATA
/*
 * Figure out what chip to replace on Trendata boards.
 * Assumes all your memory is Trendata or the non-Trendata
 * memory never fails..
 */
struct {
	u_char	m_syndrome;
	char	m_chip[4];
} memlogtab[] = {
	0x01,	"C00",	0x02,	"C01",	0x04,	"C02",	0x08,	"C03",
	0x10,	"C04",	0x19,	"L01",	0x1A,	"L02",	0x1C,	"L04",
	0x1F,	"L07",	0x20,	"C05",	0x38,	"L00",	0x3B,	"L03",
	0x3D,	"L05",	0x3E,	"L06",	0x40,	"C06",	0x49,	"L09",
	0x4A,	"L10",	0x4c,	"L12",	0x4F,	"L15",	0x51,	"L17",
	0x52,	"L18",	0x54,	"L20",	0x57,	"L23",	0x58,	"L24",
	0x5B,	"L27",	0x5D,	"L29",	0x5E,	"L30",	0x68,	"L08",
	0x6B,	"L11",	0x6D,	"L13",	0x6E,	"L14",	0x70,	"L16",
	0x73,	"L19",	0x75,	"L21",	0x76,	"L22",	0x79,	"L25",
	0x7A,	"L26",	0x7C,	"L28",	0x7F,	"L31",	0x80,	"C07",
	0x89,	"U01",	0x8A,	"U02",	0x8C,	"U04",	0x8F,	"U07",
	0x91,	"U09",	0x92,	"U10",	0x94,	"U12",	0x97, 	"U15",
	0x98,	"U16",	0x9B,	"U19",	0x9D,	"U21",	0x9E, 	"U22",
	0xA8,	"U00",	0xAB,	"U03",	0xAD,	"U05",	0xAE,	"U06",
	0xB0,	"U08",	0xB3,	"U11",	0xB5,	"U13",	0xB6,	"U14",
	0xB9,	"U17",	0xBA,	"U18",	0xBC,	"U20",	0xBF,	"U23",
	0xC1,	"U25",	0xC2,	"U26",	0xC4,	"U28",	0xC7,	"U31",
	0xE0,	"U24",	0xE3,	"U27",	0xE5,	"U29",	0xE6,	"U30"
};

memlog (m, mcr)
	int m;
	struct mcr *mcr;
{
	register i;

	switch (mcrtype[m]) {

#if VAX780
	case M780C:
	for (i = 0; i < (sizeof (memlogtab) / sizeof (memlogtab[0])); i++)
		if ((u_char)(M780C_SYN(mcr)) == memlogtab[i].m_syndrome) {
			printf (
	"mcr%d: replace %s chip in %s bank of memory board %d (0-15)\n",
				m,
				memlogtab[i].m_chip,
				(M780C_ADDR(mcr) & 0x8000) ? "upper" : "lower",
				(M780C_ADDR(mcr) >> 16));
			return;
		}
	printf ("mcr%d: multiple errors, not traceable\n", m);
	break;
#endif
	}
}
#endif

/*
 * Invalidate single all pte's in a cluster
 */
tbiscl(v)
	unsigned v;
{
	register caddr_t addr;		/* must be first reg var */
	register int i;

	asm(".set TBIS,58");
	addr = ptob(v);
	for (i = 0; i < CLSIZE; i++) {
#ifdef lint
		mtpr(TBIS, addr);
#else
		asm("mtpr r11,$TBIS");
#endif
		addr += NBPG;
	}
}
  
int	waittime = -1;

boot(paniced, arghowto)
	int paniced, arghowto;
{
	register int howto;		/* r11 == how to boot */
	register int devtype;		/* r10 == major of root dev */

#ifdef lint
	howto = 0; devtype = 0;
	printf("howto %d, devtype %d\n", arghowto, devtype);
#endif
	howto = arghowto;
	if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		(void) splnet();
		printf("syncing disks... ");
		update();
		{ register struct buf *bp;
		  int iter, nbusy;

		  for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			DELAY(40000 * iter);
		  }
		}
		printf("done\n");
	}
	splx(0x1f);			/* extreme priority */
	devtype = major(rootdev);
	if (howto&RB_HALT) {
		printf("halting (in tight loop); hit\n\t^P\n\tHALT\n\n");
		mtpr(IPL, 0x1f);
		for (;;)
			;
	} else {
		if (paniced == RB_PANIC) {
			doadump();		/* TXDB_BOOT's itsself */
			/*NOTREACHED*/
		}
		tocons(TXDB_BOOT);
	}
#if defined(VAX750) || defined(VAX730)
	if (cpu != VAX_780)
		{ asm("movl r11,r5"); }		/* boot flags go in r5 */
#endif
	for (;;)
		asm("halt");
	/*NOTREACHED*/
}

tocons(c)
{

	while ((mfpr(TXCS)&TXCS_RDY) == 0)
		continue;
	mtpr(TXDB, c);
}

int	dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
dumpsys()
{

	rpb.rp_flag = 1;
#ifdef notdef
	if ((minor(dumpdev)&07) != 1)
		return;
#endif
	dumpsize = physmem;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
	printf("dump ");
	switch ((*bdevsw[major(dumpdev)].d_dump)(dumpdev)) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error");
		break;

	default:
		printf("succeeded");
		break;
	}
}

/*
 * Machine check error recovery code.
 * Print out the machine check frame and then give up.
 */
#if defined(VAX780) || defined(VAX750)
char *mc780[] = {
	"cp read",	"ctrl str par",	"cp tbuf par",	"cp cache par",
	"cp rdtimo", 	"cp rds",	"ucode lost",	0,
	0,		0,		"ib tbuf par",	0,
	"ib rds",	"ib rd timo",	0,		"ib cache par"
};
#define MC750_TBERR	2		/* type code of cp tbuf par */
#define	MC750_TBPAR	4		/* tbuf par bit in mcesr */
#endif
#if VAX730
#define	NMC730	12
char *mc730[] = {
	"tb par",	"bad retry",	"bad intr id",	"cant write ptem",
	"unkn mcr err",	"iib rd err",	"nxm ref",	"cp rds",
	"unalgn ioref",	"nonlw ioref",	"bad ioaddr",	"unalgn ubaddr",
};
#endif

/*
 * Frame for each cpu
 */
struct mc780frame {
	int	mc8_bcnt;		/* byte count == 0x28 */
	int	mc8_summary;		/* summary parameter (as above) */
	int	mc8_cpues;		/* cpu error status */
	int	mc8_upc;		/* micro pc */
	int	mc8_vaviba;		/* va/viba register */
	int	mc8_dreg;		/* d register */
	int	mc8_tber0;		/* tbuf error reg 0 */
	int	mc8_tber1;		/* tbuf error reg 1 */
	int	mc8_timo;		/* timeout address divided by 4 */
	int	mc8_parity;		/* parity */
	int	mc8_sbier;		/* sbi error register */
	int	mc8_pc;			/* trapped pc */
	int	mc8_psl;		/* trapped psl */
};
struct mc750frame {
	int	mc5_bcnt;		/* byte count == 0x28 */
	int	mc5_summary;		/* summary parameter (as above) */
	int	mc5_va;			/* virtual address register */
	int	mc5_errpc;		/* error pc */
	int	mc5_mdr;
	int	mc5_svmode;		/* saved mode register */
	int	mc5_rdtimo;		/* read lock timeout */
	int	mc5_tbgpar;		/* tb group parity error register */
	int	mc5_cacherr;		/* cache error register */
	int	mc5_buserr;		/* bus error register */
	int	mc5_mcesr;		/* machine check status register */
	int	mc5_pc;			/* trapped pc */
	int	mc5_psl;		/* trapped psl */
};
struct mc730frame {
	int	mc3_bcnt;		/* byte count == 0xc */
	int	mc3_summary;		/* summary parameter */
	int	mc3_parm[2];		/* parameter 1 and 2 */
	int	mc3_pc;			/* trapped pc */
	int	mc3_psl;		/* trapped psl */
};

machinecheck(cmcf)
	caddr_t cmcf;
{
	register u_int type = ((struct mc780frame *)cmcf)->mc8_summary;

	printf("machine check %x: ", type);
	switch (cpu) {
#if VAX780
	case VAX_780:
#endif
#if VAX750
	case VAX_750:
#endif
#if defined(VAX780) || defined(VAX750)
		printf("%s%s\n", mc780[type&0xf],
		    (type&0xf0) ? " abort" : " fault"); 
		break;
#endif
#if VAX730
	case VAX_730:
		if (type < NMC730)
			printf("%s", mc730[type]);
		printf("\n");
		break;
#endif
	}
	switch (cpu) {
#if VAX780
	case VAX_780: {
		register struct mc780frame *mcf = (struct mc780frame *)cmcf;
		register int sbifs;
		printf("\tcpues %x upc %x va/viba %x dreg %x tber %x %x\n",
		   mcf->mc8_cpues, mcf->mc8_upc, mcf->mc8_vaviba,
		   mcf->mc8_dreg, mcf->mc8_tber0, mcf->mc8_tber1);
		sbifs = mfpr(SBIFS);
		printf("\ttimo %x parity %x sbier %x pc %x psl %x sbifs %x\n",
		   mcf->mc8_timo*4, mcf->mc8_parity, mcf->mc8_sbier,
		   mcf->mc8_pc, mcf->mc8_psl, sbifs);
		/* THE FUNNY BITS IN THE FOLLOWING ARE FROM THE ``BLACK */
		/* BOOK'' AND SHOULD BE PUT IN AN ``sbi.h'' */
		mtpr(SBIFS, sbifs &~ 0x2000000);
		mtpr(SBIER, mfpr(SBIER) | 0x70c0);
		break;
	}
#endif
#if VAX750
	case VAX_750: {
		register struct mc750frame *mcf = (struct mc750frame *)cmcf;
		int mcsr = mfpr(MCSR);

		mtpr(TBIA, 0);
		mtpr(MCESR, 0xf);
		printf("\tva %x errpc %x mdr %x smr %x rdtimo %x tbgpar %x cacherr %x\n",
		    mcf->mc5_va, mcf->mc5_errpc, mcf->mc5_mdr, mcf->mc5_svmode,
		    mcf->mc5_rdtimo, mcf->mc5_tbgpar, mcf->mc5_cacherr);
		printf("\tbuserr %x mcesr %x pc %x psl %x mcsr %x\n",
		    mcf->mc5_buserr, mcf->mc5_mcesr, mcf->mc5_pc, mcf->mc5_psl,
		    mcsr);
		if (type == MC750_TBERR && (mcf->mc5_mcesr&0xe) == MC750_TBPAR){
			printf("tbuf par: flushing and returning\n");
			return;
		}
		break;
		}
#endif
#if VAX730
	case VAX_730: {
		register struct mc730frame *mcf = (struct mc730frame *)cmcf;
		printf("params %x,%x pc %x psl %x mcesr %x\n",
		    mcf->mc3_parm[0], mcf->mc3_parm[1],
		    mcf->mc3_pc, mcf->mc3_psl, mfpr(MCESR));
		mtpr(MCESR, 0xf);
		break;
		}
#endif
	}
	memerr();
	panic("mchk");
}

#ifdef notdef
microtime(tvp)
	struct timeval *tvp;
{
	int s = spl7();

	tvp->tv_sec = time.tv_sec;
	tvp->tv_usec = (lbolt+1)*16667 + mfpr(ICR);
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	splx(s);
}
#endif

physstrat(bp, strat, prio)
	struct buf *bp;
	int (*strat)(), prio;
{
	int s;

	(*strat)(bp);
	/* pageout daemon doesn't wait for pushed pages */
	if (bp->b_flags & B_DIRTY)
		return;
	s = spl6();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, prio);
	splx(s);
}
