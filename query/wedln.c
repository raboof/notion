/*
 * ion/query/wedln.c
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
#include <ioncore/global.h>
#include <libtu/objp.h>
#include <ioncore/strings.h>
#include <ioncore/xic.h>
#include <ioncore/selection.h>
#include <ioncore/event.h>
#include <ioncore/regbind.h>
#include <ioncore/extl.h>
#include <ioncore/defer.h>
#include <libtu/minmax.h>
#include "edln.h"
#include "wedln.h"
#include "inputp.h"
#include "complete.h"


#define WEDLN_BRUSH(X) ((X)->input.brush)
#define WEDLN_WIN(X)  ((X)->input.win.win)


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
    
    grbrush_draw_string(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), x, y, 
                        str, len, TRUE, attr);
    
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
    
    grbrush_set_clipping_rectangle(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), geom);
    
    if(tx<geom->w){
        WRectangle g=*geom;
        g.x+=tx;
        g.w-=tx;
        grbrush_clear_area(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), &g);
    }
    
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

    /*if(tx<geom->w){
        WRectangle g=*geom;
        g.x+=tx;
        g.w-=tx;
        grbrush_clear_area(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), &g);
    }*/
    
    grbrush_clear_clipping_rectangle(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln));
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
    
    wedln_do_draw_str_box(wedln, geom, str+vstart+dstart, point, mark, tx);
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
        *geom=wedln->input.max_geom;
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

    th=get_textarea_height(wedln, wedln->complist.strs!=NULL);
    
    if(wedln->complist.strs==NULL){
        if(max_geom.h<th)
            geom->h=max_geom.h;
        else
            geom->h=th;
    }else{
        WRectangle g;
        
        get_completions_geom(wedln, G_MAX, &g);
        
        fit_listing(WEDLN_BRUSH(wedln), &g, &(wedln->complist));
        
        grbrush_get_border_widths(WEDLN_BRUSH(wedln), &bdw);
        
        h=wedln->complist.toth;
        th+=bdw.top+bdw.bottom;
        
        if(h+th>max_geom.h)
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


static void wedln_update_handler(WEdln *wedln, int from, bool moved)
{
    WRectangle geom;
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_textarea_geom(wedln, G_CURRENT, &geom);
    
    from-=wedln->vstart;
    
    if(moved){
        if(wedln_update_cursor(wedln, geom.w))
            from=0;
    }
    
    if(from<0)
        from=0;

    wedln_draw_str_box(wedln, &geom, wedln->vstart, wedln->edln.p, from,
                       wedln->edln.point, wedln->edln.mark);
}


void wedln_draw_completions(WEdln *wedln, bool complete)
{
    WRectangle geom;
    
    if(wedln->complist.strs!=NULL && WEDLN_BRUSH(wedln)!=NULL){
        const char *style=(REGION_IS_ACTIVE(wedln) ? "active" : "inactive");
        get_completions_geom(wedln, G_CURRENT, &geom);
        draw_listing(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), &geom,
                     &(wedln->complist), complete, style);
    }
}

    
void wedln_draw_textarea(WEdln *wedln, bool complete)
{
    WRectangle geom;
    const char *style=(REGION_IS_ACTIVE(wedln) ? "active" : "inactive");

    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_outer_geom(wedln, G_CURRENT, &geom);
    grbrush_draw_border(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), &geom, style);

    if(wedln->prompt!=NULL){
        int ty;
        const char *promptstyle=(REGION_IS_ACTIVE(wedln) 
                                 ? "active-prompt" : "inactive-prompt");
        
        get_inner_geom(wedln, G_CURRENT, &geom);
        ty=calc_text_y(wedln, &geom);
        
        grbrush_draw_string(WEDLN_BRUSH(wedln), WEDLN_WIN(wedln), geom.x, ty,
                            wedln->prompt, wedln->prompt_len, TRUE, 
                            promptstyle);
    }

    get_textarea_geom(wedln, G_CURRENT, &geom);
    
    wedln_draw_str_box(wedln, &geom, wedln->vstart, wedln->edln.p, 0,
                       wedln->edln.point, wedln->edln.mark);
}


void wedln_draw(WEdln *wedln, bool complete)
{
    wedln_draw_completions(wedln, complete);
    wedln_draw_textarea(wedln, complete);
}


/*}}}*/


/*{{{ Completions */


static void wedln_show_completions(WEdln *wedln, char **strs, int nstrs)
{
    int w=REGION_GEOM(wedln).w;
    int h=REGION_GEOM(wedln).h;
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    setup_listing(&(wedln->complist), strs, nstrs, FALSE);
    input_refit((WInput*)wedln);
    if(w==REGION_GEOM(wedln).w && h==REGION_GEOM(wedln).h)
        wedln_draw_completions(wedln, TRUE);
}


