C
C Copyright (c) 1980 The Regents of the University of California.
C All rights reserved.
C
C %sccs.include.proprietary.f%
C
C	@(#)ttnam.f	5.2 (Berkeley) 4/12/91
C

	program ttnam
	character*19 ttynam
	write(*,*) ttynam(6), ":", ttynam(1), ":"
	end
