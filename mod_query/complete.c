/*
 * ion/mod_query/complete.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#include <libtu/objp.h>
#include <ioncore/common.h>
#include "complete.h"
#include "edln.h"
#include "wedln.h"


/*{{{ Completion list processing */


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
    return strcoll(*v1, *v2);
}


static void edln_reset(Edln *edln)
{
    assert(edln->palloced>=1);

    edln->p[0]='\0';
    edln->psize=0;
    edln->point=0;
    edln->mark=-1;
    edln->histent=-1;
}


static void edln_do_set_completion(Edln *edln, const char *comp, int len,
                                   const char *beg, const char *end)
{
    edln_reset(edln);

    if(beg!=NULL)
        edln_insstr_n(edln, beg, strlen(beg), FALSE, TRUE);

    if(len>0)
        edln_insstr_n(edln, comp, len, FALSE, TRUE);

    if(end!=NULL)
        edln_insstr_n(edln, end, strlen(end), FALSE, FALSE);

    if(edln->ui_update!=NULL){
        edln->ui_update(edln->uiptr, 0,
                        EDLN_UPDATE_MOVED|EDLN_UPDATE_CHANGED|
                        EDLN_UPDATE_NEW);
    }

}


void edln_set_completion(Edln *edln, const char *comp,
                         const char *beg, const char *end)
{
    edln_do_set_completion(edln, comp, strlen(comp), beg, end);
}


int edln_do_completions(Edln *edln, char **completions, int ncomp,
                        const char *beg, const char *end, bool setcommon,
                        bool nosort)
{
    int len;

    if(ncomp==0){
        return 0;
    }else if(ncomp==1){
        len=strlen(completions[0]);
    }else{
        if(!nosort)
            qsort(completions, ncomp, sizeof(char**), compare);
        len=get_common_part_rmdup(completions, &ncomp);
    }

    if(setcommon)
        edln_do_set_completion(edln, completions[0], len, beg, end);

    return ncomp;
}


/*}}}*/


/*{{{ WComplProxy */


bool complproxy_init(WComplProxy *proxy, WEdln *wedln, int id, int cycle)
{
    watch_init(&(proxy->wedln_watch));
    if(!watch_setup(&(proxy->wedln_watch), (Obj*)wedln, NULL))
        return FALSE;

    proxy->id=id;
    proxy->cycle=cycle;

    return TRUE;
}


WComplProxy *create_complproxy(WEdln *wedln, int id, int cycle)
{
    CREATEOBJ_IMPL(WComplProxy, complproxy, (p, wedln, id, cycle));
}


void complproxy_deinit(WComplProxy *proxy)
{
    watch_reset(&(proxy->wedln_watch));
}


/*EXTL_DOC
 * Set completion list of the \type{WEdln} that \var{proxy} refers to to
 * \var{compls}, if it is still waiting for this completion run. The
 * numerical indexes of \var{compls} list the found completions. If the
 * entry \var{common_beg} (\var{common_end}) exists, it gives an extra
 * common prefix (suffix) of all found completions.
 */
EXTL_EXPORT_MEMBER
bool complproxy_set_completions(WComplProxy *proxy, ExtlTab compls)
{
    WEdln *wedln=(WEdln*)proxy->wedln_watch.obj;

    if(wedln!=NULL){
        if(wedln->compl_waiting_id==proxy->id){
            wedln_set_completions(wedln, compls, proxy->cycle);
            wedln->compl_current_id=proxy->id;
            return TRUE;
        }
    }

    return FALSE;
}


EXTL_EXPORT
IMPLCLASS(WComplProxy, Obj, complproxy_deinit, NULL);


/*}}}*/

