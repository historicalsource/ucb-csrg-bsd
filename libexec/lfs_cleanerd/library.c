/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)library.c	5.1 (Berkeley) 8/6/92";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "clean.h"

void	 add_blocks __P((FS_INFO *, BLOCK_INFO *, int *, SEGSUM *, caddr_t,
	     daddr_t, daddr_t));
void	 add_inodes __P((FS_INFO *, INODE_INFO *, int *, SEGSUM *, caddr_t,
	     daddr_t));
int	 bi_compare __P((const void *, const void *));
int	 bi_toss __P((const void *, const void *, const void *));
void	 get_ifile __P((FS_INFO *));
int	 get_superblock __P((FS_INFO *, struct lfs *));
int	 ii_compare __P((const void *, const void *));
int	 ii_toss __P((const void *, const void *, const void *));
int	 pseg_valid __P((FS_INFO *, SEGSUM *));

/*
 * This function will get information on all mounted file systems
 * of a given type.
 */
int
fs_getmntinfo(buf, type)
	struct	statfs	**buf;
	int	type;
{
	struct statfs *tstatfsp;
	struct statfs *sbp;
	int count, i, tcount;

	tcount = getmntinfo(&tstatfsp, 0);

	if (tcount < 0) {
		err(0, "getmntinfo failed");
		return (-1);
	}

	for (count = 0, i = 0; i < tcount ; ++i)
		if (tstatfsp[i].f_type == type)
			++count;

	if (count) {
		if (!(*buf = (struct statfs *)
			malloc(count * sizeof(struct statfs))))
			err(1, "fs_getmntinfo: out of space");
		for (i = 0, sbp = *buf; i < tcount ; ++i) {
			if (tstatfsp[i].f_type == type) {
				*sbp = tstatfsp[i];
				++sbp;
			}
		}
	}
	return (count);
}

/*
 * Get all the information available on an LFS file system.
 * Returns an array of FS_INFO structures, NULL on error.
 */
FS_INFO *
get_fs_info (lstatfsp, count)
	struct statfs *lstatfsp;	/* IN: array of statfs structs */
	int count;			/* IN: number of file systems */
{
	FS_INFO	*fp, *fsp;
	int	i;
	
	fsp = (FS_INFO *)malloc(count * sizeof(FS_INFO));

	for (fp = fsp, i = 0; i < count; ++i, ++fp) {
		fp->fi_statfsp = lstatfsp++;
		if (get_superblock (fp, &fp->fi_lfs))
			err(1, "get_fs_info: get_superblock failed");
		fp->fi_daddr_shift =
		     fp->fi_lfs.lfs_bshift - fp->fi_lfs.lfs_fsbtodb;
		get_ifile (fp);
	}
	return (fsp);
}

/*
 * If we are reading the ifile then we need to refresh it.  Even if
 * we are mmapping it, it might have grown.  Finally, we need to 
 * refresh the file system information (statfs) info.
 */
void
reread_fs_info(fsp, count)
	FS_INFO *fsp;	/* IN: array of fs_infos to free */
	int count;	/* IN: number of file systems */
{
	int i;
	
	for (i = 0; i < count; ++i, ++fsp) {
		if (statfs(fsp->fi_statfsp->f_mntonname, fsp->fi_statfsp))
			err(0, "reread_fs_info: statfs failed");
#ifdef MMAP_WORKS
		if (munmap(fsp->fi_cip, fsp->fi_ifile_length) < 0)
			err(0, "reread_fs_info: munmap failed");
#else
		free (fsp->fi_cip);
#endif /* MMAP_WORKS */
		get_ifile (fsp);
	}
}

/* 
 * Gets the superblock from disk (possibly in face of errors) 
 */
int
get_superblock (fsp, sbp)
	FS_INFO *fsp;		/* local file system info structure */
	struct lfs *sbp;
{
	char mntfromname[MNAMELEN+1];
        int fid;

	strcpy(mntfromname, "/dev/r");
	strcat(mntfromname, fsp->fi_statfsp->f_mntfromname+5);

	if ((fid = open(mntfromname, O_RDONLY, (mode_t)0)) < 0) {
		err(0, "get_superblock: bad open");
		return (-1);
	}

	get(fid, LFS_LABELPAD, sbp, sizeof(struct lfs));
	close (fid);
	
	return (0);
}

/* 
 * This function will map the ifile into memory.  It causes a
 * fatal error on failure.
 */
