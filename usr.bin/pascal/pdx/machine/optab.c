/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)optab.c 1.1 1/18/82";

/*
 * px opcode table
 */

#include "optab.h"

OPTAB optab[] ={
	{ "badop00" },
	{ "nodump", PSUBOP, LWORD, HWORD, STRING },
	{ "beg", PSUBOP, LWORD, LWORD, LWORD, HWORD, STRING },
	{ "end" },
	{ "call", DISP, ADDR4 },
	{ "fcall" },
	{ "frtn", PSUBOP },
	{ "fsav", SUBOP, LWORD },
	{ "sdup2" },
	{ "sdup4" },
	{ "tra", ADDR2 },
	{ "tra4", ADDR4 },
	{ "goto", DISP, ADDR4 },
	{ "lino", PSUBOP },
	{ "push", PSUBOP },
	{ "badopnn" },
	{ "if", ADDR2 },
	{ "rel2", SUBOP },
	{ "rel4", SUBOP },
	{ "rel24", SUBOP },
	{ "rel42", SUBOP },
	{ "rel8", SUBOP },
	{ "relg", SUBOP, VLEN },
	{ "relt", SUBOP, VLEN },
	{ "rel28", SUBOP },
	{ "rel48", SUBOP },
	{ "rel82", SUBOP },
	{ "rel84", SUBOP },
	{ "and" },
	{ "or" },
	{ "not" },
	{ "badopnn" },
	{ "as2" },
	{ "as4" },
	{ "as24" },
	{ "as42" },
	{ "as21" },
	{ "as41" },
	{ "as28" },
	{ "as48" },
	{ "as8" },
	{ "as" },
	{ "inx2p2", PSUBOP },
	{ "inx4p2", PSUBOP },
	{ "inx2", PSUBOP, HWORD, HWORD },
	{ "inx4", PSUBOP, HWORD, HWORD },
	{ "off", PSUBOP },
	{ "nil" },
	{ "add2" },
	{ "add4" },
	{ "add24" },
	{ "add42" },
	{ "add28" },
	{ "add48" },
	{ "add82" },
	{ "add84" },
	{ "sub2" },
	{ "sub4" },
	{ "sub24" },
	{ "sub42" },
	{ "sub28" },
	{ "sub48" },
	{ "sub82" },
	{ "sub84" },
	{ "mul2" },
	{ "mul4" },
	{ "mul24" },
	{ "mul42" },
	{ "mul28" },
	{ "mul48" },
	{ "mul82" },
	{ "mul84" },
	{ "abs2" },
	{ "abs4" },
	{ "abs8" },
	{ "badopnn" },
	{ "neg2" },
	{ "neg4" },
	{ "neg8" },
	{ "badopnn" },
	{ "div2" },
	{ "div4" },
	{ "div24" },
	{ "div42" },
	{ "mod2" },
	{ "mod4" },
	{ "mod24" },
	{ "mod42" },
	{ "add8" },
	{ "sub8" },
	{ "mul8" },
	{ "dvd8" },
	{ "stoi" },
	{ "stod" },
	{ "itod" },
	{ "itos" },
	{ "dvd2" },
	{ "dvd4" },
	{ "dvd24" },
	{ "dvd42" },
	{ "dvd28" },
	{ "dvd48" },
	{ "dvd82" },
	{ "dvd84" },
	{ "rv1", DISP, ADDR2 },
	{ "rv14", DISP, ADDR2 },
	{ "rv2", DISP, ADDR2 },
	{ "rv24", DISP, ADDR2 },
	{ "rv4", DISP, ADDR2 },
	{ "rv8", DISP, ADDR2 },
	{ "rv", PSUBOP, DISP, ADDR2 },
	{ "lv", DISP, HWORD },
	{ "lrv1", DISP, ADDR4 },
	{ "lrv14", DISP, ADDR4 },
	{ "lrv2", DISP, ADDR4 },
	{ "lrv24", DISP, ADDR4 },
	{ "lrv4", DISP, ADDR4 },
	{ "lrv8", DISP, ADDR4 },
	{ "lrv", PSUBOP, DISP, ADDR4 },
	{ "llv", DISP, ADDR4 },
	{ "ind1" },
	{ "ind14" },
	{ "ind2" },
	{ "ind24" },
	{ "ind4" },
	{ "ind8" },
	{ "ind" },
	{ "badopnn" },
	{ "con1", SUBOP },
	{ "con14", SUBOP },
	{ "con2", HWORD },
	{ "con24", HWORD },
	{ "con4", LWORD },
	{ "con8", LWORD, LWORD },
	{ "con", HWORD },
	{ "lvcon", PSUBOP, STRING },
	{ "rang2", HWORD, HWORD },
	{ "rang42", HWORD, HWORD },
	{ "rsng2", HWORD },
	{ "rsng42", HWORD },
	{ "rang4", LWORD, LWORD },
	{ "rang24", LWORD, LWORD },
	{ "rsng4", LWORD },
	{ "rsng24", LWORD },
	{ "stlim" },
	{ "llimit" },
	{ "buff", PSUBOP },
	{ "halt" },
	{ "badopnn" },
	{ "badopnn" },
	{ "badopnn" },
	{ "badopnn" },
	{ "*ord2" },
	{ "*cong" },
	{ "*conc" },
	{ "*conc4" },
	{ "*abort" },
	{ "pxpbuf", HWORD },
	{ "count", HWORD },
	{ "badopnn" },
	{ "case1op", HWORD },
	{ "case2op", HWORD },
	{ "case4op", HWORD },
	{ "*casebeg" },
	{ "*case1" },
	{ "*case2" },
	{ "*case4" },
	{ "*caseend" },
	{ "addt" },
	{ "subt" },
	{ "mult" },
	{ "inct" },
	{ "cttot", PSUBOP, HWORD, HWORD },
	{ "card", PSUBOP },
	{ "in", PSUBOP, HWORD, HWORD },
	{ "asrt" },
	{ "for1u", HWORD, HWORD, ADDR2 },
	{ "for2u", HWORD, HWORD, ADDR2 },
	{ "for4u", LWORD, LWORD, ADDR2 },
	{ "for1d", HWORD, HWORD, ADDR2 },
	{ "for2d", HWORD, HWORD, ADDR2 },
	{ "for4d", LWORD, LWORD, ADDR2 },
	{ "badopnn" },
	{ "badopnn" },
	{ "reade", ADDR4 },
	{ "read4" },
	{ "readc" },
	{ "read8" },
	{ "readln" },
	{ "eof" },
	{ "eoln" },
	{ "badopnn" },
	{ "writec" },
	{ "writes" },
	{ "writef", PSUBOP },
	{ "writln" },
	{ "page" },
	{ "nam", ADDR4 },
	{ "max", PSUBOP, HWORD },
	{ "min", PSUBOP },
	{ "unit" },
	{ "unitinp" },
	{ "unitout" },
	{ "message" },
	{ "get" },
	{ "put" },
	{ "fnil" },
	{ "badopnn" },
	{ "defname", PSUBOP, HWORD },
	{ "reset" },
	{ "rewrite" },
	{ "file" },
	{ "remove" },
	{ "flush" },
	{ "badopnn" },
	{ "badopnn" },
	{ "pack", PSUBOP, HWORD, HWORD, HWORD },
	{ "unpack", PSUBOP, HWORD, HWORD, HWORD },
	{ "argc" },
	{ "argv", PSUBOP },
	{ "badopnn" },
	{ "badopnn" },
	{ "badopnn" },
	{ "badopnn" },
	{ "clck" },
	{ "wclck" },
	{ "sclck" },
	{ "dispose", PSUBOP },
	{ "new", PSUBOP },
	{ "date" },
	{ "time" },
	{ "undef" },
	{ "atan" },
	{ "cos" },
	{ "exp" },
	{ "ln" },
	{ "sin" },
	{ "sqrt" },
	{ "chr2" },
	{ "chr4" },
	{ "odd2" },
	{ "odd4" },
	{ "pred2" },
	{ "pred4" },
	{ "pred24" },
	{ "succ2" },
	{ "succ4" },
	{ "succ24" },
	{ "seed" },
	{ "random" },
	{ "expo" },
	{ "sqr2" },
	{ "sqr4" },
	{ "sqr8" },
	{ "round" },
	{ "trunc" },
};
