/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)spec_vnops.c	7.15 (Berkeley) 11/22/89
 */

#include "param.h"
#include "systm.h"
#include "user.h"
#include "kernel.h"
#include "conf.h"
#include "buf.h"
#include "mount.h"
#include "vnode.h"
#include "../ufs/inode.h"
#include "stat.h"
#include "errno.h"

int	spec_lookup(),
	spec_open(),
	spec_read(),
	spec_write(),
	spec_strategy(),
	spec_ioctl(),
	spec_select(),
	spec_lock(),
	spec_unlock(),
	spec_close(),
	spec_ebadf(),
	spec_badop(),
	spec_nullop();

struct vnodeops spec_vnodeops = {
	spec_lookup,		/* lookup */
	spec_badop,		/* create */
	spec_badop,		/* mknod */
	spec_open,		/* open */
	spec_close,		/* close */
	spec_ebadf,		/* access */
	spec_ebadf,		/* getattr */
	spec_ebadf,		/* setattr */
	spec_read,		/* read */
	spec_write,		/* write */
	spec_ioctl,		/* ioctl */
	spec_select,		/* select */
	spec_badop,		/* mmap */
	spec_nullop,		/* fsync */
	spec_badop,		/* seek */
	spec_badop,		/* remove */
	spec_badop,		/* link */
	spec_badop,		/* rename */
	spec_badop,		/* mkdir */
	spec_badop,		/* rmdir */
	spec_badop,		/* symlink */
	spec_badop,		/* readdir */
	spec_badop,		/* readlink */
	spec_badop,		/* abortop */
	spec_nullop,		/* inactive */
	spec_nullop,		/* reclaim */
	spec_lock,		/* lock */
	spec_unlock,		/* unlock */
	spec_badop,		/* bmap */
	spec_strategy,		/* strategy */
};

/*
 * Trivial lookup routine that always fails.
 */
spec_lookup(vp, ndp)
	struct vnode *vp;
	struct nameidata *ndp;
{

	ndp->ni_dvp = vp;
	ndp->ni_vp = NULL;
	return (ENOTDIR);
}

/*
 * Open called to allow handler
 * of special files to initialize and
 * validate before actual IO.
 */
/* ARGSUSED */
spec_open(vp, mode, cred)
	register struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	dev_t dev = (dev_t)vp->v_rdev;
	register int maj = major(dev);

	if (vp->v_mount && (vp->v_mount->m_flag & M_NODEV))
		return (ENXIO);

	switch (vp->v_type) {

	case VCHR:
		if ((u_int)maj >= nchrdev)
			return (ENXIO);
		return ((*cdevsw[maj].d_open)(dev, mode, S_IFCHR));

	case VBLK:
		if ((u_int)maj >= nblkdev)
			return (ENXIO);
		return ((*bdevsw[maj].d_open)(dev, mode, S_IFBLK));
	}
	return (0);
}

/*
 * Vnode op for read
 */
spec_read(vp, uio, ioflag, cred)
	register struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	int error;
	extern int mem_no;

	if (uio->uio_rw != UIO_READ)
		panic("spec_read mode");
	if (uio->uio_resid == 0)
		return (0);
	/*
	 * XXX  Set access flag for the ufs filesystem.
	 */
	if (vp->v_tag == VT_UFS)
		VTOI(vp)->i_flag |= IACC;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Negative offsets allowed only for /dev/kmem
		 */
		if (uio->uio_offset < 0 && major(vp->v_rdev) != mem_no)
			return (EINVAL);
		VOP_UNLOCK(vp);
		error = (*cdevsw[major(vp->v_rdev)].d_read)
			(vp->v_rdev, uio, ioflag);
		VOP_LOCK(vp);
		return (error);

	case VBLK:
		if (uio->uio_offset < 0)
			return (EINVAL);
		return (readblkvp(vp, uio, cred, ioflag));

	default:
		panic("spec_read type");
	}
	/* NOTREACHED */
}

/*
 * Vnode op for write
 */
