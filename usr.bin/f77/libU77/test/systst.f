C
C Copyright (c) 1980 The Regents of the University of California.
C All rights reserved.
C
C %sccs.include.proprietary.f%
C
C	@(#)systst.f	5.2 (Berkeley) 4/12/91
C

	program systst
	integer system
	write(*,*) "date", system("date")
	write(*,*) "rm xxx", system("rm xxx")
	end
