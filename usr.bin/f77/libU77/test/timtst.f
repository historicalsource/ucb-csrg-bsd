C
C Copyright (c) 1980 The Regents of the University of California.
C All rights reserved.
C
C %sccs.include.proprietary.f%
C
C	@(#)timtst.f	5.2 (Berkeley) 4/12/91
C

	character*24 ctime, fdate
	integer time, tl(9), tgm(9)
	write(*,*) ctime(time()), fdate()
	call ltime(time(), tl)
	call gmtime(time(), tgm)
	write(*,*) tl, tgm
	end