void wedln_hide_completions(WEdln *wedln)
{
    if(wedln->complist.strs!=NULL){
        deinit_listing(&(wedln->complist));
        input_refit((WInput*)wedln);
    }
}
    

void wedln_scrollup_completions(WEdln *wedln)
{
    if(wedln->complist.strs==NULL)
        return;
    if(scrollup_listing(&(wedln->complist)))
        wedln_draw_completions(wedln, TRUE);
}


void wedln_scrolldown_completions(WEdln *wedln)
{
    if(wedln->complist.strs==NULL)
        return;
    if(scrolldown_listing(&(wedln->complist)))
        wedln_draw_completions(wedln, TRUE);
}


static void wedln_completion_handler(WEdln *wedln, const char *nam)
{
    extl_call(wedln->completor, "os", NULL, wedln, nam);
}


/*EXTL_DOC
 * This function should be called in completors (such given as
 * parameters to \code{querymod.query}) to return the set of completions
 * found. The numerical indexes of \var{completions} list the found
 * completions. If the entry \var{common_part} exists, it gives an
 * extra common prefix of all found completions.
 */
EXTL_EXPORT_MEMBER
void wedln_set_completions(WEdln *wedln, ExtlTab completions)
{
    int n=0, i=0;
    char **ptr=NULL, *beg=NULL, *p=NULL;
    
    n=extl_table_get_n(completions);
    
    if(n==0){
        wedln_hide_completions(wedln);
        return;
    }
    
    ptr=ALLOC_N(char*, n);
    if(ptr==NULL){
        warn_err();
        goto allocfail;
    }
    for(i=0; i<n; i++){
        if(!extl_table_geti_s(completions, i+1, &p)){
            goto allocfail;
        }
        ptr[i]=p;
    }

    extl_table_gets_s(completions, "common_part", &beg);
    
    n=edln_do_completions(&(wedln->edln), ptr, n, beg);
    i=n;

    if(beg!=NULL)
        free(beg);
    
    if(n>1){
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

/*}}}*/


/*{{{ Init, deinit and config update */


static bool wedln_init_prompt(WEdln *wedln, const char *prompt)
{
    char *p;
    
    if(prompt!=NULL){
        p=scat(prompt, "  ");
    
        if(p==NULL){
            warn_err();
            return FALSE;
        }
        wedln->prompt=p;
        wedln->prompt_len=strlen(p);
    }else{
        wedln->prompt=NULL;
        wedln->prompt_len=0;
    }
    wedln->prompt_w=0;
    
    return TRUE;
}


static bool wedln_init(WEdln *wedln, WWindow *par, const WRectangle *geom, 
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
    wedln->edln.completion_handler=(EdlnCompletionHandler*)wedln_completion_handler;

    init_listing(&(wedln->complist));
    
    if(!input_init((WInput*)wedln, par, geom)){
        edln_deinit(&(wedln->edln));
        free(wedln->prompt);
        return FALSE;
    }

    window_create_xic(&wedln->input.win);
    
    wedln->handler=extl_ref_fn(params->handler);
    wedln->completor=extl_ref_fn(params->completor);

    region_add_bindmap((WRegion*)wedln, &querymod_wedln_bindmap);
    
    return TRUE;
}


WEdln *create_wedln(WWindow *par, const WRectangle *geom, 
                    WEdlnCreateParams *params)
{
    CREATEOBJ_IMPL(WEdln, wedln, (p, par, geom, params));
}


static void wedln_deinit(WEdln *wedln)
{
    if(wedln->prompt!=NULL)
        free(wedln->prompt);

    if(wedln->complist.strs!=NULL)
        deinit_listing(&(wedln->complist));

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
    ioncore_defer_action((Obj*)wedln, (WDeferredAction*)wedln_do_finish);
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
    edln_insstr_n(&(wedln->edln), buf, n);
}


static const char *wedln_style(WEdln *wedln)
{
    return "input-edln";
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab wedln_dynfuntab[]={
    {window_draw,        wedln_draw},
    {input_calc_size,     wedln_calc_size},
    {input_scrollup,     wedln_scrollup_completions},
    {input_scrolldown,    wedln_scrolldown_completions},
    {window_insstr,        wedln_insstr},
    {(DynFun*)input_style,
     (DynFun*)wedln_style},
    END_DYNFUNTAB
};


IMPLCLASS(WEdln, WInput, wedln_deinit, wedln_dynfuntab);

    
/*}}}*/
