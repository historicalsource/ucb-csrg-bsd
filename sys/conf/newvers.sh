#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)newvers.sh	1.7 (Berkeley) 1/9/86
#
if [ ! -r version ]
then
	/bin/echo 0 > version
fi
touch version
v=`cat version` u=${USER-root} d=`pwd` h=`hostname` t=`date`
( /bin/echo "char sccs[] = \"@(#)4.3 BSD #${v}: ${t} (${u}@${h}:${d})\\n\";" ;
  /bin/echo "char version[] = \"4.3 BSD UNIX #${v}: ${t}\\n    ${u}@${h}:${d}\\n\";"
) > vers.c
/bin/echo `expr ${v} + 1` > version
