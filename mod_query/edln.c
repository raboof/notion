/*
 * ion/mod_query/edln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <ioncore/common.h>
#include <ioncore/selection.h>
#include <ioncore/strings.h>

#include "edln.h"
#include "wedln.h"

#define EDLN_ALLOCUNIT 16
#define EDLN_HISTORY_SIZE 256

#define UPDATE(X) edln->ui_update(edln->uiptr, X, FALSE)
#define UPDATE_MOVED(X) edln->ui_update(edln->uiptr, X, TRUE)

#define CHAR wchar_t
#define ISALNUM iswalnum
#define CHAR_AT(P, N) str_wchar_at(P, N)


/*{{{ Alloc */


static bool edln_pspc(Edln *edln, int n)
{
    char *np;
    int pa;
    
    if(edln->palloced<edln->psize+1+n){
        pa=edln->palloced+n;
        pa|=(EDLN_ALLOCUNIT-1);
        np=ALLOC_N(char, pa);
        
        if(np==NULL)
            return FALSE;

        memmove(np, edln->p, edln->point*sizeof(char));
        memmove(np+edln->point+n, edln->p+edln->point,
                (edln->psize-edln->point+1)*sizeof(char));
        free(edln->p);
        edln->p=np;
        edln->palloced=pa;
    }else{
        memmove(edln->p+edln->point+n, edln->p+edln->point,
                (edln->psize-edln->point+1)*sizeof(char));
    }

    if(edln->mark>edln->point)
        edln->mark+=n;
    
    edln->psize+=n;

    edln->modified=1;
    return TRUE;
}


static bool edln_rspc(Edln *edln, int n)
{
    char *np;
    int pa;
    
    if(n+edln->point>=edln->psize)
        n=edln->psize-edln->point;
    
    if(n==0)
        return TRUE;
    
    if((edln->psize+1-n)<(edln->palloced&~(EDLN_ALLOCUNIT-1))){
        pa=edln->palloced&~(EDLN_ALLOCUNIT-1);
        np=ALLOC_N(char, pa);
        
        if(np==NULL)
            goto norm;
        
        memmove(np, edln->p, edln->point*sizeof(char));
        memmove(np+edln->point, edln->p+edln->point+n,
                (edln->psize-edln->point+1-n)*sizeof(char));
        free(edln->p);
        edln->p=np;
        edln->palloced=pa;
    }else{
    norm:
        memmove(edln->p+edln->point, edln->p+edln->point+n,
                (edln->psize-edln->point+1-n)*sizeof(char));
    }
    edln->psize-=n;

    if(edln->mark>edln->point)
        edln->mark-=n;
    
    edln->modified=1;
    return TRUE;
}


static void edln_clearstr(Edln *edln)
{
    if(edln->p!=NULL){
        free(edln->p);
        edln->p=NULL;
    }
    edln->palloced=0;
    edln->psize=0;
}


static bool edln_initstr(Edln *edln, const char *p)
{
    int l=strlen(p), al;
    
    al=(l+1)|(EDLN_ALLOCUNIT-1);
    
    edln->p=ALLOC_N(char, al);
    
    if(edln->p==NULL)
        return FALSE;

    edln->palloced=al;
    edln->psize=l;
    strcpy(edln->p, p);
    
    return TRUE;
}


static bool edln_setstr(Edln *edln, const char *p)
{
    edln_clearstr(edln);
    return edln_initstr(edln, p);
}


/*}}}*/


/*{{{ Insert */

#if 0
bool edln_insch(Edln *edln, char ch)
{
    if(edln_pspc(edln, 1)){
        edln->p[edln->point]=ch;
        edln->point++;
        UPDATE_MOVED(edln->point-1);
        return TRUE;
    }
    return FALSE;
}


bool edln_ovrch(Edln *edln, char ch)
{
    edln_delete(edln);
    return edln_insch(edln, ch);
}
#endif


bool edln_insstr(Edln *edln, const char *str)
{
    int l;
    
    if(str==NULL)
        return FALSE;
    
    l=strlen(str);
    
    return edln_insstr_n(edln, str, l);
}


bool edln_insstr_n(Edln *edln, const char *str, int l)
{
    if(!edln_pspc(edln, l))
        return FALSE;
    
    memmove(&(edln->p[edln->point]), str, l);
    edln->point+=l;
    UPDATE_MOVED(edln->point-l);

    return TRUE;
}


/*}}}*/


/*{{{ Movement */


static int do_edln_back(Edln *edln)
{
    int l=str_prevoff(edln->p, edln->point);
    edln->point-=l;
    return l;
}


