/*
 * libtu/optparser.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_OPTPARSER_H
#define LIBTU_OPTPARSER_H

#include "types.h"


#define OPT_ID_NOSHORT_FLAG 0x10000
#define OPT_ID_RESERVED_FLAG 0x20000

#define OPT_ID(X)            ((X)|OPT_ID_NOSHORT_FLAG)
#define OPT_ID_RESERVED(X)    ((X)|OPT_ID_RESERVED_FLAG)

/* OPTP_CHAIN is the normal behavior, i.e. single-letter options can be
 *        "chained" together: 'lr -lR'. Use for normal command line programs.
 * OPTP_MIDLONG allows '-display foo' -like args but disables chaining
 *         of single-letter options. X programs should probably use this.
 * OPTP_IMMEDIATE allows immediate arguments (-I/usr/include) (and disables
 *        chaining and midlong options).
 * OPTP_NO_DASH is the same as OPTP_CHAIN but allows the dash to be omitted
 *         for 'tar xzf foo' -like behavior.
 * Long '--foo=bar' options are supported in all of the modes.
 */

enum{
    OPTP_CHAIN=0,
    OPTP_DEFAULT=0,
    OPTP_MIDLONG=1,
    OPTP_IMMEDIATE=2,
    OPTP_NO_DASH=3
};

enum{
    OPT_ARG=1,                    /* option has an argument                    */
    OPT_OPT_ARG=3                /* option may have an argument                */
};


#define END_OPTPARSEROPTS {0, NULL, 0, NULL, NULL}

typedef struct _OptParserOpt{
    int optid;
    const char *longopt;
    int    flags;
    const char *argname;
    const char *descr;
} OptParserOpt;

enum{
    OPT_ID_END=0,
    OPT_ID_ARGUMENT=1,

    E_OPT_INVALID_OPTION=-1,
    E_OPT_INVALID_CHAIN_OPTION=-2,
    E_OPT_SYNTAX_ERROR=-3,
    E_OPT_MISSING_ARGUMENT=-4,
    E_OPT_UNEXPECTED_ARGUMENT=-5
};


extern void optparser_init(int argc, char *const argv[], int mode,
                           const OptParserOpt *opts);

extern void optparser_printhelp(int mode, const OptParserOpt *opts);

extern int  optparser_get_opt();
extern const char* optparser_get_arg();
extern void optparser_print_error();

#endif /* LIBTU_OPTPARSER_H */
