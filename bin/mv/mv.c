static	char *sccsid = "@(#)mv.c	4.6 (Berkeley) 82/10/23";

/*
 * mv file1 file2
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define	DOT	"."
#define	DOTDOT	".."
#define	DELIM	'/'
#define SDELIM "/"
#define	MAXN	300
#define MODEBITS 07777
#define ROOTINO 2

char	*pname();
char	*sprintf();
char	*dname();
struct	stat s1, s2;
int	iflag = 0;	/* interactive flag. If this flag is set,
			 * the user is queried before files are
			 * destroyed by cp.
			 */
int	fflag = 0;	/* force flag. supercedes all restrictions */

main(argc, argv)
register char *argv[];
{
	register i, r;
	register char *arg;

	/* get the flag(s) */

	if (argc < 2)
		goto usage;
	while (argc>1 && *argv[1] == '-') {
		argc--;
		arg = *++argv;

		/*
		 *  all files following a null option are considered file names
		 */
		if (*(arg+1) == '\0') break;

		while (*++arg != '\0')
			switch (*arg) {

			/* interactive mode */
			case 'i':
				iflag++;
				break;

			/* force moves */
			case 'f':
				fflag++;
				break;

			/* don't live with bad options */
			default:
				goto usage;
			}
	}
	if (argc < 3)
		goto usage;
	if (lstat(argv[1], &s1) < 0) {
		fprintf(stderr, "mv: cannot access %s\n", argv[1]);
		return(1);
	}
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		if (argc != 3)
			goto usage;
		return mvdir(argv[1], argv[2]);
	}
	setuid(getuid());
	if (argc > 3)
		if (stat(argv[argc-1], &s2) < 0 || (s2.st_mode & S_IFMT) != S_IFDIR)
			goto usage;
	r = 0;
	for (i=1; i<argc-1; i++)
		r |= move(argv[i], argv[argc-1]);
	return(r);
usage:
	fprintf(stderr, "usage: mv [-if] f1 f2; or mv [-if] d1 d2; or mv [-if] f1 ... fn d1\n");
	return(1);
}

move(source, target)
char *source, *target;
{
	register c, i;
	int	status;
	char	buf[MAXN];

	if (lstat(source, &s1) < 0) {
		fprintf(stderr, "mv: cannot access %s\n", source);
		return(1);
	}
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		fprintf(stderr, "mv: directory rename only\n");
		return(1);
	}
	if (stat(target, &s2) >= 0) {
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			sprintf(buf, "%s/%s", target, dname(source));
			target = buf;
		}
		if (lstat(target, &s2) >= 0) {
			if ((s2.st_mode & S_IFMT) == S_IFDIR) {
				fprintf(stderr, "mv: %s is a directory\n", target);
				return(1);
			} else if (iflag && !fflag) {
				fprintf(stderr, "remove %s? ", target);
				i = c = getchar();
				while (c != '\n' && c != EOF)
					c = getchar();
				if (i != 'y')
					return(1);
			}
			if (s1.st_dev==s2.st_dev && s1.st_ino==s2.st_ino) {
				fprintf(stderr, "mv: %s and %s are identical\n",
						source, target);
				return(1);
			}
			if (access(target, 2) < 0 && !fflag && isatty(fileno(stdin))) {
				fprintf(stderr, "override protection %o for %s? ", 
					s2.st_mode & MODEBITS, target);
				i = c = getchar();
				while (c != '\n' && c != EOF)
					c = getchar();
				if (i != 'y')
					return(1);
			}
			if (unlink(target) < 0) {
				fprintf(stderr, "mv: cannot unlink %s\n", target);
				return(1);
			}
		}
	}
	if ((s1.st_mode & S_IFMT) == S_IFLNK) {
		register m;
		char symln[MAXN];

		if (readlink(source, symln, sizeof (symln)) < 0) {
			perror(source);
			return (1);
		}
		m = umask(~(s1.st_mode & MODEBITS));
		if (symlink(symln, target) < 0) {
			perror(target);
			return (1);
		}
		umask(m);
	} else if (link(source, target) < 0) {
		i = fork();
		if (i == -1) {
			fprintf(stderr, "mv: try again\n");
			return(1);
		}
		if (i == 0) {
			execl("/bin/cp", "cp", source, target, 0);
			fprintf(stderr, "mv: cannot exec cp\n");
			exit(1);
		}
		while ((c = wait(&status)) != i && c != -1)
			;
		if (status != 0)
			return(1);
		utime(target, &s1.st_atime);
	}
	if (unlink(source) < 0) {
		fprintf(stderr, "mv: cannot unlink %s\n", source);
		return(1);
	}
	return(0);
}

