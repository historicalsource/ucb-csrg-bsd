/* Copyright (c) 1979 Regents of the University of California */

static char sccsid[] = "@(#)CHR.c 1.2 3/7/81";

#include "h01errs.h"

char
CHR(value)

	long	value;
{
	if (value < 0 || value > 127) {
		ERROR(ECHR, value);
		return;
	}
	return (char)value;
}
