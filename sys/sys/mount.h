/*
 * Copyright (c) 1989, 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)mount.h	7.29 (Berkeley) 12/19/91
 */

typedef struct { long val[2]; } fsid_t;		/* file system id type */

/*
 * File identifier.
 * These are unique per filesystem on a single machine.
 */
#define	MAXFIDSZ	16

struct fid {
	u_short		fid_len;		/* length of data in bytes */
	u_short		fid_reserved;		/* force longword alignment */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
};

/*
 * file system statistics
 */

#define MNAMELEN 90	/* length of buffer for returned name */

struct statfs {
	short	f_type;			/* type of filesystem (see below) */
	short	f_flags;		/* copy of mount flags */
	long	f_bsize;		/* fundamental file system block size */
	long	f_iosize;		/* optimal transfer block size */
	long	f_blocks;		/* total data blocks in file system */
	long	f_bfree;		/* free blocks in fs */
	long	f_bavail;		/* free blocks avail to non-superuser */
	long	f_files;		/* total file nodes in file system */
	long	f_ffree;		/* free file nodes in fs */
	fsid_t	f_fsid;			/* file system id */
	long	f_spare[9];		/* spare for later */
	char	f_mntonname[MNAMELEN];	/* directory on which mounted */
	char	f_mntfromname[MNAMELEN];/* mounted filesystem */
};

/*
 * File system types.
 */
#define	MOUNT_NONE	0
#define	MOUNT_UFS	1
#define	MOUNT_NFS	2
#define	MOUNT_MFS	3
#define	MOUNT_PC	4
#define	MOUNT_LFS	5
#define	MOUNT_MAXTYPE	5

/*
 * Structure per mounted file system.  Each mounted file system has an
 * array of operations and an instance record.  The file systems are
 * put on a doubly linked list.
 */
struct mount {
	struct mount	*mnt_next;		/* next in mount list */
	struct mount	*mnt_prev;		/* prev in mount list */
	struct vfsops	*mnt_op;		/* operations on fs */
	struct vnode	*mnt_vnodecovered;	/* vnode we mounted on */
	struct vnode	*mnt_mounth;		/* list of vnodes this mount */
	int		mnt_flag;		/* flags */
	uid_t		mnt_exroot;		/* XXX - deprecated */
	struct statfs	mnt_stat;		/* cache of filesystem stats */
	qaddr_t		mnt_data;		/* private data */
};

/*
 * Mount flags.
 */
#define	MNT_RDONLY	0x00000001	/* read only filesystem */
#define	MNT_SYNCHRONOUS	0x00000002	/* file system written synchronously */
#define	MNT_NOEXEC	0x00000004	/* can't exec from filesystem */
#define	MNT_NOSUID	0x00000008	/* don't honor setuid bits on fs */
#define	MNT_NODEV	0x00000010	/* don't interpret special files */

/*
 * exported mount flags.
 */
#define	MNT_EXRDONLY	0x00000080	/* exported read only */
#define	MNT_EXPORTED	0x00000100	/* file system is exported */
#define	MNT_DEFEXPORTED	0x00000200	/* exported to the world */
#define	MNT_EXPORTANON	0x00000400	/* use anon uid mapping for everyone */
#define	MNT_EXKERB	0x00000800	/* exported with Kerberos uid mapping */

/*
 * Flags set by internal operations.
 */
#define	MNT_LOCAL	0x00001000	/* filesystem is stored locally */
#define	MNT_QUOTA	0x00002000	/* quotas are enabled on filesystem */

/*
 * Mask of flags that are visible to statfs()
 */
#define	MNT_VISFLAGMASK	0x0000ffff

/*
 * filesystem control flags.
 *
 * MNT_MLOCK lock the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define	MNT_UPDATE	0x00010000	/* not a real mount, just an update */
#define	MNT_DELEXPORT	0x00020000	/* delete export host lists */
#define	MNT_MLOCK	0x00100000	/* lock so that subtree is stable */
#define	MNT_MWAIT	0x00200000	/* someone is waiting for lock */
#define MNT_MPBUSY	0x00400000	/* scan of mount point in progress */
#define MNT_MPWANT	0x00800000	/* waiting for mount point */
#define MNT_UNMOUNT	0x01000000	/* unmount in progress */

/*
 * Operations supported on mounted file system.
 */
#ifdef KERNEL
#ifdef __STDC__
struct nameidata;
#endif

struct vfsops {
	int	(*vfs_mount)	__P((struct mount *mp, char *path, caddr_t data,
				    struct nameidata *ndp, struct proc *p));
	int	(*vfs_start)	__P((struct mount *mp, int flags,
				    struct proc *p));
	int	(*vfs_unmount)	__P((struct mount *mp, int mntflags,
				    struct proc *p));
	int	(*vfs_root)	__P((struct mount *mp, struct vnode **vpp));
			/* int uid,		should be uid_t */
	int	(*vfs_quotactl)	__P((struct mount *mp, int cmds, u_int uid,
				    caddr_t arg, struct proc *p));
	int	(*vfs_statfs)	__P((struct mount *mp, struct statfs *sbp,
				    struct proc *p));
	int	(*vfs_sync)	__P((struct mount *mp, int waitfor));
	int	(*vfs_fhtovp)	__P((struct mount *mp, struct fid *fhp,
				    struct vnode **vpp));
	int	(*vfs_vptofh)	__P((struct vnode *vp, struct fid *fhp));
	int	(*vfs_init)	__P((void));
};

