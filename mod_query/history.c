/*
 * ion/mod_query/history.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <libextl/extl.h>

#include "history.h"


#define HISTORY_SIZE 256


static int hist_head=HISTORY_SIZE;
static int hist_count=0;
static char *hist[HISTORY_SIZE];


int get_index(int i)
{
    if(i<0 || i>=hist_count)
        return -1;
    return (hist_head+i)%HISTORY_SIZE;
}
    

/*EXTL_DOC
 * Push an entry into line editor history.
 */
EXTL_EXPORT
bool mod_query_history_push(const char *str)
{
    char *s=scopy(str);
    
    if(s==NULL)
        return FALSE;
    
    mod_query_history_push_(s);
    
    return TRUE;
}


void mod_query_history_push_(char *str)
{
    if(hist_count>0 && strcmp(hist[hist_head], str)==0)
        return;
    
    hist_head--;
    if(hist_head<0)
        hist_head=HISTORY_SIZE-1;
    
    if(hist_count==HISTORY_SIZE)
        free(hist[hist_head]);
    else
        hist_count++;
    
    hist[hist_head]=str;
}


/*EXTL_DOC
 * Get entry at index \var{n} in line editor history, 0 being the latest.
 */
EXTL_SAFE
EXTL_EXPORT
const char *mod_query_history_get(int n)
{
    int i=get_index(n);
    return (i<0 ? NULL : hist[i]);
}


/*EXTL_DOC
 * Clear line editor history.
 */
EXTL_EXPORT
void mod_query_history_clear()
{
    while(hist_count!=0){
        free(hist[hist_head]);
        hist_count--;
        if(++hist_head==HISTORY_SIZE)
            hist_head=0;
    }
    hist_head=HISTORY_SIZE;
}



static bool match(const char *h, const char *b)
{
    const char *h_;
    
    if(b==NULL)
        return TRUE;
    
    /* Special case: search in any context. */
    if(*b=='*' && *(b+1)==':'){
        b=b+2;
        h_=strchr(h, ':');
        if(h_!=NULL)
            h=h_+1;
    }
        
    return (strncmp(h, b, strlen(b))==0);
}


static const char *skip_colon(const char *s)
{
    const char *p=strchr(s, ':');
    return (p!=NULL ? p+1 : s);
}


/*EXTL_DOC
 * Try to find matching history entry.
 */
EXTL_SAFE
EXTL_EXPORT
int mod_query_history_search(const char *s, int from, bool bwd)
{
    while(1){
        int i=get_index(from);
        if(i<0)
            return -1;
        if(match(hist[i], s))
            return from;
        if(bwd)
            from--;
        else
            from++;
    }
}


uint mod_query_history_complete(const char *s, char ***h_ret)
{
    char **h=ALLOC_N(char *, hist_count);
    int i, n=0;
    
    if(h==NULL)
        return 0;
    
    for(i=0; i<hist_count; i++){
        uint j=get_index(i);
        if(j<0)
            break;
        if(match(hist[j], s)){
            h[n]=scopy(skip_colon(hist[j]));
            if(h[n]!=NULL)
                n++;
        }
    }
    
    if(n==0)
        free(h);
    else
        *h_ret=h;
    
    return n;
}


/*EXTL_DOC
 * Return table of history entries.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_query_history_table()
{
    ExtlTab tab=extl_create_table();
    int i;

    for(i=0; i<hist_count; i++){
        int j=get_index(i);
        extl_table_seti_s(tab, i+1, hist[j]);
    }
    
    return tab;
}
