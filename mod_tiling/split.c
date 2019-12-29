/*
 * ion/mod_tiling/split.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <limits.h>
#include <string.h>
#include <X11/Xmd.h>

#include <libtu/minmax.h>
#include <libtu/rb.h>
#include <libtu/objp.h>
#include <ioncore/common.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/window.h>
#include <ioncore/resize.h>
#include <ioncore/attach.h>
#include <ioncore/manage.h>
#include <ioncore/extlconv.h>
#include <ioncore/rectangle.h>
#include <ioncore/saveload.h>
#include <ioncore/names.h>
#include "tiling.h"
#include "split.h"
#include "split-stdisp.h"


static Rb_node split_of_map=NULL;


/*{{{ Geometry helper functions */


int split_size(WSplit *split, int dir)
{
    return (dir==SPLIT_HORIZONTAL ? split->geom.w : split->geom.h);
}

int split_other_size(WSplit *split, int dir)
{
    return (dir==SPLIT_VERTICAL ? split->geom.w : split->geom.h);
}

int split_pos(WSplit *split, int dir)
{
    return (dir==SPLIT_HORIZONTAL ? split->geom.x : split->geom.y);
}

int split_other_pos(WSplit *split, int dir)
{
    return (dir==SPLIT_VERTICAL ? split->geom.x : split->geom.y);
}


/* No, these are not even supposed to be proper/consistent
 * Z \cup {\infty, -\infty} calculation rules.
 */

static int infadd(int x, int y)
{
    if(x==INT_MAX || y==INT_MAX)
        return INT_MAX;
    else
        return x+y;
}


/* Negative "unused space" means no SPLIT_UNUSED under a node, while
 * zero unused space means there's a zero-sized SPLIT_UNUSED under the
 * node.
 */
static int unusedadd(int x, int y)
{
    if(x<0 && y<0)
        return -1;
    return MAXOF(x, 0)+MAXOF(y, 0);
}


static void bound(int *what, int min, int max)
{
    if(*what<min)
        *what=min;
    else if(*what>max)
        *what=max;
}


/*}}}*/


/*{{{ Functions to get and set a region's containing node */


#define node_of_reg splittree_node_of

WSplitRegion *splittree_node_of(WRegion *reg)
{
    Rb_node node=NULL;
    int found=0;

    /*assert(REGION_MANAGER_CHK(reg, WTiling)!=NULL);*/

    if(split_of_map!=NULL){
        node=rb_find_pkey_n(split_of_map, reg, &found);
        if(found)
            return (WSplitRegion*)(node->v.val);
    }

    return NULL;
}


#define set_node_of_reg splittree_set_node_of


bool splittree_set_node_of(WRegion *reg, WSplitRegion *split)
{
    Rb_node node=NULL;
    int found;

    /*assert(REGION_MANAGER_CHK(reg, WTiling)!=NULL);*/

    if(split_of_map==NULL){
        if(split==NULL)
            return TRUE;
        split_of_map=make_rb();
        if(split_of_map==NULL)
            return FALSE;
    }

    node=rb_find_pkey_n(split_of_map, reg, &found);
    if(found)
        rb_delete_node(node);

    return (rb_insertp(split_of_map, reg, split)!=NULL);
}


/*}}}*/


/*{{{ Primn */


WPrimn primn_invert(WPrimn primn)
{
    return (primn==PRIMN_TL
            ? PRIMN_BR
            : (primn==PRIMN_BR
               ? PRIMN_TL
               : primn));
}


WPrimn primn_none2any(WPrimn primn)
{
    return (primn==PRIMN_NONE ? PRIMN_ANY : primn);
}


/*}}}*/


/*{{{ Create */


bool split_init(WSplit *split, const WRectangle *geom)
{
    split->parent=NULL;
    split->ws_if_root=NULL;
    split->geom=*geom;
    split->min_w=0;
    split->min_h=0;
    split->max_w=INT_MAX;
    split->max_h=INT_MAX;
    split->unused_w=-1;
    split->unused_h=-1;
    return TRUE;
}

bool splitinner_init(WSplitInner *split, const WRectangle *geom)
{
    return split_init(&(split->split), geom);
}


bool splitsplit_init(WSplitSplit *split, const WRectangle *geom, int dir)
{
    splitinner_init(&(split->isplit), geom);
    split->dir=dir;
    split->tl=NULL;
    split->br=NULL;
    split->current=SPLIT_CURRENT_TL;
    return TRUE;
}


bool splitregion_init(WSplitRegion *split, const WRectangle *geom,
                      WRegion *reg)
{
    split_init(&(split->split), geom);
    split->reg=reg;
    if(reg!=NULL)
        set_node_of_reg(reg, split);
    return TRUE;
}


bool splitst_init(WSplitST *split, const WRectangle *geom, WRegion *reg)
{
    splitregion_init(&(split->regnode), geom, reg);
    split->orientation=REGION_ORIENTATION_HORIZONTAL;
    split->corner=MPLEX_STDISP_BL;
    return TRUE;
}


WSplitSplit *create_splitsplit(const WRectangle *geom, int dir)
{
    CREATEOBJ_IMPL(WSplitSplit, splitsplit, (p, geom, dir));
}


WSplitRegion *create_splitregion(const WRectangle *geom, WRegion *reg)
{
    CREATEOBJ_IMPL(WSplitRegion, splitregion, (p, geom, reg));
}


WSplitST *create_splitst(const WRectangle *geom, WRegion *reg)
{
    CREATEOBJ_IMPL(WSplitST, splitst, (p, geom, reg));
}


/*}}}*/


/*{{{ Deinit */


void split_deinit(WSplit *split)
{
    assert(split->parent==NULL);
}


void splitinner_deinit(WSplitInner *split)
{
    split_deinit(&(split->split));
}


void splitsplit_deinit(WSplitSplit *split)
{
    if(split->tl!=NULL){
        split->tl->parent=NULL;
        destroy_obj((Obj*)(split->tl));
    }
    if(split->br!=NULL){
        split->br->parent=NULL;
        destroy_obj((Obj*)(split->br));
    }

    splitinner_deinit(&(split->isplit));
}


void splitregion_deinit(WSplitRegion *split)
{
    if(split->reg!=NULL){
        set_node_of_reg(split->reg, NULL);
        split->reg=NULL;
    }

    split_deinit(&(split->split));
}


void splitst_deinit(WSplitST *split)
{
    splitregion_deinit(&(split->regnode));
}


/*}}}*/


/*{{{ Size bounds management */


static void splitregion_update_bounds(WSplitRegion *node, bool UNUSED(recursive))
{
    WSizeHints hints;
    WSplit *snode=(WSplit*)node;

    assert(node->reg!=NULL);

    region_size_hints(node->reg, &hints);

    snode->min_w=MAXOF(1, hints.min_set ? hints.min_width : 1);
    snode->max_w=INT_MAX;
    snode->unused_w=-1;

    snode->min_h=MAXOF(1, hints.min_set ? hints.min_height : 1);
    snode->max_h=INT_MAX;
    snode->unused_h=-1;
}


static void splitst_update_bounds(WSplitST *node, bool UNUSED(rec))
{
    WSplit *snode=(WSplit*)node;

    if(node->regnode.reg==NULL){
        snode->min_w=CF_STDISP_MIN_SZ;
        snode->min_h=CF_STDISP_MIN_SZ;
        snode->max_w=CF_STDISP_MIN_SZ;
        snode->max_h=CF_STDISP_MIN_SZ;
    }else{
        WSizeHints hints;
        region_size_hints(node->regnode.reg, &hints);
        snode->min_w=MAXOF(1, hints.min_set ? hints.min_width : 1);
        snode->max_w=MAXOF(snode->min_w, hints.min_width);
        snode->min_h=MAXOF(1, hints.min_set ? hints.min_height : 1);
        snode->max_h=MAXOF(snode->min_h, hints.min_height);
    }

    snode->unused_w=-1;
    snode->unused_h=-1;

    if(node->orientation==REGION_ORIENTATION_HORIZONTAL){
        snode->min_w=CF_STDISP_MIN_SZ;
        snode->max_w=INT_MAX;
    }else{
        snode->min_h=CF_STDISP_MIN_SZ;
        snode->max_h=INT_MAX;
    }
}


