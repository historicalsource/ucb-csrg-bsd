/*
 * Copyright (c) 1983 Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)defs.h	3.19 (Berkeley) 8/4/88
 */

#include "ww.h"
#include <sys/time.h>

#define NWINDOW 9

struct timeval starttime;

struct ww *window[NWINDOW];	/* the windows */
struct ww *selwin;		/* the selected window */
struct ww *lastselwin;		/* the last selected window */
struct ww *cmdwin;		/* the command window */
struct ww *framewin;		/* the window for framing */
struct ww *boxwin;		/* the window for the box */
struct ww *fgwin;		/* the last foreground window */

#define isfg(w)		((w)->ww_order <= fgwin->ww_order)

char *default_shell[128];	/* default shell argv */
char *default_shellfile;	/* default shell program */
int default_nline;		/* default buffer size for new windows */
int default_smooth;		/* default "smooth" parameter */
char escapec;			/* the escape character */

	/* flags */
char quit;			/* quit command issued */
char terse;			/* terse mode */
char debug;			/* debug mode */
char incmd;			/* in command mode */

struct ww *getwin();
struct ww *openwin();
struct ww *vtowin();
struct ww *openiwin();
