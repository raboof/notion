/*
 * ion/mod_query/wedln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libextl/extl.h>
#include <libmainloop/defer.h>
#include <libmainloop/signal.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/strings.h>
#include <ioncore/xic.h>
#include <ioncore/selection.h>
#include <ioncore/event.h>
#include <ioncore/regbind.h>
#include "edln.h"
#include "wedln.h"
#include "inputp.h"
#include "complete.h"
#include "main.h"


#define WEDLN_BRUSH(X) ((X)->input.brush)


/*{{{ Drawing primitives */


static int calc_text_y(WEdln *wedln, const WRectangle *geom)
{
    GrFontExtents fnte;
        
    grbrush_get_font_extents(WEDLN_BRUSH(wedln), &fnte);
        
    return geom->y+geom->h/2-fnte.max_height/2+fnte.baseline;
}


static int wedln_draw_strsect(WEdln *wedln, const WRectangle *geom, 
                              int x, int y, const char *str, int len,
                              const char *attr)
{
    if(len==0)
        return 0;
    
    grbrush_draw_string(WEDLN_BRUSH(wedln), x, y, str, len, TRUE, attr);
    
    return grbrush_get_text_width(WEDLN_BRUSH(wedln), str, len);
}

#if 0
static void dispu(const char* s, int l)
{
    while(l>0){
        int c=(unsigned char)*s;
        fprintf(stderr, "%d[%c]", c, *s);
        s++;
        l--;
    }
    fprintf(stderr, "\n");
}
#endif

#define DSTRSECT(LEN, INV) \
    if(LEN>0){tx+=wedln_draw_strsect(wedln, geom, geom->x+tx, ty, str, LEN, INV); \
     str+=LEN; len-=LEN;}


static void wedln_do_draw_str_box(WEdln *wedln, const WRectangle *geom,
                                  const char *str, int cursor, 
                                  int mark, int tx)
{
    int len=strlen(str), ll=0, ty=0;
    const char *normalstyle=(REGION_IS_ACTIVE(wedln)
                             ? "active-normal" : "inactive-normal");
    const char *selectionstyle=(REGION_IS_ACTIVE(wedln)
                                ? "active-selection" : "inactive-selection");
    const char *cursorstyle=(REGION_IS_ACTIVE(wedln)
                             ? "active-cursor" : "inactive-cursor");
    
    /*if(tx<geom->w){
        WRectangle g=*geom;
        g.x+=tx;
        g.w-=tx;
        grbrush_clear_area(WEDLN_BRUSH(wedln), &g);
    }*/
    
    ty=calc_text_y(wedln, geom);

    if(mark<=cursor){
        if(mark>=0){
            DSTRSECT(mark, normalstyle);
            DSTRSECT(cursor-mark, selectionstyle);
        }else{
            DSTRSECT(cursor, normalstyle);
        }
        if(len==0){
            tx+=wedln_draw_strsect(wedln, geom, geom->x+tx, ty,
                                   " ", 1, cursorstyle);
        }else{
            ll=str_nextoff(str, 0);
            DSTRSECT(ll, cursorstyle);
        }
    }else{
        DSTRSECT(cursor, normalstyle);
        ll=str_nextoff(str, 0);
        DSTRSECT(ll, cursorstyle);
        DSTRSECT(mark-cursor-ll, selectionstyle);
    }
    DSTRSECT(len, normalstyle);

    if(tx<geom->w){
        WRectangle g=*geom;
        g.x+=tx;
        g.w-=tx;
        grbrush_clear_area(WEDLN_BRUSH(wedln), &g);
    }
}


