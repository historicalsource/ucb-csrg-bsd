/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)ectype.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include "ectype.h"

char	ectype[] = {
/* 0x00 */
    E_SPACE,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0x10 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0x20 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0x30 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0x40 */
    E_SPACE,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
/* 0x50 */
    E_PRINT|E_PUNCT,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
/* 0x60 */
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
/* 0x70 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
    E_PRINT|E_PUNCT,
/* 0x80 */
    0x00,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0x90 */
    0x00,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0xA0 */
    0x00,
    E_PRINT|E_PUNCT,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    E_PRINT|E_LOWER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0xB0 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0xC0 */
    E_PRINT|E_PUNCT,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0xD0 */
    E_PRINT|E_PUNCT,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0xE0 */
    E_PRINT|E_PUNCT,
    0x00,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    E_PRINT|E_UPPER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
/* 0xF0 */
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    E_PRINT|E_DIGIT,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};
