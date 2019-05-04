/*
 * libtu/prefix.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <string.h>
#include "misc.h"
#include "locale.h"
#include "output.h"

static char *the_prefix=NULL;

void prefix_set(const char *binloc, const char *dflt)
{
    int i=strlen(binloc);
    int j=strlen(dflt);

    if(binloc[0]!='/')
        die(TR("This relocatable binary should be started with an absolute path."));
    
    while(i>0 && j>0){
        if(binloc[i-1]!=dflt[j-1])
            break;
        i--;
        j--;
    }
    
    the_prefix=scopyn(binloc, i);
    
}


char *prefix_add(const char *s)
{
    if(the_prefix==NULL)
        return scopy(s);
    else
        return scat3(the_prefix, "/", s);
}


bool prefix_wrap_simple(bool (*fn)(const char *s), const char *s)
{
    bool ret=FALSE;
    
    if(the_prefix==NULL){
        ret=fn(s);
    }else{
        char *s2=prefix_add(s);
        if(s2!=NULL){
            ret=fn(s2);
            free(s2);
        }
    }
    
    return ret;
}
