/*
 * libtu/tester3.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <stdio.h>

#include "util.h"
#include "misc.h"
#include "optparser.h"


static const char usage[]=
    "Usage: $p [options]\n"
    "\n"
    "Where options are:\n"
    "$o\n";


static OptParserOpt opts[]={
    {'o',    "opt", OPT_ARG, "OPTION", "foo bar baz quk asdf jklö äölk dfgh quik aaaa bbbb cccc dddd eeee ffff"},
    {'f',    "file", OPT_ARG, "FILE", "asdfsadlfölökjasdflökjasdflkjöasdflkjöas dlöfjkasdfölkjasdfölkjasdfasdflöjasdfkasödjlfkasdlföjasdölfjkölkasjdfasdfölkjasd asdöljfasöldf  asdölfköasdlf asfdlök asdföljkadsfölasdfölasdölkfjasdölfasödlflöskflasdföaölsdf"},
    {'v',    "view",    0, NULL, "asfasdf"},
    {'z',    "zip", 0, NULL, "asdfasdf"},
    {'x',    "extract", 0, NULL, "asdfasdf"},
    {0, NULL, 0, NULL, NULL}
};

static OptParserCommonInfo tester3_cinfo={
    NULL,
    usage,
    NULL
};


int main(int argc, char *argv[])
{
    int opt;

    libtu_init(argv[0]);

    optparser_init(argc, argv, OPTP_NO_DASH, opts, &tester3_cinfo);

    while((opt=optparser_get_opt())){
        switch(opt){
        case 'o':
            printf("opt: %s\n", optparser_get_arg());
            break;
        case 'f':
            printf("file: %s\n", optparser_get_arg());
            break;
        case 'v':
            printf("view\n");
            break;
        case 'z':
            printf("zip\n");
            break;
        case 'x':
            printf("extract\n");
            break;
        default:
            optparser_print_error();
            return 1;
        }
    }
    return 0;
}

