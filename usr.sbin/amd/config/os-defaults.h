/*
 * Copyright (c) 1989 Jan-Simon Pendry
 * Copyright (c) 1989 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)os-defaults.h	5.4 (Berkeley) 2/9/92
 *
 * $Id: os-defaults.h,v 5.2.2.1 1992/02/09 15:10:17 jsp beta $
 *
 * Common OS definitions.  These may be overridden in
 * the OS specific files ("os-foo.h").
 */

/*
 * What level of AMD are we backward compatible with?
 * This only applies to externally visible characteristics.
 * Rev.Minor.Branch.Patch (2 digits each)
 */
#define	AMD_COMPAT	5000000		/* 5.0 */

/*
 * What type is free(void*) returning?
 */
#define FREE_RETURN_TYPE	void

/*
 * Is the mount table mirrored in software
 */
#define	UPDATE_MTAB

/*
 * Where to get union wait
 */
#define	WAIT	<sys/wait.h>

/*
 * Where to get mount entry info
 */
#define	MNTENT_HDR	<mntent.h>

/*
 * Include support for syslog()
 */
#define	HAS_SYSLOG

/*
 * Byte ordering
 */
#define	ARCH_ENDIAN	"unknown"

/*
 * Name of filesystem types
 */
#define	MTAB_TYPE_NFS	"nfs"
#define	MTAB_TYPE_UFS	"4.2"

/*
 * Name of mount & unmount system calls
 *
 * NOTE:
 *  UNMOUNT_TRAP takes a struct mntent *
 */
#define	MOUNT_TRAP(type, mnt, flags, mnt_data) \
	mount(type, mnt->mnt_dir, flags, mnt_data)
#define	UNMOUNT_TRAP(mnt)	unmount(mnt->mnt_dir)

/*
 * How to unmount filesystems.
 * NEED_UMOUNT_FS includes code to scan the mount table
 * to find the correct information for the unmount system
 * call.  Some systems, such as 4.4bsd, do not require
 * this - they can just do an unmount system call directly.
 */
#define	NEED_UMOUNT_FS
#define	UMOUNT_FS(dir)	umount_fs(dir)

/*
 * Type of a file handle
 */
#define	NFS_FH_TYPE	fhandle_t *
#define	NFS_FH_DREF(dst, src) { (dst) = (src); }

/*
 * How to copy an address into an NFS filehandle
 */
#define	NFS_SA_DREF(dst, src) { (dst).addr = (src); }

/*
 * Type of filesystem type
 */
#define	MTYPE_TYPE	int

/*
 * How to get a mount list
 */
#define	READ_MTAB_FROM_FILE

/*
 * Make Amd automount points appear
 * to be zero sized.  undef this
 * if the O/S has a divide by zero
 * problem in df et al.
 */
#define	HAS_EMPTY_AUTOMOUNTS

/*
 * For the RE matcher
 */
#define	CHARBITS 0377
#define STRCSPN
#define RE_HDR "re.h"