static void splitsplit_update_bounds(WSplitSplit *split, bool recursive)
{
    WSplit *tl, *br;
    WSplit *node=(WSplit*)split;

    assert(split->tl!=NULL && split->br!=NULL);

    tl=split->tl;
    br=split->br;

    if(recursive){
        split_update_bounds(tl, TRUE);
        split_update_bounds(br, TRUE);
    }

    if(split->dir==SPLIT_HORIZONTAL){
        node->max_w=infadd(tl->max_w, br->max_w);
        node->min_w=infadd(tl->min_w, br->min_w);
        node->unused_w=unusedadd(tl->unused_w, br->unused_w);
        node->min_h=MAXOF(tl->min_h, br->min_h);
        node->max_h=MAXOF(MINOF(tl->max_h, br->max_h), node->min_h);
        node->unused_h=MINOF(tl->unused_h, br->unused_h);
    }else{
        node->max_h=infadd(tl->max_h, br->max_h);
        node->min_h=infadd(tl->min_h, br->min_h);
        node->unused_h=unusedadd(tl->unused_h, br->unused_h);
        node->min_w=MAXOF(tl->min_w, br->min_w);
        node->max_w=MAXOF(MINOF(tl->max_w, br->max_w), node->min_w);
        node->unused_w=MINOF(tl->unused_w, br->unused_w);
    }
}


void split_update_bounds(WSplit *node, bool recursive)
{
    CALL_DYN(split_update_bounds, node, (node, recursive));
}


void splitsplit_update_geom_from_children(WSplitSplit *node)
{
    if(node->dir==SPLIT_VERTICAL){
        ((WSplit*)node)->geom.h=node->tl->geom.h+node->br->geom.h;
        ((WSplit*)node)->geom.y=node->tl->geom.y;
    }else if(node->dir==SPLIT_HORIZONTAL){
        ((WSplit*)node)->geom.w=node->tl->geom.w+node->br->geom.w;
        ((WSplit*)node)->geom.x=node->tl->geom.x;
    }
}


/*}}}*/


/*{{{ Status display handling helper functions. */


static WSplitST *saw_stdisp=NULL;


void splittree_begin_resize()
{
    saw_stdisp=NULL;
}


void splittree_end_resize()
{
    if(saw_stdisp!=NULL){
        split_regularise_stdisp(saw_stdisp);
        saw_stdisp=NULL;
    }
}


static void splittree_scan_stdisp_rootward_(WSplitInner *node_)
{
    WSplitSplit *node=OBJ_CAST(node_, WSplitSplit);

    if(node!=NULL){
        if(OBJ_IS(node->tl, WSplitST)){
            saw_stdisp=(WSplitST*)(node->tl);
            return;
        }else if(OBJ_IS(node->br, WSplitST)){
            saw_stdisp=(WSplitST*)(node->br);
            return;
        }
    }

    if(node_->split.parent!=NULL)
        splittree_scan_stdisp_rootward_(node_->split.parent);
}


void splittree_scan_stdisp_rootward(WSplit *node)
{
    if(node->parent!=NULL)
        splittree_scan_stdisp_rootward_(node->parent);
}


static WSplitSplit *splittree_scan_stdisp_parent(WSplit *node_, bool set_saw)
{
    WSplitSplit *r, *node=OBJ_CAST(node_, WSplitSplit);

    if(node==NULL)
        return NULL;

    if(OBJ_IS(node->tl, WSplitST)){
        if(set_saw)
            saw_stdisp=(WSplitST*)node->tl;
        return node;
    }

    if(OBJ_IS(node->br, WSplitST)){
        if(set_saw)
            saw_stdisp=(WSplitST*)node->br;
        return node;
    }

    r=splittree_scan_stdisp_parent(node->tl, set_saw);
    if(r==NULL)
        r=splittree_scan_stdisp_parent(node->br, set_saw);
    return r;
}


static bool stdisp_immediate_child(WSplitSplit *node)
{
    return (node!=NULL && (OBJ_IS(node->tl, WSplitST) ||
                           OBJ_IS(node->br, WSplitST)));
}


static WSplit *move_stdisp_out_of_way(WSplit *node)
{
    WSplitSplit *stdispp;

    if(!OBJ_IS(node, WSplitSplit))
        return node;

    stdispp=splittree_scan_stdisp_parent(node, TRUE);

    if(stdispp==NULL)
        return node;

    while(stdispp->tl!=node && stdispp->br!=node){
        if(!split_try_unsink_stdisp(stdispp, FALSE, TRUE)){
            warn(TR("Unable to move the status display out of way."));
            return NULL;
        }
    }

    return (WSplit*)stdispp;
}


/*}}}*/


/*{{{ Low-level resize code; from root to leaf */


static void split_do_resize_default(WSplit *node, const WRectangle *ng,
                                    WPrimn UNUSED(hprimn), WPrimn UNUSED(vprimn),
                                    bool UNUSED(transpose))
{
    node->geom=*ng;
}


static void splitregion_do_resize(WSplitRegion *node, const WRectangle *ng,
                                  WPrimn UNUSED(hprimn), WPrimn UNUSED(vprimn),
                                  bool UNUSED(transpose))
{
    assert(node->reg!=NULL);
    region_fit(node->reg, ng, REGION_FIT_EXACT);
    split_update_bounds(&(node->split), FALSE);
    node->split.geom=*ng;
}


static void splitst_do_resize(WSplitST *node, const WRectangle *ng,
                              WPrimn hprimn, WPrimn vprimn,
                              bool transpose)
{
    saw_stdisp=node;

    if(node->regnode.reg==NULL){
        ((WSplit*)node)->geom=*ng;
    }else{
        splitregion_do_resize(&(node->regnode), ng, hprimn, vprimn,
                               transpose);
    }
}


static int other_dir(int dir)
{
    return (dir==SPLIT_VERTICAL ? SPLIT_HORIZONTAL : SPLIT_VERTICAL);
}


static void adjust_sizes(int *tls_, int *brs_, int nsize, int sz,
                         int tlmin, int brmin, int tlmax, int brmax,
                         int primn)
{
    int tls=*tls_;
    int brs=*brs_;

    if(primn==PRIMN_TL){
        tls=tls+nsize-sz;
        bound(&tls, tlmin, tlmax);
        brs=nsize-tls;
        bound(&brs, brmin, brmax);
        tls=nsize-brs;
        bound(&tls, tlmin, tlmax);
    }else if(primn==PRIMN_BR){
        brs=brs+nsize-sz;
        bound(&brs, brmin, brmax);
        tls=nsize-brs;
        bound(&tls, tlmin, tlmax);
        brs=nsize-tls;
        bound(&brs, brmin, brmax);
    }else{ /* && PRIMN_ANY */
        tls=tls*nsize/sz;
        bound(&tls, tlmin, tlmax);
        brs=nsize-tls;
        bound(&brs, brmin, brmax);
        tls=nsize-brs;
        bound(&tls, tlmin, tlmax);
    }

    *tls_=tls;
    *brs_=brs;
}


static void get_minmaxunused(WSplit *node, int dir,
                             int *min, int *max, int *unused)
{
    if(dir==SPLIT_VERTICAL){
        *min=node->min_h;
        *max=MAXOF(*min, node->max_h);
        *unused=MINOF(node->unused_h, node->geom.h);
    }else{
        *min=node->min_w;
        *max=MAXOF(*min, node->max_w);
        *unused=MINOF(node->unused_w, node->geom.w);
    }
}


void splitsplit_do_resize(WSplitSplit *node, const WRectangle *ng,
                          WPrimn hprimn, WPrimn vprimn, bool transpose)
{
    assert(ng->w>=0 && ng->h>=0);
    assert(node->tl!=NULL && node->br!=NULL);
    assert(!transpose || (hprimn==PRIMN_ANY && vprimn==PRIMN_ANY));

    {
        WSplit *tl=node->tl, *br=node->br;
        int tls=split_size((WSplit*)tl, node->dir);
        int brs=split_size((WSplit*)br, node->dir);
        int sz=tls+brs;
        /* Status display can not be transposed. */
        int dir=((transpose && !stdisp_immediate_child(node))
                 ? other_dir(node->dir)
                 : node->dir);
        int nsize=(dir==SPLIT_VERTICAL ? ng->h : ng->w);
        int primn=(dir==SPLIT_VERTICAL ? vprimn : hprimn);
        int tlmin, tlmax, tlunused, tlused;
        int brmin, brmax, brunused, brused;
        WRectangle tlg=*ng, brg=*ng;

        get_minmaxunused(tl, dir, &tlmin, &tlmax, &tlunused);
        get_minmaxunused(br, dir, &brmin, &brmax, &brunused);

        tlused=MAXOF(0, tls-MAXOF(0, tlunused));
        brused=MAXOF(0, brs-MAXOF(0, brunused));
        /* tlmin,  brmin >= 1 => (tls>=tlmin, brs>=brmin => sz>0) */

        if(sz>2){
            if(primn==PRIMN_ANY && (tlunused>=0 || brunused>=0)){
                if(nsize<=tlused+brused){
                    /* Need to shrink a tangible node */
                    adjust_sizes(&tls, &brs, nsize, sz,
                                 tlmin, brmin, tlused, brused, primn);
                }else{
                    /* Just expand or shrink unused space */
                    adjust_sizes(&tls, &brs, nsize, sz,
                                 tlused, brused,
                                 (tlunused<0 ? tlused : tlmax),
                                 (brunused<0 ? brused : brmax), primn);
                }

            }else{
                adjust_sizes(&tls, &brs, nsize, sz,
                             tlmin, brmin, tlmax, brmax, primn);
            }
        }

        if(tls+brs!=nsize){
            /* Bad fit; just size proportionally. */
            if(sz<=2){
                tls=nsize/2;
                brs=nsize-tls;
            }else{
                tls=split_size(tl, node->dir)*nsize/sz;
                brs=nsize-tls;
            }
        }

        if(dir==SPLIT_VERTICAL){
            tlg.h=tls;
            brg.y+=tls;
            brg.h=brs;
        }else{
            tlg.w=tls;
            brg.x+=tls;
            brg.w=brs;
        }

        split_do_resize(tl, &tlg, hprimn, vprimn, transpose);
        split_do_resize(br, &brg, hprimn, vprimn, transpose);

        node->dir=dir;
        ((WSplit*)node)->geom=*ng;
        split_update_bounds((WSplit*)node, FALSE);
    }
}


