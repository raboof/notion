/*
 * ion/mod_query/wedln.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/setparam.h>
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
#include <ioncore/gr-util.h>
#include <ioncore/sizehint.h>
#include <ioncore/resize.h>
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
                              GrAttr a)
{
    if(len==0)
        return 0;
    
    grbrush_set_attr(WEDLN_BRUSH(wedln), a);
    grbrush_draw_string(WEDLN_BRUSH(wedln), x, y, str, len, TRUE);
    grbrush_unset_attr(WEDLN_BRUSH(wedln), a);
    
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

#define DSTRSECT(LEN, A)                                    \
    if(LEN>0){                                              \
        tx+=wedln_draw_strsect(wedln, geom, geom->x+tx, ty, \
                               str, LEN, GR_ATTR(A));       \
        str+=LEN; len-=LEN;                                 \
    }


GR_DEFATTR(active);
GR_DEFATTR(inactive);
GR_DEFATTR(normal);
GR_DEFATTR(selection);
GR_DEFATTR(cursor);
GR_DEFATTR(prompt);
GR_DEFATTR(info);

static void init_attr()
{
    GR_ALLOCATTR_BEGIN;
    GR_ALLOCATTR(active);
    GR_ALLOCATTR(inactive);
    GR_ALLOCATTR(normal);
    GR_ALLOCATTR(selection);
    GR_ALLOCATTR(cursor);
    GR_ALLOCATTR(prompt);
    GR_ALLOCATTR(info);
    GR_ALLOCATTR_END;
}


static void wedln_do_draw_str_box(WEdln *wedln, const WRectangle *geom,
                                  const char *str, int cursor, 
                                  int mark, int tx)
{
    int len=strlen(str), ll=0, ty=0;
    
    ty=calc_text_y(wedln, geom);

    if(mark<=cursor){
        if(mark>=0){
            DSTRSECT(mark, normal);
            DSTRSECT(cursor-mark, selection);
        }else{
            DSTRSECT(cursor, normal);
        }
        if(len==0){
            tx+=wedln_draw_strsect(wedln, geom, geom->x+tx, ty,
                                   " ", 1, GR_ATTR(cursor));
        }else{
            ll=str_nextoff(str, 0);
            DSTRSECT(ll, cursor);
        }
    }else{
        DSTRSECT(cursor, normal);
        ll=str_nextoff(str, 0);
        DSTRSECT(ll, cursor);
        DSTRSECT(mark-cursor-ll, selection);
    }
    DSTRSECT(len, normal);

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

    grbrush_begin(WEDLN_BRUSH(wedln), geom, 
                  GRBRUSH_AMEND|GRBRUSH_KEEP_ATTR|GRBRUSH_NEED_CLIP);
    
    wedln_do_draw_str_box(wedln, geom, str+vstart+dstart, point, mark, tx);

    grbrush_end(WEDLN_BRUSH(wedln));
}


static bool wedln_update_cursor(WEdln *wedln, int iw)
{
    int cx, l;
    int vstart=wedln->vstart;
    int point=wedln->edln.point;
    int len=wedln->edln.psize;
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
    int w=1, h=1;
    
    if(WEDLN_BRUSH(wedln)!=NULL)
        mod_query_get_minimum_extents(WEDLN_BRUSH(wedln), with_spacing, &w, &h);
    
    return h;
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
    geom->w=maxof(0, geom->w - wedln->prompt_w - wedln->info_w);
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
    
    if(wedln->info!=NULL){
        wedln->info_w=grbrush_get_text_width(WEDLN_BRUSH(wedln),
                                             wedln->info, 
                                             wedln->info_len);
    }

    th=get_textarea_height(wedln, wedln->compl_list.strs!=NULL);
    
    if(wedln->compl_list.strs==NULL){
        if(max_geom.h<th || !(wedln->input.last_fp.mode&REGION_FIT_BOUNDS))
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
        
        if(h+th>max_geom.h || !(wedln->input.last_fp.mode&REGION_FIT_BOUNDS))
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


void wedln_size_hints(WEdln *wedln, WSizeHints *hints_ret)
{
    int w=1, h=1;
    
    if(WEDLN_BRUSH(wedln)!=NULL){
        mod_query_get_minimum_extents(WEDLN_BRUSH(wedln), FALSE, &w, &h);
        w+=wedln->prompt_w+wedln->info_w;
        w+=grbrush_get_text_width(WEDLN_BRUSH(wedln), "xxxxxxxxxx", 10);
    }
        
    hints_ret->min_set=TRUE;
    hints_ret->min_width=w;
    hints_ret->min_height=h;
}


/*}}}*/


