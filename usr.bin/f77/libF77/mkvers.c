/*-
 * Copyright (c) 1980 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)mkvers.c	1.2 (Berkeley) 4/12/91";
#endif /* not lint */

char id_mkvers[] = "@(#)mkvers.c	1.2 4/12/91";

/*
 * extract sccs id strings from source files
 * first arg is lib name.
 * Put them in Version.c
 */

#include	<stdio.h>

#define SCCS_ID		"@(#)"
#define VERSION		"Version.c"

main(argc, argv)
int argc; char **argv;
{
	char buf[256];
	char *s, *e;
	char *index(), *ctime();
	long t;
	FILE *V, *fdopen();

	V = stdout; /* fdopen(creat(VERSION, 0644), "w"); */
	if (!V)
	{
		perror("mkvers");
		exit(1);
	}
	if (argc > 1 && argv[1][0] != '.')
	{
		fprintf(V, "char *");
		for (s = argv[1]; *s && *s != '.'; s++)
			fputc(*s, V);
		fprintf(V, "_id[] = {\n");
	}
	else
		fprintf(V, "char *sccs_id[] = {\n");
	if (argc-- > 1)
	{
		time(&t);
		s = ctime(&t) + 4;
		s[20] = '\0';
		fprintf(V, "\t\"%s%s\t%s\",\n", SCCS_ID, *++argv, s);
	}
	while (--argc)
	{
		if (freopen(*++argv, "r", stdin) == NULL)
		{
			perror(*argv);
			continue;
		}
		while(gets(buf))
		{
			s = buf;
			while(s = index(s, '@'))
				if (strncmp(s, SCCS_ID, 4) == 0)
					break;
			if (s)
			{
				e = index(s, '"');
				if (e)
					*e = '\0';
				fprintf(V, "\t\"%s\",\n", s);
				break;
			}
		}
		if (feof(stdin))
			fprintf(stderr, "%s: no sccs id string\n", *argv);
	}
	fprintf(V, "};\n");
	fclose(V);
	fflush(stdout);
	fflush(stderr);
}