void split_do_resize(WSplit *node, const WRectangle *ng,
                     WPrimn hprimn, WPrimn vprimn, bool transpose)
{
    CALL_DYN(split_do_resize, node, (node, ng, hprimn, vprimn, transpose));
}


void split_resize(WSplit *node, const WRectangle *ng,
                  WPrimn hprimn, WPrimn vprimn)
{
    split_update_bounds(node, TRUE);
    splittree_begin_resize();
    split_do_resize(node, ng, hprimn, vprimn, FALSE);
    splittree_end_resize();
}


/*}}}*/


/*{{{ Save, restore and verify code for maximization */


bool splits_are_related(WSplit *p, WSplit *node)
{
    if(p==node)
        return TRUE;

    return
        node->parent!=NULL
        ? splits_are_related(p, (WSplit*)node->parent)
        : FALSE;
}


WSplit *maxparentdir_rel(WSplit *p, WSplit *node, int dir)
{
    /* Descending from p, try to determine the first split of type dir between
     * p and node, while ignoring a potential stdisp. */
    if(OBJ_IS(p, WSplitSplit)){
        WSplitSplit *sp=(WSplitSplit*)p;
        assert(sp->tl!=NULL && sp->br!=NULL);
        assert(splits_are_related(sp->tl, node) ||
               splits_are_related(sp->br, node));

        if(OBJ_IS(sp->tl, WSplitST))
            return maxparentdir_rel(sp->br, node, dir);
        if(OBJ_IS(sp->br, WSplitST))
            return maxparentdir_rel(sp->tl, node, dir);

        if(sp->dir!=dir){
            return
                splits_are_related(sp->tl, node)
                ? maxparentdir_rel(sp->tl, node, dir)
                : maxparentdir_rel(sp->br, node, dir);
        }
    }

    return p;
}


WSplit *maxparent(WSplit *node)
{
    WSplit *p=(WSplit*)node->parent;
    return p==NULL ? node : maxparent(p);
}


WSplit *maxparentdir(WSplit *node, int dir)
{
    return maxparentdir_rel(maxparent(node), node, dir);
}

int *wh(WRectangle *geom, int orientation)
{
    return orientation==REGION_ORIENTATION_HORIZONTAL ? &geom->w : &geom->h;
}

int *xy(WRectangle *geom, int orientation)
{
    return orientation==REGION_ORIENTATION_HORIZONTAL ? &geom->x : &geom->y;
}

bool is_lt(int orientation, int corner)
{
    /* Read as "is_left" or "is_top", depending on the orientation. */
    return
        orientation==REGION_ORIENTATION_HORIZONTAL
        ? corner==MPLEX_STDISP_TL || corner==MPLEX_STDISP_BL
        : corner==MPLEX_STDISP_TL || corner==MPLEX_STDISP_TR;
}

int flip_orientation(int orientation)
{
    return
        orientation==REGION_ORIENTATION_HORIZONTAL
        ? REGION_ORIENTATION_VERTICAL
        : REGION_ORIENTATION_HORIZONTAL;
}

WRectangle stdisp_recommended_geom(WSplitST *st, WRectangle wsg)
{
    /* wsg holds the geometry of the workspace that st is on. */
    WRectangle stg=REGION_GEOM(st->regnode.reg);
    int ori=st->orientation;
    stg.w=stdisp_recommended_w(st);
    stg.h=stdisp_recommended_h(st);

    if(!is_lt(ori, st->corner))
        *xy(&stg, ori)=*wh(&wsg, ori)-*wh(&stg, ori);

    return stg;
}

bool geom_overlaps_stgeom_xy(WRectangle geom, WSplitST *st, WRectangle stg)
{
    /*    TRUE                         FALSE
     *
     *     ------                           ------
     *     |geom|                           |geom|
     *     ------                           ------
     *  -----                        -----
     *  |stg|                        |stg|
     *  -----                        -----
     */
    int ori=st->orientation;

    return
        is_lt(ori, st->corner)
        ? *xy(&geom, ori)<*wh(&stg, ori)
        : *xy(&geom, ori)+*wh(&geom, ori)>*xy(&stg, ori);
}

bool geom_aligned_stdisp(WRectangle geom, WSplitST *st)
{
    /*        TRUE               FALSE              FALSE
     *
     *                                                  ------
     *                                                  |geom|
     *           ------                                 ------
     *           |geom|              ------
     *    -----  ------       -----  |geom|      -----
     *    |stg|               |stg|  ------      |stg|
     *    -----               -----              -----
     *
     */
    WRectangle stg=REGION_GEOM(st->regnode.reg);
    int ori=flip_orientation(st->orientation);

    return
        is_lt(ori, st->corner)
        ? *xy(&geom, ori)==*wh(&stg, ori)
        : *xy(&geom, ori)+*wh(&geom, ori)==*xy(&stg, ori);
}

void grow_by_stdisp_wh(WRectangle *geom, WSplitST *st)
{
    /*          BEFORE                      AFTER
     *
     *
     *              ------                     ------
     *              |geom|                     |    |
     *      -----  ------               -----  |geom|
     *      |stg|                       |stg|  |    |
     *      -----                       -----  ------
     *
     */
    WRectangle stg=REGION_GEOM(st->regnode.reg);
    int ori=flip_orientation(st->orientation);

    if(is_lt(ori, st->corner))
        *xy(geom, ori)=0;
    *wh(geom, ori)+=*wh(&stg, ori);
}

bool frame_neighbors_stdisp(WFrame *frame, WSplitST *st)
{
    return
        geom_overlaps_stgeom_xy(REGION_GEOM(frame), st, REGION_GEOM(st)) &&
        geom_aligned_stdisp(REGION_GEOM(frame), st);
}

bool geom_clashes_stdisp(WRectangle geom, WSplitST *st)
{
    WRectangle stg=REGION_GEOM(st->regnode.reg);
    int ori=flip_orientation(st->orientation);
    return
        is_lt(ori, st->corner)
        ? *xy(&geom, ori)==0
        : *xy(&geom, ori)+*wh(&geom, ori)==*xy(&stg, ori)+*wh(&stg, ori);
}

bool is_same_dir(int dir, int ori)
{
    return
        (dir==SPLIT_HORIZONTAL && ori==REGION_ORIENTATION_HORIZONTAL) ||
        (dir==SPLIT_VERTICAL && ori==REGION_ORIENTATION_VERTICAL);
}

bool is_maxed(WFrame *frame, int dir)
{
    return
        dir==SPLIT_HORIZONTAL
        ? frame->flags&FRAME_MAXED_HORIZ && frame->flags&FRAME_SAVED_HORIZ
        : frame->flags&FRAME_MAXED_VERT && frame->flags&FRAME_SAVED_VERT;
}

bool update_geom_from_stdisp(WFrame *frame, WRectangle *ng, int dir)
{
    WRegion *ws=REGION_MANAGER(frame);
    WSplitST *st;
    WRectangle stg;
    WRectangle rstg;
    int ori;

    if(!OBJ_IS(ws, WTiling) || ((WTiling*)ws)->stdispnode==NULL)
        return FALSE;

    st=((WTiling*)ws)->stdispnode;

    if(st->fullsize || !frame_neighbors_stdisp(frame, st))
        return FALSE;

    rstg=stdisp_recommended_geom(st, ws->geom);

    if(is_same_dir(dir, st->orientation) &&
       !geom_overlaps_stgeom_xy(*ng, st, rstg))
    {
        grow_by_stdisp_wh(ng, st);
        if(is_maxed(frame, other_dir(dir)) &&
           geom_aligned_stdisp(frame->saved_geom, st))
        {
            grow_by_stdisp_wh(&frame->saved_geom, st);
        }
        return TRUE;
    }

    if(!is_same_dir(dir, st->orientation) &&
       geom_clashes_stdisp(frame->saved_geom, st))
    {
        stg=REGION_GEOM(st->regnode.reg);
        ori=flip_orientation(st->orientation);
        if(is_lt(ori, st->corner))
            *xy(ng, ori)+=*wh(&stg, ori);
        /* We've checked that this makes sense when verifying the saved layout. */
        *wh(ng, ori)-=*wh(&stg, ori);
    }

    return FALSE;
}