void edln_back(Edln *edln)
{
    int p=edln->point;
    do_edln_back(edln);
    /*if(edln->point!=p)*/
        UPDATE_MOVED(edln->point);
}


static int do_edln_forward(Edln *edln)
{
    int l=str_nextoff(edln->p, edln->point);
    edln->point+=l;
    return l;
}


void edln_forward(Edln *edln)
{
    int p=edln->point;
    do_edln_forward(edln);
    /*if(edln->point!=p)*/
        UPDATE_MOVED(p);
}


void edln_bol(Edln *edln)
{
    if(edln->point!=0){
        edln->point=0;
        UPDATE_MOVED(0);
    }
}


void edln_eol(Edln *edln)
{
    int o=edln->point;
    
    if(edln->point!=edln->psize){
        edln->point=edln->psize;
        UPDATE_MOVED(o);
    }
}


void edln_bskip_word(Edln *edln)
{
    int p, n;
    CHAR c;
    
    while(edln->point>0){
        n=do_edln_back(edln);
        c=CHAR_AT(edln->p+edln->point, n);
        if(ISALNUM(c))
            goto fnd;
    }
    UPDATE_MOVED(edln->point);
    return;
    
fnd:
    while(edln->point>0){
        p=edln->point;
        n=do_edln_back(edln);
        c=CHAR_AT(edln->p+edln->point, n);

        if(!ISALNUM(c)){
            edln->point=p;
            break;
        }
    }
    UPDATE_MOVED(edln->point);
}


void edln_skip_word(Edln *edln)
{
    int oldp=edln->point;
    CHAR c;
    
    while(edln->point<edln->psize){
        c=CHAR_AT(edln->p+edln->point, edln->psize-edln->point);
        if(ISALNUM(c))
            goto fnd;
        if(do_edln_forward(edln)==0)
            break;
    }
    UPDATE_MOVED(oldp);
    return;
    
fnd:
    while(edln->point<edln->psize){
        c=CHAR_AT(edln->p+edln->point, edln->psize-edln->point);
        if(!ISALNUM(c))
            break;
        if(do_edln_forward(edln)==0)
            break;
    }
    UPDATE_MOVED(oldp);
}


void edln_set_point(Edln *edln, int point)
{
    int o=edln->point;
    
    if(point<0)
        point=0;
    else if(point>edln->psize)
        point=edln->psize;
    
    edln->point=point;
    
    if(o<point)
        UPDATE_MOVED(o);
    else
        UPDATE_MOVED(point);
}

               
/*}}}*/


/*{{{ Delete */


void edln_delete(Edln *edln)
{
    int left=edln->psize-edln->point;
    size_t l;
    
    if(left<=0)
        return;
    
    l=str_nextoff(edln->p, edln->point);
    
    if(l>0)
        edln_rspc(edln, l);
    
    UPDATE(edln->point);
}


void edln_backspace(Edln *edln)
{
    int n;
    if(edln->point==0)
        return;
    n=do_edln_back(edln);
    if(n!=0){
        edln_rspc(edln, n);
        UPDATE_MOVED(edln->point);
    }
}

void edln_kill_to_eol(Edln *edln)
{
    edln_rspc(edln, edln->psize-edln->point);
    UPDATE(edln->point);
}


void edln_kill_to_bol(Edln *edln)
{
    int p=edln->point;
    
    edln_bol(edln);
    edln_rspc(edln, p);
    edln->point=0;
    UPDATE_MOVED(0);
}


void edln_kill_line(Edln *edln)
{
    edln_bol(edln);
    edln_kill_to_eol(edln);
    UPDATE_MOVED(0);
}


void edln_kill_word(Edln *edln)
{
    int oldp=edln->point;
    int l;
    edln_skip_word(edln);
    
    if(edln->point==oldp)
        return;
    
    l=edln->point-oldp;
    edln->point=oldp;
    edln_rspc(edln, l);
    
    UPDATE(oldp);
}


void edln_bkill_word(Edln *edln)
{
    int oldp=edln->point;
    
    edln_bskip_word(edln);
    
    if(edln->point==oldp)
        return;
    
    edln_rspc(edln, oldp-edln->point);
    UPDATE(edln->point);
}


/*}}}*/


/*{{{ Selection */


static void do_set_mark(Edln *edln, int nm)
{
    int m=edln->mark;
    edln->mark=nm;
    if(m!=-1)
        UPDATE(m < edln->point ? m : edln->point);
}


void edln_set_mark(Edln *edln)
{
    do_set_mark(edln, edln->point);
}


void edln_clear_mark(Edln *edln)
{
    do_set_mark(edln, -1);
}


