/*
 * ion/eventh.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "common.h"
#include "global.h"
#include "screen.h"
#include "property.h"
#include "pointer.h"
#include "key.h"
#include "focus.h"
#include "cursor.h"
#include "signal.h"
#include "draw.h"
#include "selection.h"
#include "thing.h"
#include "event.h"
#include "eventh.h"
#include "clientwin.h"
#include "colormap.h"
#include "objp.h"
#include "defer.h"
#include "grab.h"
#include "regbind.h"


/*{{{ Hooks */


WHooklist *handle_event_alt=NULL;


/*}}}*/


/*{{{ Prototypes */


static void handle_expose(const XExposeEvent *ev);
static void handle_map_request(const XMapRequestEvent *ev);
static void handle_configure_request(XConfigureRequestEvent *ev);
static void handle_enter_window(XEvent *ev);
static void handle_unmap_notify(const XUnmapEvent *ev);
static void handle_destroy_notify(const XDestroyWindowEvent *ev);
static void handle_client_message(const XClientMessageEvent *ev);
static void handle_focus_in(const XFocusChangeEvent *ev);
static void handle_focus_out(const XFocusChangeEvent *ev);
static void handle_property(const XPropertyEvent *ev);
static void handle_colormap_notify(const XColormapEvent *ev);
static void pointer_handler(XEvent *ev);
static void keyboard_handler(XEvent *ev);


/*}}}*/


#define CASE_EVENT(X) case X:
/*	fprintf(stderr, "[%#lx] %s\n", ev->xany.window, #X);*/


static void skip_focusenter()
{
	XEvent ev;
	WRegion *r;
	
	XSync(wglobal.dpy, False);
	
	while(XCheckMaskEvent(wglobal.dpy,
						  EnterWindowMask|FocusChangeMask, &ev)){
		if(ev.type==FocusOut)
			handle_focus_out(&(ev.xfocus));
		else if(ev.type==FocusIn)
			handle_focus_in(&(ev.xfocus));
		/*else if(ev.type==EnterNotify)
			handle_enter_window(&ev);*/
	}
}


bool handle_event_default(XEvent *ev)
{
	
	switch(ev->type){
	CASE_EVENT(MapRequest)
		handle_map_request(&(ev->xmaprequest));
		break;
	CASE_EVENT(ConfigureRequest)
		handle_configure_request(&(ev->xconfigurerequest));
		break;
	CASE_EVENT(UnmapNotify)
		handle_unmap_notify(&(ev->xunmap));
		break;
	CASE_EVENT(DestroyNotify)
		handle_destroy_notify(&(ev->xdestroywindow));
		break;
	CASE_EVENT(ClientMessage)
		handle_client_message(&(ev->xclient));
		break;
	CASE_EVENT(PropertyNotify)
		handle_property(&(ev->xproperty));
		break;
	CASE_EVENT(FocusIn)
		handle_focus_in(&(ev->xfocus));
		break;
	CASE_EVENT(FocusOut)
		handle_focus_out(&(ev->xfocus));
		break;
	CASE_EVENT(EnterNotify)
		handle_enter_window(ev);
		break;
	CASE_EVENT(Expose)		
		handle_expose(&(ev->xexpose));
		break;
	CASE_EVENT(KeyPress)
		keyboard_handler(ev);
		break;
	CASE_EVENT(KeyRelease)
		keyboard_handler(ev);
		break;
	CASE_EVENT(ButtonPress)
		pointer_handler(ev);
		break;
	CASE_EVENT(ColormapNotify)
		handle_colormap_notify(&(ev->xcolormap));
		break;
	CASE_EVENT(MappingNotify)
		XRefreshKeyboardMapping(&(ev->xmapping));
		update_modmap();
		break;
	CASE_EVENT(SelectionClear)
		clear_selection();
		break;
	CASE_EVENT(SelectionNotify)
		receive_selection(&(ev->xselection));
		break;
	CASE_EVENT(SelectionRequest)
		send_selection(&(ev->xselectionrequest));
		break;
	}
	
	return TRUE;
}


void mainloop()
{
	XEvent ev;

	wglobal.opmode=OPMODE_NORMAL;

	/* Need to force REGION_ACTIVE on some screen -- temporary kludge */
	XSetInputFocus(wglobal.dpy, wglobal.screens->root.win, PointerRoot,
				   CurrentTime);
	((WRegion*)wglobal.screens)->flags|=REGION_ACTIVE;
	wglobal.active_screen=wglobal.screens;
	
	
	for(;;){
		get_event(&ev);
		
		CALL_ALT_B_NORET(handle_event_alt, (&ev));

		execute_deferred();
		
		XSync(wglobal.dpy, False);
		if(wglobal.focus_next!=NULL && wglobal.input_mode==INPUT_NORMAL){
			skip_focusenter();
			do_set_focus(wglobal.focus_next, wglobal.warp_next);
			wglobal.focus_next=NULL;
			wglobal.warp_next=FALSE;
		}else if(wglobal.grab_released){
			skip_focusenter();
		}

		wglobal.grab_released=FALSE;
	}
}