/*{{{ Draw */


void wedln_draw_completions(WEdln *wedln, int mode)
{
    WRectangle geom;
    
    if(wedln->compl_list.strs!=NULL && WEDLN_BRUSH(wedln)!=NULL){
        get_completions_geom(wedln, G_CURRENT, &geom);
        
        draw_listing(WEDLN_BRUSH(wedln), &geom, &(wedln->compl_list), 
                     mode, GR_ATTR(selection));
    }
}


void wedln_draw_textarea(WEdln *wedln)    
{
    WRectangle geom;
    int ty;

    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_outer_geom(wedln, G_CURRENT, &geom);
    
    /*grbrush_begin(WEDLN_BRUSH(wedln), &geom, GRBRUSH_AMEND);*/
    
    grbrush_draw_border(WEDLN_BRUSH(wedln), &geom);

    get_inner_geom(wedln, G_CURRENT, &geom);

    ty=calc_text_y(wedln, &geom);

    grbrush_set_attr(WEDLN_BRUSH(wedln), GR_ATTR(prompt));
   
    if(wedln->prompt!=NULL){
        grbrush_draw_string(WEDLN_BRUSH(wedln), geom.x, ty,
                            wedln->prompt, wedln->prompt_len, TRUE);
    }
    
    if(wedln->info!=NULL){
        int x=geom.x+geom.w-wedln->info_w;
        
        grbrush_set_attr(WEDLN_BRUSH(wedln), GR_ATTR(info));
        grbrush_draw_string(WEDLN_BRUSH(wedln), x, ty,
                            wedln->info, wedln->info_len, TRUE);
        grbrush_unset_attr(WEDLN_BRUSH(wedln), GR_ATTR(info));
    }

    grbrush_unset_attr(WEDLN_BRUSH(wedln), GR_ATTR(prompt));
        
    get_textarea_geom(wedln, G_CURRENT, &geom);
    
    wedln_draw_str_box(wedln, &geom, wedln->vstart, wedln->edln.p, 0,
                       wedln->edln.point, wedln->edln.mark);

    /*grbrush_end(WEDLN_BRUSH(wedln));*/
}


static void wedln_draw_(WEdln *wedln, bool complete, bool completions)
{
    WRectangle g;
    int f=(complete ? 0 : GRBRUSH_NO_CLEAR_OK);
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    get_geom(wedln, G_CURRENT, &g);
    
    grbrush_begin(WEDLN_BRUSH(wedln), &g, f);
    
    grbrush_set_attr(WEDLN_BRUSH(wedln), REGION_IS_ACTIVE(wedln) 
                                         ? GR_ATTR(active) 
                                         : GR_ATTR(inactive));

    if(completions)
        wedln_draw_completions(wedln, LISTING_DRAW_ALL);
    
    wedln_draw_textarea(wedln);
    
    grbrush_end(WEDLN_BRUSH(wedln));
}


void wedln_draw(WEdln *wedln, bool complete)
{
    wedln_draw_(wedln, complete, TRUE);
}

/*}}} */


/*{{{ Info area */


static void wedln_set_info(WEdln *wedln, const char *info)
{
    WRectangle tageom;
    
    if(wedln->info!=NULL){
        free(wedln->info);
        wedln->info=NULL;
        wedln->info_w=0;
        wedln->info_len=0;
    }
    
    if(info!=NULL){
        wedln->info=scat3("  [", info, "]");
        if(wedln->info!=NULL){
            wedln->info_len=strlen(wedln->info);
            if(WEDLN_BRUSH(wedln)!=NULL){
                wedln->info_w=grbrush_get_text_width(WEDLN_BRUSH(wedln),
                                                     wedln->info, 
                                                     wedln->info_len);
            }
        }
    }
    
    get_textarea_geom(wedln, G_CURRENT, &tageom);
    wedln_update_cursor(wedln, tageom.w);
    
    wedln_draw_(wedln, FALSE, FALSE);
}


/*}}}*/


/*{{{ Completions */


