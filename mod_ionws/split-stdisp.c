/*
 * ion/mod_ionws/split-stdisp.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/resize.h>
#include "split.h"
#include "split-stdisp.h"


/*{{{ Helper routines */


#define STDISP_IS_HORIZONTAL(STDISP) \
        ((STDISP)->orientation==REGION_ORIENTATION_HORIZONTAL)

#define STDISP_IS_VERTICAL(STDISP) \
        ((STDISP)->orientation==REGION_ORIENTATION_VERTICAL)

#define STDISP_GROWS_L_TO_R(STDISP) (STDISP_IS_HORIZONTAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_TL ||                            \
     (STDISP)->corner==MPLEX_STDISP_BL))

#define STDISP_GROWS_R_TO_L(STDISP) (STDISP_IS_HORIZONTAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_TR ||                            \
     (STDISP)->corner==MPLEX_STDISP_BR))

#define STDISP_GROWS_T_TO_B(STDISP) (STDISP_IS_VERTICAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_TL ||                          \
     (STDISP)->corner==MPLEX_STDISP_TR))

#define STDISP_GROWS_B_TO_T(STDISP) (STDISP_IS_VERTICAL(STDISP) && \
    ((STDISP)->corner==MPLEX_STDISP_BL ||                          \
     (STDISP)->corner==MPLEX_STDISP_BR))

#define GEOM(S) (((WSplit*)S)->geom)

#define IMPLIES(X, Y) (!(X) || (Y))


static int other_dir(int dir)
{
    return (dir==SPLIT_VERTICAL ? SPLIT_HORIZONTAL : SPLIT_VERTICAL);
}


static void swap(int *x, int *y)
{
    int z=*x;
    *x=*y;
    *y=z;
}


static void swapptr(WSplit **x, WSplit **y)
{
    void *z=*x;
    *x=*y;
    *y=z;
}


static int recommended_w(WSplitST *stdisp)
{
    if(stdisp->regnode.reg==NULL)
        return CF_STDISP_MIN_SZ;
    
    return maxof(CF_STDISP_MIN_SZ, region_min_w(stdisp->regnode.reg));
}


static int recommended_h(WSplitST *stdisp)
{
    if(stdisp->regnode.reg==NULL)
        return CF_STDISP_MIN_SZ;
    
    return maxof(CF_STDISP_MIN_SZ, region_min_h(stdisp->regnode.reg));
}


/*}}}*/


/*{{{ Rotation and flipping primitives */


/* Yes, it is overparametrised */
static void rotate_right(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    assert(a->tl==(WSplit*)p && p->tl==y);
    
    /* Right rotation:
     * 
     *        a             a
     *      /  \          /   \
     *     p    x   =>   y     p
     *   /   \               /   \
     *  y     ?             ?     x
     */
    a->tl=y;
    y->parent=(WSplitInner*)a;
    p->tl=p->br;
    p->br=a->br;
    p->br->parent=(WSplitInner*)p;
    a->br=(WSplit*)p;
    
    if(a->current==SPLIT_CURRENT_BR){
        p->current=SPLIT_CURRENT_BR;
    }else if(p->current==SPLIT_CURRENT_BR){
        a->current=SPLIT_CURRENT_BR;
        p->current=SPLIT_CURRENT_TL;
    }
}


static void rot_rs_rotate_right(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    WRectangle xg, yg, pg;
    
    assert(a->dir==other_dir(p->dir));
    
    xg=GEOM(a->br);
    yg=GEOM(p->tl);
    pg=GEOM(p);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* yyxx    yyyy
         * ??xx => ??xx
         * ??xx    ??xx
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=GEOM(a).w;
        pg.y+=yg.h;
        xg.y+=yg.h;
    }else{
        /* y??    y??
         * y??    y??
         * xxx => yxx
         * xxx    yxx
         */
        yg.h+=xg.h;
        xg.w-=yg.w;
        pg.w-=yg.w;
        pg.h=GEOM(a).h;
        pg.x+=yg.w;
        xg.x+=yg.w;
    }
    
    swap(&(a->dir), &(p->dir));
    
    GEOM(p)=pg;
    split_do_resize(a->br, &xg, PRIMN_ANY, PRIMN_ANY, FALSE);
    split_do_resize(p->tl, &yg, PRIMN_ANY, PRIMN_ANY, FALSE);
    
    rotate_right(a, p, y);
}