void
get_ifile (fsp)
	FS_INFO	*fsp;
{
	struct stat file_stat;
	caddr_t ifp;
	char *ifile_name;
	int count, fid;

	ifp = NULL;
	sync();
	ifile_name = malloc(strlen(fsp->fi_statfsp->f_mntonname) +
	    strlen(IFILE_NAME)+2);
	strcat(strcat(strcpy(ifile_name, fsp->fi_statfsp->f_mntonname), "/"),
	    IFILE_NAME);

	if ((fid = open(ifile_name, O_RDWR, (mode_t)0)) < 0)
		err(1, "get_ifile: bad open");

	if (fstat (fid, &file_stat))
		err(1, "get_ifile: fstat failed");

	fsp->fi_ifile_length = file_stat.st_size;

	/* get the ifile */
#ifndef MMAP_WORKS
	if (!(ifp = malloc ((size_t)fsp->fi_ifile_length)))
		err (1, "get_ifile: malloc failed"); 
redo_read:
	count = read (fid, ifp, (size_t) fsp->fi_ifile_length);

	if (count < 0)
		err(1, "get_ifile: bad ifile read"); 
	else if (count < (int)fsp->fi_ifile_length) {
		err(0, "get_ifile");
		if (lseek(fid, 0, SEEK_SET) < 0)
			err(1, "get_ifile: bad ifile lseek"); 
		goto redo_read;
	}
#else	/* MMAP_WORKS */
	ifp = mmap ((caddr_t)0, (size_t) fsp->fi_ifile_length, PROT_READ|PROT_WRITE,
		MAP_FILE|MAP_SHARED, fid, (off_t)0);
	if (ifp < 0)
		err(1, "get_ifile: mmap failed");
#endif	/* MMAP_WORKS */

	close (fid);

	fsp->fi_cip = (CLEANERINFO *)ifp;
	fsp->fi_segusep = (SEGUSE *)(ifp + CLEANSIZE(fsp));
	fsp->fi_ifilep  = (IFILE *)((caddr_t)fsp->fi_segusep + SEGTABSIZE(fsp));

	/*
	 * The number of ifile entries is equal to the number of blocks
	 * blocks in the ifile minus the ones allocated to cleaner info
	 * and segment usage table multiplied by the number of ifile
	 * entries per page.
	 */
	fsp->fi_ifile_count = (fsp->fi_ifile_length >> fsp->fi_lfs.lfs_bshift -
	    fsp->fi_lfs.lfs_cleansz - fsp->fi_lfs.lfs_segtabsz) *
	    fsp->fi_lfs.lfs_ifpb;

	free (ifile_name);
}

/*
 * This function will scan a segment and return a list of
 * <inode, blocknum> pairs which indicate which blocks were
 * contained as live data within the segment when the segment
 * summary was read (it may have "died" since then).  Any given
 * pair will be listed at most once.
 */
int 
lfs_segmapv(fsp, seg, seg_buf, blocks, bcount, inodes, icount)
	FS_INFO *fsp;		/* pointer to local file system information */
	int seg;		/* the segment number */
	caddr_t seg_buf;	/* the buffer containing the segment's data */
	BLOCK_INFO **blocks;	/* OUT: array of block_info for live blocks */
	int *bcount;		/* OUT: number of active blocks in segment */
	INODE_INFO **inodes;	/* OUT: array of inode_info for live inodes */
	int *icount; 		/* OUT: number of active inodes in segment */
{
	BLOCK_INFO *bip;
	INODE_INFO *iip;
	SEGSUM *sp;
	SEGUSE *sup;
	struct lfs *lfsp;
	caddr_t s, segend;
	daddr_t pseg_addr, seg_addr;
	int nblocks, num_iblocks;
	time_t timestamp;

	lfsp = &fsp->fi_lfs;
	num_iblocks = lfsp->lfs_ssize;
	if (!(bip = malloc(lfsp->lfs_ssize * sizeof(BLOCK_INFO))))
		goto err0;
	if (!(iip = malloc(lfsp->lfs_ssize * sizeof(INODE_INFO))))
		goto err1;

	sup = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, seg);
	s = seg_buf + (sup->su_flags & SEGUSE_SUPERBLOCK ? LFS_SBPAD : 0);
	seg_addr = sntoda(lfsp, seg);
	pseg_addr = seg_addr + (sup->su_flags & SEGUSE_SUPERBLOCK ? btodb(LFS_SBPAD) : 0);
#ifdef VERBOSE
		printf("\tsegment buffer at: 0x%x\tseg_addr 0x%x\n", s, seg_addr);
