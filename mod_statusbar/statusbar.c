/*
 * ion/mod_statusbar/statusbar.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/ptrlist.h>
#include <libtu/misc.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/binding.h>
#include <ioncore/regbind.h>
#include <ioncore/event.h>
#include <ioncore/resize.h>
#include <ioncore/gr.h>
#include <ioncore/gr-util.h>
#include <ioncore/names.h>
#include <ioncore/strings.h>
#include <ioncore/basicpholder.h>
#include <ioncore/sizehint.h>

#include "statusbar.h"
#include "main.h"
#include "draw.h"


static void statusbar_set_elems(WStatusBar *sb, ExtlTab t);
static void statusbar_free_elems(WStatusBar *sb);
static void statusbar_update_natural_size(WStatusBar *p);
static void statusbar_arrange_systray(WStatusBar *p);
static int statusbar_systray_x(WStatusBar *p);
static void statusbar_rearrange(WStatusBar *sb, bool rs);
static void do_calc_systray_w(WStatusBar *p, WSBElem *el);
static void statusbar_calc_systray_w(WStatusBar *p);

static WStatusBar *statusbars=NULL;


/*{{{ Init/deinit */


bool statusbar_init(WStatusBar *p, WWindow *parent, const WFitParams *fp)
{
    if(!window_init(&(p->wwin), parent, fp))
        return FALSE;

    p->brush=NULL;
    p->elems=NULL;
    p->nelems=0;
    p->natural_w=1;
    p->natural_h=1;
    p->filleridx=-1;
    p->sb_next=NULL;
    p->sb_prev=NULL;
    p->traywins=NULL;
    p->systray_enabled=TRUE;
    
    statusbar_updategr(p);

    if(p->brush==NULL){
        window_deinit(&(p->wwin));
        return FALSE;
    }
    
    window_select_input(&(p->wwin), IONCORE_EVENTMASK_NORMAL);
    
    region_register((WRegion*)p);

    region_add_bindmap((WRegion*)p, mod_statusbar_statusbar_bindmap);
    
    LINK_ITEM(statusbars, p, sb_next, sb_prev);
    
    return TRUE;
}



WStatusBar *create_statusbar(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WStatusBar, statusbar, (p, parent, fp));
}


void statusbar_deinit(WStatusBar *p)
{
    UNLINK_ITEM(statusbars, p, sb_next, sb_prev);

    statusbar_free_elems(p);
    
    if(p->brush!=NULL){
        grbrush_release(p->brush);
        p->brush=NULL;
    }
    
    window_deinit(&(p->wwin));
}


/*}}}*/


/*{{{ Content stuff */


static void init_sbelem(WSBElem *el)
{
    el->type=WSBELEM_NONE;
    el->text_w=0;
    el->text=NULL;
    el->max_w=0;
    el->tmpl=NULL;
    el->meter=STRINGID_NONE;
    el->attr=STRINGID_NONE;
    el->stretch=0;
    el->align=WSBELEM_ALIGN_CENTER;
    el->zeropad=0;
    el->x=0;
    el->traywins=NULL;
}


