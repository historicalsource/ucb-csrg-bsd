# include	"curses.ext"

/*
 *	This routine gets a string starting at (_cury,_curx)
 *
 * 4/10/81 (Berkeley) @(#)getstr.c	1.2
 */
wgetstr(win,str)
reg WINDOW	*win; 
reg char	*str; {

	while ((*str = wgetch(win)) != ERR && *str != '\n');
		str++;
	if (*str == ERR) {
		return ERR;
		*str = '\0';
	}
	*str = '\0';
	return OK;
}
