/*-
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)bdef.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#define xxtop	100		/* max size of xxstack */
int xxindent, xxval, newflag, xxmaxchars, xxbpertab;
int xxlineno;		/* # of lines already output */
int xxstind, xxstack[xxtop], xxlablast, xxt;
