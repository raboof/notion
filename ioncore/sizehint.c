/*
 * ion/ioncore/sizehint.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <libtu/minmax.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "resize.h"
#include "sizehint.h"
#include "region-iter.h"


/*{{{ xsizehints_correct */


static void do_correct_aspect(int max_w, int max_h, int ax, int ay,
                              int *wret, int *hret)
{
    int w=*wret, h=*hret;

    if(ax>ay){
        h=(w*ay)/ax;
        if(max_h>0 && h>max_h){
            h=max_h;
            w=(h*ax)/ay;
        }
    }else{
        w=(h*ax)/ay;
        if(max_w>0 && w>max_w){
            w=max_w;
            h=(w*ay)/ax;
        }
    }
    
    *wret=w;
    *hret=h;
}


static void correct_aspect(int max_w, int max_h, const XSizeHints *hints,
                           int *wret, int *hret)
{
    if(!(hints->flags&PAspect))
        return;
    
    if(*wret*hints->max_aspect.y>*hret*hints->max_aspect.x){
        do_correct_aspect(max_w, max_h,
                          hints->min_aspect.x, hints->min_aspect.y,
                          wret, hret);
    }

    if(*wret*hints->min_aspect.y<*hret*hints->min_aspect.x){
        do_correct_aspect(max_w, max_h,
                          hints->max_aspect.x, hints->max_aspect.y,
                          wret, hret);
    }
}


void xsizehints_correct(const XSizeHints *hints, int *wp, int *hp, bool min)
{
    int w=*wp;
    int h=*hp;
    int bs=0;
    
    if(min){
        if(w<hints->min_width)
            w=hints->min_width;
        if(h<hints->min_height)
            h=hints->min_height;
        
        correct_aspect(w, h, hints, &w, &h);
    }else{
        if(w>=hints->min_width && h>=hints->min_height)
            correct_aspect(w, h, hints, &w, &h);
    }
    
    if(hints->flags&PMaxSize){
        if(w>hints->max_width)
            w=hints->max_width;
        if(h>hints->max_height)
            h=hints->max_height;
    }

    if(hints->flags&PResizeInc){
        /* base size should be set to 0 if none given by user program */
        bs=(hints->flags&PBaseSize ? hints->base_width : 0);
        if(w>bs)
            w=((w-bs)/hints->width_inc)*hints->width_inc+bs;
        bs=(hints->flags&PBaseSize ? hints->base_height : 0);
        if(h>bs)
            h=((h-bs)/hints->height_inc)*hints->height_inc+bs;
    }
    
    *wp=w;
    *hp=h;
}


/*}}}*/


/*{{{ xwindow_get_sizehints */


void xsizehints_sanity_adjust(XSizeHints *hints)
{
    if(!(hints->flags&PMinSize)){
        if(hints->flags&PBaseSize){
            hints->min_width=hints->base_width;
            hints->min_height=hints->base_height;
        }else{
            hints->min_width=0;
            hints->min_height=0;
        }
    }

    if(hints->min_width<0)
        hints->min_width=0;
    if(hints->min_height<0)
        hints->min_height=0;

    if(!(hints->flags&PBaseSize) || hints->base_width<0)
        hints->base_width=hints->min_width;
    if(!(hints->flags&PBaseSize) || hints->base_height<0)
        hints->base_height=hints->min_height;

    
    if(hints->flags&PMaxSize){
        if(hints->max_width<hints->min_width)
            hints->max_width=hints->min_width;
        if(hints->max_height<hints->min_height)
            hints->max_height=hints->min_height;
    }
    
    hints->flags|=(PBaseSize|PMinSize);

    if(hints->flags&PResizeInc){
        if(hints->width_inc<=0 || hints->height_inc<=0){
            warn("Invalid client-supplied width/height increment");
            hints->flags&=~PResizeInc;
        }
    }
    
    if(hints->flags&PAspect){
        if(hints->min_aspect.x<=0 || hints->min_aspect.y<=0 ||
           hints->min_aspect.x<=0 || hints->min_aspect.y<=0){
            warn("Invalid client-supplied aspect-ratio");
            hints->flags&=~PAspect;
        }
    }
    
    if(!(hints->flags&PWinGravity))
        hints->win_gravity=ForgetGravity;
}


/*}}}*/


/*{{{ xsizehints_adjust_for */


void xsizehints_adjust_for(XSizeHints *hints, WRegion *list)
{
    WRegion *reg;
    uint wdummy, hdummy;
    XSizeHints tmp_hints;
    
    FOR_ALL_MANAGED_ON_LIST(list, reg){
        region_resize_hints(reg, &tmp_hints, &wdummy, &hdummy);
        
        if(tmp_hints.flags&PMinSize){
            if(!(hints->flags&PMinSize)){
                hints->flags|=PMinSize;
                hints->min_width=tmp_hints.min_width;
                hints->min_height=tmp_hints.min_height;
            }else{
                hints->min_width=maxof(hints->min_width,
                                       tmp_hints.min_width);
                hints->min_height=maxof(hints->min_height,
                                        tmp_hints.min_height);
            }
        }
        
        if(tmp_hints.flags&PMaxSize && hints->flags&PMaxSize){
            hints->max_width=maxof(hints->max_width,
                                   tmp_hints.max_width);
            hints->max_height=maxof(hints->max_height,
                                    tmp_hints.max_height);
        }else{
            hints->flags&=~PMaxSize;
        }
    }
}


/*}}}*/


/*{{{ account_gravity */


int xgravity_deltax(int gravity, int left, int right)
{
    int woff=left+right;

    if(gravity==StaticGravity || gravity==ForgetGravity){
        return -left;
    }else if(gravity==NorthWestGravity || gravity==WestGravity ||
             gravity==SouthWestGravity){
        /* */
    }else if(gravity==NorthEastGravity || gravity==EastGravity ||
             gravity==SouthEastGravity){
        /* geom->x=geom->w+geom->x-(geom->w+woff) */
        return -woff;
    }else if(gravity==CenterGravity || gravity==NorthGravity ||
             gravity==SouthGravity){
        /* geom->x=geom->x+geom->w/2-(geom->w+woff)/2 */
        return -woff/2;
    }
    return 0;
}


int xgravity_deltay(int gravity, int top, int bottom)
{
    int hoff=top+bottom;
    
    if(gravity==StaticGravity || gravity==ForgetGravity){
        return -top;
    }else if(gravity==NorthWestGravity || gravity==NorthGravity ||
             gravity==NorthEastGravity){
        /* */
    }else if(gravity==SouthWestGravity || gravity==SouthGravity ||
             gravity==SouthEastGravity){
        /* geom->y=geom->y+geom->h-(geom->h+hoff) */
        return -hoff;
    }else if(gravity==CenterGravity || gravity==WestGravity ||
             gravity==EastGravity){
        /* geom->y=geom->y+geom->h/2-(geom->h+hoff)/2 */
        return -hoff/2;
    }
    return 0;
}


/*}}}*/
