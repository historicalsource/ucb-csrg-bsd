/*	param.h	1.8	87/05/12	*/

/*
 * Machine dependent constants for TAHOE.
 */

#ifndef ENDIAN
/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE	1234		/* least-significant byte first (vax) */
#define	BIG	4321		/* most-significant byte first */
#define	PDP	3412		/* LSB first in word, MSW first in long (pdp) */
#define	ENDIAN	BIG		/* byte order on tahoe */

/*
 * Macros for network/external number representation conversion.
 */
#if ENDIAN == BIG && !defined(lint)
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)
#else
unsigned short	ntohs(), htons();
unsigned long	ntohl(), htonl();
#endif

#define	NBPG		1024		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		10		/* LOG2(NBPG) */
#define	NPTEPG		(NBPG/(sizeof (struct pte)))

#define	KERNBASE	0xc0000000	/* start of kernel virtual */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define	DEV_BSIZE	1024
#define	DEV_BSHIFT	10		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	1024		/* NBPG for physical controllers */
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define	CLSIZE		1
#define	CLSIZELOG2	0

#define	SSIZE		2		/* initial stack size/NBPG */
#define	SINCR		2		/* increment of stack/NBPG */
#define	UPAGES		6		/* pages of u-area (2 stack pages) */

#define	MAXCKEY	255		/* maximal allowed code key */
#define	MAXDKEY	255		/* maximal allowed data key */
#define	NCKEY	(MAXCKEY+1)	/* # code keys, including 0 (reserved) */
#define	NDKEY	(MAXDKEY+1)	/* # data keys, including 0 (reserved) */

/*
 * Some macros for units conversion
 */
/* Core clicks (1024 bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (1024 bytes) to disk blocks */
#define	ctod(x)	(x)
#define	dtoc(x)	(x)
#define	dtob(x)	((x)<<PGSHIFT)

/* clicks to bytes */
#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+NBPG-1) >> PGSHIFT))

#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be if we
 * add an entry to cdevsw/bdevsw for that purpose.
 * For now though just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & PSL_CURMOD) == PSL_CURMOD)
#define	BASEPRI(ps)	(((ps) & PSL_IPL) == 0)

#define	DELAY(n)	{ register int N = 3*(n); while (--N > 0); }
#endif
