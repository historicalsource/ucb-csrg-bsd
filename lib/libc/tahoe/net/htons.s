#ifdef LIBC_SCCS
	.asciz	"@(#)htons.s	1.2 (Berkeley/CCI) 8/1/86"
#endif LIBC_SCCS

/* hostorder = htons(netorder) */

#include "DEFS.h"

ENTRY(htons, 0)
	movzwl	6(fp),r0
	ret
