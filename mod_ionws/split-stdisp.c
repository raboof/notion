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

#define STDISP_IS_HORIZONTAL(STDISP) \
        ((STDISP)->u.d.orientation==REGION_ORIENTATION_HORIZONTAL)

#define STDISP_IS_VERTICAL(STDISP) \
        ((STDISP)->u.d.orientation==REGION_ORIENTATION_VERTICAL)

#define STDISP_GROWS_L_TO_R(STDISP) (STDISP_IS_HORIZONTAL(STDISP) && \
    ((STDISP)->u.d.corner==MPLEX_STDISP_TL ||                        \
     (STDISP)->u.d.corner==MPLEX_STDISP_BL))

#define STDISP_GROWS_R_TO_L(STDISP) (STDISP_IS_HORIZONTAL(STDISP) && \
    ((STDISP)->u.d.corner==MPLEX_STDISP_TR ||                        \
     (STDISP)->u.d.corner==MPLEX_STDISP_BR))

#define STDISP_GROWS_T_TO_B(STDISP) (STDISP_IS_VERTICAL(STDISP) && \
    ((STDISP)->u.d.corner==MPLEX_STDISP_TL ||                      \
     (STDISP)->u.d.corner==MPLEX_STDISP_TR))

#define STDISP_GROWS_B_TO_T(STDISP) (STDISP_IS_VERTICAL(STDISP) && \
    ((STDISP)->u.d.corner==MPLEX_STDISP_BL ||                      \
     (STDISP)->u.d.corner==MPLEX_STDISP_BR))


/*{{{ Helper routines */


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


static void swapstr(char **x, char **y)
{
    char *z=*x;
    *x=*y;
    *y=z;
}


static int recommended_w(WSplit *stdisp)
{
    if(stdisp->u.reg==NULL)
        return CF_STDISP_MIN_SZ;
    
    return maxof(CF_STDISP_MIN_SZ, region_min_w(stdisp->u.reg));
}


static int recommended_h(WSplit *stdisp)
{
    if(stdisp->u.reg==NULL)
        return CF_STDISP_MIN_SZ;
    
    return maxof(CF_STDISP_MIN_SZ, region_min_h(stdisp->u.reg));
}


/*}}}*/


/*{{{ Rotation and flipping primitives */


/* Yes, it is overparametrised */
static void rotate_right(WSplit *a, WSplit *p, WSplit *y)
{
    assert(a->u.s.tl==p && p->u.s.tl==y);
    
    /* Right rotation:
     * 
     *        a             a
     *      /  \          /   \
     *     p    x   =>   y     p
     *   /   \               /   \
     *  y     ?             ?     x
     */
    a->u.s.tl=y;
    y->parent=a;
    p->u.s.tl=p->u.s.br;
    p->u.s.br=a->u.s.br;
    p->u.s.br->parent=p;
    a->u.s.br=p;
    
    if(a->u.s.current==SPLIT_CURRENT_BR){
        p->u.s.current=SPLIT_CURRENT_BR;
    }else if(p->u.s.current==SPLIT_CURRENT_BR){
        a->u.s.current=SPLIT_CURRENT_BR;
        p->u.s.current=SPLIT_CURRENT_TL;
    }
    
    /* Marker for a parent i of children j, k will this way stay in
     * the first common parent of j and k (these nodes included).
     */
    swapstr(&(a->marker), &(p->marker));
}