static void wedln_draw_str_box(WEdln *wedln, const WRectangle *geom,
                               int vstart, const char *str,
                               int dstart, int point, int mark)
{
    int tx=0;

    /* Some fonts and Xmb/utf8 routines don't work well with dstart!=0. */
    dstart=0;
    
    if(mark>=0){
        mark-=vstart+dstart;
        if(mark<0)
            mark=0;
    }
    
    point-=vstart+dstart;
    
    if(dstart!=0)
        tx=grbrush_get_text_width(WEDLN_BRUSH(wedln), str+vstart, dstart);

    grbrush_begin(WEDLN_BRUSH(wedln), geom, GRBRUSH_AMEND|GRBRUSH_NEED_CLIP);
    
    wedln_do_draw_str_box(wedln, geom, str+vstart+dstart, point, mark, tx);

    grbrush_end(WEDLN_BRUSH(wedln));
}


static bool wedln_update_cursor(WEdln *wedln, int iw)
{
    int cx, l;
    int vstart=wedln->vstart;
    int point=wedln->edln.point;
    int len=wedln->edln.psize;
    int mark=wedln->edln.mark;
    const char *str=wedln->edln.p;
    bool ret;
    
    if(point<wedln->vstart)
        wedln->vstart=point;
    
    if(wedln->vstart==point)
        return FALSE;
    
    while(vstart<point){
        if(point==len){
            cx=grbrush_get_text_width(WEDLN_BRUSH(wedln), str+vstart, 
                                      point-vstart);
            cx+=grbrush_get_text_width(WEDLN_BRUSH(wedln), " ", 1);
        }else{
            int nxt=str_nextoff(str, point);
            cx=grbrush_get_text_width(WEDLN_BRUSH(wedln), str+vstart, 
                                      point-vstart+nxt);
        }
        
        if(cx<iw)
            break;
        
        l=str_nextoff(str, vstart);
        if(l==0)
            break;
        vstart+=l;
    }
    
    ret=(wedln->vstart!=vstart);
    wedln->vstart=vstart;
    
    return ret;
}


/*}}}*/


/*{{{ Size/location calc */


static int get_textarea_height(WEdln *wedln, bool with_spacing)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    
    grbrush_get_border_widths(WEDLN_BRUSH(wedln), &bdw);
    grbrush_get_font_extents(WEDLN_BRUSH(wedln), &fnte);
    
    return (fnte.max_height+bdw.top+bdw.bottom+
            (with_spacing ? bdw.spacing : 0));
}


enum{G_NORESET, G_MAX, G_CURRENT};


static void get_geom(WEdln *wedln, int mode, WRectangle *geom)
{
    if(mode==G_MAX)
        *geom=wedln->input.last_fp.g;
    else if(mode==G_CURRENT)
        *geom=REGION_GEOM(wedln);
}


static void get_completions_geom(WEdln *wedln, int mode, WRectangle *geom)
{
    get_geom(wedln, mode, geom);
    geom->x=0;
    geom->y=0;
    
    geom->h-=get_textarea_height(wedln, TRUE);
    if(geom->h<0)
        geom->h=0;
}


static void get_outer_geom(WEdln *wedln, int mode, WRectangle *geom)
{
    int th;
    
    get_geom(wedln, mode, geom);
    geom->x=0;
    geom->y=0;
    
    th=get_textarea_height(wedln, FALSE);
    
    geom->y+=geom->h-th;
    geom->h=th;
}


static void get_inner_geom(WEdln *wedln, int mode, WRectangle *geom)
{
    GrBorderWidths bdw;
    
    grbrush_get_border_widths(WEDLN_BRUSH(wedln), &bdw);
    
    get_outer_geom(wedln, mode, geom);
    
    geom->x+=bdw.left;
    geom->w-=bdw.left+bdw.right;
    geom->y+=bdw.top;
    geom->h-=bdw.top+bdw.bottom;
    geom->w=maxof(0, geom->w);
    geom->h=maxof(0, geom->h);
}


static void get_textarea_geom(WEdln *wedln, int mode, WRectangle *geom)
{
    get_inner_geom(wedln, mode, geom);
    geom->x+=wedln->prompt_w;
    geom->w-=wedln->prompt_w;
    geom->w=maxof(0, geom->w);
}