static void rotate_left(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    assert(a->br==(WSplit*)p && p->br==y);
    
    /* Left rotation:
     * 
     *     a                  a
     *   /   \              /   \
     *  x     p     =>     p     y
     *      /   \        /  \
     *     ?     y      x    ?
     */
    a->br=y;
    y->parent=(WSplitInner*)a;
    p->br=p->tl;
    p->tl=a->tl;
    p->tl->parent=(WSplitInner*)p;
    a->tl=(WSplit*)p;
    
    if(a->current==SPLIT_CURRENT_TL){
        p->current=SPLIT_CURRENT_TL;
    }else if(p->current==SPLIT_CURRENT_TL){
        a->current=SPLIT_CURRENT_TL;
        p->current=SPLIT_CURRENT_BR;
    }
}


static void rot_rs_rotate_left(WSplitSplit *a, WSplitSplit *p, WSplit *y)
{
    WRectangle xg, yg, pg;
    
    assert(a->dir==other_dir(p->dir));
    
    xg=GEOM(a->tl);
    yg=GEOM(p->br);
    pg=GEOM(p);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* xx??    xx??
         * xx?? => xx??
         * xxyy    yyyy
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=GEOM(a).w;
        pg.x=GEOM(a).x;
        yg.x=GEOM(a).x;
    }else{
        /* xxx    xxy
         * xxx    xxy
         * ??y => ??y
         * ??y    ??y
         */
        yg.h+=xg.h;
        xg.w-=yg.w;
        pg.w-=yg.w;
        pg.h=GEOM(a).h;
        pg.y=GEOM(a).y;
        yg.y=GEOM(a).y;
    }
    
    swap(&(a->dir), &(p->dir));
    
    GEOM(p)=pg;
    split_do_resize(a->tl, &xg, PRIMN_ANY, PRIMN_ANY, FALSE);
    split_do_resize(p->br, &yg, PRIMN_ANY, PRIMN_ANY, FALSE);
    
    rotate_left(a, p, y);
}



static void flip_right(WSplitSplit *a, WSplitSplit *p)
{
    WSplit *tmp;
    
    assert(a->tl==(WSplit*)p);
    
    /* Right flip:
     *        a               a 
     *      /   \           /   \
     *     p     x   =>    p     y
     *   /  \            /  \
     *  ?    y          ?    x
     */
    swapptr(&(p->br), &(a->br));
    a->br->parent=(WSplitInner*)a;
    p->br->parent=(WSplitInner*)p;
    
    if(a->current==SPLIT_CURRENT_BR){
        a->current=SPLIT_CURRENT_TL;
        p->current=SPLIT_CURRENT_BR;
    }else if(p->current==SPLIT_CURRENT_BR){
        a->current=SPLIT_CURRENT_BR;
    }
}


static void rot_rs_flip_right(WSplitSplit *a, WSplitSplit *p)
{
    WRectangle xg, yg, pg;
    
    assert(a->dir==other_dir(p->dir));
    
    xg=GEOM(a->br);
    yg=GEOM(p->br);
    pg=GEOM(p);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* ??xx    ??xx
         * ??xx => ??xx
         * yyxx    yyyy
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=GEOM(a).w;
    }else{
        /* ??y    ??y
         * ??y    ??y
         * xxx => xxy
         * xxx    xxy
         */
        yg.h+=xg.h;
        xg.w-=yg.w;
        pg.w-=yg.w;
        pg.h=GEOM(a).h;
    }
    
    swap(&(a->dir), &(p->dir));
    
    GEOM(p)=pg;
    split_do_resize(a->br, &xg, PRIMN_ANY, PRIMN_ANY, FALSE);
    split_do_resize(p->br, &yg, PRIMN_ANY, PRIMN_ANY, FALSE);
    
    flip_right(a, p);
}