#endif /* VERBOSE */

	*bcount = 0;
	*icount = 0;
	for (segend = seg_buf + seg_size(lfsp), timestamp = 0; s < segend; ) {
		sp = (SEGSUM *)s;
#ifdef VERBOSE
		printf("\tpartial at: 0x%x\n", pseg_addr);
		print_SEGSUM(lfsp, sp);
		fflush(stdout);
#endif /* VERBOSE */

		nblocks = pseg_valid(fsp, sp);
		if (nblocks <= 0)
			break;

		/* Check if we have hit old data */
		if (timestamp > ((SEGSUM*)s)->ss_create)
			break;
		timestamp = ((SEGSUM*)s)->ss_create;

		/*
		 * Right now we die if we run out of room, we could probably
		 * recover if we were smart.
		 */
		if (*icount + sp->ss_ninos > num_iblocks) {
			num_iblocks = *icount + sp->ss_ninos;
			iip = realloc (iip, num_iblocks * sizeof(INODE_INFO));
			if (!iip)
				goto err1;
		}
		add_inodes(fsp, iip, icount, sp, seg_buf, seg_addr);
		add_blocks(fsp, bip, bcount, sp, seg_buf, seg_addr, pseg_addr);
		pseg_addr += fsbtodb(lfsp, nblocks) +
		    bytetoda(fsp, LFS_SUMMARY_SIZE);
		s += (nblocks << lfsp->lfs_bshift) + LFS_SUMMARY_SIZE;
	}
	qsort(iip, *icount, sizeof(INODE_INFO), ii_compare);
	qsort(bip, *bcount, sizeof(BLOCK_INFO), bi_compare);
	toss(iip, icount, sizeof(INODE_INFO), ii_toss, NULL);
	toss(bip, bcount, sizeof(BLOCK_INFO), bi_toss, NULL);
#ifdef VERBOSE
	{
		BLOCK_INFO *_bip;
		INODE_INFO *_iip;
		int i;

		printf("BLOCK INFOS\n");
		for (_bip = bip, i=0; i < *bcount; ++_bip, ++i)
			PRINT_BINFO(_bip);
		printf("INODE INFOS\n");
		for (_iip = iip, i=0; i < *icount; ++_iip, ++i)
			PRINT_IINFO(1, _iip);
	}
#endif
	*blocks = bip;
	*inodes = iip;
	return (0);

err1:	free(bip);
err0:	*bcount = 0;
	*icount = 0;
	return (-1);
	
}

/* 
 * This will parse a partial segment and fill in BLOCK_INFO structures
 * for each block described in the segment summary.  It will not include
 * blocks or inodes from files with new version numbers.  
 */
void
add_blocks (fsp, bip, countp, sp, seg_buf, segaddr, psegaddr)
	FS_INFO *fsp;		/* pointer to super block */
	BLOCK_INFO *bip;	/* Block info array */
	int *countp;		/* IN/OUT: number of blocks in array */
	SEGSUM	*sp;		/* segment summmary pointer */
	caddr_t seg_buf;	/* buffer containing segment */
	daddr_t segaddr;	/* address of this segment */
	daddr_t psegaddr;	/* address of this partial segment */
{
	IFILE	*ifp;
	FINFO	*fip;
	caddr_t	bp;
	daddr_t	*dp;
	daddr_t *iaddrp;	/* pointer to current inode block */
	int db_per_block, i, j;
	u_long page_size;

#ifdef VERBOSE
	printf("FILE INFOS\n");
#endif
	db_per_block = fsbtodb(&fsp->fi_lfs, 1);
	page_size = fsp->fi_lfs.lfs_bsize;
	bp = seg_buf + datobyte(fsp, psegaddr - segaddr) + LFS_SUMMARY_SIZE;
	bip += *countp;
	psegaddr += bytetoda(fsp, LFS_SUMMARY_SIZE);
	iaddrp = (daddr_t *)((caddr_t)sp + LFS_SUMMARY_SIZE);
	--iaddrp;
	for (fip = (FINFO *)(sp + 1), i = 0; i < sp->ss_nfinfo;
	    ++i, fip = (FINFO *)(&fip->fi_blocks[fip->fi_nblocks])) {

		ifp = IFILE_ENTRY(&fsp->fi_lfs, fsp->fi_ifilep, fip->fi_ino);
		PRINT_FINFO(fip, ifp);
		if (ifp->if_version > fip->fi_version)
			continue;
		dp = &(fip->fi_blocks[0]);
		for (j = 0; j < fip->fi_nblocks; j++, dp++) {
			while (psegaddr == *iaddrp) {
				psegaddr += db_per_block;
				bp += page_size;
				--iaddrp;
			}
			bip->bi_inode = fip->fi_ino;
			bip->bi_lbn = *dp;
			bip->bi_daddr = psegaddr;
			bip->bi_segcreate = (time_t)(sp->ss_create);
			bip->bi_bp = bp;
			psegaddr += db_per_block;
			bp += page_size;
			++bip;
			++(*countp);
		}
	}
}