static void wedln_calc_size(WEdln *wedln, WRectangle *geom)
{
    int h, th;
    GrBorderWidths bdw;
    WRectangle max_geom=*geom, tageom;
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    if(wedln->prompt!=NULL){
        wedln->prompt_w=grbrush_get_text_width(WEDLN_BRUSH(wedln),
                                               wedln->prompt, 
                                               wedln->prompt_len);
    }

    th=get_textarea_height(wedln, wedln->compl_list.strs!=NULL);
    
    if(wedln->compl_list.strs==NULL){
        if(max_geom.h<th || wedln->input.last_fp.mode==REGION_FIT_EXACT)
            geom->h=max_geom.h;
        else
            geom->h=th;
    }else{
        WRectangle g;
        
        get_completions_geom(wedln, G_MAX, &g);
        
        fit_listing(WEDLN_BRUSH(wedln), &g, &(wedln->compl_list));
        
        grbrush_get_border_widths(WEDLN_BRUSH(wedln), &bdw);
        
        h=wedln->compl_list.toth;
        th+=bdw.top+bdw.bottom;
        
        if(h+th>max_geom.h || wedln->input.last_fp.mode==REGION_FIT_EXACT)
            h=max_geom.h-th;
        geom->h=h+th;
    }
    
    geom->w=max_geom.w;
    geom->y=max_geom.y+max_geom.h-geom->h;
    geom->x=max_geom.x;

    tageom=*geom;
    get_textarea_geom(wedln, G_NORESET, &tageom);
    wedln_update_cursor(wedln, tageom.w);
}


/*}}}*/


/*{{{ Draw */


void wedln_draw_completions(WEdln *wedln, bool complete)
{
    WRectangle geom;
    
    if(wedln->compl_list.strs!=NULL && WEDLN_BRUSH(wedln)!=NULL){
        const char *style=(REGION_IS_ACTIVE(wedln) 
                           ? "active" 
                           : "inactive");
        const char *selstyle=(REGION_IS_ACTIVE(wedln) 
                              ? "active-selection" 
                              : "inactive-selection");
        
        get_completions_geom(wedln, G_CURRENT, &geom);
        
        draw_listing(WEDLN_BRUSH(wedln), &geom, &(wedln->compl_list), 
                     complete, style, selstyle);
    }
}

    
void wedln_draw_textarea(WEdln *wedln)
{
    WRectangle geom;
    const char *style=(REGION_IS_ACTIVE(wedln) ? "active" : "inactive");

    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_outer_geom(wedln, G_CURRENT, &geom);
    
    /*grbrush_begin(WEDLN_BRUSH(wedln), &geom, GRBRUSH_AMEND);*/
    
    grbrush_draw_border(WEDLN_BRUSH(wedln), &geom, style);

    if(wedln->prompt!=NULL){
        int ty;
        const char *promptstyle=(REGION_IS_ACTIVE(wedln) 
                                 ? "active-prompt" : "inactive-prompt");
        
        get_inner_geom(wedln, G_CURRENT, &geom);
        ty=calc_text_y(wedln, &geom);
        
        grbrush_draw_string(WEDLN_BRUSH(wedln), geom.x, ty,
                            wedln->prompt, wedln->prompt_len, TRUE, 
                            promptstyle);
    }

    get_textarea_geom(wedln, G_CURRENT, &geom);
    
    wedln_draw_str_box(wedln, &geom, wedln->vstart, wedln->edln.p, 0,
                       wedln->edln.point, wedln->edln.mark);

    /*grbrush_end(WEDLN_BRUSH(wedln));*/
}


void wedln_draw(WEdln *wedln, bool complete)
{
    WRectangle g;
    int f=(complete ? 0 : GRBRUSH_NO_CLEAR_OK);
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_geom(wedln, G_CURRENT, &g);
    
    grbrush_begin(WEDLN_BRUSH(wedln), &g, f);
    
    wedln_draw_completions(wedln, FALSE);
    wedln_draw_textarea(wedln);
    
    grbrush_end(WEDLN_BRUSH(wedln));
}