static void rot_rs_rotate_right(WSplit *a, WSplit *p, WSplit *y)
{
    WRectangle xg, yg, pg;
    
    assert(a->type==other_dir(p->type));
    
    xg=a->u.s.br->geom;
    yg=p->u.s.tl->geom;
    pg=p->geom;
    
    if(a->type==SPLIT_HORIZONTAL){
        /* yyxx    yyyy
         * ??xx => ??xx
         * ??xx    ??xx
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=a->geom.w;
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
        pg.h=a->geom.h;
        pg.x+=yg.w;
        xg.x+=yg.w;
    }
    
    swap(&(a->type), &(p->type));
    
    p->geom=pg;
    split_do_resize(a->u.s.br, &xg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    split_do_resize(p->u.s.tl, &yg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    
    rotate_right(a, p, y);
}


static void rotate_left(WSplit *a, WSplit *p, WSplit *y)
{
    assert(a->u.s.br==p && p->u.s.br==y);
    
    /* Left rotation:
     * 
     *     a                  a
     *   /   \              /   \
     *  x     p     =>     p     y
     *      /   \        /  \
     *     ?     y      x    ?
     */
    a->u.s.br=y;
    y->parent=a;
    p->u.s.br=p->u.s.tl;
    p->u.s.tl=a->u.s.tl;
    p->u.s.tl->parent=p;
    a->u.s.tl=p;
    
    if(a->u.s.current==SPLIT_CURRENT_TL){
        p->u.s.current=SPLIT_CURRENT_TL;
    }else if(p->u.s.current==SPLIT_CURRENT_TL){
        a->u.s.current=SPLIT_CURRENT_TL;
        p->u.s.current=SPLIT_CURRENT_BR;
    }

    swapstr(&(a->marker), &(p->marker));
}


static void rot_rs_rotate_left(WSplit *a, WSplit *p, WSplit *y)
{
    WRectangle xg, yg, pg;
    
    assert(a->type==other_dir(p->type));
    
    xg=a->u.s.tl->geom;
    yg=p->u.s.br->geom;
    pg=p->geom;
    
    if(a->type==SPLIT_HORIZONTAL){
        /* xx??    xx??
         * xx?? => xx??
         * xxyy    yyyy
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=a->geom.w;
        pg.x=a->geom.x;
        yg.x=a->geom.x;
    }else{
        /* xxx    xxy
         * xxx    xxy
         * ??y => ??y
         * ??y    ??y
         */
        yg.h+=xg.h;
        xg.w-=yg.w;
        pg.w-=yg.w;
        pg.h=a->geom.h;
        pg.y=a->geom.y;
        yg.y=a->geom.y;
    }
    
    swap(&(a->type), &(p->type));
    
    p->geom=pg;
    split_do_resize(a->u.s.tl, &xg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    split_do_resize(p->u.s.br, &yg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    
    rotate_left(a, p, y);
}



static void flip_right(WSplit *a, WSplit *p)
{
    WSplit *tmp;
    
    assert(a->u.s.tl==p);
    
    /* Right flip:
     *        a               a 
     *      /   \           /   \
     *     p     x   =>    p     y
     *   /  \            /  \
     *  ?    y          ?    x
     */
    swapptr(&(p->u.s.br), &(a->u.s.br));
    a->u.s.br->parent=a;
    p->u.s.br->parent=p;
    
    if(a->u.s.current==SPLIT_CURRENT_BR){
        a->u.s.current=SPLIT_CURRENT_TL;
        p->u.s.current=SPLIT_CURRENT_BR;
    }else if(p->u.s.current==SPLIT_CURRENT_BR){
        a->u.s.current=SPLIT_CURRENT_BR;
    }
    
    swapstr(&(a->marker), &(p->marker));
}


static void rot_rs_flip_right(WSplit *a, WSplit *p)
{
    WRectangle xg, yg, pg;
    
    assert(a->type==other_dir(p->type));
    
    xg=a->u.s.br->geom;
    yg=p->u.s.br->geom;
    pg=p->geom;
    
    if(a->type==SPLIT_HORIZONTAL){
        /* ??xx    ??xx
         * ??xx => ??xx
         * yyxx    yyyy
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=a->geom.w;
    }else{
        /* ??y    ??y
         * ??y    ??y
         * xxx => xxy
         * xxx    xxy
         */
        yg.h+=xg.h;
        xg.w-=yg.w;
        pg.w-=yg.w;
        pg.h=a->geom.h;
    }
    
    swap(&(a->type), &(p->type));
    
    p->geom=pg;
    split_do_resize(a->u.s.br, &xg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    split_do_resize(p->u.s.br, &yg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    
    flip_right(a, p);
}


static void flip_left(WSplit *a, WSplit *p)
{
    WSplit *tmp;
    
    assert(a->u.s.br==p);
    
    /* Left flip:
     *     a               a 
     *   /   \           /   \
     *  x     p    =>   y     p
     *      /  \            /  \
     *     y    ?          x    ?
     */
    swapptr(&(p->u.s.tl), &(a->u.s.tl));
    a->u.s.tl->parent=a;
    p->u.s.tl->parent=p;
    
    if(a->u.s.current==SPLIT_CURRENT_TL){
        a->u.s.current=SPLIT_CURRENT_BR;
        p->u.s.current=SPLIT_CURRENT_TL;
    }else if(p->u.s.current==SPLIT_CURRENT_TL){
        a->u.s.current=SPLIT_CURRENT_TL;
    }
    
    swapstr(&(a->marker), &(p->marker));
}


static void rot_rs_flip_left(WSplit *a, WSplit *p)
{
    WRectangle xg, yg, pg;
    
    assert(a->type==other_dir(p->type));
    
    xg=a->u.s.tl->geom;
    yg=p->u.s.tl->geom;
    pg=p->geom;
    
    if(a->type==SPLIT_HORIZONTAL){
        /* xxyy    yyyy
         * xx?? => xx??
         * xx??    xx??
         */
        yg.w+=xg.w;
        xg.h-=yg.h;
        pg.h-=yg.h;
        pg.w=a->geom.w;
        yg.x=a->geom.x;
        pg.x=a->geom.x;
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
        pg.h=a->geom.h;
        yg.y=a->geom.y;
        pg.y=a->geom.y;
        pg.x+=yg.w;
        xg.x+=yg.w;
    }
    
    swap(&(a->type), &(p->type));
    
    p->geom=pg;
    split_do_resize(a->u.s.tl, &xg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    split_do_resize(p->u.s.tl, &yg, PRIMN_ANY, PRIMN_ANY, FALSE, NULL);
    
    flip_left(a, p);
}


static bool stdisp_dir_ok(WSplit *p, WSplit *stdisp)
{
    assert(p->u.s.tl==stdisp || p->u.s.br==stdisp);
    
    return (IMPLIES(STDISP_IS_HORIZONTAL(stdisp), p->type==SPLIT_VERTICAL) &&
            IMPLIES(STDISP_IS_VERTICAL(stdisp), p->type==SPLIT_HORIZONTAL));
}


/*}}}*/


/*{{{ Sink */


static bool do_try_sink_stdisp_orth(WSplit *p, WSplit *stdisp, WSplit *other,
                                    bool force)
{
    bool doit=force;
    
    assert(other->type==SPLIT_VERTICAL || other->type==SPLIT_HORIZONTAL);
    assert(p->type=other_dir(other->type));
    assert(stdisp->type==SPLIT_STDISPNODE);
    assert(stdisp_dir_ok(p, stdisp));
    
    if(STDISP_GROWS_T_TO_B(stdisp) || STDISP_GROWS_L_TO_R(stdisp)){
        if(STDISP_GROWS_L_TO_R(stdisp)){
            assert(other->type==SPLIT_HORIZONTAL);
            if(other->u.s.tl->geom.w>=recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_T_TO_B */
            assert(other->type==SPLIT_VERTICAL);
            if(other->u.s.tl->geom.h>=recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p->u.s.br==stdisp){
                /*
                 *      p               p 
                 *    /   \           /   \   (direction of other and p
                 *  other stdisp  =>  other br   changed)
                 *  /  \            /  \
                 * tl  br          tl  stdisp
                 */
                rot_rs_flip_right(p, other);
            }else{ /* p->u.s.tl==stdisp */
                /*
                 *     p                p
                 *   /   \            /  \      (direction of other and p
                 * stdisp other  =>  other br      changed)
                 *      /  \       /  \   
                 *     tl  br    stdisp tl
                 */
                rot_rs_rotate_left(p, other, other->u.s.br);
            }
        }
    }else{ /* STDISP_GROWS_B_TO_T or STDISP_GROW_R_TO_L */
        if(STDISP_GROWS_R_TO_L(stdisp)){
            assert(other->type==SPLIT_HORIZONTAL);
            if(other->u.s.br->geom.w>=recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_B_TO_T */
            assert(other->type==SPLIT_VERTICAL);
            if(other->u.s.br->geom.h>=recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p->u.s.tl==stdisp){
                /*
                 *     p              p
                 *   /   \          /   \      (direction of other and p
                 * stdisp other  =>  tl  other    changed)
                 *      /  \           /  \
                 *     tl  br        stdisp  br
                 */
                rot_rs_flip_left(p, other);
            }else{ /* p->u.s.br==stdisp */
                /*
                 *       p             p
                 *     /   \         /   \      (direction of other and p
                 *  other stdisp  =>  tl   other   changed)
                 *  /  \                 /  \
                 * tl  br               br  stdisp
                 */
                rot_rs_rotate_right(p, other, other->u.s.tl);
            }
        }
    }
    
    return doit;
}


static bool do_try_sink_stdisp_para(WSplit *p, WSplit *stdisp, WSplit *other,
                                    bool force)
{
    if(!force){
        if(STDISP_IS_HORIZONTAL(stdisp)){
            if(recommended_w(stdisp)>=p->geom.w)
                return FALSE;
        }else{
            if(recommended_h(stdisp)>=p->geom.h)
                return FALSE;
        }
    }
    
    if(p->u.s.tl==stdisp)
        rotate_left(p, other, other->u.s.br);
    else
        rotate_right(p, other, other->u.s.tl);
    
    
    split_update_geom_from_children(other);
    
    return TRUE;
}


bool split_try_sink_stdisp(WSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;
    
    assert(node->type==SPLIT_VERTICAL || node->type==SPLIT_HORIZONTAL);
    
    while(more){
        WSplit *tl=node->u.s.tl;
        WSplit *br=node->u.s.br;
        
        if(tl->type==SPLIT_STDISPNODE){
            if(!stdisp_dir_ok(node, tl))
                break;
            if(br->type==other_dir(node->type)){
                if(!do_try_sink_stdisp_orth(node, tl, br, force))
                    break;
            }else if(br->type==node->type){
                if(!do_try_sink_stdisp_para(node, tl, br, force))
                    break;
            }else{
                break;
            }
            node=br;
        }else if(br->type==SPLIT_STDISPNODE){
            if(!stdisp_dir_ok(node, br))
                break;
            if(tl->type==other_dir(node->type)){
                if(!do_try_sink_stdisp_orth(node, br, tl, force))
                    break;
            }else if(tl->type==node->type){
                if(!do_try_sink_stdisp_para(node, br, tl, force))
                    break;
            }else{
                break;
            }
            node=tl;
        }else{
            break;
        }
        didsomething=TRUE;
        more=iterate;
    }
    
    return didsomething;
}


/*}}}*/


/*{{{ Unsink */


static bool do_try_unsink_stdisp_orth(WSplit *a, WSplit *p, WSplit *stdisp,
                                      bool force)
{
    bool doit=force;
    
#warning "Add check that we can shrink or just let stdisp take precedence?"
    
    assert(a->type==SPLIT_VERTICAL || a->type==SPLIT_HORIZONTAL);
    assert(p->type=other_dir(a->type));
    assert(stdisp->type==SPLIT_STDISPNODE);
    assert(stdisp_dir_ok(p, stdisp));
    
    if(STDISP_GROWS_T_TO_B(stdisp) || STDISP_GROWS_L_TO_R(stdisp)){
        if(STDISP_GROWS_L_TO_R(stdisp)){
            assert(a->type==SPLIT_HORIZONTAL);
            if(stdisp->geom.w<recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_T_TO_B */
            assert(a->type==SPLIT_VERTICAL);
            if(stdisp->geom.h<recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p==a->u.s.tl){
                if(stdisp==p->u.s.br)
                    rot_rs_flip_right(a, p);
                else /*stdisp==p->u.s.tl*/
                    rot_rs_rotate_right(a, p, stdisp);
            }else{ /*p==a->u.s.br*/
#if 1
                /* abnormal cases. */
                WARN_FUNC("Status display in bad split configuration.");
                return FALSE;
#else                
                if(stdisp==p->u.s.br)
                    rotate_left(a, p, stdisp);
                else /*stdisp==p->u.s.tl*/
                    flip_left(a, p);
#endif
            }
        }
    }else{ /*STDISP_GROWS_B_TO_T || STDISP_GROWS_R_TO_L*/
        if(STDISP_GROWS_R_TO_L(stdisp)){
            assert(a->type==SPLIT_HORIZONTAL);
            if(stdisp->geom.w<recommended_w(stdisp))
                doit=TRUE;
        }else{ /* STDISP_GROWS_B_TO_T */
            assert(a->type==SPLIT_VERTICAL);
            if(stdisp->geom.h<recommended_h(stdisp))
                doit=TRUE;
        }
        
        if(doit){
            if(p==a->u.s.tl){
#if 1
                /* abnormal cases. */
                WARN_FUNC("Status display in bad split configuration.");
                return FALSE;
#else                
                if(stdisp==p->u.s.br)
                    rot_rs_flip_right(a, p);
                else /*stdisp==p->u.s.tl*/
                    rot_rs_rotate_right(a, p, stdisp);
#endif                
            }else{ /*p==a->u.s.br*/
                if(stdisp==p->u.s.br)
                    rot_rs_rotate_left(a, p, stdisp);
                else /*stdisp==p->u.s.tl*/
                    rot_rs_flip_left(a, p);
            }
        }
    }
    
    return doit;
}


static bool do_try_unsink_stdisp_para(WSplit *a, WSplit *p, WSplit *stdisp, 
                                      bool force)
{
    
    if(!force){
        if(STDISP_IS_HORIZONTAL(stdisp)){
            if(recommended_w(stdisp)<=p->geom.w)
                return FALSE;
        }else{
            if(recommended_h(stdisp)<=p->geom.h)
                return FALSE;
        }
    }
    
    if(a->u.s.tl==p && p->u.s.tl==stdisp){
        rotate_right(a, p, stdisp);
    }else if(a->u.s.br==p && p->u.s.br==stdisp){
        rotate_left(a, p, stdisp);
    }else{
        warn("Status display badly located in split tree.");
        return FALSE;
    }
    
    split_update_geom_from_children(p);
    
    return TRUE;
}


bool split_try_unsink_stdisp(WSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;
    
    assert(node->type==SPLIT_VERTICAL || node->type==SPLIT_HORIZONTAL);
    
    while(more){
        WSplit *p=node->parent;
        WSplit *tl=node->u.s.tl;
        WSplit *br=node->u.s.br;
        
        if(p==NULL)
            break;
        
        if(tl->type==SPLIT_STDISPNODE){
            if(!stdisp_dir_ok(node, tl))
                break;
            if(p->type==other_dir(node->type)){
                if(!do_try_unsink_stdisp_orth(p, node, tl, force))
                    break;
            }else if(p->type==node->type){
                if(!do_try_unsink_stdisp_para(p, node, tl, force))
                    break;
            }else{
                break;
            }
        }else if(br->type==SPLIT_STDISPNODE){
            if(!stdisp_dir_ok(node, br))
                break;
            if(p->type==other_dir(node->type)){
                if(!do_try_unsink_stdisp_orth(p, node, br, force))
                    break;
            }else if(p->type==node->type){
                if(!do_try_unsink_stdisp_para(p, node, br, force))
                    break;
            }else{
                break;
            }
        }else{
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


bool split_regularise_stdisp(WSplit *stdisp)
{
    WSplit *node=stdisp->parent;
    
    assert(stdisp->type==SPLIT_STDISPNODE);
    
    if(stdisp->parent==NULL)
        return FALSE;
    
    if(STDISP_IS_HORIZONTAL(stdisp)){
        if(stdisp->geom.w<recommended_w(stdisp))
            return split_try_unsink_stdisp(node, TRUE, FALSE);
        else if(stdisp->geom.w>recommended_w(stdisp))
            return split_try_sink_stdisp(node, TRUE, FALSE);
    }else{
        if(stdisp->geom.h<recommended_h(stdisp))
            return split_try_unsink_stdisp(node, TRUE, FALSE);
        else if(stdisp->geom.h>recommended_h(stdisp))
            return split_try_sink_stdisp(node, TRUE, FALSE);
    }
    
    return FALSE;
}


/*}}}*/