/*}}}*/


/*{{{ Map, unmap, destroy */


static void handle_map_request(const XMapRequestEvent *ev)
{
	WThing *thing;
	WScreen *scr;
	
	thing=FIND_WINDOW(ev->window);
	
	if(thing!=NULL)
		return;
	
	manage_clientwin(ev->window, 0);
}


static void handle_unmap_notify(const XUnmapEvent *ev)
{
	WClientWin *cwin;

	/* We are not interested in SubstructureNotify -unmaps. */
	if(ev->event!=ev->window && ev->send_event!=True)
		return;

	cwin=find_clientwin(ev->window);
	
	if(cwin!=NULL)
		clientwin_unmapped(cwin);
}


static void handle_destroy_notify(const XDestroyWindowEvent *ev)
{
	WClientWin *cwin;

	cwin=find_clientwin(ev->window);
	
	if(cwin!=NULL)
		clientwin_destroyed(cwin);
}


/*}}}*/


/*{{{ Client configure/property/message */


static void handle_configure_request(XConfigureRequestEvent *ev)
{
	WClientWin *cwin;
	XWindowChanges wc;

	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL){
		wc.border_width=ev->border_width;
		wc.sibling=None;
		wc.stack_mode=ev->detail;
		wc.x=ev->x;
		wc.y=ev->y;
		wc.width=ev->width;
		wc.height=ev->height;
		XConfigureWindow(wglobal.dpy, ev->window, ev->value_mask, &wc);
		return;
	}
	
	clientwin_handle_configure_request(cwin, ev);
}


static void handle_client_message(const XClientMessageEvent *ev)
{
#if 0
	WClientWin *cwin;

	if(ev->message_type!=wglobal.atom_wm_change_state)
		return;
	
	cwin=find_clientwin(ev->window);

	if(cwin==NULL)
		return;
	
	if(ev->format==32 && ev->data.l[0]==IconicState){
		if(cwin->state==NormalState)
			iconify_clientwin(cwin);
	}
#endif
}


static void handle_property(const XPropertyEvent *ev)
{
	WClientWin *cwin;
	
	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL)
		return;
	
	switch(ev->atom){
	case XA_WM_NORMAL_HINTS:
		get_clientwin_size_hints(cwin);
		/*refit(cwin);*/
		return;
	
	case XA_WM_NAME:
		clientwin_get_set_name(cwin);
		break;
		
	case XA_WM_ICON_NAME:
		/* Not used */
		break;

	case XA_WM_TRANSIENT_FOR:
		warn("Changes in WM_TRANSIENT_FOR property are unsupported.");
		/*unmap_clientwin(cwin);
		manage_clientwin(ev->window, 0);*/
		break;
		
	default:
		if(ev->atom==wglobal.atom_wm_protocols){
			get_protocols(cwin);
			return;
		}
		return;
	}
}


/*}}}*/


/*{{{ Colormap */


static void handle_colormap_notify(const XColormapEvent *ev)
{
	WClientWin *cwin;
	
	if(!ev->new)
		return;

	cwin=find_clientwin(ev->window);

	if(cwin!=NULL){
		handle_cwin_cmap(cwin, ev);
		/*set_cmap(cwin, ev->colormap);*/
	}else{
		handle_all_cmaps(ev);
	}
}


/*}}}*/


/*{{{ Expose */


static void handle_expose(const XExposeEvent *ev)
{
	WWindow *wwin;
	WScreen *scr;
	XEvent tmp;
	
	while(XCheckWindowEvent(wglobal.dpy, ev->window, ExposureMask, &tmp))
		/* nothing */;

	wwin=FIND_WINDOW_T(ev->window, WWindow);

	if(wwin!=NULL){
		draw_window(wwin, FALSE);
		return;
	}
	
	/* TODO: drawlist */
	FOR_ALL_SCREENS(scr){
		if(scr->grdata.drag_win==ev->window && wglobal.draw_dragwin!=NULL){
			wglobal.draw_dragwin(wglobal.draw_dragwin_arg);
			break;
		}
	}
}


/*}}}*/


/*{{{ Enter window, focus */