static void wedln_show_completions(WEdln *wedln, char **strs, int nstrs,
                                   int selected)
{
    int w=REGION_GEOM(wedln).w;
    int h=REGION_GEOM(wedln).h;
    
    if(WEDLN_BRUSH(wedln)==NULL)
        return;
    
    setup_listing(&(wedln->compl_list), strs, nstrs, FALSE);
    wedln->compl_list.selected_str=selected;

    input_refit((WInput*)wedln);
    if(w==REGION_GEOM(wedln).w && h==REGION_GEOM(wedln).h)
        wedln_draw_completions(wedln, LISTING_DRAW_COMPLETE);
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
        wedln_draw_completions(wedln, LISTING_DRAW_COMPLETE);
}


void wedln_scrolldown_completions(WEdln *wedln)
{
    if(wedln->compl_list.strs==NULL)
        return;
    if(scrolldown_listing(&(wedln->compl_list)))
        wedln_draw_completions(wedln, LISTING_DRAW_COMPLETE);
}


static int update_nocompl=0;


static void free_completions(char **ptr, int i)
{
    while(i>0){
        i--;
        if(ptr[i]!=NULL)
            free(ptr[i]);
    }
    free(ptr);
}


bool wedln_do_set_completions(WEdln *wedln, char **ptr, int n, 
                              char *beg, char *end, int cycle,
                              bool nosort)
{    
    int sel=-1;
    
    if(wedln->compl_beg!=NULL)
        free(wedln->compl_beg);
    
    if(wedln->compl_end!=NULL)
        free(wedln->compl_end);
    
    wedln->compl_beg=beg;
    wedln->compl_end=end;
    wedln->compl_current_id=-1;
    
    n=edln_do_completions(&(wedln->edln), ptr, n, beg, end, 
                          !mod_query_config.autoshowcompl, nosort);

    if(mod_query_config.autoshowcompl && n>0 && cycle!=0){
        update_nocompl++;
        sel=(cycle>0 ? 0 : n-1);
        edln_set_completion(&(wedln->edln), ptr[sel], beg, end);
        update_nocompl--;
    }

    if(n>1 || (mod_query_config.autoshowcompl && n>0)){
        wedln_show_completions(wedln, ptr, n, sel);
        return TRUE;
    }
    
    free_completions(ptr, n);
    
    return FALSE;
}


void wedln_set_completions(WEdln *wedln, ExtlTab completions, int cycle)
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
    
    if(!wedln_do_set_completions(wedln, ptr, n, beg, end, cycle, FALSE))
        wedln_hide_completions(wedln);

    return;
    
allocfail:
    wedln_hide_completions(wedln);
    free_completions(ptr, i);
}
    

static void wedln_do_select_completion(WEdln *wedln, int n)
{
    bool redraw=listing_select(&(wedln->compl_list), n);
    wedln_draw_completions(wedln, redraw);

    update_nocompl++;
    edln_set_completion(&(wedln->edln), wedln->compl_list.strs[n],
                        wedln->compl_beg, wedln->compl_end);
    update_nocompl--;
}



static ExtlExportedFn *sc_safe_fns[]={
    (ExtlExportedFn*)&complproxy_set_completions,
    NULL
};


static ExtlSafelist sc_safelist=EXTL_SAFELIST_INIT(sc_safe_fns);


static int wedln_alloc_compl_id(WEdln *wedln)
{
    int id=wedln->compl_waiting_id+1;
    wedln->compl_waiting_id=maxof(0, wedln->compl_waiting_id+1);
    return id;
}

static bool wedln_do_call_completor(WEdln *wedln, int id, int cycle)
{
    if(wedln->compl_history_mode){
        uint n;
        char **h=NULL;
        
        wedln->compl_waiting_id=id;

        n=edln_history_matches(&wedln->edln, &h);
        
        if(n==0){
            wedln_hide_completions(wedln);
            return FALSE;
        }
        
        if(wedln_do_set_completions(wedln, h, n, NULL, NULL, cycle, TRUE)){
            wedln->compl_current_id=id;
            return TRUE;
        }
        
        return FALSE;
    }else{
        const char *p=wedln->edln.p;
        int point=wedln->edln.point;
        WComplProxy *proxy=create_complproxy(wedln, id, cycle);
        
        if(proxy==NULL)
            return FALSE;
        
        /* Set Lua-side to own the proxy: it gets freed by Lua's GC */
        ((Obj*)proxy)->flags|=OBJ_EXTL_OWNED;
        
        if(p==NULL){
            p="";
            point=0;
        }
        
        extl_protect(&sc_safelist);
        extl_call(wedln->completor, "osi", NULL, proxy, p, point);
        extl_unprotect(&sc_safelist);
        
        return TRUE;
    }
}


