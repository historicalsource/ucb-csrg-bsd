/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)file.c	5.8 (Berkeley) 5/19/88";
#endif

#ifdef FILEC
/*
 * Tenex style file name recognition, .. and more.
 * History:
 *	Author: Ken Greer, Sept. 1975, CMU.
 *	Finally got around to adding to the Cshell., Ken Greer, Dec. 1981.
 */

#include "sh.h"
#include <sgtty.h>
#include <sys/dir.h>
#include <pwd.h>

#define TRUE	1
#define FALSE	0
#define ON	1
#define OFF	0

#define ESC	'\033'

typedef enum {LIST, RECOGNIZE} COMMAND;

int	sortscmp();			/* defined in sh.glob.c */

/*
 * Put this here so the binary can be patched with adb to enable file
 * completion by default.  Filec controls completion, nobeep controls
 * ringing the terminal bell on incomplete expansions.
 */
bool filec = 0;

static
setup_tty(on)
	int on;
{
	struct sgttyb sgtty;
	static struct tchars tchars;	/* INT, QUIT, XON, XOFF, EOF, BRK */

	if (on) {
		(void) ioctl(SHIN, TIOCGETC, (char *)&tchars);
		tchars.t_brkc = ESC;
		(void) ioctl(SHIN, TIOCSETC, (char *)&tchars);
		/*
		 * This must be done after every command: if
		 * the tty gets into raw or cbreak mode the user
		 * can't even type 'reset'.
		 */
		(void) ioctl(SHIN, TIOCGETP, (char *)&sgtty);
		if (sgtty.sg_flags & (RAW|CBREAK)) {
			 sgtty.sg_flags &= ~(RAW|CBREAK);
			 (void) ioctl(SHIN, TIOCSETP, (char *)&sgtty);
		}
	} else {
		tchars.t_brkc = -1;
		(void) ioctl(SHIN, TIOCSETC, (char *)&tchars);
	}
}

/*
 * Move back to beginning of current line
 */
static
back_to_col_1()
{
	struct sgttyb tty, tty_normal;
	long omask;

	omask = sigblock(sigmask(SIGINT));
	(void) ioctl(SHIN, TIOCGETP, (char *)&tty);
	tty_normal = tty;
	tty.sg_flags &= ~CRMOD;
	(void) ioctl(SHIN, TIOCSETN, (char *)&tty);
	(void) write(SHOUT, "\r", 1);
	(void) ioctl(SHIN, TIOCSETN, (char *)&tty_normal);
	(void) sigsetmask(omask);
}

/*
 * Push string contents back into tty queue
 */
static
pushback(string)
	char *string;
{
	register char *p;
	struct sgttyb tty, tty_normal;
	long omask;

	omask = sigblock(sigmask(SIGINT));
	(void) ioctl(SHOUT, TIOCGETP, (char *)&tty);
	tty_normal = tty;
	tty.sg_flags &= ~ECHO;
	(void) ioctl(SHOUT, TIOCSETN, (char *)&tty);

	for (p = string; *p; p++)
		(void) ioctl(SHOUT, TIOCSTI, p);
	(void) ioctl(SHOUT, TIOCSETN, (char *)&tty_normal);
	(void) sigsetmask(omask);
}

/*
 * Concatenate src onto tail of des.
 * Des is a string whose maximum length is count.
 * Always null terminate.
 */
static
catn(des, src, count)
	register char *des, *src;
	register count;
{

	while (--count >= 0 && *des)
		des++;
	while (--count >= 0)
		if ((*des++ = *src++) == 0)
			 return;
	*des = '\0';
}

/*
 * Like strncpy but always leave room for trailing \0
 * and always null terminate.
 */
static
copyn(des, src, count)
	register char *des, *src;
	register count;
{

	while (--count >= 0)
		if ((*des++ = *src++) == 0)
			return;
	*des = '\0';
}

