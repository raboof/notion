/*
 * ion/regbind.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include "common.h"
#include "region.h"
#include "binding.h"
#include "regbind.h"
#include "objp.h"


INTRSTRUCT(RBind)
INTRSTRUCT(BCount)	
	

DECLSTRUCT(RBind){
	WBindmap *bindmap;
	bool grab;
	RBind *next, *prev;
};


DECLSTRUCT(BCount){
	uint key;
	uint state;
	uint count;
};


/*{{{ Binding-level management */


#define CVAL(A, B, V) ( A->V < B->V ? -1 : (A->V > B->V ? 1 : 0))

static int compare_bcount(const void *a_, const void *b_)
{
	const BCount *a=(BCount*)a_, *b=(BCount*)b_;
	int r;
	
	r=CVAL(a, b, key);
	if(r==0)
		r=CVAL(a, b, state);
	
	return r;
}

#undef CVAL


static bool inc_bcount(WScreen *scr, uint key, uint state)
{
	BCount dummy;
	BCount *bc, *bcount;
	int i;
	
	bcount=(BCount*)(scr->bcount);
	
	dummy.key=key;
	dummy.state=state;
	dummy.count=1;
	
	/* As the bindings usually should already be grabbed, it might
	 * be best to do a bsearch() first instead of simultaneously
	 * looking for the place for new entry?
	 */
	for(i=0; i<scr->n_bcount; i++){
		switch(compare_bcount((void*)&dummy, (void*)(bcount+i))){
		case 1:
			continue;
		case 0:
			bcount[i].count++;
			/*fprintf(stderr, "inc-found: %d/%x [%d,%d] | %d\n", key, state, bcount[i].count, n_bcount, bcount[i].key);*/
			return FALSE;
		}
		break;
	}

	bc=REALLOC_N(bcount, BCount, scr->n_bcount, scr->n_bcount+1);
	
	if(bc==NULL){
		warn_err();
		return FALSE;
	}
	
	memmove(bc+i+1, bc+i, sizeof(BCount)*(scr->n_bcount-i));
	memcpy(bc+i, &dummy, sizeof(BCount));
	scr->bcount=(void*)bc;
	scr->n_bcount++;
	
	return TRUE;
}


static bool dec_bcount(WScreen *scr, uint key, uint state)
{
	BCount dummy;
	BCount *bc, *bcount;
	int i;
	
	bcount=(BCount*)(scr->bcount);
	
	dummy.key=key;
	dummy.state=state;
	dummy.count=1;
	
	bc=bsearch((void*)&dummy, (void*)(bcount), scr->n_bcount, sizeof(BCount),
			   compare_bcount);
	
	if(bc==NULL){
		warn("dec_bcount: key/state not found!");
		return TRUE;
	}

	i=bc-bcount;

	/*fprintf(stderr, "dec-found: %d/%x [%d,%d]", key, state, bcount[i].count-1, n_bcount);*/

	if(--bcount[i].count>0)
		return FALSE;

	memmove(bcount+i, bcount+i+1, sizeof(BCount)*(scr->n_bcount-i-1));
	
	bc=REALLOC_N(bcount, BCount, scr->n_bcount, scr->n_bcount-1);

	scr->n_bcount--;
	
	if(bc==NULL)
		warn_err();
	else
		scr->bcount=(void*)bc;
	
	return TRUE;
}


static void do_grab_new(WScreen *scr, WBindmap *bindmap)
{
	WBinding *binding=bindmap->bindings;
	int i;
	
	for(i=0; i<bindmap->nbindings; i++, binding++){
		if(binding->act!=ACT_KEYPRESS)
			continue;
		
		/* add to table, grab if ungrab */
		if(inc_bcount(scr, binding->kcb, binding->state)){
			/*fprintf(stderr, "Grab: %d/%x [%d]\n", binding->kcb, binding->state, n_bcount);*/
			grab_binding(binding, scr->root.win);
		}
	}
}


static void do_ungrab_old(WScreen *scr, WBindmap *bindmap)
{
	WBinding *binding=bindmap->bindings;
	int i;
	
	for(i=0; i<bindmap->nbindings; i++, binding++){
		if(binding->act!=ACT_KEYPRESS)
			continue;
		
		/* remove from table, ungrab if unused */
		if(dec_bcount(scr, binding->kcb, binding->state)){
			/*fprintf(stderr, "Ungrab: %d/%x [%d]\n", binding->kcb, binding->state, n_bcount);*/
			ungrab_binding(binding, scr->root.win);
		}
	}
}


/*}}}*/


/*{{{ Buttons */


static bool is_button_act(int act)
{
	return (act==ACT_BUTTONPRESS ||
			act==ACT_BUTTONCLICK ||
			act==ACT_BUTTONDBLCLICK ||
			act==ACT_BUTTONMOTION);
}


static void grab_buttons(WRegion *reg, WBindmap *bindmap)
{
	WBinding *binding=bindmap->bindings;
	Window win;
	int i;
	
	if(!WTHING_IS(reg, WWindow))
		return;
	
	win=((WWindow*)reg)->win;
	
	if(win==None)
		return;
	
	for(i=0; i<bindmap->nbindings; i++, binding++){
		if(!is_button_act(binding->act))
			continue;
		grab_binding(binding, win);
	}
}