mvdir(source, target)
char *source, *target;
{
	register char *p;
	register i;
	char buf[MAXN];
	char c,cc;

	if (stat(target, &s2) >= 0) {
		if ((s2.st_mode&S_IFMT) != S_IFDIR) {
			fprintf(stderr, "mv: %s exists\n", target);
			return(1);
		}
		p = dname(source);
		if (strlen(target) > MAXN-strlen(p)-2) {
			fprintf(stderr, "mv :target name too long\n");
			return(1);
		}
		strcpy(buf, target);
		target = buf;
		strcat(buf, SDELIM);
		strcat(buf, p);
		if (stat(target, &s2) >= 0) {
			fprintf(stderr, "mv: %s exists\n", buf);
			return(1);
		}
	}
	if (strcmp(source, target) == 0) {
		fprintf(stderr, "mv: ?? source == target, source exists and target doesnt\n");
		return(1);
	}
	p = dname(source);
	if (!strcmp(p, DOT) || !strcmp(p, DOTDOT) || !strcmp(p, "") || p[strlen(p)-1]=='/') {
		fprintf(stderr, "mv: cannot rename %s\n", p);
		return(1);
	}
	if (stat(pname(source), &s1) < 0 || stat(pname(target), &s2) < 0) {
		fprintf(stderr, "mv: cannot locate parent\n");
		return(1);
	}
	if (access(pname(target), 2) < 0) {
		fprintf(stderr, "mv: no write access to %s\n", pname(target));
		return(1);
	}
	if (access(pname(source), 2) < 0) {
		fprintf(stderr, "mv: no write access to %s\n", pname(source));
		return(1);
	}
	if (s1.st_dev != s2.st_dev) {
		fprintf(stderr, "mv: cannot move directories across devices\n");
		return(1);
	}
	if (s1.st_ino != s2.st_ino) {
		char dst[MAXN+5];

		if (chkdot(source) || chkdot(target)) {
			fprintf(stderr, "mv: Sorry, path names including %s aren't allowed\n", DOTDOT);
			return(1);
		}
		stat(source, &s1);
		if (check(pname(target), s1.st_ino))
			return(1);
		for (i = 1; i <= NSIG; i++)
			signal(i, SIG_IGN);
		if (link(source, target) < 0) {
			fprintf(stderr, "mv: cannot link %s to %s\n", target, source);
			return(1);
		}
		if (unlink(source) < 0) {
			fprintf(stderr, "mv: %s: cannot unlink\n", source);
			unlink(target);
			return(1);
		}
		strcat(dst, target);
		strcat(dst, "/");
		strcat(dst, DOTDOT);
		if (unlink(dst) < 0) {
			fprintf(stderr, "mv: %s: cannot unlink\n", dst);
			if (link(target, source) >= 0)
				unlink(target);
			return(1);
		}
		if (link(pname(target), dst) < 0) {
			fprintf(stderr, "mv: cannot link %s to %s\n",
				dst, pname(target));
			if (link(pname(source), dst) >= 0)
				if (link(target, source) >= 0)
					unlink(target);
			return(1);
		}
		return(0);
	}
	if (link(source, target) < 0) {
		fprintf(stderr, "mv: cannot link %s and %s\n",
			source, target);
		return(1);
	}
	if (unlink(source) < 0) {
		fprintf(stderr, "mv: ?? cannot unlink %s\n", source);
		return(1);
	}
	return(0);
}

char *
pname(name)
register char *name;
{
	register c;
	register char *p, *q;
	static	char buf[MAXN];

	p = q = buf;
	while (c = *p++ = *name++)
		if (c == DELIM)
			q = p-1;
	if (q == buf && *q == DELIM)
		q++;
	*q = 0;
	return buf[0]? buf : DOT;
}

char *
dname(name)
register char *name;
{
	register char *p;

	p = name;
	while (*p)
		if (*p++ == DELIM && *p)
			name = p;
	return name;
}

check(spth, dinode)
char *spth;
ino_t dinode;
{
	char nspth[MAXN];
	struct stat sbuf;

	sbuf.st_ino = 0;

	strcpy(nspth, spth);
	while (sbuf.st_ino != ROOTINO) {
		if (stat(nspth, &sbuf) < 0) {
			fprintf(stderr, "mv: cannot access %s\n", nspth);
			return(1);
		}
		if (sbuf.st_ino == dinode) {
			fprintf(stderr, "mv: cannot move a directory into itself\n");
			return(1);
		}
		if (strlen(nspth) > MAXN-2-sizeof(DOTDOT)) {
			fprintf(stderr, "mv: name too long\n");
			return(1);
		}
		strcat(nspth, SDELIM);
		strcat(nspth, DOTDOT);
	}
	return(0);
}

chkdot(s)
register char *s;
{
	do {
		if (strcmp(dname(s), DOTDOT) == 0)
			return(1);
		s = pname(s);
	} while (strcmp(s, DOT) != 0 && strcmp(s, SDELIM) != 0);
	return(0);
}