static char
filetype(dir, file)
	char *dir, *file;
{
	char path[MAXPATHLEN];
	struct stat statb;

	catn(strcpy(path, dir), file, sizeof path);
	if (lstat(path, &statb) == 0) {
		switch(statb.st_mode & S_IFMT) {
		    case S_IFDIR:
			return ('/');

		    case S_IFLNK:
			if (stat(path, &statb) == 0 &&	    /* follow it out */
			   (statb.st_mode & S_IFMT) == S_IFDIR)
				return ('>');
			else
				return ('@');

		    case S_IFSOCK:
			return ('=');

		    default:
			if (statb.st_mode & 0111)
				return ('*');
		}
	}
	return (' ');
}

static struct winsize win;

/*
 * Print sorted down columns
 */
static
print_by_column(dir, items, count)
	char *dir, *items[];
{
	register int i, rows, r, c, maxwidth = 0, columns;

	if (ioctl(SHOUT, TIOCGWINSZ, (char *)&win) < 0 || win.ws_col == 0)
		win.ws_col = 80;
	for (i = 0; i < count; i++)
		maxwidth = maxwidth > (r = strlen(items[i])) ? maxwidth : r;
	maxwidth += 2;			/* for the file tag and space */
	columns = win.ws_col / maxwidth;
	if (columns == 0)
		columns = 1;
	rows = (count + (columns - 1)) / columns;
	for (r = 0; r < rows; r++) {
		for (c = 0; c < columns; c++) {
			i = c * rows + r;
			if (i < count) {
				register int w;

				printf("%s", items[i]);
				cshputchar(dir ? filetype(dir, items[i]) : ' ');
				if (c < columns - 1) {	/* last column? */
					w = strlen(items[i]) + 1;
					for (; w < maxwidth; w++)
						cshputchar(' ');
				}
			}
		}
		cshputchar('\n');
	}
}

/*
 * Expand file name with possible tilde usage
 *	~person/mumble
 * expands to
 *	home_directory_of_person/mumble
 */
static char *
tilde(new, old)
	char *new, *old;
{
	register char *o, *p;
	register struct passwd *pw;
	static char person[40];

	if (old[0] != '~')
		return (strcpy(new, old));

	for (p = person, o = &old[1]; *o && *o != '/'; *p++ = *o++)
		;
	*p = '\0';
	if (person[0] == '\0')
		(void) strcpy(new, value("home"));
	else {
		pw = getpwnam(person);
		if (pw == NULL)
			return (NULL);
		(void) strcpy(new, pw->pw_dir);
	}
	(void) strcat(new, o);
	return (new);
}

/*
 * Cause pending line to be printed
 */
static
retype()
{
	int pending_input = LPENDIN;

	(void) ioctl(SHOUT, TIOCLBIS, (char *)&pending_input);
}

static
beep()
{

	if (adrof("nobeep") == 0)
		(void) write(SHOUT, "\007", 1);
}

/*
 * Erase that silly ^[ and
 * print the recognized part of the string
 */
static
print_recognized_stuff(recognized_part)
	char *recognized_part;
{

	/* An optimized erasing of that silly ^[ */
	switch (strlen(recognized_part)) {

	case 0:				/* erase two characters: ^[ */
		printf("\210\210  \210\210");
		break;

	case 1:				/* overstrike the ^, erase the [ */
		printf("\210\210%s \210", recognized_part);
		break;

	default:			/* overstrike both characters ^[ */
		printf("\210\210%s", recognized_part);
		break;
	}
	flush();
}

/*
 * Parse full path in file into 2 parts: directory and file names
 * Should leave final slash (/) at end of dir.
 */
static
extract_dir_and_name(path, dir, name)
	char *path, *dir, *name;
{
	register char  *p;

	p = rindex(path, '/');
	if (p == NULL) {
		copyn(name, path, MAXNAMLEN);
		dir[0] = '\0';
	} else {
		copyn(name, ++p, MAXNAMLEN);
		copyn(dir, path, p - path);
	}
}

