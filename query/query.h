/*
 * query/query.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_QUERY_H
#define QUERY_QUERY_H

#include <wmcore/common.h>
#include <src/frame.h>

extern void query_exec(WFrame *frame);
extern void query_runwith(WFrame *frame, char *cmd, char *prompt);
extern void query_runfile(WFrame *frame, char *cmd, char *prompt);
extern void query_attachclient(WFrame *frame);
extern void query_gotoclient(WFrame *frame);
extern void query_workspace(WFrame *frame);
extern void query_workspace_with(WFrame *frame);
extern void query_yesno(WFrame *frame, char *fn, char *prompt);
extern void query_function(WFrame *frame);
extern void query_renameworkspace(WFrame *frame);

#endif /* QUERY_QUERY_H */
