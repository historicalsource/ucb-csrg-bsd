C
C Copyright (c) 1980 The Regents of the University of California.
C All rights reserved.
C
C %sccs.include.proprietary.f%
C
C	@(#)idtst.f	5.2 (Berkeley) 4/12/91
C

	character*8 getlog
	integer getuid, getgid, getpid
	write(*,*) "I am", getlog(), "uid:", getuid(), "gid:", getgid()
	write(*,*) "This is process", getpid()
	end
