/*
 * ion/ioncore/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>

#include <libtu/map.h>

#include "common.h"
#include "global.h"
#include "readconfig.h"
#include "modules.h"
#include "rootwin.h"
#include "bindmaps.h"


/*EXTL_DOC
 * Enable/disable opaque move/resize mode.
 */
EXTL_EXPORT
void ioncore_set_opaque_resize(bool opaque)
{
    ioncore_g.opaque_resize=opaque;
}


/*EXTL_DOC
 * Set double click delay in milliseconds.
 */
EXTL_EXPORT
void ioncore_set_dblclick_delay(int dd)
{
    ioncore_g.dblclick_delay=(dd<0 ? 0 : dd);
}


/*EXTL_DOC
 * Set keyboard resize mode auto-finish delay in milliseconds.
 */
EXTL_EXPORT
void ioncore_set_resize_delay(int rd)
{
    ioncore_g.resize_delay=(rd<0 ? 0 : rd);
}


/*EXTL_DOC
 * Enable/disable warping pointer to be contained in activated region.
 */
EXTL_EXPORT
void ioncore_set_warp(bool warp)
{
    ioncore_g.warp_enabled=warp;
}


/*EXTL_DOC
 * Should newly created client window be switched to immediately or
 * should the active window retain focus by default?
 */
EXTL_EXPORT
void ioncore_set_switchto(bool sw)
{
    ioncore_g.switchto_new=sw;
}


bool ioncore_read_main_config(const char *cfgfile)
{
    bool ret;
    int unset=0;

    if(cfgfile==NULL)
        cfgfile="ioncore";
    
    ret=ioncore_read_config(cfgfile, ".", TRUE);
    
    unset+=(ioncore_rootwin_bindmap->nbindings==0);
    unset+=(ioncore_mplex_bindmap->nbindings==0);
    unset+=(ioncore_frame_bindmap->nbindings==0);
    
    if(unset>0){
        warn("Some bindmaps were empty, loading ioncore-efbb");
        ioncore_read_config("ioncore-efbb", NULL, TRUE);
    }
    
    return (ret && unset==0);
}
