/*
 * ion/query/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <limits.h>
#include <unistd.h>
#include <string.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/exec.h>
#include <ioncore/clientwin.h>
#include <ioncore/focus.h>
#include <ioncore/names.h>
#include <ioncore/genws.h>
#include <ioncore/genframe.h>
#include <ioncore/reginfo.h>
#include <ioncore/extl.h>
#include "query.h"
#include "wedln.h"
#include "wmessage.h"
#include "fwarn.h"


/*{{{ Generic */


EXTL_EXPORT
void query_query(WGenFrame *frame, const char *prompt, const char *dflt,
				 ExtlFn handler, ExtlFn completor)
{
	WRectangle geom;
	WEdln *wedln;
	WEdlnCreateParams fnp;
	
	/*fprintf(stderr, "query_query called %s %s %d %d\n", prompt, dflt,
			handler, completor);*/
	
	fnp.prompt=prompt;
	fnp.dflt=dflt;
	fnp.handler=handler;
	fnp.completor=completor;
	
	wedln=(WEdln*)genframe_add_input(frame, (WRegionAddFn*)create_wedln,
									 (void*)&fnp);
	if(wedln!=NULL){
		region_map((WRegion*)wedln);
		
		if(REGION_IS_ACTIVE(frame))
			set_focus((WRegion*)wedln);
	}
}


/*}}}*/


/*{{{ Navigation */


EXTL_EXPORT
void query_handler_attachclient(WGenFrame *frame, const char *str)
{
	WClientWin *cwin=lookup_clientwin(str);
	
	if(str==NULL)
		return;
	
	if(cwin==NULL){
		query_fwarn_free(frame, errmsg("No client named '%s'", str));
		return;
	}

	if(!same_screen((WRegion*)cwin, (WRegion*)frame)){
		/* complaint should go in 'obj' -frame */
		query_fwarn(frame, "Cannot attach: not on same screen.");
		return;
	}
	
	region_add_managed_simple((WRegion*)frame, (WRegion*)cwin,
							  REGION_ATTACH_SWITCHTO);
}


static WRegion *create_ws_on_vp(WViewport *vp, const char *name)
{
	WRegionSimpleCreateFn *fn=NULL;
	char *p=strchr(name, ':');
	WRegion *ws;
	
	if(p!=NULL){
		char *s=ALLOC_N(char, p-name+1);
		if(s==NULL){
			warn_err();
		}else{
			strncpy(s, name, p-name);
			name=p+1;
			fn=lookup_region_simple_create_fn(s);
			free(s);
		}
	}else{
		fn=lookup_region_simple_create_fn_inh("WGenWS");
	}
	
	if(fn==NULL)
		return NULL;
	
	ws=region_add_managed_new_simple((WRegion*)vp, fn,
									 REGION_ATTACH_SWITCHTO);
	if(ws!=NULL)
		region_set_name(ws, name);
	
	return ws;
}


EXTL_EXPORT
void query_handler_workspace(WGenFrame *frame, const char *name)
{
	WScreen *scr=SCREEN_OF(frame);
	WRegion *ws=NULL;
	WViewport *vp=NULL;
	
	if(name==NULL || strspn(name, " \t")==strlen(name))
		return;
	
	ws=(WRegion*)lookup_workspace(name);
	
	if(ws==NULL){
		vp=viewport_of((WRegion*)frame);
		if(vp!=NULL)
			ws=create_ws_on_vp(vp, name);
		if(ws==NULL){
			query_fwarn(frame, "Unable to create workspace.");
			return;
		}
	}
	
	region_goto(ws);
}


#if 0
static void handler_workspace_with(WObj *obj, char *name, char *userdata)
{
	WScreen *scr=SCREEN_OF(obj);
	WGenWS *ws;
	WClientWin *cwin;
	WGenFrame *frame;
	WViewport *vp;
	
	if(empty_name(name))
		return;
	
	ws=lookup_workspace(name);
	cwin=lookup_clientwin(userdata);
	
	if(ws!=NULL){
		frame=(WGenFrame*)workspace_find_current(ws);
		if(frame==NULL || !WOBJ_IS(frame, WGenFrame)){
			FWARN(("Workspace %s has no current frame", name));
			return;
		}
	}else{
		if(cwin==NULL){
			FWARN(("Client disappeared"));
			return;
		}
		
		vp=viewport_of((WRegion*)obj);
		if(vp!=NULL)
			ws=create_new_ionws_on_vp(vp, name);
		
		if(ws==NULL){
			FWARN(("Unable to create workspace."));
			return;
		}
		
		frame=FIRST_CHILD(ws, WGenFrame);
		
		assert(frame!=NULL);
	}
	
	if(attach_test((WGenFrame*)frame, (WRegion*)cwin, (WGenFrame*)obj))
		region_goto((WRegion*)cwin);
}
#endif


/*}}}*/