static bool gets_stringstore(ExtlTab t, const char *str, StringId *id)
{
    char *s;
    
    if(extl_table_gets_s(t, str, &s)){
        *id=stringstore_alloc(s);
        free(s);
        return (*id!=STRINGID_NONE);
    }
    
    return FALSE;
}

    
static WSBElem *get_sbelems(ExtlTab t, int *nret, int *filleridxret)
{
    int i, n=extl_table_get_n(t);
    WSBElem *el;
    int systrayidx=-1;
    
    *nret=0;
    *filleridxret=-1;
    
    if(n<=0)
        return NULL;
    
    el=ALLOC_N(WSBElem, n); 
    
    if(el==NULL)
        return NULL;
    
    for(i=0; i<n; i++){
        ExtlTab tt;
        
        init_sbelem(&el[i]);

        if(extl_table_geti_t(t, i+1, &tt)){
            if(extl_table_gets_i(tt, "type", &(el[i].type))){
                if(el[i].type==WSBELEM_TEXT || el[i].type==WSBELEM_STRETCH){
                    extl_table_gets_s(tt, "text", &(el[i].text));
                }else if(el[i].type==WSBELEM_METER){
                    gets_stringstore(tt, "meter", &(el[i].meter));
                    extl_table_gets_s(tt, "tmpl", &(el[i].tmpl));
                    extl_table_gets_i(tt, "align", &(el[i].align));
                    extl_table_gets_i(tt, "zeropad", &(el[i].zeropad));
                    el[i].zeropad=maxof(el[i].zeropad, 0);
                }else if(el[i].type==WSBELEM_SYSTRAY){
                    const char *tmp;
                    
                    gets_stringstore(tt, "meter", &(el[i].meter));
                    extl_table_gets_i(tt, "align", &(el[i].align));
                    
                    tmp=stringstore_get(el[i].meter);
                    
                    if(tmp==NULL || strcmp(tmp, "systray")==0)
                        systrayidx=i;
                }else if(el[i].type==WSBELEM_FILLER){
                    *filleridxret=i;
                }
            }
            extl_unref_table(tt);
        }
    }
    
    if(systrayidx==-1){
        WSBElem *el2=REALLOC_N(el, WSBElem, n, n+1);
        if(el2!=NULL){
            el=el2;
            init_sbelem(&el[n]);
            el[n].type=WSBELEM_SYSTRAY;
            n++;
        }
    }
    
    *nret=n;
    
    return el;
}
    

static void free_sbelems(WSBElem *el, int n)
{
    int i;
    
    for(i=0; i<n; i++){
        if(el[i].text!=NULL)
            free(el[i].text);
        if(el[i].tmpl!=NULL)
            free(el[i].tmpl);
        if(el[i].meter!=STRINGID_NONE)
            stringstore_free(el[i].meter);
        if(el[i].attr!=STRINGID_NONE)
            stringstore_free(el[i].attr);
        if(el[i].traywins!=NULL)
            ptrlist_clear(&el[i].traywins);
    }
    
    free(el);
}


static void statusbar_set_elems(WStatusBar *sb, ExtlTab t)
{
    statusbar_free_elems(sb);
    
    sb->elems=get_sbelems(t, &(sb->nelems), &(sb->filleridx));
}


static void statusbar_free_elems(WStatusBar *sb)
{
    if(sb->elems!=NULL){
        free_sbelems(sb->elems, sb->nelems);
        sb->elems=NULL;
        sb->nelems=0;
        sb->filleridx=-1;
    }
}


/*}}}*/



/*{{{ Size stuff */


static void statusbar_resize(WStatusBar *p)
{
    WRQGeomParams rq=RQGEOMPARAMS_INIT;
    
    rq.flags=REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y;
    
    rq.geom.w=p->natural_w;
    rq.geom.h=p->natural_h;
    rq.geom.x=REGION_GEOM(p).x;
    rq.geom.y=REGION_GEOM(p).y;

    if(rectangle_compare(&rq.geom, &REGION_GEOM(p))!=RECTANGLE_SAME)
        region_rqgeom((WRegion*)p, &rq, NULL);
}


static void calc_elem_w(WStatusBar *p, WSBElem *el, GrBrush *brush)
{
    const char *str;

    if(el->type==WSBELEM_SYSTRAY){
        do_calc_systray_w(p, el);
        return;
    }
    
    if(brush==NULL){
        el->text_w=0;
        return;
    }
    
    if(el->type==WSBELEM_METER){
        str=(el->text!=NULL ? el->text : STATUSBAR_NX_STR);
        el->text_w=grbrush_get_text_width(brush, str, strlen(str));
        str=el->tmpl;
        el->max_w=maxof((str!=NULL
                         ? grbrush_get_text_width(brush, str, strlen(str))
                         : 0),
                        el->text_w);
    }else{
        str=el->text;
        el->text_w=(str!=NULL
                    ? grbrush_get_text_width(brush, str, strlen(str))
                    : 0);
        el->max_w=el->text_w;
    }
}


static void statusbar_calc_widths(WStatusBar *sb)
{
    int i;
    
    for(i=0; i<sb->nelems; i++)
        calc_elem_w(sb, &(sb->elems[i]), sb->brush);
}