#define VFS_MOUNT(MP, PATH, DATA, NDP, P) \
	(*(MP)->mnt_op->vfs_mount)(MP, PATH, DATA, NDP, P)
#define VFS_START(MP, FLAGS, P)	  (*(MP)->mnt_op->vfs_start)(MP, FLAGS, P)
#define VFS_UNMOUNT(MP, FORCE, P) (*(MP)->mnt_op->vfs_unmount)(MP, FORCE, P)
#define VFS_ROOT(MP, VPP)	  (*(MP)->mnt_op->vfs_root)(MP, VPP)
#define VFS_QUOTACTL(MP,C,U,A,P)  (*(MP)->mnt_op->vfs_quotactl)(MP, C, U, A, P)
#define VFS_STATFS(MP, SBP, P)	  (*(MP)->mnt_op->vfs_statfs)(MP, SBP, P)
#define VFS_SYNC(MP, WAITFOR)	  (*(MP)->mnt_op->vfs_sync)(MP, WAITFOR)
#define VFS_FHTOVP(MP, FIDP, VPP) (*(MP)->mnt_op->vfs_fhtovp)(MP, FIDP, VPP)
#define	VFS_VPTOFH(VP, FIDP)	  (*(VP)->v_mount->mnt_op->vfs_vptofh)(VP, FIDP)
#endif /* KERNEL */

/*
 * Flags for various system call interfaces.
 *
 * forcibly flags for vfs_umount().
 * waitfor flags to vfs_sync() and getfsstat()
 */
#define MNT_FORCE	1
#define MNT_NOFORCE	2
#define MNT_WAIT	1
#define MNT_NOWAIT	2

/*
 * Generic file handle
 */
struct fhandle {
	fsid_t	fh_fsid;	/* File system id of mount point */
	struct	fid fh_fid;	/* Id of file */
};
typedef struct fhandle	fhandle_t;

/*
 * Arguments to mount UFS-based filesystems
 */
#ifndef KERNEL
#include <sys/ucred.h>
#endif
struct ufs_args {
	char	*fspec;		/* block special device to mount */
	int	exflags;	/* export related flags */
	uid_t	exroot;		/* mapping for root uid */
	struct	ucred anon;	/* mapping for anonymous user */
	struct	sockaddr *saddr;/* net address to which exported */
	int	slen;		/* and the net address length */
	struct	sockaddr *smask;/* mask of valid bits in saddr */
	int	msklen;		/* and the smask length */
};

#ifdef MFS
/*
 * Arguments to mount MFS
 */
struct mfs_args {
	char	*name;		/* name to export for statfs */
	caddr_t	base;		/* base address of file system in memory */
	u_long size;		/* size of file system */
};
#endif /* MFS */

#ifdef NFS
/*
 * File Handle (32 bytes for version 2), variable up to 1024 for version 3
 */
union nfsv2fh {
	fhandle_t	fh_generic;
	u_char		fh_bytes[32];
};
typedef union nfsv2fh nfsv2fh_t;

/*
 * Arguments to mount NFS
 */
struct nfs_args {
	struct sockaddr	*addr;		/* file server address */
	int		sotype;		/* Socket type */
	int		proto;		/* and Protocol */
	nfsv2fh_t	*fh;		/* File handle to be mounted */
	int		flags;		/* flags */
	int		wsize;		/* write size in bytes */
	int		rsize;		/* read size in bytes */
	int		timeo;		/* initial timeout in .1 secs */
	int		retrans;	/* times to retry send */
	char		*hostname;	/* server's name */
};

/*
 * NFS mount option flags
 */
#define	NFSMNT_SOFT	0x0001	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE	0x0002	/* set write size */
#define	NFSMNT_RSIZE	0x0004	/* set read size */
#define	NFSMNT_TIMEO	0x0008	/* set initial timeout */
#define	NFSMNT_RETRANS	0x0010	/* set number of request retrys */
#define	NFSMNT_HOSTNAME	0x0020	/* set hostname for error printf */
#define	NFSMNT_INT	0x0040	/* allow interrupts on hard mount */
#define	NFSMNT_NOCONN	0x0080	/* Don't Connect the socket */
#define	NFSMNT_SCKLOCK	0x0100	/* Lock socket against others */
#define	NFSMNT_WANTSCK	0x0200	/* Want a socket lock */
#define	NFSMNT_SPONGY	0x0400	/* spongy mount (soft for stat and lookup) */
#define	NFSMNT_COMPRESS	0x0800	/* Compress nfs rpc xdr */
#define	NFSMNT_LOCKBITS	(NFSMNT_SCKLOCK | NFSMNT_WANTSCK)
#endif /* NFS */

#ifdef KERNEL
/*
 * exported vnode operations
 */
void	vfs_remove __P((struct mount *mp)); /* remove a vfs from mount list */
int	vfs_lock __P((struct mount *mp));   /* lock a vfs */
void	vfs_unlock __P((struct mount *mp)); /* unlock a vfs */
struct	mount *getvfs __P((fsid_t *fsid));  /* return vfs given fsid */
extern struct	mount *rootfs;		    /* ptr to root mount structure */
extern struct	vfsops *vfssw[];	    /* mount filesystem type table */

#else /* KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	fstatfs __P((int, struct statfs *));
int	getfh __P((const char *, fhandle_t *));
int	getfsstat __P((struct statfs *, long, int));
int	getmntinfo __P((struct statfs **, int));
int	mount __P((int, const char *, int, void *));
int	statfs __P((const char *, struct statfs *));
int	unmount __P((const char *, int));
__END_DECLS

#endif /* KERNEL */
