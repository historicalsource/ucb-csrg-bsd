static	char *sccsid = "@(#)gmon.c	1.2 (Berkeley) 10/17/81";

#include <stdio.h>

#include "monitor.h"

    /*	froms is actually a bunch of unsigned shorts indexing tos */
static unsigned short	*froms;
static struct tostruct	*tos = 0;
static unsigned short	tolimit = 0;
static char		*s_lowpc = 0;
static char		*s_highpc = 0;

static	int	ssiz;
static	int	*sbuf;

char	*sbrk();
#define	MSG "No space for monitor buffer(s)\n"

	/*ARGSUSED*/
exit(code)
	register int	code;
{
	fflush(stdout);
	_mcleanup();
	_cleanup();
#ifdef lint
	code = code;
#endif lint
	asm("	movl r11, r0");
	asm("	chmk $1");
}

_mstartup(lowpc, highpc)
	char	*lowpc;
	char	*highpc;
{
	int	monsize;
	char	*buffer;
	int	textsize;

	s_lowpc = lowpc;
	s_highpc = highpc;
	textsize = ((char *)highpc - (char *)lowpc);
	monsize = textsize + sizeof(struct phdr);
	buffer = sbrk(monsize);
	if ( buffer == (char *) -1 ) {
	    write(2, MSG, sizeof(MSG));
	    return;
	}
	froms = (unsigned short *) sbrk(textsize);
	if ( froms == (unsigned short *) -1 ) {
	    write(2 , MSG , sizeof(MSG) );
	    froms = 0;
	    return;
	}
	tos = (struct tostruct *) sbrk(textsize);
	if ( tos == (struct tostruct *) -1 ) {
	    write(2 , MSG , sizeof(MSG) );
	    froms = 0;
	    tos = 0;
	    return;
	}
	tolimit = textsize / sizeof(struct tostruct);
	tos[0].link = 0;
	monitor(lowpc, highpc, buffer, monsize);
}

_peek()
{
}

_mcleanup()
{
    FILE	*fd;
    int		fromindex;
    char	*frompc;
    int		toindex;
    int		textsize;

    monitor((int (*)())0);
    fd = fopen( "dmon.out" , "w" );
    if ( fd == NULL ) {
	perror( "mcount: dmon.out" );
	return;
    }
    fwrite( sbuf , ssiz , 1 , fd );
    textsize = s_highpc - s_lowpc;
    for ( fromindex = 0 ; fromindex < textsize>>1 ; fromindex++ ) {
	if ( froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = s_lowpc + (fromindex<<1);
	for (toindex=froms[fromindex]; toindex!=0; toindex=tos[toindex].link) {
#	    ifdef DEBUG
		printf( "[mcleanup] frompc %d selfpc %d count %d\n" ,
			frompc , tos[toindex].selfpc , tos[toindex].count );
#	    endif DEBUG
	    fwrite( &frompc, 1, sizeof frompc, fd );
	    fwrite( &tos[toindex].selfpc, 1, sizeof tos[toindex].selfpc, fd );
	    fwrite( &tos[toindex].count, 1, sizeof tos[toindex].count, fd );
	}
    }
    fclose( fd );
}

/*
 *	This routine is massaged so that it may be jsb'ed to
 */
asm("#define _mcount mcount");
mcount()
{
    register char		*selfpc;	/* r11 */
    register unsigned short	*frompcindex;	/* r10 */
    register struct tostruct	*top;		/* r9 */
    static int			profiling = 0;

    asm( "	forgot to run ex script on monitor.s" );
    asm( "#define r11 r5" );
    asm( "#define r10 r4" );
    asm( "#define r9 r3" );
#ifdef lint
    selfpc = (char *) 0;
    frompcindex = 0;
#else not lint
	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
    asm("	movl (sp), r11");	/* selfpc = ... (jsb frame) */
    asm("	movl 16(fp), r10");	/* frompcindex =     (calls frame) */
#endif not lint
	/*
	 *	check that we are profiling
	 */
    if ( profiling || tos == 0 ) {
	goto out;
    }
    profiling = 1;
	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
    if ( (char *) frompcindex < s_lowpc || (char *) frompcindex > s_highpc ) {
	goto done;
    }
    frompcindex = &froms[ ( (long) frompcindex - (long) s_lowpc ) >> 1 ];
    if ( *frompcindex == 0 ) {
	*frompcindex = ++tos[0].link;
	if ( *frompcindex >= tolimit ) {
	    goto overflow;
	}
	top = &tos[ *frompcindex ];
	top->selfpc = selfpc;
	top->count = 0;
	top->link = 0;
    } else {
	top = &tos[ *frompcindex ];
    }
    for ( ; /* goto done */ ; top = &tos[ top -> link ] ) {
	if ( top -> selfpc == selfpc ) {
	    top -> count++;
	    goto done;
	}
	if ( top -> link == 0 ) {
	    top -> link = ++tos[0].link;
	    if ( top -> link >= tolimit )
		goto overflow;
	    top = &tos[ top -> link ];
	    top -> selfpc = selfpc;
	    top -> count = 1;
	    top -> link = 0;
	    goto done;
	}
    }
done:
    profiling = 0;
    /* and fall through */
out:
    asm( "	rsb" );
    asm( "#undef r11" );
    asm( "#undef r10" );
    asm( "#undef r9" );
    asm( "#undef _mcount");

overflow:
#   define	TOLIMIT	"mcount: tos overflow\n"
    write( 2 , TOLIMIT , sizeof( TOLIMIT ) );
    tos = 0;
    froms = 0;
    goto out;
}

monitor(lowpc, highpc, buf, bufsiz)
	char	*lowpc;
	/*VARARGS1*/
	char	*highpc;
	int *buf, bufsiz;
{
	register o;

	if (lowpc == 0) {
		profil((char *)0, 0, 0, 0);
		return;
	}
	sbuf = buf;
	ssiz = bufsiz;
	((struct phdr *)buf)->lpc = lowpc;
	((struct phdr *)buf)->hpc = highpc;
	((struct phdr *)buf)->ncnt = ssiz;
	o = sizeof(struct phdr);
	buf = (int *) (((int)buf) + o);
	bufsiz -= o;
	if (bufsiz<=0)
		return;
	o = (((char *)highpc - (char *)lowpc)>>1);
	if(bufsiz < o)
		o = ((float) bufsiz / o) * 32768;
	else
		o = 0177777;
	profil(buf, bufsiz, lowpc, o);
}

