/*
 * ion/ioncore/targetid.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "targetid.h"
#include "region.h"
#include "clientwin.h"
#include "objp.h"


/* Yeah, yeah, fixed-size arrays are evil, but if the user really
 * has 1024^2 targets for client windows, much more serious usability
 * problems than lack of target id:s must have occured long before that.
 * It wouldn't be too hard to change this, though...
 */
#define N_TABS		256
#define TAB_SIZE	(1024*4)
#define MAX_ID		(N_TABS*TAB_SIZE-1)

static WRegion **tabs[N_TABS]={NULL, };
static int n_targets[N_TABS]={0, };
static int last_target[N_TABS]={0, };


/*{{{ Reorder */


static void change_id(WRegion *reg, int id)
{
	WClientWin *cwin;

	for(cwin=FIRST_THING(reg, WClientWin);
		cwin!=NULL;
		cwin=NEXT_THING(cwin, WClientWin)){
		clientwin_set_target_id(cwin, id);
	}
}


static void reorder_tab(int i)
{
	int tail=0, head;
	WRegion **t=tabs[i];
	
	while(t[tail]!=NULL && tail<TAB_SIZE)
		tail++;
	
	for(head=tail+1; head<TAB_SIZE; head++){
		if(t[head]==NULL)
			continue;
		change_id(t[head], tail);
		t[tail]=t[head];
		t[head]=NULL;
		tail++;
	}
	
	last_target[i]=tail;
}


/*}}}*/


/*{{{ Alloc, free */


int alloc_target_id(WRegion *reg)
{	
	static WRegion **tmp;
	static int i, j=0;
	
	/* First table is reserved */
	for(i=1; i<N_TABS; i++){
		if(tabs[i]==NULL){
			if(j==0)
				j=i;
			continue;
		}
		
		if(n_targets[i]==TAB_SIZE)
			continue;
		
		if(last_target[i]==TAB_SIZE)
			reorder_tab(i);

		tabs[i][last_target[i]]=reg;
		n_targets[i]++;
		last_target[i]++;
		return TAB_SIZE*i+last_target[i]-1;
	}

	if(j!=0)
		tabs[j]=ALLOC_N(WRegion*, TAB_SIZE);
	
	if(j==0 || tabs[j]==NULL){
		warn("Unable to allocate target ID");
		return 0;
	}
	
	tabs[j][0]=reg;
	n_targets[j]=1;
	last_target[j]=1;
	
	return TAB_SIZE*j;
}


int use_target_id(WRegion *reg, int id)
{
	int tab, ndx;
	
	if(id<=0 || id>MAX_ID)
		return alloc_target_id(reg);
	
	tab=id/TAB_SIZE;
	ndx=id%TAB_SIZE;
	
	if(tabs[tab]==NULL)
		tabs[tab]=ALLOC_N(WRegion*, TAB_SIZE);
	
	if(tabs[tab]==NULL || tabs[tab][ndx]!=NULL){
		warn("Unable to allocate requested target ID %d; "
			 "allocating other.", id);
		return alloc_target_id(reg);
	}
	
	tabs[tab][ndx]=reg;
	n_targets[tab]++;
	if(last_target[tab]<=ndx)
		last_target[tab]=ndx+1;
	
	return id;
}
	   
	   
void free_target_id(int id)
{
	int tab, ndx;

	if(id<=0 || id>MAX_ID)
		return;
	
	tab=id/TAB_SIZE;
	ndx=id%TAB_SIZE;
	
	if(tabs[tab]==NULL)
		return;
	
	tabs[tab][ndx]=NULL;
	n_targets[tab]--;
	
	if(n_targets[tab]==0){
		free(tabs[tab]);
		tabs[tab]=NULL;
		last_target[tab]=0;
	}else if(last_target[tab]==ndx+1){
		last_target[tab]=ndx;
	}
}


/*}}}*/


/*{{{ Find */


WRegion *find_target_by_id(int id)
{
	int tab, ndx;

	if(id<=0 || id>MAX_ID)
		return NULL;
	
	tab=id/TAB_SIZE;
	ndx=id%TAB_SIZE;
	
	if(tabs[tab]==NULL)
		return NULL;
	
	return tabs[tab][ndx];
}


/*}}}*/


/*{{{ Set */


void set_target_id(WRegion *reg, int id)
{
	if(WTHING_IS(reg, WClientWin))
		clientwin_set_target_id((WClientWin*)reg, id);
}


/*}}}*/