static void ungrab_buttons(WRegion *reg, WBindmap *bindmap)
{
	WBinding *binding=bindmap->bindings;
	Window win;
	int i;
	
	if(!WTHING_IS(reg, WWindow))
		return;
	
	win=((WWindow*)reg)->win;
	
	if(win==None)
		return;
	
	for(i=0; i<bindmap->nbindings; i++, binding++){
		if(!is_button_act(binding->act))
			continue;
		ungrab_binding(binding, win);
	}
}


/*}}}*/


/*{{{ RBind-level management */


static void do_activate_ggrab(WScreen *scr, RBind *rbind)
{
	if(rbind->bindmap->ggrab_cntr==0)
		do_grab_new(scr, rbind->bindmap);
	rbind->bindmap->ggrab_cntr++;
}


static void do_release_ggrab(WScreen *scr, RBind *rbind)
{
	rbind->bindmap->ggrab_cntr--;
	if(rbind->bindmap->ggrab_cntr==0)
		do_ungrab_old(scr, rbind->bindmap);
}


void release_ggrabs(WRegion *reg)
{
	RBind *rbind;
	
	if(!(reg->flags&REGION_HAS_GRABS))
		return;
	
	for(rbind=(RBind*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		if(rbind->grab)
			do_release_ggrab(SCREEN_OF(reg), rbind);
	}
	
	reg->flags&=~REGION_HAS_GRABS;
}


void activate_ggrabs(WRegion *reg)
{
	RBind *rbind;

	if(reg->flags&REGION_HAS_GRABS)
		return;
	
	for(rbind=(RBind*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		if(rbind->grab)
			do_activate_ggrab(SCREEN_OF(reg), rbind);
	}

	reg->flags|=REGION_HAS_GRABS;
}


static RBind *find_rbind(WRegion *reg, WBindmap *bindmap)
{
	RBind *rbind;
	
	for(rbind=(RBind*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		if(rbind->bindmap==bindmap)
			return rbind;
	}
	
	return NULL;
}


static void remove_rbind(WRegion *reg, RBind *rbind)
{
	if(rbind->grab){
		ungrab_buttons(reg, rbind->bindmap);
		if(reg->flags&REGION_HAS_GRABS)
			do_release_ggrab(SCREEN_OF(reg), rbind);
	}
	
	{
		RBind *b=reg->bindings;
		UNLINK_ITEM(b, rbind, next, prev);
		reg->bindings=b;
	}
	
	free(rbind);
}


/*}}}*/


/*{{{ Interface */


bool region_add_bindmap(WRegion *reg, WBindmap *bindmap, bool grab)
{
	RBind *rbind;
	
	if(bindmap==NULL)
		return FALSE;
	
	if(find_rbind(reg, bindmap)!=NULL)
		return FALSE;
	
	rbind=ALLOC(RBind);
	
	if(rbind==NULL){
		warn_err();
		return FALSE;
	}
	
	rbind->bindmap=bindmap;
	rbind->grab=grab;
	
	/* ??? */
	if(grab)
		grab_buttons(reg, bindmap);
	
	{
		RBind *b=reg->bindings;
		LINK_ITEM(b, rbind, next, prev);
		reg->bindings=b;
	}
	
	if(grab && REGION_IS_ACTIVE(reg))
		do_activate_ggrab(SCREEN_OF(reg), rbind);
	
	return TRUE;
}


void region_remove_bindmap(WRegion *reg, WBindmap *bindmap)
{
	RBind *rbind=find_rbind(reg, bindmap);
	
	if(rbind!=NULL)
		remove_rbind(reg, rbind);
}


void region_remove_bindings(WRegion *reg)
{
	RBind *rbind;
	
	while((rbind=(RBind*)reg->bindings)!=NULL)
		remove_rbind(reg, rbind);
}


WBinding *region_lookup_keybinding(const WRegion *reg, const XKeyEvent *ev,
								   bool grabonly, const WSubmapState *sc)
{
	RBind *rbind;
	WBinding *binding=NULL;
	const WSubmapState *s=NULL;
	WBindmap *bindmap=NULL;
	
	for(rbind=(RBind*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		if(!IS_SCREEN(reg) && grabonly && !rbind->grab)
			continue;
		
		bindmap=rbind->bindmap;
		
		for(s=sc; s!=NULL && bindmap!=NULL; s=s->next){
			binding=lookup_binding(bindmap, ACT_KEYPRESS,
								   s->state, s->key);
			/* The case binding->submap==NULL shouldn't happen unless
			 * maps were changed. We'll just ignore that situation.
			 */
			if(binding==NULL)
				break;
			
			bindmap=binding->submap;
		}
		
		if(bindmap==NULL)
			continue;
		
		binding=lookup_binding(bindmap, ACT_KEYPRESS, ev->state, ev->keycode);
		
		if(binding!=NULL)
			break;
	}
	
	return binding;
}


WBinding *region_lookup_binding_area(const WRegion *reg, int act,
									 uint state, uint kcb, int area)
{
	RBind *rbind;
	WBinding *binding=NULL;
	
	for(rbind=(RBind*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		binding=lookup_binding_area(rbind->bindmap, act, state, kcb, area);
		if(binding!=NULL)
			break;
	}
	
	return binding;
}


/*}}}*/

