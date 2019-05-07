/*
 * ion/mod_query/edln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/selection.h>
#include <ioncore/strings.h>

#include "edln.h"
#include "wedln.h"
#include "history.h"

#define EDLN_ALLOCUNIT 16

#define UPDATE(X) \
    edln->ui_update(edln->uiptr, X, 0)

#define UPDATE_MOVED(X) \
    edln->ui_update(edln->uiptr, X, EDLN_UPDATE_MOVED)

#define UPDATE_CHANGED(X)                                  \
    edln->ui_update(edln->uiptr, X,                        \
                    EDLN_UPDATE_MOVED|EDLN_UPDATE_CHANGED)

#define UPDATE_CHANGED_NOMOVE(X)         \
    edln->ui_update(edln->uiptr, X,      \
                    EDLN_UPDATE_CHANGED)

#define UPDATE_NEW()                                                       \
    edln->ui_update(edln->uiptr, 0,                                        \
                    EDLN_UPDATE_NEW|EDLN_UPDATE_MOVED|EDLN_UPDATE_CHANGED)

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


bool edln_insstr(Edln *edln, const char *str)
{
    int l;
    
    if(str==NULL)
        return FALSE;
    
    l=strlen(str);
    
    return edln_insstr_n(edln, str, l, TRUE, TRUE);
}


bool edln_insstr_n(Edln *edln, const char *str, int l, 
                   bool update, bool movepoint)
{
    if(!edln_pspc(edln, l))
        return FALSE;
    
    memmove(&(edln->p[edln->point]), str, l);
    if(movepoint){
        edln->point+=l;
        if(update)
            UPDATE_CHANGED(edln->point-l);
    }else{
        if(update)
            UPDATE_CHANGED_NOMOVE(edln->point-l);
    }

    return TRUE;
}


/*}}}*/


/*{{{ Transpose */


bool edln_transpose_chars(Edln *edln)
{
    int off1, off2, pos;
    char *buf;

    if((edln->point==0) || (edln->psize<2))
        return FALSE;

    pos=edln->point;
    if(edln->point==edln->psize)
        pos=pos-str_prevoff(edln->p, edln->point);

    off1=str_nextoff(edln->p, pos);
    off2=str_prevoff(edln->p, pos);

    buf=ALLOC_N(char, off2);
    if(buf==NULL)
        return FALSE;
    memmove(buf, &(edln->p[pos-off2]), off2);
    memmove(&(edln->p[pos-off2]), &(edln->p[pos]), off1);
    memmove(&(edln->p[pos-off2+off1]), buf, off2);
    FREE(buf);

    if(edln->point!=edln->psize)
        edln->point+=off1;

    UPDATE_CHANGED(0);
    return TRUE;
}


bool edln_transpose_words(Edln *edln)
{
    int m1, m2, m3, m4, off1, off2, oldp;
    char *buf;

    if((edln->point==edln->psize) || (edln->psize<3))
        return FALSE;

    oldp=edln->point;
    edln_bskip_word(edln);
    m1=edln->point;
    edln_skip_word(edln);
    m2=edln->point;
    edln_skip_word(edln);
    if(edln->point==m2)
        goto noact;
    m4=edln->point;
    edln_bskip_word(edln);
    if(edln->point==m1)
        goto noact;
    m3=edln->point;

    off1=m4-m3;
    off2=m3-m2;

    buf=ALLOC_N(char, m4-m1);
    if(buf==NULL)
        goto noact;
    memmove(buf, &(edln->p[m3]), off1);
    memmove(buf+off1, &(edln->p[m2]), off2);
    memmove(buf+off1+off2, &(edln->p[m1]), m2-m1);
    memmove(&(edln->p[m1]), buf, m4-m1);
    FREE(buf);

    edln->point=m4;
    UPDATE_CHANGED(0);
    return TRUE;

noact:
    edln->point=oldp;
    UPDATE_MOVED(edln->point);
    return FALSE;
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
    /*int p=edln->point;*/
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
    
    UPDATE_CHANGED_NOMOVE(edln->point);
}


void edln_backspace(Edln *edln)
{
    int n;
    if(edln->point==0)
        return;
    n=do_edln_back(edln);
    if(n!=0){
        edln_rspc(edln, n);
        UPDATE_CHANGED(edln->point);
    }
}