/*}}}*/


/*{{{ Completions */


static void wedln_show_completions(WEdln *wedln, char **strs, int nstrs)
{
    int w=REGION_GEOM(wedln).w;
    int h=REGION_GEOM(wedln).h;
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    setup_listing(&(wedln->compl_list), strs, nstrs, FALSE);
    input_refit((WInput*)wedln);
    if(w==REGION_GEOM(wedln).w && h==REGION_GEOM(wedln).h)
        wedln_draw_completions(wedln, TRUE);
}


void wedln_hide_completions(WEdln *wedln)
{
    if(wedln->compl_list.strs!=NULL){
        deinit_listing(&(wedln->compl_list));
        input_refit((WInput*)wedln);
    }
}
    

void wedln_scrollup_completions(WEdln *wedln)
{
    if(wedln->compl_list.strs==NULL)
        return;
    if(scrollup_listing(&(wedln->compl_list)))
        wedln_draw_completions(wedln, TRUE);
}


void wedln_scrolldown_completions(WEdln *wedln)
{
    if(wedln->compl_list.strs==NULL)
        return;
    if(scrolldown_listing(&(wedln->compl_list)))
        wedln_draw_completions(wedln, TRUE);
}


static ExtlExportedFn *sc_safe_fns[]={
    (ExtlExportedFn*)&wedln_set_completions,
    NULL
};


static ExtlSafelist sc_safelist=EXTL_SAFELIST_INIT(sc_safe_fns);


static void wedln_call_completor(WEdln *wedln)
{
    const char *p=wedln->edln.p;
    int point=wedln->edln.point;
    
    if(p==NULL){
        p="";
        point=0;
    }
        
    extl_protect(&sc_safelist);
    extl_call(wedln->completor, "osi", NULL, wedln, p, point);
    extl_unprotect(&sc_safelist);
}


static void timed_complete(WTimer *tmr, Obj *obj)
{
    if(obj!=NULL)
        wedln_call_completor((WEdln*)obj);
}


/*EXTL_DOC
 * This function should be called in completors (such given as
 * parameters to \code{mod_query.query}) to return the set of completions
 * found. The numerical indexes of \var{completions} list the found
 * completions. If the entry \var{common_part} exists, it gives an
 * extra common prefix of all found completions.
 */
EXTL_EXPORT_MEMBER
void wedln_set_completions(WEdln *wedln, ExtlTab completions)
{
    int n=0, i=0;
    char **ptr=NULL, *beg=NULL, *end=NULL, *p=NULL;
    
    n=extl_table_get_n(completions);
    
    if(n==0){
        wedln_hide_completions(wedln);
        return;
    }
    
    ptr=ALLOC_N(char*, n);
    if(ptr==NULL)
        goto allocfail;

    for(i=0; i<n; i++){
        if(!extl_table_geti_s(completions, i+1, &p)){
            goto allocfail;
        }
        ptr[i]=p;
    }

    extl_table_gets_s(completions, "common_beg", &beg);
    extl_table_gets_s(completions, "common_end", &end);
    
    if(wedln->compl_beg!=NULL)
        free(wedln->compl_beg);
    
    if(wedln->compl_end!=NULL)
        free(wedln->compl_end);
    
    wedln->compl_beg=beg;
    wedln->compl_end=end;
    
    n=edln_do_completions(&(wedln->edln), ptr, n, beg, end, 
                          !mod_query_config.autoshowcompl);
    i=n;

    if(n>1 || (mod_query_config.autoshowcompl && n>0)){
        wedln_show_completions(wedln, ptr, n);
        return;
    }
    
allocfail:
    wedln_hide_completions(wedln);
    while(i>0){
        i--;
        free(ptr[i]);
    }
    free(ptr);
}


static int update_nocompl=0;


