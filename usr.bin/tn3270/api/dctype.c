/*
 * Copyright (c) 1988 Regents of the University of California.
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
static char sccsid[] = "@(#)dctype.c	3.2 (Berkeley) 3/28/88";
#endif /* not lint */

#include "dctype.h"

unsigned char dctype[192] = {
/*00*/
	D_SPACE,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
/*10*/
	D_SPACE,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	0,
	0,
	0,
	0,
/*20*/
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	D_DIGIT|D_PRINT,
	0,
	0,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
/*30*/
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
	0,
	0,
	0,
	0,
	D_PUNCT|D_PRINT,
	0,
	D_PUNCT|D_PRINT,
	0,
	0,
/*40*/
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
/*50*/
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
/*60*/
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
/*70*/
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
/*80*/
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
/*90*/
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	D_LOWER|D_PRINT,
	0,
	0,
	0,
	0,
	0,
	0,
/*A0*/
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
/*B0*/
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	D_UPPER|D_PRINT,
	0,
	0,
	0,
	0,
	D_PUNCT|D_PRINT,
	D_PUNCT|D_PRINT,
};