static void statusbar_do_update_natural_size(WStatusBar *p)
{
    GrBorderWidths bdw;
    GrFontExtents fnte;
    WRegion *reg;
    PtrListIterTmp tmp;
    int totw=0, stmh=0;
    int i;

    if(p->brush==NULL){
        bdw.left=0; bdw.right=0;
        bdw.top=0; bdw.bottom=0;
        fnte.max_height=4;
    }else{
        grbrush_get_border_widths(p->brush, &bdw);
        grbrush_get_font_extents(p->brush, &fnte);
    }
    
    for(i=0; i<p->nelems; i++)
        totw+=p->elems[i].max_w;
        
    FOR_ALL_ON_PTRLIST(WRegion*, reg, p->traywins, tmp){
        stmh=maxof(stmh, REGION_GEOM(reg).h);
    }
    
    p->natural_w=bdw.left+totw+bdw.right;
    p->natural_h=maxof(stmh, fnte.max_height)+bdw.top+bdw.bottom;
}


void statusbar_size_hints(WStatusBar *p, WSizeHints *h)
{
    h->min_set=TRUE;
    h->min_width=p->natural_w;
    h->min_height=p->natural_h;
    
    h->max_set=TRUE;
    h->max_width=INT_MAX;/*p->natural_w;*/
    h->max_height=p->natural_h;
}


/*}}}*/


/*{{{ Systray */


static WSBElem *statusbar_associate_systray(WStatusBar *sb, WRegion *reg)
{
    WClientWin *cwin=OBJ_CAST(reg, WClientWin);
    WSBElem *el=NULL, *fbel=NULL;
    char *name=NULL;
    int i;
    
    if(cwin!=NULL)
        extl_table_gets_s(cwin->proptab, "statusbar", &name);
    
    for(i=0; i<sb->nelems; i++){
        const char *meter;
        
        if(sb->elems[i].type!=WSBELEM_SYSTRAY)
            continue;
        
        meter=stringstore_get(sb->elems[i].meter);
        
        if(meter==NULL){
            fbel=&sb->elems[i];
            continue;
        }
        if(name!=NULL && strcmp(meter, name)==0){
            el=&sb->elems[i];
            break;
        }
        if(strcmp(meter, "systray")==0)
            fbel=&sb->elems[i];
    }
    
    if(name!=NULL)
        free(name);
        
    if(el==NULL)
        el=fbel;
    
    if(el==NULL)
        return NULL;
    
    ptrlist_insert_last(&el->traywins, (Obj*)reg);
    
    return el;
}


static WSBElem *statusbar_unassociate_systray(WStatusBar *sb, WRegion *reg)
{
    int i;
    
    for(i=0; i<sb->nelems; i++){
        if(ptrlist_remove(&(sb->elems[i].traywins), (Obj*)reg))
            return &sb->elems[i];
    }
    
    return NULL;
}

    

static void do_calc_systray_w(WStatusBar *p, WSBElem *el)
{
    WRegion *reg;
    PtrListIterTmp tmp;
    int padding=0;
    int w=-padding;
    
    FOR_ALL_ON_PTRLIST(WRegion*, reg, el->traywins, tmp){
        w=w+REGION_GEOM(reg).w+padding;
    }
    
    el->text_w=maxof(0, w);
    el->max_w=el->text_w; /* for now */
}


static void statusbar_calc_systray_w(WStatusBar *p)
{
    int i;
    
    for(i=0; i<p->nelems; i++){
        if(p->elems[i].type==WSBELEM_SYSTRAY)
            do_calc_systray_w(p, &p->elems[i]);
    }
}


static void statusbar_arrange_systray(WStatusBar *p)
{
    WRegion *reg;
    PtrListIterTmp tmp;
    GrBorderWidths bdw;
    int padding=0, ymiddle;
    int i, x;
    
    if(p->brush!=NULL){
        grbrush_get_border_widths(p->brush, &bdw);
    }else{
        bdw.top=0;
        bdw.bottom=0;
    }
    
    ymiddle=bdw.top+(REGION_GEOM(p).h-bdw.top-bdw.bottom)/2;
    
    for(i=0; i<p->nelems; i++){
        WSBElem *el=&p->elems[i];
        if(el->type!=WSBELEM_SYSTRAY)
            continue;
        x=el->x;
        FOR_ALL_ON_PTRLIST(WRegion*, reg, el->traywins, tmp){
            WRectangle g=REGION_GEOM(reg);
            g.x=x;
            g.y=ymiddle-g.h/2;
            region_fit(reg, &g, REGION_FIT_EXACT);
            x=x+g.w+padding;
        }
    }
}