bool splitregion_do_restore(WSplitRegion *node, int dir)
{
    WFrame *frame;
    WRectangle geom=((WSplit*)node)->geom;
    WRectangle fakegeom;
    bool ret;
    bool other_max;

    if(!OBJ_IS(node->reg, WFrame))
        return FALSE;

    frame=(WFrame*)node->reg;
    if(dir==SPLIT_HORIZONTAL){
        geom.x=frame->saved_geom.x;
        geom.w=frame->saved_geom.w;
    }else{
        geom.y=frame->saved_geom.y;
        geom.h=frame->saved_geom.h;
    }

    other_max=
        dir==SPLIT_HORIZONTAL
        ? frame->flags&FRAME_MAXED_VERT
        : frame->flags&FRAME_MAXED_HORIZ;

    fakegeom=geom;
    ret=update_geom_from_stdisp(frame, &geom, dir);

    /* Tell the region the correct geometry to avoid redrawing it again when
     * the stdisp is resized by split_regularise_stdisp. Some clients (notably
     * ncurses based ones) don't seem to react well to being resized multiple
     * times within a short amount of time and don't refresh themselves
     * correctly. */
    region_fit(node->reg, &geom, REGION_FIT_EXACT);

    split_update_bounds(&(node->split), FALSE);

    /* Keep the old geometry for the WSplit. Otherwise the tiling would be
     * inconsistent, by for example having horizontal WSplitSplit's whose
     * children have different heights. The call to split_regularise_stdisp
     * below will take care of correcting the geometry of the WSplit and it
     * behaves badly when the tiling is inconsistent. */
    node->split.geom=ret ? fakegeom : geom;

    frame->flags|=other_max;
    return ret;
}

bool splitst_do_restore(WSplit *UNUSED(node), int UNUSED(dir))
{
    return FALSE;
}

bool splitsplit_do_restore(WSplitSplit *node, int dir)
{
    bool ret1, ret2, ret=FALSE;
    WSplit *snode=(WSplit*)node;

    WSplitST *st;
    WSplit *other;
    WRectangle stg;
    WRectangle og;

    assert(node->tl!=NULL && node->br!=NULL);

    if(stdisp_immediate_child(node)){
        if(OBJ_IS(node->tl, WSplitST)){
            st=(WSplitST*)node->tl;
            other=node->br;
        }else{
            st=(WSplitST*)node->br;
            other=node->tl;
        }
        stg=((WSplit*)st)->geom;
        split_do_restore(other, dir);
        og=other->geom;
        if(node->dir==SPLIT_HORIZONTAL){
            stg.y=og.y;
            stg.h=og.h;
        }else{
            stg.x=og.x;
            stg.w=og.w;
        }
        if(rectangle_compare(&stg, &((WSplit*)st)->geom)){
            splitst_do_resize(st, &stg, PRIMN_ANY, PRIMN_ANY, FALSE);
            ret=TRUE;
        }
    }else{
        /* Avoid short-circuit evaluation. */
        ret1=split_do_restore(node->tl, dir);
        ret2=split_do_restore(node->br, dir);
        ret=ret1 || ret2;
    }

    snode->geom.x=node->tl->geom.x;
    snode->geom.y=node->tl->geom.y;
    if(node->dir==SPLIT_HORIZONTAL){
        snode->geom.w=node->tl->geom.w+node->br->geom.w;
        snode->geom.h=node->tl->geom.h;
    }
    if(node->dir==SPLIT_VERTICAL){
        snode->geom.w=node->tl->geom.w;
        snode->geom.h=node->tl->geom.h+node->br->geom.h;
    }

    return ret;
}

bool split_do_restore(WSplit *node, int dir)
{
    bool ret = FALSE;
    CALL_DYN_RET(ret, bool, split_do_restore, node, (node, dir));
    return ret;
}


void splitregion_do_maxhelper(WSplitRegion *node, int dir, int action)
{
    WFrame *frame;
    if(!OBJ_IS(node->reg, WFrame))
        return;
    frame=(WFrame*)node->reg;

    if(action==SAVE){
        frame->flags|=FRAME_KEEP_FLAGS;
        if(dir==HORIZONTAL){
            frame->flags|=(FRAME_MAXED_HORIZ|FRAME_SAVED_HORIZ);
            frame->saved_geom.x=REGION_GEOM(frame).x;
            frame->saved_geom.w=REGION_GEOM(frame).w;
        }else{
            frame->flags|=(FRAME_MAXED_VERT|FRAME_SAVED_VERT);
            frame->saved_geom.y=REGION_GEOM(frame).y;
            frame->saved_geom.h=REGION_GEOM(frame).h;
        }
    }
    if(action==SET_KEEP)
        frame->flags|=FRAME_KEEP_FLAGS;
    if(action==RM_KEEP)
        frame->flags&=~FRAME_KEEP_FLAGS;
}

void splitst_do_maxhelper(WSplit *UNUSED(node), int UNUSED(dir), int UNUSED(action))
{
    return;
}

void splitsplit_do_maxhelper(WSplitSplit *node, int dir, int action)
{
    assert(node->tl!=NULL && node->br!=NULL);
    split_do_maxhelper(node->tl, dir, action);
    split_do_maxhelper(node->br, dir, action);
}

void split_do_maxhelper(WSplit *node, int dir, int action)
{
    CALL_DYN(split_do_maxhelper, node, (node, dir, action));
}


bool savedgeom_clashes_stdisp(WFrame *frame, int dir)
{
    WRegion *ws=REGION_MANAGER(frame);
    WSplitST *st;
    int ori;

    if(!OBJ_IS(ws, WTiling) || ((WTiling*)ws)->stdispnode==NULL)
        return TRUE;

    st=((WTiling*)ws)->stdispnode;
    ori=flip_orientation(st->orientation);

    return
        !is_same_dir(dir, st->orientation) &&
        frame_neighbors_stdisp(frame, st) &&
        geom_clashes_stdisp(frame->saved_geom, st)
        ? *wh(&frame->saved_geom, ori)<*wh(&REGION_GEOM(st), ori)
        : FALSE;
}

bool splitregion_do_verify(WSplitRegion *node, int dir)
{
    WFrame *frame;
    bool ret=FALSE;

    if(!OBJ_IS(node->reg, WFrame))
        return FALSE;

    frame=(WFrame*)node->reg;

    ret=is_maxed(frame, dir);

    if(dir==HORIZONTAL)
        frame->flags&=~(FRAME_MAXED_HORIZ|FRAME_SAVED_HORIZ);
    else
        frame->flags&=~(FRAME_MAXED_VERT|FRAME_SAVED_VERT);

    if(savedgeom_clashes_stdisp(frame, dir))
        return FALSE;

    return ret;
}

bool splitst_do_verify(WSplit *UNUSED(node), int UNUSED(dir))
{
    return TRUE;
}

bool splitsplit_do_verify(WSplitSplit *node, int dir)
{
    bool ret1, ret2;
    assert(node->tl!=NULL && node->br!=NULL);

    /* Avoid short-circuit evaluation. */
    ret1=split_do_verify(node->tl, dir);
    ret2=split_do_verify(node->br, dir);
    return ret1 && ret2;
}

bool split_do_verify(WSplit *node, int dir)
{
    bool ret = FALSE;
    CALL_DYN_RET(ret, bool, split_do_verify, node, (node, dir));
    return ret;
}


bool split_maximize(WSplit *node, int dir, int action)
{
    WSplit *p=maxparentdir(node, dir);
    if(action==RESTORE)
        return split_do_restore(p, dir);
    if(action==VERIFY)
        return split_do_verify(p, dir);

    split_do_maxhelper(p, dir, action);
    return TRUE;
}


/*}}}*/


/*{{{ Low-level resize code; request towards root */


static void flexibility(WSplit *node, int dir, int *shrink, int *stretch)
{
    if(dir==SPLIT_VERTICAL){
        *shrink=MAXOF(0, node->geom.h-node->min_h);
        if(OBJ_IS(node, WSplitST))
            *stretch=MAXOF(0, node->max_h-node->geom.h);
        else
            *stretch=INT_MAX;
    }else{
        *shrink=MAXOF(0, node->geom.w-node->min_w);
        if(OBJ_IS(node, WSplitST))
            *stretch=MAXOF(0, node->max_w-node->geom.w);
        else
            *stretch=INT_MAX;
    }
}


static void calc_amount(int *amount, int rs, WSplit *other, int dir)
{
    int shrink, stretch;

    flexibility(other, dir, &shrink, &stretch);

    if(rs>0)
        *amount=MINOF(rs, shrink);
    else if(rs<0)
        *amount=-MINOF(-rs, stretch);
    else
        *amount=0;
}



