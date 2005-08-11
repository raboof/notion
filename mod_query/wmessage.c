/*
 * ion/mod_query/wmessage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <libtu/objp.h>
#include <ioncore/strings.h>
#include <ioncore/global.h>
#include <ioncore/event.h>
#include "wmessage.h"
#include "inputp.h"


#define WMSG_BRUSH(WMSG) ((WMSG)->input.brush)
#define WMSG_WIN(WMSG) ((WMSG)->input.win.win)


/*{{{ Sizecalc */


static void get_geom(WMessage *wmsg, bool max, WRectangle *geom)
{
    if(max){
        geom->w=wmsg->input.last_fp.g.w;
        geom->h=wmsg->input.last_fp.g.h;
    }else{
        geom->w=REGION_GEOM(wmsg).w;
        geom->h=REGION_GEOM(wmsg).h;
    }
    geom->x=0;
    geom->y=0;
}


static void wmsg_calc_size(WMessage *wmsg, WRectangle *geom)
{
    WRectangle max_geom=*geom;
    GrBorderWidths bdw;
    int h=16;

    if(WMSG_BRUSH(wmsg)!=NULL){
        WRectangle g;
        g.w=max_geom.w;
        g.h=max_geom.h;
        g.x=0;
        g.y=0;
        
        fit_listing(WMSG_BRUSH(wmsg), &g, &(wmsg->listing));

        grbrush_get_border_widths(WMSG_BRUSH(wmsg), &bdw);
        
        h=bdw.top+bdw.bottom+wmsg->listing.toth;
    }
    
    if(h>max_geom.h || !(wmsg->input.last_fp.mode&REGION_FIT_BOUNDS))
        h=max_geom.h;
    
    geom->h=h;
    geom->w=max_geom.w;
    geom->y=max_geom.y+max_geom.h-geom->h;
    geom->x=max_geom.x;
}


/*}}}*/


/*{{{ Draw */


static void wmsg_draw(WMessage *wmsg, bool complete)
{
    const char *style=(REGION_IS_ACTIVE(wmsg) ? "active" : "inactive");
    WRectangle geom;
    
    if(WMSG_BRUSH(wmsg)==NULL)
        return;
    
    get_geom(wmsg, FALSE, &geom);
    
    grbrush_begin(WMSG_BRUSH(wmsg), &geom, 
                  (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    draw_listing(WMSG_BRUSH(wmsg), &geom, &(wmsg->listing), 
                 FALSE, style, style);
    
    grbrush_end(WMSG_BRUSH(wmsg));
}


/*}}}*/


/*{{{ Scroll */


static void wmsg_scrollup(WMessage *wmsg)
{
    if(scrollup_listing(&(wmsg->listing)))
        wmsg_draw(wmsg, TRUE);
}


static void wmsg_scrolldown(WMessage *wmsg)
{
    if(scrolldown_listing(&(wmsg->listing)))
        wmsg_draw(wmsg, TRUE);
}


/*}}}*/


/*{{{ Init, deinit draw config update */


static bool wmsg_init(WMessage *wmsg, WWindow *par, const WFitParams *fp,
                      const char *msg)
{
    char **ptr;
    int k, n=0;
    char *cmsg;
    const char *p;
    size_t l;
    
    p=msg;
    while(1){
        n=n+1;
        p=strchr(p, '\n');
        if(p==NULL || *(p+1)=='\0')
            break;
        p=p+1;
    }
    
    if(n==0)
        return FALSE;
        
    ptr=ALLOC_N(char*, n);
    
    if(ptr==NULL)
        return FALSE;
    
    for(k=0; k<n; k++)
        ptr[k]=NULL;
    
    p=msg;
    k=0;
    while(k<n){
        l=strcspn(p, "\n");
        cmsg=ALLOC_N(char, l+1);
        if(cmsg==NULL){
            while(k>0){
                k--;
                free(ptr[k]);
            }
            free(ptr);
            return FALSE;
        }
        strncpy(cmsg, p, l);
        cmsg[l]='\0';
        ptr[k]=cmsg;
        k++;
        if(p[l]=='\0')
            break;
        p=p+l+1;
    }
    
    init_listing(&(wmsg->listing));
    setup_listing(&(wmsg->listing), ptr, k, TRUE);
    
    if(!input_init((WInput*)wmsg, par, fp)){
        deinit_listing(&(wmsg->listing));
        return FALSE;
    }

    return TRUE;
}


WMessage *create_wmsg(WWindow *par, const WFitParams *fp, const char *msg)
{
    CREATEOBJ_IMPL(WMessage, wmsg, (p, par, fp, msg));
}


static void wmsg_deinit(WMessage *wmsg)
{
    if(wmsg->listing.strs!=NULL)
        deinit_listing(&(wmsg->listing));

    input_deinit((WInput*)wmsg);
}


static const char *wmsg_style(WMessage *wmsg)
{
    return "input-message";
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab wmsg_dynfuntab[]={
    {window_draw,        wmsg_draw},
    {input_calc_size,     wmsg_calc_size},
    {input_scrollup,     wmsg_scrollup},
    {input_scrolldown,    wmsg_scrolldown},
    {(DynFun*)input_style,
     (DynFun*)wmsg_style},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WMessage, WInput, wmsg_deinit, wmsg_dynfuntab);

    
/*}}}*/