static void systray_adjust_size(WRegion *reg, WRectangle *g)
{
    g->h=CF_STATUSBAR_SYSTRAY_HEIGHT;
    
    region_size_hints_correct(reg, &g->w, &g->h, TRUE);
}



static WRegion *statusbar_do_attach_final(WStatusBar *sb,
                                          WRegion *reg,
                                          void *unused)
{
    WFitParams fp;
    WSBElem *el;
    
    if(!ptrlist_insert_last(&sb->traywins, (Obj*)reg))
        return NULL;
    
    el=statusbar_associate_systray(sb, reg);
    if(el==NULL){
        ptrlist_remove(&sb->traywins, (Obj*)reg);
        return NULL;
    }

    fp.g=REGION_GEOM(reg);
    fp.mode=REGION_FIT_EXACT;
    systray_adjust_size(reg, &fp.g);
    
    region_fitrep(reg, NULL, &fp);
    
    do_calc_systray_w(sb, el);

    region_set_manager(reg, (WRegion*)sb);
    
    statusbar_rearrange(sb, TRUE);
    
    if(REGION_IS_MAPPED(sb))
        region_map(reg);
    
    return reg;
}


static WRegion *statusbar_do_attach(WStatusBar *sb, WRegionAttachData *data)
{
    WFitParams fp;
    
    fp.g.x=0;
    fp.g.y=0;
    fp.g.h=CF_STATUSBAR_SYSTRAY_HEIGHT;
    fp.g.w=CF_STATUSBAR_SYSTRAY_HEIGHT;
    fp.mode=REGION_FIT_WHATEVER|REGION_FIT_BOUNDS;
    
    return region_attach_helper((WRegion*)sb, (WWindow*)sb, &fp,
                                (WRegionDoAttachFn*)statusbar_do_attach_final, 
                                NULL, data);
}


static WRegion *statusbar_attach_ph(WStatusBar *sb, int flags,
                                    WRegionAttachData *data)
{
    return statusbar_do_attach(sb, data);
}


static WPHolder *statusbar_prepare_manage(WStatusBar *sb, 
                                          const WClientWin *cwin,
                                          const WManageParams *param,
                                          int priority)
{
    if(!MANAGE_PRIORITY_OK(priority, MANAGE_PRIORITY_LOW))
        return NULL;
    
    return (WPHolder*)create_basicpholder((WRegion*)sb, 
                                          ((WBasicPHolderHandler*)
                                           statusbar_attach_ph));
}


static void statusbar_managed_remove(WStatusBar *sb, WRegion *reg)
{
    WSBElem *el;
        
    ptrlist_remove(&sb->traywins, (Obj*)reg);
    
    el=statusbar_unassociate_systray(sb, reg);
    
    region_unset_manager(reg, (WRegion*)sb);

    if(el!=NULL && ioncore_g.opmode!=IONCORE_OPMODE_DEINIT){
        do_calc_systray_w(sb, el);
        statusbar_rearrange(sb, TRUE);
    }
}


static void statusbar_managed_rqgeom(WStatusBar *sb, WRegion *reg, 
                                     const WRQGeomParams *rq,
                                     WRectangle *geomret)
{
    WRectangle g;
    
    g.x=REGION_GEOM(reg).x;
    g.y=REGION_GEOM(reg).y;
    g.w=rq->geom.w;
    g.h=rq->geom.h;

    systray_adjust_size(reg, &g);

    if(rq->flags&REGION_RQGEOM_TRYONLY){
        if(geomret!=NULL)
            *geomret=g;
        return;
    }
    
    region_fit(reg, &g, REGION_FIT_EXACT);
    
    statusbar_calc_systray_w(sb);
    statusbar_rearrange(sb, TRUE);
    
    if(geomret!=NULL)
        *geomret=REGION_GEOM(reg);
    
}


void statusbar_map(WStatusBar *sb)
{
    WRegion *reg;
    PtrListIterTmp tmp;
    
    window_map((WWindow*)sb);
    
    FOR_ALL_ON_PTRLIST(WRegion*, reg, sb->traywins, tmp)
        region_map(reg);
}


