/*	clist.h	3.1	10/8/80	*/

/*
 * Raw structures for the character list routines.
 */
struct cblock {
	struct cblock *c_next;
	char	c_info[CBSIZE];
};
struct	cblock	cfree[];