static void edln_do_copy(Edln *edln, bool del)
{
    int beg, end;
    
    if(edln->mark<0 || edln->point==edln->mark)
        return;
    
    if(edln->point<edln->mark){
        beg=edln->point;
        end=edln->mark;
    }else{
        beg=edln->mark;
        end=edln->point;
    }
    
    ioncore_set_selection_n(edln->p+beg, end-beg);
    
    if(del){
        edln->point=beg;
        edln_rspc(edln, end-beg);
    }
    edln->mark=-1;
    
    UPDATE(beg);
}


void edln_cut(Edln *edln)
{
    edln_do_copy(edln, TRUE);
}


void edln_copy(Edln *edln)
{
    edln_do_copy(edln, FALSE);
}


/*}}}*/


/*{{{ History */


static int hist_head=EDLN_HISTORY_SIZE;
static int hist_count=0;
static char *hist[EDLN_HISTORY_SIZE];



/*EXTL_DOC
 * Push an entry into line editor history.
 */
EXTL_EXPORT
void mod_query_history_push(const char *str)
{
    char *strc;
    
    if(hist_count>0 && strcmp(hist[hist_head], str)==0)
        return;
    
    strc=scopy(str);
    
    if(strc==NULL)
        return;
    
    hist_head--;
    if(hist_head<0)
        hist_head=EDLN_HISTORY_SIZE-1;
    
    if(hist_count==EDLN_HISTORY_SIZE)
        free(hist[hist_head]);
    else
        hist_count++;
    
    hist[hist_head]=strc;
}


/*EXTL_DOC
 * Get entry at index \var{n} in line editor history, 0 being the latest.
 */
EXTL_EXPORT
const char *mod_query_history_get(int n)
{
    int i=0;
    int e=hist_head;
    
    if(n>=hist_count || n<0)
        return NULL;
    
    n=(hist_head+n)%EDLN_HISTORY_SIZE;
    return hist[n];
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
        if(++hist_head==EDLN_HISTORY_SIZE)
            hist_head=0;
    }
    hist_head=EDLN_HISTORY_SIZE;
}


static void edln_do_set_hist(Edln *edln, int e)
{
    edln->histent=e;
    edln_setstr(edln, hist[e]);
    edln->point=edln->psize;
    edln->mark=-1;
    edln->modified=FALSE;
    UPDATE_MOVED(0);
}


void edln_history_prev(Edln *edln)
{
    int e;
    
    if(edln->histent==-1){
        if(hist_count==0)
            return;
        edln->tmp_p=edln->p;
        edln->tmp_palloced=edln->palloced;
        edln->p=NULL;
        e=hist_head;
    }else{
        e=edln->histent;
        if(e==(hist_head+hist_count-1)%EDLN_HISTORY_SIZE)
            return;
        e=(e+1)%EDLN_HISTORY_SIZE;
    }
    
    edln_do_set_hist(edln, e);
}


void edln_history_next(Edln *edln)
{
    int e=edln->histent;
    
    if(edln->histent==-1)
        return;
    
    if(edln->histent==hist_head){
        edln->histent=-1;
        if(edln->p!=NULL)
            free(edln->p);
        edln->p=edln->tmp_p;
        edln->palloced=edln->tmp_palloced;
        edln->tmp_p=NULL;
        edln->psize=(edln->p==NULL ? 0 : strlen(edln->p));
        edln->point=edln->psize;
        edln->mark=-1;
        edln->modified=TRUE;
        UPDATE_MOVED(0);
        return;
    }
    
/*    e=edln->histent-1;*/

    edln_do_set_hist(edln, (e+EDLN_HISTORY_SIZE-1)%EDLN_HISTORY_SIZE);
}


/*}}}*/


/*{{{ Init/deinit */


bool edln_init(Edln *edln, const char *p)
{
    if(p==NULL)
        p="";
    
    if(!edln_initstr(edln, p))
        return FALSE;
    
    edln->point=edln->psize;
    edln->mark=-1;
    edln->histent=-1;
    edln->modified=FALSE;
    edln->completion_handler=NULL;
    edln->tmp_p=NULL;
    
    return TRUE;
}


void edln_deinit(Edln *edln)
{
    if(edln->p!=NULL){
        free(edln->p);
        edln->p=NULL;
    }
    if(edln->tmp_p!=NULL){
        free(edln->tmp_p);
        edln->tmp_p=NULL;
    }
}


char* edln_finish(Edln *edln)
{
    char *p=edln->p;
    
    /*if(edln->modified)*/
    mod_query_history_push(p);
    
    edln->p=NULL;
    edln->psize=edln->palloced=0;
    
    /*stripws(p);*/
    return str_stripws(p);
}

/*}}}*/

