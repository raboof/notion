/*
 * ion/mod_ionws/split-dock.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include "split.h"
#include "split-dock.h"


#define DOCK_GROWS_L_TO_R TRUE
#define DOCK_GROWS_R_TO_L FALSE
#define DOCK_GROWS_T_TO_B FALSE
#define DOCK_GROWS_B_TO_T FALSE
#define DOCK_RECOMMENDED_W 300
#define DOCK_RECOMMENDED_H 32


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
    WSplit *z=*x;
    *x=*y;
    *y=z;
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


static bool dock_dir_ok(WSplit *p, WSplit *dock)
{
    assert(p->u.s.tl==dock || p->u.s.br==dock);
    
    return (IMPLIES(DOCK_GROWS_L_TO_R, p->type==SPLIT_VERTICAL) &&
            IMPLIES(DOCK_GROWS_R_TO_L, p->type==SPLIT_VERTICAL) &&
            IMPLIES(DOCK_GROWS_T_TO_B, p->type==SPLIT_HORIZONTAL) &&
            IMPLIES(DOCK_GROWS_B_TO_T, p->type==SPLIT_HORIZONTAL));
}


/*}}}*/


/*{{{ Sink */


static bool do_try_sink_dock_orth(WSplit *p, WSplit *dock, WSplit *other,
                                  bool force)
{
    bool doit=force;
    
    assert(other->type==SPLIT_VERTICAL || other->type==SPLIT_HORIZONTAL);
    assert(p->type=other_dir(other->type));
    assert(dock->type==SPLIT_DOCKNODE);
    assert(dock_dir_ok(p, dock));

    if(DOCK_GROWS_T_TO_B || DOCK_GROWS_L_TO_R){
        if(DOCK_GROWS_L_TO_R){
            assert(other->type==SPLIT_HORIZONTAL);
            if(other->u.s.tl->geom.w>=DOCK_RECOMMENDED_W)
                doit=TRUE;
        }else{ /* DOCK_GROWS_T_TO_B */
            assert(other->type==SPLIT_VERTICAL);
            if(other->u.s.tl->geom.h>=DOCK_RECOMMENDED_H)
                doit=TRUE;
        }
        
        if(doit){
            if(p->u.s.br==dock){
                /*
                 *      p               p 
                 *    /   \           /   \   (direction of other and p
                 *  other dock  =>  other br   changed)
                 *  /  \            /  \
                 * tl  br          tl  dock
                 */
                rot_rs_flip_right(p, other);
            }else{ /* p->u.s.tl==dock */
                /*
                 *     p                p
                 *   /   \            /  \      (direction of other and p
                 * dock other  =>  other br      changed)
                 *      /  \       /  \   
                 *     tl  br    dock tl
                 */
                rot_rs_rotate_left(p, other, other->u.s.br);
            }
        }
    }else{ /* DOCK_GROWS_B_TO_T or DOCK_GROW_R_TO_L */
        if(DOCK_GROWS_R_TO_L){
            assert(other->type==SPLIT_HORIZONTAL);
            if(other->u.s.br->geom.w>=DOCK_RECOMMENDED_W)
                doit=TRUE;
        }else{ /* DOCK_GROWS_B_TO_T */
            assert(other->type==SPLIT_VERTICAL);
            if(other->u.s.br->geom.h>=DOCK_RECOMMENDED_H)
                doit=TRUE;
        }
        
        if(doit){
            if(p->u.s.tl==dock){
                /*
                 *     p              p
                 *   /   \          /   \      (direction of other and p
                 * dock other  =>  tl  other    changed)
                 *      /  \           /  \
                 *     tl  br        dock  br
                 */
                rot_rs_flip_left(p, other);
            }else{ /* p->u.s.br==dock */
                /*
                 *       p             p
                 *     /   \         /   \      (direction of other and p
                 *  other dock  =>  tl   other   changed)
                 *  /  \                 /  \
                 * tl  br               br  dock
                 */
                rot_rs_rotate_right(p, other, other->u.s.tl);
            }
        }
    }
    
    return doit;
}


static bool do_try_sink_dock_para(WSplit *p, WSplit *dock, WSplit *other,
                                  bool force)
{
    if(!force){
        if(DOCK_GROWS_L_TO_R || DOCK_GROWS_R_TO_L){
            if(DOCK_RECOMMENDED_W>=p->geom.w)
                return FALSE;
        }else{
            if(DOCK_RECOMMENDED_H>=p->geom.h)
                return FALSE;
        }
    }
    
    if(p->u.s.tl==dock)
        rotate_left(p, other, other->u.s.br);
    else
        rotate_right(p, other, other->u.s.tl);
    
    split_update_geom_from_children(other);
    
    return TRUE;
}