static void wedln_do_select_completion(WEdln *wedln, int n)
{
    wedln->compl_list.selected_str=n;
    wedln_draw_completions(wedln, FALSE);

    update_nocompl++;
    edln_set_completion(&(wedln->edln), wedln->compl_list.strs[n],
                        wedln->compl_beg, wedln->compl_end);
    update_nocompl--;
}


/*EXTL_DOC
 * Select next completion.
 */
EXTL_EXPORT_MEMBER
bool wedln_next_completion(WEdln *wedln)
{
    int n=-1;
    
    if(wedln->compl_list.nstrs<=0)
        return FALSE;
    
    if(wedln->compl_list.selected_str<0 ||
       wedln->compl_list.selected_str+1>=wedln->compl_list.nstrs){
        n=0;
    }else{
        n=wedln->compl_list.selected_str+1;
    }
    
    if(n!=wedln->compl_list.selected_str)
        wedln_do_select_completion(wedln, n);
    
    return TRUE;
}


/*EXTL_DOC
 * Select previous completion.
 */
EXTL_EXPORT_MEMBER
bool wedln_prev_completion(WEdln *wedln)
{
    int n=-1;
    
    if(wedln->compl_list.nstrs<=0)
        return FALSE;
    
    if(wedln->compl_list.selected_str<=0){
        n=wedln->compl_list.nstrs-1;
    }else{
        n=wedln->compl_list.selected_str-1;
    }

    if(n!=wedln->compl_list.selected_str)
        wedln_do_select_completion(wedln, n);
    
    return TRUE;
}


/*EXTL_DOC
 * Call completion handler with the text between the beginning of line and
 * current cursor position, or select next completion from list if in
 * auto-show-completions mode and \var{cycle} is set.
 */
EXTL_EXPORT_MEMBER
void wedln_complete(WEdln *wedln, bool cycle)
{
    if(cycle && mod_query_config.autoshowcompl && wedln->compl_list.nstrs>0){
        wedln_next_completion(wedln);
    }else{
        wedln_call_completor(wedln);
    }
}


/*}}}*/


/*{{{ Update handler */


static void wedln_update_handler(WEdln *wedln, int from, int flags)
{
    WRectangle geom;
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_textarea_geom(wedln, G_CURRENT, &geom);
    
    if(flags&EDLN_UPDATE_NEW)
        wedln->vstart=0;
    
    if(flags&EDLN_UPDATE_MOVED){
        if(wedln_update_cursor(wedln, geom.w))
            from=wedln->vstart;
    }
    
    from=maxof(0, from-wedln->vstart);

    wedln_draw_str_box(wedln, &geom, wedln->vstart, wedln->edln.p, from,
                       wedln->edln.point, wedln->edln.mark);
    
    if(update_nocompl==0 &&
       mod_query_config.autoshowcompl && 
       flags&EDLN_UPDATE_CHANGED){
        if(wedln->autoshowcompl_timer==NULL)
            wedln->autoshowcompl_timer=create_timer();
        if(wedln->autoshowcompl_timer!=NULL){
            timer_set(wedln->autoshowcompl_timer, 
                      mod_query_config.autoshowcompl_delay,
                      timed_complete, (Obj*)wedln);
        }
    }
}


/*}}}*/


/*{{{ Init, deinit and config update */


static bool wedln_init_prompt(WEdln *wedln, const char *prompt)
{
    char *p;
    
    if(prompt!=NULL){
        p=scat(prompt, "  ");
    
        if(p==NULL)
            return FALSE;

        wedln->prompt=p;
        wedln->prompt_len=strlen(p);
    }else{
        wedln->prompt=NULL;
        wedln->prompt_len=0;
    }
    wedln->prompt_w=0;
    
    return TRUE;
}


