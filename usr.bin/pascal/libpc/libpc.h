/* Copyright (c) 1979 Regents of the University of California */

/* static char sccsid[] = "@(#)libpc.h 1.2 3/7/81"; */

extern FILE *ACTFILE();
extern long *ADDT();
extern long CARD();
extern char CHR();
extern long CLCK();
extern long *CTTOT();
extern long ERROR();
extern long EXPO();
extern char *FNIL();
extern long FCALL();
extern struct formalrtn *FSAV();
extern long FRET();
extern struct iorec *GETNAME();
extern bool IN();
extern bool INCT();
extern double LN();
extern long MAX();
extern long *MULT();
extern char *NAM();
extern char *NIL();
extern long PRED();
extern double RANDOM();
extern char READC();
extern long READ4();
extern long READE();
extern double READ8();
extern bool RELNE();
extern bool RELEQ();
extern bool RELSLT();
extern bool RELSLE();
extern bool RELSGT();
extern bool RELSGE();
extern bool RELGLT();
extern bool RELGLE();
extern bool RELGGT();
extern bool RELGGE();
extern long ROUND();
extern long RANG4();
extern long RSNG4();
extern long SCLCK();
extern long SEED();
extern double SQRT();
extern long SUBSC();
extern long SUBSCZ();
extern long *SUBT();
extern long SUCC();
extern long TELL();
extern bool TEOF();
extern bool TEOLN();
extern long TRUNC();
extern struct iorec *UNIT();