bool split_try_sink_dock(WSplit *node, bool iterate, bool force)
{
    bool didsomething=FALSE;
    bool more=TRUE;

    assert(node->type==SPLIT_VERTICAL || node->type==SPLIT_HORIZONTAL);
    
    while(more){
        WSplit *tl=node->u.s.tl;
        WSplit *br=node->u.s.br;

        if(tl->type==SPLIT_DOCKNODE){
            if(!dock_dir_ok(node, tl))
                break;
            if(br->type==other_dir(node->type)){
                if(!do_try_sink_dock_orth(node, tl, br, force))
                    break;
            }else if(br->type==node->type){
                if(!do_try_sink_dock_para(node, tl, br, force))
                    break;
            }else{
                break;
            }
            node=br;
        }else if(br->type==SPLIT_DOCKNODE){
            if(!dock_dir_ok(node, br))
                break;
            if(tl->type==other_dir(node->type)){
                if(!do_try_sink_dock_orth(node, br, tl, force))
                    break;
            }else if(tl->type==node->type){
                if(!do_try_sink_dock_para(node, br, tl, force))
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


static bool do_try_unsink_dock_orth(WSplit *a, WSplit *p, WSplit *dock,
                                    bool force)
{
    bool doit=force;

#warning "Check that we can shrink"

    assert(a->type==SPLIT_VERTICAL || a->type==SPLIT_HORIZONTAL);
    assert(p->type=other_dir(a->type));
    assert(dock->type==SPLIT_DOCKNODE);
    assert(dock_dir_ok(p, dock));
    
    if(DOCK_GROWS_T_TO_B || DOCK_GROWS_L_TO_R){
        if(DOCK_GROWS_L_TO_R){
            assert(a->type==SPLIT_HORIZONTAL);
            if(dock->geom.w<DOCK_RECOMMENDED_W)
                doit=TRUE;
        }else{ /* DOCK_GROWS_T_TO_B */
            assert(a->type==SPLIT_VERTICAL);
            if(dock->geom.h<DOCK_RECOMMENDED_H)
                doit=TRUE;
        }
    
        if(doit){
            if(p==a->u.s.tl){
                if(dock==p->u.s.br)
                    rot_rs_flip_right(a, p);
                else /*dock==p->u.s.tl*/
                    rot_rs_rotate_right(a, p, dock);
            }else{ /*p==a->u.s.br*/
#if 1
                /* abnormal cases. */
                WARN_FUNC("Dock in bad split configuration.");
                return FALSE;
#else                
                if(dock==p->u.s.br)
                    rotate_left(a, p, dock);
                else /*dock==p->u.s.tl*/
                    flip_left(a, p);
#endif
            }
        }
    }else{ /*DOCK_GROWS_B_TO_T || DOCK_GROWS_R_TO_L*/
        if(DOCK_GROWS_R_TO_L){
            assert(a->type==SPLIT_HORIZONTAL);
            if(dock->geom.w<DOCK_RECOMMENDED_W)
                doit=TRUE;
        }else{ /* DOCK_GROWS_B_TO_T */
            assert(a->type==SPLIT_VERTICAL);
            if(dock->geom.h<DOCK_RECOMMENDED_H)
                doit=TRUE;
        }
    
        if(doit){
            if(p==a->u.s.tl){
#if 1
                /* abnormal cases. */
                WARN_FUNC("Dock in bad split configuration.");
                return FALSE;
#else                
                if(dock==p->u.s.br)
                    rot_rs_flip_right(a, p);
                else /*dock==p->u.s.tl*/
                    rot_rs_rotate_right(a, p, dock);
#endif                
            }else{ /*p==a->u.s.br*/
                if(dock==p->u.s.br)
                    rot_rs_rotate_left(a, p, dock);
                else /*dock==p->u.s.tl*/
                    rot_rs_flip_left(a, p);
            }
        }
    }
    
    return doit;
}


static bool do_try_unsink_dock_para(WSplit *a, WSplit *p, WSplit *dock, 
                                    bool force)
{
    if(!force){
        if(DOCK_GROWS_L_TO_R || DOCK_GROWS_R_TO_L){
            if(DOCK_RECOMMENDED_W<=p->geom.w)
                return FALSE;
        }else{
            if(DOCK_RECOMMENDED_H<=p->geom.h)
                return FALSE;
        }
    }
           
    if(a->u.s.tl==p && p->u.s.tl==dock){
        rotate_right(a, p, dock);
    }else if(a->u.s.br==p && p->u.s.br==dock){
        rotate_left(a, p, dock);
    }else{
        warn("Dock badly located in split tree.");
        return FALSE;
    }
    
    split_update_geom_from_children(p);
    
    return TRUE;
}


bool split_try_unsink_dock(WSplit *node, bool iterate, bool force)
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

        if(tl->type==SPLIT_DOCKNODE){
            if(!dock_dir_ok(node, tl))
                break;
            if(p->type==other_dir(node->type)){
                if(!do_try_unsink_dock_orth(p, node, tl, force))
                    break;
            }else if(p->type==node->type){
                if(!do_try_unsink_dock_para(p, node, tl, force))
                    break;
            }else{
                break;
            }
        }else if(br->type==SPLIT_DOCKNODE){
            if(!dock_dir_ok(node, br))
                break;
            if(p->type==other_dir(node->type)){
                if(!do_try_unsink_dock_orth(p, node, br, force))
                    break;
            }else if(p->type==node->type){
                if(!do_try_unsink_dock_para(p, node, br, force))
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


bool split_regularise_dock(WSplit *dock)
{
    WSplit *node=dock->parent;
    
    assert(dock->type==SPLIT_DOCKNODE);
    
    if(dock->parent==NULL)
        return FALSE;
    
    if(DOCK_GROWS_L_TO_R || DOCK_GROWS_R_TO_L){
        if(dock->geom.w<DOCK_RECOMMENDED_W)
            return split_try_unsink_dock(node, TRUE, FALSE);
        else if(dock->geom.w>DOCK_RECOMMENDED_W)
            return split_try_sink_dock(node, TRUE, FALSE);
    }else{
        if(dock->geom.h<DOCK_RECOMMENDED_H)
            return split_try_unsink_dock(node, TRUE, FALSE);
        else if(dock->geom.h>DOCK_RECOMMENDED_H)
            return split_try_sink_dock(node, TRUE, FALSE);
    }
    
    return FALSE;
}


/*}}}*/