static void splitsplit_do_rqsize(WSplitSplit *p, WSplit *node,
                                 RootwardAmount *ha, RootwardAmount *va,
                                 WRectangle *rg, bool tryonly)
{
    WPrimn hprimn=PRIMN_ANY, vprimn=PRIMN_ANY;
    WRectangle og, pg, ng;
    RootwardAmount *ca;
    WSplit *other;
    WPrimn thisnode;
    int amount;

    assert(!ha->any || ha->tl==0);
    assert(!va->any || va->tl==0);
    assert(p->tl==node || p->br==node);

    if(p->tl==node){
        other=p->br;
        thisnode=PRIMN_TL;
    }else{
        other=p->tl;
        thisnode=PRIMN_BR;
    }

    ca=(p->dir==SPLIT_VERTICAL ? va : ha);

    if(thisnode==PRIMN_TL || ca->any){
        calc_amount(&amount, ca->br, other, p->dir);
        ca->br-=amount;
    }else/*if(thisnode==PRIMN_BR)*/{
        calc_amount(&amount, ca->tl, other, p->dir);
        ca->tl-=amount;
    }

    if(((WSplit*)p)->parent==NULL /*||
       (ha->tl==0 && ha->br==0 && va->tl==0 && va->br==0)*/){
        if(((WSplit*)p)->ws_if_root!=NULL)
            pg=REGION_GEOM((WTiling*)(((WSplit*)p)->ws_if_root));
        else
            pg=((WSplit*)p)->geom;
    }else{
        splitinner_do_rqsize(((WSplit*)p)->parent, (WSplit*)p, ha, va,
                             &pg, tryonly);
    }

    assert(pg.w>=0 && pg.h>=0);

    og=pg;
    ng=pg;

    if(p->dir==SPLIT_VERTICAL){
        ng.h=MAXOF(0, node->geom.h+amount);
        og.h=MAXOF(0, other->geom.h-amount);
        adjust_sizes(&(ng.h), &(og.h), pg.h, ng.h+og.h,
                     node->min_h, other->min_h, node->max_h, other->max_h,
                     PRIMN_TL /* node is passed as tl param */);
        if(thisnode==PRIMN_TL)
            og.y=pg.y+pg.h-og.h;
        else
            ng.y=pg.y+pg.h-ng.h;
        vprimn=thisnode;
    }else{
        ng.w=MAXOF(0, node->geom.w+amount);
        og.w=MAXOF(0, other->geom.w-amount);
        adjust_sizes(&(ng.w), &(og.w), pg.w, ng.w+og.w,
                     node->min_w, other->min_w, node->max_w, other->max_w,
                     PRIMN_TL /* node is passed as tl param */);
        if(thisnode==PRIMN_TL)
            og.x=pg.x+pg.w-og.w;
        else
            ng.x=pg.x+pg.w-ng.w;
        hprimn=thisnode;
    }

    if(!tryonly){
        /* Entä jos 'other' on stdisp? */
        split_do_resize(other, &og, hprimn, vprimn, FALSE);

        ((WSplit*)p)->geom=pg;
    }

    *rg=ng;
}


void splitinner_do_rqsize(WSplitInner *p, WSplit *node,
                          RootwardAmount *ha, RootwardAmount *va,
                          WRectangle *rg, bool tryonly)
{
    CALL_DYN(splitinner_do_rqsize, p, (p, node, ha, va, rg, tryonly));
}


static void initra(RootwardAmount *ra, int p, int s, int op, int os,
                   bool any)
{
    ra->any=any;
    ra->tl=op-p;
    ra->br=(p+s)-(op+os);
    if(any){
        ra->br+=ra->tl;
        ra->tl=0;
    }
}


void split_do_rqgeom_(WSplit *node, const WRectangle *ng,
                      bool hany, bool vany, WRectangle *rg,
                      bool tryonly)
{
    RootwardAmount ha, va;

    if(node->parent==NULL){
        if(node->ws_if_root!=NULL)
            *rg=REGION_GEOM((WTiling*)(node->ws_if_root));
        else
            *rg=*ng;
    }else{
        initra(&ha, ng->x, ng->w, node->geom.x, node->geom.w, hany);
        initra(&va, ng->y, ng->h, node->geom.y, node->geom.h, vany);

        splitinner_do_rqsize(node->parent, node, &ha, &va, rg, tryonly);
    }
}


/*}}}*/


/*{{{ Resize interface */


static void bnd(int *pos, int *sz, int opos, int osz, int minsz, int maxsz)
{
    int ud=abs(*pos-opos);
    int dd=abs((*pos+*sz)-(opos+osz));
    int szrq=*sz;

    if(ud+dd!=0){
        bound(sz, minsz, maxsz);
        *pos+=(szrq-*sz)*ud/(ud+dd);
    }
}


WSplit *split_find_root(WSplit *split)
{
    if(split->parent==NULL)
        return split;
    return split_find_root((WSplit*)split->parent);
}


void splittree_rqgeom(WSplit *sub, int flags, const WRectangle *geom_,
                      WRectangle *geomret)
{
    bool hany=flags&REGION_RQGEOM_WEAK_X;
    bool vany=flags&REGION_RQGEOM_WEAK_Y;
    bool tryonly=flags&REGION_RQGEOM_TRYONLY;
    WRectangle geom=*geom_;
    WRectangle retg;
    WSplit *root=split_find_root(sub);

    if(geomret==NULL)
        geomret=&retg;

    split_update_bounds(root, TRUE);

    if(OBJ_IS(sub, WSplitST)){
        WSplitST *sub_as_stdisp=(WSplitST*)sub;

        if(flags&REGION_RQGEOM_TRYONLY){
            warn(TR("REGION_RQGEOM_TRYONLY unsupported for status display."));
            *geomret=sub->geom;
            return;
        }
        split_regularise_stdisp(sub_as_stdisp);
        geom=sub->geom;
        if(sub_as_stdisp->orientation==REGION_ORIENTATION_HORIZONTAL){
            if(geom_->h==geom.h)
                return;
            geom.h=geom_->h;
        }else{
            if(geom_->w==geom.w)
                return;
            geom.w=geom_->w;
        }
        split_update_bounds(root, TRUE);
    }

    /* Handle internal size bounds */
    bnd(&(geom.x), &(geom.w), sub->geom.x, sub->geom.w,
        sub->min_w, sub->max_w);
    bnd(&(geom.y), &(geom.h), sub->geom.y, sub->geom.h,
        sub->min_h, sub->max_h);

    /* Check if we should resize to both tl and br */

    if(hany){
        geom.w+=sub->geom.x-geom.x;
        geom.x=sub->geom.x;
    }

    if(vany){
        geom.h+=sub->geom.y-geom.y;
        geom.y=sub->geom.y;
    }

    splittree_begin_resize();

    split_do_rqgeom_(sub, &geom, hany, vany, geomret, tryonly);

    if(!tryonly){
        split_do_resize(sub, geomret, hany, vany, FALSE);
        splittree_end_resize();
        *geomret=sub->geom;
    }else{
        saw_stdisp=NULL;
    }
}


/*EXTL_DOC
 * Attempt to resize and/or move the split tree starting at \var{node}.
 * Behaviour and the \var{g} parameter are as for \fnref{WRegion.rqgeom}
 * operating on \var{node} (if it were a \type{WRegion}).
 */
EXTL_EXPORT_MEMBER
ExtlTab split_rqgeom(WSplit *node, ExtlTab g)
{
    WRectangle geom, ogeom;
    int flags=REGION_RQGEOM_WEAK_ALL;

    geom=node->geom;
    ogeom=geom;

    if(extl_table_gets_i(g, "x", &(geom.x)))
        flags&=~REGION_RQGEOM_WEAK_X;
    if(extl_table_gets_i(g, "y", &(geom.y)))
        flags&=~REGION_RQGEOM_WEAK_Y;
    if(extl_table_gets_i(g, "w", &(geom.w)))
        flags&=~REGION_RQGEOM_WEAK_W;
    if(extl_table_gets_i(g, "h", &(geom.h)))
        flags&=~REGION_RQGEOM_WEAK_H;

    geom.w=MAXOF(1, geom.w);
    geom.h=MAXOF(1, geom.h);

    splittree_rqgeom(node, flags, &geom, &ogeom);

    return extl_table_from_rectangle(&ogeom);
}


/*}}}*/


/*{{{ Split */


void splittree_changeroot(WSplit *root, WSplit *node)
{
    WTiling *ws=(WTiling*)(root->ws_if_root);

    assert(ws!=NULL);
    assert(ws->split_tree==root);
    root->ws_if_root=NULL;
    ws->split_tree=node;
    if(node!=NULL){
        node->ws_if_root=ws;
        node->parent=NULL;
    }
}