void statusbar_unmap(WStatusBar *sb)
{
    WRegion *reg;
    PtrListIterTmp tmp;
    
    window_unmap((WWindow*)sb);
    
    FOR_ALL_ON_PTRLIST(WRegion*, reg, sb->traywins, tmp)
        region_unmap(reg);
}


bool statusbar_fitrep(WStatusBar *sb, WWindow *par, const WFitParams *fp)
{
    bool wchg=(REGION_GEOM(sb).w!=fp->g.w);
    bool hchg=(REGION_GEOM(sb).h!=fp->g.h);
    
    if(!window_fitrep(&(sb->wwin), par, fp))
        return FALSE;
    
    if(wchg || hchg){
        statusbar_calculate_xs(sb);
        statusbar_arrange_systray(sb);
        statusbar_draw(sb, TRUE);
    }
    
    return TRUE;
}


WPHolder *statusbar_prepare_manage_transient(WStatusBar *sb, 
                                             const WClientWin *cwin,
                                             const WManageParams *param,
                                             int unused)
{
    WRegion *mgr=REGION_MANAGER(sb);
    
    if(mgr==NULL)
        mgr=(WRegion*)region_screen_of((WRegion*)sb);
    
    if(mgr!=NULL)
        return region_prepare_manage(mgr, cwin, param, 
                                     MANAGE_PRIORITY_NONE);
    else
        return NULL;
}

    

/*}}}*/


/*{{{ Exports */


static ExtlFn parse_template_fn;
static bool parse_template_fn_set=FALSE;


EXTL_EXPORT
void mod_statusbar__set_template_parser(ExtlFn fn)
{
    if(parse_template_fn_set)
        extl_unref_fn(parse_template_fn);
    parse_template_fn=extl_ref_fn(fn);
    parse_template_fn_set=TRUE;
}


/*EXTL_DOC
 * Set statusbar template.
 */
EXTL_EXPORT_MEMBER
void statusbar_set_template(WStatusBar *sb, const char *tmpl)
{
    ExtlTab t=extl_table_none();
    bool ok=FALSE;
    
    if(parse_template_fn_set){
        extl_protect(NULL);
        ok=extl_call(parse_template_fn, "s", "t", tmpl, &t);
        extl_unprotect(NULL);
    }

    if(ok)
        statusbar_set_template_table(sb, t);
}


/*EXTL_DOC
 * Set statusbar template as table.
 */
EXTL_EXPORT_MEMBER
void statusbar_set_template_table(WStatusBar *sb, ExtlTab t)
{
    WRegion *reg;
    PtrListIterTmp tmp;
    
    statusbar_set_elems(sb, t);

    FOR_ALL_ON_PTRLIST(WRegion*, reg, sb->traywins, tmp){
        statusbar_associate_systray(sb, reg);
    }
    
    statusbar_calc_widths(sb);
    statusbar_rearrange(sb, FALSE);
}


/*EXTL_DOC
 * Get statusbar template as table.
 */
EXTL_EXPORT_MEMBER
ExtlTab statusbar_get_template_table(WStatusBar *sb)
{
    int count = sb->nelems;
    int i;

    ExtlTab t = extl_create_table();

    for(i=0; i<count; i++){
        ExtlTab tt = extl_create_table();
        
        extl_table_sets_i(tt, "type", sb->elems[i].type);
        extl_table_sets_s(tt, "text", sb->elems[i].text);
        extl_table_sets_s(tt, "meter", stringstore_get(sb->elems[i].meter));
        extl_table_sets_s(tt, "tmpl", sb->elems[i].tmpl);
        extl_table_sets_i(tt, "align", sb->elems[i].align);
        extl_table_sets_i(tt, "zeropad", sb->elems[i].zeropad);

        extl_table_seti_t(t, (i+1), tt);
        extl_unref_table(tt);
    }

    return t;
}


static void reset_stretch(WStatusBar *sb)
{
    int i;
    
    for(i=0; i<sb->nelems; i++)
        sb->elems[i].stretch=0;
}


static void positive_stretch(WStatusBar *sb)
{
    int i;
    
    for(i=0; i<sb->nelems; i++)
        sb->elems[i].stretch=maxof(0, sb->elems[i].stretch);
}


