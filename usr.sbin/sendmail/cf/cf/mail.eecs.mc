divert(-1)
#
# Copyright (c) 1983 Eric P. Allman
# Copyright (c) 1988, 1993
#	The Regents of the University of California.  All rights reserved.
#
# %sccs.include.redist.sh%
#

include(`../m4/cf.m4')
VERSIONID(`@(#)mail.eecs.mc	8.2 (Berkeley) 11/13/94')
OSTYPE(ultrix4.1)dnl
DOMAIN(eecs.hidden)dnl
MAILER(local)dnl
MAILER(smtp)dnl
define(`confUSERDB_SPEC', `/usr/local/lib/users.cs.db,/usr/local/lib/users.eecs.db')dnl
DDBerkeley.EDU

# hosts for which we accept and forward mail (must be in .Berkeley.EDU)
CF EECS
FF/etc/sendmail.cw

LOCAL_RULE_0
R< @ $=F . $D . > : $*		$@ $>7 $2		@here:... -> ...
R$* $=O $* < @ $=F . $D . >	$@ $>7 $1 $2 $3		...@here -> ...

R$* < @ $=F . $D . >		$#local $: $1		use UDB
