divert(-1)
#
# Copyright (c) 1983 Eric P. Allman
# Copyright (c) 1988 The Regents of the University of California.
# All rights reserved.
#
# %sccs.include.redist.sh%
#

VERSIONID(@(#)use_cw_file.m4	2.1 (Berkeley) 10/2/91)

# if defined, the sendmail.cf will read the /etc/sendmail.cw file
# to find alternate names for this host.  Typically only used when
# several hosts have been squashed into one another at high speed.

define(`_USE_CW_FILE_')

divert(0)