static void flip_left(WSplitSplit *a, WSplitSplit *p)
{
    WSplit *tmp;
    
    assert(a->br==(WSplit*)p);
    
    /* Left flip:
     *     a               a 
     *   /   \           /   \
     *  x     p    =>   y     p
     *      /  \            /  \
     *     y    ?          x    ?
     */
    swapptr(&(p->tl), &(a->tl));
    a->tl->parent=(WSplitInner*)a;
    p->tl->parent=(WSplitInner*)p;
    
    if(a->current==SPLIT_CURRENT_TL){
        a->current=SPLIT_CURRENT_BR;
        p->current=SPLIT_CURRENT_TL;
    }else if(p->current==SPLIT_CURRENT_TL){
        a->current=SPLIT_CURRENT_TL;
    }
}


static void rot_rs_flip_left(WSplitSplit *a, WSplitSplit *p)
{
    WRectangle xg, yg, pg;
    
    assert(a->dir==other_dir(p->dir));
    
    xg=GEOM(a->tl);
    yg=GEOM(p->tl);
    pg=GEOM(p);
    
    if(a->dir==SPLIT_HORIZONTAL){
        /* xxyy    yyyy
         * xx?? => xx??
         * xx??    xx??
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=GEOM(a).w;
        yg.x=GEOM(a).x;
        pg.x=GEOM(a).x;
        pg.y+=yg.h;
        xg.y+=yg.h;
    }else{
        /* xxx    yxx
         * xxx    yxx
         * y?? => y??
         * y??    y??
         */
        yg.h+=xg.h;
        xg.w-=yg.w;
        pg.w-=yg.w;
        pg.h=GEOM(a).h;
        yg.y=GEOM(a).y;
        pg.y=GEOM(a).y;
        pg.x+=yg.w;
        xg.x+=yg.w;
    }
    
    swap(&(a->dir), &(p->dir));
    
    GEOM(p)=pg;
    split_do_resize(a->tl, &xg, PRIMN_ANY, PRIMN_ANY, FALSE);
    split_do_resize(p->tl, &yg, PRIMN_ANY, PRIMN_ANY, FALSE);
    
    flip_left(a, p);
}


static bool stdisp_dir_ok(WSplitSplit *p, WSplitST *stdisp)
{
    assert(p->tl==(WSplit*)stdisp || p->br==(WSplit*)stdisp);
    
    return (IMPLIES(STDISP_IS_HORIZONTAL(stdisp), p->dir==SPLIT_VERTICAL) &&
            IMPLIES(STDISP_IS_VERTICAL(stdisp), p->dir==SPLIT_HORIZONTAL));
}


/*}}}*/


/*{{{ Sink */