static char *
getentry(dir_fd, looking_for_lognames)
	DIR *dir_fd;
{
	register struct passwd *pw;
	register struct direct *dirp;

	if (looking_for_lognames) {
		if ((pw = getpwent()) == NULL)
			return (NULL);
		return (pw->pw_name);
	}
	if (dirp = readdir(dir_fd))
		return (dirp->d_name);
	return (NULL);
}

static
free_items(items)
	register char **items;
{
	register int i;

	for (i = 0; items[i]; i++)
		free(items[i]);
	free((char *)items);
}

#define FREE_ITEMS(items) { \
	long omask;\
\
	omask = sigblock(sigmask(SIGINT));\
	free_items(items);\
	items = NULL;\
	(void) sigsetmask(omask);\
}

/*
 * Perform a RECOGNIZE or LIST command on string "word".
 */
static
search(word, command, max_word_length)
	char *word;
	COMMAND command;
{
	static char **items = NULL;
	register DIR *dir_fd;
	register numitems = 0, ignoring = TRUE, nignored = 0;
	register name_length, looking_for_lognames;
	char tilded_dir[MAXPATHLEN + 1], dir[MAXPATHLEN + 1];
	char name[MAXNAMLEN + 1], extended_name[MAXNAMLEN+1];
	char *entry;
#define MAXITEMS 1024

	if (items != NULL)
		FREE_ITEMS(items);

	looking_for_lognames = (*word == '~') && (index(word, '/') == NULL);
	if (looking_for_lognames) {
		(void) setpwent();
		copyn(name, &word[1], MAXNAMLEN);	/* name sans ~ */
	} else {
		extract_dir_and_name(word, dir, name);
		if (tilde(tilded_dir, dir) == 0)
			return (0);
		dir_fd = opendir(*tilded_dir ? tilded_dir : ".");
		if (dir_fd == NULL)
			return (0);
	}

again:	/* search for matches */
	name_length = strlen(name);
	for (numitems = 0; entry = getentry(dir_fd, looking_for_lognames); ) {
		if (!is_prefix(name, entry))
			continue;
		/* Don't match . files on null prefix match */
		if (name_length == 0 && entry[0] == '.' &&
		    !looking_for_lognames)
			continue;
		if (command == LIST) {
			if (numitems >= MAXITEMS) {
				printf ("\nYikes!! Too many %s!!\n",
				    looking_for_lognames ?
					"names in password file":"files");
				break;
			}
			if (items == NULL)
				items = (char **) calloc(sizeof (items[1]),
				    MAXITEMS);
			items[numitems] = xalloc((unsigned)strlen(entry) + 1);
			copyn(items[numitems], entry, MAXNAMLEN);
			numitems++;
		} else {			/* RECOGNIZE command */
			if (ignoring && ignored(entry))
				nignored++;
			else if (recognize(extended_name,
			    entry, name_length, ++numitems))
				break;
		}
	}
	if (ignoring && numitems == 0 && nignored > 0) {
		ignoring = FALSE;
		nignored = 0;
		if (looking_for_lognames)
			(void) setpwent();
		else
			rewinddir(dir_fd);
		goto again;
	}

	if (looking_for_lognames)
		(void) endpwent();
	else
		closedir(dir_fd);
	if (numitems == 0)
		return (0);
	if (command == RECOGNIZE) {
		if (looking_for_lognames)
			 copyn(word, "~", 1);
		else
			/* put back dir part */
			copyn(word, dir, max_word_length);
		/* add extended name */
		catn(word, extended_name, max_word_length);
		return (numitems);
	}
	else { 				/* LIST */
		qsort((char *)items, numitems, sizeof(items[1]), sortscmp);
		print_by_column(looking_for_lognames ? NULL : tilded_dir,
		    items, numitems);
		if (items != NULL)
			FREE_ITEMS(items);
	}
	return (0);
}

