/* @(#)remque.s	4.2 11/1/84 */
/* remque(entry) */

#include "DEFS.h"

ENTRY(remque, 0)
	remque	*4(ap),r0
	ret