spec_write(vp, uio, ioflag, cred)
	register struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	int error;
	extern int mem_no;

	if (uio->uio_rw != UIO_WRITE)
		panic("spec_write mode");
	/*
	 * XXX  Set update and change flags for the ufs filesystem.
	 */
	if (vp->v_tag == VT_UFS)
		VTOI(vp)->i_flag |= IUPD|ICHG;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Negative offsets allowed only for /dev/kmem
		 */
		if (uio->uio_offset < 0 && major(vp->v_rdev) != mem_no)
			return (EINVAL);
		VOP_UNLOCK(vp);
		error = (*cdevsw[major(vp->v_rdev)].d_write)
			(vp->v_rdev, uio, ioflag);
		VOP_LOCK(vp);
		return (error);

	case VBLK:
		if (uio->uio_resid == 0)
			return (0);
		if (uio->uio_offset < 0)
			return (EINVAL);
		return (writeblkvp(vp, uio, cred, ioflag));

	default:
		panic("spec_write type");
	}
	/* NOTREACHED */
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
spec_ioctl(vp, com, data, fflag, cred)
	struct vnode *vp;
	register int com;
	caddr_t data;
	int fflag;
	struct ucred *cred;
{
	dev_t dev = vp->v_rdev;

	switch (vp->v_type) {

	case VCHR:
		return ((*cdevsw[major(dev)].d_ioctl)(dev, com, data, fflag));

	case VBLK:
		return ((*bdevsw[major(dev)].d_ioctl)(dev, com, data, fflag));

	default:
		panic("spec_ioctl");
		/* NOTREACHED */
	}
}

/* ARGSUSED */
spec_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	register dev_t dev;

	switch (vp->v_type) {

	default:
		return (1);		/* XXX */

	case VCHR:
		dev = vp->v_rdev;
		return (*cdevsw[major(dev)].d_select)(dev, which);
	}
}

/*
 * Just call the device strategy routine
 */
spec_strategy(bp)
	register struct buf *bp;
{
	(*bdevsw[major(bp->b_dev)].d_strategy)(bp);
	return (0);
}

/*
 * At the moment we do not do any locking.
 */
/* ARGSUSED */
spec_lock(vp)
	struct vnode *vp;
{

	return (0);
}

/* ARGSUSED */
spec_unlock(vp)
	struct vnode *vp;
{

	return (0);
}

/*
 * Device close routine
 */
/* ARGSUSED */
spec_close(vp, flag, cred)
	register struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	dev_t dev = vp->v_rdev;
	int (*cfunc)();
	int error, mode;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * If the vnode is locked, then we are in the midst
		 * of forcably closing the device, otherwise we only
		 * close on last reference.
		 */
		if (vp->v_count > 1 && (vp->v_flag & VXLOCK) == 0)
			return (0);
		cfunc = cdevsw[major(dev)].d_close;
		mode = S_IFCHR;
		break;

	case VBLK:
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		bflush(vp->v_mount);
		if (binval(vp->v_mount))
			return (0);
		/*
		 * We do not want to really close the device if it
		 * is still in use unless we are trying to close it
		 * forcibly. Since every use (buffer, vnode, swap, cmap)
		 * holds a reference to the vnode, and because we ensure
		 * that there cannot be more than one vnode per device,
		 * we need only check that we are down to the last
		 * reference to detect last close.
		 */
		if (vp->v_count > 1 && (vp->v_flag & VXLOCK) == 0)
			return (0);
		cfunc = bdevsw[major(dev)].d_close;
		mode = S_IFBLK;
		break;

	default:
		panic("spec_close: not special");
	}

	if (setjmp(&u.u_qsave)) {
		/*
		 * If device close routine is interrupted,
		 * must return so closef can clean up.
		 */
		error = EINTR;
	} else
		error = (*cfunc)(dev, flag, mode);
	return (error);
}

/*
 * Special device failed operation
 */
spec_ebadf()
{

	return (EBADF);
}

/*
 * Special device bad operation
 */
spec_badop()
{

	panic("spec_badop called");
	/* NOTREACHED */
}

/*
 * Special device null operation
 */
spec_nullop()
{

	return (0);
}
