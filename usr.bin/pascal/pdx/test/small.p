(*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)small.p	8.1 (Berkeley) 6/6/93
 *)

program small(input, output);
begin
	writeln('this is small', 1 div 0);
end.
