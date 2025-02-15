(*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)varnt.p	8.1 (Berkeley) 6/6/93
 *)

program varnt(input, output);
type
	vrec = record
		rfield : integer;
		case integer of
		1: (
			vfield : integer;
		);
		2: (
			vfield2 : real;
		);
	end;

var
	r : vrec;
begin
	r.vfield := 1;
	r.vfield2 := 2.5;
	r.rfield := 0;
	writeln('done');
end.