static void spread_stretch(WStatusBar *sb)
{
    int i, j, k;
    int diff;
    WSBElem *el, *lel, *rel;
    const char *str;
    
    for(i=0; i<sb->nelems; i++){
        el=&(sb->elems[i]);

        if(el->type!=WSBELEM_METER && el->type!=WSBELEM_SYSTRAY)
            continue;
        
        diff=el->max_w-el->text_w;
        
        lel=NULL;
        rel=NULL;
        
        if(el->align!=WSBELEM_ALIGN_RIGHT){
            for(j=i+1; j<sb->nelems; j++){
                if(sb->elems[j].type==WSBELEM_STRETCH){
                    rel=&(sb->elems[j]);
                    break;
                }
            }
        }
        
        if(el->align!=WSBELEM_ALIGN_LEFT){
            for(k=i-1; k>=0; k--){
                if(sb->elems[k].type==WSBELEM_STRETCH){
                    lel=&(sb->elems[k]);
                    break;
                }
            }
        }
        
        if(rel!=NULL && lel!=NULL){
            int l=diff/2;
            int r=diff-l;
            lel->stretch+=l;
            rel->stretch+=r;
        }else if(lel!=NULL){
            lel->stretch+=diff;
        }else if(rel!=NULL){
            rel->stretch+=diff;
        }
    }
}


static void statusbar_rearrange(WStatusBar *sb, bool rs)
{
    if(rs){
        int onw=sb->natural_w;
        int onh=sb->natural_h;
        
        statusbar_do_update_natural_size(sb);
        
        if(    (sb->natural_h>onh && REGION_GEOM(sb).h>=onh)
            || (sb->natural_h<onh && REGION_GEOM(sb).h<=onh)
            || (sb->natural_w>onw && REGION_GEOM(sb).w>=onw)
            || (sb->natural_w<onw && REGION_GEOM(sb).w<=onw)){
            
            statusbar_resize(sb);
        }
    }

    reset_stretch(sb);
    spread_stretch(sb);
    positive_stretch(sb);
    statusbar_calculate_xs(sb);
    
    if(rs)
        statusbar_arrange_systray(sb);
}



/*EXTL_DOC
 * Set statusbar template.
 */
EXTL_EXPORT_MEMBER
void statusbar_update(WStatusBar *sb, ExtlTab t)
{
    int i;
    WSBElem *el;
    bool grow=FALSE;
    
    if(sb->brush==NULL)
        return;
    
    for(i=0; i<sb->nelems; i++){
        const char *meter;
        
        el=&(sb->elems[i]);
        
        if(el->type!=WSBELEM_METER)
            continue;
        
        if(el->text!=NULL){
            free(el->text);
            el->text=NULL;
        }

        if(el->attr!=GRATTR_NONE){
            stringstore_free(el->attr);
            el->attr=GRATTR_NONE;
        }
        
        meter=stringstore_get(el->meter);
        
        if(meter!=NULL){
            const char *str;
            char *attrnm;
            
            extl_table_gets_s(t, meter, &(el->text));
            
            if(el->text==NULL){
                str=STATUSBAR_NX_STR;
            }else{
                /* Zero-pad */
                int l=strlen(el->text);
                int ml=str_len(el->text);
                int diff=el->zeropad-ml;
                if(diff>0){
                    char *tmp=ALLOC_N(char, l+diff+1);
                    if(tmp!=NULL){
                        memset(tmp, '0', diff);
                        memcpy(tmp+diff, el->text, l+1);
                        free(el->text);
                        el->text=tmp;
                    }
                }
                str=el->text;
            }
            
            if(el->tmpl!=NULL && el->text!=NULL){
                char *tmp=grbrush_make_label(sb->brush, el->text, el->max_w);
                if(tmp!=NULL){
                    free(el->text);
                    el->text=tmp;
                    str=tmp;
                }
            }

            el->text_w=grbrush_get_text_width(sb->brush, str, strlen(str));
            
            if(el->text_w>el->max_w && el->tmpl==NULL){
                el->max_w=el->text_w;
                grow=TRUE;
            }
            
            attrnm=scat(meter, "_hint");
            if(attrnm!=NULL){
                char *s;
                if(extl_table_gets_s(t, attrnm, &s)){
                    el->attr=stringstore_alloc(s);
                    free(s);
                }
                free(attrnm);
            }
        }
    }

    statusbar_rearrange(sb, grow);
    
    window_draw((WWindow*)sb, FALSE);
}


