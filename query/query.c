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
#include "complete_file.h"
#include "wmessage.h"
#include "fwarn.h"

#define FWARN(ARGS) fwarn_free((WGenFrame*)obj, errmsg ARGS)


/*{{{ Generic */


static WEdln *do_query_edln(WGenFrame *frame, WEdlnHandler *handler,
							const char *prompt, const char *dflt,
							EdlnCompletionHandler *chandler,
							void *chandler_data)
{
	WRectangle geom;
	WEdln *wedln;
	WEdlnCreateParams fnp;
	
	fnp.prompt=prompt;
	fnp.dflt=dflt;
	fnp.handler=handler;
	
	wedln=(WEdln*)genframe_add_input(frame, (WRegionAddFn*)create_wedln,
									 (void*)&fnp);
	if(wedln!=NULL){
		map_region((WRegion*)wedln);
		
		if(REGION_IS_ACTIVE(frame))
			set_focus((WRegion*)wedln);
		
		if(chandler!=NULL)
			wedln_set_completion_handler(wedln, chandler, chandler_data);
	}
	
	return wedln;
}


/*}}}*/


/*{{{ Files */


static char wdbuf[PATH_MAX+10]="/";
static int wdstatus=0;

static const char *my_getwd()
{
	
	if(wdstatus==0){
		if(getcwd(wdbuf, PATH_MAX)==NULL){
			wdstatus=-1;
			strcpy(wdbuf, "/");
		}else{
			wdstatus=1;
			strcat(wdbuf, "/");
		}
	}
	
	return wdbuf;
}


static void handler_runfile(WObj *obj, char *str, char *userdata)
{
	char *p;
	
	if(*str=='\0')
		return;
	
	if(userdata!=NULL)
		do_open_with(SCREEN_OF(obj), userdata, str);
	
	p=strrchr(str, '/');
	if(p==NULL){
		wdstatus=0;
	}else{
		*(p+1)='\0';
		strncpy(wdbuf, str, PATH_MAX);
	}
}


static void handler_runwith(WObj *obj, char *str, char *userdata)
{
	WScreen *scr=SCREEN_OF(obj);
	
	if(userdata==NULL)
		return;
	
	if(*str!='\0')
		do_open_with(scr, userdata, str);
	else
		ioncore_exec(scr, userdata);
}


static void handler_exec(WObj *obj, char *str, char *userdata)
{
	WScreen *scr=SCREEN_OF(obj);
	
	if(*str==':')
		do_open_with(scr, "ion-runinxterm", str+1);
	else
		ioncore_exec(scr, str);
}


EXTL_EXPORT
void query_exec(WGenFrame *frame)
{
	do_query_edln(frame, handler_exec, "Run:", NULL,
				  complete_file_with_path, NULL);
}


EXTL_EXPORT
void query_runfile(WGenFrame *frame, char *prompt, char *cmd)
{
	WEdln *wedln=do_query_edln(frame, handler_runfile,
							   prompt, my_getwd(), complete_file, NULL);
	if(wedln!=NULL)
		wedln->userdata=scopy(cmd);
}


EXTL_EXPORT
void query_runwith(WGenFrame *frame, char *prompt, char *cmd)
{
	WEdln *wedln=do_query_edln(frame, handler_runwith,
							   prompt, NULL, NULL, NULL);
	if(wedln!=NULL)
		wedln->userdata=scopy(cmd);
}


/*}}}*/


/*{{{ Navigation */


static bool attach_test(WGenFrame *dst, WRegion *sub, WGenFrame *obj)
{
	if(!same_screen(&dst->win.region, sub)){
		/* complaint should go in 'obj' -frame */
		FWARN(("Cannot attach: not on same screen."));
		return FALSE;
	}
	region_add_managed((WRegion*)dst, sub, TRUE);
	return TRUE;
}


static void handler_attachclient(WObj *obj, char *str, char *userdata)
{
	WClientWin *cwin=lookup_clientwin(str);
	
	if(cwin==NULL){
		FWARN(("No client named '%s'", str));
		return;
	}
	
	attach_test((WGenFrame*)obj, (WRegion*)cwin, (WGenFrame*)obj);
}


static void handler_gotoclient(WObj *obj, char *str, char *userdata)
{
	WClientWin *cwin=lookup_clientwin(str);
	
	if(cwin==NULL){
		FWARN(("No client named '%s'", str));
		return;
	}
	
	goto_region(&cwin->region);
}


EXTL_EXPORT
void query_attachclient(WGenFrame *frame)
{
	do_query_edln(frame, handler_attachclient,
				  "Attach client:", "", complete_clientwin, NULL);
}


EXTL_EXPORT
void query_gotoclient(WGenFrame *frame)
{
	do_query_edln(frame, handler_gotoclient,
				  "Goto client:", "", complete_clientwin, NULL);
}


bool empty_name(const char *p)
{
	return (strspn(p, " \t")==strlen(p));
}


