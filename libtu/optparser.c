/*
 * libtu/optparser.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "misc.h"
#include "optparser.h"
#include "output.h"
#include "private.h"


#define O_ARGS(o)       (o->flags&OPT_OPT_ARG)
#define O_ARG(o)        (o->flasg&OPT_ARG)
#define O_OPT_ARG(o)    (O_ARGS(o)==OPT_OPT_ARG)
#define O_ID(o)         (o->optid)


static const OptParserOpt *o_opts=NULL;
static char *const *o_current=NULL;
static int o_left=0;
static const char* o_chain_ptr=NULL;
static int o_args_left=0;
static const char*o_tmp=NULL;
static int o_error=0;
static int o_mode=OPTP_CHAIN;


/* */


void optparser_init(int argc, char *const argv[], int mode,
                    const OptParserOpt *opts)
{
    o_mode=mode;
    o_opts=opts;
    o_current=argv+1;
    o_left=argc-1;
    o_chain_ptr=NULL;
    o_args_left=0;
    o_tmp=NULL;
}


/* */


static const OptParserOpt *find_chain_opt(char p, const OptParserOpt *o)
{
    for(;O_ID(o);o++){
        if((O_ID(o)&~OPT_ID_RESERVED_FLAG)==p)
            return o;
    }
    return NULL;
}


static bool is_option(const char *p)
{
     if(p==NULL)
          return FALSE;
     if(*p++!='-')
          return FALSE;
     if(*p++!='-')
          return FALSE;
     if(*p=='\0')
          return FALSE;
     return TRUE;
}


/* */

enum{
    SHORT, MIDLONG, LONG
};


int optparser_get_opt()
{
#define RET(X) return o_tmp=p, o_error=X
    const char *p, *p2=NULL;
    bool dash=TRUE;
    int type=SHORT;
    const OptParserOpt *o;
    int l;

    while(o_args_left)
        optparser_get_arg();

    o_tmp=NULL;

    /* Are we doing a chain (i.e. opt. of style 'tar xzf')? */
    if(o_chain_ptr!=NULL){
        p=o_chain_ptr++;
        if(!*o_chain_ptr)
            o_chain_ptr=NULL;

        o=find_chain_opt(*p, o_opts);

        if(o==NULL)
            RET(E_OPT_INVALID_CHAIN_OPTION);

        goto do_arg;
    }

    if(o_left<1)
        return OPT_ID_END;

    o_left--;
    p=*o_current++;

    if(*p!='-'){
        dash=FALSE;
        if(o_mode!=OPTP_NO_DASH)
            RET(OPT_ID_ARGUMENT);
        p2=p;
    }else if(*(p+1)=='-'){
        /* --foo */
        if(*(p+2)=='\0'){
            /* -- arguments */
            o_args_left=o_left;
            RET(OPT_ID_ARGUMENT);
        }
        type=LONG;
        p2=p+2;
    }else{
        /* -foo */
        if(*(p+1)=='\0'){
            /* - */
            o_args_left=1;
            RET(OPT_ID_ARGUMENT);
        }
        if(*(p+2)!='\0' && o_mode==OPTP_MIDLONG)
            type=MIDLONG;

        p2=p+1;
    }

    o=o_opts;

    for(; O_ID(o); o++){
        if(type==LONG){
            /* Do long option (--foo=bar) */
            if(o->longopt==NULL)
                continue;
            l=strlen(o->longopt);
            if(strncmp(p2, o->longopt, l)!=0)
                continue;

            if(p2[l]=='\0'){
                if(O_ARGS(o)==OPT_ARG)
                     RET(E_OPT_MISSING_ARGUMENT);
                return O_ID(o);
            }else if(p2[l]=='='){
                if(!O_ARGS(o))
                     RET(E_OPT_UNEXPECTED_ARGUMENT);
                if(p2[l+1]=='\0')
                     RET(E_OPT_MISSING_ARGUMENT);
                o_tmp=p2+l+1;
                o_args_left=1;
                return O_ID(o);
            }
            continue;
        }else if(type==MIDLONG){
            if(o->longopt==NULL)
                continue;

            if(strcmp(p2, o->longopt)!=0)
                continue;
        }else{ /* type==SHORT */
            if(*p2!=(O_ID(o)&~OPT_ID_RESERVED_FLAG))
                continue;

            if(*(p2+1)!='\0'){
                if(o_mode==OPTP_CHAIN || o_mode==OPTP_NO_DASH){
                    /*valid_chain(p2+1, o_opts)*/
                    o_chain_ptr=p2+1;
                    p++;
                }else if(o_mode==OPTP_IMMEDIATE){
                    if(!O_ARGS(o)){
                        if(*(p2+1)!='\0')
                            RET(E_OPT_UNEXPECTED_ARGUMENT);
                    }else{
                        if(*(p2+1)=='\0')
                            RET(E_OPT_MISSING_ARGUMENT);
                        o_tmp=p2+1;
                        o_args_left=1;
                    }
                    return O_ID(o);
                }else{
                    RET(E_OPT_SYNTAX_ERROR);
                }
            }
        }

    do_arg:

        if(!O_ARGS(o))
            return O_ID(o);

        if(!o_left || is_option(*o_current)){
            if(O_ARGS(o)==OPT_OPT_ARG)
                return O_ID(o);
            RET(E_OPT_MISSING_ARGUMENT);
        }

        o_args_left=1;
        return O_ID(o);
    }

    if(dash)
        RET(E_OPT_INVALID_OPTION);

    RET(OPT_ID_ARGUMENT);
#undef RET
}


