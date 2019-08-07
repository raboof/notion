/*
 * notion/ioncore/tempdir.h
 *
 * Copyright (c) 2019 Moritz Wilhelmy
 *
 * See the included file LICENSE for details.
 */

#ifndef NOTION_IONCORE_TEMPDIR_H
#define NOTION_IONCORE_TEMPDIR_H

#include <libextl/extl.h>

extern const char *ioncore_tempdir();
extern void ioncore_tempdir_cleanup(void);

#endif /* NOTION_IONCORE_TEMPDIR_H */