static WRegion *create_ws_on_vp(WViewport *vp, char *name)
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


static void handler_workspace(WObj *obj, char *name, char *userdata)
{
	WScreen *scr=SCREEN_OF(obj);
	WRegion *ws=NULL;
	WViewport *vp=NULL;
	
	if(empty_name(name))
		return;
	
	ws=(WRegion*)lookup_workspace(name);
	
	if(ws==NULL){
		vp=viewport_of((WRegion*)obj);
		if(vp!=NULL)
			ws=create_ws_on_vp(vp, name);
		if(ws==NULL){
			FWARN(("Unable to create workspace."));
			return;
		}
	}
	
	goto_region(ws);
}


EXTL_EXPORT
void query_workspace(WGenFrame *frame)
{
	do_query_edln(frame, handler_workspace,
				  "Goto/create workspace:", "", complete_workspace, NULL);
}


static void handler_workspace_with(WObj *obj, char *name, char *userdata)
{
#if 0
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
		goto_region((WRegion*)cwin);
#endif
}


EXTL_EXPORT
void query_workspace_with(WGenFrame *frame)
{
#if 0
	WEdln *wedln;
	WRegion *sub=frame->current_sub;
	char *p;
	
	if(sub==NULL){
		query_workspace(frame);
		return;
	}
	
	p=region_full_name(sub);
	
	wedln=do_query_edln(frame, handler_workspace_with,
						"Create workspace/attach:", p,
						complete_workspace, NULL);
	if(wedln==NULL)
		free(p);
	else
		wedln->userdata=p;
#endif
	fwarn(frame, "This feature is not available at the moment");
}


void handler_renameworkspace(WObj *obj, char *name, char *userdata)
{
	WGenWS *ws=FIND_PARENT((WRegion*)obj, WGenWS);
	
	if(ws==NULL || empty_name(name))
		return;
	
	region_set_name((WRegion*)ws, name);
}


EXTL_EXPORT
void query_renameworkspace(WGenFrame *frame)
{
	WGenWS *ws=FIND_PARENT(frame, WGenWS);
	
	if(ws==NULL)
		return;
	
	do_query_edln(frame, handler_renameworkspace,
				  "Rename workspace to:", region_name((WRegion*)ws),
				  NULL, NULL);
}


void handler_renameframe(WObj *obj, char *name, char *userdata)
{
	region_set_name((WRegion*)obj, name);
}


EXTL_EXPORT
void query_renameframe(WGenFrame *frame)
{
	do_query_edln(frame, handler_renameframe,
				  "Name of this frame:", region_name((WRegion*)frame),
				  NULL, NULL);
}


/*}}}*/


/*{{{ Misc. */


static char *last_error_message=NULL;


static void function_warn_handler(const char *message)
{
	if(last_error_message!=NULL)
		free(last_error_message);
	last_error_message=scopy(message);
}


void handler_commands(WObj *obj, char *cmds, char *userdata)
{
	WarnHandler *old_warn_handler;
	WWatch watch=WWATCH_INIT;
	bool error=FALSE;
	
	assert(WOBJ_IS(obj, WGenFrame));
	
	setup_watch(&watch, obj, NULL);
	
	if(((WGenFrame*)obj)->current_sub!=NULL)
		obj=(WObj*)(((WGenFrame*)obj)->current_sub);
	
	old_warn_handler=set_warn_handler(function_warn_handler);
	/*error=extl_do_string(cmds, "o", obj);*/
	set_warn_handler(old_warn_handler);
	
	if(watch.obj!=NULL){
		obj=watch.obj;
		if(last_error_message!=NULL){
			FWARN(("%s", last_error_message));
		}else if(error){
			FWARN(("An unknown error occurred while trying to "
				   "parse your request"));
		}
	}
	
	reset_watch(&watch);
	
	if(last_error_message!=NULL){
		free(last_error_message);
		last_error_message=NULL;
	}
}


static void handler_yesno(WObj *obj, char *yesno, char *fnc)
{
	ExtlFn fn;
	
	if(strcasecmp(yesno, "y") && strcasecmp(yesno, "yes"))
		return;
	
	if(fnc==NULL)
		return;
	
	fn=*(ExtlFn*)fnc;
	free(fnc);
	
	extl_call(fn, "o", NULL, obj);
}


EXTL_EXPORT
void query_yesno(WGenFrame *frame, const char *prompt, ExtlFn fn)
{
	WEdln *wedln=do_query_edln(frame, handler_yesno,
							   prompt, NULL, NULL, NULL);
	if(wedln!=NULL){
		ExtlFn *fnc=ALLOC(ExtlFn);
		if(fnc==NULL)
			return;
		*fnc=extl_ref_fn(fn);
		wedln->userdata=(char*)fnc;
	}
}


EXTL_EXPORT
void query_commands(WGenFrame *frame)
{
	do_query_edln(frame, handler_commands,
				  "Commands:", NULL, extl_complete_fn, frame);
}


/*}}}*/

