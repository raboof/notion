/*
 * ion/ioncore/conf.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_CONF_H
#define ION_IONCORE_CONF_H

extern bool ioncore_read_main_config(const char *cfgfile);

extern ExtlTab ioncore_get_winprop(WClientWin *cwin);
extern ExtlTab ioncore_get_layout(const char *str);

#endif /* ION_IONCORE_CONF_H */