/*
 * For a particular segment summary, reads the inode blocks and adds
 * INODE_INFO structures to the array.  Returns the number of inodes
 * actually added.
 */
void
add_inodes (fsp, iip, countp, sp, seg_buf, seg_addr)
	FS_INFO *fsp;		/* pointer to super block */
	INODE_INFO *iip;
	int *countp;		/* pointer to current number of inodes */
	SEGSUM *sp;		/* segsum pointer */
	caddr_t	seg_buf;	/* the buffer containing the segment's data */
	daddr_t	seg_addr;	/* disk address of seg_buf */
{
	struct dinode *di;
	struct lfs *lfsp;
	IFILE *ifp;
	INODE_INFO *ip;
	daddr_t	*daddrp;
	ino_t inum;
	int i;
	
	if (sp->ss_ninos <= 0)
		return;
	
	ip = iip + *countp;
	lfsp = &fsp->fi_lfs;
#ifdef VERBOSE
	(void) printf("INODE_INFOS:\n");
#endif
	daddrp = (daddr_t *)((caddr_t)sp + LFS_SUMMARY_SIZE);
	for (i = 0; i < sp->ss_ninos; ++i) {
		if (i % INOPB(lfsp) == 0) {
			--daddrp;
			di = (struct dinode *)(seg_buf +
			    ((*daddrp - seg_addr) << fsp->fi_daddr_shift));
		} else 
			++di;
		
		inum = di->di_inum;
		ip->ii_daddr = *daddrp;
		ip->ii_inode = inum;
		ip->ii_dinode = di;
		ip->ii_segcreate = sp->ss_create;

		ifp = IFILE_ENTRY(lfsp, fsp->fi_ifilep, inum);
		PRINT_IINFO(ifp->if_daddr == *daddrp, ip);
		if (ifp->if_daddr == *daddrp) {
			ip++;
			++(*countp);
		} 
	}
}

/*
 * Checks the summary checksum and the data checksum to determine if the
 * segment is valid or not.  Returns the size of the partial segment if it
 * is valid, * and 0 otherwise.  Use dump_summary to figure out size of the
 * the partial as well as whether or not the checksum is valid.
 */	 
int
pseg_valid (fsp, ssp)
	FS_INFO *fsp;   /* pointer to file system info */
	SEGSUM *ssp;	/* pointer to segment summary block */
{
	caddr_t	p;
	int i, nblocks;
	u_long *datap;

	if ((nblocks = dump_summary(&fsp->fi_lfs, ssp, 0, NULL)) <= 0)
		return(0);
		
	/* check data/inode block(s) checksum too */
	datap = (u_long *)malloc(nblocks * sizeof(u_long));
	p = (caddr_t)ssp + LFS_SUMMARY_SIZE;
	for (i = 0; i < nblocks; ++i) {
		datap[i] = *((u_long *)p);
		p += fsp->fi_lfs.lfs_bsize;
	}
	if (cksum ((void *)datap, nblocks * sizeof(u_long)) != ssp->ss_datasum)
		return (0);
	
	return (nblocks);
}


/* #define MMAP_SEGMENT */
/* 
 * read a segment into a memory buffer
 */
