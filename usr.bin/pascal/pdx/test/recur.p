(*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)recur.p	8.1 (Berkeley) 6/6/93
 *)

program recursion(input, output);
var	i : integer;

function fact(n : integer) : integer;
begin
	if n <= 1 then begin
		fact := 1;
	end else begin
		fact := n * fact(n-1);
	end;
end;

begin
	i := 3;
	writeln(i:1, '! = ', fact(i):1);
end.
