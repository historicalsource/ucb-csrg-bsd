/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)addbytes.c	5.2 (Berkeley) 6/8/88";
#endif /* not lint */

# include	"curses.ext"

/*
 *	This routine adds the character to the current position
 *
 */
waddbytes(win, bytes, count)
reg WINDOW	*win;
reg char	*bytes;
reg int		count;
{
#define	SYNCH_OUT()	{win->_cury = y; win->_curx = x;}
#define	SYNCH_IN()	{y = win->_cury; x = win->_curx;}
	reg int		x, y;
	reg int		newx;

	SYNCH_IN();
# ifdef FULLDEBUG
	fprintf(outf, "ADDBYTES('%c') at (%d, %d)\n", c, y, x);
# endif
	while (count--) {
	    register int c;
	    static char blanks[] = "        ";

	    c = *bytes++;
	    switch (c) {
	      case '\t':
		    SYNCH_IN();
		    if (waddbytes(win, blanks, 8-(x%8)) == ERR) {
			return ERR;
		    }
		    SYNCH_OUT();
		    break;

	      default:
# ifdef FULLDEBUG
		    fprintf(outf, "ADDBYTES: 1: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, win->_firstch[y], win->_lastch[y]);
# endif
		    if (win->_flags & _STANDOUT)
			    c |= _STANDOUT;
		    {
# ifdef	FULLDEBUG
			    fprintf(outf, "ADDBYTES(%0.2o, %d, %d)\n", win, y, x);
# endif
			    if (win->_y[y][x] != c) {
				    newx = x + win->_ch_off;
				    if (win->_firstch[y] == _NOCHANGE) {
					    win->_firstch[y] =
							    win->_lastch[y] = newx;
				    } else if (newx < win->_firstch[y])
					    win->_firstch[y] = newx;
				    else if (newx > win->_lastch[y])
					    win->_lastch[y] = newx;
# ifdef FULLDEBUG
				    fprintf(outf, "ADDBYTES: change gives f/l: %d/%d [%d/%d]\n",
					    win->_firstch[y], win->_lastch[y],
					    win->_firstch[y] - win->_ch_off,
					    win->_lastch[y] - win->_ch_off);
# endif
			    }
		    }
		    win->_y[y][x++] = c;
		    if (x >= win->_maxx) {
			    x = 0;
    newline:
			    if (++y >= win->_maxy)
				    if (win->_scroll) {
					    SYNCH_OUT();
					    scroll(win);
					    SYNCH_IN();
					    --y;
				    }
				    else
					    return ERR;
		    }
# ifdef FULLDEBUG
		    fprintf(outf, "ADDBYTES: 2: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, win->_firstch[y], win->_lastch[y]);
# endif
		    break;
	      case '\n':
		    SYNCH_OUT();
		    wclrtoeol(win);
		    SYNCH_IN();
		    if (!NONL)
			    x = 0;
		    goto newline;
	      case '\r':
		    x = 0;
		    break;
	      case '\b':
		    if (--x < 0)
			    x = 0;
		    break;
	    }
    }
    SYNCH_OUT();
    return OK;
}