/*}}}*/


/*{{{ Updategr */


void statusbar_updategr(WStatusBar *p)
{
    GrBrush *nbrush;
    
    nbrush=gr_get_brush(p->wwin.win, region_rootwin_of((WRegion*)p),
                        "stdisp-statusbar");
    if(nbrush==NULL)
        return;
    
    if(p->brush!=NULL)
        grbrush_release(p->brush);
    
    p->brush=nbrush;

    statusbar_calc_widths(p);
    statusbar_rearrange(p, TRUE);
    
    window_draw(&(p->wwin), TRUE);
}


/*}}}*/


/*{{{ Misc */


int statusbar_orientation(WStatusBar *sb)
{
    return REGION_ORIENTATION_HORIZONTAL;
}


/*EXTL_DOC
 * Returns a list of all statusbars.
 */
EXTL_EXPORT
ExtlTab mod_statusbar_statusbars()
{
    ExtlTab t=extl_create_table();
    WStatusBar *sb;
    int i=1;
    
    for(sb=statusbars; sb!=NULL; sb=sb->sb_next){
        extl_table_seti_o(t, i, (Obj*)sb);
        i++;
    }
    
    return t;
}


WStatusBar *mod_statusbar_find_suitable(WClientWin *cwin,
                                        const WManageParams *param)
{
    WStatusBar *sb;

    for(sb=statusbars; sb!=NULL; sb=sb->sb_next){
        /*if(!sb->is_auto)
            continue;*/
        if(!sb->systray_enabled)
            continue;
        if(!region_same_rootwin((WRegion*)sb, (WRegion*)cwin))
            continue;
        break;
    }

    return sb;
}


bool statusbar_set_systray(WStatusBar *sb, int sp)
{
    bool set=sb->systray_enabled;
    bool nset=libtu_do_setparam(sp, set);
    
    sb->systray_enabled=nset;
    
    return nset;
}


/*EXTL_DOC
 * Enable or disable use of \var{sb} as systray.
 * The parameter \var{how} can be one of 
 * \codestr{set}, \codestr{unset}, or \codestr{toggle}.
 * Resulting state is returned.
 */
EXTL_EXPORT_AS(WStatusBar, set_systray)
bool statusbar_set_systray_extl(WStatusBar *sb, const char *how)
{
    return statusbar_set_systray(sb, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{sb} used as a systray?
 */
EXTL_EXPORT_AS(WStatusBar, is_systray)
bool statusbar_is_systray_extl(WStatusBar *sb)
{
    return sb->systray_enabled;
}


/*}}}*/


/*{{{ Load */


WRegion *statusbar_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WStatusBar *sb=create_statusbar(par, fp);

    if(sb!=NULL){
        char *tmpl=NULL;
        ExtlTab t=extl_table_none();
        if(extl_table_gets_s(tab, "template", &tmpl)){
            statusbar_set_template(sb, tmpl);
            free(tmpl);
        }else if(extl_table_gets_t(tab, "template_table", &t)){
            statusbar_set_template_table(sb, t);
            extl_unref_table(t);
        }else{
            const char *tmpl=TR("[ %date || load: %load ] %filler%systray");
            statusbar_set_template(sb, tmpl);
        }
        
        extl_table_gets_b(tab, "systray", &sb->systray_enabled);
    }
    
    return (WRegion*)sb;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab statusbar_dynfuntab[]={
    {window_draw, statusbar_draw},
    {region_updategr, statusbar_updategr},
    {region_size_hints, statusbar_size_hints},
    {(DynFun*)region_orientation, (DynFun*)statusbar_orientation},

    {region_managed_rqgeom, statusbar_managed_rqgeom},
    {(DynFun*)region_prepare_manage, (DynFun*)statusbar_prepare_manage},
    {region_managed_remove, statusbar_managed_remove},
    
    {(DynFun*)region_prepare_manage_transient, 
     (DynFun*)statusbar_prepare_manage_transient},
    
    {region_map, statusbar_map},
    {region_unmap, statusbar_unmap},
    
    {(DynFun*)region_fitrep,
     (DynFun*)statusbar_fitrep},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WStatusBar, WWindow, statusbar_deinit, statusbar_dynfuntab);

    
/*}}}*/