static bool do_try_sink_stdisp_orth(WSplitSplit *p, WSplitST *stdisp, 
                                    WSplitSplit *other, bool force)
{
    bool doit=force;
    
    assert(p->dir==other_dir(other->dir));
    assert(stdisp_dir_ok(p, stdisp));
    
    if(STDISP_GROWS_T_TO_B(stdisp) || STDISP_GROWS_L_TO_R(stdisp)){
        if(STDISP_GROWS_L_TO_R(stdisp)){
            assert(other->dir==SPLIT_HORIZONTAL);
            if(other->tl->geom.w>=recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_T_TO_B */
            assert(other->dir==SPLIT_VERTICAL);
            if(other->tl->geom.h>=recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p->br==(WSplit*)stdisp){
                /*
                 *      p               p 
                 *    /   \           /   \   (direction of other and p
                 *  other stdisp  =>  other br   changed)
                 *  /  \            /  \
                 * tl  br          tl  stdisp
                 */
                rot_rs_flip_right(p, other);
            }else{ /* p->tl==stdisp */
                /*
                 *     p                p
                 *   /   \            /  \      (direction of other and p
                 * stdisp other  =>  other br      changed)
                 *      /  \       /  \   
                 *     tl  br    stdisp tl
                 */
                rot_rs_rotate_left(p, other, other->br);
            }
        }
    }else{ /* STDISP_GROWS_B_TO_T or STDISP_GROW_R_TO_L */
        if(STDISP_GROWS_R_TO_L(stdisp)){
            assert(other->dir==SPLIT_HORIZONTAL);
            if(other->br->geom.w>=recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_B_TO_T */
            assert(other->dir==SPLIT_VERTICAL);
            if(other->br->geom.h>=recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p->tl==(WSplit*)stdisp){
                /*
                 *     p              p
                 *   /   \          /   \      (direction of other and p
                 * stdisp other  =>  tl  other    changed)
                 *      /  \           /  \
                 *     tl  br        stdisp  br
                 */
                rot_rs_flip_left(p, other);
            }else{ /* p->br==stdisp */
                /*
                 *       p             p
                 *     /   \         /   \      (direction of other and p
                 *  other stdisp  =>  tl   other   changed)
                 *  /  \                 /  \
                 * tl  br               br  stdisp
                 */
                rot_rs_rotate_right(p, other, other->tl);
            }
        }
    }
    
    return doit;
}


static bool do_try_sink_stdisp_para(WSplitSplit *p, WSplitST *stdisp, 
                                    WSplitSplit *other, bool force)
{
    if(!force){
        if(STDISP_IS_HORIZONTAL(stdisp)){
            if(recommended_w(stdisp)>=GEOM(p).w)
                return FALSE;
        }else{
            if(recommended_h(stdisp)>=GEOM(p).h)
                return FALSE;
        }
    }
    
    if(p->tl==(WSplit*)stdisp)
        rotate_left(p, other, other->br);
    else
        rotate_right(p, other, other->tl);
    
    
    splitsplit_update_geom_from_children(other);
    
    return TRUE;
}


bool split_try_sink_stdisp(WSplitSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;
    
    while(more){
        WSplit *tl=node->tl;
        WSplit *br=node->br;
        WSplitSplit *other=NULL;
        WSplitST *st;
        
        if(OBJ_IS(tl, WSplitST)){
            st=(WSplitST*)tl;
            other=OBJ_CAST(br, WSplitSplit);
        }else if(OBJ_IS(br, WSplitST)){
            st=(WSplitST*)br;
            other=OBJ_CAST(tl, WSplitSplit);
        }
        
        if(other==NULL)
            break;
        
        if(!stdisp_dir_ok(node, st))
            break;
        
        if(other->dir==other_dir(node->dir)){
            if(!do_try_sink_stdisp_orth(node, st, other, force))
                break;
        }else /*if(br->dir==node->dir)*/{
            if(!do_try_sink_stdisp_para(node, st, other, force))
                break;
        }
        node=other;
        didsomething=TRUE;
        more=iterate;
    }
    
    return didsomething;
}


/*}}}*/


/*{{{ Unsink */


static bool do_try_unsink_stdisp_orth(WSplitSplit *a, WSplitSplit *p,
                                      WSplitST *stdisp, bool force)
{
    bool doit=force;
    
#warning "Add check that we can shrink or just let stdisp take precedence?"
    
    assert(p->dir=other_dir(a->dir));
    assert(stdisp_dir_ok(p, stdisp));
    
    if(STDISP_GROWS_T_TO_B(stdisp) || STDISP_GROWS_L_TO_R(stdisp)){
        if(STDISP_GROWS_L_TO_R(stdisp)){
            assert(a->dir==SPLIT_HORIZONTAL);
            if(GEOM(stdisp).w<recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_T_TO_B */
            assert(a->dir==SPLIT_VERTICAL);
            if(GEOM(stdisp).h<recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if((WSplit*)p==a->tl){
                if((WSplit*)stdisp==p->br)
                    rot_rs_flip_right(a, p);
                else /*stdisp==p->tl*/
                    rot_rs_rotate_right(a, p, (WSplit*)stdisp);
            }else{ /*p==a->br*/
#if 1
                /* abnormal cases. */
                WARN_FUNC(TR("Status display in bad split configuration."));
                return FALSE;
#else                
                if((WSplit*)stdisp==p->br)
                    rot_rs_rotate_left(a, p, (WSplit*)stdisp);
                else /*stdisp==p->tl*/
                    rot_rs_flip_left(a, p);
#endif
            }
        }
    }else{ /*STDISP_GROWS_B_TO_T || STDISP_GROWS_R_TO_L*/
        if(STDISP_GROWS_R_TO_L(stdisp)){
            assert(a->dir==SPLIT_HORIZONTAL);
            if(GEOM(stdisp).w<recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_B_TO_T */
            assert(a->dir==SPLIT_VERTICAL);
            if(GEOM(stdisp).h<recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if((WSplit*)p==a->tl){
#if 1
                /* abnormal cases. */
                WARN_FUNC(TR("Status display in bad split configuration."));
                return FALSE;
#else                
                if((WSplit*)stdisp==p->br)
                    rot_rs_flip_right(a, p);
                else /*stdisp==p->tl*/
                    rot_rs_rotate_right(a, p, (WSplit*)stdisp);
#endif                
            }else{ /*p==a->br*/
                if((WSplit*)stdisp==p->br)
                    rot_rs_rotate_left(a, p, (WSplit*)stdisp);
                else /*stdisp==p->tl*/
                    rot_rs_flip_left(a, p);
            }
        }
    }
    
    return doit;
}


static bool do_try_unsink_stdisp_para(WSplitSplit *a, WSplitSplit *p,
                                      WSplitST *stdisp,  bool force)
{
    
    if(!force){
        if(STDISP_IS_HORIZONTAL(stdisp)){
            if(recommended_w(stdisp)<=GEOM(p).w)
                return FALSE;
        }else{
            if(recommended_h(stdisp)<=GEOM(p).h)
                return FALSE;
        }
    }
    
    if(a->tl==(WSplit*)p && p->tl==(WSplit*)stdisp){
        rotate_right(a, p, (WSplit*)stdisp);
    }else if(a->br==(WSplit*)p && p->br==(WSplit*)stdisp){
        rotate_left(a, p, (WSplit*)stdisp);
    }else{
        warn(TR("Status display badly located in split tree."));
        return FALSE;
    }
    
    splitsplit_update_geom_from_children(p);
    
    return TRUE;
}


bool split_try_unsink_stdisp(WSplitSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;
    
    while(more){
        WSplitSplit *p=OBJ_CAST(((WSplit*)node)->parent, WSplitSplit);
        WSplit *tl=node->tl;
        WSplit *br=node->br;
        WSplitST *st;

        if(p==NULL)
            break;
        
        if(OBJ_IS(tl, WSplitST))
            st=(WSplitST*)tl;
        else if(OBJ_IS(br, WSplitST))
            st=(WSplitST*)br;
        else
            break;
        
        if(!stdisp_dir_ok(node, st))
            break;
        
        if(p->dir==other_dir(node->dir)){
            if(!do_try_unsink_stdisp_orth(p, node, st, force))
                break;
        }else /*if(p->dir==node->dir)*/{
            if(!do_try_unsink_stdisp_para(p, node, st, force))
                break;
        }
        
        node=p;
        didsomething=TRUE;
        more=iterate;
    }
    
    return didsomething;
}


/*}}}*/


/*{{{ Sink or unsink */


bool split_regularise_stdisp(WSplitST *stdisp)
{
    WSplitSplit *node=OBJ_CAST(((WSplit*)stdisp)->parent, WSplitSplit);
    
    if(node==NULL)
        return FALSE;
    
    if(STDISP_IS_HORIZONTAL(stdisp)){
        if(GEOM(stdisp).w<recommended_w(stdisp))
            return split_try_unsink_stdisp(node, TRUE, FALSE);
        else if(GEOM(stdisp).w>recommended_w(stdisp))
            return split_try_sink_stdisp(node, TRUE, FALSE);
    }else{
        if(GEOM(stdisp).h<recommended_h(stdisp))
            return split_try_unsink_stdisp(node, TRUE, FALSE);
        else if(GEOM(stdisp).h>recommended_h(stdisp))
            return split_try_sink_stdisp(node, TRUE, FALSE);
    }
    
    return FALSE;
}


/*}}}*/