static void handle_enter_window(XEvent *ev)
{
	XEnterWindowEvent *eev=&(ev->xcrossing);
	WRegion *reg=NULL, *freg=NULL, *mgr=NULL;
	bool more=TRUE;
	
	if(wglobal.input_mode!=INPUT_NORMAL)
		return;

	do{
		/*if(eev->mode!=NotifyNormal && !wglobal.warp_enabled)
            return;*/
		/*if(eev->detail==NotifyNonlinearVirtual)
		    return;*/

		reg=FIND_WINDOW_T(eev->window, WRegion);
		
		if(reg==NULL)
			continue;
		
		D(fprintf(stderr, "E: %p %s %d %d\n", reg, WOBJ_TYPESTR(reg),
				  eev->mode, eev->detail));
		
		/* If an EnterWindow event was already found that we're going to
		 * handle, only notify subsequent events if they are into children
		 * of the window of this event.
		 */
		if(freg!=NULL){
			WRegion *r2=reg;
			while(r2!=NULL){
				if(r2==freg)
					break;
				r2=FIND_PARENT1(r2, WRegion);
			}
			if(r2==NULL)
				continue;
		}
		
		
		if(!REGION_IS_ACTIVE(reg))
			freg=reg;
	}while(freg!=NULL && XCheckMaskEvent(wglobal.dpy, EnterWindowMask, ev));
	
	/* Does the manager of the region want to handle focusing?
	 */
	mgr=reg;
	while(1){
		mgr=REGION_MANAGER(mgr);
		if(mgr==NULL)
			break;
		if(mgr->flags&REGION_HANDLES_MANAGED_ENTER_FOCUS)
			reg=mgr;
	}

	set_previous_of(reg);
	set_focus(reg);
}


static void handle_focus_in(const XFocusChangeEvent *ev)
{
	WRegion *reg;
	WWindow *wwin, *tmp;
	WScreen *scr;
	Colormap cmap=None;

	reg=FIND_WINDOW_T(ev->window, WRegion);
	
	if(reg==NULL)
		return;

	D(fprintf(stderr, "FI: %s %p %d %d\n", WOBJ_TYPESTR(reg), reg, ev->mode, ev->detail);)

    if(ev->mode==NotifyGrab)
		return;
	
	if(wglobal.focus_next!=NULL){
		WRegion *r2=reg;
		while(r2!=NULL){
			if(r2==wglobal.focus_next)
				break;
			r2=FIND_PARENT1(r2, WRegion);
		}
		if(r2==NULL)
			return;
	}
	
	if(WTHING_IS(reg, WScreen)){
		if(ev->detail!=NotifyInferior){
			/* Restore focus */
			set_focus(reg);
		}
		return;
	}

	/* Input contexts */
	if(WTHING_IS(reg, WWindow)){
		wwin=(WWindow*)reg;
		if(wwin->xic!=NULL)
			XSetICFocus(wwin->xic);
	}
	
	/*if(ev->detail!=NotifyInferior)*/{
		region_got_focus(reg);
	}
}


static void handle_focus_out(const XFocusChangeEvent *ev)
{
	WRegion *reg;
	WWindow *wwin;
	
	reg=FIND_WINDOW_T(ev->window, WRegion);
	
	if(reg==NULL)
		return;

	D(fprintf(stderr, "FO: %s %p %d %d\n", WOBJ_TYPESTR(reg), reg, ev->mode, ev->detail);)

	if(ev->mode==NotifyGrab)
		return;

	if(WTHING_IS(reg, WWindow)){
		wwin=(WWindow*)reg;
		if(wwin->xic!=NULL)
			XUnsetICFocus(wwin->xic);
	}
	
	if(ev->detail!=NotifyInferior)
		region_lost_focus(reg);
	else
		region_got_focus(reg);
}


/*}}}*/


/*{{{ Pointer, keyboard */


#define GRAB_EV_MASK (GRAB_POINTER_MASK|ExposureMask|  \
					  KeyPressMask|KeyReleaseMask|     \
					  EnterWindowMask|FocusChangeMask)
	
static void pointer_handler(XEvent *ev)
{
	XEvent tmp;
	Window win_pressed;
	WThing *t;
	bool mouse_grab_held=FALSE;

	if(grab_held())
		return;

	win_pressed=ev->xbutton.window;
	
	if(!handle_button_press(&(ev->xbutton)))
		return;

	mouse_grab_held=TRUE;

	while(mouse_grab_held){
		XFlush(wglobal.dpy);
		get_event_mask(ev, GRAB_EV_MASK);
		
		switch(ev->type){
		CASE_EVENT(ButtonRelease)
			if(handle_button_release(&ev->xbutton)){
				ungrab_kb_ptr();
				wglobal.grab_released=FALSE;
				/* Don't ignore following UngrabNotify EnterWindow events in
				 * case of pointer action originated grabs.
				 */
				mouse_grab_held=FALSE;
			}
			break;
		CASE_EVENT(MotionNotify)
			handle_pointer_motion(&ev->xmotion);
			break;
		CASE_EVENT(Expose)
			handle_expose(&(ev->xexpose));
			break;
		CASE_EVENT(KeyPress)
		CASE_EVENT(KeyRelease)
			call_grab_handler(ev);
			break;
		CASE_EVENT(FocusIn)
			handle_focus_in(&(ev->xfocus));
			break;
		CASE_EVENT(FocusOut)
			handle_focus_out(&(ev->xfocus));
			break;
		}
	}
}


static void keyboard_handler(XEvent *ev)
{
	if(call_grab_handler(ev))
		return;
	
	if(ev->type==KeyPress)
		handle_keypress(&(ev->xkey));
}


/*}}}*/


