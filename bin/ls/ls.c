/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ls.c	5.19 (Berkeley) 6/26/89";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include "ls.h"

int lstat(), strlen(), prablelen();
char *emalloc();

int (*lengthfcn)() = strlen;
int (*sortfcn)();
int needstat;

/* flags */
int f_accesstime;		/* use time of last access */
int f_firsttime = 1;		/* to control recursion */
int f_group;			/* show group ownership of a file */
int f_ignorelink;		/* indirect through symbolic link operands */
int f_inode;			/* print inode */
int f_listalldot;		/* list . and .. as well */
int f_listdir;			/* list actual directory, not contents */
int f_listdot;			/* list files beginning with . */
int f_longform;			/* long listing format */
int f_nonprint;			/* show unprintables as ? */
int f_recursive;		/* ls subdirectories also */
int f_reversesort;		/* reverse whatever sort is used */
int f_singlecol;		/* use single column output */
int f_size;			/* list size in short listing */
int f_specialdir;		/* force params to be directories */
int f_statustime;		/* use time of last mode change */
int f_timesort;			/* sort by time vice name */
int f_type;			/* add type character for non-regular files */

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind, stat();
	int ch;
	int namecmp(), revnamecmp(), acccmp(), revacccmp();
	int modcmp(), revmodcmp(), statcmp(), revstatcmp();

	/*
	 * terminal defaults to -C -q
	 * non-terminal defaults to -1
	 */
	if (isatty(1)) {
		f_nonprint = 1;
		lengthfcn = prablelen;
	} else
		f_singlecol = 1;

	/* root is -A automatically */
	if (!getuid())
		f_listdot = 1;

	while ((ch = getopt(argc, argv, "1ACFLRacdfgilqrstu")) != EOF) {
		switch (ch) {
		/*
		 * -1, -C and -l all override each other
		 * so shell aliasing works right
		 */
		case '1':
			f_singlecol = 1;
			f_longform = 0;
			break;
		case 'C':
			f_longform = f_singlecol = 0;
			break;
		case 'l':
			f_longform = 1;
			f_singlecol = 0;
			break;
		/* -c and -u override each other */
		case 'c':
			f_statustime = 1;
			f_accesstime = 0;
			break;
		case 'u':
			f_accesstime = 1;
			f_statustime = 0;
			break;
		case 'F':
			f_type = 1;
			break;
		case 'L':
			f_ignorelink = 1;
			break;
		case 'R':
			f_recursive = 1;
			break;
		case 'a':
			f_listalldot = 1;
			/* FALLTHROUGH */
		case 'A':
			f_listdot = 1;
			break;
		case 'd':
			f_listdir = 1;
			break;
		case 'f':
			f_specialdir = 1;
			break;
		case 'g':
			f_group = 1;
			break;
		case 'i':
			f_inode = 1;
			break;
		case 'q':
			f_nonprint = 1;
			lengthfcn = prablelen;
			break;
		case 'r':
			f_reversesort = 1;
			break;
		case 's':
			f_size = 1;
			break;
		case 't':
			f_timesort = 1;
			break;
		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/* -f turns off -F, -R, -l, -t, -s, -r, turns on -a */
	if (f_specialdir) {
		f_longform = f_recursive = f_reversesort = f_size =
		f_timesort = f_type = 0;
		f_listdot = f_listalldot = 1;
	}

	/* -d turns off -R */
	if (f_listdir)
		f_recursive = 0;

	/* if need to stat files */
	needstat = f_longform || f_recursive || f_timesort || f_size || f_type;

	/* select a sort function */
	if (f_reversesort) {
		if (!f_timesort)
			sortfcn = revnamecmp;
		else if (f_accesstime)
			sortfcn = revacccmp;
		else if (f_statustime)
			sortfcn = revstatcmp;
		else /* use modification time */
			sortfcn = revmodcmp;
	} else {
		if (!f_timesort)
			sortfcn = namecmp;
		else if (f_accesstime)
			sortfcn = acccmp;
		else if (f_statustime)
			sortfcn = statcmp;
		else /* use modification time */
			sortfcn = modcmp;
	}

	if (argc)
		args(argc, argv);
	else
		curdir();
	exit(0);
}

curdir()
{
	LS local, *stats;
	int num;
	char *names;

	if (lstat(local.name = ".", &local.lstat)) {
		(void)fprintf(stderr, "ls: .: %s\n", strerror(errno));
		exit(1);
	}
	if (num = buildstats(&local, &stats, &names))
		ls(stats, num);
}

static char path[MAXPATHLEN + 1];
static char *endofpath = path;

args(argc, argv)
	int argc;
	char **argv;
{
	register LS *dstatp, *rstatp;
	LS *dstats, *rstats;
	register int cnt, dircnt, regcnt;
	struct stat sb;
	LS *stats;
	int num, (*statfcn)(), stat(), lstat();
	char *names, top[MAXPATHLEN + 1];

	/*
	 * walk through the operands, building separate arrays of LS
	 * structures for directory and non-directory files.
	 */
	dstats = rstats = NULL;
	statfcn = f_ignorelink ? stat : lstat;
	for (dircnt = regcnt = 0; *argv; ++argv) {
		if (statfcn(*argv, &sb)) {
			(void)fprintf(stderr, "ls: %s: %s\n",
			    *argv, strerror(errno));
			if (errno == ENOENT)
				continue;
			exit(1);
		}
		if (!f_specialdir && !f_listdir && S_ISDIR(sb.st_mode)) {
			if (!dstats)
				dstatp = dstats = (LS *)emalloc((u_int)argc *
				    (sizeof(LS)));
			dstatp->name = *argv;
			dstatp->lstat = sb;
			++dstatp;
			++dircnt;
		}
		else {
			if (!rstats)
				rstatp = rstats = (LS *)emalloc((u_int)argc *
				    (sizeof(LS)));
			rstatp->name = *argv;
			rstatp->lstat = sb;
			++rstatp;
			++regcnt;
		}
	}
	/* display regular files */
	if (regcnt) {
		/*
		 * for -f flag -- switch above treats all -f operands as
		 * regular files; this code uses buildstats() to read
		 * them as directories.
		 */
		if (f_specialdir) {
			for (cnt = regcnt; cnt--;) {
				if (num = buildstats(rstats++, &stats, &names))
					ls(stats, num);
				(void)free((char *)stats);
				(void)free((char *)names);
			}
		} else
			ls(rstats, regcnt);
	}
	/* display directories */
	if (dircnt) {
		register char *p;

		if (dircnt > 1) {
			(void)getwd(top);
			qsort((char *)dstats, dircnt, sizeof(LS), sortfcn);
		}
		for (cnt = 0; cnt < dircnt; ++dstats) {
			for (endofpath = path, p = dstats->name;
			    *endofpath = *p++; ++endofpath);
			ls_dir(dstats, regcnt, regcnt || dircnt > 1);
			if (++cnt < dircnt && chdir(top)) {
				(void)fprintf(stderr, "ls: %s: %s\n",
				    top, strerror(errno));
				exit(1);
			}
		}
	}
}

ls(stats, num)
	LS *stats;
	register int num;
{
	register char *p, *savedpath;
	LS *lp;

	if (num > 1 && !f_specialdir)
		qsort((char *)stats, num, sizeof(LS), sortfcn);

	printdir(stats, num);

	if (f_recursive) {
		savedpath = endofpath;
		for (lp = stats; num--; ++lp) {
			if (!S_ISDIR(lp->lstat.st_mode))
				continue;
			p = lp->name;
			if (p[0] == '.' && (!p[1] || p[1] == '.' && !p[2]))
				continue;
			if (endofpath != path && endofpath[-1] != '/')
				*endofpath++ = '/';
			for (; *endofpath = *p++; ++endofpath);
			ls_dir(lp, 1, 1);
			*(endofpath = savedpath) = '\0';
		}
	}
}

ls_dir(lp, newline, tag)
	LS *lp;
	int newline, tag;
{
	LS *stats;
	int num;
	char *names;

	/*
	 * this doesn't really belong here, but it's the only place that
	 * everybody goes through; the `tag' variable is so that we don't
	 * print the header for directories unless we're going to display
	 * more directories, or we've already displayed files or directories.
	 * The `newline' variable keeps us from inserting a newline before
	 * we've displayed anything at all.
	 */
	if (newline)
		(void)putchar('\n');
	if (tag)
		(void)printf("%s:\n", path);

	if (chdir(lp->name)) {
		(void)fprintf(stderr, "ls: %s: %s\n",
		    lp->name, strerror(errno));
		return;
	}
	if (num = buildstats(lp, &stats, &names))
		ls(stats, num);
	(void)free((char *)stats);
	(void)free((char *)names);
	if (chdir("..")) {
		(void)fprintf(stderr, "ls: ..: %s\n", strerror(errno));
		exit(1);
	}
}

static
buildstats(lp, s_stats, s_names)
	LS *lp, **s_stats;
	char **s_names;
{
	register int cnt, maxentry;
	register char *p, *names;
	struct dirent *dp;
	LS *stats;
	DIR *dirp;

	/* make this big so we don't realloc often */
#define	DEFNUM	256
	maxentry = DEFNUM;
	*s_stats = stats = (LS *)emalloc((u_int)DEFNUM * sizeof(LS));
	*s_names = names = emalloc((u_int)lp->lstat.st_size);

	if (!(dirp = opendir(f_specialdir ? lp->name : "."))) {
		(void)fprintf(stderr, "ls: %s: %s\n",
		    lp->name, strerror(errno));
		return(0);
	}
	for (cnt = 0; dp = readdir(dirp);) {
		/* this does -A and -a */
		p = dp->d_name;
		if (p[0] == '.') {
			if (!f_listdot)
				continue;
			if (!f_listalldot && (!p[1] || p[1] == '.' && !p[2]))
				continue;
		}
		if (cnt == maxentry) {
			maxentry += DEFNUM;
			if (!(stats = (LS *)realloc((char *)stats,
			    (u_int)maxentry * sizeof(LS))))
				nomem();
		}
		if (needstat && lstat(dp->d_name, &stats[cnt].lstat)) {
			(void)fprintf(stderr, "ls: %s: %s\n",
			    dp->d_name, strerror(errno));
			if (errno == ENOENT)
				continue;
			exit(1);
		}
		/*
		 * get the inode from the directory, in case the -f flag
		 * was set and we can't stat the actual file.
		 */
		stats[cnt].lstat.st_ino = dp->d_ino;
		stats[cnt].name = names;
		bcopy(dp->d_name, names, (int)dp->d_namlen);
		names += dp->d_namlen;
		*names++ = '\0';
		++cnt;
	}
	closedir(dirp);
	return(cnt);
}