static bool wedln_init(WEdln *wedln, WWindow *par, const WFitParams *fp, 
                       WEdlnCreateParams *params)
{
    wedln->vstart=0;

    if(!wedln_init_prompt(wedln, params->prompt))
        return FALSE;
    
    if(!edln_init(&(wedln->edln), params->dflt)){
        free(wedln->prompt);
        return FALSE;
    }
    
    wedln->handler=extl_fn_none();
    wedln->completor=extl_fn_none();
    
    wedln->edln.uiptr=wedln;
    wedln->edln.ui_update=(EdlnUpdateHandler*)wedln_update_handler;

    wedln->autoshowcompl_timer=NULL;
    
    init_listing(&(wedln->compl_list));

    wedln->compl_beg=NULL;
    wedln->compl_end=NULL;
    
    if(!input_init((WInput*)wedln, par, fp)){
        edln_deinit(&(wedln->edln));
        free(wedln->prompt);
        return FALSE;
    }

    window_create_xic(&wedln->input.win);
    
    wedln->handler=extl_ref_fn(params->handler);
    wedln->completor=extl_ref_fn(params->completor);

    region_add_bindmap((WRegion*)wedln, mod_query_wedln_bindmap);
    
    return TRUE;
}


WEdln *create_wedln(WWindow *par, const WFitParams *fp,
                    WEdlnCreateParams *params)
{
    CREATEOBJ_IMPL(WEdln, wedln, (p, par, fp, params));
}


static void wedln_deinit(WEdln *wedln)
{
    if(wedln->prompt!=NULL)
        free(wedln->prompt);

    if(wedln->compl_beg!=NULL)
        free(wedln->compl_beg);

    if(wedln->compl_end!=NULL)
        free(wedln->compl_end);

    if(wedln->compl_list.strs!=NULL)
        deinit_listing(&(wedln->compl_list));

    if(wedln->autoshowcompl_timer!=NULL)
        destroy_obj((Obj*)wedln->autoshowcompl_timer);

    extl_unref_fn(wedln->completor);
    extl_unref_fn(wedln->handler);
    
    edln_deinit(&(wedln->edln));
    input_deinit((WInput*)wedln);
}


static void wedln_do_finish(WEdln *wedln)
{
    ExtlFn handler;
    char *p;
    
    handler=wedln->handler;
    wedln->handler=extl_fn_none();
    p=edln_finish(&(wedln->edln));
    
    if(region_manager_allows_destroying((WRegion*)wedln))
       destroy_obj((Obj*)wedln);
    
    if(p!=NULL)
        extl_call(handler, "s", NULL, p);
    
    free(p);
    extl_unref_fn(handler);
}


/*EXTL_DOC
 * Close \var{wedln} and call any handlers.
 */
EXTL_EXPORT_MEMBER
void wedln_finish(WEdln *wedln)
{
    mainloop_defer_action((Obj*)wedln, (WDeferredAction*)wedln_do_finish);
}


/*}}}*/


/*{{{ The rest */


/*EXTL_DOC
 * Request selection from application holding such.
 * 
 * Note that this function is asynchronous; the selection will not
 * actually be inserted before Ion receives it. This will be no
 * earlier than Ion return to its main loop.
 */
EXTL_EXPORT_MEMBER
void wedln_paste(WEdln *wedln)
{
    ioncore_request_selection_for(wedln->input.win.win);
}


void wedln_insstr(WEdln *wedln, const char *buf, size_t n)
{
    edln_insstr_n(&(wedln->edln), buf, n, TRUE, TRUE);
}


static const char *wedln_style(WEdln *wedln)
{
    return "input-edln";
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab wedln_dynfuntab[]={
    {window_draw, wedln_draw},
    {input_calc_size, wedln_calc_size},
    {input_scrollup, wedln_scrollup_completions},
    {input_scrolldown, wedln_scrolldown_completions},
    {window_insstr, wedln_insstr},
    {(DynFun*)input_style, (DynFun*)wedln_style},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WEdln, WInput, wedln_deinit, wedln_dynfuntab);

    
/*}}}*/
