/*
 * ion/query.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <limits.h>
#include <unistd.h>
#include <string.h>

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/function.h>
#include <wmcore/exec.h>
#include <wmcore/clientwin.h>
#include <wmcore/focus.h>
#include <src/frame.h>
#include <src/workspace.h>
#include <src/funtabs.h>
#include "query.h"
#include "wedln.h"
#include "complete_file.h"
#include "wmessage.h"
#include "fwarn.h"

#define FWARN(ARGS) fwarn_free((WFrame*)thing, errmsg ARGS)


/*{{{ Generic */


static WEdln *do_query_edln(WFrame *frame, WEdlnHandler *handler,
							const char *prompt, const char *dflt,
							EdlnCompletionHandler *chnd)
{
	WRectangle geom;
	WEdln *wedln;
	WEdlnCreateParams fnp;
	
	fnp.prompt=prompt;
	fnp.dflt=dflt;
	fnp.handler=handler;
	fnp.chandler=chnd;
	
	
	wedln=(WEdln*)frame_attach_input_new(frame,
										 (WRegionCreateFn*)create_wedln,
										 (void*)&fnp);
	if(wedln!=NULL)
		map_region((WRegion*)wedln);

	if(REGION_IS_ACTIVE(frame)){
		set_focus((WRegion*)wedln);
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

	
static void handler_runfile(WThing *thing, char *str, char *userdata)
{
	char *p;
	
	if(*str=='\0')
		return;
	
	if(userdata!=NULL)
		do_open_with(SCREEN_OF(thing), userdata, str);
	
	p=strrchr(str, '/');
	if(p==NULL){
		wdstatus=0;
	}else{
		*(p+1)='\0';
		strncpy(wdbuf, str, PATH_MAX);
	}
}


static void handler_runwith(WThing *thing, char *str, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	
	if(userdata==NULL)
		return;
	
	if(*str!='\0')
		do_open_with(scr, userdata, str);
	else
		wm_exec(scr, userdata);
}


static void handler_exec(WThing *thing, char *str, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	
	if(*str==':')
		do_open_with(scr, "ion-runinxterm", str+1);
	else
		wm_exec(scr, str);
}


void query_exec(WFrame *frame)
{
	do_query_edln(frame, handler_exec, "Run:", NULL, complete_file_with_path);
}


void query_runfile(WFrame *frame, char *prompt, char *cmd)
{
	WEdln *wedln=do_query_edln(frame, handler_runfile,
							   prompt, my_getwd(), complete_file);
	if(wedln!=NULL)
		wedln->userdata=scopy(cmd);
}


void query_runwith(WFrame *frame, char *prompt, char *cmd)
{
	WEdln *wedln=do_query_edln(frame, handler_runwith,
							   prompt, NULL, NULL);
	if(wedln!=NULL)
		wedln->userdata=scopy(cmd);
}


/*}}}*/


/*{{{ Navigation */


static bool attach_test(WFrame *dst, WRegion *sub, WFrame *thing)
{
	if(!same_screen(&dst->win.region, sub)){
		/* complaint should go in 'thing' -frame */
		FWARN(("Cannot attach: not on same screen."));
		return FALSE;
	}
	frame_attach_sub(dst, sub, TRUE);
	return TRUE;
}


static void handler_attachclient(WThing *thing, char *str, char *userdata)
{
	WClientWin *cwin=lookup_clientwin(str);
	
	if(cwin==NULL){
		FWARN(("No client named '%s'", str));
		return;
	}
	
	attach_test((WFrame*)thing, (WRegion*)cwin, (WFrame*)thing);
}


static void handler_gotoclient(WThing *thing, char *str, char *userdata)
{
	WClientWin *cwin=lookup_clientwin(str);
	
	if(cwin==NULL){
		FWARN(("No client named '%s'", str));
		return;
	}
	
	goto_region(&cwin->region);
}


void query_attachclient(WFrame *frame)
{
	do_query_edln(frame, handler_attachclient,
				  "Attach client:", "", complete_clientwin);
}


void query_gotoclient(WFrame *frame)
{
	do_query_edln(frame, handler_gotoclient,
				  "Goto client:", "", complete_clientwin);
}


bool empty_name(const char *p)
{
	return (strspn(p, " \t")==strlen(p));
}


static void handler_workspace(WThing *thing, char *name, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	WWorkspace *ws;
	
	if(empty_name(name))
		return;
	
	ws=lookup_workspace(name);
	
	if(ws==NULL){
		ws=create_new_workspace_on_scr(scr, name);
		if(ws==NULL){
			FWARN(("Unable to create workspace."));
			return;
		}
	}
	
	goto_region((WRegion*)ws);
}
		
		
void query_workspace(WFrame *frame)
{
	do_query_edln(frame, handler_workspace,
				  "Goto/create workspace:", "", complete_workspace);
}


static void handler_workspace_with(WThing *thing, char *name, char *userdata)
{
	WScreen *scr=SCREEN_OF(thing);
	WWorkspace *ws;
	WClientWin *cwin;
	WFrame *frame;
	
	if(empty_name(name))
		return;
	
	ws=lookup_workspace(name);
	cwin=lookup_clientwin(userdata);
	
	if(ws!=NULL){
		frame=(WFrame*)workspace_find_current(ws);
		if(frame==NULL || !WTHING_IS(frame, WFrame)){
			FWARN(("Workspace %s has no current frame", name));
			return;
		}
	}else{
		if(cwin==NULL){
			FWARN(("Client disappeared"));
			return;
		}
	
		ws=create_new_workspace_on_scr(scr, name);

		if(ws==NULL){
			FWARN(("Unable to create workspace."));
			return;
		}
	
		frame=FIRST_THING(ws, WFrame);
	
		assert(frame!=NULL);
	}
	
	if(attach_test((WFrame*)frame, (WRegion*)cwin, (WFrame*)thing))
		goto_region((WRegion*)cwin);
}


void query_workspace_with(WFrame *frame)
{
	WEdln *wedln;
	WRegion *sub=frame->current_sub;
	char *p;
	
	if(sub==NULL){
		query_workspace(frame);
		return;
	}
	
	p=region_full_name(sub);
	
	wedln=do_query_edln(frame, handler_workspace_with,
						"Create workspace/attach:", p, complete_workspace);
	if(wedln==NULL)
		free(p);
	else
		wedln->userdata=p;
}


void handler_renameworkspace(WThing *thing, char *name, char *userdata)
{
	WWorkspace *ws=FIND_PARENT(thing, WWorkspace);

	if(ws==NULL || empty_name(name))
		return;

	set_region_name((WRegion*)ws, name);
}


void query_renameworkspace(WFrame *frame)
{
	WWorkspace *ws=FIND_PARENT(frame, WWorkspace);
	
	if(ws==NULL)
		return;
	
	do_query_edln(frame, handler_renameworkspace,
				  "Rename workspace to:", region_name((WRegion*)ws),
				  NULL);
}


void handler_renameframe(WThing *thing, char *name, char *userdata)
{
	set_region_name((WRegion*)thing, name);
}


void query_renameframe(WFrame *frame)
{
	do_query_edln(frame, handler_renameframe,
				  "Name of this frame:", region_name((WRegion*)frame),
				  NULL);
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

	
void handler_function(WThing *thing, char *fn, char *userdata)
{
	WarnHandler *old_warn_handler;
	Tokenizer *tokz;
	WWatch watch=WWATCH_INIT;
	bool error;
	
	setup_watch(&watch, thing, NULL);
	
	old_warn_handler=set_warn_handler(function_warn_handler);
	error=!command_sequence(thing, fn);
	set_warn_handler(old_warn_handler);
	
	if(watch.thing!=NULL){
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


static void handler_yesno(WThing *thing, char *yesno, char *fn)
{
	if(strcasecmp(yesno, "y") && strcasecmp(yesno, "yes"))
		return;
	
	handler_function(thing, fn, NULL);
}


void query_yesno(WFrame *frame, char *prompt, char *fn)
{
	WEdln *wedln=do_query_edln(frame, handler_yesno,
							   prompt, NULL, NULL);
	if(wedln!=NULL)
		wedln->userdata=scopy(fn);
}


static int complete_mainfunc(char *nam, char ***cp_ret,
								   char **beg)
{
	return complete_func_ex(nam, cp_ret, beg, &ion_frame_funclist);
}


void query_function(WFrame *frame)
{
	do_query_edln(frame, handler_function,
				  "Function name:", NULL, complete_mainfunc);
}


/*}}}*/

