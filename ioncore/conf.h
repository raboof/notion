/*
 * ion/ioncore/conf.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_CONF_H
#define ION_IONCORE_CONF_H

extern bool ioncore_read_main_config(const char *cfgfile);

extern ExtlTab ioncore_get_winprop(WClientWin *cwin);
extern ExtlTab ioncore_get_layout(const char *str);

#endif /* ION_IONCORE_CONF_H */