static void splitsplit_replace(WSplitSplit *split, WSplit *child,
                               WSplit *what)
{
    assert(split->tl==child || split->br==child);

    if(split->tl==child)
        split->tl=what;
    else
        split->br=what;

    child->parent=NULL;

    what->parent=(WSplitInner*)split;
    what->ws_if_root=NULL; /* May not be needed. */
}


void splitinner_replace(WSplitInner *split, WSplit *child, WSplit *what)
{
    CALL_DYN(splitinner_replace, split, (split, child, what));
}


WSplitRegion *splittree_split(WSplit *node, int dir, WPrimn primn,
                              int minsize, WRegionSimpleCreateFn *fn,
                              WWindow *parent)
{
    int objmin;
    int s, sn, so;
    WSplitSplit *nsplit;
    WSplitRegion *nnode;
    WSplitInner *psplit;
    WRegion *nreg;
    WFitParams fp;
    WRectangle ng, rg;

    assert(node!=NULL && parent!=NULL);

    if(OBJ_IS(node, WSplitST)){
        warn(TR("Splitting the status display is not allowed."));
        return NULL;
    }

    splittree_begin_resize();

    if(!move_stdisp_out_of_way(node))
        return NULL;

    if(primn!=PRIMN_TL && primn!=PRIMN_BR)
        primn=PRIMN_BR;
    if(dir!=SPLIT_HORIZONTAL && dir!=SPLIT_VERTICAL)
        dir=SPLIT_VERTICAL;

    split_update_bounds(split_find_root(node), TRUE);
    objmin=(dir==SPLIT_VERTICAL ? node->min_h : node->min_w);

    s=split_size(node, dir);
    sn=MAXOF(minsize, s/2);
    so=MAXOF(objmin, s-sn);

    if(sn+so!=s){
        int rs;
        ng=node->geom;
        if(dir==SPLIT_VERTICAL)
            ng.h=sn+so;
        else
            ng.w=sn+so;
        split_do_rqgeom_(node, &ng, TRUE, TRUE, &rg, TRUE);
        rs=(dir==SPLIT_VERTICAL ? rg.h : rg.w);
        if(rs<minsize+objmin){
            warn(TR("Unable to split: not enough free space."));
            return NULL;
        }
        split_do_rqgeom_(node, &ng, TRUE, TRUE, &rg, FALSE);
        rs=(dir==SPLIT_VERTICAL ? rg.h : rg.w);
        if(minsize>rs/2){
            sn=minsize;
            so=rs-sn;
        }else{
            so=MAXOF(rs/2, objmin);
            sn=rs-so;
        }
    }else{
        rg=node->geom;
        splittree_scan_stdisp_rootward(node);
    }

    /* Create split and new window
     */
    fp.mode=REGION_FIT_EXACT;
    fp.g=rg;

    nsplit=create_splitsplit(&(fp.g), dir);

    if(nsplit==NULL)
        return NULL;

    if(dir==SPLIT_VERTICAL){
        if(primn==PRIMN_BR)
            fp.g.y+=so;
        fp.g.h=sn;
    }else{
        if(primn==PRIMN_BR)
            fp.g.x+=so;
        fp.g.w=sn;
    }

    nreg=fn(parent, &fp);

    if(nreg==NULL){
        destroy_obj((Obj*)nsplit);
        return NULL;
    }

    nnode=create_splitregion(&(fp.g), nreg);
    if(nnode==NULL){
        destroy_obj((Obj*)nreg);
        destroy_obj((Obj*)nsplit);
        return NULL;
    }

    /* Now that everything's ok, resize and move original node.
     */
    ng=rg;
    if(dir==SPLIT_VERTICAL){
        ng.h=so;
        if(primn==PRIMN_TL)
            ng.y+=sn;
    }else{
        ng.w=so;
        if(primn==PRIMN_TL)
            ng.x+=sn;
    }

    split_do_resize(node, &ng,
                    (dir==SPLIT_HORIZONTAL ? primn : PRIMN_ANY),
                    (dir==SPLIT_VERTICAL ? primn : PRIMN_ANY),
                    FALSE);

    /* Set up split structure
     */
    psplit=node->parent;

    if(psplit!=NULL)
        splitinner_replace(psplit, node, (WSplit*)nsplit);
    else
        splittree_changeroot(node, (WSplit*)nsplit);

    node->parent=(WSplitInner*)nsplit;
    ((WSplit*)nnode)->parent=(WSplitInner*)nsplit;

    if(primn==PRIMN_BR){
        nsplit->tl=node;
        nsplit->br=(WSplit*)nnode;
        nsplit->current=SPLIT_CURRENT_TL;
    }else{
        nsplit->tl=(WSplit*)nnode;
        nsplit->br=node;
        nsplit->current=SPLIT_CURRENT_BR;
    }

    splittree_end_resize();

    return nnode;
}


/*}}}*/


/*{{{ Remove */


static void splitsplit_remove(WSplitSplit *node, WSplit *child,
                              bool reclaim_space)
{
    static int nstdisp=0;
    WSplitInner *parent;
    WSplit *other;

    assert(node->tl==child || node->br==child);

    if(node->tl==child)
        other=node->br;
    else
        other=node->tl;

    assert(other!=NULL);

    if(nstdisp==0 && reclaim_space && OBJ_IS(other, WSplitST)){
        /* Try to move stdisp out of the way. */
        split_try_unsink_stdisp(node, FALSE, TRUE);
        assert(child->parent!=NULL);
        nstdisp++;
        splitinner_remove(child->parent, child, reclaim_space);
        nstdisp--;
        return;
    }

    parent=((WSplit*)node)->parent;

    if(parent!=NULL)
        splitinner_replace(parent, (WSplit*)node, other);
    else
        splittree_changeroot((WSplit*)node, other);

    if(reclaim_space)
        split_resize(other, &(((WSplit*)node)->geom), PRIMN_ANY, PRIMN_ANY);

    child->parent=NULL;

    node->tl=NULL;
    node->br=NULL;
    ((WSplit*)node)->parent=NULL;
    destroy_obj((Obj*)node);
}


void splitinner_remove(WSplitInner *node, WSplit *child, bool reclaim_space)
{
    CALL_DYN(splitinner_remove, node, (node, child, reclaim_space));
}


void splittree_remove(WSplit *node, bool reclaim_space)
{
    if(node->parent!=NULL)
        splitinner_remove(node->parent, node, reclaim_space);
    else if(node->ws_if_root!=NULL)
        splittree_changeroot(node, NULL);

    destroy_obj((Obj*)node);
}


/*}}}*/


/*{{{ Tree traversal */


static bool defaultfilter(WSplit *node)
{
    return (OBJ_IS(node, WSplitRegion) &&
            ((WSplitRegion*)node)->reg!=NULL);
}


static WSplit *split_current_todir_default(WSplit *node,
                                           WPrimn UNUSED(hprimn), WPrimn UNUSED(vprimn),
                                           WSplitFilter *filter)
{
    if(filter==NULL)
        filter=defaultfilter;

    return (filter(node) ? node : NULL);
}


static WSplit *splitsplit_current_todir(WSplitSplit *node,
                                        WPrimn hprimn, WPrimn vprimn,
                                        WSplitFilter *filter)
{
    WPrimn primn=(node->dir==SPLIT_HORIZONTAL ? hprimn : vprimn);
    WSplit *first, *second, *ret;

    if(primn==PRIMN_TL ||
       (primn==PRIMN_ANY && node->current==SPLIT_CURRENT_TL)){
        first=node->tl;
        second=node->br;
    }else if(primn==PRIMN_BR ||
       (primn==PRIMN_ANY && node->current==SPLIT_CURRENT_BR)){
        first=node->br;
        second=node->tl;
    }else{
        return NULL;
    }

    ret=split_current_todir(first, hprimn, vprimn, filter);
    if(ret==NULL)
        ret=split_current_todir(second, hprimn, vprimn, filter);
    if(ret==NULL && filter!=NULL){
        if(filter((WSplit*)node))
            ret=(WSplit*)node;
    }

    return ret;
}


WSplit *split_current_todir(WSplit *node, WPrimn hprimn, WPrimn vprimn,
                            WSplitFilter *filter)
{
    WSplit *ret=NULL;
    CALL_DYN_RET(ret, WSplit*, split_current_todir, node,
                 (node, hprimn, vprimn, filter));
    return ret;
}


/* Note: both hprimn and vprimn are inverted when descending.  Therefore
 * one should be either PRIMN_NONE or PRIMN_ANY for sensible geometric
 * navigation. (Both are PRIMN_TL or PRIMN_BR for pseudo-linear
 * next/previous navigation.)
 */
