/*
 * Determine the leftmost index of the character
 * in the string.
 */

static char *SccsId = "@(#)index.c	2.1 7/1/81";

char *
index(str, ch)
	char *str;
{
	register char *cp;
	register int c;

	for (c = ch, cp = str; *cp; cp++)
		if (*cp == c)
			return(cp);
	return(NOSTR);
}