/*
 * Object: extend what user typed up to an ambiguity.
 * Algorithm:
 * On first match, copy full entry (assume it'll be the only match) 
 * On subsequent matches, shorten extended_name to the first
 * character mismatch between extended_name and entry.
 * If we shorten it back to the prefix length, stop searching.
 */
static
recognize(extended_name, entry, name_length, numitems)
	char *extended_name, *entry;
{

	if (numitems == 1)			/* 1st match */
		copyn(extended_name, entry, MAXNAMLEN);
	else {					/* 2nd & subsequent matches */
		register char *x, *ent;
		register int len = 0;

		x = extended_name;
		for (ent = entry; *x && *x == *ent++; x++, len++)
			;
		*x = '\0';			/* Shorten at 1st char diff */
		if (len == name_length)		/* Ambiguous to prefix? */
			return (-1);		/* So stop now and save time */
	}
	return (0);
}

/*
 * Return true if check matches initial chars in template.
 * This differs from PWB imatch in that if check is null
 * it matches anything.
 */
static
is_prefix(check, template)
	register char *check, *template;
{

	do
		if (*check == 0)
			return (TRUE);
	while (*check++ == *template++);
	return (FALSE);
}

/*
 *  Return true if the chars in template appear at the
 *  end of check, I.e., are it's suffix.
 */
static
is_suffix(check, template)
	char *check, *template;
{
	register char *c, *t;

	for (c = check; *c++;)
		;
	for (t = template; *t++;)
		;
	for (;;) {
		if (t == template)
			return 1;
		if (c == check || *--t != *--c)
			return 0;
	}
}

tenex(inputline, inputline_size)
	char *inputline;
	int inputline_size;
{
	register int numitems, num_read;

	setup_tty(ON);
	while ((num_read = read(SHIN, inputline, inputline_size)) > 0) {
		static char *delims = " '\"\t;&<>()|^%";
		register char *str_end, *word_start, last_char, should_retype;
		register int space_left;
		COMMAND command;

		last_char = inputline[num_read - 1] & 0177;

		if (last_char == '\n' || num_read == inputline_size)
			break;
		command = (last_char == ESC) ? RECOGNIZE : LIST;
		if (command == LIST)
			cshputchar('\n');
		str_end = &inputline[num_read];
		if (last_char == ESC)
			--str_end;		/* wipeout trailing cmd char */
		*str_end = '\0';
		/*
		 * Find LAST occurence of a delimiter in the inputline.
		 * The word start is one character past it.
		 */
		for (word_start = str_end; word_start > inputline; --word_start)
			if (index(delims, word_start[-1]))
				break;
		space_left = inputline_size - (word_start - inputline) - 1;
		numitems = search(word_start, command, space_left);

		if (command == RECOGNIZE) {
			/* print from str_end on */
			print_recognized_stuff(str_end);
			if (numitems != 1)	/* Beep = No match/ambiguous */
				beep();
		}

		/*
		 * Tabs in the input line cause trouble after a pushback.
		 * tty driver won't backspace over them because column
		 * positions are now incorrect. This is solved by retyping
		 * over current line.
		 */
		should_retype = FALSE;
		if (index(inputline, '\t')) {	/* tab char in input line? */
			back_to_col_1();
			should_retype = TRUE;
		}
		if (command == LIST)		/* Always retype after a LIST */
			should_retype = TRUE;
		if (should_retype)
			printprompt();
		pushback(inputline);
		if (should_retype)
			retype();
	}
	setup_tty(OFF);
	return (num_read);
}

static
ignored(entry)
	register char *entry;
{
	struct varent *vp;
	register char **cp;

	if ((vp = adrof("fignore")) == NULL || (cp = vp->vec) == NULL)
		return (FALSE);
	for (; *cp != NULL; cp++)
		if (is_suffix(entry, *cp))
			return (TRUE);
	return (FALSE);
}
#endif FILEC