WSplit *splitsplit_nextto(WSplitSplit *node, WSplit *child,
                          WPrimn hprimn, WPrimn vprimn,
                          WSplitFilter *filter)
{
    WPrimn primn=(node->dir==SPLIT_HORIZONTAL ? hprimn : vprimn);
    WSplit *split=NULL, *nnode=NULL;

    if(node->tl==child && (primn==PRIMN_BR || primn==PRIMN_ANY))
        split=node->br;
    else if(node->br==child && (primn==PRIMN_TL || primn==PRIMN_ANY))
        split=node->tl;

    if(split!=NULL){
        nnode=split_current_todir(split,
                                  primn_none2any(primn_invert(hprimn)),
                                  primn_none2any(primn_invert(vprimn)),
                                  filter);
    }

    if(nnode==NULL)
        nnode=split_nextto((WSplit*)node, hprimn, vprimn, filter);

    return nnode;
}


WSplit *splitinner_nextto(WSplitInner *node, WSplit *child,
                          WPrimn hprimn, WPrimn vprimn,
                          WSplitFilter *filter)
{
    WSplit *ret=NULL;
    CALL_DYN_RET(ret, WSplit*, splitinner_nextto, node,
                 (node, child, hprimn, vprimn, filter));
    return ret;
}


WSplit *split_nextto(WSplit *node, WPrimn hprimn, WPrimn vprimn,
                     WSplitFilter *filter)
{
    while(node->parent!=NULL){
        WSplit *ret=splitinner_nextto(node->parent, node,
                                      hprimn, vprimn, filter);
        if(ret!=NULL)
            return ret;
        node=(WSplit*)node->parent;
    }
    return NULL;
}


void splitinner_mark_current_default(WSplitInner *split, WSplit *UNUSED(child))
{
    if(((WSplit*)split)->parent!=NULL)
        splitinner_mark_current(((WSplit*)split)->parent, (WSplit*)split);
}


void splitsplit_mark_current(WSplitSplit *split, WSplit *child)
{
    assert(child==split->tl || child==split->br);

    split->current=(split->tl==child ? SPLIT_CURRENT_TL : SPLIT_CURRENT_BR);

    splitinner_mark_current_default(&(split->isplit), child);
}


void splitinner_mark_current(WSplitInner *split, WSplit *child)
{
    CALL_DYN(splitinner_mark_current, split, (split, child));
}


static void splitsplit_forall(WSplitSplit *node, WSplitFn *fn)
{
    fn(node->tl);
    fn(node->br);
}


void splitinner_forall(WSplitInner *node, WSplitFn *fn)
{
    CALL_DYN(splitinner_forall, node, (node, fn));
}


static WSplit *splitsplit_current(WSplitSplit *split)
{
    return (split->current==SPLIT_CURRENT_TL ? split->tl : split->br);
}


/*EXTL_DOC
 * Returns the most previously active child node of \var{split}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplit *splitinner_current(WSplitInner *node)
{
    WSplit *ret=NULL;
    CALL_DYN_RET(ret, WSplit*, splitinner_current, node, (node));
    return ret;
}


/*}}}*/


/*{{{ X window handling */


static void splitregion_stacking(WSplitRegion *split,
                                 Window *bottomret, Window *topret)
{
    *bottomret=None;
    *topret=None;
    if(split->reg!=NULL)
        region_stacking(split->reg, bottomret, topret);
}


void splitsplit_stacking(WSplitSplit *split,
                         Window *bottomret, Window *topret)
{
    Window tlb=None, tlt=None;
    Window brb=None, brt=None;

    split_stacking(split->tl, &tlb, &tlt);
    split_stacking(split->br, &brb, &brt);

    /* To make sure that this condition holds is left to the workspace
     * code to do after a split tree has been loaded or modified.
     */
    if(split->current==SPLIT_CURRENT_TL){
        *topret=(tlt!=None ? tlt : brt);
        *bottomret=(brb!=None ? brb : tlb);
    }else{
        *topret=(brt!=None ? brt : tlt);
        *bottomret=(tlb!=None ? tlb : brb);
    }
}

void split_stacking(WSplit *split, Window *bottomret, Window *topret)
{
    *bottomret=None;
    *topret=None;
    {
        CALL_DYN(split_stacking, split, (split, bottomret, topret));
    }
}


static void splitregion_restack(WSplitRegion *split, Window other, int mode)
{
    if(split->reg!=NULL)
        region_restack(split->reg, other, mode);
}

void splitsplit_restack(WSplitSplit *split, Window other, int mode)
{
    Window bottom=None, top=None;
    WSplit *first, *second;

    if(split->current==SPLIT_CURRENT_TL){
        first=split->br;
        second=split->tl;
    }else{
        first=split->tl;
        second=split->br;
    }

    split_restack(first, other, mode);
    split_stacking(first, &bottom, &top);
    if(top!=None){
        other=top;
        mode=Above;
    }
    split_restack(second, other, mode);
}

void split_restack(WSplit *split, Window other, int mode)
{
    CALL_DYN(split_restack, split, (split, other, mode));
}


static void splitregion_map(WSplitRegion *split)
{
    if(split->reg!=NULL)
        region_map(split->reg);
}

static void splitinner_map(WSplitInner *split)
{
    splitinner_forall(split, split_map);
}

void split_map(WSplit *split)
{
    CALL_DYN(split_map, split, (split));
}


static void splitregion_unmap(WSplitRegion *split)
{
    if(split->reg!=NULL)
        region_unmap(split->reg);
}

static void splitinner_unmap(WSplitInner *split)
{
    splitinner_forall(split, split_unmap);
}

void split_unmap(WSplit *split)
{
    CALL_DYN(split_unmap, split, (split));
}


static void splitregion_reparent(WSplitRegion *split, WWindow *wwin)
{
    if(split->reg!=NULL){
        WRectangle g=split->split.geom;
        region_reparent(split->reg, wwin, &g, REGION_FIT_EXACT);
    }
}


static void splitsplit_reparent(WSplitSplit *split, WWindow *wwin)
{
    if(split->current==SPLIT_CURRENT_TL){
        split_reparent(split->br, wwin);
        split_reparent(split->tl, wwin);
    }else{
        split_reparent(split->tl, wwin);
        split_reparent(split->br, wwin);
    }
}


void split_reparent(WSplit *split, WWindow *wwin)
{
    CALL_DYN(split_reparent, split, (split, wwin));
}


/*}}}*/


/*{{{ Transpose, flip, rotate */


void splitsplit_flip_default(WSplitSplit *split)
{
    WRectangle tlng, brng;
    WRectangle *sg=&((WSplit*)split)->geom;
    WSplit *tmp;

    assert(split->tl!=NULL && split->br!=NULL);

    split_update_bounds((WSplit*)split, TRUE);

    tlng=split->tl->geom;
    brng=split->br->geom;

    if(split->dir==SPLIT_HORIZONTAL){
        brng.x=sg->x;
        tlng.x=sg->x+sg->w-tlng.w;
    }else{
        brng.y=sg->y;
        tlng.y=sg->y+sg->h-tlng.h;
    }

    tmp=split->tl;
    split->tl=split->br;
    split->br=tmp;
    split->current=(split->current==SPLIT_CURRENT_TL
                    ? SPLIT_CURRENT_BR
                    : SPLIT_CURRENT_TL);

    split_do_resize(split->tl, &brng, PRIMN_ANY, PRIMN_ANY, FALSE);
    split_do_resize(split->br, &tlng, PRIMN_ANY, PRIMN_ANY, FALSE);
}


static void splitsplit_flip_(WSplitSplit *split)
{
    CALL_DYN(splitsplit_flip, split, (split));
}


/*EXTL_DOC
 * Flip contents of \var{node}.
 */
EXTL_EXPORT_MEMBER
void splitsplit_flip(WSplitSplit *split)
{
    splittree_begin_resize();

    if(!move_stdisp_out_of_way((WSplit*)split))
        return;

    splitsplit_flip_(split);

    splittree_end_resize();
}

typedef enum{
    FLIP_VERTICAL,
    FLIP_HORIZONTAL,
    FLIP_NONE,
    FLIP_ANY
} FlipDir;


static FlipDir flipdir=FLIP_VERTICAL;


static void do_flip(WSplit *split)
{
    WSplitSplit *ss=OBJ_CAST(split, WSplitSplit);

    if(ss!=NULL){
        if((flipdir==FLIP_ANY
            || (ss->dir==SPLIT_VERTICAL && flipdir==FLIP_VERTICAL)
            || (ss->dir==SPLIT_HORIZONTAL && flipdir==FLIP_HORIZONTAL))
           && !OBJ_IS(ss->tl, WSplitST)
           && !OBJ_IS(ss->br, WSplitST)){
            splitsplit_flip_(ss);
        }
    }

    if(OBJ_IS(ss, WSplitInner))
        splitinner_forall((WSplitInner*)ss, do_flip);
}


static void splittree_flip_dir(WSplit *splittree, FlipDir dir)
{
    /* todo stdisp outta way */
    if(OBJ_IS(splittree, WSplitInner)){
        flipdir=dir;
        splitinner_forall((WSplitInner*)splittree, do_flip);
    }
}


