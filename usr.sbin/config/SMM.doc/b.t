.\" Copyright (c) 1983 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)b.t	5.1 (Berkeley) 5/25/86
.\"
.ds LH "Building Systems With Config
.ds RH "Device Defaulting Rules
.ds CF July 27, 1983
.LP
.nr H1 1
.ds CH "
.bp
.ds CH "\(hy \\n(PN \(hy
.LG
.B
.ce
APPENDIX B. RULES FOR DEFAULTING SYSTEM DEVICES
.sp 2
.R
.NL
.PP
When \fIconfig\fP processes a ``config'' rule which does
not fully specify the location of the root file system,
paging area(s), device for system dumps, and device for
argument list processing it applies a set of rules to
define those values left unspecified.  The following list
of rules are used in defaulting system devices.
.IP 1) 3
If a root device is not specified, the swap
specification must indicate a ``generic'' system is to be built.
.IP 2) 3
If the root device does not specify a unit number, it
defaults to unit 0.
.IP 3) 3
If the root device does not include a partition specification,
it defaults to the ``a'' partition.
.IP 4) 3
If no swap area is specified, it defaults to the ``b''
partition of the root device.
.IP 5) 3
If no device is specified for processing argument lists, the
first swap partition is selected.
.IP 6) 3
If no device is chosen for system dumps, the first swap
partition is selected (see below to find out where dumps are
placed within the partition).
.PP
The following table summarizes the default partitions selected
when a device specification is incomplete, e.g. ``hp0''.
.DS
.TS
l l.
Type	Partition
_
root	``a''
swap	``b''
args	``b''
dumps	``b''
.TE
.DE
.SH
Multiple swap/paging areas
.PP
When multiple swap partitions are specified, the system treats the
first specified as a ``primary'' swap area which is always used.
The remaining partitions are then interleaved into the paging
system at the time a
.IR swapon (2)
system call is made.  This is normally done at boot time with
a call to
.IR swapon (8)
from the /etc/rc file.
.SH
System dumps
.PP
System dumps are automatically taken after a system crash,
provided the device driver for the ``dumps'' device supports
this.  The dump contains the contents of memory, but not
the swap areas.  Normally the dump device is a disk in
which case the information is copied to a location near the
back of the partition.  The dump is placed in the back of the
partition because the primary swap and dump device are commonly
the same device and this allows the system to be rebooted without
immediately overwriting the saved information.  When a dump has
occurred, the system variable \fIdumpsize\fP 
is set to a non-zero value indicating the size (in bytes) of
the dump.  The \fIsavecore\fP\|(8)
program then copies the information from the dump partition to
a file in a ``crash'' directory and also makes a copy of the
system which was running at the time of the crash (usually
``/vmunix'').  The offset to the system dump is defined in the
system variable \fIdumplo\fP (a sector offset from
the front of the dump partition). The 
.I savecore
program operates by reading the contents of \fIdumplo\fP, \fIdumpdev\fP,
and \fIdumpmagic\fP from /dev/kmem, then comparing the value
of \fIdumpmagic\fP read from /dev/kmem to that located in
corresponding location in the dump area of the dump partition.
If a match is found, 
.I savecore
assumes a crash occurred and reads \fIdumpsize\fP from the dump area
of the dump partition.  This value is then used in copying the
system dump.  Refer to 
\fIsavecore\fP\|(8)
for more information about its operation.
.PP
The value \fIdumplo\fP is calculated to be 
.DS
\fIdumpdev-size\fP \- DUMPDEV
.DE
where \fIdumpdev-size\fP is the size of the disk partition
where system dumps are to be placed, and DUMPDEV is 10 Megabytes.
If the disk partition is not large enough to hold a 10 Megabyte
dump, \fIdumplo\fP is set to 0 (the front of the partition).
For sites with more than 10 Megabytes of memory the definition
of DUMPDEV in /sys/vax/autoconf.c will have to be changed.
