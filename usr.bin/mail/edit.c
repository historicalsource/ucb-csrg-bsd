#

#include "rcv.h"
#include <stdio.h>
#include <sys/stat.h>

/*
 * Mail -- a mail program
 *
 * Perform message editing functions.
 */

static char *SccsId = "@(#)edit.c	1.1 10/8/80";

/*
 * Edit a message list.
 */

editor(msgvec)
	int *msgvec;
{
	char *edname;

	if ((edname = value("EDITOR")) == NOSTR)
		edname = EDITOR;
	return(edit1(msgvec, edname));
}

/*
 * Invoke the visual editor on a message list.
 */

visual(msgvec)
	int *msgvec;
{
	char *edname;

	if ((edname = value("VISUAL")) == NOSTR)
		edname = VISUAL;
	return(edit1(msgvec, edname));
}

/*
 * Edit a message by writing the message into a funnily-named file
 * (which should not exist) and forking an editor on it.
 * We get the editor from the stuff above.
 */

edit1(msgvec, ed)
	int *msgvec;
	char *ed;
{
	register char *cp, *cp2;
	register int c;
	int *ip, pid, mesg, lines;
	unsigned int ms;
	int (*sigint)(), (*sigquit)();
	FILE *ibuf, *obuf;
	char edname[15], nbuf[10];
	struct message *mp;
	extern char tempEdit[];
	off_t fsize(), size;
	struct stat statb;
	long modtime;

	/*
	 * Set signals; locate editor.
	 */

	sigint = signal(SIGINT, SIG_IGN);
	sigquit = signal(SIGQUIT, SIG_IGN);

	/*
	 * Deal with each message to be edited . . .
	 */

	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		mp->m_flag |= MODIFY;

		/*
		 * Make up a name for the edit file of the
		 * form "Message%d" and make sure it doesn't
		 * already exist.
		 */

		cp = &nbuf[10];
		*--cp = 0;
		while (mesg) {
			*--cp = mesg % 10 + '0';
			mesg /= 10;
		}
		cp2 = copy("Message", edname);
		while (*cp2++ = *cp++)
			;
		if (!access(edname, 2)) {
			printf("%s: file exists\n", edname);
			goto out;
		}

		/*
		 * Copy the message into the edit file.
		 */

		close(creat(edname, 0600));
		if ((obuf = fopen(edname, "w")) == NULL) {
			perror(edname);
			goto out;
		}
		if (send(mp, obuf) < 0) {
			perror(edname);
			fclose(obuf);
			remove(edname);
			goto out;
		}
		fflush(obuf);
		if (ferror(obuf)) {
			remove(edname);
			fclose(obuf);
			goto out;
		}
		fclose(obuf);

		/*
		 * Fork/execl the editor on the edit file.
		 */

		if (stat(edname, &statb) < 0)
			modtime = 0;
		modtime = statb.st_mtime;
		pid = vfork();
		if (pid == -1) {
			perror("fork");
			remove(edname);
			goto out;
		}
		if (pid == 0) {
			if (sigint != SIG_IGN)
				signal(SIGINT, SIG_DFL);
			if (sigquit != SIG_IGN)
				signal(SIGQUIT, SIG_DFL);
			execl(ed, ed, edname, 0);
			perror(ed);
			_exit(1);
		}
		while (wait(&mesg) != pid)
			;

		/*
		 * Now copy the message to the end of the
		 * temp file.
		 */

		if (stat(edname, &statb) < 0) {
			perror(edname);
			goto out;
		}
		if (modtime == statb.st_mtime) {
			remove(edname);
			goto out;
		}
		if ((ibuf = fopen(edname, "r")) == NULL) {
			perror(edname);
			remove(edname);
			goto out;
		}
		remove(edname);
		fseek(otf, (long) 0, 2);
		size = fsize(otf);
		mp->m_block = blockof(size);
		mp->m_offset = offsetof(size);
		ms = 0;
		lines = 0;
		while ((c = getc(ibuf)) != EOF) {
			if (c == '\n')
				lines++;
			putc(c, otf);
			if (ferror(otf))
				break;
			ms++;
		}
		mp->m_size = ms;
		mp->m_lines = lines;
		if (ferror(otf))
			perror("/tmp");
		fclose(ibuf);
	}

	/*
	 * Restore signals and return.
	 */

out:
	signal(SIGINT, sigint);
	signal(SIGQUIT, sigquit);
}