static void timed_complete(WTimer *tmr, Obj *obj)
{
    WEdln *wedln=(WEdln*)obj;
    
    if(wedln!=NULL){
        int id=wedln->compl_timed_id;
        wedln->compl_timed_id=-1;
        if(id==wedln->compl_waiting_id && id>=0)
            wedln_do_call_completor((WEdln*)obj, id, FALSE);
    }
}


/*EXTL_DOC
 * Select next completion.
 */
EXTL_EXPORT_MEMBER
bool wedln_next_completion(WEdln *wedln)
{
    int n=-1;

    if(wedln->compl_current_id!=wedln->compl_waiting_id)
        return FALSE;
    
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

    if(wedln->compl_current_id!=wedln->compl_waiting_id)
        return FALSE;
    
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
 * current cursor position, or select next/previous completion from list if in
 * auto-show-completions mode and \var{cycle} is set to \codestr{next} or 
 * \codestr{prev}, respectively. 
 * The \var{mode} may be \codestr{history} or \codestr{normal}. If it is 
 * not set, the previous mode is used. Normally next entry is not cycled to
 * despite the setting of \var{cycle} if mode switch occurs. To override
 * this, use \codestr{next-always} and \codestr{prev-always} for \var{cycle}.
 */
EXTL_EXPORT_MEMBER
void wedln_complete(WEdln *wedln, const char *cycle, const char *mode)
{
    bool valid=TRUE;
    int cyclei=0;
    
    if(mode!=NULL){
        if(strcmp(mode, "history")==0){
            valid=wedln->compl_history_mode;
            wedln->compl_history_mode=TRUE;
        }else if(strcmp(mode, "normal")==0){
            valid=!wedln->compl_history_mode;
            wedln->compl_history_mode=FALSE;
        }
        if(!valid){
            wedln_set_info(wedln,
                           (wedln->compl_history_mode
                            ? TR("history")
                            : NULL));
        }
    }
    
    if(cycle!=NULL){
        if((valid && strcmp(cycle, "next")==0) || 
           strcmp(cycle, "next-always")==0){
            cyclei=1;
        }else if((valid && strcmp(cycle, "prev")==0) || 
                 strcmp(cycle, "prev-always")==0){
            cyclei=-1;
        }
    }
    
    if(valid && cyclei!=0 && mod_query_config.autoshowcompl && 
       wedln->compl_list.nstrs>0){
        if(cyclei>0)
            wedln_next_completion(wedln);
        else
            wedln_prev_completion(wedln);
    }else{
        int oldid=wedln->compl_waiting_id;
    
        if(!wedln_do_call_completor(wedln, wedln_alloc_compl_id(wedln), 
                                    cyclei)){
            wedln->compl_waiting_id=oldid;
        }
    }
}


/*EXTL_DOC
 * Get history completion mode.
 */
EXTL_EXPORT_MEMBER
bool wedln_is_histcompl(WEdln *wedln)
{
    return wedln->compl_history_mode;
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
        wedln->compl_current_id=-1; /* invalidate */
        if(wedln->autoshowcompl_timer==NULL)
            wedln->autoshowcompl_timer=create_timer();
        if(wedln->autoshowcompl_timer!=NULL){
            wedln->compl_timed_id=wedln_alloc_compl_id(wedln);
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

    init_attr();
    
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

    wedln->compl_waiting_id=-1;
    wedln->compl_current_id=-1;
    wedln->compl_timed_id=-1;
    wedln->compl_beg=NULL;
    wedln->compl_end=NULL;
    wedln->compl_tab=FALSE;
    wedln->compl_history_mode=FALSE;
    
    wedln->info=NULL;
    wedln->info_len=0;
    wedln->info_w=0;
    
    wedln->cycle_bindmap=NULL;
    
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
        
    if(wedln->info!=NULL)
        free(wedln->info);

    if(wedln->compl_beg!=NULL)
        free(wedln->compl_beg);

    if(wedln->compl_end!=NULL)
        free(wedln->compl_end);

    if(wedln->compl_list.strs!=NULL)
        deinit_listing(&(wedln->compl_list));

    if(wedln->autoshowcompl_timer!=NULL)
        destroy_obj((Obj*)wedln->autoshowcompl_timer);

    if(wedln->cycle_bindmap!=NULL)
        bindmap_destroy(wedln->cycle_bindmap);
    
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
    
    region_rqdispose((WRegion*)wedln);
    
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
    {region_size_hints, wedln_size_hints},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WEdln, WInput, wedln_deinit, wedln_dynfuntab);

    
/*}}}*/