int
mmap_segment (fsp, segment, segbuf)
	FS_INFO *fsp;		/* file system information */
	int segment;		/* segment number */
	caddr_t *segbuf;	/* pointer to buffer area */
{
	struct lfs *lfsp;
	int fid;		/* fildes for file system device */
	daddr_t seg_daddr;	/* base disk address of segment */
	off_t seg_byte;
	size_t ssize;
	char mntfromname[MNAMELEN+2];

	lfsp = &fsp->fi_lfs;

	/* get the disk address of the beginning of the segment */
	seg_daddr = sntoda(lfsp, segment);
	seg_byte = datobyte(fsp, seg_daddr);
	ssize = seg_size(lfsp);

	strcpy(mntfromname, "/dev/r");
	strcat(mntfromname, fsp->fi_statfsp->f_mntfromname+5);

	if ((fid = open(mntfromname, O_RDONLY, (mode_t)0)) < 0) {
		err(0, "mmap_segment: bad open");
		return (-1);
	}

#ifdef MMAP_SEGMENT
	*segbuf = mmap ((caddr_t)0, seg_size(lfsp), PROT_READ,
	    MAP_FILE, fid, seg_byte);
	if (*(long *)segbuf < 0) {
		err(0, "mmap_segment: mmap failed");
		return (NULL);
	}
#else /* MMAP_SEGMENT */
#ifdef VERBOSE
	printf("mmap_segment\tseg_daddr: %lu\tseg_size: %lu\tseg_offset: %qu\n",
	    seg_daddr, ssize, seg_byte);
#endif
	/* malloc the space for the buffer */
	*segbuf = malloc(ssize);
	if (!*segbuf) {
		err(0, "mmap_segment: malloc failed");
		return(NULL);
	}

	/* read the segment data into the buffer */
	if (lseek (fid, seg_byte, SEEK_SET) != seg_byte) {
		err (0, "mmap_segment: bad lseek");
		free(*segbuf);
		return (-1);
	}
	
	if (read (fid, *segbuf, ssize) != ssize) {
		err (0, "mmap_segment: bad read");
		free(*segbuf);
		return (-1);
	}
#endif /* MMAP_SEGMENT */
	close (fid);

	return (0);
}

void
munmap_segment (fsp, seg_buf)
	FS_INFO *fsp;		/* file system information */
	caddr_t seg_buf;	/* pointer to buffer area */
{
#ifdef MMAP_SEGMENT
	munmap (seg_buf, seg_size(&fsp->fi_lfs));
#else /* MMAP_SEGMENT */
	free (seg_buf);
#endif /* MMAP_SEGMENT */
}


/*
 * USEFUL DEBUGGING TOOLS:
 */
void
print_SEGSUM (lfsp, p)
	struct lfs *lfsp;
	SEGSUM	*p;
{
	if (p)
		(void) dump_summary(lfsp, p, DUMP_ALL, NULL);
	else printf("0x0");
	fflush(stdout);
}

int
bi_compare(a, b)
	const void *a;
	const void *b;
{
	const BLOCK_INFO *ba, *bb;
	int diff;

	ba = a;
	bb = b;

	if (diff = (int)(ba->bi_inode - bb->bi_inode))
		return (diff);
	if (diff = (int)(ba->bi_lbn - bb->bi_lbn))
		return (diff);
	if (diff = (int)(ba->bi_segcreate - bb->bi_segcreate))
		return (diff);
	diff = (int)(ba->bi_daddr - bb->bi_daddr);
	return (diff);
}	

int
bi_toss(dummy, a, b)
	const void *dummy;
	const void *a;
	const void *b;
{
	const BLOCK_INFO *ba, *bb;

	ba = a;
	bb = b;

	return(ba->bi_inode == bb->bi_inode && ba->bi_lbn == bb->bi_lbn);
}

/*
 * Right now, we never look at the actually data being
 * passed to the kernel in iip->ii_dinode.  Therefore,
 * if the same inode appears twice in the same block
 * (i.e.  has the same disk address), it doesn't matter
 * which entry we pass.  However, if we get the kernel
 * to start looking at the dinode, then we will care
 * and we'll need some way to distinguish which inode
 * is the more recent one.
 */
int
ii_compare(a, b)
	const void *a;
	const void *b;
{
	const INODE_INFO *ia, *ib;
	int diff;

	ia = a;
	ib = b;

	if (diff = (int)(ia->ii_inode - ib->ii_inode))
		return (diff);
	if (diff = (int)(ia->ii_segcreate - ib->ii_segcreate))
		return (diff);
	diff = (int)(ia->ii_daddr - ib->ii_daddr);
	return (diff);
}

int
ii_toss(dummy, a, b)
	const void *dummy;
	const void *a;
	const void *b;
{
	const INODE_INFO *ia, *ib;

	ia = a;
	ib = b;

	return(ia->ii_inode == ib->ii_inode);
}

void
toss(p, nump, size, dotoss, client)
	void *p;
	int *nump;
	size_t size;
	int (*dotoss) __P((const void *, const void *, const void *));
	void *client;
{
	int i;
	void *p1;

	if (*nump == 0)
		return;

	for (i = *nump; --i > 0;) {
		p1 = p + size;
		if (dotoss(client, p, p1)) {
			bcopy(p1, p, i * size);
			--(*nump);
		} else 
			p += size;
	}
}
