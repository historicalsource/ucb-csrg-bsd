divert(-1)
#
# Copyright (c) 1983 Eric P. Allman
# Copyright (c) 1988, 1993
#	The Regents of the University of California.  All rights reserved.
#
# %sccs.include.redist.sh%
#

divert(0)
VERSIONID(`@(#)solaris2.m4	8.1 (Berkeley) 8/7/93')
divert(-1)

define(`ALIAS_FILE', /etc/mail/aliases)
define(`HELP_FILE', /var/lib/sendmail.hf)
define(`STATUS_FILE', /etc/mail/sendmail.st)
define(`LOCAL_MAILER_FLAGS', `fSn')