static bool split_fliptrans_to(WSplit *node, const WRectangle *geom,
                              bool trans, FlipDir flip)
{
    WRectangle rg;
    WSplit *node2;

    splittree_begin_resize();

    /* split_do_resize can do things right if 'node' has stdisp as child,
     * but otherwise transpose will put the stdisp in a bad split
     * configuration if it is contained within 'node', so we must
     * first move it and its fixed parent split below node. For correct
     * geometry calculation we move it immediately below node, and
     * resize stdisp's fixed parent node instead.
     */
    node2=move_stdisp_out_of_way(node);

    if(node2==NULL)
        return FALSE;

    split_update_bounds(node2, TRUE);

    split_do_rqgeom_(node2, geom, PRIMN_ANY, PRIMN_ANY, &rg, FALSE);

    split_do_resize(node2, &rg, PRIMN_ANY, PRIMN_ANY, trans);

    if(flip!=FLIP_NONE)
        splittree_flip_dir(node2, flip);

    splittree_end_resize();

    return TRUE;
}


bool split_transpose_to(WSplit *node, const WRectangle *geom)
{
    return split_fliptrans_to(node, geom, TRUE, FLIP_ANY);
}


/*EXTL_DOC
 * Transpose contents of \var{node}.
 */
EXTL_EXPORT_MEMBER
void split_transpose(WSplit *node)
{
    WRectangle g=node->geom;

    split_transpose_to(node, &g);
}


bool split_rotate_to(WSplit *node, const WRectangle *geom, int rotation)
{
    FlipDir flip=FLIP_NONE;
    bool trans=FALSE;

    if(rotation==SCREEN_ROTATION_90){
        flip=FLIP_HORIZONTAL;
        trans=TRUE;
    }else if(rotation==SCREEN_ROTATION_180){
        flip=FLIP_ANY;
    }else if(rotation==SCREEN_ROTATION_270){
        flip=FLIP_VERTICAL;
        trans=TRUE;
    }

    return split_fliptrans_to(node, geom, trans, flip);
}

/*}}}*/


/*{{{ Exports */


/*EXTL_DOC
 * Return parent split for \var{split}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplitInner *split_parent(WSplit *split)
{
    return split->parent;
}


/*EXTL_DOC
 * Returns the area of workspace used by the regions under \var{split}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab split_geom(WSplit *split)
{
    return extl_table_from_rectangle(&(split->geom));
}


/*EXTL_DOC
 * Returns the top or left child node of \var{split} depending
 * on the direction of the split.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplit *splitsplit_tl(WSplitSplit *split)
{
    return split->tl;
}


/*EXTL_DOC
 * Returns the bottom or right child node of \var{split} depending
 * on the direction of the split.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WSplit *splitsplit_br(WSplitSplit *split)
{
    return split->br;
}

/*EXTL_DOC
 * Returns the direction of \var{split}; either ''vertical'' or
 * ''horizontal''.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
const char *splitsplit_dir(WSplitSplit *split)
{
    return (split->dir==SPLIT_VERTICAL ? "vertical" : "horizontal");
}


/*EXTL_DOC
 * Returns the region contained in \var{node}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *splitregion_reg(WSplitRegion *node)
{
    return node->reg;
}


/*}}}*/


/*{{{ Save support */


ExtlTab split_base_config(WSplit *node)
{
    ExtlTab t=extl_create_table();
    extl_table_sets_s(t, "type", OBJ_TYPESTR(node));
    return t;
}


static bool splitregion_get_config(WSplitRegion *node, ExtlTab *ret)
{
    ExtlTab rt, t;

    if(node->reg==NULL)
        return FALSE;

    if(!region_supports_save(node->reg)){
        warn(TR("Unable to get configuration for %s."),
             region_name(node->reg));
        return FALSE;
    }

    rt=region_get_configuration(node->reg);
    t=split_base_config(&(node->split));
    extl_table_sets_t(t, "regparams", rt);
    extl_unref_table(rt);
    *ret=t;

    return TRUE;
}


static bool splitst_get_config(WSplitST *node, ExtlTab *ret)
{
    *ret=split_base_config((WSplit*)node);
    return TRUE;
}


static bool splitsplit_get_config(WSplitSplit *node, ExtlTab *ret)
{
    ExtlTab tab, tltab, brtab;
    int tls, brs;

    if(!split_get_config(node->tl, &tltab))
        return split_get_config(node->br, ret);

    if(!split_get_config(node->br, &brtab)){
        *ret=tltab;
        return TRUE;
    }

    tab=split_base_config((WSplit*)node);

    tls=split_size(node->tl, node->dir);
    brs=split_size(node->br, node->dir);

    extl_table_sets_s(tab, "dir", (node->dir==SPLIT_VERTICAL
                                   ? "vertical" : "horizontal"));

    extl_table_sets_i(tab, "tls", tls);
    extl_table_sets_t(tab, "tl", tltab);
    extl_unref_table(tltab);

    extl_table_sets_i(tab, "brs", brs);
    extl_table_sets_t(tab, "br", brtab);
    extl_unref_table(brtab);

    *ret=tab;

    return TRUE;
}


bool split_get_config(WSplit *node, ExtlTab *tabret)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, split_get_config, node, (node, tabret));
    return ret;
}


/*}}}*/


/*{{{ The classes */


static DynFunTab split_dynfuntab[]={
    {split_do_resize, split_do_resize_default},
    {(DynFun*)split_current_todir, (DynFun*)split_current_todir_default},
    END_DYNFUNTAB,
};

static DynFunTab splitinner_dynfuntab[]={
    {splitinner_mark_current, splitinner_mark_current_default},
    {split_map, splitinner_map},
    {split_unmap, splitinner_unmap},
    END_DYNFUNTAB,
};

static DynFunTab splitsplit_dynfuntab[]={
    {split_update_bounds, splitsplit_update_bounds},
    {split_do_resize, splitsplit_do_resize},
    {split_do_maxhelper, splitsplit_do_maxhelper},
    {(DynFun*)split_do_restore, (DynFun*)splitsplit_do_restore},
    {(DynFun*)split_do_verify, (DynFun*)splitsplit_do_verify},
    {splitinner_do_rqsize, splitsplit_do_rqsize},
    {splitinner_replace, splitsplit_replace},
    {splitinner_remove, splitsplit_remove},
    {(DynFun*)split_current_todir, (DynFun*)splitsplit_current_todir},
    {(DynFun*)splitinner_current, (DynFun*)splitsplit_current},
    {(DynFun*)splitinner_nextto, (DynFun*)splitsplit_nextto},
    {splitinner_mark_current, splitsplit_mark_current},
    {(DynFun*)split_get_config, (DynFun*)splitsplit_get_config},
    {splitinner_forall, splitsplit_forall},
    {split_restack, splitsplit_restack},
    {split_stacking, splitsplit_stacking},
    {split_reparent, splitsplit_reparent},
    {splitsplit_flip, splitsplit_flip_default},
    END_DYNFUNTAB,
};

static DynFunTab splitregion_dynfuntab[]={
    {split_update_bounds, splitregion_update_bounds},
    {split_do_resize, splitregion_do_resize},
    {split_do_maxhelper, splitregion_do_maxhelper},
    {(DynFun*)split_do_restore, (DynFun*)splitregion_do_restore},
    {(DynFun*)split_do_verify, (DynFun*)splitregion_do_verify},
    {(DynFun*)split_get_config, (DynFun*)splitregion_get_config},
    {split_map, splitregion_map},
    {split_unmap, splitregion_unmap},
    {split_restack, splitregion_restack},
    {split_stacking, splitregion_stacking},
    {split_reparent, splitregion_reparent},
    END_DYNFUNTAB,
};

static DynFunTab splitst_dynfuntab[]={
    {split_update_bounds, splitst_update_bounds},
    {split_do_resize, splitst_do_resize},
    {split_do_maxhelper, splitst_do_maxhelper},
    {(DynFun*)split_do_restore, (DynFun*)splitst_do_restore},
    {(DynFun*)split_do_verify, (DynFun*)splitst_do_verify},
    {(DynFun*)split_get_config, (DynFun*)splitst_get_config},
    END_DYNFUNTAB,
};


EXTL_EXPORT
IMPLCLASS(WSplit, Obj, split_deinit, split_dynfuntab);

EXTL_EXPORT
IMPLCLASS(WSplitInner, WSplit, splitinner_deinit, splitinner_dynfuntab);

EXTL_EXPORT
IMPLCLASS(WSplitSplit, WSplitInner, splitsplit_deinit, splitsplit_dynfuntab);

EXTL_EXPORT
IMPLCLASS(WSplitRegion, WSplit, splitregion_deinit, splitregion_dynfuntab);

EXTL_EXPORT
IMPLCLASS(WSplitST, WSplitRegion, splitst_deinit, splitst_dynfuntab);


/*}}}*/