/* */


const char* optparser_get_arg()
{
    const char *p;

    if(o_tmp!=NULL){
        /* If o_args_left==0, then were returning an invalid option
         * otherwise an immediate argument (e.g. -funsigned-char
         * where '-f' is the option and 'unsigned-char' the argument)
         */
        if(o_args_left>0)
            o_args_left--;
        p=o_tmp;
        o_tmp=NULL;
        return p;
    }

    if(o_args_left<1 || o_left<1)
        return NULL;

    o_left--;
    o_args_left--;
    return *o_current++;
}


/* */

static void warn_arg(const char *e)
{
    const char *p=optparser_get_arg();

    if(p==NULL)
        warn("%s (null)", e);
    else
        warn("%s \'%s\'", e, p);
}


static void warn_opt(const char *e)
{
    if(o_tmp!=NULL && o_chain_ptr!=NULL)
        warn("%s \'-%c\'", e, *o_tmp);
    else
        warn_arg(e);
}


void optparser_print_error()
{
    switch(o_error){
    case E_OPT_INVALID_OPTION:
    case E_OPT_INVALID_CHAIN_OPTION:
        warn_opt(TR("Invalid option"));
        break;

    case E_OPT_SYNTAX_ERROR:
        warn_arg(TR("Syntax error while parsing"));
        break;

    case E_OPT_MISSING_ARGUMENT:
        warn_opt(TR("Missing argument to"));
        break;

    case E_OPT_UNEXPECTED_ARGUMENT:
        warn_opt(TR("No argument expected:"));
        break;

    case OPT_ID_ARGUMENT:
        warn(TR("Unexpected argument"));
        break;

    default:
        warn(TR("(unknown error)"));
    }

    o_tmp=NULL;
    o_error=0;
}


/* */


static uint opt_w(const OptParserOpt *opt, bool midlong)
{
    uint w=0;

    if((opt->optid&OPT_ID_NOSHORT_FLAG)==0){
        w+=2; /* "-o" */
        if(opt->longopt!=NULL)
            w+=2; /* ", " */
    }

    if(opt->longopt!=NULL)
        w+=strlen(opt->longopt)+(midlong ? 1 : 2);

    if(O_ARGS(opt)){
        if(opt->argname==NULL)
            w+=4;
        else
            w+=1+strlen(opt->argname); /* "=ARG" or " ARG" */

        if(O_OPT_ARG(opt))
            w+=2; /* [ARG] */
    }

    return w;
}


#define TERM_W 80
#define OFF1 2
#define OFF2 2
#define SPACER1 "  "
#define SPACER2 "  "

static void print_opt(const OptParserOpt *opt, bool midlong,
                      uint maxw, uint tw)
{
    FILE *f=stdout;
    const char *p, *p2, *p3;
    uint w=0;

    fprintf(f, SPACER1);

    /* short opt */

    if((O_ID(opt)&OPT_ID_NOSHORT_FLAG)==0){
        fprintf(f, "-%c", O_ID(opt)&~OPT_ID_RESERVED_FLAG);
        w+=2;

        if(opt->longopt!=NULL){
            fprintf(f, ", ");
            w+=2;
        }
    }

    /* long opt */

    if(opt->longopt!=NULL){
        if(midlong){
            w++;
            fprintf(f, "-%s", opt->longopt);
        }else{
            w+=2;
            fprintf(f, "--%s", opt->longopt);
        }
        w+=strlen(opt->longopt);
    }

    /* arg */

    if(O_ARGS(opt)){
        w++;
        if(opt->longopt!=NULL && !midlong)
            putc('=', f);
        else
            putc(' ', f);

        if(O_OPT_ARG(opt)){
            w+=2;
            putc('[', f);
        }

        if(opt->argname!=NULL){
            fprintf(f, "%s", opt->argname);
            w+=strlen(opt->argname);
        }else{
            w+=3;
            fprintf(f, "ARG");
        }

        if(O_OPT_ARG(opt))
            putc(']', f);
    }

    while(w++<maxw)
        putc(' ', f);

    /* descr */

    p=p2=opt->descr;

    if(p==NULL){
        putc('\n', f);
        return;
    }

    fprintf(f, SPACER2);

    maxw+=OFF1+OFF2;
    tw-=maxw;

    while(strlen(p)>tw){
        p3=p2=p+tw-2;

        while(*p2!=' ' && p2!=p)
            p2--;

        while(*p3!=' ' && *p3!='\0')
            p3++;

        if((uint)(p3-p2)>tw){
            /* long word - just wrap */
            p2=p+tw-2;
        }

        writef(f, p, p2-p);
        if(*p2==' ')
            putc('\n', f);
        else
            fprintf(f, "\\\n");

        p=p2+1;

        w=maxw;
        while(w--)
            putc(' ', f);
    }

    fprintf(f, "%s\n", p);
}


void optparser_printhelp(int mode, const OptParserOpt *opts)
{
    uint w, maxw=0;
    const OptParserOpt *o;
    bool midlong=mode&OPTP_MIDLONG;

    o=opts;
    for(; O_ID(o); o++){
        w=opt_w(o, midlong);
        if(w>maxw)
            maxw=w;
    }

    o=opts;

    for(; O_ID(o); o++)
        print_opt(o, midlong, maxw, TERM_W);
}
