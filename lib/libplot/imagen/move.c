/*-
 * Copyright (c) 1985 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)move.c	5.2 (Berkeley) 4/22/91";
#endif /* not lint */

#include "imp.h"
move(xi,yi){
        imPx = xi;
        imPy = yi;
}
