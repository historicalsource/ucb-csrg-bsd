/* @(#)data.c	4.3 (Berkeley) 5/15/84 */
#include <stdio.h>
#include <sys/param.h>

struct	_iobuf	_iob[_NFILE] ={
	{ 0, NULL, NULL, NULL, _IOREAD, 0},
	{ 0, NULL, NULL, NULL, _IOWRT, 1},
	{ 0, NULL, NULL, NULL, _IOWRT+_IONBF, 2},
};
/*
 * Ptr to end of buffers
 */
struct	_iobuf	*_lastbuf ={ &_iob[_NFILE] };
