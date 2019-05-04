/*
 * libtu/output.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#if HAS_SYSTEM_ASPRINTF
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <string.h>

#include "misc.h"
#include "output.h"
#include "util.h"
#include "private.h"

#if !HAS_SYSTEM_ASPRINTF
#include "snprintf_2.2/snprintf.c"
#endif


static void default_warn_handler(const char *message);


static bool verbose_mode=FALSE;
static int verbose_indent_lvl=0;
static bool progname_enable=TRUE;
static WarnHandler *current_warn_handler=default_warn_handler;

#define INDENTATOR_LENGTH 4

static char indentator[]={' ', ' ', ' ', ' '};

static void do_dispatch_message(const char *message);


void verbose(const char *p, ...)
{
    va_list args;
    
    va_start(args, p);
    
    verbose_v(p, args);
    
    va_end(args);
}
           

void verbose_v(const char *p, va_list args)
{
    int i;
    
    if(verbose_mode){
        for(i=0; i<verbose_indent_lvl; i++)
            writef(stdout, indentator, INDENTATOR_LENGTH);
        
        vprintf(p, args);
        fflush(stdout);
    }
}


void verbose_enable(bool enable)
{
    verbose_mode=enable;
}


int verbose_indent(int depth)
{
    int old=verbose_indent_lvl;
    
    if(depth>=0)
        verbose_indent_lvl=depth;
    
    return old;
}
        

void warn_progname_enable(bool enable)
{
    progname_enable=enable;
}


static void put_prog_name()
{
    const char*progname;
    
    if(!progname_enable)
        return;
    
    progname=libtu_progname();
    
    if(progname==NULL)
        return;
    
    fprintf(stderr, "%s: ", (char*)progname);
}

/* warn
 */


static void fallback_warn()
{
    put_prog_name();
    fprintf(stderr, TR("Oops. Error string compilation failed: %s"),
            strerror(errno));
}
    
    
#define CALL_V(NAME, ARGS) \
    do { va_list args; va_start(args, p); NAME ARGS; va_end(args); } while(0)

#define DO_DISPATCH(NAME, ARGS) \
    do{ \
        char *msg; \
        if((msg=NAME ARGS)!=NULL){ \
            do_dispatch_message(msg); \
            free(msg);\
        }else{ \
            fallback_warn(); \
        } \
    }while(0)


void libtu_asprintf(char **ret, const char *p, ...)
{
    *ret=NULL;
    CALL_V(vasprintf, (ret, p, args));
    if(*ret==NULL)
        warn_err();
}


void libtu_vasprintf(char **ret, const char *p, va_list args)
{
    *ret=NULL;
    vasprintf(ret, p, args);
    if(*ret==NULL)
        warn_err();
}


void warn(const char *p, ...)
{
    CALL_V(warn_v, (p, args));
}


void warn_obj(const char *obj, const char *p, ...)
{
    CALL_V(warn_obj_v, (obj, p, args));
}


void warn_obj_line(const char *obj, int line, const char *p, ...)
{
    CALL_V(warn_obj_line_v, (obj, line, p, args));
}


void warn_obj_v(const char *obj, const char *p, va_list args)
{
    warn_obj_line_v(obj, -1, p, args);
}


void warn_v(const char *p, va_list args)
{
    DO_DISPATCH(errmsg_v, (p, args));
}


void warn_obj_line_v(const char *obj, int line, const char *p, va_list args)
{
    DO_DISPATCH(errmsg_obj_line_v, (obj, line, p, args));
}


void warn_err()
{
    DO_DISPATCH(errmsg_err, ());
}


void warn_err_obj(const char *obj)
{
    DO_DISPATCH(errmsg_err_obj, (obj));
}

void warn_err_obj_line(const char *obj, int line)
{
    DO_DISPATCH(errmsg_err_obj_line, (obj, line));
}


/* errmsg
 */

#define CALL_V_RET(NAME, ARGS) \
    char *ret; va_list args; va_start(args, p); ret=NAME ARGS; \
    va_end(args); return ret;


char* errmsg(const char *p, ...)
{
    CALL_V_RET(errmsg_v, (p, args));
}