void edln_kill_to_eol(Edln *edln)
{
    edln_rspc(edln, edln->psize-edln->point);
    UPDATE_CHANGED_NOMOVE(edln->point);
}


void edln_kill_to_bol(Edln *edln)
{
    int p=edln->point;
    
    edln_bol(edln);
    edln_rspc(edln, p);
    edln->point=0;
    UPDATE_CHANGED(0);
}


void edln_kill_line(Edln *edln)
{
    edln_bol(edln);
    edln_kill_to_eol(edln);
    UPDATE_CHANGED(0);
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
    
    UPDATE_CHANGED_NOMOVE(oldp);
}


void edln_bkill_word(Edln *edln)
{
    int oldp=edln->point;
    
    edln_bskip_word(edln);
    
    if(edln->point==oldp)
        return;
    
    edln_rspc(edln, oldp-edln->point);
    UPDATE_CHANGED(edln->point);
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


bool edln_set_context(Edln *edln, const char *str)
{
    char *s=scat(str, ":"), *cp;
    
    if(s==NULL)
        return FALSE;
    
    cp=strchr(s, ':');
    while(cp!=NULL && *(cp+1)!='\0'){
        *cp='_';
        cp=strchr(cp, ':');
    }

    if(edln->context!=NULL)
        free(edln->context);
    edln->context=s;
        
    return TRUE;
}


static void edln_do_set_hist(Edln *edln, int e, bool match)
{
    const char *str=mod_query_history_get(e), *s2;
    if(str!=NULL){
        if(edln->histent<0){
            edln->tmp_p=edln->p;
            edln->tmp_palloced=edln->palloced;
            edln->p=NULL;
        }
        
        /* Skip context label */
        s2=strchr(str, ':');
        if(s2!=NULL)
            str=s2+1;
        
        edln->histent=e;
        edln_setstr(edln, str);
        edln->point=(match
                     ? MINOF(edln->point, edln->psize) 
                     : edln->psize);
        edln->mark=-1;
        edln->modified=FALSE;
        UPDATE_NEW();
    }
}


static char *history_search_str(Edln *edln)
{
    char *sstr;
    char tmp=edln->p[edln->point];
    edln->p[edln->point]='\0';
    sstr=scat(edln->context ? edln->context : "*:", edln->p);
    edln->p[edln->point]=tmp;
    return sstr;
}


static int search(Edln *edln, int from, bool bwd, bool match)
{
    int e;
    
    if(match && edln->point>0){
        char *tmpstr=history_search_str(edln);
        if(tmpstr==NULL)
            return edln->histent;
        e=mod_query_history_search(tmpstr, from, bwd, FALSE);
        free(tmpstr);
    }else{
        e=mod_query_history_search(edln->context, from, bwd, FALSE);
    }
    
    return e;
}


void edln_history_prev(Edln *edln, bool match)
{
    int e=search(edln, edln->histent+1, FALSE, match);
    if(e>=0)
        edln_do_set_hist(edln, e, match);
}


void edln_history_next(Edln *edln, bool match)
{
    int e=edln->histent;
    
    if(edln->histent<0)
        return;

    e=search(edln, edln->histent-1, TRUE, match);
    
    if(e>=0){
        edln_do_set_hist(edln, e, match);
    }else{
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
        UPDATE_NEW();
    }
}


uint edln_history_matches(Edln *edln, char ***h_ret)
{
    char *tmpstr=history_search_str(edln);
    uint ret;
    
    if(tmpstr==NULL){
        *h_ret=NULL;
        return 0;
    }
        
    ret=mod_query_history_complete(tmpstr, h_ret);
    
    free(tmpstr);
    
    return ret;
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
    edln->tmp_p=NULL;
    edln->context=NULL;
    
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
    if(edln->context!=NULL){
        free(edln->context);
        edln->context=NULL;
    }
}


static const char *ctx(Edln *edln)
{
    if(edln->context!=NULL)
        return edln->context;
    else
        return "default:";
}


char* edln_finish(Edln *edln)
{
    char *p=edln->p, *hist;
    
    if(p!=NULL){
        libtu_asprintf(&hist, "%s%s", ctx(edln), p);
        if(hist!=NULL)
            mod_query_history_push_(hist);
    }
    
    edln->p=NULL;
    edln->psize=edln->palloced=0;
    
    /*stripws(p);*/
    return str_stripws(p);
}

/*}}}*/

