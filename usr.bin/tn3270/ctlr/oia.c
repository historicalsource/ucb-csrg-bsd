/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)oia.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * Routines to maintain the Operator Information Area.
 */

#include "../general/general.h"

#include "oia.h"
#include "../general/globals.h"


init_oia()
{
    ClearElement(OperatorInformationArea);
}