char *errmsg_obj(const char *obj, const char *p, ...)
{
    CALL_V_RET(errmsg_obj_v, (obj, p, args));
}


char *errmsg_obj_line(const char *obj, int line, const char *p, ...)
{
    CALL_V_RET(errmsg_obj_line_v, (obj, line, p, args));
}


char* errmsg_obj_v(const char *obj, const char *p, va_list args)
{
    return errmsg_obj_line_v(obj, -1, p, args);
}


char *errmsg_v(const char *p, va_list args)
{
    char *res;
    libtu_vasprintf(&res, p, args);
    return res;
}


char *errmsg_obj_line_v(const char *obj, int line, const char *p, va_list args)
{
    char *res1=NULL, *res2, *res3;
    
    if(obj!=NULL){
        if(line>0)
            libtu_asprintf(&res1, "%s:%d: ", obj, line);
        else        
            libtu_asprintf(&res1, "%s: ", obj);
    }else{
        if(line>0)
            libtu_asprintf(&res1, "%d: ", line);
    }
    libtu_vasprintf(&res2, p, args);
    if(res1!=NULL){
        if(res2==NULL)
            return NULL;
        res3=scat(res1, res2);
        free(res1);
        free(res2);
        return res3;
    }
    return res2;
}


char *errmsg_err()
{
    char *res;
    libtu_asprintf(&res, "%s\n", strerror(errno));
    return res;
}


char *errmsg_err_obj(const char *obj)
{
    char *res;
    if(obj!=NULL)
        libtu_asprintf(&res, "%s: %s\n", obj, strerror(errno));
    else
        libtu_asprintf(&res, "%s\n", strerror(errno));
    return res;
}


char *errmsg_err_obj_line(const char *obj, int line)
{
    char *res;
    if(obj!=NULL){
        if(line>0)
            libtu_asprintf(&res, "%s:%d: %s\n", obj, line, strerror(errno));
        else
            libtu_asprintf(&res, "%s: %s\n", obj, strerror(errno));
    }else{
        if(line>0)
            libtu_asprintf(&res, "%d: %s\n", line, strerror(errno));
        else
            libtu_asprintf(&res, "%s\n", strerror(errno));
    }
    return res;
}


/* die
 */


void die(const char *p, ...)
{
    set_warn_handler(NULL);
    CALL_V(die_v, (p, args));
}


void die_v(const char *p, va_list args)
{
    set_warn_handler(NULL);
    warn_v(p, args);
    exit(EXIT_FAILURE);
}


void die_obj(const char *obj, const char *p, ...)
{
    set_warn_handler(NULL);
    CALL_V(die_obj_v, (obj, p, args));
}


void die_obj_line(const char *obj, int line, const char *p, ...)
{
    set_warn_handler(NULL);
    CALL_V(die_obj_line_v, (obj, line, p, args));
}


void die_obj_v(const char *obj, const char *p, va_list args)
{
    set_warn_handler(NULL);
    warn_obj_v(obj, p, args);
    exit(EXIT_FAILURE);
}


void die_obj_line_v(const char *obj, int line, const char *p, va_list args)
{
    set_warn_handler(NULL);
    warn_obj_line_v(obj, line, p, args);
    exit(EXIT_FAILURE);
}


void die_err()
{
    set_warn_handler(NULL);
    warn_err();
    exit(EXIT_FAILURE);
}


void die_err_obj(const char *obj)
{
    set_warn_handler(NULL);
    warn_err_obj(obj);
    exit(EXIT_FAILURE);
}


void die_err_obj_line(const char *obj, int line)
{
    set_warn_handler(NULL);
    warn_err_obj_line(obj, line);
    exit(EXIT_FAILURE);
}


static void default_warn_handler(const char *message)
{
    put_prog_name();
    fprintf(stderr, "%s", message);
    putc('\n', stderr);
}


static void do_dispatch_message(const char *message)
{
    if(current_warn_handler!=NULL)
        current_warn_handler(message);
    else
        default_warn_handler(message);
}


WarnHandler *set_warn_handler(WarnHandler *handler)
{
    WarnHandler *old=current_warn_handler;
    current_warn_handler=(handler!=NULL ? handler : default_warn_handler);
    return old;
}
