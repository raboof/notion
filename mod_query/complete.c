/*
 * ion/mod_query/complete.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include <ioncore/common.h>
#include "complete.h"
#include "edln.h"
#include "wedln.h"


static int str_common_part_l(const char *p1, const char *p2)
{
    int i=0;
    
    while(1){
        if(*p1=='\0' || *p1!=*p2)
            break;
        p1++; p2++; i++;
    }
    
    return i;
}


/* Get length of part common to all completions
 * and remove duplicates
 */
static int get_common_part_rmdup(char **completions, int *ncomp)
{
    int i, j, c=INT_MAX, c2;
    
    for(i=0, j=1; j<*ncomp; j++){
        c2=str_common_part_l(completions[i], completions[j]);
        if(c2<c)
            c=c2;

        if(completions[i][c2]=='\0' && completions[j][c2]=='\0'){
            free(completions[j]);
            completions[j]=NULL;
            continue;
        }
        i++;
        if(i!=j){
            completions[i]=completions[j];
            completions[j]=NULL;
        }
    }
    *ncomp=i+1;
    
    return c;
}


static int compare(const void *p1, const void *p2)
{
    const char **v1, **v2;
    
    v1=(const char **)p1;
    v2=(const char **)p2;
    return strcmp(*v1, *v2);
}


void edln_complete(Edln *edln)
{
    char *p;
    int len;
    
    if(edln->completion_handler==NULL)
        return;
    
    len=edln->point;
    
    p=ALLOC_N(char, len+1);
    
    if(p==NULL)
        return;
    
    memcpy(p, edln->p, len);
    p[len]='\0';
    
    edln->completion_handler(edln->uiptr, p);
    
    free(p);
}


int edln_do_completions(Edln *edln, char **completions, int ncomp,
                         const char *beg)
{
    int len;
    
    if(ncomp==0){
        return 0;
    }else if(ncomp==1){
        len=strlen(completions[0]);
    }else{
        qsort(completions, ncomp, sizeof(char**), compare);
        len=get_common_part_rmdup(completions, &ncomp);
    }
    
    edln_kill_to_bol(edln);
    
    if(beg!=NULL)
        edln_insstr(edln, beg);
    
    if(len!=0)
        edln_insstr_n(edln, completions[0], len);
    
    return ncomp;
}

